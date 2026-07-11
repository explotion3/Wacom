---
type: presentation-contract
scope: wacom-first-person-card-layer
status: active
updated: 2026-07-11
tags:
  - wacom/ui
  - wacom/cards
  - wacom/first-person
  - wacom/battle
  - wacom/run
---

# First-person Card Layer 文档

> [!info] 本文职责
> 本文记录第一人称卡牌层的当前制作与运行时合同：正式布局默认、Battle / Run 数据源、hover / inspect / drag、target / drop 边界、开发预览 / 兼容边界和 `WBP_FPCardView` 制作要求。

> [!warning] 边界
> 第一人称卡牌层只做 UI 表现和玩家意图桥接。Battle 规则见 [WacomBattle.md](./WacomBattle.md)，Run 规则见 [WacomRun.md](./WacomRun.md)，世界 target / drop 路由见 [WacomWorldInteraction.md](./WacomWorldInteraction.md)，BattleHUD 命令出口见 [WacomBattleUI.md](./WacomBattleUI.md)。

## §1 正式方向

正式主手牌方向是 HUD-rendered first-person card layer：

```text
Battle / Run snapshot
-> card presentation data
-> HUD / UMG card widgets
-> first-person anchor projects hand center
-> card layer applies authored 2D layout and slot motion
```

卡牌仍由 UMG 渲染，因此保留 HUD 级别清晰度、材质动画和 WBP 可迭代性。第一人称感来自 anchor 投影、身体 / tunnel / battle base rotation、soft clamp、hand center smoothing 和 slot motion，而不是把常驻手牌做成 `WidgetComponent`。

## §2 核心对象

| 类型 | 职责 | 不负责 |
|---|---|---|
| `UWacomFirstPersonCardAnchorComponent` | 作为制作参数 façade、投影锚点 owner 和对外事件 façade；从 Details 构建 layout / motion / feedback config | 不提交 Battle / Run 命令，不直接持有规则状态 |
| `WacomFirstPersonCardLayerTypes.h` | 第一人称卡牌层公共 UI 协议：entry、slot view、drag / pointer view、transition hint、feedback hint、motion / visual / feedback config；`FWacomFirstPersonCardLayerEntry::InteractionIntent` 是 SlotWidget 消费的手势意图 | 不包含 AnchorComponent 制作参数或运行时实现，不把 Battle / Run 规则枚举作为 Widget 手势判断入口 |
| `FWacomFirstPersonCardAnchorRuntimeState` | Anchor 私有 runtime source 状态：entries、view data、legacy transition hints、feedback hints、presentation frame hints、presentation gate、hovered card / card target handle | 不暴露 Blueprint API，不负责布局或 widget 生命周期 |
| `FWacomFirstPersonCardSlotLayoutBuilder` | Anchor 私有布局构建器：根据 resolved config、投影后 hand anchor 和 viewport size 生成基础 slot view / input hit 几何 | 不依赖 `UWacomFirstPersonCardAnchorComponent`，不处理 hover 视觉合成或命令 |
| `FWacomFirstPersonCardLayerOwner` | Anchor 私有 CardLayerWidget 生命周期 owner：创建 / 移除 widget、应用 layer config、推送 presentation frame / transition hints / feedback hints 和 slots | 不解析 anchor / viewport，不读取 runtime source，不转发 Battle / Run 命令 |
| `FWacomFirstPersonCardLayerDelegateRouter` | Anchor 私有 LayerWidget 事件 router：绑定 / 解绑 native delegates、同步 hovered runtime state、转发 Anchor 对外 delegates | 不创建 widget，不解析布局，不提交 Battle / Run 命令 |
| `UWacomFirstPersonCardLayerWidget` | 按 entries 的正式 UI facts reconcile slot widget，维护 active / outgoing slot，绘制 drag arrow 和 layer-level feedback；dirty gate 只把 `InteractionIntent` 当手势差异 | 不读取 Battle / Run 规则状态 |
| `UWacomFirstPersonCardLayerSlotWidget` | 持有单卡 `UWacomFirstPersonCardViewWidget`，处理 hover / press / inspect / drag gesture、visual slot motion 和 Card Depth 输入采样 | 不直接调用 BattleSession 或 RunSession，不直接创建卡面反馈 Image / 材质控件 |
| `FWacomFirstPersonCardDepthMotion` | App-private 单卡深度运动：Hover 使用卡内 pointer，Drag 使用低通后的 pointer velocity，输出可中断的 tilt / perspective / shadow view | 不持有 Widget，不读取 Battle / Run 规则，不加载材质资产 |
| `UWacomFirstPersonCardViewWidget` | 第一人称卡面 wrapper：组合通用 `UWacomCardView`、first-person 反馈层、单个 fake-3D Retainer 和独立阴影 | 不处理 hover / drag 手势，不提交 Battle / Run 命令 |
| `FWacomFirstPersonCardLayerPresentationFrame` | Battle / Run 共用的 C++ 表现帧 contract：`SourceId + entries + transition hints + feedback hints + CommitMode` | 不读取规则状态，不决定 hints 来自哪个领域事件 |
| `WacomFirstPersonCardLayerSourceIds` | Battle / Run 共用的保留 runtime source id：`BattleHand`、`RunFirstPersonBattleDeck`、`RunFirstPersonMenuSuppressed` | 不生成菜单 lease 的自定义 source id，不读取 Anchor 状态 |
| `UWacomRunFirstPersonCardSourceComponent` | 探索期把 Run Card Workspace / menu lease 写入 anchor runtime source，缓存当前 `CardInstanceId -> FRunCardWorkspaceEntry` 来源 metadata，并集中回答 default / menu lease / suppressed source ownership 查询 | 不提交 Run 规则，不直接扫描 RunState 物理区 |
| `FWacomFirstPersonCardDetailMotionController` | App-private 共享详情面板 motion core：预热 panel、缓存当前只读详情数据、淡入淡出 / scale / follow motion、稳定换边和 teardown state | 不创建 widget，不读取 Battle / Run 规则，不暴露 Blueprint 制作面 |
| `FWacomFirstPersonCardDetailPanelHost` | App-private 共享详情面板 host：创建 / AddToViewport / RemoveFromParent / viewport size fallback，并调用 motion core 预热 panel | 不读取卡牌数据，不判断 Battle / Run source ownership |
| `FWacomFirstPersonCardCameraLookBridge` | App-private 共享 camera-look gesture core：统一 pointer / inspect / drag 对 camera-look override 的 ownership，active gesture 期间压制普通 pointer view，release / cancel / teardown 时清理 | 不直接操作 BattleCamera 或 RunTunnel，不判断投放目标，不读取规则 |
| `FWacomRunFirstPersonCardDetailController` | Run first-person hover / inspect 详情 adapter：绑定 / 解绑 Anchor hover delegates、执行 hover / inspect / scrub 详情决策、缓存 Run 详情数据并委托共享 detail motion core；source ownership 由 `UWacomRunFirstPersonCardSourceComponent` 判断，数据来源由 `AWacomPlayerController` 通过 `URunSession::FindInstance()` 提供 | 不提交 Run 规则，不接管 `BattleHand` source |
| `FWacomRunFirstPersonCardDragController` | Run first-person card drag adapter：绑定 / 解绑 Anchor drag delegates，分流 inspect、正式拖拽、menu zone drop、world drop 和 pointer camera-look | 不提交 Run 规则，不执行具体 menu/world drop 结算 |
| `FWacomBattleHandPresentationController` | BattleHUD 内部战斗手牌表现事务：收集 Battle events、选择 baseline、生成 entries + transition hints + feedback hints frame | 不读取 Anchor 投影，不提交 Battle 命令 |
| `FWacomBattleHUDFirstPersonHandBridge` | BattleHUD 内部同步 battle hand presentation frame、drag preview / release | 不暴露 Blueprint API |
| `FWacomBattleHUDCardDetailController` | Battle first-person 详情 adapter：从 Battle hand snapshot / preview 构建详情数据、维护 source guard，并委托共享 detail motion core | 不改变卡牌规则，不接管 Run source |

## §3 Authoring 默认

默认 first-person hand authoring surface 是：

```text
ProjectionMode = BodyLocked
Layout = fixed Authored2D
ViewportClampMode = SoftClampToViewport
```

`BodyLocked` 使用 Battle base rotation 或 Run Tunnel spline base 作为稳定身体基准，不让 cursor look 重新计算卡牌世界槽位。投影仍使用当前真实相机，因此鼠标移动镜头时仍有合理第一人称空间变化。

`Look Responsive Projected` 是保留的次级投影风格。它会把 `UWacomCursorLookDriverComponent` 的当前鼠标镜头偏移按 `LookInfluenceYaw / LookInfluencePitch` 混入手牌锚点，再使用当前真实相机投影。这样 look 同时影响 anchor 计算和相机投影，适合需要更强跟随感、空间漂移或视差感的手牌表现；代价是稳定性和可读性弱于 `BodyLocked`，需要按具体场景单独调参。C++ 枚举值仍叫 `LegacyWorldProjected`，只是为了蓝图 / 资产序列化兼容。

