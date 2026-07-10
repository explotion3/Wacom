---
type: architecture
scope: wacom-project
status: active
updated: 2026-07-10
tags:
  - wacom/architecture
  - wacom/modules
  - wacom/boundaries
---

# Architecture

> [!info] 本文职责
> 本文定义 Wacom 当前模块边界和长期架构原则。具体规则事实放在领域文档；本文只记录跨模块依赖、所有权、Public / Private 边界和系统选择。

> [!warning] 架构约束
> 不要让高层模块反向污染底层模块。规则逻辑留在领域模块，UI 只读 Snapshot / ViewModel / ViewData 并提交命令。

## 1. 当前项目状态

当前项目是 UE 5.8 项目，采用 7 模块结构。

现有模块：

- `WacomCore`：通用类型、GameplayTags、跨模块共享契约。
- `WacomData`：卡牌、敌人、意图、角色、商店、探索事件等静态定义。
- `WacomBattle`：战斗内核。
- `WacomRun`：战斗之间的 Run 状态、背包、SpecialZone、负重、压力、经验、商店、探索事件、战斗结果回传和存档结构。
- `WacomApp`：游戏层、第一人称、输入绑定、世界交互，以及 UI 表现层的物理承载模块。具体 UI 数据流、Widget、Toast、ViewData 以 `WacomUI.md` 为准。
- `WacomEditor`：内容生成、编辑器工具、数据校验（Editor-only target）。
- `WacomTests`：自动化测试（DeveloperTool）。

选择 Day 1 就拆成多个模块而不是单模块内部目录模拟，原因是编译器级别的反循环依赖约束在空项目期成本最低，后期迁移成本最高。不走 UE 插件化，保留将来剥离为 `WacomRules` 插件的可能。

## 2. 架构目标

当前架构目标：

- 可以通过探索场景中的可交互 Actor 启动战斗、商店或探索事件，并在流程结束后回到探索。
- 战斗规则集中在战斗内核中，不写进 UI 或场景 Actor。
- UI 只读取 Snapshot / ViewModel / ViewData / PresentationView，不直接修改领域状态。
- 玩家输入转换为领域命令或 Run/App 请求，由对应领域入口统一结算。
- 卡牌效果、手牌区域、敌方部位行动、Run 背包、商店、RunEvent 和战斗结果回传可以独立测试。
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
  -> WacomApp
  -> WacomEditor
```

依赖方向由各模块的 `Build.cs` 硬约束，禁止反向依赖。
`WacomTests` 是测试 harness，可以依赖 Runtime、App UI 和 Editor validation；任何运行时模块都不能反向依赖 `WacomTests`。

## 4. 模块职责

| 模块 | 职责 | 不应该负责 |
| --- | --- | --- |
| `WacomCore` | 通用类型、GameplayTags 声明、通用结果类型 | 战斗流程、UI、资产编辑器 |
| `WacomData` | 卡牌、敌人、角色、商店、探索事件等静态定义 | 本场战斗状态、Run 库存、Widget、输入 |
| `WacomBattle` | 战斗生命周期、命令结算、手牌区域、卡牌效果、敌方部位行动、Snapshot/Event/ResultPacket | UI 展示、Run 探索、关卡交互 |
| `WacomRun` | 战斗外状态、背包、压力、经验、商店、探索事件、战斗结果回传和 SaveGame schema | 单场战斗内规则细节、UI |
| `WacomApp` | GameMode、PlayerController、世界交互、输入、UI 表现层、交互目标命中与桥接 | 修改 Battle / Run 状态真相 |
| `WacomEditor` | 内容生成 Commandlet、Data Validation、开发辅助 | 运行时规则依赖 |
| `WacomTests` | 自动化测试、测试 fixture | 运行时业务逻辑 |

## 5. 目录结构

模块层：

```text
Source/
  WacomCore/
    Public/ { Types/, Tags/ }
    Private/ { Tags/ }
  WacomData/
    Public/ { Cards/, Enemies/, Characters/, Events/, Shops/ }
    Private/ { ... }
  WacomBattle/
    Public/ { Session/, Commands/, Snapshots/, Events/, Runtime/ }
    Private/ {
      Session/, Core/, Commands/, Deck/, Hand/, Cards/,
      Enemy/, Initiative/, Statuses/, Combatants/, Events/, Snapshots/,
      Effects/, Resolution/, Passives/, Rewards/
    }
  WacomRun/
    Public/ { RunSession.h, RunState.h, RunStateTypes.h, WacomSaveGame.h }
    Private/
  WacomApp/
    Public/ { Actors/, Core/, GameFramework/, Interaction/, UI/ }
    Private/ { Actors/, Core/, GameFramework/, Interaction/, UI/ }
  WacomEditor/
    Public/ { Validation/ }
    Private/ { Commandlets/, ContentBuilders/, Validation/ }
  WacomTests/
    Public/ { Fixtures/ }
    Private/ { Fixtures/, Battle/, Run/, UI/ }
