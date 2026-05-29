---
type: design-note
scope: wacom-ui
status: draft
updated: 2026-05-29
tags:
  - wacom/ui
  - wacom/cards
  - wacom/run
  - wacom/battle
  - wacom/first-person
---

# 第一人称卡牌层设计

> [!info] 本文职责
> 本文记录 Wacom 第一人称卡牌表现的长期方向。它还不是具体实现清单。目标是明确：当前 `WidgetComponent / RenderTarget` 的 3D 手牌原型不应继续作为正式卡牌系统扩展。

## 背景

Wacom 的 Run 体验正在靠近 Shroom and Gloom 一类第一人称卡牌探索游戏：玩家在第一人称纸片隧道里前进，鼠标保持可见，用于和场景、卡牌交互；卡牌应该像处在玩家第一人称空间里的物体，而不是贴死在屏幕上的 HUD 按钮。

当前 `AWacomBattle3DHandPresenter` 证明了“世界空间卡牌”是可行的，但它仍然只是过渡原型：

- 它通过 `UWidgetComponent` 渲染 `UCardWidget`。
- `UWidgetComponent` 先把 Widget 渲染到内部纹理，再把纹理显示在世界空间 quad 上。
- 卡牌点击和 hover 会回传给 `UBattleHUD`，这一点仍然是正确的：`UBattleHUD` 是战斗命令出口。

这条路线适合实验，但不适合成为主要卡牌表现路径。

## 为什么不继续扩展 WidgetComponent 主路线

`WidgetComponent` 适合场景面板、终端、VR 控件或临时原型，但不适合作为主要手牌系统。

已经观察到的问题：

- **渲染区域裁剪**：卡牌内容超出 Widget draw size 后会被 RenderTarget 裁掉。HUD Widget 可以更自然地扩展、溢出、动画和分层。
- **世界渲染副作用**：即便卡牌材质是无光照，世界空间 Widget 仍会经过世界渲染、透明排序、后处理、曝光、深度等路径。
- **UI 动态材质不稳定**：HUD 中正常动画的流光材质，在 WidgetComponent 中可能因为手动重绘或低频重绘而不动。强制频繁重绘又会带来清晰度和性能问题。
- **文字和详情可读性压力大**：卡牌需要稳定的文字、徽章、费用、长描述、hover 详情、拖拽态和未来 Run 交互。把这些都塞进世界空间 RenderTarget 会让正常 UI 迭代变难。
- **布局被 mesh / draw size 绑定**：视觉形状、命中范围、Widget draw size、世界缩放会相互耦合，卡牌大小和可读性调参都会变脆。

因此，`AWacomBattle3DHandPresenter` 应保留为 prototype / 对照工具，而不是战斗与 Run 卡牌的正式基础。

## 推荐方向：HUD 渲染，第一人称锚点布局

推荐路线是 first-person card layer：

```text
Battle / Run snapshot
  -> 卡牌表现数据
  -> HUD 层里的 UMG 卡牌 Widget
  -> first-person anchor 计算虚拟手牌平面
  -> 投影后的卡牌位置驱动 UMG render transform
```

卡牌仍然由 UMG 渲染，因此保留 HUD 级别的清晰度和 UI 材质动画能力。真正改变的是布局：它不再像固定在屏幕底部的普通 HUD 手牌条，而是由 first-person anchor 描述“玩家手牌应该处在相机 / 角色空间的哪里”，再由 UMG 层把这个虚拟手牌投影到屏幕。

这样能得到想要的手感：

- 卡牌跟随 PlayerCharacter / tunnel 推进，而不是独立悬在屏幕上。
- 卡牌仍然清晰、稳定、易读，因为它们还是 HUD Widget。
- 默认身体锁定投影下，cursor look 不再影响卡牌世界槽位和扇形 layout；鼠标移动镜头时屏幕投影仍会随当前相机产生空间变化。额外视差只作为 legacy / 后续实验选项。
- 同一套 card layer 后续可服务战斗手牌、Run 探索卡、背包 / 地图卡和打出表现位。

## 第一人称锚点模型

在 `AWacomPlayerCharacter` 上增加可复用的卡牌手牌锚点：

```text
UWacomFirstPersonCardAnchorComponent
```

职责：

- 为当前本地玩家输出虚拟手牌 transform 或虚拟手牌平面。
- 探索期使用 Run Tunnel base transform。
- 战斗期使用 Battle camera base rotation。
- 默认使用身体锁定投影，不把 cursor look offset 应用到卡牌槽位位置。
- 保留 legacy 世界投影模式，可按配置比例应用 cursor look offset，用于调试对照或后续视差实验。
- 提供跟随平滑、滞后等手感参数。
- 不持有卡牌 Widget。
- 不提交战斗或 Run 命令。

建议参数：

| 字段 | 含义 |
|---|---|
| `DistanceFromView` | 虚拟手牌平面距离锚点多远 |
| `VerticalOffset` | 手牌在视野里有多低 |
| `HorizontalOffset` | 可选的左右偏移 |
| `ProjectionMode` | 投影模式；默认 `BodyLocked`，锁定身体 / tunnel / battle 基准上的 3D layout，但仍用当前真实相机投影，保留第一人称空间感 |
| `LookInfluenceYaw` | legacy 世界投影下 cursor yaw offset 对手牌的影响比例；默认 `BodyLocked` 不使用它影响位置 |
| `LookInfluencePitch` | legacy 世界投影下 cursor pitch offset 对手牌的影响比例；默认 `BodyLocked` 不使用它影响位置 |
| `FollowInterpSpeed` | 手牌跟随锚点变化的速度 |
| `ProjectionPadding` | 投影和视口 clamp 的安全边距 |

关键区别：

- Run Tunnel 和 Battle camera 可以把完整 cursor look 用到实际相机上。
- 默认 `BodyLocked` 下，卡牌槽位的 3D layout 只使用 Battle base rotation / Run Tunnel spline base 这类稳定身体基准，不使用 cursor look 重新计算扇形；投影仍使用当前真实相机，所以鼠标镜头偏转时会有合理的第一人称透视变化。`LegacyWorldProjected` 保留“layout 吃 LookInfluence + 当前相机投影”的旧行为，用于对照扇形被拉扯的问题。