第一人称手牌运行时固定使用 `Authored2D`：只投影整副手牌中心点。每张卡的位置、下坠、扇形角度、层级和主体底部可读保护由 2D 参数计算，避免每张卡分别世界投影导致尺寸和扇形拉扯。旧的 `LegacyProjectedFan2D` 每卡 3D 槽位分别投影路径已清理，不再作为 PIE / debug comparison 入口。

`SoftClampToViewport` 允许手牌中心部分离开视口，超过 soft allowance 后再柔性拉回，保留空间感。`HardClampToViewport` 用于复现旧的始终屏内行为，`AllowOffscreen` 用于验证最接近空间物体的表现。

当前 first-person hand 的正式制作入口是 `UWacomFirstPersonCardAnchorComponent` Details 面板参数。`BP_WacomPlayerCharacter` 上的 AnchorComponent 直接承载卡面 Widget、锚点位置、投影、手牌形状、slot motion、hover、gesture、feedback 和 camera look 调参；这些字段是当前 Battle / Run 手牌表现的主线来源。

Anchor 空间优先级是 Battle camera look、active first-person view stage blend、active 且未 suspended 的 Run Tunnel，最后才是 camera fallback。进入战斗 viewpoint blend 时 Run Tunnel 会保留 Segment / Distance 但处于 suspended；此时它不能继续提供手牌锚点，否则 card layer 会停在探索样条位置直到 Battle camera look 激活。Stage blend 暴露当前 staged base View Pose 给 Anchor，鼠标 additive look 仍通过共享 cursor look / 当前相机投影体现，不把鼠标偏移写成新的 Viewpoint base。

旧 `UWacomFirstPersonCardLayoutPreset` DataAsset、runtime override API、Anchor 内部 preset resolve、editor validator 和相关测试已经删除。战斗与探索手牌表现都应通过玩家 AnchorComponent Details 参数调节；如果后续需要共享调参模板，应重新设计新的数据合同，而不是恢复旧 preset 路径。

`LookInfluenceYaw / LookInfluencePitch` 也属于 first-person hand 表现参数，当前推荐直接在 AnchorComponent Details 中调整。制作起点：战斗默认使用 `BodyLocked`；探索、特殊检查或希望手牌有更强空间跟随感的场景可使用 `Look Responsive Projected`。`LookInfluenceYaw` 建议先在 `0.05-0.35` 内调，`LookInfluencePitch` 建议先在 `0.03-0.20` 内调；如果手牌在移动鼠标时过度漂移、读牌不稳或与镜头响应产生二次晃动，优先降低这两个值，再调整 clamp / smoothing。

Anchor Details 分类使用稳定编号，当前口径如下：

| 分类 | 主要内容 |
|---|---|
| `01 Card View` | `CardLayerWidgetClass`、`FirstPersonCardViewClass`、`CardLayerZOrder` |
| `02 Anchor World Position` | 世界锚点距离、偏移、间距、扇形 yaw、跟随速度 |
| `03 Projection` | 投影模式、Look Responsive、viewport clamp、pixel snap、角度限制 |
| `04 Hand Shape` | Authored2D 间距、宽度、中心抬升、曲线、底部保护、卡牌 scale、edge drop、不可用透明度 |
| `05 Slot Motion` | 基础 slot 插值、入场 / 离场通用参数 |
| `06 Transition Motion` | Drawn / RunHandEntered / Gained / HandAnchorEntered / Played / Discarded 事件转场方向、来源和入场 timing |
| `07 Transition Audio` | Drawn / RunHandEntered / Gained / HandAnchorEntered 入场音效和统一音量 / 音高倍率 |
| `07 Hover` | hover lift / scale / ZOrder / hit hysteresis |
| `08 Targeting State` | pending targeting、target select deemphasis |
| `09 Gesture` | 按住读牌、拖出提交、快捷键拿起卡牌、inspect 姿态、aim arrow |
| `10 Interaction Feedback` | hover overlay、pressed、confirm、deny、commit、retained feedback |
| `11 Fake 3D & Shadow` | Hover / Drag tilt、pointer velocity filter、perspective strength、独立阴影 offset / opacity / scale |
| `11 Drag Target Feedback` | world / card target 颜色、opacity、arrow snap、card-target focus |
| `12 Camera Look While UI` | hover / pointer 和 drag 期间 camera look override |
| `90 Development Preview` | preview source-only 字段：`bDrawPreviewCardLayer`、`PreviewCardDefinitions`、`PreviewCardCountFallback` |
| `99 Debug` | debug widget、projection debug、motion / drag diagnostics |

`CardLayerWidgetClass` 和 `CardLayerZOrder` 是正式第一人称卡牌层配置，同时服务 Battle / Run runtime hand 与 PIE 预览，不属于 preview-only。`HandCardRenderScale`、`HandMaxEdgeDropPixels`、`bScaleEdgeDropByHandCount`、`ShortHandEdgeDropPixels`、`EdgeDropScaleMinCardCount` 和 `EdgeDropScaleMaxCardCount` 是 runtime hand 表现参数，不是 preview-only。`Development Preview` 只保留预览开关、预览卡牌定义和占位卡数量。

Anchor debug view 会报告 `RawCursorLookOffset`、`AppliedAnchorLookOffset`、`LookInfluenceYaw`、`LookInfluencePitch` 和 `bLookResponsiveProjection`。排查时可以用它区分“鼠标确实产生了 look offset”与“该 offset 是否被当前 ProjectionMode 应用到 hand anchor”。Debug view 不再报告 layout preset 状态。

边缘下坠是纯表现参数，不影响战斗规则、手牌数量或卡牌状态。`HandMaxEdgeDropPixels` 表示大手牌时最外侧卡牌的最大下坠；默认开启 `bScaleEdgeDropByHandCount` 后，5 张及以下使用 `ShortHandEdgeDropPixels`，12 张及以上使用 `HandMaxEdgeDropPixels`，中间数量用 SmoothStep 平滑过渡。左右手锚点牌在规则上承担手牌区域切分语义，但在 first-person hand 表现层仍按普通卡牌参与下坠、缩放、扇形角度和层级计算。推荐起点是 `ShortHandEdgeDropPixels = 64`、`HandMaxEdgeDropPixels = 110`、`EdgeDropScaleMinCardCount = 5`、`EdgeDropScaleMaxCardCount = 12`。

Hover 命中滞后是纯输入表现参数，不影响战斗规则、手牌数量或卡牌状态。`HoverHitHysteresisPixels` 默认 `16`，建议调参范围 `0-64` UMG 布局像素；它只用于卡牌基础命中分界线附近的 hover 稳定性，避免鼠标贴着重叠边界移动时频繁抖动切卡。

## §4 Runtime Source

Runtime source 优先级：

| Source | 写入者 | 行为 |
|---|---|---|
| Battle runtime hand | `UBattleHUD` first-person hand bridge | 使用 `FBattleSnapshot.Hand.Cards`，启用 battle hand hover / hold / drag |
| Run default source | `UWacomRunFirstPersonCardSourceComponent` | 探索期显示默认 Run Card Workspace；当前 provider 是 Run BattleDeck 物理卡和可选投影卡，只读展示 |
| Run menu lease | `UWacomRunMenuWidgetBase` / owning menu（旧 `UWacomMenuWidgetBase` API 仅作 deprecated 兼容桥） | GameMenu 内临时显示候选持有卡，可启用 hold / drag 到 menu Zone |
| Development preview | Anchor development preview | PIE / 开发验证，不是 Battle / Run runtime source |

进入战斗时，GameMode / PlayerController 会清理探索期 Run source 和 active menu lease。退出战斗回到 Exploration 后，PlayerController 重新激活 Run first-person source 并刷新当前默认 Run workspace 展示。

BattleHUD 的 first-person hand bridge 只拥有 `WacomFirstPersonCardLayerSourceIds::BattleHand()` runtime source。清理或 `NativeDestruct` 可能晚于 Run source 重新激活，因此 BattleHUD 解绑自身 delegate 时必须检查 Anchor 当前 `RuntimeCardLayerSourceId`：只有仍为 `BattleHand` 时才关闭 first-person card interaction、取消拖拽和清 runtime data；如果已经被 `RunFirstPersonBattleDeck` 或 menu lease 接管，只能解绑 BattleHUD delegate 和清战斗 world preview，不得改写 Run source 的交互状态。

