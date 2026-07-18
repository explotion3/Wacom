# Tasks: Floor 1 Production 46 DataAsset 播种与校验

**Input**: `spec.md`、`plan.md`、`research.md`、`data-model.md`、`contracts/`

**Tests**: 本轮改变 Editor 制作能力并创建正式二进制内容；每个 C++ checkpoint 必须编译和运行聚焦 Automation，资产创建后必须做真实 AssetRegistry/加载/结构/strict/double-run/hash/LFS/MCP audit。

**Organization**: 任务按可独立验证的 checkpoint 排列。任何资产 mutation 必须等 Phase 4 的 C++、测试、首次提交和 Editor 身份门禁全部通过。

## Phase 1 — Safe workspace and rule truth

- [x] T001 Confirm `codex/formal-floor1-production-assets` starts from latest clean main successor and record branch/HEAD/merge-base in `specs/014-formal-floor1-production-assets/quickstart.md`
- [x] T002 Confirm target worktree Git/LFS status and relevant Unreal/Live Coding processes in `specs/014-formal-floor1-production-assets/quickstart.md`
- [x] T003 Confirm all seven SerpentWood target roots are absent and `Binaries/Win64` is not shared in `specs/014-formal-floor1-production-assets/quickstart.md`
- [x] T004 Read `AGENTS.md`, `Docs/AgentIntegrationWorkflow.md`, `Docs/UnrealMCPWorkflow.md`, domain Docs, Spec 011/012/013, live schema, validators, builders, and tests
- [x] T005 Freeze first-seed authority, stable/tunable boundary, group order, error behavior, and Guardian target correction in `specs/014-formal-floor1-production-assets/research.md`

## Phase 2 — Spec Kit design gate

- [x] T006 Write feature requirements, stories, edge cases, non-goals, and success criteria in `specs/014-formal-floor1-production-assets/spec.md`
- [x] T007 Write implementation checkpoints, module boundaries, commands, validation, and commit strategy in `specs/014-formal-floor1-production-assets/plan.md`
- [x] T008 Write manifest/report/state and stable/tunable models in `specs/014-formal-floor1-production-assets/data-model.md`
- [x] T009 Write exact 46-package and four read-only dependency table in `specs/014-formal-floor1-production-assets/contracts/asset-manifest.md`
- [x] T010 Write commandlet/editor bridge, no-overwrite, exit-code, and report contract in `specs/014-formal-floor1-production-assets/contracts/seeder-command-contract.md`
- [x] T011 Write structural/strict/general validation tiers in `specs/014-formal-floor1-production-assets/contracts/validation-contract.md`
- [x] T012 Write run/8140 identity, group allowlists, writer lifecycle, and audit contract in `specs/014-formal-floor1-production-assets/contracts/mcp-write-contract.md`
- [x] T013 Update `.specify/feature.json` and the managed pointer in `AGENTS.md` to Spec 014
- [x] T014 Generate requirements-quality production checklist in `specs/014-formal-floor1-production-assets/checklists/production-assets.md`
- [x] T015 Run Spec Kit cross-artifact consistency analysis and record the result in `specs/014-formal-floor1-production-assets/quickstart.md`

## Phase 3 — Shared manifest and inspect-only service

**Goal**: A single private WacomEditor service can enumerate and inspect the 46-entry manifest without creating or dirtying assets.

**Independent Test**: `Wacom.Editor.FormalFloor1Content.Manifest` passes with target packages absent; inspect reports 46 missing assets, performs zero saves, and validates transient expected objects.

- [x] T016 [P] Add commandlet declaration in `Source/WacomEditor/Private/Commandlets/WacomBuildFormalFloor1ContentCommandlet.h`
- [x] T017 Add manifest/options/report public-private contract in `Source/WacomEditor/Private/ContentBuilders/FormalFloor1ContentBuilder.h`
- [x] T018 Implement option parsing, group selection, exact 46-entry manifest, count/uniqueness/dependency checks in `Source/WacomEditor/Private/ContentBuilders/FormalFloor1ContentBuilder.cpp`
- [x] T019 Implement deterministic transient expected Card configurators for all 12 Cards in `Source/WacomEditor/Private/ContentBuilders/FormalFloor1ContentBuilder.cpp`
- [x] T020 Implement deterministic transient Behavior/Part/Enemy configurators for all 19 EnemyGraph assets in `Source/WacomEditor/Private/ContentBuilders/FormalFloor1ContentBuilder.cpp`
- [x] T021 Implement deterministic transient Encounter/Event/Pickup/Shop configurators for all 15 NodeDefinitions in `Source/WacomEditor/Private/ContentBuilders/FormalFloor1ContentBuilder.cpp`
- [x] T022 Implement inspect-only load/class/state traversal and zero-dirty guarantee in `Source/WacomEditor/Private/ContentBuilders/FormalFloor1ContentBuilder.cpp`
- [x] T023 Implement JSON report and exit-category mapping in `Source/WacomEditor/Private/ContentBuilders/FormalFloor1ContentBuilder.cpp`
- [x] T024 Implement commandlet adapter in `Source/WacomEditor/Private/Commandlets/WacomBuildFormalFloor1ContentCommandlet.cpp`
- [x] T025 Implement in-process `Wacom.BuildFormalFloor1Content` editor console adapter in `Source/WacomEditor/Private/ContentBuilders/FormalFloor1ContentEditorCommand.cpp`
- [x] T026 Add minimum non-reflection Automation view in `Source/WacomEditor/Public/Testing/WacomFormalFloor1ContentAutomationTestView.h` and `Source/WacomEditor/Private/Testing/WacomFormalFloor1ContentAutomationTestView.cpp`
- [x] T027 Add isolated manifest/args/report/transient-contract tests in `Source/WacomTests/Private/Editor/FormalFloor1ContentManifestSpec.cpp`

