# Quickstart: Journey 成功结算与终局交接基线

## Workspace contract

- Worktree: `D:\UE_Project\5.7\WacomWorktrees\run-level-authoring-baseline\Wacom`
- Branch: `codex/run-level-authoring-baseline`
- Base: `70023f91308ab27091eacaca3723a73b3cbe6e05`
- Binary assets: prohibited
- Builder: prohibited
- Unreal commands: always include `-NoDreamShaderEditorBridge`

## Initial audit — 2026-07-17

- Branch/HEAD: PASS — `codex/run-level-authoring-baseline` @ `70023f91`.
- Git status: PASS — clean before implementation.
- Git LFS: PASS — no pending objects.
- Unreal processes: PASS — target worktree not open; one `UnrealEditor-Cmd` belongs to another worktree and does not occupy target assets.
- Source/Docs vs Spec 009: PASS — live code confirms legacy `bRunActive`, Save v4, no terminal field, no success event/state/UI; the approved plan corrects that recorded blocker.
- Spec Kit cross-artifact analysis: PASS — 18 functional requirements and 32 dependency-ordered tasks have full story coverage; no Constitution violation, ambiguity, duplicate requirement or unmapped task remains. One LOW command-template placeholder was replaced with the concrete JourneyCompletion prefix.

## Checkpoint 1 — Spec 010 and static terminal contract

Expected files: Spec Kit artifacts, Journey fields, validator/runtime initialization, focused Data test.

```powershell
& 'E:\UE_5.8\Engine\Build\BatchFiles\Build.bat' WacomEditor Win64 Development -Project='D:\UE_Project\5.7\WacomWorktrees\run-level-authoring-baseline\Wacom\Wacom.uproject' -WaitMutex -NoHotReloadFromIDE -NoDreamShaderEditorBridge
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\WacomWorktrees\run-level-authoring-baseline\Wacom\Wacom.uproject' -ExecCmds='Automation RunTests Wacom.Data.Map.Validation; Quit' -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache -NoDreamShaderEditorBridge
```

Result: PASS.

- Default Unity build: PASS, 2026-07-17. 首次执行发现 Runtime guard 被 patch 到相似的 pickup context，已移回 `Initialize()`；同时暴露基线 Run tests 的匿名 Unity helper 冲突，采用仅测试内唯一命名修复。首次长重编译的调用等待超时，但后台 Build.bat 完成；随后同命令返回 `Target is up to date / Result: Succeeded`，并再次以 4-action 增量编译确认成功。
- `Wacom.Data.Map.Validation`: PASS, 7/7 tests，包含新 `JourneySuccessTerminal` 与 `JourneySuccessTerminalRuntimeInitialization`。
- 首轮 test failure: 测试 fixture 的“partial handle”仍保留 FloorId，属于测试构造错误；清空 FloorId 后重编译与 7/7 rerun 通过，production validator 无需修改。
- Checkpoint diff hash: `6db16138dc067b60efdf2a0c37471231f1a5515e` (`git diff --binary | git hash-object --stdin`).
- Binary/asset risk: none；未修改或保存 Content package，未运行 builder。

## Checkpoint 2 — Atomic Run success settlement

```powershell
& 'E:\UE_5.8\Engine\Build\BatchFiles\Build.bat' WacomEditor Win64 Development -Project='D:\UE_Project\5.7\WacomWorktrees\run-level-authoring-baseline\Wacom\Wacom.uproject' -WaitMutex -NoHotReloadFromIDE -NoDreamShaderEditorBridge
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\WacomWorktrees\run-level-authoring-baseline\Wacom\Wacom.uproject' -ExecCmds='Automation RunTests Wacom.Run.JourneyCompletion; Quit' -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache -NoDreamShaderEditorBridge
```

Also run affected Encounter/Result/notification prefixes identified from live tests. Result: PASS.

- Default Unity build: PASS, 2026-07-17. 新增 Outcome/summary、原子终局结算、成功后写路径 guard 与小型 contract test 后完成默认 Unity 编译。编译同时暴露 `RunMapModule.cpp` 与 `RunFloorTransitionModule.cpp` 的既有 anonymous-namespace Unity helper 同名冲突；仅将前者 helper 唯一命名，不改变运行时行为。
- `Wacom.Run.JourneyCompletion`: PASS, 2/2。覆盖 terminal Victory、mutual-destruction/压力失败线上的 success 优先级、摘要、末尾事件、单次 revision/broadcast、重复战果、非终局/撤离/失败/Undetermined/stale ticket、非法摘要回滚及成功后公共写入口拒绝。
- `Wacom.Run.NodeActivity.Encounter`: PASS, 3/3。
- `Wacom.Run.Result`: PASS, 7/7；`Wacom.Run.BattleRewardCardsAddedToBackpack`: PASS, 1/1。
- `Wacom.Run.NotificationCoalescing`: PASS, 5/5。
- Checkpoint diff hash: `6a0653c1354181d01e055da41d38ec9e87e39387` (`git diff --binary | git hash-object --stdin`).
- Binary/asset risk: none；未修改 Content package，未运行 builder。

