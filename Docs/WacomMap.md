---
type: domain-design
scope: wacom-map
status: core-implemented
updated: 2026-07-17
tags:
  - wacom/map
  - wacom/run
  - wacom/exploration
---

# WacomMap

> [!info] 本文职责
> 本文记录 Logical Map Graph、Map Node Lifecycle、Map Travel、Floor Transition 与 Floor Exposure 的已确认规则。Run 时间、压力、背包和事件事实仍见 [WacomRun.md](./WacomRun.md)；Run Path 移动与场景表现见 [WacomApp.md](./WacomApp.md)。

> [!warning] 当前阶段
> 地图静态合同、原子初始化、节点生命周期、Traversal Ticket、同层 Map Travel、Action Point、节点活动、Camp、不可逆跨层、Run Path 场景适配、当前 Floor Map Screen、制作校验、Debug Journey 与 Journey 成功结算/主菜单交接已落地。正式 `Journey.Main.01` 的三层各 20 节点/21 边图、内容槽、稳定身份和 `26–28 / 44–46 AP` 总节奏均已冻结；蛇印/蜕印资格使用独立持久 Credential，Floor 1 的敌人、奖励、事件、Shop 与 `38 core + 8 branch reward = 46` 个未来资产合同也已冻结。但尚未创建 Production DataAsset 或世界关卡，`L_Exploration` 继续承接 Authoring baseline，不是正式 Floor 1。生产实现仍受三层 46 个节点 Definition 的实际制作、各层支持内容资产和资产权威审计阻塞；真实 Golden Path PIE 须在 Production 资产轮完成。

## §1 两层图合同

探索使用两套不同的图：

- Logical Map Graph 是活动旅程中的离散规则图，维护 Floor、Map Node、有向连接、揭示、完成、可达、传送和跨层推进。
- Run Path Traversal 是 `WacomApp` 中的局部第一人称轨道表现，只维护当前场景的 Spline、分支意图、前后移动、镜头和到达边界事实。
- Map Node 的 UI 坐标不需要对应世界坐标；一条 Map Edge 可以由一段手工 Tunnel、一个局部岔路场景或直接场景转场表现。
- Run Path Component 不读取卡牌门槛、Floor、压力、节点完成状态或 Map Travel 合法性；这些事实只由 `URunSession` 的 Snapshot / Command / Resolution 提供。

## §2 有向连接与局部后退

- Logical Map Graph 的连接是有向的，决定首次探索方向。
- Run Path 内允许玩家使用 W / S 沿当前局部轨道前进或后退。
- 局部后退不构成反向 Map Node 迁移；返回已经完成的宏观节点使用 Map Travel。
- 到达未选择的岔路 Decision Gate 时平滑停止；选择合法分支后继续。

通道采用两阶段提交：分支意图先经当前 Floor Scene Registry 验证 Path、source Anchor、target Anchor 和预期内容 Host，随后规则层返回一次性 Traversal Ticket；只有到达终点并再次验证场景绑定后才提交 target Node。中途退回起点提交 Cancel，逻辑节点始终留在 source。Complete 失败会显式补偿 Cancel 并回源；规则成功提交后禁止回源，目标 Actor 同帧失效时使用 Begin 阶段缓存的 target Transform，保证逻辑与画面停在同一侧。

内容 Host 在场景中可见不等于可提交。带 `UWacomRunMapNodeBindingComponent` 的 Host 必须等待 target Node 完成上述 Complete、成为正式 current node，并确认当前没有其他探索事务后才开放交互。当前 BattleTrigger 已执行此门控：玩家在 Path 中途点击 Encounter 只得到“请先完成当前行动 / 抵达节点”的提示，不会提前开始 Encounter，也不会改变 Run 表现状态；Shop、RunEvent 与 Treasure 后续应沿用同一 Host availability 合同。

## §3 节点揭示与生命周期

Map Node Lifecycle 固定为：

```text
Hidden → Revealed → Visited → Resolved
```

- 进入 Floor 时只揭示当前节点和直接相邻节点。
- Floor Entrance 与 Boss 从进入 Floor 起显示远处轮廓；具体路径、门槛和内容在正常探索后揭示。
- `Visited` 只表示玩家到达过，不等于内容完成。
- 普通节点在战斗、事件、搜刮或其它内容规则成功结束后进入 `Resolved`。
- 商店、营地等安全且可重复节点可以在首次安全进入后进入 `Resolved`；重复使用仍按各自规则消费资源或时间。
- 没有活动战斗、RunEvent 或剧情事务时，玩家可以离开尚未 Resolved 的普通死胡同，例如暂时打不开的宝箱或未满足的卡牌门槛。

## §4 Map Travel

- Map Travel 只允许前往当前 Floor 内已经 `Resolved` 的 Map Node。
- 传送免费，不消耗当前时段预算，也不直接增加压力。
- 战斗、RunEvent、剧情和其它未结束事务期间不能传送。
- Map Travel 不能跨 Floor。
- 到达死胡同时，App 应提示打开地图，并默认聚焦上一个可传送节点。
- Map Screen 只提交目标节点意图；规则层验证成功后，App Adapter 才装配目标场景并激活对应 Run Path 入口。

## §5 Floor Transition