## Phase 4 — Seed-only create path and C++ checkpoint

**Goal**: Missing assets can be created once, while existing correct-class assets are never modified or saved.

**Independent Test**: Transient tests prove preflight, missing/existing/wrong-class states, structural versus strict comparison, and no force mode; target Production packages remain absent.

- [x] T028 Implement whole-group preflight of existing packages, class correctness, dependencies, and target collisions in `Source/WacomEditor/Private/ContentBuilders/FormalFloor1ContentBuilder.cpp`
- [x] T029 Implement create-configure-save-post-load validation for missing packages only in `Source/WacomEditor/Private/ContentBuilders/FormalFloor1ContentBuilder.cpp`
- [x] T030 Implement default structural comparator for Cards and exact stable effect topology in `Source/WacomEditor/Private/ContentBuilders/FormalFloor1ContentBuilder.cpp`
- [x] T031 Implement default structural comparator for Behavior/Part/Enemy and FormalProduction Part validation in `Source/WacomEditor/Private/ContentBuilders/FormalFloor1ContentBuilder.cpp`
- [x] T032 Implement default structural comparator for Encounter/Event/Pickup/Shop and forbidden-reference closure in `Source/WacomEditor/Private/ContentBuilders/FormalFloor1ContentBuilder.cpp`
- [x] T033 Implement optional exact editable-property `CompareSeedDefaults` comparison in `Source/WacomEditor/Private/ContentBuilders/FormalFloor1ContentBuilder.cpp`
- [x] T034 Extend `Source/WacomTests/Private/Editor/FormalFloor1ContentManifestSpec.cpp` for legacy-null, Part mappings, 24 Intents, 13 Choices, HP curve, Guardian AllEnemyParts, invalid args, and comparator/no-overwrite boundaries
- [x] T035 Pass `Scripts/Invoke-WacomUnrealMcp.ps1 -Action AssertClosedForBuild` for the target worktree and record it in `specs/014-formal-floor1-production-assets/quickstart.md`
- [x] T036 Compile default Unity `WacomEditor Win64 Development` and record command/result/hash in `specs/014-formal-floor1-production-assets/quickstart.md`
- [x] T037 Run `Wacom.Editor.FormalFloor1Content.Manifest` and affected transient validator/rule tests with `-NoDreamShaderEditorBridge`, recording discovered/pass/fail counts in `specs/014-formal-floor1-production-assets/quickstart.md`
- [x] T038 Run inspect-only commandlet against absent target roots and prove zero content changes in `specs/014-formal-floor1-production-assets/quickstart.md`
- [x] T039 Stage only Spec 014, managed pointers, Editor service/adapters, and transient tests; confirm zero `.uasset/.umap`; commit `feat(editor): add formal floor1 content seeder`

## Phase 5 — MCP Cards seed (12)

**Goal**: Create and accept exactly 12 Card packages.

**Independent Test**: Cards report 12 created/12 saved/0 failed; strict inspection and real Card tests pass; no non-Card package changes.

- [x] T040 Start the exact target Editor through `Scripts/Invoke-WacomUnrealMcp.ps1` using Role `run`, exact ProjectRoot/branch, and record session provenance in `specs/014-formal-floor1-production-assets/quickstart.md`
- [x] T041 AssertReady immediately before Cards mutation and acquire writer for the exact 12 package allowlist from `contracts/asset-manifest.md`
- [x] T042 Invoke `Wacom.BuildFormalFloor1Content SeedMissing Group=Cards CompareSeedDefaults` through the exact MCP Editor session and save the Cards JSON report
- [x] T043 Audit Git paths, 12 Card SHA-256, LFS status, report counts, Editor log, and allowlist before releasing the Cards writer; record audit JSON in `specs/014-formal-floor1-production-assets/quickstart.md`
- [x] T044 Run read-only Cards strict inspection and real Card validation tests without saving assets

