---
type: ui-binding-contract
scope: wacom-ui-battle
status: active
updated: 2026-06-07
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
- 正式 BattleHUD 新制作应使用 `CombatLogFeed + BattleCombatLogBlock`；旧 `EventLogPanel / EventToast` 已删除，不再作为主 HUD 绑定。
- Scene enemy authoring、PartActor debug summary 和 target handle 细节只在 [WacomWorldInteraction.md](./WacomWorldInteraction.md) 维护。

## WBP_BattleHUD

父类：`UBattleHUD`

推荐资产路径：按项目 UI Settings 或 BattleHUD class 配置；完整 WBP 应绑定下列主要槽位。

推荐绑定：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `PlayerStatusBar` | `UPlayerStatusBar` | Optional | 玩家 HP / Shield / San 显示 |
| `ActionPanel` | `UActionPanel` | Optional | Wait / EndTurn 按钮和等待值 |
| `EquipmentBar` | `UEquipmentBar` | Optional | 装备条占位 |
| `DrawPileView` | `UPileCountView` | Optional | 抽牌堆数量 |
| `DiscardPileView` | `UPileCountView` | Optional | 弃牌堆数量；当本回合使用牌堆非空时显示为 `弃牌堆数+本回合使用数`，例如 `2+3` |
| `ExhaustPileView` | `UPileCountView` | Optional | 消耗牌堆数量 |
| `CombatLogFeed` | `UBattleCombatLogFeedWidget` | Optional | 正式常驻玩家战斗记录 |
| `BattlePresentationStack` | `UBattlePresentationStackWidget` | Optional | 已提交卡牌的只读表现 backlog |

WBP 不应做：

- 不绑定或调用旧 `EventLogPanel / EventToast`；这些旧类已删除。
- 不绑定旧 `HandPanel` 或 `CardDetailLayer` 作为 BattleHUD runtime 路径。
- 不直接 Push 击倒弹窗、直接消费 `FBattleEvent`、提交 Battle 规则命令或维护表现队列。
- 不把 `BattlePresentationStack` 做成可点击、可拖拽或规则栈。
- 不绑定敌方 2D fallback widget。正式场景敌人走 `SceneEnemyHostSlots + AWacomBattleEnemyActor` prefab，并由 Host 蓝图/子 Actor 明确声明 PartActor。配置 `EncounterDefinition` 的 Trigger 必须执行 `SyncSceneEnemyHostSlotsFromEncounter()` 并用 `SceneEnemyHostSlots` 覆盖每个 EnemySlotId。
- 不读取或假设 `Snapshot.Enemy`。敌人快照只在 `FBattleSnapshot.Enemies` 中，BattleHUD C++ 会把目标选择、日志和场景 bridge 同步到所有 enemy slot；WBP 不应自行维护第一敌人的兼容显示。

最小 PIE 验收：

- 玩家状态、牌堆数量、ActionPanel 和 CombatLogFeed 在 Snapshot 刷新后显示。
- `CombatLogFeed` 可滚动，连续出牌后能查看最近命令块。
- `BattlePresentationStack` 只显示小卡表现，不响应输入。
- 有 `SceneEnemyHostSlots` 的战斗通过 Host prefab 扫描到的 PartActor Status Badge 阅读敌方状态；缺 Host 时没有 2D 敌方 fallback，且 `EncounterDefinition` 正式入口会被编辑器验证判为 invalid。

当前 `FBattleSnapshot.PileCounts` 额外公开 `PlayedCount`（本回合使用牌堆数量）。本轮 WBP 合同不要求新增 `PlayedPileView`，正式 HUD 仍只绑定并显示抽牌堆、弃牌堆和消耗牌堆三项；`UBattleHUD` 会把 `DiscardCount` 与 `PlayedCount` 合并显示在 `DiscardPileView` 上，`PlayedCount > 0` 时显示为类似 `2+3` 的复合数量。

BattleHUD 战斗手牌由 first-person card layer 提供，不再通过 WBP_BattleHUD 绑定 `UHandPanel`。战斗卡牌详情由 BattleHUD 创建 viewport-level `UWacomCardDetailPanel`，不再需要 BattleHUD WBP 提供 `CardDetailLayer`。详情面板的结构化规则文本与背包共用 `UWacomCardDetailTokenFlowWidget / TokenLineWidget / TokenWidget` 制作合同，BattleHUD 只提供 `FWacomCardDetailViewData`。WBP 应消费 `FWacomCardDetailViewData.Sections`，按 Builder 给出的 section 顺序和标题渲染；`Description`、`TaskLines` 和 flat `TokenLines` 只作为迁移期兼容字段，不应作为分区来源。被动区块标题由 `UWacomCardDetailPanel` 提供，`Passive` token 正文不应再次显示 `被动：`；无法完整结构化的被动应显示 `Passive.DisplayText` 正文，而不是只显示触发条件。手写 `Description` 和 `Passive.DisplayText` 共用 `{Effect.N}` 占位符语法；主动描述中的 N 指向卡牌主动 `Effects`，被动正文中的 N 指向当前 `FCardPassive.Effects`。

