# 背包 UI WBP 绑定清单

本文档用于在编辑器中制作背包相关 WBP。C++ 仍保留 fallback 布局；正式界面应优先按下列接口绑定。

## WBP_BackpackScreen

父类：`UWacomBackpackScreen`

推荐绑定：

| 控件名 | 推荐类型 | 用途 |
|---|---|---|
| `DeleteZoneHost` | `PanelWidget` | C++ 填充删牌 DropTarget |
| `BattleDeckZoneHost` | `PanelWidget` | C++ 填充备战区 DropTarget 和卡牌列表 |
| `FluxMainCardsHost` | `PanelWidget` | C++ 填充通量主卡区 |
| `FluxContentDropTargetHost` | `PanelWidget` | C++ 填充通量内容 DropTarget 和内容卡列表 |
| `SpecialZonesHost` | `PanelWidget` | C++ 动态填充 `UWacomSpecialZoneWidget` |
| `BurdenZoneHost` | `PanelWidget` | C++ 填充负重区标题、DropTarget 和卡牌列表 |
| `CardDetailLayer` | `CanvasPanel` | C++ 填充悬浮卡牌详情面板 |

可选绑定：

| 控件名 | 推荐类型 | 用途 |
|---|---|---|
| `TitleText` | `TextBlock` | 背包标题 |
| `GoldText` | `TextBlock` | 金币显示 |
| `DeleteZoneTitleText` | `TextBlock` | 删牌区提示，可不绑，由 C++ 创建 |
| `BattleDeckTitleText` | `TextBlock` | 备战区数量/容量标题 |
| `BackpackTitleText` | `TextBlock` | 背包区数量/通量容量标题 |
| `CloseButton` | `Button` | 关闭背包 |
| `BattleDeckCardsBox` | `WrapBox` | 直接承接备战卡列表；不绑则 C++ 在 Host 中创建 |
| `FluxMainCardsBox` | `WrapBox` | 直接承接通量主卡；不绑则 C++ 在 Host 中创建 |
| `FluxContentCardsBox` | `WrapBox` | 直接承接通量内容卡；不绑则 C++ 在 Host 中创建 |

注意：
- 旧 `FluxZoneHost / BackpackCardsBox` 混合布局已删除，不要再绑定。
- 如果推荐 Host 没绑定，C++ 会输出 warning，该区域不会显示运行时内容。
- `CardDetailLayer` 应覆盖背包界面可见区域，并放在卡牌区域之上；详情面板为 `HitTestInvisible`，不会抢拖拽或右键输入。
- WBP 只负责布局和样式，不直接调用 `RunSession`。

## WBP_WacomSpecialZoneWidget

父类：`UWacomSpecialZoneWidget`

推荐绑定：

| 控件名 | 推荐类型 | 用途 |
|---|---|---|
| `TitleText` | `TextBlock` | 特殊区标题 |
| `BattleReadyBadge` | `TextBlock` | B 主卡已入战标记 |
| `OwnerCardHost` | `PanelWidget` | C++ 填充 B 主卡 |
| `ContentDropTargetHost` | `PanelWidget` | C++ 填充 SpecialZone DropTarget |

可选绑定：

| 控件名 | 推荐类型 | 用途 |
|---|---|---|
| `ContentCardsBox` | `WrapBox` | 直接承接内容卡列表；不绑则 C++ 在 DropTarget 中创建 |

注意：
- `ContentDropTargetHost` 内部由 C++ 创建 `UWacomZoneDropTarget`，目标为 `EZoneKind::SpecialZone + OwnerInstanceId`。
- 内容卡右键入战请求由 `UWacomSpecialZoneWidget` 转发给 `UWacomBackpackScreen`，WBP 不直接改 Run 状态。

## WBP_WacomDeckCardWidget

父类：`UWacomDeckCardWidget`

推荐绑定：

| 控件名 | 推荐类型 | 用途 |
|---|---|---|
| `CardView` | `UWacomCardView` | 通用卡面显示 |

可选绑定：

