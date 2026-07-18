# Quickstart and Validation Ledger: Floor 1 Production 46 DataAsset 播种与校验

## Workspace baseline — 2026-07-18

| Item | Result |
|---|---|
| Original MCP baseline | `b4604e929a03d80baa0a9aa9cef7a325601984fa` |
| Actual implementation base | `d7c6b70b1dc1006f08fff8c598d58e65f53a5813` |
| Base delta | one unrelated `fix(backpack): preserve release presentation`; audited, no Floor1 content overlap |
| Branch | `codex/formal-floor1-production-assets` |
| Worktree | `D:\UE_Project\5.7\WacomWorktrees\formal-floor1-production-assets\Wacom` |
| Initial branch/merge-base | both equal actual base |
| Initial Git status | clean |
| Initial Git LFS | no staged/unstaged change; `git lfs fsck` PASS |
| UnrealEditor / Live Coding | not running |
| Target Production roots | absent; 0 target assets |
| Worktree Binaries | absent |
| Local dependency hydration | `Content/Art`、`Content/Asset` use independent D-drive copies under `D:\UE_Project\5.7\WacomWorktreeData\formal-floor1-production-assets`; junctions are ignored and not commit content |

## Live truth audit

Read before planning:

- `AGENTS.md`、`Docs/AgentIntegrationWorkflow.md`、`Docs/UnrealMCPWorkflow.md`；
- Data/Battle/Run/Architecture/TODO/Questions/Roadmap long-term docs；
- Spec 011/012/013 relevant complete artifacts and contracts；
- live DataAsset headers, GameplayTags, shared validators, ContentBuilderHelpers, Snake/TrainingWarrior patterns, commandlets and focused tests；
- `.specify/memory/constitution.md` and disabled extension hooks.

Findings:

- PASS — all 46 target roots/packages are absent, so initial seed is authorized and unambiguous.
- PASS — four existing Wayfarer card dependencies are separate read-only packages.
- PASS — current schema/validators express all content without source/schema/tag changes.
- PASS — FormalProduction profile already requires Aid/Destroy and legacy null.
- PASS — shared `BuildDataAsset()` updates existing assets and therefore must not be used for seed-only ownership.
- PASS — new Unreal MCP run endpoint is role `run`, endpoint `ue_wacom_run`, port `8140`, writer-eligible.
- PASS — no named Unreal tools were loaded in this long-running task before Editor start; after start use a disposable asset agent for that Editor lifecycle if necessary.
- CORRECTED — Spec 013 card-manifest row for Guardian Destroy omitted TargetMode; authority is `AllEnemyParts` in live Docs/data-model.

### Spec Kit cross-artifact analysis — 2026-07-18

- PASS — prerequisites resolve to Spec 014 and expose `research.md`、`data-model.md`、`contracts/`、`quickstart.md` and `tasks.md`.
- PASS — 25 functional requirements, 10 success criteria and 62 dependency-ordered tasks have implementation or validation coverage.
- PASS — manifest table contains exactly 46 writable rows; class distribution and `12/19/15` groups agree across spec, plan, tasks and contracts.
- PASS — both checklists contain 48 requirement-quality assertions; checked state means design coverage, not runtime completion.
- PASS — no Critical/High/Medium contradiction, ambiguous authority, uncovered requirement or scope leak remains.
- CLARIFIED — Editor console options accept bare or single-leading-dash forms and normalize to the commandlet grammar.
- `git diff --check`: PASS at the design gate.

## Fixed identity

```text
Role: run
Endpoint: ue_wacom_run
Port: 8140
ThreadId: 019f69c0-56e1-7280-b959-6b796da49af0
ExpectedBranch: codex/formal-floor1-production-assets
ProjectRoot: D:\UE_Project\5.7\WacomWorktrees\formal-floor1-production-assets\Wacom
```

## Commands

### Spec Kit

```powershell
$env:SPECIFY_FEATURE='014-formal-floor1-production-assets'
& '.specify/scripts/powershell/check-prerequisites.ps1' -Json -RequireTasks -IncludeTasks
```

Run read-only cross-artifact analysis after tasks are final. Extension hooks are installed but disabled; agent-context is updated explicitly after plan.

### Build gate

