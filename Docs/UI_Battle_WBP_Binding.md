---
type: ui-binding-contract
scope: wacom-ui-battle
status: active
updated: 2026-06-05
tags:
  - wacom/ui
  - wacom/wbp
  - wacom/battle
  - wacom/contract
---

# 战斗 UI WBP 绑定清单

> [!info] 本文职责
> 本文只记录 Battle WBP 制作合约：父类、推荐资产路径、绑定槽位、WBP 不应承担的职责和最小 PIE smoke 检查。

> [!warning] 合同边界
> 本文不定义战斗规则、BattleHUD 数据流、world target 系统或 first-person hand 运行时行为。规则见 [WacomBattle.md](./WacomBattle.md)，Battle UI 行为见 [WacomBattleUI.md](./WacomBattleUI.md)，场景目标见 [WacomWorldInteraction.md](./WacomWorldInteraction.md)，第一人称卡牌层见 [First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md)。

## 制作原则

- WBP 只做显示、布局、动画和轻量表现 hook；玩家命令只回传给 `UBattleHUD` 或对应父类入口。
- WBP 不直接调用 `UBattleSession`，不消费或修改 `BattleState`，不自行解析 `FBattleEvent` 作为规则状态。
- `BindWidgetOptional` 缺失不会崩溃，但对应区域不会显示或刷新；required binding 缺失会导致父类构造失败或控件不可用。
- 正式 BattleHUD 新制作应使用 `CombatLogFeed + BattleCombatLogBlock`；旧 `EventLogPanel / EventToast` 不再作为主 HUD 绑定。
- Scene enemy authoring、PartActor debug summary 和 target handle 细节只在 [WacomWorldInteraction.md](./WacomWorldInteraction.md) 维护。

## WBP_BattleHUD

父类：`UBattleHUD`

推荐资产路径：按项目 UI Settings 或 BattleHUD class 配置；完整 WBP 应绑定下列主要槽位。

推荐绑定：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `PlayerStatusBar` | `UPlayerStatusBar` | Optional | 玩家 HP / Shield / San 显示 |
| `HandPanel` | `UHandPanel` | Optional | legacy 2D hand fallback / 对照 |
| `EnemyInfoBar` | `UEnemyInfoBar` | Optional | legacy 2D enemy fallback / debug |
| `ActionPanel` | `UActionPanel` | Optional | Wait / EndTurn 按钮和等待值 |
| `EquipmentBar` | `UEquipmentBar` | Optional | 装备条占位 |
| `DrawPileView` | `UPileCountView` | Optional | 抽牌堆数量 |
| `DiscardPileView` | `UPileCountView` | Optional | 弃牌堆数量；当本回合使用牌堆非空时显示为 `弃牌堆数+本回合使用数`，例如 `2+3` |
| `ExhaustPileView` | `UPileCountView` | Optional | 消耗牌堆数量 |
| `CombatLogFeed` | `UBattleCombatLogFeedWidget` | Optional | 正式常驻玩家战斗记录 |
| `BattlePresentationStack` | `UBattlePresentationStackWidget` | Optional | 已提交卡牌的只读表现 backlog |
| `CardDetailLayer` | `CanvasPanel` | Optional | legacy 2D hand hover 详情 host |

WBP 不应做：

- 不绑定或调用旧 `EventLogPanel / EventToast` 作为主 HUD 路径。
- 不直接 Push 击倒弹窗、直接消费 `FBattleEvent`、提交 Battle 规则命令或维护表现队列。
- 不把 `BattlePresentationStack` 做成可点击、可拖拽或规则栈。
- 不用 `EnemyInfoBar` 承接新的 HD-2D 场景敌人制作；正式场景敌人走 `SceneEnemyHost + PartActor`。

最小 PIE 验收：

- 玩家状态、牌堆数量、ActionPanel 和 CombatLogFeed 在 Snapshot 刷新后显示。
- `CombatLogFeed` 可滚动，连续出牌后能查看最近命令块。
- `CardDetailLayer` 不阻挡手牌、按钮或敌方目标点击。
- `BattlePresentationStack` 只显示小卡表现，不响应输入。
- 有 `SceneEnemyHost` 的战斗默认不依赖 `EnemyInfoBar` 阅读敌方状态。

当前 `FBattleSnapshot.PileCounts` 额外公开 `PlayedCount`（本回合使用牌堆数量）。本轮 WBP 合同不要求新增 `PlayedPileView`，正式 HUD 仍只绑定并显示抽牌堆、弃牌堆和消耗牌堆三项；`UBattleHUD` 会把 `DiscardCount` 与 `PlayedCount` 合并显示在 `DiscardPileView` 上，`PlayedCount > 0` 时显示为类似 `2+3` 的复合数量。

