---
type: tech-debt
scope: wacom-current-debt
status: active
updated: 2026-07-18
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
| 回合结束保留 / 弃牌时序 | `BattleTurnLifecycleModule` 权威固定在敌方行动之前 | 若规则明确放在敌方行动之后，需要迁移 Turn Lifecycle 的事件、checkpoint 和 early-exit 合同 |
| Retain Seal 旧反射字段命名 | 为保护已有 Blueprint 序列化，`bEnableRetainedFeedback` 与 `RetainedFeedback*` 继续作为 Anchor `20 Card Retain Seal` 的制作字段，运行时已经改为 Sealing / Held / Releasing | 只有在提供 Blueprint/DataAsset 重命名迁移并完成资产审计时，才统一改名为 `CardRetainSeal*`；当前不要复制第二套字段 |
| `Magnitude.Source.TargetStatusStacks` 资产参数 | Public `FCardEffect` 仍借用 `TargetZone` 传 Status Tag；Private Effect Semantics 已在 decode seam 转成独立 magnitude plan | 只有在允许迁移现有 DataAsset schema 时，新增专用反射字段并提供资产迁移；运行时不再扩散该复用 |
| `Effect.GainKeyword` / `Effect.RemoveStatus` 资产参数 | Public `FCardEffect` 仍借用 `TargetZone`；Private handler 已只消费 typed Keyword / Status 参数，不再存在 `EffectContext::MetaTag` | 只有在允许迁移现有 DataAsset schema 时，拆为专用反射 payload；保持 semantic definition 为唯一解释入口 |
| `Effect.Shuffle.FromBothToOther` HandZone 参数 | 制作校验兼容接受任意 `HandZone.*`，但正式执行始终从 Both 选择；Effect Semantics 显式保留该兼容怪点 | 先确认是否要扩展为通用 FromZone，或收紧资产只允许 Both；确认后同步校验、内容迁移和规则测试 |
| `IsDeleteFunctionAvailable` | 接口已存在，但当前 Run 删牌事务和背包 UI 仍按始终可删的简化口径工作 | 等删牌可用性口径确认后，Run 校验和 UI 显隐统一接入 |
| 手牌锚点左右归属 | `FHandCardSnapshot` 不带左右手角色，UI 用遍历顺序启发式 | 给 `FHandCardSnapshot` 加 `EHandAnchorRole` 字段 |
| BattleResult identity 兼容字段 | `FBattleResultPacket` 仍同时暴露 `PartId / Identity / PartKey`、`DestroyedParts / DestroyedPartKeys`；Run 撤离进度的新写入和 Run 结算日志已收敛为 key-first，GameMode 已删除按部位数量重新推断 Victory；`DestroyedParts` 仅作旧数据 / 手写 snapshot fallback | 继续把测试、日志和消费者迁到 `FBattleEnemyPartKey` / `DestroyedPartKeys`；确认没有资产 / 蓝图依赖后再降级或移除 legacy projection 字段 |

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
| Resolved Encounter 磁盘投影 | Map Node lifecycle 是内存态完成真相；SaveGame v3 仍只持久化 `DestroyedTriggerIds`，GameMode 在非撤离 Victory 成功 settlement 后写兼容投影，Bootstrap 用统一 Trigger 退役入口还原场景 | 下一次 SaveGame schema 升级应持久化 Map Node lifecycle，再从节点状态派生或迁移 Trigger 投影；在新版本迁移完成前不要让 Actor ID 反向成为规则真相 |
| EnemyPart `KnockdownRewardCard` legacy 字段 | 旧 TrainingWarrior、Snake 和其它尚未授权迁移的 Part 仍可让 Aid/Destroy 回退读取同一张卡；新字段与 legacy 混填由 validator 拒绝 | 先迁 TrainingWarrior、Snake 和全部 Production Part 到显式 Aid/Destroy，运行 builder 双跑与 Battle/Run/UI 回归，再用 AssetRegistry/引用审计证明零 legacy 依赖；满足前不得删除字段或 fallback |

---

<a id="techdebt-ui"></a>
## UI 层技术债

UI 当前事实入口见 `WacomUI.md`；CommonUI shell 见 `WacomUIFoundation.md`，Battle UI 见 `WacomBattleUI.md`，first-person card layer 见 `First_Person_Card_Layer_Design.md`。本节只记录仍需替换或收口的临时写法。

