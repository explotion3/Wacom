# Data Model: Floor 1 Preview 启动层

## 1. Preview runtime selection

### Base journey-resolution seam

| Field | Contract |
|---|---|
| Owner | `AWacomGameMode` |
| API | non-reflection virtual `UWacomJourneyDefinition* ResolveJourneyDefinitionForNewRun()` |
| Base result | exact `DefaultJourneyDefinition` pointer |
| Consumer | `AWacomPlayerController::BeginPlay()` before `URunSession::Initialize()` |
| Persistence | none |

### Preview Journey

| Field | Preview value |
|---|---|
| Outer | active `AWacomRunFloorPreviewGameMode` instance |
| Flags | `RF_Transient`; never standalone/public/saved |
| `JourneyId` | `Journey.Preview.<FloorId>` |
| `DisplayName` | `[Preview] <Floor DisplayName>`; falls back to FloorId text if Floor title is empty |
| `SupportedCharacters` | one entry: resolved `DefaultCharacter` |
| `Floors` | one entry: exact Descriptor `FloorDefinition` |
| `PhaseBudgets` | current `UWacomJourneyDefinition` constructor defaults |
| Decay curves | current `UWacomJourneyDefinition` constructor defaults |
| `SuccessTerminalNode` | empty |

The transient Journey is static input to the existing Run initialization. It does not become a Production asset and does not alter the Floor.

### Cached identity

| Field | Meaning | Validation |
|---|---|---|
| `PreviewJourney` | cached transient Journey | non-null, correct outer and flags |
| `ResolvedDescriptor` | Descriptor used on first resolution | still valid and still the unique world Descriptor |
| `ResolvedFloor` | exact Floor pointer | still equals Descriptor Floor and `PreviewJourney->Floors[0]` |
| `ResolvedFloorId` | identity at first resolution | non-empty and unchanged |

State transition:

```text
Unresolved
  -> invalid environment/descriptor/character -> Rejected (no object)
  -> valid PIE facts -> Resolved (create one transient Journey)

Resolved
  -> identical facts -> return same object
  -> any identity drift -> RejectedDrift (keep old object owned, return null)
```

## 2. Preview GameMode Blueprint

| Field | Required value |
|---|---|
| Package | `/Game/Wacom/Run/Preview/GM_WacomRunFloorPreview` |
| Asset type | `UBlueprint` |
| Parent | exact `AWacomRunFloorPreviewGameMode` |
| `PlayerControllerClass` | exact `GM_Wacom` CDO value |
| `DefaultPawnClass` | exact `GM_Wacom` CDO value |
| `DefaultCharacter` | exact `GM_Wacom` CDO value |
| `BattleHUDClass` | exact `GM_Wacom` CDO value |
| `ExplorationHUDClass` | exact `GM_Wacom` CDO value |
| `JourneySummaryScreenClass` | exact `GM_Wacom` CDO value |
| `DefaultJourneyDefinition` | null |

An existing package is inspect-only. Any mismatch is a blocking authority error; it is never resynchronized.

## 3. Production map bootstrap state

| Field | Required value |
|---|---|
| Package | `/Game/Wacom/Maps/Run/L_Run_Floor_Main_01` |
| `WorldSettings.DefaultGameMode` | generated class of Preview GameMode Blueprint |
| Preview PlayerStart count | exactly 1 |
| PlayerStart object/label | `PlayerStart_FloorMain01Preview` |
| PlayerStart transform | exact unique `Node.Entry` Anchor actor transform |
| Run binding | none |

All pre-existing Spec 015 scene entities remain invariant:

```text
1 Descriptor
20 Anchors
21 Paths
4 BranchTargets
16 content Hosts
8 enemy Hosts
11 viewpoints
20 graybox pads
```

Map mutation eligibility:

- scene contract validates before mutation;
- exactly one Entry Anchor exists and its transform is finite;
- no PlayerStart exists, or the exact Preview PlayerStart is already the only one and correct;
- current GameMode override is null, `GM_Wacom_C`, or the Preview class; any other override requires an authority decision.

## 4. Seed report

| Field | Meaning |
|---|---|
| `ManifestCount` | always 2 |
| `CreatedCount` | new Preview Blueprint packages created |
| `ModifiedCount` | existing map packages changed |
| `ExistingCount` | already-conforming packages |
| `SavedCount` | packages actually saved |
| `FailedCount` | package/preflight/reload/idempotence failures |
| `FirstPass` | create/modify/save facts |
| `SecondPass` | must be `Created=0, Modified=0, Saved=0, Failed=0` |
| `Diagnostics` | stable, package-scoped failure text |

## 5. Persistence and identity matrix

| Fact | Stable/persisted? | Authority |
|---|---|---|
| Production Floor/Node/Edge IDs | yes | existing Floor DataAsset |
| Preview JourneyId | runtime-stable only | Preview GameMode derivation |
| Preview Journey object | no | GameMode transient cache |
| Blueprint package and parent | yes | Preview asset contract |
| Map GameMode override | yes, temporary | map World Settings |
| Preview PlayerStart identity/transform | yes, temporary | map actor |
| Player Run state | in-memory existing behavior | `URunSession` |
| SaveGame | unchanged/disabled | existing App/Run contract |

## 6. Release blocker

The Preview Blueprint itself may remain as a reusable editor tool, but a Production map referencing it is not shippable. Formal Journey readiness removes the map override and replaces it with the Production startup path; the Preview PlayerStart is retained or replaced only after spawn ownership is defined by that path.
