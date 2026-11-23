#!/usr/bin/env python3
"""Build the unified 32-bit ASI and run the inherited portable input checks."""
from pathlib import Path
import hashlib
import json
import shutil
import zipfile
import subprocess

root = Path(__file__).resolve().parent
build, dist = root / 'build', root / 'dist'
build.mkdir(exist_ok=True)
dist.mkdir(exist_ok=True)
flags = ['-std=c11', '-O2', '-Wall', '-Wextra', '-Werror']
for feature, test in [('coop', 'input_test'), ('skip', 'skip_test')]:
    source = root / f'tests/{feature}/{test}.c'
    output = build / test
    subprocess.run(['cc', *flags, str(source), '-o', str(output)], check=True)
    subprocess.run([str(output)], check=True)
    for name in [test, 'hook_test']:
        subprocess.run(['i686-w64-mingw32-gcc', *flags, '-static',
                        str(root / f'tests/{feature}/{name}.c'), '-o',
                        str(build / f'{feature}_{name}.exe')], check=True)
subprocess.run(['i686-w64-mingw32-gcc', *flags, '-static',
                str(root / 'tests/ui_test.c'), '-o', str(build / 'ui_test.exe')], check=True)
asi = dist / 'RE5.AutoConfirm.asi'
subprocess.run(['i686-w64-mingw32-gcc', *flags, '-shared', '-static',
                '-Wl,--no-insert-timestamp', '-Wl,--image-base,0x64f00000',
                '-ffile-prefix-map=' + str(root) + '=.',
                *map(str, [root / 'src/main.c', *sorted((root / 'src').glob('*/main.c'))]),
                '-o', str(asi)], check=True)
shutil.copy2(root / 'RE5.AutoConfirm.ini', dist)
(build / 'build-report.json').write_text(json.dumps({
    'asi_sha256': hashlib.sha256(asi.read_bytes()).hexdigest(),
    'runtime_verified': False,
}, indent=2) + '\n')
print(asi)

version = (root / 'VERSION').read_text().strip()
package = dist / f'RE5.AutoConfirm-v{version}.zip'
with zipfile.ZipFile(package, 'w', zipfile.ZIP_DEFLATED) as archive:
    for source in [asi, dist / 'RE5.AutoConfirm.ini', root / 'README.md', root / 'README.ko.md', root / 'LICENSE']:
        archive.write(source, source.name)
    archive.write(root / 'assets/logo.png', 'assets/logo.png')
with zipfile.ZipFile(package) as archive:
    assert archive.testzip() is None
    assert set(archive.namelist()) == {'RE5.AutoConfirm.asi', 'RE5.AutoConfirm.ini', 'README.md', 'README.ko.md', 'LICENSE', 'assets/logo.png'}
(dist / f'{package.name}.sha256').write_text(hashlib.sha256(package.read_bytes()).hexdigest() + '  ' + package.name + '\n')
print(package)
