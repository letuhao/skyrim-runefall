# Runefall content pack authoring

Use when shipping a Runefall content pack or third-party compatibility profile.

## Manifest format (format 1)
```json
{
  "packId": "unique",
  "plugin": "Runefall - Unique.esp",
  "format": 1,
  "minBaseVersion": "0.1.0",
  "contentFiles": ["data/unique_affixes.csv", "data/unique_sources.csv"],
  "recordCount": 40,
  "recordEditors": ["RF_MGEF_Unique_..."],
  "conflicts": []
}
```

## Rules
- Base plugin must be `Runefall.esp` and present.
- Pack plugin names must start with Runefall for built-in packs; `.esl/.esp/.esm` only.
- Format must be 1; mismatches are rejected, never silently loaded.
- minBaseVersion must be <= the shipped plugin version.
- Load order is last-loaded-wins; never patch vanilla records in place.
- Run `python tools/validate-pack-manifests.py build/dist/Data/Runefall/packs` before packaging.

## Compatibility profiles
Add a `compat_profiles` entity row for OWL/SPID/KID-style setups with requiredPlugins,
lootMode and protectVanillaLoot=true. Do not claim support for an untested list.
