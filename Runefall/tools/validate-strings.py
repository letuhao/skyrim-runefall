#!/usr/bin/env python3
"""Fail-closed English-only string table validator for Runefall."""
from __future__ import annotations

import json
import re
import sys
import unicodedata
from pathlib import Path

CORE_PREFIXES = ("attribute.", "category.", "hunt.", "material.", "menu.", "prefix.", "rarity.", "recipe.", "skill.", "slot.", "status.", "suffix.", "ui.", "unique.", "unit.")


def fail(message: str) -> None:
    print(f"FAIL: {message}")
    raise SystemExit(1)


def main() -> None:
    if len(sys.argv) != 2:
        fail("usage: validate-strings.py <strings.json>")
    path = Path(sys.argv[1])
    if not path.is_file():
        fail(f"missing string table: {path}")
    table = json.loads(path.read_text(encoding="utf-8"))
    if table.get("schemaVersion") != 1:
        fail("unsupported strings schemaVersion")
    columns = table.get("columns")
    rows = table.get("rows")
    if columns != ["key", "english", "context"] or not isinstance(rows, list):
        fail("strings table must have key/english/context columns and rows")
    if len(rows) < 250:
        fail(f"string coverage too small: {len(rows)} rows")
    seen: set[str] = set()
    for index, row in enumerate(rows):
        key = row.get("key") if isinstance(row, dict) else None
        value = row.get("english") if isinstance(row, dict) else None
        if not isinstance(key, str) or not key:
            fail(f"row {index}: invalid key")
        if key in seen:
            fail(f"row {index}: duplicate key {key}")
        seen.add(key)
        if not isinstance(value, str) or not value.strip():
            fail(f"row {index}: empty value for {key}")
        if not any(key.startswith(prefix) for prefix in CORE_PREFIXES):
            fail(f"row {index}: key {key} is outside the Runefall namespace")
        for char in value:
            name = unicodedata.name(char, "")
            if "CJK" in name or "HIRAGANA" in name or "KATAKANA" in name or "HANGUL" in name:
                fail(f"row {index}: non-English character in {key}")
        if value.count("{") != value.count("}"):
            fail(f"row {index}: unbalanced placeholder braces in {key}")
        if re.search(r"\{[^0-9{}]+\}", value):
            fail(f"row {index}: non-numeric placeholder in {key}")
    print(f"PASS strings: {len(rows)} keys, unique, English-only, balanced placeholders")
    print("RESULT: PASS")


if __name__ == "__main__":
    main()
