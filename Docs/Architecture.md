# Architecture

本文定义当前项目的长期架构方向和第一阶段落地边界。目标是在规则仍会变化的情况下，先建立一个可测试、可替换、可扩展的项目框架。

## 1. 当前项目状态

当前项目是 UE 5.7 项目，采用 7 模块结构。

现有模块：

- `WacomCore`：基础类型、ID、GameplayTags 声明。
- `WacomData`：卡牌、敌人、意图、角色等静态定义。
- `WacomBattle`：战斗内核。
- `WacomRun`：战斗之间的状态与探索（第一阶段几乎为空）。
- `WacomApp`：游戏层、第一人称、输入绑定、UI、测试 Actor。同时作为主游戏模块。
- `WacomEditor`：编辑器工具、数据校验（Editor-only target）。
- `WacomTests`：自动化测试（DeveloperTool）。

选择 Day 1 就拆成多个模块而不是单模块内部目录模拟，原因是编译器级别的反循环依赖约束在空项目期成本最低，后期迁移成本最高。不走 UE 插件化，保留将来剥离为 `WacomRules` 插件的可能。

## 2. 架构目标

第一阶段架构目标：

- 可以通过测试场景中的战斗 Actor 启动一场测试战斗。
- 战斗规则集中在战斗内核中，不写进 UI 或场景 Actor。
- UI 只读取快照和事件，不直接修改战斗状态。
- 玩家输入转换为命令，由战斗内核统一结算。
- 卡牌效果、手牌区域、敌方部位行动可以独立测试。
- 策划规则变化时，优先修改数据、效果执行器或局部 Resolver，而不是重写整体流程。

长期架构目标：

- 战斗、数据、Run、UI、编辑器工具各自有明确边界。
- 内容以 DataAsset / DataTable / Registry 思路组织。
- 自动化测试覆盖关键规则。
- 运行时模块不依赖编辑器模块。
- 表现层不成为规则真相。

## 3. 模块图

模块拆分如下：

```text
WacomCore
  <- WacomData
      <- WacomBattle
          <- WacomRun
              <- WacomApp

WacomEditor
  -> WacomCore
  -> WacomData
  -> WacomBattle

WacomTests
  -> WacomCore
  -> WacomData
  -> WacomBattle
  -> WacomRun
```

依赖方向由各模块的 `Build.cs` 硬约束，禁止反向依赖。

## 4. 模块职责

| 模块 | 职责 | 不应该负责 |
| --- | --- | --- |
| `WacomCore` | 基础 ID、轻量枚举、GameplayTags 声明、通用结果类型 | 战斗流程、UI、资产编辑器 |
| `WacomData` | 卡牌、敌人、意图、状态、角色等静态定义；资产查询入口 | 本场战斗状态、Widget、输入 |
| `WacomBattle` | 战斗生命周期、命令结算、手牌区域、卡牌效果、敌方部位行动、快照事件 | UI 展示、Run 探索、关卡交互 |
| `WacomRun` | 战斗之间的状态、背包、探索事件、商店、休息、路线或区域状态 | 单场战斗内规则细节 |
| `WacomApp` | 第一人称角色、输入绑定、测试场景交互、UI、HUD、动画和特效触发；游戏主模块 | 修改战斗状态真相 |
| `WacomEditor` | 数据校验、开发按钮、内容生成、自动化测试辅助 | 运行时规则依赖 |
| `WacomTests` | 自动化测试、测试 fixture | 运行时业务逻辑 |

## 5. 目录结构

模块层：

```text
Source/
  WacomCore/
    Public/ { Types/, Tags/ }
    Private/ { Tags/ }
  WacomData/
    Public/ { Cards/, Enemies/, Characters/, Registry/ }
    Private/ { Cards/, Enemies/, Characters/, Registry/ }
  WacomBattle/
    Public/ { Session/, Commands/, Snapshots/, Events/, Runtime/ }
    Private/ {
      Session/, Core/, Commands/, Deck/, Hand/,
      Enemy/, Status/, Events/, Snapshots/,
      Effects/, Resolution/, Passives/
    }
  WacomRun/
    Public/, Private/
  WacomApp/
    Public/ { Actors/, Pawns/, Controllers/, UI/ }
    Private/ { Actors/, Pawns/, Controllers/, UI/ }
  WacomEditor/
    Public/
    Private/ { Validators/, Commands/, Bootstrap/ }
  WacomTests/
    Public/ { Fixtures/ }
    Private/ { Fixtures/, Battle/ }
```