## Card Layer Widget

新增 HUD 层卡牌渲染器：

```text
UWacomFirstPersonCardLayerWidget
```

职责：

- 持有可见的 `UWacomCardView` / 后续交互外壳实例；只读展示阶段不使用 `UCardWidget`，避免提前引入战斗点击和 hover 语义。
- 用 `InstanceId` 维护卡牌视觉身份。
- 消费 `UWacomFirstPersonCardAnchorComponent` 的布局输入。
- 把虚拟第一人称卡牌位置转换成 UMG render transform。
- 支持 hover、选中、拖拽和 play pose 状态。
- 所有命令提交都留在被动卡牌视图之外。

该 Widget 不应该直接知道战斗或 Run 规则。它接收卡牌 entries，向外发出卡牌交互意图。

未来可能的数据来源：

| 来源 | 数据 | 命令出口 |
|---|---|---|
| 战斗手牌 | `FBattleSnapshot.Hand` / `FHandCardSnapshot` | `UBattleHUD` |
| Run 探索手牌 | Run card / tool view data | PlayerController / Run screen flow |
| 背包 / 地图卡 | Run snapshot / backpack view data | Backpack / Run command flow |

## 交互模型

第一版实现不应急着做拖拽，先把基础手感做对。

推荐阶段：

1. **Idle hand**：卡牌以第一人称锚点驱动的扇形 / 条形手牌出现。
2. **Hover**：卡牌在 HUD 空间里上浮、缩放或旋转，同时仍服从 anchor layout。
3. **Select**：卡牌进入稳定的选中姿态。需要目标的战斗卡让 BattleHUD 进入 `TargetSelect`。
4. **Play focus**：卡牌移动到短暂的中心 / 前置表现位，再提交命令或进入事件表现队列。
5. **Drag**：后续再让选中 / 拖拽卡牌朝场景对象或 Run Widget 移动。

Battle 和 Run 应尽量共享视觉状态机，但命令出口保持分离。

## 和真实 3D 卡牌的关系

推荐的 HUD-anchored layer 不排斥真实 3D card mesh。它只是限定真实 3D 卡牌更适合出现的位置。

适合真实 3D mesh 的场景：

- 卡牌提交后的打出 flourish。
- 奖励卡飞入手牌。
- 卡包 / 宝箱 / 商店揭示。
- 稀有卡流光、翻面等短时表现。
- 场景中代表卡牌的实体道具。

不适合作为真实 3D mesh 的场景：

- 常驻、需要清晰阅读的手牌。
- 长文本和详细规则显示。
- 大量手牌的响应式布局与 hover 详情。

简化成一句话：

```text
可读手牌 = HUD-rendered first-person card layer
短时表现 = 可选 3D card actors / meshes
```

## 从当前原型迁移

`AWacomBattle3DHandPresenter` 可以保留一些有价值的验证结论：

- 它证明了 `BattleHUD` 应继续作为命令出口。
- 它已经按 `InstanceId` 维护卡牌身份。
- 它有确定性的布局 helper。
- 它把 click / hover 意图转发出去，而不是直接调用 `UBattleSession`。

但以下部分不应照搬到正式路线：

- 用 `UWidgetComponent` 作为主要卡面渲染器。
- 用完全跟随 camera 的布局作为主要手牌锚点。
- 用 world-space widget hit testing 作为主要卡牌输入路径。
- 让 RenderTarget 尺寸成为卡牌布局权威。

first-person card layer 落地后，3D presenter 可以继续藏在 prototype / debug flag 后面，直到它不再有对照价值。

## 未决问题

- Battle 和 Run 使用同一个共享 card layer widget + 不同数据 adapter，还是各自 Widget 但共用 layout service？
- 手牌应受多少 cursor look 影响：0%、15%、30%，还是按鼠标离中心距离走曲线？
- 选中卡牌应该移动到纯 screen-space focus pose，还是投影后的虚拟 3D focus pose？
- 卡牌拖拽应该使用纯 HUD 鼠标位置、投影到第一人称平面的位置，还是混合方案？
- 大量手牌是否需要类似 Shroom and Gloom 的分页 / 压缩 / 导航方案？

## 建议实现顺序

### V0-AA 当前状态：Drag Camera Look Continuity

