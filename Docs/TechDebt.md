---
type: tech-debt
scope: wacom-current-debt
status: active
updated: 2026-07-08
tags:
  - wacom/tech-debt
  - wacom/docs
---

# TechDebt

> [!info] 本文职责
> 本文记录当前仍存在的临时写法、兼容路径、临时决定和正式替代方案。短期任务看 [`TODO.md`](TODO.md)，未来功能方向看 [`Roadmap.md`](Roadmap.md)，会改变规则口径的问题放 [`Questions.md`](Questions.md)。

> [!warning] 使用约束
> 这里不是功能愿望池，也不保存实现流水账。公开面和文档重构历史见 [Wacom_Public_Surface_And_Docs_History.md](./DevLog/Wacom_Public_Surface_And_Docs_History.md)；当前规则真相仍以领域文档为准。

## 规则层技术债

| 项 | 临时做法 / 当前决定 | 正式方案 / 处理方向 |
|---|---|---|
| 双手区保留 | 硬编码“左右手都在时双手区普通卡保留”，不区分角色 | 多角色时由 `CharacterDefinition` 或 `CardDefinition` 字段驱动 |
| ZoneHook Trigger 种类 | 只支持 `OnPlay` / `OnPerfectReleaseHit` | 按卡牌需求扩展 `OnTurnStart`、`OnDiscard`、`OnEnterZone` 等 |
| 费用转移 | 只支持 `ReduceCost(LastShuffled) + AddCost(Self)` 组合 | 更复杂的多点 / 条件费用转移引入 `CostLedger` 或 `CostTransferEvent` |
| CompanionPlayedCount | 全局计数，不区分哪张伙伴 | 当前对齐 BugGirl.md §5；多角色或多类伙伴时再评估是否拆分 |
| 回合结束保留 / 弃牌时序 | `EndTurnResolver` 中放在敌方行动之前 | 若规则明确放在敌方行动之后，需要调整 resolver 时序 |
| `Magnitude.Source.TargetStatusStacks` 参数 | 借用 `FCardEffect::TargetZone` 传 Status Tag | 给 `FCardEffect` 或 `FEffectContext` 加专用 `FilterTag` 字段 |
| `Effect.GainKeyword` / `Effect.RemoveStatus` 参数 | 借用 `FEffectContext::MetaTag` 或 `TargetZone` 传 Keyword / Status Tag | 同上，收口到专用 `FilterTag` 字段 |
| `IsDeleteFunctionAvailable` | 接口已存在，但当前 Run 删牌事务和背包 UI 仍按始终可删的简化口径工作 | 等删牌可用性口径确认后，Run 校验和 UI 显隐统一接入 |
| 手牌锚点左右归属 | `FHandCardSnapshot` 不带左右手角色，UI 用遍历顺序启发式 | 给 `FHandCardSnapshot` 加 `EHandAnchorRole` 字段 |
| BattleResult identity 兼容字段 | `FBattleResultPacket` 仍同时暴露 `PartId / Identity / PartKey`、`DestroyedParts / DestroyedPartKeys`；Run 撤离进度的新写入、Run 结算日志和 GameMode 撤离全灭判断已收敛为 key-first，`DestroyedParts` 仅作旧数据 / 手写 snapshot fallback | 继续把测试、日志和消费者迁到 `FBattleEnemyPartKey` / `DestroyedPartKeys`；确认没有资产 / 蓝图依赖后再降级或移除 legacy projection 字段 |

---

<a id="techdebt-data-save"></a>
## 数据与存档债

| 项 | 临时做法 / 当前决定 | 正式方案 / 处理方向 |
|---|---|---|
| 存档系统暂停 | `bSaveSystemEnabled = false`；底层 SaveGame / RunState 拷贝和迁移机制保留 | Demo 完善后恢复 Bootstrap 读盘、PauseMenu Save、MainMenu Continue |
| RunEvent 状态 | `RunEventStates` 当前只保存在 Run 内存态 | 接入 SaveGame，并定义状态生命周期 |
| Shop 状态 | `ShopStates` 当前只保存在 Run 内存态 | 接入 SaveGame，并定义库存刷新与跨日保留规则 |
| 金币存档 | `Gold` 当前作为 Run 内资源，不写入 SaveGame | 恢复存档系统时确认是否入档 |
| `BattleState` 非反射 | 裸 struct + pImpl，GC 引用靠 Session 的 `ReferencedAssets` | 若需存档 / 网络，升级为 USTRUCT 或 UObject |
| SaveGame 迁移 | 版本迁移 switch 已有 v0 到 v2 模型 | 每次升版本加新 case，永远不改已存在 case |

