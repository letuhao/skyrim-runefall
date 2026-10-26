# L2A - Vanilla actor value coverage (pre-L2 check)

Status: design note, 2026-10-26. Input for L2. No engine investigation was performed: the
actor value model is already fully exposed by Mutagen / Synthesis (which the project uses for
record work), and Bethesda has not changed actor value behavior since release. This document
only classifies the existing AV set for design coverage.

Question being answered: do our 9 attributes cover the actor values in a way that gives them
value across many gameplay systems instead of making them combat-only?

## 1. Source of truth

- The AV list is part of the Mutagen data model used by Synthesis; every standard actor value
  exists there. No custom dump, reverse engineering or cross-version verification is needed
  or planned (user direction: do not waste tokens re-investigating stable engine behavior).
- Usage in the mod: read AVs through the normal mapped API (Mutagen types in generation tools;
  CommonLibSSE/Papyrus in runtime code). Follow documented Bethesda/community usage patterns.
- Classification below is by dominant gameplay domain; some values belong to more than one
  cluster (skill modifiers are both magic/combat and skill support).

## 2. What the 149 values actually are

| Cluster | Approx count | Examples | Player-attribute relevance |
|---|---|---|---|
| Core resources | 3 | Health, Magicka, Stamina | Direct: every build |
| Skills (18) | 18 | OneHanded, Marksman, Sneak, Lockpicking, Speechcraft, Smithing, Enchanting, Alchemy | Used as gates/inputs; never modified by us |
| Resistance and mitigation | 7 | DamageResist, MagicResist, Fire/Frost/Shock/PoisonResist, NormalWeaponsResist | Direct: defense |
| Regeneration and rates | 8 | HealRate, MagickaRate, StaminaRate, MagickaRateMod, CombatHealthRegenMult | Direct: sustain |
| Offense | 8 | AttackDamageMult, MeleeDamage, UnarmedDamage, CritChance, BowStaggerBonus, WeaponSpeedMult, ShieldPerks, ArmorPerks | Direct but risky (mod conflicts) |
| Movement, carry, perception | 14 | SpeedMult, JumpingBonus, CarryWeight, InventoryWeight, MobilityCondition, NightEye, DetectLifeRange, Telekinesis | Partial: exploration |
| Stealth/detection | 5 | SneakMod, SneakPowerMod, SneakSkillAdvance, Muffled, Invisibility | Partial: stealth |
| Economy/social | 13 | Speechcraft Mod/PowerMod/SkillAdvance, FavorPointsBonus, FavorsPerDay, FavorActive, BypassVendorStolenCheck, Fame, Infamy | Partial: Cunning only |
| Magic and shouts | ~20 | Alteration/Conjuration/Destruction/Illusion/Restoration Mod, PowerMod, SkillAdvance; AbsorbChance; ShoutRecoveryMult | Partial: 3 mage attributes |
| Skill support modifiers | ~30 | OneHandedMod/PowerMod/SkillAdvance, SmithingPowerMod, PickPocketMod, BlockPowerMod, etc. | Used indirectly; dangerous to edit |
| AI and behavior | 10 | Aggression, Assistance, Confidence, Morality, Mood, Energy, WaitingForPlayer, Paralysis | Out of scope (NPC AI, not player) |
| Unused/engine/quest | ~12 | Variable01-10, DragonSouls, DragonRend, Mysticism, quest-used Favor* timers | Out of scope by decision D-006 |

Two rules follow from this inventory:

1. We will not try to "cover" all 149. AI/behavior values, engine variables and quest-used
   Favor* values are explicitly out of scope (D-006: no vanilla AV repurposing).
2. Player-facing coverage is measured on the first ten clusters only. That is the real target
   for "attributes must matter in many gameplay areas".

## 3. Coverage matrix of the current 9 attributes

Domains: Combat (C), Defense (D), Stealth (S), Crafting (Cr), Economy (E), Exploration (X),
Magic utility (M), Social/utility (U). P = primary, S = secondary, - = none.

| Attribute | C | D | S | Cr | E | X | M | U | Current non-combat anchors |
|---|---|---|---|---|---|---|---|---|---|
| Might | P | S | - | - | - | P | - | - | carry weight |
| Endurance | P | P | - | - | - | - | - | - | none |
| Precision | P | - | - | P | - | - | - | - | crafted item power |
| Agility | P | S | - | - | - | S | - | - | movement |
| Guile | - | - | P | - | S | S | - | - | lockpick/pickpocket, magic/rune find |
| Cunning | S | - | S | S | P | - | - | P | vendor value, poison potency |
| Intellect | P | - | - | P | - | - | P | - | enchantment power |
| Wisdom | P | S | - | - | - | - | P | - | none (magic is still combat) |
| Willpower | P | - | - | - | - | - | P | - | none |

Findings:

1. Endurance, Wisdom and Willpower are combat/magic-only. They are the "combat monger" risk
   the user flagged.
