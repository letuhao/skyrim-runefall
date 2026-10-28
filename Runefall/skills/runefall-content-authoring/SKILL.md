# Runefall content authoring

Use when adding or changing Runefall gameplay data. All content is JSON-only; never author CSV
and never write FormIDs except official-master or unique-reference keys.

## Workflow
1. Read `content/base/pack.json` and the matching schema in `content/schema/v1`.
2. Add or edit the entity JSON under `content/base/data/`; register it in pack.json `content`.
3. Every item carries `schemaVersion: 1`; unknown fields are rejected by the fail-closed validator.
4. Run `python content/schema/validate_content.py`; do not continue on any failure.
5. Run the generator and static gates; commit the generated dist plus source JSON.

## Entity rules
- Stable keys only (EditorID/classId/bandId/profileId); no absolute FormID literals.
- Cross-entity references are keys, resolved at runtime by DataStore/FormCatalog.
- Stage/rank values are 1-100; stage bands keep min <= max.
- Protections use the 70% cap, 3%/stage, 5% floor; player/anomaly protection stays off.
- No CSV authoring; RuntimeJson is the only CSV->JSON bridge and deletes the CSV.

## Acceptance
Validator PASS, generator PASS, static gates 9/9, and the affected batch checks PASS.