当前已经建立 `UWacomFirstPersonCardAnchorComponent`、HUD debug 投影点、默认关闭的静态卡牌层、正式的战斗手牌呈现模式、first-person battle hand hover/click、first-person hover detail provider、hover 详情跟随 / ZOrder 修正、first-person card render quality 基础、V0-J 的身体锁定锚点投影、V0-L 的美术可控 2D hand layout solver、V0-M 的 offscreen anchor soft clamp、V0-N 的 anchor motion stability、V0-O 的 card slot motion polish、V0-P 的 slot motion lifecycle diagnostics / self-repair、V0-Q 的 event-aware card transitions、V0-R 的 pending / TargetSelect focus polish、V0-S 的 playable / hover / press feedback polish、V0-T 的 hand layout preset / tuning profile、V0-U 的 transition origin / card movement readability、V0-V 的 hover detail / inspect readability polish、V0-W 的 play commit / target impact readability、V0-Z 的 hold inspect + card drag / aim commit，以及 V0-AA 的 drag camera look continuity。静态层使用 `UWacomCardView` 或其专用 WBP 子类在 HUD / UMG 中渲染 3-5 张非交互卡牌；战斗 adapter 则把 `FBattleSnapshot.Hand` 转成带身份的 first-person card layer entry 后交给同一个 layer 显示。V0-Z 后第一人称手牌的默认快速轻点仍保留：`bEnableClickToPlayCard=true` 时，按下后在 `CardInspectHoldDelaySeconds` 内释放继续走 `BattleHUD->OnCardClickedByUser(CardInstanceId)`。按住超过 delay 且没有超过拖拽阈值时，slot 进入 `Inspecting`，源卡通过 slot motion 移到视口读牌位置并放大，BattleHUD 使用同一套 first-person detail host 显示详情；松开只恢复，不提交。无目标卡（`None / Self / AllEnemyParts`）超过 `CardDragStartThresholdPixels` 后进入拖动，向上超过 `NoTargetCardDragOutCommitDistancePixels` 后进入 `ArmedForCommit`，释放才复用现有点击出牌路径提交。需要敌方部位目标的卡进入 `AimingTargetedCard`，源卡保持选中姿态，layer 用 C++ `NativePaint` 绘制从源卡 visual slot 到鼠标的箭头；BattleHUD 每次更新用 PlayerController 的只读 target probe 构建 `FWacomInteractionTargetHandle`，再用 `UBattleSession::CanTargetWithCard()` 判断是否合法。V0-AA 后拖拽仍保留 UMG mouse capture，避免拖到卡外丢 release；但 drag view 会记录 DPI-aware widget-space 指针和归一化视口坐标，BattleHUD 会把它作为临时 cursor-look override 传给 `UWacomBattleCameraLookComponent`，所以拖拽卡牌 / 箭头跟手的同时，战斗镜头仍可随拖拽方向轻微偏转。释放到合法 world enemy part 后调用现有 `SubmitPlayCard(CardId, TargetPartId)`，成功后继续触发 V0-W commit pulse、Played exit 和 `TargetConfirmed` cue。`HandCard` 本轮只进入 aim / probe，能识别 Card target，但释放不会提交规则，走 deny / cancel 表现。V0-Z / V0-AA 不做真实飞牌轨迹、拖拽 ghost WBP、Run 规则提交、准星瞄准模式或旧 `UCardWidget / UHandPanel` 拖拽兼容。

