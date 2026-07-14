# Tasks: Backpack Workspace Refactor

**Input**: Design documents from `specs/003-backpack-workspace-refactor/`

**Prerequisites**: [spec.md](./spec.md), [plan.md](./plan.md), [research.md](./research.md), [data-model.md](./data-model.md), [contracts/run-batch-storage-contract.md](./contracts/run-batch-storage-contract.md), [contracts/backpack-workspace-input-contract.md](./contracts/backpack-workspace-input-contract.md), [contracts/backpack-wbp-binding-contract.md](./contracts/backpack-wbp-binding-contract.md), [quickstart.md](./quickstart.md)

**Tests**: Required. Run rules require atomicity tests; App input and CommonUI lifecycle require focused automation; motion, WBP and material feel require PIE validation.

## Phase 1: Setup and Migration Audit

**Purpose**: Confirm live-file scope and protect the unrelated dirty Battle/card-presentation work before edits.

- [x] T001 Re-run the Backpack symbol and call-site audit from `specs/003-backpack-workspace-refactor/quickstart.md` and append the exact current migration inventory to `specs/003-backpack-workspace-refactor/research.md`.
- [x] T002 Record the active implementation slice and the protected unrelated dirty-worktree boundary in `Docs/TODO.md` before changing `Source/WacomRun/` or `Source/WacomApp/`.
- [x] T003 Identify every current producer/consumer of `UWacomCardDragOperation`, `UWacomZoneDropTarget`, `UWacomDeleteZoneDropTarget`, `URunSession::MoveInstance`, and `URunSession::DeleteCardForGoldByInstance` and record the keep/migrate/remove decision in `specs/003-backpack-workspace-refactor/research.md`.
- [x] T004 Verify the actual UE 5.8 compile and focused automation commands on this machine and correct any stale engine path in `specs/003-backpack-workspace-refactor/quickstart.md` before the first code slice.

---

## Phase 2: Foundational Contracts and Test Seams

**Purpose**: Introduce stable shared types and test access without implementing user-facing behavior yet.

**CRITICAL**: Complete this phase before any Workspace or batch transaction story.

- [x] T005 Define non-Blueprint atomic move/delete request, validation, preview, and result structs with a required strict `ExpectedStorageRevision` field in `Source/WacomRun/Public/Deck/RunDeckBatchTypes.h`.
- [x] T006 Declare C++-only batch validation/commit entry points and document one-notification semantics in `Source/WacomRun/Public/RunSession.h`.
- [x] T007 Declare working-state batch rule helpers, shared request normalization, and duplicate/source validation in `Source/WacomRun/Private/Deck/RunDeckRules.h`.
- [x] T008 [P] Define private `FWacomBackpackZoneKey`, layout entry, selection, carry, and pending-confirmation state types in `Source/WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceTypes.h`.
- [x] T009 [P] Add designer-facing card/workspace/fan/motion/material tuning fields with Chinese tooltips in `Source/WacomApp/Public/UI/Backpack/WacomBackpackWorkspaceStyle.h` and `Source/WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceStyle.cpp`.
- [x] T010 [P] Define the production non-reflection read-only `FWacomBackpackWorkspaceAutomationTestView` in `Source/WacomApp/Public/UI/Backpack/WacomBackpackWorkspaceWidget.h`, then add a wrapper that only consumes that view in `Source/WacomTests/Private/UI/Backpack/BackpackWorkspaceTestAccess.h` and `Source/WacomTests/Private/UI/Backpack/BackpackWorkspaceTestAccess.cpp`; do not add Blueprint or scattered `ForTest` APIs.
- [x] T011 Compile `WacomEditor` with the command in `specs/003-backpack-workspace-refactor/quickstart.md` and resolve foundational public/private include or UHT errors before Phase 3.

**Checkpoint**: Public Run contract names, private App state types, and test seam compile without changing existing backpack behavior.

---

## Phase 3: User Story 1 — Single Active Region and Free Workspace (Priority: P1)

**Goal**: Replace the simultaneous multi-zone layout with a persistent right rack and one finite central workspace whose current-Run layout survives zone switch and screen reactivation.

**Independent Test**: With 4+ regions and 20+ cards, only one central region is active; free positions/angles/ZOrder restore within the same Run, reset for a new Run, remain at least 30% visible, and Arrange All restores default layout.

### Tests for User Story 1

