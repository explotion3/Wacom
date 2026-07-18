# Data Model: Floor 1 Production Floor / Map / Host 灰盒基线

## 1. Binary package manifest

| Group | Expected class | Stable purpose | Package |
|---|---|---|---|
| Floor | `UWacomFloorMapDefinition` | `Floor.Main.01` graph authority | `/Game/Wacom/Data/Map/Production/DA_Floor_Main_01` |
| EnemyHosts | Blueprint generated from `AWacomBattleEnemyActor` | BrushSnake graybox prefab | `/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/BrushSnake/BP_EnemyHost_BrushSnake_Graybox` |
| EnemyHosts | Blueprint generated from `AWacomBattleEnemyActor` | MoltGuard graybox prefab | `/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/MoltGuard/BP_EnemyHost_MoltGuard_Graybox` |
| EnemyHosts | Blueprint generated from `AWacomBattleEnemyActor` | RootStalker graybox prefab | `/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/RootStalker/BP_EnemyHost_RootStalker_Graybox` |
| EnemyHosts | Blueprint generated from `AWacomBattleEnemyActor` | ShallowGuardian graybox prefab | `/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/ShallowGuardian/BP_EnemyHost_ShallowGuardian_Graybox` |
| Scene | Blueprint generated from `AActor` | non-interactive Exit marker | `/Game/Wacom/Run/SceneActors/Graybox/BP_WacomRunFloorEntranceMarker_Graybox` |
| Scene | `UWorld` | isolated Production Floor 1 world | `/Game/Wacom/Maps/Run/L_Run_Floor_Main_01` |

Total: 7 packages = 1 Floor + 5 Blueprint + 1 World.

## 2. Floor node model

Each node stores:

- `NodeId: FName`
- `NodeType: EWacomMapNodeType`
- `MapPosition: FVector2D`
- `bAllowCamp: bool`
- one typed payload appropriate to NodeType
- `bBoss` for the Guardian encounter

Stable identity is Floor-scoped. A content Host PersistentId is derived:

```text
PersistentId = FloorId + "." + NodeId
```

Example:

```text
Floor.Main.01 + Node.Route.A.01
= Floor.Main.01.Node.Route.A.01
```

The full canonical table is in `contracts/floor1-production-floor-contract.md`.

## 3. Edge model

Each edge stores:

- `EdgeId: FName`
- `FromNodeId: FName`
- `ToNodeId: FName`

Edges are directed. Every edge has exactly one world Path actor. Only the four outgoing branch edges from `Node.Junction.01` and `Node.Junction.02` have BranchTarget actors.

## 4. Scene actor model

### FloorSceneDescriptor

| Field | Value |
|---|---|
| Count | 1 |
| FloorDefinition | `/Game/Wacom/Data/Map/Production/DA_Floor_Main_01` |
| Purpose | Declares the Floor represented by the loaded world |

### NodeAnchor

| Field | Contract |
|---|---|
| Count | 20 |
| NodeId | exactly one per Floor node |
| NodeType | mirrors Floor node type |
| Transform | initial graybox layout; tunable |

### Path

| Field | Contract |
|---|---|
| Count | 21 |
| EdgeId | exactly one per Floor edge |
| Source/target | match edge endpoints |
| Spline | clear traversal lane; tunable shape |

### BranchTarget

| Field | Contract |
|---|---|
| Count | 4 |
| Edge | `Edge.Route.A.01`, `Edge.Route.B.01`, `Edge.Route.C.01`, `Edge.Route.D.01` |
| Destination | corresponding first branch node Anchor |

### ContentHost

| Field | Contract |
|---|---|
| Count | 16 |
| Node binding | exactly one per non-Navigation node |
| NodeType | exact Floor node type |
| PersistentId | `Floor.Main.01.<NodeId>` |
| Payload mirror | exact Definition/InteractionDefinition or explicit Exit marker semantics |

## 5. Enemy Host prefab model

