---
type: orchestration-spec
scope: wacom-app
status: active
updated: 2026-07-18
tags:
  - wacom/app
  - wacom/gameflow
  - wacom/input
  - wacom/world-interaction
---

# WacomApp 模块文档

> [!info] 本文职责
> 本文是 WacomApp 的 App orchestration 当前事实入口。它记录 GameMode、PlayerController、PlayerCharacter、输入协调、世界交互路由和战斗进出流程。

> [!note] 边界
> UI 总原则和 Screen 摘要见 [WacomUI.md](./WacomUI.md)；CommonUI shell / Settings / AppToast 见 [WacomUIFoundation.md](./WacomUIFoundation.md)；BattleHUD 见 [WacomBattleUI.md](./WacomBattleUI.md)。世界交互 Actor 制作、target handle、debug / validation 约定见 [WacomWorldInteraction.md](./WacomWorldInteraction.md)。

## §1 模块职责

`WacomApp` 是游戏主模块和表现协调层（`PRIMARY_GAME_MODULE`）。

负责：

- `AWacomGameMode / AWacomMenuGameMode / AWacomPlayerController / AWacomPlayerCharacter`。
- 探索与战斗的 game flow 状态切换。
- 输入上下文协调、Enhanced Input profile、CommonUI input config、鼠标显隐和 click / mouse-over lease。
- 世界交互路由：E 键最近候选、鼠标 hover、左键点击、Battle scene target、Run world card drop、Run menu zone drop。
- UI shell 调用入口：确保 PrimaryLayout、Push / Pop HUD 或菜单；具体 shell 行为见 `WacomUIFoundation.md`，Screen 数据流见 `WacomUI.md`。
- first-person runtime source 所有权协调：探索期 Run Card Workspace 展示、GameMenu lease、进入战斗时清理、退出战斗后恢复。

不负责：

- 单场战斗规则真相；战斗命令必须提交给 `UBattleSession`。
- Run / 背包 / 商店 / RunEvent 规则真相；战外事务必须提交给 `URunSession`。

背包自由工作台的布局 owner 是 `UWacomBackpackWorkspaceStateSubsystem`：按 `URunSession` 对象身份保存活动区域和 `ZoneKey -> InstanceId -> normalized layout`，同一 Run 关闭/重开保留，新 Run 或 GameInstance 结束清空。该状态不序列化；选择、框选、水平牌列携带、确认暂停和鼠标捕获属于 Screen 活跃期 interaction model，Deactivate 时清理。
- 静态数据资产定义；卡牌、敌人、商店、RunEvent 等资产契约见 `WacomData.md`。
- WBP 视觉细节、卡牌展示规则和 UI 子控件刷新模型；这些见 `WacomUI.md`、UI 专题文档和 Binding 文档。

App / UI 对玩家已拥有卡提交精确 `InstanceId`，不以 Definition 指代某张具体卡；DataAsset / RunEvent 表达“某种卡”的资产语义时例外。

---

## §2 关卡与 GameMode

| 关卡 | GameMode | 职责 |
|---|---|---|
| `L_MainMenu` | `AWacomMenuGameMode` | 主菜单，不 Spawn 探索 Pawn；注入主菜单 ViewData，并处理菜单 Action、确认、退出和 travel |
| `L_Exploration` | `AWacomGameMode` | 探索主流程，持有 GameFlowState，进入 / 退出战斗，初始化探索 HUD |
| `L_Run_Floor_Main_01` | `GM_WacomRunFloorPreview`（仅 Editor PIE） | Floor 1 Production 灰盒与本地 Scene Binding/交互验证；不是当前主菜单或发行启动目标 |

新 Run 初始化统一经过 `AWacomGameMode::ResolveJourneyDefinitionForNewRun()` 这一非反射 C++ seam。基础实现仍精确返回 `DefaultJourneyDefinition`，因此 `L_Exploration`、Authoring、Debug 和未来 Production Journey 的现有配置语义不变；`AWacomPlayerController` 只消费解析结果并交给 `URunSession::Initialize()`，不自行推断关卡或 Floor。

`AWacomRunFloorPreviewGameMode` 只在 Editor PIE 中工作。它从当前 World 唯一 `AWacomRunFloorSceneDescriptorActor` 读取精确 Floor，构造由 GameMode 持有的 `RF_Transient` 单层 Journey：`JourneyId=Journey.Preview.<FloorId>`、标题为 `[Preview] <Floor DisplayName>`、角色来自 Preview GameMode 配置，`SuccessTerminalNode` 为空，AP/Decay 沿用 Journey 默认值。同一次 PIE 重复解析返回同一实例；Descriptor 缺失/重复、Floor 无效、角色缺失、非 PIE 环境或解析后的 Descriptor/Floor 漂移都 fail closed。它不写 DataAsset、不改变 `WacomRun` 合同，也不能作为 Standalone、发行或主菜单启动路径。

当前 `EGameFlowState` 包含：

- `Exploration`：允许 Run Path 移动、世界交互、打开暂停 / 背包 / 商店 / RunEvent 等 GameMenu。
- `Battle`：暂停 Run Path 移动，启用 Battle camera look，切换输入 profile，并 Push `UBattleHUD` 到 Game 层。
- `JourneySummary`：terminal success 后的只读总结与主菜单交接阶段；不恢复 Run hand、交互 Toast 或探索写入口。

`UWacomExplorationHUD` 使用 `GameAndUI + NoCapture` 保持鼠标可见和世界点击；每次激活后会等待 CommonUI 完成本轮 input config，再在下一帧把 Slate 焦点交给游戏视口。失活或销毁会取消待执行请求，因此首次进入嵌入式 PIE、战斗返回探索或关闭 GameMenu 后都不需要额外点击视口才能收到 W/S 与鼠标视角输入，也不会让过期 HUD 在页面切换后抢回焦点。Cursor Look 优先使用 PlayerController 的视口鼠标坐标；嵌入式 PIE 尚未产生第一次 viewport mouse event 时，回退到 Slate 光标与 viewport geometry 的相对坐标，不以一次点击作为视角激活条件。

当前 `AWacomGameMode::bSaveSystemEnabled == false`，自动存档和读取路径会静默 no-op。SaveGame schema 与恢复边界见 [WacomRun.md](./WacomRun.md)。

### Local Settings 启动与事务

本地机器设置由 `UWacomGameUserSettings` 持久化到 UE 的 `GameUserSettings.ini`，与玩家档案、活动旅程、滚动备份和 `WacomRunSaveGame` 完全分离。`DefaultEngine.ini` 通过 `GameUserSettingsClassName` 注册该类型；Wacom 自定义 schema 当前为 `1`。项目平衡档是首次启动和显式恢复默认的唯一来源：当前显示器桌面分辨率、无边框窗口、VSync 开、60 FPS、高画质、四路音量 / 视角响应 / 镜头运动 100%、不反转 Y、完整闪光与完整 UI 动效；无法取得桌面分辨率时先保留当前有效分辨率，再回退 `1280 × 720`。已有有效 ini 不会在启动时被覆盖；自定义 schema 无法迁移时只重置音量、视角响应、镜头运动和表现辅助字段，不清空 UE 持有的分辨率、窗口模式或画质数据。

桌面主视图的项目级抗锯齿默认固定为 UE 5.8 `SMAA High + Color Edge`：`r.AntiAliasingMethod=5`、`r.SMAA.Quality=2`、`r.SMAA.EdgeMode=0`。该非时域路径优先保持 Paper2D、像素化场景反馈和快速第一人称镜头下的清晰边缘，不使用 TAA / TSR 的历史累积；移动端仍单独保持 `r.Mobile.AntiAliasing=1`（FXAA），本项不作为玩家本地设置暴露。

`UWacomSettingsSubsystem` 是唯一编辑事务 owner：`BeginEdit()` 返回唯一 token 和当前 snapshot；`Preview()` 只即时应用四路音量、视角响应 / Y 反转、镜头运动、闪光模式和 UI 动效模式；显示、VSync、帧率上限与整体画质只在 `Apply()` 时生效。错误 token、过期 token、重复编辑、重复确认和重复撤销都被拒绝且无副作用。`Cancel()` 恢复编辑前的即时预览值且不写盘。