## WBP_FPCardView

父类：`UWacomFirstPersonCardViewWidget`

推荐资产路径：`/Game/Wacom/UI/Card/WBP_FPCardView`

配置入口：`BP_WacomPlayerCharacter -> FirstPersonCardAnchorComponent -> FirstPersonCardViewClass`

用途：first-person card layer 的卡面 wrapper，服务静态预览和 BattleHUD runtime battle hand。它组合通用 `UWacomCardView` 与 first-person 专属反馈层，只显示 `FWacomCardViewData` 和交互反馈，不承接点击、hover 命令、目标选择或战斗规则。

推荐结构：

```text
WBP_FPCardView
└─ RootOverlay / Overlay
   ├─ CardView : UWacomCardView
   │  └─ BleedCanvas / SizeBox
   │     └─ RetainerBox
   │        └─ Overlay
   │           └─ CardSizeBox / SizeBox
   │              └─ 原 WBP_CardView 卡面内容
   ├─ FeedbackOverlay : Image
   └─ InteractionFeedbackImage : Image
```

关键绑定 / 命名：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `CardView` | `UWacomCardView` | Optional BindWidget | 通用卡面显示、`FWacomCardViewData` 刷新、主体命中几何来源 |
| `FeedbackOverlay` | `Image` | Optional BindWidget | playable hover / drag target / card target affordance 的 full-card overlay |
| `InteractionFeedbackImage` | `Image` | Optional BindWidget | pressed / confirm / commit / deny 的第一人称源卡交互反馈层；尺寸、层级和默认材质由 WBP 控制 |
| `CardSizeBox` | `SizeBox` | `CardView` 内 Required by convention | 296 x 420 主体显示和交互参考范围 |
| `CostDigitImage` | `Image` | `CardView` 内 Optional | 单位费用数字图标 brush |
| `SurfaceFoilOverlay` | `Widget` | `CardView` 内 Optional | 复用 `UWacomCardView` 弱流光 / 表面装饰；未绑定时不会自动创建覆盖层 |

WBP 合同：

- `WBP_FPCardView` 外层只负责包装和反馈层；通用卡面内容应放在 `CardView` 子控件里。
- `FeedbackOverlay` 和 `InteractionFeedbackImage` 都由 WBP 控制尺寸、锚点和层级；C++ 只写颜色、透明度和材质参数。
- `InteractionFeedbackImage` 优先使用 Anchor 的 `InteractionFeedbackMaterial`；该材质为空时，会复用 WBP Image brush 上预设的材质。推荐制作流程是：常规风格直接把材质放到 `InteractionFeedbackImage` 的 brush 上；需要角色 / 场景级替换时再在 Anchor 上填 override。若没有材质，pressed / confirm / commit 仍可退化为普通 tint，deny 只保留 shake，不退回整卡红色 overlay。
- 交互反馈材质需要支持 C++ 写入参数：`FeedbackColor`、`EdgeWidth`、`EdgeSoftness`、`VignetteStrength`、`VignetteRadius`、`VignetteSoftness`、`Opacity`、`Pulse`。
- 不再支持旧 `DenyFeedbackEdgeImage` fallback；源卡交互反馈统一绑定到 `InteractionFeedbackImage`。
- `CardView` 内部可使用透明 bleed 画布，保证超出主体边界的装饰被 Retainer 完整渲染。
- `SurfaceFoilOverlay` 是显式 opt-in 装饰层；需要卡面流光时由 WBP 自己添加并绑定该 Image。
- `CardView.CardSizeBox` 默认保持 296 x 420，并居中放在 bleed 画布中；缺失时运行时回退旧主体尺寸。
- 透明 bleed 只负责渲染，不扩大 hover、click、drag 起手或 Card target probe 范围。
- 不绑定按钮，不在 WBP 图里实现 hover / pending / disabled 状态机。
- 材质流光和表面装饰继续走内层 `UWacomCardView` 路径；first-person 反馈走 wrapper 绑定控件，不在 slot widget 内新增 Image / 材质刷新逻辑。

最小 PIE 验收：

- Battle 中 first-person hand 使用该卡面，旋转时边缘没有明显黑边、断线或主体裁切。
- 鼠标在主体范围外、bleed 范围内不触发 hover 或拖拽起手。
- 费用图标、卡名、类型、效果徽章和耐久显示仍跟普通 CardView 数据一致。

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
- 不使用 `WBP_FPCardView` 作为小卡类。
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

