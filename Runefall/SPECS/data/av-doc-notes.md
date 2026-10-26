# Actor values and safe modification channels - documented notes

**Method (read-only; no game install, mod or plugin modified, 2026-10-26).** `web_search`/
`web_fetch` are misconfigured, so fetching used PowerShell `Invoke-RestMethod`. Working: UESP
`https://en.uesp.net/w/api.php` (action=parse; `/wiki/` 404s), CK wiki
`https://ck.uesp.net/w/api.php`, `api.github.com` + `raw.githubusercontent.com`, local MO2 reads.
UESP `list=search` is dead; titles via `prefixsearch`.

## 1. DamageResist vs the armor rating formula

**Armor rating is computed from equipment + skill + perks; the result is stored in the
`DamageResist` actor value.** Not a separate stat.

- `DamageResist` = AV 39, "Armor rating (points, not damage reduction %)", AVIF `0x000005CE` -
  https://en.uesp.net/wiki/Skyrim_Mod:Actor_Value_Indices
- displayed rating = SUM(item ratings) + shield + armor effects; item = CEILING[(base+quality) x
  (1+0.4 x skill/100)] x Custom Fit/Well Fitted x Matching Set x armor perk -
  https://en.uesp.net/wiki/Skyrim:Armor#Armor_Rating
- reduction % = rating x 0.12 + 3.0 x pieces; hidden +25/piece; cap 80% (667 armor, 567 displayed
  with four pieces) - https://en.uesp.net/wiki/Skyrim:Armor#Armor_Cap
- flat points are valid input: "Resist Damage ... as though the creature was wearing armor of the
  specified number of points" - https://en.uesp.net/wiki/Skyrim:Resist_Damage ; the Alteration
  "Armor" group "increases the caster's Armor Rating" -
  https://en.uesp.net/wiki/Skyrim:Armor_(effect)
- native helper `ActorValueOwner::GetArmorRatingSkillMultiplier(float)` -
  https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/A/ActorValueOwner.h
- **persistence:** `SetActorValue` on DamageResist, CarryWeight, SpeedMult, MeleeDamage, CritChance,
  UnarmedDamage, the rates, Mass, NormalWeaponsResist "will not persist in consecutive saves" -
  https://ck.uesp.net/wiki/Actor_Value#Notes . Use abilities / effect modifiers instead.
- bonus channels: perk entry **"Modify Armor Rating"** -
  https://ck.uesp.net/wiki/Perk_Entry_Point ; or an "Armor" Value-Modifier effect.

## 2. Intended use of individual values

| AV (idx) | Documented meaning (UESP AV Indices) | Caveat |
|---|---|---|
| `AttackDamageMult` (154) | multiplier for all physical damage - weapons, fists, bashing (1 = default); also moves displayed damage | multiplicative; stacks with combat mods |
| `MeleeDamage` (34) | flat +points to listed weapon damage (default 0) | "actually affects the damage display" |
| `UnarmedDamage` (35) | flat +points to fist damage | narrow, low risk |
| `CritChance` (33) | % chance to crit; crit **damage** comes from weapon data/perks | crits bypass armor - https://en.uesp.net/wiki/Skyrim:Damage#Critical_Strikes |
| `WeaponSpeedMult` (85) | attack / bow-draw speed; 1 = 100%, 0.5 = 50%, **0 also = 100%** | 0-vs-1 trap; CK: "1 to 5 work well, 0 to reset" |
| `SpeedMult` (30) | movement speed %, default 100 | CK: applied only when inventory weight or carry capacity change |
| `CarryWeight` (32) | max weight before forced walk | non-persistent via SetActorValue |

Skill modifiers, per https://ck.uesp.net/wiki/Actor_Value#List_of_Actor_Values:
- `<skill>Mod` (96-113): "changed by perks and fortify skill enchantments; the automatic perk
  **PerkSkillBoosts** translates those into actual game effects."
- `<skill>PowerMod` (135-152): "changed by fortify skill potions; the automatic perk
  **AlchemySkillBoosts** translates those into actual game effects" - like raising the skill, except
  schools map to duration (Alteration/Conjuration) or magnitude (Destruction/Illusion/Restoration).