- Floor Transition 是不可逆的跨层规则事务；进入下一 Floor 后默认不能返回上一 Floor。
- 入口可以要求稳定 Run Credential，也可以保留指定卡牌/关键词条件；两类数组同时存在时采用 AND。卡牌条件检查 Backpack、BattleDeck、SpecialZone 与 BurdenZone 中的真实持有卡牌。
- 满足入口条件的 Credential 与卡牌都不被消耗；首次成功后永久记录入口已解锁。
- 每个强制 Credential 必须由前置、不可绕过且配置有效的固定 Pickup 保证授予；卡牌条件继续要求前置可达子图存在保证可获得的满足方式或明确替代条件。
- 进入前显示专用确认：不可返回提示、已揭示但未完成的节点数量、未知区域提示、当前压力、额外压力预期和入口条件。
- 旧 Floor 在地图历史中保留只读摘要，但不能成为 Map Travel 目标。
- 当前入口的 `FRunFloorTransitionPreview` 由 Snapshot 输出已知未完成数量、未知区域、压力和门槛事实。Request 只创建确认票据；Confirm 会重新验证门槛并提交；Cancel 只释放确认事务，避免关闭未来 Modal 后锁死探索。
- 成功只重置目标 Floor 的进入日和节点初始 lifecycle；JourneyDay、当前时段、剩余 Action Point、卡牌和压力全部保留。旧 Floor 只保留 `FRunFloorHistorySnapshot`。

## §6 Floor Exposure

- Floor Exposure 是当前 Floor 的连续停留天数，不是第九种压力。
- 第一层正常完整探索目标约为 1.5–2 天；前三天是安全宽限期，第 3 天用于路线失误、恢复和补漏，从第 4 天开始追加额外压力。
- Hunger 与 Fatigue 可以通过食物、休息或其它正式 Run 行为减少，不承担不可逆旅程时钟。
- Decay 是主要的长期时钟，唯一每日入口计算 `DailyDecay = BaseDecay + OverstayDecay`。
- `BaseDecay` 保留为按旅程天数可配置的曲线；首轮所有天数都返回 `+5`，后续由策划调整，不在代码中展开 day switch。
- `OverstayDecay` 在玩家超过当前 Floor 宽限期后追加：第 1–3 天为 0，第 4 天 +2，第 5 天 +5，第 6 天 +9，第 7 天及以后每日 +12 封顶。
- 同层 Map Travel 不重置 Floor Exposure。
- Floor Transition 成功后重置新 Floor 的滞留计数，但此前已经获得的压力不会清除。
- 普通 Camp 不能减少 Decay；只有稀有卡牌、事件或特殊奖励可以提供显式 Decay 恢复。

## §7 Action Point 与节点类型

Action Point（时段行动点）是唯一预算领域名称，正式状态字段为 `RemainingActionPoints`。所有扣减只能经 `WacomRun` 私有事务入口执行，不提供 UI/GameMode 可直接调用的公共扣点函数，也不保留早期按“节点数量”计时的兼容语义。

每天固定预算保持现有设计：

| 时段 | Action Point | 时段事件 |
|---|---:|---|
| Morning | 2 | Morning Planning 固定消费 1 点，实际剩 1 点 |
| Day | 6 | 主要探索、战斗、商店和事件 |
| Dusk | 2 | 可选择 Picnic；Picnic 按正常 1 点消费 |
| Night | 2 | 选择 Camp 或 Night Exploration |
| Sunrise | 1 | 仅 Night Exploration 路径进入，作为上一天的后置收益 |

三天是 Floor Exposure 宽限期，不要求玩家必须消耗三天才能完整探索。地图内容成本负责形成约 1.5–2 天的正常完整探索时长，不通过空节点拖长流程。

固定消耗口径：

| 行为 | Action Point |
|---|---:|
| Run Path 移动、岔路选择、地图揭示、打开地图 | 0 |
| 同层 Map Travel | 0 |
| 战斗胜利 | 1 |
| 战斗撤离 | 0；保留已确认的部位进度、经验和卡牌，节点不完成 |
| 战斗失败 | Run 结束，不推进已结束旅程的时段 |
| RunEvent | 由选项显式配置；普通终局默认 1 |
| 宝箱、搜刮 | 首次成功获取时 1；失败检查 0 |
| 商店 | 浏览和空手离开 0；本次访问首次成功交易时整体消费 1 |
| 同次商店后续交易 | 0 |
| Camp Action | 不作为 Map Node 或普通 1 点行为，进入专门的扎营与时段推进事务 |
| Floor Transition | 0；入口前置内容按自身规则消费 |

首版节点类型固定为：

- `Navigation / Junction`
- `Encounter`
- `RunEvent`
- `Shop`
- `Treasure`
- `FloorEntrance`

Boss 使用 `Encounter.Boss` 细分，锁定宝箱使用 Treasure 条件配置，不增加独立基础类型。纯走廊不应为了地图排版创建 Map Node；没有选择、内容、地标或规则意义的空间由 Run Path 表现。

### Camp Action

