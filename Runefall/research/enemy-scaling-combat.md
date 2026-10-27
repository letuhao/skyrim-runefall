# Enemy Scaling & Combat Research

Evidence extracted from reference mods under `D:\Works\source\skyrim-moddings\rpg-mechanism\` (read-only). Paths below are relative to that root.

Runefall enemy pillar (GAME-DESIGN §4): enemy tiers Elite → Champion → Boss → Legendary → Mythic, runtime assignment (no leveled-list edits), buff packages, damage protection, stage-gap. This note maps reference mods to those needs.

## 1. Dynamic Enemy Spawns SKSE (Nexus 178556, v3.4)

Path: `Dynamic Enemy Spawns SKSE 178556 3.4 2026-09-29T14-44Z 2Bej58NL8/`
Source: `.../Source And PDB Files 178556 3.4 2026-09-29T14-44Z HZCMOyR1r/Source and PDB files for both/`

Files:
- Mod: `Dynamic Enemy Spawns SKSE/SKSE/Plugins/AutoEnemySpawn.dll`, `AutoEnemySpawn.ini`, `AutoEnemySpawn_Translation.ini`
- Source: `src/{SpawnManager.cpp,h,Settings.cpp,h,UI.cpp,h,SaveHook.h,Strings.*}`

Architecture:
- SKSE plugin listens to `TESCellFullyLoadedEvent`, `TESLoadGameEvent`, `TESContainerChangedEvent`, `TESDeathEvent`, `TESActivateEvent`.
- On cell load, scans hostile actors in cell, caches by EditorID into `_npcCache` (TESNPC*), excludes unique/no-respawn from spawn pool unless allowed.
- Computes spawn count from `enemySpawnMultiplierMin/Max` (rolled) × base count, with `numFollowerMultiplier` for follower count, capped by `enemyLimitInterior` (35) for interiors and `enemyMultiplierExteriorMax` (3.0) for exteriors; `maxExteriorSpawnRadius`.
- Places clones via `TESDataHandler::CreateReferenceAtLocation(baseNPC, pos/rot, parentCell, worldspace)` in batches of `spawnsPerBatch` (2) with `batchDelayMs` (150ms) through `SKSE::GetTaskInterface()->AddTask`; stale-session guard drops batches after load transition.
- Tracks spawned actors in `_spawnedActors` (ActorHandle → base FormID) for loot/death/cleanup; can transport dead spawns to cleanup cell; blocks activation of spawned dead bodies.
- `copyEquipmentFromOriginal` (default true) copies armor/weapons from source actor to clone via `EquipManager::EquipObject` after checking clone did not already have armor/weapon.
- Config keys: `enemySpawnMultiplierMin/Max`, `numFollowerMultiplier`, `enemyMultiplierExteriorMax`, `maxExteriorSpawnRadius`, `enemyLimitInterior`, `spawnsPerBatch`, `batchDelayMs`, `allowDuplicatingUniqueEnemies`, `allowDuplicatingNoRespawnEnemies`, `enableCorpseLooting`, `transportDeadSpawnsToCleanupCell`, `copyEquipmentFromOriginal`, `enemyBlacklist`, `forceCountedFollowers`, `debug`.

Hooks/APIs:
- Event sinks on cell/load/container/death/activate; task interface for batched main-thread placement; save-hook for load transition snapshot.

Pitfalls:
- Clones use base NPC forms; leveled creature ExtraLeveledCreature resolution needed for stable base; FF-prefix (0xFF) cached forms skipped.
- Interior cap prevents runaway spawns; exterior multiplier can explode density—tune `enemyMultiplierExteriorMax`.
- Equipment copy only armor/weapons, not spells/perks; clones do not inherit source level variance.
- Spawned actors must be tracked to avoid orphaned cleanup and false loot.

Relevance to Runefall:
- Runtime spawn-density layer can reuse this pattern for “Elite+” waves in a cell without ESP edits. Track spawned actors by tier in the same map to apply tier buff packages after placement.
- Batched placement with session guards is a safe template for any main-thread actor injection.
- Equipment copy can be replaced/augmented by Runefall tier loadouts (armor sets by tier) instead of cloning source gear.

## 2. EnemyLevelVariance (Nexus 170530, v2.5.0)

Path: `EnemyLevelVariance-170530-2-5-0-1770806888/`

Files:
- `SKSE/Plugins/EnemyLevelVariance.dll`, `.ini`, `.pdb`
- No source shipped (only binary + pdb).

Config (`EnemyLevelVariance.ini`):
- `[Main]`: `bEnabled=1`, `bClampEncounterZone=1`, `bUsePlayerMinFloor=0`, `bLowLevelPlayerProtection=0`, `bClampEncounterZoneMax=1`, `iDeltaUpMax=10`, `iDeltaDownMax=10`.

Architecture (inferred from config + Nexus description):
- SKSE plugin applies configurable virtual-level variance to hostile enemies and scales stats accordingly.
- Encounter zone clamping both directions with max up/down delta; optional player-min floor and low-level player protection.
- In-game tuning via SKSE Menu Framework MCP or INI.

Pitfalls:
- Binary-only; verify compatibility with installed SKSE/runtime; no source to audit.
- Virtual level changes can desync leveled lists/encounter zones if other mods also clamp.

Relevance to Runefall:
- Direct model for Runefall’s “stage-gap” (tier power steps) and low-level protection: clamp enemy virtual level to player ± tier delta instead of editing NPC levels.
- Can combine with Runefall tier assignment: set variance range per tier (Elite ±2, Champion ±4, Boss ±6, etc.) rather than one global delta.

## 3. Ultimate Boss Fight (folder)

Path: `Ultimate Boss Fight/` (collection, no single framework)

Contents:
- Boss asset packs: Bone Colossus, Dwarven Automatons Pack, Rebirth Monster, The Dragon Apostate, TsunDal Boss Overhaul, Ultimate Boss Fight Mod, Unique Boss Fights, Vyrthur Boss Fight Overhaul.
- Vyrthur pack ships DAR animation replacer conditions (`meshes/actors/character/animations/DynamicAnimationReplacer/_CustomConditions/...`) and `SKSE/PayloadInterpreter/Config/Virthur Animation - Payload.ini`.

Architecture:
- Asset/animation-driven boss overhauls; no central scaling code. Boss identity comes from unique NPCs, stats, spells, and animation sets.
- DAR conditions gate boss-only attack animations by FormID/condition.

Pitfalls:
- Large asset surface; merging multiple boss packs can conflict on animation replacer condition IDs and sound records.
- No runtime tier assignment; boss-ness is hard-coded per NPC.

Relevance to Runefall:
- Use as content library for Mythic/Legendary boss encounters, but implement tiering in Runefall’s runtime assignment (tag boss NPCs) rather than relying on pack identity.
- Animation condition IDs are a compatibility risk if Runefall also uses DAR/OAR; coordinate condition ranges.

## 4. Combo System (Nexus 181633, v1.0.0)

Path: `Combo System 181633 1.0.0 2026-10-02T14-40Z Z3mWyIHEt/`

Files:
- `SKSE/Plugins/ComboSystem.dll`
- `SKSE/Plugins/BehaviorDataInjector/ComboSystem_BDI.json`
- `PrismaUI/views/ComboSystem/` (UI), `Viny Mods/Combo System/Language.json`

Architecture:
- SKSE + BDI injects actor variables/events: `HitValueCMF`, `ComboValueCMF`, `ComboPointsCMF`, `TierComboPointsCMF`, events `MCO_EndAnimation`, `TierAdvanceCMF`, `TierReduceCMF`, `TierGot*CMF` (F/E/D/C/B/A/S/SS/SSS/Z).
- PrismaUI menu for configuration; combat feedback overlays.

Pitfalls:
- Requires Behavior Data Injector and compatible animation framework; tier events are combo-rank tiers, not enemy tiers.
- Not a damage/scaling system.

Relevance to Runefall:
- Optional combat-feedback layer: combo ranks can feed Runefall buff packages (e.g., hit-streak buffs) but should remain decoupled from enemy tier assignment.
- BDI pattern useful if Runefall needs custom actor variables for tier state.

## 5. Equipment Durability System NG (Nexus 109899, v5.1)

Path: `EquipmentDurabilitySystem NG 109899 5.1 2026-09-03T06-00Z pQ0AZ3juw/`

Files:
- `SKSE/Plugins/EquipmentDurabilitySystem-NG.dll`
- `SKSE/Plugins/EquipmentDurability/EquipmentLists.ini`, `EquipmentTranslation.json`
- `Scripts/Source/EquipmentDurability.psc` (SkyUI groups fix variant)

Architecture:
- SKSE tracks durability of equipped weapons/armor by material and enchantment; degrades on use/hits.
- `EquipmentLists.ini` maps material keywords (Fur/Leather/Iron/Steel/.../Daedric) and enchantments (vanilla + Thaumaturgy + Summermyst) to durability values and repair materials.
- Translation file localizes UI strings; SkyUI menu for durability bars.

Pitfalls:
- Large INI surface; modded items need material/enchant entries or default durability.
- Conflicts with other hit/damage hooks (e.g., Calamity, KYE2) if multiple systems modify incoming damage.

Relevance to Runefall:
- If Runefall wants enemy equipment to degrade (or tiered enemies to have “unbreakable” gear), integrate via this material/enchant mapping rather than custom damage math.
- Durability can be a secondary “damage protection” knob: Mythic enemies with high-durability gear vs player; but keep separate from core tier damage reduction to avoid double-dipping.

## 6. Know Your Enemy 2 (Nexus 93258, v2.1.0) + Armors (Nexus 94067, v1.0.3)

Path:
- `Know Your Enemy 2-93258-2-1-0-1766976489/`
- `Know Your Enemy 2 - Armors-94067-1-0-3-1765636540/`

Files:
- Main: `know_your_enemy2.esp`, `know_your_enemy_2_DISTR.ini`, `know_your_enemy_2_KID.ini`, scripts `kye2_{init,inspect,mcm,unarmed_fix}_script.psc`.
- Armors: `know_your_enemy_2_armors.esp`, `..._DISTR.ini`, `..._KID.ini`, scripts `kye2a_*`.

Architecture:
- SPID `_DISTR.ini` assigns perks/spells/keywords to NPCs by race and filters (`-ActorTypeGhost`, etc.): e.g. `Perk = kye2_animal_perk|...|BearBrownRace,...`; `Spell = kye2_troll_ab|...`; intelligence tiers (low/moderate/high).
- Perk/spell packages encode resistances, weaknesses, damage behaviors by creature family (animal, beast, undead, daedra, etc.); unarmed fix script.
- Armors KID tags armor materials: `kye2a_heavy_cuirass`/`light_cuirass`/`robes`, `kye2a_leather`/`metal`/`nonconductive`, weapon type keywords, drain magic effect keyword.
- These keywords drive damage protection/weakness interactions (e.g., nonconductive vs metal vs leather).

Pitfalls:
- Huge race/keyword matrix; adding new races or mods requires updating DISTR/KID.
- Perk assignment is load-order sensitive; duplicates can stack.

Relevance to Runefall:
- Best template for Runefall “buff packages”: assign tier packages by NPC tag/race using SPID-style filters instead of editing NPCs.
- Damage protection: material keywords (leather/metal/nonconductive) can map to tier resistances (e.g., Elite+ metal armor = physical reduction; nonconductive = shock resistance).
- Stage-gap: KYE2’s intelligence/creature tiers (low/moderate/high) parallel Runefall’s tier ladder; reuse perk-package pattern per tier.

## 7. Calamity (Nexus 193193, v2.3.0, MO2/Vortex)

Path: `Calamity 2.3.0 (MO2 Vortex) 193193 2.3.0 2026-10-27T07-47Z VMSnJrVTn/` (extracted to temp for inspection)

Files:
- `CalamityAffixes.esp`, `CalamityAffixes_DISTR.ini`, `CalamityAffixes_KID.ini`
- `SKSE/Plugins/CalamityAffixes.dll`, `affixes.json`, `runtime_contract.json`, `InventoryInjector/CalamityAffixes.json`
- Scripts: `CalamityAffixes_{AffixManager,AffixEffectBase,ModeControl,ModEventEmitter,PlayerAlias,SkseBridge,MCMConfig}.psc`

Architecture:
- Diablo-like affix system: affixes rolled per item instance at runtime by SKSE plugin; `affixes.json` defines triggers (Hit, IncomingHit, DotApply, Kill, LowHealth), actions (CastSpell, ConvertDamage, CorpseExplosion, SpawnTrap, etc.), loot weights/chances, boss container multipliers.
- `runtime_contract.json` declares supported triggers/action types and runeword catalog.
- SKSE→Papyrus bridge emits `CalamityAffixes_Hit` / `CalamityAffixes_DotApply` mod events; `AffixManager.ProcessOutgoingHit` handles proc budgeting (ICD, proc budget per window).
- KID only tags magic effects (`CAFF_TAG_DOT`) for DoT detection; affix keywords are instance-rolled, not distributed.
- Loot config: `chancePercent`, `bossContainerEditorIdAllowContains`, source multipliers, name format `{base}[{affix}]`.

Pitfalls:
- Heavy runtime proc budgeting; overlapping with KYE2/EDS damage hooks can compound.
- Instance-rolled affixes need cleanup of legacy/invalid affixes on load.

Relevance to Runefall:
- Model for Runefall’s damage-protection/trigger layer: define tier buff packages as “affix-like” triggers/actions (e.g., Elite: thorns on hit; Mythic: corpse explosion) via SKSE→Papyrus bridge.
- Boss-container detection pattern can identify Mythic loot sources.
- Keep Runefall’s tier effects additive and budgeted (ICD, proc budget) to avoid performance issues.

## 8. Concentration Strain (Nexus 189317, v1)

Path: `Concentration Strain 189317 1 2026-08-23T03-29Z kATsMWjXx/`
Source: `.../Src 189317 1 2026-08-23T03-33Z Dc5uGkCfW/src/`

Files:
- `ConcentrationStrain/SKSE/Plugins/ConcentrationStrain.dll`
- Source: `Plugin.cpp`, `Settings.cpp/h`, `UI.cpp/h`, `Utils.h`, `PCH.h`

Architecture:
- SKSE input event sink tracks held cast buttons for concentration spells; interrupts cast after `InterruptSeconds` (default 12, range 1–60) from `Data/SKSE/Plugins/ConcentrationStrain.ini`.
- Lockout mode suppresses auto-restart until button release.

Pitfalls:
- Player-only input handling; not enemy scaling.
- Simple single-value config; no tier integration.

Relevance to Runefall:
- Low relevance to enemy scaling; useful only if Runefall adds concentration-spell limits for tiered casters. Otherwise skip.

## 9. Dealing With Ysolda (Nexus 194285, v2)

Path: `Dealing With Ysolda 194285 2 2026-10-07T21-48Z Dc5uGkoFM/`

Files:
- `DealingWithYsolda.esp`, `Readme.txt`, `LICENSE.txt`, scripts `DealingWithYsolda*.pex`, `SEQ/DealingWithYsolda.seq`, FaceGen.

Architecture:
- Quest/NPC overhaul for Ysolda; dialogue, thug, poisoned partnership, vent, witness. No combat scaling.

Pitfalls:
- None relevant.

Relevance to Runefall:
- Not relevant to enemy scaling/combat; include only as a non-combat reference. Skip for this pillar.

## Relevance to Runefall — Enemy Tiers & Combat

- **Runtime assignment**: Follow Dynamic Enemy Spawns’ cell-load scanning + SKSE event sinks and batched placement; tag spawned/assigned actors with Runefall tier in a sidecar map (ActorHandle → tier). No leveled-list edits; use SkyPatcher `addToLLs` only if tiered NPCs must appear in leveled lists (Fortunate Foes pattern).
- **Level variance / stage-gap**: Use EnemyLevelVariance’s clamp model (player ± delta, encounter zone clamping, low-level protection) with per-tier deltas instead of global variance.
- **Buff packages**: Use KYE2’s SPID-style perk/spell/keyword distribution by race/tag to apply tier packages (resistances, weaknesses, thorns, auras) without editing NPCs. Material keywords (leather/metal/nonconductive) map to tier damage protection.
- **Damage protection**: Combine KYE2 material keywords + Equipment Durability material mapping; keep tier damage reduction separate from durability to avoid double-dipping. Budget triggers like Calamity (ICD, proc budget).
- **Boss handling**: Ultimate Boss Fight provides content (unique bosses, animations) but tiering should be runtime-tagged; coordinate DAR/OAR condition IDs.
- **Spawn density**: Dynamic Enemy Spawns’ multiplier/follower scaling + interior caps can drive tiered wave density; copy equipment or replace with tier loadouts.
- **Compatibility**: Binary-only mods (EnemyLevelVariance) and input mods (Concentration Strain) are optional; Calamity/KYE2/EDS all hook damage—sequence carefully and document load order. Combo System tier events are unrelated (combo ranks), keep decoupled.