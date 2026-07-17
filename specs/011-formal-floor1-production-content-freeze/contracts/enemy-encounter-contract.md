# Contract: SerpentWood Enemy and Encounter

## Enemy package contract

Each archetype owns exactly one `UEnemyDefinition` and one `UEnemyBehaviorDefinition`. The Enemy uses `DefaultPhaseId=Default`; every PartSlot has a non-null PartDefinition, null BehaviorOverride and an explicit InitialIntentSetId matching that slot.

| EnemyId | Parts | Total HP | Total EXP |
|---|---:|---:|---:|
| `Enemy.SerpentWood.BrushSnake` | Head, Body | 16 | 2 |
| `Enemy.SerpentWood.MoltGuard` | Head, Carapace, Tail | 28 | 4 |
| `Enemy.SerpentWood.RootStalker` | Head, Coil | 26 | 4 |
| `Enemy.SerpentWood.ShallowGuardian` | Head, Body, Tail, Crest | 52 | 9 |

Exactly 11 PartDefinitions exist. Every PartId is `SerpentWood.<Archetype>.<Part>` and every `KnockdownRewardCard` is null until the separate Aid/Destroy/Withdraw P0 is resolved.

## Behavior contract

- One `Default` phase per Behavior.
- One IntentSet per PartSlot.
- `SelectorMode=Sequence`; authored order is the loop order.
- No selector rules, cooldowns or fallback intent are required.
- Damage intents use the approved `Damage/Initiative/Resistance` triple.
- Poison/Slow/Twilight/Shield intents use `ResistanceValue=0` and `Duration=0`.
- Player Slow uses the existing default random-unique hand-affliction selection with `TargetCardCount=1`.
- Player Twilight uses the existing all-current-hand behavior; Duration never means target count.

The exact 24 intents and numbers are canonical in [data-model.md](../data-model.md). No Effect/Target combination outside the current authoring matrix is permitted.

## Encounter contract

| EncounterId | Exact slots | HP |
|---|---|---:|
| `Encounter.SerpentWood.Scout` | `Scout=BrushSnake` | 16 |
| `Encounter.SerpentWood.MoltGuard` | `Guard=MoltGuard` | 28 |
| `Encounter.SerpentWood.Ambush` | `Left=BrushSnake`, `Right=BrushSnake` | 32 |
| `Encounter.SerpentWood.RootStalker` | `Stalker=RootStalker` | 26 |
| `Encounter.SerpentWood.EliteSentinel` | `Guard=MoltGuard`, `Scout=BrushSnake` | 44 |
| `Encounter.SerpentWood.ShallowGuardian` | `Guardian=ShallowGuardian` | 52 |

Acceptance invariants:

1. Six EncounterDefinitionIds are unique and non-empty.
2. EnemySlotIds are unique within each Encounter and preserve authored order.
3. Every slot references one of the four SerpentWood EnemyDefinitions.
4. Single Encounter enemy count is at most two.
5. HP curve is `16 → 26–32 → 44 → 52`.
6. `bBoss=true` is configured only on `Floor.Main.01/Node.Guardian.01`; Encounter and Enemy types receive no boss field or special case.
7. Encounter definitions do not store Host, formation, art, world position or scene retirement behavior.

## Deferred authoring

SceneEnemyHost prefab, multi-enemy staging, PartActor visual mode, hit bounds, animation style, art, audio and world transforms are not frozen. TrainingWarrior may be consulted as a pipeline example but cannot be referenced by SerpentWood definitions.
