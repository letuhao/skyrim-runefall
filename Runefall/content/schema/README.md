# Runefall JSON content schema

Status: F0.2 draft, 2026-10-28. This is the authoritative content schema for agent-authored packs,
offline compilation and runtime fallback.

## Layout

    content/
      schema/
        v1/                 JSON Schema files, version 1 namespace
        validate_content.py schema + base pack validator
      base/
        pack.json           base content manifest
        data/               entity JSON files

## Rules

1. JSON only. CSV is not an authored format.
2. Content keys are stable: rf:<entity-type>/<slug>.
3. FormIDs are not content references; use editorId/masterForm references only where allowed.
4. Every file declares schemaVersion.
5. A pack manifest lists its entity files; the validator resolves and checks them.
6. Unknown or invalid content fails the pack, not the game.

## Commands

    python content/schema/validate_content.py

## Versioning

- Schema files are immutable within a version directory; breaking changes create v2.
- Pack manifests declare schemaVersion; the loader rejects unsupported versions.
- Entity renames require a migration mapping; generated FormIDs are never authored.
