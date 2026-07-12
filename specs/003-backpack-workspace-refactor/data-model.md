# Data Model: Backpack Workspace Refactor

## Ownership Overview

```text
URunSession / FRunState (WacomRun, authoritative)
├── physical card zone membership
├── capacity / burden / gold
└── storage revision + state-change broadcast

Backpack Workspace State (WacomApp, transient per Run)
├── active zone key
├── zone layout maps
└── no SaveGame serialization

Backpack Interaction Model (WacomApp, active Screen only)
├── selection
├── marquee
├── carry session
└── pending delete confirmation snapshot
```

## 1. `FWacomBackpackZoneKey`

Identifies one region shown in the right rack and central workspace.

| Field | Type | Meaning |
|---|---|---|
| `ZoneKind` | `EZoneKind` | Backpack/flux content, BattleDeck, BurdenZone, or SpecialZone |
| `OwnerInstanceId` | `FGuid` | Valid only for SpecialZone; normalized invalid for other zones |

### Validation

- A SpecialZone key is valid only when `OwnerInstanceId` is valid and resolves to the current Snapshot owner.
- Non-SpecialZone keys always normalize `OwnerInstanceId` to invalid.
- Delete is an action target, not a workspace zone key.
- Keys must support equality and hashing for layout maps.

## 2. Run Batch Move Contract

### Request

| Field | Meaning |
|---|---|
| `InstanceIds` | Ordered, unique physical card identities to move |
| `ExpectedSource` | One zone key matching the active workspace at pickup/submit |
| `Target` | One different zone key |
| `ExpectedStorageRevision` | Required revision captured from the source Snapshot; commit requires exact equality with current Run storage revision |

### Validation/Result

| Field | Meaning |
|---|---|
| `bAllowed` / `bSucceeded` | Whole-request outcome |
| `DisabledReason` | Stable existing/new Run reason key for App toast mapping |
| `AffectedCount` | Zero on failure; request size on success |
| `StorageRevision` | Revision after success or unchanged revision on failure |

### Invariants

- Empty arrays, duplicate IDs, invalid IDs, missing definitions and stale source membership fail.
- A revision mismatch always fails before working-state mutation; Workspace callers cannot opt out of the guard.
- Every card must be physically owned and movable by existing Run rules.
- Validation and mutation run on a working state; the authoritative state changes only after all items succeed.
- Success recomputes dependent burden/capacity state as one transaction and broadcasts once.
- Failure changes no zone array, SpecialZone entry, pressure, gold or revision.

## 3. Run Batch Delete Contract

### Request

| Field | Meaning |
|---|---|
| `InstanceIds` | Ordered, unique physical card identities to delete |
| `ExpectedSource` | Active workspace zone used as a stale-source guard |
| `ExpectedStorageRevision` | Required revision captured for preview/confirmation; confirm requires exact equality with current Run storage revision |

### Preview/Result

| Field | Meaning |
|---|---|
| `bAllowed` / `bSucceeded` | Whole-request outcome |
| `DisabledReason` | Stable failure reason |
| `CardCount` | Number of requested/removed cards |
| `TotalGoldReward` | Sum of current per-card reward; zero on failed commit |
| `StorageRevision` | Post-commit or unchanged revision |

### Invariants

- Preview may show the total reward but is not authority for commit.
- Confirm always revalidates against current Run state.
- A revision mismatch always fails the entire delete request and restores the suspended carry state.
- All capacity-provider and permanent-remove restrictions are evaluated for the collection on working state.
- Success grants gold exactly once after all removals succeed and broadcasts once.
- Cancel and failure preserve cards, gold, pressure, revision and carry snapshot.

## 4. `FWacomBackpackWorkspaceLayoutEntry`

Presentation-only data keyed by Card InstanceId.

| Field | Type | Meaning |
|---|---|---|
| `NormalizedPosition` | `FVector2D` | Card anchor within finite workspace |
| `AngleDegrees` | `float` | Display rotation retained from group placement |
| `LayerRank` | integer | Relative ZOrder only; never rule order |
| `bHasManualPlacement` | bool | Distinguishes free placement from default solver position |

### Validation

