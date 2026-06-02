---
type: ui-binding-contract
scope: wacom-ui-battle
status: active
updated: 2026-05-29
tags:
  - wacom/ui
  - wacom/wbp
  - wacom/battle
  - wacom/contract
---

# 战斗 UI WBP 绑定清单

> [!info] 本文职责
> 本文只记录战斗 UI WBP 制作合约。战斗规则见 [[WacomBattle]]，战斗 UI 数据流和交互行为见 [[WacomUI]]。

> [!warning] 合同边界
> 本文不是战斗规则文档；不要在 WBP 合同里定义出牌、目标选择、伤害或战后结算规则。

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
- 卡面弱流光装饰由 `UWacomCardView` 的可选 `SurfaceFoilOverlay` 承接；`WBP_CardWidget` 不需要单独处理材质参数，也不要让覆盖层挡住 `RootButton`。
- 不要对整个 `WBP_CardWidget` 做 hover 位移，否则鼠标命中区域会移动并可能造成下沿抖动。
- 缺 `RootButton` 不崩溃，但无法点击；缺 `FrameBorder` 只影响高亮显示。
- `BP_OnHoverChanged` 和 `BP_OnTargetingHighlightChanged` 可用于 WBP 表现，不要在这些事件里提交规则命令。

PIE 检查：

- hover 后鼠标停在卡牌下沿不抖动。
- 需要目标的卡牌被点击后，当前卡能显示选中态。
- 进入目标选择后，卡牌详情面板会隐藏。

---

## WBP_FirstPersonCardView

父类：`UWacomCardView`

推荐资产路径：`/Game/Wacom/UI/Card/WBP_FirstPersonCardView`

配置入口：`BP_WacomPlayerCharacter -> FirstPersonCardAnchorComponent -> FirstPersonCardViewClass`

用途：

- 第一人称卡牌层的卡面皮肤，服务静态预览和 BattleHUD runtime battle hand。
- 解决 `WBP_CardView` 被 first-person layer 整卡旋转时的采样锯齿、细线断裂和边缘像素破坏。
- 不替代 `WBP_CardWidget`，不承接点击、hover 命令、目标选择或战斗规则。

推荐结构：

```text
WBP_FirstPersonCardView
└─ RetainerBox
   └─ CardContentRoot / Overlay（约 0.99 RenderScale）
      └─ 原 WBP_CardView 卡面内容
```

WBP 合同：

- 继承 `UWacomCardView`，继续只接收 `FWacomCardViewData`。
- 根部建议包一层 `RetainerBox`；RetainerBox 内部卡面内容轻微缩放到约 `0.99`，给旋转采样预留透明边缘。
- 卡面基础尺寸保持 296 x 420；`FirstPersonCardAnchorComponent.StaticCardRenderScale=1.0` 时应接近美术设计尺寸。
- 高对比边框、贴图边缘和细线不要紧贴贴图边界；贴图建议预留透明 Alpha 留白。
- 材质流光和表面装饰继续走 `UWacomCardView` / `SurfaceFoilOverlay` 路径，不在 first-person slot widget 内新增材质刷新逻辑。
- 不创建 `UCardWidget`，不绑定按钮，不提交 `UBattleSession` 命令，不在 WBP 图里实现 hover/pending/disabled 状态机。
- Hover、pending、disabled opacity、ZOrder 和点击意图由 `UWacomFirstPersonCardLayerSlotWidget` 与 `BattleHUD` 管理。

PIE 检查：

