---
type: ui-binding-contract
scope: wacom-ui-backpack
status: active
updated: 2026-07-08
tags:
  - wacom/ui
  - wacom/wbp
  - wacom/backpack
  - wacom/contract
---

# 背包 UI WBP 绑定清单

> [!info] 本文职责
> 本文只记录背包、卡牌显示和卡牌详情相关 WBP 制作合约。背包规则、容量、删牌、负重和持有区事务见 [`WacomRun.md`](WacomRun.md)；卡牌字段见 [`WacomData.md`](WacomData.md)；UI 数据流和交互入口见 [`WacomUI.md`](WacomUI.md)。

> [!warning] 合同边界
> WBP 只负责布局、样式、拖拽视觉和展示层表现，不直接调用 `URunSession`，不定义移动、删牌、容量、负重或入战规则。

## WBP_BackpackScreen

父类：`UWacomBackpackScreen`

推荐资产：`WBP_BackpackScreen`

注册方式：

- 顶层背包 WBP 通过 `Edit > Project Settings > Wacom UI Settings` 注册。
- 在 `WidgetClasses` 中添加 `UI.Widget.BackpackScreen`，Class 指向正式 `WBP_BackpackScreen`。
- 未注册、软类加载失败或类型不匹配时，回退 C++ `UWacomBackpackScreen`。

推荐绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `DeleteZoneHost` | `PanelWidget` | C++ 填充删牌 DropTarget |
| `BattleDeckZoneHost` | `PanelWidget` | C++ 填充备战区 DropTarget 和卡牌列表 |
| `FluxContentDropTargetHost` | `PanelWidget` | C++ 填充通量内容 DropTarget 和内容卡列表 |
| `SpecialZonesHost` | `PanelWidget` | C++ 动态填充 `UWacomSpecialZoneWidget` |
| `BurdenZoneHost` | `PanelWidget` | C++ 填充负重区标题和卡牌列表；正式 UI 不暴露主动拖入负重区的 DropTarget，无负重卡时折叠 |
| `CardDetailLayer` | `CanvasPanel` | C++ 填充悬浮卡牌详情面板 |

可选绑定：

| 控件名 | 推荐类型 | 缺省行为 |
|---|---|---|
| `TitleText` | `TextBlock` | 不绑则标题不显示 |
| `GoldText` | `TextBlock` | 不绑则金币不显示 |
| `CloseButton` | `Button` | 不绑则依赖菜单 Back / ESC 关闭 |
| `DeleteZoneTitleText` | `TextBlock` | 不绑则 C++ 默认外壳显示提示 |
| `BattleDeckTitleText` | `TextBlock` | 不绑则 C++ 默认外壳显示标题 |
| `BackpackTitleText` | `TextBlock` | 不绑则 C++ 默认外壳显示标题 |
| `BattleDeckCardsBox` | `WrapBox` | 不绑则 C++ 在 Host 中创建 |
| `FluxContentCardsBox` | `WrapBox` | 不绑则 C++ 在 Host 中创建 |

WBP 不应做：

- 不直接调用 `URunSession` 或背包命令 API。
- 不预放运行时卡牌；Host 内容由 C++ 根据 snapshot 填充。
- 不在 WBP 图里判断容量、删牌金币、负重、SpecialZone 入战或拖拽目标是否合法；DropTarget hover preview、drop 提交、删牌奖励、入战 toggle validation 和移动 / 删牌 / 入战 Toast 文案由 `UWacomBackpackScreen` 私有 command flow / presentation helper 统一处理。
- 不绑定旧 `FluxZoneHost / BackpackCardsBox` 混合布局槽位；这些旧槽位不再是制作合同。

最小验收：

