[CmdletBinding()]
param(
    [string]$GameRoot,
    [string]$PackageRoot=(Split-Path -Parent $PSScriptRoot),
    [string]$LogRoot=(Join-Path $env:LOCALAPPDATA 'Stormslinger\logs'),
    [string]$OutputDirectory=(Join-Path $env:LOCALAPPDATA 'Stormslinger\feedback'),
    [switch]$NonInteractive
)
$ErrorActionPreference='Stop'
$manifestPath=Join-Path $PackageRoot 'release-manifest.json'
$manifest=Get-Content -LiteralPath $manifestPath -Raw -Encoding UTF8 | ConvertFrom-Json
if($manifest.variant -ne 'standalone'){throw 'This feedback tool is for the native standalone package.'}
if(-not $GameRoot){
    $receipt=Join-Path $PackageRoot 'feedback-location.json'
    if(Test-Path -LiteralPath $receipt){
        try{$GameRoot=(Get-Content -LiteralPath $receipt -Raw -Encoding UTF8 | ConvertFrom-Json).game_root}catch{}
    }
}
if(-not $GameRoot -and -not $NonInteractive){$GameRoot=Read-Host 'Enter game folder, or press Enter to export logs only'}
$hasGame=$GameRoot -and (Test-Path -LiteralPath $GameRoot -PathType Container)
if($hasGame){$GameRoot=[IO.Path]::GetFullPath($GameRoot).TrimEnd('\','/')}
[IO.Directory]::CreateDirectory($OutputDirectory)|Out-Null
$outputBase=[IO.Path]::GetFullPath($OutputDirectory).TrimEnd('\','/')
$token=[Guid]::NewGuid().ToString('N')
$temporary=Join-Path $outputBase ('feedback-tmp-'+$token)
[IO.Directory]::CreateDirectory($temporary)|Out-Null
$errors=[Collections.Generic.List[string]]::new()
function Save-Json([string]$Name,$Data){
    $Data|ConvertTo-Json -Depth 8|Set-Content -LiteralPath (Join-Path $temporary $Name) -Encoding UTF8
}
function File-Sha([string]$File){
    $hash=[Security.Cryptography.SHA256]::Create()
    $stream=[IO.File]::Open($File,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite)
    try{return [BitConverter]::ToString($hash.ComputeHash($stream)).Replace('-','').ToLower()}finally{$stream.Dispose();$hash.Dispose()}
}
function Safe-Fields($Object){
    $result=[ordered]@{}
    $strings=@('utc','version','build','session','event','component')
    $scalars=@('component','detail','native_status','pid','code','stage','mh_status','failure_origin','win32_error','phase_before','phase_after','attempts','group','record','occurrence','previous_marker_present','installed','exit_request_observed','exit_code','status','bootstrap_install_stage','bootstrap_mh_status','bootstrap_failure_origin','bootstrap_win32_error','weapon_tree_enabled','weapon_phase_before','weapon_phase_after','shooting_startup_requested','page','language_index','confirmed','persisted')
    foreach($name in $strings){
        $property=$Object.PSObject.Properties[$name]
        if($property -and $property.Value -is [string] -and $property.Value -match '^[A-Za-z0-9_.:+-]{1,128}$'){$result[$name]=$property.Value}
    }
    foreach($name in $scalars){
        $property=$Object.PSObject.Properties[$name]
        if($property -and ($property.Value -is [bool] -or $property.Value -is [int] -or $property.Value -is [long] -or $property.Value -is [double])){$result[$name]=$property.Value}
    }
    return $result
}
function Safe-Target([string]$Relative){
    if(-not $hasGame){return $null}
    if($Relative -notmatch '^(?i)(dinput8\.dll|nativePC[/\\].+)$' -or $Relative -match '(^|[/\\])\.\.([/\\]|$)|:|^[\\/]' -or $Relative.IndexOf([char]0) -ge 0){throw 'Unsafe manifest path'}
    $target=[IO.Path]::GetFullPath((Join-Path $GameRoot $Relative))
    if(-not $target.StartsWith($GameRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Unsafe manifest path'}
    $cursor=$target
    while($cursor -and $cursor -ne $GameRoot){
        if(Test-Path -LiteralPath $cursor){if((Get-Item -LiteralPath $cursor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Reparse target not collected'}}
        $cursor=Split-Path -Parent $cursor
    }
    return $target
}
try{
    $version=[ordered]@{schema_version=1;variant='standalone';diagnostics=$manifest.diagnostics;expected_game_sha256=$manifest.game_exe_sha256;expected_file_count=@($manifest.files).Count}
    # Only package-controlled diagnostics fields are published, never the receipt.
    $version.diagnostics=[ordered]@{version=$manifest.diagnostics.version;build=$manifest.diagnostics.build;log_file_count=3;max_log_bytes=2097152;collect_saves=$false;collect_dumps=$false}
    Save-Json 'version.json' $version
    $fileRows=[Collections.Generic.List[object]]::new()
    foreach($entry in $manifest.files){
        $actual=$null;$state='game_folder_unavailable'
        try{
            $target=Safe-Target $entry.relative_path
            if($target){
                if(Test-Path -LiteralPath $target -PathType Leaf){$actual=File-Sha $target;$state=if($actual -eq $entry.sha256){'matches'}else{'different'}}else{$state='missing'}
            }
        }catch{Write-Verbose $_.Exception.Message;$state='unreadable_or_unsafe';$errors.Add('payload_check_failed')}
        if($entry.relative_path -match '^(?i)(dinput8\.dll|nativePC[/\\].+)$' -and $entry.relative_path -notmatch ':|(^|[/\\])\.\.([/\\]|$)'){
            $fileRows.Add([ordered]@{relative_path=$entry.relative_path;state=$state;expected_sha256=$entry.sha256;actual_sha256=$actual})
        }
    }
    $gameHash=$null
    if($hasGame){
        $executable=Join-Path $GameRoot 'MonsterHunterWorld.exe'
        if(Test-Path -LiteralPath $executable -PathType Leaf){try{$gameHash=File-Sha $executable}catch{$errors.Add('game_hash_unreadable')}}
    }
    Save-Json 'installation-check.json' ([ordered]@{game_folder_available=[bool]$hasGame;actual_game_sha256=$gameHash;game_executable_matches=($gameHash -eq $manifest.game_exe_sha256);files=@($fileRows.ToArray())})
    $logDestination=Join-Path $temporary 'logs';[IO.Directory]::CreateDirectory($logDestination)|Out-Null
    $logCount=0;$invalidLines=0
    foreach($name in @('runtime.jsonl','runtime.1.jsonl','runtime.2.jsonl')){
        $source=Join-Path $LogRoot $name
        if(Test-Path -LiteralPath $source -PathType Leaf){
            if((Get-Item -LiteralPath $source).Length -gt 2097152){$errors.Add('oversized_log_not_collected');continue}
            $safeLines=[Collections.Generic.List[string]]::new()
            try{
                foreach($line in [IO.File]::ReadLines($source)){
                    try{$safe=Safe-Fields (ConvertFrom-Json -InputObject $line);if($safe.Count){$safeLines.Add(($safe|ConvertTo-Json -Compress -Depth 4))}}catch{$invalidLines++}
                }
                $safeLines|Set-Content -LiteralPath (Join-Path $logDestination $name) -Encoding UTF8
                $logCount++
            }catch{$errors.Add('runtime_log_unreadable')}
        }
    }
    $session=Join-Path $LogRoot 'session.json'
    if(Test-Path -LiteralPath $session -PathType Leaf){
        try{if((Get-Item -LiteralPath $session).Length -le 4096){Save-Json 'session.json' (Safe-Fields (Get-Content -LiteralPath $session -Raw -Encoding UTF8|ConvertFrom-Json))}}catch{$errors.Add('session_record_unreadable')}
    }
    if($hasGame){
        $status=Join-Path $GameRoot 'Stormslinger-development-status.json'
        if(Test-Path -LiteralPath $status -PathType Leaf){
            try{if((Get-Item -LiteralPath $status).Length -le 8192){Save-Json 'runtime-status.json' ([ordered]@{file_utc=(Get-Item -LiteralPath $status).LastWriteTimeUtc.ToString('o');data=(Safe-Fields (Get-Content -LiteralPath $status -Raw -Encoding UTF8|ConvertFrom-Json));note='A status file may be from an earlier run; correlate its time with session logs.'})}}catch{$errors.Add('runtime_status_unreadable')}
        }
    }
    Save-Json 'collection.json' ([ordered]@{collected_utc=[DateTime]::UtcNow.ToString('o');os_version=[Environment]::OSVersion.Version.ToString();process_64bit=[Environment]::Is64BitProcess;runtime_log_count=$logCount;invalid_log_lines=$invalidLines;incomplete_marker_present=(Test-Path -LiteralPath (Join-Path $LogRoot 'running.marker'));errors=@($errors.ToArray());privacy='No saves, player names, full system paths or dumps collected; no upload performed.'})
    @'
Describe what happened, what you expected, and how to reproduce it.
Add a screenshot or video separately if useful. Do not send your save by default.
Missing exit markers are clues, not proof of a crash. Logs do not capture every crash cause.
'@ | Set-Content -LiteralPath (Join-Path $temporary 'REPRODUCTION.txt') -Encoding UTF8
    $zip=Join-Path $outputBase ('Stormslinger-feedback-'+[DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss')+'-'+$token.Substring(0,8)+'.zip')
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    [IO.Compression.ZipFile]::CreateFromDirectory($temporary,$zip)
    Write-Output $zip
}finally{
    $checked=[IO.Path]::GetFullPath($temporary)
    if($checked.StartsWith($outputBase+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase) -and (Split-Path -Leaf $checked) -eq ('feedback-tmp-'+$token)){
        Remove-Item -LiteralPath $checked -Recurse -Force
    }
}