- Camp 不是 Map Node 类型，也不要求每层固定放置营地节点。
- 玩家只在 Night、且没有活动战斗、RunEvent、剧情或 Floor Transition 时可以请求 Camp；Dusk 保留 Picnic，不开放 Camp。
- 当前节点允许 Camp 时直接使用当前节点；否则只沿有向边和已经 `Resolved` 的图按最少 Map Edge 数寻找最近的 `Resolved + bAllowsCamp` 节点。距离相同时使用稳定 NodeId 排序，禁止依赖地图 UI 坐标。
- 找到目标后通过 Map Travel 相同的规则结果交给 App Adapter 装配落点；找不到合法节点时拒绝，并明确显示原因。
- Camp 结束当前 Night，放弃剩余 Night Action Point 并跳过 Sunrise，随后进入次日 Morning。选择 Night Exploration 时保留 2 点 Night 预算，耗尽后进入 Sunrise。
- Camp Action 消费 1 Action Point；随后进入露营阶段，子活动不再额外消费 Action Point。
- 一次 Camp 只允许选择并结算一个主要 Camp Activity。休息只是其中一种；卡牌强化、特殊事件、背包调整和技能活动作为 typed 扩展方向保留，当前不实现具体内容。
- 普通休息或其它普通 Camp Activity 不能减少 Decay；Decay 与 Floor Exposure 不会因 Camp 被清除或重置。
- 卡牌强化必须通过未来正式卡牌强化事务提交，Camp Screen 只能发送活动选择和待强化卡牌意图，不能直接改写 `FCardInstance`。
- `BeginCamp` 签发 `FRunCampTicket` 并锁定互斥生命周期；启动表现失败或玩家返回使用 `CancelCamp`。成功定位是免费提交，因此 Cancel 不把玩家传回原节点。
- `CompleteCampActivity` 只接受 `IRunCampActivityHandler` 的 typed outcome；本轮没有 production handler，自动化 fake 仅验证事务边界，不代表正式恢复数值。

## §8 Authoring 与验证方向

- 首版使用手工 Logical Map Graph 和手工 Run Path 场景，不实现 runtime PCG。
- 每个 Floor 使用一个 DataAsset 作为 Logical Map Graph 的规则真源，保存稳定 FloorId、NodeId、有向 Edge、地图 UI 坐标、内容定义和入口条件。
- 每个可独立加载的 Run Floor World 必须且只能放置一个 `AWacomRunFloorSceneDescriptorActor`，由 World 单向引用其 Floor DataAsset。Descriptor 不复制 Node/Edge、不保存 Run 状态，也不让 Floor 反向引用 World。
- 当前资产分为三类：`L_Exploration + DA_Journey_LevelAuthoring + DA_Floor_LevelAuthoring_01 + GM_Wacom` 是人工制作基线；`DA_Journey_Debug + DA_Floor_Debug_01 + GM_WacomRunDebug + L_RunExploration_Debug` 是 Debug builder 独占夹具；`Journey.Main.01 / Floor.Main.*` 是已冻结但尚未落成资产的 Production 设计身份。Authoring baseline 的当前 8 节点图仍不得被存档、内容文档或后续代码默认视为正式 Floor 1。
- 静态地图 DataAsset 类型属于 `WacomData`；Map Node Lifecycle、Action Point、Map Travel、Floor Transition 与 Floor Exposure 的运行时规则属于 `WacomRun` 内部的深层地图 Module，不新增 UE `.Build.cs` 模块。
- 场景 Actor / Component 只提供 `NodeId / EdgeId / NodeAnchor / content host` 映射；Actor 连线和关卡坐标不能成为 Logical Map Graph 的规则真相。`AWacomRunPathBranchTargetActor` 只广播 EdgeId，`AWacomRunPathSegmentActor` 只保存 EdgeId + Spline，`UWacomRunMapNodeBindingComponent` 只声明 Host 的 NodeId + NodeType。
- 现有 Battle / Shop / RunEvent / Treasure Host 为复用外围 flow 可以保留 Definition 字段作为 façade mirror；Scene Validator 要求它与 Floor typed payload 一致。规则层只认 Floor DataAsset，Host 不得反向生成或覆盖地图内容。
- 玩家锚定在节点时，App 只消费 `OutgoingEdges[].bCanTraverse` 构建路线选择状态：0 条结构出口为死胡同，存在出口但 0 条合法路线为暂时不可通行，1 条合法路线在首次 W 时自动 Begin，多条合法路线必须先明确选择 Edge。路线选择不增加 Run 命令或 Snapshot 字段。
- BranchTarget 是多出口 Decision Gate 的表现映射，不是移动开关。静态声明出度大于等于 2 的节点才为每条 Edge 制作一个 BranchTarget；单出口节点只制作 PathSegment。运行时只显示当前节点合法 Edge 对应的入口，开始移动、取消、抵达、地图传送或 Session 重绑后按最新显式 Snapshot 重建。
- 多出口入口按道路初始方向从左到右排序，鼠标 hover / click 或 A/D、左摇杆、E / 手柄 A 只上报 EdgeId；Branch Actor 不保存目标节点、规则门槛或 Segment 引用。单出口不要求玩家寻找或点击场景 Actor。
- NodeAnchor 的 View 朝向可以面向节点内容，PathSpline 起始切线可以面向道路方向；二者不要求制作时完全相同。开始 Traversal 的首帧保持 NodeAnchor View，App 在短距离内平滑对齐到 PathSpline，禁止用瞬时切换或 Character controller yaw 反写制造镜头中心跳变。
- 首版入口视觉由 `DShader/Material/World/M_WacomRunBranchEntrance.dsm` 生成 `/Game/DreamMaterials/World/M_WacomRunBranchEntrance`：Available 使用稳定青色，Focused 使用稳定琥珀色；Full / Reduced / Off 只改变装饰脉冲，Off 仍保留合法选择的语义颜色。
- `WacomEditor` 已提供 Journey/Floor Data Validation、严格只读的 loaded-world Scene Binding Validation 和可重复 Debug builder；未来图形 authoring Adapter 仍必须编辑同一份 DataAsset，不能维护第二份图数据。
- Floor、Map Node、Map Edge 和入口使用稳定身份，为未来滚动备份和恢复保留确定性。当前 SaveGame v5 保存 Run Credential、Outcome 与最近成功摘要，但仍不保存 Journey/Floor/Node、节点 lifecycle 或 Floor history。
- Editor 场景验证覆盖 Descriptor 缺失/重复/空引用，重复、缺失或意外的 NodeAnchor / EdgePath / content host，host 类型与 typed payload 不一致，以及多出口 Edge 缺失/重复 BranchTarget、单出口 Edge 残留 BranchTarget。Spline 少于 2 点、长度不超过 10 cm、非有限 Transform 和方向颠倒是 Error；端点偏离不超过 100 cm 通过，`(100, 300] cm` 为 Warning，超过 300 cm 为 Error。诊断固定为 `Severity / Code / ObjectPath / Message`，菜单、commandlet、builder 和测试共用同一只读实现。
- 制作人员可用 `Tools -> Wacom -> Validate Current Run Floor` 验证当前 World；CI/命令行使用 `-run=WacomValidateRunFloorScene -Map=/Game/...`，退出码 `0/1/2` 分别表示通过或仅 Warning、场景合同 Error、参数/加载/Descriptor 解析失败。两个入口都不得修复、标脏或保存 Package。
- Debug builder 命令为 `-run=WacomBuildRunExplorationDebugAssets`。它只写 Debug Journey/Floor/GameMode/map；Player BP 和三个共享 Run Path Blueprint 只读校验父类，正式 `GM_Wacom`、Authoring 数据和 `L_Exploration` 禁止写。连续运行必须保持 JourneyId、FloorId、NodeId、EdgeId、Actor 计数和内容引用稳定，但不承诺 Debug 生成 Actor GUID 或二进制文件哈希稳定。带 `Wacom.Generated.RunExploration` 的 Anchor/Path/Branch 属于 builder ownership；六个内容 Host 是按 Definition 复用的手工实例，不带该 tag，其 NodeId、NodeType、Blueprint class 和人工 transform 必须跨连续构建保持不变。
- Debug map 与正式 `L_Exploration` 共享逻辑身份和场景制作合同，不要求两张地图的世界 transform 相同。`MapPosition` 只控制地图 UI；Debug fixture 的世界道路可独立摆放，但仍必须通过端点、初始方向、BranchTarget 和 content binding 校验。当前 Junction 从 `Event.SnakeGift` 沿 +X 抵达时，`Treasure.PoisonFang` 为左路（-Y）、`Treasure.KeyChest` 为右路（+Y）、`Event.FlagReward` 为正前方。
- 地图规则 Module 应提供较小 Interface；场景 Actor、Spline、地图 Widget 和关卡 travel 只作为 App Adapter，不进入地图规则 Implementation。

