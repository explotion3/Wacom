---
type: presentation-contract
scope: wacom-first-person-card-layer
status: active
updated: 2026-06-13
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
| `WacomFirstPersonCardLayerTypes.h` | 第一人称卡牌层公共 UI 协议：entry、slot view、drag / pointer view、transition hint、motion / visual / feedback config | 不包含 AnchorComponent 制作参数或运行时实现 |
| `FWacomFirstPersonCardAnchorRuntimeState` | Anchor 私有 runtime source 状态：entries、view data、transition hints、hovered card / card target handle | 不暴露 Blueprint API，不负责布局或 widget 生命周期 |
| `FWacomFirstPersonCardSlotLayoutBuilder` | Anchor 私有布局构建器：根据 resolved config、投影后 hand anchor 和 viewport size 生成基础 slot view / input hit 几何 | 不依赖 `UWacomFirstPersonCardAnchorComponent`，不处理 hover 视觉合成或命令 |
| `FWacomFirstPersonCardLayerOwner` | Anchor 私有 CardLayerWidget 生命周期 owner：创建 / 移除 widget、应用 layer config、推送 transition hints 和 slots | 不解析 anchor / viewport，不读取 runtime source，不转发 Battle / Run 命令 |
| `FWacomFirstPersonCardLayerDelegateRouter` | Anchor 私有 LayerWidget 事件 router：绑定 / 解绑 native delegates、同步 hovered runtime state、转发 Anchor 对外 delegates | 不创建 widget，不解析布局，不提交 Battle / Run 命令 |
| `UWacomFirstPersonCardLayerWidget` | 按 entries reconcile slot widget，维护 active / outgoing slot，绘制 drag arrow 和 layer-level feedback | 不读取 Battle / Run 规则状态 |
| `UWacomFirstPersonCardLayerSlotWidget` | 持有单卡 `UWacomFirstPersonCardViewWidget`，处理 hover / press / inspect / drag gesture 和 visual slot motion | 不直接调用 BattleSession 或 RunSession，不直接创建卡面反馈 Image / 材质控件 |
| `UWacomFirstPersonCardViewWidget` | 第一人称卡面 wrapper：组合通用 `UWacomCardView` 与 first-person 专属 `FeedbackOverlay`、`InteractionFeedbackImage` | 不处理 hover / drag 手势，不提交 Battle / Run 命令 |
| `UWacomRunFirstPersonCardSourceComponent` | 探索期把 Run BattleDeck / menu lease 写入 anchor runtime source | 不提交 Run 规则 |
| `FWacomBattleHUDFirstPersonHandBridge` | BattleHUD 内部同步 battle hand、transition hints、drag preview / release | 不暴露 Blueprint API |
| `FWacomBattleHUDCardDetailController` | first-person viewport 详情数据、motion、定位和 teardown | 不改变卡牌规则 |

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
| `06 Transition Motion` | Drawn / Gained / Played / Discarded 事件转场方向和来源 |
| `07 Hover` | hover lift / scale / ZOrder / hit hysteresis |
| `08 Targeting State` | pending targeting、target select deemphasis |
| `09 Gesture` | 按住读牌、拖出提交、快捷键拿起卡牌、inspect 姿态、aim arrow |
| `10 Interaction Feedback` | hover overlay、pressed、confirm、deny、commit feedback |
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
| Run default source | `UWacomRunFirstPersonCardSourceComponent` | 探索期显示 Run BattleDeck 物理卡和可选投影卡，只读展示 |
| Run menu lease | `UWacomMenuWidgetBase` / owning menu | GameMenu 内临时显示候选持有卡，可启用 hold / drag 到 menu Zone |
| Development preview | Anchor development preview | PIE / 开发验证，不是 Battle / Run runtime source |

进入战斗时，GameMode / PlayerController 会清理探索期 Run source 和 active menu lease。退出战斗回到 Exploration 后，PlayerController 重新激活 Run first-person source 并刷新当前 BattleDeck 展示。