| 项 | 临时做法 / 当前决定 | 正式方案 / 处理方向 |
|---|---|---|
| 战斗 UI 全量刷新 | 每次命令后 BattleHUD 从 Snapshot 全量刷新子 widget | 加动画时动画系统自行 diff；数据源仍保持 Snapshot |
| 战斗 UI 不做 ViewModel | BattleHUD 持有 `UBattleSession*`，子 widget 读 `FBattleSnapshot` | UI 复杂度上升或外部 widget 需要战斗状态时，再抽 `UWacomBattleViewModel` |
| C++ 硬编码默认布局 | 部分旧 Widget 仍为 `Blueprintable` 非 Abstract 并带 C++ fallback；Scene Enemy Panel / PartEntry 已迁为 abstract CommonUserWidget + 正式 WBP，不再保留原生完整布局 | 其它界面按业务切片迁移；正式 WBP 后 C++ 只保留协议、ViewData 与生命周期，不重新建立第二套视觉 |
| Battle Widget Session Blueprint 面 | C++ owner 注入已改走 `UWacomBattleWidgetBase.SetInjectedBattleSession / GetInjectedBattleSession`；`SetSession / GetSession` 已降为 C++ only 旧兼容 wrapper，不再暴露给 Blueprint；正式 WBP 制作面只消费 Snapshot / ViewData 并回传玩家意图 | 保留 C++ wrapper 到旧测试和零散调用迁移完成；如外部 widget 需要只读战斗状态，再评估 `UWacomBattleViewModel` / provider |
| HP 条瞬间跳变 | `SetPercent` 直接设值 | 加 `SetTargetPercent` + Tick / 动画插值 |
| 场景敌人表现 polish | 主链路已拆成 `SceneEnemyHost + PartActor + WorldTargetBridge + Presentation`；TrainingWarrior 覆盖 Host Idle / Action / Destroyed，通用 Multi-Part Part Action 已支持精确身份 / Layer、barrier、watchdog 和 Destroyed 抢占；Snake 与 SlimeTrio 的独立 Placeholder 包覆盖三部位 VisualLayers、错帧 Idle 与逐层 Destroyed，但都没有正式美术和 Action Clip | 正式 Snake / SlimeTrio 素材到位后分别替换对应 `/Game/Wacom/Art/Placeholders/Enemies/<Pack>`、删除已知占位包并让 `-FailOnPlaceholder` 通过，再显式制作 Intent Style；继续补多层同步、材质描边、tooltip 和风险动效，只有复杂局部状态确有需要时再接 PaperZD/Animator |
| Floor 1 Production 灰盒表现与出口 | `L_Run_Floor_Main_01` 的四个 SerpentWood Enemy Host 使用受控 Placeholder；`Node.Exit.01` 只有非交互灰盒 marker，没有 FloorId-to-world handoff | 正式美术到位后逐个替换 Host 引用并跑发布 placeholder gate；完成 Production Journey/Floor 2/3 后，由 App flow 实现跨层交接并用正式入口表现替换 marker，禁止在 Level Blueprint 或 marker 中硬编码 travel |
| Floor 1 Preview GameMode 发布阻塞 | `L_Run_Floor_Main_01` 为直接 Editor PIE 暂时覆盖到 `GM_WacomRunFloorPreview`，运行时 Journey 是 Descriptor 派生的 transient 单层对象，无终局、跨层或存档语义 | 完整 `DA_Journey_Main_01`、Floor 2/3 world 与 FloorId-to-world handoff 落地后，把地图切到正式 Production GameMode/启动配置并移除 Preview 依赖；不得把 Preview Blueprint 晋升为发行入口 |
| Scene Enemy Panel 兼容资产命名 | 正式面板仍沿用既有 `BP_WacomBattleEnemyPanelWidget / BP_WacomBattleEnemyPartEntryWidget` 路径，避免本轮重命名波及地图与 Host 资产 | 等地图和 Host 资产可统一重存时，单独迁到 `WBP_` 命名并用 redirect / dependency audit 清理旧路径 |
| 普通 Enemy Panel 部位上限 | 正式 WBP 对 1–4 个部位提供等宽连续分段，双侧详情复用同一 ViewData；超过 4 个只给制作警告并继续运行 | 首个复杂 Boss 内容切片引入专用 Boss WidgetClass；不要把滚动、分页或 Phase UI 塞回普通面板 |
| Enemy Inspection 详情深度 | 当前详情只展示 Snapshot 已有的 HP、Shield、Initiative、Intent 名称/先机/抵抗、Buff 与 Destroyed；不翻译 Intent effects，也不包含 Lore / 图鉴 | 等战斗策划冻结公开信息口径后新增独立 presentation facts；不要让 WBP 解析规则 effect 或直接读取 DataAsset 猜说明 |
| Enemy Intent 图标扩展 | 单部位紧凑面板使用 UI-only `IntentId -> IconBrush` 精确映射，TrainingWarrior 三个 Intent 已有正式图标，未知 Intent 使用星形 fallback；未增加规则字段或基于 effect 的猜测 | 新正式敌人内容包在同一 Style 中显式补映射；若图标数量和主题变体明显增长，再拆按敌人家族/皮肤选择的 Style，不把 Intent 分类或伤害数字塞进本切片 |
| Scene Enemy Host 旧原生面板 override | 老地图/Host 可能序列化了已改为 abstract 的原生 Panel class；`PostLoad()` 只在运行时把这个精确旧值视为未配置并回退项目默认 WBP，不修改 package | 下次有意重存相关地图/Host 时清空旧 override；完成资产审计后删除这条精确兼容分支 |
| CombatLog 纯文字 | 常驻 CombatLog 暂不做图标、颜色、Niagara、音效或动画；旧 EventToast / BattleEventLogPanel / 单事件 legacy bridge 已删除 | 升级为事件表现调度器，接 Niagara、音效、tone 颜色、icon、筛选、事件详情和战后回放 |
| 击倒 Dialog C++ 布局 | CanvasPanel + Border + Button + Aid/Destroy 奖励文本硬编码；行为和连续刷新已由 ViewData 测试覆盖 | 正式 `WBP_KnockdownChoiceDialog` 承接同名 BindWidget 锚点并只调整视觉；不要在 WBP 复制奖励查询、可用性或焦点规则 |
| UI Style 资产命名 V0 | 通用样式资产已迁到 `/Game/Wacom/UI/Style/`，但仍保留 `tiny_menu_Button`、`MyCommonTextStyle` 等原型命名 | 后续设计系统整理时统一命名为语义化 Style asset，例如 `WBPStyle_Button_CommandPrimary` / `TextStyle_CommandButton`，并通过资产审计确认没有旧路径引用后再重命名 |
| Backpack UI C++ 默认布局 | 正式 Screen/Workspace/ZonePile/DeleteConfirm/Style 由 Builder 生成；旧右侧区域栏、缩略 Preview 与 `WBP_BackpackCardView` 生产链已删除。纯 C++ fallback 只构建同一 Workspace Host 和覆盖层，并消费同一 Scene Builder / Registry 数据流 | 保留行为等价 fallback，但不扩展为第二套视觉或输入结构；牌堆美术只修改正式 `WBP_BackpackZonePile`，真实卡面统一复用 `WBP_FPCardView`，不得复制 Workspace 输入或 Run 规则 |
| Shop 长列表 | Shop 已有 snapshot / offer signature dirty gate 和 offer row identity reconcile，但商品列表仍是普通 Panel | 商品量明显上升并有 profiler 证据时再迁 `ListView` / `TileView` 或做正式虚拟化；Shop 正式卡面预览另起切片 |
| 像素风 UI 分辨率适配 | 全局 UI 已统一为 `1920×1080` 封顶 DPI；背包正式 Screen 已移除固定 `1600×900` 子画布并改为 Fill，卡面以独立 ScaleBox 固定 `1.0`、Workspace 位置整数像素对齐，不按高度连续改写比例。桌面 PIE 已覆盖四档目标分辨率、16:10、超宽和 100 cards，用户未观察到布局、采样或明显空闲卡顿问题 | 仅保留跨平台非整数 DPI/不同抗锯齿管线和超过 100 张时的真实风险；出现目标平台证据或 profiler 瓶颈后再决定最近邻资源规范或虚拟化，不凭当前定性验收提前重构 |
| 探索 HUD 时段总节点数 | 只显示剩余节点，没有本时段总节点快照 | `FRunState` 加 `TotalNodeCountForPhase`，或 HUD 在时段切换时记录初始值 |
| AppToast C++ fallback 表现 | 未配置 settings 时仍使用文字 fallback；viewport 创建已受真实本地玩家 / `LocalPlayer` 条件保护，离屏自动化注入 Widget 不进入 viewport | 正式 WBP 后接颜色、图标、动画、音效和全局日志策略 |
| PrimaryLayout 固定路径 fallback | PrimaryLayout 仍允许 settings -> 固定 `WBP_PrimaryGameLayout` 路径 fallback -> null | 资产路径稳定后评估是否也完全转为 settings-only |
| Local Settings 音频分类 | `SC_Wacom_Master / Music / SFX / UI` 和用户 SoundMix 已建立，项目当前没有可明确迁移的 `/Game/Wacom` 音频源资产；未设置 SoundClass 的声音只受 Master 控制，第三方 / 来源不明资产未修改 | 正式音乐、UI 和玩法音效进入 `/Game/Wacom/Audio` 后在导入 / 制作校验中要求显式 SoundClass；用 AssetRegistry 审计存量声音，再按所有权分批迁移，不批改第三方资产 |
| Local Settings 世界材质闪光覆盖 | 全局 Flash 策略覆盖 first-person 卡牌装饰表现；场景敌人 Impact / TargetPreview Niagara 已通过 Dynamic W 与 `DecorativeIntensity` 区分 semantic / decorative，Reduced Motion 关闭位移碎片；其他世界材质和未来 VFX 尚未统一 | 后续世界表现沿用同一 semantic / decorative 参数入口并订阅 Local Settings；不得用全局材质扫描或粗暴关闭所有 emissive，以免隐藏可打 / 不可打、伤害或破坏裂印等语义反馈 |

