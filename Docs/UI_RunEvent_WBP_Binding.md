---
type: ui-binding-contract
scope: wacom-ui-runevent
status: active
updated: 2026-07-07
tags:
  - wacom/ui
  - wacom/wbp
  - wacom/runevent
  - wacom/contract
---

# RunEvent UI WBP 绑定清单

> [!info] 本文职责
> 本文只记录探索事件界面 WBP 制作合约。事件规则、条件、效果和卡牌支付事务见 [`WacomRun.md`](WacomRun.md)；事件静态数据见 [`WacomData.md`](WacomData.md)；UI 数据流见 [`WacomUI.md`](WacomUI.md)；Run menu zone drop 和 target handle 见 [`WacomWorldInteraction.md`](WacomWorldInteraction.md)。

> [!warning] 合同边界
> WBP 只负责布局、样式和 preview 表现，不直接调用 `URunSession`，不删除卡牌，不推进事件节点，不在图里判断支付卡是否合法。

## WBP_RunEventScreen

父类：`UWacomRunEventScreen`

推荐资产：`WBP_RunEventScreen`

注册方式：

- 顶层 RunEvent WBP 通过 `Edit > Project Settings > Wacom UI Settings` 注册。
- 在 `WidgetClasses` 中添加 `UI.Widget.RunEventScreen`，Class 指向正式 `WBP_RunEventScreen`。
- 未注册、软类加载失败或类型不匹配时，回退 C++ `UWacomRunEventScreen`。

推荐绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `TitleText` | `TextBlock` | C++ 写入当前事件节点标题 |
| `BodyText` | `TextBlock` | C++ 写入当前事件节点正文 |
| `ChoiceList` | `VerticalBox` | C++ 动态填充选项行；不要预放正式选项 |
| `CloseButton` | `Button` | 关闭当前事件界面 |

可选绑定：

| 控件名 | 推荐类型 | 缺省行为 |
|---|---|---|
| `EmptyText` | `TextBlock` | 无选项时显示；不绑则无空状态文案 |

配置项：

| 属性 | 用途 |
|---|---|
| `ChoiceButtonWidgetClass` | 动态选项行 WBP 类；为空时使用 C++ `UWacomRunEventChoiceButton` |
| `PaymentDropTargetWidgetClass` | 卡牌支付选项外层 Zone WBP 类；为空时使用 C++ `UWacomRunMenuDropTargetWidget` |
| `PaymentChoiceMinDesiredWidth` | 支付选项动态包装层最小宽度，单位 Slate Unit |

WBP 不应做：

- 不直接调用 `URunSession` 或 RunEvent choice API。
- 不预放正式选项；`ChoiceList` 由 C++ 刷新事件 snapshot 时按稳定 `ChoiceId` reconcile，WBP 不应缓存动态选项顺序或实例所有权。
- 不手写支付 Zone 的具体事件含义；Screen 只把 choice snapshot 中的 `ZoneId / StableTargetId` 写入 drop target。
- 不覆盖 drop target WBP 的 preview scale、颜色或材质参数；这些由 drop target 自身合同控制。

最小验收：

- 注册正式 `WBP_RunEventScreen` 后，打开探索事件时能显示标题、正文和动态选项。
- 普通选项只创建 `ChoiceButtonWidgetClass`。
- 需要卡牌支付的选项会创建或复用 `PaymentDropTargetWidgetClass`，并把 choice row 包进该 Zone target。
- `LogRunEventScreenDebugSummary()` 可用于 PIE 排查 active node、choice 可用性、Zone 映射、候选数量和最近 drop 结果。

## WBP_RunEventChoiceButton

父类：`UWacomRunEventChoiceButton`

推荐资产：`WBP_RunEventChoiceButton`

推荐绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `ChoiceButton` | `Button` | 接收点击并广播 ChoiceId |
| `LabelText` | `TextBlock` | 显示选项文案 |
| `PaymentStatusText` | `TextBlock` | 显示卡牌支付候选数量或缺失原因 |
| `RequirementList` | `VerticalBox` | 显示普通条件需求列表；可不绑定，由 WBP 自己读取 view 渲染 |
| `ConsequenceList` | `VerticalBox` | 显示提交前后果预览；可不绑定，由 WBP 自己读取 view 渲染 |
| `DisabledReasonText` | `TextBlock` | 显示普通禁用原因 |

Presentation view 数据源：

- `PaymentStatusText / RequirementList / ConsequenceList / DisabledReasonText` 由 C++ fallback 根据 presentation view 写入。
- `RequirementItems` 是 WBP 自定义需求列表的唯一数据源。每项包含 `Text / bSatisfied / DisabledReason / Tone / Kind`。
- `ConsequenceItems` 是 WBP 自定义后果预览的唯一数据源。每项包含 `Text / Tone / Kind / EffectType`。
- `ConsequenceItems` 只表达静态配置意图，不模拟实际提交后的金币 clamp、节点变化或事务失败。
- `BP_OnRunEventChoiceSnapshotApplied` 会在 C++ 应用新 snapshot 并完成默认文本刷新后触发；WBP 可在这里读取 `GetChoiceRequirementView()` 和 `GetChoiceConsequenceView()` 刷新自定义动画、颜色、图标或状态。禁用样式只表达本次 snapshot 的 preview 事实，最终提交仍以 `URunSession` 返回的 `FRunEventChoiceResult` 为准。

