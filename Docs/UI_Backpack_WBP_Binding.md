---
type: ui-binding-contract
scope: wacom-ui-backpack
status: active
updated: 2026-07-13
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
> WBP 只负责布局、样式、Workspace 指针表现和展示层表现，不直接调用 `URunSession`，不定义移动、删牌、容量、负重或入战规则。

## WBP_BackpackScreen

父类：`UWacomBackpackScreen`

> 新正式结构以以下 Workspace 合同为准；下文旧 simultaneous-zone Host 仅用于尚未迁移资产的折叠兼容，不得继续作为可见输入 owner。

| 必选/推荐绑定 | 类型 | 合同 |
|---|---|---|
| `WorkspaceHost` | `PanelWidget` | 中央唯一活动区域，容纳 `UWacomBackpackWorkspaceWidget` |
| `ZoneRackHost` | `PanelWidget` | 右侧常驻区域牌匣 |
| `DeleteTargetHost` | `PanelWidget` | 批量销毁目标，仅接收 Screen 已验证的携带释放 |
| `DeleteConfirmHost` | `PanelWidget` | 批量确认 modal host，默认 Collapsed |
| `ArrangeAllButton` | `Button` | 只重置当前区布局，不调用 Run move API |
| `CardDetailLayer` | `CanvasPanel` | 非携带期详情覆盖层 |

推荐子资产父类分别为 `UWacomBackpackWorkspaceWidget`、`UWacomBackpackZoneRackWidget`、`UWacomBackpackZoneRackEntryWidget`、`UWacomBackpackDeleteConfirmWidget`。这些 WBP 必须保持被动，不访问 `URunSession`，不显示携带索引/数量文字；目标 preview、批量提交、Toast 和确认恢复都由 Screen flow 负责。

正式资产：

| 资产 | 父类 / 职责 |
|---|---|
| `WBP_BackpackScreen` | `UWacomBackpackScreen`；CommonUI Screen 与正式 Host |
| `WBP_BackpackWorkspace` | `UWacomBackpackWorkspaceWidget`；中央 Canvas、框选层和空状态 |
| `WBP_BackpackZoneRack` | `UWacomBackpackZoneRackWidget`；右侧常驻区域列表 |
| `WBP_BackpackZoneRackEntry` | `UWacomBackpackZoneRackEntryWidget`；激活、合法目标和拒绝目标表现 |
| `WBP_BackpackDeleteConfirm` | `UWacomBackpackDeleteConfirmWidget`；批量数量/奖励和 modal 焦点 |
| `WBP_BackpackCardView` | `UWacomRetainedCardViewWidget`；静态 Retainer 包装并复用新版 authored 卡面，关闭内层动态 `SurfaceFoilOverlay`，不拥有输入或 first-person 效果 |
| `DA_BackpackWorkspaceStyle` | 卡牌尺寸、边界、扇形、lift、运动时间、状态颜色和 `M_BackpackWorkspaceCardFeedback`；材质未加载时自动退回纯色边框 |

上述资产由 WacomEditor builder 重建；同一 builder 会把 `WBP_WacomDeckCardWidget.BackpackCardView` 校准为 `WBP_BackpackCardView`，后者在单个静态 Retainer 内复用已完成新版排版的 `WBP_FirstPersonCardView`，外层继续使用 `CardFaceScaleBox` 等比缩放。Retainer 只在子控件失效或卡面数据变化后重绘，不启用逐帧 phase，也不绑定 first-person 效果材质。背包 wrapper 默认关闭内层 `SurfaceFoilOverlay`，同时折叠该层并清空其材质 Brush，避免 invalidation-only 缓存冻结动态流光；该实例策略不修改战斗/第一人称卡面资产的默认流光。禁止在生成资产里复制 Run 规则或增加并行输入 owner：

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\Wacom\Wacom.uproject' -run=WacomBuildBackpackUI -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache
```

`M_BackpackWorkspaceCardFeedback` 以 `DShader/Material/Card/M_BackpackWorkspaceCardFeedback.dsm` 为真源。修改 DreamShader 源码或升级/修补插件生成器后，必须显式强制重建材质资产：

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\Wacom\Wacom.uproject' -run=DreamShader compile '-Source=DShader/Material/Card/M_BackpackWorkspaceCardFeedback.dsm' -Force -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache
```