第一阶段重点在 `WacomBattle/Private`。`WacomRun` 和 `WacomApp` 只做最小入口。

`BattleState`、`BattleResolver`、各命令 Resolver、各效果执行器都在 `WacomBattle/Private`，
外部模块编译期不可见。对外入口是 `WacomBattle/Public/Session/BattleSession.h`。

## 6. 战斗内核边界

战斗内核负责单场战斗的唯一规则真相。

核心对象建议：

- `BattleSession`：一场战斗的生命周期入口。
- `BattleState`：当前战斗状态。
- `BattleCommand`：玩家或系统输入的操作。
- `BattleResolver`：统一结算命令。
- `BattleSnapshot`：给 UI 读取的只读状态。
- `BattleEvent`：给 UI、日志和测试读取的事件流。
- `CardEffectDispatcher`：卡牌效果分发（Target 映射 + 条件评估 + Magnitude 计算 + 执行）。
- `EffectExecutor`：效果注册制执行器（按 EffectTag 分派到 Handler）。
- `MagnitudeResolver`：Magnitude 计算注册制（Literal / RuntimeCost / 扩展）。
- `ConditionResolver`：效果/被动条件评估注册制。
- `InitiativeResolver`：先机命中 / 抵抗 / 完美释放。
- `ZoneHookResolver`：ZoneHook 消费（OnPlay / OnPerfectReleaseHit）。
- `PassiveDispatcher`：被动触发调度（AfterPlayed / OnCompanionCount）。
- `PoisonResolver`：中毒结算。
- `HandZoneService`：手牌区域和腾挪规则。
- `EnemyPartActionResolver`：敌方部位行动子流程。

UI、Actor 和测试入口都不应该直接改 `BattleState`。它们只能提交命令，读取快照和事件。

## 7. Command / Snapshot / Event

### Command

玩家操作进入战斗内核时统一转换为命令。

第一阶段命令：

- `PlayCard`
- `Wait`
- `EndTurn`

后续可扩展：

- `SelectKnockdownOption`
- `ChooseReward`
- `MoveCardByDebug`

### Snapshot

Snapshot 是 UI 的读取模型。

Snapshot 应包含：

- 当前阶段。
- 玩家生命值、最大生命值。
- 当前等待值。
- 手牌队列与区域。
- 抽牌堆、弃牌堆、消耗区数量。
- 敌人部位 HP、意图、先机、状态。
- 可用操作。
- 可用卡牌和出牌预览信息。

Snapshot 不允许被 UI 修改。

### Event

Event 用来记录结算过程。

示例：

- `BattleStarted`
- `TurnStarted`
- `CardsDrawn`
- `CardPlayed`
- `HandZoneChanged`
- `InitiativeHit`
- `ResistanceResolved`
- `PerfectReleaseResolved`
- `EnemyPartActed`
- `EnemyPartHpEmptied`
- `BattleEnded`

Event 不作为战斗真相，只作为表现、日志和测试验证依据。

## 8. 数据边界

静态配置和运行时状态必须分开。

静态定义示例：

- `CardDefinition`
- `EnemyDefinition`
- `EnemyPartDefinition`
- `IntentDefinition`
- `CharacterDefinition`

运行时实例示例：

- `RuntimeCardInstance`
- `RuntimeEnemyPart`
- `RuntimeIntent`
- `RuntimeStatus`

不要在战斗中直接修改静态卡牌定义。Cost 修正、状态层数、卡牌临时变化都应写到运行时实例或战斗状态中。

## 9. UI 与表现边界