---

<a id="techdebt-ui"></a>
## UI 层技术债

UI 当前事实入口见 `WacomUI.md`；CommonUI shell 见 `WacomUIFoundation.md`，Battle UI 见 `WacomBattleUI.md`，first-person card layer 见 `First_Person_Card_Layer_Design.md`。本节只记录仍需替换或收口的临时写法。

| 项 | 临时做法 / 当前决定 | 正式方案 / 处理方向 |
|---|---|---|
| 战斗 UI 全量刷新 | 每次命令后 BattleHUD 从 Snapshot 全量刷新子 widget | 加动画时动画系统自行 diff；数据源仍保持 Snapshot |
| 战斗 UI 不做 ViewModel | BattleHUD 持有 `UBattleSession*`，子 widget 读 `FBattleSnapshot` | UI 复杂度上升或外部 widget 需要战斗状态时，再抽 `UWacomBattleViewModel` |
| C++ 硬编码默认布局 | Widget 类 `Blueprintable` 非 Abstract，带 C++ fallback 布局；BattleHUD / BackpackScreen 的 fallback 构建已抽到私有 helper | 美术阶段用 WBP 替换视觉，C++ 保留协议和兜底 |
| Battle Widget Session Blueprint 面 | `UWacomBattleWidgetBase.SetSession / GetSession` 仍保留给 C++ owner 注入和旧 WBP 兼容，但 Blueprint surface 已标记 deprecated；正式 WBP 制作面应只消费 Snapshot / ViewData 并回传玩家意图 | 完成资产审计后评估是否把 session getter 降为 C++ only，或用专用 `UWacomBattleViewModel` / provider 承接外部只读状态 |
| HP 条瞬间跳变 | `SetPercent` 直接设值 | 加 `SetTargetPercent` + Tick / 动画插值 |
| 场景敌人表现 polish | 主链路已拆成 `SceneEnemyHost + PartActor + WorldTargetBridge + Presentation`；普通小怪走 Host 整体图 + hit-only 部位，精英 / Boss 走 PartActor VisualLayers | 继续补正式 sprite/flipbook 美术、材质描边、tooltip、风险动效和 PaperZD/Animator 状态机 |
| CombatLog 纯文字 | 常驻 CombatLog 暂不做图标、颜色、Niagara、音效或动画；旧 EventToast / BattleEventLogPanel / 单事件 legacy bridge 已删除 | 升级为事件表现调度器，接 Niagara、音效、tone 颜色、icon、筛选、事件详情和战后回放 |
| 击倒 Dialog C++ 布局 | CanvasPanel + Border + Button 硬编码 | 正式 `WBP_KnockdownChoiceDialog` 承接同名 BindWidget 锚点 |
| Backpack UI C++ 默认布局 | 拖拽模型已接入，fallback 布局 / 运行时区域构建已抽到私有 helper，但视觉仍主要由 C++ 构造 | 正式 `WBP_BackpackScreen` 和局部 WBP 替换视觉 |
| Backpack / Shop 长列表 | Backpack / SpecialZone / Shop 已有 revision gate、signature dirty gate 和 identity reconcile；Backpack 的 snapshot refresh gate、普通卡列表 reconcile 和 SpecialZone section reconcile 已抽到 App-private helper，Shop 的 snapshot / offer dirty gate 和 offer row reconcile 已抽到 App-private helper，但列表仍是 WrapBox / VerticalBox | 卡量明显上升时再迁 `ListView` / `TileView` 或做正式虚拟化；Shop 正式卡面预览另起切片 |
| 像素风 UI 分辨率适配 | 背包卡牌等像素图控件依赖固定 SizeBox 和 `DPI Scale = 1.0`；非整数 DPI 缩放会导致像素点显示不均匀 | 统一设计像素安全缩放档位，并配合 WrapBox / ScrollBox 做布局重排 |
| 探索 HUD 时段总节点数 | 只显示剩余节点，没有本时段总节点快照 | `FRunState` 加 `TotalNodeCountForPhase`，或 HUD 在时段切换时记录初始值 |
| AppToast C++ fallback 表现 | 未配置 settings 时仍使用文字 fallback；viewport 创建已受真实本地玩家 / `LocalPlayer` 条件保护，离屏自动化注入 Widget 不进入 viewport | 正式 WBP 后接颜色、图标、动画、音效和全局日志策略 |
| PrimaryLayout 固定路径 fallback | PrimaryLayout 仍允许 settings -> 固定 `WBP_PrimaryGameLayout` 路径 fallback -> null | 资产路径稳定后评估是否也完全转为 settings-only |

