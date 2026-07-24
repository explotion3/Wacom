---
type: architecture
scope: wacom-project
status: active
updated: 2026-07-24
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
- `WacomData`：卡牌、敌人、意图、角色、商店、探索事件、Logical Map Graph 等静态定义。
- `WacomBattle`：战斗内核。
- `WacomRun`：战斗之间的 Run 状态、背包、SpecialZone、负重、压力、经验、商店、探索事件、Logical Map Graph 运行时规则、战斗结果回传和存档结构。
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
  -> WacomApp

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
| `WacomData` | 卡牌、敌人、角色、商店、探索事件、地图 Floor / Node / Edge 等静态定义 | 本场战斗状态、Run 库存、地图运行时状态、Widget、输入 |
| `WacomBattle` | 战斗生命周期、命令结算、手牌区域、卡牌效果、敌方部位行动、Snapshot/Event/ResultPacket | UI 展示、Run 探索、关卡交互 |
| `WacomRun` | 战斗外状态、背包、压力、经验、商店、探索事件、地图节点生命周期 / 传送 / 跨层 / 行动点规则、战斗结果回传和 SaveGame schema | 单场战斗内规则细节、Spline / 场景 Actor、UI |
| `WacomApp` | GameMode、PlayerController、世界交互、输入、UI 表现层、Run Floor Scene Descriptor 与场景绑定协调 | 修改 Battle / Run 状态真相 |
| `WacomEditor` | 内容生成 Commandlet、Data/Scene Validation、Details 制作入口、Blueprint SCS / 关卡实例组件事务写入、开发辅助 | 运行时规则依赖、Validator 隐式写入、正式关卡自动覆盖 |
| `WacomTests` | 自动化测试、测试 fixture | 运行时业务逻辑 |

### Scene Enemy Host 制作边界

Scene Enemy 使用单向依赖 `WacomEditor -> WacomApp -> WacomData`。`AWacomBattleEnemyActor` 是唯一 Actor；部位、视觉和反馈锚点全部是同一 Blueprint SCS 中的 typed Component：纯 `USceneComponent` 的 `UWacomBattleEnemyPartComponent`、`UWacomBattleEnemyPartFlipbookLayerComponent` / `UWacomBattleEnemyPartSpriteLayerComponent` 与 `UWacomBattleEnemyPartImpactAnchorComponent`。Part 只拥有 Component Transform、identity、规则映射和反馈配置；`InteractionVisualLayerId` 指定层的真实 Paper2D 属性和稳定 Idle/第一帧 BodySetup 才是碰撞制作真相。Part 不再是 Primitive，也不保存 BoxExtent；不存在 ChildActor、生成 VisualLayer 或 Host 整体视觉的第二份数据。

`WacomApp` 的纯只读 `FWacomBattleSceneEnemyHostAuthoringReport` 同时读取 live component 与 Blueprint SCS template，供 Validator、debug、Details 和同步计划消费；求值不得改身份、创建组件、调用 `Modify()` 或 dirty package。所有写操作由 `WacomEditor` 的 `FWacomBattleSceneEnemyHostAuthoring` 独占：同步按钮按 `EnemyDefinition.Parts` 创建缺失 Part、默认 Flipbook layer 和 ImpactAnchor，只派生已有 Part 的 `PartId`，并保留 viewport transform、视觉资源和人工扩展组件。它不再读写手工碰撞尺寸。Blueprint SCS 与关卡 transactional InstanceComponent 共用同一计划，多选 Host 共用一次事务，无变化不 dirty。

每个 Host 的 `UWacomBattleEnemySceneRuntimeComponent` 是唯一运行时所有者。它在 typed component 注册、注销、Construction/BeginPlay 或显式 topology 通知时收集层级，并用非反射 `FPartRuntimeState` 保存稳定身份、Snapshot facts、targetable、hover、prediction、cue、Action、Destroyed、terminal、interaction collision 与反馈组件生命周期。HUD coordinator 直接缓存 `UWacomBattleEnemyPartComponent`；指定 typed visual provider 只转发 Part handle。Runtime 为正式 interaction visual 启用稳定 Sprite BodySetup；配置异常且 Part 已绑定、已登记 HUD、存活、未 Retire 时，才懒创建 App-private transient fallback Box，并按 interaction visual bounds、直接 visual 并集、固定安全盒的顺序推导尺寸。Niagara 表现不读取该正式碰撞或 fallback Primitive：App-private `FWacomBattleEnemyPartPresentationBounds` 从 authored Idle visual 的局部 RenderBounds 与世界 Transform 保存中心及三个世界半轴，按摄像机平面做 oriented projection。Target Preview 以该视觉中心和宽高定位；ImpactAnchor 只决定命中 Niagara / 声音落点，Impact 直径仍取同一稳定视觉 Bounds。Snapshot refresh 不扫描 Actor 层级、不重建 authored 组件，也不重置 authored transform、Flipbook 进度、稳定碰撞源或 Presentation Bounds 资源。

