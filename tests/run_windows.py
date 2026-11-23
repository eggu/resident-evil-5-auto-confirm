#!/usr/bin/env python3
"""Run synthetic native-image checks on Windows, isolated from any game folder."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
if os.name != 'nt':
    raise SystemExit('Run this script on Windows after build.py; it does not launch the game.')
asi = root / 'dist/RE5.AutoConfirm.asi'
for executable in ['re5dx9.exe', 'Launcher.exe']:
    cases = [('ui_test.exe', [mode], False) for mode in ['off', 'save', 'confirm', 'all', 'unsupported', 'slot-conflict', 'save-only-slot-conflict', 'confirm-only-save-conflict']]
    cases += [('skip_hook_test.exe', [mode], True) for mode in ['off', 'buttons', 'unlock', 'movies', 'all', 'default', 'unsupported', 'conflict', 'previous-name']]
    cases += [('coop_hook_test.exe', ['RE5.AutoConfirm.asi', *mode], True)
              for mode in [[], ['off'], ['unsupported']]]
    for harness, args, needs_asi in cases:
        with tempfile.TemporaryDirectory(prefix='re5-auto-confirm-') as tmp:
            directory = Path(tmp)
            shutil.copy2(root / 'build' / harness, directory / executable)
            if needs_asi:
                shutil.copy2(asi, directory)
            subprocess.run([str(directory / executable), *args], cwd=directory, check=True, timeout=20)
for name in ['coop_input_test.exe', 'skip_skip_test.exe']:
    subprocess.run([str(root / 'build' / name)], check=True, timeout=20)
print('All synthetic Windows checks passed; in-game acceptance remains separate.')
