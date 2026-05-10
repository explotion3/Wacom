---
inclusion: always
---

# Wacom 项目总约定

本文是项目级的持久约定。每次对话默认加载。

## 项目身份

- 单人第一人称 HD-2D 卡牌冒险游戏。
- UE 5.7。
- 主导程序：单人。
- 核心设计文档位于 `Docs/`：
  - `Architecture.md`：模块与依赖
  - `Battle_Rules.md`：战斗流程
  - `Hand_Zone_Rules.md`：手牌区域
  - `Game_Design_Document.md`：GDD
  - `Data_Schema_Draft.md`：数据结构与字段
  - `Characters/BugGirl.md`：角色虫妹
  - `FirstPerson_HD2D_Card_Variant.md`：方案背景
  - `Phase2_Plan.md`：第二阶段规划
  - `Phase2_Temporary_Decisions.md`：第二阶段临时决定
  - `Dev_Log.md`：开发日志

## 当前阶段

**第一阶段已完成**（S1-S11）。战斗内核 + 数据 + 自动化测试 + PIE 测试入口全部就位。

**当前处于第二阶段**，下一步是 P1（战斗 UI 骨架）。

## 模块结构（7 模块，非插件化）

```
Source/
├── WacomCore/      Runtime, PreDefault     依赖: Core, CoreUObject, GameplayTags (+Engine private)
├── WacomData/      Runtime, PreDefault     依赖: WacomCore
├── WacomBattle/    Runtime, Default        依赖: WacomData
├── WacomRun/       Runtime, Default        依赖: WacomBattle
├── WacomApp/       Runtime, Default        依赖: WacomRun (主游戏模块, PRIMARY_GAME_MODULE)
├── WacomEditor/    Editor, Default         依赖: WacomBattle + UnrealEd/AssetTools/...
└── WacomTests/     DeveloperTool, Default  依赖: WacomRun
```

**依赖方向由 `Build.cs` 硬约束，严禁反向依赖。**

```
WacomCore <- WacomData <- WacomBattle <- WacomRun <- WacomApp
WacomEditor / WacomTests 位于依赖链之外，只向运行时模块单向依赖。
```

## 第二阶段切片顺序（Phase2_Plan.md）

1. ⏳ P1：UI 基础设施（CommonUI 框架、Layer 管理、Widget 基类体系、通用组件）
2. P2：战斗 UI（在 P1 框架上实现战斗界面）
3. P3：规则补全（保留 / 中毒 / ZoneHook / 伙伴被动）
4. P4：Enhanced Input 迁移
5. P5：UI 动画基础
6. P6：主题与样式

每个切片结束必须编译通过 + 现有自动化测试全绿。

## UI 架构约定（P1 起生效）

- **CommonUI 作为 UI 管理层**：`UWacomGameUIManagerSubsystem` + `UWacomUIPolicy` + `UWacomPrimaryGameLayout`。
- **四层 Layer**：`Game`（战斗 HUD）/ `GameMenu`（暂停、背包）/ `Modal`（确认框）/ `Overlay`（Toast）。
- **Widget 基类**：`UWacomActivatableWidget`（提供 Snapshot 刷新、动画钩子、Session 访问）。
- **C++ 定义结构和接口，Widget Blueprint 做布局和样式**。后续美术换皮只改 WBP，不动 C++。
- **刷新策略**：每次命令后全量从 Snapshot 重建。不做增量 diff。
- **交互状态机**：Idle → TargetSelect → Resolving → 回到 Idle。
- **UI 不修改战斗状态**：只读 `FBattleSnapshot`，只发 `FBattleCommand`。
- **Widget 不直接调用 SubmitCommand**：通过委托通知 HUD，HUD 统一提交。
- **代码位置**：`WacomApp/Public/UI/{Foundation,Battle,Common}/` + `WacomApp/Private/UI/`。
- **资产位置**：`Content/Wacom/UI/{Foundation,Battle,Common}/WBP_*.uasset`。

## 反射 / UCLASS 使用门槛

**只有以下理由才使用 UCLASS / USTRUCT / UENUM / UPROPERTY：**

