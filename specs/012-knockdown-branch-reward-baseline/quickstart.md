# Quickstart and Validation Ledger: 击倒分支奖励合同基线

## Workspace baseline

| Item | Result |
|---|---|
| Main/base commit | `8b5fe0db30bd0b23f7c6917a40d20082f3d9ad02` |
| Branch | `codex/knockdown-branch-reward-baseline` |
| Worktree | `D:\UE_Project\5.7\WacomWorktrees\knockdown-branch-reward-baseline\Wacom` |
| Merge-base with main | `8b5fe0db30bd0b23f7c6917a40d20082f3d9ad02` |
| Initial Git state | clean |
| Initial LFS worktree state | no staged/unstaged LFS change; `git lfs fsck` PASS on main baseline |
| UnrealEditor / Live Coding | not running |
| Local dependency hydration | independent D-drive `Content/Art` and `Content/Asset` junctions; ignored and not committed |

## Protected binary baseline

Record before implementation and compare after all Unreal commands:

```powershell
Get-FileHash -Algorithm SHA256 `
  'Content/Wacom/Data/Enemies/TrainingWarrior/DA_Part_TrainingWarrior_Body.uasset', `
  'Content/Wacom/Data/Cards/Rewards/DA_Card_BrokenCleave.uasset'
```

Expected: both hashes remain byte-identical; no `.uasset/.umap` appears in Git diff.

Baseline SHA-256:

- `DA_Part_TrainingWarrior_Body.uasset`: `0444400F1EB9038325EF2BDB081CD372D18A5C820E7A3A0974CA1C5910C6497A`
- `DA_Card_BrokenCleave.uasset`: `37AB3F6673AB08F37B95E66E9F9F825CB680FC548FAE0E76906B30F2DE31EE1A`

## Spec Kit readiness

- `check-prerequisites.ps1 -RequireTasks -IncludeTasks`: PASS; all expected design documents and `tasks.md` discovered.
- requirements checklist: `16/16` complete.
- contract-quality checklist: `16/16` complete.
- Cross-artifact review: `19/19` functional requirements and `10/10` success criteria are covered by user stories, contracts and dependency-ordered tasks; no Critical/High/Medium inconsistency, ambiguity or uncovered requirement found.
- `git diff --check`: PASS at implementation gate (only line-ending notices, no whitespace error).

## Checkpoint 1 — Data and authoring

### Compile

```powershell
& 'E:\UE_5.8\Engine\Build\BatchFiles\Build.bat' WacomEditor Win64 Development `
  -Project='D:\UE_Project\5.7\WacomWorktrees\knockdown-branch-reward-baseline\Wacom\Wacom.uproject' `
  -WaitMutex -NoHotReloadFromIDE
```

### Tests

```powershell
$Prefixes = @('Wacom.Data.EnemyPart', 'Wacom.Data.Enemy.TrainingWarrior')
foreach ($Prefix in $Prefixes) {
  & 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' `
    'D:\UE_Project\5.7\WacomWorktrees\knockdown-branch-reward-baseline\Wacom\Wacom.uproject' `
    "-ExecCmds=Automation RunTests $Prefix; Quit" `
    -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache `
    -NoDreamShaderEditorBridge -stdout -FullStdOutLogOutput
}
```

**Result**: PASS — default Unity build succeeded (`137` actions, `147.00s`). `Wacom.Data.EnemyPart` found/passed `4/4`; `Wacom.Data.Enemy.TrainingWarrior` found/passed `5/5`; both ended with Automation exit code `0`. Logs: `Spec012-Checkpoint1.log`, `Spec012-Checkpoint1-TrainingWarrior.log`.

The first attempt chained two `Automation RunTests` clauses in one `-ExecCmds`; UE accepted only the first and reported the second as an unknown nested command. The second prefix was rerun in a separate clean Editor-Cmd process. All later commands use one Automation prefix per process.

Known startup-only hydration warning: BugGirlBag references missing local dependency `/Game/Asset/Card_Luo/Card62/T_Card_Illustration_Test_Depth5_Source`. It is outside this feature and neither protected target asset reported a failed load. Final audit separates this pre-existing local hydration gap from target-asset load results.

## Checkpoint 2 — Battle settlement

