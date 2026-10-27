# COMBAT RATINGS - PCR, NCR, RCR, TTK, TTD

Status: draft 1, 2026-10-28. Data-driven, descriptive ratings for UI, encounter design and calibration. They do not replace actual combat stats; real damage still uses AVs, perks, spells and equipment.

## 1. Definitions

- PCR, Player Combat Rating: how strong the player current build is from offense, effective HP, sustain and utility.
- NCR, Normal Creature Rating: the same estimate for a non-rune creature from its actual vanilla or modded stats.
- RCR, Rune Creature Rating: the same estimate for a rune creature, including its rune stage and family modifiers.
- TTK, Time To Kill: expected seconds for the attacker to kill the target.
- TTD, Time To Die: expected seconds for the target to kill the attacker.
- Q, challenge ratio: TTK divided by TTD. Below 1 the attacker is favored; above 1 the target is favored.

Ratings are normalized so 100 means the expected baseline at that stage. 50 is half the expected build, 200 is double. Ratings are for comparison, labels, encounter recommendation and balance reports; they are not part of the damage formula.

## 2. Input model

All inputs are data or measured runtime values, never hardcoded in C++.

Player/actor input:

- offenseDPS: sustained single-target DPS from weapon, spells, summons, DoTs, crit, attack speed and damage bonuses.
- effectiveHP: health divided by (1 - mitigation), plus healing/shield per second times expected fight duration, capped at 50 percent of base effectiveHP.
- mitigation: weighted physical/magic damage reduction from armor, resistances, block and defensive effects, capped at 80 percent.
- utilityFactor: 0.85 to 1.15 from mobility, control, stealth opening, sustain and cooldown uptime.
- referenceStage: player Rune Level for PCR; creature rune stage for RCR; derived rating level for NCR.

Derived:

- effectiveDPS(attacker, target) = rawDPS x hitFactor x (1 - targetMitigation) x stageProtectionMultiplier.
- effectiveHP(target) = health / (1 - mitigation) + sustain x fightDuration, with caps.
- TTK = effectiveHP(target) / effectiveDPS(attacker, target).
- TTD = effectiveHP(attacker) / effectiveDPS(target, attacker).
- Q = TTK / TTD.

## 3. Rating formulas

Reference baselines are data rows, not constants in code. Suggested starting baselines:

- refDPS(stage) = baseDPS x offenseCapacity(stage) / 100.
- refEHP(stage) = baseEHP x defenseCapacity(stage) / 100.
- baseDPS = 10.0; baseEHP = 150.0 at stage 1.

Then:

- PCR = 100 x sqrt( (offenseDPS / refDPS(playerStage)) x (effectiveHP / refEHP(playerStage)) ) x utilityFactor.
- NCR = 100 x sqrt( (offenseDPS / refDPS(ratingStage)) x (effectiveHP / refEHP(ratingStage)) ) x utilityFactor.
- RCR = same as NCR but ratingStage is the creature rune stage and family/elite modifiers are already reflected in actual stats.

This gives a relative build score: a player at expected gear and attributes near 100; a glass cannon may have high offense but lower effectiveHP and settle near 100; an over-geared player rises above 100.

## 4. Suggested challenge bands

Starting targets, to be adjusted from real play:

| Encounter | TTK | TTD | Q |
|---|---:|---:|---:|
| Normal trash at player level | 4-8 s | 25-40 s | 0.1-0.3 |
| Normal elite or boss creature | 10-20 s | 15-25 s | 0.4-1.0 |
| Rune creature, same stage | 15-30 s | 15-25 s | 0.7-1.5 |
| Rune creature, +10 stages | 30-60 s | 8-15 s | 2-6 |
| Rune creature, +20 stages | 60-120 s | 4-8 s | 8-20 |
| Rune creature, +30 stages (protection cap) | 120-240 s | 2-5 s | 25-60 |
| Designed boss, appropriate stage | 2-5 min | 30-60 s | 3-8 |
| Raid boss | 5-15 min | 20-40 s | 10-30 |

These are not hard gates. A skilled player with the right build can beat a +20 stage target; the numbers describe expected effort, not permission.

## 5. Threat labels

Map PCR/NCR/RCR or Q to player-facing labels, data-driven:

| Rating or Q | Label |
|---|---|
| below 50 | Trivial |
| 50-80 | Manageable |
| 80-120 | Fair |
| 120-180 | Dangerous |
| above 180 | Lethal |

For a rune creature, combine stage gap and rating: a same-stage creature with RCR 180 is still dangerous; a +30-stage creature with RCR 60 is still protected. Show both if the UI can.

## 6. Data schema

rating_profiles.json

- profileId, baseDPS, baseEHP, offenseWeight, defenseWeight, utilityWeight, mitigationCap, sustainCap, ratingStageSource.

rating_bands.json

- bandId, minRating, maxRating, labelKey, colorKey, notes.

combat_targets.json

- targetId, encounterClass, stageRelation, ttkMin, ttkMax, ttdMin, ttdMax, qMin, qMax, weight.

actor_threat_profiles.json

- role/family/actorRef, ratingKind (NCR/RCR), referenceStageSource, roleModifier, fallbackGroup, notes.

All formulas use these data values. Changing balance is a data change.

## 7. Ownership and use

- Ratings and TTK/TTD are computed by the DLL and stored in diagnostics/snapshots.
- UI uses ratings for labels and recommendations; it never derives them from names.
- Encounter generator uses ratings plus stage gap for spawn selection; it must not treat rating as stage.
- Calibration reports use the same formulas; no second source of truth.
- Normal creatures have NCR only; they do not gain rune protection. Modded normal creatures fall back by role and rating level.
- Player has PCR and Rune Level separate. Gear changes PCR, never Rune Level.

## 8. Open calibration inputs

- Base DPS/EHP per stage; initial 10/150 at stage 1 is a placeholder.
- Role utility weights and mitigation profiles.
- Hit factor and armor/resistance weighting by damage type.
- Boss and raid target bands.
- Whether ratings are shown as raw numbers or only labels in release UI.