- `AWacomPlayerCharacter` 持有 `FirstPersonCardAnchorComponent`。
- Anchor 优先使用 Battle camera base rotation，其次使用 Run Tunnel spline base transform，最后 fallback 到当前 camera transform。
- `FirstPersonCardAnchorComponent.bUseFirstPersonCardLayoutPreset` 默认关闭。开启且 `FirstPersonCardLayoutPreset` 有效时，Anchor 会在运行时生成 resolved config 供布局、投影、motion 和反馈使用；关闭或 preset 为空时，继续使用组件实例上的参数。Preset 不覆盖 `FirstPersonCardViewClass`、静态预览卡牌、debug 开关、viewport ZOrder 或 `BattleHUD::BattleHandPresentationMode`。
- `ProjectionMode` 默认 `BodyLocked`：卡牌槽位的 3D layout 使用 Battle base rotation / Run Tunnel spline base 作为稳定身体基准，不让 shared cursor look 参与扇形位置和世界槽位计算；随后仍通过当前玩家相机投影到 widget-space，避免手牌 HUD 化。
- `LegacyWorldProjected` 保留旧路径：shared cursor look 会按 `LookInfluenceYaw/Pitch` 影响 card anchor / layout，然后再通过当前玩家相机投影；该模式只用于调试对照旧的漂移 / 扇形破坏问题，或后续视差实验。
- `CardLayoutMode` 默认 `Authored2D`：只投影当前 hand anchor 中心点，每张卡牌的最终位置由 `AuthoredCardSpacingPixels / AuthoredMaxHandWidthPixels / AuthoredHandScreenOffset / StaticCardEdgeDropPixels / AuthoredCenterLiftPixels` 等 2D 参数计算。
- `LegacyProjectedFan2D` 保留 V0-L 之前的对照行为：每张卡牌继续通过 `ComputeCardTransform()` 生成 3D 槽位并分别投影，便于 PIE 对比旧的空间投影手感和排布拉扯问题。
- `ViewportClampMode` 默认 `SoftClampToViewport`：投影成功后先保留 `UnclampedWidgetPosition`，再按视口限制模式生成最终布局坐标。`HardClampToViewport` 会按 `ProjectionPadding` 强制留在屏幕内，复现旧行为；`SoftClampToViewport` 会把 safe rect 向外扩 `SoftClampOffscreenAllowancePixels`，点在 soft rect 内不拉回，超出后按 `SoftClampBlendRangePixels` 平滑停到扩展边界；`AllowOffscreen` 完全不限制坐标，适合对照最接近 3D 空间物体的表现。
- V0-M 不做自动旧手牌兜底：在 `FirstPersonHandOnly` 下，如果玩家把手牌锚点看出屏幕，旧 `UHandPanel` 不会自动恢复；玩家需要把视角转回，或把 `BattleHandPresentationMode` 切回 fallback / legacy 模式。
- `bEnableAnchorScreenSmoothing` 默认开启，只作用于 `Authored2D` 的整副手牌中心：hand anchor center 投影和 viewport clamp 完成后，先平滑中心点，再叠加 authored 2D offsets、edge drop、hover lift、pending lift 和最终 pixel snap。它不逐张平滑卡牌，因此不会改变卡牌间距、下坠、扇形角度、层级或左右顺序。
- `AnchorScreenSmoothingSpeed` 控制中心点追随速度，数值越低越稳但越滞后；`AnchorScreenSmoothingResetDistancePixels` 用于传送、切 segment、投影恢复或大幅跳变时重置 smoothing，避免手牌慢慢飘向新位置。
- Anchor tick 在 BeginPlay 后添加 `UWacomRunTunnelMovementComponent` 和 `UWacomBattleCameraLookComponent` 作为 prerequisite，让手牌每帧读取已经更新后的 RunTunnel distance、角色 / 相机 transform 和战斗 camera base 状态。
- `bEnableCardSlotMotion` 默认开启，只作用于 `UWacomFirstPersonCardLayerSlotWidget` 的视觉状态缓存：slot target 仍由 anchor layout 一次性算出，slot widget 只把 `VisualSlotView` 追向 `TargetSlotView`。`CardSlotMotionSpeed` 控制位置 / 角度 / 缩放，`CardSlotOpacitySpeed` 控制透明度，`CardSlotMotionResetDistancePixels` 用于传送、切 segment 或窗口变化时直接贴合。
- 新 runtime battle hand 优先使用 `CardInstanceId` 作为 slot motion key；静态预览或 placeholder 没有有效 id 时使用 `StaticIndex:{Index}`。因此 snapshot 重排时同一张卡复用同一个 slot widget，新卡淡入，消失的卡进入 outgoing 列表淡出 / 下滑，到期后移除。
- `bEnableEventAwareCardTransitions` 默认开启，只改变 first-person layer 的入场 / 离场表现，不改变 `UHandPanel`、`BattleSession` 或命令路径。V0-U 后 `bEnableReadableTransitionOrigins` 默认开启：每种 transition 都可配置 origin mode、viewport anchor、scale multiplier 和 angle offset；关闭后完全回到 V0-Q 的 offset-only 行为。`SlotOffset` 基于目标 slot 或当前 visual slot 加偏移；`HandAnchorOffset` 基于 `SlotView.AnchorWidgetPosition` 加偏移；`ViewportAnchor` 基于 DPI-aware widget-space viewport anchor 加偏移，取不到 viewport 时 fallback 到 `SlotOffset`。
- 默认 transition profile：Drawn 使用 `HandAnchorOffset + DrawnCardEnterOffsetPixels`，从手牌中心下方进入；Gained 使用 `HandAnchorOffset + GainedCardEnterOffsetPixels`，从手牌中心上方 / 战斗空间方向进入；Played 使用 `SlotOffset + PlayedCardExitOffsetPixels`，从当前卡向上离开；Discarded 使用 `SlotOffset + DiscardedCardExitOffsetPixels`，从当前卡向下离开。scale / angle accent 只影响 `VisualSlotView` 的入场起点或离场终点，不修改目标 slot、slot key、ZOrder 或输入数据。
- `BattleHUD` 会在消费 `FBattleEvent` 后暂存一批 first-person transition events，并在下一次 `NativeRefreshFromSnapshot()` 覆盖 transition previous snapshot 前生成 one-shot hints；`NativeOnUIStateChanged()` 可以刷新 pending / hover 视觉，但不会污染下一次 snapshot diff 的 previous hand。
- Hover、pending、disabled 和 hand anchor 的目标 transform 仍由 anchor 计算；slot motion 只负责过渡，不改变点击合法性。ZOrder 立即跟随 target，避免 hover / pending 卡被过渡中的其他卡遮挡。Hover 详情收到的是动画中的 visual slot 位置，因此会跟随移动中的卡牌，而不是跳到最终目标点。
- V0-P 后 `UWacomFirstPersonCardLayerWidget` 会记录 `FWacomFirstPersonCardLayerMotionDebugView`：输入 slot 数、active slot 数、outgoing slot 数、RootCanvas 中 slot child 数、正在 tick 的 motion slot 数、重复 key 数，以及本次刷新创建 / 复用 / 移除 / outgoing 开始结束 / 清理未追踪 child 的数量。
- 每次 `SetCardSlots()` 后都会校验并修复 slot 生命周期不变量：active 不能复用同一个 widget，outgoing 不能包含 active widget 或 active key，RootCanvas 不能残留未追踪 slot child；重复 incoming key 会用 `#SlotIndex:{Index}` 消歧并记录，不刷战斗规则 warning。同一 key 如果在 outgoing 淡出期间重新进入手牌，会优先回收 outgoing widget 作为 active，避免同一张卡同时存在 active 和幽灵 outgoing。
- outgoing slot 使用内部安全上限 `Max(LastSlots.Num() * 2, 16)`，超过时清理最旧 outgoing 并记录 invariant violation。`bLogCardLayerMotionDiagnostics` 默认关闭；开启后仅在检测到不变量修复或异常清理时输出一条简短 summary，用于 PIE 排查幽灵 widget、outgoing 泄漏和 FPS 阶梯式下降。
- `bDrawDebugProjection` 默认关闭；开启后在 HUD 上绘制 5 个非交互 debug 点，用于 PIE 验证未来手牌位置。
- `bDrawStaticCardLayer` 默认关闭；开启后创建 `UWacomFirstPersonCardLayerWidget`，显示配置的 `StaticPreviewCardDefinitions`，未配置时显示 placeholder 卡牌。
- `FirstPersonCardViewClass` 用于指定第一人称卡牌层的卡面 Widget；正式验证建议设置为 `/Game/Wacom/UI/Card/WBP_FirstPersonCardView`。该 WBP 可以在 `WBP_CardView` 基础上加入 RetainerBox、透明边缘留白和轻微内部缩放，以降低整卡旋转采样带来的边缘锯齿。为空时只作为测试兜底回退到 `UWacomCardView`，不作为正式第一人称主手牌卡面。
- first-person layer 的 `CanvasSlot` 使用 widget-space 布局位置，不再直接使用 raw screen pixel；debug view 同时记录 raw screen position、widget position、snapped widget position、viewport scale、layout mode、anchor widget position、authored layout offset 和 normalized hand offset。
- V0-N 后 debug view / slot view / projected point 还会记录 `ViewportClampMode`、`UnclampedWidgetPosition`、`bOutsideViewport`、`OffscreenDistancePixels`、`UnsmoothedAnchorWidgetPosition`、`SmoothedAnchorWidgetPosition`、`AnchorScreenSmoothingDistancePixels` 和 `bAnchorScreenSmoothed`，用于判断“投影成功但已经离屏”“真正投影失败”和“当前中心点是否经过平滑”。
- `bEnableCardLayerPixelSnapping` 默认开启，最终位置会在 edge drop、pending lift、hover lift 后 snap 到 `CardLayerPixelSnapGrid`，默认 1.0 UMG layout unit。
- `bClampCardLayerRenderAngle` 默认开启，slot render angle 被限制在 `MaxCardLayerRenderAngleDegrees` 内，默认 4 度；后续扇形表现优先通过位置下坠和 hover/pending 归正继续优化，而不是继续增加旋转角。
- `BattleHUD::BattleHandPresentationMode` 控制战斗手牌入口：`LegacyHandPanel` 只使用旧 `UHandPanel`；`FirstPersonHandWithLegacyFallback` 默认写入 first-person runtime hand、启用 hover/click/detail，并保留旧手牌可见；`FirstPersonHandOnly` 在 first-person runtime hand 有效时隐藏旧手牌。
- Runtime entry 保留 `CardInstanceId / Zone / bIsHandAnchor / bIsPlayable / bIsPendingTargeting / TargetMode`；`TargetMode` 只用于 first-person slot gesture 分类，不在 layer 内判断规则合法性。`UWacomCardView` 仍只显示卡面，第一人称 layer 用 render transform、opacity 和 ZOrder 表现轻量状态。
- `bIsPendingTargeting` 的卡会作为 TargetSelect 焦点卡上移、放大、提高 ZOrder，并可按 `PendingTargetingAngleBlend` 向 0 度轻微归正；同一副手牌存在 pending 卡时，非 pending 卡只按透明度倍率轻微弱化，不改变布局或输入。Pending 卡 hover 仍会标记 hovered slot 和允许点击取消，但不会额外叠加 hover lift / scale / ZOrder。
- `bEnableCardInteractionFeedback` 默认开启，只在 `UWacomFirstPersonCardLayerSlotWidget` 内用 C++ overlay 和 render transform 做轻量反馈。可打卡 hover 叠加 `PlayableHoverFeedbackColor / Opacity`；按下可交互卡乘 `PressedFeedbackScale` 并叠加 pressed tint；有效释放播放短 confirm pulse；不可打卡释放播放 deny tint 和横向 shake，但不广播 click intent。
- `bEnablePlayCommitFeedback` 默认开启，只在成功提交 `PlayCard` 后由 one-shot transition hint 触发。无目标卡成功提交后，移除的卡牌 outgoing slot 会播放 commit pulse 并按 Played profile 离开；目标卡成功提交后，pending 卡同样播放 commit pulse / Played exit，并向目标部位发送 `TargetConfirmed` cue。若能取得目标部位 widget 的 DPI-aware viewport 中心，Played exit 会轻微朝目标方向偏移；取不到时回到默认 Played exit。命令失败、无 Session、表现队列 busy、取消 TargetSelect 或非目标选择点击都不会播放 commit / target confirm。
- `TargetConfirmed` cue 与 BattleEvent cue 分开：`FWacomBattlePresentationTargetCue::CueKind=TargetConfirmed` 只播放轻量确认高亮 / scale pulse；后续真实伤害、破坏仍由表现队列按 `DamageDealt / EnemyPartHpEmptied` 发送 `BattleEvent` cue。它不是伤害提示，也不伪装成伤害。
- Runtime battle hand 是有效外部数据源；即使手牌为空，也显示为空，不回退 placeholder，避免战斗中出现假卡。
- `LegacyHandPanel` 模式会清理 first-person runtime source、禁用交互、解绑 delegates 并恢复旧手牌 visibility。
- Anchor 缺失、战斗结束、Session 切换、HUD destruct 或清理 runtime hand 时，旧 `UHandPanel` 会恢复原始 visibility，避免战斗不可操作。
- 交互开启后，layer 根为 `SelfHitTestInvisible`，只让具体卡牌 slot 接收鼠标，不用全屏根控件抢输入。
- Hover 会影响 first-person layer 自身视觉：卡牌轻微上移、放大并提高 ZOrder；同时在 Idle 状态下通过 `BattleHUD` 显示现有 `UWacomCardDetailPanel`。旧 `UHandPanel` hover 详情继续使用 BattleHUD 内部 `CardDetailLayer`；first-person hover 详情使用独立 viewport popup panel，默认 `FirstPersonCardDetailViewportZOrder=9999`，高于 first-person card layer。详情数据来自 BattleHUD 最近一次 `FBattleSnapshot.Hand`，定位来自 first-person slot 投影后的屏幕锚点，不需要创建或伪装 `UCardWidget`。
- V0-V 后旧 `UHandPanel` 详情和 first-person viewport 详情共用 BattleHUD 的读牌 motion state。`bEnableCardDetailReadabilityPolish` 默认开启：初次 hover 会等待 `CardDetailHoverDelaySeconds` 后淡入，离开普通来源时淡出；已显示详情时滑到另一张卡会立即换数据和目标锚点，不重新完整延迟。详情位置按 `CardDetailFollowSpeed` 追向目标，超过 `CardDetailPositionResetDistancePixels` 会直接贴合，`CardDetailSideSwitchHysteresisPixels` 用于减少贴边时左右反复跳。关闭该开关会恢复旧的立即显示 / 隐藏 / 定位行为。
- 可打、非 pending 卡才会获得 hover lift / scale / ZOrder；不可打卡仍可 hover 以查看 Idle 详情，但不会表现成可打姿态。Pending 卡保持 V0-R 焦点状态，hover 不额外叠加 lift / scale / ZOrder，但仍可按下并点击取消 TargetSelect。
- first-person hover 详情会订阅 hovered slot layout update：当 hover lift、pending 状态、hand layout、Run Tunnel 推进，或当前真实相机投影让 hovered 卡牌屏幕锚点变化时，详情面板只重算位置，不重建数据。默认 `BodyLocked` 下，鼠标镜头偏移不会重算扇形 layout，但会通过当前相机投影产生合理的详情跟随。
- 左键点击有效、已投影、可用的 slot 时，只广播 `CardInstanceId` 并由 `BattleHUD` 进入现有 `OnCardClickedByUser()` flow；不可用卡允许 hover，但点击 no-op。
- V0-Z 后 slot 增加 `Idle / Pressed / Inspecting / DraggingNoTargetCard / AimingTargetedCard / ArmedForCommit / Cancelled` 手势状态。Mouse leave 不会取消 pressed / drag；只有 mouse up、右键 / ESC 后续入口、interaction disabled、slot exit、projection failed、BattleEnd、Session 切换或 runtime source clear 会清理手势。进入 drag / aim 后会 suppress 原 click mouse-up，避免同一手势双提交。
- Runtime source 清理、战斗结束、Session 切换、HUD destruct 或关闭开关时，会禁用交互、解绑 delegates、清理 hover 状态并移除 stale runtime hand。