- [x] T012 [P] [US1] Add failing zone-key normalization, default-layout, 30%-visible clamp, stable ZOrder, and Arrange All tests in `Source/WacomTests/Private/UI/Backpack/BackpackWorkspaceLayoutSpec.cpp`.
- [x] T013 [P] [US1] Add failing same-Run restore, screen reactivation, snapshot reconcile, removed-card cleanup, and new-Run reset tests in `Source/WacomTests/Private/UI/Backpack/BackpackWorkspaceLifecycleSpec.cpp`.
- [x] T014 [P] [US1] Add failing zone-rack active-entry/count/capacity and single-active-workspace presenter tests in `Source/WacomTests/Private/UI/Backpack/BackpackZoneRackSpec.cpp`.

### Implementation for User Story 1

- [x] T015 [US1] Implement deterministic default placement, normalized/manual transform resolution, layer compaction, fan-transform retention, and visible-bounds clamp in `Source/WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceLayoutSolver.h` and `Source/WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceLayoutSolver.cpp`.
- [x] T016 [US1] Implement the current-Run transient layout owner, Run identity reset, per-zone layout maps, and Snapshot cleanup in `Source/WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceStateSubsystem.h` and `Source/WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceStateSubsystem.cpp`.
- [x] T017 [P] [US1] Add passive Canvas/card-host/marquee/empty-state bindings and layout application API in `Source/WacomApp/Public/UI/Backpack/WacomBackpackWorkspaceWidget.h` and `Source/WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceWidget.cpp`.
- [x] T018 [P] [US1] Add passive zone entry title/count/capacity/active/preview presentation and activation intent in `Source/WacomApp/Public/UI/Backpack/WacomBackpackZoneRackEntryWidget.h` and `Source/WacomApp/Private/UI/Backpack/WacomBackpackZoneRackEntryWidget.cpp`.
- [x] T019 [US1] Add ZoneKey-based rack entry reconciliation and stable entry reuse in `Source/WacomApp/Public/UI/Backpack/WacomBackpackZoneRackWidget.h` and `Source/WacomApp/Private/UI/Backpack/WacomBackpackZoneRackWidget.cpp`.
- [x] T020 [US1] Add InstanceId-based active-workspace card reconciliation while preserving existing `UWacomCardView`, badges, detail source, and read-only projections in `Source/WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceReconciler.h` and `Source/WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceReconciler.cpp`.
- [x] T021 [US1] Refactor `UWacomBackpackScreen` bindings/state coordination to build one Workspace plus persistent ZoneRack from `FRunBackpackStorageSnapshot` in `Source/WacomApp/Public/UI/Backpack/WacomBackpackScreen.h` and `Source/WacomApp/Private/UI/Backpack/WacomBackpackScreen.cpp`.
- [x] T022 [US1] Replace the simultaneous-zone C++ fallback tree with a fallback implementing the same Workspace/ZoneRack hosts in `Source/WacomApp/Private/UI/Backpack/BackpackFallbackLayoutBuilder.h` and `Source/WacomApp/Private/UI/Backpack/BackpackFallbackLayoutBuilder.cpp`.
- [x] T023 [US1] Route Arrange All and same-zone collect to layout reset without calling Run move APIs in `Source/WacomApp/Private/UI/Backpack/WacomBackpackCommandFlow.h` and `Source/WacomApp/Private/UI/Backpack/WacomBackpackCommandFlow.cpp`.
- [x] T024 [US1] Update existing backpack test construction to use the new required hosts without adding new cases to the giant file in `Source/WacomTests/Private/UI/BackpackScreenTestAccess.h` and `Source/WacomTests/Private/UI/BackpackScreenTestAccess.cpp`.
- [x] T025 [US1] Run `Wacom.UI.Backpack` automation and the WacomEditor compile command from `specs/003-backpack-workspace-refactor/quickstart.md`; fix all US1 failures before Phase 4.

**Checkpoint**: US1 works without multi-select or batch movement; rack/navigation/layout persistence are independently usable and validated.

---

## Phase 4: User Story 2 — Marquee Selection and Persistent Fan Carry (Priority: P1)

**Goal**: Add desktop-like selection plus Demo-inspired persistent fan carry with initial-release guard, default-rightmost no-lift semantics, wheel current selection, one-card left release and all-card right release.

**Independent Test**: Select and carry at least 15 cards; 50 pickup releases place zero cards, no index UI appears, wheel clamps, default current does not lift, subsequent left release places one and right release places all.

### Tests for User Story 2