- 推荐 Host 绑定后，删牌、备战、通量内容、SpecialZone 和负重区都能显示 C++ 动态填充内容。
- `CardDetailLayer` 覆盖背包界面可见区域，位于卡牌区域上方，详情面板不抢拖拽或右键输入。
- 拖拽 hover preview、最终 drop 提交和 SpecialZone 内容卡右键入战使用 Screen flow validation；WBP / DropTarget / DeckCard 不自行复制容量、删牌奖励、入战归属规则或 Toast 文案映射。
- 推荐 Host 未绑定时，对应区域缺失运行时内容，C++ 只作为兼容路径输出 warning 或使用默认外壳。

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

WBP 不应做：

- 不在 `ContentHost` 里预放运行时卡牌。
- 不直接调用 `URunSession`。
- 不把区块外壳写成规则入口；它只承接样式、标题和内容容器。

最小验收：

- `ContentHost` 是容器控件，例如 `VerticalBox`、`Overlay`、`CanvasPanel`。
- 局部 WBP 缺少 `ContentHost` 时，仅该区块回退到 C++ 默认外壳。
- 运行时 DropTarget、WrapBox、动态卡牌或 SpecialZone 列表由 C++ 填入。

## WBP_WacomSpecialZoneWidget

父类：`UWacomSpecialZoneWidget`

推荐资产：`WBP_WacomSpecialZoneWidget`

该资产是单个 SpecialZone 区块的可选美术制作入口，不是运行时硬依赖。未制作或未在 `UWacomBackpackScreen.SpecialZoneWidgetClass` 指向该资产时，`UWacomBackpackScreen` 会使用 C++ `UWacomSpecialZoneWidget` fallback，并由 `SpecialZonesHost -> SpecialZonesPanel -> UWacomSpecialZoneWidget` 链路动态填充。

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

WBP 不应做：

- 不直接改 Run 状态。
- 不自行判定 B 主卡是否可入战或内容卡是否可右键入战。
- 不在 `ContentDropTargetHost` 中预放正式 DropTarget；C++ 会创建 `UWacomZoneDropTarget`。

最小验收：

- B 主卡、内容 DropTarget 和内容卡列表能按 snapshot 显示。
- 内容卡右键入战请求由 `UWacomSpecialZoneWidget` 转发给 `UWacomBackpackScreen`。
- `BattleReadyBadge` 只显示入战标记，不提交入战命令。

## WBP_WacomDeckCardWidget

父类：`UWacomDeckCardWidget`

推荐资产：`WBP_WacomDeckCardWidget`

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

WBP 不应做：

- 不提供删除按钮；删牌通过拖到删牌区触发。
- 不直接修改卡牌所在持有区。
- 不自行构造拖拽 payload。
- 不根据所在列表、`PhysicalZone` 或 `bBattleEnabledInSpecialZone` 自行判定右键入战是否可用；该 affordance 由 C++ ViewData 写入。

最小验收：

- `CardView` 能显示 `FWacomCardViewData`。
- 绑定 `CardBody` 后，拖拽源透明度变化可见。
- 未绑定 `CardView` 时仍能生成拖拽 payload，但没有正式卡面显示。
- SpecialZone 内容卡和 BattleDeck 投影卡是否可右键切换入战，只由 `FRunStorageCardView.bCanToggleBattleEnabledInSpecialZone` 驱动。

## WBP_CardView

父类：`UWacomCardView`

推荐资产：`WBP_CardView`

推荐绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `CostDigitImage` | `Image` | 一位数费用图标，优先用于美术数字 |
| `ValueText` | `TextBlock` | 价值，当前用于删牌金币价值 |
| `NameText` | `TextBlock` | 卡名 |
| `TypeText` | `TextBlock` | 类型 / 词条 |
| `CardArt` | `Image` | 卡图 |
| `EffectBadgeSlot1` | `Overlay` | 第 1 个效果数值徽章固定插槽 |
| `EffectBadgeSlot2` | `Overlay` | 第 2 个效果数值徽章固定插槽 |
| `EffectBadgeSlot3` | `Overlay` | 第 3 个效果数值徽章固定插槽 |
| `EffectBadgeSlot4` | `Overlay` | 第 4 个效果数值徽章固定插槽 |
| `DisabledOverlay` | `Border` | 禁用遮罩 |