分辨率或窗口模式变化后进入 15 秒确认态。此时引擎已应用候选视频模式，但不会调用 `SaveSettings()`；`ConfirmVideoMode()` 才确认并保存，`RevertVideoMode()` 或超时会恢复最后确认的视频模式，再保存同批次的其它设置。因此崩溃或强退不会把未确认的不可用分辨率写成下次启动配置。

分辨率选项由 `UWacomSettingsSubsystem::GetScreenResolutionOptions()` 统一生成，Screen 不直接查询平台显示模式，也不把任意当前值无条件塞回列表。项目最低支持 `1280 × 720`，固定常用档位为 `1280 × 720 / 1366 × 768 / 1600 × 900 / 1920 × 1080 / 2560 × 1440 / 3840 × 2160`：独占全屏只保留显卡和显示器实际报告支持的档位，窗口模式只保留不超过当前桌面工作区的档位；桌面原生分辨率和当前有效自定义分辨率只有在满足最低尺寸及对应模式限制时才作为额外选项保留。无边框窗口固定跟随当前桌面并禁用分辨率行；没有合法候选项时只禁用该行，不阻断其它设置。旧配置低于最低尺寸时可以显示原值，但不会继续作为选择目标，玩家首次步进会进入合法档位。

正式游戏窗口禁止玩家使用窗口边框任意缩放，也不提供最大化按钮；最小化和关闭按钮保留，窗口最小尺寸与项目最低支持视口统一为 `1280 × 720`。普通窗口的尺寸只能通过 Settings 的分辨率目录与 Apply 流程改变，无边框和独占全屏继续遵循各自的视频模式语义，因此鼠标拖拽、系统最大化不会绕过白名单或 15 秒确认事务。该项目级 Slate 窗口合同不覆盖编辑器拥有的 PIE 窗口。

Subsystem 在 GameInstance 初始化时应用已保存的非分辨率配置，并在 Game / PIE World 就绪时应用音频总线；运行时消费者通过 native settings-changed delegate 订阅并在 teardown 时反订阅，不使用 Tick 查询设置。Run Path Traversal 使用鼠标位置驱动视角，只叠加响应倍率和 Y 反转；镜头运动倍率同时缩放 Walk Bob 与 Camera Shake，`0` 会输出零 Bob 并停止已启动 Shake。`BP_WacomPlayerCharacter` 当前在 `UWacomRunPathTraversalComponent` 上启用已有 `BP_RunTunnelWalkCameraShake` 资产，并保持 `WalkBobComponent.bEnableWalkBob=false`，因此默认、半强度和关闭三档都作用于唯一的 CameraShake 路径，不叠加两套晃动。

`UWacomSettingsScreen` 是这套事务的唯一 CommonUI 页面协调器。页面覆盖显示、图形、音频、视角、辅助五类字段；App-private 字段描述表统一标签、离散 / 连续类型、格式与 Preview 白名单。Screen 在事务开始、窗口模式变化、恢复默认以及确认 / 回滚后的新事务中重新取得模式专属分辨率目录；切换模式时若当前 Draft 不合法，会在本地选择宽高距离最近的合法档位，不提前应用引擎视频模式。无边框窗口下分辨率仍显示，但禁用并标注“跟随桌面”。“恢复默认”从 Subsystem 获取完整项目平衡档，使用当前 token 只预览允许即时预览的字段并装入 Draft，不弹额外确认框、不会直接写盘；玩家仍需 Apply，视频变化继续走 15 秒确认。Apply 无视频模式变化时保存后留在页面并开启新 token；有视频模式变化时 Push `UWacomSettingsConfirmationDialog`，保留、恢复、超时和 Modal Push 失败都回到实际 Snapshot 并开启新 token。脏状态返回会先确认放弃；外部 teardown 会 Cancel 活动 edit 或 Revert 待确认视频模式。

主菜单与暂停菜单都只调用 App-private `FWacomSettingsScreenFlow`。该 flow 统一解析 `UI.Widget.SettingsScreen` 的软类、异步 Push 到 `UI.Layer.GameMenu`、拒绝重复打开，并在资产缺失时回退 `UWacomSettingsScreen`。主菜单 ViewData 现在开放 Settings，Journey History / Credits 仍隐藏；暂停菜单使用同一 Screen，关闭后由 CommonUI focus restoration 回到原入口。Screen、选项行和确认 Modal 均不访问 SaveGame。

主菜单继续使用 `L_MainMenu + AWacomMenuGameMode + UWacomTitleScreen + UWacomMainMenuScreen`。菜单 travel 目标必须使用 UE package path：

- Exploration：`/Game/Wacom/Maps/L_Exploration`
- MainMenu：`/Game/Wacom/Maps/L_MainMenu`

不要把 travel 目标写成 `/Game/Wacom/Maps/L_Exploration.L_Exploration` 这类 ObjectPath。UE PIE 下 ObjectPath travel 曾触发 `FPackagePath::TryFromMountedName was passed an ObjectPath` 和 `!NewPIEWorld->bIsWorldInitialized` ensure。

主菜单采用 App flow 与 Screen 分离。`AWacomMenuGameMode` 进入 `L_MainMenu` 时先把 `UWacomTitleScreen` Push 到 `UI.Layer.GameMenu` 作为稳定栈底；标题页只把键盘按键、鼠标左键和手柄按键转换为继续意图，ESC / Gamepad B 在根页面被消费。继续意图成功后 GameMode 再 Push `UWacomMainMenuScreen`；主菜单 ESC / B 上报 `EWacomMainMenuAction::ReturnToTitle`，GameMode 只有确认 TitleScreen 仍存在于同一 Stack 时才 Pop 主菜单，避免根页面缺失时产生空 UI 栈。每次进入主菜单关都重新从 TitleScreen 开始；Settings / Modal 仍叠在 MainMenu 上方并使用 CommonUI 焦点恢复。

`AWacomMenuGameMode` 构造 `FWacomMainMenuViewData`，绑定 `UWacomMainMenuScreen::OnActionRequestedNative`，并处理其余 `EWacomMainMenuAction`；Screen 只应用 ViewData、刷新 fallback / WBP 和上报玩家意图，不读取或删除 SaveGame，不调用 `OpenLevel()` 或退出 API。Title / MainMenu class 分别收紧为 `TSubclassOf<UWacomTitleScreen>` 与 `TSubclassOf<UWacomMainMenuScreen>`，C++ 默认加载 `/Game/Wacom/UI/Menus/WBP_TitleScreen` 和 `/Game/Wacom/UI/Menus/WBP_MainMenuScreen`，资产缺失或加载失败时回退对应原生 Screen；travel 前和 `EndPlay` 都会显式解绑。当前存档总开关关闭，因此 GameMode 不访问磁盘，Continue、Journey History、Credits 保持隐藏；Settings 已开放并进入统一 Settings Screen flow，Start New Journey 仍直接走 `/Game/Wacom/Maps/L_Exploration`。未来档案服务接入后，只替换 GameMode 的 ViewData 构建与 Action flow，不让 Screen 重新拥有 slot 语义。

Journey 成功页沿用同一 passive Screen 边界。`AWacomGameMode` 只消费 Run settlement 中的 `JourneySucceeded` event，不读取 Actor label、EncounterId 或硬编码 NodeId；它从同批 `PostSnapshot.CompletionSummary` 与 Journey `DisplayName` 构造 `FWacomJourneySummaryViewData`。`UWacomJourneySummaryScreen` 只显示“Journey 成功”、Journey 标题、完成天数、Floor/Node 进度和最终压力，并通过 `OnContinueRequestedNative` 上报按钮或 ESC/Back 的一次性继续意图。当前只提供原生 C++ fallback 和 WBP 表现钩子，不创建 `WBP_JourneySummary`，也不新增 Widget GameplayTag。

