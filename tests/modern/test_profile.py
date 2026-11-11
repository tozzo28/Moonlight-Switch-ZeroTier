import importlib.util
from pathlib import Path
import tempfile
import json

spec = importlib.util.spec_from_file_location("migration", Path(__file__).resolve().parents[2] / "scripts/migrate-modern-profile.py")
migration = importlib.util.module_from_spec(spec)
spec.loader.exec_module(migration)
old = {"hosts": [{"address": "192.168.1.8", "favorites": [1]}], "settings": {"keyboard_fingers": 3, "keyboard_type": 0, "mouse_speed_multiplier": 34, "video_codec": 1}, "mapping_layouts": [{"name": "custom"}]}
working = {"hosts": [{"address": "192.0.2.18", "hostname": "desktop", "mac": "aa:bb:cc:dd:ee:ff"}], "settings": {"video_codec": 0}}
merged = migration.merge_profiles(old, working)
assert len(merged["hosts"]) == 2
assert merged["settings"] == old["settings"]
assert merged["mapping_layouts"] == old["mapping_layouts"]
assert merged["hosts"][0]["favorites"] == [1]
assert merged["hosts"][1]["remote_address"] == ""
assert len(migration.merge_profiles(merged, working)["hosts"]) == 2
assert len(old["hosts"]) == 1
with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    original = root / "switch/Moonlight-Switch"
    legacy = root / "switch/moonlight"
    original.mkdir(parents=True)
    (legacy / "key").mkdir(parents=True)
    (original / "settings.json").write_text(json.dumps(old))
    (legacy / "settings.json").write_text(json.dumps(working))
    for name in ("client.pem", "key.pem"):
        (legacy / "key" / name).write_bytes(b"test-credential")
    before = (original / "settings.json").read_bytes()
    result = migration.migrate(root)
    assert json.loads((result / "settings.json").read_text()) == merged
    assert (result / "key/key.pem").read_bytes() == b"test-credential"
    assert (original / "settings.json").read_bytes() == before
    (result / "settings.json").write_text('{"user_changes":true}')
    migration.migrate(root)
    assert json.loads((result / "settings.json").read_text()) == {"user_changes": True}
print("profile: original settings/favorites/mappings preserved, ZeroTier host/paired keys added, idempotent migration OK")
