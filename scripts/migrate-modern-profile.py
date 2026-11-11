#!/usr/bin/env python3
"""Create a separate original-UI profile, preserving all existing SD profiles."""
import argparse
import copy
import json
import os
from pathlib import Path
import shutil


def merge_profiles(original, working):
    merged = copy.deepcopy(original)
    hosts = merged.setdefault("hosts", [])
    existing = {item.get("address") for item in hosts}
    for item in working.get("hosts", []):
        if item.get("address") and item["address"] not in existing:
            host = copy.deepcopy(item)
            host.setdefault("remote_address", "")
            host.setdefault("favorites", [])
            hosts.append(host)
            existing.add(item["address"])
    return merged


def migrate(sd_root):
    switch = Path(sd_root) / "switch"
    original = switch / "Moonlight-Switch"
    working = switch / "moonlight"
    output = switch / "Moonlight-Switch-ZeroTier"
    original_settings = json.loads((original / "settings.json").read_text())
    working_settings = json.loads((working / "settings.json").read_text())
    key_source = working / "key"
    for name in ("client.pem", "key.pem"):
        if not (key_source / name).is_file():
            raise RuntimeError("Missing paired client credentials; existing profiles were not modified")
    output.mkdir(exist_ok=True)
    (output / "key").mkdir(exist_ok=True)
    (output / "boxart").mkdir(exist_ok=True)
    target = output / "settings.json"
    if not target.exists():
        temporary = output / "settings.json.tmp"
        with temporary.open("w") as file:
            json.dump(merge_profiles(original_settings, working_settings), file, indent=4, ensure_ascii=False)
            file.flush()
            os.fsync(file.fileno())
        os.replace(temporary, target)
    for name in ("client.pem", "key.pem"):
        destination = output / "key" / name
        if not destination.exists():
            shutil.copy2(key_source / name, destination)
            with destination.open("rb") as file:
                os.fsync(file.fileno())
    for source in original.glob("gamepad_mapping*.json"):
        destination = output / source.name
        if not destination.exists():
            shutil.copy2(source, destination)
    return output


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("sd_root", type=Path)
    args = parser.parse_args()
    print("Separate profile prepared:", migrate(args.sd_root))