```powershell
$Prefixes = @(
  'Wacom.Battle.Knockdown',
  'Wacom.Battle.KnockdownReward',
  'Wacom.Run.BattleRewardCardsAddedToBackpack',
  'Wacom.Run.NotificationCoalescing')
foreach ($Prefix in $Prefixes) {
  & 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' `
    'D:\UE_Project\5.7\WacomWorktrees\knockdown-branch-reward-baseline\Wacom\Wacom.uproject' `
    "-ExecCmds=Automation RunTests $Prefix; Quit" `
    -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache `
    -NoDreamShaderEditorBridge
}
```

**Result**: PASS after one test-only compile correction. Initial build failed because the new spec used nonexistent `TArray::CountByPredicate`; replaced with an explicit loop. The immediate rebuild succeeded (`4` actions). Focused results: `Wacom.Battle.Knockdown` `19/19`; `Wacom.Battle.KnockdownReward` `5/5`; `Wacom.Run.BattleRewardCardsAddedToBackpack` `1/1`; `Wacom.Run.NotificationCoalescing` `5/5`; every process ended with Automation exit code `0`.

## Checkpoint 3 — Passive Dialog

```powershell
$Prefixes = @(
  'Wacom.UI.Battle.KnockdownChoice',
  'Wacom.UI.Battle.PresentationQueue.KnockdownDialogDelayedAndGuarded',
  'Wacom.UI.Battle.PresentationPlan.Gained')
foreach ($Prefix in $Prefixes) {
  & 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' `
    'D:\UE_Project\5.7\WacomWorktrees\knockdown-branch-reward-baseline\Wacom\Wacom.uproject' `
    "-ExecCmds=Automation RunTests $Prefix; Quit" `
    -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache `
    -NoDreamShaderEditorBridge
}
```

**Result**: PASS — default Unity build succeeded (`18` actions). `Wacom.UI.Battle.KnockdownChoice` passed `3/3`; delayed/guarded queue passed `1/1`; gained presentation plan passed `2/2`; every process ended with Automation exit code `0`.

## Final focused suite

Run the default Unity compile again, then execute each prefix in its own clean Editor-Cmd process:

```powershell
$Prefixes = @(
  'Wacom.Data.EnemyPart',
  'Wacom.Data.Enemy.TrainingWarrior',
  'Wacom.Battle.Knockdown',
  'Wacom.Battle.KnockdownReward',
  'Wacom.Run.BattleRewardCardsAddedToBackpack',
  'Wacom.Run.NotificationCoalescing',
  'Wacom.UI.Battle.KnockdownChoice',
  'Wacom.UI.Battle.PresentationQueue.KnockdownDialogDelayedAndGuarded',
  'Wacom.UI.Battle.PresentationPlan.Gained')
foreach ($Prefix in $Prefixes) {
  & 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' `
    'D:\UE_Project\5.7\WacomWorktrees\knockdown-branch-reward-baseline\Wacom\Wacom.uproject' `
    "-ExecCmds=Automation RunTests $Prefix; Quit" `
    -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache `
    -NoDreamShaderEditorBridge
}
```

**Result**: PASS — final default Unity build succeeded (`4` actions, `6.65s`). The nine prefixes passed `4 + 5 + 19 + 5 + 1 + 5 + 3 + 1 + 2 = 45` Automation executions with process exit code `0` and no failed test. `Wacom.Battle.KnockdownReward` is intentionally rerun as a narrower prefix after its cases have also been covered by `Wacom.Battle.Knockdown`.

During the first final test launch, another worktree was running an authorized `-DisableUnity` build and held UE's global `Build.bat` mutex. The waiting Editor-Cmd had not reached Automation and was stopped without touching the external build. Remaining final commands added `-Multiprocess`, which UE 5.8 uses to skip the redundant `TargetPlatformManager` AutoSDK `ValidatePlatforms` subprocess; project loading and Automation behavior remain unchanged. Every final Unreal command retained `-NoDreamShaderEditorBridge`.

## Read-only editor audit

Requirements:

- Load/AssetRegistry inspect TrainingWarrior Part and BrokenCleave.
- Compile loaded Blueprints without saving.
- Search logs for failed load, Blueprint compile error, package save and DreamShader builder activity.
- Do not execute `WacomRegenerateContent`, `WacomBuildEnemyPack` or any builder.