## Enemy Panel WBP

### WBP_BattleEnemyPanelWidget

父类：`UWacomBattleEnemyPanelWidget`

用途：敌人 Host 头顶的世界空间聚合面板。`AWacomBattleEnemyActor.EnemyPanelWidgetComponent` 承载该 WBP，HUD 只从 `FBattleSnapshot.Enemies` 派发只读 view data，按敌人聚合展示所有部位。

推荐绑定：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `EnemyListBox` | `VerticalBox` | Optional | 每个敌人一组；缺省时 C++ fallback 自动创建 |

推荐类默认值：

| 属性 | 推荐值 | 运行时职责 |
|---|---|---|
| `PartEntryWidgetClass` | `BP_WacomBattleEnemyPartEntryWidget` | 面板内每个部位条目的正式 WBP 类；为空时使用 C++ fallback |

WBP 不应做：不直接读取或修改 `UBattleSession`，不在部位 Actor 上创建常驻状态 UI。

### WBP_BattleEnemyPartEntryWidget

父类：`UWacomBattleEnemyPartEntryWidget`

用途：敌人面板内的通用部位条目，展示单个部位的名称、HP/MaxHP、护盾、先机、意图和状态文本。

推荐绑定：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `PartNameText` | `TextBlock` | Optional | 部位名 |
| `HpText` | `TextBlock` | Optional | HP / MaxHP |
| `ShieldText` | `TextBlock` | Optional | 护盾；无护盾时可为空或隐藏 |
| `InitiativeText` | `TextBlock` | Optional | 当前先机 |
| `StatsText` | `TextBlock` | Optional | HP / 护盾 / 先机的汇总兜底文本；正式 WBP 拆出上面三个字段后可以不放 |
| `IntentText` | `TextBlock` | Optional | 当前意图 |
| `StatusText` | `TextBlock` | Optional | 状态标签汇总 |
| `DestroyedOverlay` | `Widget` | Optional | 部位破坏时的弱化/覆盖层 |

刷新语义：

- Panel 按 `EnemySlotId` 复用敌人组，按 `EnemySlotId + PartSlotId` 复用部位条目；同一部位只更新 view data，不重建条目 Widget。
- `Shield == 0` 时 `ShieldText` 会清空并折叠；如果 WBP 只绑定 `StatsText` 而未绑定 `HpText / ShieldText / InitiativeText`，汇总文本仍会显示。
- `bDestroyed` 时 `DestroyedOverlay` 显示，条目整体透明度降低。
- C++ fallback 使用暗色紧凑面板和水平部位条目：部位名、HP、护盾、先机、意图同排展示，状态和破坏标记作为次级信息显示。
- C++ fallback 自带轻量表现动效：新增条目错峰淡入/轻微下移归位，HP、护盾和破坏状态变化时短促 pulse。正式 WBP 可以用 UMG Animation 覆盖更完整的动效表现。

WBP 不应做：不提交战斗命令，不反向写入 Snapshot，不承担 world target/hover/drag preview 反馈。
## PIE Smoke Checklist

- `WBP_BattleHUD` 能显示玩家状态、ActionPanel、牌堆数量、CombatLogFeed 和 PresentationStack；敌人聚合面板不挂在 HUD Canvas，而挂在 `AWacomBattleEnemyActor` 头顶。
- `WaitButton / EndTurnButton` 可点击并由 HUD 状态控制可用性。
- `WBP_FPCardView` 的 `CardSizeBox` 主体命中范围正确，bleed 画布不扩大交互范围。
- Combat Log 连续追加后可滚动，Presentation Stack 小卡不挡输入。
- 有 `SceneEnemyHostSlots` 的战斗中，每个 `AWacomBattleEnemyActor` 头顶的 EnemyPanel 能按敌人聚合展示所有部位状态；PartActor 只显示 target、drag preview、prediction 等场景反馈，普通部位 hover 使用所属敌人的聚合面板响应。
- `EncounterDefinition` 正式入口必须配置 `SceneEnemyHostSlots`；推荐先执行 `SyncSceneEnemyHostSlotsFromEncounter()` 生成 slots，再逐项填写 Host。缺 Host、漏映射或多余 EnemySlotId 是摆放错误。
- 旧 `WBP_CardWidget / WBP_HandPanel / WBP_EnemyInfoBar / WBP_EnemyPartWidget / EventLogPanel / EventToast / WBP_BattleEnemyPartStatusBadgeWidget` 已删除，不再作为 BattleHUD 制作入口。
