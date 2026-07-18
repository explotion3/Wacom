# Floor 1 Production Asset Manifest

## Authority

This manifest is the complete write allowlist for Spec 014. Package names are extensionless Unreal package names. Object names equal package leaves. Stable IDs and references are authoritative; display text and balancing numbers are initial seed defaults after creation.

The manifest contains exactly 46 writable packages in three dependency-ordered groups: 12 Cards, 19 EnemyGraph, and 15 NodeDefinitions. The four Shop dependencies listed after the table are read-only and never enter a writer lease.

## Cards (12 writable packages)

| # | Class | Stable CardId | Package |
|---:|---|---|---|
| 1 | `UCardDefinition` | `Reward.SerpentWood.HerbalPoultice` | `/Game/Wacom/Data/Cards/Rewards/SerpentWood/DA_Card_HerbalPoultice` |
| 2 | `UCardDefinition` | `Reward.SerpentWood.HunterSnare` | `/Game/Wacom/Data/Cards/Rewards/SerpentWood/DA_Card_HunterSnare` |
| 3 | `UCardDefinition` | `Reward.SerpentWood.MoltWard` | `/Game/Wacom/Data/Cards/Rewards/SerpentWood/DA_Card_MoltWard` |
| 4 | `UCardDefinition` | `Card.Run.SerpentSigil` | `/Game/Wacom/Data/Cards/Run/SerpentWood/DA_Card_SerpentSigil` |
| 5 | `UCardDefinition` | `Reward.SerpentWood.BrushSnake.Aid` | `/Game/Wacom/Data/Cards/Rewards/SerpentWood/BrushSnake/DA_Card_BrushSnake_Aid` |
| 6 | `UCardDefinition` | `Reward.SerpentWood.BrushSnake.Destroy` | `/Game/Wacom/Data/Cards/Rewards/SerpentWood/BrushSnake/DA_Card_BrushSnake_Destroy` |
| 7 | `UCardDefinition` | `Reward.SerpentWood.MoltGuard.Aid` | `/Game/Wacom/Data/Cards/Rewards/SerpentWood/MoltGuard/DA_Card_MoltGuard_Aid` |
| 8 | `UCardDefinition` | `Reward.SerpentWood.MoltGuard.Destroy` | `/Game/Wacom/Data/Cards/Rewards/SerpentWood/MoltGuard/DA_Card_MoltGuard_Destroy` |
| 9 | `UCardDefinition` | `Reward.SerpentWood.RootStalker.Aid` | `/Game/Wacom/Data/Cards/Rewards/SerpentWood/RootStalker/DA_Card_RootStalker_Aid` |
| 10 | `UCardDefinition` | `Reward.SerpentWood.RootStalker.Destroy` | `/Game/Wacom/Data/Cards/Rewards/SerpentWood/RootStalker/DA_Card_RootStalker_Destroy` |
| 11 | `UCardDefinition` | `Reward.SerpentWood.ShallowGuardian.Aid` | `/Game/Wacom/Data/Cards/Rewards/SerpentWood/ShallowGuardian/DA_Card_ShallowGuardian_Aid` |
| 12 | `UCardDefinition` | `Reward.SerpentWood.ShallowGuardian.Destroy` | `/Game/Wacom/Data/Cards/Rewards/SerpentWood/ShallowGuardian/DA_Card_ShallowGuardian_Destroy` |

## EnemyGraph (19 writable packages)