<a id="techdebt-run-session"></a>
## RunSession 结构债

| 项 | 临时做法 / 当前决定 | 正式方案 / 处理方向 |
|---|---|---|
| `URunSession` 仍承担多个领域流程 | 背包 / 负重 / 永久移除规则、RunEvent、商店、战斗回传、SaveGame、时间、地图、节点活动、Camp 和跨层均已抽到私有 helper/module；初始化和探索命令使用显式 result，UI revision 使用 typed 私有 dirty contract；`RunSession.cpp` 仍是这些领域入口的 public façade 和事务提交 owner | 不再把新规则直接堆入 façade；后续按独立可测领域继续下沉私有 module，但保持 `URunSession` 作为 Run 聚合根和通知 owner |
| Run→Battle 入口 legacy fallback | `AWacomGameMode::EnterBattle` 已拒绝缺失 / 无效 RunSession，不再用 `DefaultCharacter / DefaultRandomSeed` 拼脱离 RunState 的 fallback 战斗；C++ no-trigger `URunSession::BuildInitParamsForBattle(FBattleInitParams&)` 已移除，Run 侧测试改为显式 trigger-aware contract；BlueprintCallable `OnBattleFinished(Packet)` 包装和 `DefaultRandomSeed` 资产兼容字段仍保留，但均已标记 deprecated | 资产审计后再决定移除或重命名 `OnBattleFinished(Packet)` 包装与 GameMode legacy 随机种子字段；新增 C++ 测试默认调用 `OnBattleFinishedFromTrigger` / `BuildInitParamsForBattle(TriggerId, Params)` |