- 在 `BP_WacomPlayerCharacter` 的 `FirstPersonCardAnchorComponent` 上把 `FirstPersonCardViewClass` 设置为 `WBP_FirstPersonCardView`。
- 如需使用表现预设，在内容浏览器创建 `WacomFirstPersonCardLayoutPreset` DataAsset，例如 `DefaultReadable`、`WideFan`、`CompactStable`，然后在 `FirstPersonCardAnchorComponent` 上开启 `bUseFirstPersonCardLayoutPreset` 并设置 `FirstPersonCardLayoutPreset`。Preset 只覆盖手牌表现数值；`FirstPersonCardViewClass`、静态预览卡牌、debug、ZOrder 和 `BattleHandPresentationMode` 仍由原组件或 HUD 配置控制。
- 切换不同 preset 后，手牌布局、hover、pending、slot motion、event transition、transition origin 和 interaction feedback 应立即按新 preset 更新；不应从旧配置慢慢漂过去。关闭 `bUseFirstPersonCardLayoutPreset` 后，应恢复组件 Details 面板上的手动调参。
- `FirstPersonCardAnchorComponent.ProjectionMode` 默认保持 `BodyLocked`；该模式下鼠标移动战斗镜头时，第一人称手牌仍应有空间透视变化，但扇形 layout 不应被拉扯或重新排布。需要对照旧漂移 / 扇形破坏问题时，可临时切到 `LegacyWorldProjected`。
- `FirstPersonCardAnchorComponent.CardLayoutMode` 默认保持 `Authored2D`；该模式下只投影整副手牌中心点，再用 2D 参数排布卡牌。需要对照旧的每卡 3D 槽位投影表现时，可临时切到 `LegacyProjectedFan2D`。
- `FirstPersonCardAnchorComponent.ViewportClampMode` 默认保持 `SoftClampToViewport`；抬头 / 低头时手牌中心可以部分离开屏幕，超过软范围后柔性停在扩展边界附近。临时切到 `HardClampToViewport` 可复现旧的永远屏内感觉；临时切到 `AllowOffscreen` 可验证完全允许离屏的空间感。
- `FirstPersonCardAnchorComponent.bEnableAnchorScreenSmoothing` 默认保持开启；Run Tunnel 中按住 W / S 时，前进下降、后退上升的整体趋势应保留，但快速移动时卡牌中心不应出现明显高频上下抖动。临时关闭该开关可对照旧抖动；调低 `AnchorScreenSmoothingSpeed` 会更稳但更滞后，调高会更跟手但抖动抑制更弱。
- `FirstPersonCardAnchorComponent.bEnableCardSlotMotion` 默认保持开启；这是单张卡牌 slot 的视觉过渡，不是整副手牌中心 smoothing。Hover、pending、出牌、抽牌、Wait / EndTurn 后的手牌重排应平滑过渡；临时关闭该开关可对照旧的硬切表现。`CardSlotMotionSpeed` 越高越跟手，`CardSlotOpacitySpeed` 控制淡入淡出速度，`CardSlotMotionResetDistancePixels` 用于大跳变时直接贴合。
- `FirstPersonCardAnchorComponent.bEnableEventAwareCardTransitions` 默认保持开启；`bEnableReadableTransitionOrigins` 默认保持开启。抽牌 / Wait / EndTurn 新增卡应从手牌中心下方进入再展开到目标 slot，击倒获得毒牙等 `CardGained` 新增卡应从手牌中心上方 / 战斗空间方向进入，打出的卡应从当前位置向上离开，手牌上限弃置的卡应向下离开。临时关闭 readable origins 应回到 V0-Q 的相对 slot offset；临时把某个 transition 的 origin mode 切到 `ViewportAnchor`，应能从配置的视口归一化锚点方向进入或离开。临时关闭 event-aware transitions 应回到通用 enter / exit 偏移。
- 进入 `TargetSelect` 后，pending 卡应平滑上浮、放大、提高层级并轻微归正；其他 first-person 手牌只轻微降透明，不下沉、不缩小。鼠标移入 pending 卡时不应再额外跳一次，详情面板保持隐藏。
- `FirstPersonCardAnchorComponent.bEnableCardInteractionFeedback` 默认保持开启；Battle Idle 中 hover 可打 first-person 卡应轻微高亮并保持可打姿态，hover 不可打卡仍可查看详情但不应上浮 / 放大 / 提层。按下可打卡有轻微 pressed 感，释放后正常出牌或进入 TargetSelect；点击不可打卡只播放短促红色 deny / 横向 shake，不应出牌或进入 TargetSelect。
- `FirstPersonCardAnchorComponent.bEnablePlayCommitFeedback` 默认保持开启；点击无目标 first-person 卡时，按下 / 释放仍保持 V0-S pressed / confirm 手感，命令成功提交后被打出的卡应额外播放短 commit pulse，并按 Played transition 离开。点击需要敌方部位目标的卡进入 `TargetSelect` 后，pending 焦点保持 V0-R 表现；点中敌方部位并成功提交后，pending 卡播放 commit pulse / Played exit，目标部位先播放短促 `TargetConfirmed` 确认反馈。这个确认反馈不等同于伤害；如果后续卡牌造成伤害，目标仍应按现有 `DamageDealt / EnemyPartHpEmptied` queue 播放伤害 / 破坏反馈。再次点击 pending 卡取消 TargetSelect 时只取消，不播放 commit 或 target confirm。
- `FirstPersonCardAnchorComponent.bEnableFirstPersonCardDragCommit` 默认保持开启；`bEnableClickToPlayCard=true` 时轻点 first-person 可打卡仍保持快速出牌 / TargetSelect。按住超过 `CardInspectHoldDelaySeconds` 且不移动时，卡牌应平滑移动到 `CardInspectScreenPosition` 附近并按 `CardInspectScale` 放大，同时显示详情；松开只恢复，不应出牌。无目标卡向上拖出超过 `NoTargetCardDragOutCommitDistancePixels` 后进入 commit-ready，释放才提交；未超过阈值释放应取消。需要敌方部位目标的卡拖动后源卡保持选中姿态，C++ aim arrow 从源卡连到鼠标；释放到合法敌方部位后提交并播放 TargetConfirmed，释放到空处或拖到另一张卡时只取消 / deny。本轮 `HandCard` 和 Card target 只验证能被识别，不提交规则。
- `FirstPersonCardAnchorComponent.bAllowCameraLookDuringCardDrag` 默认保持开启；拖拽 first-person 卡牌时仍保留 UMG mouse capture，卡牌 / 箭头应贴近鼠标，且 Battle camera 会继续按拖拽指针轻微偏转，不再像视角被冻结。关闭该开关可对照旧表现。拖拽 World target 只来自当前拖拽指针的场景 trace；`EnemyInfoBar` 不作为 UI drop target。
- `FirstPersonCardAnchorComponent.bLogCardLayerMotionDiagnostics` 默认保持关闭；只有排查幽灵 Widget、outgoing 泄漏或 FPS 阶梯式下降时临时开启。开启后反复获得卡牌或触发移除动画，日志 summary 中的 `Active=` 应等于当前手牌槽位数，`RootChildren=` 不应持续增长，`Outgoing=` 只应在移除 / 投影失败淡出期间短暂出现并回落。
- BattleHUD 默认 `BattleHandPresentationMode=FirstPersonHandWithLegacyFallback`；如需只看第一人称手牌，可切到 `FirstPersonHandOnly`。
- 进入战斗后确认 first-person 卡牌大角度排布下没有明显锯齿、黑边或像素断裂。
- Hover、点击无目标卡、点击目标卡进入 `TargetSelect`、pending 视觉和详情面板仍走现有 first-person layer 流程；详情面板应跟随动画中的 hovered 卡牌视觉位置，而不是跳到目标点。

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
| `CombatLogFeed` | `UBattleCombatLogFeedWidget` | 常驻可滚动玩家战斗记录 |
| `BattlePresentationStack` | `UBattlePresentationStackWidget` | 待播放卡牌表现的小卡堆叠 |
| `CardDetailLayer` | `CanvasPanel` | 承接战斗手牌 hover 详情面板 |

