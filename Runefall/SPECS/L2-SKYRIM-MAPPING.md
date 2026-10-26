# L2 - Skyrim mapping

Status: draft 2026-10-26. Inputs: L1 gameplay model, L2A actor value coverage, D-020
(documented behavior, no engine re-investigation). This layer translates every L1 mechanic
into a Skyrim mechanism and lists what the engine will not do without risk.

Rules for this layer:

- Use existing, documented channels (vanilla effect/perk/AV patterns, established community
  frameworks) before inventing anything.
- No vanilla record edits. New forms stay in the ESL plugin; behavior stays in the DLL.
- Anything uncertain is a Phase 0 validation item, not an excuse to research the engine now.

## 1. Attribute and derived-stat mapping

General pattern: attributes are stored in the native co-save. A single "Runefall attribute
package" is applied as an ability (spell + magic effects) whose effects are conditioned on our
GLOBAL records and re-applied on load, level-up, gear change and respec. This is the same
documented channel vanilla blessings, standing stones and fortify enchantments use.

| L1 attribute | Effect | Mechanism | Notes |
|---|---|---|---|
| Might | melee damage | Skill modifier effects on OneHandedMod/TwoHandedMod (vanilla "Fortify One-Handed/Two-Handed" pattern) | falls back to perk entry "Modify Attack Damage" if the modifier channel is capped by other mods |
| Might | carry weight | Value modifier on CarryWeight | documented AV, used by vanilla effects |
| Might | stagger resistance | perk condition / native stagger hook if no documented AV applies | Phase 0 check |
| Might | gather yield | native hook on harvesting/ore interaction | no documented Papyrus surface; our DLL owns it |
| Endurance | health | Value modifier on Health | via ability |
| Endurance | armor | same channel as vanilla "Fortify Armor Rating" effects (perk entry or effect archetype; copy the vanilla pattern at L3 record level) | do not invent a formula |
| Endurance | block effectiveness | modifier on BlockMod / BlockPowerMod (vanilla fortify block pattern) | |
| Endurance | stamina and regen | Value modifier on Stamina, peak modifier on StaminaRate | documented vanilla pattern |
| Precision | ranged damage | modifier on MarksmanMod (fortify archery pattern) | |
| Precision | attack speed | perk entry "Modify Attack Speed" (documented perk entry point) | avoids unsafe direct weapon-speed AV writes |
| Precision | crafted item power / material efficiency | native crafting hook (see section 5) | |
| Agility | crit chance | modifier on CritChance AV (documented AV) | crit multiplication implemented natively if needed |
| Agility | attack speed | same as Precision channel (shared perk entry) | |
| Agility | evasion / movement / fall reduction | movement stays small and capped via SpeedMult ability if safe; evasion and fall reduction native hooks | SpeedMult is conflict-prone: cap low and make toggleable |
| Guile | lockpicking / pickpocket | modifier on LockpickingMod / PickPocketMod (vanilla fortify lockpick/pickpocket pattern) | skill values stay read-only |
| Guile | magic find / rune find | native loot engine values | |
| Guile | sneak detection | modifier on SneakMod (vanilla fortify sneak pattern) | |
| Cunning | vendor value | Speechcraft Mod / native price multiplier; documented vanilla barter effects first | |
| Cunning | poison potency | AlchemyMod (fortify alchemy pattern) or native poison hook | |
| Cunning | stealth damage | SneakPowerMod / native stealth multiplier | |
| Cunning | persuasion/intimidation | Speechcraft effect channel where documented; native check hook fallback | flagged in cut list if no clean channel |
| Intellect | magicka | Value modifier on Magicka | |
| Intellect | destruction magnitude | modifier on DestructionMod/PowerMod (fortify school pattern) | |
| Intellect | enchantment power | native enchantment hook or EnchantingMod (fortify enchanting pattern) | |
| Intellect | spell learning from books | Papyrus OnRead event on books, credited by native API | documented Papyrus event |
| Wisdom | restoration magnitude | modifier on RestorationMod/PowerMod | |
| Wisdom | ward/armor | alteration/ward modifiers | |
| Wisdom | magicka regen | peak modifier on MagickaRate | |
| Wisdom | potion effectiveness | hold: AlchemyMod increases crafted potion magnitude; consumed potion potency needs a native hook (flagged) | Phase 0 |
| Willpower | conjuration/illusion magnitude | ConjurationMod/IllusionMod | |
| Willpower | summon duration | native duration modifier in the cast path | |
| Willpower | cooldown reduction | native cooldown system | |
| Willpower | intimidation | native check hook (Speechcraft/Illusion flags as inputs) | Phase 0 |

Caps: every percent bonus is capped in INI; stacking is additive with diminishing returns, not
multiplicative. Attributes never write base actor values permanently; the ability package is
recomputed, so load order and uninstall stay clean.