<a id="techdebt-ui-architecture"></a>
## UI 架构债

| 项 | 临时做法 / 当前决定 | 正式方案 / 处理方向 |
|---|---|---|
| ViewModel FieldNotify 未被 WBP 消费 | C++ 父类用 `OnRunViewModelRefreshedNative` 粗粒度多播 + 手动 SetText | 美术阶段 WBP 配 Global Collection Identifier `WacomRunViewModel`，View Bindings 直接绑字段；全 WBP 后删粗粒度路径 |
| `OnRunStateChangedNative` 粗粒度广播 | 组合事务已收口为事务末尾一次广播；普通 public mutation 仍保持成功后一次广播 | 订阅方仍按粗粒度事件幂等刷新；新增组合 Run mutation 时补 `Wacom.Run.NotificationCoalescing` 测试 |
| BattleHUDRuntime helper 剩余膨胀 | 初始化 / 命令结果的 generation、幂等、gate、Snapshot / Log / queue / hand / EndTurn 应用顺序已抽到 App-private `FWacomBattleHUDResultApplicator`，并由 `BattleHUDResultApplicationSpec.cpp` 覆盖；Snapshot presenter、CommandBar presenter、command controller、targeting controller、first-person drop resolver、scene target、presentation、combat log、hand 和 detail 也已分层。`FWacomBattleHUDRuntime` 仍集中持有 helper 生命周期和较宽的 HUD façade 转发面，`BattleWidgetSpec.cpp` 仍承载 fallback layout、event presentation 和其他跨专题旧测试 | Battle result presentation coordination 债已移除。后续只在 Runtime façade 继续膨胀时按 low-risk slice 收窄转发面；测试拆分配合生产重构或降低验证成本推进，不再建立第二条结果应用路径 |
| BattleHUD legacy 点击手牌入口 | `OnCardClickedByUser` 已标记 Blueprint deprecated 并收口为空兼容入口；C++ 自动化测试已不再调用该入口，Content / Config / uproject 粗扫未发现资产引用，`FWacomBattleHUDRuntime` 的 legacy façade 转发已移除 | 后续做一次编辑器资产加载 / Blueprint 编译审计；确认无旧节点后移除 Blueprint 面或保留到下一个兼容清理批次 |
| PlayerController Run world 路由剩余边界 | Battle scene target click / probe 已由 `FWacomBattleSceneInteractionRouter` 承接；数字牌位快捷键只向 BattleHUD 提交 hand index / pointer，不再读取 BattleSession snapshot；Run world hover / click / probe、clickable resolver、hover debug 和 probe preview state 已由 `FWacomRunWorldInteractionRouter` 承接；`AWacomPlayerController` 仍保留 InputKey 顺序、timer、E 键 candidate list、trace seam、RunSession / GameMenu ownership 和 Run card drop coordinator context 注入 | 暂不继续抽 E 键 candidate list 或 Run world card drop 提交流程。若后续 Run card drop target adapter 继续膨胀，再优先在 `FWacomRunFirstPersonCardDropCoordinator` 内拆 adapter / transaction helper，保持 `URunSession` 规则事务和 Widget passive 边界 |
| First-person card 输入仲裁分散 | `NoCapture` 下快捷键拖拽的右键取消已由 `UWacomGameViewportClient` 生命周期内的 Slate Input Processor 可靠接入；数字快捷键启动、external pointer pump、左键 release、回合边界取消和 fallback 仍分别位于 PlayerController、ViewportClient 与 Run drag controller | 后续独立切片新增 App-private `FirstPersonCardInputRouter`，统一取消 / 释放 / 快捷键重选的手势语义；Slate、Enhanced Input、UMG 和 PlayerController 只作为 adapter，不建立全局超级输入管理器，也不改变 Layer 的 gesture truth ownership |
| `UWacomMenuWidgetBase` 上的 Run first-person 兼容桥 | Backpack / Shop / RunEvent 已继承 `UWacomRunMenuWidgetBase`，生产 lease / drop ownership、PlayerController owner lookup 和 drop coordinator submit/resolve 都只走 Run 专用父类；Foundation 菜单基类上的旧 `SetOwnedRunFirstPersonCardLayerMenuLeaseFromRunCards`、Run menu drop resolve / submit 钩子已标记 deprecated，只转发到 Run base 或返回 unsupported，保留给旧 Blueprint 节点编译；2026-07-08 Blueprint compile audit 中 `Content/Wacom/Test/WBP_MenuTest.uasset` 仍报告 deprecated 节点警告 | 后续先审计 / 修复直接引用 Foundation 基类旧节点的资产（已知 `Content/Wacom/Test/WBP_MenuTest.uasset`），确认可 reparent / 删除后再移除 Foundation deprecated API，让 MainMenu / Pause / Confirm 等通用菜单彻底不暴露 Run 节点 |
| First-person card layer 巨型 spec | 生产侧已拆出 Gesture Controller、Interaction Playback、Presentation Controller、Readiness Coordinator、Surface Arbiter 与 Cue Painter；语义 Playback、Ready/Surface 所有权及其瞬态编排状态已归入 App-private Controller。PlayableFeedback、CardDragInspect、DragTargetFeedback、手势状态和 Slot 配置测试已经迁入独立 Spec，旧配置标准化组也已从 `FirstPersonCardLayerSpec.cpp` 移除。巨型 Spec 目前主要仍承载 anchor/layout、runtime source 与跨层 Slot motion 集成场景 | 后续优先迁移 `RuntimeSource` 与 `SlotMotion` 专题；继续复用现有 tests-only interaction fixture，不得为了拆文件复制整套 HUD/World 装配 |
| First-person card layer motion / surface effect 剩余职责 | Hover/Drag fake-3D、Inspect scrub、Battle 无目标 drag-out、Battle/Run 镜头跟随、Transition Audio、Gained/Retained 语义运动和手牌目标 lift/scale/ZOrder 已恢复；正式 Drag 使用一次性拾牌反馈，持续阶段不再播放 Selection。PlayCard 已使用命令级阶段编排，费用 Preview/Rewrite 通过明确 `CardRuntimeCostChanged / CardStatusChanged` 许可事件与 Snapshot 差异接入 `CostDigitImage` 双 Atlas MID；Gained 已使用 `CardGainedResolved` checkpoint 和正面像素结晶 Surface，能在手牌上限弃置前完整显示入手。像素棱镜 helper、Selection Style 和纹理源仍作为实验基础保留，但不在生产基础材质或 Slot 常驻 Tick 中运行 | 继续 PIE 验收 Source/Target/Outcome/Return、Gained 结晶 -> HandLimit 弃置、不同稀有度边缘、快速连续改费、BattleEnd/阻塞选择边界、视差/反光和 DPI。Gained 来源目前仍是通用 authored origin；若以后需要从具体敌人部位或奖励面板投射，应新增 App 表现来源合同，不能让 Layer 读取击倒规则。卡名、耐久、Value、效果徽章、插画、稀有度与 Run 升级尚无统一权威变化事务；后续应逐字段建立明确事件 / transaction |
| First-person card Shipping PSO 冷启动 | Battle/Run 已在来源 staging 异步驻留 Soft Sprite / Enter Sound，并提前建立 Cost/Badge 局部 MID；真实 Paint Generation Gate 也保证首播不跳帧。但 Editor PIE 不能代表 Win64 Shipping 的材质 PSO 命中与驱动缓存表现 | 独立切片对 Win64 Shipping 做冷进程、冷 DDC 与不同显卡驱动测量，收集基础 Fake3D、所有活动 Surface、Cost/Badge 与敌人 Niagara 材质 PSO，建立稳定缓存并验证打包版本；不要用延长动画或全局钳制 DeltaTime 掩盖 PSO stall |
| DreamShader 1.4.1 UE 5.8 / ComponentMask 本地补丁 | 上游 1.4.1 验证范围截至 UE 5.7；项目在 `DreamShaderSettings.cpp` 为 Runtime/Game 目标增加 `WITH_METADATA` guard，避免非编辑器构建调用不可用的 `UEnum::HasMetaData`。另在 `DreamShaderMaterialGeneratorCodeUE.cpp` 让 `UE.Expression(ComponentMask)` 新节点先清空 Unreal 默认启用的 R/G，再应用 DSL 通道参数，使文档中的 `R=true` 以及项目使用的 `B=true / A=true` 都是单通道语义；`SwizzleInputMasks / ExplicitComponentMaskChannels` 已覆盖普通 swizzle、Named Reroute 与显式 mask | 上游发布正式 UE 5.8 或 ComponentMask 修复后逐项对比并优先移除等价本地补丁；以后更新 DreamShader 时必须重跑三目标 BuildPlugin、完整 `DreamShader` 自动化，并重新生成 / 验证 first-person fake-3D 与 shadow 材质 |
| RunEventScreen 剩余协调职责 | 选项行 / 支付 DropTarget identity reconcile 已抽到 `FWacomRunEventChoiceListReconciler`；payment drop resolve / submit 已抽到 `FWacomRunEventPaymentDropFlow`；menu lease request 构造已抽到 `FWacomRunEventPaymentLeaseBuilder`；debug view / summary 已抽到 `FWacomRunEventScreenDebugBuilder`；cached choices 和支付 Zone 映射查询已收口到 `FWacomRunEventPresentationStateView / Edit`；Screen 仍持有刷新提交、关闭流程和 UPROPERTY 缓存所有权 | 保持 Screen 作为刷新、WBP 绑定和玩家意图入口。若 RunEvent UI 继续膨胀，再评估是否把 UPROPERTY 缓存所有权迁入专用反射 state USTRUCT，或进一步抽刷新事务 helper |
| WacomApp Public UI API surface | 公开面已完成多轮分类和测试访问收口；当前剩余债务是 prototype / test-only surface、Blueprint-visible 制作面保守保留和资产审计前不删除 | 历史见 [Wacom_Public_Surface_And_Docs_History.md](./DevLog/Wacom_Public_Surface_And_Docs_History.md)；继续按小切片评估，不用“无 C++ 调用”作为删除依据 |

