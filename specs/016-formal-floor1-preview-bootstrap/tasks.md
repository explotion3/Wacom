# Tasks: Floor 1 Preview 启动层

**Input**: Design documents from `specs/016-formal-floor1-preview-bootstrap/`

**Prerequisites**: Spec 015 complete source/content chain at current HEAD; current dirty Production map preserved; user-approved continuous branch exception; target Editor closed for builds; exact two-package writer authority before binary mutation.

**Tests**: Default Unity compilation, focused App/Editor automation, affected Run/Data/scene regressions, AssetRegistry/Blueprint/hash/LFS audit, and user PIE are mandatory.

## Phase 1: Workspace and Spec alignment

**Purpose**: Protect the dirty binary and make the continuous Spec 015→016 dependency explicit before source edits.

- [x] T001 Audit branch, HEAD, merge-base, Git status, Git LFS status, target map hash, and relevant Unreal/Live Coding processes in `specs/016-formal-floor1-preview-bootstrap/quickstart.md`
- [x] T002 Record the user-approved decision to stay on `codex/formal-floor1-production-scene-baseline` without merge/rebase or new worktree in `spec.md`, `plan.md`, and `quickstart.md`
- [x] T003 Read Spec 015 contracts/ledger plus live `AWacomGameMode`, `AWacomPlayerController`, Descriptor resolver, builder, validator, and tests before editing
- [x] T004 Create Spec 016 `spec.md`, `plan.md`, `research.md`, `data-model.md`, `contracts/`, `tasks.md`, `quickstart.md`, and `checklists/`
- [x] T005 Update `.specify/feature.json` and the managed `AGENTS.md` pointer to Spec 016
- [x] T006 Run non-destructive Spec Kit consistency analysis and resolve any CRITICAL/HIGH discrepancy before implementation

---

## Phase 2: Foundational runtime contract

**Purpose**: Establish the reusable resolver seam and Preview invariant before asset tooling.

**CRITICAL**: No binary mutation begins before this phase compiles and its focused tests pass.

- [x] T007 [P] Add Preview runtime failure-matrix automation in `Source/WacomTests/Private/App/RunFloorPreviewBootstrapSpec.cpp`
- [x] T008 Add the non-reflection virtual resolver declaration and base contract to `Source/WacomApp/Public/GameFramework/WacomGameMode.h`
- [x] T009 Implement the base resolver as an exact `DefaultJourneyDefinition` projection in `Source/WacomApp/Private/GameFramework/WacomGameMode.cpp`
- [x] T010 Route `AWacomPlayerController::BeginPlay()` through the resolver and log the actual resolved Journey in `Source/WacomApp/Private/GameFramework/WacomPlayerController.cpp`
- [x] T011 Add `AWacomRunFloorPreviewGameMode` public Blueprintable type and cached transient fields in `Source/WacomApp/Public/GameFramework/WacomRunFloorPreviewGameMode.h`
- [x] T012 Implement PIE gate, unique Descriptor resolution, transient Journey construction, same-instance caching, and drift rejection in `Source/WacomApp/Private/GameFramework/WacomRunFloorPreviewGameMode.cpp`
- [x] T013 Confirm source changes add no reflection setter, GameplayTag, SaveGame, Build.cs, module edge, Run schema, or DataAsset field
- [x] T014 Close the target Editor, pass `AssertClosedForBuild`, and compile default Unity `WacomEditor Win64 Development`
- [x] T015 Run `Wacom.App.RunFloorPreviewBootstrap` and `Wacom.UI.RunSceneBinding` with `-NoDreamShaderEditorBridge`; record logs/hashes in `quickstart.md`

**Checkpoint**: Base GameMode behavior is unchanged; valid PIE Preview produces one stable transient Journey; invalid worlds/descriptors fail closed.

---

## Phase 3: User Story 1 — Direct Preview runtime (Priority: P1)

**Goal**: Prove direct-map Run initialization can receive the exact Descriptor Floor without modifying Run rules.

**Independent Test**: `Wacom.App.RunFloorPreviewBootstrap` covers valid and invalid resolver paths using transient worlds and definitions.