WBP 合同：

- 所有绑定当前都是 `BindWidgetOptional`，缺失不会崩溃；但缺失对应控件会让该区域不显示或不刷新。
- 如果制作完整 BattleHUD WBP，应尽量绑定上表控件，避免只显示局部 UI。
- `CardDetailLayer` 未绑定时，如果 HUD 根控件是 `CanvasPanel`，C++ 会创建 fallback layer。
- 详情面板为 `HitTestInvisible`，不抢点击。
- `CombatLogFeed` 是 BattleHUD 内部常驻滚动记录，不通过 `UWacomGameUIManagerSubsystem::PushContentToLayer()` 打开。V0-CJ/V0-CK 后旧 `EventLogPanel` 抽屉和 `EventToast` 单条提示框都已从 BattleHUD 主路径移除；新的 BattleHUD WBP 不要再绑定 `EventLogPanel / EventToast`，也不要调用 `ToggleBattleEventLog()`。
- `BattlePresentationStack` 是只读表现 backlog，不是规则栈。它只显示已成功提交的 `PlayCard`，最上面是下一张要完成表现并移除的小卡，最新打出的卡压在最下面。V0-CM 后 entry 只显示缩小后的完整卡面，不显示卡名、目标、数量、溢出文字或黑色底座；WBP 也不要再额外加这些文字层。小卡应通过整体缩放承载 `UWacomCardView`，避免卡面内部文字、图标或费用保持原尺寸。Widget 应保持 `HitTestInvisible`，不要把它做成可点击、可拖拽或命令入口。
- WBP 和子控件只调用 `BattleHUD` 的玩家意图入口；出牌、等待、结束回合、目标选择、事件消费、表现队列和击倒弹窗编排由 C++ private flow helper 承担，不在 WBP 图里实现。
- `CombatLogFeed` 应放在右侧偏上区域，承接旧日志框位置；`BattlePresentationStack` 可放在它附近但不要遮挡手牌、敌方部位或目标选择。Combat Log 显示命令块和每条事件 detail line，是没有正式动画时的主要玩家可读反馈。WBP 不要自行消费 `FBattleEvent` 或直接 Push 击倒弹窗。
- 第一人称战斗手牌由 `BattleHUD::BattleHandPresentationMode` 控制。`LegacyHandPanel` 只使用旧 `UHandPanel`；默认 `FirstPersonHandWithLegacyFallback` 显示并启用 first-person runtime hand，同时保留旧手牌可见作为 fallback 和对照；`FirstPersonHandOnly` 在 runtime hand / anchor 有效时折叠旧手牌，异常、战斗结束或清理 runtime hand 时自动恢复旧手牌。该 layer 不创建 `UCardWidget`，点击只转发到 `BattleHUD->OnCardClickedByUser(CardInstanceId)`，不直接提交 `UBattleSession`。Hover 详情由 BattleHUD 根据最近一次 `FBattleSnapshot.Hand` 和 first-person slot 屏幕锚点显示；旧手牌详情继续使用 `CardDetailLayer`，first-person 详情使用独立 viewport popup host，默认 `FirstPersonCardDetailViewportZOrder=9999`，不依赖旧 `UCardWidget` 几何。`bEnableCardDetailReadabilityPolish` 默认开启时，两种战斗详情 host 共用短 hover delay、淡入淡出、轻量 scale、位置平滑跟随和贴边 side hysteresis；关闭后恢复旧的硬切表现。
- 敌方目标表现由 `BattleHUD` 私有目标注册表分发 TargetCue；WBP 不直接消费 `FBattleEvent`。当前 `EnemyInfoBar` 只负责在刷新时注册当前 2D 部位 Widget，`EnemyPartWidget` 只播放命中/破坏轻反馈。V0-CN 后场景敌人推荐使用 `AWacomBattleEnemyActor + AWacomBattleEnemyPartActor`：Host 只负责分组、debug 和制作校验；每个 PartActor 有自己的 Visibility-only `HitBounds`、可见 `PartVisual`、screen-space `PredictionWidget`、screen-space `StatusBadgeWidget`、`UWacomInteractionTargetComponent` 和 `UWacomBattleEnemyPartWorldTargetBridgeComponent`。V0-CR 后每个 `ABattleTriggerActor` 应在 Details 中绑定对应 `SceneEnemyHost`；进入战斗时 `BattleHUD` 只同步当前 Host attached PartActor，非当前 Host 的部位不会收到 hover、prediction、TargetCue、drag preview 或 release 提交。V0-CS 后当前 Host 有效时 `BattleHUD` 默认隐藏 `EnemyInfoBar`，场景 Status Badge 成为主要敌方状态阅读入口；缺 Host 时 `EnemyInfoBar` 继续作为 fallback/debug。V0-CT 后 Host 默认会按 attached PartActor 顺序给 Status / Prediction Badge 加稳定错开；PartActor facade 可调 badge draw size、scale、opacity 和相对位置，Prediction 出现时额外上移，破坏态 Status Badge 降低透明度但不隐藏。PartActor facade 填写 `PartId`，bridge 在 snapshot / UIState 刷新后绑定当前运行时 `PartInstanceId`、注册 TargetCue handler、同步可选目标提示和常驻状态 Badge。V0-CQ/V0-CS/V0-CT 后 bridge debug view / summary 会暴露当前 runtime part 的 HP、护盾、状态、先机、意图、破坏状态、最近一次拖卡 / TargetSelect prediction 输入、hover target 的 stable id / world target id / screen position，以及 `StatusBadgeVisible / StatusBadgeLocation / StatusBadgeScale / CurrentStatusBadgeOpacity / PredictionVisible / PredictionBadgeLocation / PredictionBadgeScale / PredictionBadgeOffsetActive / PredictionMode / PredictedInitiative / PerfectCandidate / ActionRisk / PredictionReject / BadgeStaggerIndex`。组件 V0 视觉反馈是对 PartActor `PartVisual` 的短暂 scale pulse 和持续状态 scale；预测由 PartActor 上方的 `UWacomBattleEnemyPartPredictionWidget` 临时显示，状态由 `UWacomBattleEnemyPartStatusBadgeWidget` 常驻显示。持续状态优先级为 `DragPreview > TargetableAffordance > HoverProbe > BaseScale`，所以拖卡 preview 和 TargetSelect 可选提示不会被普通 hover 覆盖。主点击路径是 `AWacomPlayerController` 在左键 Release 且 HUD 处于 `TargetSelect` 时执行 Visibility cursor trace，命中后查找 `IWacomInteractionTargetProvider` 并构建 `FWacomInteractionTargetHandle`；只有 `TargetKind=World`、`TargetTag=Interaction.Target.Battle.EnemyPart`、运行时 id 有效且来自当前 Host registry 的 handle 会转发到 `BattleHUD->OnEnemyPartClickedByUser()`。如果 Release 先落到 BattleHUD 根层，HUD 的 `MouseButtonUp` 兜底会复用同一路由。
- V0-W 后 TargetCue 分为 `BattleEvent` 和 `TargetConfirmed`。`TargetConfirmed` 只表示目标点击成功提交，2D `EnemyPartWidget` 使用短促确认色，场景 `UWacomBattleEnemyPartWorldTargetBridgeComponent` 使用较轻 scale pulse；`DamageDealt / EnemyPartHpEmptied` 仍由表现队列发送 `BattleEvent` cue，不能在 WBP 中把确认 cue 当成伤害 cue。