可选绑定：

| 控件名 | 推荐类型 | 缺省行为 |
|---|---|---|
| `SurfaceFoilOverlay` | `Image` | 卡面弱流光覆盖层；未绑定时不显示流光 |
| `EffectStatsHost` | `PanelWidget` | 兼容流式效果徽章；未绑定任意 `EffectBadgeSlot*` 时才使用 |

WBP 不应做：

- 不提交战斗、背包或 Run 命令。
- 不在卡牌主体里承载完整描述、被动、长规则文本或身材说明；这些由详情面板承接。
- 不动态创建费用子控件；费用图标只使用固定 `CostDigitImage`。
- 不把 Slow / Freeze / Twilight / Draw / Discard / Initiative / Cost 等效果强行放进主体徽章。

最小验收：

- `UWacomCardView` 只显示 `FWacomCardViewData`。
- `EffectBadgeSlot1-4` 按 `EffectBadges[]` 顺序动态创建 `UWacomCardEffectBadgeWidget`，不是按类型固定位置。
- 只要绑定了任意 `EffectBadgeSlot*`，固定插槽模式启用，空 slot 折叠，超过 4 个的徽章不显示。
- 当前主体只显示美术已配置的五类徽章：伤害、中毒、灼烧、回复、护盾。
- `SurfaceFoilOverlay` 放在卡面内容最上层、`DisabledOverlay` 下方，保持不可命中，不挡拖拽。
- 未绑定部分控件不会崩溃，但对应信息不会显示。

## WBP_CardDetailPanel

父类：`UWacomCardDetailPanel`

推荐资产：`WBP_CardDetailPanel`

推荐绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `SectionsBox` | `PanelWidget` | C++ 动态填充详情区块 |

WBP 不应做：

- 不提交战斗、背包或 Run 命令。
- 不在 WBP 图里重新计算卡牌规则说明。
- 不阻挡背包拖拽或右键输入。

最小验收：

- `UWacomCardDetailPanel` 只显示 `FWacomCardDetailViewData.Sections`。
- 面板会把非空 semantic section 动态转成多个 `UWacomCardDetailSectionWidget`。
- `Description` section 来自卡牌主动效果 explanation blocks；`Passive` section 来自被动触发 explanation block 和被动效果 blocks。
- `UCardDefinition::Description`、`FCardPassive::DisplayText`、旧 `Description`、`ChangeLines`、`PassiveLines`、`TaskLines`、扁平 `TokenLines` 和旧 token flow 字段都不再进入详情面板。
- 未绑定 `SectionsBox` 时，C++ 兼容路径会创建基础容器。

## WBP_CardDetailSection

父类：`UWacomCardDetailSectionWidget`

推荐资产：`WBP_CardDetailSection`

`WBP_CardDetailPanel` 的 Class Defaults 可以通过 `SectionWidgetClass` 手动指定其它 Section WBP；未指定时使用约定路径或 C++ fallback。

推荐绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `TitleText` | `CommonTextBlock` | 区块标题；建议通过 `UWacomCardDetailTheme.TitleTextStyle` 或 WBP 样式统一字体 |
| `BodyText` | `UWacomCardDetailRichTextBlock` | RichText 正文；C++ 会把 semantic blocks/runs 渲染为 markup 并设置文本 |

WBP 不应做：

- 不提交战斗、背包或 Run 命令。
- 不解析 `UCardDefinition`、`FCardEffect`、`FCardPassive` 或 GameplayTag。
- 不把区块模板写成特定卡牌规则入口；模板归 `UWacomCardExplanationLexicon` 管。

最小验收：