```

当前重点已经从纯战斗内核扩展到 Run/App 闭环：

- `WacomBattle/Private` 继续承载战斗规则真相。
- `WacomRun` 承载战斗外状态、背包、多 zone 卡牌归属、压力/经验、商店、RunEvent、战斗结果回传。
- `WacomApp` 承载 CommonUI 层级、探索/战斗状态切换、世界交互、输入协调和 HUD/Screen/Toast。

RunEvent Choice Evaluation 的唯一 Implementation 位于 `WacomRun/Private/Events/FRunEventExecutor`。RunEvent Snapshot、普通提交、卡牌支付预检和正式支付提交共享同一份 condition / payment fact 求值；提交入口始终读取最新 `FRunState`，Public Snapshot / Result 与 Blueprint contract 不暴露内部 seam。Effects 仍在 working-state 事务中执行和回滚，不进入 Choice Evaluation。

`BattleState`、`BattleResolver`、各命令 Resolver、各效果执行器都在 `WacomBattle/Private`，
外部模块编译期不可见。对外入口是 `WacomBattle/Public/Session/BattleSession.h`。

## 6. 战斗内核边界

战斗内核负责单场战斗的唯一规则真相。

核心公共契约是 `UBattleSession + FBattleCommand + FBattleSnapshot + FBattleEvent + FBattleResultPacket`。Resolver、Executor、Service 和 `BattleState` 都在 `WacomBattle/Private`，外部模块只通过公共契约交互。

`UBattleSession` 是 public facade，不承载规则实现。初始化、命令外壳、击倒请求流和战后包构造分别收口到 `WacomBattle/Private` helper：`BattleInitializer`、`BattleCommandPipeline`、`KnockdownFlowService`、`BattleResultPacketBuilder`。这些 helper 可以读写 `BattleState`，但不进入 Public API，也不被 UI / Run / App 直接 include。

PlayCard Evaluation 的唯一 Implementation 位于 `WacomBattle/Private/Commands` 的 `FPlayCardEvaluator`。该深层 Module 在一个 Private Interface 后集中源卡、结构性目标、运行时费用、阶段与拒绝投影：Target Probe 严格求值具体显式对象，Preview Candidate 把可选 Preview Focus 与规范化执行绑定分离，Commit Evaluation 为正式提交与 Action Preview 生成携带 `StateVersion` 的 Prepared PlayCard。这个 seam 不新增 UE 反射类型，也不扩张 `WacomBattle/Public` 的 Session / Command / Preview 或 Blueprint Interface。

PlayCard Transaction 的唯一 Implementation 位于 `WacomBattle/Private/Commands/PlayCardResolver`。`PlayCardResolver` 只消费 Prepared PlayCard，不重复阶段、源卡、目标或费用判断；Prepared 状态版本过期时必须在事件、RNG 和状态变更前拒绝。正式提交使用 formal operation adapter，Action Preview 在复制的 `BattleState` 上使用 deterministic preview adapter；两者共享 Commit Evaluation 与同一结算顺序。operation adapter 会显式透传到 Effect、ZoneHook、Passive、OnDiscard 和 EnemyAction，不使用全局或 `thread_local` 模式。

Effect Semantics 的唯一 Implementation 位于 `WacomBattle/Private/Effects/Semantics`。该深层 Module 通过 code-defined semantic family 集中 Card / Intent 支持、资产字段到 typed target / parameter facts 的 decode、制作合法性、目标展开、preview determinism、handler 和 Target Preview projector；Registry 只作为 `EffectTag -> semantics` 索引，不是万能配置表。`FCardEffectChain` 是词法 scratch 所有者，Main、ZoneHook、PerfectRelease 和 Passive 只建立正确生命周期并提交 effect segment，不维护 `LastShuffledCardId` 或 EffectType 分支。formal 与 preview 两个 operation adapter 继续位于 resolved invocation 和 handler 之间，随机目标选择只能在 adapter 放行后发生。该 Private Interface 不新增 UE 反射类型、Public Blueprint surface 或 Build.cs 依赖。

Combatant Mutation 的唯一 Implementation 位于 `WacomBattle/Private/Combatants`。该深层 Module 在一个 typed Private Interface 后集中玩家和敌方部位的 HP、Shield、stack status、玩家 HP 阈值与运行时部位破坏边沿；Damage Facts 同时记录请求伤害、护盾吸收、实际 HP 损失和 overkill，`DamageDealt.Amount` 只投影实际 HP 损失。Operation Adapter、RNG、Effect Chain 继续策略、击倒请求阶段和 Battle End 判断不下沉到该 Module。

Status Semantics 的唯一 Implementation 位于 `WacomBattle/Private/Statuses`。它按状态与宿主解释施加、待生效手牌控制、回合物化、消费和 Poison cadence，再分别委托 Combatant Mutation、Card Runtime State 与 Initiative Timeline；它不是通用 callback registry 或 timed-status 配置表。玩家侧 Slow / Freeze / Twilight 先以 `PendingHandAfflictions` 保存，抽牌并重建 Hand 后才选择具体卡牌，物化后立即删除 pending facts。

Card Runtime State 位于 `WacomBattle/Private/Cards`，集中单卡正层数 `StatusStacks`、永久战内 `RuntimeCostModifier`、状态费用贡献、冻结限制和唯一 `FCardCostFacts` 计算。PlayCard Evaluation、Effect Preview、Snapshot 与 Action Preview 不再自行拼接费用。Initiative Timeline 位于 `WacomBattle/Private/Initiative`，是正常运行时 `CurrentInitiative` 的唯一写入口；Combatant Mutation 在部位破坏原子事务中归零先机属于明确例外。

运行时 stack status 按宿主各有唯一正层数真相：Combatant 状态位于玩家/部位 `StatusStacks`，Card 状态位于 `FRuntimeCardInstance.StatusStacks`。Snapshot 的 `Statuses` 都由对应层数表投影，不是第二份可变状态；当前未实现 timed status，也不保留平行 `StatusInstances` 容器。

Turn Lifecycle 的唯一 Implementation 位于 `WacomBattle/Private/Turns`。`FBattleTurnLifecycleModule` 通过 `StartInitialPlayerTurn / CompleteCurrentTurn` 两个 Private Interface 集中首回合事件、TurnStart、TurnEnd、敌方行动前后 BattleEnd gate、下一回合和 PresentationJournal checkpoint 顺序；`BattleInitializer` 与 `EndTurnResolver` 只保留各自的初始化 / Command Adapter 职责。该 Module 不吸收 Enemy Action、BattleEnd 或 Card Zone 算法，不新增通用 stage registry、Journal Adapter、Operation Adapter、反射类型或 Public surface。

`WacomBattle/Private/Hand/BattleCardZoneTransition` 是卡牌区域迁移的深层规则 seam。当前权威范围包括正式出牌去向、Combo 稳定返回、`Effect.Discard / Effect.Card.DiscardSelected / Effect.Card.ExhaustSelected` 与 EndTurn 普通手牌弃置。Combo 与相邻冻结共享 PlayCard Evaluation 捕获的前邻居、后邻居和原始 index；若效果链已经显式腾挪源卡，则显式移动优先。`DeckService` 只作为无事件的物理移动 primitive，PlayedPile 自然清理仍由 Turn Lifecycle 单独编排；`HandZoneMoveEventService` 是 HandLimit、奖励和 Companion 等未迁移路径仍使用的过渡 Implementation。

UI、Actor 和测试入口都不应该直接改 `BattleState`。它们只能提交命令，读取快照、事件和战后包。

## 7. Command / Snapshot / Event

### Command

玩家操作进入战斗内核时统一转换为命令。

当前核心命令：

- `PlayCard`
- `Wait`
- `EndTurn`
- `KnockdownChoice`

新增 Command 时，应先确认它是否属于战斗规则真相，再把公共命令类型放进 `WacomBattle/Public`，具体执行仍留在 `WacomBattle/Private`。

### Snapshot

Snapshot 是 UI 和测试的只读状态模型，不允许被 UI 修改。战斗 Snapshot 细节见 [WacomBattle.md](./WacomBattle.md)。

### Event

Event 用来记录结算过程，不作为战斗真相，只作为表现、日志和测试验证依据。UI 可以把 Event 转成 `PresentationView`，但 PresentationView 不得回流为规则输入。

## 8. 数据边界

静态配置和运行时状态必须分开。

静态定义示例：

- `CardDefinition`
- `EnemyDefinition`
- `EnemyPartDefinition`
- `IntentDefinition`
- `CharacterDefinition`
- `ShopDefinition`
- `RunEventDefinition`

运行时实例示例：

- `RuntimeCardInstance`
- `RuntimeEnemyPart`
- `RuntimeIntent`
- `RuntimeCardInstance` 的卡牌状态与战内费用事实

不要在战斗中直接修改静态卡牌定义。Cost 修正、状态层数、卡牌临时变化都应写到运行时实例或战斗状态中。

## 9. UI 与表现边界

UI 负责展示，不负责规则。

UI 可以：

- 展示 Snapshot。
- 播放 Event 对应动画。
- 发起 Command。
- 展示出牌预览。
- 把领域 Snapshot / Result / Event 转成 UI-only ViewData 或 PresentationView。

UI 不可以：

- 直接移动手牌。
- 直接扣敌人先机。
- 直接修改敌人 HP。
- 自己计算最终规则结果作为真相。
- 让 ViewData / PresentationView 反向成为领域规则输入。

当前 UI 仍以 C++ 默认布局为主，但必须遵守边界。UI 当前事实集中记录在 `WacomUI.md`，WBP 绑定合同分别记录在 `UI_Backpack_WBP_Binding.md` 和 `UI_Battle_WBP_Binding.md`。

Run 域 HUD 使用 `UWacomRunViewModelProvider` + `UWacomRunViewModel`；Shop / RunEvent / Backpack Screen 读取 `URunSession` Snapshot 或 ViewData；Battle UI 保持 `FBattleSnapshot` 推送模型。两者都不直接修改规则状态。

当前 UI 侧 ViewData / PresentationBuilder 包括卡牌展示、商店商品、RunEvent 结果、BattleEvent 表现、AppToast 和目标选择视图。它们属于 `WacomApp` 表现层。

复杂 `WacomApp` Widget 流程不要扩成跨模块 Public API。Screen / HUD 负责 View 所有权、生命周期、绑定和重建；购买、选择、移动、删除、确认弹窗、Toast 和访问结束等编排应收口到 `WacomApp/Private` 的 coordinator / command flow helper。

`WacomApp/Public` 的 Widget 头文件只保留 WBP 绑定、运行时生命周期和玩家真实操作所需的 API。测试口不得以 Blueprint / Public Widget API 的形式暴露，尤其不得新增 callable 的 `ForTest`、`ForAutomationTest`、`OverrideForTest` UFUNCTION 或 Blueprint 可见测试函数。测试访问的优先级如下：

1. 在 `WacomTests` 中定义 tests-only probe subclass，借由真实生命周期和公开玩家意图入口观察行为。
2. 如生产代码确有扩展点需求，提供 protected production seam；该 seam 必须有运行时语义，不能只为测试绕过流程。
3. 最后才使用 automation-only private friend / test-access；这类入口不能是 `UFUNCTION` 或 Blueprint 可见 API，并应尽量藏在 `WacomApp/Private` 或测试模块内。

测试不能锁死未来 WBP、CommonUI 生命周期或 MVVM 重构的内部形状。自动化测试应验证玩家可观察行为、Snapshot / ViewData 输出和命令副作用，而不是依赖某个临时 C++ fallback 布局或 Public Widget 测试捷径。

## 10. 验证入口

当前世界交互入口统一通过 `IWacomWorldInteractable`：

```text
玩家进入交互半径
-> Actor 注册为 CandidateInteractable
-> PlayerController 选择最近且 CanInteract 的对象
-> 按 E 调用 TryInteract
-> Battle / Shop / RunEvent 各自进入对应领域入口和 UI
```

当前实现的世界交互对象包括 BattleTrigger、ShopTrigger 和 RunEventTrigger。

## 11. 当前已落地骨架

已经落地的骨架包括：

1. `WacomBattle`：BattleSession、Command、Snapshot、Event、ResultPacket、击倒事件、奖励卡、效果执行器。
2. `WacomRun`：RunSession、背包、SpecialZone、负重、经验/压力、商店、RunEvent、战斗结果回传、SaveGame schema。
3. `WacomApp`：GameMode、PlayerController、世界交互接口、CommonUI 层级、探索 HUD、BattleHUD、Backpack / Shop / RunEvent Screen、AppToast。
4. `WacomData`：卡牌、敌人、角色、商店、RunEvent 静态定义和生成内容。
5. `WacomEditor`：WacomRegenerateContent commandlet、Shop / RunEvent Data Validation。
6. 自动化测试覆盖 Battle / Run / UI / Data validation 关键规则。

## 12. 自动化测试重点

测试不是独立规则源。具体测试清单按领域文档和 `Source/WacomTests` 当前实现维护。

架构层只要求：

- 关键规则必须有自动化测试覆盖。
- UI / App 测试不能绕过领域入口直接改内部状态。
- UI / App 测试不能要求 `WacomApp/Public` 暴露 Blueprint callable 的测试接口；需要测试访问时按 tests-only probe subclass、protected production seam、automation-only private friend / test-access 的顺序收口。
- DataAsset 生成和 Validator 至少有结构验证测试。
- 影响 Build.cs 依赖、Public API、SaveGame schema 或跨模块契约的改动要优先补测试。

## 13. 插件与系统选择

当前已采用 / 保持采用：

- `Enhanced Input`：输入映射。
- `GameplayTags`：词条、状态、效果类型、区域等标识。
- `CommonUI`：主 UI 层级和 Activatable Widget 管理。
- `ModelViewViewModel`：Run 域 ViewModel / Provider，供探索 HUD 和背包顶部统计使用。
- `Niagara`：计划用于后续表现；当前不构成模块边界或规则依赖。

当前不把 GAS 作为战斗核心。核心是卡牌规则内核，不是典型技能 Ability 生命周期。GAS 后续可以作为状态和属性系统参考，但不应阻塞战斗框架。

## 14. 依赖规则

代码依赖方向应保持单向：

```text
WacomCore <- WacomData <- WacomBattle <- WacomRun <- WacomApp
```

`WacomEditor` 位于运行时依赖链之外，只允许依赖运行时模块和编辑器模块。`WacomTests` 是测试 harness，可依赖 Runtime、App 和 Editor validation；任何生产模块都不能依赖 `WacomTests`。

约束：

- 依赖方向由 `Build.cs` 强制，严禁反向依赖。
- Public 头文件只放外部需要的协议和轻量类型。
- Private 放规则实现和服务类；外部模块无法 include Private 头。
- 头文件优先前向声明，具体 include 放到 `.cpp`。
- 只有需要反射、蓝图、资产或序列化的类型才使用 `UCLASS / USTRUCT / UENUM`。
- `WacomApp` 不得直接修改 `BattleState`，只能通过 `UBattleSession` 提交命令、读取快照和事件。

## 15. 暂不处理 / 后续方向

后续功能方向不在本文追踪，见 [Roadmap.md](./Roadmap.md)、[TechDebt.md](./TechDebt.md) 和 [Questions.md](./Questions.md)。本文只在这些方向改变模块边界或依赖图时更新。
