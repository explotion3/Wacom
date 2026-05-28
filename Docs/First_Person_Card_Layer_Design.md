---
type: design-note
scope: wacom-ui
status: draft
updated: 2026-05-28
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

### V0-N 当前状态：Anchor Motion Stability

当前已经建立 `UWacomFirstPersonCardAnchorComponent`、HUD debug 投影点、默认关闭的静态卡牌层、正式的战斗手牌呈现模式、first-person battle hand hover/click、first-person hover detail provider、hover 详情跟随 / ZOrder 修正、first-person card render quality 基础、V0-J 的身体锁定锚点投影、V0-L 的美术可控 2D hand layout solver、V0-M 的 offscreen anchor soft clamp，以及 V0-N 的 anchor motion stability。静态层使用 `UWacomCardView` 或其专用 WBP 子类在 HUD / UMG 中渲染 3-5 张非交互卡牌；战斗 adapter 则把 `FBattleSnapshot.Hand` 转成带身份的 first-person card layer entry 后交给同一个 layer 显示。V0-L 后默认不再把每张卡牌都生成 3D 槽位并分别投影，而是只投影整副手牌中心点，再用稳定的 2D 参数计算每张卡的屏幕偏移、下坠、旋转和层级，避免 projected basis 动态缩放带来的拉伸和采样不稳定。V0-M 继续沿用这个方向，但不再把手牌中心强制限制在屏幕安全区域内；默认 `SoftClampToViewport` 允许锚点离开视口一段距离，超过软范围后再柔性拉回。V0-N 不改变 Run Tunnel 的 spline / distance 表现，也不锁屏；它只让 anchor tick 在 RunTunnel movement 和 Battle camera look 之后执行，并在 `Authored2D` 下对投影后的整副手牌中心做轻量 screen smoothing，保留前进下降、后退上升的空间趋势，同时压掉移动时叠加在中心点上的高频抖动。V0-E 增加 `UWacomFirstPersonCardLayerSlotWidget`，让卡牌 slot 可接收 hover 和左键点击，并把点击意图转发回 `BattleHUD->OnCardClickedByUser(CardInstanceId)`；V0-F 把 hover 详情从旧 `UCardWidget` 几何中解耦，改为由 `BattleHUD` 根据最近一次 battle snapshot 和 first-person slot 的屏幕锚点显示详情；V0-H 进一步把 first-person hover 详情拆到独立 viewport popup host，并跟随 hovered slot 的实时布局更新，避免被 first-person card layer 遮挡。V0-I 用 `BattleHUD::BattleHandPresentationMode` 取代三个 prototype bool，默认 `FirstPersonHandWithLegacyFallback`，表示战斗默认启用第一人称手牌并保留旧 `UHandPanel` 兜底；`FirstPersonHandOnly` 只在 first-person runtime hand 有效时折叠旧手牌，异常时自动恢复。Render Quality V0-A 把投影坐标改为 DPI-aware widget-space，并默认启用像素对齐和 render angle clamp，作为基础诊断与兜底。