## 2. Skill system mapping

- Cast model (decision D-021): **native hotkey layer** primary. The DLL binds skill keys,
  owns cooldowns/charges, casts the skill's spell/ability, and exposes the state to the UI.
  Fallback if the input layer is unstable: lesser-power casting (vanilla power slot) with the
  same cooldown state.
- Active skills: one SPEL per skill (three ranks upgrade the spell or swap it) in our ESL.
  Unlocking rank 1 grants the spell via the documented AddSpell path; respec removes it.
- Passive skills: PERK records with conditions on our GLOBAL state; granted by AddPerk and
  removed on respec.
- Rank gates: read the 18 Skyrim skill values and the paired attribute (read-only) when the
  player spends skill points; no skill AV writes.
- Skill point storage, ranks and cooldown state: native co-save (section 6).
- UI: character sheet lists 18 lines, ranks, gates and spend/refund buttons (section 7).

## 3. Loot mapping - dynamic generation is mandatory

Design rule: item instances are generated at runtime. There is no record per variant and no
combinatorial pre-generation. Five enhancements across ~20 equipment slots and six rarity
bands is millions of combinations; none of it becomes forms.

- Drop tables are **data** (CSV/JSON), evaluated by the DLL at drop time. They are never
  records and never leveled-list edits.
- Roll point: container/corpse/vendor/quest-reward generation, before the player sees the
  contents (the lifecycle DiabloRim hooks locally; our rules are ours).
- Roll pipeline: rarity -> ilvl -> enhancement count = rarity -> categories -> magnitudes ->
  sockets -> name (L1 section 6).
- Instance mutation, using documented channels verified in the installed 1.6.1170 sources:
  - Dynamic enchantment: SKSE exposes CreateEnchantment / SetEnchantment / GetEnchantment on
    ObjectReference and the WornObject equivalents for worn items (SKSE additions built
    2024-01-17). Worlds Dawn's loot engine uses WornObject.CreateEnchantment +
    ChangeDisplayName + SetItemHealthPercent from Papyrus; DiabloRim performs the same class of
    instance mutation natively and serializes its rolled state in its own co-save.
  - Display name: ObjectReference/WornObject SetDisplayName - the persisted per-instance name
    carrying rarity and enhancement prefix/suffix.
  - Tempering: SetItemHealthPercent.
  - Metadata that cannot be encoded in the enchantment or name (rarity id, item level, socket
    contents) lives in the co-save registry keyed by stable item identity. The enriched item
    instance is the product; no form is created for it.
- Container contents are not object references. The DLL applies instance data through the
  native inventory-entry path, with the Worlds Dawn-proven dummy-actor approach as fallback for
  the container case. Phase 0 selects the working path; both keep the loot dynamic.
- Pity/dry-streak state and container roll state: co-save.
- Anti-abuse: container roll persisted per container instance (ref FormID, reset on cell reset);
  boss drops rolled at death and stored. Best effort; documented limitation if not perfect.
- Loot authority modes (additive/hybrid/takeover) are a runtime config switch in the same hook,
  so OWL and other loot mods keep their records intact.
- **General drop table (G11):** every enemy kill resolves the baseline table by rank x RF tier
  (item chance/count, rarity bias, rune yield, reserved unique chance). Data only; no records.
- **Race hunt tables (G12):** the DLL reads the actor's base race FormKey, resolves its EditorID,
  maps it to a hunt family from race_groups.csv, and rolls that family's reward rows. The unique
  affix is first-kill-only (per player/family flag in co-save); the hunt material is repeatable
  with a chance that scales by RF tier. Hunt packs are optional; without the pack the base logs
  and skips race rewards.
- Race detection uses vanilla RACE records only; vampire/werewolf variants are their own races.
  No NPC records are edited.

## 4. Enemy tier mapping

- Rank assignment: native spawn hook if available (decision pending Phase 0), else load-scan
  fallback: when an actor loads, the DLL assigns a rank from area tier + deterministic roll.
  No NPC record edits.
- Buff package: abilities/perks from our ESL applied at spawn/load with caps; rank marker
  keyword for other systems. Application must be idempotent (one rank per actor).
- Monster affixes: our MGEF/SPEL records with conditions; 1-4 per rank as L1; visible rank
  badge through the UI module (per-instance actor names are not a reliable engine feature).
- Bosses: hand-authored boss templates in our ESL, spawned/replaced by the native tier engine
  at fixed sites; no leveled-list edits. Attunement ritual runs through a Papyrus script and
  the native API.
- Save safety: prefer runtime-applied effects re-derived on load over permanent perk/ability
  grants to generic NPCs; validate in Phase 0.

## 5. Rune and crafting mapping