Each enemy Blueprint contains:

- parent class `AWacomBattleEnemyActor`
- exact formal `UEnemyDefinition`
- authoring mode compatible with scene-host placement
- child part actor components synchronized from all EnemyParts in Definition order
- placeholder presentation references confined to `/Game/Wacom/Art/Placeholders`
- no Run node binding and no content Host PersistentId

| Archetype | EnemyDefinition | Part count |
|---|---|---:|
| BrushSnake | `/Game/Wacom/Data/Enemies/SerpentWood/BrushSnake/DA_Enemy_BrushSnake` | 2 |
| MoltGuard | `/Game/Wacom/Data/Enemies/SerpentWood/MoltGuard/DA_Enemy_MoltGuard` | 3 |
| RootStalker | `/Game/Wacom/Data/Enemies/SerpentWood/RootStalker/DA_Enemy_RootStalker` | 2 |
| ShallowGuardian | `/Game/Wacom/Data/Enemies/SerpentWood/ShallowGuardian/DA_Enemy_ShallowGuardian` | 4 |

Total prefab part identities: 11.

## 6. Encounter scene composition

| NodeId | Encounter | Slot → enemy Host prefab | Instance count |
|---|---|---|---:|
| `Node.Main.01` | Scout | `Scout → BrushSnake` | 1 |
| `Node.Route.A.02` | MoltGuard | `Guard → MoltGuard` | 1 |
| `Node.Route.B.01` | Ambush | `Left → BrushSnake`, `Right → BrushSnake` | 2 |
| `Node.Route.D.01` | RootStalker | `Stalker → RootStalker` | 1 |
| `Node.Main.02` | EliteSentinel | `Guard → MoltGuard`, `Scout → BrushSnake` | 2 |
| `Node.Guardian.01` | ShallowGuardian | `Guardian → ShallowGuardian` | 1 |

Total: 6 Encounter content Hosts and 8 enemy scene Host instances.

## 7. Viewpoint model

Battle, Event, and Shop Hosts use the existing viewpoint staging contract.

| Interaction type | Host count | Viewpoint count |
|---|---:|---:|
| Battle | 6 | 6 |
| RunEvent | 4 | 4 |
| Shop | 1 | 1 |
| Pickup | 4 | 0 required |
| FloorEntrance marker | 1 | 0 |

Total dedicated viewpoints: 11. Viewpoint transforms face the staged content from the player's arrival direction and remain tunable.

## 8. Seed state model

For every target entry:

```text
Missing -> EligibleForCreate -> Created -> Configured -> Saved -> ReloadValidated
ExistingExpectedClass -> InspectOnly -> Valid/Invalid (never Saved)
ExistingWrongClassOrName -> Collision -> WholeGroupBlocked
DependencyMissingOrInvalid -> DependencyFailure -> WholeGroupBlocked
```

No `Force`, `Replace`, `Delete`, or `Regenerate` transition exists.

## 9. Validation report model

The report contains:

- command/options and selected groups
- target package state for all entries
- created/saved/inspected/error counts
- Floor node/edge/type/reachability/dominance facts
- scene actor counts and exact binding mismatch list
- Host payload/PersistentId facts
- Encounter slot/enemy/part composition facts
- Blueprint compile results
- placeholder reference list
- forbidden reference list
- expected and actual changed package paths
- second-run zero-save facts

## 10. Readiness state

| Capability | End-of-slice state |
|---|---|
| Floor 1 graph DataAsset | Implemented and locally validated |
| Floor 1 Production graybox world | Implemented and locally validated |
| Floor 1 scene binding | Implemented and locally validated |
| SerpentWood graybox enemy prefabs | Implemented; placeholder-art blocked |
| Production Journey | Not created |
| Floor 2/3 world assets | Not created |
| FloorEntrance interaction/travel | Not implemented |
| Full Journey Golden Path PIE | Blocked |
| Release art/polish/balance | Blocked |
