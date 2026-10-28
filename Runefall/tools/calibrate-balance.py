#!/usr/bin/env python3
"""Offline balance calibration for Runefall 40-hour pacing and combat ratings."""
from __future__ import annotations

import json
import math
import sys
from pathlib import Path

def fail(message: str) -> None:
    print(f"FAIL: {message}")
    raise SystemExit(1)

def load(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))

def rows(table: dict) -> dict[str, dict]:
    return {row.get("stage", row.get("profileId", "")): row for row in table.get("rows", [])}

def main() -> None:
    if len(sys.argv) != 2:
        fail("usage: calibrate-balance.py <data-dir>")
    data = Path(sys.argv[1])
    stage = load(data / "stage_ladder.json")
    progression = load(data / "player_progression.json")["rows"][0]
    pace = load(data / "combat_pace_profiles.json")
    if len(stage.get("rows", [])) != 100:
        fail("stage ladder must have 100 stages")
    stages = {int(row["stage"]): row for row in stage["rows"]}
    for value in ("1", "10", "20", "50", "100"):
        pass
    def pcr(a: int, b: int) -> float:
        x, y = stages[a], stages[b]
        return math.sqrt((float(x["offenseCapacity"]) / float(y["offenseCapacity"])) * (float(x["defenseCapacity"]) / float(y["defenseCapacity"])))
    plus10 = pcr(11, 1) if 11 in stages else 0
    plus20 = pcr(21, 1) if 21 in stages else 0
    top = pcr(100, 1)
    if not (plus10 >= 1.10 and plus20 >= 1.25 and top >= 4.0):
        fail(f"stage curve too flat: +10={plus10:.2f} +20={plus20:.2f} top={top:.2f}")
    base = float(progression["xpBase"])
    growth = float(progression["xpGrowth"])
    total_xp = sum(base * (growth ** (rank - 1)) for rank in range(1, 100))
    xp_per_hour = 600.0
    hours = total_xp / xp_per_hour
    if not (30.0 <= hours <= 50.0):
        fail(f"40h pacing off: totalXP={total_xp:.0f} hours={hours:.1f}")
    for profile in pace["rows"]:
        for key in ("ttkMin", "ttkMax", "ttdMin", "ttdMax", "qMin", "qMax"):
            if float(profile[key]) < 0:
                fail(f"negative pace value {profile['profileId']}/{profile['encounterClass']}/{key}")
        if float(profile["ttkMax"]) <= float(profile["ttkMin"]) or float(profile["ttdMax"]) <= float(profile["ttdMin"]):
            fail(f"invalid pace band {profile['profileId']}/{profile['encounterClass']}")
    print(f"PASS pacing: totalXP={total_xp:.0f}, {hours:.1f}h at {xp_per_hour:.0f} XP/h")
    print(f"PASS stage curve: PCR +10={plus10:.2f}x, +20={plus20:.2f}x, 100/1={top:.2f}x")
    print(f"PASS pace profiles: {len(pace['rows'])} bands valid")
    print("RESULT: PASS")

if __name__ == "__main__":
    main()
