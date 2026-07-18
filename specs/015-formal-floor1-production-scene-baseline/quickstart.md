# Quickstart / Verification Ledger: Floor 1 Production Scene Baseline

This file is the continuously updated execution ledger for Spec 015. Commands, hashes, skipped checks, writer audits, and risks are recorded as they occur.

## 1. Workspace preflight

**Date**: 2026-07-18

```text
Worktree: D:\UE_Project\5.7\WacomWorktrees\formal-floor1-production-scene-baseline\Wacom
Branch: codex/formal-floor1-production-scene-baseline
Base main: 9fbdb74858b9b4b705a216c8dd7ff539fe44b19e
Initial HEAD: 9fbdb74858b9b4b705a216c8dd7ff539fe44b19e
Initial merge-base with main: 9fbdb74858b9b4b705a216c8dd7ff539fe44b19e
```

Preflight results:

- Git status: clean on the target branch.
- Git LFS status: clean.
- Relevant UnrealEditor/UnrealEditor-Cmd/Live Coding processes: none at branch creation.
- Branch and worktree did not exist before creation.
- No merge/rebase of the old Run branch was performed.

## 2. User-approved scope decisions

```text
A — create a new empty Production map; do not migrate L_Exploration
A — create Floor 1 only; no incomplete Production Journey; Exit is non-interactive graybox
A — create four reusable formal SerpentWood graybox enemy Host Blueprints
```

Consequences:

- Local Floor graph and scene binding can be fully validated.
- Full Production Journey, Floor 2/3, floor transition, Journey success, and Golden Path PIE remain blocked.
- Placeholder art references remain explicit release blockers.

## 3. Spec Kit design review

Artifacts generated:

- `spec.md`
- `plan.md`
- `research.md`
- `data-model.md`
- `contracts/floor1-production-floor-contract.md`
- `contracts/production-scene-binding-manifest.md`
- `contracts/graybox-enemy-host-contract.md`
- `contracts/mcp-write-contract.md`
- `contracts/validation-contract.md`
- `tasks.md`
- `checklists/requirements.md`
- `checklists/production-scene.md`

Cross-artifact analysis completed 2026-07-18:

```text
Prerequisite feature directory:
D:\UE_Project\5.7\WacomWorktrees\formal-floor1-production-scene-baseline\Wacom\specs\015-formal-floor1-production-scene-baseline

Requirements: 16 FR + 10 buildable SC
User stories: 5
Tasks: 83
Coverage: 100%
Constitution CRITICAL issues: 0
HIGH issues: 0
Ambiguity/duplication issues: 0
```

The prerequisite script required `SPECIFY_FEATURE=015-formal-floor1-production-scene-baseline` and `SPECIFY_FEATURE_DIRECTORY=specs/015-formal-floor1-production-scene-baseline` because repository branches use the mandatory `codex/` prefix. With those explicit non-mutating overrides, it resolved the correct feature and all required artifacts.

Execution watch item: if any group-specific creator source is added after the first source-only commit, it must compile, test, and receive a focused source follow-up commit before that group's binary mutation. This is already permitted by the commit strategy and does not change the approved contracts.

## 4. D-drive hydration

Live audit correction before hydration:

- `Content/DreamMaterials` is present and tracked by Git/LFS in the new worktree; it must not be replaced by a junction.
- `Content/Art` and `Content/Asset` are absent and are the only required non-Git hydration trees.
- The main D-drive source contains 518 Art files / 186,875,528 bytes and 429 Asset files / 90,035,725 bytes.
- Binaries, Intermediate, Saved, and DDC will remain worktree-local on D:, not shared with any other Editor identity.

T018–T023 completion details continue below after copy/link verification.

Hydration completed 2026-07-18:

```text
Dedicated root:
D:\UE_Project\5.7\WacomWorktreeData\formal-floor1-production-scene-baseline

Content/Art junction ->
D:\UE_Project\5.7\WacomWorktreeData\formal-floor1-production-scene-baseline\LocalDependencies\Content\Art

Content/Asset junction ->
D:\UE_Project\5.7\WacomWorktreeData\formal-floor1-production-scene-baseline\LocalDependencies\Content\Asset
```

- Art copy: 60 directories, 518 files, 186,875,528 bytes, 0 failed.
- Asset copy: 50 directories, 429 files, 90,035,725 bytes, 0 failed.
- Both resolved junction targets were verified beneath the dedicated root.
- No pre-existing target directory or junction was replaced.
- `Content/DreamMaterials` remained the tracked worktree directory.
- Binaries/Intermediate/Saved were initially absent and will be generated independently beneath the D-drive worktree.
- Seven target binary paths were all absent.
- All 46 Spec 014 manifest files were present and hash-readable. Ordered path/hash aggregate SHA-256: `D9308919279B3E7DC706AF26869A558808B6524B075B20A5E4E771DDFE4D6AC0`.
- `git lfs fsck`: `Git LFS fsck OK`.
- The hydration junctions are ignored local dependencies and do not appear in Git status.

Post-build Unreal audit completed:

```text
WacomBuildFormalFloor1Content -Group=All -CompareSeedDefaults
Manifest/selected/existing: 46 / 46 / 46
Missing/failed/saved: 0 / 0 / 0
Exit: 0
Report SHA-256: E43D9A53A060E76F35C11C01A7D53AF1EDD97ED0108F08B9B43277061FCF788C
```

The target inspect reported all seven target packages absent, with zero failures
and zero saves. Together with the filesystem hash audit, this closes T022/T023.

## 5. Source-only checkpoint

Implemented T024–T038:

- private seven-package manifest and strict argument parser; `Force` is rejected;
- deterministic transient Floor graph plus exact payload and dominance comparator;
- four formal Enemy Host prefab descriptors, 11-part initial-sync layout, controlled placeholder policy, compile/reload inspection, and no hardcoded Encounter slot;
- non-interactive Exit marker Blueprint descriptor with non-colliding cube, `RunMapNodeBinding`, and Blueprint `PersistentId`;
- new-blank-map scene creator with exact `1 descriptor / 20 Anchors / 21 Paths / 4 BranchTargets / 16 content Hosts / 8 enemy Hosts / 11 viewpoints`;
- shared scene validator, payload/PersistentId/encounter-slot checks, dependency closure, and forbidden Production reference audit;
- commandlet, in-process console adapter, non-reflection test view, and isolated Automation specs.

Build gate and compilation:

```text
AssertClosedForBuild: ReadyForBuild=True
Target: WacomEditor Win64 Development, default Unity configuration
Final result: Succeeded
Final incremental compile execution: 8.41 seconds
UBT log SHA-256 at capture: 59E9430491E23C07C1439D952C0C24910F5EA09A10F6725A3B44C75BA1D8B7ED
```

Two expected correction cycles occurred before the final pass:

- live `FEnemyPartSlot` uses `PartDef`, not the planning alias `PartDefinition`;
- the marker variable pin category is the literal K2 `name` category so the existing module graph remains unchanged; no `BlueprintGraph` dependency was added.

Focused Automation:

```text
Wacom.Editor.FormalFloor1ProductionScene: 3 discovered / 3 passed / 0 failed
Log SHA-256: 6A55248FB6FCA9FEEF8A35011EA358AEA01BC2206FED20491310BE34738D47D5

Wacom.Data.Map.Validation: 7 discovered / 7 passed / 0 failed
Log SHA-256: 03F70C212C6AD8579747C79AAA0D3B098BE592A365D4C7E6BAEECC97D2DC9099
```

Source-checkpoint inspect:

```text
Manifest/selected/missing: 7 / 7 / 7
Existing/failed/saved: 0 / 0 / 0
Expected inspect exit: 1 (targets absent)
Report SHA-256: D433697D8F5613D8AAD6491248BA29FA8E59562AB1473415B615EC0E3C1FBE62
Content binary changes: none
```

Known unrelated startup load warning: existing
`/Game/Wacom/Data/Cards/BugGirl/DA_Card_BugGirlBag` references absent local package
`/Game/Asset/Card_Luo/Card62/T_Card_Illustration_Test_Depth5_Source`.
The exact file was absent from the main D-drive hydration source and both known
C-drive reference worktrees. It is not referenced by Spec 015 content, did not
fail focused tests, and was not synthesized or repaired in this task. Final
failed-load reporting must keep this separate from Spec 015 package validation.

T039 source-only commit is the next gate; no binary mutation may begin before it.

## 6. Floor asset checkpoint

Pending T040–T045.

Expected `UWacomFloorMapDefinition` package:

```text
/Game/Wacom/Data/Map/Production/DA_Floor_Main_01
```

Record MCP session/writer audit, seed report, SHA-256, LFS state, and exact graph validation.

## 7. Enemy Host checkpoint

Pending T046–T054.

Expected packages: four entries from `contracts/graybox-enemy-host-contract.md`.

Record MCP session/writer audit, hashes, Blueprint compile, 11-part closure, placeholder dependencies, and tests.

## 8. Scene checkpoint

Pending T055–T070.

Expected packages:

```text
/Game/Wacom/Run/SceneActors/Graybox/BP_WacomRunFloorEntranceMarker_Graybox
/Game/Wacom/Maps/Run/L_Run_Floor_Main_01
```

Record exact actor counts, validator output, hashes, five-Blueprint compile, AssetRegistry/failed loads, viewport review, and Exit no-travel evidence.

## 9. Final validation matrix

| Check | Result | Evidence |
|---|---|---|
| Default Unity WacomEditor build | Pending | — |
| Focused Automation | Pending | — |
| 7/7 AssetRegistry/load/class | Pending | — |
| 5/5 Blueprint compile | Pending | — |
| Floor 20/21 graph validation | Pending | — |
| Scene 1/20/21/4/16 binding validation | Pending | — |
| 8 enemy Host slot composition | Pending | — |
| Second seed 0 create/0 save | Pending | — |
| Seven SHA-256 stable | Pending | — |
| 46 dependencies byte-identical | Pending | — |
| Git LFS fsck | Pending | — |
| Full Golden Path PIE | Required skip | No valid Production Journey/Floor2/3/transition runtime |

## 10. Continuous risks and skips

- **PIE risk**: Local map binding tests cannot prove full Run startup or cross-floor completion without a valid Production Journey.
- **Exit risk**: FloorEntrance Host is a non-interactive graybox marker and must not be mistaken for implemented travel.
- **Art risk**: Four enemy prefabs use controlled placeholders and are not release-ready.
- **Balance risk**: This slice does not evaluate encounter/card balance.
- **Binary risk**: `.uasset/.umap` changes are non-mergeable; integration must choose this branch as authority for the seven new paths.
- **Builder risk**: Only the named seed-only command is allowed; all other builders remain forbidden.

## 11. Final handoff

Pending completion. Must include:

- branch/worktree/base/final commits;
- changed source, Docs, Spec, `.uasset`, and `.umap` files;
- MCP session and writer audit paths;
- seven hashes and dependency-hash result;
- Git LFS state;
- build/Automation/Blueprint/AssetRegistry/scene/viewport results;
- full PIE skip reason;
- known issues/conflicts;
- explicit “not merged, not pushed” statement.