**Result**: PASS with one unrelated hydration warning disclosed.

- `Wacom.Data.Enemy.TrainingWarrior.AssetContract` loaded the TrainingWarrior Part and BrokenCleave together with the expected behavior/enemy/encounter dependency set.
- `Wacom.Data.Enemy.TrainingWarrior.HostAndFormalArt` performed synchronous AssetRegistry discovery/dependency checks and passed.
- `Wacom.Editor.BattleSceneEnemyHostDebugAuthoring.ExistingBlueprintCompilesWithoutLegacyActorEntryPoints` compiled `BP_SnakeHost_Debug` successfully (`1/1`), restored its original dirty flag, and logged zero `SavePackage` / package-save lines.
- Target packages emitted no failed-load or Blueprint compile error. Editor startup did report the known local hydration gap: `DA_Card_BugGirlBag` references missing `/Game/Asset/Card_Luo/Card62/T_Card_Illustration_Test_Depth5_Source`. This dependency is unrelated to the protected target packages and exists only in another worktree's local hydration layer; it was not copied because this round forbids creating binary files.
- No builder command or DreamShader bridge was executed.

## Static/final audit

```powershell
rg -n 'Reward\.SerpentWood\.(BrushSnake|MoltGuard|RootStalker|ShallowGuardian)\.(Aid|Destroy)' `
  specs/012-knockdown-branch-reward-baseline Docs

git diff --check
git diff --name-only 8b5fe0db...HEAD
git status --short --branch
git lfs status
git lfs fsck
```

Acceptance:

- exactly 8 unique future CardIds and 4 theme roots;
- no `uasset/umap`, GameplayTag, SaveGame, Build.cs or module dependency diff;
- all Spec Kit tasks/checklists complete and cross-artifact analysis has no Critical/High/Medium issue;
- worktree clean after one independent commit.

## Skipped validation and risk

- **Builder**: skipped by scope; source is migrated for the next authorized run, assets remain legacy-only.
- **PIE**: skipped because no formal branch-specific reward assets exist. Automation covers selection, ViewData, Dialog and Run persistence; visual card art/hand feel is unchanged.
- **Full Wacom suite**: not required unless focused tests expose cross-domain regression; final scope uses the explicitly requested prefixes.
- **Binary migration**: intentionally deferred; protected hashes and Git diff prove this round did not rewrite assets.

## Result ledger

| Check | Result | Evidence / notes |
|---|---|---|
| Spec/plan/tasks/checklists | PASS | prerequisite discovery PASS; requirements `16/16`; contract-quality `16/16`; no Critical/High/Medium issue |
| Checkpoint 1 compile/tests | PASS | Unity build; EnemyPart `4/4`; TrainingWarrior `5/5` |
| Checkpoint 2 compile/tests | PASS | corrected one test-only API misuse; rebuild PASS; focused prefixes `19/19 + 5/5 + 1/1 + 5/5` |
| Checkpoint 3 compile/tests | PASS | Unity build; Dialog `3/3`; queue `1/1`; presentation plan `2/2` |
| Final Unity compile | PASS | default Unity `WacomEditor Win64 Development`; `4` actions; `6.65s` |
| Final focused Automation | PASS | nine requested prefixes; `45` executions; all process exit codes `0`; no failure |
| AssetRegistry/failed-load | PASS | TrainingWarrior `AssetContract` and synchronous `HostAndFormalArt` PASS; target packages have no failed load; unrelated BugGirl hydration warning disclosed above |
| Blueprint compile | PASS | `BP_SnakeHost_Debug` compile `1/1`; original dirty flag restored; zero package-save line |
| Protected asset SHA-256 | PASS | Part `0444400F...497A`; BrokenCleave `37AB3F66...E1A`; both identical to baseline |
| `git diff --check` / range | PASS | whitespace check PASS; 8 unique reward IDs / 4 archetype roots; zero binary, Build.cs, GameplayTag or SaveGame path |
| Git/LFS/fsck | PASS | no staged/unstaged LFS binary; `git lfs fsck OK`; post-commit clean state verified in handoff |
| Builder / PIE | SKIPPED | scope exclusions above |