| 控件名 | 推荐类型 | 用途 |
|---|---|---|
| `CardBody` | `Border` | 卡牌拖拽视觉状态透明度 |
| `BattleEnabledBadge` | `TextBlock` | SpecialZone 内容卡已选入战 |
| `ProjectedFromBadge` | `TextBlock` | 投影来源或主卡已出战标记 |

注意：
- 卡牌本体不提供删除按钮；删牌通过拖到删牌区触发。
- 未绑定 `CardView` 时仍能生成拖拽 payload，但没有正式卡面显示。

## WBP_CardView

父类：`UWacomCardView`

推荐绑定：

| 控件名 | 推荐类型 | 用途 |
|---|---|---|
| `CostText` | `TextBlock` | 费用 |
| `ValueText` | `TextBlock` | 价值，第一阶段使用删牌金币价值 |
| `PhysiqueText` | `TextBlock` | 身材/容量摘要，例如 `+6生命`、`1耐久/3容量` |
| `NameText` | `TextBlock` | 卡名 |
| `TypeText` | `TextBlock` | 类型/词条 |
| `CardArt` | `Image` | 卡图 |
| `EffectStatsHost` | `PanelWidget` | C++ 动态填充效果数值徽章，例如伤害 7、冻结 1 |
| `DisabledOverlay` | `Border` | 禁用遮罩 |

可选绑定：

| 控件名 | 推荐类型 | 用途 |
|---|---|---|
| `DescriptionText` | `TextBlock` | 详情描述。默认小卡面建议隐藏，后续用于放大态/详情面板 |

注意：
- `UWacomCardView` 只显示 `FWacomCardViewData`，不提交战斗或背包命令。
- 默认小卡面只显示摘要：名字、卡图、词条、费用、价值、身材/容量和效果数值徽章。
- 描述、任务、变化等长文本默认不进入小卡主体；后续应由详情面板或放大态承接。
- `EffectStatsHost` 内部由 C++ 按 `EffectBadges[]` 动态创建 `UWacomCardEffectBadgeWidget`；不要在 WBP 里预放固定数量的徽章。
- 未绑定部分控件不会崩溃，但对应信息不会显示。

## WBP_CardDetailPanel

父类：`UWacomCardDetailPanel`

推荐绑定：

| 控件名 | 推荐类型 | 用途 |
|---|---|---|
| `NameText` | `TextBlock` | 卡牌名称 |
| `DescriptionText` | `TextBlock` | 完整描述文本 |
| `TasksBox` | `PanelWidget` | C++ 动态填充任务行 |
| `ChangesBox` | `PanelWidget` | C++ 动态填充变化行 |
| `PassivesBox` | `PanelWidget` | C++ 动态填充被动行 |

注意：
- `UWacomCardDetailPanel` 只显示 `FWacomCardDetailViewData`，不提交战斗、背包或 Run 命令。
- 当前详情数据只从 `CardDefinition.DisplayName / Description / Passives` 推导；任务、变化字段等待卡牌数据结构正式扩展后接入。
- 该面板不会自动弹出，也不处理 hover / selected 生命周期；背包或战斗 UI 后续决定何时显示它。
- 未绑定槽位时 C++ fallback 会创建基础标题、描述、任务、变化、被动区域。

## WBP_CardEffectBadge

父类：`UWacomCardEffectBadgeWidget`

推荐绑定：

| 控件名 | 推荐类型 | 用途 |
|---|---|---|
| `BadgeBody` | `Border` | 徽章底板，可做圆形/描边材质 |
| `IconImage` | `Image` | 效果图标，贴图由 WBP 自己配置 |
| `ValueText` | `TextBlock` | 效果数值，例如 `7` |
| `LabelText` | `TextBlock` | fallback 标签，例如 `伤害`、`冻结` |

注意：
- `UWacomCardEffectBadgeWidget` 只显示单个 `FWacomCardViewEffectBadge`。
- 后续美术阶段可以在 WBP 里按 `Kind` 配不同图标或颜色；本轮 C++ fallback 只提供基础颜色和文字。
- 该 widget 不提交战斗、背包或 Run 命令。