PIE 检查：

- `CardDetailLayer` 覆盖 HUD 可见区域，并位于手牌和敌方部位之上。
- 详情面板不会阻挡手牌、等待、结束回合或敌方部位点击。
- 快速从一张手牌滑到另一张时，未停留超过 delay 的卡不应弹详情；详情已显示时内容切换到新卡，不应闪关。贴近屏幕边缘时，详情不应因卡牌轻微移动而在左右两侧反复跳。
- 第一人称战斗手牌：在 BattleHUD / WBP_BattleHUD 上查看 `BattleHandPresentationMode`。默认 `FirstPersonHandWithLegacyFallback` 下，进入战斗后 first-person layer 应显示真实手牌并可 hover/click/detail，旧 `HandPanel` 仍显示并可作为对照入口。切到 `FirstPersonHandOnly` 后，旧 `HandPanel` 应在 first-person runtime hand 有效时隐藏，只看到 first-person hand；hover 卡牌时该 layer 会轻微上移 / 放大，并在卡牌旁显示与旧手牌一致的详情面板，详情面板应位于所有 first-person 卡牌之上。`bEnableCardDetailReadabilityPolish=true` 时，first-person 和旧手牌详情都应短暂停留后淡入、离开后淡出，跟随卡牌移动时不应突兀跳动；关闭该开关可对照旧硬切。若开启 `bUseFirstPersonCardLayoutPreset`，可在 PIE 中切换 `DefaultReadable / WideFan / CompactStable` 等 `WacomFirstPersonCardLayoutPreset`，确认布局、离屏手感、smoothing、slot motion、event transition、transition origin、pending/hover 姿态和 interaction feedback 都按 preset 变化，且 `FirstPersonCardViewClass=WBP_FirstPersonCardView` 不被 preset 改动；关闭 preset 后应恢复组件上的手动参数。`FirstPersonCardAnchorComponent.ProjectionMode=BodyLocked` 且 `CardLayoutMode=Authored2D` 时，移动鼠标让 camera cursor look 偏转，整副手牌应保留第一人称空间投影变化，但每张卡的尺寸稳定，扇形排布、下坠和左右顺序由 2D 参数保持稳定。`ViewportClampMode=SoftClampToViewport` 时，抬头 / 低头应能看到手牌中心部分离开屏幕，不再被硬拉回视口边缘；超过 soft range 后不会无限飞远，而是柔性停在扩展边界附近。Run Tunnel 中按住 W / S 时，手牌前进下降、后退上升的空间反馈应保留；`bEnableAnchorScreenSmoothing=true` 时快速移动不应出现明显高频上下抖动，关闭后可对照旧表现。`bEnableCardSlotMotion=true` 时，hover 上浮 / 放大、进入 `TargetSelect` 的 pending 聚焦、出牌 / 抽牌 / Wait / EndTurn 后的手牌重排应平滑过渡；已有卡按身份滑到新位置。进入 `TargetSelect` 后，pending 卡应上浮 / 放大 / 提层 / 轻微归正，其他卡轻微降透明；hover pending 卡不应额外上跳，详情保持隐藏，再次点击同卡或取消后所有状态平滑恢复。`bEnableEventAwareCardTransitions=true` 且 `bEnableReadableTransitionOrigins=true` 时，抽牌 / Wait / EndTurn 新卡从手牌中心下方进入，击倒获得毒牙从手牌中心上方 / 战斗空间方向进入，打出的卡从当前位置向上离开，手牌上限弃置卡向下离开；重排但仍存在的卡只滑到新位置，不重新播放入场。把 Drawn / Gained / Played / Discarded 的 origin mode 临时切到 `SlotOffset` 应复现 V0-Q 的相对 slot 偏移；切到 `ViewportAnchor` 应能观察从屏幕边缘或指定视口位置进入 / 离开。反复触发击倒获得毒牙或其他即时加手牌事件时，新卡应正常显示，不应出现有空槽位但无卡面的幽灵 Widget；summary 中的 `Active=` 应等于手牌数，`RootChildren=` 不应随触发次数持续增长，`Outgoing=` 不应阶梯式累积，FPS 不应每次获得卡牌都下降一档。关闭 `bEnableEventAwareCardTransitions` 可对照通用 enter / exit；关闭 `bEnableCardSlotMotion` 可对照旧硬切。切到 `HardClampToViewport` 可对照旧的永远屏内表现；切到 `AllowOffscreen` 可让手牌完整离屏。`FirstPersonHandOnly` 下手牌离屏时旧 `HandPanel` 不自动恢复，这是当前表现验证的明确选择。调 `AuthoredCardSpacingPixels / AuthoredMaxHandWidthPixels / AuthoredHandScreenOffset / StaticCardEdgeDropPixels / FanYawDegrees / SoftClampOffscreenAllowancePixels / SoftClampBlendRangePixels / AnchorScreenSmoothingSpeed / CardSlotMotionSpeed / CardSlotOpacitySpeed / PendingTargetingAngleBlend / TargetSelectNonPendingOpacityMultiplier` 应能直接改变排布、离屏手感、移动稳定性、单卡过渡和 TargetSelect 焦点强度；使用 preset 时应优先在 preset 资产里调这些表现数值。临时切到 `LegacyProjectedFan2D` 可对照旧的每卡 3D 槽位投影表现；临时切到 `LegacyWorldProjected` 可对照旧的 look influence 路径。移出卡牌后详情消失；点击需要敌方目标的卡进入 `TargetSelect` 后详情隐藏，同一张 first-person 卡保持 pending 视觉。点击无目标卡应走现有出牌流程。切到 `LegacyHandPanel` 后，first-person battle hand 不显示/不交互，旧 `HandPanel` 恢复为唯一手牌入口。
- 场景敌方目标推荐结构：关卡放置一个 `AWacomBattleEnemyActor` 作为 Host，再把多个 `AWacomBattleEnemyPartActor` 附着到 Host，最后在对应 `ABattleTriggerActor.SceneEnemyHost` 上引用该 Host。每个 PartActor 填写 `PartId`（例如 `Snake.Head / Snake.Body / Snake.Tail` 或测试 `Test.Part.Head`）、`HitBoundsExtent` 和 visual facade；内部会自动配置 `HitBounds` 阻挡 Visibility trace、`PartVisual` 无碰撞、`InteractionTargetTag=Interaction.Target.Battle.EnemyPart`，并让 bridge 写入当前战斗的运行时 `PartInstanceId`。旧的手挂 `DefaultSceneRoot + Cube + WacomInteractionTargetComponent + WacomBattleEnemyPartWorldTargetBridgeComponent` 仍可用于排障，但不再是推荐制作入口，也不会被当前 Host registry 的正常路径绑定。
- 点击需要目标的 first-person 卡并点中场景目标后，应先看到目标确认 pulse，再看到后续伤害 / 破坏 cue。命令失败、未进入 `TargetSelect` 的普通点击、再次点击 pending 卡取消目标选择，都不应播放 target confirm。普通表现队列 busy 时仍可继续出牌；但玩家已请求 Wait / EndTurn 后，pending turn-boundary 会锁住继续出牌和目标选择，直到卡牌表现栈的旧卡完成 exit motion 并真正移除。
- V0-Z / V0-AA drag 验证：轻点 first-person 可打卡仍快速出牌；按住不动超过 hold delay 应进入读牌姿态并显示详情，松开不出牌；无目标卡向上拖出超过阈值后释放提交；需要敌方部位目标的卡拖动时显示箭头，释放到合法敌方部位提交，释放到空处取消 / deny；拖动期间视角应继续轻微跟随拖拽方向，源卡和箭头终点不应因相机运动从鼠标位置拉偏；拖到另一张 first-person 卡时 debug / bridge 能识别 Card target，但本轮不提交。
- V0-AB drag target feedback 验证：无目标卡达到拖出阈值后源卡应出现 commit-ready 反馈；有目标卡拉到合法场景敌方部位时，箭头变确认色并轻微吸附到目标位置，场景 primitive 轻量放大；拉到非法目标或空处时箭头为拒绝色且释放不提交；拉到另一张 first-person 卡时目标卡显示 CardProbe 色但释放不提交；释放到 `EnemyInfoBar` 不作为 UI drop target。
- `VisualTargetComponent` 可以保持 `None`；Bridge 会自动使用 Owner 上第一个 `UPrimitiveComponent`。如果 Actor 有多个 mesh 或独立点击盒，显式指定 `VisualTargetComponent = Cube` 即可；点击命中不再由单独的 `ClickTargetComponent` 字段承担，而是由 PlayerController 的 Visibility trace 命中 Actor 后读取 `IWacomInteractionTargetProvider`。不要在 Details 面板里创建嵌在 Bridge 组件下的 `StaticMeshComponent_0` 临时对象；这会让引用指向错误对象并挡住自动 fallback。
- 进入战斗并刷新 snapshot 后，只有当前 Trigger 的 `SceneEnemyHost` 下的 Bridge 会自动绑定当前战斗的 `PartInstanceId`；命中 / 破坏 TargetCue 到达时应短暂放大后恢复。选择需要敌方部位目标的卡牌进入 `TargetSelect` 后，匹配、未破坏且属于当前 Host registry 的场景 target 应持续轻微放大；单击命中的 primitive，会由 `AWacomPlayerController` 的 Visibility cursor trace router 构建 world handle，并在 HUD registry 过滤通过后转发到 `BattleHUD->OnEnemyPartClickedByUser()`。未匹配的 `PartId` 会保持未注册，已破坏部位不会播放可选提示，非当前 Host 部位会被当作无效场景目标。
- 当前主路由只处理左键 Release，并且只在 `TargetSelect` 中消费。若仍需要点两次，优先确认 BattleHUD 根层 `MouseButtonUp` 兜底是否编译进当前 PIE；若点击完全没有触发，再检查：Trigger 是否绑定正确 `SceneEnemyHost`、PartActor 是否 attached 到该 Host、PartActor `PartId` 是否匹配当前敌人部位定义、内部 `InteractionTarget` 的 tag 是否为 `Interaction.Target.Battle.EnemyPart`、`HitBounds` 是否阻挡 `Visibility`、以及 `AWacomPlayerController` 是否是当前 PlayerController。V0-CQ 后 PlayerAction 中鼠标悬停 PartActor 会有轻量缩放并在部位上方显示当前先机；TargetSelect hover 或拖卡指向当前 Host 部位时显示 Cost 推进预测。V0-CS 后进入战斗并绑定 Host 时，每个当前 Host PartActor 会常驻显示状态 Badge；部位受伤、护盾变化、先机推进、状态变化或破坏后，下一次 snapshot 刷新会同步 Badge。V0-CT 后默认三个蛇部位的 Badge 会按 Host attached 顺序轻微左右/上下错开，Prediction 显示时上移，不应覆盖 Status 主读数；破坏态 Badge 保持常驻但更淡。BattleEnd、PendingKnockdown、pending Wait/EndTurn 或拖起 first-person 卡时普通 hover 应清空；拖卡 preview 自己仍可显示 prediction。材质描边和正式敌人美术留后续。
- Debug/Authoring：选中 PartActor，可调用 `GetBattleSceneEnemyPartDebugSummary()` 或 Details 面板里的 `LogBattleSceneEnemyPartDebugSummary()`。最小健康状态应为 `InteractionConfigured=true`、`LastBind=MatchedPartId`、`Bound=true`、`Registered=true`；进入目标选择后还应看到 `Targetable=true`。V0-CQ/V0-CS/V0-CT 后还应能看到 `RuntimeFacts=true`、当前 `Hp / MaxHp / Shield / Initiative / Intent / StatusText / StatusBadgeVisible / StatusBadgeLocation / StatusBadgeDrawSize / StatusBadgeScale / CurrentStatusBadgeOpacity`，拖卡 preview 或 TargetSelect hover 后能看到 `DragCost / DragSwift / DragCanSubmit / DragReject / PredictionVisible / PredictionMode / PredictedInitiative / PredictionBadgeLocation / PredictionBadgeScale / PredictionBadgeOffsetActive / PerfectCandidate / ActionRisk / PredictionReject`，鼠标悬停后能看到 `HoverActive=true / HoverStableId / HoverWorldTargetId`。选中 Host 可调用 `GetBattleSceneEnemyDebugSummary()` 检查 attached part 数量、`EnemyDefinition`、未知 `PartId`、已绑定部位数、runtime initiative total、`HoveredParts`、`PredictionVisibleParts`、`StatusBadgeVisibleParts`、`BadgeLayoutAppliedParts`、`UsedByBattleHUD` 和当前 HUD 名称。选中 Trigger 可用 `GetBattleTriggerDebugSummary()` 检查 `SceneEnemyHost / SceneEnemyHostParts / SceneEnemyHostDefinitionMatches`。不要直接展开内部 `HitBounds / PartVisual / InteractionTarget / WorldTargetBridge / PredictionWidget / StatusBadgeWidget` 的 Collision Details；制作字段改 Actor facade。

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

