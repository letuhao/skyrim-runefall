#!/usr/bin/env python3
"""Validate Runefall JSON schemas and the base content pack."""
from __future__ import annotations
import json
import sys
from pathlib import Path

import jsonschema

SCHEMA_DIR = Path(__file__).resolve().parent / "v1"
CONTENT_DIR = SCHEMA_DIR.parent.parent / "base"
ENTITY_SCHEMA = {
    "stageLadder": "stage-ladder.schema.json",
    "tierDefinitions": "tier-definitions.schema.json",
    "curveProfiles": "curve-profiles.schema.json",
    "difficultyProfiles": "difficulty-profiles.schema.json",
    "assignmentRules": "assignment-rules.schema.json",
    "contentBands": "content-bands.schema.json",
    "legacyTierMap": "legacy-tier-map.schema.json",
    "roles": "roles.schema.json",
    "raceProfiles": "race-profiles.schema.json",
    "raceClassificationRules": "race-classification-rules.schema.json",
    "threatProtection": "threat-protection.schema.json",
    "enemyPackages": "enemy-packages.schema.json",
    "equipmentSets": "equipment-sets.schema.json",
    "playerProgression": "player-progression.schema.json",
    "penetrationProfiles": "penetration-profiles.schema.json",
    "worldBands": "world-bands.schema.json",
    "encounterTables": "encounter-tables.schema.json",
    "rareSpawns": "rare-spawns.schema.json",
    "huntContracts": "hunt-contracts.schema.json",
    "bosses": "bosses.schema.json",
    "anomalyFamilies": "anomaly-families.schema.json",
    "economyProfiles": "economy-profiles.schema.json",
    "classes": "classes.schema.json",
    "compatProfiles": "compat-profiles.schema.json",
    "endgameProfiles": "endgame-profiles.schema.json",
    "skillPerks": "skill-perks.schema.json",
    "ratingProfiles": "rating-profiles.schema.json",
    "combatPaceProfiles": "combat-pace-profiles.schema.json",
    "ratingDisplayProfiles": "rating-display-profiles.schema.json",
}

def load(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))

def validator_for(schema_path: Path):
    schema = load(schema_path)
    resolver = jsonschema.RefResolver(base_uri=schema_path.as_uri(), referrer=schema)
    cls = jsonschema.validators.validator_for(schema)
    return cls(schema, resolver=resolver)

def main() -> int:
    errors: list[str] = []
    schemas = sorted(SCHEMA_DIR.glob("*.schema.json"))
    if not schemas:
        print("FAIL: no schemas")
        return 1
    for path in schemas:
        try:
            load(path)
        except Exception as exc:
            errors.append(f"{path.name}: {exc}")
    pack_path = CONTENT_DIR / "pack.json"
    if not pack_path.exists():
        errors.append("base pack.json missing")
    else:
        try:
            pack = load(pack_path)
            validator_for(SCHEMA_DIR / "pack.schema.json").validate(pack)
        except Exception as exc:
            errors.append(f"pack.json: {exc}")
        else:
            for entry in pack.get("content", []):
                entity = entry.get("entity")
                rel = entry.get("file")
                schema_name = ENTITY_SCHEMA.get(entity)
                if not schema_name:
                    errors.append(f"unknown entity {entity}")
                    continue
                path = CONTENT_DIR / rel
                if not path.exists():
                    errors.append(f"missing {rel}")
                    continue
                try:
                    validator_for(SCHEMA_DIR / schema_name).validate(load(path))
                except Exception as exc:
                    errors.append(f"{rel}: {exc}")
    if errors:
        for error in errors:
            print("FAIL:", error)
        return 1
    print(f"PASS: {len(schemas)} schemas parsed; base pack and {len(ENTITY_SCHEMA)} entity files validate")
    return 0

if __name__ == "__main__":
    sys.exit(main())
