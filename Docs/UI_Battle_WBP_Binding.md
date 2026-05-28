---
type: ui-binding-contract
scope: wacom-ui-battle
status: active
updated: 2026-05-28
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
- `FirstPersonCardAnchorComponent.ProjectionMode` 默认保持 `BodyLocked`；该模式下鼠标移动战斗镜头时，第一人称手牌仍应有空间透视变化，但扇形 layout 不应被拉扯或重新排布。需要对照旧漂移 / 扇形破坏问题时，可临时切到 `LegacyWorldProjected`。
- `FirstPersonCardAnchorComponent.CardLayoutMode` 默认保持 `Authored2D`；该模式下只投影整副手牌中心点，再用 2D 参数排布卡牌。需要对照旧的每卡 3D 槽位投影表现时，可临时切到 `LegacyProjectedFan2D`。
- `FirstPersonCardAnchorComponent.ViewportClampMode` 默认保持 `SoftClampToViewport`；抬头 / 低头时手牌中心可以部分离开屏幕，超过软范围后柔性停在扩展边界附近。临时切到 `HardClampToViewport` 可复现旧的永远屏内感觉；临时切到 `AllowOffscreen` 可验证完全允许离屏的空间感。
- `FirstPersonCardAnchorComponent.bEnableAnchorScreenSmoothing` 默认保持开启；Run Tunnel 中按住 W / S 时，前进下降、后退上升的整体趋势应保留，但快速移动时卡牌中心不应出现明显高频上下抖动。临时关闭该开关可对照旧抖动；调低 `AnchorScreenSmoothingSpeed` 会更稳但更滞后，调高会更跟手但抖动抑制更弱。
- `FirstPersonCardAnchorComponent.bEnableCardSlotMotion` 默认保持开启；这是单张卡牌 slot 的视觉过渡，不是整副手牌中心 smoothing。Hover、pending、出牌、抽牌、Wait / EndTurn 后的手牌重排应平滑过渡；临时关闭该开关可对照旧的硬切表现。`CardSlotMotionSpeed` 越高越跟手，`CardSlotOpacitySpeed` 控制淡入淡出速度，`CardSlotMotionResetDistancePixels` 用于大跳变时直接贴合。
- `FirstPersonCardAnchorComponent.bEnableEventAwareCardTransitions` 默认保持开启；抽牌 / Wait / EndTurn 新增卡应从下方进入，击倒获得毒牙等 `CardGained` 新增卡应从上方 / 战斗空间方向进入，打出的卡应向上离开，手牌上限弃置的卡应向下离开。临时关闭该开关应回到通用 enter / exit 偏移。
- 进入 `TargetSelect` 后，pending 卡应平滑上浮、放大、提高层级并轻微归正；其他 first-person 手牌只轻微降透明，不下沉、不缩小。鼠标移入 pending 卡时不应再额外跳一次，详情面板保持隐藏。
- `FirstPersonCardAnchorComponent.bEnableCardInteractionFeedback` 默认保持开启；Battle Idle 中 hover 可打 first-person 卡应轻微高亮并保持可打姿态，hover 不可打卡仍可查看详情但不应上浮 / 放大 / 提层。按下可打卡有轻微 pressed 感，释放后正常出牌或进入 TargetSelect；点击不可打卡只播放短促红色 deny / 横向 shake，不应出牌或进入 TargetSelect。
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
- WBP 和子控件只调用 `BattleHUD` 的玩家意图入口；出牌、等待、结束回合、目标选择、事件消费、表现队列和击倒弹窗编排由 C++ private flow helper 承担，不在 WBP 图里实现。
- `EventToast` 只显示表现队列送来的单条提示；不要在 WBP 中自行消费 `FBattleEvent` 或直接 Push 击倒弹窗。
- 第一人称战斗手牌由 `BattleHUD::BattleHandPresentationMode` 控制。`LegacyHandPanel` 只使用旧 `UHandPanel`；默认 `FirstPersonHandWithLegacyFallback` 显示并启用 first-person runtime hand，同时保留旧手牌可见作为 fallback 和对照；`FirstPersonHandOnly` 在 runtime hand / anchor 有效时折叠旧手牌，异常、战斗结束或清理 runtime hand 时自动恢复旧手牌。该 layer 不创建 `UCardWidget`，点击只转发到 `BattleHUD->OnCardClickedByUser(CardInstanceId)`，不直接提交 `UBattleSession`。Hover 详情由 BattleHUD 根据最近一次 `FBattleSnapshot.Hand` 和 first-person slot 屏幕锚点显示；旧手牌详情继续使用 `CardDetailLayer`，first-person 详情使用独立 viewport popup host，默认 `FirstPersonCardDetailViewportZOrder=9999`，不依赖旧 `UCardWidget` 几何。
- 敌方目标表现由 `BattleHUD` 私有目标注册表分发 TargetCue；WBP 不直接消费 `FBattleEvent`。当前 `EnemyInfoBar` 只负责在刷新时注册当前 2D 部位 Widget，`EnemyPartWidget` 只播放命中/破坏轻反馈。场景敌人原型可通过 `UWacomBattlePresentationTargetComponent` 注册到同一表；组件可手动写 `PartInstanceId`，也可填写稳定 `PartId` 并由 BattleHUD 的 `bEnableSceneEnemyTargetBindingPrototype` 在 snapshot 刷新和目标选择 UIState 刷新后自动绑定运行时 id。组件 V0 视觉反馈是对 `VisualTargetComponent` 或 Owner 首个 primitive 的短暂 scale pulse；进入 `TargetSelect` 且该部位可选时，同一 primitive 会播放持续 scale 呼吸提示。V0-C 主点击路径是 `AWacomPlayerController` 在左键 Release 且 HUD 处于 `TargetSelect` 时执行 Visibility cursor trace，命中后查找 `UWacomBattlePresentationTargetComponent` 并转发 HUD 目标选择意图；如果 Release 先落到 BattleHUD 根层，HUD 的 `MouseButtonUp` 兜底会复用同一路由。V0-B 的 `ClickTargetComponent` / `Primitive.OnClicked` 绑定仍保留为兼容辅助路径。V0 是同 `PartInstanceId` 后注册者替换旧注册者，不保证 2D/3D 同时播放；开启场景绑定原型时，场景 target 会在 `EnemyInfoBar` 刷新后重新注册，保持替换 2D target 的 V0 语义。