Battle 敌人交互独占 `WacomBattleInteraction`（`ECC_GameTraceChannel1`）：敌人 interaction visual 与 transient fallback 只 Block 该通道并 Ignore `Visibility`；Run/场景对象继续使用既有 Visibility 合同。PlayerController 的统一 click/hover/drag probe 先以 Visibility 计算真实遮挡深度，再以专用通道执行严格 Line Trace 和 `8cm` Sphere Sweep，并用当前 HUD registry、遮挡深度和稳定排序收敛为唯一 Part。该通道常量只在 `WacomApp` 定义，不进入 Battle、Data 或 Save contract。

Battle scene 输入由 PlayerController 的共享查询 seam 统一：`Visibility` Line Trace 只确定真实场景遮挡深度，专用通道 Line Trace 优先解析严格 Sprite 轮廓，失败后再做默认 `8cm` Sphere Sweep；registry、遮挡和确定性排序都在该 seam 内完成，click、hover 和 first-person drag 不得各自重写命中策略。App-private outline feedback controller 只创建挂在 interaction visual 下的 transient padded-quad proxy，消费 SceneRuntime 已归约的 Selectable/Hovered/None 状态与 Target Preview Style；它不是 authored Visual Layer，不拥有 target identity，也不改变规则、原视觉材质或原视觉 transform。代理画布只增加容纳 `2 source pixels` 描边的透明留白，C++ 从 Paper2D `BakedRenderData` 解出 local-to-atlas 仿射映射，材质 counter-remap 后保证 Sprite 主体仍保持原世界尺寸。描边算法真源位于 `DShader/Material/World/M_WacomBattleEnemyPartInteractionOutline.dsm` 与对应 `.dsh` helper，生成资产固定在 `/Game/DreamMaterials/World`；`WacomEditor` 不再暴露手工构建 MaterialExpression 图的第二套实现。

Scene Enemy 的高频同步使用一次性 sync frame：HUD coordinator 对当前已收到的 `FBattleSnapshot` 和同一份 Target Selection 只建立一次 Enemy/Part 索引，再把精确 Part facts 推给 SceneRuntime；不得在每个 Host 或 Part 内重新 `BuildSnapshot()`、重复线性扫描或重算相同 targetability。SceneRuntime 对相同 Snapshot facts、targetable、hover 与 preview 执行 no-op，Host 面板按 Snapshot Version 去重。Hover Probe 只在配置间隔到达时射线，并使用 HUD 已缓存的 Snapshot Version、UI State、Pending Card 与 WorldTargetId 复用表现；SceneRuntime 不创建 world Prediction Widget，Action Preview 只进入 Host Enemy Panel。

敌人行动与破坏只属于 `WacomApp` 表现层。`WacomBattle` Presentation Journal 仍记录行动前后 Snapshot 与事件序号；queue 以完整 `EncounterId + EnemySlotId + PartSlotId` 路由到对应 Part Style 的精确 `TargetVisualLayerId`。共享 `FWacomBattleEnemyActionPlayback` 管理弱 timer、serial、Impact / Complete exactly-once 和 watchdog；真实 Flipbook component 只原地换片并恢复 authored 状态。`EnemyDestroyedClip` 可由 Host 内唯一一个 Part Style 提供整体终态；局部 Destroyed 仍在对应 typed Visual component 上换成破损资源。Combat-only Snapshot push、EnemyPanel、inspection 和 Encounter retirement 保持原数据链，不反向修改 `WacomBattle` / `WacomData` schema。

