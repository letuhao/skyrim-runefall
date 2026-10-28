# Runefall authoring skills

These project-local skills direct an autonomous agent to produce Runefall-valid content.
They are not runtime mod code and are not packaged into Data/.

- `runefall-content-authoring`: JSON-only entity authoring against content/schema/v1.
- `runefall-pack-authoring`: format-1 pack manifests and load-order-safe packaging.

Validation is mandatory before a pack is accepted:

```powershell
python content/schema/validate_content.py
python tools/validate-pack-manifests.py build/dist/Data/Runefall/packs
```
