# Tasks: Floor 1 Production Floor / Map / Host 灰盒基线

**Input**: `spec.md`, `plan.md`, `research.md`, `data-model.md`, `contracts/`

**Tests**: Every C++ checkpoint requires default Unity WacomEditor compilation and focused Automation. Binary creation requires exact MCP writer audit, real AssetRegistry/Blueprint/scene validation, hashes, and idempotence.

**Organization**: Tasks are dependency ordered. No binary mutation may begin before Phase 4 source/tests compile, the source-only checkpoint is committed, the D-drive hydration is verified, and the exact Editor identity is ready.

## Phase 1 — Safe workspace and rule truth

- [x] T001 Create `codex/formal-floor1-production-scene-baseline` worktree from clean main `9fbdb74858b9b4b705a216c8dd7ff539fe44b19e`
- [x] T002 Confirm branch, HEAD, merge-base, Git/LFS clean state, and absence of relevant Unreal/Live Coding processes; record in `quickstart.md`
- [x] T003 Read AGENTS, integration/MCP workflow, Run/Map/DataAuthoring/App/WorldInteraction/Architecture Docs, Specs 006/007/009/010/011/013/014, live validators, builders, Host classes, and tests
- [x] T004 Audit existing Floor/Journey/map packages, 46 Spec 014 assets, shared scene Host Blueprints, path actors, prototype enemy Host, placeholder roots, and FloorEntrance runtime capability
- [x] T005 Obtain and freeze user decisions: new empty map; Floor-only/no incomplete Journey; four reusable enemy Host prefabs

## Phase 2 — Spec Kit design gate

- [x] T006 Write scope, stories, requirements, edge cases, non-goals, and success criteria in `spec.md`
- [x] T007 Record world authority, Journey exclusion, Exit marker debt, enemy prefab, placeholder, idempotence, validation, and hydration decisions in `research.md`
- [x] T008 Define package, Floor, scene actor, enemy prefab, encounter composition, seed-state, report, and readiness models in `data-model.md`
- [x] T009 Freeze exact 20-node/21-edge Floor contract in `contracts/floor1-production-floor-contract.md`
- [x] T010 Freeze world layout, 41 scene bindings, 16 Host mappings, viewpoints, and graybox rules in `contracts/production-scene-binding-manifest.md`
- [x] T011 Freeze four Blueprint/11-part/eight-instance enemy Host contract in `contracts/graybox-enemy-host-contract.md`
- [x] T012 Freeze run/8140 identity, seven-package allowlists, writer lifecycle, and evidence in `contracts/mcp-write-contract.md`
- [x] T013 Freeze layered build/Automation/AssetRegistry/Blueprint/scene/idempotence validation in `contracts/validation-contract.md`
- [x] T014 Write implementation checkpoints, D-drive hydration, risk controls, validation matrix, and commit strategy in `plan.md`
- [x] T015 Generate requirements and Production-scene quality checklists
- [x] T016 Update `.specify/feature.json` and AGENTS managed pointer to Spec 015
- [x] T017 Run Spec Kit cross-artifact analysis, repair inconsistencies, and record results in `quickstart.md`

## Phase 3 — D-drive hydration and absent-target audit

- [x] T018 Audit the proven existing D-drive worktree hydration pattern and all new worktree Content/Binaries/Intermediate/Saved/DDC paths
- [x] T019 Create dedicated `D:\UE_Project\5.7\WacomWorktreeData\formal-floor1-production-scene-baseline` storage without replacing any existing directory/junction
- [x] T020 Hydrate only `Content/Art` and `Content/Asset` from D-drive sources and verify junction targets remain inside the dedicated root; preserve Git/LFS-managed `Content/DreamMaterials`
- [x] T021 Verify Binaries, Intermediate, Saved, and local DDC remain independent beneath the D-drive worktree; confirm no C-drive worktree storage is consumed
- [x] T022 Run AssetRegistry/package existence and SHA-256 audit proving all seven target packages are absent and 46 formal dependencies are readable
- [x] T023 Record hydration topology, disk paths, target absence, dependency hashes, and remaining PIE limitation in `quickstart.md`

## Phase 4 — Inspect-only manifest and seed service

**Goal**: Add a private Editor service that knows the exact seven packages and all expected structures without creating content by default.

**Independent Test**: Manifest/transient specs pass while all seven target packages remain absent; inspect reports seven missing and zero dirty/saved files.

