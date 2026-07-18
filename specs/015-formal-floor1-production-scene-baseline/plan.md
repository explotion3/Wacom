# Implementation Plan: Floor 1 Production Floor / Map / Host 灰盒基线

**Branch**: `codex/formal-floor1-production-scene-baseline` | **Date**: 2026-07-18 | **Spec**: `specs/015-formal-floor1-production-scene-baseline/spec.md`

**Input**: Create the exact Floor 1 Production FloorDefinition, isolated graybox map, complete scene binding, and four reusable formal enemy graybox Host prefabs using a seed-only Editor workflow.

## Summary

Implement a private WacomEditor manifest/builder/commandlet that creates seven missing packages in three dependency-ordered groups. First create and validate `DA_Floor_Main_01` as a `UWacomFloorMapDefinition`; then create four `AWacomBattleEnemyActor` Blueprint prefabs from the formal EnemyDefinitions; finally create a non-interactive FloorEntrance graybox marker and the isolated Production map containing the complete Floor scene contract. Existing packages are inspect-only. Every binary write occurs through the verified `run` Unreal MCP session with an exact writer allowlist. The result is locally scene-valid but intentionally not a complete Production Journey.

## Technical Context

**Language/Version**: C++20 as configured by Unreal Engine 5.8; Blueprint/UWorld binary authoring through Editor APIs

**Primary Dependencies**: Unreal Engine 5.8 Editor, AssetRegistry, AssetTools, Kismet/Blueprint Editor utilities already available to `WacomEditor`; WacomData/WacomRun/WacomApp public authoring contracts

**Storage**: Git LFS for `.uasset/.umap`; no database or SaveGame changes

**Testing**: Unreal Automation in `WacomTests`; read-only AssetRegistry/load checks; scene validator; Blueprint compiler; SHA-256/LFS/MCP audit; viewport inspection

**Target Platform**: Win64 Development Editor

**Project Type**: UE multi-module game project

**Performance Goals**: Editor-only seed/inspect completes without runtime Tick or shipping dependencies; map scene registry remains O(actor count) as existing contract

**Constraints**: no new module dependency, no Build.cs, no GameplayTag, no runtime schema changes, no existing binary writes, no builder outside the named feature command, no full PIE claim

**Scale/Scope**: 7 packages; 20 nodes; 21 edges; 41 binding actors; 16 content Hosts; 8 enemy scene Hosts; 11 viewpoints

## Constitution Check

### I. Canonical rule truth and data-driven design — PASS

- Floor graph/payload truth is a `UWacomFloorMapDefinition`, not map labels or Level Blueprint logic.
- Map actors mirror and bind stable IDs; they do not become rule authority.
- Existing Spec 014 DataAssets are referenced read-only.

### II. Module boundaries — PASS

- All creation/configuration/validation implementation lives in `WacomEditor`.
- Runtime modules gain no Editor dependency and no public contract change.
- Tests live in `WacomTests` and use a narrow non-reflection Editor test view.

### III. Reusable architecture — PASS WITH DOCUMENTED TEMPORARY DEBT

- Enemy Host Blueprints are reusable per archetype and use the existing authoring service.
- The seed service is manifest-based and inspect-first, not a one-shot script.
- The FloorEntrance marker is temporary graybox debt because cross-floor runtime does not exist; it has an explicit removal trigger and no fake behavior.

### IV. Deterministic mutation and idempotence — PASS

- Whole-group preflight and exact allowlists prevent partial or out-of-scope saves.
- Existing targets are never configured or saved.
- Second run must be zero-create/zero-save with stable hashes.

### V. Test strategy — PASS

- Stable graph/binding/composition contracts use Automation.
- Visual spacing/direction uses viewport review.
- Full Golden Path PIE is explicitly blocked rather than simulated with an invalid Journey.

### VI. Documentation and evidence — PASS

- Durable scene-authoring facts will update domain Docs.
- Temporary Exit marker and placeholder art gates will be recorded in TODO/TechDebt.
- Quickstart will contain build/test/hash/audit/skip evidence.

## Project Structure

### Feature artifacts

```text
specs/015-formal-floor1-production-scene-baseline/
├── spec.md
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── tasks.md
├── contracts/
│   ├── floor1-production-floor-contract.md
│   ├── production-scene-binding-manifest.md
│   ├── graybox-enemy-host-contract.md
│   ├── mcp-write-contract.md
│   └── validation-contract.md
└── checklists/
    ├── requirements.md
    └── production-scene.md
```

### Source changes

