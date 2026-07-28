---
type: presentation-contract
scope: wacom-ui
status: active
updated: 2026-07-28
tags:
  - wacom/ui
  - wacom/wbp
---

# WacomUI 文档

> [!info] 本文职责
> 本文是 Wacom UI 表现层总入口。它记录 UI 数据流、ownership、测试访问原则和专题导航；CommonUI shell、BattleHUD、first-person hand 等细节放到专题文档。

> [!warning] 阅读边界
> 本文不定义战斗、Run、卡牌或资产规则。规则真相见 [WacomBattle.md](./WacomBattle.md)、[WacomRun.md](./WacomRun.md)、[WacomData.md](./WacomData.md)；内容生成 / 校验见 [WacomDataAuthoring.md](./WacomDataAuthoring.md)，GameplayTag 见 [WacomGameplayTags.md](./WacomGameplayTags.md)。App 编排和输入流程见 [WacomApp.md](./WacomApp.md)，世界交互和 target handle 见 [WacomWorldInteraction.md](./WacomWorldInteraction.md)。

<a id="wacomui-ui-ownership-map"></a>
## §1 UI 总原则

UI 不直接修改战斗或 Run 状态。Widget 读取 Snapshot、ViewData 或 ViewModel，然后把玩家意图提交给 Screen、HUD、PlayerController 或领域 Session 的明确入口。

| 领域 | 数据来源 | 命令出口 |
|---|---|---|
| ExplorationHUD | `UWacomRunViewModelProvider -> UWacomRunViewModel` | 只读显示探索状态和交互提示 |
| Run Map | `URunSession::BuildCurrentFloorMapSnapshot()` 经 App-private ViewData builder | `UWacomRunMapScreen` 只上报 Select / ConfirmTravel / Close；PlayerController-owned Flow 重验并调用唯一 Coordinator MapTravel seam |
| Backpack | `URunSession::BuildBackpackStorageSnapshot()` 与 Run ViewModel 标量 | `UWacomBackpackScreen` 接收 UI 意图；统一 `Workspace` 是卡牌、框选、携带和牌堆移动的唯一输入 owner，备战/特殊/负重以被动内嵌牌堆消费 Scene ViewData；出售目标的预检、原子提交与 Toast 经 Screen 私有 command flow 进入 RunSession，成功后由 Workspace 只播放结果离场；正式 WBP 与纯 C++ fallback 共用同一个 Workspace，不再构建旧 Zone Host、确认流程或 UMG DragDrop owner；卡牌详情生命周期和定位由 App-private detail controller 承接 |
| Shop | `URunSession::BuildCurrentShopSnapshot()` | `UWacomShopScreen` 接收 UI 意图，私有 flow 编排购买、关闭访问和 Toast |
| RunEvent | `URunSession::BuildCurrentRunEventSnapshot()` | `UWacomRunEventScreen` 接收 UI 意图，私有 flow 编排选项提交、支付、关闭和 Toast |
| Battle | `FBattleSnapshot`、`FBattleEvent`、Battle ViewData | `UBattleHUD` 是唯一战斗 UI 命令出口 |

Widget 可以有 C++ fallback 布局，但 C++ 的职责是协议、生命周期和兜底显示；正式视觉由 WBP 承接。复杂流程应收口到 `WacomApp/Private` 的 flow / coordinator helper，Screen 保持 View 所有权和玩家意图入口。

## §2 专题入口

| 专题 | 当前事实入口 | 关注点 |
|---|---|---|
| UI Foundation | [WacomUIFoundation.md](./WacomUIFoundation.md) | CommonUI shell、PrimaryLayout、UI Settings、Widget registry、Modal、MainMenu、Run MVVM、AppToast |
| Battle UI | [WacomBattleUI.md](./WacomBattleUI.md) | BattleHUD、命令出口、HUD state、targeting、presentation flow、Combat Log、当前 Battle UI surface |
| First-person card layer | [First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md) | 第一人称手牌 authoring / runtime contract、Battle/Run source、hover/drag/drop、WBP_FPCardView |
| World interaction | [WacomWorldInteraction.md](./WacomWorldInteraction.md) | Run world target、Battle scene target、Run menu zone target、target handle |
| WBP 绑定 | [UI_Backpack_WBP_Binding.md](./UI_Backpack_WBP_Binding.md)、[UI_Battle_WBP_Binding.md](./UI_Battle_WBP_Binding.md)、[UI_RunEvent_WBP_Binding.md](./UI_RunEvent_WBP_Binding.md) | 父类、资产路径、BindWidget 槽位和 PIE 检查 |

## §3 Ownership Map

Wacom UI 采用“Shell 集中 + Screen coordinator 分域负责”的结构，不新增全局大 UI manager，也不让被动 Widget 直接写规则状态。

| 层级 | Owner | 职责 | 边界 |
|---|---|---|---|
| UI Shell | `UWacomGameUIManagerSubsystem + UWacomPrimaryGameLayout` | 创建 / 重建根布局，按 Game / GameMenu / Modal / Overlay 管理 CommonUI stack | 不解释战斗、Run 或菜单业务规则 |
| 顶层配置 | `UWacomUIDeveloperSettings` | 注册 PrimaryLayout、AppToast 和 `UI.Widget.*` 顶层 WBP 覆盖 | 不作为运行时状态来源 |
| App 流程 | `AWacomGameMode / AWacomMenuGameMode / AWacomPlayerController` | 进入 / 退出战斗，主菜单 travel，探索交互，目标输入路由 | 不把 Screen 内部布局写进流程类 |
| GameMenu 路由 | `FWacomExplorationScreenRouter` | 统一地图、背包、暂停菜单、商店、RunEvent 的打开、关闭、异步 Push 和访问 rollback | 不提交具体 Run 规则效果 |
| 输入上下文 | `UWacomInputContextCoordinatorSubsystem` | 统一 CommonUI input config、鼠标显隐 / capture、Enhanced Input profile 和 click / mouse-over lease | 不读取具体 Widget 业务状态 |
| Screen coordinator | `UBattleHUD / UWacomBackpackScreen / UWacomShopScreen / UWacomRunEventScreen` | 持有 Screen 生命周期、WBP 绑定、ViewData 刷新和玩家意图入口 | 子 Widget 不直接写领域状态 |
| Passive Widget | 卡牌、列表项、Badge、Panel、Button 等 | 显示 ViewData 并通过委托上报玩家意图 | 不直接调用 `UBattleSession` 或 `URunSession` 写 API |

## §4 Public Widget API 与测试口

复杂 Widget 的 public 头只保留 WBP 绑定、运行时生命周期和真实玩家意图入口。不要为了自动化测试在 `WacomApp/Public` 暴露 callable 的 `ForTest`、`ForAutomationTest`、`OverrideForTest`，也不要新增 Blueprint 可见测试 `UFUNCTION`。

测试复杂 Widget 行为时按以下优先级处理：

1. 在 `WacomTests` 中写 tests-only probe subclass，通过真实激活、刷新和玩家意图入口观察结果。
2. 如果运行时代码本身需要可扩展点，提炼 protected production seam；它必须对生产行为有清晰语义。
3. 只有前两者不适合时，才使用 automation-only private friend / test-access；这类入口不能是 `UFUNCTION` 或 Blueprint API。

