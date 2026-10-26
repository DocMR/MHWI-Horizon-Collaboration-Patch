from pathlib import Path
import hashlib,json,zipfile,shutil
P=Path(chr(92)*2+'?'+chr(92)+str(Path(__file__).resolve().parents[1]))
BASE='Monster Hunter World Iceborne - Horizon Zero Dawn Collaboration Patch'
ROOT=P/(BASE+' - Release Packages')
OLD=P/'staging/Horizon-pre-unified-delivery-originals-20261023'
OUT=ROOT/'Unified Edition'
STAGE=P/'staging/Horizon-unified-delivery-20261023'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 OUT.mkdir(exist_ok=True);STAGE.mkdir(exist_ok=True)
 native=OLD/BASE/(BASE+'.zip')
 plugin=P/'staging/Horizon-mapping-link-fix-20261023/stracker-fixed.zip'
 native_script=P/'tools/Install-HorizonPatch.ps1';plugin_script=P/'tools/Install-HorizonStrackerPatch.ps1'
 with zipfile.ZipFile(native) as z:
  m=json.loads(z.read('StormslingerSupport/release-manifest.json'))
  native_hash=next(r['sha256'] for r in m['files'] if r['relative_path']=='dinput8.dll')
 script=native_script.read_text(encoding='utf-8-sig').split('try {\n if(-not $ArchivePath)',1)[0]
 assert script.endswith('}\n')
 script+=(P/'tools/horizon_unified_dispatch_body.ps1').read_text(encoding='utf-8-sig')
 values={'__NATIVE_HASH__':native_hash,'__NATIVE_ZIP_HASH__':sha(native),'__PLUGIN_ZIP_HASH__':sha(plugin),'__NATIVE_SCRIPT_HASH__':sha(native_script),'__PLUGIN_SCRIPT_HASH__':sha(plugin_script)}
 for old,new in values.items():script=script.replace(old,new)
 ps=P/'tools/Install-HorizonUnifiedPatch.ps1';ps.write_text(script,encoding='utf-8-sig',newline='\r\n')
 cmd=(P/'tools/Install-HorizonStrackerPatch.cmd').read_text(encoding='utf-8').split('# HORIZON_POWERSHELL_PAYLOAD\n',1)[0]+'# HORIZON_POWERSHELL_PAYLOAD\n'+script
 (OUT/'Install-Patch.cmd').write_text(cmd,encoding='utf-8',newline='\r\n')
 archive=OUT/(BASE+' - Unified Edition.zip')
 with zipfile.ZipFile(archive,'w',zipfile.ZIP_STORED) as z:
  for path,name in [(native,'payload/native.zip'),(plugin,'payload/stracker.zip'),(native_script,'tools/native.ps1'),(plugin_script,'tools/stracker.ps1')]:z.write(path,name)
 text=(OLD/BASE/'安装说明 - Installation Instructions.txt').read_text(encoding='utf-8-sig')
 text=text.replace('（原生完整版）','（统一安装版）').replace('(Complete Native Edition)','(Unified Edition)')
 text=text.replace('补丁自带所需DLL和资源，无需Python、Stracker或额外MOD加载器；不保证与其它MOD兼容。','补丁自带两种加载方式，无需Python。安装器自动判断：未安装加载器时使用原生方式；已有受支持的Stracker加载器时使用插件方式，保留原dinput8.dll、loader.dll及配置。检测到其它已识别加载器、不完整或不受支持的加载链时拒绝安装，并提示安装受支持的Stracker’s Loader 4.0及启用enablePluginLoader。安装器不下载或替换第三方加载器；不保证与其它MOD兼容。')
 text=text.replace('Required patch DLLs and resources are included. Python, Stracker and additional mod loaders are not needed. Compatibility with other mods is not guaranteed.','Both loading modes are included; Python is not required. The installer automatically selects native mode when no loader is present, or plugin mode for a supported Stracker installation, retaining its original dinput8.dll, loader.dll and configuration. Other recognized loaders or incomplete/unsupported loader chains are refused with instructions to install the supported Stracker’s Loader 4.0 and enable enablePluginLoader. The installer does not download or replace third-party loaders. Compatibility with other mods is not guaranteed.')
 old='手动方法：将ZIP全部内容手动解压到游戏根目录，直接得到nativePC、dinput8.dll、Export-Feedback.cmd、Uninstall-Patch.cmd和StormslingerSupport，不要套额外文件夹。'
 text=text.replace(old,'本统一ZIP是安装器的数据包，不要直接解压到游戏目录，也不要自行选择内部子包。保持ZIP和Install-Patch.cmd同目录，由安装器自动选择并解压。两文件可一起复制到其它电脑使用；该电脑仍须满足前置条件。')
 old='Manual alternative: Extract all ZIP contents directly into the game root so that nativePC, dinput8.dll, Export-Feedback.cmd, Uninstall-Patch.cmd and StormslingerSupport appear there without an extra enclosing folder.'
 text=text.replace(old,'The unified ZIP is installer data. Do not extract it directly into the game directory or select an internal package yourself. Keep it beside Install-Patch.cmd; the installer selects and extracts the correct mode automatically. Copy both files together to another PC, which must still meet the requirements.')
 text=text.replace('；手动解压不会重置已确认记录。','。').replace(' Manual extraction does not reset an existing acknowledgement.','')
 start=text.index('手动方法：关闭游戏，按ZIP中的文件路径移除补丁')
 end=text.index('\n\n说明：',start)
 text=text[:start]+'请使用已安装的Uninstall-Patch.cmd卸载，它会识别本次安装方式。插件方式下不删除原Stracker加载器及其它插件；不要直接删除整个nativePC。'+text[end:]
 start=text.index('Manual alternative: Close the game, remove the patch files')
 end=text.index('\n\nNotes:',start)
 text=text[:start]+'Use the installed Uninstall-Patch.cmd, which matches the installed mode. Plugin mode preserves the original Stracker loader and unrelated plugins. Do not delete the entire nativePC folder.'+text[end:]
 text='当前统一包：离线检查完成后交由玩家手动实机验收。 / Unified package: pending user-operated live acceptance after offline checks.\n\n'+text
 text+='\n\nStracker’s Loader: https://www.nexusmods.com/monsterhunterworld/mods/1982?tab=files\n受支持文件会由安装器自动核验；未知改名注入器无法仅凭文件检测保证识别。 / Supported files are checked automatically; file-based detection cannot guarantee identification of arbitrarily renamed injectors.\n'
 (OUT/'安装说明 - Installation Instructions.txt').write_text(text,encoding='utf-8-sig',newline='\r\n')
 report=dict(status='built_tests_pending',directory=str(OUT),archive_sha256=sha(archive),installer_sha256=sha(OUT/'Install-Patch.cmd'),inputs=values,game_modified=False,live_tested=False)
 (STAGE/'delivery-index.json').write_text(json.dumps(report,indent=2))
 print(json.dumps(report))
if __name__=='__main__':main()
