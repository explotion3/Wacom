# Quickstart and Validation Ledger: Floor 2 Production 47 DataAsset 播种与校验

## Workspace baseline — 2026-07-19

| Item | Result |
|---|---|
| Base | `8e54505e7c28b3721b5155d642de9933f380f170` |
| Branch | `codex/formal-floor2-production-assets` |
| Worktree | `D:\UE_Project\5.7\WacomWorktrees\formal-floor1-production-scene-baseline\Wacom` |
| Initial merge-base/HEAD | both `8e54505e` |
| Initial Git/LFS | clean; `git lfs fsck` PASS |
| Initial UnrealEditor/LiveCoding | target worktree not running |
| MoltCavern roots | all seven absent; 0 target assets |
| Commit gate | no stage/commit until user explicitly accepts final review packet |

## Fixed MCP identity

```text
Role: run
Endpoint: ue_wacom_run
Port: 8140
ThreadId: 019f69c0-56e1-7280-b959-6b796da49af0
ExpectedBranch: codex/formal-floor2-production-assets
ProjectRoot: D:\UE_Project\5.7\WacomWorktrees\formal-floor1-production-scene-baseline\Wacom
```

## Build gate

```powershell
$ProjectRoot='D:\UE_Project\5.7\WacomWorktrees\formal-floor1-production-scene-baseline\Wacom'
$Branch='codex/formal-floor2-production-assets'
$Mcp=Join-Path $ProjectRoot 'Scripts\Invoke-WacomUnrealMcp.ps1'

& $Mcp -Action AssertClosedForBuild -Role run `
  -ProjectRoot $ProjectRoot -ExpectedBranch $Branch

& 'E:\UE_5.8\Engine\Build\BatchFiles\Build.bat' `
  WacomEditor Win64 Development -Project="$ProjectRoot\Wacom.uproject" `
  -WaitMutex -NoHotReloadFromIDE -NoDreamShaderEditorBridge
```

## Focused automation

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' `
  "$ProjectRoot\Wacom.uproject" `
  -ExecCmds='Automation RunTests <PREFIX>; Quit' `
  -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache `
  -NoDreamShaderEditorBridge
```

Primary prefixes:

```text
Wacom.Editor.FormalProductionContentSeedService
Wacom.Editor.FormalFloor1Content.Manifest
Wacom.Data.FormalFloor1Content
Wacom.Editor.FormalFloor2Content.Manifest
Wacom.Data.FormalFloor2Content
Wacom.Data.Card.Validation
Wacom.Data.EnemyPart.Validation
Wacom.Data.Enemy.Validation
Wacom.Data.EnemyValidation.Behavior
Wacom.Data.Validation.EncounterDefinition
Wacom.Data.RunEvent.Validation
Wacom.Data.RunPickup.Validation
Wacom.Data.Shop.Validation
Wacom.Battle.RuleContentMatrix
Wacom.Battle.KnockdownReward
Wacom.Run.BattleRewardCardsAddedToBackpack
Wacom.Run.Deck.Shop
Wacom.Run.NodeActivity.Shop
Wacom.Run.VisitOwnership.Shop
Wacom.Run.Event
Wacom.Run.Pickup
Wacom.Run.NotificationCoalescing
```

## Inspect commandlet

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' `
  "$ProjectRoot\Wacom.uproject" `
  -run=WacomBuildFormalFloor2Content -Group=All -CompareSeedDefaults `
  -Report="$ProjectRoot\Saved\FormalFloor2Content\inspect-all.json" `
  -Unattended -NoPause -NoSplash -NullRHI -NoDreamShaderEditorBridge
```

Before assets exist expected exit is 1 with `47 missing / 0 created / 0 saved`.

## MCP lifecycle and per-group command

```powershell
& $Mcp -Action Start -Role run -ProjectRoot $ProjectRoot `
  -ExpectedBranch $Branch -AllowDirty
& $Mcp -Action AssertReady -Role run -ProjectRoot $ProjectRoot `
  -ExpectedBranch $Branch
```

For each exact group package list in [asset-manifest.md](contracts/asset-manifest.md):

```text
AssertReady
AcquireWriter(ThreadId, exact group packages)
Editor console:
  Wacom.BuildFormalFloor2Content SeedMissing Group=<Group> CompareSeedDefaults Report=Saved/FormalFloor2Content/<group>-seed.json
Editor console inspect:
  Wacom.BuildFormalFloor2Content Group=<Group> CompareSeedDefaults Report=Saved/FormalFloor2Content/<group>-inspect.json
git status / SHA-256 / LFS audit
ReleaseWriter
```

## Checkpoint ledger

### Checkpoint 1 — Spec and shared core

