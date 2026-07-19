# Implementation Plan: Floor 1 Preview 启动层

**Branch**: `codex/formal-floor1-production-scene-baseline` | **Date**: 2026-07-19 | **Spec**: `specs/016-formal-floor1-preview-bootstrap/spec.md`

**Input**: Add a strictly Editor-PIE, descriptor-derived transient Journey and an allowlisted Preview bootstrap for the existing Spec 015 Floor 1 Production map.

## Summary

Introduce a narrow virtual journey-resolution seam in `AWacomGameMode`, consume it from `AWacomPlayerController`, and implement `AWacomRunFloorPreviewGameMode` as a reusable PIE-only subclass. It resolves the loaded Floor through the existing unique Descriptor contract and caches a GameMode-owned transient Journey rather than fabricating Production content. An Editor-only two-package seed/inspect service creates the generic Preview GameMode Blueprint and updates only the Production map's World Settings plus one Entry PlayerStart. Runtime contracts in `WacomRun`/`WacomData` remain unchanged.

## Wacom Domain Context

**Primary Domain**: Run-exploration, UI-App shell, content authoring/validation

**Required Docs Read**:

- [x] `AGENTS.md`
- [x] `Docs/AgentIntegrationWorkflow.md`
- [x] `Docs/UnrealMCPWorkflow.md`
- [x] `Docs/WacomApp.md`
- [x] `Docs/WacomMap.md`
- [x] `Docs/WacomDataAuthoring.md`
- [x] `Docs/Architecture.md`
- [x] Spec 015 artifacts and live implementation

**Docs To Update**:

- [x] `Docs/WacomApp.md`
- [x] `Docs/WacomMap.md`
- [x] `Docs/WacomDataAuthoring.md`
- [x] `Docs/Architecture.md`
- [x] `Docs/TODO.md`
- [x] `Docs/Questions.md`
- [x] `Docs/TechDebt.md`

**Owning Modules**: `WacomApp`, `WacomEditor`, `WacomTests`

**Dependency Direction**:

```text
WacomCore <- WacomData <- WacomBattle <- WacomRun <- WacomApp

WacomEditor -> WacomData/WacomRun/WacomApp for editor-only authoring
WacomTests  -> runtime/app/editor validation as test harness
```

No Build.cs or module edge changes are required.

## Technical Context

**Language/Engine**: C++20, Unreal Engine 5.8

**Primary UE Systems**: GameMode/PlayerController lifecycle, DataAssets, map World Settings, Blueprint generation, AssetRegistry, Automation Tests, Editor save/reload utilities

**Storage/State**: one `RF_Transient` GameMode-owned Journey; no `FRunState` or SaveGame change

**Runtime Contracts**: non-reflection virtual resolver on `AWacomGameMode`; existing `FRunInitializationParams` and Descriptor resolver remain authoritative

**Testing**: `Wacom.App.RunFloorPreviewBootstrap`, `Wacom.Editor.FormalFloor1PreviewBootstrap`, five existing Run/Data/Editor prefixes, default Unity WacomEditor build, asset inspection, hashes/LFS, and user PIE

**Target Platform**: Windows Editor PIE only

**Performance Goals**: resolution occurs only during new-Run initialization; no Tick, polling, persistent allocation, or repeated graph copy

**Constraints**: no main merge/rebase, no new worktree, no subagent MCP, no builder, exact two-package binary allowlist, no commit before PIE acceptance

**Scale/Scope**: three runtime source files plus a new GameMode pair; one small Editor service/command/test view; two focused test files; one Blueprint and one map mutation

**Blueprint Exposure Strategy**: the Preview class is `Blueprintable` solely so content can configure the existing GameMode presentation classes. Journey resolution stays a non-UFUNCTION C++ virtual; its invariants are not designer-editable.

**Data/GameplayTag Impact**: no new DataAsset schema or GameplayTag; one transient instance of the existing Journey class

**Save/Load Impact**: none; save system remains globally disabled and Preview terminal state is not restorable

**UI/App Lifecycle Impact**: existing Exploration HUD, CommonUI, input contexts, card layer, battle-return staging, menu focus, and JourneySummary class are copied from `GM_Wacom`; Preview changes only Journey selection before Run initialization.

## Constitution Check

- **Docs and AGENTS Are the Rule Truth — PASS**: named rule docs and long-term updates are explicit.
- **Wacom Module Boundaries Are Mandatory — PASS**: App owns startup orchestration; Editor owns mutation; Run/Data contracts are unchanged.
- **Domain Rules Before Presentation — PASS**: Preview supplies existing static inputs to `URunSession`; it does not implement Run rules.
- **Data, GameplayTags, and Authoring Stay Explicit — PASS**: exact two assets and no schema/tag changes are declared.
- **Reusable Systems Over One-Off Work — PASS WITH TEMPORARY DEBT**: the generic Preview GameMode is reusable for future Floor maps, while any Preview dependency in a Production map is a tracked release blocker with a removal trigger.
- **Validation Is Part of the Slice — PASS**: source, Editor asset contract, regression, LFS/hash, and mandatory manual PIE gates are defined.

## Phase 0: Research

Research resolves:

- why `GM_Wacom` causes Descriptor/Floor mismatch on this map;
- whether the existing Descriptor resolver is reusable without public API expansion;
- the smallest safe PIE-only world check;
- how Blueprint CDO configuration can mirror `GM_Wacom` without copying its Journey;
- how the map can be changed without resaving/rebuilding scene actors;
- how to preserve the current dirty binary while continuing on the Spec 015 branch.