Runtime source 只拥有卡牌 entries、legacy transition hints、feedback hints、presentation frame hints、presentation gate、hovered card / card target handle、interaction ownership 和 source 生命周期；视觉调参来自 AnchorComponent Details。BattleHUD / Run source 不设置、不清理、也不持有 layout preset override。代码上这些运行时状态由 Anchor 私有 `FWacomFirstPersonCardAnchorRuntimeState` 保存，Anchor 的正式生产写入入口是 `ApplyRuntimeCardLayerSourceLifecycleFrame(FWacomFirstPersonCardLayerSourceLifecycleFrame)`；`CommitRuntimeCardLayerFrame(FWacomFirstPersonCardLayerPresentationFrame)` 仍保留为兼容包装和小型测试入口。Battle / Run 都先构建 frame / lifecycle frame，再由 `CommitMode` 决定提交语义：`PresentationFrame` 写入 entries 并替换 pending frame hints / feedback hints，**即使两个 hint 数组都为空也仍是显式替换**，必须清除旧的 deferred hint；`StateRefresh` 只刷新 entries / interaction 且保留尚未消费的 pending hints，`PreviewOverlay` 写入临时预览 entries 且不消费或清理 pending hints，`Suppressed` 写入 0 entries 并清理 pending hints、legacy hints 和 transient interaction。source lifecycle frame 还统一表达 presentation gate、interaction enabled、cancel drag、runtime data clear 和 visual clear；生产路径不要在 BattleHUD / Run source 里把这些 side API 分散调用。`bApplyAsPresentationFrame` 只保留为旧调用兼容语义；新代码应显式设置 `CommitMode`。Legacy transition hints 只保留给自动化测试和历史兼容，不作为 Battle / Run 正式写入路径。Presentation gate 是 source-scoped 播放闸门：gate 关闭时 entries 可以继续刷新，但 pending frame hints / feedback hints / legacy hints 不会被 `FWacomFirstPersonCardLayerOwner` 消费到 Layer，也就不会在镜头 staging、hand suppression 或其他不可见阶段提前启动播放。Layer 侧也必须按“已应用才消费”处理：如果本次 `SetCardSlots` 没有对应 slot，或对应 slot 暂时不可投影，pending enter hint 或 retained feedback hint 保留到后续刷新；只有 presentation gate 已打开、匹配到 projected slot 并启动 enter playback、匹配到 outgoing slot 并启动 exit playback，或匹配到 projected slot 并启动 retained feedback 后，才移除对应 hint。

BattleHUD runtime 战斗手牌不再有 legacy 2D hand 可见性恢复路径。进入 battle entry staging suppression 时，BattleHUD 会先关闭 `BattleHand` presentation gate、清空当前 card layer visual slot，再写入 0 entries 的空 `BattleHand` runtime source；这既阻止 development preview fallback，也避免旧 slot 被 `SetCardSlots(0)` 当作 outgoing slot 播离场动画。suppression 不再推进 Battle hand presentation controller 的 snapshot baseline；如果首回合 entry reveal frame 已提交到 Anchor 但尚未被 Layer tick 消费，bridge 会在清空 visual source 前把该 frame 还原为 controller 的待播事件，保证解除 suppression 后只重放一次正式 Drawn 入场。退出战斗后的手牌恢复只依赖 Run source ownership 交接，不能通过旧 2D hand 兜底。

打开 Backpack / Pause / Shop / RunEvent 等 GameMenu 时，默认压制 Run default source，避免卡层遮挡菜单。菜单需要卡牌交互时，应显式申请 owned menu lease。

Run default source / menu lease / GameMenu suppression 的所有权由 `UWacomRunFirstPersonCardSourceComponent` 做 desired-state reconcile：source active、RunSession、Pawn Anchor、suppression 和 lease 任一状态变化后，都重新把 Anchor 对齐到当前应显示的 runtime source。缺 RunSession 或 Anchor 时不会把一次失败当成最终状态，而是记录 pending/block reason；条件恢复后自动重试。探索入口（进入关卡、读档完成、战斗返回）统一调用 `AWacomPlayerController::PrepareExplorationRunFirstPersonCardLayer()` 清理 stale menu context 并恢复默认 `RunFirstPersonBattleDeck` source；该 source id 是兼容命名，当前语义是默认 Run workspace。`RefreshRunFirstPersonCardLayer()` 仍保留为兼容/手动诊断入口，但不是唯一正确刷新路径。Run default、active menu lease、suppressed source 和 `BattleHand` 的处理权也由该 component 提供 C++ 查询；PlayerController 不直接比较这些保留 source id。

Run source 的卡牌集合来自 `URunSession::BuildRunCardWorkspaceSnapshot()`。`DefaultExploration` workspace 当前返回 BattleDeck 物理卡和可选 BattleDeck 投影卡；`OwnedCardsFilter` workspace 用于 RunEvent 支付、菜单 lease 和未来其它候选选牌场景，按 request 扫描 Run 的真实 owned zones 并返回每张卡的 `PhysicalZone / ZoneOwnerInstanceId / bIsProjectedBattleDeckCard` metadata。`UWacomRunFirstPersonCardSourceComponent` 把 workspace entries 转成 `FWacomFirstPersonCardLayerEntry` 并缓存 metadata；通用 first-person card layer 不理解 `Backpack / BattleDeck / SpecialZone / BurdenZone`，也不推断 Run 规则。Run source adapter 不再从卡牌战斗 `TargetMode` 推导拖拽行为，而是把 `DefaultExploration / OwnedCardsFilter` workspace entries 写成 `InteractionIntent = DragToDropTarget`。Battle adapter 在 `WacomBattleCardPresentation` 内部把战斗规则目标模式映射成 `CommitNoTarget / AimWorldTarget / AimCardTarget`，通用 layer types 不再提供 TargetMode 转 InteractionIntent 的公共 helper，也不保留 legacy TargetMode debug projection 字段。SlotWidget 只根据 `InteractionIntent` 决定进入无目标拖拽、世界瞄准、手牌瞄准、Run/App drop target 拖拽或仅读牌。未来默认展示切到 Run 专属牌组时，只应替换 Run workspace provider，不重写 Anchor / Layer / Slot gesture。

Run default source 和 provider-backed menu lease 的正式写入入口同样是 `FWacomFirstPersonCardLayerPresentationFrame`。默认 `SourceId` 来自 `WacomFirstPersonCardLayerSourceIds::RunDefault()`，组件仍允许制作侧显式覆写；GameMenu suppression 使用保留的 `RunMenuSuppressed()` source，menu lease 自身继续使用请求里传入的自定义 source id。默认手牌第一次出现、从 GameMenu suppression 恢复、menu lease 交还默认 source、默认 source 新增卡，或 RunEvent / 菜单通过 `OwnedCardsFilter` 筛出候选持有卡并接管 hand 时，组件会为对应可见卡生成 `RunHandEntered` transition hint，并以 `PresentationFrame` commit 写入 Anchor；普通 refresh / hover detail / unchanged revision 使用 `StateRefresh` 或直接跳过，不重播。`RunHandEntered` 是 Run/App-only 表现语义，v1 复用 `Drawn` 入场 profile，不进入 `CardsDrawn`。GameMenu suppression 写入 0 entries 的 `Suppressed` frame，让 card layer 清空当前手牌；只有 inactive、EndPlay、进入 Battle 或 Anchor teardown 才允许硬清 slot motion。Run menu lease 不再支持 entries-only raw 入口；所有菜单候选卡必须来自 `OwnedCardsFilter` provider，以便 `CardInstanceId`、workspace metadata、drag/drop 和入场动画保持同一语义来源。

Run first-person hover / inspect detail 是 App 层只读表现合同：`FWacomRunFirstPersonCardDetailController` 只在 Exploration 且 `UWacomRunFirstPersonCardSourceComponent::CanHandleRunFirstPersonCardLayerSource()` 允许当前 runtime source 时响应 `OnFirstPersonCardLayerCardHovered` 和 `Inspecting` gesture；如果当前 source 是 `BattleHand`、suppressed source 或其他 owner，则必须忽略并隐藏详情。`AWacomPlayerController` 只作为 UObject delegate endpoint 和 RunSession 数据查询入口保留。详情数据使用 `CardInstanceId` 调 `URunSession::FindInstance()` 反查真实持有卡实例，再用现有 `UWacomCardPresentationBuilder::BuildCardDetailViewData()` 构建静态卡牌详情；v1 不显示 Run zone badge、可用性预览或来源提示。hovered visual slot 更新时面板随 slot 重新定位；inspect 期间详情由 inspect 持有，普通 unhover 不隐藏，inspect scrub 切到另一张卡时详情跟随切换；`DraggingNoTargetCard / AimingTargetedCard / ArmedForCommit` 才视为正式拖拽并隐藏详情。Run source clear、进入战斗、menu lease 结束或 Controller teardown 时清理。

Run 详情面板由 App-private `FWacomRunFirstPersonCardDetailController` 接线，并和 `FWacomBattleHUDCardDetailController` 共用 `FWacomFirstPersonCardDetailMotionController` 与 `FWacomFirstPersonCardDetailPanelHost`。PlayerController 在 BeginPlay / 探索手牌准备 / Run source 激活后预热 `UWacomCardDetailPanel / WBP_CardDetailPanel`，先 AddToViewport 并保持 Collapsed + opacity 0，避免第一次 inspect 时创建 WBP 露帧。显示 / 隐藏使用和 Battle 详情同一套淡入淡出、轻微 scale、follow speed 和 side-switch hysteresis 语义；同一 `CardInstanceId` 的 hover layout update 或 inspect drag update 只更新目标 slot，不重复 `FindInstance -> BuildCardDetailViewData -> SetCardDetailData`。scrub 到另一张卡或 Run 持有卡详情数据实际切换时才重新设置面板数据。Run 详情面板复用 PlayerController 的 Run detail authoring 参数控制 class、估算尺寸、padding、z-order、卡牌锚点基础尺寸和 motion 默认值；Battle 详情面板复用 BattleHUD 对应 authoring 参数。二者共享 motion/cache/prewarm/panel-host core，但保留各自的数据来源和 source ownership。

