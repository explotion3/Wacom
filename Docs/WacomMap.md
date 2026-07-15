---
type: domain-design
scope: wacom-map
status: core-implemented
updated: 2026-07-14
tags:
  - wacom/map
  - wacom/run
  - wacom/exploration
---

# WacomMap

> [!info] 本文职责
> 本文记录 Logical Map Graph、Map Node Lifecycle、Map Travel、Floor Transition 与 Floor Exposure 的已确认规则。Run 时间、压力、背包和事件事实仍见 [WacomRun.md](./WacomRun.md)；Run Path 移动与场景表现见 [WacomApp.md](./WacomApp.md)。

> [!warning] 当前阶段
> 地图静态合同、原子初始化、节点生命周期、Traversal Ticket、同层 Map Travel、Action Point、节点活动、Camp、不可逆跨层、Run Path 场景适配、制作校验与 Debug Journey 已落地。`L_Exploration` 已迁移到唯一正式 Run Path；当前未交付的是地图 Screen、正式 Camp 内容和多 Floor 旅程内容。

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
- 入口可以要求拥有指定卡牌或关键词；检查 Backpack、BattleDeck、SpecialZone 与 BurdenZone 中的真实持有卡牌。
- 满足入口条件的卡牌不被消耗；首次成功后永久记录入口已解锁。
- 每个强制入口条件必须在前置可达子图中存在至少一种保证可获得的满足方式，或存在明确替代条件。
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
- 静态地图 DataAsset 类型属于 `WacomData`；Map Node Lifecycle、Action Point、Map Travel、Floor Transition 与 Floor Exposure 的运行时规则属于 `WacomRun` 内部的深层地图 Module，不新增 UE `.Build.cs` 模块。
- 场景 Actor / Component 只提供 `NodeId / EdgeId / NodeAnchor / content host` 映射；Actor 连线和关卡坐标不能成为 Logical Map Graph 的规则真相。`AWacomRunPathBranchTargetActor` 只广播 EdgeId，`AWacomRunPathSegmentActor` 只保存 EdgeId + Spline，`UWacomRunMapNodeBindingComponent` 只声明 Host 的 NodeId + NodeType。
- 现有 Battle / Shop / RunEvent / Treasure Host 为复用外围 flow 可以保留 Definition 字段作为 façade mirror；Scene Validator 要求它与 Floor typed payload 一致。规则层只认 Floor DataAsset，Host 不得反向生成或覆盖地图内容。
- 玩家锚定在节点时，App 只消费 `OutgoingEdges[].bCanTraverse` 构建路线选择状态：0 条结构出口为死胡同，存在出口但 0 条合法路线为暂时不可通行，1 条合法路线在首次 W 时自动 Begin，多条合法路线必须先明确选择 Edge。路线选择不增加 Run 命令或 Snapshot 字段。
- BranchTarget 是多出口 Decision Gate 的表现映射，不是移动开关。静态声明出度大于等于 2 的节点才为每条 Edge 制作一个 BranchTarget；单出口节点只制作 PathSegment。运行时只显示当前节点合法 Edge 对应的入口，开始移动、取消、抵达、地图传送或 Session 重绑后按最新显式 Snapshot 重建。
- 多出口入口按道路初始方向从左到右排序，鼠标 hover / click 或 A/D、左摇杆、E / 手柄 A 只上报 EdgeId；Branch Actor 不保存目标节点、规则门槛或 Segment 引用。单出口不要求玩家寻找或点击场景 Actor。
- NodeAnchor 的 View 朝向可以面向节点内容，PathSpline 起始切线可以面向道路方向；二者不要求制作时完全相同。开始 Traversal 的首帧保持 NodeAnchor View，App 在短距离内平滑对齐到 PathSpline，禁止用瞬时切换或 Character controller yaw 反写制造镜头中心跳变。
- 首版入口视觉由 `DShader/Material/World/M_WacomRunBranchEntrance.dsm` 生成 `/Game/DreamMaterials/World/M_WacomRunBranchEntrance`：Available 使用稳定青色，Focused 使用稳定琥珀色；Full / Reduced / Off 只改变装饰脉冲，Off 仍保留合法选择的语义颜色。
- `WacomEditor` 已提供 Journey/Floor Data Validation、loaded-world Scene Binding Validation 和可重复 Debug builder；未来图形 authoring Adapter 仍必须编辑同一份 DataAsset，不能维护第二份图数据。
- Floor、Map Node、Map Edge 和入口使用稳定身份，为未来滚动备份和恢复保留确定性，但本阶段不启用 SaveGame schema。
- Editor 验证覆盖重复身份、无效连接、不可达强制节点、无法满足的 Floor Entrance、typed payload、NodeAnchor / EdgePath / content host 缺失、重复与错类型，以及多出口 Edge 缺失/重复 BranchTarget、单出口 Edge 残留 BranchTarget。
- Debug builder 生成稳定的 `DA_Journey_Debug`、`DA_Floor_Debug_01` 和三个 Run Path Blueprint，并迁移 `GM_Wacom`、玩家蓝图与 `L_Exploration`；连续运行必须保持 JourneyId、FloorId、NodeId、EdgeId 和内容引用稳定。
- 地图规则 Module 应提供较小 Interface；场景 Actor、Spline、地图 Widget 和关卡 travel 只作为 App Adapter，不进入地图规则 Implementation。

### 当前 Floor 地图 Screen

`URunSession::BuildCurrentFloorMapSnapshot()` 是地图页面唯一的整层只读规则投影。它返回当前探索版本、Floor 标题、玩家可见节点/有向边、生命周期、同层传送可用性和 `RecommendedTravelTarget`；不包含 Actor、Spline、世界坐标或可写 DataAsset 指针，也不会广播状态变化。普通 `Hidden` 节点及关联边不显示；具有 landmark 语义的入口 / Boss 可以显示无标题、无路径、不可传送的轮廓。

同层地图传送继续遵循“免费且只到非当前 `Resolved` 节点”。确认前 App 必须让 Screen、最新 Map Snapshot 与 Coordinator 的版本一致，并由 Coordinator 预检同 Floor NodeAnchor 和有限 Transform；之后只调用既有 `MapTravel` 规则命令。提交前拒绝保持页面并刷新真实状态；规则已经提交但场景定位失败时关闭过期页面并记录严重诊断，禁止重试同一结果。

推荐落点只用于死胡同打开地图时的瞬时默认焦点：将当前 Floor 静态有向边视作无向邻接，选择图距离最近的合法 `Resolved` 节点，平局按 NodeId。它不写入 RunState、SaveGame 或最近访问历史。

## §9 尚待确认

- 首版 Floor 数量、每层有效节点数量和整段旅程目标天数。
- Hunger / Fatigue 恢复活动的具体值、资源成本和重复使用口径。
- 卡牌强化、特殊事件、背包调整和技能类 Camp Activity 的正式规则与 UI 流程。
- 首版每种节点类型的内容占比和必保底节点。