- Named semantics: `SmithingPowerMod` = tempering strength; `AlchemyPowerMod` = potion/poison
  strength; `EnchantingPowerMod` = player-made enchant power; `SpeechcraftMod` = price/Haggling
  maths; `MarksmanMod` = bow damage; `MarksmanPowerMod` bug hits all weapons (USKP 1.2).
- `<skill>SkillAdvance` (114-131): "As of Patch 1.9 ... have **no effect**." Replacement is
  `ActorValueInfo` (`SetSkillUseMult`, `SetSkillImproveMult/Offset`, `AddSkillExperience`) -
  `...\mods\Skyrim Script Extender (SKSE64)\Scripts\Source\ActorValueInfo.psc`.

## 3. Established channels per domain

| Domain | Documented rule | Documented lever |
|---|---|---|
| Lockpicking | sweet spot ~ `60*(2^-diff)*(0.82+0.6*L/100)*MatchingPerk*(1+Ench+Potion)` deg; break time x `1+0.5*L/100` - https://en.uesp.net/wiki/Skyrim:Lockpicking#Sweet_Spot_and_Partial_Pick_Zones | perk entries: Modify Lockpick Sweet Spot / Level Allowed / Starting Arc / Unbreakable |
| Pickpocket | `15 + PlayerSkill - NPC_Skill/4` + item effects; cap 90% - https://en.uesp.net/wiki/Skyrim:Pickpocket#Skill_Usage | perk entries: Modify Max Pickpocket Chance, Can Pickpocket Equipped Item |
| Persuasion / Intimidation | bribe and intimidate formulas both consume `Player Speech Actor Value` - https://en.uesp.net/wiki/Skyrim:Speech#Bribe_Formula , #Intimidate_Formula | perk entry **Mod Player Intimidation** |
| Vendor prices | `price factor = 3.3 - 0.013*min(skill,100)`; perks + Fortify Barter multiply; **skill >100 has no effect** - https://en.uesp.net/wiki/Skyrim:Speech#Prices | `SpeechcraftMod`; Fortify Barter |
| Alchemy potency | `Result = 4 x BaseMag x (1+0.5*Skill/100) x perks x enchants` - https://en.uesp.net/wiki/Skyrim:Alchemy#Formula | `AlchemyPowerMod` |
| Tempering | Fine..Legendary at 14/31/65/100/134/168 skill, +2 chest / +1 other per level - https://en.uesp.net/wiki/Skyrim:Smithing#Item_Quality | `SmithingPowerMod`; tempering perk entry |
| Sneak detection | `Detection = -15 + (sound+visual+noticer)*(1-dist/2500)^2 + (noticer - sneaker)`; Sneak skill reduces only the noticer factor - https://en.uesp.net/wiki/Skyrim:Sneak#Remaining_Undetected | Sneak AV/perks; PO3 Detection natives |

## 4. What the installed frameworks document

**Papyrus Extender 6.5.1** (local `H:\Skyrim\MantrasOfMara\mods\powerofthree's Papyrus Extender`;
Nexus 22854).
- `float GetActorValueModifier(Actor akActor, int aiModifier, String asActorValue)` - Permanent = 0,
  Temporary = 1, Damage = 2; no Base case, use `GetBaseActorValue` -
  https://github.com/powerof3/PapyrusExtenderSSE/wiki/Actor#getactorvaluemodifier
- `GetPrimaryActorValue(MagicEffect)`, `GetSecondaryActorValue(MagicEffect)` (local `PO3_SKSEFunctions.psc`).
- Detection natives `CanActorBeDetected`, `CanActorDetect`, `IsDetectedByAnyone`,
  `ForceActorDetection/Detecting`, `PreventActorDetection/Detecting`, `ResetActorDetection/Detecting` -
  https://github.com/powerof3/PapyrusExtenderSSE/wiki/Detection
- No AV setter in the shipped script source: PE is a read/detection surface only.

**Actor Value Generator 1.9.5.6** (Nexus 84743; `SKSE\Plugins\ActorValueExtension.dll`).
- Config: TOML, suffix `_AVG`, under `Data/SKSE/Plugin/ActorValueData`; `Commons` reserved -
  https://github.com/NoahBoddie/actor-value-generator/blob/master/LEGACY.md