2. Stealth is thin (Guile, Cunning) and only touches locks/pickpocket/stealth damage; it does
   not reach Sneak detection, Muffle or Invisibility.
3. Economy is carried by Cunning alone (vendor value); Speechcraft persuasion, intimidation
   and favor-style NPC interactions have no attribute.
4. Crafting is decent (Precision, Intellect, Cunning) but alchemy potions and material
   gathering are not covered by any attribute.
5. Exploration is only carry weight and movement; stamina/sprint and discovery rewards have
   no attribute anchor.
6. Combat is covered by all nine, which is fine for an ARPG - but it cannot be the only axis.

## 4. Proposed non-combat anchors (attribute effect update)

Each attribute gains one non-combat effect, anchored either to a safe vanilla actor value or
to a Runefall system (which the native DLL implements). No unused AI/quest AVs are touched.

| Attribute | Added non-combat effect per point | Anchor | Domain |
|---|---|---|---|
| Might | +0.5 percent ore/hide/wood gathering yield | Runefall gather hook | Crafting/Economy |
| Endurance | +1 stamina and +0.5 percent stamina regeneration | Stamina, StaminaRate (vanilla) | Exploration |
| Precision | +0.5 percent crafting material efficiency | Runefall craft hook | Crafting |
| Agility | (already movement) +0.3 percent fall damage reduction | Runefall movement hook | Exploration |
| Guile | (already locks/loot) +0.3 percent Sneak detection reduction | SneakMod internals / Runefall hook | Stealth |
| Cunning | +0.5 percent persuasion and intimidation success | Speechcraft check hook | Social |
| Intellect | (already enchantment) +0.5 percent spell learning from books | Runefall reading hook | Magic utility |
| Wisdom | +1 percent potion effectiveness | Alchemy/Potion hook | Crafting |
| Willpower | +0.5 percent intimidation chance | Speechcraft/Illusion hook | Social |

After this update every attribute has at least one non-combat anchor, and every non-combat
domain (crafting, economy, exploration, stealth, social) has at least two attributes
touching it.

## 5. Actor value safety classification for L2

Safe to modify as intended (still subject to caps and conflict tests):

- Health, Magicka, Stamina via ability effects (never permanently baked into the base record).
- CarryWeight (small values; other mods also touch it).
- DamageResist and the elemental resistances; NormalWeaponsResist.
- Regen rates (HealRate, MagickaRate, StaminaRate) within caps.
- Skill "Mod" and "PowerMod" values are the engine's intended skill-effectiveness channel, but
  perk overhauls also use them: treat as read-mostly, apply our bonuses through our own
  systems where possible.

Read-only inputs (we read them, we never write them):

- The 18 skill values: used as gates (Skyrim skill 20/50/75) and as crafting/detection inputs.
- Speechcraft, Alchemy, Smithing, Enchanting: used to compute our own non-combat bonuses.
- InventoryWeight: read for encumbrance-related effects.
- Sneak, Lockpicking, Pickpocket: read for check difficulty where the engine exposes it.

Avoid modifying (conflict or physics risk):

- SpeedMult, JumpingBonus (physics/animation interactions).
- AttackDamageMult, MeleeDamage, UnarmedDamage (heavily modded by combat mods; use our own
  multiplier channel instead).
- Left/RightAttackCondition, EquippedItemCharge, VoiceRate, BowSpeedBonus (engine timing,
  changes can break combat feel or animations).
- AI/behavior values, Favor* values, unused Variable01-10: out of scope by decision.

## 6. Mapping decisions this analysis feeds (no engine investigation)

These are implementation choices for L2, made from documented Bethesda/community patterns:

1. Armor bonus: use the same documented channel the game already uses for armor bonuses
   (vanilla fortify/armor effects and perk conditions). Do not invent an armor formula.
2. Lockpicking/pickpocket/persuasion: follow the documented vanilla "Fortify <skill>" pattern
   (skill actor value modifier effects) where it applies; use a native hook only where the
   check is hardcoded and no documented surface exists.
3. Gather/craft/potion efficiency: implement through native hooks if no documented Papyrus
   surface exists; never by editing vanilla records.
4. Skill Mod/PowerMod values: treated as read-mostly; apply our bonuses through our own
   effects/perks in the documented way, not by overwriting perk systems.
5. CarryWeight and resistances: applied through standard ability effects with caps - the
   documented pattern used by vanilla blessings/enchantments.
6. Social anchor: use the documented Speechcraft/Illusion effect channel where one exists;
   otherwise a Runefall-native check.

## 7. Notes

- AV data source: the Mutagen/Synthesis data model already contains every actor value. The
  temporary dump tool and JSON files were removed as redundant (user direction: no engine
  re-investigation).
- Compatibility with installed overhauls (Adamant, Experience, etc.) is a testing task at
  implementation time, not engine research.
- Actor value behavior is treated as stable across game versions, per user direction and
  Bethesda's unchanged AV model.
