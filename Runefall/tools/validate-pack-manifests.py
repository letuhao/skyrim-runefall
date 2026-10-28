#!/usr/bin/env python3
"""Fail-closed validator for Runefall format-1 pack manifests."""
from __future__ import annotations

import json
import sys
from pathlib import Path

REQUIRED = ("packId", "plugin", "format", "minBaseVersion", "contentFiles", "recordCount", "recordEditors", "conflicts")


def fail(message: str) -> None:
    print(f"FAIL: {message}")
    raise SystemExit(1)


def main() -> None:
    if len(sys.argv) != 2:
        fail("usage: validate-pack-manifests.py <packs-dir>")
    pack_dir = Path(sys.argv[1]).resolve()
    if not pack_dir.is_dir():
        fail(f"pack directory missing: {pack_dir}")
    data_root = pack_dir.parent
    seen: set[str] = set()
    manifests = sorted(pack_dir.glob("*.json"))
    if not manifests:
        fail(f"no pack manifests in {pack_dir}")
    for path in manifests:
        try:
            manifest = json.loads(path.read_text(encoding="utf-8"))
        except Exception as exc:  # noqa: BLE001 - validator must report the file
            fail(f"{path.name}: invalid JSON: {exc}")
        for key in REQUIRED:
            if key not in manifest:
                fail(f"{path.name}: missing {key}")
        pack_id = manifest["packId"]
        if not isinstance(pack_id, str) or not pack_id:
            fail(f"{path.name}: invalid packId")
        if pack_id in seen:
            fail(f"{path.name}: duplicate packId {pack_id}")
        seen.add(pack_id)
        if manifest["format"] != 1:
            fail(f"{path.name}: format {manifest['format']} != 1")
        plugin = manifest["plugin"]
        if not isinstance(plugin, str) or not plugin.lower().endswith((".esp", ".esl", ".esm")):
            fail(f"{path.name}: invalid plugin {plugin!r}")
        files = manifest["contentFiles"]
        if not isinstance(files, list) or not all(isinstance(item, str) for item in files):
            fail(f"{path.name}: contentFiles must be a string list")
        for item in files:
            if not (data_root / item).is_file():
                fail(f"{path.name}: content file missing: {item}")
        if not isinstance(manifest["recordCount"], int) or manifest["recordCount"] < 0:
            fail(f"{path.name}: invalid recordCount")
        if not isinstance(manifest["recordEditors"], list) or not all(isinstance(item, str) for item in manifest["recordEditors"]):
            fail(f"{path.name}: recordEditors must be a string list")
        if not isinstance(manifest["conflicts"], list):
            fail(f"{path.name}: conflicts must be a list")
        print(f"PASS {path.name}: {pack_id} -> {plugin} ({manifest['recordCount']} records)")
    print(f"RESULT: PASS ({len(manifests)} manifests)")


if __name__ == "__main__":
    main()
