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
- Commit: PENDING

### Checkpoint 2 — Cards 12

- AssertReady/session: PENDING
- Writer audit: PENDING
- Created/saved: expected 12/12
- Strict/structural: PENDING
- Second run: expected 0/0
- Hash/LFS: PENDING

### Checkpoint 3 — EnemyGraph 19

- Writer audit: PENDING
- Created/saved: expected 19/19
- FormalProduction 11/11: PENDING
- Intent/Enemy graph: PENDING
- Second run/hash/LFS: PENDING

### Checkpoint 4 — NodeDefinitions 15

- Read-only dependency hashes: PENDING
- Writer audit: PENDING
- Created/saved: expected 15/15
- Encounter/Event/Pickup/Shop: PENDING
- Second run/hash/LFS: PENDING

### Final validation

- Editor closed / `AssertClosedForBuild`: PENDING
- Default Unity WacomEditor compile: PENDING
- Focused Automation discovery/count/pass: PENDING
- AssetRegistry 46/46 + failed-load: PENDING
- Forbidden reference closure: PENDING
- 46 file SHA-256 manifest: PENDING
- Four read-only dependency hash unchanged: PENDING
- `git diff --check`: PENDING
- Git LFS status/fsck: PENDING
- Commit 1 / Commit 2: PENDING
- Final clean worktree: PENDING

## Explicit skips and remaining risk

- **Builder exclusions**: `WacomRegenerateContent`、Snake、TrainingWarrior、Debug map、DreamShader、card/material/backpack builders are prohibited.
- **Blueprint compile**: skipped because no Blueprint changes.
- **PIE**: skipped because no Production Journey/Floor/map/Host exists; real Golden Path remains a later scene-authoring gate.
- **Balance/art/readability**: not claimed. Seed values implement frozen contracts, but cost/rarity/magnitude/HP/shop/event numbers and text are intentionally tunable after seed.
- **Backpack volume**: Floor 1 may yield 14–17/20 reward cards; overflow product decision remains open.
- **World authority**: 46 DataAsset completion does not choose or modify the Production map authority.

## Final result ledger

To be filled continuously after each checkpoint. A skipped or failed item must include reason, exact command/report and remaining risk; no result is inferred from an earlier branch.