`UMaterialExpressionVertexColor` 的默认输出 0 是 `RGB`，独立输出 4 才是 `A`。材质必须通过 `UE.Expression(Class="VertexColor", OutputIndex=0/4)` 分别连接两者；禁止对默认输出 0 再做 A `ComponentMask`，否则 SM6 会对 `float3` 请求第 4 分量并报 `Not enough components`。DreamShader 返回“Generated”只表示资产图生成成功，不等于最终 ShaderMap 编译成功；`Wacom.UI.Backpack.Material.WorkspaceFeedbackCompiles` 会调用 UE 材质重编译、检查 compile errors，穿透 Named Reroute 验证 RGB/Alpha 的真实源输出索引，并禁止非法 A mask。

当前像素安全制作基准为 1600×900、DPI 1.0；跨分辨率缩放仍按 `Docs/TechDebt.md` 的像素安全适配项处理，不在 WBP 内复制规则布局。
`ScreenSize -> Root -> MainLayout` 的槽位必须显式水平/垂直 Fill；只设置 `SizeBox` 宽高而不设置子槽 Fill，会让内部布局退回最小 Desired Size。进入持续携带状态后，卡牌详情层必须保持隐藏，避免遮挡扇形和鼠标目标。
Screen 在运行时向 `WorkspaceHost`、`ZoneRackHost`、`DeleteConfirmHost` 插入子控件时，必须按宿主实际 Slot 类型显式配置 Fill，不能依赖 `UPanelWidget::AddChild` 的默认槽位参数；尤其 `OverlaySlot` 默认 Left/Top，会把 Desired Size 为零的 Canvas Workspace 压缩到零尺寸。`WorkspaceWidget` 还必须保持 `Visible`，不能使用 `SelfHitTestInvisible`，因为中央空白区域需要由它接收左键并开始框选。
Workspace 首次加入 CommonUI 层时，Snapshot reconcile 可能早于 Slate 几何完成。此时 `CardCanvas` 必须暂时隐藏，不能把 `1280×720` 回退坐标暴露给玩家；Workspace 通过短生命周期 ActiveTimer 等待真实尺寸连续稳定两帧，再向 Screen 广播一次布局刷新。最终 `CanvasSlot` 尺寸、位置、角度和交互表现全部写入后，Workspace 仍在下一次 Slate 更新请求一次静态卡面补绘，且早于 `NativeConstruct()` 的请求不能丢失。

`UI.Layer.GameMenu` 的 CommonUI 进入/退出过渡会把层级 Alpha 传入子控件；若静态 Retainer 在过渡中首次离屏绘制，该 Alpha 会被烘入卡面缓存，造成界面已经完全显示而卡牌仍半透明。`UWacomPrimaryGameLayout` 因此缓存并广播每个 Layer Stack 的 `OnTransitioningChanged` 状态；`UWacomBackpackScreen` 只订阅所属 GameMenu 层，在过渡期间让 Workspace 所有卡面调用 `SetRetainRendering(false)` 直绘，过渡结束后恢复 retained rendering 并显式 `RequestRender()`。该状态必须同时应用到已绑定卡牌和过渡中后创建的卡牌；Screen `NativeDestruct()` 必须解除订阅。这里不使用固定延迟，也不启用逐帧 phase。几何与补绘 ActiveTimer 完成后立即停止，不形成空闲 Tick 或逐帧 Snapshot rebuild。
正式 Screen 把 `ArrangeAllButton` 放在顶部命令栏，不占用中央自由工作台的安全边界。默认整理在行数超出可用高度时压缩行距，保持每行坐标可辨识；手动位置、角度和 ZOrder 不受该默认布局算法影响。

