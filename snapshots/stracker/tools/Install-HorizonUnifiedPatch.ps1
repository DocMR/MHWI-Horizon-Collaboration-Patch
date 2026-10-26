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
  $directory=if($env:HORIZON_INSTALLER_SELF){Split-Path -Parent $env:HORIZON_INSTALLER_SELF}else{$PSScriptRoot}
  $ArchivePath=Join-Path $directory 'Monster Hunter World Iceborne - Horizon Zero Dawn Collaboration Patch - Unified Edition.zip'
 }
 $ArchivePath=(Resolve-Path -LiteralPath $ArchivePath).Path
 if(-not $GameRoot){
  $games=@(SteamGames|Select-Object -Unique)
  if($games.Count -eq 1){$GameRoot=$games[0]}else{
   $dialog=New-Object Windows.Forms.FolderBrowserDialog
   $dialog.Description='Select MonsterHunterWorld.exe folder / 选择MonsterHunterWorld.exe所在目录'
   if($dialog.ShowDialog() -ne [Windows.Forms.DialogResult]::OK){exit 2}
   $GameRoot=$dialog.SelectedPath
  }
 }
 $GameRoot=(Resolve-Path -LiteralPath $GameRoot).Path.TrimEnd('\','/')
 if(-not(Test-Path -LiteralPath (CheckedTarget $GameRoot 'MonsterHunterWorld.exe') -PathType Leaf)){throw 'MonsterHunterWorld.exe not found / 未找到游戏主程序'}
 RequireClosed
 $help='检测到不受支持、混装或不完整的加载器，尚未写入补丁。请先备份并按原作者说明卸载冲突加载器；若需要插件版，请从 https://www.nexusmods.com/monsterhunterworld/mods/1982?tab=files 安装受支持的 Stracker''s Loader 4.0 完整文件，并在loader-config.json中设置enablePluginLoader为true。已有其它MOD时请勿直接删除nativePC。 / Unsupported, mixed or incomplete loader detected; no patch files written. Back up and uninstall conflicting loaders following their authors'' instructions. For plugin mode, install the supported complete Stracker''s Loader 4.0 from https://www.nexusmods.com/monsterhunterworld/mods/1982?tab=files and set enablePluginLoader=true. Do not delete nativePC containing other mods.'
 # Check known alternate proxy/plugin entry locations; arbitrary renamed injectors
 # cannot be identified reliably by a file-only installer.
 foreach($name in @('winmm.dll','dxgi.dll','d3d11.dll','d3d9.dll','version.dll','dsound.dll','winhttp.dll','dinput.dll','xinput1_3.dll','xinput9_1_0.dll','dtdata.dll','d3d12.dll')){
  if(Test-Path -LiteralPath (CheckedTarget $GameRoot $name)){throw ($help+' / '+$name)}
 }
 if(@(Get-ChildItem -LiteralPath $GameRoot -Filter '*.asi' -File).Count){throw ($help+' / ASI plugins')}
 $proxy=CheckedTarget $GameRoot 'dinput8.dll'
 $loader=CheckedTarget $GameRoot 'loader.dll'
 $configPath=CheckedTarget $GameRoot 'loader-config.json'
 $plugin=CheckedTarget $GameRoot 'nativePC/plugins/HorizonCollaboration.dll'
 $variant='standalone'
 if(Test-Path -LiteralPath $proxy){
  $hash=FileHash $proxy
  if($hash -eq 'ecdb41433e2f4bf48f33f19c457eebe8af26466f09313a89f2ea6659d9b271eb'){
   if((Test-Path -LiteralPath $loader) -or (Test-Path -LiteralPath $configPath) -or (Test-Path -LiteralPath $plugin)){throw $help}
  }elseif($hash -eq '6e38baff0bddc5014046e3ba5a733814f95f65d5ca67e2fb15d18c5106d4e059'){
   if(-not(Test-Path -LiteralPath $loader -PathType Leaf) -or (FileHash $loader) -ne '17ec93d9d57809e4968666961caf996f7d819c05b280fbb6d444b95920a801ee'){throw $help}
   if(-not(Test-Path -LiteralPath $configPath -PathType Leaf)){throw $help}
   try{$config=Get-Content -LiteralPath $configPath -Raw -Encoding UTF8|ConvertFrom-Json}catch{throw $help}
   if($config.enablePluginLoader -ne $true){throw $help}
   $quest=CheckedTarget $GameRoot 'nativePC/plugins/QuestLoader.dll'
   if((Test-Path -LiteralPath $quest) -and (FileHash $quest) -ne '97380a19c12822c318ebc7ef09df601823cbf33ec674e1aee9f8a690d5422c08'){throw $help}
   $variant='stracker'
  }else{throw $help}
 }elseif((Test-Path -LiteralPath $loader) -or (Test-Path -LiteralPath $configPath) -or (Test-Path -LiteralPath $plugin)) {throw $help}
 if($variant -eq 'standalone'){
  $plugins=CheckedTarget $GameRoot 'nativePC/plugins'
  if((Test-Path -LiteralPath $plugins) -and @(Get-ChildItem -LiteralPath $plugins -Filter '*.dll' -File -Recurse).Count){throw $help}
 }
 $manifestPath=CheckedTarget $GameRoot 'StormslingerSupport/release-manifest.json'
 if(Test-Path -LiteralPath $manifestPath){
  $old=Get-Content -LiteralPath $manifestPath -Raw -Encoding UTF8|ConvertFrom-Json
  if($old.variant -ne $variant){throw ('请先使用旧补丁卸载工具卸载并恢复原文件，再安装。 / Uninstall the previous patch and restore originals first.')}
 }
 $temporary=Join-Path ([IO.Path]::GetTempPath()) ('Horizon-unified-'+[Guid]::NewGuid().ToString('N'))
 [IO.Directory]::CreateDirectory($temporary)|Out-Null
 try{
  $zip=[IO.Compression.ZipFile]::OpenRead($ArchivePath)
  try{
   $expected=if($variant -eq 'standalone'){@{'payload/native.zip'='ab7208d95ecffa33f45702b47d38759213d024f43848dd37ea4c55812a365b5d';'tools/native.ps1'='87ec8d617b03f5d586224a1a875ded3bddc2b78fbd792c1da275a0e8d06d66a4'}}else{@{'payload/stracker.zip'='97ff416efe0209f7a24572fe065520e95d2ab613ed4cf1c5ed35632aca8e8b51';'tools/stracker.ps1'='541de156cd3f20249fd07fbf8c79c8d516f6b7b80d57838f78134fb80083f7f5'}}
   foreach($relative in $expected.Keys){
    $entries=@($zip.Entries|Where-Object {$_.FullName -eq $relative})
    if($entries.Count -ne 1){throw 'Unified package missing/duplicate entry / 统一包条目缺失或重复'}
    $target=Join-Path $temporary (Split-Path -Leaf $relative)
    [IO.Compression.ZipFileExtensions]::ExtractToFile($entries[0],$target,$false)
    if((FileHash $target) -ne $expected[$relative]){throw 'Unified package checksum failed / 统一包校验失败'}
   }
  }finally{$zip.Dispose()}
  $stem=if($variant -eq 'standalone'){'native'}else{'stracker'}
  Write-Output ('Automatic mode / 自动安装模式: '+$variant)
  & (Join-Path $temporary ($stem+'.ps1')) -ArchivePath (Join-Path $temporary ($stem+'.zip')) -GameRoot $GameRoot
  $result=$LASTEXITCODE
 }finally{
  # Only delete the two files extracted into this newly created directory.
  foreach($name in @('native.zip','native.ps1','stracker.zip','stracker.ps1')){ $path=Join-Path $temporary $name; if([IO.File]::Exists($path)){[IO.File]::Delete($path)} }
  [IO.Directory]::Delete($temporary,$false)
 }
 exit $result
}catch{
 [void][Windows.Forms.MessageBox]::Show(('Installation stopped / 安装停止:'+[Environment]::NewLine+$_.Exception.Message),$caption,[Windows.Forms.MessageBoxButtons]::OK,[Windows.Forms.MessageBoxIcon]::Error)
 exit 1
}