- [x] T026 [P] [US2] Add failing click/Ctrl-click/blank-click/center-marquee/Ctrl-marquee/Ctrl+A/read-only-exclusion tests in `Source/WacomTests/Private/UI/Backpack/BackpackWorkspaceSelectionSpec.cpp`.
- [x] T027 [P] [US2] Add failing initial-release guard, default-rightmost/current/ZOrder/no-lift, wheel clamp/no-wrap, left-one/right-all, fan reflow, and no-index-view tests in `Source/WacomTests/Private/UI/Backpack/BackpackWorkspaceInteractionSpec.cpp`, including 50 deterministic pickup/initial-release repetitions with zero misplaced cards.
- [x] T028 [P] [US2] Add failing mouse-capture, zone-switch cancel, Deactivate, invalidating-refresh, and focus-restoration tests in `Source/WacomTests/Private/UI/Backpack/BackpackWorkspaceLifecycleSpec.cpp`.

### Implementation for User Story 2

- [x] T029 [US2] Implement the pure selection/marquee/carry state machine and all transitions from `backpack-workspace-input-contract.md` in `Source/WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceInteractionModel.h` and `Source/WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceInteractionModel.cpp`.
- [x] T030 [US2] Extend the layout solver with style-driven carried fan transforms, stable rightmost highest-Z ordering, non-default current lift, and active-animation-only interpolation in `Source/WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceLayoutSolver.h` and `Source/WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceLayoutSolver.cpp`.
- [x] T031 [US2] Centralize pointer down/move/up, wheel, key, marquee drawing, and mouse-capture release in `Source/WacomApp/Public/UI/Backpack/WacomBackpackWorkspaceWidget.h` and `Source/WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceWidget.cpp`.
- [x] T032 [US2] Refactor card widgets into passive workspace items that emit pointer/hover intent and display selected/current/read-only state without starting `UDragDropOperation` in `Source/WacomApp/Public/UI/Backpack/WacomDeckCardWidget.h` and `Source/WacomApp/Private/UI/Backpack/WacomDeckCardWidget.cpp`.
- [x] T033 [US2] Add Screen-level interaction coordination, target hit resolution, detail suppression during carry, and zero-index-label presentation in `Source/WacomApp/Public/UI/Backpack/WacomBackpackScreen.h` and `Source/WacomApp/Private/UI/Backpack/WacomBackpackScreen.cpp`.
- [x] T034 [US2] Implement selected/current/valid-target/rejected-target visual-state mapping from style data in `Source/WacomApp/Private/UI/Backpack/WacomBackpackScreenPresenter.cpp` and `Source/WacomApp/Public/UI/Backpack/WacomBackpackScreenPresenter.h`.
- [x] T035 [US2] Unify Activate/Deactivate/Back/zone-switch/Snapshot-invalidated cancellation and provider unsubscription in `Source/WacomApp/Private/UI/Backpack/WacomBackpackScreen.cpp`.
- [x] T036 [US2] Migrate existing hover/detail and SpecialZone right-click toggle tests to coexist with the Workspace input owner in `Source/WacomTests/Private/UI/BackpackScreenSpec.cpp` without adding selection/carry cases to that file.
- [x] T037 [US2] Run `Wacom.UI.Backpack` automation, compile WacomEditor, and complete the selection/carry section of `specs/003-backpack-workspace-refactor/quickstart.md` in PIE before Phase 5.

**Checkpoint**: US1+US2 form the interaction MVP: players can organize, select, carry and place cards inside one active workspace.

---

## Phase 5: User Story 3 — Atomic Cross-Zone Move and Same-Zone Collect (Priority: P2)

**Goal**: Connect one-card/all-card rack release to an all-or-nothing Run transaction, while same-zone release remains a presentation-only collect action.

**Independent Test**: Valid requests move all requested cards with one revision/broadcast; capacity, duplicate, stale source/revision or invalid SpecialZone Owner moves zero cards and leaves carry active.

### Tests for User Story 3

- [x] T038 [P] [US3] Add failing atomic success, capacity-last-item failure, duplicate/invalid/stale-source/strict-stale-revision, SpecialZone Owner disappearance, burden, and one-broadcast tests in `Source/WacomTests/Private/Run/BackpackBatchMoveSpec.cpp`, repeating rejected move transactions 50 times with zero partial success.
- [x] T039 [P] [US3] Add failing one-card versus all-card rack release, rejected-preview carry retention, two-zone reconcile, and same-zone no-revision collect tests in `Source/WacomTests/Private/UI/Backpack/BackpackWorkspaceMoveSpec.cpp`.