Battle presentation 的编排计时也统一留在 `WacomApp` Private。`FWacomBattlePresentationTimerOwner` 用稳定 key 独占 Event Queue Advance、Presentation Plan Poll 和 Stack Entry Exit 三类一次性 timer，并为每次注册保存原始 World、TimerHandle、serial 与业务回调；TimerManager delegate 只捕获弱 owner、key 和 serial。正常 clear 会通过原始 World 精确撤销 handle；析构遇到 World 已不可用时只遗弃 owner 内的回调状态，残留弱 delegate 即使随后触发也不会进入已释放 coordinator。Queue、Plan 和 Stack 仍拥有各自的表现语义，但不再各自管理 TimerHandle 或 raw delegate；该 Implementation 不改变 `WacomBattle` 规则、barrier 顺序或既有时长。

正式敌人内容包同样保持单向写入边界。`WacomEditor` 的 manifest-driven enemy-pack commandlet 可以用 AssetTools 晋升明确授权范围内的本地 Paper2D 依赖闭包，并幂等构建 DataAsset、Encounter 与 Host Blueprint；生成后的 `/Game/Wacom` 资产是运行时唯一依赖。本地 ignored `/Game/Art` 不是运行时 fallback，也不会被 `WacomRegenerateContent` 读取。TrainingWarrior、Snake 与 SlimeTrio 都由同一 component-native Host builder 生成；单部位和多部位只由 `EnemyDefinition.Parts` 数量与 SCS Part 组件数量区别，不存在运行时模式分支。单帧终态的资产名、来源动画与帧索引由各 manifest 精确声明，不在推广器中硬编码具体敌人。

`/Game/Wacom/Art/Placeholders` 是受控开发资产而不是正式出货内容。依赖审计 JSON v2 单独列出该根目录，普通开发审计允许存在，发布审计必须启用 `-FailOnPlaceholder`。Snake 与 SlimeTrio 当前各自持有一份独立 Slime 占位闭包；未来正式素材替换必须切换对应 Host 引用、删除已知占位包并让发布 gate 通过，不能通过重命名或移动来规避 Placeholder 语义。

完成 Encounter 的场景退役同样不进入 BattleHUD 或规则模块。`WacomRun` 的 Map Node `Resolved` 是完成真相；`WacomApp/GameMode` 只在同一 Encounter ticket 的非撤离 Victory 成功提交后启动退役，并复用返回探索的镜头 / 后置工作双 barrier。Floor Node payload 独占静态 `EncounterDefinition`，对应 Node Anchor 的 `UWacomRunEncounterSceneBindingComponent` 独占 `SceneEnemyHostSlots + BattleEntryViewpoint` 场景映射；GameMode 持有 weak binding callback，barrier 完成后调用 Host runtime retirement，Anchor 与 binding 保留。BattleHUD 只负责完成 Destroyed 动画和清 target registry，不拥有探索场景 Actor 生命周期。

### Logical Map Graph 边界

Logical Map Graph 不新增 UE Module，继续沿用现有依赖链：

- `WacomData/Public/Map`：反射的 Journey/Floor/Node/Edge 和 typed payload，只保存静态制作真相。
- `WacomRun/Public/Exploration`：Snapshot、事件、C++-only Command/Result 与 opaque token；`FRunState` 组合持有 time/exploration runtime state。
- `WacomRun/Private/Exploration`：lifecycle、traversal、AP、Camp、Floor transition 和内容活动事务实现。
- `WacomRun/Private/Credential`：稳定 Credential ID 的校验、幂等授予与入口持有求值；`FRunState::GrantedCredentialIds` 是唯一权威状态，App/Data 只转发静态声明或只读查询。
- `WacomApp`：World 单向引用 Floor 的 `AWacomRunFloorSceneDescriptorActor`、App-private resolver、working Scene Registry、Spline、NodeAnchor、ContentHost、输入、镜头和结果表现；不得把 Actor 或世界坐标回写成规则真相。Descriptor 是需要关卡制作与 Blueprint 只读引用的反射 Actor；resolver、原子刷新状态和 Coordinator prepare/commit 仍是 Private 普通 C++。新 Run 的 Journey 选择经过 `AWacomGameMode` 的非反射虚函数：基础实现投影 `DefaultJourneyDefinition`，Editor PIE-only Preview 子类可从唯一 Descriptor 构造 GameMode-owned transient Journey，但不改变 `WacomRun` 或静态 DataAsset。
- `WacomEditor`：Journey/Floor validation、共用的 World-only read-only Scene validator、ToolMenus/validation commandlet，以及只拥有 Debug namespace 的可重复内容 builder。manifest-driven formal production seed service 位于 Editor Private，由 Floor 1/2 profile 分别提供 exact 46/47 manifest、初始字段、只读依赖与特有不变量；默认 inspect-only，只有显式 `SeedMissing` 创建缺失 DataAsset，绝不覆盖或重存已有正确 class 资产，structural inspection 守稳定身份与引用，strict seed-default comparison 只服务首次验收。Floor 1 Preview bootstrap 复用同一只读 scene validator，写权限严格缩到 Preview GameMode Blueprint 与现有 Production map 两个 Package。所有入口保持 `WacomEditor → WacomData/WacomApp` 既有方向，不产生 runtime 反向依赖。