## WBP_BattleEnemyPartStatusBadgeWidget

父类：`UWacomBattleEnemyPartStatusBadgeWidget`

用途：挂在 `AWacomBattleEnemyPartActor.StatusBadgeWidgetComponent` 上的 screen-space 常驻状态 Badge。它只读取 `FWacomBattleEnemyPartStatusBadgeView`，不提交 Battle 命令、不参与 hit test。

推荐绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `BadgeBorder` | `Border` | 紧凑状态底板，可按破坏态换色 |
| `PartNameTextBlock` | `TextBlock` | 部位名 |
| `HpBar` | `UWacomProgressBar` | HP / MaxHP |
| `InitiativeTextBlock` | `TextBlock` | 当前先机 |
| `IntentTextBlock` | `TextBlock` | 当前意图或已破坏 |
| `ShieldTextBlock` | `TextBlock` | 护盾，空文本时可折叠 |
| `StatusTextBlock` | `TextBlock` | 状态摘要，空文本时可折叠 |

WBP 合同：

- Widget 应保持 `HitTestInvisible`，不要阻挡 Visibility trace、TargetSelect 点击或 first-person drag preview / release。
- Status Badge 常驻显示当前 Host 已绑定部位，包括破坏部位；破坏态用文案或样式表达，不隐藏。
- Prediction Badge 是独立的临时预测 Widget；不要把预测文案和状态常驻读数合并到同一个 WBP。
- V0-CT 后 C++ fallback 是可读紧凑布局：部位名、HP、先机和意图为主，其中意图独占一行，避免三部位并排时被 HP 条和先机挤出画布；护盾/状态为空时不占高度。WBP 应保持类似信息密度，长中文文案应 clip 或自行缩写，不要撑大 screen-space badge。
- PartActor facade 上的 `StatusBadgeScale / PredictionBadgeScale / StatusBadgeOpacity / DestroyedStatusBadgeOpacity / PredictionBadgeZOffsetWhenVisible` 由 bridge 在运行时应用到 UserWidget；WBP 不应覆盖这些表现参数。

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
- 它刷新部位列表后把当前 `EnemyPartWidget` 注册到 `BattleHUD` 私有目标注册表；TargetCue 播放时由 HUD 按 `PartInstanceId` 命中对应部位。
- `EnemyInfoBar` 不提交 Battle 命令；点击由部位 Widget 委托回传到 `BattleHUD->OnEnemyPartClickedByUser()`。
- `BuildTargetSelectionView()` 是只读表现契约；WBP 不读取或修改 HUD 内部 pending 状态。
- 当前 `BattleTrigger.SceneEnemyHost` 有效时，BattleHUD 默认折叠 `EnemyInfoBar`；该 WBP 主要服务缺 Host 的旧关卡 fallback/debug。

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

