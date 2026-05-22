# 战斗 UI WBP 绑定清单

本文只记录战斗 UI WBP 制作合约。战斗规则见 `WacomBattle.md`，战斗 UI 数据流和交互行为见 `WacomUI.md`。

---

## WBP_CardWidget

父类：`UCardWidget`

推荐资产路径：`/Game/Wacom/UI/Battle/WBP_CardWidget`

`UHandPanel` 默认尝试加载该路径；资产不存在时回退到 C++ 默认 `UCardWidget`。

推荐绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `RootButton` | `Button` | 覆盖整张手牌，接收点击 |
| `HoverVisualRoot` | `Widget` / `Overlay` | 卡牌视觉根层，hover 只移动这一层 |
| `FrameBorder` | `Border` | 显示可用状态与目标选择高亮 |
| `CardView` | `UWacomCardView` | 通用卡面显示 |
| `ZoneText` | `TextBlock` | 可选分区标签 |

推荐结构：

```text
WBP_CardWidget
└─ Root / Overlay
   ├─ HoverVisualRoot
   │  └─ FrameBorder
   │     └─ CardView / ZoneText
   └─ RootButton
```

WBP 合同：

- `RootButton` 与 `HoverVisualRoot` 推荐为同级；`RootButton` 覆盖卡牌原始占位。
- `CardView` 只负责视觉，不处理点击、出牌、目标选择或战斗命令。
- `FrameBorder / CardView / ZoneText` 推荐放进 `HoverVisualRoot`，保证 hover 时整张卡面一起上浮。
- 不要对整个 `WBP_CardWidget` 做 hover 位移，否则鼠标命中区域会移动并可能造成下沿抖动。
- 缺 `RootButton` 不崩溃，但无法点击；缺 `FrameBorder` 只影响高亮显示。
- `BP_OnHoverChanged` 和 `BP_OnTargetingHighlightChanged` 可用于 WBP 表现，不要在这些事件里提交规则命令。

PIE 检查：

- hover 后鼠标停在卡牌下沿不抖动。
- 需要目标的卡牌被点击后，当前卡能显示选中态。
- 进入目标选择后，卡牌详情面板会隐藏。

---

## WBP_HandPanel

父类：`UHandPanel`

推荐资产路径：`/Game/Wacom/UI/Battle/WBP_HandPanel`

`BattleHUD` 默认尝试加载该路径；资产不存在时回退到 C++ 默认 `UHandPanel`。

推荐绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `UnifiedHandSlot` | `PanelWidget` | C++ 按 `FHandCardVisualEntry.VisualIndex` 填充所有手牌 |

配置项：

| 属性 | 用途 |
|---|---|
| `CardWidgetClass` | 普通手牌使用的 `UCardWidget` 子类 |
| `AnchorCardWidgetClass` | 左右手锚点牌使用的 `UCardWidget` 子类；为空时使用 `CardWidgetClass` |
| `CardSpacing` | 卡牌之间的水平间距 |
| `HandContentPadding` | 整条手牌内容边距 |
| `bCenterCardsWhenNotOverflow` | 未溢出时尝试居中 |
| `CardVerticalAlignment` | 卡牌在手牌带中的垂直对齐 |

WBP 合同：

- `UHandPanel` 只创建和摆放手牌，不直接执行出牌命令。
- `UnifiedHandSlot` 是当前默认视觉入口；推荐使用 `HorizontalBox`。
- 卡牌尺寸继续在 `WBP_CardWidget` 中控制，不要用 `WBP_HandPanel` 缩放卡牌。

---

## WBP_BattleHUD

父类：`UBattleHUD`