Editor build 可执行 `Wacom.Backpack.SeedPIEValidation`，通过正常 Run 获牌入口把当前 Run 补到至少 24 张实体牌和 2 个 SpecialZone，用于正式 WBP 的结构、密集布局、框选和携带 PIE。该命令不自动保存，也不伪造 SpecialZone 内容、Burden 或事务拒绝条件。

注册方式：

- 顶层背包 WBP 通过 `Edit > Project Settings > Wacom UI Settings` 注册。
- 在 `WidgetClasses` 中添加 `UI.Widget.BackpackScreen`，Class 指向正式 `WBP_BackpackScreen`。
- 未注册、软类加载失败或类型不匹配时，回退 C++ `UWacomBackpackScreen`。

### 迁移兼容绑定（非正式制作合同）

以下槽位已经从正式 `WBP_BackpackScreen` 移除，只为 C++ fallback 和迁移期旧测试保留。它们是只读展示路径，不拥有拖拽、移动或销毁输入；新资产不得继续添加这些绑定。

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `DeleteZoneHost` | `PanelWidget` | C++ 填充只读迁移提示，不接收销毁输入 |
| `BattleDeckZoneHost` | `PanelWidget` | C++ 填充只读备战区卡牌列表 |
| `FluxContentDropTargetHost` | `PanelWidget` | C++ 填充只读通量内容卡列表 |
| `SpecialZonesHost` | `PanelWidget` | C++ 动态填充只读 `UWacomSpecialZoneWidget` 区块；内容卡右键入战仍经 Screen command flow |
| `BurdenZoneHost` | `PanelWidget` | C++ 填充负重区标题和卡牌列表；正式 UI 不暴露主动拖入负重区的 DropTarget，无负重卡时折叠 |
| `CardDetailLayer` | `CanvasPanel` | C++ 填充悬浮卡牌详情面板 |

迁移兼容可选绑定：

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
- 不在 WBP 图里判断容量、删牌金币、负重、SpecialZone 入战或携带目标是否合法；Workspace/ZoneRack/DeleteTarget preview、批量提交、删牌奖励、入战 toggle validation 和移动 / 删牌 / 入战 Toast 文案由 `UWacomBackpackScreen` 私有 command flow / presentation helper 统一处理。
- 不绑定旧 `FluxZoneHost / BackpackCardsBox` 混合布局槽位；这些旧槽位不再是制作合同。

兼容路径最小验收（只用于迁移期回归）：

- 推荐 Host 绑定后，删牌提示、备战、通量内容、SpecialZone 和负重区都能显示 C++ 动态填充的只读内容。
- `CardDetailLayer` 覆盖背包界面可见区域，位于卡牌区域上方，详情面板不抢 Workspace 指针或右键输入。
- 迁移 Host 不支持单卡 drag/drop；正式 Workspace 的目标 preview、最终批量提交和 SpecialZone 内容卡右键入战使用 Screen flow validation，WBP / DeckCard 不自行复制容量、删牌奖励、入战归属规则或 Toast 文案映射。
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
| `ContentHost` | `PanelWidget` | C++ 填充只读 WrapBox、动态卡牌或 SpecialZone 列表 |

WBP 不应做：

- 不在 `ContentHost` 里预放运行时卡牌。
- 不直接调用 `URunSession`。
- 不把区块外壳写成规则入口；它只承接样式、标题和内容容器。

最小验收：

- `ContentHost` 是容器控件，例如 `VerticalBox`、`Overlay`、`CanvasPanel`。
- 局部 WBP 缺少 `ContentHost` 时，仅该区块回退到 C++ 默认外壳。
- 运行时只读 WrapBox、动态卡牌或 SpecialZone 列表由 C++ 填入。

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
| `ContentDropTargetHost` | `PanelWidget` | 历史命名的只读内容 Host；C++ 填充内容卡列表，不创建 DropTarget |

可选绑定：

| 控件名 | 推荐类型 | 缺省行为 |
|---|---|---|
| `ContentCardsBox` | `WrapBox` | 不绑则 C++ 在只读内容 Host 中创建 |

WBP 不应做：