- [x] T016 [US1] Cover base GameMode resolver compatibility and Preview non-PIE rejection in `RunFloorPreviewBootstrapSpec.cpp`
- [x] T017 [US1] Cover missing/duplicate/empty Descriptor and missing character failures in `RunFloorPreviewBootstrapSpec.cpp`
- [x] T018 [US1] Cover exact Floor pointer, identity/display, one character, defaults, empty terminal, and `RF_Transient` in `RunFloorPreviewBootstrapSpec.cpp`
- [x] T019 [US1] Cover repeated same-instance resolution and Descriptor/Floor drift rejection in `RunFloorPreviewBootstrapSpec.cpp`
- [x] T020 [US1] Re-run the App prefix after corrections and update runtime evidence in `quickstart.md`

**Checkpoint**: Runtime Preview selection is independently complete before Editor content work.

---

## Phase 4: Foundational Editor seed/inspect service

**Purpose**: Create a strict two-package mutation path that preserves Spec 015 scene authority.

- [x] T021 [P] Add non-reflection report/test view contract in `Source/WacomEditor/Public/Testing/WacomFormalFloor1PreviewBootstrapAutomationTestView.h`
- [x] T022 [P] Add focused Editor automation in `Source/WacomTests/Private/Editor/FormalFloor1PreviewBootstrapSpec.cpp`
- [x] T023 Define exact package constants, result types, and public private-module entry in `Source/WacomEditor/Private/ContentBuilders/FormalFloor1PreviewBootstrap.h`
- [x] T024 Implement source `GM_Wacom` CDO resolution and strict Preview Blueprint inspection in `Source/WacomEditor/Private/ContentBuilders/FormalFloor1PreviewBootstrap.cpp`
- [x] T025 Implement missing-only Preview Blueprint creation, six-field copy, forced null Journey, compile, and package save in `FormalFloor1PreviewBootstrap.cpp`
- [x] T026 Implement read-only Spec 015 scene preflight and exact Entry Anchor/PlayerStart audit in `FormalFloor1PreviewBootstrap.cpp`
- [x] T027 Implement map mutation limited to World Settings override and missing exact Preview PlayerStart in `FormalFloor1PreviewBootstrap.cpp`
- [x] T028 Implement map/Blueprint post-save reload inspection and internal second-pass zero-write enforcement in `FormalFloor1PreviewBootstrap.cpp`
- [x] T029 Implement the no-argument `WacomSeedFormalFloor1PreviewBootstrap` console command in `Source/WacomEditor/Private/ContentBuilders/FormalFloor1PreviewBootstrapEditorCommand.cpp`
- [x] T030 Implement automation view adapters without writable Blueprint/test API in `FormalFloor1PreviewBootstrap.cpp`
- [x] T031 Validate collision policy for wrong existing Blueprint, unexpected GameMode, PlayerStart, Entry Anchor, and scene contract in focused tests
- [x] T032 Confirm Editor service never calls Spec 015/014/Debug/full-content builders and never saves outside the exact manifest
- [x] T033 Close target Editor, compile default Unity, and run `Wacom.Editor.FormalFloor1PreviewBootstrap`
- [x] T034 Run `Wacom.Data.Map.Validation`, `Wacom.Run.Map.Traversal`, `Wacom.UI.RunPathTraversal`, and `Wacom.Editor.RunSceneValidation`; record evidence

**Checkpoint**: Source and synthetic tests pass; real assets remain untouched except the preserved pre-existing dirty map.

---

## Phase 5: User Story 2 — Allowlisted asset bootstrap (Priority: P1)

**Goal**: Create/configure the two Preview packages without changing any other scene/content asset.

**Independent Test**: one verified Editor command plus persisted-asset inspection and internal idempotence report.

