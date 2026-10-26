"""Emit the supported executable identity from the certified source metadata."""
import json
from pathlib import Path
P=Path(__file__).resolve().parents[1]
meta=json.loads((P/'research/exported-text-metadata.json').read_text(encoding='utf-8'))
digest=bytes.fromhex(meta['exe_sha256']);assert len(digest)==32
(P/'staging/Horizon-other-module/src/generated_executable_identity.inc').write_text(
 '// Generated supported on-disk executable SHA256.\nconstexpr std::uint8_t kExecutableDigest[] = {'+
 ', '.join(f'0x{x:02X}' for x in digest)+'};\n',encoding='utf-8')
print(json.dumps({'supported_exe_sha256':digest.hex()}))