UI 负责展示，不负责规则。

UI 可以：

- 展示 Snapshot。
- 播放 Event 对应动画。
- 发起 Command。
- 展示出牌预览。

UI 不可以：

- 直接移动手牌。
- 直接扣敌人先机。
- 直接修改敌人 HP。
- 自己计算最终规则结果作为真相。

第一阶段 UI 可以简陋，但必须遵守边界。

## 10. 测试入口

第一阶段需要一个测试场景和一个战斗 Actor。

测试流程：

```text
进入测试场景
-> 与战斗 Actor 交互
-> 创建 BattleSession
-> 加载虫妹测试卡组
-> 加载蛇测试敌人
-> 进入战斗 UI 或调试界面
-> 战斗胜利 / 失败
-> 返回测试场景入口或战斗 Actor 附近
```

测试 Actor 只负责启动和结束测试战斗，不负责战斗规则。

## 11. 第一阶段实现顺序

建议按以下顺序实现：

1. 建立 `BattleState`、`BattleCommand`、`BattleSnapshot`、`BattleEvent`。
2. 实现 `BattleSession` 和 `BattleResolver`。
3. 实现起始阶段抽牌和等待值。
4. 实现 `HandZoneService`。
5. 实现 `PlayCard / Wait / EndTurn` 三个命令。
6. 实现敌方部位行动子流程。
7. 实现最小卡牌效果：伤害、腾挪、等待、先机扣减。
8. 实现先机命中、抵抗、完美释放的占位流程。
9. 实现测试敌人蛇。
10. 实现测试场景战斗 Actor。
11. 添加自动化测试覆盖关键规则。

## 12. 自动化测试重点

第一阶段至少覆盖：

- 回合开始抽 5 张普通卡牌。
- 左右手牌插入后两者之间至少有一张普通卡牌。
- 左右手都在手牌时，新抽卡不会移动已有手牌位置。
- 普通卡牌上限为 10，左右手牌不计入。
- 等待先扣当前等待值，再等待值 +1。
- 每回合等待值重置为 2。
- 费用大于敌方总先机时，卡牌不可用。
- Cost 等于部位先机时触发先机命中。
- 抵抗先于完美释放。
- HP 归零部位立刻失去意图和先机，不参与后续先机扣减。
- 左右手牌打出后不进入任何区域。
- 连击牌打出后留在原位置。
- 结束阶段调用敌方部位行动子流程。

## 13. 插件与系统选择

第一阶段建议使用：

- `Enhanced Input`：输入映射。
- `GameplayTags`：词条、状态、效果类型、区域等标识。
- `CommonUI`：如果第一阶段就开始搭 UI 层，建议早用。
- `Niagara`：后续表现。

第一阶段不建议把 GAS 作为战斗核心。当前核心是卡牌规则内核，不是典型技能 Ability 生命周期。GAS 后续可以作为状态和属性系统参考，但不应阻塞第一阶段战斗框架。

## 14. 依赖规则

代码依赖方向应保持单向：

```text
WacomCore <- WacomData <- WacomBattle <- WacomRun <- WacomApp
```

`WacomEditor` 和 `WacomTests` 位于依赖链之外，只允许向运行时模块单向依赖。

约束：

- 依赖方向由 `Build.cs` 强制，严禁反向依赖。
- Public 头文件只放外部需要的协议和轻量类型。
- Private 放规则实现和服务类；外部模块无法 include Private 头。
- 头文件优先前向声明，具体 include 放到 `.cpp`。
- 只有需要反射、蓝图、资产或序列化的类型才使用 `UCLASS / USTRUCT / UENUM`。
- `WacomApp` 不得直接修改 `BattleState`，只能通过 `UBattleSession` 提交命令、读取快照和事件。

## 15. 暂不处理

第一阶段暂不实现：

- 完整 Run。
- SAN 对探索场景的影响。
- 背包容量成长。
- 完整状态公式。
- 暮色引虫灯任务。
- 击倒奖励分支。
- 失去手指后的左右手变化。
- 正式 UI 美术。
