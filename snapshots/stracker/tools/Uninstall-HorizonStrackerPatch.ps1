[CmdletBinding()]
# No pre-uninstall patch backup is created. Original restore still uses the player-provided backup.
param([string]$GameRoot=(Split-Path -Parent $PSScriptRoot),[string]$BackupRoot,[switch]$NonInteractive,[switch]$ViaCommand)
$ErrorActionPreference='Stop'
function RequireClosed {
 if(Get-CimInstance Win32_Process -Filter "name='MonsterHunterWorld.exe'"){throw 'Close MonsterHunterWorld.exe before uninstalling / 请先关闭游戏再卸载'}
}
function Hash([string]$p){
 $h=[Security.Cryptography.SHA256]::Create();$s=[IO.File]::OpenRead($p)
 try{return [BitConverter]::ToString($h.ComputeHash($s)).Replace('-','').ToLower()}finally{$s.Dispose();$h.Dispose()}
}
function Checked([string]$root,[string]$relative){
 if([IO.Path]::IsPathRooted($relative) -or $relative -match '(^|[\\/])\.\.([\\/]|$)|:'){throw 'Unsafe relative path'}
 $p=[IO.Path]::GetFullPath((Join-Path $root $relative))
 if(-not $p.StartsWith($root.TrimEnd('\','/')+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Path outside expected directory'}
 $walk=$p
 while($walk.Length -ge $root.Length){
  if((Test-Path -LiteralPath $walk) -and ((Get-Item -LiteralPath $walk -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)){throw 'Linked path refused'}
  if($walk -eq $root){break};$walk=Split-Path -Parent $walk
 }
 return $p
}
try{
 $GameRoot=(Resolve-Path -LiteralPath $GameRoot).Path.TrimEnd('\','/')
 if(-not(Test-Path -LiteralPath (Join-Path $GameRoot 'MonsterHunterWorld.exe') -PathType Leaf)){throw 'MonsterHunterWorld.exe not found'}
 RequireClosed
 $support=Checked $GameRoot 'StormslingerSupport'
 $manifestPath=Checked $GameRoot 'StormslingerSupport/release-manifest.json'
 $manifest=Get-Content -LiteralPath $manifestPath -Raw -Encoding UTF8|ConvertFrom-Json
 if($manifest.variant -ne 'stracker' -or $manifest.distribution_layout -ne 'extract_archive_contents_directly_into_game_root'){throw 'Wrong package manifest'}
 if(-not $NonInteractive){
  Add-Type -AssemblyName System.Windows.Forms
  $answer=[Windows.Forms.MessageBox]::Show('Uninstall this patch? Saves and unrelated MOD files are kept. Modified patch files are preserved. / 卸载本补丁？保留存档、其它MOD和已变更文件。','Horizon Collaboration Patch / 地平线联动补丁',[Windows.Forms.MessageBoxButtons]::OKCancel)
  if($answer -ne [Windows.Forms.DialogResult]::OK){exit 2}
  if(-not $BackupRoot){
   $restore=[Windows.Forms.MessageBox]::Show('Restore files overwritten at installation from your pre-install backup? / 是否从安装前备份恢复被覆盖的文件？','Horizon Collaboration Patch / 地平线联动补丁',[Windows.Forms.MessageBoxButtons]::YesNo)
   if($restore -eq [Windows.Forms.DialogResult]::Yes){
    $dialog=New-Object Windows.Forms.FolderBrowserDialog;$dialog.Description='Select backup containing nativePC and/or dinput8.dll / 选择包含原nativePC或dinput8.dll的备份目录'
    if($dialog.ShowDialog() -ne [Windows.Forms.DialogResult]::OK){exit 2};$BackupRoot=$dialog.SelectedPath
   }
  }
 }
 if($BackupRoot){
  $BackupRoot=(Resolve-Path -LiteralPath $BackupRoot).Path.TrimEnd('\','/')
  if($BackupRoot -eq $GameRoot -or $BackupRoot.StartsWith($GameRoot+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'Use an original backup outside the game directory'}
  if(-not(Test-Path -LiteralPath (Join-Path $BackupRoot 'nativePC')) -and -not(Test-Path -LiteralPath (Join-Path $BackupRoot 'dinput8.dll'))){throw 'Backup must contain nativePC or dinput8.dll'}
 }
 $allowedAux=@('Export-Feedback.cmd','Uninstall-Patch.cmd','StormslingerSupport/Export-Feedback.ps1','StormslingerSupport/Uninstall-Patch.ps1','StormslingerSupport/LICENSE-minhook.txt','StormslingerSupport/release-manifest.json')
 $rows=@($manifest.files)+@($manifest.auxiliary_files)+@([pscustomobject]@{relative_path='StormslingerSupport/release-manifest.json';sha256=(Hash $manifestPath)})
 $seen=@{};$operations=@();$parents=@{}
 foreach($row in $rows){
  $rel=$row.relative_path.Replace('\','/')
  if(-not $rel.StartsWith('nativePC/') -and $allowedAux -notcontains $rel){throw 'Unexpected owned file'}
  if($seen.ContainsKey($rel.ToLower())){throw 'Duplicate owned file'};$seen[$rel.ToLower()]=$true
  $target=Checked $GameRoot $rel
  $state='missing';if(Test-Path -LiteralPath $target -PathType Leaf){$state=if((Hash $target) -eq $row.sha256){'remove'}else{'preserve_modified'}}
  $original=$null
  if($BackupRoot -and $state -ne 'preserve_modified'){
   $candidate=Checked $BackupRoot $rel;if(Test-Path -LiteralPath $candidate -PathType Leaf){$original=$candidate}
  }
  $operations+=@([pscustomobject]@{relative_path=$rel;target=$target;state=$state;original=$original})
  $parent=Split-Path -Parent $target
  while($parent -and $parent -ne $GameRoot){$parents[$parent]=$true;$parent=Split-Path -Parent $parent}
 }
 if(@($operations|Where-Object {$_.state -eq 'preserve_modified'}).Count){
  foreach($op in $operations){if($allowedAux -contains $op.relative_path -and $op.state -eq 'remove'){$op.state='keep_support';$op.original=$null}}
 }
 $result=@()
 # Dialogs and backup selection may have taken time. Check again immediately
 # before the first deletion/restoration, after every interactive prompt.
 RequireClosed
 foreach($op in $operations){
  if($op.state -eq 'remove' -and -not($ViaCommand -and $op.relative_path -eq 'Uninstall-Patch.cmd')){Remove-Item -LiteralPath $op.target -Force}
  if($op.original){
   [IO.Directory]::CreateDirectory((Split-Path -Parent $op.target))|Out-Null
   Copy-Item -LiteralPath $op.original -Destination $op.target
   if((Hash $op.target) -ne (Hash $op.original)){throw 'Original restore mismatch'}
   $op.state='original_restored'
  }
  $result+=@([ordered]@{relative_path=$op.relative_path;result=$op.state})
 }
 $preserved=@($result|Where-Object {$_.result -eq 'preserve_modified'}).Count
 # The manifest is handled like other owned files above, including restoration
 # of an older original from the player's backup. Do not delete it again here.
 foreach($parent in @($parents.Keys|Sort-Object Length -Descending)){
  $checked=Checked $GameRoot $parent.Substring($GameRoot.Length).TrimStart('\')
  if((Test-Path -LiteralPath $checked -PathType Container) -and -not(Get-ChildItem -LiteralPath $checked -Force)){Remove-Item -LiteralPath $checked -Force}
 }
 [ordered]@{status='uninstall_finished';preserved_modified_files=$preserved;original_backup_used=[bool]$BackupRoot;files=$result;saves_touched=$false}|ConvertTo-Json -Depth 6|Write-Output
 Write-Host "Finished. Modified files preserved: $preserved."
 if(-not $NonInteractive){[void](Read-Host 'Press Enter to close / 按回车关闭')}
 if($preserved -gt 0){exit 2}
 # The CMD wrapper self-deletes only on exit0. Retain an original CMD restored
 # from backup instead of deleting that original after PowerShell returns.
 if($ViaCommand -and @($result|Where-Object {$_.relative_path -eq 'Uninstall-Patch.cmd' -and $_.result -eq 'original_restored'}).Count){exit 3}
}catch{
 Write-Host ('Uninstall stopped: '+$_.Exception.Message) -ForegroundColor Red
 if(-not $NonInteractive){[void](Read-Host 'Press Enter to close / 按回车关闭')}
 exit 1
}
