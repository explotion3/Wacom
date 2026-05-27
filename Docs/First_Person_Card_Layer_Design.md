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
- Cursor look 可以只部分影响卡牌，并带一点滞后或阻尼，让卡牌不像死死粘在镜头上。
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
- 只应用可配置比例的 cursor look offset。
- 提供跟随平滑、滞后等手感参数。
- 不持有卡牌 Widget。
- 不提交战斗或 Run 命令。

建议参数：

| 字段 | 含义 |
|---|---|
| `DistanceFromView` | 虚拟手牌平面距离锚点多远 |
| `VerticalOffset` | 手牌在视野里有多低 |
| `HorizontalOffset` | 可选的左右偏移 |
| `LookInfluenceYaw` | cursor yaw offset 对手牌的影响比例 |
| `LookInfluencePitch` | cursor pitch offset 对手牌的影响比例 |
| `FollowInterpSpeed` | 手牌跟随锚点变化的速度 |
| `ProjectionPadding` | 投影和视口 clamp 的安全边距 |

关键区别：

- Run Tunnel 和 Battle camera 可以把完整 cursor look 用到实际相机上。
- 卡牌手牌只应使用部分 cursor look，甚至延迟使用 cursor look，让它更像跟随身体 / 手，而不是完全跟随眼睛。

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

### V0-F 当前状态：Anchor + Static Card Layer + Battle Hand Interaction + Hover Detail Provider

当前已经建立 `UWacomFirstPersonCardAnchorComponent`、HUD debug 投影点、默认关闭的静态卡牌层、默认关闭的战斗手牌 adapter、默认关闭的 first-person battle hand hover/click 交互原型，以及 first-person hover detail provider。静态层使用 `UWacomCardView` 在 HUD / UMG 中渲染 3-5 张非交互卡牌，由 anchor 投影驱动屏幕位置、旋转和缩放；战斗 adapter 则把 `FBattleSnapshot.Hand` 转成带身份的 first-person card layer entry 后交给同一个 layer 显示。V0-E 增加 `UWacomFirstPersonCardLayerSlotWidget`，让卡牌 slot 可选地接收 hover 和左键点击，并把点击意图转发回 `BattleHUD->OnCardClickedByUser(CardInstanceId)`；V0-F 则把 hover 详情从旧 `UCardWidget` 几何中解耦，改为由 `BattleHUD` 根据最近一次 battle snapshot 和 first-person slot 的屏幕锚点显示同一个 `UWacomCardDetailPanel`。

- `AWacomPlayerCharacter` 持有 `FirstPersonCardAnchorComponent`。
- Anchor 优先使用 Battle camera base rotation，其次使用 Run Tunnel spline base transform，最后 fallback 到当前 camera transform。
- Shared cursor look 只按配置比例影响 card anchor，默认 yaw 25%、pitch 15%。
- `bDrawDebugProjection` 默认关闭；开启后在 HUD 上绘制 5 个非交互 debug 点，用于 PIE 验证未来手牌位置。
- `bDrawStaticCardLayer` 默认关闭；开启后创建 `UWacomFirstPersonCardLayerWidget`，显示配置的 `StaticPreviewCardDefinitions`，未配置时显示 placeholder 卡牌。
- `BattleHUD::bEnableFirstPersonBattleHandLayerPrototype` 默认关闭；开启后，战斗 HUD 在 snapshot 刷新和 UIState 变化时把真实手牌按顺序转成 `FWacomFirstPersonCardLayerEntry`，并覆盖同一个 first-person card layer 的静态预览。
- Runtime entry 保留 `CardInstanceId / Zone / bIsHandAnchor / bIsPlayable / bIsPendingTargeting`；`UWacomCardView` 仍只显示卡面，第一人称 layer 用 render transform、opacity 和 ZOrder 表现轻量状态。
- `bIsPendingTargeting` 的卡会轻微上移、放大并提高 ZOrder；不可用卡继续使用 disabled overlay，并叠加 layer 级透明度；手牌锚点卡只做轻微缩放区分。
- Runtime battle hand 是有效外部数据源；即使手牌为空，也显示为空，不回退 placeholder，避免战斗中出现假卡。
- 默认交互关闭时，first-person layer 和卡牌 slot 都保持 `HitTestInvisible`，行为等同 V0-D 的只读展示层。
- `BattleHUD::bEnableFirstPersonBattleHandInteractionPrototype` 默认关闭；只有它和 `bEnableFirstPersonBattleHandLayerPrototype` 同时开启时，first-person battle hand slot 才会变为可 hover / 可点击。
- 交互开启后，layer 根为 `SelfHitTestInvisible`，只让具体卡牌 slot 接收鼠标，不用全屏根控件抢输入。
- Hover 会影响 first-person layer 自身视觉：卡牌轻微上移、放大并提高 ZOrder；同时在 Idle 状态下通过 `BattleHUD` 显示现有 `UWacomCardDetailPanel`。详情数据来自 BattleHUD 最近一次 `FBattleSnapshot.Hand`，定位来自 first-person slot 投影后的屏幕锚点，不需要创建或伪装 `UCardWidget`。
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

5. V0-F：Hover Detail Provider
   - 已把 `CardDetailPanel` 从 `UCardWidget` 专属入口抽成 BattleHUD 内部通用详情 provider。
   - 旧 `UHandPanel` hover 和 first-person slot hover 共用同一个详情面板；first-person 详情只在 Idle 显示，进入 TargetSelect、提交命令、刷新到 BattleEnd 或 Session 切换时隐藏。
   - 详情 provider 只读 snapshot，不让 first-person layer 直接访问或修改 `UBattleSession`。

6. Play focus pose
   - 在命令 / 事件表现前增加短暂卡牌打出表现位。

7. Run card layer
   - 探索卡牌复用 anchor 和视觉状态机。