## WBP_FirstPersonCardView

父类：`UWacomCardView`

推荐资产路径：`/Game/Wacom/UI/Card/WBP_FirstPersonCardView`

配置入口：`BP_WacomPlayerCharacter -> FirstPersonCardAnchorComponent -> FirstPersonCardViewClass`

用途：first-person card layer 的卡面皮肤，服务静态预览和 BattleHUD runtime battle hand。它只显示 `FWacomCardViewData`，不承接点击、hover 命令、目标选择或战斗规则。

推荐结构：

```text
WBP_FirstPersonCardView
└─ BleedCanvas / SizeBox
   └─ RetainerBox
      └─ Overlay
         └─ CardSizeBox / SizeBox
            └─ 原 WBP_CardView 卡面内容
```

关键绑定 / 命名：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `CardSizeBox` | `SizeBox` | Required by convention | 296 x 420 主体显示和交互参考范围 |
| `CostDigitImage` | `Image` | Optional | 单位费用数字图标 brush |
| `SurfaceFoilOverlay` | `Widget` | Optional | 复用 `UWacomCardView` 弱流光 / 表面装饰 |

WBP 合同：

- 外层可使用透明 bleed 画布，保证超出主体边界的装饰被 Retainer 完整渲染。
- `CardSizeBox` 默认保持 296 x 420，并居中放在 bleed 画布中；缺失时运行时回退旧主体尺寸。
- 透明 bleed 只负责渲染，不扩大 hover、click、drag 起手或 Card target probe 范围。
- 不创建 `UCardWidget`，不绑定按钮，不在 WBP 图里实现 hover / pending / disabled 状态机。
- 材质流光和表面装饰继续走 `UWacomCardView` 路径，不在 first-person slot widget 内新增材质刷新逻辑。

最小 PIE 验收：

- Battle 中 first-person hand 使用该卡面，旋转时边缘没有明显黑边、断线或主体裁切。
- 鼠标在主体范围外、bleed 范围内不触发 hover 或拖拽起手。
- 费用图标、卡名、类型、效果徽章和耐久显示仍跟普通 CardView 数据一致。

## Legacy 2D Hand WBP

### WBP_CardWidget

父类：`UCardWidget`

推荐资产路径：`/Game/Wacom/UI/Battle/WBP_CardWidget`

用途：旧 `UHandPanel` 的单卡外壳，只服务 legacy 2D hand fallback / 对照路径。正式 first-person hand 卡面使用 `WBP_FirstPersonCardView`。

推荐绑定：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `RootButton` | `Button` | Optional but needed for click | 覆盖整张手牌，接收点击 |
| `HoverVisualRoot` | `Widget` / `Overlay` | Optional | hover 时移动的视觉根层 |
| `FrameBorder` | `Border` | Optional | 可用状态与目标选择高亮 |
| `CardView` | `UWacomCardView` | Optional | 通用卡面显示 |
| `ZoneText` | `TextBlock` | Optional | 分区标签 |

WBP 不应做：

- 不直接提交 `UBattleSession` 命令。
- 不把整张 root 做 hover 位移，避免命中区域跟着移动。
- 不把它当作新的 first-person hand 交互控件。

最小 PIE 验收：

- legacy 2D hand 中卡牌可 hover，鼠标停在下沿不抖动。
- 需要目标的卡牌进入目标选择后能显示选中态。

### WBP_HandPanel

父类：`UHandPanel`

推荐资产路径：`/Game/Wacom/UI/Battle/WBP_HandPanel`

用途：legacy 2D hand fallback / 对照入口。默认 `FirstPersonHandWithLegacyFallback` 可保留它作为兜底显示；正式手牌主线是 first-person card layer。

推荐绑定：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `UnifiedHandSlot` | `PanelWidget` | Optional | C++ 按 `FHandCardVisualEntry.VisualIndex` 填充手牌 |

WBP 不应做：

- 不直接执行出牌命令。
- 不缩放卡牌尺寸；卡牌尺寸由 `WBP_CardWidget` 控制。
- 不作为新手牌功能的制作主线。

最小 PIE 验收：

- `UnifiedHandSlot` 能显示所有 legacy hand 卡牌。
- `CardWidgetClass / AnchorCardWidgetClass / CardSpacing / HandContentPadding` 能影响 fallback 手加载和布局。

## Combat Log WBP

### WBP_BattleCombatLogFeed

父类：`UBattleCombatLogFeedWidget`