## Checkpoint 3 — SaveGame v5

```powershell
& 'E:\UE_5.8\Engine\Build\BatchFiles\Build.bat' WacomEditor Win64 Development -Project='D:\UE_Project\5.7\WacomWorktrees\run-level-authoring-baseline\Wacom\Wacom.uproject' -WaitMutex -NoHotReloadFromIDE -NoDreamShaderEditorBridge
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\WacomWorktrees\run-level-authoring-baseline\Wacom\Wacom.uproject' -ExecCmds='Automation RunTests Wacom.Run.Save; Quit' -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache -NoDreamShaderEditorBridge
```

Result: PASS.

- Default Unity build: PASS, 19 actions，UHT/schema、WacomRun、WacomTests 均编译成功。
- `Wacom.Run.Save`: PASS, 10/10。覆盖 v0→v5 连续迁移、v4 active/inactive 映射、Credential 迁移、成功摘要 deterministic roundtrip、v5 Outcome/摘要非法组合拒绝、Succeeded/Failed 终态 apply 零修改/零广播拒绝、StarterDeck rebuild 与现有 instance-zone roundtrip。
- Save system enablement: unchanged；`AWacomGameMode::bSaveSystemEnabled` 未开启，v5 仅冻结底层 schema/迁移语义。
- Checkpoint diff hash: `9b014a84d6c945a2d888f15ec251dd2cf0840c11` (`git diff --binary | git hash-object --stdin`).
- Binary/asset risk: none；SaveGame C++ schema 升级未写入任何实际 slot/package，现有 IO test slot 仍按原测试流程清理。

## Checkpoint 4 — Summary screen and handoff

```powershell
& 'E:\UE_5.8\Engine\Build\BatchFiles\Build.bat' WacomEditor Win64 Development -Project='D:\UE_Project\5.7\WacomWorktrees\run-level-authoring-baseline\Wacom\Wacom.uproject' -WaitMutex -NoHotReloadFromIDE -NoDreamShaderEditorBridge
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\WacomWorktrees\run-level-authoring-baseline\Wacom\Wacom.uproject' -ExecCmds='Automation RunTests Wacom.UI.JourneySummary; Quit' -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache -NoDreamShaderEditorBridge
```

Also run GameMode handoff, Battle return staging and input/focus prefixes. Result: PASS.

- Default Unity build: PASS。首轮编译只发现 automation view 的 `FString`/`FName` 赋值错误；改为读取 focus target 的 `GetFName()` 后 6-action 增量编译通过。
- `Wacom.UI.JourneySummary`: PASS, 4/4。覆盖只读 ViewData、原生 fallback widget tree、CommonUI focus、按钮与 ESC/Back 单次 intent、仅消费 `JourneySucceeded` event、Return-to-Run barrier 后展示、不恢复 Run hand/toast、push failure 安全交接、teardown→next-tick travel 顺序和幂等。
- `Wacom.UI.Battle.FirstPersonViewStageReturnFlow`: PASS, 4/4。
- `Wacom.UI.RunPathTraversal.BattleReturn`: PASS, 1/1；`Wacom.UI.Battle.EntryFirstPersonViewpoint.RunPath`: PASS, 4/4。
- `Wacom.UI.ExplorationHUD.GameViewportFocusLifecycle`: PASS, 1/1；`Wacom.UI.InputContext.BattleRestoresExploration`: PASS, 1/1；`Wacom.UI.GameMenu.PointerRouting.NativeButtonsReceiveSlateMouseClicks`: PASS, 1/1。
- `JourneySummaryScreenClass` 原生 C++ fallback 已编译；未创建 WBP、未新增 GameplayTag。确认与 Back 都只上报 intent，GameMode 统一 teardown 并在下一帧 travel 到 `/Game/Wacom/Maps/L_MainMenu`。
- Checkpoint diff hash: `c06a990d3ecad011f80e350e0fe94b0802cb73f1` (`git diff --binary | git hash-object --stdin`).
- Binary/asset risk: none；未修改或加载保存任何 Widget Blueprint package。

