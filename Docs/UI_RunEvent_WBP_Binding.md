---
type: ui-binding-contract
scope: wacom-ui-runevent
status: active
updated: 2026-05-31
tags:
  - wacom/ui
  - wacom/wbp
  - wacom/runevent
  - wacom/contract
---

# RunEvent UI WBP 绑定清单

> [!info] 本文职责
> 本文只记录探索事件界面 WBP 制作合约。事件规则、条件、效果和卡牌支付事务见 [[WacomRun]]，UI 数据流和交互行为见 [[WacomUI]]。

> [!warning] 合同边界
> WBP 只负责布局、样式和 preview 表现，不直接调用 `URunSession`，不删除卡牌，不推进事件节点。

## WBP_RunEventScreen

父类：`UWacomRunEventScreen`

加载口径：

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

WBP 合同：

- `ChoiceList` 是运行时选项 host，C++ 会在刷新事件时清空并重建。
- 普通选项只创建 `ChoiceButtonWidgetClass`。
- 需要卡牌支付的选项会创建 `PaymentDropTargetWidgetClass`，并把 choice row 包进该 Zone target。
- Screen 只设置支付 Zone 的 `ZoneId / StableTargetId`，不覆盖 drop target WBP 的 preview scale、颜色或材质参数。
- `LogRunEventScreenDebugSummary()` 可在 PIE 中排查 active node、choice 可用性摘要、候选数量、Zone 映射和最近 drop 结果。V0-AZ 后 summary 还包含 `Preview=[ChoiceId:Available=...:First=...:Req=总数/未满足数:Pay=候选数:Consequences=数量:Outcome=...]`，方便确认 WBP 绑定看到的状态来自哪条 snapshot。

---

## WBP_RunEventChoiceButton

父类：`UWacomRunEventChoiceButton`

推荐绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `ChoiceButton` | `Button` | 接收点击并广播 ChoiceId |
| `LabelText` | `TextBlock` | 显示选项文案 |
| `PaymentStatusText` | `TextBlock` | 显示卡牌支付候选数量或缺失原因 |
| `RequirementList` | `VerticalBox` | 显示普通条件需求列表；可不绑定，由 WBP 自己读取 view 渲染 |
| `ConsequenceList` | `VerticalBox` | 显示提交前后果预览；可不绑定，由 WBP 自己读取 view 渲染 |
| `DisabledReasonText` | `TextBlock` | 显示普通禁用原因 |

WBP 合同：

- `ChoiceButton` 只负责点击；支付选项点击不提交支付，规则层会返回“需要拖入卡牌支付”。
- `PaymentStatusText / RequirementList / ConsequenceList / DisabledReasonText` 由 C++ fallback 根据 presentation view 写入：有候选显示 `拖入卡牌支付：{N} 张可用`，无候选显示 `缺少可支付卡牌：{Reason}`，普通条件显示需求列表，后果预览显示提交意图，普通非支付阻塞显示 `不可选：{Reason}`。
- `RequirementItems` 是 WBP 自定义需求列表的唯一数据源。每项包含 `Text / bSatisfied / DisabledReason / Tone / Kind`，可用来显示图标、颜色和布局；不要在 WBP 图里重新判断金币、行动点、压力、持卡、事件状态或 RunFlag。
- `ConsequenceItems` 是 WBP 自定义后果预览的唯一数据源。每项包含 `Text / Tone / Kind / EffectType`；它是静态配置意图，不代表已模拟实际提交结果。RunFlag 效果也会作为 `设置标记：{FlagId}` / `清除标记：{FlagId}` 出现在这里。
- 支付缺失时不要再额外展示一行普通禁用原因；C++ fallback 会折叠 `DisabledReasonText`，避免重复表达同一问题。
- `BP_OnRunEventChoiceSnapshotApplied` 会在 C++ 应用新 snapshot 并完成默认文本刷新后触发；WBP 可在这里读取 `GetChoiceRequirementView()` 和 `GetChoiceConsequenceView()`，按 `Tone / PrimaryReason / PaymentCandidateCount / RequirementItems / ConsequenceItems` 刷新自定义动画、颜色、图标或状态。
- 不要在 ChoiceButton WBP 图里直接调用 `ChooseRunEventOptionWithResult` 或 `ChooseRunEventOptionWithPaidCardResult`。

---

## WBP_RunEventPaymentDropTarget

父类：`UWacomRunMenuDropTargetWidget`

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

WBP 合同：

- `ZoneId` 由 `UWacomRunEventScreen` 从 choice snapshot 写入；不要在 WBP 默认值里硬写具体事件 Zone。
- 监听 `BP_OnRunMenuDropPreviewStateChanged` 自定义视觉。状态语义为：
  - `Probe`：命中 Zone，但不会提交。
  - `SubmitReady`：松手会由当前 RunEventScreen 提交。
  - `Invalid`：当前卡、Zone 或规则校验不合法。
  - `Submitted`：提交成功。
- DropTarget WBP 不直接删除卡牌、不推进事件，也不读取候选卡列表。

## PIE 检查

- 在 Wacom UI Settings 中注册 `WBP_RunEventScreen`。
- 在 `WBP_RunEventScreen` 默认值里设置 `ChoiceButtonWidgetClass` 和 `PaymentDropTargetWidgetClass`。
- 打开蛇巢事件，普通选项应使用自定义 choice row。
- 持有 `PoisonFang` 时，“交出毒牙”选项外层应使用自定义 drop target；拖毒牙到该选项时显示 `SubmitReady`，释放后显示 `Submitted`、移除精确实例并显示支付 / outcome Toast。
- 没有 `PoisonFang` 时不显示 first-person 候选卡，支付行显示缺失原因。
- 调用 `LogRunEventScreenDebugSummary()` 应能看到 active node、choice availability / requirement / consequence / preview 摘要、`RunEvent.Pay.* -> ChoiceId` 映射、候选数量和最近 drop 结果。

## 美术接入 TODO

当前 C++ fallback 是正式可运行基线，不要求立刻制作 RunEvent WBP。后续美术接入时按以下顺序推进：

- [ ] 制作 `WBP_RunEventScreen`，只负责事件面板布局、标题、正文、选项列表 host 和关闭按钮外观。
- [ ] 制作 `WBP_RunEventChoiceButton`，只负责选项行外观、支付状态文案、需求列表、后果预览、禁用原因和 hover / pressed / disabled 表现。
- [ ] 制作 `WBP_RunEventPaymentDropTarget`，只负责支付 Zone 的 `Probe / SubmitReady / Invalid / Submitted` 视觉反馈。
- [ ] 在 `WBP_RunEventScreen` 默认值中设置 `ChoiceButtonWidgetClass` 和 `PaymentDropTargetWidgetClass`。
- [ ] 在 Wacom UI Settings 中注册 `UI.Widget.RunEventScreen` 到正式 `WBP_RunEventScreen`。
- [ ] 用蛇巢事件做 PIE 验收：有毒牙、无毒牙、拖错卡、拖空处、支付成功、事件关闭后无残留候选卡。

美术接入边界：

- 不在 WBP 图里调用 `URunSession` 或 RunEvent choice API。
- 不在 WBP 图里手写 `ZoneId`，它由 `UWacomRunEventScreen` 从 snapshot 写入。
- 不在 WBP 图里判断卡牌是否合法；合法性来自 RunSession validation 和 menu drop intent。
- 不改变 C++ fallback。正式 WBP 缺失、加载失败或未注册时，RunEvent 支付仍必须可用。
