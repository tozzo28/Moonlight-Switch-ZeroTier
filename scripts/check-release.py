#!/usr/bin/env python3
"""Validate an NRO staging directory, including repeat builds with checksums."""
from pathlib import Path
import hashlib
import struct
import sys

NRO_PATH = 'switch/Moonlight-Switch/Moonlight-Switch.nro'


def check_nro(data):
    if len(data) < 0x80 or data[16:20] != b'NRO0':
        raise ValueError('Invalid NRO header')
    size = struct.unpack_from('<I', data, 24)[0]
    if size < 0x80 or size + 56 > len(data) or data[size:size + 4] != b'ASET':
        raise ValueError('Missing or out-of-bounds asset header')
    for entry in (8, 24, 40):
        offset, length = struct.unpack_from('<QQ', data, size + entry)
        if offset < 56 or size + offset + length > len(data):
            raise ValueError('Asset lies outside NRO')


def check_directory(root):
    actual = {p.relative_to(root).as_posix() for p in root.rglob('*') if p.is_file()}
    if NRO_PATH not in actual or actual - {NRO_PATH, 'SHA256SUMS'}:
        raise ValueError(f'Unexpected release contents: {sorted(actual)}')
    data = (root / NRO_PATH).read_bytes()
    check_nro(data)
    if str(Path(__file__).resolve().parents[1]).encode() in data:
        raise ValueError('Current build directory appears in NRO; check prefix maps')
    checksum = root / 'SHA256SUMS'
    if checksum.exists():
        expected = f'{hashlib.sha256(data).hexdigest()}  {NRO_PATH}'
        if checksum.read_text(encoding='utf-8').strip() != expected:
            raise ValueError('Staging checksum does not match NRO')
    print('NRO structure, release allowlist and available checksum passed; hardware tests are separate.')


if __name__ == '__main__':
    check_directory(Path(sys.argv[1]))