成功战斗仍先 Pop BattleHUD、结算 Encounter 并完成既有 Return-to-Run 镜头 staging；staging barrier 完成后直接进入 `JourneySummary`，不恢复 Run first-person hand 或 interact Toast。GameMode 把总结页 Push 到既有 `UI.Layer.GameMenu`；确认或 Back 后先解绑 Screen、`TearDownPrimaryLayout()`，再在下一帧 travel 到 `/Game/Wacom/Maps/L_MainMenu`。Screen push、PrimaryLayout 或 PlayerController 缺失时跳过页面并走完全相同的安全主菜单交接，避免玩家停在不可操作的终局世界。

Floor 1 Production 灰盒中的 `Node.Exit.01` 当前使用非交互 marker，仅表达 Floor DataAsset 的 `TargetFloorId=Floor.Main.02` 与 Credential 条件已有场景落点。它不接收 E/hover/click、不调用 `AdvanceFloor()` 或 `OpenLevel()`，也没有 Level Blueprint 特例。未来跨层交接必须由 GameMode/App flow 消费成功的 `WacomRun` Floor transition result，再以 FloorId-to-world 配置选择目标 map；不得把 travel 规则塞进 marker、Actor label 或当前灰盒 map。

---

## §3 PlayerController 与 PlayerCharacter

`AWacomPlayerController` 是探索输入、RunSession 持有者、GameMenu 请求入口和世界目标路由者。

主要职责：

- BeginPlay 创建并持有 `URunSession`，并通过当前 `AWacomGameMode` 的 Journey resolver 取得新 Run 定义；基础 GameMode 仍返回 `DefaultJourneyDefinition`，PlayerController 不读取 Descriptor 来拼 Journey。
- 新探索状态有效时，先从当前 World 唯一 `AWacomRunFloorSceneDescriptorActor` 解析显式 Floor，再在独立 working `FWacomRunSceneBindingRegistry` 中完整枚举和校验 Anchor/Path/Branch/content host；Snapshot 版本与 Floor 两次复核后，Coordinator 预检和 Registry 安装一次提交。任一步失败都保留旧 Registry、Coordinator 版本、Traversal、Pawn Transform 和 HUD 表现。Coordinator 继续只应用显式 Begin / Complete / Cancel Resolution，不自行判断地图合法性。
- 初始化 `UWacomInputContextCoordinatorSubsystem`，提供探索 / 战斗 mapping context 给 coordinator。
- 处理探索交互、地图、暂停菜单、背包、商店、RunEvent 打开请求；具体 GameMenu 打开、关闭、异步 Push 和 Shop / RunEvent rollback 由私有 `FWacomExplorationScreenRouter` 承接。
- 转发战斗快捷键到当前 `UBattleHUD / UBattleSession`；Battle scene target click / probe 由 App-private `FWacomBattleSceneInteractionRouter` 承接，`AWacomPlayerController` 只保留 public façade、trace / flow protected seam 和输入入口。
- Run world hover / click / probe 由 App-private `FWacomRunWorldInteractionRouter` 承接；`AWacomPlayerController` 保留 timer、InputKey 顺序、trace protected seam、E 键 candidate list 和与 Run card drop coordinator 的 context 注入。
- 维护探索期 first-person Run Card Workspace source；默认 workspace 当前来自 Run `BattleDeck` 物理卡和可选投影入战卡，GameMenu 可通过 owned menu lease 临时接管候选持有卡显示和 Zone drop。
- 维护 Run first-person hover / inspect 详情：只在 Exploration 且 `UWacomRunFirstPersonCardSourceComponent` 判定当前 card layer source 属于 Run default source 或 active menu lease 时，通过 `URunSession::FindInstance()` 反查 owned card instance 并显示静态 `UWacomCardDetailPanel`；App-private `FWacomRunFirstPersonCardDetailController` 负责 Anchor hover delegate 绑定、hover / inspect / scrub 决策、Run detail 数据缓存和面板状态，并委托 Battle / Run 共享的 `FWacomFirstPersonCardDetailMotionController` 与 `FWacomFirstPersonCardDetailPanelHost` 处理预热、面板创建、AddToViewport、淡入淡出 / scale / follow motion、稳定换边和 teardown。`AWacomPlayerController` 只作为 UObject delegate endpoint 和 RunSession 查询入口保留。同一张卡的 inspect update / hover layout update 只更新 motion target，不重复构建详情；scrub 到另一张卡时才切换详情数据。`Inspecting` 期间详情保持并支持 scrub 切卡，正式拖拽、进入战斗、source 清理或 lease 结束时隐藏。
- 维护 Run first-person card drag：App-private `FWacomRunFirstPersonCardDragController` 负责 Anchor drag delegate 绑定、inspect 与正式拖拽分流、正式拖拽期间详情隐藏；`Inspecting` 刷新读牌详情、允许手牌内 scrub 切换 source，并用 drag view 持续驱动当前 Run Path camera look，但不触发投放 probe。只有离开 scrub 手牌区域后，最后 Inspect 卡才能升级为正式拖拽；release / preview 统一交给 App-private `FWacomRunFirstPersonCardDropCoordinator`。Hover / Inspect / Drag 的 cursor override 只写入 `UWacomRunPathTraversalComponent`；不恢复共享 camera-look bridge 或第二套探索移动。Drop Coordinator 继续以显式 context contract 使用 RunSession / world hit，menu owner / world receiver 决定提交结果。
- Run first-person source 使用 desired-state reconcile：PlayerController BeginPlay 会创建 / 绑定 RunSession 并请求探索期手牌 source 处于 active；如果当帧 RunSession、Pawn / `UWacomFirstPersonCardAnchorComponent`、GameMenu suppression 或 menu lease 条件尚未就绪，`UWacomRunFirstPersonCardSourceComponent` 会记录 pending/block reason，并在 RunSession 绑定、`SetPawn()`、GameMenu suppression 解除、menu lease 清理或 RunStateChanged 时自动重试。进入探索关、读档 bootstrap 完成和战斗返回探索都走 `PrepareExplorationRunFirstPersonCardLayer()`，该入口会清理 stale menu widgets / suppression / menu lease，再恢复默认 Run workspace source，不依赖背包关闭补刷。Run default、active menu lease、suppressed source 和 `BattleHand` 的处理权查询集中在 `UWacomRunFirstPersonCardSourceComponent`，PlayerController 不直接比较保留 source id。
- Run first-person source 写入默认 workspace 和 provider-backed menu lease 时使用 Battle / Run 共用的 `FWacomFirstPersonCardLayerSourceLifecycleFrame` 包住 presentation frame、interaction ownership 和必要清理，并统一经 `UWacomFirstPersonCardAnchorComponent::ApplyRuntimeCardLayerSourceLifecycleFrame()` 提交到 Anchor。组件通过 `URunSession::BuildRunCardWorkspaceSnapshot()` 读取 Run 层只读 workspace，并缓存 `CardInstanceId -> FRunCardWorkspaceEntry` 来源 metadata；App 不再自行遍历 RunState 四个物理区来决定候选卡。初次显示、从 GameMenu suppression 恢复、menu lease 交还默认 source、默认 source 新增卡，或 RunEvent / 菜单通过 `OwnedCardsFilter` 筛出候选卡接管 hand 时生成 `RunHandEntered` 入场 hints；普通刷新不重播。进入背包 / 事件等压制默认手牌的菜单场景写入 0 entries 的 `Suppressed` frame，让 card layer 清空手牌，而不是直接隐藏或硬清 slot motion；menu lease 不再有 entries-only raw 正式入口，必须通过 owned-card provider 给出候选卡。
- typed Encounter arrival 与 E 重试只通过 native `TryEnterCurrentEncounter()` 转发，不提前清理手牌、详情或 drop probe。只有 GameMode 的 `TryEnterBattle()` 成功取得 Encounter NodeActivity、成功创建 BattleSession 并 Push BattleHUD 后，才一次性清理 Run first-person source；任一校验或 Push 失败都完整保留 Run HUD、手牌和交互状态。
- 在成功进入战斗、Controller EndPlay 或菜单 lease 结束时清理对应 first-person source / lease，避免 Run 展示层和 BattleHUD runtime hand 抢占同一 layer。
- GameMenu 可选走 first-person viewpoint staging：当前 Shop 和 RunEvent 使用该通用流程，Trigger 生成 entry stage request，PlayerController / router 在 UI 出现前锁定探索输入并清空 Run first-person hand，到位后再开始对应 `URunSession` 事务并 Push Screen；菜单关闭后 staged return 到当前 Run Path View，完成后恢复手牌和交互提示。
- 当前 Floor 地图走非破坏性 GameMenu 路径：`IA_OpenMap` 只在 Exploration + Anchored + 无活动探索事务、Traversal 或其它 active/pending GameMenu 时接受。行进中按 M / 手柄 View 不会中断 Traversal Ticket，而是提示先到达节点；其它门控拒绝记录稳定 Detail，避免把 Snapshot、版本或 Flow 故障误判为按键失效。PlayerController 独占 App-private `FWacomRunMapScreenFlow`；Flow 用 generation 拒绝过期 async callback，绑定 Screen / Session delegate，应用完整 Map ViewData，并经唯一 Coordinator seam 提交同层 MapTravel。关闭、Push 失败、Session/Pawn 重绑和 EndPlay 都会对称清理并把焦点还给游戏视口。地图不使用 Shop/RunEvent 的镜头 staging。