PIE 检查：

- `CardDetailLayer` 覆盖 HUD 可见区域，并位于手牌和敌方部位之上。
- 详情面板不会阻挡手牌、等待、结束回合或敌方部位点击。
- 快速从一张手牌滑到另一张时，详情内容切换到新卡，不应闪关。
- 第一人称战斗手牌：在 BattleHUD / WBP_BattleHUD 上查看 `BattleHandPresentationMode`。默认 `FirstPersonHandWithLegacyFallback` 下，进入战斗后 first-person layer 应显示真实手牌并可 hover/click/detail，旧 `HandPanel` 仍显示并可作为对照入口。切到 `FirstPersonHandOnly` 后，旧 `HandPanel` 应在 first-person runtime hand 有效时隐藏，只看到 first-person hand；hover 卡牌时该 layer 会轻微上移 / 放大，并在卡牌旁显示与旧手牌一致的详情面板，详情面板应位于所有 first-person 卡牌之上。`FirstPersonCardAnchorComponent.ProjectionMode=BodyLocked` 且 `CardLayoutMode=Authored2D` 时，移动鼠标让 camera cursor look 偏转，整副手牌应保留第一人称空间投影变化，但每张卡的尺寸稳定，扇形排布、下坠和左右顺序由 2D 参数保持稳定。`ViewportClampMode=SoftClampToViewport` 时，抬头 / 低头应能看到手牌中心部分离开屏幕，不再被硬拉回视口边缘；超过 soft range 后不会无限飞远，而是柔性停在扩展边界附近。Run Tunnel 中按住 W / S 时，手牌前进下降、后退上升的空间反馈应保留；`bEnableAnchorScreenSmoothing=true` 时快速移动不应出现明显高频上下抖动，关闭后可对照旧表现。`bEnableCardSlotMotion=true` 时，hover 上浮 / 放大、进入 `TargetSelect` 的 pending 聚焦、出牌 / 抽牌 / Wait / EndTurn 后的手牌重排应平滑过渡；已有卡按身份滑到新位置。进入 `TargetSelect` 后，pending 卡应上浮 / 放大 / 提层 / 轻微归正，其他卡轻微降透明；hover pending 卡不应额外上跳，详情保持隐藏，再次点击同卡或取消后所有状态平滑恢复。`bEnableEventAwareCardTransitions=true` 时，抽牌 / Wait / EndTurn 新卡从下方进入，击倒获得毒牙从上方 / 战斗空间方向进入，打出的卡向上离开，手牌上限弃置卡向下离开；重排但仍存在的卡只滑到新位置，不重新播放入场。反复触发击倒获得毒牙或其他即时加手牌事件时，新卡应正常显示，不应出现有空槽位但无卡面的幽灵 Widget；summary 中的 `Active=` 应等于手牌数，`RootChildren=` 不应随触发次数持续增长，`Outgoing=` 不应阶梯式累积，FPS 不应每次获得卡牌都下降一档。关闭 `bEnableEventAwareCardTransitions` 可对照通用 enter / exit；关闭 `bEnableCardSlotMotion` 可对照旧硬切。切到 `HardClampToViewport` 可对照旧的永远屏内表现；切到 `AllowOffscreen` 可让手牌完整离屏。`FirstPersonHandOnly` 下手牌离屏时旧 `HandPanel` 不自动恢复，这是当前表现验证的明确选择。调 `AuthoredCardSpacingPixels / AuthoredMaxHandWidthPixels / AuthoredHandScreenOffset / StaticCardEdgeDropPixels / FanYawDegrees / SoftClampOffscreenAllowancePixels / SoftClampBlendRangePixels / AnchorScreenSmoothingSpeed / CardSlotMotionSpeed / CardSlotOpacitySpeed / PendingTargetingAngleBlend / TargetSelectNonPendingOpacityMultiplier` 应能直接改变排布、离屏手感、移动稳定性、单卡过渡和 TargetSelect 焦点强度。临时切到 `LegacyProjectedFan2D` 可对照旧的每卡 3D 槽位投影表现；临时切到 `LegacyWorldProjected` 可对照旧的 look influence 路径。移出卡牌后详情消失；点击需要敌方目标的卡进入 `TargetSelect` 后详情隐藏，同一张 first-person 卡保持 pending 视觉。点击无目标卡应走现有出牌流程。切到 `LegacyHandPanel` 后，first-person battle hand 不显示/不交互，旧 `HandPanel` 恢复为唯一手牌入口。
- 场景敌方目标原型：在关卡 Actor 上挂 `UWacomBattlePresentationTargetComponent`，填写 `PartId`（例如 `Test.Part.Head` 或正式敌人部位定义的 PartId），并在 BattleHUD/WBP 上开启 `bEnableSceneEnemyTargetBindingPrototype`。Actor 上需要有可缩放/可点击的 `UPrimitiveComponent`，最小结构可以是 `DefaultSceneRoot + Cube(StaticMeshComponent) + WacomBattlePresentationTarget`。`StaticMeshComponent` 继承自 `UPrimitiveComponent`，所以 `Cube` 可以同时作为视觉反馈和点击目标。
- `VisualTargetComponent` / `ClickTargetComponent` 可以保持 `None`；组件会自动使用 Owner 上第一个 `UPrimitiveComponent`。如果 Actor 有多个 mesh 或独立点击盒，再显式指定：例如 `VisualTargetComponent = Cube`、`ClickTargetComponent = BoxCollision`。不要在 Details 面板里创建嵌在 target component 下的 `StaticMeshComponent_0` 临时对象；这会让引用指向错误对象并挡住自动 fallback。
- 进入战斗并刷新 snapshot 后，该组件会自动绑定当前战斗的 `PartInstanceId`；命中 / 破坏 TargetCue 到达时应短暂放大后恢复。选择需要敌方部位目标的卡牌进入 `TargetSelect` 后，匹配且未破坏的场景 target 应持续轻微放大/呼吸；单击组件绑定的 primitive，会由 `AWacomPlayerController` 的 Visibility cursor trace router 转发到 `BattleHUD->OnEnemyPartClickedByUser()`。未匹配的 `PartId` 会保持未注册，已破坏部位不会播放可选提示。
- `Primitive.OnClicked` 兼容路径仍保留，但 V0-C 后不再需要关卡蓝图临时 trace。当前主路由只处理左键 Release，并且只在 `TargetSelect` 中消费，用来覆盖单击有效阶段落在 MouseUp 的 PIE 情况。若仍需要点两次，优先确认 BattleHUD 根层 `MouseButtonUp` 兜底是否编译进当前 PIE；若点击完全没有触发，再检查：BattleHUD 是否开启 `bEnableSceneEnemyTargetBindingPrototype`、组件 `PartId` 是否匹配当前敌人部位定义、目标 primitive 是否阻挡 `Visibility`、以及 `AWacomPlayerController` 是否是当前 PlayerController。Hover 高亮、鼠标命中提示、材质描边和正式敌人表现留后续。
- Debug/Authoring：选中带组件的 Actor，可调用 `GetBattlePresentationTargetDebugSummary()` 或 Details 面板里的 `LogBattlePresentationTargetDebugSummary()`。最小健康状态应为 `LastAutoBind=MatchedPartId`、`Registered=true`、`BlocksVisibility=true`；进入目标选择后还应看到 `TargetAffordance=true`、`Targetable=true`；点击成功后应看到 `LastClick=Forwarded`。如果不是，先看 `LastAutoBind / LastRegistration / LastClick / TargetDisabledReason` 的稳定原因。`ValidateBattlePresentationTargetAuthoring()` 可快速检查是否缺少 `PartId/PartInstanceId`、找不到 Primitive、或 click target 当前没有 Query + Visibility Block。

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
- 它刷新部位列表后把当前 `EnemyPartWidget` 注册到 `BattleHUD` 私有目标注册表；TargetCue 播放时由 HUD 按 `PartInstanceId` 命中对应部位。
- `EnemyInfoBar` 不提交 Battle 命令；点击由部位 Widget 委托回传到 `BattleHUD->OnEnemyPartClickedByUser()`。
- `BuildTargetSelectionView()` 是只读表现契约；WBP 不读取或修改 HUD 内部 pending 状态。

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