`URunSession::Initialize(FRunInitializationParams)` 使用完整 working state，成功时一次提交角色持有区、Journey/Floor、时间、压力和探索版本并返回 `FRunInitializationResult`；失败时保留旧 Session。App 和测试都必须显式消费该结果，不保留只返回 bool 的初始化入口。

Run scene refresh 同样采用 working-state 原子提交：Snapshot 先与唯一 Descriptor 的 Floor 对齐，再完整构建 Registry 和 Coordinator plan；版本/Floor 漂移、场景身份错误或表现预检失败都保留上一代已安装状态。场景绑定收口本身没有修改 `WacomRun` Snapshot/Command/Resolution；后续 Credential 与 Journey success 切片把 SaveGame 依次升到 v4/v5，但没有新增 GameplayTag、`Build.cs` 或模块依赖，`WacomEditor` 继续使用既有 Private `WacomApp` 依赖。

需要资产制作、Details 面板或 Blueprint 只读绑定的数据类型使用反射；探索 Command、Resolution、一次性 token 和 resolver/module 保持普通 C++。`UWacomRunPathTraversalComponent` 是唯一场景移动组件，Segment / Branch / Anchor 只保存场景绑定身份，不形成第二份规则图。当前 SaveGame schema 5 保存确定排序的 `GrantedCredentialIds`、`ERunOutcome` 与独立成功摘要；v4 active/inactive 分别迁移为 InProgress/Failed。Journey/Floor/Node、节点生命周期和 Floor history 仍不保存，终态档也不能恢复为活动 Run，因此不能宣称支持地图恢复或 Continue。

Journey success 继续遵守既有依赖方向：`WacomData` 只声明 `DisplayName + SuccessTerminalNode` 静态终局，`WacomRun` 在 terminal Encounter working-state 事务中生成 Outcome/summary/末尾 event，`WacomApp` 只消费 event、展示 passive ViewData 并编排 CommonUI teardown/travel，`WacomEditor` 只做静态制作校验。Defeat 与 success 不复用状态语义；Screen 不读取 RunSession，也不调用 travel。该切片没有增加模块、GameplayTag 或依赖边。

击倒分支奖励同样沿用单向依赖：`WacomData` 的 `UEnemyPartDefinition` 保存 Aid/Destroy 静态引用并提供唯一 legacy-compatible C++ 查询；`WacomBattle` 在原有选择事务中消费查询，同时生成不含规则对象的只读奖励摘要；`WacomApp` 的 Dialog 只渲染摘要并提交选择意图；`WacomEditor` 用 General/FormalProduction profile 管制作门禁；`WacomRun` 继续只消费 `FBattleGainedCard.SourceChoice` 和 Card Definition。不得让 App 直接读取 Part Definition、让 Editor validator 进入 runtime、或为奖励预览新增 Battle/Run 状态和模块依赖。

