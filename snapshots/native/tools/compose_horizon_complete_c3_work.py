"""Assemble implemented C2 feedback repairs without installing or releasing them."""
from pathlib import Path
import hashlib
import json
import shutil

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    target = ROOT / 'staging/Horizon-complete-c3-work-r2-20261022'
    if target.exists():
        raise SystemExit('Work package already exists; preserve it and choose a new output.')
    base = ROOT / 'staging/Horizon-complete-cumulative-c2-r2-20261021/package/payload'
    overlays = [
        ROOT / 'staging/Horizon-complete-focus-repair-r3-20261021/nativePC',
        ROOT / 'staging/Horizon-complete-tutorial-video-repair-20261021/nativePC',
        ROOT / 'staging/Horizon-complete-hunter-head-repair-20261022/nativePC',
    ]
    dll = ROOT / 'runtime/ninja-horizon-complete-c3-repairs-20261021/dinput8.dll'
    assert base.is_dir() and dll.is_file()
    inputs = [(p, p.relative_to(base)) for p in sorted(base.rglob('*')) if p.is_file()]
    for source in overlays:
        assert source.is_dir()
        inputs += [(p, Path('nativePC') / p.relative_to(source))
                   for p in sorted(source.rglob('*')) if p.is_file()]
    inputs += [(dll, Path('dinput8.dll'))]
    changes = []
    for source, relative in inputs:
        out = target / 'payload' / relative
        old = sha(out) if out.exists() else None
        new = sha(source)
        if old and old != new:
            # Shared existing tables require deliberate merges, never overwrite
            # an unrelated cumulative table with a single-feature base table.
            assert relative.as_posix() == 'dinput8.dll', relative
        out.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, out)
        changes.append(dict(path=relative.as_posix(), source=str(source.relative_to(ROOT)),
                            before=old, sha256=new))
    files = [dict(path=p.relative_to(target / 'payload').as_posix(),
                  size=p.stat().st_size, sha256=sha(p))
             for p in sorted((target / 'payload').rglob('*')) if p.is_file()]
    (target / 'work-state.json').write_text(json.dumps(dict(
        status='incomplete_work_package_not_installable_release',
        files=files, composition=changes,
        pending=['Notice panel geometry live acceptance', 'Hunter armor preview/appearance repair',
                 'Fresh release/support manifests and paired source',
                 'Concentrated manual acceptance including deferred armor hang'],
        game_modified=False), ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(dict(output=str(target), files=len(files), game_modified=False)))


if __name__ == '__main__':
    main()