WBP 不应做：

- 不在 ChoiceButton WBP 图里直接调用 `ChooseRunEventOptionWithResult` 或 `ChooseRunEventOptionWithPaidCardResult`。
- 不重新判断金币、行动点、压力、持卡、事件状态或 RunFlag；这些事实来自 presentation view。
- 支付选项点击不提交支付；规则层会返回“需要拖入卡牌支付”。
- 支付缺失时不要再额外展示一行普通禁用原因；C++ fallback 会折叠 `DisabledReasonText`，避免重复表达同一问题。

最小验收：

- 普通可选项点击后能经 Screen flow 提交事件选择。
- 支付选项显示候选数量或缺失原因。
- 需求列表和后果预览来自 presentation view，而不是 WBP 自行推导。
- 禁用项能显示普通禁用原因，支付缺失不重复显示普通禁用原因。

## WBP_RunEventPaymentDropTarget

父类：`UWacomRunMenuDropTargetWidget`

推荐资产：`WBP_RunEventPaymentDropTarget`

用途：

- 作为 RunEvent 卡牌支付选项的外层 Zone target。
- 将 first-person menu lease 卡牌拖到该区域时，向 PlayerController 暴露 `FWacomInteractionTargetHandle(TargetKind=Zone)`。

配置项：

| 属性 | 用途 |
|---|---|
| `bEnableRunMenuDropProbe` | 是否允许 first-person menu lease 卡牌命中该 Zone |
| `bEnableFallbackPreview` | 是否启用 C++ tint / scale fallback preview |
| `ProbePreviewScale` | probe / submit-ready 的轻量缩放倍率 |
| `ProbePreviewColor` | 普通 probe 颜色 |
| `InvalidPreviewColor` | 非法目标颜色 |
| `ReleasedProbePreviewColor` | 可提交 / 已提交确认颜色 |

Preview 状态语义：

| 状态 | 含义 |
|---|---|
| `Probe` | 命中 Zone，但不会提交 |
| `SubmitReady` | 松手会由当前 RunEventScreen 提交 |
| `Invalid` | 当前卡、Zone 或规则校验不合法 |
| `Submitted` | 提交成功 |

WBP 不应做：

- 不在 WBP 默认值里硬写具体事件 Zone；`ZoneId` 由 `UWacomRunEventScreen` 从 choice snapshot 写入。
- 不直接删除卡牌、不推进事件，也不读取候选卡列表。
- 不自行判断卡牌是否合法；合法性来自 RunSession validation 和 menu drop intent。

最小验收：

- 支付选项外层能响应 first-person menu lease 卡牌拖入。
- 合法卡拖入时显示 `SubmitReady`，释放成功后显示 `Submitted`。
- 非法卡或错误 Zone 显示 `Invalid`，且不修改 Run 状态。

## PIE Smoke Checklist

- 在 Wacom UI Settings 中注册 `WBP_RunEventScreen`。
- 在 `WBP_RunEventScreen` 默认值里设置 `ChoiceButtonWidgetClass` 和 `PaymentDropTargetWidgetClass`。
- 打开蛇巢事件，普通选项使用自定义 choice row。
- 持有 `PoisonFang` 时，“交出毒牙”选项外层使用自定义 drop target；拖毒牙到该选项时显示 `SubmitReady`，释放后显示 `Submitted`、移除精确实例并显示支付 / outcome Toast。
- 没有 `PoisonFang` 时不显示 first-person 候选卡，支付行显示缺失原因。
- 拖错卡、拖空处或事件关闭后释放，不应提交支付，也不应留下候选卡 preview。
- 绑定 `RequirementList / ConsequenceList` 后，可用 `DA_Event_DebugFlagReward` 检查非支付条件和后果预览来自 presentation view。
- `LogRunEventScreenDebugSummary()` 能辅助排查 active node、choice availability、requirement / consequence preview、Zone 映射、候选数量和最近 drop 结果。

## Authoring Notes

正式 RunEvent WBP 只替换外观、布局和 preview 表现。C++ fallback 是正式可运行基线；正式 WBP 缺失、加载失败或未注册时，RunEvent 支付仍必须可用。

美术接入时建议按 `WBP_RunEventScreen`、`WBP_RunEventChoiceButton`、`WBP_RunEventPaymentDropTarget` 的顺序推进。每个 WBP 都应遵守本文的“不应做”边界，不把 RunEvent 规则、支付事务或候选卡判断写进 Blueprint 图。