BattleHUD 的 first-person hand bridge 只拥有 `BattleHand` runtime source。清理或 `NativeDestruct` 可能晚于 Run source 重新激活，因此 BattleHUD 解绑自身 delegate 时必须检查 Anchor 当前 `RuntimeCardLayerSourceId`：只有仍为 `BattleHand` 时才关闭 first-person card interaction、取消拖拽和清 runtime data；如果已经被 `RunFirstPersonBattleDeck` 或 menu lease 接管，只能解绑 BattleHUD delegate 和清战斗 world preview，不得改写 Run source 的交互状态。

Runtime source 只拥有卡牌 entries、transition hints、hovered card / card target handle、interaction ownership 和 source 生命周期；视觉调参来自 AnchorComponent Details。BattleHUD / Run source 不设置、不清理、也不持有 layout preset override。代码上这些运行时状态由 Anchor 私有 `FWacomFirstPersonCardAnchorRuntimeState` 保存，Anchor 的 public API 保持 source façade 语义。

BattleHUD runtime 战斗手牌不再有 legacy 2D hand 可见性恢复路径。进入 battle entry staging suppression 时，BattleHUD 会先清空当前 card layer visual slot，再写入 0 entries 的空 `BattleHand` runtime source；这既阻止 development preview fallback，也避免旧 slot 被 `SetCardSlots(0)` 当作 outgoing slot 播离场动画。退出战斗后的手牌恢复只依赖 Run source ownership 交接，不能通过旧 2D hand 兜底。

打开 Backpack / Pause / Shop / RunEvent 等 GameMenu 时，默认压制 Run default source，避免卡层遮挡菜单。菜单需要卡牌交互时，应显式申请 owned menu lease。

## §5 Slot Motion 与 Transition

Layer 使用稳定 motion key 复用 slot widget：

- Battle runtime hand 优先使用 `CardInstanceId`。
- Development preview 或 placeholder 使用 `StaticIndex:{Index}`。
- 同一 key 重新进入时复用 active 或回收 outgoing widget，避免幽灵 widget。

Anchor 每帧计算基础目标 slot，Layer 只在输入 slot、transition hint、配置或生命周期状态实际变化时完整 reconcile。SlotWidget 会先把基础 slot 与 hover / pending / drag card-target focus 等状态合成为最终 presentation，Slot motion 再独立 tick，把 visual position、angle、scale、opacity 和 ZOrder 追向这个最终 presentation。

Slot motion 现在按语义选择 motion intent / motion profile：`Layout` 负责普通手牌重排，`Hover` 负责悬浮 presentation，`Pending` 负责等待选目标源卡和非源卡弱化，`DragTargetFocus` 负责当前拖拽指针压中的唯一手牌目标，`Enter` / `Exit` 负责入场和离场。每个 `FWacomFirstPersonCardMotionProfile` 包含 `MotionSpeed / OpacitySpeed / EasePower`，Tick 时用当前 active profile 计算位置、角度、缩放和透明度追踪 alpha。

Anchor Details 的默认入口仍是 `CardSlotMotionSpeed / CardSlotOpacitySpeed / CardSlotMotionEasePower`，这三个值会映射到全部 profile；如果某个 profile 保持默认值，Normalize 时也会继承这三个旧参数。当前只开放少量高级覆盖项：`bOverrideHoverMotionProfile` 单独调悬浮，`bOverrideDragTargetFocusMotionProfile` 单独调拖拽手牌目标 focus，`bOverrideEnterExitMotionProfile` 单独调入场和离场。`Layout` 和 `Pending` 仍继承全局参数，避免 Details 面板过早变成全量 motion 表。`CardSlotMotionEasePower = 1` 保持线性旧手感；大于 `1` 时当前 profile 每帧起步更柔和，小于 `1` 时更快贴近目标。

