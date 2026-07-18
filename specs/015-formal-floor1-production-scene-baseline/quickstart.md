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

Completed through the formal Run MCP endpoint. The one-package writer allowed only:

```text
/Game/Wacom/Data/Map/Production/DA_Floor_Main_01
```

Evidence:

```text
MCP session: 5845bab1-2c80-4237-9372-6c9721c7b260
Seed report: Saved/FormalFloor1ProductionScene/Spec015-floor-seed.json
Report SHA-256: 45E68EA9A9DACD161667B1B1C6AF97426A8EFDA337C6FC7025DDA7349A4E5D9B
Writer audit: C:\Users\ahhh\AppData\Local\Wacom\UnrealMcp\Audits\20260718-160553-994-run-5845bab1-2c80-4237-9372-6c9721c7b260.json
Asset SHA-256: 8391FCFC362D7F4B3B7168D38B3B5746917470BCF335C9089EAFEE4C4C86BA43
```

The loaded object is `UWacomFloorMapDefinition`. Validation passed exact `20 Nodes / 21 Edges`, type distribution `4 Navigation / 6 Encounter / 4 RunEvent / 4 Treasure / 1 Shop / 1 FloorEntrance`, Entry reachability, both branch merges, Key dominance, finite unique map positions, four Camp nodes, typed payload closure, Guardian boss flag, and Exit target/credential. No Journey package was fabricated.

## 7. Enemy Host checkpoint

Four missing-only Blueprint packages were created from the formal EnemyDefinitions through the existing `SyncPartsFromDefinition` authoring path. Existing paths remain inspect-only.

```text
MCP session: edfa2416-81c5-4603-a381-b32ddf4f7f74
Seed report: Saved/FormalFloor1ProductionScene/Spec015-enemy-hosts-seed.json
Report SHA-256: 28B06D7C6A0044CEABCFA34AFF2DFA32871C7274CD738F63233FAA1F05C068DD
Writer audit: C:\Users\ahhh\AppData\Local\Wacom\UnrealMcp\Audits\20260718-161720-000-run-edfa2416-81c5-4603-a381-b32ddf4f7f74.json
```

| Archetype | SHA-256 |
|---|---|
| BrushSnake | `00AEE67043990FC2644D5437753EF0BCE5F79D4CE868A95D7E50B39E3B6595B8` |
| MoltGuard | `E0E779190E1121FAF9E5115E5A355D3C1C0C128C92609E4BBD0A31EA5BA059BA` |
| RootStalker | `74387C768B30553CF267976A84AE3FC8371F361E3548350860649EC40B1B6BCB` |
| ShallowGuardian | `1ABDFFE0DAE1043DC90C04DA15B53FF3B97316A8FED3B737A8DC1A12CF0674A0` |

All four compile and reload with the expected parent, formal Definition, and exact 11-part closure. Placeholder dependencies are restricted to the controlled Wacom placeholder root. This is a development graybox contract, not release-art approval; `-FailOnPlaceholder` remains a release blocker.

## 8. Scene checkpoint

The Scene group created:

```text
/Game/Wacom/Run/SceneActors/Graybox/BP_WacomRunFloorEntranceMarker_Graybox
/Game/Wacom/Maps/Run/L_Run_Floor_Main_01
```

The marker compiles with an instance-editable Blueprint `PersistentId`, `RunMapNodeBinding`, and a non-colliding visible cube. It has no interaction, click bridge, travel component, `OpenLevel`, or Level Blueprint gameplay logic. Final marker SHA-256 is `905193ACB8CF509FF1FDF1ECF1F110C80EA3E6BF8CAD6601D4B07A1B37B62A0D`.

Final map contract after reload:

```text
1 Descriptor / 20 Anchors / 20 pads / 21 Paths / 4 BranchTargets
16 content Hosts / 8 enemy Hosts / 11 viewpoints / 20 enemy PartActors
Encounter instances: BrushSnake 4 / MoltGuard 2 / RootStalker 1 / ShallowGuardian 1
```