<a id="techdebt-run-session"></a>
## RunSession 结构债

| 项 | 临时做法 / 当前决定 | 正式方案 / 处理方向 |
|---|---|---|
| `URunSession` 仍承担多个领域流程 | 背包 / 负重 / 永久移除规则、RunEvent 执行、商店事务、战斗回传结算、SaveGame 字段拷贝均已抽到私有 helper；Run UI snapshot revision dirty bitset 已收口为 typed 私有 contract；`RunSession.cpp` 仍保留 public 命令协调、slot IO、时间 / 压力等基础入口 | 暂不继续拆；后续若时间 / 压力或 slot IO 继续膨胀，再按低风险切片拆私有 helper |
| Run→Battle 入口 legacy fallback | `AWacomGameMode::EnterBattle` 已拒绝缺失 / 无效 RunSession，不再用 `DefaultCharacter / DefaultRandomSeed` 拼脱离 RunState 的 fallback 战斗；C++ no-trigger `URunSession::BuildInitParamsForBattle(FBattleInitParams&)` 已移除，Run 侧测试改为显式 trigger-aware contract；BlueprintCallable `OnBattleFinished(Packet)` 包装和 `DefaultRandomSeed` 资产兼容字段仍保留，但均已标记 deprecated | 资产审计后再决定移除或重命名 `OnBattleFinished(Packet)` 包装与 GameMode legacy 随机种子字段；新增 C++ 测试默认调用 `OnBattleFinishedFromTrigger` / `BuildInitParamsForBattle(TriggerId, Params)` |

<a id="techdebt-ui-architecture"></a>
## UI 架构债