`AWacomPlayerCharacter` 是第一人称探索 Pawn：

- 持有 Camera、CharacterMovement、唯一正式 `UWacomRunPathTraversalComponent`、cursor look driver、Battle camera look 和 `UWacomFirstPersonCardAnchorComponent`。
- Run Path component 独占探索移动、Pawn Transform、基础 Yaw、View Source、cursor look、CameraShake 和 Suspend / Resume。`Anchored` 与 `Traversing` 都会停止并禁用 `CharacterMovement`，并在持有 Run Path 时关闭 Character 的 `bUseControllerRotationYaw`：ActorRotation 只表达 Anchor / Spline 基础朝向，ControlRotation 才叠加 cursor look，避免两套系统逐帧争夺朝向。Deactivate 时恢复进入 Run Path 前的 Yaw 所有权设置。Anchored 状态也不会把 W/S 泄漏给 CharacterMovement。地图状态或场景绑定未就绪时保持安全锚定 / 停止，不回退到第二套移动实现。战斗和 GameMenu staging 只 Suspend 当前移动而不 UnPossess。
- 战斗期启用 Battle camera look，在战斗 base rotation 上叠加共享 cursor look offset。
- first-person anchor 只负责给 HUD / card layer 提供稳定锚点；projection/layout 细节见 [First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md)。

战斗时不 UnPossess 的原因是保持 PlayerController 的 InputComponent 活跃，让 Battle input profile 继续通过 Controller 路由到战斗 UI。

核心蓝图资产归档到 `/Game/Wacom/Core`：GameMode 放 `/Game/Wacom/Core/GameModes`，PlayerCharacter 放 `/Game/Wacom/Core/Player`，PlayerController 放 `/Game/Wacom/Core/Controllers`。Run Path authoring 蓝图放 `/Game/Wacom/Run/Path/Blueprints`。`/Game/Wacom/_GAME` 是历史临时目录，不再作为新增资产入口；具体目录规范见 [Content_Organization.md](./Content_Organization.md)。

`AWacomRunTunnelPaperLayerActor` 是独立保留的探索纸片层 C++ 父类：自带一个无碰撞、无阴影的 Plane StaticMeshComponent，蓝图子类可配置材质模板、Texture2D 数组、材质贴图参数名、固定索引或稳定随机种子。它的历史类名只用于既有美术资产兼容；该 Actor 只做场景视觉 authoring，在 Construction / BeginPlay 创建动态材质实例并写入贴图参数，不参与 Run Path 移动、节点规则、世界交互或卡牌投放结算。

### Run Path Scene Registry 与结果应用

- 一个独立 Run Floor World 必须且只能有一个 `AWacomRunFloorSceneDescriptorActor`；App-private resolver 先验证 World、唯一 Descriptor、非空 Floor、非空 FloorId 和 Snapshot FloorId 一致。不得再从场景 Actor 集合猜 Floor，也不得由调用方绕过 Descriptor 任意传入 Floor。
- Registry 每次只对应 Descriptor 已解析的当前 Floor，建立 `EdgeId -> PathSegment`、`NodeId -> NodeAnchor`、多出口 `EdgeId -> BranchTarget`、普通内容节点的 `NodeId + NodeType -> content host`，以及 Encounter 的 `NodeId -> UWacomRunEncounterSceneBindingComponent` 完整 working 映射。缺失、重复、意外身份、host 类型、slot 映射或 payload 不匹配都在安装前拒绝，不能把部分 Registry 暴露给运行中表现。
- 刷新顺序固定为 `Snapshot -> Descriptor -> working registry -> completeness -> version/Floor drift recheck -> Coordinator prepare -> teardown/install/commit`。旧绑定只在所有预检完成后替换；Descriptor/枚举/完整性、Coordinator prepare 或版本漂移失败均无 UI/规则副作用。
- Encounter 没有可点击 content host。目标 Node 的 `CompleteTraversal` 成功、目标 Anchor 应用完成后，Coordinator 才广播不携带任意 Actor 的 typed arrival request；PlayerController 通过 Floor Node payload 与 Registry binding 调用 native `TryEnterBattle()`。重复 arrival、未抵达、非 `Visited Encounter`、活动事务或不完整 binding 都被门禁拒绝。
- `FWacomRunExplorationPresentationCoordinator` 依据显式 Snapshot 维护 App-private route-choice state。Anchored 首次正向越阈值时：唯一合法 Edge 自动复用正式 Begin 路径；多条合法 Edge 只提示选择且不提交规则；结构死胡同和暂时锁定分别使用明确 AppToast，均不改变版本。
- `FWacomRunPathBranchSelectionController` 由 PlayerController 独占，只在 `ChoiceRequired + Exploration + Anchored + 无 active/pending GameMenu` 时显示合法 BranchTarget。鼠标 hover/click、A/D 或左摇杆左右移动焦点、E/手柄 A 确认都走同一 EdgeId 意图；多出口状态下只在场景输入拥有权有效时消费 E 与场景点击，GameMenu 激活或镜头过渡期间必须把鼠标完整交给 CommonUI。
- `AWacomRunPathBranchTargetActor` 只广播 EdgeId，并且只有 `Available / Focused` 能广播。Hidden 同时关闭视觉和 Visibility collision；Blueprint 只能响应只读表现状态，不能决定合法性、保存 target Segment / Node 或直接移动玩家。
- Begin 成功后规则位置仍是 source；W/S 只改变局部 Spline distance。回到起点提交 Cancel；到达终点重新验证 target/host，再提交 Complete。Encounter arrival 只在规则 Complete 与 Anchor 应用都成功后发送一次；返回探索本身不发送 arrival。
- Coordinator 严格接受连续 `VersionBefore -> VersionAfter` 的 Resolution。Traversal 与 MapTravel 由 Coordinator 自己提交；Encounter、Shop、RunEvent、Treasure/Pickup 等外部节点活动必须由各自的 WacomApp adapter 把 Begin、Settlement、Choice、End 或 Cancel 返回的显式 `FRunExplorationResolution` 立即交给 `ApplyNodeActivityResolution()`，同步版本并刷新路线选择状态。`URunSession` 不反向调用 UI/PlayerController，App 也不得只观察 `RunStateChanged` 后拉取最新 Snapshot 来掩盖正常通道遗漏。过期、跳版本、重复结果或 Session/PostSnapshot 不匹配都会被拒绝；PlayerController 只在该通道异常时重建当前 Session 场景绑定作为可玩性恢复。
- target/host 在 Complete 前失效时先 Cancel 并回到缓存 source；Complete 规则失败也显式补偿 Cancel。Complete 成功后绝不回源，目标 Actor 同帧失效时使用 Begin 缓存 target Transform。
- first-person card Anchor 与 ViewStage return 只消费 Run Path 的当前 View Transform；`EWacomFirstPersonCardAnchorMode::RunPath` 是唯一探索锚点模式。
- `Anchored` 与 `Traversing` 都保持 Run Path component Tick：Anchored 只更新 cursor look 和 view transform，不修改 Spline distance、不启动 Walk CameraShake 或 Bob；Traversing 才沿 Spline 移动。正式 Scene Binding 可能在组件 `BeginPlay` 前完成初始 Anchor，因此 `BeginPlay` 必须按已有 Traversal state 恢复 Tick，不能无条件关闭并覆盖已经提交的 Anchor 生命周期。cursor look 在 Slate 可用且指针位于游戏视口内时优先读取 Slate 的实时绝对指针并按 viewport geometry 归一化，`APlayerController` 鼠标坐标只作为后备；因此嵌入式 PIE 尚未收到第一次点击或按键时也不得冻结初始节点视角。首次 W 开始 Traversal 时，distance 0 仍使用当前 NodeAnchor View；随后在 `PathEntryViewAlignmentDistance`（默认 120 cm，且不超过 Path 长度）内以 SmoothStep 将位置与基础朝向对齐到 Spline，入口 Anchor 与通道切线不一致也不会瞬间改中心。Anchored 导航输入使用按压/释放 latch，抵达新节点时持续按住 W 不会自动进入下一条 Edge。