- 不直接改 Run 状态。
- 不自行判定 B 主卡是否可入战或内容卡是否可右键入战。
- 不在 `ContentDropTargetHost` 中预放输入控件；该字段只因旧资产兼容保留名称，移动输入统一属于正式 Workspace。

最小验收：

- B 主卡和内容卡列表能按 snapshot 显示。
- 内容卡右键入战请求由 `UWacomSpecialZoneWidget` 转发给 `UWacomBackpackScreen`。
- `BattleReadyBadge` 只显示入战标记，不提交入战命令。

## WBP_WacomDeckCardWidget

父类：`UWacomDeckCardWidget`

推荐资产：`WBP_WacomDeckCardWidget`

推荐绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `CardFaceScaleBox` | `ScaleBox` | 保留卡面 `360×424` 设计坐标系，使用固定 `UserSpecified = 0.75` 统一缩放；子槽必须水平/垂直居中，不能 Fill 后重排卡面 |
| `BackpackCardView` | `WBP_BackpackCardView`（父类 `UWacomRetainedCardViewWidget`） | 内部 `CardFaceRetainer -> WBP_FirstPersonCardView`；按数据失效重绘并提供缩放后的抗锯齿缓存，关闭动态 `SurfaceFoilOverlay`，不包含 fake-3D、战斗反馈或手势语义 |
| `WorkspaceFeedbackOverlay` | `Border` | 位于卡面上方、角标下方的纯表现反馈层；选中、合法目标和拒绝状态使用 Style 颜色与不透明度，显示时必须为 `HitTestInvisible` |

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
- 不改用 `WBP_FPCardView`：它是第一人称手牌的实时 Retainer/反馈 wrapper。背包使用独立 `WBP_BackpackCardView`，只保留静态 Retainer 与同源 `WBP_FirstPersonCardView` 卡面。
- 不在 invalidation-only Retainer 中重新开启内层动态 `SurfaceFoilOverlay`；如未来背包需要流光，应先提供显式持续重绘预算或改为适合静态缓存的独立效果，再调整 wrapper 策略。
- 不把 `BackpackCardView` 或其内部 `CardView` 直接放进 Fill 槽后依靠外层 `CanvasSlot.SetSize()` 压缩；这会让固定坐标卡面重新布局并造成费用、名称、耐久和徽章错位。
- 不根据 Workspace 卡牌高度动态改写 `CardFaceScaleBox`。正式资产统一使用 `0.75` 基础缩放，`ApplyCardLayout()` 只写逻辑尺寸、整数像素位置、角度和 ZOrder，并启用 Widget 像素对齐；当前牌只通过扇形上抬表达，不再额外放大卡面。
- 不用透明的 `CardBody` Brush 承载选中提示。`WorkspaceFeedbackOverlay` 独立覆盖卡面，不参与卡面缩放、布局和命中。
- `M_BackpackWorkspaceCardFeedback` 只作为 `WorkspaceFeedbackOverlay` 的 UI Brush：中性状态该层折叠，当前生产链路只在卡牌被选中时显示青色边缘；它不是卡面流光、hover 或普通携带效果。Presenter 已预留合法/拒绝颜色，但在目标预览状态正式接入 Workspace 刷新前不会触发。
- 携带状态的指针位置由 Workspace 的 carry-only ActiveTimer 从 Slate cursor 持续采样；卡牌移出自身命中范围后仍应跟随。卡牌 PointerMove 只作为同一输入 owner 的事件入口，不是持续跟随的唯一数据源。
- 起手释放保护只属于开始携带的那一次指针手势。若 Slate 因捕获切换没有送达该手势的 PointerUp，下一次左键或右键 PointerDown 必须结束保护，使这次新手势的 PointerUp 立即执行单张或整组释放，不能再被误吞。
- 左键向工作台、同区牌匣、跨区牌匣或销毁确认提交当前牌后，未提交的卡牌必须保持原 Carry 顺序并继续扇形携带。跨区移动/销毁的 Run 广播刷新由 Screen 延后到 Carry 已提交当前牌之后，剩余 Carry 同步更新 storage revision，支持连续逐张操作。
- 销毁确认期间只暂停 Workspace 输入、跟随采样和鼠标捕获，扇形冻结显示；取消恢复原 Carry，确认成功只移除本次确认的牌，只有没有剩余牌时才退出携带。