### Implementation for User Story 3

- [x] T040 [US3] Implement normalized-request validation and working-copy atomic move application using existing single-card rules in `Source/WacomRun/Private/Deck/RunDeckRules.cpp`.
- [x] T041 [US3] Implement `URunSession` batch move preview/commit with authoritative revalidation, one state replacement, one revision advancement, and one broadcast in `Source/WacomRun/Private/RunSession.cpp`.
- [x] T042 [US3] Map carry InstanceIds/source revision/target ZoneKey to batch requests and unified failure Toasts in `Source/WacomApp/Private/UI/Backpack/WacomBackpackCommandFlow.h`, `Source/WacomApp/Private/UI/Backpack/WacomBackpackCommandFlow.cpp`, and `Source/WacomApp/Private/UI/Backpack/WacomBackpackToastText.cpp`.
- [x] T043 [US3] Connect zone-rack hover preview and left-current/right-all release intent to Screen command flow in `Source/WacomApp/Private/UI/Backpack/WacomBackpackZoneRackEntryWidget.cpp` and `Source/WacomApp/Private/UI/Backpack/WacomBackpackScreen.cpp`.
- [x] T044 [US3] Reconcile source/target workspaces after one Run notification and restore carry unchanged on failed commit in `Source/WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceReconciler.cpp` and `Source/WacomApp/Private/UI/Backpack/WacomBackpackScreen.cpp`.
- [x] T045 [US3] Preserve one-card API behavior and add one-item parity assertions to the focused `Source/WacomTests/Private/Run/BackpackBatchMoveSpec.cpp`; do not further enlarge legacy `BackpackSpec.cpp`.
- [x] T046 [US3] Run `Wacom.Run.Backpack`, `Wacom.UI.Backpack`, and WacomEditor compile from `specs/003-backpack-workspace-refactor/quickstart.md`; fix all atomicity and refresh-count failures before Phase 6.

**Checkpoint**: Cross-zone batch movement is authoritative and atomic; same-zone collect creates no Run mutation.

---

## Phase 6: User Story 4 — Atomic Batch Delete with Carry Restoration (Priority: P2)

**Goal**: Delete a carried collection through one preview/confirmation and one atomic Run commit; cancel or stale failure returns to the exact carry state.

**Independent Test**: One confirmation shows count/total gold, success deletes all and grants once, while cancel or any invalid/stale item deletes none and restores remaining order/current card.

### Tests for User Story 4

- [x] T047 [P] [US4] Add failing batch delete reward, invalid-last-item rollback, capacity-provider/burden, stale source/strict-stale-revision, and one-broadcast tests in `Source/WacomTests/Private/Run/BackpackBatchDeleteSpec.cpp`, repeating rejected delete transactions 50 times so T038+T047 cover 100 atomic failure runs with zero partial success.
- [x] T048 [P] [US4] Add failing preview count/total, modal focus, cancel restore, stale-confirm failure restore, and successful carry exit tests in `Source/WacomTests/Private/UI/Backpack/BackpackWorkspaceDeleteSpec.cpp`.

### Implementation for User Story 4

- [x] T049 [US4] Implement working-copy atomic permanent removal and reward accumulation in `Source/WacomRun/Private/Deck/RunDeckRules.cpp`.
- [x] T050 [US4] Implement `URunSession` delete preview/commit with revalidation, one gold grant, one state replacement, one revision advancement, and one broadcast in `Source/WacomRun/Private/RunSession.cpp`.
- [x] T051 [P] [US4] Add passive batch count/reward/confirm/cancel bindings and intents in `Source/WacomApp/Public/UI/Backpack/WacomBackpackDeleteConfirmWidget.h` and `Source/WacomApp/Private/UI/Backpack/WacomBackpackDeleteConfirmWidget.cpp`.
- [x] T052 [US4] Add pending-confirm carry snapshot, focus handoff, confirm-time revalidation, cancel/failure restoration, and success cleanup to `Source/WacomApp/Private/UI/Backpack/WacomBackpackCommandFlow.cpp` and `Source/WacomApp/Private/UI/Backpack/WacomBackpackScreen.cpp`.
- [x] T053 [US4] Map atomic delete disabled reasons and total reward success feedback in `Source/WacomApp/Private/UI/Backpack/WacomBackpackToastText.cpp` and `Source/WacomApp/Private/UI/Backpack/WacomBackpackScreenPresenter.cpp`.
- [x] T054 [US4] Run `Wacom.Run.Backpack`, `Wacom.UI.Backpack`, and WacomEditor compile from `specs/003-backpack-workspace-refactor/quickstart.md`; fix all rollback, reward, modal-focus, and carry-restoration failures.