当前测试访问口径：

| 区域 | 访问方式 |
|---|---|
| BattleHUD | `FWacomBattleHUDTestHarness`、`UWacomBattleHUDDetailTest` 和 BattleHUD automation test view |
| First-person card layer | production 非反射 test view + `FWacomFirstPersonCardLayerTestAccess` |
| Backpack | `FWacomBackpackScreenTestAccess` |
| CardView / EffectBadge | automation test view + `FWacomCardViewTestAccess` |
| Shop / RunEvent | automation test view + `FWacomShopRunEventTestAccess` |
| PlayerController / world probe | `FWacomPlayerControllerRunInteractionTestAccess`、`FWacomBattleSceneTargetClickTestAccess`、Run world / menu drop / choice button probe wrapper |

## §5 Run UI 摘要

Run UI 只显示 RunSession 的当前事实和 presentation view，不直接改 RunState。

| Screen / Widget | 层级 | 当前职责 |
|---|---|---|
| `UWacomExplorationHUD` | Game | 显示探索时段、剩余节点、交互提示；读取 Run ViewModel |
| `UWacomRunMapScreen` | GameMenu | 显示当前 Floor 已知节点/边与同层传送候选；只维护瞬时选择并广播意图，完整生命周期和提交由 `FWacomRunMapScreenFlow` 承接 |
| `UWacomBackpackScreen` | GameMenu | 展示背包、备战区、负重区和 SpecialZone；拖拽 hover preview、drop 提交或按钮意图经 Screen flow 进入 RunSession；Screen 不复制负重区规则，目标合法性由 RunSession 决定 |
| `UWacomShopScreen` | GameMenu | 展示当前商店 snapshot、金币、购买/强化双页签；购买、按 InstanceId 强化和关闭访问经 Screen flow 提交 |
| `UWacomRunEventScreen` | GameMenu | 展示当前事件节点、选项、支付需求和后果预览；选项提交和卡牌支付经 Screen flow 提交 |
| `UWacomRunMenuWidgetBase` | Run GameMenu base | Backpack / Shop / RunEvent 的 Run 专用菜单血统；承载 Run first-person menu lease / drop 合同 |
| `UWacomMenuWidgetBase` | Common GameMenu base | 处理 CommonUI activation 和 Back 请求；仅保留 deprecated Run first-person menu lease / drop Blueprint 兼容桥 |

Run / Backpack / Shop / RunEvent 的规则真相仍在 [WacomRun.md](./WacomRun.md)。WBP 制作槽位见各 Binding 文档。`UWacomBackpackScreen` 只保留 CommonUI 生命周期、WBP/fallback 绑定、Snapshot 刷新、金币文本、CommonInput 提示/帮助协调和玩家意图入口；正式 WBP 与纯 C++ fallback 都只走 `Snapshot -> FWacomBackpackWorkspaceSceneBuilder -> FWacomBackpackWorkspaceVisualRegistry -> UWacomBackpackWorkspaceWidget`。移动 / 删牌 / SpecialZone 入战 toggle 的 Toast 文案由 App-private command presentation helper 承接；卡牌详情由 `UWacomCardPresentationBuilder` 生成通用数据，再由 `FWacomBackpackCardDetailController` 管理 source guard、显示隐藏和 viewport-safe positioning；存放区 snapshot revision / signature dirty gate 由 `FWacomBackpackStorageRefreshGate` 承接。`FWacomBackpackWorkspaceRuntime` 是 Workspace 的单一非反射实现所有权根：Registry 以实际 Canvas 子控件为真相并统一复用 Static、Carry、CarryActive 与 Settlement 的视觉实例，同时持有 Scene 顺序的唯一弱引用卡牌名册；Visual State 持有基础布局、布局过渡、Settlement 与释放交接；Motion Coordinator 统一管理局部姿态、活动卡 DepthMotion 与收落完成；Gesture / Navigation / Presentation Controller 分别拥有屏幕空间手势、稳定虚拟焦点和 Hand Lens/选择冻结/Carry/几何稳定表现状态。Screen 和 Widget 不得再保存平行卡牌数组或同义视觉状态。旧区域 Widget、旧列表 reconciler 和并行 fallback 展示链已删除，不得再建立第二套卡牌所有权。DeckCard 的入战 affordance、投影角标和只读语义来自 `FRunStorageCardView` / Workspace scene，Widget 不按自身父级重新推断规则。

Backpack 的实现进一步收敛为 production-private Runtime Module：`FWacomBackpackWorkspaceRuntimeHost` 是 Gesture、Navigation、Presentation 与 `UWacomBackpackWorkspaceWidget` 之间唯一的 UMG/Slate seam，只提供当前 Geometry、Style、InteractionModel、Registry、Canvas、语义表现操作和原生意图广播，不拥有规则、手势、导航或表现状态。`FWacomBackpackWorkspaceGestureController` 私有拥有卡牌/牌堆/空白 Pointer gesture、屏幕空间阈值、捕获回复和牌堆回滚；`FWacomBackpackWorkspaceNavigationController` 私有拥有键盘/手柄动作解析、稳定虚拟焦点及语义放置目标；`InteractionModel` 仍独立拥有选择、框选、Carry、PileMove、逻辑捕获和 `BuildReleaseIntent()`。`FWacomBackpackWorkspacePresentationController` 私有拥有 Presentation Dirty 合并、固定 Refresh Pipeline、Frame Work 推导、Carry/Hand Lens/Settlement/SaleDeparture 阶段顺序和 automation metrics；`FWacomBackpackWorkspaceFrameScheduler` 只维护 dirty/work/generation/延迟任务合同。`UWacomBackpackWorkspaceWidget` 只作为 Adapter 保留 WBP/fallback 绑定、生命周期、`NativeOn*` 转发与输入回复翻译、`NativePaint`、Screen/Reconciler 调用面和唯一 `RegisterActiveTimer`，不再直接组合交互不变量；Timer 回调校验 generation 后立即转交 Presentation Controller。Host 在 Destruct 后失效，陈旧输入或帧回调不得惰性重建 Runtime。量化口径和当前部分双基线见 [Backpack Presentation Performance Baseline](./BackpackPerformanceBaseline.md)。

### Backpack Workspace 输入与事务流

正式结构是一个填满 GameMenu 的 `UWacomBackpackWorkspaceWidget`。通量区就是自由工作台；备战区、特殊区和非空负重区由 Scene Builder 一次投影为工作台内嵌牌堆，同时只展开一个。旧右侧区域栏与缩略 Preview 运行时链路、类和 WBP 已删除。视觉实例按 `InstanceId + OwnerInstanceId + PhysicalZone + Role` reconcile；Interaction Model 只接收按物理 `InstanceId` 去重后的可移动实体，BattleDeck projection 和 SpecialZone 主卡预览只参与浏览，不能覆盖同 ID 实体的选择来源。负重卡是完整已拥有的物理实体：负重堆展开后进入与备战实体卡相同的点击、Ctrl、框选、Ctrl+A、Carry 和无鼠标操作链。折叠与展开都持有全部真实卡面。

```text
Run Snapshot / revision
  -> BackpackScreen coordinator
  -> Workspace Scene Builder（单份卡牌/牌堆/布局/命中合同）
  -> Visual Registry（跨层复用）-> passive Workspace
  -> selection / carry intent
  -> BackpackCommandFlow
  -> Run atomic batch command
  -> one notification -> reconcile
```