- Status: PASS (2026-07-19)
- Spec Kit: feature/plan/task setup used explicit Spec 018 path; `check-prerequisites.ps1` rejects the repository-required `codex/...` branch naming pattern, so cross-artifact checks were completed directly against this directory. No content inconsistency found.
- Floor 1 46-file pre/post aggregate: `E4679723E6DF499CB40E8468591BC76009B0F6D40C8BC9C603457EC7D3A229C9` / unchanged.
- Read-only dependency SHA-256 baseline:
  - HerbalPoultice: `CA2C7E7008C8EFE04988B33B342D8E4CE73836B8905DD661122EE381F521801C`
  - Starter ChitinWard: `57E48E2539BCEB40CA764D0E67BBEE7DF7FE14E7902AAD3E94FC2A1B150481D8`
  - Starter MoltCut: `BE8D1887A32692CD82F55C696F5E80E02BB5798568D331EDBDFF7B082334369F`
- Compile: default Unity `WacomEditor Win64 Development` PASS, 56 actions, 52.3 s; `AssertClosedForBuild` verified role/worktree/branch/HEAD first.
- Tests: shared service `2/2`, Floor 1 manifest `3/3`, Floor 1 persisted strict asset `1/1`, all PASS.
- A first over-broad `Wacom.Editor.Formal` launch also selected the known Floor 1 Production Scene closure test and failed only on the accepted BugGirl test-card dependency. It did not affect the exact reruns or modify assets.
- Binary delta: 0; Floor 1 aggregate and Git status unchanged.

### Checkpoint 2 — Floor 2 profile/transient tests

- Status: PASS (2026-07-19)
- Compile: default Unity `WacomEditor Win64 Development` PASS, 12 actions, 9.5 s.
- `Wacom.Editor.FormalFloor2Content.Manifest`: `4/4` PASS (arguments, counts/identities, transient defaults/comparator, read-only missing inspect).
- Shared service rerun `2/2` PASS; Floor 1 manifest rerun `3/3` PASS.
- Empty-root inspect: `47 selected / 47 missing / 0 created / 0 saved / 0 failed`, expected validation exit 1; JSON `Saved/Automation/Spec018-empty-inspect.json` written.
- MoltCavern binary count after inspect: 0. Floor 1 aggregate and all three read-only dependency hashes unchanged.
- Binary delta: 0.

### Checkpoint 3 — Cards 12

- MCP session: `f0ce880a-32f7-46fc-867e-731a7d53bb40`, role `run`, endpoint `ue_wacom_run:8140`, branch/HEAD `codex/formal-floor2-production-assets@8e54505e`.
- Exact 12-package writer lease released successfully. Audit: `C:\Users\ahhh\AppData\Local\Wacom\UnrealMcp\Audits\20260719-140725-899-run-f0ce880a-32f7-46fc-867e-731a7d53bb40.json`.
- Seed report `cards-seed.json`: `12 created / 12 saved / 0 failed`; strict report `cards-inspect.json`: `12 existing / 0 created / 0 saved / 0 failed`.
- 12 files exist under Cards/Rewards and Cards/Run MoltCavern roots; every file has `filter=lfs`. Per-file SHA-256 is frozen in the writer audit.
- The first Editor lifecycle ended after the Cards slice. A manually reopened, unregistered Editor was detected and rejected; the user closed it before a new MCP-managed session was created. No writer was held and no asset was lost.

### Checkpoint 4 — EnemyGraph 20

- MCP session: `0fe614c3-2d06-484d-9d9e-022f17c593c8`, Editor PID `26488`, exact worktree/branch/HEAD verified before the lease.
- Exact 20-package writer lease released successfully. Audit: `C:\Users\ahhh\AppData\Local\Wacom\UnrealMcp\Audits\20260719-151812-220-run-0fe614c3-2d06-484d-9d9e-022f17c593c8.json`.
- Seed report `enemygraph-seed.json`: `20 created / 20 saved / 0 failed`; strict report `enemygraph-inspect.json`: `20 existing / 0 created / 0 saved / 0 failed`.
- Real assets contain `4 Enemy / 4 Behavior / 12 Part / 26 Intent`; all 12 Parts use explicit Aid+Destroy and clear legacy. All 20 files are LFS; hashes are frozen in the writer audit.

### Checkpoint 5 — NodeDefinitions 15

- Reused verified MCP session `0fe614c3-2d06-484d-9d9e-022f17c593c8` only after releasing EnemyGraph and rerunning `AssertReady`.
- Exact 15-package writer lease released successfully. Audit: `C:\Users\ahhh\AppData\Local\Wacom\UnrealMcp\Audits\20260719-152816-445-run-0fe614c3-2d06-484d-9d9e-022f17c593c8.json`.
- Seed report `nodedefinitions-seed.json`: `15 created / 15 saved / 0 failed`; strict report `nodedefinitions-inspect.json`: `15 existing / 0 created / 0 saved / 0 failed`.
- Real assets contain `7 Encounter / 3 Event / 4 Pickup / 1 Shop` and `10 Event Choice`. All 15 files are LFS.
- HerbalPoultice, Starter ChitinWard and Starter MoltCut retained their baseline hashes exactly; none entered the writer allowlist.

### Final validation