Run first-person drag / drop 由 App-private `FWacomRunFirstPersonCardDragController` 接线。它只在 Run default source 可做 world drop 或 active menu lease 可做 menu zone drop 时绑定 Anchor drag / pointer delegates；`Inspecting` 和正式拖拽期间的 camera-look ownership 委托给 Battle / Run 共用的 `FWacomFirstPersonCardCameraLookBridge`，正式拖拽开始或更新时隐藏 Run 详情，释放时只把 formal drag transaction 交给 `FWacomRunFirstPersonCardDropCoordinator`。Run source 写入的 `DragToDropTarget` intent 使用无目标拖拽的源卡视觉表现：源卡中心跟随指针，由 drop coordinator 解析菜单区域或世界投放目标，而不是由 SlotWidget 推断 Run 规则。Coordinator 是 Run card drop/router 的唯一入口，按 menu lease zone adapter 优先、world receiver adapter 其次的顺序处理 probe / preview / resolve / submit / debug；PlayerController 只保留 UObject delegate endpoint、drop target 注册转发和 RunSession / world hit 查询能力，并通过显式 context contract 把这些能力注入 coordinator，不再让 coordinator 直接读取 Controller 私有状态。`UWacomRunMenuDropTargetWidget` 仍只做 Zone handle 与轻量 preview，RunEvent payment / owning menu 和 `UWacomRunWorldCardDropReceiverComponent` 继续决定各自规则提交结果，避免把 Run 规则或 world interaction 结算塞进 card layer adapter。

## §5 Slot Motion 与 Transition

Layer 使用稳定 motion key 复用 slot widget：

- Battle runtime hand 优先使用 `CardInstanceId`。
- Development preview 或 placeholder 使用 `StaticIndex:{Index}`。
- 同一 key 重新进入时复用 active 或回收 outgoing widget，避免幽灵 widget。

Anchor 每帧计算基础目标 slot，Layer 只在输入 slot、transition hint、配置或生命周期状态实际变化时完整 reconcile。SlotWidget 保留输入、gesture、反馈计时器和最终 UMG 渲染；App-private `FWacomFirstPersonCardMotionMixer` 以纯值计算固定执行 `layout -> state -> gesture / transition -> local feedback` 合成顺序，并负责 motion intent、插值、到达判定和最终局部 transform。带明确语义 hint 的固定时长 enter / exit 由 App-private `FWacomFirstPersonCardTransitionPlayback` 以互斥 `None / Enter / Exit` 状态播放：Enter 每帧追随最新手牌目标，Exit 在启动时冻结终点；新播放、slot 重用、force-settle 和 teardown 都显式中断旧状态。Layer / Slot 不因此获得 Battle 或 Run 规则职责。

Slot motion 现在按语义选择 motion intent / motion profile：`Layout` 负责普通手牌重排，`Hover` 负责悬浮 presentation，`Pending` 负责等待选目标源卡和非源卡弱化，`DragTargetFocus` 负责当前拖拽指针压中的唯一手牌目标，`Enter` / `Exit` 负责入场和离场。每个 `FWacomFirstPersonCardMotionProfile` 包含 `MotionSpeed / OpacitySpeed / EasePower`，Tick 时用当前 active profile 计算位置、角度、缩放和透明度追踪 alpha。带明确语义 hint 的 enter / exit 使用独立 elapsed / delay / duration playback；普通 layout / hover / drag 和无语义的兼容离场仍沿用 profile 追踪模型。

Anchor Details 的默认入口仍是 `CardSlotMotionSpeed / CardSlotOpacitySpeed / CardSlotMotionEasePower`，这三个值会映射到全部 profile；如果某个 profile 保持默认值，Normalize 时也会继承这三个旧参数。当前只开放少量高级覆盖项：`bOverrideHoverMotionProfile` 单独调悬浮，`bOverrideDragTargetFocusMotionProfile` 单独调拖拽手牌目标 focus，`bOverrideEnterExitMotionProfile` 单独调入场和离场。`Layout` 和 `Pending` 仍继承全局参数，避免 Details 面板过早变成全量 motion 表。`CardSlotMotionEasePower = 1` 保持线性旧手感；大于 `1` 时当前 profile 每帧起步更柔和，小于 `1` 时更快贴近目标。

Motion profile 只影响最终 visual slot 追踪，不改变 `InputHitCenter / InputHitScale / InputHitAngleDegrees` 等稳定命中几何，也不改变 hover / press / drag target resolver。候选目标 affordance 仍只控制 `FeedbackOverlay`，不会触发 `DragTargetFocus` 的 lift / scale / ZOrder；deny shake、confirm / commit pulse 和 `InteractionFeedbackImage` 也不接入 motion profile。

Transition audio 是 first-person card layer 的表现能力，不属于 Battle / Run 规则。Anchor `07 Transition Audio` 下的 `DrawnCardEnterSound / RunHandCardEnterSound / GainedCardEnterSound / HandAnchorCardEnterSound` 只在对应 transition hint 被 Layer 消费、目标 slot 可投影并实际开始入场 playback 时触发；没有 transition hint 的普通新 slot、layout refresh、reflow、投影恢复或 hover / drag presentation 不播放入场音效。入场音效使用 UI 2D 播放，`CardEnterSoundVolumeMultiplier / CardEnterSoundPitchMultiplier` 控制统一音量和音高；声音触发点会尊重 `SequenceIndex * StaggerSeconds`，多张抽牌不会在同一帧一起响。

默认表现：

- 新卡按 enter offset 和 enter opacity 淡入。
- 移除或投影失败的可见卡进入 outgoing，短暂淡出 / 下滑后移除。
- 同一 slot key 的普通 layout refresh 不再按距离阈值 snap；仍可复用 visual 的卡牌从当前 visual slot 平滑追向最新目标。
- 同一 slot key 从不可投影恢复到可投影时，SlotWidget 必须立即恢复 visual projection 并显示在最新目标 slot，不能依赖 collapsed widget 的后续 tick 自行恢复。
- Drawn / RunHandEntered / Gained / HandAnchorEntered / Played / Discarded transition hint 只改变表现来源或离场方向，不改变 snapshot 真相、命令路径或 slot key。

`CardsDrawn` 事件由 `FWacomBattleHandPresentationController` 转成 Battle hand presentation frame 中的 `Drawn` transition hint。Controller 优先使用 `CardsDrawn.CardInstanceIds` 中的真实抽牌 / 移入手牌普通卡 ID，并且只为这些 ID 中仍存在于最新 `BattleSnapshot.Hand.Cards` 的普通卡生成 hint；同一批可见 hint 会按目标 hand snapshot 的普通手牌槽位从左到右写入连续稳定的 `SequenceIndex / SequenceCount`，被手牌上限立刻弃掉的 ID 不占可见动画序列。旧式或测试手写的 Count-only `CardsDrawn` 事件仍保留兼容 fallback：仅在 `CardInstanceIds` 为空时，才按 baseline snapshot 与 next snapshot 中“新出现的普通卡”分配 Drawn hint，左右手 anchor 不占抽牌预算。Battle hand 不再让普通 snapshot refresh 直接决定“新卡是否可见”：有 pending 事件时必须提交显式 `entries + hints` frame，没有 pending 事件时才是普通 entries refresh；状态刷新、输入解锁刷新或 CommandBar 可用性刷新不会用空 hints 覆盖尚未消费的 Drawn frame。Anchor 把 presentation frame hints 与 legacy hints 分开保存，Layer owner 优先消费 frame hints，消费成功后才交给 `UWacomFirstPersonCardLayerWidget` reconcile。Layer 只消费这些表现语义，不重新推断战斗事件、不读牌堆、不修改 Battle snapshot；如果 battle entry presentation gate 尚未打开，或镜头 staging / viewport 投影导致 slot 暂不可见，Drawn hint 会留在 Anchor runtime source 或 Layer pending set，直到 gate 打开且 slot 可见并真正启动入场，避免卡牌在不可见阶段播放完后直接落位。当前 `Drawn` 入场使用 `06 Transition Motion` 下的专用参数：`DrawnCardEnterDurationSeconds` 控制固定时长播放，`DrawnCardEnterStaggerSeconds` 按 sequence index 做批次错峰，`DrawnCardEnterArcLiftPixels` 叠加抛物线式上扬弧线，`DrawnCardEnterEasePower` 控制入场 ease，`bBlockInteractionDuringDrawnCardEnter` 决定播放期间是否临时禁止 hover / press / drag。播放完成后 SlotWidget 交回普通 slot motion，由 `Layout / Hover / Pending / DragTargetFocus` 等 motion intent 继续接管。

`Drawn / RunHandEntered / Gained / HandAnchorEntered` 的入场音效和 motion profile 一起从 transition hint 解析出来：普通 refresh 只更新目标 slot，不能重播同一张卡的入场音效；slot 在 projection gate 打开前不可见时，音效和入场动画一样推迟到真正启动 playback 的那一帧。