### 当前 Floor 地图 Screen

`URunSession::BuildCurrentFloorMapSnapshot()` 是地图页面唯一的整层只读规则投影。它返回当前探索版本、Floor 标题、玩家可见节点/有向边、生命周期、同层传送可用性和 `RecommendedTravelTarget`；不包含 Actor、Spline、世界坐标或可写 DataAsset 指针，也不会广播状态变化。普通 `Hidden` 节点及关联边不显示；具有 landmark 语义的入口 / Boss 可以显示无标题、无路径、不可传送的轮廓。

同层地图传送继续遵循“免费且只到非当前 `Resolved` 节点”。确认前 App 必须让 Screen、最新 Map Snapshot 与 Coordinator 的版本一致，并由 Coordinator 预检同 Floor NodeAnchor 和有限 Transform；之后只调用既有 `MapTravel` 规则命令。提交前拒绝保持页面并刷新真实状态；规则已经提交但场景定位失败时关闭过期页面并记录严重诊断，禁止重试同一结果。

推荐落点只用于死胡同打开地图时的瞬时默认焦点：将当前 Floor 静态有向边视作无向邻接，选择图距离最近的合法 `Resolved` 节点，平局按 NodeId。它不写入 RunState、SaveGame 或最近访问历史。

## §9 正式三层 Journey 图与总节奏冻结

首版正式 Journey 使用三层不可逆顺序；三层图与内容槽均已完成设计冻结：

| Identity | 默认标题 | 本轮事实 |
|---|---|---|
| `Journey.Main.01` | 蛇巢之路 | 三层不可逆顺序的正式 Journey 身份 |
| `Floor.Main.01` | 蛇巢浅林 | 20 节点 / 21 边正式设计 |
| `Floor.Main.02` | 蛇蜕洞窟 | 20 节点 / 21 边；蜕印 Credential 门槛 |
| `Floor.Main.03` | 毒巢核心 | 20 节点 / 21 边；无入口 terminal Guardian |