2026-07-13 自动化基线：完整 `Wacom.UI.FirstPersonCardLayer` 共 268 项全部通过。此前 21 项旧合同漂移已收口：Development Preview / empty runtime fallback 改为正式 runtime source 空状态；删除 Preview toggle、箭头吸附和整卡 commit-ready overlay 三项无效测试；其余 drag-target 测试只保留 target identity、validity、focus、清理和中性箭头指针跟随；`Gained` 使用独立语义；玩家蓝图默认 Card Use Style 修回 Pixel Edge Flip。生产 `FWacomFirstPersonCardDragConfig` 不再保留仅供旧测试编译的颜色、overlay、probe scale 与 snap 字段。

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
| DreamShader 旧材质 generated include 失效 | 允许 commandlet rendering 的全工程 shader 检查会因 `M_CardSurface_CosmicFoil` 与 `M_CosmicBlob` 引用已失效的 `/DreamShaderGenerated/*.ush` 失败；新 first-person OrderedDither 已单独完成 PCD3D_SM6 生成且没有自身编译错误。2026-07-16 已验证仅对 `M_CardSurface_CosmicFoil.dsm` 执行 `-Force` 重生成不足以清除资产缓存警告，但正式材质仍可加载，first-person card layer 282 项测试通过，Blueprint 全量编译为 0 error / 0 failed load | 对两个旧 `.dsm` 执行 **clean generated shaders + 全量强制重生成**，确认资产不再缓存旧虚拟 include；随后恢复全工程 AllowCommandletRendering shader gate |
| `L_Exploration` Construction Script MID 序列化漂移 | `BP_RunTunnelGrassPaperLayer` 的 33 个实例会在编辑器载入 / Save All 后把动态材质实例名称、父材质、纹理参数和 LightingGuid 重新写入地图包；2026-07-17 因同一地图还包含用户明确删除旧 Plane 的修改，本轮保留当前资产值，不能把整张地图当生成残留恢复 | 将纸片层的 MID 创建和参数写入收口为不持久化的 transient/runtime 表现状态，避免 Construction Script 把动态实例作为地图制作数据；完成后对同一基线连续载入并保存两次，要求 `L_Exploration.umap` LFS 哈希稳定。收口前集成地图必须做 T3D 语义审计，不以二进制大小或 Save All 结果直接判断 |
| Exhausted 消散旧 `PlayedDissolve` C++ 命名 | 为避免破坏已制作 DataAsset 的反射字段和序列化数据，`CardPlayedDissolveStyle`、相关 Style 类型及资产名暂保留；运行时已经只由 `Exhausted` 语义驱动 | 后续完成资产重命名 / redirector / Blueprint compile audit 后，统一迁移为 `CardExhaustedDissolve*`，再删除兼容名称；不得在此之前复制第二套 Style 数据 |
| Worktree 依赖 ignored Content | 开发期由 `InitializeWacomWorktree.ps1` 仅为 Art / Asset 建立独立 D 盘依赖层；`Content/DreamMaterials` 现有 61 个 `.uasset` 已整体由 Git LFS 管理。2026-07-16 只读 AssetRegistry 基线只剩 `/Game/DreamMaterials/M_Card_Step2_Inst` 没有本地磁盘资产 | 按 [`Content_Dependency_Audit.md`](./Content_Dependency_Audit.md) 继续确认 Art / Asset 所有权、登记第三方 manifest，并处理 `M_Card_Step2_Inst` 的旧引用或重建；最终删除对主工程 seed source 的依赖并启用外部依赖 gate |
| 卡牌核心表面缺少独立 Finish Mask | 第一版分层视差使用现有插画 / Frame / Rarity 的 RGB 与 Alpha 程序化计算高光、金属明暗和虹彩，不要求内容团队补贴图 | 美术资产稳定后评估一张可选的打包 Mask（例如 R=金属、G=箔片、B=凹凸、A=虹彩）；只在能显著提升不同主题复用时加入 MI，不把单一卡牌特例写进规则数据 |
| EffectBadge Added/Removed 缺少正式规则事务 | App 已有稳定 `PresentationKey`、ValueChanged/Added/Removed Hint、局部 Playback、移除后重排与新增展开能力；当前生产触发只接受 `CardRuntimeCostChanged / CardStatusChanged` 许可下的可见 ValueChanged，不从普通 Snapshot 数组差异推断增删 | 规则或升级系统引入动态 Effect 增删时，先定义明确事件/事务、来源与稳定 EffectIndex，再由 Battle Presentation 生成 Added/Removed Hint；随后补真实旧槽位几何缓存与 PIE 验收，禁止 Widget 自己把数组变化解释成规则事实 |

