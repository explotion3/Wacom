# Production Content Checklist: 正式 Floor 1 Production 内容合同冻结

**Purpose**: Reviewer checklist for the frozen SerpentWood content and future 38-asset implementation handoff.
**Created**: 2026-07-17
**Feature**: [spec.md](../spec.md)

## Enemy and behavior

- [x] CHK001 Four unique EnemyIds use `Enemy.SerpentWood.*`.
- [x] CHK002 Exactly eleven PartIds exist and every PartSlot is explicit.
- [x] CHK003 Part HP/EXP totals match 16/2, 28/4, 26/4 and 52/9.
- [x] CHK004 Every new Part has `KnockdownRewardCard=null` with P0 caveat.
- [x] CHK005 Each Enemy owns one Behavior with `Default` phase.
- [x] CHK006 Each PartSlot maps to one explicit `Sequence` IntentSet.
- [x] CHK007 Exactly 24 Intents are listed in authored loop order.
- [x] CHK008 Damage I/R and status/shield I/R values match the approved table.
- [x] CHK009 Player Slow uses one-card HandAffliction; Twilight uses all-current-hand semantics.

## Encounter gradient

- [x] CHK010 Six Encounter IDs and all EnemySlotIds are explicit.
- [x] CHK011 Encounter HP totals are 16, 28, 32, 26, 44 and 52.
- [x] CHK012 No Encounter contains more than two enemies.
- [x] CHK013 Guardian boss identity remains a Floor node payload only.

## Cards, Pickup and Shop

- [x] CHK014 Four new CardIds have exact cost/rarity/keyword/target/effect fields.
- [x] CHK015 SerpentSigil has no added keyword and draws one from Draw.
- [x] CHK016 Four Pickups are fixed Card rewards; only SerpentSigil grants Credential.
- [x] CHK017 Wayfarer has exactly five Offers in frozen order and prices.
- [x] CHK018 Existing Starter cards and live `PoisonFang` are the only external card dependencies.
- [x] CHK019 Route A +2 Gold and Route B +3 Gold each enable a purchase from initial 0.

## RunEvents

- [x] CHK020 Four EventIds and thirteen ChoiceIds are listed.
- [x] CHK021 Every Choice is terminal, closes, completes and uses Automatic AP.
- [x] CHK022 `MoltTrailKnown` and `MarshRouteKnown` are FName RunFlags, not GameplayTags.
- [x] CHK023 All negative Gold effects have matching MinGold conditions.
- [x] CHK024 Misdeed/Fatigue deltas and gates use existing RunEvent schema.
- [x] CHK025 Event choices preserve one AP each and do not change the Floor AP range.

## Manifest and production gate

- [x] CHK026 Manifest contains exactly 38 numbered package rows with unique paths.
- [x] CHK027 Category totals equal 6/4/4/1 + 4/4/4/11.
- [x] CHK028 Future builder write scope is limited to manifest packages; existing dependencies are read-only.
- [x] CHK029 Production rows contain no debug, authoring, test, badge fixture or TrainingWarrior dependency.
- [x] CHK030 Binary implementation, validation, world authority, scene work and Golden Path PIE remain explicitly pending.

## Notes

- This checklist reviews design quality, not nonexistent DataAsset bytes.
- The asset implementation round must rerun equivalent checks against loaded assets and references.