- 该 Widget 作为详情区块通用模板使用。
- 描述和被动都通过同一套 Section 外框显示；正文由 `BodyText` 的 RichText style set 决定字体、颜色、runtime preview 数值语气等视觉。
- `BodyText` 至少要能显示普通文本、`Value`、`ValueBuffed`、`ValueNerfed`、`Status`、`Keyword`、`Muted` 几类 RichText style。目标预览时正文只显示最终数值，强化值用 `ValueBuffed`，削弱值用 `ValueNerfed`，不再显示 `基础值 -> 预览值`。
- 未绑定槽位时，C++ 兼容路径会创建基础标题和 RichText 正文。

## Card Explanation Assets

详情正文由 C++ 生成 semantic document，再由 RichText 渲染。美术 / 策划后续主要调整这两个 DataAsset：

| 资产类型 | 用途 |
|---|---|
| `UWacomCardExplanationLexicon` | 配置效果和被动触发的 typed explanation template。精确 `EffectType / Passive.Trigger` 优先，找不到时尝试父 tag fallback。 |
| `UWacomCardDetailTheme` | 配置标题 CommonTextStyle、正文 RichText style set、inline 图标 / 状态 brush 和 fallback brush。 |

模板 slot v1 支持：

| Slot | 说明 |
|---|---|
| `{value:Magnitude}` | 效果数值；目标预览时显示最终数值，并通过 `ValueBuffed / ValueNerfed` 标记强化或削弱。 |
| `{value:TriggerThreshold}` | 被动触发阈值。 |
| `{icon:EffectIcon}` | 当前效果图标语义。 |
| `{status:EffectStatus}` | 当前效果关联状态，例如中毒、冻结、眩晕。 |
| `{keyword:Tag}` | 当前效果或触发 tag 的关键词显示。 |

旧 `WBP_CardDetailTokenFlow / WBP_CardDetailTokenLine / WBP_CardDetailToken` 已删除，不再作为运行时依赖或制作入口；详情正文统一从 semantic `Blocks / Runs` 渲染到 `WBP_CardDetailSection` 的 `BodyText`。

## WBP_CardEffectBadge

父类：`UWacomCardEffectBadgeWidget`

推荐资产：`WBP_CardEffectBadge`

推荐绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `BadgeFrameImage` | `Image` | 按效果类型显示伤害 / 中毒 / 灼烧 / 回复 / 护盾等徽章底图 |
| `DigitHost` | `HorizontalBox` | C++ 按数值动态填充图片数字 |

WBP 不应做：

- 不提交战斗、背包或 Run 命令。
- 不使用 TextBlock 显示正式效果数值。
- 不把 `MaxHpBonus` 做成效果徽章；黄色“额外生命值”美术图标当前对应 `Status.Shield` 护盾效果。

最小验收：

- `UWacomCardEffectBadgeWidget` 只显示单个 `FWacomCardViewEffectBadge`。
- `DigitHost` 放在 `BadgeFrameImage` 上方，并保持 `HitTestInvisible`。
- `BadgeFrameSprites` 和 `DigitSprites` 在该 Widget Class Defaults 中配置。
- 缺少底图时只隐藏底图；缺少任意数字图时隐藏数字。
- 图片数字默认至少显示 3 位，个位数补零成 `001`；中间数字使用 `InteriorDigitPadding`。
- `EWacomCardViewEffectBadgeKind` 其他枚举值只作为后续扩展 / debug 余量。

## PIE Smoke Checklist

- 推荐 Host 绑定后，删牌、备战、通量内容、SpecialZone 和负重区都能显示 C++ 动态填充内容。
- 通量区只展示内容卡。
- SpecialZone 能显示 B 主卡、内容卡和入战标记。
- 负重区无溢出卡时折叠，有溢出卡时显示。
- 卡牌 hover 时详情层不抢拖拽或右键。
- 拖拽失败和删牌失败仍由 AppToast 给出原因。