## WBP_BattleCombatLogFeed

父类：`UBattleCombatLogFeedWidget`

推荐资产路径：`/Game/Wacom/UI/Battle/WBP_BattleCombatLogFeed`

推荐绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `BlocksScrollBox` | `ScrollBox` | 常驻记录滚动区域 |
| `BlocksBox` | `PanelWidget` | C++ 动态填充命令块 |
| `TitleText` | `TextBlock` | 标题 |

推荐结构：

```text
WBP_BattleCombatLogFeed
└─ Root
   └─ VerticalBox
      ├─ TitleText
      └─ ScrollBox
         └─ BlocksBox
```

配置项：

| 属性 | 用途 |
|---|---|
| `MaxVisibleBlocks` | 常驻滚动记录最多保留的命令块数量 |
| `bAutoScrollToLatest` | 追加命令块后是否滚动到最新 |
| `BlockWidgetClass` | 单个命令块使用的 Widget 类 |

WBP 合同：

- `BlocksBox` 可以是 `VerticalBox`；C++ 只负责动态 AddChild。
- 命令块只显示 `FWacomBattleCombatLogBlockView`，不提交战斗命令。
- 常驻记录区域需要可滚动，避免快速连续出牌后只能看到最近几条。

---

## WBP_BattleCombatLogBlock

