# Tasks: Enemy Visual Authoring Modes

**Input**: Design documents from `specs/001-enemy-visual-authoring-modes/`

**Prerequisites**: [plan.md](./plan.md), [spec.md](./spec.md),
[research.md](./research.md), [data-model.md](./data-model.md),
[contracts/enemy-visual-authoring-contract.md](./contracts/enemy-visual-authoring-contract.md),
[quickstart.md](./quickstart.md)

**Tests**: Compile validation and focused Battle scene enemy automation are
required by the feature spec and Wacom constitution.

**Organization**: Tasks are grouped by user story so each story can be
implemented and validated independently.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel after its prerequisites are complete
- **[Story]**: User story label, for example US1, US2, US3
- Each task names exact repository paths
- Production module tasks stay separate from `WacomTests`

## Phase 1: Setup and Scope Guard

**Purpose**: Confirm current rule truth and keep the slice inside WacomApp scene
authoring.

- [X] T001 Review scene enemy authoring facts in `AGENTS.md`, `Docs/Architecture.md`, `Docs/WacomBattleUI.md`, and `Docs/WacomWorldInteraction.md`
- [X] T002 Review current Host and PartActor implementation in `Source/WacomApp/Public/Actors/WacomBattleEnemyActor.h`, `Source/WacomApp/Private/Actors/WacomBattleEnemyActor.cpp`, `Source/WacomApp/Public/Actors/WacomBattleEnemyPartActor.h`, and `Source/WacomApp/Private/Actors/WacomBattleEnemyPartActor.cpp`
- [X] T003 [P] Review existing Battle scene enemy test helpers in `Source/WacomTests/Private/UI/BattleWidgetSpec.cpp` and `Source/WacomTests/Private/UI/BattleHUDTestHarness.cpp`
- [X] T004 [P] Confirm quickstart validation commands and manual scenarios in `specs/001-enemy-visual-authoring-modes/quickstart.md`

---

## Phase 2: Foundational Contracts

**Purpose**: Add the shared C++ vocabulary needed by all stories before behavior
is wired.

- [X] T005 Add Host visual mode/struct declarations and Host debug fields in `Source/WacomApp/Public/Actors/WacomBattleEnemyActor.h`
- [X] T006 Add Host visual root/component storage and public read helpers in `Source/WacomApp/Public/Actors/WacomBattleEnemyActor.h`
- [X] T007 Add PartActor hit-only authoring context fields and setter declarations in `Source/WacomApp/Public/Actors/WacomBattleEnemyPartActor.h`
- [X] T008 [P] Add focused Host visual test scaffolding helpers in `Source/WacomTests/Private/UI/BattleWidgetSpec.cpp`

**Checkpoint**: Host visual and PartActor hit-only concepts have explicit public
authoring/debug contracts, but no rule module is touched.

---

## Phase 3: User Story 1 - 普通小怪整体视觉制作 (Priority: P1) MVP

**Goal**: A normal enemy Host can display one full-body sprite/flipbook while
child PartActors provide independent identity, `HitBounds`, hover, click,
drag-target, prediction, and badge behavior without independent art.

**Independent Test**: `Wacom.UI.Battle.BattleSceneEnemyActor` covers Host visual
+ hit-only child parts; manual PIE confirms each part remains independently
targetable.

### Tests and Validation for User Story 1

- [X] T009 [P] [US1] Add automation coverage for Host visual component generation and no-collision behavior in `Source/WacomTests/Private/UI/BattleWidgetSpec.cpp`
- [X] T010 [P] [US1] Add automation coverage for Host visual plus three hit-only child PartActors in `Source/WacomTests/Private/UI/BattleWidgetSpec.cpp`
- [X] T011 [P] [US1] Add automation coverage that Host visual does not alter `HitBounds` target routing in `Source/WacomTests/Private/UI/BattleWidgetSpec.cpp`

### Implementation for User Story 1