```powershell
$ProjectRoot='D:\UE_Project\5.7\WacomWorktrees\formal-floor1-production-assets\Wacom'
$Branch='codex/formal-floor1-production-assets'
$Mcp=Join-Path $ProjectRoot 'Scripts\Invoke-WacomUnrealMcp.ps1'

& $Mcp -Action AssertClosedForBuild -Role run `
  -ProjectRoot $ProjectRoot -ExpectedBranch $Branch

& 'E:\UE_5.8\Engine\Build\BatchFiles\Build.bat' `
  WacomEditor Win64 Development -Project="$ProjectRoot\Wacom.uproject" `
  -WaitMutex -NoHotReloadFromIDE
```

### Start/ready

```powershell
& $Mcp -Action Start -Role run -ProjectRoot $ProjectRoot -ExpectedBranch $Branch
& $Mcp -Action AssertReady -Role run -ProjectRoot $ProjectRoot -ExpectedBranch $Branch
```

每次 MCP call 前重复 AssertReady。Editor 生命周期中不切 branch、更新 HEAD 或编译。

### Commandlet inspect

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' `
  "$ProjectRoot\Wacom.uproject" `
  -run=WacomBuildFormalFloor1Content -Group=All -CompareSeedDefaults `
  -Report="$ProjectRoot\Saved\FormalFloor1\inspect-all.json" `
  -Unattended -NoPause -NoSplash -NullRHI -NoDreamShaderEditorBridge
```

### MCP writer sequence

完整 46-package 表见 [asset-manifest.md](./contracts/asset-manifest.md)。每组 lease 只使用对应 package 子集：

```text
AssertReady
AcquireWriter(ThreadId, group packages)
MCP invokes: Wacom.BuildFormalFloor1Content -SeedMissing -Group=<Group> -CompareSeedDefaults -Report=<Saved JSON>
git status --short
ReleaseWriter
record audit JSON + package hashes + LFS
```

Cards、EnemyGraph、NodeDefinitions 串行；不共用一个跨三组的 writer lease。

## Checkpoint ledger

### Checkpoint 1 — Spec/manifest/transient tests

- Status: PASS — 2026-07-18
- `AssertClosedForBuild`: PASS; exact branch/project/HEAD `d7c6b70b1dc1006f08fff8c598d58e65f53a5813`
- WacomEditor compile: PASS — default Unity `Win64 Development`; `UnrealEditor-WacomEditor.dll` SHA-256 `6DB861C67AC7E1B88BCD412EDFFC232392F41712D923ECF34BDAE8FBB2B07002`; `UnrealEditor-WacomTests.dll` SHA-256 `0B218E7FDAD577BFCE9BB1B0A8491BD127AFBB90719B4BC4F07A72847AA595FE`
- `Wacom.Editor.FormalFloor1Content.Manifest`: 3 discovered / 3 passed / 0 failed
- Existing schema/rule prefixes: 39 discovered / 39 passed / 0 failed (`Card 4`, `EnemyPart 4`, `Behavior 5`, `Encounter 4`, `RunEvent 5`, `Shop 2`, `Pickup 9`, `RuleContentMatrix 6`)
- Inspect-only absent-root commandlet: expected exit `1/Validation`; manifest 46, selected 46, missing 46, created/saved/failed 0; report `Saved/FormalFloor1Content/inspect-absent.json`
- Real-asset spec: source compiled now to obey the no-C++-edit-during-Editor rule; intentionally not executed until the 46 assets exist
- Binary delta: 0 `.uasset/.umap`; all seven target roots still contain 0 files
- `git diff --check`: PASS; `git lfs status`: no binary change; `git lfs fsck`: PASS
- Commit: `dd5ba14b6c19cfdde27655fc4b5264d6f482fe6b feat(editor): add formal floor1 content seeder`

### Checkpoint 2 — Cards 12