### Floor 1 节点与内容槽

| NodeId | NodeType | 内容职责 / 预留 Production ID | Camp |
|---|---|---|:---:|
| `Node.Entry` | Navigation | 林地入口 | Yes |
| `Node.Main.01` | Encounter | 教学伏击；`Encounter.SerpentWood.Scout` | No |
| `Node.Junction.01` | Navigation | 第一处分岔 | Yes |
| `Node.Route.A.01` | RunEvent | 蛇蜕事件；`Event.SerpentWood.CastSkin` | No |
| `Node.Route.A.02` | Encounter | 蛇蜕守卫；`Encounter.SerpentWood.MoltGuard` | No |
| `Node.Route.A.03` | Treasure | 草药补给；`Pickup.SerpentWood.HerbCache` | No |
| `Node.Route.B.01` | Encounter | 毒雾伏击；`Encounter.SerpentWood.Ambush` | No |
| `Node.Route.B.02` | Treasure | 猎人遗物；`Pickup.SerpentWood.HunterCache` | No |
| `Node.Route.B.03` | RunEvent | 猎人痕迹；`Event.SerpentWood.HunterTrace` | No |
| `Node.Junction.02` | Navigation | 第一轮汇合 | Yes |
| `Node.Route.C.01` | Shop | 林下行商；`Shop.SerpentWood.Wayfarer` | No |
| `Node.Route.C.02` | RunEvent | 行商情报；`Event.SerpentWood.MerchantRumor` | No |
| `Node.Route.D.01` | Encounter | 盘根伏蛇；`Encounter.SerpentWood.RootStalker` | No |
| `Node.Route.D.02` | RunEvent | 毒沼抉择；`Event.SerpentWood.PoisonMarsh` | No |
| `Node.Route.D.03` | Treasure | 蜕壳密藏；`Pickup.SerpentWood.MoltCache` | No |
| `Node.Key.01` | Treasure | 必经蛇印；`Pickup.SerpentWood.SerpentSigil` 同时授予 `Card.Run.SerpentSigil` 与 `Credential.Run.SerpentSigil` | No |
| `Node.Junction.03` | Navigation | Boss 前汇合 | Yes |
| `Node.Main.02` | Encounter | 精英巡猎者；`Encounter.SerpentWood.EliteSentinel` | No |
| `Node.Guardian.01` | Encounter | 浅巢守卫；`Encounter.SerpentWood.ShallowGuardian`，`bBoss=true` | No |
| `Node.Exit.01` | FloorEntrance | 指向 `Floor.Main.02`，非消耗检查 `Credential.Run.SerpentSigil` | No |

节点类型精确配比为 `4 Navigation / 6 Encounter / 4 RunEvent / 4 Treasure / 1 Shop / 1 FloorEntrance`。四个 Navigation 都允许 Camp；这只冻结落点，不增加 Camp NodeType 或正式 Camp Activity 数值。分支内容不固定左援助/右风险语义。

### Floor 1 有向图

```text
Node.Entry -Edge.Main.01-> Node.Main.01 -Edge.Main.02-> Node.Junction.01

Node.Junction.01
  -Edge.Route.A.01-> Node.Route.A.01 -Edge.Route.A.02-> Node.Route.A.02
  -Edge.Route.A.03-> Node.Route.A.03 -Edge.Route.A.04-> Node.Junction.02
  -Edge.Route.B.01-> Node.Route.B.01 -Edge.Route.B.02-> Node.Route.B.02
  -Edge.Route.B.03-> Node.Route.B.03 -Edge.Route.B.04-> Node.Junction.02

Node.Junction.02
  -Edge.Route.C.01-> Node.Route.C.01 -Edge.Route.C.02-> Node.Route.C.02
  -Edge.Route.C.03-> Node.Key.01
  -Edge.Route.D.01-> Node.Route.D.01 -Edge.Route.D.02-> Node.Route.D.02
  -Edge.Route.D.03-> Node.Route.D.03 -Edge.Route.D.04-> Node.Key.01

Node.Key.01 -Edge.Main.03-> Node.Junction.03
  -Edge.Main.04-> Node.Main.02
  -Edge.Main.05-> Node.Guardian.01
  -Edge.Main.06-> Node.Exit.01
```

所有 20 个节点从 Entry 可达；A/B 汇合到 `Node.Junction.02`，C/D 汇合到 `Node.Key.01`。蛇印节点支配精英、守卫与出口，删除它后 Guardian/Exit 不可达。最短推进为 `8–9 AP`，完整探索为 `14–15 AP`；唯一 1 点差值来自是否在 Shop 完成本次访问的首次成功交易。

### Floor 1 Production 内容与路线节奏

Floor 1 的 15 个内容节点已完成 Production 内容设计冻结，不再只是空槽。战斗梯度：

