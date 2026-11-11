#!/usr/bin/env python3
"""Offline checks for tracked source files and verified public dependencies."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
SKIP = ('.git/', '.build-core/', '.build-tests/', 'build/', 'modern/build/', 'dist/', 'release/', 'runtime/', 'sdcard/', 'switch/')


def source_files():
    try:
        top = subprocess.run(['git', '-C', str(ROOT), 'rev-parse', '--show-toplevel'], capture_output=True, text=True)
        if top.returncode == 0 and Path(top.stdout.strip()).resolve() == ROOT:
            result = subprocess.run(['git', '-C', str(ROOT), 'ls-files', '-z'], capture_output=True, check=True)
            names = [x.decode('utf-8') for x in result.stdout.split(b'\0') if x]
            if names:
                return [ROOT / name for name in names]
    except (OSError, subprocess.CalledProcessError):
        pass
    return [p for p in ROOT.rglob('*') if p.is_file()
            and not p.relative_to(ROOT).as_posix().startswith(SKIP)
            and '__pycache__' not in p.parts]


def check():
    errors = []
    files = source_files()
    allowed = {f'modern/lib/switch/{n}.a' for n in ('libavcodec', 'libavformat', 'libavutil', 'libswresample')}
    private = {'identity.secret', 'identity.public', 'connection.log', 'settings.json', '.env', '.env.local'}
    for p in files:
        rel = p.relative_to(ROOT).as_posix()
        if not p.is_file():
            errors.append(f'Missing tracked file: {rel}')
            continue
        if p.stat().st_size > 100 * 1024 * 1024:
            errors.append(f'File exceeds 100 MiB: {rel}')
        if p.name.lower() in private or p.suffix.lower() in {'.pem', '.nro', '.elf', '.nacp', '.log', '.bak', '.pyc', '.o', '.obj'}:
            errors.append(f'Private/runtime/build file tracked: {rel}')
        if p.suffix == '.a' and rel not in allowed:
            errors.append(f'Unapproved precompiled archive: {rel}')
        if rel.startswith(('.git/', 'release/', 'dist/', 'switch/', 'sdcard/')):
            errors.append(f'Generated/private directory tracked: {rel}')
    options = (ROOT / 'src/zerotier/Lifecycle.hpp').read_text(encoding='utf-8')
    if not all(x in options for x in ('bool enabled = false;', 'std::string network;', 'std::string peer;')):
        errors.append('ZeroTier factory defaults must remain disabled and empty.')
    meta = json.loads((ROOT / 'modern/lib/switch/UPSTREAM-VERIFICADO.json').read_text(encoding='utf-8'))
    for entry in meta['libraries']:
        if hashlib.sha256((ROOT / 'modern/lib/switch' / entry['file']).read_bytes()).hexdigest() != entry['sha256']:
            errors.append(f'FFmpeg hash mismatch: {entry["file"]}')
    ffdir = ROOT / 'third_party/ffmpeg-source'
    origin = json.loads((ffdir / 'ORIGEM.json').read_text(encoding='utf-8'))
    if hashlib.sha256((ffdir / f'FFmpeg-{origin["commit"]}.tar.gz').read_bytes()).hexdigest() != origin['source_archive_sha256']:
        errors.append('FFmpeg source archive hash mismatch.')
    curl = ROOT / 'curl/source/curl-7.69.1.tar.xz'
    if hashlib.sha256(curl.read_bytes()).hexdigest() != '03c7d5e6697f7b7e40ada1b2256e565a555657398e6c1fcfa4cb251ccd819d4f':
        errors.append('curl source archive hash mismatch.')
    if errors:
        print('\n'.join(errors), file=sys.stderr)
        return 1
    largest = max(p.stat().st_size for p in files)
    print(f'Repository checks passed: {len(files)} source files; largest {largest} bytes.')
    print('Verified public libraries and source archives; no listed private/runtime artifacts tracked.')
    return 0


if __name__ == '__main__':
    raise SystemExit(check())