商店卡牌强化也保持 `WacomData -> WacomRun -> WacomApp`：Data 声明不可变 Definition 链和 Shop 静态价格，Editor 校验局部结构与完整 catalog 图；Run 聚合根按 InstanceId 在 working state 中权威重算 Quote、扣费、替换 Definition 并复用 commerce/AP settlement；App 只转发 visit request、展示只读 Quote/Result 和提交带 Definition guard 的意图。Battle 继续只消费 Run 构造的当前 Definition，SaveGame v5 继续保存该 DefinitionAssetPath；没有新增模块依赖、GameplayTag 或 schema。静态 Definition 永不在运行时被改写。

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
    Private/ { Battle/, Credential/, Deck/, Events/, Exploration/, Save/, Shops/ }
  WacomApp/
    Public/ { Actors/, Core/, GameFramework/, Interaction/, UI/ }
    Private/ { Actors/, Core/, GameFramework/, Interaction/, UI/ }
  WacomEditor/
    Public/ { Authoring/, Validation/ }
    Private/ { Authoring/, Commandlets/, ContentBuilders/, Details/, Validation/ }
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

核心公共契约是 `UBattleSession + FBattleInitializationResult + FBattleCommand + FBattleResolution + FBattleSnapshot + FBattleEvent + FBattleResultPacket`。Resolver、Executor、Service 和 `BattleState` 都在 `WacomBattle/Private`，外部模块只通过公共契约交互。

`UBattleSession` 是 public facade，不承载规则实现或跨调用输出队列。`Initialize` 在 fresh working state / event bus / referenced-assets 上执行并返回 `FBattleInitializationResult`；失败保留旧战斗，成功提交一场 version 1、event sequence 0 起步的新战斗。`ResolveCommand` 在复制的 working-state 和独立 transaction event bus 上执行；失败不 commit，成功统一递增一次 `StateVersion`，并原子返回 events、presentation journal 和 post snapshot。WacomApp 与测试直接消费这两个结果 Interface，不再通过 `SubmitCommand / Consume*` split-consume Adapter。

`WacomApp` 对这两个结果只有一个表现应用 seam：App-private `FWacomBattleHUDResultApplicator`。它由 `FWacomBattleHUDRuntime` 独占，集中初始化 generation / entry gate、Session 与 state version 幂等，以及 Snapshot、Combat Log、first-person transition、presentation queue / stack 和 EndTurn plan 的应用顺序。`FWacomBattleHUDCommandController` 只构造并提交命令；`AWacomGameMode` 只发出 Begin / Attach / Release 生命周期信号。该 seam 不新增反射类型、不扩大 `WacomBattle/Public`，也不改变模块依赖。

PlayCard Evaluation 的唯一 Implementation 位于 `WacomBattle/Private/Commands` 的 `FPlayCardEvaluator`。该深层 Module 在一个 Private Interface 后集中源卡、结构性目标、运行时费用、阶段与拒绝投影：Target Probe 严格求值具体显式对象，Preview Candidate 把可选 Preview Focus 与规范化执行绑定分离，Commit Evaluation 为正式提交与 Action Preview 生成携带 `StateVersion` 的 Prepared PlayCard。这个 seam 不新增 UE 反射类型，也不扩张 `WacomBattle/Public` 的 Session / Command / Preview 或 Blueprint Interface。

PlayCard Transaction 的唯一 Implementation 位于 `WacomBattle/Private/Commands/PlayCardResolver`。`PlayCardResolver` 只消费 Prepared PlayCard，不重复阶段、源卡、目标或费用判断；Prepared 状态版本过期时必须在事件、RNG 和状态变更前拒绝。正式提交使用 formal operation adapter，Action Preview 在复制的 `BattleState` 上使用 deterministic preview adapter；两者共享 Commit Evaluation 与同一结算顺序。operation adapter 会显式透传到 Effect、ZoneHook、Passive、OnDiscard 和 EnemyAction，不使用全局或 `thread_local` 模式。

Effect Semantics 的唯一事实位于 `WacomBattle/Private/Effects/Semantics/FEffectSemanticRegistry` 的不可变 descriptor 集合。Combatant Mutation、Card Movement、Card Runtime Mutation 与 Initiative family 分别注册自己的 descriptor；运行时执行、Target Preview 与 `FWacomBattleRuleContentContract` 都读取该集合。`BattleEffectSemanticsModule` 保留 chain 执行、目标展开、magnitude 和 preview 编排，`FCardEffectChain` 继续拥有词法 scratch。新增 Effect 不再维护独立 runtime / preview / authoring tag 表。

