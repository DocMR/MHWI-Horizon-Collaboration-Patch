"""Package bounded static crafting evidence for later composition, not runtime."""
from pathlib import Path
import hashlib
import json

P=Path(__file__).resolve().parents[1]
REPORTS=(
    'palico-set-recipe-path','palico-craft-binding','palico-set-cost-shape',
    'palico-material-admission','palico-funds-capacity','inventory-quantity',
    'armor-output','armor-selection','craft-display-refresh',
)
FOLLOWUPS=(
    'postcraft_interaction_followup_2026_10_20',
    'hunter_postcraft_followup_2026_10_20',
)


def main():
    rows=[]
    for name in REPORTS:
        path=P/f'research/horizon-other-{name}.json'
        raw=path.read_bytes()
        report=json.loads(raw)
        rows.append(dict(path=str(path.relative_to(P)),sha256=hashlib.sha256(raw).hexdigest(),
            evidence=report))
    result=dict(schema_version=1,module='horizon-other',content_complete=False,
        scope='Target Palico equipment registration, selected recipe, native admission, payment and output static evidence.',
        target_series=[66,156,179],target_rows=[61,126,149,223,288,311],reports=rows,
        merge_rule='Evidence only. Reuse original native flow and existing six exclusion edits; do not install additional payment/output hooks from this document.',
        remaining=['Menu list unlock/progression producer and final enable-state aggregation',
            'Actual full-set menu mode and target recipe routing end to end',
            'Post-craft equip/save persistence',
            'Hunter armor/layered and weapon crafting remain separately scoped'],
        dynamic_validation_performed=False)
    target=P/'staging/Horizon-other-module/merge/craft-native-evidence.json'
    # Keep the authored native-interaction joins when rebuilding base evidence.
    # Refresh their hashes from the same report bytes packaged above.
    previous=json.loads(target.read_text(encoding='utf-8-sig')) if target.exists() else {}
    hashes={row['path'].replace('\\','/'):row['sha256'] for row in rows}
    for key in FOLLOWUPS:
        if key in previous:
            entry=dict(previous[key])
            entry['sha256']=hashes[entry['report'].replace('\\','/')]
            result[key]=entry
    target.write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(reports=len(rows),target_rows=6,content_complete=False)))


if __name__=='__main__':
    main()