---

## §4 输入协调

`UWacomInputContextCoordinatorSubsystem` 是本地玩家输入上下文的唯一协调者。GameMode、PlayerController 和运行时组件只声明当前意图，Subsystem 统一应用：

- CommonUI input config。
- 鼠标显隐 / capture。
- Enhanced Input mapping context。
- PlayerController `bEnableClickEvents / bEnableMouseOverEvents` owner lease。

`UWacomGameViewportClient : UCommonGameViewportClient` 是极窄的 Slate 前置输入仲裁 owner，不替代 Coordinator 或 PlayerController。由于正式输入配置保持 `NoCapture`，ViewportClient 注册 `Game` priority 的 App-private `IInputProcessor`，在 Widget 路由前处理两类已声明输入所有权：快捷键来源 first-person card active drag 的右键中性取消，以及 active World Shop 的左键 press/release。World Shop press 只在指针属于当前 GameViewport 时路由并消费；release 还负责在指针越界时成对清除已按下状态。其余输入返回未处理并继续正常路由。Viewport `HandleRerouteInput()` 和 PlayerController `InputKey()` 的同条件分支只作为其它捕获模式 fallback。

当前上下文：

| Context | CommonUI input config | IMC |
|---|---|---|
| `MainMenu` | `Menu + NoCapture`，鼠标可见 | 不启用探索 / 战斗 IMC |
| `Exploration` | `All + NoCapture`，鼠标可见 | `IMC_Exploration` |
| `Battle` | `All + NoCapture`，鼠标可见 | `IMC_Battle` |

PlayerController 上的 `PushMappingContext / PopMappingContext` helper 仍保留为兼容 / 调试入口；正式流程由 coordinator 管理。PlayerController BeginPlay、MenuGameMode BeginPlay 和 GameMode BeginPlay 都会初始化或重设 coordinator，防止 PIE 复用 Controller 时输入状态停留在上一关。

Run Path 是正式探索移动模型，不提供普通 FPS FreeLook 探索 profile。`UWacomRunPathTraversalComponent` 在 Traversing 把 W/S 作为连续路径轴；Anchored 持续消费 cursor look，并把 W/A/D 作为带释放重置的一次性导航意图，绝不直接修改 Spline distance。Suspended 保留 Path / distance 并停止移动和 CameraShake；Resume 不重建规则 Ticket。first-person card hover / drag 的归一化指针继续驱动同一 cursor look override。CameraShake、设置中的镜头运动倍率与 stop grace 保持已验证语义。`L_Exploration` 与 `BP_WacomPlayerCharacter` 已迁移到唯一的 Run Path 实现，不再保留旧移动 fallback。

当前 IA 口径：

| IA | 用途 |
|---|---|
| `IA_Move` | WASD 移动 |
| `IA_Look` | 鼠标视角 |
| `IA_Interact` | 探索期优先重试当前逻辑 Encounter；没有重试时与最近世界交互对象互动 |
| `IA_OpenMenu` | ESC 打开或关闭菜单 |
| `IA_OpenBackpack` | 探索期打开背包 |
| `IA_PlayCard1~7` | 战斗手牌快捷键 |
| `IA_Wait` | 战斗等待 |
| `IA_EndTurn` | 战斗结束回合 |

---

## §5 世界交互路由

世界交互有五类入口，全部由 `AWacomPlayerController` 或当前 Screen / HUD 统一路由，不让 Actor 或被动 Widget 直接改规则状态：

| 入口 | 当前路由 | 规则出口 |
|---|---|---|
| E 键近距离交互 | overlap 候选 -> 最近 `IWacomWorldInteractable::TryInteract()` | 对应 Actor 请求 PlayerController / GameMode / RunSession |
| 鼠标 hover | cursor trace -> interaction target provider -> `FWacomRunWorldInteractionRouter` 或 `FWacomBattleSceneInteractionRouter` probe | 只更新提示 / 预览，不提交规则 |
| 探索左键点击 | GameMenu pointer gate -> Battle scene router target click -> Run Path branch click -> Run world interaction router click -> `Super::InputKey()` | active/pending GameMenu 与其镜头过渡优先拥有左键；只有纯场景输入期才回到 `TryInteract()` 或道路选择 |
| Run world card drop | first-person drag release -> Run world target probe -> `UWacomRunWorldCardDropReceiverComponent` | `URunSession::Validate/SubmitRunWorldCardInteraction()` |
| Run menu zone drop | active GameMenu + active menu lease -> `UWacomRunMenuDropTargetWidget` Zone probe | owning menu 决定 probe、prototype destroy 或 RunEvent payment submit |

正式 opt-in Run world clickable 对象包括 RunEventTrigger、ShopTrigger、RunPickup、RunCardPickup、RunRewardPickup 和 KeyChest。Encounter 不参与 Run world hover/click；Actor 制作字段、debug summary、prototype sample button、Data Validation 和 target handle 细节见 [WacomWorldInteraction.md](./WacomWorldInteraction.md)。

带 `UWacomRunMapNodeBindingComponent` 的 Run world Actor 会在共享交互资格层验证其 `NodeId / NodeType` 与当前 Run Snapshot 一致。该验证同时约束 E 键候选、hover / probe 和左键点击，并且早于 Actor `TryInteract()`、Shop / RunEvent viewpoint staging 与 GameMenu async push；因此已经离开的节点内容不会造成镜头先移向旧 Actor 再返回。未绑定节点的独立原型 Actor 继续保持既有行为。

打开 Map / Backpack / Pause / Shop / RunEvent 等 GameMenu 时，PlayerController 使用同一 `CanRouteRunScenePointerInput()` 门控关闭 BranchTarget hover/click、Run world hover/click 和 Anchored 导航意图，避免 CommonUI 按钮的左键释放被场景路由截走；菜单关闭且回程过渡结束后才恢复。菜单 first-person card drag/drop 继续走自己的 menu lease / drop seam，不依赖普通场景点击路由。

---

## §6 UI Shell 与菜单入口

WacomApp 负责调用 UI shell，但不在本文定义具体 UI 视觉和刷新规则。

当前 UI shell ownership：