Screen 持有输入租约和纯 interaction model；Workspace Runtime 用互斥的 `Idle / CardPress / Marquee / Carry / PileMove / Suspended` 统一处理单来源框选、拖动阈值、滚轮、按键、标题拖堆和鼠标捕获；DeckCard 只发指针意图，不创建独立 `UDragDropOperation`。Gesture Controller 接管卡牌、牌堆与空白 Workspace 的 Pointer Down/Move/Up，并通过 private-only input reply 让 Adapter 统一映射真实 Slate capture/focus；Navigation Controller 接管键盘/手柄动作和分层取消。`CancelInteraction()` 只转发 Runtime 语义清理，FocusLost 仅解除 Hand Lens 锁定而不取消 Carry。卡牌、牌堆和框选从屏幕坐标起点统一调用 Slate 平台/DPI 拖拽阈值，不使用本地 `5px`。键盘/手柄的方向输入维护 `InstanceId / Zone` 虚拟焦点，Slate 焦点仍在 Workspace 根；Enter/A、Space/X、T/Y、Q/E、LB/RB、Esc/B 与 F1 形成与鼠标同构的拾取、选择、目标释放、切卡、取消和帮助入口。语义释放显式区分 `Pointer / Flux / Pile / Delete`；Flux 清除手工布局，Pile 走现有原子移动事务，Delete 在释放时直接预检并提交原子出售，不进入模态确认。牌堆位置、ZOrder、单一展开项和通量自由布局保存在 GameInstance Workspace State Store：同一 Run 重开保留，新 Run 清空，不进入 SaveGame。备战、特殊与负重牌堆都只允许标题拖动，释放按 16px 网格/边缘吸附并保证标题不重叠；开始拖堆会快照牌框位置与 ZOrder，取消同时恢复牌框和卡牌，成功才提交 Store。负重区暂时清空并隐藏时仍保留其同一 Run 手工位置和层级，再次出现时恢复；新 Run 或“重置牌堆布局”才清除。通量整理避让全部牌堆和固定销毁区。折叠使用固定背包显示尺寸（`296×420 × CardDisplayScale`）、零旋转和 10–24px 水平露出；正式展开牌堆复用同一批 Widget，以 `HandLensStrip` 形成左压缩堆、动态完整区和右压缩堆。空间足够时全部完整展示，空间不足时由鼠标在整条稳定走廊中的连续横向位置驱动三段分配；卡面不滚动、不裁切，也不随 Workspace 高度动态缩放。Hand Lens 只决定布局，浏览详情、Fake3D 与左键拾取统一使用 Workspace 的权威悬浮身份：真实 PointerMove 按当前视觉卡身和 Canvas ZOrder 获取新卡；Frame Scheduler 处理同一缓存指针时，优先由当前卡的视觉卡身、稳定目标卡身和可见条带保持身份。卡牌自身的 Hover 上浮或临时 ZOrder 提升因此不会让压缩堆下层实体卡在静止鼠标下反复抢占；鼠标主动移到其它可见卡仍会重新获取，鼠标也可跟随抬起后的卡身，进入牌堆标题则继续立即清除焦点。PointerDown 复用同一稳定身份，保证详情卡与实际拾取卡一致；只有当前稳定目标不再覆盖缓存指针时，动画帧才会重新解析其它视觉卡。框选与 Ctrl 选择冻结屏幕所见中心、固定卡面尺寸、角度和 ZOrder，直到选择生命周期结束；框选矩形与冻结后的有向卡身矩形只要发生相交（包括边缘接触）就算命中，不再要求卡牌中心进入框内。DeckCard 按 `InstanceId` 缓存 Focus 与 `Rejected > Valid > Selected` 权威语义，Workspace 再以完整兄弟卡牌/Retainer 子树的最大 LayerId 为基线集中绘制角标，并按 `Static < Settlement < CarryCache < CarryActive`、Canvas ZOrder 和子顺序隐藏被更高卡身覆盖的整枚角标，随后绘制活动框选填充和边框；不得依赖各卡互不可比的局部 `NativePaint + 1`，也不得让统一顶层角标穿透上层卡牌。活跃框选指针移动只触发 Paint invalidation。稳定几何或等价 Snapshot 重复提交相同目标时保留进行中的 transition，禁止取消后整组瞬移。收起时每张卡直接过渡到各自的最终折叠槽位，不允许先汇聚到标题中心。Simplified 立即完成且不产生 Hover/焦点/携带空间上抬、角度补偿或视觉弹簧；离开后冻结最后透镜布局直到牌堆收起。多卡携带继续使用独立 `FocusWindowStrip`，默认只完整展示当前滚轮卡一张，其余卡向左右压缩。携带在合法折叠目标停留约 0.35s 后自动展开，失败保持完整携带，不产生部分提交。

负重牌堆始终保留警告外观、命中和虚拟导航目标，但 `bAcceptsExternalCardDrop=false`：来自通量、备战或特殊区的卡指向负重区时，Screen 在调用任何 Run 预检或 mutation 之前统一返回 Rejected，并显示“负重区只接收容量溢出的卡牌”；指针与 Enter/A 都保持完整 Carry、选择和 storage revision，Rejected 目标不触发自动展开。负重来源卡放回原堆只执行同区收拢；移向通量、备战或特殊区继续走现有 Run 原子批量移动及容量/类型校验；移向销毁区继续走 DeleteProvider、批量出售和逐卡材质离场。负重区的入站仍只由 `RecomputeBurden()` 产生。

销毁区始终保留布局、命中和虚拟导航目标。Screen 只消费 `FRunBackpackStorageSnapshot.bDeleteFunctionAvailable` 与 Run 预检结果：没有 DeleteProvider 时复用 Rejected 图标和轮廓并显示“需要删牌能力卡”，指针与 Enter/A 都拒绝提交且完整保留 Carry。主操作预览当前携带卡的单张事务；右键/T/Y 等全量释放仍提交完整 Carry，因此包含最后一张提供者的多卡请求会原子拒绝并显示“最后一张删牌能力卡只能单独出售”。单独出售最后提供者后，该卡正常播放出售离场材质，权威 Snapshot 随即锁定销毁区，仍在 Carry 中的其它卡同帧转为 Rejected。

正式展开牌堆支持左 Shift 临时锁定当前 Hand Lens 布局，作为精确选牌辅助。锁定期间 Workspace 仍更新缓存指针和实际视觉卡身份，只跳过三段布局求解；松键立即按最新指针恢复。该状态只属于 Workspace 当前生命周期，且与现有 Ctrl 多选、Run 状态和 Workspace State Store 无关。

