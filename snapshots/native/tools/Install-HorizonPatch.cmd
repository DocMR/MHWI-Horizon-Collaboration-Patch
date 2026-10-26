@echo off
setlocal
chcp 65001 >nul
set "HORIZON_INSTALLER_SELF=%~f0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "$text=[IO.File]::ReadAllText($env:HORIZON_INSTALLER_SELF,[Text.Encoding]::UTF8); $marker='# HORIZON_POWERSHELL_PAYLOAD'; & ([scriptblock]::Create($text.Substring($text.LastIndexOf($marker)+$marker.Length)))"
exit /b %errorlevel%
# HORIZON_POWERSHELL_PAYLOAD
[CmdletBinding()]
param([string]$ArchivePath,[string]$GameRoot)
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.IO.Compression.FileSystem
$caption='Horizon Collaboration Patch / 地平线联动补丁'
$installationStarted=$false
function FileHash([string]$path){
 $hasher=[Security.Cryptography.SHA256]::Create();$stream=[IO.File]::OpenRead($path)
 try{return [BitConverter]::ToString($hasher.ComputeHash($stream)).Replace('-','').ToLowerInvariant()}
 finally{$stream.Dispose();$hasher.Dispose()}
}
function CheckedTarget([string]$root,[string]$relative){
 if([IO.Path]::IsPathRooted($relative) -or $relative -match '(^|[\\/])\.\.([\\/]|$)|:'){throw 'Unsafe archive path / 压缩包路径不安全'}
 $target=[IO.Path]::GetFullPath((Join-Path $root $relative))
 if(-not $target.StartsWith($root.TrimEnd('\','/')+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'Path outside game directory / 路径超出游戏目录'}
 $walk=$target
 while($walk.Length -ge $root.Length){
  if((Test-Path -LiteralPath $walk) -and ((Get-Item -LiteralPath $walk -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)){throw 'Linked target refused / 不支持链接目标'}
  if($walk -eq $root){break};$walk=Split-Path -Parent $walk
 }
 return $target
}
function SteamGames {
 $roots=@()
 foreach($key in @('HKCU:\Software\Valve\Steam','HKLM:\SOFTWARE\WOW6432Node\Valve\Steam','HKLM:\SOFTWARE\Valve\Steam')){
  if(Test-Path -LiteralPath $key){
   $value=Get-ItemProperty -LiteralPath $key
   foreach($name in @('SteamPath','InstallPath')){
    $property=$value.PSObject.Properties[$name]
    if($property -and $property.Value){$roots+=@([string]$property.Value)}
   }
  }
 }
 if(${env:ProgramFiles(x86)}){$roots+=@(Join-Path ${env:ProgramFiles(x86)} 'Steam')}
 $libraries=@($roots)
 foreach($root in @($roots|Select-Object -Unique)){
  $vdf=Join-Path $root 'steamapps/libraryfolders.vdf'
  if(Test-Path -LiteralPath $vdf){
   $text=Get-Content -LiteralPath $vdf -Raw -Encoding UTF8
   foreach($match in [regex]::Matches($text,'"path"\s+"([^"\r\n]+)"')){$libraries+=@($match.Groups[1].Value.Replace('\\','\'))}
  }
 }
 foreach($library in @($libraries|Select-Object -Unique)){
  $manifest=Join-Path $library 'steamapps/appmanifest_582010.acf'
  if(-not(Test-Path -LiteralPath $manifest -PathType Leaf)){continue}
  $text=Get-Content -LiteralPath $manifest -Raw -Encoding UTF8
  $match=[regex]::Match($text,'"installdir"\s+"([^"\r\n]+)"')
  if($match.Success){
   $candidate=Join-Path (Join-Path $library 'steamapps/common') $match.Groups[1].Value
   if(Test-Path -LiteralPath (Join-Path $candidate 'MonsterHunterWorld.exe') -PathType Leaf){[IO.Path]::GetFullPath($candidate)}
  }
 }
}
function RequireClosed {
 if(Get-CimInstance Win32_Process -Filter "name='MonsterHunterWorld.exe'"){throw 'Close MonsterHunterWorld.exe before installing / 请先关闭游戏再安装'}
}
function CheckInstallTargets($items,[string]$root){
 # Inspect the complete write set before overwriting the first original file.
 # Opening an existing file for write access does not truncate or change it.
 foreach($item in $items){
  $target=CheckedTarget $root $item.entry.FullName.Replace('\','/')
  if(Test-Path -LiteralPath $target){
   $existing=Get-Item -LiteralPath $target -Force
   if($existing.PSIsContainer){throw ('A file target is a directory / 文件目标被文件夹占用: '+$target)}
   if($existing.Attributes -band [IO.FileAttributes]::ReadOnly){throw ('Read-only target; no patch files written / 目标文件只读，尚未写入补丁: '+$target)}
   try{
    $probe=[IO.File]::Open($target,[IO.FileMode]::Open,[IO.FileAccess]::Write,[IO.FileShare]::ReadWrite)
    $probe.Dispose()
   }catch{throw ('Target is locked or not writable; no patch files written / 目标被占用或不可写，尚未写入补丁: '+$target)}
  }
  $parent=Split-Path -Parent $target
  while($parent -and $parent -ne $root){
   if((Test-Path -LiteralPath $parent) -and -not(Test-Path -LiteralPath $parent -PathType Container)){
    throw ('Parent path is a file / 上级目录被文件占用: '+$parent)
   }
   $parent=Split-Path -Parent $parent
  }
 }
}
try {
 if(-not $ArchivePath){
  $installerDirectory=if($env:HORIZON_INSTALLER_SELF){Split-Path -Parent $env:HORIZON_INSTALLER_SELF}else{$PSScriptRoot}
  $ArchivePath=Join-Path $installerDirectory 'Monster Hunter World Iceborne - Horizon Zero Dawn Collaboration Patch.zip'
 }
 $ArchivePath=(Resolve-Path -LiteralPath $ArchivePath).Path
 if(-not $GameRoot){
  $games=@(SteamGames|Select-Object -Unique)
  if($games.Count -eq 1){$GameRoot=$games[0]}
  else{
   $dialog=New-Object Windows.Forms.FolderBrowserDialog
   $dialog.Description='Select the folder containing MonsterHunterWorld.exe / 选择MonsterHunterWorld.exe所在目录'
   if($games.Count -gt 0){$dialog.SelectedPath=$games[0]}
   if($dialog.ShowDialog() -ne [Windows.Forms.DialogResult]::OK){exit 2}
   $GameRoot=$dialog.SelectedPath
  }
 }
 $GameRoot=(Resolve-Path -LiteralPath $GameRoot).Path.TrimEnd('\','/')
 $exe=Join-Path $GameRoot 'MonsterHunterWorld.exe'
 if(-not(Test-Path -LiteralPath $exe -PathType Leaf)){throw 'MonsterHunterWorld.exe not found / 未找到游戏主程序'}
 RequireClosed
 $zip=[IO.Compression.ZipFile]::OpenRead($ArchivePath)
 try {
  $entry=$zip.GetEntry('StormslingerSupport/release-manifest.json')
  if(-not $entry){throw 'Package manifest missing / 缺少补丁清单'}
  $reader=New-Object IO.StreamReader($entry.Open(),[Text.Encoding]::UTF8)
  try{$manifest=$reader.ReadToEnd()|ConvertFrom-Json}finally{$reader.Dispose()}
  if($manifest.variant -ne 'standalone' -or $manifest.distribution_layout -ne 'extract_archive_contents_directly_into_game_root'){throw 'Wrong patch package / 补丁包格式错误'}
  if((FileHash $exe) -ne $manifest.game_exe_sha256){throw 'Unsupported game executable version / 游戏主程序版本不匹配'}
  $expected=@{}
  $allowedAux=@('Export-Feedback.cmd','Uninstall-Patch.cmd','StormslingerSupport/Export-Feedback.ps1','StormslingerSupport/Uninstall-Patch.ps1','StormslingerSupport/LICENSE-minhook.txt')
  foreach($row in @($manifest.files)+@($manifest.auxiliary_files)){
   $relative=$row.relative_path.Replace('\','/')
   if($relative -ne 'dinput8.dll' -and -not $relative.StartsWith('nativePC/') -and $allowedAux -notcontains $relative){throw 'Unexpected install target / 清单包含非补丁安装目标'}
   if($expected.ContainsKey($relative.ToLowerInvariant())){throw 'Duplicate manifest path / 清单路径重复'}
   $expected[$relative.ToLowerInvariant()]=$row
  }
  $expected['stormslingersupport/release-manifest.json']=$null
  $entries=@();$seen=@{}
  foreach($item in $zip.Entries){
   if(-not $item.Name){continue}
   $relative=$item.FullName.Replace('\','/');$key=$relative.ToLowerInvariant()
   if(-not $expected.ContainsKey($key) -or $seen.ContainsKey($key)){throw 'Unexpected or duplicate archive entry / 压缩包存在未登记或重复文件'}
   $target=CheckedTarget $GameRoot $relative
   $seen[$key]=$true;$entries+=@([pscustomobject]@{entry=$item;target=$target;row=$expected[$key]})
   if($expected[$key]){
    $hasher=[Security.Cryptography.SHA256]::Create();$stream=$item.Open()
    try{$hash=[BitConverter]::ToString($hasher.ComputeHash($stream)).Replace('-','').ToLowerInvariant()}finally{$stream.Dispose();$hasher.Dispose()}
    if($hash -ne $expected[$key].sha256 -or $item.Length -ne $expected[$key].size){throw 'Archive file check failed / 压缩包文件校验失败'}
   }
  }
  if($seen.Count -ne $expected.Count){throw 'Archive is incomplete / 压缩包内容不完整'}
  $message="安装目录 / Installation folder:`r`n$GameRoot`r`n`r`n是否已将游戏根目录中原有文件备份到游戏目录外？`r`n本工具会覆盖同名文件，不自动备份。选择是开始解压；选择否或关闭则取消。`r`n`r`nHave you backed up the existing game-root files outside the game folder?`r`nThis tool overwrites matching files and does not create a backup. Choose Yes to extract, or No/close to cancel."
  $answer=[Windows.Forms.MessageBox]::Show($message,$caption,[Windows.Forms.MessageBoxButtons]::YesNo,[Windows.Forms.MessageBoxIcon]::Warning,[Windows.Forms.MessageBoxDefaultButton]::Button2)
  if($answer -ne [Windows.Forms.DialogResult]::Yes){exit 2}
  RequireClosed
  CheckInstallTargets $entries $GameRoot
  RequireClosed
  $installationStarted=$true
  foreach($item in $entries){
   [IO.Directory]::CreateDirectory((Split-Path -Parent $item.target))|Out-Null
   [IO.Compression.ZipFileExtensions]::ExtractToFile($item.entry,$item.target,$true)
   if($item.row -and (FileHash $item.target) -ne $item.row.sha256){throw 'Extracted file check failed / 解压文件校验失败'}
  }
 }finally{$zip.Dispose()}
 # Reset only the content acknowledgement after a successful installation.
 # Logs, feedback archives and saves remain untouched.
 $acknowledgement=Join-Path ([Environment]::GetFolderPath('LocalApplicationData')) 'Stormslinger/horizon-content.version'
 if($env:LOCALAPPDATA){$acknowledgement=Join-Path $env:LOCALAPPDATA 'Stormslinger/horizon-content.version'}
 if(Test-Path -LiteralPath $acknowledgement -PathType Leaf){Remove-Item -LiteralPath $acknowledgement}
 # This directory is not shipped by the patch. Remove only an empty, ordinary
 # directory; preserve DLSS files and linked directories belonging to others.
 $dlss=Join-Path $GameRoot 'DLSS'
 if(Test-Path -LiteralPath $dlss -PathType Container){
  $directory=Get-Item -LiteralPath $dlss -Force
  if(-not($directory.Attributes -band [IO.FileAttributes]::ReparsePoint) -and -not(Get-ChildItem -LiteralPath $dlss -Force|Select-Object -First 1)){
   [IO.Directory]::Delete($dlss,$false)
  }
 }
 [void][Windows.Forms.MessageBox]::Show('安装完成，请手动启动游戏。下次启动将显示追加内容。 / Installation complete. Start the game manually to view the added content notice.',$caption)
}catch{
 $failure='安装停止 / Installation stopped:'+[Environment]::NewLine+$_.Exception.Message
 if($installationStarted){$failure+=[Environment]::NewLine+[Environment]::NewLine+'安装可能只完成了一部分，请勿启动游戏。请先排除故障，再重新安装；如需撤回，请使用安装前备份恢复被覆盖的文件。 / Installation may be incomplete. Do not start the game. Resolve the error and reinstall, or restore overwritten files from your pre-installation backup.'}
 [void][Windows.Forms.MessageBox]::Show($failure,$caption,[Windows.Forms.MessageBoxButtons]::OK,[Windows.Forms.MessageBoxIcon]::Error)
 exit 1
}
