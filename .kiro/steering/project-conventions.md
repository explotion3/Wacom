---
inclusion: always
---

# Wacom 项目总约定

本文是项目级的持久开发规则。每次对话默认加载。

## 项目身份

- 单人第一人称 HD-2D 卡牌冒险游戏，UE 5.7，主导程序单人。
- 文档位于 `Docs/`：`Game_Design.md` / `Architecture.md` / `WacomBattle.md` / `WacomRun.md` / `WacomApp.md` / `WacomData.md` / `TODO.md` / `Dev_Log.md` / `Characters/BugGirl.md` / `Karpathy_Guidelines_CN.md`

## 任务启动流程

每次接到新任务时：

1. 判断任务属于哪个领域
2. 读取对应文档和代码
3. **如果需求模糊**（多种理解方式、边界不清、成功标准不明确）：激活 `karpathy-guidelines` skill，显式列出假设和疑问，与主导达成一致后再动手
4. 完成后同步更新对应文档

| 领域 | 读取文档 | 读取代码 |
|---|---|---|
| `Battle` | `Docs/WacomBattle.md` | `Source/WacomBattle/` |
| `Run` | `Docs/WacomRun.md` | `Source/WacomRun/` |
| `Data` | `Docs/WacomData.md` | `Source/WacomData/` |
| `App/UI` | `Docs/WacomApp.md` | `Source/WacomApp/` |
| `Editor` | `Docs/Architecture.md` | `Source/WacomEditor/` |
| `Docs` | 目标文档本身 | 不需要读代码 |

跨领域任务读取多个文档。不确定归属时先问。

## UI 约定

- CommonUI 管理层：`UWacomGameUIManagerSubsystem` + `UWacomPrimaryGameLayout`
- 四层 Layer：`Game` / `GameMenu` / `Modal` / `Overlay`
- Widget 血统：`UWacomActivatableWidget`（根）→ `UWacomBattleWidgetBase`（战斗）/ `UWacomMenuWidgetBase`（菜单）
- C++ 定义结构和接口，WBP 做布局和样式
- UI 只读 Snapshot + 发 Command，不修改战斗状态
- Widget 不直接 SubmitCommand / OpenLevel，委托给 HUD / GameMode

## 反射使用门槛

只有以下理由才用 UCLASS / USTRUCT / UENUM / UPROPERTY：蓝图访问、DataAsset 承载、序列化、GC 引用管理、反射遍历、UMG Widget。否则用纯 C++。

## Public / Private 边界

- `Public/`：对外协议、轻量类型、入口。`Private/`：规则实现、内部服务。
- 头文件优先前向声明，具体 include 放 `.cpp`。
- 战斗真相必须留在 `WacomBattle/Private/`，外部只通过 `UBattleSession` + Snapshot + Event 访问。

## 术语表

- ✅ `EnemyPart`（敌方部位）、`HandZone`（手牌区域）、`HandQueue`（手牌队列）、`RuntimeCost` vs `BaseCost`、`HandAnchor`（左手牌/右手牌）

## GameplayTag 约束

- 所有 tag 在 `WacomCore/Public/Tags/WacomGameplayTags.h` 用 `UE_DECLARE_GAMEPLAY_TAG_EXTERN` 声明
- 严禁字符串拼 tag
- 新增 tag 同步更新 `WacomData.md §5`

## 完成即验证

每个切片写完必须编译通过 + 自动化测试全绿。

```
编译: "e:\UE_5.7\Engine\Build\BatchFiles\Build.bat" WacomEditor Win64 Development -Project="d:\UE_Project\5.7\Wacom\Wacom.uproject" -WaitMutex -FromMsBuild
测试: "e:\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "d:\UE_Project\5.7\Wacom\Wacom.uproject" -ExecCmds="Automation RunTests Wacom; Quit" -Unattended -NoPause -NoSplash -NullRHI
```

## 随机与确定性

所有随机使用 `BattleState.Rng`（FRandomStream），禁止 `FMath::Rand*`。测试可注入 seed。

## 临时决定

先查 `TODO.md`。有就按它走；没有就新建写进去；正式化后标记。

## 规则真相

规则真相在 `Docs/*.md`，不在脑内。遇到分歧：先查文档 → 没覆盖就标注"规则分歧"向主导提问 → 共识后先改文档再改代码。绝不悄悄引入未写入文档的规则分支。

## 开发日志

重要里程碑写入 `Dev_Log.md`：哪个切片完成、踩坑、文档修订点。不写流水账。