背包表现制作值保存在 `DA_BackpackWorkspaceStyle`，`WBP_BackpackScreen.WorkspaceStyle` 只持有普通资产引用；禁止把 Style 作为 `Instanced/EditInlineNew` UObject 嵌入 Widget Blueprint 根详情。该 Style 资产是受版本控制的人工制作真相，Builder 只能创建缺失资产，不得在日常重建时覆盖或重新保存已有制作值。Style 版本 4 在版本 3 的通用区域 Appearance、可着色图标/九宫格 Brush、投放反馈与响应式详情尺寸之上增加 Focus、Selected、ValidDrop、RejectedDrop 四个 64×64 透明像素图标 Brush；原运动、Hand Lens 与用户微调参数必须原样保留。Focus 与语义状态图标允许同时显示，语义优先级为 `Rejected > Valid > Selected`，灰度下仍须依靠形状区分。区域采用颜色、图标和轮廓三重编码，卡牌透明度不参与区域层级表达。编辑器入口为 `WBP_BackpackScreen → WorkspaceStyle → DA_BackpackWorkspaceStyle`。v4 正式资产已完成集成，历史一次性迁移入口及专用实现已经删除；未来资产变更仍按 MCP 工作流或新的显式授权执行，不得恢复旧迁移路径。普通左键按下未选中的可移动卡也会在当帧完成选择与 `Carry` 迁移；首次松开只消费起手保护，不依赖后续鼠标位移才开始跟随。

Backpack Screen 的 `InteractionHintText` 不复用 `StatusText`：它根据 CommonInput 当前鼠标键盘/手柄类型和 Workspace 模式显示短提示；`ControlsHelpButton` 或 F1 打开被动 `WBP_BackpackControlsHelp`。Widget 树内部按钮和子控件委托属于 Construct/Destruct 生命周期；Run ViewModel Provider、Local Settings、CommonInput 与 PrimaryLayout 层转场四类外部订阅只在 Screen Active 期间存在。Deactivate 先关闭帮助层、取消瞬态交互并隐藏详情，再集中退订；重新 Activate 会读取四个来源的当前状态、通过 RefreshGate 追赶一次权威 Snapshot，并把焦点交回 Workspace 根。帮助层捕获打开前 Slate 焦点，关闭后恢复，且不直接修改 Run 状态。

背包详情使用同一个 `WBP_BackpackCardDetailPanel` 实例：逻辑宽度不低于 1600 时进入右侧固定检查栏并永久预留约 360px；更窄时重挂到全屏 `CardDetailLayer`，按卡牌相反侧定位并夹紧。投放反馈由 Screen 产生 `None / Valid / Rejected / Destructive` ViewData，牌堆和销毁区只被动显示；反馈遮罩永远不参与命中，不得改变 Hand Lens、框选或 Carry 热路径。

正式 Screen 直接 Fill 全局 `1920×1080` DPI 设计空间，不再拥有固定 `1600×900` 子画布或第二套全屏缩放；Header Auto、Workspace Fill、右下 DeleteTarget 覆盖层组成唯一布局合同。背包卡面继续复用原生 `CardRenderSize=296×420` 的 Battle `WBP_FPCardView`，但由 `CardDisplayScale=0.78` 统一缩放到约 `231×328`；卡面、Canvas 布局、命中、框选、Hand Lens 与 Carry 使用同一显示尺寸。该固定背包缩放不消费 Battle/Run first-person `PresentationScale`，也不随 Workspace 高度连续变化。App-private `FWacomBackpackWorkspaceMotionCoordinator` 只把 hover/carry 映射到 Fake3D、视差、接触阴影和实时重绘开关；普通交互最多一张背包卡持续动态重绘，其余卡只在内容或状态变化时请求补绘。Workspace 分为 `PileFrameLayer / StaticCardLayer / SettlementLayer / MarqueeLayer / CarryRoot`；Registry 每次 Scene reconcile 都线性扫描实际 Canvas 并重建 ViewKey、物理 InstanceId 与 ZoneKey 索引，瞬态索引不构成 UObject 所有权。`CarryRoot` 内含静止的 `CarryCache(CarryLayer)` 和独立的 `CarryActiveLayer`：缓存分支保存非当前携带卡，实时分支只保存当前最前卡。携带开始时只重挂载一次，普通鼠标移动只更新外层 `CarryRoot` 的受限视觉锚点；精确逻辑指针立即进入目标判定，视觉锚点仅做最大落后受限的轻量收敛，不移动缓存节点、不重算牌列、不全量刷新静态卡，也不触发 Snapshot/Scene reconcile。成功跨区释放时，Workspace 先建立 pending visual handoff；即使目标 ViewKey 的 `PhysicalZone` 改变或目标 Snapshot 晚一帧，卡牌仍留在携带视觉层，Registry 按 `InstanceId` 迁移同一受保护实体 Widget，再由目标 Scene 直接交接到目标静态布局。禁止销毁 A 实例、创建 B 实例、在等待窗口恢复 A，或生成 A→B 过渡。部分释放只交接已提交项。出售则先成功提交 Run 原子事务，再把每张原 Widget 通过 Slate 保活重挂载移入 `SettlementLayer` 并从 Registry、命中和导航中排除；进入离场控制器时先同步清除 Selected/Current 反馈层和 Focus/Drop/Selected 原生角标，再开始卡面材质。`SaleDepartureController` 对每个新出售批次生成随机身份顺序，但不再一次填充四张窗口：首次只启动一张，只有该卡的材质与首帧 ready 并真正开始播放后才启动下一次随机倒计时；倒计时归零时每帧最多再启动一张，因此前卡继续消散、后卡逐张错峰叠入。Full Motion 使用原 Style 的 `0.40s` 播放与 `0.05s` hold，相邻起播间隔按 InstanceId/批次派生为 `0.09–0.12s`；Simplified Motion 为每张 `0.12s`、间隔 `0.03–0.04s`。同时播放仍限制为最多四张，慢帧丢弃多余倒计时而不追赶批量起播；空闲序列的第一张请求开始音效。出售序列运行时暂停普通 Hover/Carry 实时深度材质，保证总实时 Retainer 不超过四个；材质/首帧 readiness 未就绪时后续发射与该卡进度一起冻结，`0.75s` 失败只安全移除表现，不回滚已完成规则事务。新出售批次可继续提交并排队；Deactivate/Destruct/Reset 强制清理且重新打开不重播。携带最前卡的中性深度表现与指针位置无关，鼠标热路径禁止重复写 Retainer phase、材质参数或 `RequestRender`。离散表现变化通过带原因的定向 Refresh Pipeline 合并，固定按导航目标、Carry 父层、Carry Strip、指定卡静态/语义、Motion 目标、焦点、Accessibility 与 Paint 顺序应用；局部请求按 `InstanceId` 取并集，任一全卡请求覆盖局部集合。框选移动和 Carry/PileMove 指针热路径不进入全卡刷新。`FWacomBackpackWorkspaceFrameScheduler` 统一承接待处理刷新、几何稳定、Carry/PileMove 追踪、Hover 延迟、基础布局、局部 Motion、Settlement、出售离场、焦点退出、牌堆收起和延迟卡面补绘，Workspace 源码只注册一个按需帧 `ActiveTimer`；无待办时自动停止，Simplified Motion 同帧落到最终姿态。牌堆收拢以真实过渡完成为结束事件，不使用等时长回调。首次显示时 Workspace 等待 Slate 几何连续稳定，再以同一 Snapshot 重排；稳定前隐藏统一 Canvas，最终布局与交互表现落定后最早在下一次 Slate 调度帧补绘静态卡面。Retainer 或出售材质在几何未稳定、卡层不可见或缓存暂停时保留请求但不空转，条件恢复后再唤醒同一个 Scheduler。GameMenu CommonUI 过渡期间，Screen 根据 PrimaryLayout 层过渡事件暂停卡面 Retainer 缓存并直绘，结束后恢复和补绘，避免把淡入 Alpha 烘入静态缓存。背包采用分层 Back：Escape 依次取消瞬态交互、收起展开牌堆，再交给 CommonUI 关闭；B 始终直接关闭并统一清理。出售失败保持完整 Carry，出售成功立即更新金币、Storage revision 与对应 Carry 项并显示结果 Toast，不提供 Undo。

