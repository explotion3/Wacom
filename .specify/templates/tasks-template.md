---
description: "Wacom task list template for feature implementation"
---

# Tasks: [FEATURE NAME]

**Input**: Design documents from `/specs/[###-feature-name]/`

**Prerequisites**: plan.md (required), spec.md (required for user stories),
research.md, data-model.md, contracts/

**Tests**: Compile validation and the smallest meaningful automation/manual test
set are REQUIRED by the Wacom constitution. Include Battle, Run, Data, UI,
SaveGame, editor validation, or manual quickstart tasks as appropriate.

**Organization**: Tasks are grouped by user story so each story can be
implemented and validated independently.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependency order conflict)
- **[Story]**: User story label, for example US1, US2, US3
- Include exact file paths in descriptions
- Keep production module tasks separate from WacomTests tasks

## Wacom Path Conventions

- **Core contracts/tags**: `Source/WacomCore/Public/`, `Source/WacomCore/Private/`
- **Static data contracts**: `Source/WacomData/Public/`, `Source/WacomData/Private/`
- **Battle rules**: `Source/WacomBattle/Public/`, `Source/WacomBattle/Private/`
- **Run rules**: `Source/WacomRun/Public/`, `Source/WacomRun/Private/`
- **App/UI/input/world**: `Source/WacomApp/Public/`, `Source/WacomApp/Private/`
- **Editor validation/tools**: `Source/WacomEditor/Public/Validation/`,
  `Source/WacomEditor/Private/`
- **Automation tests**: `Source/WacomTests/Public/Fixtures/`,
  `Source/WacomTests/Private/`
- **Docs**: `Docs/Architecture.md`, `Docs/Wacom*.md`, binding docs,
  `Docs/TODO.md`, `Docs/TechDebt.md`

<!--
  The tasks below are examples. $speckit-tasks MUST replace them with actual
  tasks from spec.md, plan.md, data-model.md, quickstart.md, and contracts/.
-->

## Phase 1: Setup and Docs Alignment

**Purpose**: Confirm Wacom rule truth, affected modules, and validation scope.

- [ ] T001 Read and cite `AGENTS.md`, `Docs/Architecture.md`, and affected
  domain docs in `specs/[###-feature-name]/plan.md`
- [ ] T002 Identify exact owning module(s), public/private boundary, and Build.cs
  dependency direction in `specs/[###-feature-name]/plan.md`
- [ ] T003 [P] Record required documentation updates in
  `specs/[###-feature-name]/quickstart.md` and affected `Docs/` files
- [ ] T004 [P] Record compile and focused automation/manual validation commands
  in `specs/[###-feature-name]/quickstart.md`

---

## Phase 2: Foundational Contracts

**Purpose**: Shared contracts that block user story implementation.

**CRITICAL**: No user story work begins until public contracts, data contracts,
or test fixtures required by multiple stories are defined.

- [ ] T005 Define or update public module contract in
  `Source/[OwningModule]/Public/[Path]/[Contract].h`
- [ ] T006 Keep rule implementation helpers in
  `Source/[OwningModule]/Private/[Path]/[Helper].{h,cpp}`
- [ ] T007 [P] Add or update GameplayTags in
  `Source/WacomCore/Public/Tags/WacomGameplayTags.h` if required
- [ ] T008 [P] Add or update DataAsset contract in
  `Source/WacomData/Public/[Area]/[Definition].h` if required
- [ ] T009 [P] Add or update editor validation in
  `Source/WacomEditor/Public/Validation/` and `Source/WacomEditor/Private/`
  if required
- [ ] T010 [P] Add or update shared test fixtures/access wrappers in
  `Source/WacomTests/Public/Fixtures/` or `Source/WacomTests/Private/`
- [ ] T011 Update affected docs before implementation begins if rule truth
  changes materially

**Checkpoint**: Contracts and docs are aligned; user story implementation can
begin.

---

## Phase 3: User Story 1 - [Title] (Priority: P1) MVP

**Goal**: [Brief description of what this story delivers]

**Independent Test**: [Focused automation/manual flow proving this story works
without later stories]

### Tests and Validation for User Story 1

> Write or update tests before implementation when this story changes rules,
> SaveGame, data validation, public contracts, or a reproduced bug.

- [ ] T012 [P] [US1] Add/update focused automation test in
  `Source/WacomTests/Private/[Domain]/[SpecName].cpp`
- [ ] T013 [P] [US1] Add/update manual/editor validation steps in
  `specs/[###-feature-name]/quickstart.md`

### Implementation for User Story 1

- [ ] T014 [P] [US1] Implement domain rule or transaction in
  `Source/[OwningModule]/Private/[Path]/[File].cpp`
- [ ] T015 [US1] Expose only required stable contract in
  `Source/[OwningModule]/Public/[Path]/[File].h`
- [ ] T016 [US1] Wire UI/App command or event flow in
  `Source/WacomApp/Private/[Path]/[File].cpp` if UI is involved
