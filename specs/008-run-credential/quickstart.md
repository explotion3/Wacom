# Quickstart: Run 持久任务凭证

## Workspace

- Worktree: `D:\UE_Project\5.7\WacomWorktrees\run-level-authoring-baseline\Wacom`
- Branch: `codex/run-level-authoring-baseline`
- Base commit: `b13672b9e1ee36cc6022e9ed5378cebcde7ab278`
- Start state: Git tracked changes only contained the new Spec 008 pointer; LFS clean；无 Unreal Editor / ShaderCompileWorker 进程。
- Stable Credential: `Credential.Run.SerpentSigil`
- Migration: v3→v4 initializes an empty Credential collection without card inference

## Safety boundaries

- 不运行 builder。
- 不创建或保存 `.uasset/.umap`。
- 不修改 GameplayTag、Build.cs、模块依赖、Battle contract 或 SaveGame 总开关。
- 所有 Unreal 命令加 `-NoDreamShaderEditorBridge`。

## Compile command

```powershell
& 'E:\UE_5.8\Engine\Build\BatchFiles\Build.bat' WacomEditor Win64 Development -Project='D:\UE_Project\5.7\WacomWorktrees\run-level-authoring-baseline\Wacom\Wacom.uproject' -WaitMutex -NoHotReloadFromIDE -NoDreamShaderEditorBridge
```

## Automation command template

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\WacomWorktrees\run-level-authoring-baseline\Wacom\Wacom.uproject' -ExecCmds='Automation RunTests <PREFIX>; Quit' -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache -NoDreamShaderEditorBridge
```

## Checkpoint log

| Checkpoint | Compile | Focused tests | Result / log | Notes |
|---|---|---|---|---|
| CP0 Spec/design | N/A | Spec cross-artifact checks | PASS | 15 FR / 6 SC covered; 0 Critical/High; split small Save/Actor specs |
| CP1 Data fields + validation | PASS | `Wacom.Data.RunPickup.Validation` 9/9; parent `Wacom.Data.RunPickup` 11/11 | PASS | UBT 61 actions, Result Succeeded; Debug Gold/PoisonFang assets load with empty grants |
| CP2 Credential state + Pickup | PASS | Credential 8/8; Pickup 9/9; Actor forwarding 1/1 | PASS | Initial test-helper compile error fixed; final UBT Result Succeeded; atomicity/idempotency/four removal paths pass |
| CP3 FloorEntrance + map validator | PASS | FloorTransition 6/6; Map Validation 5/5 | PASS | UBT 13 actions; credential-only/card non-inference/mixed AND/revalidation + dominance all pass |
| CP4 SaveGame v4 | PASS | `Wacom.Run.Save` 8/8 | PASS | UBT 14 actions then 4-action expected-log fix; first run only failed because intentional reject logs were not registered, final run exit 0; deterministic order/v3 empty migration/invalid atomic reject pass |
| CP5 Docs + full regression | PASS | 7 focused prefixes, 64/64 total | PASS | final UBT up-to-date/Result Succeeded; RunPickup 11, Credential 8, Pickup 9, RewardPickup 17, FloorTransition 6, Map Validation 5, Save 8; all exit 0 |

## Blueprint / AssetRegistry / failed-load

| Check | Result | Notes |
|---|---|---|
| Existing Debug Pickup DataAssets load | PASS | Gold/PoisonFang tests load real assets; `GrantedCredentialIds.Num()==0` |
| RewardPickup Blueprint/class load | PASS | `Wacom.UI.WorldInteraction.RunRewardPickup` 17/17 includes Blueprint inheritance/default/authoring and Debug sample asset checks plus full-Definition forwarding |
| Failed-load log audit | PASS | No content、Blueprint 或 Linker load failure；仅 UE 启动期可选 aqProf/Vtune/WinPix/Wintab DLL 缺失，以及 Automation 注册前的 Engine self-test `Condition failed` 噪声；所有目标测试均 exit 0 |
| Content binary diff | PASS | `git diff --name-status -- '*.uasset' '*.umap'` 无输出；未保存 Package，因此无新增二进制哈希 |

## PIE

本轮没有正式 `Credential.Run.SerpentSigil` Pickup 或 FloorEntrance 资产，因此不做 PIE。Automation 覆盖规则、入口、迁移与 validator；现有 Debug 资产只验证兼容加载。运行时资产零改动，现有 Debug/Authoring PIE 行为风险低；尚未覆盖的是未来 Production 资产绑定后的端到端表现。未来 Production 资产轮的 PIE 应验证：获得表现卡和 Credential、移除表现卡后仍可进入、保存/恢复后仍可进入。

## Final audit placeholders

- Final commit: 见交接报告（即包含本文件的提交）
- Changed files: 42 paths；11 个 Spec 008 工件，长期 Docs/托管指针，Credential/Pickup/FloorEntrance/SaveGame v4 源码及小型测试；精确列表见 final commit
- `git diff --check`: PASS（仅 Git 提示工作区 LF 将按配置转 CRLF，无 whitespace error）
- `git lfs status`: PASS，无待提交 LFS object
- Stable-ID audit: PASS；`Credential.Run.SerpentSigil` 在运行时实现中无硬编码，只出现在测试、Spec 与长期内容合同
- `.uasset/.umap` hashes: N/A；二进制 diff 为零，未创建或覆盖 Package
- Skipped items: PIE（无 Production 资产可绑定验证）与 builder（明确禁止且本轮不需要）；SaveGame 总开关仍保持 disabled，本轮只验证底层 v4 schema
- Unreal process audit: PASS，最终审计时 Editor/Cmd/ShaderCompileWorker 数量为 0