`UWacomShopScreen` 保留 Screen 生命周期、WBP 绑定、cached shop snapshot、商品/强化行创建和玩家意图入口；shop snapshot revision 的购买与强化 signature dirty gate 由 `FWacomShopRefreshGate` 分别承接。商品行继续由 `FWacomShopOfferRowListReconciler` 按 Offer identity reconcile；强化行由 `FWacomShopUpgradeRowListReconciler` 按物理 `InstanceId` reconcile，重复 Definition 不合并。`FWacomShopUpgradePresentationBuilder` 只把权威 Quote 编译为当前/下一 CardView、价格、禁用原因和字段差异，不自行判定规则；下一卡继续使用通用 rarity border，并以 `EWacomCardViewValueEmphasis` 标记相对当前卡变化的数字，Badge 只做静态被动叠色，不启动 Battle 数值改写材质。Screen flow 提交 stale Definition guard 并把 Result 映射为 Toast；服务关闭时隐藏强化页。正式 WBP 只提供同名绑定和布局，不引用 Debug 内容或复制事务。

`UWacomRunEventScreen` 保留 Screen 生命周期、WBP 绑定、cached choices、支付 Zone 映射和玩家意图入口；cached choices 与 Zone 映射的查询、支付 Zone -> Choice 解析和 debug 摘要由 App-private `FWacomRunEventPresentationStateView / Edit` 承接。Cached choices 只作为展示、复用、支付 Zone 映射和调试输入，不作为普通选项提交的最终可用性判断；点击选项必须调用 `URunSession::ChooseRunEventOptionWithResult()` 并以 `FRunEventChoiceResult` 为准。选项行和支付 DropTarget 的 identity reconcile、排序、复用和移除由 `FWacomRunEventChoiceListReconciler` 承接，稳定键为 `ChoiceId`，避免刷新事件 snapshot 时重建动态 WBP 实例或丢失支付 Zone 表现状态。卡牌支付 drop 的 resolve / submit 由 `FWacomRunEventPaymentDropFlow` 承接；Screen 只提供当前 RunSession、presentation state view、Toast 和结束状态，并在 flow 返回后记录调试摘要。卡牌支付候选进入 first-person menu lease 的 request 构造由 App-private `FWacomRunEventPaymentLeaseBuilder` 承接，负责从支付选项聚合稳定、去重、有效的 `ExplicitCardInstanceIds`，Screen 只负责清理或提交菜单租约。PIE / 蓝图排查用的 debug view、单行 summary 和 drop result summary 由 `FWacomRunEventScreenDebugBuilder` 统一生成，避免 Screen 内散落诊断字符串拼装。

## §6 卡牌展示与 Builder

`UWacomCardPresentationBuilder` 是卡牌 UI 展示数据统一入口和 Blueprint 可用门面。它从 `UCardDefinition` 生成 `FWacomCardViewData`、`FWacomCardDetailViewData` 和效果徽章 view，只服务 UI 表现，不参与 Battle 或 Run 结算。小卡卡面数据、紧凑描述、体格/价值展示和效果徽章由 App-private `WacomCardFaceViewDataBuilder` 负责；详情文档的 Section 组装由 App-private `WacomCardDetailDocumentBuilder` 负责，settings 中的 explanation lexicon 读取和路径缓存由 App-private `WacomCardExplanationLexiconProvider` 负责，settings 中的 detail theme 读取和路径缓存由 App-private `WacomCardDetailThemeProvider` 负责，block / section 组装由 App-private `WacomCardExplanationCompiler` 负责，CardDefinition 专属模板、词典模板和 fallback 优先级由 App-private `WacomCardExplanationTemplateResolver` 负责，`{value:...}` / `{icon:...}` / `{status:...}` / `{keyword:...}` 等 typed slot 到 semantic run 的转换由 App-private `WacomCardExplanationTemplateRenderer` 负责；Token 语法与 Editor 校验共用 WacomData 的 `WacomCardExplanationTemplateContract`。tag / named text 的显示文案由 App-private `WacomCardExplanationText` 统一。Widget 只消费最终 `FWacomCardViewData` / `Sections`，不推断卡面字段、Effect / Passive 的分区规则。

Battle 卡面可以额外传入 `FWacomCardPresentationRuntimeContext`（C++ only）来覆盖当前展示事实。该 context 由 `WacomApp/Private/UI/Battle` 从 `FHandCardSnapshot` 构造；拖拽、敌人 hover 或 TargetSelect hover 有候选目标时，可以再叠加 `WacomBattle` 返回的 `FBattleCardTargetPreview` facts。`FBattleCardTargetPreview` 只表示规则事实；App 侧统一通过 `WacomBattleCardPresentation::BuildTargetPreviewPresentation()` 生成 `FWacomBattleCardTargetPreviewPresentation`，其中包含 hand layer entries、source detail 和可选 target hand card detail。Bridge / coordinator 只应用这份 presentation，不在各自路径里重新拼卡面和详情。Builder 只消费这些 UI-only facts，不读取 `UBattleSession`，也不重算战斗规则。

Runtime context 当前覆盖：