## Phase 6 — MCP EnemyGraph seed (19)

**Goal**: Create and accept exactly 4 Behavior, 11 Part, and 4 Enemy packages.

**Independent Test**: EnemyGraph report 19 created/19 saved/0 failed; 24 Intents and all 11 explicit Aid/Destroy mappings pass; no other package changes.

- [x] T045 AssertReady and acquire writer for the exact 19 EnemyGraph package allowlist
- [x] T046 Invoke `Wacom.BuildFormalFloor1Content SeedMissing Group=EnemyGraph CompareSeedDefaults` through MCP and save the EnemyGraph JSON report
- [x] T047 Audit Git paths, 19 hashes, LFS, 24 Intent/11 Part report facts, and allowlist before releasing the EnemyGraph writer; record audit JSON
- [x] T048 Run read-only strict inspection plus EnemyPart FormalProduction, Enemy, Behavior, Battle RuleContent, and KnockdownReward tests

## Phase 7 — MCP NodeDefinitions seed (15)

**Goal**: Create and accept exactly 6 Encounter, 4 Event, 4 Pickup, and 1 Shop packages.

**Independent Test**: NodeDefinitions report 15 created/15 saved/0 failed; exact slots/choices/rewards/offers pass; four shared Shop dependencies remain byte-identical.

- [x] T049 Record pre-write SHA-256 for the four read-only Shop Card dependencies, AssertReady, and acquire writer for the exact 15 NodeDefinitions packages
- [x] T050 Invoke `Wacom.BuildFormalFloor1Content SeedMissing Group=NodeDefinitions CompareSeedDefaults` through MCP and save the NodeDefinitions JSON report
- [x] T051 Audit Git paths, 15 hashes, LFS, read-only dependency hashes, report, and allowlist before releasing the NodeDefinitions writer; record audit JSON
- [x] T052 Run read-only strict inspection plus Encounter/Event/Pickup/Shop and affected Run resolution tests

## Phase 8 — Idempotence, real-asset regression, and docs

- [x] T053 AssertReady and execute each of Cards/EnemyGraph/NodeDefinitions a second time with `SeedMissing CompareSeedDefaults`; record 0 created/0 saved and unchanged 46 hashes
- [x] T054 Add read-only real-asset smoke and exact structure tests in `Source/WacomTests/Private/Editor/FormalFloor1ContentAssetSpec.cpp`; compile it before Editor start and execute it only after all 46 assets exist
- [x] T055 Run AssetRegistry/failed-load/class/count/forbidden-reference audit for all 46 packages without saving
- [x] T056 Release all writer ownership, capture final MCP session/audit provenance, close Editor normally, and pass `AssertClosedForBuild`
- [x] T057 Compile final default Unity WacomEditor and run all focused prefixes listed in `specs/014-formal-floor1-production-assets/plan.md`
- [x] T058 Update `Docs/WacomData.md`, `Docs/WacomDataAuthoring.md`, `Docs/WacomBattle.md`, `Docs/WacomRun.md`, `Docs/WacomMap.md`, `Docs/Architecture.md`, `Docs/TODO.md`, `Docs/Questions.md`, and `Docs/Roadmap.md` with durable implemented facts
- [x] T059 Record Blueprint/PIE skips, balance/card-art/backpack risks, full validation matrix, report/audit paths, hashes, and changed binary paths in `specs/014-formal-floor1-production-assets/quickstart.md`
- [x] T060 Run Spec Kit consistency recheck, `git diff --check`, scope/forbidden-path audit, `git lfs status`, and `git lfs fsck`
- [x] T061 Stage the 46 `.uasset`, durable Docs, and final Spec ledger only; commit `feat(content): seed formal floor1 production assets`
- [x] T062 Confirm branch/worktree/LFS clean and produce the standard `Docs/AgentIntegrationWorkflow.md` handoff without merge or push

## Dependencies

```text
Phase 1 -> Phase 2 -> Phase 3 -> Phase 4/first commit
                              -> Phase 5 Cards
                                 -> Phase 6 EnemyGraph
                                    -> Phase 7 NodeDefinitions
                                       -> Phase 8/final commit
```

- Cards must exist before Enemy Parts can reference branch rewards.
- EnemyGraph must exist before Encounters can reference Enemies.
- All C++ and transient tests must be committed before the Editor asset-writing lifecycle starts.
- No build, branch/HEAD change, or C++ edit occurs while the Editor session is open.
- Automation and inspectors are read-only; only the three explicit MCP writer windows may create Production packages.

## Parallel opportunities

`[P]` is intentionally rare. One worktree and one asset writer are used. Documentation analysis may be performed independently, but C++ source and all binary mutations remain serial to preserve auditability.