推荐资产路径：`/Game/Wacom/UI/Battle/WBP_BattleCombatLogFeed`

推荐绑定：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `BlocksScrollBox` | `ScrollBox` | Optional | 常驻记录滚动区域 |
| `BlocksBox` | `PanelWidget` | Optional | C++ 动态填充命令块 |
| `TitleText` | `TextBlock` | Optional | 标题 |

配置项：

| 属性 | 用途 |
|---|---|
| `MaxVisibleBlocks` | 最多保留命令块数量 |
| `bAutoScrollToLatest` | 追加后是否滚动到最新 |
| `BlockWidgetClass` | 单个命令块 Widget 类 |

WBP 不应做：

- 不自行消费 raw `FBattleEvent`。
- 不提交战斗命令。
- 不替代 `UWacomBattleCombatLogBuilder` 的 ViewData 构造职责。

最小 PIE 验收：

- 快速连续出牌后仍可滚动查看最近命令块。
- 追加新命令块时可按配置滚动到最新。

### WBP_BattleCombatLogBlock

父类：`UBattleCombatLogBlockWidget`

推荐资产路径：`/Game/Wacom/UI/Battle/WBP_BattleCombatLogBlock`

推荐绑定：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `HeaderText` | `TextBlock` | Optional | 命令块标题 |
| `DetailsBox` | `PanelWidget` | Optional | C++ 动态填充 detail line |

WBP 合同：

- `SetCombatLogBlockData()` 保存完整 `FWacomBattleCombatLogBlockView` 并触发 `BP_OnCombatLogBlockUpdated`。
- WBP 可读取 `VisualTone / IconKey` 调整样式。
- 命令块只是显示组件，不提交战斗命令。

## Presentation Stack WBP

### WBP_BattlePresentationStack

父类：`UBattlePresentationStackWidget`

推荐资产路径：`/Game/Wacom/UI/Battle/WBP_BattlePresentationStack`

推荐绑定：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `StackCanvas` | `CanvasPanel` | Optional | C++ 动态摆放只读小卡 entry |

配置项：

| 属性 | 用途 |
|---|---|
| `MaxVisibleEntries` | 最多显示的小卡数量 |
| `MiniCardViewClass` | 小卡使用的只读 CardView 类 |
| `MiniCardSize` | 小卡宽高，Slate 像素 |
| `EntryOffset` | 小卡错位距离，Slate 像素 |

WBP 不应做：

- 不额外显示卡名、目标、数量或规则解释文案。
- 不使用 `WBP_FirstPersonCardView` 作为小卡类。
- 不把 stack entry 做成可点击、可拖拽或规则命令入口。

最小 PIE 验收：

- 出牌后出现缩小完整卡面。
- 小卡保持 `HitTestInvisible`，不挡手牌、按钮或目标选择。

## Shared Battle Widgets

### WBP_PlayerStatusBar

父类：`UPlayerStatusBar`

推荐绑定：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `HpBar` | `UWacomProgressBar` | Optional | 玩家 HP |
| `ShieldText` | `TextBlock` | Optional | 护盾文本，0 时可隐藏 |
| `SanText` | `TextBlock` | Optional | San 占位文本 |

WBP 不应做：不提交玩家命令，不修改 BattleSession。

### WBP_ActionPanel

父类：`UActionPanel`

推荐绑定：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `WaitButton` | `Button` | Required | 请求等待 |
| `EndTurnButton` | `Button` | Required | 请求结束回合 |
| `WaitLabel` | `TextBlock` | Optional | 等待按钮文字 |
| `EndTurnLabel` | `TextBlock` | Optional | 结束回合按钮文字 |
| `WaitValueText` | `TextBlock` | Optional | 当前等待值 |

WBP 不应做：不直接提交 `FBattleCommand`；按钮可用性由父类 / HUD state 控制。

### WBP_EquipmentBar

父类：`UEquipmentBar`

推荐绑定：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `TitleText` | `TextBlock` | Optional | 装备占位标题 |
| `FrameBorder` | `Border` | Optional | 装备条底板 |

当前 Snapshot 未提供装备数据，装备条只显示占位文案“装备：无”。装备数据接入后，再更新本绑定合同。

### WBP_PileCountView

父类：`UPileCountView`

推荐绑定：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `LabelText` | `TextBlock` | Optional | 抽牌堆 / 弃牌堆 / 消耗牌堆标签 |
| `CountText` | `TextBlock` | Optional | 数量；允许 HUD 写入类似 `2+3` 的复合数量文本 |
| `FrameBorder` | `Border` | Optional | 计数块底板 |

WBP 不应做：不修改牌堆或规则状态。