| # | Class | Stable identity | Package |
|---:|---|---|---|
| 13 | `UEnemyBehaviorDefinition` | `SerpentWood.BrushSnake.Behavior` | `/Game/Wacom/Data/Enemies/SerpentWood/BrushSnake/DA_Behavior_BrushSnake` |
| 14 | `UEnemyPartDefinition` | `SerpentWood.BrushSnake.Head` | `/Game/Wacom/Data/Enemies/SerpentWood/BrushSnake/DA_Part_BrushSnake_Head` |
| 15 | `UEnemyPartDefinition` | `SerpentWood.BrushSnake.Body` | `/Game/Wacom/Data/Enemies/SerpentWood/BrushSnake/DA_Part_BrushSnake_Body` |
| 16 | `UEnemyDefinition` | `Enemy.SerpentWood.BrushSnake` | `/Game/Wacom/Data/Enemies/SerpentWood/BrushSnake/DA_Enemy_BrushSnake` |
| 17 | `UEnemyBehaviorDefinition` | `SerpentWood.MoltGuard.Behavior` | `/Game/Wacom/Data/Enemies/SerpentWood/MoltGuard/DA_Behavior_MoltGuard` |
| 18 | `UEnemyPartDefinition` | `SerpentWood.MoltGuard.Head` | `/Game/Wacom/Data/Enemies/SerpentWood/MoltGuard/DA_Part_MoltGuard_Head` |
| 19 | `UEnemyPartDefinition` | `SerpentWood.MoltGuard.Carapace` | `/Game/Wacom/Data/Enemies/SerpentWood/MoltGuard/DA_Part_MoltGuard_Carapace` |
| 20 | `UEnemyPartDefinition` | `SerpentWood.MoltGuard.Tail` | `/Game/Wacom/Data/Enemies/SerpentWood/MoltGuard/DA_Part_MoltGuard_Tail` |
| 21 | `UEnemyDefinition` | `Enemy.SerpentWood.MoltGuard` | `/Game/Wacom/Data/Enemies/SerpentWood/MoltGuard/DA_Enemy_MoltGuard` |
| 22 | `UEnemyBehaviorDefinition` | `SerpentWood.RootStalker.Behavior` | `/Game/Wacom/Data/Enemies/SerpentWood/RootStalker/DA_Behavior_RootStalker` |
| 23 | `UEnemyPartDefinition` | `SerpentWood.RootStalker.Head` | `/Game/Wacom/Data/Enemies/SerpentWood/RootStalker/DA_Part_RootStalker_Head` |
| 24 | `UEnemyPartDefinition` | `SerpentWood.RootStalker.Coil` | `/Game/Wacom/Data/Enemies/SerpentWood/RootStalker/DA_Part_RootStalker_Coil` |
| 25 | `UEnemyDefinition` | `Enemy.SerpentWood.RootStalker` | `/Game/Wacom/Data/Enemies/SerpentWood/RootStalker/DA_Enemy_RootStalker` |
| 26 | `UEnemyBehaviorDefinition` | `SerpentWood.ShallowGuardian.Behavior` | `/Game/Wacom/Data/Enemies/SerpentWood/ShallowGuardian/DA_Behavior_ShallowGuardian` |
| 27 | `UEnemyPartDefinition` | `SerpentWood.ShallowGuardian.Head` | `/Game/Wacom/Data/Enemies/SerpentWood/ShallowGuardian/DA_Part_ShallowGuardian_Head` |
| 28 | `UEnemyPartDefinition` | `SerpentWood.ShallowGuardian.Body` | `/Game/Wacom/Data/Enemies/SerpentWood/ShallowGuardian/DA_Part_ShallowGuardian_Body` |
| 29 | `UEnemyPartDefinition` | `SerpentWood.ShallowGuardian.Tail` | `/Game/Wacom/Data/Enemies/SerpentWood/ShallowGuardian/DA_Part_ShallowGuardian_Tail` |
| 30 | `UEnemyPartDefinition` | `SerpentWood.ShallowGuardian.Crest` | `/Game/Wacom/Data/Enemies/SerpentWood/ShallowGuardian/DA_Part_ShallowGuardian_Crest` |
| 31 | `UEnemyDefinition` | `Enemy.SerpentWood.ShallowGuardian` | `/Game/Wacom/Data/Enemies/SerpentWood/ShallowGuardian/DA_Enemy_ShallowGuardian` |

Each Part references the Aid/Destroy Card pair for its own archetype. `KnockdownRewardCard` is null. Each Enemy references only its own Parts and Behavior. Each Behavior contains the exact `Default` phase, `Sequence` sets, and 24 total Intents frozen by Spec 011.

## NodeDefinitions (15 writable packages)

