# Contract: Refactored Backpack WBP Boundary

## `WBP_BackpackScreen`

Parent remains `UWacomBackpackScreen`.

### Required hosts

| Binding | Responsibility |
|---|---|
| `WorkspaceHost` | Receives one runtime Workspace widget for the active zone |
| `ZoneRackHost` | Receives the persistent right-side rack |
| `DeleteTargetHost` | Receives passive delete target visual/hit region |
| `CardDetailLayer` | Existing card detail presentation above workspace cards |

### Optional controls

| Binding | Responsibility |
|---|---|
| `TitleText` | Backpack title |
| `GoldText` | Current gold |
| `CloseButton` | Close request only |
| `ArrangeAllButton` | Emits arrange-current-zone intent |
| `DeleteConfirmHost` | Hosts one confirmation modal; C++ fallback may create it |

## `WBP_BackpackWorkspace`

Parent: `UWacomBackpackWorkspaceWidget`.

| Binding | Responsibility |
|---|---|
| `CardCanvas` | Runtime card entries positioned by C++ layout solver |
| `SelectionMarquee` | Hit-test-invisible selection rectangle |
| `CarryLayer` | Optional overlay for carried cards; may be the same Canvas if ZOrder is sufficient |
| `EmptyStateText` | Optional active-zone empty state |

The WBP does not create runtime card entries, calculate fan transforms, capture Run state or submit commands.

## `WBP_BackpackZoneRack` and entry

Parent classes: `UWacomBackpackZoneRackWidget` and `UWacomBackpackZoneRackEntryWidget`.

- Rack receives ordered zone view records from Screen and reconciles entries by ZoneKey.
- Entry displays title, count/capacity, active state and drop-preview state.
- Entry emits activate/hover intent; it does not validate or mutate zones.
- SpecialZone entries retain Owner identity and battle-ready information from Snapshot.
- Burden entry may collapse when empty if current UI rule remains desired; if active and becomes empty, Screen selects the next valid zone deterministically.

## Card entry

Reuse/refactor `UWacomDeckCardWidget` or a focused workspace subclass:

- bind `BackpackCardView` to `WBP_BackpackCardView`, a passive `UWacomRetainedCardViewWidget` containing one invalidation-driven, non-phase Retainer and the authored `WBP_FirstPersonCardView`;
- disable the inner `SurfaceFoilOverlay` for this static-retained instance, collapse it and release its material brush; the shared battle/first-person card face keeps its normal default policy;
- keep fake-3D, first-person surface effects and gesture semantics out of the backpack wrapper;
- display workspace feedback and badges outside the retained card face;
- expose selected/current/read-only visual state;
- emit pointer down/hover intent;
- never construct the old persistent-operation replacement itself;
- never infer rule legality from list position.

## Delete confirmation

- One modal for the entire carried collection.
- Shows card count and expected total gold.
- Confirm/cancel are intents to Screen command flow.
- It does not retain authority; confirm triggers Run revalidation.

## Fallback and migration

- C++ fallback must instantiate the same Workspace/ZoneRack contract, not the old simultaneous multi-zone rule/input path.
- Old `DeleteZoneHost`, `BattleDeckZoneHost`, `FluxContentDropTargetHost`, `SpecialZonesHost` and `BurdenZoneHost` are removed from the formal binding contract after migration.
- Old `UWacomCardDragOperation` and `UWacomZoneDropTarget` must not remain an alternative input owner in the final state.
- Missing optional visuals/materials may degrade appearance but cannot change behavior.

## Presentation style

Workspace layout/motion/material values come from an App presentation style asset or Widget class defaults with Chinese tooltips. WBP may override appearance, but cannot alter atomicity, source identity, selection eligibility or release semantics.
