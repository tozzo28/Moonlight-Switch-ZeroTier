#!/usr/bin/env python3
"""Create local Release assets from a tested NRO and Git-tracked sources."""
from pathlib import Path
import argparse
import hashlib
import re
import shutil
import subprocess
import sys
import zipfile
from importlib.util import spec_from_file_location, module_from_spec

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--tag', default='v1.5.0-zerotier.1')
    parser.add_argument('--nro', type=Path, default=ROOT / 'release/switch/Moonlight-Switch/Moonlight-Switch.nro')
    parser.add_argument('--out', type=Path, default=ROOT / 'dist')
    args = parser.parse_args()
    if not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9._-]*', args.tag):
        parser.error('Invalid tag')
    subprocess.run([sys.executable, str(ROOT / 'scripts/check-repository.py')], check=True)
    result = subprocess.run(['git', '-C', str(ROOT), 'ls-files', '-z'], capture_output=True, check=True)
    tracked = [x.decode('utf-8') for x in result.stdout.split(b'\0') if x]
    if not tracked:
        parser.error('Stage the intended files with git add before packaging')
    data = args.nro.read_bytes()
    spec = spec_from_file_location('release_check', ROOT / 'scripts/check-release.py')
    module = module_from_spec(spec)
    spec.loader.exec_module(module)
    module.check_nro(data)
    args.out.mkdir(parents=True, exist_ok=True)
    prefix = f'Moonlight-Switch-ZeroTier-{args.tag}'
    binary_zip = args.out / f'{prefix}-switch.zip'
    source_zip = args.out / f'{prefix}-sources.zip'
    nro_name = 'switch/Moonlight-Switch/Moonlight-Switch.nro'
    with zipfile.ZipFile(binary_zip, 'w', zipfile.ZIP_DEFLATED, compresslevel=3) as z:
        z.writestr(nro_name, data)
        z.writestr('SHA256SUMS', f'{hashlib.sha256(data).hexdigest()}  {nro_name}\n')
        z.writestr('SOURCE.md', f'Sources: accompanying {source_zip.name}. Preserve sources, build scripts, licenses and notices.\n')
        z.write(ROOT / 'docs/INSTALACAO.md', 'README.md')
        z.write(ROOT / 'THIRD_PARTY_NOTICES.md', 'THIRD_PARTY_NOTICES.md')
        z.write(ROOT / 'docs/LICENCAS-REVISAO.md', 'LICENCAS-REVISAO.md')
        z.write(ROOT / 'docs/FONTES-COMPILADAS.json', 'FONTES-COMPILADAS.json')
        for name in tracked:
            if Path(name).name.upper().startswith(('LICENSE', 'COPYING', 'NOTICE', 'AUTHORS')):
                z.write(ROOT / name, 'licenses/' + name)
    with zipfile.ZipFile(source_zip, 'w', zipfile.ZIP_DEFLATED, compresslevel=3) as z:
        for name in tracked:
            z.write(ROOT / name, prefix + '/' + name)
    for archive in (binary_zip, source_zip):
        with zipfile.ZipFile(archive) as z:
            if z.testzip() is not None:
                raise ValueError(f'Corrupt ZIP: {archive.name}')
    checksums = ''.join(f'{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.name}\n' for p in (binary_zip, source_zip))
    (args.out / 'SHA256SUMS').write_text(checksums, encoding='utf-8')
    shutil.copyfile(ROOT / 'RELEASE_NOTES.md', args.out / 'RELEASE_NOTES.md')
    print(f'Local Release assets ready: {args.out}')
    print(binary_zip.name)
    print(source_zip.name)
    print('No upload was performed.')


if __name__ == '__main__':
    main()
