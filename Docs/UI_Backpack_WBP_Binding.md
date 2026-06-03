---
type: ui-binding-contract
scope: wacom-ui-backpack
status: active
updated: 2026-06-03
tags:
  - wacom/ui
  - wacom/wbp
  - wacom/backpack
  - wacom/contract
---

# 背包 UI WBP 绑定清单

> [!info] 本文职责
> 本文只记录背包相关 WBP 制作合约。背包规则见 [[WacomRun]]，UI 数据流和交互行为见 [[WacomUI]]。

> [!warning] 合同边界
> 本文不是背包规则文档；不要在 WBP 合同里定义移动、删牌、容量或负重规则。

## WBP_BackpackScreen

父类：`UWacomBackpackScreen`

加载口径：

- 顶层背包 WBP 通过 `Edit > Project Settings > Wacom UI Settings` 注册。
- 在 `WidgetClasses` 中添加 `UI.Widget.BackpackScreen`，Class 指向正式 `WBP_BackpackScreen`。
- 未注册、软类加载失败或类型不匹配时，直接回退 C++ `UWacomBackpackScreen`。
- 当前不会自动按固定路径加载一个总的 `WBP_BackpackScreen`。
- 局部 Zone WBP 仍由 `UWacomBackpackScreen` 按默认路径尝试加载；这些只替换单个区块外壳。

推荐绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `DeleteZoneHost` | `PanelWidget` | C++ 填充删牌 DropTarget |
| `BattleDeckZoneHost` | `PanelWidget` | C++ 填充备战区 DropTarget 和卡牌列表 |
| `FluxContentDropTargetHost` | `PanelWidget` | C++ 填充通量内容 DropTarget 和内容卡列表 |
| `SpecialZonesHost` | `PanelWidget` | C++ 动态填充 `UWacomSpecialZoneWidget` |
| `BurdenZoneHost` | `PanelWidget` | C++ 填充负重区标题、DropTarget 和卡牌列表；无负重卡时折叠 |
| `CardDetailLayer` | `CanvasPanel` | C++ 填充悬浮卡牌详情面板 |

可选绑定：

| 控件名 | 推荐类型 | 缺省行为 |
|---|---|---|
| `TitleText` | `TextBlock` | 不绑则标题不显示 |
| `GoldText` | `TextBlock` | 不绑则金币不显示 |
| `CloseButton` | `Button` | 不绑则需要依赖菜单 Back / ESC 关闭 |
| `DeleteZoneTitleText` | `TextBlock` | 不绑则 C++ 默认外壳显示提示 |
| `BattleDeckTitleText` | `TextBlock` | 不绑则 C++ 默认外壳显示标题 |
| `BackpackTitleText` | `TextBlock` | 不绑则 C++ 默认外壳显示标题 |
| `BattleDeckCardsBox` | `WrapBox` | 不绑则 C++ 在 Host 中创建 |
| `FluxContentCardsBox` | `WrapBox` | 不绑则 C++ 在 Host 中创建 |

WBP 合同：

- `WBP_BackpackScreen` 只做布局和样式，不直接调用 `RunSession`。
- 推荐 Host 未绑定时，对应区域无法显示运行时内容，C++ 会输出 warning。
- `CardDetailLayer` 应覆盖背包界面可见区域，并位于卡牌区域上方；详情面板为 `HitTestInvisible`。
- 旧 `FluxZoneHost / BackpackCardsBox` 混合布局已删除，不要再绑定。

---

## 局部 Zone WBP

父类：`UWacomBackpackZoneSectionWidget`

局部 Zone WBP 用来替换单个区块外壳，不要求一次完成整个 `WBP_BackpackScreen`。

| 推荐资产名 | 用途 |
|---|---|
| `WBP_BackpackDeleteZone` | 删牌区外壳 |
| `WBP_BackpackBattleDeckZone` | 备战区外壳 |
| `WBP_BackpackFluxContentZone` | 通量内容区外壳 |
| `WBP_BackpackSpecialZones` | 特殊存放区列表外壳 |
| `WBP_BackpackBurdenZone` | 负重区外壳 |

推荐绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `TitleText` | `TextBlock` | C++ 写入区块标题 |
| `ContentHost` | `PanelWidget` | C++ 填充 DropTarget、WrapBox、动态卡牌或 SpecialZone 列表 |

WBP 合同：

- `ContentHost` 必须是容器控件，例如 `VerticalBox`、`Overlay`、`CanvasPanel`。
- 不要在 `ContentHost` 里预放卡牌；运行时内容由 C++ 填入。
- 缺少 `ContentHost` 时，仅该局部区块回退到 C++ 默认外壳。
- 局部 Zone WBP 不直接调用 `RunSession`。

---

## WBP_WacomSpecialZoneWidget

父类：`UWacomSpecialZoneWidget`

推荐绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `TitleText` | `TextBlock` | C++ 写入特殊区标题 |
| `BattleReadyBadge` | `TextBlock` | 显示 B 主卡已入战标记 |
| `OwnerCardHost` | `PanelWidget` | C++ 填充 B 主卡 |
| `ContentDropTargetHost` | `PanelWidget` | C++ 填充 SpecialZone DropTarget |