- Rune item: five MISC records (R1-R5) in the ESL, weightless, stacking, visible; native
  counter cache for fast totals. Pickup and stacking are documented item behavior.
- Materials: MISC records per tier/family in the ESL; node/creature sources are native hooks.
- Salvage/craft/reroll/socket: native API invoked from a Papyrus forge activator script; the
  DLL validates the transaction (item metadata, rune count, materials).
- **v1 core recipes:** only rune conversion and socket crafting are deterministic COBJ records
  (about 6-8). Reroll and rarity upgrade go through the native API, not COBJ, because COBJ
  cannot express their rules.
- **Crafting Table expansion (reserved):** deterministic base-item crafting, targeted affix
  crafting, Affix Shards, material recipe chains and the crafting UI are deferred to
  Runefall - Crafting.esp (D-045). v1 must not require them; hunt materials feed Rune Forge
  reroll/upgrade costs in v1.
- Crafting efficiency anchors (Precision) hook the crafting menu creation path.
- Potion effectiveness (Wisdom) is flagged: crafted-potency through AlchemyMod is documented;
  consumed-potion potency needs a native hook or is cut in v1.
- Rune conversion (5:1) is a native API + UI action.

## 6. Save and uninstall

- Co-save layout (versioned header): item metadata map, actor rank map, skill/attribute state,
  rune and pity counters, container roll state, config snapshot.
- Save-time write, load-time rebuild, revert-time clear. Schema changes are additive only.
- Uninstall: removing DLL + ESP leaves items without our metadata (fall back to base forms) and
  no permanent record changes; runtime effects disappear on load. Enchantment/name extra data
  applied to items may persist as ordinary item state - documented and tested in Phase 0.
- Avoid permanent per-NPC perk grants; if unavoidable for bosses, document the save impact.

## 7. UI surfaces (decision D-022: SKSE Menu Framework for v1)

| Surface | Content | Implementation |
|---|---|---|
| Character sheet | 9 attributes, 18 skills, ranks, gates, points, respec | SKSE Menu Framework page, data from native API |
| Rune Forge | rune counts, salvage list, craft/reroll/socket, materials | Menu Framework page + Papyrus activator |
| Item tooltip | rarity, enhancements, sockets, item power | native tooltip hook or Menu Framework detail panel |
| HUD (optional) | rune total, cooldowns | v2 module; not required for vertical slice |
| Notifications | rank-up, drops, attunement | documented message/notification channels |

Prisma UI is deferred to an optional v2 module. MCM Helper remains the fallback if the menu
framework misbehaves in Phase 0.

## 8. Native vs Papyrus split and budgets

| Function | Where |
|---|---|
| Loot roll, item metadata, tier assignment, salvage/craft transactions, cooldowns, hotkeys, co-save | DLL |
| Menu pages, text, notifications, book-reading effects, forge activator flow | Papyrus + framework |
| Record generation (ESL content, ENCH/MGEF/SPEL/PERK/COBJ/MISC) | Mutagen build tooling |
| Config | INI + CSV/JSON data files |

Budgets: no per-frame work except input polling; container hook O(contents); spawn hook O(1);
co-save growth target < 1 KB per 100 tracked items; all caps configurable.

## 9. Cut / reshape list - audited 2026-10-26

Dynamic loot is **not** on this list. It is a required mechanic, proven feasible by both
reference mods (DiabloRim natively, Worlds Dawn through SKSE natives), and the earlier
"pre-generated unique records" fallback was wrong for a combinatorial item space and has been
removed.

| Item | Verdict | Reason |
|---|---|---|
| Dynamic loot and drop tables | **Not cut - required** | Millions of combinations cannot be forms. Runtime instance generation with dynamic enchantment, display name and tempering is the design; drop tables are CSV/JSON data. |
| Per-instance actor names for monster affixes | Implement attempt, not cut | SKSE exposes ObjectReference.SetDisplayName (same family used for item names). If the actor health bar ignores it, use the UI rank badge for presentation only, not as a gameplay cut. |
| Consumed-potion potency (Wisdom) | Deferred, keep as risk | No documented clean channel; crafted-potion potency works through AlchemyMod. Optional native hook, not part of loot. |
| Persuasion / intimidation (Cunning/Willpower) | Implement | Documented perk entry "Mod Player Intimidation" plus a native check hook where needed; fall back to vendor value only if Phase 0 disproves both. |
| Save-scum-proof boss drops | Best effort | Persistence limits documented; not a loot-generation cut. |
| Movement-speed bonuses (Agility) | Capped, toggleable | SpeedMult is physics-sensitive; ship small caps and a toggle rather than cutting the attribute anchor. |
| Smart drops (optional module) | May ship disabled | Pure UX module; the only feature allowed to be cut from v1 if hook cost is high. |
| Pre-generated item variant fallback | **Removed** | Contradicts the combinatorial requirement; dynamic generation is mandatory. |