- `UWacomGameUIManagerSubsystem` 持有 `UWacomPrimaryGameLayout`，负责 Ensure / TearDown PrimaryLayout 和按 layer Push / Pop。
- `UWacomUIDeveloperSettings` 是顶层 UI WBP 的项目级软类注册表。
- `FWacomExplorationScreenRouter` 统一地图、背包、暂停菜单、商店和 RunEvent 的 GameMenu 打开 / 关闭 / async push / rollback。地图不会为了打开而关闭已有其它 GameMenu；M / 手柄 View 的 pending 或重复请求无副作用。
- `UBattleHUD` 是战斗 UI 命令出口、WBP 绑定 owner、配置 owner 和 GC 引用 owner。

顶层 Screen / Toast 不再通过 `AWacomPlayerController` 暴露 ScreenClass 覆盖入口。RunMap / Backpack / PauseMenu / Shop / RunEvent 通过 `UWacomUIDeveloperSettings.WidgetClasses` 的 `UI.Widget.*` tag 注册；地图 tag 为 Core 中的 `UI.Widget.RunMapScreen`，并要求父类 `UWacomRunMapScreen`。缺失或加载失败时回到对应 C++ fallback。UI 数据流见 [WacomUI.md](./WacomUI.md)，CommonUI 层级 / Settings / Toast 见 [WacomUIFoundation.md](./WacomUIFoundation.md)，BattleHUD helper 和 Battle UI surface 见 [WacomBattleUI.md](./WacomBattleUI.md)。

菜单按钮不直接 OpenLevel；切关卡委托给 GameMode 或 PlayerController。主菜单和暂停菜单切关前先 `TearDownPrimaryLayout()`，再在下一帧 `OpenLevel()`，避免在按钮点击 / CommonUI deactivate 链中立即切关。

### Knockdown Choice 被动奖励预览

`UWacomKnockdownChoiceDialog` 只消费 `FKnockdownChoiceView`，不读取 `UEnemyPartDefinition`、不加载奖励资产、也不重新判断选项合法性。Aid / Destroy option 的 `bHasRewardCard / RewardCardId / RewardCardName` 由 Battle 通过与实际结算相同的查询构造；Dialog 只显示 `奖励：<名称>`，名称为空时使用稳定 CardId，两者都不可用或没有奖励时显示“无卡牌奖励”。空奖励不会让 UI 自行禁用按钮。

原生 fallback 在 `AidButton / DestroyButton` 下提供 `AidRewardText / DestroyRewardText`；正式 WBP 可通过同名 `BindWidgetOptional` 锚点替换布局。`NativeConstruct` 和每次 `SetContext` 使用同一刷新路径，因此连续多部位击倒不会保留上一部位的奖励文字或按钮状态。Modal layer、必须三选一、ESC/Back 拦截、BattleHUD 命令提交和 deactivate 时机保持不变；完整 CardView、缩略图和新焦点流不属于该 Dialog。

### Shop 卡牌强化被动合同

Shop Trigger 只把 `UShopDefinition` 的商品与可选强化价格转成 `FRunShopVisitRequest`，PlayerController / ScreenRouter 只负责访问生命周期和请求转发；是否可强化、实际价格、金币、过期 Definition 与原子提交都由 RunSession 权威决定。`FRunShopSnapshot.CardUpgradeQuotes` 和 `FRunShopCardUpgradeResult` 是 App 可消费的被动数据，Widget 不读取或修改 CardDefinition 链，也不能自行推断资格。

`UWacomShopScreen` 与全局注册的 `/Game/Wacom/UI/Shop/WBP_ShopScreen` 现提供“购买 / 强化”双页签。强化页只列出存在下一 Definition 的实体 Instance，同 Definition 多实例不会合并；金币不足项保留可见但禁用。选择后用两张通用 `WBP_CardView` 展示当前/下一版本，并列出稀有度、费用和 Effect Magnitude/Duration 差异。下一版本卡面直接消费自身稀有度边框，并只把相对当前版本提高或降低的效果数字标为被动正/负语义色；该比较色不写入 CardDefinition，也不会延续到手牌、背包或战斗中的普通卡面。内联按钮只提交 Quote 中的 `InstanceId + ExpectedCurrentDefinition + ExpectedNextDefinition`，Screen flow 消费 Run 的权威 Result，显示稳定中文 Toast，并按 Exploration Resolution / visit close 语义收口；同名强化用“卡名（旧稀有度 → 新稀有度）”反馈，不弹第二个确认框。

强化 service 关闭时隐藏强化页并保持旧商店 purchase-only 行为。Snapshot/Gold/Quote 刷新通过独立 signature 和 InstanceId reconciler 更新，保留仍合法的页签与选择；达到链末端后清除选择。Activate/Deactivate、RunSession 订阅、鼠标焦点和 ESC/Back 与购买页共用对称生命周期。C++ fallback 与 WBP 走同一被动合同，WBP 不引用 Debug 卡或 Shop 资产。卡牌回收金币展示继续统一调用 `URunSession::GetDeleteGoldRewardForCard()`。

### GameMenu viewpoint staging

GameMenu viewpoint staging 是 Exploration GameMenu 的通用临时镜头站位流程，当前接入者是 Shop 和 RunEvent，共用同一套 PlayerController 状态和 Run Path return flow。`AWacomShopTriggerActor.ShopEntryViewpoint` 是商店入口的可选第一人称镜头站位；`AWacomRunEventTriggerActor.RunEventEntryViewpoint` 是事件入口的可选第一人称镜头站位。未配置时沿用普通路径：Shop 为 `BeginShopVisit -> Push ShopScreen`，RunEvent 为 `BeginRunEvent -> Push RunEventScreen`。配置后，Trigger 会构造 `FWacomFirstPersonViewStageRequest`：Shop 使用 `Reason=ShopEntry`，RunEvent 使用 `Reason=RunEventEntry`，`DebugSource` 优先使用 `PersistentId`，并复制 viewpoint 的 View Transform、blend 时间、曲线和 ease power。

staged GameMenu 打开时，`FWacomExplorationScreenRouter` 先锁定探索输入、清空 / suppress Run first-person hand，并通过 `FWacomFirstPersonViewStageCoordinator` 移动到目标 View Pose；到位后才执行对应 Screen 的 async push。Shop 的 Begin、成功 Purchase、正常 End 和 async push rollback，以及 RunEvent 的 Begin、成功 Choice（包括卡牌支付）、正常 End 和 async push rollback，都必须把显式 `FRunExplorationResolution` 交给 PlayerController 的唯一节点活动表现入口，保证 Coordinator 版本在回程前已经与 Session 对齐；空手关闭仍只取消本次节点活动，并允许可重入内容再次进入。ShopScreen / RunEventScreen 关闭时仍由自身 deactivate 流程结束自己持有的 visit，UI 立即退场；PlayerController 识别该 staged GameMenu 后保持手牌 suppression，调用 `FWacomFirstPersonViewStageReturnFlow::ReturnToRunPath()`，回到当前 Run Path View 后恢复探索输入、刷新 Run first-person hand 和 interact toast。push / refresh 失败时走现有 rollback，并立即 return to Run Path。

Router 内部用共享私有 helper 承接 “stage first-person view -> open GameMenu -> arm return” 的握手；Shop / RunEvent 只提供各自的 async push 和 RunSession 事务，后续新 GameMenu viewpoint staging 应复用这条路径，不再复制 staging 生命周期。

GameMenu 打开前的通用 guard 同样收口在 Router 内部：Exploration 状态检查、UIManager / PrimaryLayout 就绪、关闭旧 GameMenu、pending async push 防重入由共享 prepare 处理；ShopId、PersistentId、Definition 等业务合法性仍由对应入口各自校验。

---

## §7 战斗进出流程

### EnterBattle

```text
Run Path CompleteTraversal 成功
-> 目标 Anchor 应用完成
-> typed Encounter arrival
-> PlayerController.TryEnterCurrentEncounter(NodeHandle, SceneBinding)
-> GameMode.TryEnterBattle(NodeHandle, EncounterDefinition, SceneBinding)
```