The first scene writer audit is `20260718-163241-858-run-c6214e8e-0bbe-46b2-9b8c-97a4369a0eba.json`. Two source corrections were required before final acceptance: SCS-template validation for generated Blueprint components and explicit map loading for persisted-world inspection. A third correction made marker `PersistentId` instance editable. All source corrections were compiled/tested and committed before the corrected asset replay. Corrected replay and first all-asset idempotence audits are:

```text
C:\Users\ahhh\AppData\Local\Wacom\UnrealMcp\Audits\20260718-164906-611-run-d28f046d-9f01-48be-ad43-6f5560911484.json
C:\Users\ahhh\AppData\Local\Wacom\UnrealMcp\Audits\20260718-165556-923-run-6b72b22a-70b9-4fa5-bb6c-333f15a25296.json
```

Read-only viewport review used session `a2082194-1dbe-48da-8c6f-16fc108d1e6e`, PID `28564`, with no writer and no PIE. It confirmed:

- world progression is `+X`; A/C are `-Y` (left), B/D are `+Y` (right);
- Junction 1 and Junction 2 BranchTargets are symmetrically staged at `Y=-208/+208`;
- Hosts are approximately 180 cm beside/behind their Anchors and stay off the white traversal paths;
- all 20 enemy PartActors have independent finite target bounds;
- six Battle, four Event, and one Shop viewpoint map one-to-one to their nodes;
- Exit label is `Host_Node_Exit_01_GRAYBOX_NO_TRAVEL`, with only root, billboard, graybox mesh, and node binding components.

`CaptureViewport` unexpectedly dirtied editor-only map state and the normal close path wrote it to disk, changing the then-current map hash from `166BB84F...63F2F` to `4654627B...F7DDDA`. This was not accepted as deliverable state. With the Editor closed, the exact task-owned untracked map path and hash were reverified, that one file was removed, and the map was replayed under a new exact seven-package writer lease without loading a viewport. Recovery evidence:

```text
MCP session: 193309b7-6970-4500-9040-03c041e2fb20
First replay: Scene Existing=1 / Created=1 / Saved=1 / Failed=0
Second replay: All Existing=7 / Created=0 / Saved=0 / Failed=0
Reports: Spec015-map-replay-first.json, Spec015-map-replay-idempotence.json
Writer audit: C:\Users\ahhh\AppData\Local\Wacom\UnrealMcp\Audits\20260718-172225-648-run-193309b7-6970-4500-9040-03c041e2fb20.json
Final map SHA-256: 352944F4FC53234CD3EE6E7A7D52BCFF76B1BF20CA7C0CE4D796E109EC579ECF
```

The final map was clean before close and retained the same hash after close. The other six hashes remained unchanged throughout recovery. No screenshot file was saved or committed.

## 9. Final validation matrix

| Check | Result | Evidence |
|---|---|---|
| Default Unity WacomEditor build | Passed | `Result: Succeeded`; build log SHA `98E3504FFFF018106196588F1D3A09A394E5DFBC20155659A349D037345E7A45` |
| Data/Editor Automation | 20/20 passed | log SHA `35EAC583DB3ABF50A14B14B37F55F0F9E20B4B9EAB042A78470FCBBA5A61A944` |
| Run scene/path Automation | 30/30 passed | log SHA `3AA709424227333B24D1C17C9228865D9D0AFBF763F06A0C5DBBAC81EADFBD38` |
| Battle Host Automation | 17/17 passed | log SHA `7E07F849CA31203532FAAD672FCEF8208E359F925FBB825EB83A729373AB1A7D` |
| 7/7 AssetRegistry/load/class | Passed | `Wacom.Editor.FormalFloor1ProductionScene.Assets.RegistryAndBlueprintCompile` |
| 5/5 Blueprint compile | Passed | four enemy prefabs + Exit marker |
| Floor 20/21 graph validation | Passed | transient and real persisted contract tests |
| Scene 1/20/21/4/16 binding validation | Passed | real persisted contract + commandlet |
| `WacomValidateRunFloorScene` | Passed | `Valid`, 0 diagnostics; log SHA `4A8C0B222222E4DDC1A1805DE1689C7F007BD7A4CA771109311A14544295A7B8` |
| 8 enemy Host slot composition | Passed | exact six Encounter slot mappings |
| Second seed 0 create/0 save | Passed | replay idempotence report SHA `B305B83927C10B308B2DA79C8E682AD9EE1E00256C8446D44DC52B301A765848` |
| Seven SHA-256 stable | Passed | final hashes below; unchanged after idempotence/close/tests |
| 46 dependencies byte-identical | Passed | count 46; ordered `Package=SHA` aggregate remains `D9308919279B3E7DC706AF26869A558808B6524B075B20A5E4E771DDFE4D6AC0` |
| Forbidden Production references | Passed | no Debug/Authoring/Test/legacy-map dependency in the real-asset contract |
| Git LFS | Passed | all 7 paths use `filter=lfs`; writer and final `git lfs fsck` report OK |
| Full Golden Path PIE | Required skip | No valid Production Journey/Floor2/3/transition runtime |