Combatant Mutation 的唯一 Implementation 位于 `WacomBattle/Private/Combatants`。该深层 Module 在一个 typed Private Interface 后集中玩家和敌方部位的 HP、Shield、stack status、玩家 HP 阈值与运行时部位破坏边沿；Damage Facts 同时记录请求伤害、护盾吸收、实际 HP 损失和 overkill，`DamageDealt.Amount` 只投影实际 HP 损失。Operation Adapter、RNG、Effect Chain 继续策略、击倒请求阶段和 Battle End 判断不下沉到该 Module。

Status Semantics 的唯一 Implementation 位于 `WacomBattle/Private/Statuses`。它按状态与宿主解释施加、待生效手牌控制、回合物化、消费和 Poison cadence，再分别委托 Combatant Mutation、Card Runtime State 与 Initiative Timeline；它不是通用 callback registry 或 timed-status 配置表。玩家侧 Slow / Freeze / Twilight 先以 `PendingHandAfflictions` 保存，抽牌并重建 Hand 后才选择具体卡牌，物化后立即删除 pending facts。

Card Runtime State 位于 `WacomBattle/Private/Cards`，集中单卡正层数 `StatusStacks`、永久战内 `RuntimeCostModifier`、状态费用贡献、冻结限制和唯一 `FCardCostFacts` 计算。PlayCard Evaluation、Effect Preview、Snapshot 与 Action Preview 不再自行拼接费用。Initiative Timeline 位于 `WacomBattle/Private/Initiative`，是正常运行时 `CurrentInitiative` 的唯一写入口；Combatant Mutation 在部位破坏原子事务中归零先机属于明确例外。

运行时 stack status 按宿主各有唯一正层数真相：Combatant 状态位于玩家/部位 `StatusStacks`，Card 状态位于 `FRuntimeCardInstance.StatusStacks`。Snapshot 的 `Statuses` 都由对应层数表投影，不是第二份可变状态；当前未实现 timed status，也不保留平行 `StatusInstances` 容器。

Turn Lifecycle 的唯一 Implementation 位于 `WacomBattle/Private/Turns`。`FBattleTurnLifecycleModule` 通过 `StartInitialPlayerTurn / CompleteCurrentTurn` 两个 Private Interface 集中首回合事件、TurnStart、TurnEnd、敌方行动前后 BattleEnd gate、下一回合和 PresentationJournal checkpoint 顺序；`BattleInitializer` 与 `EndTurnResolver` 只保留各自的初始化 / Command Adapter 职责。该 Module 不吸收 Enemy Action、BattleEnd 或 Card Zone 算法，不新增通用 stage registry、Journal Adapter、Operation Adapter、反射类型或 Public surface。

`WacomBattle/Private/Cards/CardZoneAggregate` 是卡牌注册、六个定位容器、`FRuntimeCardInstance::Location` 和容器内顺序的唯一写 seam；它原子维护 membership/index/location 并产出 typed transition facts。`BattleCardZoneTransition` 消费这些 facts，集中 Effect、EndTurn、HandLimit、奖励和 Companion 的事件 / OnDiscard 顺序；旧 `HandZoneMoveEventService` 与 `DeckService::DiscardFromHand / ExhaustFromHand` 双入口已删除。`HandZoneService` 只解释 Hand 布局、区域和随机重排规则，`DeckService` 只编排抽牌/洗牌/PlayedPile 自然清理并委托 aggregate 写状态。

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

一卡两面继续遵守 `WacomData -> WacomRun / WacomBattle -> WacomApp`：`UCardDefinition` 持有共享身份、Battle Face v1 和可选 RunFace 静态合同；Run 的 `FCardInstance.InstanceId + Definition` 是唯一卡牌身份，face context 只是表现 / 事务语境，不创建副本、不进入 SaveGame，也不改变卡牌 Zone。`WacomApp` 的 `UWacomCardPresentationBuilder` 显式按 Battle / Run context 生成 passive ViewData；旧入口固定 Battle。Battle / Run source adapter 决定环境默认面并可附带另一面 ViewData，通用 Layer/Slot 只在 `InspectLocked` 中本地切换；App-private `FWacomFirstPersonCardInputRouter` 只统一 Slate / Viewport / PlayerController / Enhanced Input 意图、external pointer 与取消优先级，Layer/Slot 继续拥有手势、命中、动画 gate 和 matching-release reservation 真相。World Shop 是 pre-UI 高优先级 owner；Backpack / CommonUI 通过 source lifecycle 取消手牌，Router 不扩张为全局输入管理器。WBP 不读取 Definition。锁定反面不能提交 Battle 命令或 Run drop。RunFace ActionTag 的规则解释、成本与使用后处置必须由未来 `WacomRun` Room / target transaction 承担，不能写进 Builder、Widget 或世界 Actor。

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