推荐绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `PlayerStatusBar` | `UPlayerStatusBar` | 玩家 HP / Shield / San 显示 |
| `HandPanel` | `UHandPanel` | 手牌生成、hover 转发、点击委托 |
| `EnemyInfoBar` | `UEnemyInfoBar` | 当前 2D 敌方部位 fallback 列表 |
| `ActionPanel` | `UActionPanel` | 等待、结束回合和等待值 |
| `EquipmentBar` | `UEquipmentBar` | 装备条占位；当前显示“装备：无” |
| `DrawPileView` | `UPileCountView` | 抽牌堆数量 |
| `DiscardPileView` | `UPileCountView` | 弃牌堆数量 |
| `ExhaustPileView` | `UPileCountView` | 消耗区数量 |
| `EventToast` | `UEventToast` | 战斗内即时事件提示 |
| `CardDetailLayer` | `CanvasPanel` | 承接战斗手牌 hover 详情面板 |
| `EventLogPanel` | `UBattleEventLogPanel` | 战斗事件日志抽屉；不走 CommonUI Layer |

WBP 合同：

- 所有绑定当前都是 `BindWidgetOptional`，缺失不会崩溃；但缺失对应控件会让该区域不显示或不刷新。
- 如果制作完整 BattleHUD WBP，应尽量绑定上表控件，避免只显示局部 UI。
- `CardDetailLayer` 未绑定时，如果 HUD 根控件是 `CanvasPanel`，C++ 会创建 fallback layer。
- 详情面板为 `HitTestInvisible`，不抢点击。
- `EventLogPanel` 是 BattleHUD 内部子组件，不通过 `UWacomGameUIManagerSubsystem::PushContentToLayer()` 打开。
- WBP 中可用自定义按钮调用 `BattleHUD::ToggleBattleEventLog()`。

PIE 检查：

- `CardDetailLayer` 覆盖 HUD 可见区域，并位于手牌和敌方部位之上。
- 详情面板不会阻挡手牌、等待、结束回合或敌方部位点击。
- 快速从一张手牌滑到另一张时，详情内容切换到新卡，不应闪关。

---

## WBP_PlayerStatusBar

父类：`UPlayerStatusBar`

推荐绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `HpBar` | `UWacomProgressBar` | 玩家 HP |
| `ShieldText` | `TextBlock` | 护盾文本，0 时可隐藏 |
| `SanText` | `TextBlock` | San 占位文本 |

---

## WBP_ActionPanel

父类：`UActionPanel`

推荐绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `WaitButton` | `Button` | 点击后请求等待 |
| `EndTurnButton` | `Button` | 点击后请求结束回合 |
| `WaitLabel` | `TextBlock` | 等待按钮文字 |
| `EndTurnLabel` | `TextBlock` | 结束回合按钮文字 |
| `WaitValueText` | `TextBlock` | 当前等待值 |

WBP 合同：

- `WaitButton / EndTurnButton` 是必需绑定；缺失会导致 WBP 构造失败。
- 按钮可用性由 C++ 根据 BattleHUD UIState 更新，WBP 不直接提交 Battle 命令。

---

## WBP_EquipmentBar

父类：`UEquipmentBar`

推荐绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `TitleText` | `TextBlock` | 装备占位标题 |
| `FrameBorder` | `Border` | 装备条底板 |

当前 Snapshot 还没有装备数据，第一版显示“装备：无”。

---

## WBP_PileCountView

父类：`UPileCountView`

推荐绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `LabelText` | `TextBlock` | 抽牌堆 / 弃牌堆 / 消耗区标签 |
| `CountText` | `TextBlock` | 数量 |
| `FrameBorder` | `Border` | 计数块底板 |

`BattleHUD` 会分别把 `DrawPileView / DiscardPileView / ExhaustPileView` 的 Label 和 Count 写入该控件。

---

## WBP_EventToast

父类：`UEventToast`

推荐绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `Container` | `VerticalBox` | 动态显示战斗事件 Toast 文本 |

WBP 合同：

- `UEventToast` 只负责显示队列和过期移除；事件文案来自 `UWacomBattleEventPresentationBuilder`。
- `Container` 未绑定时 C++ fallback 会创建基础容器。

---

## WBP_EnemyInfoBar

父类：`UEnemyInfoBar`

推荐绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `PartsContainer` | `PanelWidget` | C++ 动态填充 `UEnemyPartWidget` |

WBP 合同：

