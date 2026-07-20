# Card Upgrade Authoring Contract

## Eligibility

- Direct rarity steps: White→Blue, Blue→Yellow, Yellow→Purple.
- Purple and Intrinsic have no next version.
- `Card.Run.*` and `Physique.Capacity > 0` cards may not have or be a target of an upgrade link.
- A linked card must provide a non-None UpgradeFamilyId; every card in the chain uses the same value.

## Chain integrity

- CardId is unique in the validated catalog.
- Maximum four cards per family.
- No self-link, cycle, skipped rarity, merge or branch.
- Each target has at most one predecessor.

## Structural comparator

Allowed differences:

- CardId, Rarity, BaseCost.
- DisplayName, Description, CardIllustration, CardIllustrationDepthMap.
- Magnitude and Duration of positionally corresponding `Effects` and `PerfectReleaseEffects`.
- Upgrade link metadata.

Required equal:

- Keywords, TargetMode and HandCardTargetFilter.
- all FCardPhysique fields.
- effect counts/order and EffectType, Target, TargetZone, MagnitudeSource, Condition, MagnitudeModifiers, deprecated compatibility flag.
- ZoneHooks and Passives in full.

At least one BaseCost/Magnitude/Duration value must differ per direct step.

## Validation levels

- `FWacomCardDefinitionValidation`: validates the current asset and its reachable direct chain.
- `FWacomCardUpgradeCatalogValidation`: validates a supplied catalog for unique CardId, predecessor count, family size and cross-asset merge/cycle invariants.
- Existing no-chain CardDefinition remains valid.