UI 允许不同业务按切片从 C++ fallback 迁移到正式 WBP，但必须保持同一被动边界。Scene Enemy Panel 已完成 WBP-only V3.1 收敛：`WacomApp` 的 abstract CommonUserWidget 父类只保留 ViewData、稳定条目复用、语义动画路由和生命周期；App-private `FWacomBattleEnemyPartPresentationState` 把真实事实、Preview、Context 与 Local Settings 归约为 typed cue 和一次性 Vitals Frame，`FWacomBattleEnemyVitalsMaterialAdapter` 独占 MID 与参数名；布局、字体、颜色、纹理、材质和动画曲线属于 `/Game/Wacom/UI/Enemy` 的 WBP / UI assets。单/多部位共用唯一 Panel / Entry WidgetTree，不再存在 SinglePart 子 WBP、专用 DeveloperSetting 或继承绑定补丁；Snapshot 转换只位于 App-private HUD coordinator。`WacomEditor` 的 `WacomBuildEnemyUI` 只做只读合同审计，不再包含会覆盖人工布局的 Enemy UI Builder 或专用 MCP mutation toolset；运行时模块不得依赖 UMGEditor、MovieScene 制作 API或资产写入。UI 当前事实集中记录在 `WacomUI.md`，WBP 绑定合同分别记录在 `UI_Backpack_WBP_Binding.md` 和 `UI_Battle_WBP_Binding.md`。

Scene Enemy 紧凑条和双侧详情继续共享同一 ViewData 数据链。Scene coordinator 只构建一次 `FWacomBattleEnemyPanelViewData` 并推送给 Host 面板；检查协调器仅为这份只读事实附加当前选中的 `FBattlePartSlotIdentity`，不拉取 Session、不缓存第二份战斗状态。单/多部位 WBP 只改变条目排布；等宽 segment、Shield overlay、Buff overflow 和检查热区均属于表现合同。App-private `WacomBattleEnemyUILayerPolicy` 集中 compact screen layer 与 inspection viewport 的相对层级；Host Adapter 负责投影层，Widget 不拥有 viewport/input policy。WacomEditor 的 Enemy UI hit-test policy 统一校验并受控修复 `InspectHitTarget / PartList / PartNavigator / CloseButton / PartSelectButton` 到各自 WBP Root 的完整 Slate 祖先链；交互路径只能经过 `Visible` 或 `SelfHitTestInvisible`，不能用 `HitTestInvisible` 祖先阻断子控件。检查面板是 BattleHUD 私有生命周期中的非模态 viewport Widget，不新增领域 UI state；Runtime gate 统一负责 Idle 可点、拖卡/TargetSelect/Resolving/BattleEnd 自动关闭和 Escape 优先级。

Run 域 HUD 使用 `UWacomRunViewModelProvider` + `UWacomRunViewModel`；Shop / RunEvent / Backpack Screen 读取 `URunSession` Snapshot 或 ViewData；Battle UI 保持 `FBattleSnapshot` 推送模型。两者都不直接修改规则状态。

Backpack Workspace 的领域边界保持为 `WacomRun` 拥有 storage、容量、原子交易和持久规则，`WacomApp` 拥有 Scene、Carry、Settlement、Details、输入与 UI 表现。`UWacomBackpackScreen` 是 UI 意图协调者，显式 `Pointer / Flux / Pile / Delete` release intent 只能经既有 command flow 调用 Run 写入口；Passive Widget 不直接写 Session。`FWacomBackpackWorkspaceRuntime` 作为 App-private 所有权根继续持有 Registry、Visual State 和 Motion Coordinator，并新增 Gesture / Navigation / Presentation Controller：分别承接 Slate 屏幕空间阈值与捕获、`InstanceId / Zone` 稳定虚拟焦点、Hand Lens/选择冻结/Carry/几何稳定表现状态。`UWacomBackpackWorkspaceWidget` 只保留 WBP/Slate 生命周期、输入入口、Canvas 应用和单一按需 ActiveTimer。跨 `Static / CarryCache / CarryActive / Settlement` 的卡牌迁移仍统一走 App-private `ReparentCardPreservingSlate`，保持同一 `SObjectWidget -> WBP_FPCardView -> Retainer` 子树连续；真正销毁或去重继续执行正常 Widget/Slate 释放。