Motion profile 只影响最终 visual slot 追踪，不改变 `InputHitCenter / InputHitScale / InputHitAngleDegrees` 等稳定命中几何，也不改变 hover / press / drag target resolver。候选目标 affordance 仍只控制 `FeedbackOverlay`，不会触发 `DragTargetFocus` 的 lift / scale / ZOrder；deny shake、confirm / commit pulse 和 `InteractionFeedbackImage` 也不接入 motion profile。

默认表现：

- 新卡按 enter offset 和 enter opacity 淡入。
- 移除或投影失败的可见卡进入 outgoing，短暂淡出 / 下滑后移除。
- 目标大跳变超过 reset distance 时直接贴合。
- Drawn / Gained / Played / Discarded transition hint 只改变表现来源或离场方向，不改变 snapshot 真相、命令路径或 slot key。

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

快捷键 `1~7` 进入拖拽时使用双位置初始化：`PressScreenPosition` 固定为被选中卡牌的基础手牌位置，`CurrentScreenPosition / PointerViewportPosition` 使用 PlayerController 读取到的当前鼠标 widget-space 坐标；没有鼠标坐标时才退回卡牌自身位置。`CurrentPointerView` 只表示普通 hover / pointer view，不作为快捷键拖拽启动坐标来源，避免鼠标悬浮在 B 卡时按 A 卡快捷键却串用 B 的旧 pointer。

快捷键拖拽启动后，`AWacomPlayerController` 每帧执行 active-drag pointer pump：如果当前 Anchor 的 first-person card layer 存在 active gesture，就优先通过 Slate viewport geometry 读取全局 cursor 在 viewport 内的 widget-space 坐标；如果 Slate viewport geometry 不可用，再退回 `GetMousePosition()` + `UWidgetLayoutLibrary::GetViewportScale()` 的 PlayerController 路径。随后调用 `UpdateFirstPersonCardDragPointer()` 喂给 Layer。Layer 继续复用 active gesture slot 的 `UpdateGesture()` 链路刷新 `DragView`、aim arrow、card target probe 和上层 camera look override。没有 active gesture 或读取不到鼠标位置时 pump 为 no-op，不会改变普通 hover / pointer view。左键 release 时，PlayerController 会先尝试释放 active first-person drag：能读取鼠标坐标时先 pump 到最新位置再 release，读取不到时使用 DragView 当前指针位置 release；只有没有 active drag 时，左键 release 才继续走 Battle scene click、Run tunnel branch 或 Run world interactable click 路由。

Battle 回合边界快捷键 `IA_Wait` / `IA_EndTurn` 在 PlayerController 入口先检查 first-person card layer active gesture。只要当前手势不是 `Idle` / `Cancelled`（包括 pressed、inspect、no-target drag、targeted aim、armed commit），本次快捷键会取消并消费该手势，不向 BattleHUD 提交等待或结束回合；下一次按键才按普通命令入口执行。取消后源卡保留当前 visual slot，并继续用 slot motion 返回手牌布局，不触发普通布局大跳变 reset。单纯 hover 不属于 active gesture，不会阻塞等待或结束回合。

需要敌方部位目标的卡 release 到合法 world enemy part 后，BattleHUD 调用现有 play-card world target 路径。需要手牌目标的卡 release 到合法 first-person card target 后，BattleHUD 提交 hand-card target。UI 手势层只提交 target identity，不判断加费、减费、弃置或消耗规则。

悬浮和拖拽期间，卡牌层都会记录 DPI-aware widget-space 指针和归一化视口坐标。普通 hover 的 mouse move 被 UMG 处理后，`UWacomFirstPersonCardLayerWidget` 会广播 `FWacomFirstPersonCardPointerView`，由 BattleHUD 写入 Battle camera look 临时 override，探索 / Run 则由 `AWacomPlayerController` 写入 Run Tunnel cursor look override。因此鼠标在悬浮放大的卡牌上移动时，镜头仍会跟随当前指针，不会等离开卡牌后突然跳到新位置。