| # | Class | Stable identity | Package |
|---:|---|---|---|
| 32 | `UEncounterDefinition` | `Encounter.SerpentWood.Scout` | `/Game/Wacom/Data/Encounters/SerpentWood/DA_Encounter_Scout` |
| 33 | `UEncounterDefinition` | `Encounter.SerpentWood.MoltGuard` | `/Game/Wacom/Data/Encounters/SerpentWood/DA_Encounter_MoltGuard` |
| 34 | `UEncounterDefinition` | `Encounter.SerpentWood.Ambush` | `/Game/Wacom/Data/Encounters/SerpentWood/DA_Encounter_Ambush` |
| 35 | `UEncounterDefinition` | `Encounter.SerpentWood.RootStalker` | `/Game/Wacom/Data/Encounters/SerpentWood/DA_Encounter_RootStalker` |
| 36 | `UEncounterDefinition` | `Encounter.SerpentWood.EliteSentinel` | `/Game/Wacom/Data/Encounters/SerpentWood/DA_Encounter_EliteSentinel` |
| 37 | `UEncounterDefinition` | `Encounter.SerpentWood.ShallowGuardian` | `/Game/Wacom/Data/Encounters/SerpentWood/DA_Encounter_ShallowGuardian` |
| 38 | `URunEventDefinition` | `Event.SerpentWood.CastSkin` | `/Game/Wacom/Data/Events/SerpentWood/DA_Event_CastSkin` |
| 39 | `URunEventDefinition` | `Event.SerpentWood.HunterTrace` | `/Game/Wacom/Data/Events/SerpentWood/DA_Event_HunterTrace` |
| 40 | `URunEventDefinition` | `Event.SerpentWood.MerchantRumor` | `/Game/Wacom/Data/Events/SerpentWood/DA_Event_MerchantRumor` |
| 41 | `URunEventDefinition` | `Event.SerpentWood.PoisonMarsh` | `/Game/Wacom/Data/Events/SerpentWood/DA_Event_PoisonMarsh` |
| 42 | `URunPickupDefinition` | `Pickup.SerpentWood.HerbCache` | `/Game/Wacom/Data/Pickups/SerpentWood/DA_Pickup_HerbCache` |
| 43 | `URunPickupDefinition` | `Pickup.SerpentWood.HunterCache` | `/Game/Wacom/Data/Pickups/SerpentWood/DA_Pickup_HunterCache` |
| 44 | `URunPickupDefinition` | `Pickup.SerpentWood.MoltCache` | `/Game/Wacom/Data/Pickups/SerpentWood/DA_Pickup_MoltCache` |
| 45 | `URunPickupDefinition` | `Pickup.SerpentWood.SerpentSigil` | `/Game/Wacom/Data/Pickups/SerpentWood/DA_Pickup_SerpentSigil` |
| 46 | `UShopDefinition` | `Shop.SerpentWood.Wayfarer` | `/Game/Wacom/Data/Shops/SerpentWood/DA_Shop_Wayfarer` |

## Read-only dependencies

These packages must load before NodeDefinitions creation. Their SHA-256 is recorded before and after the operation. They are not writable packages and must never be included in `AcquireWriter`:

```text
/Game/Wacom/Data/Cards/BugGirl/StarterPack/DA_Card_Starter_ChitinWard
/Game/Wacom/Data/Cards/BugGirl/StarterPack/DA_Card_Starter_AntennaSearch
/Game/Wacom/Data/Cards/BugGirl/StarterPack/DA_Card_Starter_MoltCut
/Game/Wacom/Data/Cards/Rewards/DA_Card_PoisonFang
```

## Count and closure invariants

- Total writable packages: exactly 46.
- Total by class: 12 Card, 4 Behavior, 11 Part, 4 Enemy, 6 Encounter, 4 RunEvent, 4 Pickup, 1 Shop.
- Cards have no dependencies on other new packages.
- EnemyGraph may reference only Cards in this manifest and EnemyGraph entries in the same archetype.
- NodeDefinitions may reference Cards, EnemyGraph, or the four explicit read-only Shop cards.
- No reference closure may contain `Debug`, `Authoring`, `Test.*`, `BadgeDisplayTests`, Snake prototype, or TrainingWarrior content.
