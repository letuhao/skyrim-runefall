# Runefall — Research Index

Purpose: knowledge context and architecture notes extracted from reference
mods in `D:\Works\source\skyrim-moddings\rpg-mechanism\`. These are read-only
evidence realms; this folder is the writable research mirror for Runefall.

Use these notes to design Runefall features, including the planned in-game
web UI menu (Prisma UI) and the four pillars (progression, loot, enemy
tiers, crafting).

## Files

- `isekai-hero-skyrim.md` — **DEEP DIVE** of IsekaiHero (full C++ source, Prisma view, ModAPI, professions/progression/shop/skill tree/quests, crafting hooks, co-save, MCM, packaging).
- `codexofpowerng.md` — **DEEP DIVE** of CodexOfPowerNG (build/progression system: registration, build options/effects, inventory integration, rewards sync, undo, co-save serialization, Prisma UI).
- `calamity-reactive-loot-affixes.md` — **DEEP DIVE** of Calamity (reactive affix engine: JSON affix catalog + modules, 4-layer arch: native EventBridge/7 sinks, C# generator, Prisma UI panel, Papyrus; trigger pipeline health-damage/hit/DoT; instance storage & ExtraUniqueID serialization; KID/SPID; VFX).
- `adaptive-attributes-skse.md` — **DEEP DIVE** of Adaptive Attributes SKSE (native progression: XP/milestones/caps/baseline/reconciliation, native hooks, event sinks, form registry, settings, co-save schema v7, Papyrus bridge, VR, OStim).
- `dynamic-enemy-spawns-skse.md` — **DEEP DIVE** of Dynamic Enemy Spawns SKSE (spawn system: ScanManager, SpawnManager, safety filters, encounter identity, reward dedup, cleanup/disengage, equipment copy, furniture placement, UI).
- `container-distribution-framework.md` — **DEEP DIVE** of CDF (JSON distribution grammar, runtime hook on TESObjectREFR::Initialize/Reset, conditions, change types, safety gating, compatibility with SPID/KID/FLM).
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