- 本场 `RuntimeCost`、可用状态、卡面 Cost、disabled overlay，以及 `Magnitude.Source.RuntimeCost` / 旧 `bMagnitudeFromRuntimeCost` 徽章。
- 目标预览事实仍按 `EffectIndex` 与规则效果对齐，但紧凑卡面按 `Damage / Poison / Heal / Shield` 等语义种类聚合为 `Badge.<Kind>`。条件分量被 skip 时只是不计入该语义组的预测总值；同组存在基础贡献时继续正常显示基础值，整个语义组都不会生效时保留 Badge 并低亮，不再绘制覆盖卡面的像素叉。
- 详情面板 `Sections`：Builder 从当前 `ResolveProfile()` 的 `Effects / Passives`、`FEffectCondition`、`FMagnitudeModifier`、`FWacomCardPresentationRuntimeContext` 和 `UWacomCardExplanationLexicon` 生成语义 `Blocks / Runs`。CardDefinition 的 `ExplanationTemplates.EffectTemplates / PassiveTemplates` 按索引拥有最高句式优先级，并由四阶共用；Effect 模板仍自动追加 Condition / Modifier，Passive 模板是完整玩家文案。对应专属模板为空后才依次回退 legacy `FCardPassive::DisplayText`、Lexicon 和 C++ 安全句式。`UCardDefinition::Description` 只在完全没有结构化详情 section 时作为受限回退，按普通正文显示且不解析 `{Effect.0}` 等旧占位。Effect 专属模板支持 `{value:Magnitude}`、`{icon:EffectIcon}`、`{status:EffectStatus}`、`{keyword:Tag}`；Passive 专属模板支持 `{value:TriggerThreshold}`、`{value:PassiveEffect[N].Magnitude}`、`{icon:PassiveEffect[N].Icon}` 和 `{status:PassiveEffect[N].Status}`。玩家可见的全局基础句式固定为 `{icon:EffectIcon} 造成 {value:Magnitude} 伤害。`、`{icon:EffectIcon} 恢复 {value:Magnitude} 生命。`、`{icon:EffectIcon} 获得 {value:Magnitude} 护盾。`；Poison / Slow / Freeze / Twilight / Burn 统一使用 `施加 {value:Magnitude} {status:EffectStatus}。`，其中 status slot 同时产生 inline 状态图标与中文状态名，不得重复添加 effect icon。Draw 模板默认不显示图标并写为“抽 {value:Magnitude} 张牌。”，Discard / ExhaustSelf / GainKeyword / RemoveStatus / ModifyInitiative 等正式效果也有可读模板。`PassiveOutcomeTemplates` 用于最终回退时 `OnCompanionCount` 这类规则真实存在但不走 `Passive.Effects` 的结果句；`MagnitudeSourceTemplates` 用于 `Magnitude.Source.RuntimeCost / TargetStatusStacks / HandCount` 等数值来源短语，例如“相当于当前费用 2”。`FCardEffect.Condition`、作为结构化回退输入的 `FCardPassive.Condition` 和 `FCardEffect.MagnitudeModifiers` 以静态说明追加到对应 block，不在 UI 层执行规则判断或重新计算最终数值。目标预览时若 Battle preview 给出的最终数值不同于基础显示值，数值 run 只显示最终值并使用 `ValueBuffed / ValueNerfed` 标记强化或削弱；未被 preview 改写时可保留来源短语。skip 效果会由 compiler 插入 `Muted` 前缀 run。卡牌详情正式中文字体不支持乘号字形，因此 explanation 模板使用字母 `x`，不使用 `×`。

旧 `FWacomCardDetailViewData.Description`、`ChangeLines`、`PassiveLines`、`TaskLines`、扁平 `TokenLines` 和旧 token flow contract 已删除。详情面板不再暴露平行纯文本正文或 `GetDescriptionText()`；需要显示的正文必须进入正式 `Sections` 语义文档。`FWacomCardViewData.Description` 仍是小卡卡面的紧凑描述；expanded detail 只在空结构化详情时复用它作为兜底可读文本。

没有 runtime context 的背包、商店和 Run 卡面继续使用静态定义展示；旧 `BuildCardViewData(Card)` / `BuildCardDetailViewData(Card)` 路径会生成基础 explanation document，但不产生 target preview 数值。

当前复用方：

- 背包卡牌、拖拽预览和卡牌详情。
- 战斗 first-person hand、Presentation Stack 小卡和 Combat Log detail。
- 商店商品 ViewData。

`UWacomCardView` 只显示 `FWacomCardViewData`，不提交战斗、背包或 Run 命令。卡牌详情由 `UWacomCardDetailPanel` 显示 `FWacomCardDetailViewData.Sections`；`WBP_CardDetailSection` 的正式制作合同是 `TitleText: CommonTextBlock` 与 `BodyText: UWacomCardDetailRichTextBlock`，正文由 `WacomCardDetailRichTextRenderer` 从语义 `Blocks / Runs` 转成 RichText markup。`BodyText` 会自动注册详情 inline icon decorator；`Status` run 显示 `CardDetailTheme.StatusBrushes` 图标并保留状态文字，显式 `{icon:EffectIcon}` 显示 `CardDetailTheme.IconBrushes` 图标。BattleHUD 内部 card detail controller 只服务 first-person viewport 详情；背包详情由 `FWacomBackpackCardDetailController` 承接背包界面内的 panel lifecycle、source guard 和定位。

## §7 WBP 绑定文档分工

Binding 文档只记录 WBP 制作合约，不写规则真相。

| 文档 | 职责 |
|---|---|
| [UI_Backpack_WBP_Binding.md](./UI_Backpack_WBP_Binding.md) | 背包、局部 Zone、SpecialZone、DeckCard、CardView、CardDetail、EffectBadge 的父类、路径、绑定槽位和 PIE 检查 |
| [UI_Battle_WBP_Binding.md](./UI_Battle_WBP_Binding.md) | BattleHUD、手牌、CombatLogFeed、PresentationStack、场景敌人状态 Badge 和 first-person card view 的 WBP 绑定协议 |
| [UI_RunEvent_WBP_Binding.md](./UI_RunEvent_WBP_Binding.md) | RunEventScreen、ChoiceButton、PaymentDropTarget 的父类、路径、绑定槽位和 PIE 检查 |
| [UI_RunMap_WBP_Binding.md](./UI_RunMap_WBP_Binding.md) | Run Map Screen、动态节点、1920×1080 画布和像素风状态合同 |

需要知道“为什么这样做”时，回到本文和对应专题文档；需要知道“绑定哪个槽位”时，读 Binding 文档。

## §8 World Shop card presentation

World Shop 的每件商品由 `UWidgetComponent` 以 `EWidgetSpace::World` 渲染。`UWacomWorldShopCardWidget` 是被动 world-safe adapter：它直接创建精确的 `/Game/Wacom/UI/Card/WBP_FirstPersonCardView`，继续消费同一 `FWacomCardViewData` 和 `296×420` 卡身合同，但不把 HUD 专用 `/Game/Wacom/UI/Card/WBP_FPCardView` Retainer/Fake3D 包装嵌进 `WidgetComponent`。嵌套 Retainer 与世界 Widget 的离屏渲染会形成第二层 RenderTarget/颜色处理，在真实 PIE 中已表现为明显发白和对比度丢失，因此不是合法的世界卡面链路。

World adapter 使用 `360×488` 逻辑设计面，Host 默认以 `720×976` DrawSize 做 2× supersampling，并用 `0.10` world scale 保持约 `72×98 cm` 的实体尺寸；价格/禁用状态占底部 `296×52`。Widget 根内容必须先由 `WorldCardRenderSurface` 明确占满整个 `720×976` RenderTarget，再让内层 `ScaleBox` 把 `360×488` 逻辑面精确放大 2 倍；不能依赖 Virtual Window 自动拉伸期望尺寸，否则卡面会按未放大的逻辑尺寸呈现，PIE 可见宽高只有制作线框的一半。这样世界商品与 HUD 手牌共享精确 CardView 资产、数据、卡身比例和材质语义，同时把 HUD-only Retainer、倾斜余量与手牌交互留在 first-person layer。整个 adapter 只有一个透明 Primary Action，卡面主体和底部价格区点击都提交同一个 `(OfferId, Generation)` intent，不另设 Buy 按钮。已购买商品保留原槽位并显示“已购买”，但 Widget 不再广播 intent；金币不足商品仍广播到权威购买入口以获得失败反馈，不使用 Slate Disabled Effect 洗白卡面。

Primary Action 的 Normal/Hovered/Pressed/Disabled 画刷全部是 `FSlateNoResource`，不得以默认白色 Button brush 充当卡牌背景。正式包装与内层卡面使用 `SelfHitTestInvisible`，左键仍由整卡 action 接管；关键词命中不增加透明 Button 或覆盖层，也不改变购买的 pressed/released/clicked 链路。三选一与强化拖放区仍属于后续独立活动。

