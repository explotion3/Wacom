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
| `BackpackTitleText` | `TextBlock` | 背包区标题。通量内容占用/容量由通量内容区标题显示 |
| `CloseButton` | `Button` | 关闭背包 |
| `BattleDeckCardsBox` | `WrapBox` | 直接承接备战卡列表；不绑则 C++ 在 Host 中创建 |
| `FluxMainCardsBox` | `WrapBox` | 直接承接通量主卡；不绑则 C++ 在 Host 中创建 |
| `FluxContentCardsBox` | `WrapBox` | 直接承接通量内容卡；不绑则 C++ 在 Host 中创建 |

注意：
- 旧 `FluxZoneHost / BackpackCardsBox` 混合布局已删除，不要再绑定。
- 如果推荐 Host 没绑定，C++ 会输出 warning，该区域不会显示运行时内容。
- `CardDetailLayer` 应覆盖背包界面可见区域，并放在卡牌区域之上；详情面板为 `HitTestInvisible`，不会抢拖拽或右键输入。
- WBP 只负责布局和样式，不直接调用 `RunSession`。

## 局部 Zone WBP

如果暂时不制作完整 `WBP_BackpackScreen`，可以先制作局部区块 WBP。`UWacomBackpackScreen`
的 C++ fallback 会按下列约定路径自动加载；资产不存在时继续使用 C++ fallback。

父类统一选择：`UWacomBackpackZoneSectionWidget`

| 资产名 | 用途 |
|---|---|
| `WBP_BackpackDeleteZone` | 删牌区外壳 |
| `WBP_BackpackBattleDeckZone` | 备战区外壳 |
| `WBP_BackpackFluxMainZone` | 通量主卡区外壳 |
| `WBP_BackpackFluxContentZone` | 通量内容区外壳 |
| `WBP_BackpackSpecialZones` | 特殊存放区列表外壳 |
| `WBP_BackpackBurdenZone` | 负重区外壳 |

推荐绑定：

| 控件名 | 推荐类型 | 用途 |
|---|---|---|
| `TitleText` | `TextBlock` | C++ 写入区块标题，例如 `[ 备战区 ] 5 / 15` |
| `ContentHost` | `PanelWidget` | C++ 填充 DropTarget、WrapBox、动态卡牌或 SpecialZone 列表 |

注意：
- 局部 Zone WBP 只做外壳样式；不要在里面写移动、删牌、刷新逻辑。
- `ContentHost` 必须是容器控件，例如 `VerticalBox`、`Overlay`、`CanvasPanel`。
- 如果某个局部 Zone WBP 缺少 `ContentHost`，C++ 会只让该区块回退默认外壳，其他区块不受影响。
- 不要在 `ContentHost` 里预放卡牌；运行时内容由 C++ 填入。

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
| `SectionsBox` | `PanelWidget` | C++ 动态填充详情区块 |

注意：
- `UWacomCardDetailPanel` 只显示 `FWacomCardDetailViewData`，不提交战斗、背包或 Run 命令。
- 当前详情数据只从 `CardDefinition.DisplayName / Description / Passives` 推导；任务、变化字段等待卡牌数据结构正式扩展后接入。
- 面板本身不显示卡名；小卡旁悬浮时只展示默认隐藏内容。
- 面板会把非空 `Description / TaskLines / ChangeLines / PassiveLines` 转成多个 `UWacomCardDetailSectionWidget`。
- 未绑定 `SectionsBox` 时 C++ fallback 会创建基础容器。

## WBP_CardDetailSection

父类：`UWacomCardDetailSectionWidget`

推荐绑定：

| 控件名 | 推荐类型 | 用途 |
|---|---|---|
| `TitleText` | `TextBlock` | 区块标题，例如 `描述`、`任务`、`变化`、`被动` |
| `LinesBox` | `PanelWidget` | C++ 动态填充区块文本行 |

注意：
- `UWacomCardDetailSectionWidget` 是详情区块通用模板。
- 后续新增“容量效果 / 获得方式 / 稀有度说明”等详情内容时，优先新增数据到 section，不再给 `WBP_CardDetailPanel` 添加固定控件。
- 未绑定槽位时 C++ fallback 会创建基础标题和多行文本。

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