- [x] T035 [US2] Record pre-mutation hashes for the dirty map, 46 Spec 014 assets, and Spec 015's six other binaries in `quickstart.md`
- [x] T036 [US2] Start the exact `run` Editor identity, call `AssertReady`, and record project/branch/HEAD/PID/session provenance
- [x] T037 [US2] Acquire one writer lease for `/Game/Wacom/Run/Preview/GM_WacomRunFloorPreview` and `/Game/Wacom/Maps/Run/L_Run_Floor_Main_01`
- [x] T038 [US2] Have the user execute `WacomSeedFormalFloor1PreviewBootstrap` once in the verified Editor
- [x] T039 [US2] Inspect Git status and actual saved packages before releasing the writer; fail closed on any out-of-scope path
- [x] T040 [US2] Capture writer audit JSON and report, release the lease, close Editor normally, and pass `AssertClosedForBuild`
- [x] T041 [US2] Verify Blueprint load/compile/parent/CDO and map override/unique plain Entry PlayerStart through focused automation/AssetRegistry
- [x] T042 [US2] Verify exact Spec 015 scene counts/bindings/transforms, Floor Definition, and all non-target hashes remain unchanged
- [x] T043 [US2] Verify internal second pass `0 created / 0 modified / 0 saved`, both targets use LFS, and `git lfs fsck` passes
- [x] T044 [US2] Re-run default Unity compile and all affected focused automation if any source correction occurred after the mutation

**Checkpoint**: only the Preview Blueprint and Production map are binary changes, and they pass persisted contract/idempotence validation.

---

## Phase 6: User Story 3 — Manual PIE acceptance (Priority: P1)

**Goal**: Prove the authored local Floor loop remains playable across input/UI/interaction return boundaries.

**Independent Test**: user completes the consolidated `contracts/preview-pie-contract.md` checklist.

- [x] T045 [US3] Provide the user one complete PIE checklist covering startup logs, Entry anchoring, mouse/W/S/HUD/hand, M/ESC clicks, path, battle return, one Event/Reward/Shop return, Exit dead end, and post-stop hashes
- [x] T046 [US3] Keep all changes uncommitted while the user performs PIE and record the explicit result in `quickstart.md`
- [x] T047 [US3] If any item fails, keep changes uncommitted, diagnose/fix, rerun affected automation, and repeat the full PIE checklist (not invoked; no PIE behavior failed)
- [x] T048 [US3] After explicit success, compare the two binary hashes with pre-PIE hashes and record the acceptance timestamp/result

**Checkpoint**: user explicitly confirms all PIE items; only then can commit work begin.

---

## Phase 7: Docs, final validation, and deferred commits

**Purpose**: Move long-term Preview facts into Docs and prepare a standard handoff without merging or pushing.

- [x] T049 Update `Docs/WacomApp.md` with the base resolver and PIE-only Preview GameMode contract
- [x] T050 Update `Docs/WacomMap.md` with direct-map Preview startup, Entry spawn, and Exit dead-end scope
- [x] T051 Update `Docs/WacomDataAuthoring.md` with the two-package seed/inspect workflow, exact allowlist, and idempotence gate
- [x] T052 Update `Docs/Architecture.md` with App resolver/Editor authoring ownership and unchanged module direction
- [x] T053 Update `Docs/TODO.md`, `Docs/Questions.md`, and `Docs/TechDebt.md` with completed Preview capability and Production Journey removal blocker
- [x] T054 Complete `quickstart.md` with build/test/MCP/writer/hash/AssetRegistry/Blueprint/PIE evidence and remaining risks
- [x] T055 Run Spec Kit consistency recheck, `git diff --check`, forbidden-scope audit, `git status`, `git lfs status`, and `git lfs fsck`
- [x] T056 After user PIE authorization only, stage source/tests/Spec/Docs and create `feat(app): add run floor preview bootstrap`
- [x] T057 Stage only the Preview Blueprint/map plus final binary evidence updates and create `feat(content): enable floor1 preview startup`
- [x] T058 Confirm worktree/LFS clean and produce the standard `Docs/AgentIntegrationWorkflow.md` handoff; do not merge main or push

## Dependencies & execution order

```text
Phase 1
  -> Phase 2 runtime seam/build/test
     -> Phase 3 complete runtime matrix
        -> Phase 4 Editor service/build/test
           -> Phase 5 writer + two assets + persisted validation
              -> Phase 6 user PIE gate
                 -> Phase 7 docs/final validation/commits
```

No subagent or parallel Unreal writer is permitted. `[P]` tasks denote independent text/test-file preparation only; the main session owns all implementation and verification.

## Implementation strategy

1. Preserve current binary state and finish specifications.
2. Establish/validate the runtime seam without touching assets.
3. Establish/validate Editor tooling against transient fixtures.
4. Mutate the exact two assets once through the verified writer.
5. Stop for user PIE acceptance with no commits.
6. Only after acceptance, update final evidence and create the two ordered commits.