拖拽过程中仍保留 UMG mouse capture，并继续通过 `FWacomFirstPersonCardDragView` 传递拖拽指针。SlotWidget mouse capture 负责鼠标按下、Pressed 阶段拖拽阈值判断、Inspect 阶段和 release/cancel 路由；一旦手势进入正式拖拽态（`DraggingNoTargetCard / ArmedForCommit / AimingTargetedCard`），鼠标拖拽和快捷键拖拽都会统一由 PlayerController active-drag pointer pump 写入 `CurrentScreenPosition / PointerViewportPosition`。因此鼠标离开 SlotWidget 后，拖拽箭头和镜头仍会持续跟随全局 cursor；进入或经过其他 SlotWidget 时，SlotWidget pointer enter / move 只消费事件并压制普通 hover，不再改写正式拖拽指针。拖拽镜头旧参数 `bAllowCameraLookDuringCardDrag`、`CardDragCameraLookScale`、`CardDragCameraLookInterpSpeedOverride` 继续只控制 drag override，保持既有资产兼容；hover / pointer 通用路径使用 `bAllowCameraLookDuringCardPointer`、`CardPointerCameraLookScale`、`CardPointerCameraLookInterpSpeedOverride`。这些 camera look 参数当前推荐在 AnchorComponent Details 的 `12 Camera Look While UI` 分类中调整。拖拽 active 时会清空并压制普通 pointer view，避免 hover override 与 drag override 抢同一个 camera look 状态。

Layer pointer arbitration 是 first-person hand 输入的正式入口。`UWacomFirstPersonCardLayerSlotWidget` 的 Slate mouse down / move / up 只把 pointer 事件转交给 `UWacomFirstPersonCardLayerWidget`，再按 Layer 返回的 route action 映射成 `Unhandled`、`Handled`、`CaptureMouse` 或 `ReleaseMouseCapture`；没有 Owner Layer 的 hand Slot 不再自管 press / drag / release 生命周期。`PressedSlotWidget` 只在没有 active gesture 且 press 成功开启新手势时写入。已有 active gesture 时，release 永远优先释放当前 gesture；mouse-origin drag 的 slot move 只在 `Pressed / Inspecting` 阶段写入 pointer，用于启动拖拽或读牌姿态；进入正式拖拽态后，slot move 与 external-origin drag 一样只消费并压制普通 hover，正式拖拽 pointer 只能由 PlayerController pump 写入。external drag 下点击目标手牌时，slot press 只更新当前源卡 drag pointer / card target，不抢占 pressed slot，也不重新开启目标卡手势。

鼠标拖拽和快捷键拖拽进入 active drag 时共用 SlotWidget 内部的 card-drag promotion 路径。拖拽内部同时记录两条互不替代的输入语义：`GestureSource` 表示手势起源和生命周期意图，目前为 `MousePress` 或 `KeyboardShortcut`；`GestureInputSource` 表示启动阶段来源，目前为 `MousePointer` 或 `ExternalPointer`。真实鼠标拖拽使用 `MousePress + MousePointer`，`1~7` 快捷键拖拽使用 `KeyboardShortcut + ExternalPointer`。`GestureSource` 不直接改变提交规则、目标合法性或视觉动画，只让后续输入生命周期可以按起源扩展；`GestureInputSource` 不再决定正式拖拽态的持续 pointer 写入来源，正式拖拽统一由 PlayerController pointer pump 更新。无目标卡的 `CurrentScreenPosition / PointerViewportPosition` 会立即更新，用于提交距离、DragView 和 release；源卡视觉位置不再直接 snap 到 pointer，而是以同一套 slot motion 追踪 drag override，并在拖拽 override 中把源卡角度归零，不继承手牌扇形角度。需要目标的卡牌仍保持 aim arrow 端点立即跟随真实 pointer。