Results are recorded in `research.md`; no clarification remains.

## Phase 1: Design Artifacts

- `data-model.md`: transient Journey, cached descriptor identity, Preview Blueprint, PlayerStart, and seed report.
- `contracts/runtime-preview-contract.md`: resolver and lifetime/failure behavior.
- `contracts/preview-asset-contract.md`: exact Blueprint/map contract.
- `contracts/preview-seed-command-contract.md`: command, allowlist, collision, save/reload/idempotence semantics.
- `contracts/preview-pie-contract.md`: manual acceptance and invalidation gate.
- `quickstart.md`: continuously updated execution ledger.
- `tasks.md`: dependency-ordered implementation and validation slices.

## Project Structure

```text
Source/WacomApp/
├── Public/GameFramework/
│   ├── WacomGameMode.h
│   └── WacomRunFloorPreviewGameMode.h
└── Private/GameFramework/
    ├── WacomGameMode.cpp
    ├── WacomPlayerController.cpp
    └── WacomRunFloorPreviewGameMode.cpp

Source/WacomEditor/
├── Public/Testing/WacomFormalFloor1PreviewBootstrapAutomationTestView.h
└── Private/ContentBuilders/
    ├── FormalFloor1PreviewBootstrap.h
    ├── FormalFloor1PreviewBootstrap.cpp
    └── FormalFloor1PreviewBootstrapEditorCommand.cpp

Source/WacomTests/Private/
├── App/RunFloorPreviewBootstrapSpec.cpp
└── Editor/FormalFloor1PreviewBootstrapSpec.cpp

Content/Wacom/
├── Run/Preview/GM_WacomRunFloorPreview.uasset
└── Maps/Run/L_Run_Floor_Main_01.umap
```

**Structure Decision**: the only stable public runtime type is the Blueprintable GameMode and base virtual seam. Descriptor resolution remains App-private. Editor mutation implementation is private; tests receive a narrow non-reflection summary view.

## Implementation Checkpoints

### Checkpoint 1 — Spec and runtime seam

1. Finish Spec 016 artifacts and managed pointers.
2. Add base resolver virtual and route PlayerController initialization through it.
3. Add Preview subclass with strict PIE checks, exact Descriptor resolution, transient Journey creation, identity caching, and fail-closed logging.
4. Add small App automation failure matrix.
5. Close target Editor, compile default Unity, run `Wacom.App.RunFloorPreviewBootstrap` plus relevant scene-binding tests.

### Checkpoint 2 — Editor bootstrap service

1. Implement exact two-package manifest and report.
2. Preflight existing Blueprint/map authority before modifying either package.
3. Create missing Preview Blueprint and copy only the six frozen CDO fields.
4. Change only map GameMode override and add missing exact Entry PlayerStart.
5. Save allowlisted packages, reload/inspect, run internal second pass, and require zero writes.
6. Add focused Editor automation without invoking mutation against real assets.
7. Compile and run focused Editor/regression tests.

### Checkpoint 3 — Binary mutation and asset validation

1. Assert the exact Editor identity and acquire one `run` writer lease for both packages.
2. Record pre-hashes for the dirty map, Spec 014 assets, and Spec 015 six other binaries.
3. User executes `WacomSeedFormalFloor1PreviewBootstrap` once in the verified Editor.
4. Inspect git status before release, capture audit JSON, release writer, close Editor, and verify saved paths.
5. Run Blueprint/load/AssetRegistry/scene checks, second-pass evidence, LFS/hash audits, and final build/tests if source changed.

### Checkpoint 4 — PIE gate and deferred commit

1. Give the user one consolidated PIE checklist.
2. Keep every change uncommitted until the user explicitly confirms all checks.
3. If any runtime or asset change follows, rerun relevant automation and PIE.
4. After acceptance only, stage and create the two approved commits, verify clean/LFS, and hand off without merge/push.

## Validation Plan

**Compile**:

```powershell
& 'E:\UE_5.8\Engine\Build\BatchFiles\Build.bat' WacomEditor Win64 Development -Project='D:\UE_Project\5.7\WacomWorktrees\formal-floor1-production-scene-baseline\Wacom\Wacom.uproject' -WaitMutex -NoHotReloadFromIDE
```

**Focused Automation**:

```text
Wacom.App.RunFloorPreviewBootstrap
Wacom.Editor.FormalFloor1PreviewBootstrap
Wacom.Data.Map.Validation
Wacom.Run.Map.Traversal
Wacom.UI.RunSceneBinding
Wacom.UI.RunPathTraversal
Wacom.Editor.RunSceneValidation
```

All Unreal command invocations add `-NoDreamShaderEditorBridge` and do not run a builder.

**Asset validation**: Blueprint load/compile/parent/CDO; map World Settings and one plain Entry PlayerStart; Spec 015 scene counts and bindings; exact hash and LFS scope; internal second pass zero writes.

**Manual PIE**: entry, camera/movement/HUD/hand, M and ESC mouse interaction, path, battle return, one non-battle interaction return, Exit dead end, and post-stop hash audit. Preview GameMode drift is forbidden; an intentional user-authored map adjustment requires explicit acceptance plus focused post-save validation against the new authoritative hash.

## Complexity Tracking

| Temporary dependency | Why Needed | Formal replacement | Removal gate |
|---|---|---|---|
| Production map overrides a Preview GameMode | Complete Production Journey/Floor2/3 do not exist, but Floor 1 needs local PIE | Production GameMode using `Journey.Main.01` and formal cross-floor flow | Remove Preview override/PlayerStart dependency before release once the three-floor Journey is authored and validated |