| 项 | 临时做法 / 当前决定 | 正式方案 / 处理方向 |
|---|---|---|
| ViewModel FieldNotify 未被 WBP 消费 | C++ 父类用 `OnRunViewModelRefreshedNative` 粗粒度多播 + 手动 SetText | 美术阶段 WBP 配 Global Collection Identifier `WacomRunViewModel`，View Bindings 直接绑字段；全 WBP 后删粗粒度路径 |
| `OnRunStateChangedNative` 粗粒度广播 | 组合事务已收口为事务末尾一次广播；普通 public mutation 仍保持成功后一次广播 | 订阅方仍按粗粒度事件幂等刷新；新增组合 Run mutation 时补 `Wacom.Run.NotificationCoalescing` 测试 |
| BattleHUDRuntime helper 剩余膨胀 | Snapshot presentation、CommandBar presenter、command controller、targeting controller 和 first-person drop resolver 已抽到 App-private 文件；Command / Targeting HUD flow 合同测试已拆到 `BattleHUDCommandFlowSpec.cpp`，Combat Log 合同测试已拆到 `BattleCombatLogSpec.cpp`，PresentationStack 纯展示合同测试已拆到 `BattlePresentationStackSpec.cpp`，PresentationQueue / turn-boundary / pending barrier lifecycle / teardown / BattleEnd / knockdown 延迟展示合同测试已拆到 `BattlePresentationQueueSpec.cpp`，InteractionTarget world bridge / scene click 合同测试已拆到 `BattleInteractionTargetSpec.cpp`，battle scene enemy target registry 合同测试已拆到 `BattleSceneEnemyTargetRegistrySpec.cpp`，battle scene enemy hover probe 合同测试已拆到 `BattleSceneEnemyHoverProbeSpec.cpp`，BattleHUD first-person hand / first-person card detail 合同测试已拆到 `BattleHUDFirstPersonSpec.cpp`，battle scene enemy prediction、PartActor facade / presentation setup、bridge runtime facts、world target handle、host visual、host visual routing、host identity / child actor scan、runtime facts / host counts、debug snake child actor authoring、part slot identity / duplicate validation、prediction badge offset、badge layout debug / stagger、VisualLayers 和 authoring validation 合同测试已拆到 `BattleSceneEnemyActorSpec.cpp`；Battle scene target click / probe 已抽到 `FWacomBattleSceneInteractionRouter`，`FWacomBattleHUDRuntime` 仍集中持有 helper 生命周期和较宽的 HUD façade 转发面，`BattleWidgetSpec.cpp` 仍承载 fallback layout、event presentation 和其他跨专题旧测试 | 后续优先处理生产 module 的 remaining thick coordinator（例如 Battle result legacy contract）；测试拆分只配合生产重构或降低验证成本推进。Runtime 若继续膨胀，再按 low-risk slice 收窄 façade 转发面或抽出更深的 presentation state module |
| BattleHUD legacy 点击手牌入口 | `OnCardClickedByUser` 已标记 Blueprint deprecated 并收口为空兼容入口；C++ 自动化测试已不再调用该入口，Content / Config / uproject 粗扫未发现资产引用，`FWacomBattleHUDRuntime` 的 legacy façade 转发已移除 | 后续做一次编辑器资产加载 / Blueprint 编译审计；确认无旧节点后移除 Blueprint 面或保留到下一个兼容清理批次 |
| PlayerController Run world 路由剩余边界 | Battle scene target click / probe 已由 `FWacomBattleSceneInteractionRouter` 承接；数字牌位快捷键只向 BattleHUD 提交 hand index / pointer，不再读取 BattleSession snapshot；Run world hover / click / probe、clickable resolver、hover debug 和 probe preview state 已由 `FWacomRunWorldInteractionRouter` 承接；`AWacomPlayerController` 仍保留 InputKey 顺序、timer、E 键 candidate list、trace seam、RunSession / GameMenu ownership 和 Run card drop coordinator context 注入 | 暂不继续抽 E 键 candidate list 或 Run world card drop 提交流程。若后续 Run card drop target adapter 继续膨胀，再优先在 `FWacomRunFirstPersonCardDropCoordinator` 内拆 adapter / transaction helper，保持 `URunSession` 规则事务和 Widget passive 边界 |
| `UWacomMenuWidgetBase` 上的 Run first-person 兼容桥 | Backpack / Shop / RunEvent 已继承 `UWacomRunMenuWidgetBase`，生产 lease / drop ownership、PlayerController owner lookup 和 drop coordinator submit/resolve 都只走 Run 专用父类；Foundation 菜单基类上的旧 `SetOwnedRunFirstPersonCardLayerMenuLeaseFromRunCards`、Run menu drop resolve / submit 钩子已标记 deprecated，只转发到 Run base 或返回 unsupported，保留给旧 Blueprint 节点编译；2026-07-08 Blueprint compile audit 中 `Content/Wacom/Test/WBP_MenuTest.uasset` 仍报告 deprecated 节点警告 | 后续先审计 / 修复直接引用 Foundation 基类旧节点的资产（已知 `Content/Wacom/Test/WBP_MenuTest.uasset`），确认可 reparent / 删除后再移除 Foundation deprecated API，让 MainMenu / Pause / Confirm 等通用菜单彻底不暴露 Run 节点 |
| First-person card layer 巨型 spec | `FirstPersonCardLayerDropIntentSpec.cpp` 已从 `FirstPersonCardLayerSpec.cpp` 拆出 Battle drop intent / release / card-target affordance 专题；BattleHUD hand source identity、pending target 状态、late cleanup ownership 与 BattleEnd clear 覆盖已从 giant spec 删除，保留在 `BattleHUDFirstPersonSpec.cpp`；BattleHUD CardDetail hover / source switch / hovered slot follow / large jump reset / side hysteresis / invalid data / state clear 覆盖已迁到 `BattleHUDFirstPersonSpec.cpp`；generic `DetailProvider.HoveredSlotLayoutUpdateBroadcasts` 仍保留在 layer spec，因为它验证 Anchor/Layer 广播而非 HUD CardDetail adapter；`FirstPersonCardLayerSpec.cpp` 仍同时承载 anchor/layout、slot motion、transition、runtime source、detail、gesture 和 drag-target feedback 等大量专题 | 后续优先做生产模块收口；测试拆分只在配合生产重构或降低验证成本时推进。若继续拆测试，按 `FirstPersonCardLayerMotionSpec.cpp`、`FirstPersonCardLayerGestureSpec.cpp`、`FirstPersonCardLayerRuntimeSourceSpec.cpp` 等专题拆，并收口 tests-only helper，避免复制世界创建、HUD 装配和 slot 构造逻辑 |
| BackpackScreen Presenter 边界 | Presenter 已抽展示计算；顶部 stats / section title 刷新已抽到 `FWacomBackpackHeaderPresenter`；卡牌详情面板生命周期 / source guard / 定位已抽到 App-private detail controller；snapshot refresh dirty gate、普通卡牌列表 reconcile 和 SpecialZone 区块 reconcile 已抽到 App-private helper；DropTarget 只转发拖拽意图，命令提交和确认框统一在 Screen；legacy `BurdenDropTarget` 字段和 UI 侧 `BurdenZone` 禁止特判已移除，负重区只显示 Run snapshot | 如果 Screen 继续膨胀，优先审计命令 flow 边界或进一步收口 zone rebuild transaction；命令 flow 已有私有对象，先保持 Screen 作为玩家意图入口 |
| RunEventScreen 剩余协调职责 | 选项行 / 支付 DropTarget identity reconcile 已抽到 `FWacomRunEventChoiceListReconciler`；payment drop resolve / submit 已抽到 `FWacomRunEventPaymentDropFlow`；menu lease request 构造已抽到 `FWacomRunEventPaymentLeaseBuilder`；debug view / summary 已抽到 `FWacomRunEventScreenDebugBuilder`；cached choices 和支付 Zone 映射查询已收口到 `FWacomRunEventPresentationStateView / Edit`；Screen 仍持有刷新提交、关闭流程和 UPROPERTY 缓存所有权 | 保持 Screen 作为刷新、WBP 绑定和玩家意图入口。若 RunEvent UI 继续膨胀，再评估是否把 UPROPERTY 缓存所有权迁入专用反射 state USTRUCT，或进一步抽刷新事务 helper |
| WacomApp Public UI API surface | 公开面已完成多轮分类和测试访问收口；当前剩余债务是 prototype / test-only surface、Blueprint-visible 制作面保守保留和资产审计前不删除 | 历史见 [Wacom_Public_Surface_And_Docs_History.md](./DevLog/Wacom_Public_Surface_And_Docs_History.md)；继续按小切片评估，不用“无 C++ 调用”作为删除依据 |