最小验收：

- `BackpackCardView` 的实际类为 `WBP_BackpackCardView`；其唯一 `CardFaceRetainer` 开启 retained rendering 与 invalidation redraw、关闭 phase redraw、没有 effect material，内部 `CardView` 的实际类为 `WBP_FirstPersonCardView`；wrapper 默认关闭内层 Surface Foil，运行时该 Overlay 为 `Collapsed` 且不再持有材质 Brush。
- `BackpackCardView` 由 `CardFaceScaleBox` 以固定 `0.75` 承载；`360×424` 设计根面对应 `270×318`，可见卡体约为 `222×315`，卡框、费用、名称、卡图、类型和效果徽章保持同一设计坐标系，横向出血不被压入 `220×320` 逻辑命中主体。
- 选中卡牌可见 `WorkspaceFeedbackOverlay`，中性卡牌该层为 `Collapsed`；反馈层不会截获鼠标，也不会改变固定缩放。
- 绑定 `CardBody` 后，拖拽源透明度变化可见。
- 未绑定 `BackpackCardView` 时卡牌仍可转发 Workspace 指针意图，但没有正式卡面显示。
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
| `SurfaceFoilOverlay` | `Image` | 卡面弱流光覆盖层；未绑定时不显示流光；背包静态 Retainer wrapper 会按实例关闭并释放其材质 Brush |
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
- `SurfaceFoilOverlay` 放在卡面内容最上层、`DisabledOverlay` 下方，保持不可命中，不挡拖拽；该条适用于允许流光的通用/第一人称卡面，背包 wrapper 的禁用策略优先。
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
- `Description` section 优先来自卡牌主动效果 explanation blocks；当卡牌没有任何结构化详情 section 且 `UCardDefinition::Description` 非空时，才把 `Description` 作为普通正文回退显示。
- `Passive` section 来自被动触发 explanation block、规则专用 passive outcome block 和可执行被动效果 blocks。
- `FCardPassive::DisplayText`、旧平行 `Description`、`ChangeLines`、`PassiveLines`、`TaskLines`、扁平 `TokenLines` 和旧 token flow 字段都不再进入详情面板。
- 未绑定 `SectionsBox` 时，C++ 兼容路径会创建基础容器。

## WBP_CardDetailSection

父类：`UWacomCardDetailSectionWidget`

推荐资产：`WBP_CardDetailSection`

`WBP_CardDetailPanel` 的 Class Defaults 可以通过 `SectionWidgetClass` 手动指定其它 Section WBP；未指定时使用约定路径或 C++ fallback。

推荐绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `TitleText` | `CommonTextBlock` | 区块标题；建议通过 `UWacomCardDetailTheme.TitleTextStyle` 或 WBP 样式统一字体 |
| `BodyText` | `UWacomCardDetailRichTextBlock` | RichText 正文；C++ 会把 semantic blocks/runs 渲染为 markup、自动注册详情 inline icon decorator 并设置文本 |

WBP 不应做：

- 不提交战斗、背包或 Run 命令。
- 不解析 `UCardDefinition`、`FCardEffect`、`FCardPassive` 或 GameplayTag。
- 不把区块模板写成特定卡牌规则入口；模板归 `UWacomCardExplanationLexicon` 管。

最小验收：

- 该 Widget 作为详情区块通用模板使用。
- 描述和被动都通过同一套 Section 外框显示；正文由 `BodyText` 的 RichText style set 决定字体、颜色、runtime preview 数值语气等视觉。
- `BodyText` 至少要能显示普通文本、`Value`、`ValueBuffed`、`ValueNerfed`、`Status`、`Keyword`、`Muted` 几类 RichText style。目标预览时正文只显示最终数值，强化值用 `ValueBuffed`，削弱值用 `ValueNerfed`，不再显示 `基础值 -> 预览值`。
- `Status` run 会渲染为 inline 状态图标 + `Status` 文本；`Icon` run 会在模板显式使用 `{icon:EffectIcon}` 时渲染为 inline 效果图标。WBP 不需要手动添加 RichText decorator class。
- 正式 WBP 必须绑定 `BodyText`；只有未提供 WBP 根布局的 C++ fallback 路径会创建基础标题和 RichText 正文。