## Scene Enemy Status WBP

### WBP_BattleEnemyPartStatusBadgeWidget

父类：`UWacomBattleEnemyPartStatusBadgeWidget`

用途：挂在 `AWacomBattleEnemyPartActor.StatusBadgeWidgetComponent` 上的 screen-space 常驻状态 Badge。它只读取 `FWacomBattleEnemyPartStatusBadgeView`。

推荐绑定：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `BadgeBorder` | `Border` | Optional | 紧凑状态底板 |
| `PartNameTextBlock` | `TextBlock` | Optional | 部位名 |
| `HpBar` | `UWacomProgressBar` | Optional | HP / MaxHP |
| `InitiativeTextBlock` | `TextBlock` | Optional | 当前先机 |
| `IntentTextBlock` | `TextBlock` | Optional | 当前意图或已破坏 |
| `ShieldTextBlock` | `TextBlock` | Optional | 护盾 |
| `StatusTextBlock` | `TextBlock` | Optional | 状态摘要 |

WBP 不应做：

- 不阻挡 Visibility trace、TargetSelect 点击或 first-person drag preview / release。
- 不提交 Battle 命令。
- 不把临时 Prediction 文案合并进常驻 Status Badge；Prediction Widget 是独立表现。

最小 PIE 验收：

- Badge 常驻显示当前 Host 已绑定部位，包括破坏部位。
- 长中文文案不会撑大 screen-space badge 到遮挡其他部位。

## Legacy 2D Enemy Fallback WBP

### WBP_EnemyInfoBar

父类：`UEnemyInfoBar`

用途：legacy 2D enemy fallback / debug。正式敌方场景表现优先使用 `SceneEnemyHost + AWacomBattleEnemyActor + AWacomBattleEnemyPartActor`；有 `SceneEnemyHost` 时 BattleHUD 默认隐藏该 fallback。

推荐绑定：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `PartsContainer` | `PanelWidget` | Optional | C++ 动态填充 `UEnemyPartWidget` |

WBP 不应做：

- 不直接提交 Battle 命令。
- 不作为新的 HD-2D 敌人表现入口。
- 不读取或修改 HUD 内部 pending 状态。

### WBP_EnemyPartWidget

父类：`UEnemyPartWidget`

用途：legacy 2D enemy fallback / debug 的单部位 Widget。它显示快照、上报点击和播放 TargetCue 轻反馈，但不是正式场景敌人制作入口。

推荐绑定：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `RootButton` | `Button` | Required | 点击目标部位 |
| `HpBar` | `UWacomProgressBar` | Required | 部位 HP |
| `NameText` | `TextBlock` | Optional | 部位名 |
| `InitiativeText` | `TextBlock` | Optional | 当前先机 |
| `IntentText` | `TextBlock` | Optional | 当前意图 |
| `ShieldText` | `TextBlock` | Optional | 护盾 |
| `StatusText` | `TextBlock` | Optional | 状态摘要 |
| `FrameBorder` | `Border` | Optional | 破坏 / 可选目标视觉反馈 |

WBP 不应做：

- 不解析规则或直接调用 `UBattleSession`。
- 不把 `TargetConfirmed` 当作伤害 cue；伤害和破坏 cue 由表现队列发送。

## Legacy Battle Event Log

`UBattleEventLogPanel`、`UBattleEventLogEntryWidget` 和 `UEventToast` 只作为遗留兼容类保留，避免旧 WBP 资产断父类或手动 PIE 对照入口失效。

新 `WBP_BattleHUD` 不应绑定 `EventLogPanel / EventToast`，也不应调用 `ToggleBattleEventLog()`。正式玩家日志链路是 `UWacomBattleCombatLogBuilder -> UBattleCombatLogFeedWidget -> UBattleCombatLogBlockWidget`。

## PIE Smoke Checklist

- `WBP_BattleHUD` 能显示玩家状态、ActionPanel、牌堆数量、CombatLogFeed 和 PresentationStack。
- `WaitButton / EndTurnButton` 可点击并由 HUD 状态控制可用性。
- `WBP_FirstPersonCardView` 的 `CardSizeBox` 主体命中范围正确，bleed 画布不扩大交互范围。
- Combat Log 连续追加后可滚动，Presentation Stack 小卡不挡输入。
- 有 `SceneEnemyHost` 的战斗中，Status Badge 可读，`EnemyInfoBar` 只作为 fallback/debug。
- legacy `WBP_CardWidget / WBP_HandPanel / WBP_EnemyInfoBar / WBP_EnemyPartWidget` 仍可用于旧 fallback 路径，但不作为新制作主线。
