# Contract: Floor 1 Part Reward Assignment

## Exact Part mapping

| # | PartId | Aid CardId | Destroy CardId | Legacy `KnockdownRewardCard` |
|---:|---|---|---|---|
| 1 | `SerpentWood.BrushSnake.Head` | `Reward.SerpentWood.BrushSnake.Aid` | `Reward.SerpentWood.BrushSnake.Destroy` | null |
| 2 | `SerpentWood.BrushSnake.Body` | `Reward.SerpentWood.BrushSnake.Aid` | `Reward.SerpentWood.BrushSnake.Destroy` | null |
| 3 | `SerpentWood.MoltGuard.Head` | `Reward.SerpentWood.MoltGuard.Aid` | `Reward.SerpentWood.MoltGuard.Destroy` | null |
| 4 | `SerpentWood.MoltGuard.Carapace` | `Reward.SerpentWood.MoltGuard.Aid` | `Reward.SerpentWood.MoltGuard.Destroy` | null |
| 5 | `SerpentWood.MoltGuard.Tail` | `Reward.SerpentWood.MoltGuard.Aid` | `Reward.SerpentWood.MoltGuard.Destroy` | null |
| 6 | `SerpentWood.RootStalker.Head` | `Reward.SerpentWood.RootStalker.Aid` | `Reward.SerpentWood.RootStalker.Destroy` | null |
| 7 | `SerpentWood.RootStalker.Coil` | `Reward.SerpentWood.RootStalker.Aid` | `Reward.SerpentWood.RootStalker.Destroy` | null |
| 8 | `SerpentWood.ShallowGuardian.Head` | `Reward.SerpentWood.ShallowGuardian.Aid` | `Reward.SerpentWood.ShallowGuardian.Destroy` | null |
| 9 | `SerpentWood.ShallowGuardian.Body` | `Reward.SerpentWood.ShallowGuardian.Aid` | `Reward.SerpentWood.ShallowGuardian.Destroy` | null |
| 10 | `SerpentWood.ShallowGuardian.Tail` | `Reward.SerpentWood.ShallowGuardian.Aid` | `Reward.SerpentWood.ShallowGuardian.Destroy` | null |
| 11 | `SerpentWood.ShallowGuardian.Crest` | `Reward.SerpentWood.ShallowGuardian.Aid` | `Reward.SerpentWood.ShallowGuardian.Destroy` | null |

## Validation profile

- All eleven rows are `FormalProduction`: both explicit references are required and legacy must be null.
- Aid/Destroy resolving to the same CardDefinition would pass generic schema validation but violates this exact Floor 1 content manifest.
- TrainingWarrior/Snake legacy-only binary assets are outside this mapping and retain their authorized migration gate.

## Reward frequency

- Each processed Part knockdown produces exactly one player choice result.
- Aid or Destroy may grant one card; Withdraw grants none.
- The two references are alternatives, never two simultaneous rewards.
- Repeated Definitions create independent runtime Card Instances.
- No per-enemy claimed state, duplicate suppression, cap or replacement reward exists.

## Route yield proof

| Component | Part count |
|---|---:|
| Mandatory Scout | 2 |
| Mandatory EliteSentinel | 5 |
| Mandatory ShallowGuardian | 4 |
| Route A MoltGuard | 3 |
| Route B Ambush | 4 |
| Route C | 0 |
| Route D RootStalker | 2 |

Therefore:

```text
A/C = 2 + 3 + 5 + 4     = 14
B/C = 2 + 4 + 5 + 4     = 15
A/D = 2 + 3 + 2 + 5 + 4 = 16
B/D = 2 + 4 + 2 + 5 + 4 = 17
Full = 2 + 3 + 4 + 2 + 5 + 4 = 20
```

Reward choice adds no AP. Existing Encounter completion, Event, Treasure and Shop rules preserve Floor 1 at `8–9 / 14–15 AP`.
