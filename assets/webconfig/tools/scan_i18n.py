#!/usr/bin/env python3
"""
scan_i18n.py

Scans all JSON files in assets/webconfig/i18n for common malformed
placeholder/template tokens that may break the i18n parser.

Checks for:
  - "$ 1" (space between $ and digit)
  - "1$" (digit before $)
  - "{{Plural:" (capitalized template name)
  - "||" (double pipe in plural options)
  - "| <space>" (spaces after pipe delimiter)

Writes two reports:
  - assets/webconfig/i18n/i18n_scan_report.json
  - assets/webconfig/i18n/i18n_scan_report.txt

Usage:
  python3 assets/webconfig/tools/scan_i18n.py
"""

from __future__ import annotations
import re
import os
import json
from pathlib import Path
from typing import List, Dict, Any

ROOT = Path(__file__).resolve().parents[1]
I18N_DIR = ROOT / 'i18n'
REPORT_JSON = I18N_DIR / 'i18n_scan_report.json'
REPORT_TXT = I18N_DIR / 'i18n_scan_report.txt'

PATTERNS = [
    ("space_after_dollar", re.compile(r"\$\s+\d+"), 'Space between $ and digit ("$ 1")'),
    ("digit_before_dollar", re.compile(r"\d+\$"), 'Digit before $ ("1$")'),
    ("capital_plural", re.compile(r"\{\{\s*Plural:"), 'Capitalized "Plural:" token'),
    ("double_pipe", re.compile(r"\|\|"), 'Double pipe in plural/template options ("||")'),
    ("space_after_pipe", re.compile(r"\|\s+[^\s]"), 'Space after pipe delimiter in template params ("| ...")'),
]


def scan_file(path: Path) -> List[Dict[str, Any]]:
    issues: List[Dict[str, Any]] = []
    try:
        with path.open('r', encoding='utf-8') as fh:
            for lineno, raw in enumerate(fh, start=1):
                line = raw.rstrip('\n')
                for name, rx, desc in PATTERNS:
                    m = rx.search(line)
                    if m:
                        issues.append({
                            'line': lineno,
                            'column': m.start() + 1,
                            'match': m.group(0),
                            'type': name,
                            'description': desc,
                            'snippet': line.strip(),
                        })
    except Exception as e:
        issues.append({'type': 'read_error', 'description': str(e)})
    return issues


def main() -> int:
    if not I18N_DIR.exists() or not I18N_DIR.is_dir():
        print('i18n directory not found:', I18N_DIR)
        return 2

    files = sorted([p for p in I18N_DIR.iterdir() if p.is_file() and p.suffix == '.json'])
    report: Dict[str, Any] = {'generated': None, 'files': {}}
    from datetime import datetime, timezone
    # Use timezone-aware UTC datetime to avoid DeprecationWarning
    report['generated'] = datetime.now(timezone.utc).isoformat()

    for p in files:
        issues = scan_file(p)
        if issues:
            report['files'][p.name] = issues

    # Write JSON report
    try:
        with REPORT_JSON.open('w', encoding='utf-8') as jf:
            json.dump(report, jf, indent=2, ensure_ascii=False)
    except Exception as e:
        print('Failed to write JSON report:', e)
        return 3

    # Write human readable report
    lines: List[str] = []
    lines.append(f'i18n scan report - generated {report["generated"]}')
    lines.append('')

    if not report['files']:
        lines.append('No issues found.')
    else:
        for fn, issues in report['files'].items():
            lines.append(f'File: {fn}')
            for it in issues:
                if it.get('type') == 'read_error':
                    lines.append(f"  [ERROR] Could not read file: {it.get('description')}")
                else:
                    lines.append(f"  Line {it['line']}, Col {it['column']}: {it['description']} -> {it['match']}")
                    lines.append(f"    {it['snippet']}")
            lines.append('')

    try:
        with REPORT_TXT.open('w', encoding='utf-8') as tf:
            tf.write('\n'.join(lines))
    except Exception as e:
        print('Failed to write text report:', e)
        return 4

    print('Scan complete. Files checked:', len(files))
    print('Issues found in', len(report['files']), 'files.')
    print('Reports written to:', REPORT_JSON, REPORT_TXT)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