PlayerController 是被动意图 façade；GameMode 才拥有 Encounter 规则事务、BattleSession、BattleHUD、Run hand 清理和镜头切换的原子交接。GameMode 重新验证当前逻辑节点、`Visited Encounter`、无活动 ticket、规则定义、scene binding、UI 与 flow 状态，重复 arrival 只能成功一次。规则拒绝、初始化失败或 HUD Push 失败时不允许留下“手牌已隐藏但仍是 Run HUD”的半切换状态；玩家保持在当前节点并得到“按 E 重试”，E 重试不检查物理距离。撤离或自动启动失败会让该 `MapNodeHandle` 在本次 Run 内进入“必须手动重试”状态：离开节点只隐藏当前提示，再次到达同一 Encounter 仍只恢复“按 E 重试”，不会重新自动开战；成功按 E 进入战斗时消费该状态，若再次撤离则重新记录。

GameMode 进入战斗时：

1. 设置 `EGameFlowState::Battle`。
2. 清理探索期 Run first-person default workspace source 和 active menu lease。
3. Suspend PlayerCharacter 的 Run Path 探索移动；若 Encounter Anchor binding 配置了 battle entry viewpoint，则先把第一人称摄像机 View Pose 对齐到该站位，再启用 Battle camera look。Viewpoint 可配置过渡时间，默认 0 秒立即对齐。
4. 由 RunSession 使用显式 `MapNodeHandle + EncounterDefinitionId` 构造战斗玩家侧参数（角色、随机种子、备战卡组、撤离重入进度），由 Floor Node 的 `EncounterDefinition` 构造敌人槽；若 RunSession 缺失或无法构造参数，GameMode 会拒绝进入战斗，不再用 `DefaultCharacter / DefaultRandomSeed` 拼 fallback 战斗。
5. 创建 / 初始化 `UBattleSession`。
6. 取得 Encounter NodeActivity ticket，并把成功 Begin 的显式 Resolution 应用到 Run 表现 Coordinator；失败或后续 HUD Push 失败时同样显式应用 Cancel 结果，不能遗留版本漂移。
7. 通过 UIManager Push `UBattleHUD` 到 Game 层。
8. 将 input coordinator 切到 `Battle` profile。
9. 记录当前 `MapNodeHandle + weak EncounterSceneBinding + EncounterDefinition`，并把 binding 的 `SceneEnemyHostSlots` 映射传给 BattleHUD 场景目标 registry。

敌人入口只走 Floor Node `EncounterDefinition + Encounter Anchor SceneEnemyHostSlots`。`EncounterDefinition.EnemySlots` 负责规则敌人槽，binding 的 `SceneEnemyHostSlots` 负责 `EnemySlotId -> SceneEnemyHost` 的场景表现绑定；缺 Host、漏 slot、多余 slot、Host 重复或跨 Encounter 共享会被 runtime registry 与编辑器验证阻止。场景敌人点击、hover 和拖卡目标路由只认当前 BattleHUD registry 中的 `EncounterId + EnemySlotId + PartSlotId`，不通过 Actor 名称、单 Host 缓存或旧第一敌人入口推断身份。

Battle entry viewpoint 是 `WacomApp` 的场景 / 镜头编排能力，不属于 `WacomRun` 或 `WacomBattle` 规则。关卡中可摆放 `AWacomFirstPersonViewpointActor`，并由对应 Node Anchor 的 `UWacomRunEncounterSceneBindingComponent.BattleEntryViewpoint` 引用；该 Actor 的 transform 表示第一人称摄像机 View Pose，不是玩家 Capsule/root 位置。场景 binding 构造 `FWacomFirstPersonViewStageRequest`，`FWacomFirstPersonViewStageCoordinator` 是 App 层 request consumer，GameMode 是当前 battle 入口调用者。进入战斗时 GameMode 会先 suspend Run Path Traversal，再把 request 交给 coordinator：coordinator 按 View Pose 和摄像机相对偏移反推 Pawn root / Controller rotation，或启动平滑过渡，并在 staging 完成时激活 Battle camera look。`StageBlendTimeSeconds` 默认为 0 秒，此时立即对齐；若大于 0，则由 PlayerCharacter 的 first-person view stage blend component 从当前 View Pose 平滑移动到目标 View Pose。`StageBlendCurve` 和 `StageBlendEasePower` 控制过渡速度曲线；默认 `SmoothStep` 保持旧手感，`EaseOut` 适合快速靠近后柔和停下，`EaseInOut` 适合更明显的起止缓动。过渡期间组件使用 Battle camera look 的 yaw / pitch clamp、scale 和 interp 参数按鼠标位置叠加 cursor look offset；完成交接时 Battle camera look 的 base rotation 仍使用原始 Viewpoint，但当前 cursor look offset 会被保留，避免镜头从鼠标偏移角度回弹到原始 Viewpoint。

First-person card Anchor 在 `BattleCameraLook` 尚未激活但 stage blend active 时优先使用当前 staged base View Pose；suspended Run Path 不再拥有 Anchor 空间，避免手牌锚点停在探索样条位置直到过渡结束。Anchor 在 `RunPath` / `BattleCamera` / `ViewStageBlend` 之间交接时使用 `CameraStageFollowInterpSpeed` 而不是普通 `FollowInterpSpeed`；该值默认 0，表示立即贴合目标镜头空间，避免 HUD 或 hand source 已刷新但锚点还在二次追随。BattleHUD / BattleSession 仍按 EnterBattle 当前时序创建；若镜头过渡被延后，BattleHUD 会先进入 `BattleInputReady=false` 且 first-person `BattleHand` runtime layer suppressed 的 staging 状态。suppressed 状态会关闭 `BattleHand` presentation gate、清空当前 card layer visual slot，并写入一个 0 entries 的空 `BattleHand` runtime source，用来阻止 Anchor 回退到 preview card layer；suppressed 期间的 Snapshot 刷新只更新普通 HUD / 场景目标，不显示或交互第一人称战斗手牌，也不推进 `FWacomBattleHandPresentationController` 的展示 baseline。

首回合 `CardsDrawn` 会作为 deferred entry reveal transaction 保留，等 Battle camera look 激活、GameMode 解除 suppression 后，再由 `FWacomBattleHandPresentationController` 用空手牌 baseline 从当前 Battle snapshot 生成一次显式 `entries + Drawn hints` presentation frame；解除后的正式 hand sync 会重新打开 presentation gate，Anchor 才能把 frame hints 交给 Layer 播放，消费后不会在后续普通刷新中重播。如果 entry reveal frame 已经提交到 Anchor 但还没被 Layer tick 消费时又进入 suppression，BattleHUD first-person hand bridge 会把该 frame 还原为 controller 的待播事件后再清空 visual source，避免卡牌在不可见阶段丢失动画或直接落位。Battle camera look 激活后，GameMode 重新从当前 Battle snapshot 刷新 first-person hand，并解锁玩家命令。未配置 viewpoint 或 blend 为 0 秒时保持旧行为，从当前探索位置立即进入可操作战斗。本阶段不做 CameraDirector 或 Level Sequence。

### ExitBattle

```text
BattleSession 结算完毕
-> BattleHUD.OnBattleEndedNative(EBattleOutcome) 通知 GameMode
-> GameMode.ExitBattle()
```

GameMode 退出战斗时：