- AssertReady/session: PASS — role `run`, endpoint `ue_wacom_run:8140`, SessionId `ac1fdb62-d1d2-4741-904f-c95f2909fd26`, PID `21772`, exact project/branch/source commit.
- Writer audit: PASS — exact 12-package allowlist; audit `C:\Users\ahhh\AppData\Local\Wacom\UnrealMcp\Audits\20260718-100828-594-run-ac1fdb62-d1d2-4741-904f-c95f2909fd26.json`.
- Created/saved: 12/12, failed 0, exit 0; `Saved/FormalFloor1Content/cards-seed.json`.
- Strict/structural: 12 existing, 0 missing/failed/saved; `Saved/FormalFloor1Content/cards-inspect.json`; real Card validators passed.
- Second run: PASS — 12 existing, 0 created/missing/failed/saved; `cards-second-run.json`; audit `C:\Users\ahhh\AppData\Local\Wacom\UnrealMcp\Audits\20260718-105202-391-run-8c55aecb-fb2f-4e44-85ea-bfb06a10c22b.json`.
- Hash/LFS: exact 12 new `.uasset`, no other package; per-file SHA-256 captured by writer audit and included in the final 46-file aggregate.

### Checkpoint 3 — EnemyGraph 19

- Writer audit: PASS — exact 19-package allowlist; audit `C:\Users\ahhh\AppData\Local\Wacom\UnrealMcp\Audits\20260718-101147-401-run-ac1fdb62-d1d2-4741-904f-c95f2909fd26.json`.
- Created/saved: 19/19, failed 0, exit 0; `Saved/FormalFloor1Content/enemy-graph-seed.json`.
- FormalProduction 11/11: PASS — every Part has explicit same-archetype Aid/Destroy and null legacy.
- Intent/Enemy graph: PASS — 4 Sequence Behavior, 11 Part, 4 Enemy, 24 ordered Intents; strict inspect reports 19 existing and 0 saved in `enemy-graph-inspect.json`.
- Second run/hash/LFS: PASS — 19 existing, 0 created/missing/failed/saved; `enemy-graph-second-run.json`; aggregate hash unchanged; audit `C:\Users\ahhh\AppData\Local\Wacom\UnrealMcp\Audits\20260718-105425-002-run-8c55aecb-fb2f-4e44-85ea-bfb06a10c22b.json`.

### Checkpoint 4 — NodeDefinitions 15

- Read-only dependency hashes: recorded before mutation and unchanged afterward:
  - ChitinWard `57E48E2539BCEB40CA764D0E67BBEE7DF7FE14E7902AAD3E94FC2A1B150481D8`
  - AntennaSearch `C7D8EF3F696E6480C24ECA7F7BA59E891DAB8C9BA37C69CB19597B7091A28E56`
  - MoltCut `BE8D1887A32692CD82F55C696F5E80E02BB5798568D331EDBDFF7B082334369F`
  - PoisonFang `1CFA5710FDB7EF1004E20E9AF3141CCF9DB76D5848383A48120DB2D7105711EA`
- Writer audit: PASS — exact 15-package allowlist; audit `C:\Users\ahhh\AppData\Local\Wacom\UnrealMcp\Audits\20260718-101456-262-run-ac1fdb62-d1d2-4741-904f-c95f2909fd26.json`.
- Created/saved: 15/15, failed 0, exit 0; `Saved/FormalFloor1Content/node-definitions-seed.json`.
- Encounter/Event/Pickup/Shop: PASS — 6/4/4/1 exact definitions; strict inspect reports 15 existing and 0 saved in `node-definitions-inspect.json`.
- Second run/hash/LFS: PASS — 15 existing, 0 created/missing/failed/saved; `node-definitions-second-run.json`; aggregate hash unchanged; audit `C:\Users\ahhh\AppData\Local\Wacom\UnrealMcp\Audits\20260718-105638-922-run-8c55aecb-fb2f-4e44-85ea-bfb06a10c22b.json`.

### Final validation