**Checkpoint**: All four user stories are functional and contract-tested before asset polish and old-path deletion.

---

## Phase 7: WBP, DreamShader, Migration Cleanup, and Documentation

**Purpose**: Finish the formal visual assets, remove the obsolete input owner, update long-term rule truth, and run final validation.

- [x] T055 Refactor `Content/Wacom/UI/Backpack/WBP_BackpackScreen.uasset` to bind `WorkspaceHost`, `ZoneRackHost`, `DeleteTargetHost`, `CardDetailLayer`, `ArrangeAllButton`, and `DeleteConfirmHost` from `contracts/backpack-wbp-binding-contract.md`.
- [x] T056 [P] Create `Content/Wacom/UI/Backpack/WBP_BackpackWorkspace.uasset` and `Content/Wacom/UI/Backpack/WBP_BackpackZoneRack.uasset` with passive Canvas/rack bindings and no Run calls.
- [x] T057 [P] Create `Content/Wacom/UI/Backpack/WBP_BackpackZoneRackEntry.uasset` and `Content/Wacom/UI/Backpack/WBP_BackpackDeleteConfirm.uasset` with active/preview/rejected and modal-focus presentation.
- [x] T058 [P] Create and register `Content/Wacom/UI/Backpack/DA_BackpackWorkspaceStyle.uasset` with card size, fan angle/overlap, current lift, follow/settle timing, visible fraction, selection, target and material parameters.
- [x] T059 Author the Wacom-native Graph-first workspace card feedback shader in `DShader/Material/Card/M_BackpackWorkspaceCardFeedback.dsm` with reusable helper code in `DShader/Shared/WacomBackpackWorkspaceCardFeedback.dsh`.
- [x] T060 Build and configure `Content/Wacom/UI/Backpack/Materials/M_BackpackWorkspaceCardFeedback.uasset` from the DreamShader source and bind it through `Content/Wacom/UI/Backpack/DA_BackpackWorkspaceStyle.uasset` without changing hit geometry.
- [x] T061 Remove the obsolete persistent input-owner implementation from `Source/WacomApp/Public/UI/Backpack/WacomCardDragOperation.h`, `Source/WacomApp/Private/UI/Backpack/WacomCardDragOperation.cpp`, `Source/WacomApp/Public/UI/Backpack/WacomZoneDropTarget.h`, `Source/WacomApp/Private/UI/Backpack/WacomZoneDropTarget.cpp`, `Source/WacomApp/Public/UI/Backpack/WacomDeleteZoneDropTarget.h`, and `Source/WacomApp/Private/UI/Backpack/WacomDeleteZoneDropTarget.cpp`, updating build references and tests to leave one Screen/Workspace path.
- [x] T062 Split or migrate remaining old drag/drop assertions out of `Source/WacomTests/Private/UI/BackpackScreenSpec.cpp` into the focused files under `Source/WacomTests/Private/UI/Backpack/`, then remove only obsolete drag/drop methods from `Source/WacomTests/Private/UI/BackpackScreenTestAccess.h` and `Source/WacomTests/Private/UI/BackpackScreenTestAccess.cpp` while preserving unrelated Screen test access.
- [x] T063 [P] Document atomic move/delete rules, working-state rollback, revision/broadcast semantics, and no SaveGame layout impact in `Docs/WacomRun.md`.
- [x] T064 [P] Document Snapshot-to-Workspace intent flow, Run-scoped transient layout owner, input lifecycle, controller scope, and Wacom-native material boundary in `Docs/WacomUI.md` and `Docs/WacomApp.md`.
- [x] T065 Update the formal Workspace/ZoneRack/card/confirmation WBP contract and demote old simultaneous-zone Host instructions to an explicit migration-only section in `Docs/UI_Backpack_WBP_Binding.md`.
- [x] T066 Resolve the active feature entry and record only genuinely deferred WBP/material/controller work or unavoidable migration debt in `Docs/TODO.md` and `Docs/TechDebt.md`.
- [x] T067 Run the migration `rg` command from `specs/003-backpack-workspace-refactor/quickstart.md` and remove or explicitly justify every remaining old drag/drop production match in `specs/003-backpack-workspace-refactor/research.md`.
- [x] T068 Compile WacomEditor with the command in `specs/003-backpack-workspace-refactor/quickstart.md` after all source/assets/contracts are migrated.
- [x] T069 Run `Wacom.Run.Backpack` and `Wacom.UI.Backpack` automation with the commands in `specs/003-backpack-workspace-refactor/quickstart.md` and record results in `Docs/TODO.md` only if failures remain.
- [ ] T070 Complete every PIE checklist section in `specs/003-backpack-workspace-refactor/quickstart.md`, including a recorded under-10-second active-workspace identification check with 20+ cards/4+ zones, no index UI, default no-lift, atomic rejection, modal restore, same-Run layout and DreamShader-disabled fallback.
- [x] T071 Run the full `Automation RunTests Wacom` command from `specs/003-backpack-workspace-refactor/quickstart.md` and document any unrelated failure with log evidence in `Docs/TODO.md`.
- [x] T072 Review `Docs/WacomRun.md`, `Docs/WacomUI.md`, `Docs/WacomApp.md`, `Docs/UI_Backpack_WBP_Binding.md`, `Docs/TODO.md`, and `Docs/TechDebt.md` against implemented facts so no long-term rule remains only in `specs/003-backpack-workspace-refactor/`.
- [x] T073 Stabilize selected-card pickup so pointer-down enters carry immediately, pickup pointer-up only consumes the initial guard, and the next left/right click releases one/all without requiring an extra pointer move.
- [x] T074 Add `WBP_BackpackCardView` with an invalidation-only static Retainer around the authored `WBP_FirstPersonCardView`, migrate `WBP_WacomDeckCardWidget.BackpackCardView`, and cover the asset/render contract without importing first-person effects or input.
- [x] T075 Disable, collapse, and release the inner `SurfaceFoilOverlay` material brush for the backpack static-retained wrapper while preserving the shared battle/first-person card face default, with focused runtime and formal-asset contract coverage.
- [x] T076 Connect `M_BackpackWorkspaceCardFeedback.dsm` directly to native VertexColor RGB output 0 and Alpha output 4, force-regenerate the material asset, and add a UE material-recompile/graph regression test that rejects A masks applied to RGB output 0.

