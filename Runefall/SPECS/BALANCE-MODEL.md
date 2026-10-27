# BALANCE MODEL - player/enemy scale, 40-hour pacing, protection

Status: draft 1, 2026-10-28. All numbers are data defaults; the goal is a 40-hour Runefall progression with player attributes providing about one third of same-stage enemy power and equipment/class providing the rest.

## 1. Player vs enemy power target

At the same stage, the expected player build should be 33 percent of the enemy baseline from attributes alone; equipment, runes and class passives provide the remaining 67 percent. A player with weak gear is therefore underpowered; a player with strong gear can exceed the baseline.

Definitions:

- enemyBaseline(stage): normalized enemy combat rating at that stage using RCR/NCR formulas.
- playerAttributePower(stage): contribution from the 9 attributes alone, target 0.33 x enemyBaseline(stage).
- playerGearClassPower(stage): contribution from equipment, runes, passives and class nodes, target 0.67 x enemyBaseline(stage).
- PCR: actual player combat rating; target 100 at expected stage gear.

This separates the two design axes: Rune Level gives progression and a controlled attribute floor; gear/class is the main build expression.

## 2. Three balance levers

When the 33 percent target is not met in play, use one of these data levers:

| Lever | Change | Effect |
|---|---|---|
| A. Attribute points | change points per stage or breakthrough bonus | raises/lowers all builds equally; easiest global tuning |
| B. Enemy power | change enemy health/damage curve or role modifiers | makes the world harder; does not fix weak builds specifically |
| C. Per-attribute power | change effect per attribute point | changes build shaping and can favor specific stats |

Recommended default: keep 3 points per stage and tune lever C first, because it preserves the 40-hour attribute plan; use lever B only after gear/class budgets are measured.

## 3. Attribute budget baseline

- 3 attribute points per Rune Level.
- 10 tiers; breakthrough bonus batches: 9, 12, 15, 18, 21, 24, 27, 30, 33, 36 points.
- Total attribute points at Rune Level 100: 300 + 225 = 525.
- 9 attributes; a focused build spreads roughly 175 points across its 3 signature attributes (about 58 each).
- No hard cap; level and breakthrough budget gate points.
- Extra points may come from potions, equipment, rune attributes and rune skill respec.

Starting per-point effects are data defaults, not final: each point should add roughly 0.15 to 0.25 percent of same-stage player power before gear, depending on attribute. Tune lever C with PCR measurements.

## 4. 40-hour pacing

Target: 40 hours of Runefall-specific play from Rune Level 1 to 100, including hunts, crafting, breakthroughs and travel. This is a pacing budget, not a stopwatch requirement.

| Tier | Stages | Hours | Rune kills | Kills/stage |
|---|---:|---:|---:|---:|
| 1 | 1-10 | 1.5 | 90 | 9 |
| 2 | 11-20 | 2.5 | 150 | 15 |
| 3 | 21-30 | 3.0 | 210 | 21 |
| 4 | 31-40 | 3.5 | 270 | 27 |
| 5 | 41-50 | 4.0 | 330 | 33 |
| 6 | 51-60 | 4.5 | 390 | 39 |
| 7 | 61-70 | 5.0 | 450 | 45 |
| 8 | 71-80 | 5.0 | 510 | 51 |
| 9 | 81-90 | 5.0 | 570 | 57 |
| 10 | 91-100 | 6.0 | 630 | 63 |
| Total | 1-100 | 40.0 | 3600 | 36 avg |

The hours include breakthrough bosses, materials and crafting. Rune kills are the main XP currency; normal creatures and quests are secondary or later systems.

## 5. XP and combat-count economy

- XP per rune kill scales by stage and by creature class.
- XP required per stage is data-driven and tuned so the table above holds.
- Elite, boss and raid kills count as multiple normal kills; do not multiply XP by a second hidden level curve.
- Tier breakthroughs require a raid boss plus materials; they are not just XP thresholds.
- Suggested data fields: xpPerKillBase, xpPerKillStageExponent, xpToNextStageBase, xpToNextStageGrowth, killsPerStageTarget, tierHoursTarget, breakthroughCost.
- A stage can also be advanced by rune XP potions and later quests, but the 40-hour target assumes hunt combat.

## 6. Damage categories and protection

Protection only applies to rune creatures with a rune stage. Normal creatures have no rune protection.

Formula:

- G = max(0, targetStage - attackerStage - penetration).
- DR = min(0.70, 0.03 x G).
- finalDamage = rawDamage x max(1 - DR, 0.05).

Cap is 70 percent, chosen because rune actors are strong; 5 percent floor retained so low-stage players can still eventually kill high-stage targets.

Category rules:

| Category | Protection |
|---|---|
| Direct melee, ranged, spell | full |
| Damage over time | 50 percent of protection |
| Reflected damage | none |
| Environmental, fall, scripted | none |
| Player as target | no rune protection by default |
| Anomaly as target | no host rune protection |

## 7. Penetration

Penetration reduces the effective stage gap. Sources:

- rune-forged weapons and armour
- class passives and rune skills
- consumables and temporary effects
- spell-channel penetration for mages (no weapon required)

Rules:

- penetration is capped per hit by data; starting cap 20 stages.
- penetration never increases damage directly; it only reduces DR.
- penetration sources are additive up to the cap; no hidden multiplicative stacking.
- normal creatures ignore penetration.
- player protection is not a ladder feature; THREAT-PROTECTION owns target policy.

## 8. NPC simulation

NPCs do not use the player attribute/class system. A spawned NPC gets:

- a stage from the encounter/zone/rate table,
- one role/family package,
- 2-4 perks or ability effects,
- one gear/outfit set and loot table,
- an optional light archetype tag.

No deep build simulation, no full skill tree, no per-actor attribute allocation. This keeps performance and save size controlled, while gear remains vanilla-lootable.

## 9. Data files

All values above live in JSON: stage ladder, attribute definitions, progression curve, pacing targets, protection profiles, penetration sources, NPC packages. The DLL reads them; changing balance is a data change.

## 10. Open calibration

- per-attribute power values and caps
- gear/class power budget at each tier
- exact XP curve numbers
- role modifiers for NCR/RCR
- penetration cap and source values
- boss/raid time budgets
