# Runefall — Nexus Mods page

## Short description

A Torchlight/Diablo-style loot and power overhaul for Skyrim SE/AE 1.6.1170: six rarity bands,
600 affixes, rune tiers, RuneForge crafting, race hunt rewards, and skill/attribute progression —
applied at runtime with a native SKSE DLL and ESL content.

## Requirements

- Skyrim SE/AE **1.6.1170** only (1.7.x is not supported in v1)
- SKSE64 **1.6.1170**
- Address Library for SKSE Plugins (**1.6.1170** bins)
- SKSE Menu Framework **3.18 release** (Nexus 120352) for the Mod Control Panel pages
- No SkyUI, MCM Helper, JContainers, PapyrusUtil, PO3 Extender or UILib dependency

## Installation

1. Install with MO2/Vortex; the FOMOD installs Core and offers optional **Unique** and **Hunt**
   packs.
2. Ensure Runefall.esp, Runefall - Unique.esp and Runefall - Hunt.esp load after Skyrim.esm and
   after USSEP if present.
3. Start a new game or load a clean save made without Runefall; the co-save stores progression
   and hunt state.

## Features (v1)

- Six rarity bands and 600 affixes generated from data into runtime enchantments.
- Death-drop loot with rarity/rank/tier rolls and named enchanted items.
- Race-family first kills grant material and a unique enchanted item.
- Rune tiers, RuneForge spending, attributes/skills and native attunement.
- Co-save schema for progression, runes, item registry and hunt first-kill state.
- Debug hotkeys F10 snapshot, F11 self-tests, F12 probe item; F1 opens the Mod Control Panel.
- Papyrus API for tier, attributes, skills, runes, first kills, snapshots and attunement.

## Known limitations (0.1.0)

- Runtime loot and hunt paths are verified; menu page interaction and Papyrus runtime calls still
  need a full manual pass.
- Boss-kill gating for attunement is not implemented; attunement spends runes only.
- Enemy variants/monster affixes, extra hooks and the full compatibility matrix are incomplete.
- Only Skyrim 1.6.1170 is tested; 1.7.x/SE/VR are unsupported in v1.
- Batch harness core checks pass; synthetic keyboard input is a known harness limitation.

## AI disclosure

Parts of this build (content-generation scripts, validation harness, docs and implementation
scaffolding) were developed with AI coding assistance. Shipped binaries are built from the
repository source; no third-party game assets are redistributed.

## Credits

- CommonLibSSE-NG (alandtse fork) — MIT
- SKSE team — SKSE64
- Mutagen — plugin generation
- SKSE Menu Framework (Thiago099) — menu API
- Bethesda Game Studios — Skyrim SE/AE (no assets redistributed)