Run 探索期默认手牌和 provider-backed menu lease 的卡牌进入使用 `RunHandEntered` transition hint。它由 `UWacomRunFirstPersonCardSourceComponent` 生成并装入 `FWacomFirstPersonCardLayerPresentationFrame`：默认 `RunFirstPersonBattleDeck` source 初次显示或从菜单恢复时为全部可见默认手牌生成，默认 source 已显示时只为新增 card id 生成；RunEvent 支付、菜单临时选牌等 `OwnedCardsFilter` provider lease 初次接管或新增候选卡时也为对应可见卡生成。Run menu lease 不存在无动画 entries-only 正式入口；如果某个菜单不希望播放入场，应显式提交不带 hints 的 `PresentationFrame` 或走后续可配置动画策略，而不是绕过 workspace provider。v1 中 `RunHandEntered` 复用 Drawn 的 origin、offset、duration、stagger、arc lift、ease 和交互阻塞参数。

左右手 anchor 的生成入手使用 `HandAnchorEntered` transition hint。它是 UI-only 表现语义，不进入 `CardsDrawn.CardInstanceIds`，也不改变左右手牌保留 / 生成规则。EndTurn `TurnStartDraw` phase 会先临时隐藏本次新出现的左右手 anchor，让普通抽牌完成；随后 `TurnStartHandAnchorEnter` phase 提交完整 hand snapshot 并只为这些 anchor 播放生成入手。Battle entry opening reveal 同样分两帧：普通 `Drawn` frame 先播，bridge 等播放结束或无播放时提交完整 snapshot + `HandAnchorEntered` follow-up frame。`HandAnchorEntered` 的 origin、offset、scale、angle、duration、stagger、arc lift、ease 和播放期间交互阻塞都在 Anchor `06 Transition Motion` 下独立调参。

`CardsRetained` 事件由同一个 controller 转成 Battle hand presentation frame 中的 retained feedback hint，而不是 `Retained` transition hint。Controller 的 loose event 路径只为仍存在于下一份普通手牌中的卡生成 feedback，过滤无效 ID、重复 ID、左右手 anchor 和已经离开手牌的 ID；EndTurn `TurnEndRetain` phase 会在不改变 `CardsRetained` 规则事件的前提下，为 retain checkpoint snapshot 中仍存在的左右手 anchor 追加同款 retained feedback。同一批 feedback 使用稳定 `SequenceIndex / SequenceCount`。Layer 侧 retained feedback 是 post-layout pulse：卡牌保持当前 slot identity 和 layout target，只短促上浮、轻微放大并播放暖金色 feedback overlay。普通 refresh 不会取消正在播放的 retained feedback；slot 暂不可投影时 hint 会留到下一次 projected refresh。

`CardGained` 事件由同一个 controller 转成 Battle hand presentation frame 中的 `Gained` transition hint。`Gained` 和 `Drawn / RunHandEntered / HandAnchorEntered` 一样必须启动有限时长 enter playback；Anchor `06 Transition Motion` 下的 `GainedCardEnterDurationSeconds`、`GainedCardEnterStaggerSeconds`、`GainedCardEnterArcLiftPixels`、`GainedCardEnterEasePower` 和 `bBlockInteractionDuringGainedCardEnter` 控制奖励卡入场的时长、错峰、弧线、缓动和播放期间交互阻塞。

`Played / Discarded` 离场使用和 enter 对称的固定 elapsed playback。BattleHUD 每次提交 BattleHand presentation frame 时，把当前 UMG 几何中心转成 DPI-aware 逻辑 viewport 坐标，并随同一 source lifecycle frame 写入 `DrawPile / DiscardPile / PlayTarget` presentation anchors；Anchor runtime state 按 source 保存，source 切换、runtime clear 或 visual suppression 时清理。`Drawn` 使用 DrawPile 完整坐标作为起点，`Discarded` 使用 DiscardPile 完整坐标作为终点，二者不再叠加旧 authored offset；`RunHandEntered / Gained / HandAnchorEntered` 暂不使用这些 Battle 锚点。`Played` 成功提交后优先使用 release resolver 已确认的真实目标屏幕位置；没有真实目标时使用 PlayTarget；锚点无效时才回退旧 origin / offset。Layer 只消费这些表现坐标，不重新判断目标是否合法。连续 `CardDiscarded / HandLimitDiscarded / CardExhausted` loose events 和 EndTurn discard phase 都携带稳定 `SequenceIndex / SequenceCount`；`DiscardedCardExitStaggerSeconds`（默认 `0.06s`）据此错峰启动，`CardSlotExitDuration` 控制单张离场时长。表现计划 phase 超时时必须先调用 Anchor / Layer force-settle，清除未应用 hint、把 active slot 收到最终目标并移除 outgoing slot，再进入下一 phase，不能让旧阶段动画与新阶段重叠。

`bEnableReadableTransitionOrigins` 控制没有有效 presentation anchor 时的旧 origin / offset fallback，不关闭有限时长播放、错峰和弧线；有效的 Battle MotionAnchor 或真实 Played target 始终是完整空间事实。需要在 PIE 中验证抽牌、Run 手牌入场、战斗奖励卡或左右手生成手感时，优先调整 WBP MotionAnchor 位置和 `06 Transition Motion` 的非位置参数；不应在 BattleHUD、Run source 或 BattleSession 中硬编码动画位置、延迟或曲线。

本项目可以参考本地 Godot Fake3D Card Game UI Demo 0.2 的行为目标，例如真实牌堆起终点、批次 stagger、两段式出牌、翻面、tilt / shadow depth 和可中断通道；该参考项目采用 GPL-3.0，因此 Wacom 只吸收动作语义和节奏判断，必须以现有 UE / UMG / Material 架构独立实现，不复制其代码、shader、资产或工程结构。

Card Depth 是独立于 slot layout / semantic transition / local feedback 的次级表现通道。Hover 根据 DPI-aware widget-space pointer、当前视觉卡牌中心、扇形角度、主体尺寸和 render scale 计算卡内归一化位置；Drag 不使用卡内位置，因为无目标拖拽时卡牌中心会跟随 pointer，而是使用低通后的 pointer velocity 形成惯性倾斜。倾角和阴影使用 `1 - exp(-Speed * DeltaTime)` 的帧率无关追踪，可在 pointer 停止、release、cancel、source clear 或 Widget 复用时返回稳定状态。`Enter / Exit` 播放期间 Card Depth 目标强制压平到基础阴影，避免空间转场与局部透视争夺主动作。目标候选卡仍保持平面；只有 Hover 卡或正式 Drag source 消费 tilt。

Anchor `11 Fake 3D & Shadow` 是 Battle / Run 共用的数值制作入口。默认采用克制纸牌实体感：Hover 最大 `6°`、Drag 最大 `9°`、Drag 达到最大倾角速度 `1400 UMG px/s`，基础 / Hover / Drag 阴影 Y 偏移分别为 `4 / 10 / 22 px`。关闭 `bEnableCardFake3D` 时材质倾角归零但独立阴影可以继续工作；关闭 `bEnableCardIndependentShadow` 时只关闭阴影，不影响卡面内容或命中。Retainer Effect Material 由 WBP 持有并随 Widget 资产加载，运行时不通过软引用同步加载。

Card Depth 材质采用 DreamShader 1.4.1 制作：`DShader/Material/Card/M_FirstPersonCard_Fake3D.dsm` 生成 `/Game/DreamMaterials/Card/M_FirstPersonCard_Fake3D`，`DShader/Material/Card/M_FirstPersonCard_Shadow.dsm` 生成 `/Game/DreamMaterials/Card/M_FirstPersonCard_Shadow`。前者是唯一 Retainer Effect Material，动态采样名固定为 `Texture`，运行时参数固定为 `TiltX / TiltY / PerspectiveStrength`；后者只生成圆角软阴影 mask，位移、缩放和整体透明度仍由 C++ Card Depth 控制。`.dsm` 是可版本管理的材质真源，生成 `.uasset` 只作为 WBP 制作结果；改变参数名、生成路径或 ShadowHost 宽高比时必须同步本节与 WBP binding 文档。

Layer debug view 记录 active / outgoing / RootCanvas child / ticking slot 和本次刷新创建、复用、移除、异常修复数量。诊断日志默认关闭，只在手动排查时开启。

## §6 Battle 交互

第一人称战斗手牌不恢复旧 2D hand 的拖拽语义，也不直接提交 `UBattleSession`。

当前交互：

| 手势 | 行为 |
|---|---|
| hover idle card | 可查看详情；可打卡获得 lift / scale / ZOrder，不可打卡只允许读牌 |
| quick press / release | 中性返回手牌，不提交卡牌 |
| keyboard `1~7` | 直接拿起对应手牌并进入拖拽 / 瞄准手势；后续 release 仍走同一拖拽提交路径 |
| hold | 超过读牌延迟后进入 inspect，源卡移动到读牌位置并显示 first-person detail，松开不提交 |
| drag no-target card | 向上超过 commit distance 后 armed，release 才提交无目标出牌 |
| drag targeted card | 源卡保持 selected-source，C++ arrow 指向鼠标；release 到合法 target 后提交 |

