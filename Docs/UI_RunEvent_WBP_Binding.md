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
- `LogRunEventScreenDebugSummary()` 可在 PIE 中排查 active node、候选数量、Zone 映射和最近 drop 结果。

---

## WBP_RunEventChoiceButton

父类：`UWacomRunEventChoiceButton`

推荐绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `ChoiceButton` | `Button` | 接收点击并广播 ChoiceId |
| `LabelText` | `TextBlock` | 显示选项文案 |
| `PaymentStatusText` | `TextBlock` | 显示卡牌支付候选数量或缺失原因 |
| `DisabledReasonText` | `TextBlock` | 显示普通禁用原因 |

WBP 合同：

- `ChoiceButton` 只负责点击；支付选项点击不提交支付，规则层会返回“需要拖入卡牌支付”。
- `PaymentStatusText` 由 C++ fallback 写入：有候选显示 `拖入卡牌支付：{N} 张可用`，无候选显示 `缺少可支付卡牌：{Reason}`。
- `BP_OnRunEventChoiceSnapshotApplied` 会在 C++ 应用新 snapshot 并完成默认文本刷新后触发；WBP 可在这里刷新自定义动画、颜色或状态。
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
- 调用 `LogRunEventScreenDebugSummary()` 应能看到 active node、`RunEvent.Pay.* -> ChoiceId` 映射、候选数量和最近 drop 结果。