```text
Source/WacomEditor/Private/ContentBuilders/
├── FormalFloor1ProductionSceneBuilder.h
├── FormalFloor1ProductionSceneBuilder.cpp
└── FormalFloor1ProductionSceneEditorCommand.cpp

Source/WacomEditor/Private/Commandlets/
├── WacomBuildFormalFloor1ProductionSceneCommandlet.h
└── WacomBuildFormalFloor1ProductionSceneCommandlet.cpp

Source/WacomEditor/Public/Testing/
└── WacomFormalFloor1ProductionSceneAutomationTestView.h

Source/WacomEditor/Private/Testing/
└── WacomFormalFloor1ProductionSceneAutomationTestView.cpp

Source/WacomTests/Private/Editor/
├── FormalFloor1ProductionSceneManifestSpec.cpp
└── FormalFloor1ProductionSceneAssetSpec.cpp
```

Exact file placement may be adjusted to live module conventions without changing ownership.

### Durable documentation

```text
Docs/WacomMap.md
Docs/WacomDataAuthoring.md
Docs/WacomApp.md
Docs/WacomWorldInteraction.md
Docs/Architecture.md
Docs/TODO.md
Docs/Questions.md
Docs/Roadmap.md
Docs/TechDebt.md
```

Only files whose durable facts materially change will be edited after implementation.

## Command contract

Proposed commandlet/editor command:

```text
Wacom.BuildFormalFloor1ProductionScene
  [Inspect]
  [SeedMissing]
  [Group=Floor|EnemyHosts|Scene|All]
  [CompareSeedDefaults]
  [Report=<absolute-json-path>]
```

Defaults:

- no `SeedMissing` means inspect-only;
- missing target in inspect is reported and produces a non-success validation result but no dirty package;
- `Group=All` preflights all seven packages and all dependencies before the first save;
- invalid/missing group or unknown argument fails without mutation;
- no force/replace/regenerate/delete option exists.

## Implementation checkpoints

### Checkpoint 1 — Spec/manifest and inspect-only service

1. Freeze seven-package manifest, graph, scene actors, Host payloads, enemy composition, layout, and validation contracts.
2. Add builder options/report and exact in-code manifest.
3. Add deterministic transient expected FloorDefinition construction.
4. Add expected Blueprint/map descriptors without creating packages.
5. Add inspect-only AssetRegistry/load/class/collision/dependency reporting.
6. Add narrow Automation test view and manifest/transient tests.
7. Compile default Unity WacomEditor and run manifest/Data.Map tests.

**Exit criteria**: Target packages remain absent; inspect reports seven missing; no content change; build/tests pass.

### Checkpoint 2 — Seed-only Floor group

1. Implement whole-group preflight and missing-only asset creation.
2. Create/configure `UWacomFloorMapDefinition` from the canonical 20/21 manifest.
3. Validate exact graph and references before and after save/reload.
4. Compile and run transient tests before Editor mutation.
5. Through MCP writer allowlist, seed exactly one Floor package.
6. Run real Floor/Data.Map validation and record hash/audit.

**Exit criteria**: One Floor package exists and validates; 46 content assets remain byte-identical; no Blueprint/map yet.

### Checkpoint 3 — Four reusable Enemy Host Blueprints

1. Implement initial-only Blueprint creation from `AWacomBattleEnemyActor`.
2. Set exact EnemyDefinition and use existing part synchronization authoring.
3. Add controlled placeholder presentation and archetype-specific part layout defaults.
4. Compile each Blueprint, save/reload, and validate parent/definition/11-part closure.
5. Through MCP writer allowlist, seed exactly four packages.
6. Run focused Battle scene-authoring and Blueprint tests.

**Exit criteria**: Four Blueprints compile; 11 parts map correctly; placeholder references are reported; no existing asset changes.

### Checkpoint 4 — Exit marker and Production map

1. Create a non-interactive `AActor` Blueprint marker with visual graybox components, `UWacomRunMapNodeBindingComponent`, and Blueprint `PersistentId` field.
2. Create the new empty World and basic graybox environment.
3. Place/configure descriptor, 20 Anchors, 21 Paths, 4 BranchTargets, 16 content Hosts, 8 enemy Host instances, and 11 viewpoints.
4. Bind exact FloorDefinition/payload/slot/PersistentId values.
5. Ensure no Level Blueprint gameplay logic and no Debug/Authoring/generated ownership.
6. Compile marker, save map, reload, and run local scene validation.
7. Through MCP writer allowlist, save only marker and map.

**Exit criteria**: Exact scene counts and bindings pass; Exit remains visibly non-interactive; no fake Journey/travel exists.

### Checkpoint 5 — Idempotence, regression, Docs, and handoff

1. Add real-asset focused spec and compile after assets exist.
2. Execute full inspect and all seed groups a second time; require zero saves and stable hashes.
3. Run final Unity build and focused Automation.
4. Run Blueprint compile, AssetRegistry/failed-load, forbidden-reference, scene validator, and read-only viewport review.
5. Update durable Docs and feature quickstart.
6. Run diff/LFS/scope checks and commit source/Spec separately from binary/Docs if needed for safe integration.