1. `UWacomFirstPersonCardAnchorComponent`
   - 暴露 debug projected points 和 static card slot views。
   - 验证 Run Tunnel 与 Battle camera 的 base transform，以及 UMG 卡面投影位置。

2. `UWacomFirstPersonCardLayerWidget`
   - 用静态 view data 或配置的卡牌定义渲染少量测试卡牌。
   - 由 anchor 驱动屏幕位置。

3. V0-C / V0-D：Battle hand adapter + identity / visual states
   - 已把 `FBattleSnapshot.Hand` 接入 card layer，并保留卡牌实例身份、左右手区域、手牌锚点、可用性和等待选目标状态。
   - 旧 `UHandPanel` 继续负责正式战斗手牌交互，first-person layer 先做并行对照。

4. V0-E：Hover + Click Intent Bridge
   - 已增加可选 hover 和 click intent bridge。
   - click 只转发给 `UBattleHUD`，仍不让卡牌层直接提交 `UBattleSession` 命令。
   - 不做拖拽、右键取消、长按或 Run 卡牌交互。

5. V0-F / V0-H：Hover Detail Provider + Follow / ZOrder
   - 已把 `CardDetailPanel` 从 `UCardWidget` 专属入口抽成 BattleHUD 内部通用详情 provider。
   - 旧 `UHandPanel` hover 使用 legacy `CardDetailLayer` 内的详情面板；first-person slot hover 使用独立 viewport popup 详情面板，层级高于 first-person card layer。
   - first-person 详情只在 Idle 显示，进入 TargetSelect、提交命令、刷新到 BattleEnd 或 Session 切换时隐藏。
   - hovered first-person slot 每次重排后会广播最新 `FWacomFirstPersonCardLayerSlotView`，BattleHUD 只在来源 `CardInstanceId` 匹配时更新详情位置。
   - 详情 provider 只读 snapshot，不让 first-person layer 直接访问或修改 `UBattleSession`。