- First Editor lifecycle closed normally; all writer leases released; `AssertClosedForBuild`: PASS.
- Default Unity WacomEditor compile: PASS after close, target up to date; DLL hashes remain Editor `6DB861C67AC7E1B88BCD412EDFFC232392F41712D923ECF34BDAE8FBB2B07002`, Tests `0B218E7FDAD577BFCE9BB1B0A8491BD127AFBB90719B4BC4F07A72847AA595FE`.
- Focused Automation: 101/101 passed, 0 failed. Breakdown: exact Data/schema 35, real assets 1, Editor/Battle/Run affected paths 65. Live shop coverage uses `Wacom.Run.Deck.Shop`、`Wacom.Run.NodeActivity.Shop`、`Wacom.Run.VisitOwnership.Shop` and `Wacom.Run.SnapshotRevisions.ShopMutationPaths`; the planned alias `Wacom.Run.Shop` discovers 0 and is not counted.
- Auxiliary broad `Wacom.Data`: 71/72 passed. The sole failure is the pre-existing out-of-scope `Wacom.Data.BattleStarterContent.StarterPackAssetValidation`, whose recursive BugGirl scan sees existing PoisonNeedle/Test/BadgeDisplayTests content; all Floor 1 and affected Data tests passed and this task does not rewrite unrelated assets/tests.
- AssetRegistry 46/46 + failed-load: PASS; `Saved/FormalFloor1Content/asset-registry-audit.json` reports exact class counts `12 Card / 4 Behavior / 11 Part / 4 Enemy / 6 Encounter / 4 Event / 4 Pickup / 1 Shop`, no missing/extra/duplicate/wrong-class/failed-load.
- Forbidden reference closure: PASS — no Debug/Authoring/Test/BadgeDisplayTests/TrainingWarrior package path in the 46-asset closure.
- 46-file SHA-256 sorted-manifest aggregate: `45AD16A43A6E1D393301DF33B44014410829C4022A985BB87AC3772E16389F26`; individual first-seed hashes are retained in the three writer audit JSON files.
- Four read-only dependency hashes unchanged: PASS, values recorded in Checkpoint 4.
- Second Editor lifecycle: PASS — exact committed HEAD `9ef1e163976db3411c16b733e6289e5b7e1cbc6a`, SessionId `8c55aecb-fb2f-4e44-85ea-bfb06a10c22b`, PID `19968`; all three exact writer leases released with zero new dirty paths, Editor closed normally without save prompt, final `AssertClosedForBuild` passed.
- Spec Kit consistency recheck: PASS — 25 FR、10 SC、62 dependency-ordered tasks remain aligned; no Critical/High/Medium contradiction, uncovered requirement or scope leak.
- `git diff --check` / staged pointer check / forbidden-scope audit: PASS — 46 `.uasset`, 12 Docs/Spec files, 0 `.umap`, 0 white-list violation; all 46 index blobs are LFS pointers.
- Git LFS status/fsck: PASS — 46 objects staged as LFS; `git lfs fsck OK`.
- Commit 1: `dd5ba14b6c19cfdde27655fc4b5264d6f482fe6b feat(editor): add formal floor1 content seeder`.
- Commit 2: `feat(content): seed formal floor1 production assets` (final hash is reported by the integration handoff after the ledger-only amend).
- Final clean worktree/LFS: PASS after final amendment and verification.

## Explicit skips and remaining risk

- **Builder exclusions**: `WacomRegenerateContent`、Snake、TrainingWarrior、Debug map、DreamShader、card/material/backpack builders are prohibited.
- **Blueprint compile**: skipped because no Blueprint changes.
- **PIE**: skipped because no Production Journey/Floor/map/Host exists; real Golden Path remains a later scene-authoring gate.
- **Balance/art/readability**: not claimed. Seed values implement frozen contracts, but cost/rarity/magnitude/HP/shop/event numbers and text are intentionally tunable after seed.
- **Backpack volume**: Floor 1 may yield 14–17/20 reward cards; overflow product decision remains open.
- **World authority**: 46 DataAsset completion does not choose or modify the Production map authority.
- **MCP workflow defect**: `AcquireWriter` currently rejects a completely clean worktree because its internal empty `RelativePaths` argument is mandatory. This run used a temporary untracked `.spec014-writer-baseline` with recorded hash only to establish the dirty baseline; it was deleted after release and is not delivery content. The writer still enforced every exact package allowlist and fail-closed audit. Shared workflow scripts were not modified in this feature.

## Final result ledger

- First seed: exact `12 + 19 + 15 = 46` created/saved, 0 failed; no `.umap` and no out-of-allowlist asset.
- All strict inspect after first seed: 46 existing, 0 created/missing/failed/saved; `Saved/FormalFloor1Content/all-inspect.json`.
- Second-run acceptance from the committed content state: Cards 12、EnemyGraph 19、NodeDefinitions 15 all report `Created=0 / Failed=0 / Saved=0`; no package changed and the sorted 46-file aggregate stayed `45AD16A43A6E1D393301DF33B44014410829C4022A985BB87AC3772E16389F26`.
- Delivery contains two commits only, 46 LFS `.uasset`, no `.umap`, no runtime/schema/tag/module change, no merge and no push.