- [x] T024 Add options, groups, manifest entries, expected scene records, report, and error model in `Source/WacomEditor/Private/ContentBuilders/FormalFloor1ProductionSceneBuilder.h`
- [x] T025 Implement exact seven-package/group manifest and dependency closure in `FormalFloor1ProductionSceneBuilder.cpp`
- [x] T026 Implement deterministic transient `DA_Floor_Main_01` `UWacomFloorMapDefinition` configuration and structural graph comparator
- [x] T027 Implement expected Blueprint descriptors, part sets, map actor/binding counts, Host payloads, encounter slots, and placeholder policies without package creation
- [x] T028 Implement inspect-only AssetRegistry/load/class/collision/dependency/forbidden-reference traversal with zero-dirty guarantee
- [x] T029 Implement JSON report and stable exit-category mapping
- [x] T030 Add `WacomBuildFormalFloor1ProductionScene` commandlet declaration/adapter
- [x] T031 Add in-process `Wacom.BuildFormalFloor1ProductionScene` Editor console adapter
- [x] T032 Add minimum non-reflection Automation view in `WacomEditor/Public/Testing` and `Private/Testing`
- [x] T033 Add isolated manifest/args/transient-graph/scene-contract tests in `WacomTests/Private/Editor/FormalFloor1ProductionSceneManifestSpec.cpp`
- [x] T034 Implement whole-group preflight, missing-only create states, no-force/no-overwrite paths, and existing-expected-class inspect behavior
- [x] T035 Implement optional `CompareSeedDefaults` reporting that never writes tunable drift
- [x] T036 Pass `AssertClosedForBuild`, compile default Unity WacomEditor, and record command/result/hash
- [x] T037 Run `Wacom.Editor.FormalFloor1ProductionScene` transient tests plus `Wacom.Data.Map.Validation`; record discovered/pass/fail counts
- [x] T038 Run inspect-only command against absent targets and prove zero Content changes
- [x] T039 Stage only Spec 015, pointers, Editor source, and transient tests; confirm zero `.uasset/.umap`; commit `feat(editor): add formal floor1 scene seeder`

## Phase 5 — Seed and validate Floor DataAsset

**Goal**: Create exactly one valid `DA_Floor_Main_01` package.

**Independent Test**: Asset loads as `UWacomFloorMapDefinition` and exact graph/identity/reference validation passes; no other binary package changes.

- [x] T040 Start exact target Editor through the formal MCP workflow using Role `run`, exact ProjectRoot/branch, and record session provenance
- [x] T041 AssertReady immediately before mutation and acquire writer for the one-package Floor allowlist
- [x] T042 Invoke `SeedMissing Group=Floor CompareSeedDefaults` and save exactly `DA_Floor_Main_01`
- [x] T043 Audit Git path, SHA-256, LFS, report, Editor log, and allowlist while writer is held; then release writer
- [x] T044 Run real `UWacomFloorMapDefinition` validation: 20/21 counts, type distribution, IDs, reachability, merges, dominance, positions, Camp, payloads, Guardian, Exit target/credential, and forbidden references
- [x] T045 Record report/audit JSON and Floor validation evidence in `quickstart.md`

## Phase 6 — Seed and validate four enemy Host Blueprints

**Goal**: Create four reusable archetype prefabs from formal EnemyDefinitions without modifying shared content.

**Independent Test**: 4/4 Blueprints compile and expose exact parent/definition/11-part topology; all visual dependencies are controlled placeholders.

- [x] T046 Implement initial-only Blueprint creation and EnemyDefinition assignment for four archetypes
- [x] T047 Invoke existing `SyncPartsFromDefinition` authoring for initial creation and apply controlled graybox part presentation/layout defaults
- [x] T048 Add compile/reload/parent/definition/part/placeholder validation and ensure existing Blueprint paths are inspect-only
- [x] T049 Close Editor, pass `AssertClosedForBuild`, compile default Unity WacomEditor, and run transient enemy Host authoring tests
- [x] T050 Start/AssertReady exact Editor and acquire writer for the four-package EnemyHosts allowlist
- [x] T051 Invoke `SeedMissing Group=EnemyHosts CompareSeedDefaults`; compile and save exactly four Blueprints
- [x] T052 Audit Git paths, four SHA-256 values, LFS, placeholder references, report, and allowlist while writer is held; then release writer
- [x] T053 Run real Blueprint compile, 4 definition/11 part closure, controlled-reference, and affected Battle scene-host authoring tests
- [x] T054 Record report/audit JSON, placeholder release blocker, and validation evidence in `quickstart.md`

## Phase 7 — Seed and validate Exit marker and Production map

**Goal**: Create the non-interactive Exit marker and a complete locally valid Production Floor 1 graybox world.

**Independent Test**: Reloaded map contains exact descriptor/Anchor/Path/BranchTarget/content Host/enemy Host/viewpoint contracts and passes scene validation.