<a id="techdebt-wacomapp-public-ui-api-surface"></a>
### WacomApp Public UI API surface 当前剩余债务

当前原则：

- 正式 WBP 制作合同优先保留：`BindWidget`、`EditDefaultsOnly` 配置、`BlueprintImplementableEvent`、必要 `BlueprintCallable` 和展示 builder 的 `BlueprintPure` 入口。
- C++ public 但不应直接 Blueprint 化的诊断、详情显示或内部协调 helper，后续按资产影响和测试覆盖逐个评估。first-person layer / slot / anchor / Run source 的自动化测试入口已收口为 private + `WacomTests/Private` access wrapper；PlayerController Run drop 的 probe / resolve / submit 测试入口也收口到 private friend + `WacomTests/Private` access wrapper；后续不要新增 public callable `ForTest`。
- `UWacomRunMenuCardLeaseTestMenu` runtime 原型菜单已删除，PlayerController / console runtime 入口同步移除；Actor `ConfigureDebug...Sample`、first-person legacy comparison / prototype preview 等仍只作为 prototype / compatibility / debug 入口保留。
- Blueprint-visible 项即使没有 C++ 调用方，也可能被 WBP 或 `.uasset` 引用；没有资产审计前不作为删除依据。

历史整理记录见 [Wacom_Public_Surface_And_Docs_History.md](./DevLog/Wacom_Public_Surface_And_Docs_History.md)。

---

## 工具链与构建债

| 项 | 临时做法 / 当前决定 | 正式方案 / 处理方向 |
|---|---|---|
| MSVC 工具链 14.38 | 旧引擎阶段曾有 “not preferred” 警告；当前 UE 5.8 构建使用 14.44 | 旧开发机若仍使用 14.38，升级到 14.44+ |
| `RunSession.cpp::ApplySaveGameToRunState` C1001 ICE | 把 `RestoreCardInstanceList` 提取为 anonymous-namespace file-scope free function | MSVC 14.44+ 或 Clang 后视情况合回 lambda |

---