6. V0-G / V0-I：Legacy HandPanel Visibility Toggle + Battle Hand Presentation Mode
   - 已把旧手牌隐藏和 first-person hand 开关收口为 `BattleHandPresentationMode`。
   - 旧 `UHandPanel` 不删除、不解绑、不停止刷新；`FirstPersonHandOnly` 只在 first-person runtime hand 有效时改变 visibility，作为安全 fallback 保留。

7. Render Quality V0-A
   - 已把卡牌投影从 raw screen pixel 改到 DPI-aware widget-space。
   - 已加入像素对齐、最大 render angle clamp 和 projection quality debug 字段。
   - 不改 `WBP_CardView` 结构，不引入 RetainerBox / RenderTarget / Slate 自绘。

8. V0-J：Body-Anchored Current-Camera Projection
   - 已把 first-person card layer 默认投影模式切为 `BodyLocked`。
   - 战斗中用 `UWacomBattleCameraLookComponent::GetBaseBattleRotation()` 与当前相机位置生成稳定的身体锚点；Run Tunnel 中使用 active segment spline transform 和当前 distance；fallback 仍使用当前 camera transform 并记录原因。
   - `LookInfluenceYaw/Pitch` 暂只服务 `LegacyWorldProjected`，默认身体锁定模式不让它影响卡牌世界槽位和扇形 layout。
   - Debug summary / slot view / projected point 会记录 projection mode、是否 body locked layout、是否 current camera projection、look offset 是否参与布局，以及 raw/widget/snapped position。

9. V0-K：Projected Card Basis 实验已回退
   - 该实验尝试用虚拟 3D 卡牌平面投影推导 UMG 角度和缩放。
   - PIE 观察到动态缩放会带来新的像素拉伸和卡面质量问题，因此不作为当前主线保留。
   - 后续卡牌表现优先走专用 `WBP_FirstPersonCardView` 和美术可控 2D layout solver。

10. V0-L：Authored 2D Hand Layout Solver
   - 已把默认 `CardLayoutMode` 切到 `Authored2D`。
   - `Authored2D` 只投影整副手牌中心点，再按 2D 参数排布每张卡牌：水平间距、最大宽度、整体屏幕偏移、中心上抬、边缘下坠曲线、扇形旋转曲线和中心卡层级。
   - 卡面 scale 保持稳定，只来自 `StaticCardRenderScale` 和 hover / pending / hand anchor 状态倍率，不再从投影距离或卡牌平面 basis 动态推导。
   - `LegacyProjectedFan2D` 保留为 PIE 对照和安全 fallback，不作为默认美术调参路径。

11. V0-M：Offscreen Anchor Projection / Soft Clamp
   - 已新增 `ViewportClampMode`：`HardClampToViewport / SoftClampToViewport / AllowOffscreen`。
   - 默认 `SoftClampToViewport`，允许手牌中心离开屏幕安全区一段距离，再按 smoothstep 柔性拉回到扩展边界。
   - `ProjectionPadding` 继续作为 HardClamp 的安全边距，也作为 SoftClamp 的 safe rect 基础；`SoftClampOffscreenAllowancePixels` 控制允许离屏距离，`SoftClampBlendRangePixels` 控制越界后的拉回过渡。
   - `bProjected=true` 只表示世界点投影成功；坐标是否在屏幕外由 `bOutsideViewport / OffscreenDistancePixels` 诊断。真正投影失败或点在相机后方时 slot 仍隐藏。
   - `FirstPersonHandOnly` 下手牌离屏时不会自动恢复旧 `UHandPanel`，这是为了验证空间手牌表现的明确选择。