- Editor closed / `AssertClosedForBuild`: PASS for role/worktree/branch/HEAD.
- Default Unity compile: PASS after asset close and PASS again after the synthetic missing test and AssetRegistry closure additions. Final compile rebuilt 16 actions successfully.
- Shared-core cleanup: removed the superseded, unexported Floor 1 legacy execution body. `AssertClosedForBuild` then passed and the default Unity build rebuilt 4 actions successfully; exact shared service/Floor 1/Floor 2 regression reruns passed `2 + 3 + 1 + 4 + 1 = 11/11`.
- Focused Automation: `113/113` final selected cases PASS: shared/manifest/persisted `11`, Data schema `35`, Battle/Run smoke `67`.
- A post-seed manifest rerun exposed that the first missing-inspect test depended on the real directory remaining empty. It was corrected to use a synthetic, non-created `/Game/Wacom/Data/Spec018SyntheticMissing` manifest. The formal data-root guard remained active; final Floor 2 manifest is `4/4` PASS and no synthetic package/directory exists.
- AssetRegistry/failed-load/class/count/stable-reference/strict-default and allowlist dependency closure: PASS. The closure permits only the 47 manifest packages and three declared read-only cards; no Debug, Authoring, Test, BadgeDisplayTests, TrainingWarrior, Character, map, Host, UI or material dependency was found.
- Final real commandlet inspect: `47 existing / 0 created / 0 saved / 0 failed`, exit 0, report `all-final-inspect.json`.
- 47-file aggregate SHA-256 after seed and after all final tests: `24487EEE6A097D1CAA68F7326A91CBCC9E3592AF832FF0386A86F9AFD32C94CF`; unchanged.
- Floor 1: no SerpentWood binary appears in Git status and the exact persisted Floor 1 strict test passes; the checkpoint aggregate remained unchanged. Three declared read-only card hashes also match their baselines exactly.
- Git/LFS/range: `git diff --check` PASS, `git lfs fsck` PASS, all 47 files resolve to `filter=lfs`, zero untracked Data assets outside the seven MoltCavern roots, zero `.umap`/Blueprint/Build.cs/GameplayTag/SaveGame change, zero staged path.
- Spec Kit: 20 FR and 8 SC remain aligned with plan/tasks/contracts; both checklists have zero open item. Repository branch-name tooling still rejects the required `codex/...` name, so final analysis used the explicit Spec 018 directory. Pointer and identity audits pass.
- User review: accepted on 2026-07-20; the user explicitly authorized the planned two-commit delivery.
- Delivery: `feat(editor): generalize formal production content seeding` followed by `feat(content): seed formal floor2 production assets`; no merge or push.

### Latest main enemy-component refactor compatibility audit

- `main` advanced from the feature base to `26079c10301c5d17522d82ec6cd7b393e46762c6` with the component-native Scene Enemy refactor. This feature branch did not merge or rebase it.
- `UEnemyDefinition`, `UEnemyPartDefinition`, `UEnemyBehaviorDefinition`, and `UEncounterDefinition` have identical Git blobs at the feature base and `main@26079c10`; the 47 rule DataAssets require no migration or resave.
- The Floor 2 dependency closure contains no Host, UI, map, material, animation-style, or other `WacomApp` presentation asset. Source and binary paths have no overlap with the enemy refactor. Five overlapping long-term Docs were checked with a read-only three-way merge simulation and produced zero conflict blocks.
- Future Floor 2 Host work must use `AWacomBattleEnemyActor` with typed `UWacomBattleEnemyPartComponent` children, direct visual/impact child components, and one `UWacomBattleEnemySceneRuntimeComponent`; the removed PartActor/ChildActor/Bridge/Presentation architecture is forbidden. Optional PartAnimationStyle assets remain outside this 47-asset rules slice.
- Integration must retain the current `main` enemy-component implementation and rerun the Floor 2 formal content tests together with `Wacom.Editor.EnemyScene.ComponentAuthoring`, `Wacom.Editor.EnemyScene.LegacyAudit`, `Wacom.UI.Battle.EnemyScene.ComponentRuntime`, `Wacom.UI.Battle.EnemyScene.RuntimePerformance`, `Wacom.UI.Battle.EnemyScene.FlipbookPlayback`, the TrainingWarrior/Snake/SlimeTrio data prefixes, and `Wacom.Editor.FormalFloor1ProductionScene.Assets`.

## Explicit skips and risks

- Blueprint compile: skipped; no Blueprint changes.
- PIE: skipped; no Floor 2 Floor/map/Host/Journey exists.
- Full builders: prohibited.
- Balance/art/backpack volume: 17–18/24 branch cards require later gameplay/balance acceptance.
- BugGirl StarterDeck pollution: known external issue; not changed or hidden.
- Asset LFS note: files are intentionally unstaged pending user review, so LFS ownership is evidenced by `.gitattributes` (`filter=lfs`) and writer audits; pointer materialization occurs only when the approved asset commit is staged.
