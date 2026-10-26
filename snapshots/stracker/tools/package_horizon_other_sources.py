"""Package generated module sources for compilation without game/PS inputs.

This is a source snapshot for the final merge, not an installable game patch.
"""
from pathlib import Path
import hashlib
import json
import re
import zipfile

ROOT = Path(__file__).resolve().parents[1]
MODULE = ROOT / 'staging/Horizon-other-module'


def main():
    receipt = json.loads((MODULE / 'merge/catalog-build.json').read_text(encoding='utf-8'))
    if receipt.get('exit_code') != 0:
        raise ValueError('Build the current module successfully before packaging')
    for row in receipt['files']:
        source = MODULE / row['file']
        if source.parent == MODULE / 'src':
            if hashlib.sha256(source.read_bytes()).hexdigest() != row['sha256']:
                raise ValueError(f'Build receipt is stale: {source}')

    selected = set((MODULE / 'src').iterdir())
    commands = ['build-horizon-other-static.cmd', 'build-horizon-other-link-only.cmd',
                'horizon-other-msvc.cmd']
    for name in commands:
        command = ROOT / 'tools' / name
        selected.add(command)
        for token in re.findall(r'[^\s"<>|]+\.(?:cpp|asm|c)\b', command.read_text(encoding='utf-8')):
            selected.add((MODULE / token.replace('\\', '/')).resolve())
    vendor = ROOT / 'runtime/vendor/minhook-1.3.4'
    selected.update(path for path in vendor.rglob('*') if path.is_file())

    pending = list(selected)
    blobs = {}
    while pending:
        path = pending.pop().resolve()
        if path in blobs:
            continue
        if not path.is_relative_to(ROOT) or not path.is_file():
            raise ValueError(f'Invalid source dependency: {path}')
        data = path.read_bytes()
        blobs[path] = data
        if path.suffix not in ('.cpp', '.h', '.c', '.inc', '.asm'):
            continue
        for name in re.findall(r'^\s*#\s*include\s*"([^"\n]+)"', data.decode('utf-8-sig'), re.M):
            dependency = (path.parent / name).resolve()
            if not dependency.is_file() and name == 'MinHook.h':
                dependency = vendor / 'include/MinHook.h'
            pending.append(dependency)

    files = {path.relative_to(ROOT).as_posix(): data for path, data in blobs.items()}
    files['build.cmd'] = ("@echo off\r\ncall \"%~dp0tools\\build-horizon-other-static.cmd\"\r\n"
                          "if not \"%errorlevel%\"==\"0\" exit /b %errorlevel%\r\n"
                          "call \"%~dp0tools\\build-horizon-other-link-only.cmd\"\r\n"
                          "exit /b %errorlevel%\r\n").encode('ascii')
    files['README.txt'] = (
        'Horizon other-content generated source snapshot (incomplete module)\r\n'
        'Extract to a separate directory and run build.cmd with Visual Studio x64 C++ tools.\r\n'
        'Use an x64 developer prompt, Visual Studio Installer discovery, or set HORIZON_VCVARS64.\r\n'
        'Compilation and link checking require no game executable, PS assets, Python or research files.\r\n'
        'Generated sources are frozen inputs; regeneration still requires the original project.\r\n'
        'The static library is under staging/Horizon-other-module/build.\r\n'
        'The NOENTRY/no-export DLL under build/link-only is diagnostic only; never load/install it.\r\n'
        'Shared runtime sources must have one owner in the final merge; do not link duplicate copies.\r\n'
        'This archive contains no game resources, production loader or installer.\r\n'
    ).encode('utf-8')
    manifest = {'schema_version': 1, 'kind': 'generated-source-build-snapshot',
                'content_complete': False, 'installable': False,
                'files': [{'path': name, 'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
                          for name, data in sorted(files.items())]}
    files['source-manifest.json'] = json.dumps(manifest, indent=2).encode('utf-8')
    destination = ROOT / 'staging/Horizon-other-source-build.zip'
    temporary = destination.with_suffix('.zip.tmp')
    with zipfile.ZipFile(temporary, 'w', zipfile.ZIP_DEFLATED) as archive:
        for name, data in sorted(files.items()):
            info = zipfile.ZipInfo(name, date_time=(2026, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(info, data)
    temporary.replace(destination)
    print(json.dumps({'archive': str(destination), 'files': len(files),
                      'bytes': destination.stat().st_size, 'installable': False}))


if __name__ == '__main__':
    main()
