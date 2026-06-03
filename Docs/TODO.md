---
type: task-index
scope: wacom-short-term
status: active
updated: 2026-06-03
tags:
  - wacom/todo
  - wacom/docs
---

# TODO 索引

> [!info] 本文职责
> 这里只放短期任务入口和跳转，不写规则真相。当前规则看领域文档；长期方向看 [[Roadmap]]；临时写法看 [[TechDebt]]；待确认问题看 [[Questions]]。

> [!warning] 执行约束
> 标记为 `Blocked` 的任务不要直接写死设计口径。先在 [[Questions]] 或对应领域文档中收口，再进入实现。

## 文档分工

| 文档 | 职责 | 维护规则 |
|---|---|---|
| [[Roadmap]] | 未实现功能、后续方向、可按阶段推进的内容扩展 | 新功能还没有进入短期实现前，先放这里 |
| [[TechDebt]] | 临时写法、兼容字段、临时决定、正式替代方案 | 代码里出现 `TODO(技术债)` 时同步到这里 |
| [[Questions]] | 会影响规则、策划口径或长期架构的待确认问题 | 不在代码里静默写死这些问题 |
| 本文 | 短期任务索引和跳转 | 不承载长说明，避免重新变成大杂烩 |

## 当前推进顺序

1. 先收口 P0 规则问题。
2. 再推进地图 / 节点服务。
3. 地图口径确定后，再接击倒分支、RunEvent / Shop 的节点生成与存档恢复。
4. UI WBP、表现 polish 和存档恢复按 Demo 范围穿插推进。

## P0 策划确认

