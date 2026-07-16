# Backpack Real Card Pile Contract

## Formal Widget Contract

### `WBP_BackpackWorkspace`

- Parent: `UWacomBackpackWorkspaceWidget`
- Required layers: `PileFrameLayer`、`StaticCardLayer`、`MarqueeLayer`、`CarryLayer`。
- `CarryLayer` must render above static cards and must not clip fan children.
- Workspace remains the only mouse capture, marquee, card carry and pile move owner.

### `WBP_BackpackZonePile`

- Parent: `UWacomBackpackZonePileWidget`
- Required bindings: `FrameBorder`、`DragHandle`、`TitleText`、`CountText`、`StatusText`、`DropFeedback`。
- It must not contain `PreviewHost` or create card widgets.
- It is passive: it emits expand/move intent and applies resolved frame/header geometry; it never accesses `URunSession`.

### `WBP_WacomDeckCardWidget`

- Hosts exactly one `WBP_FPCardView` under the fixed card-size wrapper.
- Does not host `WBP_BackpackCardView` or a second outer effect Retainer.
- Selection/rejection overlays remain hit-test invisible.

## Scene and Identity Contract

- Every Flux, Battle, Special and Burden display card has one full Card View Key.
- Reconcile searches both `StaticCardLayer` and `CarryLayer` before creating a widget.
- Collapsing or expanding a pile does not recreate its card widgets.
- Battle projection and Special owner views may coexist with a physical view of the same InstanceId because their full keys differ.

## Interaction Contract

- Collapsed physical cards are noninteractive but visually normal; clicking their area routes to pile expansion.
- Expanded physical/content cards restore existing click, marquee, carry and cross-zone behavior.
- Battle projected cards, Special owner views and Burden locked cards never enter selection or carry.
- Only the title drag handle starts whole-pile movement.
- Escape cancels carry/marquee/pile move first, then collapses the open pile, then allows screen close; B closes directly and clears all transient state.

## Layout Contract

- Card logical size is fixed `220×320`; card-face scale remains `0.75`.
- Collapsed exposure defaults to 16px and adapts only within 10–24px; collapsed rotation is zero.
- Expanded exposure adapts within 32–72px with mild fan rotation and no scrolling.
- Solver returns `Anchor`, `HeaderRect`, `FrameRect`, `ExpansionDirection` and all card layouts.
- Stored pile position is the title anchor. Expanded visual bounds may extend left or right but must remain inside Workspace.

## Carry Performance Contract

- Beginning carry reparents each carried widget once into `CarryLayer` and calculates local fan layout once.
- Pointer movement updates only the CarryLayer anchor. It must not call Scene refresh, full interaction presentation refresh or reapply non-carried base layouts.
- Carry fan layout is recalculated only for begin, wheel/current-index change, card-count change, style change or effective geometry change.
- Latest pointer input is visible at the CarryLayer anchor by the next Slate frame with <=1px error for 1, 7, 15 and 21 cards.
- Releasing/cancelling reparents remaining widgets safely and restores their reconciled static layouts.

## Rendering Contract

- Static cards redraw on content or visual-state invalidation only.
- At most one Backpack card view is in realtime rendering mode.
- Hover realtime is allowed only for an interactive expanded card when not carrying.
- Carry realtime belongs only to the current/front card and supersedes hover.
- Leaving realtime mode clears all transient material/transform state and requests a final static render.

## Lifecycle Contract

- CommonUI deactivation, B close, screen destruction and Run replacement clear mouse capture, carry layer membership, hover state, pending auto-expand and realtime rendering.
- Layer transition Retainer bypass/restore continues without baking screen opacity into card caches.
- Snapshot revision drift preserves valid carried widgets by key and removes invalid views without duplicates.

## Removed Contract

- `UWacomBackpackPilePreviewWidget`, Preview ViewData, `PilePreviewWidgetClass`, `PreviewHost` and `WBP_BackpackCardView` are not formal production dependencies after migration.
- `UWacomRetainedCardViewWidget` may remain as a generic reusable class but is not the active Backpack card surface.