- [X] T012 [US1] Implement Host visual component creation, destruction, and refresh helpers in `Source/WacomApp/Private/Actors/WacomBattleEnemyActor.cpp`
- [X] T013 [US1] Wire Host visual refresh into construction, Details changes, and authoring refresh in `Source/WacomApp/Private/Actors/WacomBattleEnemyActor.cpp`
- [X] T014 [US1] Inject Host visual availability into child PartActors during Host authoring refresh in `Source/WacomApp/Private/Actors/WacomBattleEnemyActor.cpp`
- [X] T015 [US1] Implement PartActor hit-only mode so legacy cube visuals are hidden when Host visual supplies normal-enemy art in `Source/WacomApp/Private/Actors/WacomBattleEnemyPartActor.cpp`
- [X] T016 [US1] Ensure generated Host visual components have no collision and do not affect target provider resolution in `Source/WacomApp/Private/Actors/WacomBattleEnemyActor.cpp`

**Checkpoint**: User Story 1 is independently functional and can be validated
without US2/US3 polish.

---

## Phase 4: User Story 2 - 精英 / Boss 独立部位视觉保留 (Priority: P2)

**Goal**: Existing per-PartActor `VisualLayers` remain the official elite/Boss
path, including sprite/flipbook generation, sorting, tint, visibility, missing
asset diagnostics, and per-part feedback target behavior.

**Independent Test**: `Wacom.UI.Battle.BattleSceneEnemyActor` proves a Boss
PartActor with VisualLayers still renders and receives feedback independently
from any Host whole-body visual.

### Tests and Validation for User Story 2

- [X] T017 [P] [US2] Add regression coverage that PartActor `VisualLayers` override hit-only classification in `Source/WacomTests/Private/UI/BattleWidgetSpec.cpp`
- [X] T018 [P] [US2] Add regression coverage that target cue, hover, and drag preview scale the PartActor `VisualLayersRoot`, not the Host visual root, in `Source/WacomTests/Private/UI/BattleWidgetSpec.cpp`
- [X] T019 [P] [US2] Add regression coverage for static sprite and flipbook Host visual coexisting with per-part VisualLayers in `Source/WacomTests/Private/UI/BattleWidgetSpec.cpp`

### Implementation for User Story 2

- [X] T020 [US2] Preserve existing PartActor VisualLayers generation and feedback priority while applying Host visual context in `Source/WacomApp/Private/Actors/WacomBattleEnemyPartActor.cpp`
- [X] T021 [US2] Ensure Host visual feedback is not used as the default target for per-part bridge cue scaling in `Source/WacomApp/Private/Actors/WacomBattleEnemyActor.cpp` and `Source/WacomApp/Private/Actors/WacomBattleEnemyPartActor.cpp`

**Checkpoint**: Boss-style enemies still work after the normal-enemy Host visual
path is added.

---

## Phase 5: User Story 3 - 编辑器诊断区分视觉模式 (Priority: P3)

**Goal**: Details `Authoring Status`, debug summary, and validation distinguish
Host visual, PartActor VisualLayers, hit-only parts, legacy prototype visuals,
and truly missing visual resources.

**Independent Test**: `Wacom.UI.Battle.BattleSceneEnemyActor` covers debug view
and validation states; manual Validate Map reports legal normal/Boss configs
and clear errors for broken identity/hit bounds.

### Tests and Validation for User Story 3

- [X] T022 [P] [US3] Add automation coverage for Host debug view fields and debug summary text in `Source/WacomTests/Private/UI/BattleWidgetSpec.cpp`
- [X] T023 [P] [US3] Add automation coverage for PartActor `VisualAuthoringMode=HitOnly` and Details authoring cache values in `Source/WacomTests/Private/UI/BattleWidgetSpec.cpp`
- [X] T024 [P] [US3] Add automation coverage for validation warnings when Host and all child parts have no visible resources in `Source/WacomTests/Private/UI/BattleWidgetSpec.cpp`
- [X] T025 [P] [US3] Add automation coverage that identity, duplicate `PartSlotId`, and invalid `HitBounds` remain invalid even when Host visual exists in `Source/WacomTests/Private/UI/BattleWidgetSpec.cpp`

### Implementation for User Story 3