`UWacomWorldShopHUDWidget` 的根是透明 `SelfHitTestInvisible` overlay，只显示金币和 Esc 提示，不覆盖场景颜色，也不拦截 World WidgetInteraction。商品 Widget 不读取或修改 Run 状态；刷新由 coordinator 读取最新 Shop snapshot 后重新投影 ViewData。

成功交易不会仅因本次扣除了当前时段最后 1 点 Action Point 而关闭 World Shop。Run 会把剩余点保留为 0、继续持有 Shop visit；coordinator 刷新售出槽位并保持 LookOnly/WidgetInteraction，玩家仍可浏览或进行本访问内后续 0 AP 交易。只有 Esc、Host/Screen teardown 或其它权威 visit close 才退出表现；关闭结果若携带延迟的 phase advance，App 先应用该 `FRunExplorationResolution`，再恢复 Run Path。旧 ShopScreen 与 World Shop 共用这一条 `WacomRun` 规则。

`FWacomCursorLookProfile` 除 Clamp、Scale 和 Interp 外还提供 `CursorDeadZoneNormalized` 与 `CursorResponseExponent`。死区按水平/垂直轴独立移除并重新映射到 `0..1`，随后应用正指数曲线；鼠标到达视口边缘时仍精确达到 Clamp。默认 `DeadZone=(0,0) / Exponent=1` 与旧 Run/Battle 线性行为等价。组合式正式商店默认不覆盖 Look Profile，进入后复制玩家当前 Run Path 的 live Clamp、Scale、Interp、DeadZone 和 Exponent；Host 的可选 override 只保留给 transient 验证或未来明确需要独立镜头手感的活动，不写回 Run/Battle 制作参数。

World Shop 入场不再只冻结手牌点击，而是使用可复用的 Run hand `World Activity Suppression`。它立即取消 hover/drag 和交互，在 `0.18 s` 内把同一个 card layer 向下移动 `0.42×viewport height` 并淡出；卡牌 entries、`SourceId`、`InstanceId`、Slot Widget 和 transition hint 都不清空。退出时必须先完成 Run Path 镜头返回，再反向恢复同一批 Widget，因此不会产生新的 `RunHandEntered` hint 或重播发牌动画；Staging 失败、Host teardown 和 Shutdown 都必须释放 suppression，Shutdown 使用立即恢复。

世界卡面曝光与合成模式通过独立 transient PIE 实验台验证，不直接拿正式商店承担材质试错。命令 `Wacom.WorldCardRender.OpenPIEValidation [OptionalCardDefinitionPath]` 会用同一个 `FWacomCardViewData` 同时创建一张屏幕空间参考卡，以及 `Engine Transparent`、`Engine Masked`、`Wacom Masked Raw`、`Wacom Masked Exposure` 四张相机相对世界卡；它们都直接使用精确的 `WBP_FirstPersonCardView`，世界卡统一为 `720×976` DrawSize 和 `0.10` world scale。该实验不打开 Shop Visit、不访问或修改 Run、金币、背包、手牌和购买状态，世界卡也没有碰撞、焦点或输入命中。

正式基础材质 `/Game/Wacom/UI/Card/World/M_WorldCardSurface` 保持 `Surface / Unlit / Masked / Two Sided`，沿用 Widget 3D 的 `SlateUI`、`BackColor`、`TintColorAndOpacity`、`OpacityFromTexture` 与 Alpha 组合，并在最终 RGB 前加入 `EyeAdaptationInverse`。参数 `ExposureCompensationStrength` 的 `0` 表示不补偿，`1` 表示完整反曝光补偿；实验中可用 `Wacom.WorldCardRender.SetExposureStrength <0..1>` 实时比较。人工对照的胜出配置为 `Wacom Masked Exposure + 1.0`；App-private 材质适配器是路径、Masked MID 和参数写入的唯一运行时入口，正式 World Shop 固定使用 `1.0`，依赖缺失时在开始 Shop Visit 前 fail closed 并回退旧 ShopScreen。

两个 transient 验证入口默认位于相机前 `320 cm`：渲染实验台继续逐帧相机相对，World Shop transient Host 只在打开时定位一次，Production Host transform 不受影响。`Wacom.WorldShop.DumpPIEValidation` 输出 World route/Host、Mouse WIC hover/hit、首件 Offer、金币、Backpack 与 StorageRevision，以及首件卡定义相对 Open 基线的持有数增量；它只用于验证，不修改交易或 Run 状态。

正式关卡入口使用 `AWacomWorldShopActor` 组合既有 `AWacomShopTriggerActor`、内部 `AWacomFirstPersonViewpointActor` ChildActor、`CardLayoutRoot`、`ShopFocusAnchor`、`UWacomRunMapNodeBindingComponent` 和独立 `PresentationRoot`。正式路径不再创建内部 `AWacomWorldShopHostActor` ChildActor：`OfferLayoutAnchor_01..08` 本身继承无 Primitive/BodyInstance 的 `UWacomWorldShopOfferAnchorComponent`，既是 BP Viewport 中的制作位置，也是 Coordinator 创建 `UWidgetComponent` 时使用的真实运行时 Anchor。`FWacomWorldShopPresentationHost` 只在打开时描述 Owner、真实 Anchor、DrawSize、Pivot、Scale、InteractionDistance 和 Look Profile，使正式 Actor 与 transient/旧 Host 共用路由而不复制场景层级。八个 Anchor 分别由一个 `VisibleDefaultsOnly` 原生组件属性锚定：它仍保留 `CPF_Edit`，因此 UE Blueprint Subobject Editor 能解析继承组件的 Details 和 Transform；同时 `CPF_DisableEditOnInstance` 让 Level Editor 的 Actor Details 在生成属性树前排除组件引用，禁止把 `TriggerSphere / ClickBounds / ShopViewFrustum` 的 `BodyInstance` 与 Attachment 图内联展开。组件制作必须从 BP Components 树进入，关卡 Actor Details 只编辑实例规则/身份参数。禁止恢复内联 `Instanced TArray`，运行时 getter 只返回这八个固定指针，不接纳派生 BP 额外组件。`CardWorldScale` 是 Slate 渲染像素到世界厘米的绝对换算，Coordinator 在组件注册前写入 DrawSize、Pivot 和绝对 World Scale，不允许地图 Actor 的非均匀 Scale 再放大卡牌，并保证首帧 `CurrentDrawSize`、Bounds 与命中平面使用正式尺寸。统一 `CardWorldScale=0.13` 时，完整 `720×976` 渲染/命中平面为 `93.6×126.9 cm`；固定 `296×420` 卡面与 `296×52` 价格框组成的可见商品范围约为 `77.0×122.7 cm`。选中任意 Anchor 时，`WacomEditor` Component Visualizer 忽略父级缩放，同时绘制暗色外层渲染平面、亮色可见商品框、价格框分隔线、槽号、中心和正面方向；当前槽高亮，不创建碰撞、Widget、材质或 BodyInstance。Location/Rotation 属于 BP 制作数据，逐槽 Scale、SlotId、SlotOrder 和 Enabled 均由 C++ 锁定，Construction 只恢复槽身份和单位缩放，不复制 Transform、不重排人工位置。Class Defaults 将制作参数收敛到单一 `Wacom|World Shop` 分类，原生组件引用单独进入 `Wacom|World Shop Components`，同名折叠栏不是多份 Scale。默认 2×4 布局使用 `8 cm` 间距，Focus 附着于 `CardLayoutRoot` 并位于第二列中心约 `Y=-50.8 cm`。C++ 继续拥有路由、回退、Viewpoint 解析和数据校验；Blueprint 子类只制作无碰撞场景表现，不在 EventGraph 复制购买、Visit 或 Run 规则。`/Game/Wacom/Maps/SceneActor/BP_WacomWorldShop` 的 Backboard / Counter 仍只负责无碰撞场景表现。