快捷键 `1~7` 进入拖拽时，PlayerController 只收集 one-based hand index 和当前鼠标 widget-space 坐标，再交给 BattleHUD / first-person hand bridge 从已同步的 battle hand snapshot 解析 `CardInstanceId` 并启动 external drag；PlayerController 不读取 `UBattleSession` 或自行构造 `FBattleSnapshot`。拖拽内部使用双位置初始化：`PressScreenPosition` 固定为被选中卡牌的基础手牌位置，`CurrentScreenPosition / PointerViewportPosition` 使用 PlayerController 读取到的当前鼠标 widget-space 坐标；没有鼠标坐标时才退回卡牌自身位置。`CurrentPointerView` 只表示普通 hover / pointer view，不作为快捷键拖拽启动坐标来源，避免鼠标悬浮在 B 卡时按 A 卡快捷键却串用 B 的旧 pointer。

快捷键拖拽启动后，`AWacomPlayerController` 每帧执行 active-drag pointer pump：如果当前 Anchor 的 first-person card layer 存在 active gesture，就优先通过 Slate viewport geometry 读取全局 cursor 在 viewport 内的 widget-space 坐标；如果 Slate viewport geometry 不可用，再退回 `GetMousePosition()` + `UWidgetLayoutLibrary::GetViewportScale()` 的 PlayerController 路径。随后调用 `UpdateFirstPersonCardDragPointer()` 喂给 Layer。Layer 继续复用 active gesture slot 的 `UpdateGesture()` 链路刷新 `DragView`、aim arrow、card target probe 和上层 camera look override。没有 active gesture、读取不到鼠标位置，或当前 active gesture 不是 `ExternalPointer` 来源时，pump 为 no-op，不会改变普通 hover / pointer view，也不会覆盖鼠标来源拖拽的最新指针。左键 release 时，PlayerController 会先尝试释放 active first-person drag：能读取鼠标坐标时先 pump 到最新位置再 release，读取不到时使用 DragView 当前指针位置 release；只有没有 active drag 时，左键 release 才继续走 Battle scene click、Run tunnel branch 或 Run world interactable click 路由。

Battle 回合边界快捷键 `IA_Wait` / `IA_EndTurn` 在 PlayerController 入口先检查 first-person card layer active gesture。只要当前手势不是 `Idle` / `Cancelled`（包括 pressed、inspect、no-target drag、targeted aim、armed commit），本次快捷键会取消并消费该手势，不向 BattleHUD 提交等待或结束回合；下一次按键才按普通命令入口执行。取消后源卡保留当前 visual slot，并继续用 slot motion 返回手牌布局，不触发普通布局大跳变 reset。单纯 hover 不属于 active gesture，不会阻塞等待或结束回合。

需要敌方部位目标的卡 release 到合法 world enemy part 后，BattleHUD 调用现有 play-card world target 路径。需要手牌目标的卡 release 到合法 first-person card target 后，BattleHUD 提交 hand-card target。UI 手势层只提交 target identity，不判断加费、减费、弃置或消耗规则。Battle first-person hand bridge 生成 hand-card release / probe / full-hand affordance 时，只消费 `UBattleSession::ValidateTargetWithCard()` 返回的合法性和 reject reason；非 hand-card source 的卡牌目标停留在 probe-only，真实 hand-card source 的无效目标才显示 invalid card-target feedback。

Battle Action Preview 只在有效释放语义成立时由 BattleHUD / first-person hand bridge 请求：拖拽源卡压中合法 world enemy part、合法 hand-card target，或无目标卡已经达到 `ArmedForCommit` 提交距离后，bridge 调用 `UBattleSession::BuildCardActionPreview()`，再把只读 projected values 交给 BattleHUD runtime 更新卡面 / 详情 / 玩家状态条 / 敌人部位面板。规则层还要求当前阶段是 `PlayerAction` 并通过完整 PlayCard preflight；`PendingKnockdownChoice`、BattleEnd 或其它非玩家行动阶段即使目标本身仍合法，也不会生成 Action Preview。单纯拖出手牌区但尚未 armed、仍在寻找目标或目标无效时，first-person layer 只显示轻量候选目标提示，不显示玩家侧回血、护盾或敌人净结果。release、cancel、离开有效目标、退出无目标 armed 状态、snapshot version 变化或 BattleEnd 时，BattleHUD 必须清理 action preview，让 hand layer、玩家状态条和敌人面板恢复最近一次真实 Snapshot / ViewData。

悬浮和拖拽期间，卡牌层都会记录 DPI-aware widget-space 指针和归一化视口坐标。普通 hover 的 mouse move 被 UMG 处理后，`UWacomFirstPersonCardLayerWidget` 会广播 `FWacomFirstPersonCardPointerView`，由 BattleHUD 写入 Battle camera look 临时 override，探索 / Run 则由 `AWacomPlayerController` 写入 Run Tunnel cursor look override。因此鼠标在悬浮放大的卡牌上移动时，镜头仍会跟随当前指针，不会等离开卡牌后突然跳到新位置。

拖拽过程中仍保留 UMG mouse capture，并继续通过 `FWacomFirstPersonCardDragView` 传递拖拽指针。SlotWidget mouse capture 负责鼠标按下、Pressed 阶段拖拽阈值判断、Inspect 阶段、鼠标来源正式拖拽 pointer 更新和 release/cancel 路由；快捷键或程序化启动的 `ExternalPointer` 拖拽才由 PlayerController active-drag pointer pump 写入 `CurrentScreenPosition / PointerViewportPosition`。这样鼠标来源拖拽不会被同帧偏旧的全局 pump 坐标覆盖，external drag 又可以在没有真实 mouse capture 的情况下持续跟随全局 cursor。`Inspecting` 读牌姿态和正式拖拽一样使用 `FWacomFirstPersonCardDragView` 刷新 drag camera look override；区别是 Inspect 不触发 menu / world / battle drop probe，松开也不提交。拖拽镜头旧参数 `bAllowCameraLookDuringCardDrag`、`CardDragCameraLookScale`、`CardDragCameraLookInterpSpeedOverride` 继续只控制 drag override，保持既有资产兼容；hover / pointer 通用路径使用 `bAllowCameraLookDuringCardPointer`、`CardPointerCameraLookScale`、`CardPointerCameraLookInterpSpeedOverride`。这些 camera look 参数当前推荐在 AnchorComponent Details 的 `12 Camera Look While UI` 分类中调整。拖拽 active 时会清空并压制普通 pointer view，避免 hover override 与 drag override 抢同一个 camera look 状态。
当 Layer 因 BattleEnd、菜单切换、source clear、Deactivate 或关闭交互而程序化取消手势时，必须由 Layer 释放它自己持有的 Slate pointer capture，再清理 Slot gesture state；不得依赖后续 mouse-up 返回 `ReleaseMouseCapture`，也不得无条件释放其他 UI 的 capture。

Layer pointer arbitration 是 first-person hand 输入的正式入口。`UWacomFirstPersonCardLayerSlotWidget` 的 Slate mouse down / move / up 只把 pointer 事件转交给 `UWacomFirstPersonCardLayerWidget`，再按 Layer 返回的 route action 映射成 `Unhandled`、`Handled`、`CaptureMouse` 或 `ReleaseMouseCapture`；没有 Owner Layer 的 hand Slot 不再自管 press / drag / release 生命周期。`PressedSlotWidget` 只在没有 active gesture 且 press 成功开启新手势时写入。已有 active gesture 时，release 永远优先释放当前 gesture；mouse-origin drag 的 slot move 在 `Pressed / Inspecting / DraggingNoTargetCard / ArmedForCommit / AimingTargetedCard` 阶段都可以写入 pointer，用于启动拖拽、读牌姿态和鼠标来源拖拽持续更新。external-origin drag 的 slot enter / move 只消费并压制普通 hover，不改写正式拖拽 pointer；它的 pointer 只能由 PlayerController pump 写入。external drag 下点击目标手牌时，slot press 只更新当前源卡 drag pointer / card target，不抢占 pressed slot，也不重新开启目标卡手势。

鼠标拖拽和快捷键拖拽进入 active drag 时共用 SlotWidget 内部的 card-drag promotion 路径。拖拽内部同时记录两条互不替代的输入语义：`GestureSource` 表示手势起源和生命周期意图，目前为 `MousePress` 或 `KeyboardShortcut`；`GestureInputSource` 表示持续 pointer owner，目前为 `MousePointer` 或 `ExternalPointer`。真实鼠标拖拽使用 `MousePress + MousePointer`，`1~7` 快捷键拖拽使用 `KeyboardShortcut + ExternalPointer`。`GestureSource` 不直接改变提交规则、目标合法性或视觉动画，只让后续输入生命周期可以按起源扩展；`GestureInputSource` 决定正式拖拽态的持续 pointer 写入来源：`MousePointer` 只接受 slot pointer route，`ExternalPointer` 只接受 PlayerController pump。SlotWidget 的 promotion 分支只消费 `InteractionIntent`：`CommitNoTarget / DragToDropTarget` 进入 `DraggingNoTargetCard / ArmedForCommit`，`AimWorldTarget / AimCardTarget` 进入 `AimingTargetedCard`，`InspectOnly` 不升级为正式拖拽。LayerWidget slot refresh 的等价判断同样只比较正式 entry facts 和 `InteractionIntent`，不会同步或保留旧 TargetMode 调试投影。无目标卡和 Run/App drop target 卡的 `CurrentScreenPosition / PointerViewportPosition` 会立即更新，用于提交距离、DragView 和 release；源卡视觉位置以当前 pointer 作为 drag override 的视觉中心，再由同一套 slot motion 追踪该 override，并在拖拽 override 中把源卡角度归零，不继承手牌扇形角度。需要目标的战斗卡牌仍保持 aim arrow 端点立即跟随真实 pointer。

