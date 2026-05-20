# 战斗 UI WBP 绑定清单

本文档用于在编辑器中制作战斗手牌相关 WBP。C++ 保留 fallback 布局；正式界面优先按下列接口绑定。

## WBP_CardWidget

父类：`UCardWidget`

推荐资产路径：`/Game/Wacom/UI/Battle/WBP_CardWidget`

`UHandPanel` 默认会尝试加载该路径；资产不存在时回退到 C++ 默认 `UCardWidget`。

推荐绑定：

| 控件名 | 推荐类型 | 用途 |
|---|---|---|
| `RootButton` | `Button` | 覆盖整张手牌，接收点击出牌/进入目标选择 |
| `FrameBorder` | `Border` | 显示可用状态与目标选择高亮 |
| `CardView` | `UWacomCardView` | 通用卡面显示，消费 `FWacomCardViewData` |
| `ZoneText` | `TextBlock` | 可选分区标签，显示左手/双手/右手区域 |

注意：
- `CardView` 只负责视觉，不处理点击、出牌、目标选择或战斗命令。
- `UCardWidget` 会用 `FHandCardSnapshot.RuntimeCost` 覆盖卡牌基础费用，保证战斗内费用变化显示正确。
- `bIsPlayable=false` 时，`UCardWidget` 会禁用 `RootButton`，并把 `FWacomCardViewData.bDisabled` 传给 `CardView`。
- `ZoneText` 不属于 `CardView`，它是战斗手牌外壳上的额外标签；绑定后不管 `CardView` 是否存在都会刷新。
- 战斗手牌 hover 反馈由 `UCardWidget` 外壳负责：默认使用 Render Transform 让卡牌上浮并轻微放大，不改变手牌布局占位。
- Hover 参数可在 `WBP_CardWidget` Details 中调整：`bEnableHoverFeedback` 控制开关，`HoverLift` 控制上浮距离，`HoverScale` 控制渲染缩放。
- `BP_OnHoverChanged` 可用于 WBP 中接音效、动画或额外材质反馈；不要把出牌或规则命令放进这个事件。
- 如果没有绑定 `RootButton`，该手牌不会崩溃，但无法点击。
- 如果没有绑定 `FrameBorder`，目标选择高亮颜色不会显示，但不会影响出牌流程。

## WBP_HandPanel

父类：`UHandPanel`

推荐资产路径：`/Game/Wacom/UI/Battle/WBP_HandPanel`

`BattleHUD` 默认会尝试加载该路径；资产不存在时回退到 C++ 默认 `UHandPanel`。

推荐绑定：

| 控件名 | 推荐类型 | 用途 |
|---|---|---|
| `UnifiedHandSlot` | `PanelWidget` | 默认统一水平手牌带，C++ 按 `FHandCardVisualEntry.VisualIndex` 填充所有手牌 |

推荐结构：

```text
WBP_HandPanel
└─ Root / Border / Overlay
   └─ HorizontalBox，命名为 UnifiedHandSlot
```

配置项：

| 属性 | 用途 |
|---|---|
| `CardWidgetClass` | 普通手牌使用的 `UCardWidget` 子类，例如 `WBP_CardWidget` |
| `AnchorCardWidgetClass` | 左右手锚点牌使用的 `UCardWidget` 子类；为空时使用 `CardWidgetClass` |
| `CardSpacing` | 卡牌之间的水平间距 |
| `HandContentPadding` | 整条手牌内容的首尾和上下边距 |
| `bCenterCardsWhenNotOverflow` | 未溢出时尝试让 `UnifiedHandSlot` 在父容器中居中 |
| `CardVerticalAlignment` | 卡牌在手牌带中的垂直对齐 |

注意：
- `UHandPanel` 只创建和摆放手牌，不直接执行出牌命令。
- 点击事件由 `UCardWidget` 转发到 `UHandPanel`，再交给 `BattleHUD`。
- `UnifiedHandSlot` 是当前默认视觉入口；如果没有 `WBP_HandPanel`，C++ fallback 会创建 Border + HorizontalBox。
- `UHandPanel` 只保留 `UnifiedHandSlot` 作为 WBP 绑定接口；左右手/双手区仍保留在 `FHandCardVisualEntry.LogicalZone` 中，后续扇形 renderer 应复用该 entry。
- `UnifiedHandSlot` 推荐使用 `HorizontalBox`；C++ 会设置卡牌间距、内容边距和未溢出居中。
- 如果使用 C++ fallback `BattleHUD`，手牌区外层大小由 `BattleHUD.HandPanelSize / HandPanelBottomOffset` 控制；如果使用完整 `WBP_BattleHUD`，则由 WBP 的 `HandPanel` slot 控制。
- 卡牌尺寸继续在 `WBP_CardWidget` 中控制；不要用 `WBP_HandPanel` 缩放卡牌，否则视觉尺寸和布局占位会不一致。
- 当前默认是统一水平手牌带；扇形手牌、拖拽出牌和 3D 部位目标选择是后续表现项。
