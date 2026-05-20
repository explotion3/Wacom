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

兼容绑定：

| 控件名 | 推荐类型 | 用途 |
|---|---|---|
| `NameText` | `TextBlock` | 未绑定 `CardView` 时显示卡名 |
| `CostText` | `TextBlock` | 未绑定 `CardView` 时显示战斗内运行时费用 |
| `ZoneText` | `TextBlock` | 未绑定 `CardView` 时显示左右手区域 |

注意：
- `CardView` 只负责视觉，不处理点击、出牌、目标选择或战斗命令。
- `UCardWidget` 会用 `FHandCardSnapshot.RuntimeCost` 覆盖卡牌基础费用，保证战斗内费用变化显示正确。
- `bIsPlayable=false` 时，`UCardWidget` 会禁用 `RootButton`，并把 `FWacomCardViewData.bDisabled` 传给 `CardView`。
- 如果没有绑定 `RootButton`，该手牌不会崩溃，但无法点击。
- 如果没有绑定 `FrameBorder`，目标选择高亮颜色不会显示，但不会影响出牌流程。

## WBP_HandPanel

父类：`UHandPanel`

推荐绑定：

| 控件名 | 推荐类型 | 用途 |
|---|---|---|
| `LeftZoneSlot` | `PanelWidget` | 左手区普通卡 |
| `LeftAnchorSlot` | `PanelWidget` | 左手锚点牌 |
| `BothZoneSlot` | `PanelWidget` | 双手区普通卡 |
| `RightAnchorSlot` | `PanelWidget` | 右手锚点牌 |
| `RightZoneSlot` | `PanelWidget` | 右手区普通卡 |

配置项：

| 属性 | 用途 |
|---|---|
| `CardWidgetClass` | 普通手牌使用的 `UCardWidget` 子类，例如 `WBP_CardWidget` |
| `AnchorCardWidgetClass` | 左右手锚点牌使用的 `UCardWidget` 子类；为空时使用 `CardWidgetClass` |

注意：
- `UHandPanel` 只创建和摆放手牌，不直接执行出牌命令。
- 点击事件由 `UCardWidget` 转发到 `UHandPanel`，再交给 `BattleHUD`。
- 当前仍是线性手牌布局；扇形手牌、拖拽出牌和 3D 部位目标选择是后续表现项。