---

## 已收口的旧临时项

这些条目已写入正式领域文档，后续不要再作为 TODO 重复追踪：

- Run 探索原型路径已收口：正式规则由 Journey/Floor Logical Map Graph、Action Point、节点 activity ticket、Camp 和 Floor Transition module 承接；App 只保留 `UWacomRunPathTraversalComponent`、Path Segment / Branch / NodeAnchor 与 Scene Registry。旧移动 / Segment / Branch 原生类、Blueprint 资产和 Config redirect 已删除，独立纸片美术 Actor 不参与移动规则。
- Run 初始化兼容面已收口：PlayerController 和测试 fixture 统一提交 `FRunInitializationParams` 并消费 `FRunInitializationResult`，失败不破坏旧 Session；SaveGame schema 3 明确不序列化 exploration state。
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
- Card detail `ChangeLines` legacy text path 已清理：`FWacomCardDetailViewData` 不再暴露未渲染的变化文本字段，Battle target preview 文本旁路也已移除；费用和目标预览表现应走卡面数值、`EffectPreviews` 或正式 `Sections` semantic document。
- Card detail `PassiveLines` legacy mirror 已清理：被动正文只通过正式 `Sections` / explanation blocks 承载，`Passive.DisplayText` fallback 不再生成平行纯文本字段，也不再作为详情面板输入。
- Card detail `TaskLines` 与扁平 `TokenLines` legacy mirror 已清理：`FWacomCardDetailViewData` 的正式详情文档只保留 `Sections`，后续任务、预览或风味文本应新增正式 section，而不是维护平行数组镜像。
- Card detail `Description` legacy mirror 已清理：`FWacomCardDetailViewData` 不再暴露平行纯文本正文或 `UWacomCardDetailPanel::GetDescriptionText()`；`UCardDefinition::Description` 只在没有任何结构化详情 section 时作为普通正文回退，不解析旧占位，也不重新成为规则说明入口。
- Card detail explanation system 已收口：`UWacomCardPresentationBuilder` 保留为 public / Blueprint facade，`WacomCardDetailDocumentBuilder` 负责详情 `Sections` 组装，`WacomCardExplanationCompiler` 负责 block / section 组装，`WacomCardExplanationTemplateResolver` 负责词典模板选择与 fallback 模板，`WacomCardExplanationTemplateRenderer` 负责 typed slot 到 runs 的转换，Widget 不承载 Description / Passive / Effect 分区逻辑。
- Card detail MagnitudeModifiers 展示已收口：`FCardEffect.MagnitudeModifiers` 由 App-private explanation helper 生成静态条件 / 数值修正说明；最终数值仍只来自 Battle preview facts 或静态字段，Widget / UI builder 不执行规则判断。
- Card explanation lexicon 读取已收口：`WacomCardExplanationLexiconProvider` 负责从 UI Settings 读取 `CardExplanationLexicon` 并按软对象路径缓存已加载资产，`WacomCardDetailDocumentBuilder` 不再每次详情构建时直接同步加载 settings asset。
- Card detail theme 读取已收口：`WacomCardDetailThemeProvider` 负责从 UI Settings 读取 `CardDetailTheme` 并按软对象路径缓存已加载主题，`UWacomCardDetailSectionWidget` 不再每次刷新时直接同步加载 settings asset。
- Card detail viewport / child widget 创建路径已收口：first-person 详情面板由 `FWacomFirstPersonCardDetailPanelHost` 统一判断是否能进入 viewport，详情 section 子 Widget 由 App-private `WacomCardDetailWidgetFactory` 创建，离屏自动化不再通过 `GetWorld()` 分支污染日志。
- 旧 card detail token flow 已删除：`UWacomCardDetailTokenFlowWidget / TokenLineWidget / TokenWidget` 和 `WBP_CardDetailTokenFlow / WBP_CardDetailTokenLine / WBP_CardDetailToken` 不再是运行时依赖；避免留下指向已删除 native 类的坏资产。
- BattleHUD 旧 CardDetail immediate positioning helper 已移除：详情面板定位测试和正式路径都以 App-private `FWacomFirstPersonCardDetailMotionController` / `FWacomBattleHUDCardDetailController` 的 stable motion 语义为准。
- AppToast viewport owner 检查已收口：`UWacomAppToastSubsystem` 只在真实本地玩家和 `LocalPlayer` 就绪时创建 / 加入 viewport，离屏自动化注入 Widget 可复用且不再通过 Widget `GetWorld()` 判定旧 World/PC。
- Run GameMenu suppression 查询已收口：PlayerController、Run world interaction router 和 Run first-person drop context 共用 `HasActiveRunGameMenuOrTransitionSuppression()`，drop context 命名也对齐 active GameMenu / viewpoint transition suppression 语义，不再重复解释这两类状态。
- Active GameMenu 弱引用列表维护已收口：`AWacomPlayerController` 用私有 helper 统一清理、移除和查询 active GameMenu widget，后续迁移 Run menu 专属基类时不需要在 suppression / return flow / drop context 里重复解释列表状态。
- Run GameMenu lease / drop ownership 已迁入专用血统：`UWacomRunMenuWidgetBase` 现在持有 owned menu lease、resolve / submit 默认拒绝实现和 deactivation 清理；PlayerController / drop coordinator 只把 `UWacomRunMenuWidgetBase` 当 Run menu owner，旧 `UWacomMenuWidgetBase` API 仅作为 deprecated Blueprint 兼容桥保留。
- Run first-person menu lease contract 已从具体 source component 头拆出：`FWacomRunMenuCardLeaseRequest / Result` 现在位于 `UI/Run/WacomRunMenuCardLeaseTypes.h`，Foundation / PlayerController public 头不再为了纯 lease 数据依赖 `UWacomRunFirstPersonCardSourceComponent` 完整定义。
- Run world target handle 接受条件已收口：`FWacomRunWorldInteractionRouter` 的 cursor probe 和 widget-position probe 共用 `IsAcceptedRunWorldTargetHandle()`，避免 Run world click / hover 与 card-drop probe 对 `TargetKind / TargetTag / WorldTargetId` 的判断漂移。
- Backpack 旧单卡 UMG DragDrop 路径已清理：`WacomCardDragOperation`、`WacomZoneDropTarget`、`WacomDeleteZoneDropTarget` 及其旧测试 access 已删除；移动/销毁只保留 Workspace -> Screen -> batch command flow，迁移 Host 仅作只读 fallback。
- SpecialZone 入战 toggle 规则和 affordance 已收口：`ValidateSetSpecialZoneCardBattleEnabled()` / `ValidateToggleSpecialZoneCardBattleEnabled()` 通过 `FRunDeckOperationValidation.DisabledReason` 暴露 `CardNotFound / NotInSpecialZone` 等稳定 reason，`FRunStorageCardView` 显式携带右键入战和角标 ViewData，App command flow 只提交 toggle 意图并做 Toast 呈现，不再用 `FindInstance()` 或列表来源自行推断规则失败 / affordance。
- DeckCard 右键 toggle setter public 面已收口：生产和测试入口都通过 `SetStorageCardView(FRunStorageCardView)` 表达右键入战 affordance，`SetRightClickToggleEnabled()` 只作为 `UWacomDeckCardWidget` 私有实现细节保留。
- RunEvent 普通选项提交权威已收口：`FWacomRunEventScreenFlow::ChooseChoice` 不再用 cached `FRunEventChoiceSnapshot::bAvailable` 作为提交前 veto，普通点击始终调用 `URunSession::ChooseRunEventOptionWithResult()` 并消费 `FRunEventChoiceResult`。
- Run Deck 移动事务已收口：`URunSession::MoveInstance()` 不再手写源/目标 zone mutation，已通过校验后的物理区移动、SpecialZone battle flag 清理、B 主卡 entry 保底和负重重算由 `FRunDeckRules::MoveInstance()` 统一承接；显式移动到负重区会刷新负重压力但不立即回填该卡。
- Run Deck 操作失败 reason contract 已收口：`FRunDeckOperationValidation.DisabledReason` 仍保持 `FName` 兼容面，但生产代码通过 `WacomRunDeckOperationReasons` 获取 deck 原始失败码，RunEvent / UI 可在各自语境内翻译展示文案，不再依赖散落字符串拼写。
- Battle Combat Log 合同测试已拆出：`BattleCombatLogSpec.cpp` 覆盖 builder、feed、HUD history 和 controller 合同，统一使用 `Wacom.UI.Battle.CombatLog` 前缀；`BattleWidgetSpec.cpp` 不再承载 combat log 专题。
- Card face view data builder 已抽出：`UWacomCardPresentationBuilder` 不再直接持有小卡卡面字段、紧凑描述、体格/价值展示和效果徽章映射逻辑，`WacomCardFaceViewDataBuilder` 负责生成 `FWacomCardViewData` 和 `FWacomCardViewEffectBadge`。
- Legacy battle event log 已清理：`UBattleEventLogPanel / UBattleEventLogEntryWidget / UEventToast`、`BuildLegacyEventBlock()` 和旧 `WBP_BattleEventLogEntry / WBP_BattleEventLogPanel` 残留资产已删除；正式日志只走 `CombatLogFeed + BattleCombatLogBlock`。
- Enemy system refactor 主链路已收口：旧敌方 2D fallback、第一敌人 HUD 入口、Actor 名称身份推断、旧部位意图序列主合同和旧单 Host Trigger 入口已删除；当前规则和制作口径见 `WacomBattle.md`、`WacomData.md`、`WacomRun.md`、`WacomWorldInteraction.md`、`WacomBattleUI.md` 和 `WacomDataAuthoring.md`。
- TrainingWarrior 已验证通用 Simple Host 正式内容管线；Snake 受控占位包已验证首个 Multi-Part Host、三段局部 Destroyed 和发布 Placeholder gate。通用 Multi-Part 单主层行动动画已经完成，剩余债务是正式 Snake 美术与 Intent Clip、确有需求时的多层联动 / Boss 状态机、材质描边、tooltip 和 PaperZD。不要把这些能力塞回 TrainingWarrior 的单 Host Flipbook Style，也不要让新的敌人运行时重新依赖 ignored `/Game/Art`。
- 敌人行动已具备 Journal 前后 Combat facts、Clip Impact marker、严格 Impact → Complete barrier，以及 PlayerStatusBar V2 的 HP 延迟伤害条、护盾受击/破裂、低血和 Action Preview 区段。剩余表现债是正式 Damage/Shield 音频资产、受击方向和确有必要时的镜头/屏幕反馈；这些能力应继续消费同一 Impact 语义，不得重新从最终 Snapshot、事件名称或动画时间猜命中。
- Resolved Encounter 场景生命周期已收口为 `Run settlement -> Trigger pending -> return barrier -> Host/Part retirement`；HUD 不销毁探索 Actor，GameMode 不再按部位数推断 Victory。剩余存档债务只是 v3 仍以 `DestroyedTriggerIds` 投影未持久化的 Map Node lifecycle。
- Battle exit legacy outcome byte 出口已收口：`AWacomPlayerController::RequestExitBattle()` 公开面改为 `EBattleOutcome`，正式 BattleEnd 主链路仍由 `UBattleHUD::OnBattleEndedNative(EBattleOutcome)` 触发 GameMode，并在 Session 释放前构造 `FBattleResultPacket` 给 Run 层。