快捷键启动的 external drag 没有真实鼠标按住状态，因此鼠标点击目标手牌时，目标 SlotWidget 的 mouse down 不能重新占有 `PressedSlotWidget` 或开始目标卡的新 press。Layer 在已有 active gesture 时会把这次 pointer press 作为当前 drag 的最终指针更新并消费事件，随后 mouse up 释放的仍是原来的源卡；这样 `HandCard` 目标卡牌可以通过“快捷键进入拖拽 -> 点击另一张手牌”完成 card-to-card release，和普通鼠标拖拽到目标卡后松开左键得到一致提交语义。

读牌姿态支持 inspect scrub。鼠标长按进入 `Inspecting` 后，Layer 会用所有 projected、非 exiting、有效、可交互手牌 card body 的稳定 input hit 几何计算整体 AABB，再加 `CardInspectScrubHandPaddingPixels`（默认 `32 x 48` UMG 布局像素）作为 scrub 区域。pointer 仍在该区域内时，Slot 继续更新 pointer / camera diagnostics，但禁止 `Inspecting -> DraggingNoTargetCard / AimingTargetedCard` 的距离升级；pointer 命中另一张可开始读牌/拖拽的 card body 时，Layer 会把 inspect source 静默从当前卡转移到目标卡，旧卡不广播 cancel，新卡广播新的 inspect started，用于详情面板切换。pointer 落在卡牌缝隙但仍在 scrub 区域内时保持上一张预览卡；如果已经从 A scrub 到 B，之后移出 scrub 区域并超过拖拽阈值，进入拖拽/瞄准的是当前预览源 B。

Hover 输入命中与最终视觉几何分离。Anchor 先投影并平滑整副手牌中心，随后把 resolved config、投影点、viewport size 和 runtime state 交给私有 `FWacomFirstPersonCardSlotLayoutBuilder`。Builder 只写入基础布局、基础 `RenderScale / RenderOpacity / RenderAngleDegrees / ZOrder`、稳定输入几何 `InputHitCenter / InputHitScale / InputHitAngleDegrees / InputHitOrder`，以及 `bIsHovered`、`bHasPendingTargetingCardInHand`、`Entry.bIsPendingTargeting` 等状态标记。Hover lift / scale / ZOrder、pending lift / scale / angle / ZOrder、target-select deemphasis 和 drag card-target focus 统一由 `UWacomFirstPersonCardLayerSlotWidget` 的 presentation resolver 合成。悬浮卡仍然可以放大、抬升并绘制在上层，但鼠标命中哪张牌由 `UWacomFirstPersonCardLayerWidget` 使用基础几何统一解析，不再由被 Slate 命中的单个 slot 自行决定。

`FWacomFirstPersonCardLayerSlotView` 本轮仍保留原字段名以降低调用方和测试改动量，但语义需要区分：`GetSlotView()` 更接近基础 slot + state flags，`GetVisualSlotView()` 是 SlotWidget 合成和 motion 后的最终视觉真相。详情面板、hover update、drag arrow 和 card-target feedback 应使用 final visual slot 语义；稳定 hover / press / drag 起手命中应继续使用基础 input hit 几何。

视觉状态优先级固定为：pending source 高于普通 hover；drag card-target focus 不触发普通 hover，只负责当前指针压中的唯一目标卡 lift / scale / ZOrder；target affordance 只控制 overlay，不参与 lift / scale / ZOrder；pressed、commit pulse 和 deny shake 保留为短时 micro feedback，其中 deny 只做 shake + 边缘 / 暗角反馈，不改变主 scale / ZOrder。