- [ ] T017 [US1] Add DataAsset/content/validator changes in
  `Source/WacomData/`, `Source/WacomEditor/`, and docs if involved
- [ ] T018 [US1] Update affected Wacom docs and remove or record temporary debt

**Checkpoint**: User Story 1 is independently functional and validated.

---

## Phase 4: User Story 2 - [Title] (Priority: P2)

**Goal**: [Brief description of what this story delivers]

**Independent Test**: [How to verify this story works on its own]

### Tests and Validation for User Story 2

- [ ] T019 [P] [US2] Add/update focused automation test in
  `Source/WacomTests/Private/[Domain]/[SpecName].cpp`
- [ ] T020 [P] [US2] Add/update quickstart/manual validation if automation is
  not sufficient

### Implementation for User Story 2

- [ ] T021 [P] [US2] Implement reusable system extension in
  `Source/[OwningModule]/Private/[Path]/[File].cpp`
- [ ] T022 [US2] Integrate through documented contract from User Story 1 if
  needed
- [ ] T023 [US2] Update affected docs and validation notes

**Checkpoint**: User Story 1 and 2 both work independently.

---

## Phase 5: User Story 3 - [Title] (Priority: P3)

**Goal**: [Brief description of what this story delivers]

**Independent Test**: [How to verify this story works on its own]

### Tests and Validation for User Story 3

- [ ] T024 [P] [US3] Add/update focused automation or manual validation
- [ ] T025 [P] [US3] Add regression coverage for edge cases introduced by this
  story

### Implementation for User Story 3

- [ ] T026 [P] [US3] Implement story-specific reusable extension in
  `Source/[OwningModule]/Private/[Path]/[File].cpp`
- [ ] T027 [US3] Wire presentation/input only through WacomApp if needed
- [ ] T028 [US3] Update docs and quickstart validation

**Checkpoint**: All selected user stories are independently functional.

---

[Add more user story phases as needed.]

---

## Phase N: Polish, Docs, and Validation

**Purpose**: Finish cross-cutting work and verify the delivered slice.

- [ ] TXXX Remove one-off Actor/Widget/Level Blueprint shortcuts or document
  approved `TODO(技术债)` in `Docs/TODO.md` or `Docs/TechDebt.md`
- [ ] TXXX [P] Update affected `Docs/` files and binding/authoring docs
- [ ] TXXX [P] Add additional automation coverage for high-risk edge cases
- [ ] TXXX Run compile validation:

```powershell
& 'E:\UE_5.8\Engine\Build\BatchFiles\Build.bat' WacomEditor Win64 Development -Project='D:\UE_Project\5.7\Wacom\Wacom.uproject' -WaitMutex -NoHotReloadFromIDE
```

- [ ] TXXX Run focused automation validation:

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\Wacom\Wacom.uproject' -ExecCmds='Automation RunTests [PREFIX]; Quit' -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache
```

- [ ] TXXX Run `specs/[###-feature-name]/quickstart.md` manual/editor
  validation steps if applicable

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup and Docs Alignment**: No dependencies
- **Foundational Contracts**: Depends on Setup; blocks all user stories
- **User Stories**: Depend on Foundational Contracts
- **Polish/Validation**: Depends on selected stories

### Within Each User Story

- Tests/validation tasks before implementation for rules, SaveGame, data
  validation, public contracts, and reproduced bugs
- Public contracts before cross-module integration
- Data/contracts before runtime resolver or transaction code
- Domain implementation before WacomApp presentation wiring
- UI lifecycle/focus/unsubscription before final UI validation
- Story complete before moving to lower-priority story

### Parallel Opportunities

- Documentation and test fixture tasks marked [P] can run in parallel
- Separate module files can run in parallel when no shared public contract is
  being edited
- Different user stories can be parallelized only after foundational contracts
  are stable

---

## Implementation Strategy

### MVP First

1. Complete Setup and Foundational Contracts.
2. Implement User Story 1.
3. Stop and validate User Story 1 independently.
4. Update docs and record any temporary debt.
5. Continue only after MVP validation is clear.

### Wacom Review Checklist

- [ ] Docs and `AGENTS.md` were respected.
- [ ] Module dependency direction is unchanged or explicitly justified.
- [ ] Public API is minimal; implementation remains private.
- [ ] UI remains passive and uses commands/events/ViewModel/ViewData.
- [ ] DataAsset/GameplayTag/validator impacts are documented.
- [ ] No one-off Actor/Widget shortcut remains without debt tracking.
- [ ] Compile and focused tests/manual validation are complete or risk is stated.

---

## Notes

- Avoid vague tasks, same-file conflicts, or cross-story dependencies that break
  independent validation.
- Avoid adding `ForTest` Blueprint/Public API to WacomApp; prefer WacomTests
  probes, production protected extension points, or private test access wrappers.
- Avoid string-built GameplayTags, temporary globals, and Tick polling for UI
  state.