## Final audit

- Default Unity compile: PASS。命令使用 WacomEditor Win64 Development 默认 Unity 配置及 `-NoDreamShaderEditorBridge`；最终 7 actions（含新增 Production terminal 必填回归）`Result: Succeeded`。
- All affected focused tests: PASS, 14 prefixes / 51 tests。`Wacom.Data.Map.Validation` 7/7；`Wacom.Run.JourneyCompletion` 2/2；Encounter 3/3；Result 7/7；BattleReward 1/1；NotificationCoalescing 5/5；Save 10/10；JourneySummary 4/4；ReturnFlow 4/4；BattleReturn 1/1；EntryViewpoint 4/4；ViewportFocus 1/1；InputRestore 1/1；NativeMouse 1/1。每个前缀的独立日志保存在 `Saved/Logs/Spec010-Final-*.log`，均为 exit 0、零 failed 且 `TEST COMPLETE. EXIT CODE: 0`。
- Spec Kit cross-artifact analysis: PASS。先以 `SPECIFY_FEATURE=010-journey-success-settlement-baseline` 运行 prerequisite check；12 项必需工件齐全，18 条 FR 与 32 个依赖有序 task 一致，checklist 零未完成项，无 Critical/High/Medium 问题或 placeholder。
- AssetRegistry dependency audit: PASS。`WacomAuditContentDependencies -Root=/Game/Wacom/Data/Map` 扫描 4 个 Map package、遍历 47 个 `/Game` package，报告 43 个跨 Map 根依赖（现有 Debug/Authoring content graph，均有 on-disk asset）；报告写入忽略的 `Saved/Reports/Spec010JourneyAssetAudit`，未使用 `FailOnExternal`。
- Failed-load audit: PASS。只读 Python commandlet 实际加载 `/Game/Wacom/Data/Map/Authoring/DA_Journey_LevelAuthoring` 与 `/Game/Wacom/Data/Map/DA_Journey_Debug`，两者均为 `WacomJourneyDefinition`，无 Journey/package failed-load。日志仅有 UE 启动时对 `aqProf/Vtune/WinPix` 可选 profiling DLL 的常规探测 warning，与项目资产无关。一次性审计脚本已删除。
- Journey asset hashes: PASS，审计前后不变。`DA_Journey_LevelAuthoring.uasset = EA044BF14A636FC1DCB4A0D650C5B9648E52D707796126650AF7CCB917F929C8` (2470 bytes)；`DA_Journey_Debug.uasset = 7EA99E780946F531BD08155B5CF03630074101836F26ED60B3328E04D2924EA6` (2296 bytes)。
- Content/Config/Build.cs/GameplayTag diff absence: PASS。无 `.uasset/.umap`、`Content/`、`Config/`、`*.Build.cs` 或 `Source/WacomCore/Public/Tags/` 变更；未运行 builder，`AWacomGameMode::bSaveSystemEnabled` 仍为 `false`。
- Blueprint: SKIPPED by design。本轮未创建/修改 WBP 或其他 Blueprint，因此无二进制 Blueprint compile 任务；原生 C++ fallback Screen 已编译并由 `Wacom.UI.JourneySummary` 覆盖。
- `git diff --check`: PASS。Git staged changed-file audit 为 54 个文本源码/测试/文档文件；delivery diff hash 为 `7e97466b3e2b8930622f6653072e59b0086294b4`（包含新文件，排除自引用的 `quickstart.md/tasks.md` ledger）。
- Git/LFS clean after commit: 由最终提交后 `git status` / `git lfs status` 复核；本记录不在提交后反向修改自身。

## Explicit skip and risk

- **Actual Floor 3 PIE**: SKIPPED by design. No Production Journey/Floor3 DataAsset or terminal scene exists. Automation covers terminal rules, atomic transaction, persistence and App handoff; the Production asset round must run the real Golden Path PIE.
- **Builder / Blueprint asset compile**: Builder is prohibited and no WBP/uasset is created. Native C++ fallback is compiled and automation-tested; existing assets receive a read-only failed-load audit only.
- **Remaining runtime risk**: 当前没有 Production Journey/Floor 3 DataAsset、终局场景或 WBP 表现，因此本轮只能证明规则、Save schema、原生 fallback 与 App handoff 合同；生产资产轮必须再做真实 Floor 3 Golden Path PIE。Defeat/压力满/手指耗尽的统一总结交接仍是明确后续，本轮不伪造。
