# Contract: SerpentWood Graybox Enemy Hosts

## Package and definition mapping

| Archetype | Blueprint package | EnemyDefinition | Parts |
|---|---|---|---|
| BrushSnake | `/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/BrushSnake/BP_EnemyHost_BrushSnake_Graybox` | `/Game/Wacom/Data/Enemies/SerpentWood/BrushSnake/DA_Enemy_BrushSnake` | Head, Body |
| MoltGuard | `/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/MoltGuard/BP_EnemyHost_MoltGuard_Graybox` | `/Game/Wacom/Data/Enemies/SerpentWood/MoltGuard/DA_Enemy_MoltGuard` | Head, Carapace, Tail |
| RootStalker | `/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/RootStalker/BP_EnemyHost_RootStalker_Graybox` | `/Game/Wacom/Data/Enemies/SerpentWood/RootStalker/DA_Enemy_RootStalker` | Head, Coil |
| ShallowGuardian | `/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/ShallowGuardian/BP_EnemyHost_ShallowGuardian_Graybox` | `/Game/Wacom/Data/Enemies/SerpentWood/ShallowGuardian/DA_Enemy_ShallowGuardian` | Head, Body, Tail, Crest |

## Blueprint structure

- Parent class: `AWacomBattleEnemyActor`.
- EnemyDefinition: exact table value.
- Part child actors/components: generated through `FWacomBattleSceneEnemyHostAuthoring::SyncPartsFromDefinition` during initial creation only.
- Part identity/order: exact Definition part array; no hand-authored substitute IDs.
- Part hit bounds: visible and independently targetable in graybox staging.
- No RunMapNodeBinding, PersistentId, EncounterDefinition, Level Blueprint dependency, or runtime spawn script.
- No modification of `BP_EnemyHost_Snake` or any shared/prototype Host.

## Placeholder boundary

- Allowed presentation dependency root: `/Game/Wacom/Art/Placeholders`.
- Allowed engine primitives: `/Engine/BasicShapes` and default engine materials used only for graybox presentation.
- Forbidden: modifying or saving any placeholder, material, Paper2D source, EnemyDefinition, EnemyPart, card, DreamShader, or other Agent-owned package.
- Every non-engine presentation dependency must be listed in the seed/validation report.

## Encounter instance mapping

| Encounter node | Slot | Blueprint archetype |
|---|---|---|
| `Node.Main.01` | `Scout` | BrushSnake |
| `Node.Route.A.02` | `Guard` | MoltGuard |
| `Node.Route.B.01` | `Left` | BrushSnake |
| `Node.Route.B.01` | `Right` | BrushSnake |
| `Node.Route.D.01` | `Stalker` | RootStalker |
| `Node.Main.02` | `Guard` | MoltGuard |
| `Node.Main.02` | `Scout` | BrushSnake |
| `Node.Guardian.01` | `Guardian` | ShallowGuardian |

`EnemySlotId` is instance-specific. The Blueprint prefab does not hardcode an Encounter slot.

## Seed-only rule

On first creation, the tool may create the Blueprint, set the matching EnemyDefinition, synchronize parts, apply controlled placeholder presentation defaults, compile, save, and reload-validate it.

On every later run, the tool may only load, compile/inspect read-only, and report drift. It must not call part synchronization, modify defaults, compile-and-save, or resave the Blueprint.

## Validation

- 4/4 packages load as Blueprint assets with the correct generated parent.
- 4/4 compile without errors.
- 11/11 expected part identities are present exactly once.
- Every part child maps to the matching formal EnemyPartDefinition.
- No prefab hardcodes an EnemySlotId.
- All non-engine visual dependencies are controlled placeholders.
- Real map composition has 8/8 correct slot-to-prefab instances.