## 已收口的旧临时项

这些条目已写入正式领域文档，后续不要再作为 TODO 重复追踪：

- 中毒穿透护盾，见 `WacomBattle.md §7`
- 中毒触发时机，见 `WacomBattle.md §7`
- 晕厥层数模型，见 `WacomBattle.md §9`
- BattleHUD 创建职责已迁移到 `UWacomGameUIManagerSubsystem`
- BattleHUD coordinator 过重已收口：`UBattleHUD` 保留为 Battle Screen façade，业务状态迁入 `FWacomBattleHUDRuntime`，命令、目标选择、Snapshot presentation、scene enemy、presentation、combat log、first-person hand 和 card detail 分别由 App-private runtime / controller / coordinator 承接；后续修改这些 helper 时优先补 HUD 合同测试并复用 `WacomTests/Private` harness。
- Enhanced Input 入口已由 `UWacomInputContextCoordinatorSubsystem` 统一承接
- 背包容量、A / B 容器、SpecialZone、负重区规则已在 `WacomRun.md §5` 正式化
- RunEvent、Shop、AppToast 的基础链路已在 `WacomRun.md`、`WacomApp.md`、`WacomUI.md`、`WacomUIFoundation.md` 和 Data 专题文档正式化
- 顶层 Backpack / Shop / RunEvent 的 `PlayerController` ScreenClass 配置路径已移除，统一走 Wacom UI Settings -> C++ fallback
- Shop / RunEvent / AppToast 的旧固定 WBP 路径 fallback 已移除；PrimaryLayout 是保留的唯一固定路径 fallback
- `UWacomCardView::BuildFromCardDefinition / BuildDetailFromCardDefinition` legacy static API 已清理，新代码统一使用 `UWacomCardPresentationBuilder`
- Run Definition 级 deck wrappers 已清理；已拥有卡操作统一走 `InstanceId`，Definition 只保留获取 / 奖励 / RunEvent 资产语义。
- Battle 世界空间手牌 prototype public surface 已从 runtime / tests 中移除；正式战斗手牌主线为 first-person card layer。
- Legacy 2D battle hand 已清理：`UHandPanel / UCardWidget`、`WBP_HandPanel / WBP_CardWidget` 和独立 legacy hand 测试已删除；BattleHUD 运行时只走 first-person card layer。
- First-person entry legacy target projection 已清理：`FWacomFirstPersonCardLayerEntry` 正式 interface 只保留 `InteractionIntent`，不再暴露 `DebugLegacyTargetMode`；Battle target-mode 映射只保留在 Battle adapter 私有 helper，Run source 写入 `DragToDropTarget`，hand-card 目标合法性由 Battle validation reject reason 驱动。
- First-person presentation frame 生产写入已收口：Run source 和 Battle hand presentation controller 不再写 legacy `bApplyAsPresentationFrame`，正式路径显式设置 `CommitMode`；该 legacy 字段仅作为旧调用兼容保留。
- Card detail `ChangeLines` legacy text path 已清理：`FWacomCardDetailViewData` 不再暴露未渲染的变化文本字段，Battle target preview 文本旁路也已移除；费用和目标预览表现应走卡面数值、`EffectPreviews` 或正式 `Sections` token。
- Card detail `PassiveLines` legacy mirror 已清理：被动正文只通过正式 `Sections` / passive token line 承载，`Passive.DisplayText` fallback 不再生成平行纯文本字段。
- Card detail `TaskLines` 与扁平 `TokenLines` legacy mirror 已清理：`FWacomCardDetailViewData` 的正式详情文档只保留 `Sections`，后续任务、预览或风味文本应新增正式 section，而不是维护平行数组镜像。
- Card detail `Description` legacy mirror 已清理：`FWacomCardDetailViewData` 不再暴露平行纯文本正文或 `UWacomCardDetailPanel::GetDescriptionText()`；`UCardDefinition::Description` 只作为 Builder 编译“描述”Section 的输入，`FWacomCardViewData.Description` 仍保留为小卡卡面紧凑描述。
- Card detail document builder 已抽出：`UWacomCardPresentationBuilder` 保留为 public / Blueprint facade，`WacomCardDetailDocumentBuilder` 负责详情 `Sections` 组装，`WacomCardDetailTextCompiler` 只负责编译 token line，Widget 不承载 Description / Passive / Effect 分区逻辑。
- Card detail viewport / child widget 创建路径已收口：first-person 详情面板由 `FWacomFirstPersonCardDetailPanelHost` 统一判断是否能进入 viewport，详情 section / token 子 Widget 由 App-private `WacomCardDetailWidgetFactory` 创建，离屏自动化不再通过 `GetWorld()` 分支污染日志。
- BattleHUD 旧 CardDetail immediate positioning helper 已移除：详情面板定位测试和正式路径都以 App-private `FWacomFirstPersonCardDetailMotionController` / `FWacomBattleHUDCardDetailController` 的 stable motion 语义为准。
- AppToast viewport owner 检查已收口：`UWacomAppToastSubsystem` 只在真实本地玩家和 `LocalPlayer` 就绪时创建 / 加入 viewport，离屏自动化注入 Widget 可复用且不再通过 Widget `GetWorld()` 判定旧 World/PC。
- Run GameMenu suppression 查询已收口：PlayerController、Run world interaction router 和 Run first-person drop context 共用 `HasActiveRunGameMenuOrTransitionSuppression()`，drop context 命名也对齐 active GameMenu / viewpoint transition suppression 语义，不再重复解释这两类状态。
- Active GameMenu 弱引用列表维护已收口：`AWacomPlayerController` 用私有 helper 统一清理、移除和查询 active GameMenu widget，后续迁移 Run menu 专属基类时不需要在 suppression / return flow / drop context 里重复解释列表状态。
- Run GameMenu lease / drop ownership 已迁入专用血统：`UWacomRunMenuWidgetBase` 现在持有 owned menu lease、resolve / submit 默认拒绝实现和 deactivation 清理；PlayerController / drop coordinator 只把 `UWacomRunMenuWidgetBase` 当 Run menu owner，旧 `UWacomMenuWidgetBase` API 仅作为 deprecated Blueprint 兼容桥保留。
- Run first-person menu lease contract 已从具体 source component 头拆出：`FWacomRunMenuCardLeaseRequest / Result` 现在位于 `UI/Run/WacomRunMenuCardLeaseTypes.h`，Foundation / PlayerController public 头不再为了纯 lease 数据依赖 `UWacomRunFirstPersonCardSourceComponent` 完整定义。
- Run world target handle 接受条件已收口：`FWacomRunWorldInteractionRouter` 的 cursor probe 和 widget-position probe 共用 `IsAcceptedRunWorldTargetHandle()`，避免 Run world click / hover 与 card-drop probe 对 `TargetKind / TargetTag / WorldTargetId` 的判断漂移。
- Battle Combat Log 合同测试已拆出：`BattleCombatLogSpec.cpp` 覆盖 builder、feed、HUD history 和 controller 合同，统一使用 `Wacom.UI.Battle.CombatLog` 前缀；`BattleWidgetSpec.cpp` 不再承载 combat log 专题。
- Card face view data builder 已抽出：`UWacomCardPresentationBuilder` 不再直接持有小卡卡面字段、紧凑描述、体格/价值展示和效果徽章映射逻辑，`WacomCardFaceViewDataBuilder` 负责生成 `FWacomCardViewData` 和 `FWacomCardViewEffectBadge`。
- Legacy battle event log 已清理：`UBattleEventLogPanel / UBattleEventLogEntryWidget / UEventToast`、`BuildLegacyEventBlock()` 和旧 `WBP_BattleEventLogEntry / WBP_BattleEventLogPanel` 残留资产已删除；正式日志只走 `CombatLogFeed + BattleCombatLogBlock`。
- Enemy system refactor 主链路已收口：旧敌方 2D fallback、第一敌人 HUD 入口、Actor 名称身份推断、旧部位意图序列主合同和旧单 Host Trigger 入口已删除；当前规则和制作口径见 `WacomBattle.md`、`WacomData.md`、`WacomRun.md`、`WacomWorldInteraction.md`、`WacomBattleUI.md` 和 `WacomDataAuthoring.md`。
- Battle exit legacy outcome byte 出口已收口：`AWacomPlayerController::RequestExitBattle()` 公开面改为 `EBattleOutcome`，正式 BattleEnd 主链路仍由 `UBattleHUD::OnBattleEndedNative(EBattleOutcome)` 触发 GameMode，并在 Session 释放前构造 `FBattleResultPacket` 给 Run 层。