- `EnemyInfoBar` 每次 Snapshot 刷新会重建部位列表。
- 它读取 `BattleHUD::BuildTargetSelectionView()`，再调用每个 `EnemyPartWidget::SetTargetable(bool)`。
- `EnemyInfoBar` 不提交 Battle 命令；点击由部位 Widget 委托回传到 `BattleHUD->OnEnemyPartClickedByUser()`。

---

## WBP_EnemyPartWidget

父类：`UEnemyPartWidget`

推荐绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `RootButton` | `Button` | 点击目标部位 |
| `HpBar` | `UWacomProgressBar` | 部位 HP |
| `NameText` | `TextBlock` | 部位名 |
| `InitiativeText` | `TextBlock` | 当前先机 |
| `IntentText` | `TextBlock` | 当前意图 |
| `ShieldText` | `TextBlock` | 护盾 |
| `StatusText` | `TextBlock` | 状态摘要 |
| `FrameBorder` | `Border` | 破坏 / 可选目标视觉反馈 |

WBP 合同：

- `RootButton / HpBar` 是必需绑定；其余为可选。
- `EnemyPartWidget` 是当前 2D fallback/debug 目标，不是最终 HD-2D / PaperZD 敌人表现。
- WBP 可以响应 `BP_OnTargetableChanged`、`BP_OnDestroyedChanged` 做临时高亮，但不要在这里解析规则或直接调用 `UBattleSession`。

---

## WBP_BattleEventLogPanel

父类：`UBattleEventLogPanel`

推荐资产路径：`/Game/Wacom/UI/Battle/WBP_BattleEventLogPanel`

推荐绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `EntriesBox` | `PanelWidget` | C++ 动态填充日志行 |
| `TitleText` | `TextBlock` | 标题 |
| `CloseButton` | `Button` | 点击后关闭日志抽屉 |

推荐结构：

```text
WBP_BattleEventLogPanel
└─ Root
   └─ VerticalBox
      ├─ Header
      │  ├─ TitleText
      │  └─ CloseButton
      └─ ScrollBox
         └─ EntriesBox
```

配置项：

| 属性 | 用途 |
|---|---|
| `MaxEntries` | 日志面板最多保留的可显示事件数量 |
| `bAutoScrollToLatest` | 追加事件后是否滚动到最新 |
| `EntryWidgetClass` | 单条日志使用的 Widget 类 |

WBP 合同：

- `EntriesBox` 可以是 `VerticalBox`；C++ 只负责动态 AddChild。
- 当前日志行可只显示 `MessageText`；完整 ViewData 仍保存在 Entry Widget 上。
- 单条日志不提交战斗命令。

---

## WBP_BattleEventLogEntry

父类：`UBattleEventLogEntryWidget`

推荐资产路径：`/Game/Wacom/UI/Battle/WBP_BattleEventLogEntry`

推荐绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `MessageText` | `TextBlock` | 显示 `FBattleEventPresentationView.MessageText` |

WBP 合同：

- `SetEventLogEntryData()` 会保存完整 `FBattleEventPresentationView` 并触发 `BP_OnEventLogEntryUpdated`。
- WBP 可在 `BP_OnEventLogEntryUpdated` 中读取 `VisualTone / IconKey` 调整样式。
- 单条日志只是显示组件，不提交战斗命令。

---

## 非正式敌方部位表现

当前 `EnemyInfoBar` 和 `EnemyPartWidget` 是早期 2D fallback/debug 表现，不是最终 HD-2D 敌人实现。它们的当前行为见 `WacomUI.md`，正式 PaperZD / HD-2D 部位表现后续应消费同一份 `FBattleTargetSelectionView`。

---

## PIE 检查清单

- `RootButton` 与 `HoverVisualRoot` 是同级，hover 不改变根命中区域。
- `UnifiedHandSlot` 能显示所有手牌，卡牌间距和边距可调。
- 手牌详情显示在悬停卡牌旁边，空间不足时换边，并 clamp 到可见范围。
- `EventLogPanel` 可打开/关闭，新增战斗事件后能追加日志行。
- 目标选择时选中卡有可见反馈，敌方可选部位由当前 2D fallback 或未来部位表现承接。
