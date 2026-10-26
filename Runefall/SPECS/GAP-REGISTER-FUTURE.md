# Future gap register and scope reservation

Status: 2026-10-26. v1 scope-lock document. Purpose: brainstorm every gap deliberately reserved
for a future expansion, separate them from gaps that must be covered now (cheap hooks), and flag
risks to watch. Nothing here blocks v1 unless marked NOW.

Decision codes:

- **NOW** - must be designed or hooked in v1; retrofitting later is expensive or save-breaking.
- **RESERVE** - deliberately out of v1; assigned to a future expansion pack or release.
- **WATCH** - not a feature gap, but a risk to measure during v1 and revisit after release.

## 1. Reserved gameplay systems (future expansion packs)

| ID | Gap / opportunity | Why reserve | Goes to | Retrofit cost |
|---|---|---|---|---|
| F-01 | Crafting Table: base-item recipes, targeted affix crafting, Affix Shards, recipe chains, craft UI | Already accepted as expansion (D-045); v1 is hunting-first | Runefall - Crafting.esp | High |
| F-02 | Affix transfer/extraction: move an enhancement between items, fuse two affixes | Needs item metadata surgery and balance; v1 reroll is enough | Crafting II | Med |
| F-03 | Rune families: elemental/archetype runes with socket colors and set affinities (v1 runes are rank-only) | Multiplies content and UI; v1 keeps one readable resource | Rune pack | High |
| F-04 | Item sets and set bonuses (Diablo/Torchlight set identity) | Needs per-set records, set tracker UI, drop equity design | Set pack | High |
| F-05 | Legendary proc affixes with conditional triggers (v1 unique affixes are fixed effects) | Needs native proc system plus balance; v1 uniques prove the drop loop | Unique II | High |
| F-06 | Subclass/specialization trees beyond the 18 skills | Deepens build identity but triples skill content | Progression pack | High |
| F-07 | Hunt contracts / bounty board: targeted race hunts with escalating rewards | Highest-value expansion for the hunting pillar; v1 race tables are its foundation | Hunt II | Med |
| F-08 | Hunt families below the 10-NPC threshold (10 reserved: gargoyle, ice wraith, death hound, wisp, horker, boar, netch, slaughterfish, bear, bristleback) | Data already mined; withheld to keep v1 scope | Hunt II | Low |
| F-09 | Unique pack II: remaining mined candidates (31 - 20 = 11 boss sources; 48 - 20 = 28 quest sources) | Content reserves from the same mining run; no new tooling needed | Unique II | Low |
| F-10 | Additional affix families beyond the 120 core (community/expansion packs) | Data-only overlay format already supports it; reserve balance work | Affix packs | Low |
| F-11 | Endgame RF tiers 6+, infinite scaling, Rune Ascension / NG+ loop | v1 stops at RF 5; endgame needs separate economy balancing | Endgame pack | High |
| F-12 | Hardcore / seasonal modes, permadeath, difficulty presets | Mode rules touch save policy and UI; v1 has INI multipliers only | Modes pack | Med |
| F-13 | Loadouts and gear presets | Nice UX, not core loot loop | UI pack | Med |
| F-14 | Boss encounter expansion: phases, arena mechanics, encounter modifiers | Needs hand-authored boss logic beyond our rank system | Boss pack II | High |
| F-15 | Monster ranks 6+ and advanced elite auras | v1 has 5 ranks and 10 affixes; expand after balance data | Boss/Enemy pack | Med |
| F-16 | Follower/companion hunt scaling and rune ranks | Companion balance is its own rabbit hole; reserve | Companion pack | Med |
| F-17 | Player economy services: rune gambling vendor, trade, transmutation (3 items to 1) | Economy sinks reserved until v1 faucet data exists | Economy pack | Med |
| F-18 | Transmutation / affix fusion | Needs F-02 first | Crafting II | Med |

## 2. Reserved presentation and quality of life

| ID | Gap | Why reserve | Goes to | Retrofit cost |
|---|---|---|---|---|
| F-20 | Diablo-style inventory grid / full character sheet art (v1 uses SKSE Menu Framework pages) | Large UI project; v1 needs functional pages only | UI pack | High |
| F-21 | Loot filter, auto-salvage, auto-pickup rules | Needs tag-rich loot data (we already store rarity/enhancement/hunt tags); reserve the UX | QoL pack | Med |
| F-22 | Item comparison metrics (DPS/defense scores), tooltip deepening | Requires formula sign-off and UI space | UI pack | Med |
| F-23 | HUD cooldowns, rarity beams, rank badge animation | v1 ships static badges only | UI pack | Med |
| F-24 | Accessibility: colorblind-safe rarity markers, text scaling, input remapping | v1 ships minimal markers; full accessibility pass later | QoL pack | Low |
| F-25 | Audio/visual identity: rune pickup sounds, UI SFX, unique affix VFX | Art/audio production; v1 uses vanilla effects | Asset pack | Med |
| F-26 | Localization packs beyond English | strings.csv keys are ready now; translation content later | Localization packs | Low |
| F-27 | Expanded asset set: more icons, alternate skins | v1 minimum asset set only | Asset pack | Low |