快捷键启动的 external drag 没有真实鼠标按住状态，因此鼠标点击目标手牌时，目标 SlotWidget 的 mouse down 不能重新占有 `PressedSlotWidget` 或开始目标卡的新 press。Layer 在已有 active gesture 时会把这次 pointer press 作为当前 drag 的最终指针更新并消费事件，随后 mouse up 释放的仍是原来的源卡；这样 `HandCard` 目标卡牌可以通过“快捷键进入拖拽 -> 点击另一张手牌”完成 card-to-card release，和普通鼠标拖拽到目标卡后松开左键得到一致提交语义。

读牌姿态支持 inspect scrub。鼠标长按进入 `Inspecting` 后，Layer 会用所有 projected、非 exiting、有效、可交互手牌 card body 的稳定 input hit 几何计算整体 AABB，再加 `CardInspectScrubHandPaddingPixels`（默认 `32 x 48` UMG 布局像素）作为 scrub 区域。pointer 仍在该区域内时，Slot 继续更新 pointer / camera diagnostics，但禁止 `Inspecting -> DraggingNoTargetCard / AimingTargetedCard` 的距离升级；pointer 命中另一张可开始读牌/拖拽的 card body 时，Layer 会把 inspect source 静默从当前卡转移到目标卡，旧卡不广播 cancel，新卡广播新的 inspect started，用于详情面板切换。pointer 落在卡牌缝隙但仍在 scrub 区域内时保持上一张预览卡；如果已经从 A scrub 到 B，之后移出 scrub 区域并超过拖拽阈值，进入拖拽/瞄准的是当前预览源 B。

Hover 输入命中与最终视觉几何分离。Anchor 先投影并平滑整副手牌中心，随后把 resolved config、投影点、viewport size 和 runtime state 交给私有 `FWacomFirstPersonCardSlotLayoutBuilder`。Builder 只写入基础布局、基础 `RenderScale / RenderOpacity / RenderAngleDegrees / ZOrder`、稳定输入几何 `InputHitCenter / InputHitScale / InputHitAngleDegrees / InputHitOrder`，以及 `bIsHovered`、`bHasPendingTargetingCardInHand`、`Entry.bIsPendingTargeting` 等状态标记。Hover lift / scale / ZOrder、pending lift / scale / angle / ZOrder、target-select deemphasis 和 drag card-target focus 统一由 `UWacomFirstPersonCardLayerSlotWidget` 的 presentation resolver 合成。悬浮卡仍然可以放大、抬升并绘制在上层，但鼠标命中哪张牌由 `UWacomFirstPersonCardLayerWidget` 使用基础几何统一解析，不再由被 Slate 命中的单个 slot 自行决定。

`FWacomFirstPersonCardLayerSlotView` 本轮仍保留原字段名以降低调用方和测试改动量，但语义需要区分：`GetSlotView()` 更接近基础 slot + state flags，`GetVisualSlotView()` 是 SlotWidget 合成和 motion 后的最终视觉真相。详情面板、hover update、drag arrow 和 card-target feedback 应使用 final visual slot 语义；稳定 hover / press / drag 起手命中应继续使用基础 input hit 几何。

视觉状态优先级固定为：pending source 高于普通 hover；drag card-target focus 不触发普通 hover，只负责当前指针压中的唯一目标卡 lift / scale / ZOrder；target affordance 只控制 overlay，不参与 lift / scale / ZOrder；pressed、commit pulse 和 deny shake 保留为短时 micro feedback，其中 deny 只做 shake + 边缘 / 暗角反馈，不改变主 scale / ZOrder。

源卡短时交互反馈统一走 `InteractionFeedbackImage`，包括 `Pressed / Confirm / Commit / Deny / Retained`。SlotWidget 只计算状态、shake、scale 和 `FWacomFirstPersonCardInteractionFeedbackView`，然后交给 `UWacomFirstPersonCardViewWidget`；实际尺寸、层级和默认材质由 WBP 内的 `InteractionFeedbackImage` 控件负责。Retained feedback 是 Battle event 派生的非阻塞 pulse，不改变 slot motion target、命中区或 transition kind。材质来源有两级：AnchorComponent Details 的 `InteractionFeedbackMaterial` 显式 override 优先；为空时 C++ 会复用 `InteractionFeedbackImage` 自身 brush 上的材质，作为 WBP 默认材质。若没有任何材质，pressed / confirm / commit / retained 会退化为普通 tint，deny 仍保留 shake，且不会回退成整卡红色填充。`DenyFeedbackEdgeImage` 旧 fallback 已删除，新制作主线必须使用 `InteractionFeedbackImage`。拖拽 `InvalidCardTarget / ValidCardTarget`、card probe、commit ready 和 playable hover 等目标/候选提示继续使用 `FeedbackOverlay` full-card overlay 语义，但 overlay 控件本身也由 `WBP_FPCardView` 绑定和控制尺寸。

`InteractionFeedbackImage` 材质需要支持以下参数名，C++ 会在每次反馈刷新时写入动态材质实例：

| 参数 | 类型 | 含义 |
|---|---|---|
| `FeedbackColor` | Vector | 当前反馈颜色，来自 pressed / confirm / commit / deny 对应配置 |
| `EdgeWidth` | Scalar | 四边高亮宽度，UV 单位 |
| `EdgeSoftness` | Scalar | 边缘向内淡出的柔和宽度，UV 单位 |
| `VignetteStrength` | Scalar | 暗角强度 |
| `VignetteRadius` | Scalar | 暗角开始出现的中心距离 |
| `VignetteSoftness` | Scalar | 暗角淡入柔和度 |
| `Opacity` | Scalar | 状态基础不透明度 |
| `Pulse` | Scalar | 反馈生命周期脉冲，0 到 1 |

稳定命中 resolver 只考虑 projected、非 exiting、有效 `CardInstanceId` 且可交互的 active slot。透明 bleed 不参与命中，命中范围仍只使用 `UWacomCardView.FixedCardBodyHitSize` 对应的 card body bounds。多张基础 body 重叠时，不按 hover 后 ZOrder 抢输入，而是按手牌左右顺序的相邻中心线分界选择；当前 hover 卡在分界线附近享有 `HoverHitHysteresisPixels` 滞后区，越过滞后区后才切到相邻卡。

Hover、press / drag 起手和拖拽中的 card target probe 复用同一个稳定命中 resolver。这样即使视觉上放大的悬浮卡覆盖了右侧卡，鼠标移动到右侧卡的基础区域时也能自然切到右侧卡；拖拽探测手牌目标时会排除源卡，并继续保留现有 CardTarget probe / valid / invalid feedback 语义。

拖拽到手牌目标时，稳定命中 resolver 只负责解析目标 `CardInstanceId` 和目标 slot identity，不负责判断目标是否合法。BattleHUD 继续通过 `UBattleSession::ValidateTargetWithCard()` 校验，并把 `CardProbe`、`ValidCardTarget` 或 `InvalidCardTarget` 下发回 first-person card layer。同一拖拽目标上的 `ValidCardTarget / InvalidCardTarget` 会持续保留；只有指针切到另一张目标卡、离开 card body，或当前状态还没有 HUD 校验结果时，layer 才回到本地 `CardProbe` 等待下一次校验。

Card target 选中反馈拆成候选 affordance 和当前 focus 两层，不复用普通 hover。Affordance 表示一张手牌是否可作为当前源卡的目标，只负责绿色 / 红色 overlay，可多张同时存在；focus 表示拖拽指针当前命中的唯一目标卡，才应用 focus lift、scale 和 ZOrder boost。`CardProbe / ValidCardTarget / InvalidCardTarget` 都可作为 focus 状态，并继续用 overlay 颜色区分 probe、valid、invalid；该状态不调用普通 hover，不广播 hover / detail delegate，不写 `HoveredCardInstanceId`，也不影响 hover 稳定命中。像“加费测试”这类允许普通手牌和左右手锚点的 HandCard 目标卡，会显示全手牌合法候选全绿，但只有指针压中的那张卡获得唯一 focus。拖拽进入 active 状态后会清空并压制普通 hover，pointer enter / move 只更新拖拽手势和 CardTarget probe；release / cancel 当帧不会立即恢复 ordinary hover，避免目标卡 focus 与 hover lerp 叠加造成双重放大或双重缩小。release 成功后如果指针仍压在手牌 card body 上，Layer 会刷新普通 pointer view 但保持 `bIsHovered=false`，让 HUD / Run 的 camera look override 在 drag override 清理后同帧接续到当前指针，避免镜头先回到默认/中心再跳回鼠标方向。`bEnableDragTargetFeedback` 只控制 probe / focus / overlay 的显示；关闭后拖拽目标 identity 仍可被解析并交给 HUD / Battle 规则处理。