- Types: **Adaptive** (per-actor, "manipulated as you would any other actor value, and with the same
  level of persistence"), **Functional** (formula), **Exclusive** (*"Currently not implemented"*),
  Routines, Properties, Include Lists.
- Formula vocabulary `GetActorValue('Health', All)`, `Base`, `Maximum`; shipped
  `ActorValue_AVG.toml` maps Base=1, Permanent=2, Temporary=4, Damage=8, Max/Maximum=7, All=15.
- Caveat: a Functional value needs the right get/set pair to be driven by a magic effect.

**Custom Skills Framework 3.1.0.0** (Nexus 41780). API v3, `global native`, in
`H:\Skyrim\MantrasOfMara\mods\Custom Skills Framework\Source\Scripts\CustomSkills.psc`:
`GetAPIVersion`, `OpenCustomSkillMenu(string)`, `ShowTrainingMenu(string,int,Actor)`,
`AdvanceSkill(string,float)`, `IncrementSkill(string)`, `IncrementSkillBy(string,int)`,
`GetSkillName(string)`, `GetSkillLevel(string)`, `ShowSkillIncreaseMessage(string,int)`,
`DebugReload()`. Config-defined string skill IDs; console bridge `csf`
(local `SKSE\CustomConsole\CustomSkills.yaml`). Upstream: https://github.com/Exit-9B/CustomSkills

**SKSE** adds no AV arithmetic; `ActorValueInfo.psc` supplies `GetActorValueInfoByName`,
`GetActorValueInfoByID` **"id (0-164)"**, `IsSkill`, `Get/SetSkillUseMult`, `Get/SetSkillImproveMult/Offset`,
`Get/Set/AddSkillExperience`, `GetExperienceForLevel`, `GetCurrentValue`, `GetBaseValue`,
`GetMaximumValue`. Native `ActorValueOwner`: `GetActorValue`, `GetPermanentActorValue`,
`GetBaseActorValue`, `SetBaseActorValue`, `ModActorValue`,
`RestoreActorValue(ACTOR_VALUE_MODIFIER,...)`, `SetActorValue`, `GetClampedActorValue`.

## 5. Follow this / do not invent

**Documented - follow.** (1) Add armor as flat `DamageResist` points or via perk entry "Modify Armor
Rating"; never re-derive the 0.12%/point curve. (2) Read AV components explicitly
(`GetActorValueModifier` 0/1/2 + `GetBaseActorValue`); `GetActorValue` alone is ambiguous.
(3) Permanent bonuses as abilities/effect modifiers; `SetActorValue` only for transient effects.
(4) For engine-owned checks (locks, pickpocket, intimidation, tempering, prices) use perk entry
points, not parallel rolls. (5) Own progression uses `ActorValueInfo` skill-XP multipliers, never
`SkillAdvance`. (6) Resolve AVs by index (0-164) **and** name.

**Not documented - avoid.** (1) `<skill>SkillAdvance` (dead since 1.9). (2) AV names outside the
0-164 table (the spec's 149 AVIF records is an AVIF-only under-count). (3) "WeaponSpeedMult 0 = off"
(0 is documented as 100%). (4) AVG "Exclusive" values (unimplemented). (5) Any claim that PE / AVG /
CSF expose a generic AV setter. (6) Writing `SpeedMult` expecting instant effect.

## 6. UNVERIFIED / open

- The engine routine that recomputes `DamageResist` from worn equipment: **UNVERIFIED**, inferred
  from the SetActorValue persistence note, `GetArmorRatingSkillMultiplier`, and the UESP formula.
- CK wiki pages target Skyrim LE; SE/AE 1.6.1170 and 1.7.104 parity **UNVERIFIED**.
- AVG path `Data/SKSE/Plugin/ActorValueData` (docs) vs `SKSE\Plugins\ActorValueData` (shipped) -
  **UNVERIFIED** which is canonical.
- CSF config schema (upstream `docs/schema`) not read; only Papyrus API + console YAML verified.
- No documented AV-level hook for gathering yield or persuasion success %; those stay Runefall-native.