| Node | Encounter | Composition | Total HP |
|---|---|---|---:|
| `Node.Main.01` | `Encounter.SerpentWood.Scout` | BrushSnake | 16 |
| `Node.Route.A.02` | `Encounter.SerpentWood.MoltGuard` | MoltGuard | 28 |
| `Node.Route.B.01` | `Encounter.SerpentWood.Ambush` | 2 × BrushSnake | 32 |
| `Node.Route.D.01` | `Encounter.SerpentWood.RootStalker` | RootStalker | 26 |
| `Node.Main.02` | `Encounter.SerpentWood.EliteSentinel` | MoltGuard + BrushSnake | 44 |
| `Node.Guardian.01` | `Encounter.SerpentWood.ShallowGuardian` | ShallowGuardian | 52 |

路线内容与收益：

- Route A：蛇蜕事件可选择研究情报、`Gold +2 / Misdeed +2` 或离开；随后 MoltGuard 与草药敷剂 Pickup。选择 Gold 时从初始 0 Gold 已可购买一个 2 Gold 商品。
- Route B：双 BrushSnake 伏击、猎人绊索 Pickup、猎人痕迹事件；事件可选择路线情报、`Gold +3 / Misdeed +3` 或 `Misdeed -2`。选择 Gold 时可购买任一 Wayfarer 商品。
- Route C：Wayfarer 固定出售 `Starter.ChitinWard(2)`、`Starter.AntennaSearch(2)`、`Starter.MoltCut(3)`、现有正式 `PoisonFang(2)`、`Reward.SerpentWood.HerbalPoultice(2)`；行商情报可用蛇蜕 flag 换毒沼路线、1 Gold 买图、增加 2 Misdeed 偷听或放弃。
- Route D：RootStalker、毒沼事件和蜕壳护符 Pickup；已知路线可 `Fatigue -2`，也可花 2 Gold 或直接 `Fatigue +5` 涉水。
- `Node.Key.01` 固定授予 `Card.Run.SerpentSigil + Credential.Run.SerpentSigil`；入口继续只检查 Credential。

所有 RunEvent 选项都是 terminal Automatic 选项，成功固定 1 AP；Treasure/Encounter 仍各按首次成功/胜利 1 AP，Shop 只有本次访问首次成功购买 1 AP。因此路线数值没有改变 `8–9 / 14–15 AP`：情报/道德选择影响经济与压力，不改变事件 AP；区间仍只来自是否购买。

击倒奖励量按 Encounter 部位数固定为：必经 Scout + EliteSentinel + Guardian 共 11；Route A 增加 MoltGuard 3，Route B 增加双 BrushSnake 4，Route C 不增加 Encounter，Route D 增加 RootStalker 2。因此 A/C、B/C、A/D、B/D 四条关键路线分别获得 `14 / 15 / 16 / 17` 张分支奖励卡，完整探索全部六场 Encounter 最多获得 20 张。每个部位只获得所选 Aid 或 Destroy 的一张独立实例，允许重复；选择不额外消耗 AP。该体量是后续卡组/背包平衡和 PIE 的显式风险，不在本轮增加去重或替代奖励。

Floor 1 核心内容与八张击倒奖励卡设计 blocker 已关闭，`38 core + 8 branch reward = 46` 个静态 Production DataAsset 也已创建并通过真实加载、AssetRegistry、引用/哈希与幂等审计；Host、Production Journey/Floor DataAsset 和正式关卡仍未创建。其它 Aid/Destroy/Withdraw 后果、背包容量、世界资产权威、正式场景和 Golden Path PIE 继续分别处理。精确静态内容合同见 [WacomData.md](./WacomData.md) §13，路径和 seed-only 制作边界见 [WacomDataAuthoring.md](./WacomDataAuthoring.md) §4。

### Floor 2 节点与内容槽

| NodeId | NodeType | 内容职责 / 预留 Production ID | Camp |
|---|---|---|:---:|
| `Node.Entry` | Navigation | 洞窟入口 | Yes |
| `Node.Main.01` | Encounter | 鳞岩伏击；`Encounter.MoltCavern.ScaleScout` | No |
| `Node.Junction.01` | Navigation | 裂隙岔口 | Yes |
| `Node.Route.A.01` | RunEvent | 残蜕回声；`Event.MoltCavern.CastoffEcho` | No |
| `Node.Route.A.02` | Encounter | 石鳞守地；`Encounter.MoltCavern.StoneScaleGuard` | No |
| `Node.Route.A.03` | Treasure | 菌光补给；`Pickup.MoltCavern.FungalCache` | No |
| `Node.Route.B.01` | Encounter | 孵室伏击；`Encounter.MoltCavern.HatcheryAmbush` | No |
| `Node.Route.B.02` | Treasure | 矿脉密藏；`Pickup.MoltCavern.MineralCache` | No |
| `Node.Route.B.03` | RunEvent | 失踪探路者；`Event.MoltCavern.LostDelver` | No |
| `Node.Junction.02` | Navigation | 旧井汇流 | Yes |
| `Node.Route.C.01` | Shop | 深窟行商；`Shop.MoltCavern.DeepWayfarer` | No |
| `Node.Route.C.02` | Encounter | 断桥守敌；`Encounter.MoltCavern.BridgeSentinel` | No |
| `Node.Route.D.01` | Encounter | 毒泉猎手；`Encounter.MoltCavern.VenomHunter` | No |
| `Node.Route.D.02` | RunEvent | 蜕壳仪式；`Event.MoltCavern.MoltingRite` | No |
| `Node.Route.D.03` | Treasure | 毒晶密藏；`Pickup.MoltCavern.VenomCrystalCache` | No |
| `Node.Key.01` | Treasure | 深窟蜕印；`Pickup.MoltCavern.MoltSeal` 同时预留 `Card.Run.MoltSeal` 与 `Credential.Run.MoltSeal` | No |
| `Node.Junction.03` | Navigation | 核门前哨 | Yes |
| `Node.Main.02` | Encounter | 蜕窟巡猎；`Encounter.MoltCavern.EliteMolter` | No |
| `Node.Guardian.01` | Encounter | 洞窟守卫；`Encounter.MoltCavern.CavernGuardian`，`bBoss=true` | No |
| `Node.Exit.01` | FloorEntrance | 指向 `Floor.Main.03`，非消耗检查 `Credential.Run.MoltSeal` | No |