- Position is clamped so at least 30% of the rendered card remains visible.
- Angle is presentation-only and finite.
- Layer ranks may be compacted without changing relative order.
- Entries are removed when InstanceId disappears, changes zone, becomes invisible/read-only, or the Run identity changes.
- `Arrange All` removes manual entries for the active zone and uses the default solver.

## 5. `FWacomBackpackWorkspaceRunState`

Transient App state associated with one `URunSession` identity.

| Field | Meaning |
|---|---|
| `RunIdentity` | Weak identity of the bound Run session |
| `ActiveZone` | Last valid active zone, if any |
| `LayoutsByZone` | ZoneKey → InstanceId → LayoutEntry |
| `LastObservedStorageRevision` | Reconcile aid, not authority |

### Lifecycle

```text
Bind first Run -> create empty state
Screen deactivate -> retain layouts, discard active interaction
Screen reconstruct same Run -> restore layouts
Snapshot refresh -> reconcile entries by InstanceId and physical ZoneKey
Bind different/new Run -> discard prior workspace state
GameInstance shutdown -> discard all workspace state
```

## 6. Workspace Card View Record

Derived from `FRunStorageCardView` plus App presentation fields.

| Field | Meaning |
|---|---|
| `InstanceId` | Stable identity |
| `ZoneKey` | Physical source zone |
| `CardViewData` | Existing card face data |
| `bMovable` | Physical and allowed to enter selection/carry |
| `bReadOnlyProjection` | Visible but excluded from commands |
| `LayoutTransform` | Resolved default/manual/carry transform |
| `SelectionState` | Unselected/selected/current-carry for styling only |

The Widget does not infer `bMovable` from where it was created; Screen/coordinator derives it from Snapshot facts and Run preview policy.

## 7. Selection Model

| Field | Meaning |
|---|---|
| `OrderedSelectedIds` | Stable ordered unique InstanceIds |
| `AnchorId` | Optional last direct selection for deterministic ordering |
| `MarqueeStart/Current` | Workspace-local drag rectangle while marquee is active |
| `SelectionMode` | Replace or toggle (Ctrl) |

### Transitions

- Blank click → empty selection.
- Card click → replace with one card.
- Ctrl card click → toggle one card.
- Blank drag → replace by card-center hits.
- Ctrl blank drag → toggle card-center hits against selection snapshot from drag start.
- Ctrl+A → all movable physical cards in stable workspace order.
- Region switch, Deactivate, invalidating refresh → clear.

## 8. Carry Session

| Field | Meaning |
|---|---|
| `RemainingIds` | Stable fan order; default current is the final/rightmost visual card |
| `CurrentIndex` | Selected release card, clamped; default points to rightmost |
| `DefaultIndex` | Rightmost initial/current default index after reflow |
| `PointerPosition` | Workspace/screen-local fan anchor |
| `bInitialReleaseGuardArmed` | True until pickup button's first release is consumed |
| `SourceZone` | Expected source for batch request |
| `SourceRevision` | Stale-intent hint |
| `OriginalLayoutEntries` | Restore data for cancel/failed confirmation if needed |

### State Transitions

```text
Idle
  -> Pressed/Marquee
  -> Carrying(guard armed)
  -> Carrying(guard consumed)
       -> left release current to workspace/same-zone/cross-zone
       -> wheel changes current index
       -> right release all to workspace/same-zone/cross-zone
       -> delete target -> ConfirmingDelete
       -> cancel/deactivate/region switch -> Idle

ConfirmingDelete
  -> cancel/failure -> Carrying(restored current index and ids)
  -> success -> Idle
```

### Fan Invariants

- Default rightmost/highest-Z card is immediately releasable and receives no lift.
- Only a non-default current card receives lift.
- Wheel clamps and never wraps.
- After one card leaves, remaining order is stable, fan closes, and current index clamps.
- No count/index UI is generated.

## 9. Presentation Style

Designer-facing WacomApp asset/class-default values:

- Card render size and workspace safe-visible fraction.
- Default layout spacing and margins.
- Fan maximum angle, spacing/overlap and depth step.
- Non-default current lift distance.
- Pointer follow, settle, collect and invalid-drop feedback timing.
- Selection rectangle/outline colors and zone rack active/preview/rejected styles.
- Optional card surface material/fake-3D parameters.

These values never enter `FRunState`, Run validation, capacity calculation or SaveGame.