## 3. Compatibility and platform reservations

| ID | Gap | Decision | Why |
|---|---|---|---|
| F-30 | Compatibility patch expansion (OWL, DiabloRim coexistence, Worlds Dawn, perk overhauls, survival, Requiem) | RESERVE, WATCH in v1 | v1 ships core patches; a dedicated compat pack grows with user reports |
| F-31 | VR support | RESERVE | CommonLibSSE-NG multi-runtime makes it possible; needs VR test hardware |
| F-32 | Skyrim 1.5.97 legacy support | RESERVE | v1 targets 1.6.1170 only (D-062); legacy doubles testing |
| F-33 | Console support | RESERVE (practically impossible) | SKSE requirement excludes consoles; document, do not promise |
| F-34 | Skyrim Together / multiplayer | WATCH | Loot and co-save state are not multiplayer-safe; document as unsupported |
| F-35 | Third-party pack ecosystem: registry, version negotiation, pack docs | NOW (minimal) + RESERVE (registry) | Pack manifest/versioning ships now; catalog/registry later |

## 4. Must cover now (cheap hooks that prevent painful retrofits)

| ID | Hook | Why now |
|---|---|---|
| N-01 | Co-save schema version + migration stub + rollback path | Any later item/rank state change without versioning breaks saves |
| N-02 | Data format version (formats.json) + pack manifest compatibility negotiation | Crafting/Hunt/Unique packs already need it; adding later means reissuing all packs |
| N-03 | Pack independence contract (no cross-plugin FormIDs, EditorID resolution) | v1 is already multi-ESL; retrofitting masters would renumber everything |
| N-04 | Per-family hunt firstOnly state and pack-removal cleanup on load | Removing Hunt/Unique packs must not corrupt saves; design now |
| N-05 | Localization key coverage in strings.csv for every shipped string | Retrofit means re-authoring all UI text |
| N-06 | Loot tags (rarity, enhancement categories, hunt family) stored in metadata | Future loot filter/UI needs them; adding later means re-rolling saves |
| N-07 | UI data API separation: menu reads native API/CSV, never hardcodes | Allows a full UI pack later without touching core |
| N-08 | Diagnostic logging + /rf debug export (load order, pack versions, co-save stats) | Bug reports and future migration need this evidence |
| N-09 | Performance budgets recorded per hook (kill, loot, save) | Optimization later needs a baseline |
| N-10 | Uninstall and pack-disable matrix tested for every module | Public trust depends on clean removal; hard to retrofit |

## 5. Risks to watch (not features)

1. **Co-save growth**: item metadata + hunt state + pack state; budget under 1 KB per 100 items,
   measure at 1,000 and 10,000 items.
2. **Pack fragmentation**: six ESLs in v1; a user installing a subset must not see broken content;
   manifest checks are the guard.
3. **Compatibility drift**: OWL/DiabloRim/overhauls change; watch user reports and keep a patch
   pack.
4. **Power creep**: 19 hunt uniques + 40 boss/quest uniques must not trivialize RF tiers; cap and
   audit unique effects.
5. **Hunting fatigue**: 19 families x first-kill rewards is finite; monitor whether material drop
   rates keep repeat hunting meaningful; if not, F-07 contracts become the next pack.
6. **Data-only balance edits**: users editing CSVs can create invalid states; validator on load
   should fail safe (disable feature, log) instead of crashing.
7. **Localization/asset scope**: English-only, vanilla icons at launch; do not promise parity.
8. **Console/multiplayer expectations**: state unsupported clearly on the Nexus page.

## 6. Expansion roadmap derived from this register

| Release | Packs | Source |
|---|---|---|
| v1 | Base, Unique (40), Hunt (19), Bosses, Patches | current spec |
| 1.1 | Crafting Table (F-01, F-02, F-18) | D-045 |
| 1.2 | Hunt II (F-07, F-08), Unique II (F-09) | mining reserves |
| 1.3 | UI/QoL (F-20..F-24), localization (F-26) | presentation backlog |
| 1.4 | Endgame/modes (F-11, F-12) | after v1 economy data |
| 2.0 | Rune families, sets, legendary procs (F-03..F-05) | content overhaul |

## 7. Rules for adding future gaps

1. Every new gap gets an ID, a decision code and a destination pack.
2. RESERVE items never enter v1 code; only their cheap hooks (section 4) may.
3. WATCH items are measured in v1 (logs/metrics) and reviewed at each release.
4. A reserved feature that turns out to need a v1 hook must update section 4 and D-047 before work.