- [X] T026 [US3] Populate Host visual facts in `FWacomBattleSceneEnemyDebugView` and Host Details authoring cache in `Source/WacomApp/Private/Actors/WacomBattleEnemyActor.cpp`
- [X] T027 [US3] Populate PartActor hit-only visual facts in `FWacomBattleSceneEnemyPartDebugView` and PartActor Details authoring cache in `Source/WacomApp/Private/Actors/WacomBattleEnemyPartActor.cpp`
- [X] T028 [US3] Update Host and PartActor data validation messages for Host visual, hit-only, VisualLayers, legacy prototype, and no-art-anywhere cases in `Source/WacomApp/Private/Actors/WacomBattleEnemyActor.cpp` and `Source/WacomApp/Private/Actors/WacomBattleEnemyPartActor.cpp`

**Checkpoint**: Makers can identify the active visual mode from Details/debug
without entering PIE.

---

## Phase 6: Docs, Validation, and Handoff

**Purpose**: Move durable facts out of the feature spec and verify the delivered
slice.

- [X] T029 [P] Update scene enemy visual authoring semantics in `Docs/WacomBattleUI.md`
- [X] T030 [P] Update Host visual, hit-only PartActor, VisualLayers, targeting, and validation semantics in `Docs/WacomWorldInteraction.md`
- [X] T031 [P] Update `Docs/TODO.md` only if implementation leaves explicit technical debt or a required future follow-up
- [X] T032 Run compile validation with `E:\UE_5.7\Engine\Build\BatchFiles\Build.bat` for `D:\UE_Project\5.7\Wacom\Wacom.uproject`
- [X] T033 Run focused automation `Wacom.UI.Battle.BattleSceneEnemyActor` with `E:\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe`
- [X] T034 Run focused automation `Wacom.UI.Battle.BattleSceneEnemyTargetRegistry` with `E:\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe`
- [X] T035 Run manual editor/PIE validation scenarios from `specs/001-enemy-visual-authoring-modes/quickstart.md`

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1 Setup and Scope Guard**: no dependencies.
- **Phase 2 Foundational Contracts**: depends on Phase 1 and blocks all user
  stories.
- **US1 Normal Enemy MVP**: depends on Phase 2 and is the recommended first
  implementation slice.
- **US2 Boss VisualLayers Retention**: depends on US1 only where Host visual
  context interacts with PartActor visuals; otherwise tests can be prepared in
  parallel.
- **US3 Diagnostics**: depends on the visual modes from US1/US2.
- **Phase 6 Docs/Validation**: depends on selected stories.

### Within Each User Story

- Write or update focused tests before implementation tasks for the story.
- Update public authoring/debug declarations before `.cpp` behavior.
- Keep `WacomBattle`, `WacomData`, `WacomRun`, `EncounterDefinition`, and
  `BattleTrigger` unchanged unless a task is explicitly revised.
- Complete and validate the current story before moving to the next lower
  priority story.

### Parallel Opportunities

- T003 and T004 can run in parallel after T001/T002.
- T008 can run in parallel with T005-T007 once file ownership is clear.
- T009-T011 can be prepared in parallel because they target separate behavioral
  assertions in the same test prefix.
- T017-T019 can be prepared in parallel after US1 contracts exist.
- T022-T025 can be prepared in parallel after debug field names are finalized.
- T029-T031 can run in parallel after implementation behavior is stable.

---

## Implementation Strategy

### MVP First

1. Complete Phase 1 and Phase 2.
2. Implement US1 only.
3. Compile and run `Wacom.UI.Battle.BattleSceneEnemyActor`.
4. Manually verify one normal Host visual with three hit-only parts in PIE.
5. Continue to US2 and US3 after the normal enemy path is stable.

### Incremental Delivery

- **MVP**: US1 gives artists the normal-enemy production path.
- **Second slice**: US2 proves the new Host visual path does not regress
  elite/Boss per-part visuals.
- **Final slice**: US3 makes the editor authoring/debug experience clear enough
  to avoid false configuration warnings.

### Wacom Review Checklist

- Docs and `AGENTS.md` were respected.
- Module dependency direction is unchanged.
- Public API is limited to WacomApp authoring/debug contracts.
- UI and BattleHUD remain passive over Battle rules.
- `HitBounds` remains the only target hit source.
- No GameplayTag, DataAsset schema, Battle command, Run state, or SaveGame
  change was introduced.
- Compile, focused automation, and manual editor validation are complete or any
  skipped validation risk is stated.