- [ ] **确认减速、暮气、冻结等状态的数值公式和触发时机**
  - 状态：`Blocked: 策划确认`
  - 归属：战斗规则
  - 入口：[Questions: 状态与触发时机](./Questions.md#questions-status)

- [ ] **明确击倒事件 Aid / Withdraw / Destroy 的正式分支效果**
  - 状态：`Blocked: 策划确认`
  - 归属：战斗 / Run
  - 入口：[Questions: 击倒与战后结算](./Questions.md#questions-knockdown)

## P1 近期实现候选

- [x] **UI Ownership / BattleHUD 私有 coordinator 拆分**
  - 状态：`Done: V0-CZ 已整理 UI Ownership Map；V0-DA 已抽出 SceneEnemyTargetCoordinator；V0-DB 已抽出 PresentationCoordinator；V0-DC 已抽出 CombatLogController；V0-DD 已抽出 FirstPersonHandBridge；V0-DE 已抽出 CardDetailController；V0-DF 已补 BattleHUD 私有 helper 合同回归安全网；V0-DG 已把 BattleHUD 测试装配收口到 WacomTests 私有 harness`
  - 归属：UI 架构 / App
  - 入口：[WacomUI: UI Ownership Map](./WacomUI.md#wacomui-ui-ownership-map) / [TechDebt: UI 架构债](./TechDebt.md#techdebt-ui-architecture)
  - 说明：当前 UI 采用 “Shell 集中 + Screen coordinator 分域负责”：`UWacomGameUIManagerSubsystem + PrimaryLayout` 只管 CommonUI 根层，`GameMode / PlayerController / ScreenRouter / InputCoordinator` 只管流程和输入，具体 Screen 负责自己的 ViewData 与玩家意图入口。`BattleHUD` 仍是战斗 UI 命令出口。V0-DA 后 scene enemy target registry / hover probe / prediction 与 status badge sync 已收口到私有 coordinator；V0-DB 后 presentation queue / card stack / turn-boundary barrier 已收口到私有 coordinator；V0-DC 后 combat log history / trim / recent feed sync / readable log 输出已收口到私有 controller；V0-DD 后 first-person battle hand runtime sync、delegate bind/unbind、drag preview/release、drop intent、Card target affordance、camera look override 和 transition hint cache 已收口到私有 bridge；V0-DE 后旧手牌和 first-person 共享 card detail panel / motion / source guard / viewport-canvas 定位已收口到私有 controller；V0-DF 后 `Wacom.UI.Battle` 通过 HUD/test receiver 合同测试锁住这些私有 helper 不泄漏成 public / Blueprint API；V0-DG 后重复 HUD 装配由 `FWacomBattleHUDTestHarness + UWacomBattleHUDDetailTest` 负责。拆分都留在 `WacomApp/Private`，测试 harness 留在 `WacomTests/Private`，不新增 Blueprint API，不改变 `UBattleSession` 或战斗规则层。建议先暂停继续拆 BattleHUD，除非 HUD 剩余职责再次明显膨胀。

- [x] **交互目标系统：统一 target handle、Provider 接口、World 命中**
  - 状态：`Done`
  - 归属：Core / App
  - 说明：`WacomCore/Public/Types/WacomInteractionTargetTypes.h`（Handle + Kind + TargetTag + StableTargetId）、`WacomApp/Public/Interaction/WacomInteractionTargetProvider.h`（接口）、`WacomApp/Public/Components/WacomInteractionTargetComponent.h`（通用组件）、`UWacomBattleEnemyPartWorldTargetBridgeComponent`（Battle 敌方部位桥接）、`TryRouteBattleSceneTargetClick` 已使用 Provider 路径。Card / Zone TargetKind 的命中来源和规则层 Resolver 为后续任务。

- [x] **交互目标系统：Battle scene enemy current Host registry**
  - 状态：`Done: V0-CR 已把 Battle 场景敌方部位从 BattleHUD 全 World 扫描收口到 BattleTrigger.SceneEnemyHost 显式绑定`
  - 归属：App
  - 说明：V0-CR 后 `ABattleTriggerActor.SceneEnemyHost` 是推荐制作入口；`AWacomGameMode::EnterBattle()` 会把当前 Trigger 的 Host 传给 `BattleHUD`，HUD 只同步该 Host 下 attached `AWacomBattleEnemyPartActor` 的 bridge。hover、prediction、TargetCue、TargetSelect click 和 first-person 拖卡 release 都会被当前 Host registry 过滤；同关卡其他 Host 的部位不会参与当前战斗。缺 `SceneEnemyHost` 只报 warning，旧 Trigger 仍可进入战斗并保留 `EnemyInfoBar` fallback/debug，但场景 PartActor 不绑定。后续如果 Run 可交互 Actor 数量继续增多，可再做通用组件注册 / 反注册 registry。

- [ ] **战斗表现队列：快速连出后的视觉追赶 polish**
  - 状态：`Ready: V0-CL 后续表现优化`
  - 归属：App / UI
  - 说明：V0-CI 后普通战斗事件 presentation queue 不再阻塞 PlayCard，旧事件表现可能落后于最新 Snapshot。V0-CJ/V0-CK 后 BattleHUD 使用常驻可滚动 Battle Combat Log 补足玩家可读反馈，旧日志抽屉和旧 EventToast 单条提示框都不再挂在 HUD 主路径。V0-CL 后新增只读 BattlePresentationStack 小卡堆叠，并把 Wait / EndTurn 改成 turn-boundary barrier：stack 未清空时先 pending，期间锁住继续出牌和目标选择。V0-CM 后表现栈收成纯小卡堆，只显示整体缩放的完整卡面，不再显示卡名、目标、数量、溢出文字或黑色底座；卡牌到达 boundary 后先播放短 exit motion，再移除并释放 pending turn-boundary。V0-DB 后 presentation queue、stack 和 pending barrier 的运行时状态已收口到私有 `FWacomBattleHUDPresentationCoordinator`。后续可按体验需要加入 cue 合并、旧表现跳过、速度压缩、正式动画、stack 入场 polish、动画回放或规则层 command batch id。

- [ ] **战斗规则内容化：按 authoring matrix 扩展正式卡牌 / 敌人内容**
  - 状态：`In Progress: 已建立 authoring matrix、validator、transient runtime fixture、真实生成资产 smoke 和测试 helper 收口`
  - 归属：Battle / Data / Editor
  - 说明：当前制作合同来源是 `FWacomBattleRuleContentContract`，Data validator 负责拦截未接入配置；`Wacom.Battle.RuleContentMatrix` 证明矩阵允许项能进入 `UBattleSession` 结算；`Wacom.Battle.GeneratedStarterContent` 证明真实生成资产没有字段漂移。生成内容测试路径集中在 `FWacomGeneratedBattleContentAssets`，Snapshot/Event 查询集中在 `FWacomBattleFixture`。后续可继续做正式掉落 / 奖励池、更多敌人包、卡牌平衡和正式卡组入口；如果需要 `OnDraw`、`OnTurnStart/End`、敌人更多自我效果、改先机类意图、更多状态公式或正式暮气被动，需要先接运行时 resolver，再更新合同、文档和验证测试。

- [ ] **战斗场景敌人 Actor：承接敌方部位表现与先机预测**
  - 状态：`In Progress: V0-CN 已新增 Host + PartActor facade；V0-CO 已补预测 readiness 诊断；V0-CP 已补稳定 hover target 状态；V0-CQ 已补玩家可见 Cost 推进预测；V0-CR 已把绑定收口到当前 BattleTrigger.SceneEnemyHost；V0-CS 已补 PartActor 常驻状态 Badge 并让有 Host 的战斗默认隐藏 EnemyInfoBar；V0-CT 已打磨 Badge 可读 fallback、Host 稳定错开、Prediction 显示上移和破坏态弱化`
  - 归属：App / UI / Battle World Target
  - 说明：V0-CN 后推荐关卡使用 `AWacomBattleEnemyActor + AWacomBattleEnemyPartActor` 摆放战斗场景敌人：Host 只做分组 / debug / validation，PartActor 持有安全 facade、Visibility 命中体、可见体、screen-space 预测 Widget、screen-space 状态 Badge、`UWacomInteractionTargetComponent` 和 `UWacomBattleEnemyPartWorldTargetBridgeComponent`。V0-CQ/V0-CS/V0-CT 后 PartActor / bridge debug summary 已输出 runtime part facts、HP / 护盾 / 状态 / 意图 / 拖卡或 TargetSelect prediction 输入、hover target facts、Badge draw size / relative location / scale / opacity / stagger，以及 `StatusBadgeVisible / PredictionVisible / PredictionMode / PredictionBadgeOffsetActive / PredictedInitiative / PerfectCandidate / ActionRisk / PredictionReject`；Host summary 汇总 bound parts、runtime initiative total、hovered parts、visible prediction count、visible status badge count 和 badge layout applied count。V0-CR 后每个 `ABattleTriggerActor` 显式绑定自己的 `SceneEnemyHost`，当前战斗只同步该 Host attached parts，避免多组敌人互相串 hover / prediction / cue / drag release。V0-CS 后当前 Host 有效时 `EnemyInfoBar` 默认隐藏，场景 PartActor Status Badge 成为主要敌方状态阅读入口；缺 Host 时 `EnemyInfoBar` 仍作为 2D fallback 和调试入口。V0-CT 后 Status Badge fallback 让意图独占一行，Prediction 显示时上移不覆盖主状态，破坏态 Badge 变淡但不隐藏。普通 hover 显示当前先机，拖起卡牌或 TargetSelect hover 时显示 `当前先机 -> 预计先机`、完美释放候选和部位行动风险；迅捷卡显示先机不变，非法目标显示 reject reason。后续切片可做 visual polish：材质描边、贴近部位 tooltip、正式美术样式、更清楚的风险动效和 Status Badge 美术替换。V1 只做 Cost 推进风险预览，不模拟完整卡牌主效果、ZoneHook、破坏、改先机或中毒后的最终结果。

- [ ] **交互目标系统：Card-World / Card-Card drag resolver / Zone 命中来源接入**
  - 状态：`Done: V0-CG 已把 Run world card interaction 从单 GoldReward 硬切到通用 Rewards payload；V1 支持 Gold 和 Card，KeyChest 推荐入口仍是 CardInteractionDefinition > 手填 receiver fallback。后续正式内容、动画、SaveGame、复杂筛选、掉落表和更多奖励类型仍待切片`
  - 归属：App / Battle
  - V0-BT：RunEvent / Shop / BattleTrigger 摆放实例接入 Validate Map/Level。缺 `PersistentId`、缺关键配置或空商店商品是 error；重复 `PersistentId` 是 warning；推荐 BP 默认资产空配置仍有效。
  - 说明：First-person hand 的 Card target 已由 `UWacomFirstPersonCardLayerSlotWidget / LayerWidget / AnchorComponent` 暴露为 `FWacomInteractionTargetHandle(TargetKind=Card)`，使用 visual slot screen position，且不要求目标卡可打。V0-Z 已把 first-person 源卡接入 hold inspect、无目标卡向上拖出释放提交、目标卡 aim arrow 和 world enemy part release 提交；V0-AA 保留 UMG mouse capture，同时用 drag pointer 驱动 BattleCameraLook override 和 widget-position world probe，避免拖拽时视角冻结或 target probe 读旧 cursor。V0-AB 增加释放前 affordance：无目标卡 armed 显示 commit-ready，合法/非法 world target 改变箭头颜色并触发场景 bridge transient preview，Card target 显示 probe 或确认色。V0-AC 后 BattleHUD 用 `ResolveFirstPersonCardDropIntent()` 统一 preview 和 release 语义。V0-AD 已让合法 `TargetMode=HandCard` 源卡拖到另一张手牌解析为 `PlayCardCardTarget` 并提交 `TargetCardInstanceId`，用于指定手牌加费 / 减费。V0-AE 已支持 `Effect.Card.DiscardSelected / ExhaustSelected + Target.SelectedHandCard`，可以把另一张普通手牌移入弃牌堆 / 消耗区。V0-AF 已统一手牌弃牌 / 消耗事件：真正弃牌会发 `CardDiscarded` 并触发目标卡 `OnDiscard`，消耗会发 `CardExhausted` 但不触发 `OnDiscard`；手牌上限仍保留 `HandLimitDiscarded` 兼容事件。V0-AG 后 Battle 中 `UBattleSession::ValidateTargetWithCard()` 提供可解释拒绝原因，`CanTargetWithCard()` 转调 validation；拖拽 HandCard 源卡时 first-person hand 会整手牌显示合法 / 非法 Card target affordance，源卡使用 visual-only selected-source 姿态但不进入旧 `TargetSelect`。V0-AH 后普通手牌 / 左右手锚点资格收口到 `UCardDefinition::HandCardTargetFilter`，显式 filter 优先，旧资产按 Add/ReduceCost 允许锚点、Discard/ExhaustSelected 拒绝锚点的兼容推断保持行为。V0-AI 后 filter 支持 `RequiredTargetKeywords / BlockedTargetKeywords`，目标有效关键词包含定义关键词和战斗临时关键词，左右手锚点允许后也同样参与 keyword 条件。V0-AJ 后 Run / 探索 Actor 可通过 `UWacomRunWorldInteractionTargetBridgeComponent` 标记为 `Interaction.Target.Run.Object`，PlayerController 可 probe 并播放轻量 preview。V0-BD 到 V0-BH 已依次让 RunEvent、Shop、BattleTrigger 支持远距离 mouse click / hover；V0-BI 把 hover 视觉信号收口到 RunWorldBridge；V0-BJ 把 PlayerController click、hover 和 probe preview 共用到同一个 clickable resolver。V0-BK 后 `AWacomRunPickupActor` 成为第一个正式 Pickup：E 键和左键都调用 `URunSession::CollectGoldPickup()`，使用 `CollectedPickupIds` 防重复结算，并默认显示 `点击拾取` hover、scale preview 和金币 toast。V0-BL 后 Pickup 增加 `ConfigureDebugGoldPickupSample()`、配置健康度、重复 `PersistentId` warning、renderable visual 和 click target debug summary，方便关卡大量摆放时排查 ID / 金币 / visual 配置。V0-BM 后 `AWacomRunCardPickupActor` 支持固定单卡奖励：E 键和左键都调用 `URunSession::CollectCardPickup()`，与金币 Pickup 共用 `CollectedPickupIds`，成功后获得卡牌、显示卡牌获得 Toast，并提供 `ConfigureDebugCardPickupSample()` 与卡牌配置 debug summary。V0-BN 后 `AWacomRunPickupActorBase` 统一金币 / 卡牌 Pickup 的组件、hover/click、E 键候选、lifecycle、shared debug 和跨类型 duplicate id 诊断，具体奖励结算仍留在金币 / 卡牌子类。V0-BO 后金币 / 卡牌样例配置按钮共用 Base authoring helper，统一刷新 TriggerSphere、ClickBounds、stable id、prompt 和 lifecycle，并验证 `BP_WacomRunPickupActor / BP_WacomRunCardPickupActor` 作为只配置外观的推荐摆放入口。V0-BP 后 `UWacomRunPickupDefinition` 提供数据驱动固定奖励定义，`AWacomRunRewardPickupActor` 可用同一场景 `PersistentId` key 拾取 Definition 中的金币或固定单卡奖励。V0-BQ 后 `DA_Pickup_DebugGold3 / DA_Pickup_DebugPoisonFang` 由 `WacomRegenerateContent` 生成，RewardPickupActor Details 按钮可快速绑定这两个样例；专用金币 / 卡牌 Actor 仍保留为 fallback 和快速 debug 入口。V0-BR 后 `BP_WacomRunRewardPickupActor` 成为正式推荐摆放入口，默认不烘焙 `PersistentId` / `PickupDefinition`，每个实例配置唯一场景 ID 和 Definition。V0-BS 后 Pickup 摆放实例接入 Validate Map/Level：缺 ID / 奖励配置是 error，重复 `PersistentId` 是 warning，推荐 BP 默认空配置仍有效。V0-BV 后旧 `AWacomDebugChestActor` 已移除，新的 `AWacomRunKeyChestActor + UWacomRunWorldCardDropReceiverComponent` 接入第一条 Run card-to-world 原型：拖入 `DA_Card_DebugKey` 时消耗精确钥匙 instance、奖励 3 金币，并写入 `CompletedRunWorldInteractionIds`；普通 E / 左键只提示，不直接开箱。V0-BW 后 KeyChest 内部组件继续隐藏 Collision / BodyInstance Details，关卡制作改为编辑 Actor facade 字段，并由 `RefreshAuthoringState()` 同步 TriggerRadius、ClickBoundsExtent、VisualMesh、VisualScale、VisualRelativeLocation、receiver 和 stable id。V0-BX 后 KeyChest 摆放校验会拒绝缺 ID、无正向卡牌筛选、只填 BlockedKeywords 或 reward 配置无效，并在 debug summary 中稳定输出 ConfigReason、Duplicate、PositiveFilter 和 receiver keyword / reward facts。V0-BZ 后 KeyChest 增加 completed facade：监听 RunState 变化后按 `CompletedRunWorldInteractionIds` 切换 `CompletedVisualMesh / CompletedVisualScale / CompletedVisualRelativeLocation`，完成后仍保留 hover / 左键 / E 键命中并只显示已打开提示。V0-CA 后 Run world card drop release 失败会按原因显示 AppToast，preview 不弹 Toast，debug summary 记录 `FailureToast`。V0-CB 后失败 Toast 文案由 receiver 的 completed/rejected/config/source/generic prompt contract 生成，debug summary 记录 `ToastSource / FailureToast`；后续门、机关、祭坛、NPC 等 receiver 不应给 PlayerController 加类型分支。V0-CC/V0-CD 后 `UWacomRunWorldCardInteractionDefinition` 成为通用 receiver DataAsset，`DA_RunWorldCardInteraction_DebugKeyGold3` 由 `WacomRegenerateContent` 生成，`ConfigureDebugKeyChestInteractionDefinitionSample()` 可把 KeyChest 实例迁到通用 Definition 推荐路径。V0-CF 后旧 KeyChest 专用 Definition 链路已删除，KeyChest 只保留通用 Definition 和 receiver fallback。V0-CG 后 Run world card interaction request/Definition/receiver fallback 全部使用 `Rewards` 数组，成功时可一次发放 Gold + Card 并只广播一次，PlayerController 按 reward payload 展示金币或卡牌获得 Toast。V0-AK 到 V0-AQ 的 Run first-person card source、menu lease、Run menu drop intent 和 RunEvent card payment 仍按原合同工作；RunEvent 的 V0-AR 到 V0-AY 支付状态、需求列表、后果预览和 outcome toast 仍不变。后续仍待接入掉落表、Pickup / 世界交互 SaveGame、更多奖励类型、费用/卡牌类型/区域/伙伴食物专用属性等复杂筛选、多卡支付、任意卡支付、更强 world outline/material polish、鼠标旁 tooltip、专用飞牌动画、真实飞牌轨迹和常驻结果面板。

- [ ] **交互目标系统：规则层 Target Resolver**
  - 状态：`Ready: Card-to-Card / Run / Zone 规则接入时`
  - 归属：Battle / Run
  - 说明：域层 Resolver 根据 `TargetKind` 判断当前卡能否作用到目标，替代硬编码的 TargetMode 过滤。

- [ ] **接入击倒事件实际分支、奖励卡差异化和节点事件联动**
  - 状态：`Blocked: P0 击倒口径`
  - 归属：战斗 / RunEvent
  - 入口：[Roadmap: 击倒事件扩展](./Roadmap.md#roadmap-knockdown)

- [ ] **推进地图系统：节点、通道、迷雾、撤离回路、地图状态**
  - 状态：`Ready: 需先定模块边界`
  - 归属：Run / 地图
  - 入口：[Roadmap: 地图与探索](./Roadmap.md#roadmap-map)

- [ ] **推进 RunEvent：随机事件池、更多条件效果、地图节点生成、存档**
  - 状态：`Blocked: 地图节点口径`
  - 归属：Run / Data / App
  - 入口：[Roadmap: 探索事件](./Roadmap.md#roadmap-runevent)

- [ ] **商店正式化：随机商品池、价格公式、正式 WBP、存档接入**
  - 状态：`Blocked: 地图节点 / 存档口径`
  - 归属：Run / Data / App
  - 入口：[Roadmap: 商店](./Roadmap.md#roadmap-shop)

- [ ] **背包正式 WBP、拖拽 polish、必要时做增量刷新**
  - 状态：`Ready: 美术 / WBP 工作`
  - 归属：UI / Run
  - 入口：[Roadmap: 背包 UI](./Roadmap.md#roadmap-backpack-ui)

- [ ] **RunEvent 正式 WBP 外观接入**
  - 状态：`Ready: 美术 / WBP 工作`
  - 归属：UI / RunEvent
  - 入口：[RunEvent WBP 绑定清单](./UI_RunEvent_WBP_Binding.md)
  - 说明：V0-AU 已提供 `ChoiceButtonWidgetClass / PaymentDropTargetWidgetClass / PaymentChoiceMinDesiredWidth` authoring surface；V0-AV 暂不创建默认资产。后续美术可制作 `WBP_RunEventScreen / WBP_RunEventChoiceButton / WBP_RunEventPaymentDropTarget`，只替换外观、布局和 preview 表现；C++ fallback 继续负责逻辑、动态选项、ZoneId 写入、menu lease、drop intent 和 RunEvent 事务。

## P2 后续清理

- [x] **用编辑器迁移旧内容目录并清理 `_GAME` 临时蓝图目录**
  - 状态：`Done: Content Organization V1-A`
  - 归属：Content / App / Data
  - 入口：[Content Organization](./Content_Organization.md)

- [ ] **战斗手牌表现升级：扇形布局、拖拽出牌、3D 目标选择**
  - 状态：`Blocked: HD-2D 表现方案`
  - 归属：UI / 战斗表现
  - 入口：[Roadmap: 战斗 UI](./Roadmap.md#roadmap-battle-ui)

- [x] **First-person card layer polish：详情面板跟随卡牌与层级修正**
  - 状态：`Done: V0-H 已接入独立 viewport popup host；V0-I 已用 BattleHandPresentationMode 收口第一人称手牌配置入口；V0-V 已加入共享详情读牌 motion；V0-DE 已把战斗详情 runtime state 收口到私有 CardDetailController`
  - 归属：UI / 战斗表现
  - 说明：旧 `UHandPanel` 详情继续使用 BattleHUD 内部 `CardDetailLayer`；first-person hover 详情使用独立 viewport panel。两种战斗详情 host 共用 `bEnableCardDetailReadabilityPolish` 下的短 hover delay、淡入淡出、轻量 scale、位置平滑跟随和贴边 side hysteresis；V0-DE 后 source guard、motion state、定位和 teardown 已由 `FWacomBattleHUDCardDetailController` 持有，HUD 保留 WBP 绑定、配置和 GC 引用。TargetSelect、命令提交、BattleEnd、Session 切换和 runtime hand 清理仍强制隐藏详情。战斗手牌入口由 `LegacyHandPanel / FirstPersonHandWithLegacyFallback / FirstPersonHandOnly` 三种模式控制。

- [ ] **First-person card render quality：扇形布局微调与专用卡面规范**
  - 状态：`In Progress: V0-K projected basis 已回退；V0-L 默认改为 Authored2D hand layout solver；V0-M 默认允许手牌锚点 SoftClamp 离屏；V0-N 已加入 hand center screen smoothing；V0-O 已加入 card slot motion polish；V0-P 已加入 slot motion lifecycle diagnostics / self-repair；V0-Q 已加入 event-aware card transitions；V0-R 已加入 pending / TargetSelect focus polish；V0-S 已加入 playable / hover / press feedback polish；V0-T 已加入 hand layout preset / tuning profile；V0-U 已加入 transition origin / card movement readability；V0-V 已加入 hover detail / inspect readability polish；V0-W 已加入 play commit / target impact readability；V0-Z 已加入 hold inspect + card drag / aim commit；V0-AA 已加入 drag camera look continuity；V0-DJ 已将 WBP_FirstPersonCardView 透明 bleed 画布和 296x420 主体交互命中解耦；V0-DL 已加入 Authored2D 主体底部视口可读保护；Retainer TypeName 显隐问题已在干净 ds cardview2 worktree 复现，固定 visual canvas 方案因破坏扇形排布不采用；CostDigitImage 单图绑定仍无法消除 Retainer 内费用小图标运动采样抖动，后续不要再从 CostDigitsHost/动态子控件方向处理`
  - 归属：UI / 战斗表现
  - 说明：继续保留 `WBP_FirstPersonCardView` 的 RetainerBox、透明留白、内部缩放和贴图采样规范。外层 bleed 画布可大于主体以完整渲染右下身材 / 耐久装饰，但排布、hover、click、drag 起手和 Card target probe 只按内部 `CardSizeBox` 主体命中，缺绑定时回退 296 x 420。`bKeepAuthoredCardBodyBottomInViewport` 默认保护主体底部不被视口裁掉，避免 TypeName / 类型文字只能靠 hover 上浮才露出。费用图标当前只保留 `CostDigitImage` 绑定；已确认固定 Brush 尺寸和单 Image 路径仍不能完全解决 Retainer 内小图标随整卡运动 / 旋转时的采样抖动。后续若继续处理，应优先评估把 `CostDigitImage` 像 TypeName 一样移到 Retainer 外、做更严格的像素对齐 / 采样材质，或改为专用 Slate / 材质渲染路径。V0-Z 默认保留轻点出牌；hold delay 后读牌居中放大并显示详情，无目标卡向上拖出释放提交，有目标卡拉箭头到合法敌方部位释放提交，仍复用 BattleHUD / BattleSession 命令路径。后续只有在美术反馈需要时再微调扇形参数、下坠、层级、hover/pending 姿态、slot motion 速度、commit pulse、drag/aim 读牌姿态或角度 clamp；也可单独做多环境 preset / 战斗状态自动切 preset。不要为了抗锯齿牺牲当前手牌排布表现，也不要回退到 3D Presenter / RenderTarget 路线。

- [ ] **存档系统恢复：Bootstrap 读盘、PauseMenu Save、MainMenu Continue**
  - 状态：`Blocked: Demo 范围确认；V0-CH 已确认 MainMenu PIE travel 卡顿/输入异常不是存档恢复，当前 bSaveSystemEnabled=false 仍保持新 Run`
  - 归属：Run / App
  - 入口：[Roadmap: 存档恢复](./Roadmap.md#roadmap-save)

- [ ] **MainMenu V2 资产重做评估**
  - 状态：`Deferred: V0-CH 先修现有 L_MainMenu package-path deferred travel；只有现有主菜单资产后续确认不可维护时再新建 L_MainMenu_V2`
  - 归属：App / UI
  - 入口：[Roadmap: App 壳层与菜单](./Roadmap.md#roadmap-app-shell-and-menus)

- [ ] **清理 UI MVVM 迁移尾项：WBP ViewBinding、逐步移除 C++ 手动 SetText fallback**
  - 状态：`Ready: WBP 化后清理`
  - 归属：UI 架构
  - 入口：[TechDebt: UI 架构债](./TechDebt.md#techdebt-ui-architecture)

## 维护规则

> [!tip] 新任务放哪
> - 1-2 轮内准备做：放本文。
> - 只是未来方向：放 [[Roadmap]]。
> - 当前实现里已经存在的临时方案：放 [[TechDebt]]。
> - 需要你或策划确认口径：放 [[Questions]]。

> [!note] 迁移说明
> 原 `TODO.md` 的未实现功能已迁入 [[Roadmap]]；临时写法、临时决定、兼容入口已迁入 [[TechDebt]]；待确认规则问题已迁入 [[Questions]]。已经正式化到 `WacomRun.md`、`WacomBattle.md`、`WacomApp.md`、`WacomUI.md`、`WacomData.md` 的实现，不再在这里重复追踪。