- `AWacomPlayerCharacter` 持有 `FirstPersonCardAnchorComponent`。
- Anchor 优先使用 Battle camera base rotation，其次使用 Run Tunnel spline base transform，最后 fallback 到当前 camera transform。
- `ProjectionMode` 默认 `BodyLocked`：卡牌槽位的 3D layout 使用 Battle base rotation / Run Tunnel spline base 作为稳定身体基准，不让 shared cursor look 参与扇形位置和世界槽位计算；随后仍通过当前玩家相机投影到 widget-space，避免手牌 HUD 化。
- `LegacyWorldProjected` 保留旧路径：shared cursor look 会按 `LookInfluenceYaw/Pitch` 影响 card anchor / layout，然后再通过当前玩家相机投影；该模式只用于调试对照旧的漂移 / 扇形破坏问题，或后续视差实验。
- `CardLayoutMode` 默认 `Authored2D`：只投影当前 hand anchor 中心点，每张卡牌的最终位置由 `AuthoredCardSpacingPixels / AuthoredMaxHandWidthPixels / AuthoredHandScreenOffset / StaticCardEdgeDropPixels / AuthoredCenterLiftPixels` 等 2D 参数计算。
- `LegacyProjectedFan2D` 保留 V0-L 之前的对照行为：每张卡牌继续通过 `ComputeCardTransform()` 生成 3D 槽位并分别投影，便于 PIE 对比旧的空间投影手感和排布拉扯问题。
- `ViewportClampMode` 默认 `SoftClampToViewport`：投影成功后先保留 `UnclampedWidgetPosition`，再按视口限制模式生成最终布局坐标。`HardClampToViewport` 会按 `ProjectionPadding` 强制留在屏幕内，复现旧行为；`SoftClampToViewport` 会把 safe rect 向外扩 `SoftClampOffscreenAllowancePixels`，点在 soft rect 内不拉回，超出后按 `SoftClampBlendRangePixels` 平滑停到扩展边界；`AllowOffscreen` 完全不限制坐标，适合对照最接近 3D 空间物体的表现。
- V0-M 不做自动旧手牌兜底：在 `FirstPersonHandOnly` 下，如果玩家把手牌锚点看出屏幕，旧 `UHandPanel` 不会自动恢复；玩家需要把视角转回，或把 `BattleHandPresentationMode` 切回 fallback / legacy 模式。
- `bEnableAnchorScreenSmoothing` 默认开启，只作用于 `Authored2D` 的整副手牌中心：hand anchor center 投影和 viewport clamp 完成后，先平滑中心点，再叠加 authored 2D offsets、edge drop、hover lift、pending lift 和最终 pixel snap。它不逐张平滑卡牌，因此不会改变卡牌间距、下坠、扇形角度、层级或左右顺序。
- `AnchorScreenSmoothingSpeed` 控制中心点追随速度，数值越低越稳但越滞后；`AnchorScreenSmoothingResetDistancePixels` 用于传送、切 segment、投影恢复或大幅跳变时重置 smoothing，避免手牌慢慢飘向新位置。
- Anchor tick 在 BeginPlay 后添加 `UWacomRunTunnelMovementComponent` 和 `UWacomBattleCameraLookComponent` 作为 prerequisite，让手牌每帧读取已经更新后的 RunTunnel distance、角色 / 相机 transform 和战斗 camera base 状态。
- `bDrawDebugProjection` 默认关闭；开启后在 HUD 上绘制 5 个非交互 debug 点，用于 PIE 验证未来手牌位置。
- `bDrawStaticCardLayer` 默认关闭；开启后创建 `UWacomFirstPersonCardLayerWidget`，显示配置的 `StaticPreviewCardDefinitions`，未配置时显示 placeholder 卡牌。
- `FirstPersonCardViewClass` 用于指定第一人称卡牌层的卡面 Widget；正式验证建议设置为 `/Game/Wacom/UI/Card/WBP_FirstPersonCardView`。该 WBP 可以在 `WBP_CardView` 基础上加入 RetainerBox、透明边缘留白和轻微内部缩放，以降低整卡旋转采样带来的边缘锯齿。为空时只作为测试兜底回退到 `UWacomCardView`，不作为正式第一人称主手牌卡面。
- first-person layer 的 `CanvasSlot` 使用 widget-space 布局位置，不再直接使用 raw screen pixel；debug view 同时记录 raw screen position、widget position、snapped widget position、viewport scale、layout mode、anchor widget position、authored layout offset 和 normalized hand offset。
- V0-N 后 debug view / slot view / projected point 还会记录 `ViewportClampMode`、`UnclampedWidgetPosition`、`bOutsideViewport`、`OffscreenDistancePixels`、`UnsmoothedAnchorWidgetPosition`、`SmoothedAnchorWidgetPosition`、`AnchorScreenSmoothingDistancePixels` 和 `bAnchorScreenSmoothed`，用于判断“投影成功但已经离屏”“真正投影失败”和“当前中心点是否经过平滑”。
- `bEnableCardLayerPixelSnapping` 默认开启，最终位置会在 edge drop、pending lift、hover lift 后 snap 到 `CardLayerPixelSnapGrid`，默认 1.0 UMG layout unit。
- `bClampCardLayerRenderAngle` 默认开启，slot render angle 被限制在 `MaxCardLayerRenderAngleDegrees` 内，默认 4 度；后续扇形表现优先通过位置下坠和 hover/pending 归正继续优化，而不是继续增加旋转角。
- `BattleHUD::BattleHandPresentationMode` 控制战斗手牌入口：`LegacyHandPanel` 只使用旧 `UHandPanel`；`FirstPersonHandWithLegacyFallback` 默认写入 first-person runtime hand、启用 hover/click/detail，并保留旧手牌可见；`FirstPersonHandOnly` 在 first-person runtime hand 有效时隐藏旧手牌。
- Runtime entry 保留 `CardInstanceId / Zone / bIsHandAnchor / bIsPlayable / bIsPendingTargeting`；`UWacomCardView` 仍只显示卡面，第一人称 layer 用 render transform、opacity 和 ZOrder 表现轻量状态。
- `bIsPendingTargeting` 的卡会轻微上移、放大并提高 ZOrder；不可用卡继续使用 disabled overlay，并叠加 layer 级透明度；手牌锚点卡只做轻微缩放区分。
- Runtime battle hand 是有效外部数据源；即使手牌为空，也显示为空，不回退 placeholder，避免战斗中出现假卡。
- `LegacyHandPanel` 模式会清理 first-person runtime source、禁用交互、解绑 delegates 并恢复旧手牌 visibility。
- Anchor 缺失、战斗结束、Session 切换、HUD destruct 或清理 runtime hand 时，旧 `UHandPanel` 会恢复原始 visibility，避免战斗不可操作。
- 交互开启后，layer 根为 `SelfHitTestInvisible`，只让具体卡牌 slot 接收鼠标，不用全屏根控件抢输入。
- Hover 会影响 first-person layer 自身视觉：卡牌轻微上移、放大并提高 ZOrder；同时在 Idle 状态下通过 `BattleHUD` 显示现有 `UWacomCardDetailPanel`。旧 `UHandPanel` hover 详情继续使用 BattleHUD 内部 `CardDetailLayer`；first-person hover 详情使用独立 viewport popup panel，默认 `FirstPersonCardDetailViewportZOrder=9999`，高于 first-person card layer。详情数据来自 BattleHUD 最近一次 `FBattleSnapshot.Hand`，定位来自 first-person slot 投影后的屏幕锚点，不需要创建或伪装 `UCardWidget`。
- first-person hover 详情会订阅 hovered slot layout update：当 hover lift、pending 状态、hand layout、Run Tunnel 推进，或当前真实相机投影让 hovered 卡牌屏幕锚点变化时，详情面板只重算位置，不重建数据。默认 `BodyLocked` 下，鼠标镜头偏移不会重算扇形 layout，但会通过当前相机投影产生合理的详情跟随。
- 左键点击有效、已投影、可用的 slot 时，只广播 `CardInstanceId` 并由 `BattleHUD` 进入现有 `OnCardClickedByUser()` flow；不可用卡允许 hover，但点击 no-op。
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

13. V0-H：Hover Detail Follow + ZOrder
   - 已完成：first-person hover 详情跟随 hovered card 实时投影位置，并显示在 first-person card layer 之上。
   - 旧 `UHandPanel` hover 详情路径不变，仍作为 fallback 和对照。
   - 后续只保留细节微调：详情面板偏移、动画、避免贴边时跳动。

14. Render Quality V0-B
   - 当前不把“降低旋转角”作为主线目标；`WBP_FirstPersonCardView` 已能承接较大角度旋转的抗锯齿需求，排布表现优先。
   - 后续只在美术反馈需要时微调扇形参数：下坠、层级、hover / pending 姿态和可选角度 clamp。

15. First-person card view polish
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