可选绑定：

| 控件名 | 推荐类型 | 缺省行为 |
|---|---|---|
| `ContentCardsBox` | `WrapBox` | 不绑则 C++ 在 DropTarget 中创建 |

WBP 合同：

- `ContentDropTargetHost` 内部由 C++ 创建 `UWacomZoneDropTarget`。
- 内容卡右键入战请求由 `UWacomSpecialZoneWidget` 转发给 `UWacomBackpackScreen`，WBP 不直接改 Run 状态。

---

## WBP_WacomDeckCardWidget

父类：`UWacomDeckCardWidget`

推荐绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `CardView` | `UWacomCardView` | 通用卡面显示 |

可选绑定：

| 控件名 | 推荐类型 | 缺省行为 |
|---|---|---|
| `CardBody` | `Border` | 不绑则无法显示拖拽源透明度变化 |
| `BattleEnabledBadge` | `TextBlock` | 不绑则不显示 SpecialZone 内容卡已选入战 |
| `ProjectedFromBadge` | `TextBlock` | 不绑则不显示投影来源或主卡已出战标记 |

WBP 合同：

- 卡牌本体不提供删除按钮；删牌通过拖到删牌区触发。
- 未绑定 `CardView` 时仍能生成拖拽 payload，但没有正式卡面显示。

---

## WBP_CardView

父类：`UWacomCardView`

推荐绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `CostDigitImage` | `Image` | 一位数费用图标，优先用于美术数字 |
| `ValueText` | `TextBlock` | 价值，当前用于删牌金币价值 |
| `NameText` | `TextBlock` | 卡名 |
| `TypeText` | `TextBlock` | 类型 / 词条 |
| `CardArt` | `Image` | 卡图 |
| `EffectStatsHost` | `PanelWidget` | C++ 动态填充效果数值徽章 |
| `DisabledOverlay` | `Border` | 禁用遮罩 |

可选绑定：

| 控件名 | 推荐类型 | 缺省行为 |
|---|---|---|
| `SurfaceFoilOverlay` | `Image` | 卡面弱流光覆盖层；未绑定时不显示流光 |

WBP 合同：

- `UWacomCardView` 只显示 `FWacomCardViewData`，不提交战斗、背包或 Run 命令。
- 卡牌主体只保留主要名字、类型、卡图、图片数字和少量必要徽章；完整描述、被动、长规则文本和身材说明由 `WBP_CardDetailPanel` 承接。
- 费用图标只使用固定 `CostDigitImage`，C++ 只替换它的 Brush，不动态创建费用子控件；费用为多位数、缺数字图标或未绑定该 Image 时，卡牌主体不再显示文字费用。
- `EffectStatsHost` 内部由 C++ 按 `EffectBadges[]` 动态创建 `UWacomCardEffectBadgeWidget`。
- `SurfaceFoilOverlay` 推荐放在卡面内容最上层、`DisabledOverlay` 下方，Brush 使用 `/Game/DreamMaterials/Card/M_CardSurface_CosmicFoil`；它必须设为不可命中，不要挡住战斗手牌或背包拖拽。
- 未绑定 `SurfaceFoilOverlay` 时，C++ 会在运行时尝试挂到第一个 `Overlay` 容器上作为临时覆盖层；正式 WBP 仍建议显式绑定，便于控制层级。
- 未绑定部分控件不会崩溃，但对应信息不会显示。

---

## WBP_CardDetailPanel

父类：`UWacomCardDetailPanel`

推荐绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `SectionsBox` | `PanelWidget` | C++ 动态填充详情区块 |

WBP 合同：

- `UWacomCardDetailPanel` 只显示 `FWacomCardDetailViewData`，不提交战斗、背包或 Run 命令。
- 面板会把非空详情数据转成多个 `UWacomCardDetailSectionWidget`。
- 未绑定 `SectionsBox` 时 C++ fallback 会创建基础容器。

---

## WBP_CardDetailSection

父类：`UWacomCardDetailSectionWidget`

推荐绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `TitleText` | `TextBlock` | 区块标题 |
| `LinesBox` | `PanelWidget` | C++ 动态填充区块文本行 |

WBP 合同：

- 该 Widget 是详情区块通用模板。
- 未绑定槽位时 C++ fallback 会创建基础标题和多行文本。

---

## WBP_CardEffectBadge

父类：`UWacomCardEffectBadgeWidget`

推荐绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `BadgeBody` | `Border` | 徽章底板 |
| `IconImage` | `Image` | 效果图标，贴图由 WBP 自己配置 |
| `ValueText` | `TextBlock` | 效果数值 |
| `LabelText` | `TextBlock` | fallback 标签 |

WBP 合同：

- `UWacomCardEffectBadgeWidget` 只显示单个 `FWacomCardViewEffectBadge`。
- 该 Widget 不提交战斗、背包或 Run 命令。

---

## PIE 检查清单

- 推荐 Host 绑定后，各区能显示 C++ 动态填充内容。
- 通量区只展示内容卡。
- 卡牌 hover 时详情层不抢拖拽或右键。
- 负重区无溢出卡时折叠，有溢出卡时显示。
- 拖拽失败和删牌失败仍由 AppToast 给出原因。
