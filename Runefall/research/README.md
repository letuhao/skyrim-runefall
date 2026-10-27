# Runefall — Research Index

Purpose: knowledge context and architecture notes extracted from reference
mods in `D:\Works\source\skyrim-moddings\rpg-mechanism\`. These are read-only
evidence realms; this folder is the writable research mirror for Runefall.

Use these notes to design Runefall features, including the planned in-game
web UI menu (Prisma UI) and the four pillars (progression, loot, enemy
tiers, crafting).

## Files

- `ui-web-menu.md` — UI stack: Prisma UI (web UI in-game), SKSE Menu Framework, HUD overlays, widgets, Prisma UI example plugin architecture.
- `loot-systems.md` — Loot generation, rarity/affixes, container distribution, Death Drop Overhaul, Dynamic Container Loot, Grid Inventory.
- `progression-attributes.md` — Actor values, attributes, perks, custom skills, Adaptive Attributes, Concentration Strain.
- `enemy-scaling-combat.md` — Dynamic spawns, enemy level variance, boss fights, combo system, equipment durability, KYE2.
- `crafting-economy-quests.md` — Spell crafting, crafting pulls, container distribution, quest UX mods, economy.
- `frameworks-infra.md` — Behavior data injection, dynamic persistent forms, Lexicon, PROTEUS, engine fixes, misc.

## Conventions

- Each doc: mod name + Nexus ID/version, file layout, key architecture, hooks/APIs,
  data formats, configuration, pitfalls, and "Relevance to Runefall".
- Source paths point back into `rpg-mechanism\...` (read-only).
- Nothing here is shipped; these are design inputs only.