1. Pop BattleHUD 并清理 BattleSession 引用。
2. 清空探索期 `UWacomRunFirstPersonCardSourceComponent` runtime layer，避免 Run 手牌在回程 staging 中从临时 Viewpoint 平移回 Run Path。
3. 将 input coordinator 切回 `Exploration` profile，但暂不恢复探索移动输入。
4. 停用 Battle camera look，但保留当前可见视角，不先拉回 battle base rotation。
5. 从当前 suspended Run Path 构造 `RunPathReturn` stage request。
6. 通过 `FWacomFirstPersonViewStageReturnFlow` 将第一人称视角移动回 Run Path View Pose。
7. stage 完成后恢复 PlayerCharacter 探索移动；若回程 blend 为 0 或无法构造 request，则同步恢复。
8. 从 `UBattleSession::BuildResultPacket()` 构造 `FBattleResultPacket`，使用进入战斗前取得的 `FRunNodeActivityTicket` 调 RunSession 原子结算战斗结果、Action Point、撤离进度和节点 lifecycle，并在恢复输入前把 Settlement 或 Cancel 的显式 Resolution 应用到同一个 Run 表现 Coordinator。撤离进度只按当前 `MapNodeHandle` 写入，不再经过场景 Actor ID。
9. 真胜利时由节点结算结果驱动 weak scene binding 在双 barrier 后退役对应 Enemy Host；撤离、Undetermined、启动/结算失败保留 Host，并为对应 `MapNodeHandle` 记录本次 Run 内的手动 E 重试要求。离开节点只隐藏提示，之后再次抵达同一节点仍不会自动开战。Defeat 不提供重试。
10. 普通结算把 `EGameFlowState` 回到 `Exploration`，等待 return staging completion 后重新激活 Run first-person hand 并刷新交互 Toast；`JourneySucceeded` 则进入 `JourneySummary`，同一 barrier 后展示成功页且不恢复两者。

普通战斗退出回到 Exploration 后，PlayerController 会重新激活并刷新 `UWacomRunFirstPersonCardSourceComponent`，让 first-person card layer 再次显示当前默认 Run workspace。当前默认 workspace provider 仍读取 Run `BattleDeck` 物理卡和可选投影卡；这个刷新只读 Run snapshot，不提交 Run 命令；若回程是 deferred blend，刷新必须等镜头回到 Run Path 后再发生。Journey success 不执行该刷新。

`AWacomPlayerController::RequestExitBattle(EBattleOutcome)` 仅作为外部手动结束战斗的 typed façade 保留，不再接受裸 `uint8` Outcome。正式 BattleEnd 主链路仍是 BattleHUD 根据 Snapshot BattleEnd 广播 `EBattleOutcome`，由 GameMode 在 Session 释放前生成 `FBattleResultPacket` 交给 Run 层。

若进入战斗时使用了 battle entry viewpoint，退出战斗不保存该临时站位。Run Path Traversal 保留进入战斗前的 Segment / Distance，并负责生成 `RunPathReturn` stage request；`ReturnStageBlendTimeSeconds` 默认 0.35 秒，可在 `UWacomRunPathTraversalComponent` 的 `Wacom|Run Path|Staging` 分类中调整，`ReturnStageBlendCurve` 和 `ReturnStageBlendEasePower` 用同一套 stage blend 曲线语义控制回程节奏。回程过渡期间 BattleHUD 已退场、探索输入仍锁定、first-person Run 手牌为空；`FWacomFirstPersonViewStageReturnFlow` 负责调用 stage coordinator 回到样条 View Pose，并在完成后恢复探索输入。Battle exit return 恢复 Run Path 时会按当前鼠标位置保留 / 预热 cursor look offset，不先把镜头拉回样条中心角度，避免回程完成瞬间出现中心回弹。这个 return flow 也可被商店、剧情、RunEvent 等临时站位复用。

`UBattleHUD::NativeDestruct()` 可能在 Run first-person source 恢复之后才执行。BattleHUD 清理 first-person hand 时只允许清自己写入的 `BattleHand` runtime source；如果 Anchor 已经显示 `RunFirstPersonBattleDeck` 或 menu lease，只能解绑 BattleHUD delegate 和清战斗预览，不能关闭 Anchor 的 first-person card interaction。否则会出现回到 Exploration 后卡牌仍可见但无法拖拽 / 使用。

战斗结果包和 Run 结算规则见 [WacomRun.md](./WacomRun.md)；战斗内规则见 [WacomBattle.md](./WacomBattle.md)。

---

## §8 验证入口

| 场景 | 用途 |
|---|---|
| `L_Exploration` | 完整探索 -> 世界交互 -> 战斗 / 商店 / RunEvent 流程；PIE 战斗验证走正式 `AWacomGameMode` 生命周期 |
| `L_RunExploration_Debug` | Debug builder 独占的可重建 Run scene fixture；使用 `GM_WacomRunDebug` 与 Debug Floor Descriptor |
| `L_MainMenu` | 主菜单与启动流程 |

Run Floor 场景制作验证使用 `Tools -> Wacom -> Validate Current Run Floor`；无界面验证使用 `-run=WacomValidateRunFloorScene -Map=/Game/Wacom/Maps/...`。两者共用 `WacomEditor` 的只读结构化 validator，不会修复或保存地图。`Wacom.Editor.RunSceneValidation` 覆盖诊断、几何阈值、dirty 不变量、菜单和命令退出码；`Wacom.UI.RunSceneBinding` 覆盖 Descriptor 与原子安装。

常用自动化前缀：

- App / 世界交互：`Wacom.UI.WorldInteraction`
- RunEvent UI：`Wacom.UI.Event`
- Shop UI：`Wacom.UI.Shop`
- Run menu drop target：`Wacom.UI.RunMenuDropTarget`
- Battle UI：`Wacom.UI.Battle`

## §9 World Shop purchase-only route

`AWacomShopTriggerActor::WorldShopHost` 是显式 opt-in 制作引用。请求必须是 purchase-only、Host 与玩家属于同一 World、Host Anchor 容量足够且配置合法，`AWacomPlayerController` 才把它交给 App-private `FWacomWorldShopActivityCoordinator`；缺 Host、非法 Host、容量不足、含 Card Upgrade 服务或当前输入 owner 冲突时，原始 `FRunShopVisitRequest` 与 stage request 完整回退既有 `UWacomShopScreen`，不会提前 Begin/End visit。

World route 的唯一运行时 owner 位于 PlayerController：它按 `stage -> temporary cursor look -> BeginShopVisitWithResult -> resolution -> world widgets/HUD/input` 顺序进入；整卡 intent 按 `PurchaseShopOffer -> resolution -> refresh/teardown` 顺序提交；Esc 和 EndPlay 按 exact visit token 结束。`FWacomShopVisitPresentationFlow` 是旧 ShopScreen 与 World route 共用的 App-private 事务顺序 helper，Widget/Host 不持有 `URunSession`。

Mouse `UWidgetInteractionComponent` 的领域 owner 仍由 PlayerController coordinator 管理，但组件实例附着到当前受控 Pawn；这是正式输入合同，使引擎自动 trace ignore 覆盖玩家 Capsule/子组件，避免相机起点先命中自己而吞掉免费购买。没有 Pawn 的过渡态才回退把组件附着到 PlayerController。`All + NoCapture` 下 Slate 可能先消费 mouse-down 而不调用 `PlayerController::InputKey`，因此正式左键链路是 `UWacomGameViewportClient` 的 Slate input preprocessor -> `AWacomPlayerController` 单一 native pre-UI route -> World Shop coordinator/WIC；`HandleRerouteInput()` 与 `InputKey()` 只作为其它捕获模式 adapter。pre-UI route 固定先询问 World Shop owner，再交给 App-private first-person Card Router，避免商店 press/release 穿透到手牌或世界。每次 press 记录 owner、hovered component、hit-test-visible、命中 Actor/Component 和距离，供 transient PIE 精确诊断；Widget 仍只广播 intent，不直接购买。

LookOnly 保留鼠标并冻结 Run Path 移动，默认复制 `UWacomRunPathTraversalComponent::GetLiveCursorLookProfile()`；Host 可用 `bOverrideCursorLookProfile` 提供本次活动的临时覆盖。`UWacomBattleCameraLookComponent` 的 runtime profile 与 authored Battle 字段分离，退出后不写回制作值。

仅编辑器 PIE 可使用 `Wacom.WorldShop.OpenPIEValidation`：它复制当前世界第一个有效 Shop Trigger 的前 8 个商品、关闭副本的 Upgrade、令首件免费，并在镜头前创建 `RF_Transient` 2×4 Host。`Wacom.WorldShop.ClearPIEValidation` 关闭 visit 并销毁临时 Host；两条命令都不保存地图或资产。