当前 UI 侧 ViewData / PresentationBuilder 包括卡牌展示、商店商品、RunEvent 结果、BattleEvent 表现、AppToast 和目标选择视图。它们属于 `WacomApp` 表现层。

复杂 `WacomApp` Widget 流程不要扩成跨模块 Public API。Screen / HUD 负责 View 所有权、生命周期、绑定和重建；购买、选择、移动、删除、确认弹窗、Toast 和访问结束等编排应收口到 `WacomApp/Private` 的 coordinator / command flow helper。

`WacomApp/Public` 的 Widget 头文件只保留 WBP 绑定、运行时生命周期和玩家真实操作所需的 API。测试口不得以 Blueprint / Public Widget API 的形式暴露，尤其不得新增 callable 的 `ForTest`、`ForAutomationTest`、`OverrideForTest` UFUNCTION 或 Blueprint 可见测试函数。测试访问的优先级如下：

1. 在 `WacomTests` 中定义 tests-only probe subclass，借由真实生命周期和公开玩家意图入口观察行为。
2. 如生产代码确有扩展点需求，提供 protected production seam；该 seam 必须有运行时语义，不能只为测试绕过流程。
3. 最后才使用 automation-only private friend / test-access；这类入口不能是 `UFUNCTION` 或 Blueprint 可见 API，并应尽量藏在 `WacomApp/Private` 或测试模块内。

测试不能锁死未来 WBP、CommonUI 生命周期或 MVVM 重构的内部形状。自动化测试应验证玩家可观察行为、Snapshot / ViewData 输出和命令副作用，而不是依赖某个临时 C++ fallback 布局或 Public Widget 测试捷径。

## 10. 验证入口

普通 Run world 交互入口统一通过 `IWacomWorldInteractable`：

```text
玩家进入交互半径
-> Actor 注册为 CandidateInteractable
-> PlayerController 选择最近且 CanInteract 的对象
-> 按 E 调用 TryInteract
-> Shop / RunEvent 等进入对应领域入口和 UI
```

Encounter 不再属于上述世界交互：Run Path 的 `CompleteTraversal` 与目标 Anchor 应用成功后广播 typed arrival，App 通过 Node payload + Encounter scene binding 自动调用 `TryEnterBattle()`；撤离或启动失败会在 PlayerController 的本次 Run 内按 `MapNodeHandle` 保留手动重试要求，离开节点只隐藏提示，二次 arrival 只恢复 E 提示而不自动开战。E 重试只认当前逻辑节点，不依赖 Candidate、Overlap 或点击。当前普通世界交互对象包括 ShopTrigger、RunEventTrigger、Pickup 与 KeyChest 等。

## 11. 当前已落地骨架

已经落地的骨架包括：

1. `WacomBattle`：BattleSession、Command、Snapshot、Event、ResultPacket、击倒事件、奖励卡、效果执行器。
2. `WacomRun`：RunSession、背包、SpecialZone、负重、经验/压力、商店、RunEvent、战斗结果回传、SaveGame schema。
3. `WacomApp`：GameMode、PlayerController、世界交互接口、CommonUI 层级、探索 HUD、BattleHUD、Backpack / Shop / RunEvent Screen、AppToast。
4. `WacomData`：卡牌、敌人、角色、商店、RunEvent 静态定义和生成内容。
5. `WacomEditor`：内容 commandlet、Shop / RunEvent / Map Data Validation、Run Floor Scene validator、Debug-only Run fixture builder，以及 exact-manifest、initial-only 的 Floor 1 Production DataAsset/scene seeder；Production seeder 只创建缺失 package，不覆盖已有资产或拥有后续人工调参。Floor 1 Preview bootstrap 另以双 Package allowlist 提供 PIE-only GameMode/PlayerStart 接线，不是正式 Journey builder。
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
- `Niagara`：用于 App 世界目标确认、伤害、目标预演与部位破坏表现；Graph 写入和编译只在 `WacomEditor`，运行时仍不构成规则依赖。

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