12. V0-N：Anchor Motion Stability
   - 已为 `UWacomFirstPersonCardAnchorComponent` 增加 RunTunnel movement 和 Battle camera look tick prerequisite，减少读取上一帧移动 / 相机状态造成的错位。
   - 已在 `Authored2D` 下对投影和 viewport clamp 后的整副手牌中心做 screen smoothing；每张卡的 authored offset、下坠、fan angle、ZOrder、hover / pending lift 仍按原规则计算。
   - 投影失败、layout / projection / clamp / anchor mode 变化，以及超过 `AnchorScreenSmoothingResetDistancePixels` 的目标跳变都会重置 smoothing，避免传送或切 segment 后慢漂。
   - `LegacyProjectedFan2D` 不套用该 smoothing，用于对照旧的每卡投影表现。

13. V0-O：Card Slot Motion / Transition Polish
   - 已为 first-person card slot 增加默认开启的轻量 UMG 过渡。位置、角度、缩放和透明度由 `UWacomFirstPersonCardLayerSlotWidget::NativeTick()` 插值到目标布局，不引入 UMG Animation 资产。
   - `UWacomFirstPersonCardLayerWidget` 按 motion key 复用 slot widget：战斗 runtime hand 使用 `CardInstanceId`，静态预览使用 `StaticIndex:{Index}`。这让 snapshot 重排、抽牌、出牌、Wait / EndTurn 刷新时已有卡按身份滑到新位置。
   - 新 slot 使用 `CardSlotEnterOffsetPixels / CardSlotEnterOpacity` 进入；移除或投影失败的可见 slot 会短暂保留 outgoing 视觉，按 `CardSlotExitOffsetPixels / CardSlotExitDuration` 淡出并移除。
   - 大跳变超过 `CardSlotMotionResetDistancePixels` 时直接贴合目标，避免传送、切 segment、窗口变化后卡牌慢慢飘过去。
   - Hover / pending / disabled / hand anchor 的目标状态仍由 anchor 计算；slot motion 不改变输入路径，click 仍只广播 `CardInstanceId`。详情跟随使用 visual slot 位置，所以会跟随动画中的卡牌。

14. V0-P：Slot Motion Robustness + Perf Diagnostics
   - 已增加 `FWacomFirstPersonCardLayerMotionDebugView` 和 summary 输出，统计 input / active / outgoing / RootCanvas child / ticking slot，以及本次刷新创建、复用、移除和异常修复数量。
   - `UWacomFirstPersonCardLayerWidget` 在每次刷新后修复 active 重复 widget、outgoing 与 active 重叠、RootCanvas 未追踪 child，以及 outgoing 超限，防止幽灵 widget 和 FPS 阶梯式下降。
   - 重复 incoming key 会被消歧并统计；同一 key 的 outgoing slot 重新进入手牌时会被回收复用，不会额外创建同身份 active widget。
   - `bLogCardLayerMotionDiagnostics` 默认关闭，只在手动排查时开启，且只在检测到 invariant violation / abnormal repair 时输出简短 warning。

15. V0-Q：Event-Aware Card Transitions
   - 已增加 `EWacomFirstPersonCardSlotTransitionKind` 和 `FWacomFirstPersonCardLayerTransitionHint`。Hint 是 one-shot 表现数据，只在下一次 `SetCardSlots()` 消费，不作为手牌真相。
   - `BattleHUD` 用上一份 transition snapshot 与新 snapshot 做 hand diff；BattleEvent 只提供语义 hint。`CardGained` 优先精确匹配新增卡，剩余新增卡按 `CardsDrawn.Count` 顺序分配 Drawn，移除卡按 `CardPlayed` / `HandLimitDiscarded` 分配 Played / Discarded。
   - `UWacomFirstPersonCardLayerWidget` 仍按 motion key 复用 active/outgoing widget；hint 会解析成 transition motion profile，覆盖新 active slot 的视觉入场来源或 outgoing slot 的视觉离场终点。重排但仍存在的卡不触发 enter / exit，只走平滑重排。

16. V0-U：Transition Origin / Card Movement Readability
   - 已增加 `EWacomFirstPersonCardTransitionOriginMode` 和 readable transition origin profile。`SlotOffset` 保留 V0-Q 行为；`HandAnchorOffset` 用手牌中心锚点作为入场 / 离场基准；`ViewportAnchor` 用 widget-space viewport anchor 模拟屏幕边缘、牌堆或战斗空间方向，缺少 viewport 时回退 SlotOffset。
   - `bEnableReadableTransitionOrigins` 默认开启；Drawn / Gained 默认从 hand anchor 加偏移进入，Played / Discarded 默认从当前 visual slot 加偏移离开。关闭该开关会回到 V0-Q 的 offset-only 行为。
   - Scale multiplier 和 angle offset 只写入 `VisualSlotView` 起点或 outgoing target，不改变 `TargetSlotView`、战斗命令、hover/click、detail follow 或 slot lifecycle diagnostics。

17. V0-R：Pending / TargetSelect Focus Polish
   - 已为 pending 卡增加配置化 `PendingTargetingZOrderBoost`、`bPendingTargetingStraightenAngle`、`PendingTargetingAngleBlend`。
   - TargetSelect 中存在 pending 卡时，非 pending 卡按 `TargetSelectNonPendingOpacityMultiplier` 轻微降透明；不可用卡透明度会与该倍率组合。
   - Pending 卡 hover 不再叠加额外 lift / scale / ZOrder，避免目标选择时 hover 抖动；详情仍只在 Idle 显示。

18. V0-S：Playable / Hover / Press Feedback Polish
   - 已增加 `FWacomFirstPersonCardSlotFeedbackConfig`，由 anchor 传给 layer，再传给 slot widget。
   - Slot widget 内部创建 `HitTestInvisible` 的 C++ feedback overlay，承载可打 hover、pressed、confirm 和 deny 反馈，不要求修改 `WBP_FirstPersonCardView`。
   - 不可打卡 hover 不再获得可打卡上浮 / 放大 / 提层；不可打卡点击只播放 deny 反馈，不广播 click intent。