## 10. Decisions recorded by this layer

- D-021 Active skill cast model: native hotkey layer primary; lesser-power fallback.
- D-022 UI: SKSE Menu Framework for v1; Prisma UI optional v2; MCM Helper fallback.
- D-023 Dynamic per-item generation is mandatory: instance enchantment + display name +
  tempering + co-save registry; no pre-generated variant fallback.
- D-024 Loot roll at container generation with persisted container state; no leveled-list edits.
- D-025 Enemy ranks: spawn hook primary, load-scan fallback; no NPC record edits.
- D-026 Boss content: native spawn replacement at fixed sites; no leveled-list edits.
- D-029 Drop tables are data (CSV/JSON) evaluated at runtime; no record-per-variant generation.
- D-030 Container-item instance mutation: native inventory-entry path first, dummy-actor path
  (Worlds Dawn-proven) as fallback; Phase 0 selects.

## 11. Phase 0 validation items produced here

1. Container-open hook fires before contents are displayed; can roll and persist per container.
2. Dynamic generation at breadth: 1,000 rolled items across all six rarities, enhancement counts
   and socket counts; no form is created per variant; database/load growth remains flat.
3. Instance mutation persistence: dynamic enchantment (CreateEnchantment/SetEnchantment),
   display name (SetDisplayName) and tempering (SetItemHealthPercent) survive save/load and
   keep working after uninstall; no orphaned records.
4. Container path: native inventory-entry mutation works for items inside a container; if not,
   the dummy-actor fallback produces the same result.
5. Spawn hook or load-scan assigns exactly one rank per actor and survives cell reload.
6. AddSpell/AddPerk grants and removal are clean on existing saves (respec test).
7. SKSE Menu Framework page registers, reads native state and renders.
8. Native input hotkeys fire in gameplay and are rebindable.
9. Tooltip/name display of dynamic items works (rarity, enhancements, sockets).
10. Co-save schema writes/reads/reverts with version bump.
11. Multi-runtime build loads on 1.6.1170 (supported target); 1.7.104 deferred (D-062).
12. No per-frame cost beyond input polling; measured hook times recorded.

## 12. Dev/agent tooling used for L3+

- Mutagen (NuGet, current release) for record generation and validation - already proven in
  esp-explorer; no custom AV tooling needed (D-020).
- Synthesis app (local) for patch-themed testing of generated records.
- PapyrusCompiler from the MO2 Stock Game for scripts.
- Optional agent tools if needed later: houseCARL (load-order data layer/MCP) for record QA and
  patch inspection; SkyrimForge (typed automation MCP) for guarded record jobs. Neither is a
  runtime dependency of the mod.

## 13. Reference implementation: PROTEUS 3.4.0

Analysed in _context\rpg-mechanism\10-PROTEUS.md. It is a mature runtime actor/record editor
and the best local evidence for the mechanisms this layer maps.

What it proves:

- SKSE itself exposes native Papyrus record-edit methods for item/weapon fields (verified in
  the installed 1.6.1170 sources: Armor.SetArmorRating, Armor.SetWeightClass, Form.SetWeight,
  Form.SetGoldValue, Weapon.SetSpeed/SetReach/SetStagger/SetCritDamage/SetCritMultiplier,
  Game.ShowRaceMenu). A DLL is not required for base-record stat edits.
- Runtime keyword mutation of shared form records is practical through a native bridge
  (ProteusAddKeywordToForm / RemoveKeywordOnForm / ReplaceKeywordOnForm).
- Runtime actor edits (sex, level, race menu, hair color, bulk perk/spell grant-remove) are
  practical.
- Base-record edits are **session scoped and shared**: PROTEUS keeps durable intent in
  JContainers JSON presets (including *_ACROSS_ALL_SAVES files) and re-applies them; its own
  DLL comment warns that some edits leak across saves within a session.
- A native bridge with bulk queries and caches is the right shape for a menu-heavy mod.

Consequences for Runefall (already folded into the sections above):

1. Per-instance rarity/enhancements cannot be represented by base-record edits. D-023
   (extra data + co-save) is confirmed as the correct route.
2. We never mutate vanilla/shared records; keyword mutation applies only to our own forms.
   New D-027.
3. Persistence is co-save-first. External CSV/JSON/INI are content and config, never
   authoritative per-instance state. New D-028.
4. The native bridge should expose bulk query + cache functions for the character sheet and
   Rune Forge, and small mutation functions for our own state - not Papyrus loops over forms.
5. We do not adopt PROTEUS's dependency stack (JContainers, PO3 Papyrus Extender, UILib) or
   its custom Flash UI; SKSE Menu Framework remains the v1 choice.
6. Its 39 patch ESPs are the reference model for our L5 compatibility patches.