---

## Dependencies

```text
Phase 1 audit
  -> Phase 2 contracts/test seams
      -> US1 single workspace/layout
          -> US2 selection/carry MVP
              -> US3 atomic cross-zone move
              -> US4 atomic batch delete
                  -> WBP/DreamShader/old-path cleanup/docs/final validation
```

- Phase 2 blocks every user story.
- US1 blocks US2 because selection/carry requires stable ZoneKey, Workspace geometry and InstanceId reconciliation.
- US2 blocks US3/US4 because rack/delete releases consume the carry model.
- US3 and US4 share foundational batch contracts but may be implemented independently after US2; both must finish before old drag/drop removal.
- Asset polish can begin after the matching public Widget/style classes compile, but final binding and old-path removal wait for all behavior stories.

## Parallel Opportunities

- T008, T009, and T010 touch independent App/test files after Run public names are decided.
- T012, T013, and T014 can be written in parallel before US1 implementation.
- T017 and T018 can proceed in parallel after T008/T009.
- T026, T027, and T028 can be written in parallel against the input contract.
- T038 and T039 can be written in parallel; T040/T041 then implement Run behavior before T042–T044 App wiring.
- T047 and T048 can be written in parallel; T049/T050 then implement Run behavior while T051 builds the passive modal shell.
- T056–T058 and T063–T064 can proceed in parallel when their production contracts are stable; one owner must perform final WBP and Docs review.

## Implementation Strategy

### MVP

MVP is US1 + US2, not US1 alone:

1. Right-side rack and one current-Run free workspace.
2. Selection, marquee and persistent fan carrying inside that workspace.

This delivers the new backpack structure and signature interaction without yet changing authoritative zone ownership.

### Incremental Delivery

1. Compile foundation with no behavior change.
2. Deliver and validate free organization.
3. Deliver and validate selection/carry hand feel.
4. Add atomic cross-zone movement.
5. Add atomic delete confirmation.
6. Finish formal WBP/materials, delete the old input owner, update Docs, and run final validation.

## Format Validation

- All tasks use `- [ ] T###` checklist format.
- User-story tasks include `[US1]`–`[US4]` labels.
- `[P]` appears only where files and incomplete dependencies do not overlap.
- Every task names an exact repository file or artifact path.
