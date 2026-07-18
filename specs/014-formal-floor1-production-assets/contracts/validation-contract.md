# Validation Contract

## Validation tiers

### General schema validation

Every loaded or newly created object must pass the existing validator for its DataAsset class. Enemy Parts additionally pass `FormalProduction`: explicit Aid and Destroy rewards, null legacy `KnockdownRewardCard`, and otherwise valid Part data.

### Floor 1 structural validation (default)

Default inspection protects stable identity and topology while allowing approved manual tuning. It verifies:

- package/object/class and the stable ID field;
- Card keyword and target mode; ordered effect count/type/target/source semantics;
- Enemy Part identity and explicit Aid/Destroy references, with null legacy field;
- Enemy PartSlot IDs/order/references, Behavior reference, phase and initial IntentSet references;
- Behavior ID, `Default` phase, Part-to-Sequence mapping, Intent IDs/order, ordered effect type/target, and absence of unsupported selector/fallback structure;
- Encounter ID, slot IDs/order, and Enemy references;
- Event ID, Choice IDs/order, terminal/completion/close/AP policy, ordered condition/effect kinds and flag/pressure identities;
- Pickup ID, reward kind, Card/Credential identity and references;
- Shop ID, Offer order, Card references and nonnegative authored prices;
- forbidden-reference closure and exact manifest/class counts.

### Initial seed comparison (optional)

`CompareSeedDefaults` additionally compares every editable authored field against the frozen Spec 011/013 seed, including localized text and balancing numbers. It is required for initial asset acceptance and double-run proof. It is not the normal long-term authoring validator.

## Stable versus tunable fields

| Stable after creation | Manually tunable after first seed |
|---|---|
| package/object/class | display names and descriptions |
| all stable IDs | Card cost and rarity |
| reference topology and array/sequence order | Card effect magnitudes |
| Card keyword and target mode | Part HP and EXP |
| effect/condition kind and target/source semantics | Intent initiative, resistance, and magnitude |
| Part Aid/Destroy mapping and null legacy field | Shop prices |
| Encounter/Enemy/Part/Intent slot identities | Event numeric Gold/Misdeed/Fatigue values |
| Pickup reward kind and credential identity | other explicitly documented balance numbers |

Changing a stable field requires a reviewed contract/migration change; the seed tool is never used to push such a change into existing assets. Approved tunable changes may cause `CompareSeedDefaults` to fail while default structural validation remains green.

## Exact content assertions

- Manifest count is `46 = 12 Card + 4 Behavior + 11 Part + 4 Enemy + 6 Encounter + 4 Event + 4 Pickup + 1 Shop`.
- Behavior graph contains exactly 24 Intents and one Sequence IntentSet per PartSlot.
- Four archetypes map all 11 Parts to their own Aid/Destroy pair.
- All eight branch cards follow Spec 013. `Reward.SerpentWood.ShallowGuardian.Destroy` has `TargetMode=AllEnemyParts`; both Damage 4 and Poison 1 target `Target.AllEnemyParts`.
- Six Encounters preserve total HP progression `16 -> 26/28/32 -> 44 -> 52` and contain no more than two enemies.
- Four Events contain 13 terminal Choices and preserve their frozen flag/effect topology.
- Four Pickups and one Shop preserve their frozen reward/offer references.

## Failure and side-effect rules

- Inspect-only validation produces zero dirty packages and zero saves.
- A wrong class, duplicate identity, invalid reference, failed dependency, forbidden reference, or structural mismatch is a failure even if the object passes generic Data Validation.
- A strict-only mismatch is reported separately from structural failure.
- The second `SeedMissing + CompareSeedDefaults` run must create/save zero packages and leave every selected `.uasset` SHA-256 unchanged.
- Tests must use transient objects or read-only real assets; Automation must not create, save, repair, or delete Production packages.