类型精确配比为 `4 Navigation / 7 Encounter / 3 RunEvent / 4 Treasure / 1 Shop / 1 FloorEntrance`。`Node.Key.01` 支配精英、守卫与出口；表现卡可被移除，入口资格只来自持久 Credential。

### Floor 2 有向图

```text
Node.Entry -Edge.Main.01-> Node.Main.01 -Edge.Main.02-> Node.Junction.01

Node.Junction.01
  -Edge.Route.A.01-> Node.Route.A.01 -Edge.Route.A.02-> Node.Route.A.02
  -Edge.Route.A.03-> Node.Route.A.03 -Edge.Route.A.04-> Node.Junction.02
  -Edge.Route.B.01-> Node.Route.B.01 -Edge.Route.B.02-> Node.Route.B.02
  -Edge.Route.B.03-> Node.Route.B.03 -Edge.Route.B.04-> Node.Junction.02

Node.Junction.02
  -Edge.Route.C.01-> Node.Route.C.01 -Edge.Route.C.02-> Node.Route.C.02
  -Edge.Route.C.03-> Node.Key.01
  -Edge.Route.D.01-> Node.Route.D.01 -Edge.Route.D.02-> Node.Route.D.02
  -Edge.Route.D.03-> Node.Route.D.03 -Edge.Route.D.04-> Node.Key.01

Node.Key.01 -Edge.Main.03-> Node.Junction.03
  -Edge.Main.04-> Node.Main.02
  -Edge.Main.05-> Node.Guardian.01
  -Edge.Main.06-> Node.Exit.01
```

所有 20 个节点从 Entry 可达；A/B 汇合到 `Node.Junction.02`，C/D 汇合到 `Node.Key.01`。最短推进为 `8–9 AP`，完整探索为 `14–15 AP`，差值只来自 Floor 2 Shop 的首次成功交易。

### Floor 3 节点与内容槽

| NodeId | NodeType | 内容职责 / 预留 Production ID | Camp |
|---|---|---|:---:|
| `Node.Entry` | Navigation | 核心外环 | Yes |
| `Node.Main.01` | Encounter | 巢心先锋；`Encounter.VenomCore.CoreVanguard` | No |
| `Node.Junction.01` | Navigation | 毒脉分流 | Yes |
| `Node.Route.A.01` | RunEvent | 毒脉共振；`Event.VenomCore.VeinResonance` | No |
| `Node.Route.A.02` | Encounter | 脉道守卫；`Encounter.VenomCore.VeinGuardian` | No |
| `Node.Route.A.03` | Treasure | 解毒储备；`Pickup.VenomCore.AntidoteCache` | No |
| `Node.Route.B.01` | Encounter | 孵群巡猎；`Encounter.VenomCore.BroodPatrol` | No |
| `Node.Route.B.02` | Treasure | 仪式密藏；`Pickup.VenomCore.RitualCache` | No |
| `Node.Route.B.03` | RunEvent | 核心低语；`Event.VenomCore.CoreWhisper` | No |
| `Node.Junction.02` | Navigation | 巢心汇流 | Yes |
| `Node.Route.C.01` | Encounter | 内环哨卫；`Encounter.VenomCore.InnerSentinel` | No |
| `Node.Route.C.02` | RunEvent | 献祭抉择；`Event.VenomCore.SacrificeChoice` | No |
| `Node.Route.D.01` | Encounter | 毒液潜猎；`Encounter.VenomCore.ToxinStalker` | No |
| `Node.Route.D.02` | RunEvent | 巢心搏动；`Event.VenomCore.HeartPulse` | No |
| `Node.Route.D.03` | Treasure | 毒池储备；`Pickup.VenomCore.VenomReservoir` | No |
| `Node.Core.01` | Treasure | 必经核心恩赐；`Pickup.VenomCore.CoreBoon` | No |
| `Node.Junction.03` | Navigation | 核心前庭 | Yes |
| `Node.Main.02` | Encounter | 精英收割者；`Encounter.VenomCore.EliteHarvester` | No |
| `Node.Main.03` | Encounter | 终门先锋；`Encounter.VenomCore.FinalVanguard` | No |
| `Node.Guardian.01` | Encounter | 毒巢核心守卫；`Encounter.VenomCore.CoreGuardian`，`bBoss=true`，Journey terminal design node | No |

类型精确配比为 `4 Navigation / 8 Encounter / 4 RunEvent / 4 Treasure`，没有 Shop 或 FloorEntrance。`Node.Core.01` 是两条第二轮支路的必经奖励并支配完整终局链。

