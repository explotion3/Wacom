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
| `HoverVisualRoot` | `Widget` / `Overlay` | 卡牌视觉根层，hover 上浮/缩放只移动这一层 |
| `FrameBorder` | `Border` | 显示可用状态与目标选择高亮 |
| `CardView` | `UWacomCardView` | 通用卡面显示，消费 `FWacomCardViewData` |
| `ZoneText` | `TextBlock` | 可选分区标签，显示左手/双手/右手区域 |

推荐结构：

```text
WBP_CardWidget
└─ Root / Overlay
   ├─ HoverVisualRoot
   │  └─ FrameBorder
   │     └─ CardView / ZoneText
   └─ RootButton
```

注意：
- `CardView` 只负责视觉，不处理点击、出牌、目标选择或战斗命令。
- `UCardWidget` 会用 `FHandCardSnapshot.RuntimeCost` 覆盖卡牌基础费用，保证战斗内费用变化显示正确。
- `bIsPlayable=false` 时，`UCardWidget` 会禁用 `RootButton`，并把 `FWacomCardViewData.bDisabled` 传给 `CardView`。
- `ZoneText` 不属于 `CardView`，它是战斗手牌外壳上的额外标签；绑定后不管 `CardView` 是否存在都会刷新。
- 战斗手牌 hover 反馈由 `UCardWidget` 外壳负责：默认使用 Render Transform 让 `HoverVisualRoot` 上浮并轻微放大，不改变手牌布局占位和鼠标命中区域。
- Hover 参数可在 `WBP_CardWidget` Details 中调整：`bEnableHoverFeedback` 控制开关，`HoverLift` 控制上浮距离，`HoverScale` 控制渲染缩放。
- 推荐把 `FrameBorder / CardView / ZoneText` 放进 `HoverVisualRoot`，把 `RootButton` 放在同级顶层并覆盖整张卡；不要对整个 `WBP_CardWidget` 做 hover 位移，否则命中区域会跟着移动，鼠标停在卡牌下沿时容易抖动。
- `BP_OnHoverChanged` 可用于 WBP 中接音效、动画或额外材质反馈；不要把出牌或规则命令放进这个事件。
- 目标选择状态由 `BattleHUD` 持有：点击 `SingleEnemyPart` 手牌进入目标选择，再次点击同一张手牌会取消选择。
- 选中态颜色/描边建议绑定 `FrameBorder` 或监听 `BP_OnTargetingHighlightChanged` 做 WBP 表现；不要在 WBP 里自行记录目标选择状态。
- `WBP_CardWidget` 不创建详情面板；hover 详情由 `UCardWidget` 上报、`UHandPanel` 转发、`BattleHUD` 统一显示。
- 如果没有绑定 `RootButton`，该手牌不会崩溃，但无法点击。
- 如果没有绑定 `FrameBorder`，目标选择高亮颜色不会显示，但不会影响出牌流程。

PIE 检查清单：
- `RootButton` 与 `HoverVisualRoot` 是同级，不在 `HoverVisualRoot` 内部。
- `RootButton` 覆盖整张卡牌的原始占位；hover 后鼠标停在卡牌下沿不应抖动。
- `FrameBorder / CardView / ZoneText` 在 `HoverVisualRoot` 内，hover 时整张卡面一起上浮。
- 鼠标移到卡牌上时会出现详情面板；进入目标选择后详情面板隐藏。
- 点击需要目标的卡牌后，当前卡仍能通过 `FrameBorder` 或 `BP_OnTargetingHighlightChanged` 显示选中态。

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

## WBP_BattleHUD

父类：`UBattleHUD`

可选绑定：

| 控件名 | 推荐类型 | 用途 |
|---|---|---|
| `CardDetailLayer` | `CanvasPanel` | 承接战斗手牌 hover 详情面板，建议覆盖整个 HUD 并位于手牌与敌方部位之上 |

注意：
- `CardDetailLayer` 未绑定时，如果 HUD 根控件是 `CanvasPanel`，C++ 会运行时创建 fallback layer。
- 详情面板使用 `/Game/Wacom/UI/Card/WBP_CardDetailPanel`，缺失时回退到 C++ `UWacomCardDetailPanel`。
- 详情面板默认显示在悬停卡牌左侧；左侧空间不足时显示在右侧，并会 clamp 到 `CardDetailLayer` 可见范围内。
- `BattleHUD` 会记录当前详情来源卡，快速切换 hover 卡牌时，旧卡的 unhover 不会关闭新卡详情。
- 详情面板为 `HitTestInvisible`，不抢点击；进入目标选择时会隐藏。
- `BattleHUD::BuildTargetSelectionView()` 是敌方目标选择表现桥。当前 2D `EnemyInfoBar / EnemyPartWidget` 使用它，后续 HD-2D/PaperZD 敌方部位 Actor 也应按 `PartInstanceId` 读取它来驱动高亮和可点击状态。

## EnemyPartWidget / EnemyInfoBar

当前 `EnemyInfoBar` 和 `EnemyPartWidget` 是战斗早期 2D fallback/debug 表现，不是最终 HD-2D 敌人实现。

注意：
- `EnemyInfoBar` 动态生成 `EnemyPartWidget`，并从 `BattleHUD::BuildTargetSelectionView()` 读取哪些部位可被选中。
- `EnemyPartWidget` 只负责显示 `FEnemyPartSnapshot`、接收 `SetTargetable(bool)` 和把点击委托回 `EnemyInfoBar`。
- 不建议在 `EnemyPartWidget` 上继续堆正式敌人动画、像素精灵或复杂命中逻辑；正式 HD-2D/PaperZD 敌人表现应消费同一份 `FBattleTargetSelectionView`，再把点击意图回传到 `BattleHUD->OnEnemyPartClickedByUser()`。

PIE 检查清单：
- `CardDetailLayer` 覆盖整个 HUD 可见区域，并位于手牌和敌方部位之上。
- 详情面板不会阻挡点击手牌、等待、结束回合或敌方部位。
- 从一张手牌快速滑到另一张手牌时，详情内容应切换到新卡，不应闪关。
- 最左侧手牌空间不足时，详情面板应显示在卡牌右侧。