- 需要蓝图访问
- 需要 DataAsset / DataTable 资产承载
- 需要序列化（存档、网络）
- 需要 UObject GC 引用管理
- 需要反射遍历
- **UMG Widget 类必须是 UCLASS**

否则用纯 C++ struct / enum class / 原生容器。

## Public / Private 边界

- `Public/`：外部模块可以 include，只放协议、轻量类型、对外入口。
- `Private/`：规则实现、内部服务类，外部编译期不可见。
- 头文件优先前向声明；具体 include 放 `.cpp`。

战斗真相 (`BattleState`、`BattleResolver`、各 Resolver、Executor) 必须留在 `WacomBattle/Private/`。
UI 和 Actor 只能通过 `UBattleSession` + `FBattleSnapshot` + `FBattleEvent` 访问战斗。

## 术语表（统一用词）

对齐 `Data_Schema_Draft.md §1`。代码和新文档禁止混用：

- ✅ `EnemyPart` / 敌方部位。❌ `enemy unit` / `Unit` / `Part`（单用时）
- ✅ `HandZone` / 手牌区域
- ✅ `HandQueue` / 手牌队列
- ✅ `RuntimeCost` vs `BaseCost`
- ✅ `HandAnchor`（左手牌 / 右手牌）

## GameplayTag 约束

- 所有 tag 必须在 `WacomCore/Public/Tags/WacomGameplayTags.h` 用 `UE_DECLARE_GAMEPLAY_TAG_EXTERN` 声明。
- 严禁业务代码里用字符串拼 tag。
- 新增 tag 时同步更新 `Data_Schema_Draft.md §2`。

## 完成即验证（硬要求）

**每个切片写完必须在本机编译通过才算完成。**

编译命令：

```
"e:\UE_5.7\Engine\Build\BatchFiles\Build.bat" WacomEditor Win64 Development -Project="d:\UE_Project\5.7\Wacom\Wacom.uproject" -WaitMutex -FromMsBuild
```

自动化测试：

```
"e:\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "d:\UE_Project\5.7\Wacom\Wacom.uproject" -ExecCmds="Automation RunTests Wacom.Battle; Quit" -Unattended -NoPause -NoSplash -NullRHI
```

重建 DataAsset：

```
"e:\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "d:\UE_Project\5.7\Wacom\Wacom.uproject" -run=WacomRegenerateContent -NoSplash -Unattended
```

## 随机与确定性

- 所有腾挪、随机插入、随机目标使用 `BattleState.Rng`（`FRandomStream`）。
- 不得调用 `FMath::Rand`、`FMath::RandRange` 等全局随机。
- 测试可注入 seed 复现。

## 不引入的东西（第二阶段）

- GAS（GameplayAbilitySystem）：不作为战斗核心。
- DataTable：继续用 PrimaryDataAsset。
- SaveGame / 网络复制：先不写。
- 完整 Run 外层：第二阶段只做战斗。
- 卡牌拖拽 / 手牌扇形 / 动画 / 特效 / 音效：P1 不做。
- ViewModel 层：P1 不做，后续按需引入。

## 临时决定的处理

- 遇到"先怎么做"的问题，先查 `Phase2_Temporary_Decisions.md`。
- 有已定的临时决定就按它走，不纠结。
- 没有就做一个新的，写进去。
- 正式化后标记为"已正式化"。

## 规则问题的唯一真相

战斗、手牌、卡牌规则的真相在 `Docs/*.md`，不在脑内。遇到规则分歧时：

1. 先查 `Docs/Battle_Rules.md` / `Hand_Zone_Rules.md` / `Data_Schema_Draft.md`。
2. 如果文档没覆盖或矛盾，标注为"规则分歧"，向主导提问，不自行发挥。
3. 达成共识后先改文档，再改代码。

绝不在代码里悄悄引入未写入文档的规则分支。

## 开发日志

重要里程碑写入 `Docs/Dev_Log.md`，仅记录：
- 哪个切片完成
- 遇到并绕过的约束
- 规则文档因为实现需要做的修订点

不写每日流水账。