`WacomEditor` 的 Anchor Component Visualizer 必须在 `GUnrealEd` 可用后幂等注册；模块早于 Editor engine 启动时延迟到 PostEngineInit，不允许静默跳过。线框只存在于包含该 Editor 模块实现并完成全量编译的 worktree/Editor 进程，不能用另一分支的已运行 Editor 验收尚未集成的工具源码。

正式 Host Owner 进入 World Shop 活动后，App-private entry-bounds guard 会把同一组合 Actor 的入口 `ClickBounds` 临时设为 `NoCollision`，避免它在卡牌前截断 Mouse WidgetInteraction 的 `Visibility` trace；旧外部 Host 仍可通过 Parent Actor 兼容解析。只有镜头完全返回 Run Path、活动结束后才恢复进入前的碰撞模式。并行的 Exploration HUD visibility guard 只把 Game Layer 当前 `UWacomExplorationHUD` 根可见性折叠为 `Collapsed`，不 Deactivate、不 Pop，因此 CommonUI `GameAndUI / NoCapture` 输入配置保持不变；商店金币、Esc、购买反馈与全局 Toast 继续显示。正常退出、Staging 失败、Host Owner EndPlay 和 Shutdown 都必须恢复进入前的精确 Visibility，并刷新当前世界交互提示。

World Shop 从 Staging 起接管指针时，必须立即清除普通 Run Hover/Probe，并让后续 probe、hover、click 全部服从统一 Run scene pointer gate；因此 Backboard 不得继续缩放或写入 CustomDepth。Slate preprocessor 只为游戏视口内被 World Shop 实际消费的 LeftMouseDown 保留成对 Release，不能吞掉 Editor/PIE 控件自己的 MouseUp；NoCapture 下的 Esc 只在当前游戏视口拥有焦点或鼠标上下文、且 World Shop 正在接管输入时补充转发。

世界卡牌浏览由 App-private `FWacomWorldCardInteractionPresenter` 承担，World Shop 只是首个接入活动。活动向 Presenter 提供稳定 ItemId、`UWidgetComponent`、根 Widget、`UWacomCardView`、卡定义身份和只读 `FWacomCardDetailViewData`；Presenter 不访问 `URunSession`、金币、背包或购买入口。默认 Hover 在 `0.12 s` 内沿相机方向前移 `8 cm` 并放大到 `1.06`，移出、商品移除、身份替换和 teardown 都恢复创建时的精确相对 Transform；固定 Inspect 卡保持抬起，浏览另一张卡时两者可同时抬起。活动无关的共享类型集中在 `WacomApp/Public/UI/Card/WacomWorldCardInteractionTypes.h`：`FWacomWorldCardInteractionStyle` 是表现参数合同，`FWacomWorldCardPointerSample` 是由当次活动的 Mouse WidgetInteraction owner 产出、被表现层消费的指针采样。Presenter 只依赖这个头，不反向包含任何 `UI/Shop/` 头文件；World Shop 只是首个产出 pointer sample 的活动，后续三选一和强化卡拖入活动复用同一形状，不各自定义。`FWacomWorldCardInteractionStyle` 由 `BP_WacomWorldShop` Class Defaults 统一制作，后续三选一等世界卡活动可以复用同一表现合同。它是被该蓝图序列化的 USTRUCT，按类型名而非头文件路径解析：可以换头文件，但重命名 struct 或任一字段都会把制作值静默重置为 C++ 默认，`Wacom.UI.WorldCardInteraction.AuthoredStyle` 会在那种情况下立即失败。

`FWacomCardViewData.TypeSemanticTokens` 是 `TypeText` 内逐词语义的唯一命中真相：Builder 按最终显示顺序同时生成 `SemanticId / SourceTag / DisplayText / 字符范围` 与 `TypeText`，不得由 Widget 重新拆字符串。`UWacomCardView` 使用真实 `TypeText` CachedGeometry、字体度量、居中与换行宽度做只读命中；分隔符、空白和文字外区域 fail closed。默认语义词典位于 `UWacomCardExplanationLexicon.CardFaceSemantics`，配置的 `DA_CardExplanationLexicon_Default` 优先，C++ 默认条目兜底。`UWacomCardExplanationLexicon::FindCardFaceSemantic` 只按 `SemanticId`、再按精确 `SourceTag` 匹配单个词典内的条目，命中即返回，不要求字段完整；跨词典的合并由 App-private `WacomCardExplanationLexiconProvider::FindCardFaceSemantic` **按字段**完成：配置资产逐字段覆盖 C++ 默认，留空的 `DisplayName` / `Description` 回落到默认条目。因此制作时只填 `DisplayName` 改显示名不会连带清空该关键词的 Tooltip 正文。两个字段最终都为空时才算缺失：不显示玩家可见的裸 Tag，并只在开发环境按 SemanticId 警告一次。

关键词停留默认 `0.15 s` 后由被动 `UWacomCardSemanticTooltipWidget` 在真实鼠标右上显示，跟随鼠标并按游戏视口安全区水平/垂直翻转与夹紧。它不使用 Slate `ToolTipWidgetDelegate`：World WidgetInteraction 的虚拟鼠标不保证驱动平台鼠标所属的 Slate Tooltip window/user。右键由 World Shop 的 pre-UI owner 消费并强制刷新一次 WIC 命中：同卡切换关闭、异卡直接切换、空白关闭；详情惰性创建现有 `WBP_CardDetailPanel`，失败时回退 C++ `UWacomCardDetailPanel`，固定在目标卡所在屏幕半区的对侧。Tooltip 和详情都为 `HitTestInvisible`，不会获得焦点、拦截左键或提交规则；售出和金币不足商品仍可查看，购买刷新只更新固定详情数据。

`DA_Floor_LevelAuthoring_01` 的 `Shop.Snake` 已指向 purchase-only `/Game/Wacom/Data/Shops/DA_Shop_LevelAuthoringSnake`：8 个商品、第一件免费、强化服务关闭，避免 25 商品 + 强化服务越过 2×4 世界 Host 的合法路由矩阵。`L_Exploration` 的唯一正式实例必须继续保留 `BP_WacomWorldShop` class、Actor label/root transform、`PersistentId=Shop.Test.001`、`NodeId=Shop.Snake`、`TriggerRadius=200 cm`、ShopDefinition 和其它人工地图对象；正式卡位和统一尺寸在 BP 类中制作，关卡实例不逐槽覆盖。Viewpoint 到 Focus 继续保持受支持的 `180-320 cm` 范围和朝向对齐，Look 使用 Run live profile。该 Authoring 内容不是 Floor 1 Production Shop，不能迁移进 `L_Run_Floor_Main_01` 或冒充 Production 数据。