**Exit criteria**: Branch/worktree/LFS clean; standard handoff identifies two readiness states: local Floor scene complete, full Journey blocked.

## Worktree hydration and Unreal lifecycle

Use dedicated D-drive data root:

```text
D:\UE_Project\5.7\WacomWorktreeData\formal-floor1-production-scene-baseline\
└── LocalDependencies\Content\
    ├── Art
    └── Asset
```

`Content/DreamMaterials` remains Git/LFS-managed in the worktree. Binaries, Intermediate, Saved, and DDC remain independent beneath the new D-drive worktree; they are not junctioned to another worktree or duplicated into the hydration root.

Before creating junctions:

- audit the new worktree paths and compare the proven existing D-drive hydration pattern;
- verify every destination resolves beneath the dedicated data root;
- never replace a non-empty directory or existing junction without reporting it;
- use D-drive source copies only; do not consume C-drive worktree storage.

Unreal Editor lifecycle follows `Docs/UnrealMCPWorkflow.md` and `contracts/mcp-write-contract.md`. Editor must be closed for every C++ build. No branch or HEAD changes while Editor is alive.

## Validation matrix

| Check | Before assets | After Floor | After EnemyHosts | After Scene | Final |
|---|:---:|:---:|:---:|:---:|:---:|
| Default Unity compile | Yes | Yes if C++ changed | Yes if C++ changed | Yes if C++ changed | Yes |
| Manifest/transient tests | Yes | Yes | Yes | Yes | Yes |
| Data.Map validation | transient | real | real | real | real |
| Enemy Host authoring | transient | — | real | real | real |
| Blueprint compile | — | — | 4 | 5 | 5 |
| Scene validation | expected manifest | — | — | real map | real map |
| AssetRegistry/failed-load | absent audit | 1 | 5 total | 7 total | 7 total |
| MCP writer audit | — | 1 package | 4 packages | 2 packages | final release |
| Hash/idempotence | baseline | group hash | group hash | group hash | all 7 stable |
| Viewport review | — | — | prefab preview | map | map |
| Full Golden Path PIE | blocked | blocked | blocked | blocked | blocked |

## Focused test targets

The live test registry will be enumerated before execution. Minimum intended scopes:

```text
Wacom.Data.Map.Validation
Wacom.Editor.FormalFloor1ProductionScene
Wacom.Editor.RunSceneValidation
affected Wacom.Run scene-binding/path tests
affected Battle scene enemy Host authoring tests
```

All Unreal command lines include `-NoDreamShaderEditorBridge`. No all-content builder is run.

## Commit strategy

Preferred integration-friendly sequence:

1. `feat(editor): add formal floor1 scene seeder` — Spec 015, managed pointers, Editor source, transient tests; zero binary assets.
2. `feat(content): seed formal floor1 production scene` — seven assets, real-asset tests, durable Docs, final evidence.

If implementation needs a small source follow-up discovered only after real assets exist, keep it as a separate focused commit before the binary/Docs commit. Do not squash across binary authority without integration review.

## Risk register

| Risk | Control |
|---|---|
| Wrong Editor/worktree mutates assets | AssertReady identity + exact role/branch/HEAD + writer lease |
| Seeder overwrites manual tuning | existing targets inspect-only; no force path; second-run zero-save |
| Existing 46 assets are resaved | exclude from allowlist; pre/post hashes; fail closed on dirty path |
| Empty map lacks runtime support | explicitly local graybox; no Journey/Golden Path claim |
| Exit marker mistaken for implemented travel | visible diagnostic label; no interaction; Docs/TechDebt gate |
| Placeholder art mistaken for release content | controlled reference report + readiness blocker |
| Enemy parts drift from Definitions | existing sync authoring on create; structural test on every validation |
| Map scene binding silently drifts | exact validator counts/IDs/payload/PersistentId checks |
| New worktree consumes C drive | verified D-drive hydration and independent build outputs |

## Complexity tracking

| Necessary exception | Why needed | Rejected simpler option |
|---|---|---|
| Temporary Blueprint FloorEntrance marker | Validator requires a Host while cross-floor runtime is absent | Fake `OpenLevel` Actor would encode false behavior; omitting Host breaks local contract |
| Editor-only seed service | Seven binary assets have deterministic, reference-rich structure and must be repeatable/auditable | Manual blind authoring cannot prove exact scope or idempotence |
| Four enemy prefab Blueprints | Four formal part topologies require reusable scene representations | Reusing prototype Snake Host would misrepresent content and modify shared authority |