## Card Explanation Assets

详情正文由 C++ 生成 semantic document，再由 RichText 渲染。美术 / 策划后续主要调整这两个 DataAsset：

| 资产类型 | 用途 |
|---|---|
| `UWacomCardExplanationLexicon` | 配置效果、被动触发、规则专用被动结果、tag 显示名和详情内部 named text/template。精确 `EffectType / Passive.Trigger` 优先，找不到时尝试父 tag fallback。 |
| `UWacomCardDetailTheme` | 配置标题 CommonTextStyle、正文 RichText style set、`IconBrushes` / `StatusBrushes` / `FallbackInlineBrush` / `InlineIconRenderOffset`。`BodyText` 会自动使用该 Theme 解析 inline 图标；图标垂直位置优先通过 `InlineIconRenderOffset.Y` 微调。 |

模板 slot v1 支持：

| Slot | 说明 |
|---|---|
| `{value:Magnitude}` | 效果数值；未被目标预览改写时可附带 `MagnitudeSourceTemplates` 来源短语，例如“相当于当前费用 2”；目标预览改写后只显示最终数值，并通过 `ValueBuffed / ValueNerfed` 标记强化或削弱。 |
| `{value:TriggerThreshold}` | 被动触发阈值。 |
| `{icon:EffectIcon}` | 当前效果图标语义；RichText 正文按 `CardDetailTheme.IconBrushes` 显示 inline 图标，缺图且无 fallback 时退回 label 文本。 |
| `{status:EffectStatus}` | 当前效果关联状态，例如中毒、冻结、眩晕；RichText 正文按 `CardDetailTheme.StatusBrushes` 显示 inline 状态图标，并继续保留状态中文名。状态图标和中文名之间由 renderer 保留一个空格。 |
| `{keyword:Tag}` | 当前效果或触发 tag 的关键词显示。 |

默认 Lexicon 中 `Effect.Damage`、`Effect.Heal` 和 `Status.Shield` 模板会显式使用 `{icon:EffectIcon}`，因此详情正文会显示对应 inline 图标；图标具体放在句首、句中或句尾由 DataAsset 模板决定。`Effect.Draw` 默认模板是 `抽 {value:Magnitude} 张牌。`，不会显示抽牌图标，但仍会复用 `MagnitudeSourceTemplates`，例如 RuntimeCost 会显示为“抽 相当于当前费用 2 张牌。”。Discard / ExhaustSelf / GainKeyword / RemoveStatus / ModifyInitiative 等正式效果也应在默认 Lexicon 中有模板；中毒、减速、冻结、暮气等状态仍通过 `{status:EffectStatus}` 显示状态图标和状态名。

自动化 `Wacom.UI.CardDetail.Assets.DefaultContent` 会审计默认 `CardExplanationLexicon` 和 `CardDetailTheme`：检查正式效果 / 被动 / 数值来源 / tag 显示名模板、RichText style row，以及关键 icon/status brush 是否能通过精确配置或 fallback brush 解析。新增正式效果 tag 或详情样式行时需要同步该测试。

`PassiveOutcomeTemplates` 当前用于 `Passive.Trigger.OnCompanionCount` 这类运行时真实存在但不走 `Passive.Effects` 的结果说明。`MagnitudeSourceTemplates` 控制 `Magnitude.Source.RuntimeCost / TargetStatusStacks / HandCount` 的来源短语；`TagDisplayNames` 控制中毒 / 减速 / 左手区等详情正文显示名；`NamedTexts` 控制“描述 / 被动”、条件句、数值修正句和 `Muted` skip 前缀等详情内部文案。

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