父类：`UBattleCombatLogBlockWidget`

推荐资产路径：`/Game/Wacom/UI/Battle/WBP_BattleCombatLogBlock`

推荐绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `HeaderText` | `TextBlock` | 显示命令块标题 |
| `DetailsBox` | `PanelWidget` | C++ 动态填充 detail line |

WBP 合同：

- `SetCombatLogBlockData()` 会保存完整 `FWacomBattleCombatLogBlockView` 并触发 `BP_OnCombatLogBlockUpdated`。
- WBP 可在 `BP_OnCombatLogBlockUpdated` 中读取 `VisualTone / IconKey` 调整样式。
- 命令块只是显示组件，不提交战斗命令。

---

## Legacy Battle Event Log

`UBattleEventLogPanel` 和 `UBattleEventLogEntryWidget` 暂时保留为遗留兼容类，避免旧 WBP 资产断父类。它们不再是 `WBP_BattleHUD` 推荐绑定，也不会被 C++ fallback BattleHUD 创建。新制作应使用 `CombatLogFeed + BattleCombatLogBlock`。

---

## 非正式敌方部位表现

当前 `EnemyInfoBar` 和 `EnemyPartWidget` 是早期 2D fallback/debug 表现，不是最终 HD-2D 敌人实现。V0-CN 后正式场景摆放优先使用 `AWacomBattleEnemyActor + AWacomBattleEnemyPartActor`；V0-CS 后有 `SceneEnemyHost` 的战斗默认用 PartActor Status Badge 读取敌方部位状态，并隐藏 `EnemyInfoBar`。后续 PaperZD / HD-2D 部位表现应扩展 PartActor visual、Status Badge 和 Prediction Widget，而不是把新预测、状态或命中反馈继续堆进 `EnemyInfoBar`。

---

## PIE 检查清单

- `RootButton` 与 `HoverVisualRoot` 是同级，hover 不改变根命中区域。
- `UnifiedHandSlot` 能显示所有手牌，卡牌间距和边距可调。
- 手牌详情显示在悬停卡牌旁边，空间不足时换边，并 clamp 到可见范围。
- `CombatLogFeed` 可滚动，连续快速出牌后能查看本场最近命令块。
- 目标选择时选中卡有可见反馈，敌方可选部位由当前 2D fallback 或未来部位表现承接。