### Floor 3 有向图与终局边界

```text
Node.Entry -Edge.Main.01-> Node.Main.01 -Edge.Main.02-> Node.Junction.01

Node.Junction.01
  -Edge.Route.A.01-> Node.Route.A.01 -Edge.Route.A.02-> Node.Route.A.02
  -Edge.Route.A.03-> Node.Route.A.03 -Edge.Route.A.04-> Node.Junction.02
  -Edge.Route.B.01-> Node.Route.B.01 -Edge.Route.B.02-> Node.Route.B.02
  -Edge.Route.B.03-> Node.Route.B.03 -Edge.Route.B.04-> Node.Junction.02

Node.Junction.02
  -Edge.Route.C.01-> Node.Route.C.01 -Edge.Route.C.02-> Node.Route.C.02
  -Edge.Route.C.03-> Node.Core.01
  -Edge.Route.D.01-> Node.Route.D.01 -Edge.Route.D.02-> Node.Route.D.02
  -Edge.Route.D.03-> Node.Route.D.03 -Edge.Route.D.04-> Node.Core.01

Node.Core.01 -Edge.Main.03-> Node.Junction.03
  -Edge.Main.04-> Node.Main.02
  -Edge.Main.05-> Node.Main.03
  -Edge.Main.06-> Node.Guardian.01
```

Guardian 无出边，战斗胜利后由通用 Journey success 合同完成 Journey。Production `DA_Journey_Main_01` 必须把 `SuccessTerminalNode` 配为 `Floor.Main.03 + Node.Guardian.01`；Editor/Runtime 会要求它位于最后一层、节点存在、为 `Encounter + bBoss=true`、从 Entry 可达、无出边，且最后一层没有 FloorEntrance。非撤离 terminal Victory 会在同一 Run 事务中 Resolve 节点、生成成功摘要、写入 `Outcome=Succeeded` 并把 `JourneySucceeded` 作为末尾事件；不能用 Actor、Level Blueprint、EncounterId、`bRunActive=false` 或伪 FloorEntrance 代替。Floor 3 最短推进为 `10 AP`，完整探索为 `16 AP`。

### Journey 总节奏

| Floor | 最短推进 | 完整探索 |
|---|---:|---:|
| `Floor.Main.01` | 8–9 | 14–15 |
| `Floor.Main.02` | 8–9 | 14–15 |
| `Floor.Main.03` | 10 | 16 |
| **Journey** | **26–28** | **44–46** |

区间只来自 Floor 1/2 Shop 是否发生首次成功交易。正常 Camp/恢复节奏的内容吞吐约为 8–10 AP/天，因此关键推进目标约 3 天，完整探索约 5–6 天；积极 Night Exploration 并使用 Sunrise 时，完整探索理论下界约 4 天。这些是 authoring 目标，不新增强制 Journey timer，也不改变三天 Floor Exposure 宽限。

### 身份与生产门禁

- `Journey.Main.01`、三个 FloorId、每层冻结的 20 个 NodeId/21 个 EdgeId、两个 CardId 与两个 CredentialId 都是稳定身份。SaveGame v5 已通用持久化 Credential、Outcome 与最近一次成功摘要；探索图进度仍未进入磁盘 schema。
- 内容 Host 的跨 Floor runtime `PersistentId` 固定按 `<FloorId>.<NodeId>` 派生，例如 `Floor.Main.01.Node.Route.A.01`。Actor Label、资产名、GUID、坐标和 transform 都不是身份。
- DisplayName、描述、MapPosition 和世界 Transform 仍可调；NodeType、Edge 端点、Journey 顺序和蛇印门槛不是表现调参。
- 三层共 46 个 Production 节点内容 ID。Floor 1 的 15 个节点已冻结敌人槽、事件选项、Shop Offers 与奖励数值；Floor 2/3 的 31 个节点仍只冻结职责。所有视觉资产、Host 和世界 Transform 继续另案；现有带 Debug 语义的 Event/Shop/Reward 夹具不能作为正式引用。
- 蛇印和蜕印都采用表现卡 + 独立 Credential：必经 Pickup 原子授予两者，删牌流程不影响资格，入口只检查 Credential。
- Floor 2/3 缺图、通用 Journey success 与 Floor 1 内容设计 blocker 已关闭。Production Journey/Floor DataAsset 与正式关卡仍受三层 46 个节点 Definition 的实际制作、首层 31 个支持资产（含 8 张击倒奖励卡）、Floor 2/3 支持内容设计和资产权威审计阻塞；禁止创建空壳资产或用 Debug 内容绕过。

Floor 1 默认 MapPosition 与原始冻结证据见 `specs/007-formal-floor1-content-freeze/`；Floor 2/3 默认 MapPosition、完整 Edge 表、Journey pacing 与当前 Production readiness gate 见 `specs/009-formal-floor23-journey-pacing-freeze/`。上述长期身份、拓扑、配比和阻塞事实以本节为准。

## §10 尚待确认

- Hunger / Fatigue 恢复活动的具体值、资源成本和重复使用口径。
- 卡牌强化、特殊事件、背包调整和技能类 Camp Activity 的正式规则与 UI 流程。