Final seven hashes:

| Asset | SHA-256 |
|---|---|
| `DA_Floor_Main_01.uasset` | `8391FCFC362D7F4B3B7168D38B3B5746917470BCF335C9089EAFEE4C4C86BA43` |
| `BP_EnemyHost_BrushSnake_Graybox.uasset` | `00AEE67043990FC2644D5437753EF0BCE5F79D4CE868A95D7E50B39E3B6595B8` |
| `BP_EnemyHost_MoltGuard_Graybox.uasset` | `E0E779190E1121FAF9E5115E5A355D3C1C0C128C92609E4BBD0A31EA5BA059BA` |
| `BP_EnemyHost_RootStalker_Graybox.uasset` | `74387C768B30553CF267976A84AE3FC8371F361E3548350860649EC40B1B6BCB` |
| `BP_EnemyHost_ShallowGuardian_Graybox.uasset` | `1ABDFFE0DAE1043DC90C04DA15B53FF3B97316A8FED3B737A8DC1A12CF0674A0` |
| `BP_WacomRunFloorEntranceMarker_Graybox.uasset` | `905193ACB8CF509FF1FDF1ECF1F110C80EA3E6BF8CAD6601D4B07A1B37B62A0D` |
| `L_Run_Floor_Main_01.umap` | `352944F4FC53234CD3EE6E7A7D52BCFF76B1BF20CA7C0CE4D796E109EC579ECF` |

## 10. Continuous risks and skips

- **PIE risk**: Local map binding tests cannot prove full Run startup or cross-floor completion without a valid Production Journey.
- **Exit risk**: FloorEntrance Host is a non-interactive graybox marker and must not be mistaken for implemented travel.
- **Art risk**: Four enemy prefabs use controlled placeholders and are not release-ready.
- **Balance risk**: This slice does not evaluate encounter/card balance.
- **Binary risk**: `.uasset/.umap` changes are non-mergeable; integration must choose this branch as authority for the seven new paths.
- **Builder risk**: Only the named seed-only command is allowed; all other builders remain forbidden.
- **Viewport tooling risk**: programmatic `CaptureViewport` dirtied and auto-saved editor-only map state in this UE build. The map was replayed and re-audited; future read-only reviews should avoid that tool until its save behavior is isolated.
- **Known unrelated warnings**: startup can report the pre-existing missing BugGirlBag illustration source; commandlet startup also reports existing PaperZD config, DreamShader generated-include, and duplicate Python enum-name warnings. None are referenced by the seven targets or caused focused failures.

## 11. Final handoff

```text
Branch: codex/formal-floor1-production-scene-baseline
Worktree: D:\UE_Project\5.7\WacomWorktrees\formal-floor1-production-scene-baseline\Wacom
Base: 9fbdb74858b9b4b705a216c8dd7ff539fe44b19e
Source checkpoint chain: 67514493, 449a544b, 7df248c8, 6fa405df, 5946cd8c, 26d14dc4
Final content commit: produced by T082 as `feat(content): seed formal floor1 production scene`; authoritative hash is reported in the integration handoff.
```

No main merge, rebase, push, shared builder, or Debug/Authoring asset mutation was performed. The final handoff must choose this branch as binary authority for the seven new paths; `.uasset/.umap` must not be text-merged.