Battle first-person 拖拽进入有效释放语义后，Action Preview 不只更新当前压中的目标部位。HUD 会把 `BuildCardActionPreview()` 返回的所有 projected enemy part facts 同步到敌人聚合面板和对应场景部位 prediction badge；如果一次出牌推进多个部位到行动，所有这些部位都会显示 projected 先机 `0` 和 action risk。单纯拖出手牌区但未命中有效目标时仍只显示轻量可作用对象提示，不显示玩家侧收益或敌人净结果。

Drag-target focus 默认参数为 `DragCardTargetFocusLiftPixels = 18`、`DragCardTargetFocusScale = 1.045`、`DragCardTargetFocusZOrderBoost = 650`。推荐调参范围：lift `0-48` UMG 布局像素、scale `1.0-1.12`、ZOrder boost `0-1400`。这些都是表现层参数，不改变手牌顺序、牌堆、出牌结算或 HandCard target validation。

## §7 Target / Drop 边界

First-person drag target 来源：

| Target | 来源 | 提交方 |
|---|---|---|
| Battle world enemy part | PlayerController 按 drag widget position 做 world probe，命中当前 SceneEnemyHost registry 的 part handle | BattleHUD command flow |
| Battle hand card | First-person card layer 的 card target bridge | BattleHUD command flow |
| Run world object | `FWacomRunFirstPersonCardDropCoordinator` world receiver adapter probes `Interaction.Target.Run.Object` | Run world card drop receiver + RunSession |
| Run menu zone | Registered `UWacomRunMenuDropTargetWidget` | Owning menu submit policy |

Battle enemy world target 只来自当前 SceneEnemyHost registry 中的 PartActor / WorldTargetBridge。旧敌方 2D fallback 已删除，不再作为 first-person drag/drop target 或 TargetCue 注册入口。

Battle 目标合法性由 `UBattleSession::ValidateTargetWithCard()` 和 PlayCard resolver 判定。Run world drop 合法性由 receiver 和 `URunSession::ValidateRunWorldCardInteraction()` 判定。Run menu Zone drop 默认 probe-only，只有 owning menu 明确接管时才提交。

## §8 Projection Styles / Preview Boundaries

| 入口 | 当前口径 |
|---|---|
| `BodyLocked` | 稳定默认投影风格：cursor look 不参与 hand anchor 计算，只通过当前真实相机影响最终投影 |
| `Look Responsive Projected` | 保留的视差投影风格：cursor look 先参与 hand anchor 计算，再通过当前真实相机投影；内部兼容枚举名仍为 `LegacyWorldProjected` |
| `LookInfluenceYaw / LookInfluencePitch` | 只服务 `Look Responsive Projected`，控制 cursor look 对 hand anchor 的影响比例 |
| Development preview layer | 只用于 PIE / 开发验证，不是 Battle / Run runtime source |
| Runtime card layer interaction | 当前 runtime source owner 使用 `SetFirstPersonCardLayerInteractionEnabled()` / `IsFirstPersonCardLayerInteractionEnabled()` 控制；`bEnableBattleHandInteraction` 和 BattleHand 命名 getter 仅保留为旧资产 / 旧 Blueprint 兼容别名 |

`LegacyWorldProjected` 的枚举名因蓝图 / 资产序列化兼容暂时保留，但制作语义不是待删除 fallback。后续如果要进一步整理，可以单独做一刀把编辑器文案、文档和资产制作指南继续朝 `LookResponsiveProjected` 命名靠拢；是否真正重命名 C++ 枚举值需要先评估资产迁移成本。

不要把 development preview layer 当作 Battle / Run runtime source，也不要恢复旧 `LegacyProjectedFan2D` 每卡 3D 槽位分别投影路径。当前两个投影风格都共享 `Authored2D` 手牌布局，区别只在整副手牌中心 anchor 是否吃 cursor look。

## §9 `WBP_FPCardView` 制作合同

推荐资产路径：`/Game/Wacom/UI/Card/WBP_FPCardView`

父类：`UWacomFirstPersonCardViewWidget`

使用入口：

- `BP_WacomPlayerCharacter -> FirstPersonCardAnchorComponent -> FirstPersonCardViewClass`
- 同一个入口服务 development preview 和 BattleHUD runtime battle hand。
- C++ 不硬编码该 WBP 路径；为空时使用原生 `UWacomFirstPersonCardViewWidget` 调试视图。

制作要求：

- 外层 WBP 负责 first-person 专属卡面包装；通用卡面仍放在名为 `CardView` 的 `UWacomCardView` 子控件中，不改 `UWacomCardView::SetCardViewData()` 语义。
- 必须提供 `CardView : UWacomCardView`，用于显示 `FWacomCardViewData` 和提供 `CardSizeBox` 主体命中几何。
- 建议提供 `FeedbackOverlay : UImage`，用于 playable hover / drag target / card target affordance full-card overlay；尺寸和层级由 WBP 控制。
- 建议提供 `InteractionFeedbackImage : UImage`，用于 pressed / confirm / commit / deny 第一人称源卡交互反馈层；尺寸、层级和默认材质由 WBP 控制，通常放在 `FeedbackOverlay` 上方。
- 建议提供 `CardShadowImage : UImage`，作为 `Fake3DSurfaceRetainer` 的前一个兄弟节点；C++ 独立写入 offset、opacity 和 scale，阴影不参与卡面透视采样。
- 建议提供 `Fake3DSurfaceRetainer : URetainerBox`，作为第一人称卡面的唯一 Retainer。`CardView / FeedbackOverlay / InteractionFeedbackImage` 必须位于该 Retainer 的同一个 surface overlay 内；不要在 `CardView` 内再嵌套 Retainer。
- 不再支持旧 `DenyFeedbackEdgeImage` fallback；需要源卡交互反馈时必须绑定 `InteractionFeedbackImage`。
- 外层可以是大于主体的透明 bleed 画布，例如 392 x 422 或 392 x 516，用于完整渲染主体外装饰。
- 内层 `CardView` 必须提供 `CardSizeBox` 主体 `SizeBox`，默认 296 x 420，并在 bleed 画布中尽量居中。
- 命中范围使用 `UWacomCardView.FixedCardBodyHitSize`，默认 296 x 420；不会因 bleed 画布、RetainerBox 或布局压缩而变小。
- RootOverlay 应保留约 `32-40 px` 透明 bleed，Clipping 使用 `Inherit`，避免透视角和独立阴影被裁切。主体命中仍固定使用 `CardSizeBox`，bleed 不扩大输入范围。
- `Fake3DSurfaceRetainer` Effect Material 使用 DreamShader 生成的 `/Game/DreamMaterials/Card/M_FirstPersonCard_Fake3D`；Domain 为 User Interface，采样参数名使用 Retainer 的 `Texture`，C++ 写入标量参数 `TiltX`、`TiltY`、`PerspectiveStrength`。没有 Effect Material 时卡面安全退化为平面，独立阴影仍可显示。
- `CardShadowImage` Brush 使用 DreamShader 生成的 `/Game/DreamMaterials/Card/M_FirstPersonCard_Shadow`；默认 `CardAspectRatio=0.7257` 对应当前 `ShadowHost 328 x 452`，修改该尺寸后需同步材质实例或 `.dsm` 默认值。
- 费用图标使用固定 `CostDigitImage : Image` 绑定；多位数、缺图标或未绑定时不会回退成文字费用。
- 材质流光和 disabled overlay 继续走内层 `UWacomCardView` 现有绑定；first-person 交互反馈走 wrapper 的 `FeedbackOverlay / InteractionFeedbackImage`。
- WBP 只负责卡面显示质量，不提交战斗命令，不读取 `UBattleSession`。

验收口径：

- 大角度扇形排布下没有明显锯齿、像素断裂或黑边。
- 卡面材质动画在 HUD first-person layer 中正常刷新。
- Hover、Drag、pending、disabled、pressed、confirm、deny、retained 由 slot motion、Card Depth 和 wrapper feedback 表现，WBP 只提供控件和材质承载，不重复实现状态机。
- 透明 bleed 区只用于渲染，不扩大 hover、press、drag 起手或 card target probe 范围。

## §10 测试入口

推荐自动化前缀：

- `Wacom.UI.FirstPersonCardLayer`
- `Wacom.UI.RunFirstPersonCardLayer`
- `Wacom.UI.Battle`

测试访问通过 production 非反射 automation test view 和 `WacomTests/Private/UI/FirstPersonCardLayerTestAccess.h` 读取 / 驱动。CardView / EffectBadge 计数通过 `FWacomCardViewTestAccess`。PlayerController Run / Battle target probe 通过对应 `WacomTests/Private` access wrapper，不直接依赖散落 probe forwarding 方法或测试字段。

Slot gesture 测试应通过 `InteractionIntent` 构造 entry / slot，避免再用 `TargetMode` 作为手势配置入口；只有验证 Battle adapter 投影时，才允许用 adapter 私有实现或测试内显式命名的 Battle target-mode projection helper。