源卡短时交互反馈统一走 `InteractionFeedbackImage`，包括 `Pressed / Confirm / Commit / Deny`。SlotWidget 只计算状态、shake、scale 和 `FWacomFirstPersonCardInteractionFeedbackView`，然后交给 `UWacomFirstPersonCardViewWidget`；实际尺寸、层级和默认材质由 WBP 内的 `InteractionFeedbackImage` 控件负责。材质来源有两级：AnchorComponent Details 的 `InteractionFeedbackMaterial` 显式 override 优先；为空时 C++ 会复用 `InteractionFeedbackImage` 自身 brush 上的材质，作为 WBP 默认材质。若没有任何材质，pressed / confirm / commit 会退化为普通 tint，deny 仍保留 shake，且不会回退成整卡红色填充。`DenyFeedbackEdgeImage` 旧 fallback 已删除，新制作主线必须使用 `InteractionFeedbackImage`。拖拽 `InvalidCardTarget / ValidCardTarget`、card probe、commit ready 和 playable hover 等目标/候选提示继续使用 `FeedbackOverlay` full-card overlay 语义，但 overlay 控件本身也由 `WBP_FPCardView` 绑定和控制尺寸。

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

Drag-target focus 默认参数为 `DragCardTargetFocusLiftPixels = 18`、`DragCardTargetFocusScale = 1.045`、`DragCardTargetFocusZOrderBoost = 650`。推荐调参范围：lift `0-48` UMG 布局像素、scale `1.0-1.12`、ZOrder boost `0-1400`。这些都是表现层参数，不改变手牌顺序、牌堆、出牌结算或 HandCard target validation。

## §7 Target / Drop 边界

First-person drag target 来源：

| Target | 来源 | 提交方 |
|---|---|---|
| Battle world enemy part | PlayerController 按 drag widget position 做 world probe，命中当前 SceneEnemyHost registry 的 part handle | BattleHUD command flow |
| Battle hand card | First-person card layer 的 card target bridge | BattleHUD command flow |
| Run world object | PlayerController probe `Interaction.Target.Run.Object` | Run world card drop receiver + RunSession |
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
| Battle hand interaction | 使用 `bEnableBattleHandInteraction`、`SetBattleHandInteractionEnabled()`、`IsBattleHandInteractionEnabled()` |

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
- 不再支持旧 `DenyFeedbackEdgeImage` fallback；需要源卡交互反馈时必须绑定 `InteractionFeedbackImage`。
- 外层可以是大于主体的透明 bleed 画布，例如 392 x 422 或 392 x 516，用于完整渲染主体外装饰。
- 内层 `CardView` 必须提供 `CardSizeBox` 主体 `SizeBox`，默认 296 x 420，并在 bleed 画布中尽量居中。
- 命中范围使用 `UWacomCardView.FixedCardBodyHitSize`，默认 296 x 420；不会因 bleed 画布、RetainerBox 或布局压缩而变小。
- RetainerBox 内部可轻微缩放，给旋转采样留下透明边缘。
- 费用图标使用固定 `CostDigitImage : Image` 绑定；多位数、缺图标或未绑定时不会回退成文字费用。
- 材质流光和 disabled overlay 继续走内层 `UWacomCardView` 现有绑定；first-person 交互反馈走 wrapper 的 `FeedbackOverlay / InteractionFeedbackImage`。
- WBP 只负责卡面显示质量，不提交战斗命令，不读取 `UBattleSession`。

验收口径：

- 大角度扇形排布下没有明显锯齿、像素断裂或黑边。
- 卡面材质动画在 HUD first-person layer 中正常刷新。
- Hover、pending、disabled、pressed、confirm、deny 由 slot transform、opacity 和 wrapper feedback overlay 表现，WBP 只提供控件和材质承载，不重复实现状态机。
- 透明 bleed 区只用于渲染，不扩大 hover、press、drag 起手或 card target probe 范围。

## §10 测试入口

推荐自动化前缀：

- `Wacom.UI.FirstPersonCardLayer`
- `Wacom.UI.RunFirstPersonCardLayer`
- `Wacom.UI.Battle`

测试访问通过 production 非反射 automation test view 和 `WacomTests/Private/UI/FirstPersonCardLayerTestAccess.h` 读取 / 驱动。CardView / EffectBadge 计数通过 `FWacomCardViewTestAccess`。PlayerController Run / Battle target probe 通过对应 `WacomTests/Private` access wrapper，不直接依赖散落 probe forwarding 方法或测试字段。