19. V0-V：Hover Detail / Inspect Readability Polish
   - 已完成：旧 `UHandPanel` 详情和 first-person viewport 详情共用短 hover delay、淡入淡出、轻量 scale、位置跟随平滑和 side hysteresis。
   - first-person 详情仍使用独立 viewport popup host，高于 first-person card layer；旧手牌详情仍留在 BattleHUD 内部 `CardDetailLayer`。
   - 本轮不加入 inspect lock，不新增输入语义，不改变 BattleHUD 命令路径。

20. V0-Z：Hold Inspect + Card Drag/Aim Commit
   - 已增加 first-person slot 手势状态。轻点仍保持快速点击出牌；hold delay 后进入读牌姿态，卡牌居中放大并显示 first-person detail，松开不误提交。
   - 无目标卡拖出手牌上方阈值后 armed，释放才提交；有目标卡拖动后用 C++ aim arrow 连接源卡和鼠标，释放到合法 world enemy part 后复用现有 `SubmitPlayCard`。
   - PlayerController 提供只读 world target probe，拖拽过程中不会调用 click router 直接提交。Card target 使用 V0-Y 的 first-person hovered card target bridge，本轮只诊断 / deny，不扩规则。
   - 手势坐标统一使用 DPI-aware widget-space viewport position，不使用移动中的 slot local geometry。按下时会冻结一份 visual slot 作为 inspect / drag / aim 的起始基准，后续 anchor 刷新、slot motion 或相机移动不会把源卡和 aim arrow 从鼠标位置拉偏。
   - EnemyInfoBar 不是 drag / aim 的 UI target。拖拽目标只来自 first-person Card target bridge 或 PlayerController 对世界对象的 `IWacomInteractionTargetProvider` probe；EnemyInfoBar 仍可用于 legacy UI 点击、详情显示和表现 cue 注册。
   - 旧 `UCardWidget / UHandPanel` 不接入拖拽，仍只保留 legacy click / hover 路径。

21. V0-AA：Drag Camera Look Continuity
   - 已为 first-person drag view 增加拖拽指针 viewport position / normalized viewport position。Slot 继续保留 UMG mouse capture，release 稳定性不变。
   - `UWacomBattleCameraLookComponent` 支持临时 cursor-look override。BattleHUD 在 first-person card drag / inspect update 时把拖拽指针传给 BattleCameraLook；release、cancel、runtime hand clear 或 BattleEnd 时清理 override。
   - 拖拽 target probe 优先使用拖拽指针位置做 world trace，避免 UI capture 下继续读旧 OS cursor。点击路由仍保留原来的 under-cursor 路径。
   - 源卡仍使用按下时冻结的 visual slot 作为 inspect / drag / aim 基准，镜头运动不会把源卡或 aim arrow 从拖拽指针拉偏。EnemyInfoBar 仍不是 UI drop target。

22. Render Quality V0-B
   - 当前不把“降低旋转角”作为主线目标；`WBP_FirstPersonCardView` 已能承接较大角度旋转的抗锯齿需求，排布表现优先。
   - 后续只在美术反馈需要时微调扇形参数：下坠、层级、hover / pending 姿态和可选角度 clamp。

23. First-person card view polish
   - 已确认第一人称层应使用专用 `WBP_FirstPersonCardView`，不要继续把通用 `WBP_CardView` 直接作为长期主手牌卡面。
   - 后续 polish 重点是沉淀该 WBP 的制作规范：RetainerBox 使用边界、贴图透明留白、内部缩放、安全边框、材质动画刷新频率，以及不同 DPI / 视口尺寸下的旋转采样表现。

## `WBP_FirstPersonCardView` 制作合同

推荐资产路径：`/Game/Wacom/UI/Card/WBP_FirstPersonCardView`

父类：`UWacomCardView`

使用入口：

- 在 `BP_WacomPlayerCharacter -> FirstPersonCardAnchorComponent -> FirstPersonCardViewClass` 中设置。
- 同一个入口同时服务静态预览层和 BattleHUD 写入的 runtime battle hand。
- C++ 不硬编码该 WBP 路径；为空时只使用 `UWacomCardView` 作为测试 fallback，避免没有资产时崩溃。

制作要求：

- 以 `WBP_CardView` 为基础复制，不要改 `UWacomCardView` 的数据入口和 `SetCardViewData()` 语义。
- 根部建议使用 `RetainerBox` 缓存卡面，再在 RetainerBox 内部把实际卡面内容轻微缩放到约 `0.99`，给旋转采样留下透明边缘。
- 卡面基础尺寸保持 296 x 420，first-person layer 的 `StaticCardRenderScale=1.0` 时应接近美术设计尺寸。
- 高对比边框、描边、稀疏细线和贴图边缘需要预留透明 Alpha 留白；不要让有效像素紧贴贴图边界。
- 材质动画、流光和 disabled overlay 仍应通过 `UWacomCardView` 的现有绑定与 `SurfaceFoilOverlay` 路径工作，不在 first-person slot widget 内另开一套数据绑定。
- Widget 本体保持只读卡面：不创建 `UCardWidget`，不提交战斗命令，不在 WBP 图里读取 `UBattleSession`。

验收口径：

- first-person layer 大角度扇形排布下，边缘不应出现明显锯齿、像素断裂或黑边。
- 卡面材质动画在 HUD first-person layer 中正常刷新。
- Hover / pending / disabled 状态由 first-person layer 的 slot transform、opacity 和 `FWacomCardViewData::bDisabled` 叠加表现，WBP 不重复实现同一套状态机。
- 如果未来普通 HUD、背包或详情需要不同尺寸或结构，优先调整各自 WBP，不把第一人称 RetainerBox 规范反向套回通用 `WBP_CardView`。

14. Play focus pose
   - 在命令 / 事件表现前增加短暂卡牌打出表现位。

15. Run card layer
   - 探索卡牌复用 anchor 和视觉状态机。