- [x] T055 Implement initial-only Exit marker Blueprint creation with visual diagnostics, RunMapNodeBinding, Blueprint PersistentId, and no interaction/travel logic
- [x] T056 Implement new empty World creation and basic graybox environment without Level Blueprint gameplay logic
- [x] T057 Place/configure the one descriptor and exact 20 Anchors from the world layout manifest
- [x] T058 Place/configure exact 21 Path actors and four BranchTargets with correct source/target bindings and clear traversal splines
- [x] T059 Place/configure 16 content Hosts with exact NodeType, payload mirror, PersistentId, trigger reachability, and no Navigation Host
- [x] T060 Place/configure eight enemy Host instances with exact Encounter slot mapping and six Battle viewpoints
- [x] T061 Place/configure four Event and one Shop viewpoint; stage Pickups and Exit marker beside Anchors without blocking traversal
- [x] T062 Implement reload-time map contract inspection, local scene validator invocation, forbidden-reference scan, and Level Blueprint no-logic check
- [x] T063 Close Editor, pass `AssertClosedForBuild`, compile default Unity WacomEditor, and run transient scene-contract tests
- [x] T064 Start/AssertReady exact Editor and acquire writer for the two-package Scene allowlist
- [x] T065 Invoke `SeedMissing Group=Scene CompareSeedDefaults`; compile marker and save only marker/map
- [x] T066 Audit Git paths, two SHA-256 values, LFS, report, Editor log, and allowlist while writer is held; then release writer
- [x] T067 Run `WacomValidateRunFloorScene` against the Production map and affected `Wacom.Editor.RunSceneValidation`/Run scene-binding/path tests
- [x] T068 Run real 5/5 Blueprint compile, 7/7 AssetRegistry/failed-load/class checks, exact scene counts, Host payload/PersistentId, encounter composition, and forbidden-reference audits
- [x] T069 Perform read-only viewport inspection of branch direction, Host visibility/spacing, traversal clearance, viewpoint staging, part targetability, and Exit diagnostic; do not save
- [x] T070 Record report/audit JSON, scene validation, viewport findings, and explicit Golden Path PIE skip in `quickstart.md`

## Phase 8 — Idempotence, regression, durable Docs, and handoff

- [x] T071 Run `SeedMissing Group=All CompareSeedDefaults` a second time under exact seven-package writer allowlist; require 0 created/0 saved and stable seven SHA-256 values
- [x] T072 Add read-only real-asset tests in `WacomTests/Private/Editor/FormalFloor1ProductionSceneAssetSpec.cpp`
- [x] T073 Release writer, close Editor normally, pass `AssertClosedForBuild`, and compile final default Unity WacomEditor
- [x] T074 Run all focused prefixes from `contracts/validation-contract.md`; record exact discovered/pass/fail counts
- [x] T075 Re-run AssetRegistry/failed-load, five Blueprint compile, local scene validator, 46-dependency hash, forbidden-reference, and LFS audits
- [x] T076 Update `Docs/WacomMap.md` and `Docs/WacomDataAuthoring.md` with implemented Floor/map/Host package and scene-authoring facts
- [x] T077 Update `Docs/WacomApp.md`/`Docs/WacomWorldInteraction.md` only with the confirmed non-interactive Exit limitation and future cross-floor ownership boundary
- [x] T078 Update `Docs/Architecture.md` only if the reusable Editor seed service adds durable authoring architecture; otherwise record no architecture change
- [x] T079 Update `Docs/TODO.md`, `Docs/Questions.md`, `Docs/Roadmap.md`, and `Docs/TechDebt.md` with completed local graybox work and remaining Journey/Floor2/3/transition/art/Exit-marker blockers
- [x] T080 Complete `quickstart.md` with branch/base/final commits, changed files, seven hashes, writer/session audit, build/test/AssetRegistry/Blueprint/scene/viewport results, skips, and risks
- [x] T081 Run Spec Kit consistency recheck, `git diff --check`, scope and forbidden-path audit, `git status`, `git lfs status`, and `git lfs fsck`
- [x] T082 Stage real-asset tests, seven binaries, durable Docs, and final evidence only; commit `feat(content): seed formal floor1 production scene`
- [x] T083 Confirm worktree/LFS clean and produce standard `Docs/AgentIntegrationWorkflow.md` handoff; do not merge main or push

## Dependencies

```text
Phase 1 -> Phase 2 -> Phase 3 hydration/absence
                         -> Phase 4 source + tests + source-only commit
                            -> Phase 5 Floor
                               -> Phase 6 EnemyHosts
                                  -> Phase 7 Scene
                                     -> Phase 8 idempotence/docs/final commit
```

`Scene` depends on both `Floor` and `EnemyHosts`. Binary mutation is forbidden before the source-only checkpoint compiles, tests, and commits.

## Parallel opportunities

No subagent or parallel binary mutation is planned. The main task executes groups serially because Unreal Editor has one writer and all packages participate in the same dependency chain. Within a closed-Editor checkpoint, independent read-only hashes or test enumeration may run in parallel only when it cannot obscure logs.
