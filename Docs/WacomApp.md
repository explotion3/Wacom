---
type: orchestration-spec
scope: wacom-app
status: active
updated: 2026-07-13
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

背包自由工作台的布局 owner 是 `UWacomBackpackWorkspaceStateSubsystem`：按 `URunSession` 对象身份保存活动区域和 `ZoneKey -> InstanceId -> normalized layout`，同一 Run 关闭/重开保留，新 Run 或 GameInstance 结束清空。该状态不序列化；选择、框选、扇形携带、确认暂停和鼠标捕获属于 Screen 活跃期 interaction model，Deactivate 时清理。
- 静态数据资产定义；卡牌、敌人、商店、RunEvent 等资产契约见 `WacomData.md`。
- WBP 视觉细节、卡牌展示规则和 UI 子控件刷新模型；这些见 `WacomUI.md`、UI 专题文档和 Binding 文档。

App / UI 对玩家已拥有卡提交精确 `InstanceId`，不以 Definition 指代某张具体卡；DataAsset / RunEvent 表达“某种卡”的资产语义时例外。

---

## §2 关卡与 GameMode

| 关卡 | GameMode | 职责 |
|---|---|---|
| `L_MainMenu` | `AWacomMenuGameMode` | 主菜单，不 Spawn 探索 Pawn；注入主菜单 ViewData，并处理菜单 Action、确认、退出和 travel |
| `L_Exploration` | `AWacomGameMode` | 探索主流程，持有 GameFlowState，进入 / 退出战斗，初始化探索 HUD |

当前 `EGameFlowState` 只有 `Exploration` 与 `Battle`：

- `Exploration`：允许 Run Tunnel 移动、世界交互、打开暂停 / 背包 / 商店 / RunEvent 等 GameMenu。
- `Battle`：暂停 Run Tunnel 移动，启用 Battle camera look，切换输入 profile，并 Push `UBattleHUD` 到 Game 层。

当前 `AWacomGameMode::bSaveSystemEnabled == false`，自动存档和读取路径会静默 no-op。SaveGame schema 与恢复边界见 [WacomRun.md](./WacomRun.md)。

### Local Settings 启动与事务

本地机器设置由 `UWacomGameUserSettings` 持久化到 UE 的 `GameUserSettings.ini`，与玩家档案、活动旅程、滚动备份和 `WacomRunSaveGame` 完全分离。`DefaultEngine.ini` 通过 `GameUserSettingsClassName` 注册该类型；Wacom 自定义 schema 当前为 `1`。项目平衡档是首次启动和显式恢复默认的唯一来源：当前显示器桌面分辨率、无边框窗口、VSync 开、60 FPS、高画质、四路音量 / 视角响应 / 镜头运动 100%、不反转 Y、完整闪光与完整 UI 动效；无法取得桌面分辨率时先保留当前有效分辨率，再回退 `1280 × 720`。已有有效 ini 不会在启动时被覆盖；自定义 schema 无法迁移时只重置音量、视角响应、镜头运动和表现辅助字段，不清空 UE 持有的分辨率、窗口模式或画质数据。

`UWacomSettingsSubsystem` 是唯一编辑事务 owner：`BeginEdit()` 返回唯一 token 和当前 snapshot；`Preview()` 只即时应用四路音量、视角响应 / Y 反转、镜头运动、闪光模式和 UI 动效模式；显示、VSync、帧率上限与整体画质只在 `Apply()` 时生效。错误 token、过期 token、重复编辑、重复确认和重复撤销都被拒绝且无副作用。`Cancel()` 恢复编辑前的即时预览值且不写盘。

分辨率或窗口模式变化后进入 15 秒确认态。此时引擎已应用候选视频模式，但不会调用 `SaveSettings()`；`ConfirmVideoMode()` 才确认并保存，`RevertVideoMode()` 或超时会恢复最后确认的视频模式，再保存同批次的其它设置。因此崩溃或强退不会把未确认的不可用分辨率写成下次启动配置。

分辨率选项由 `UWacomSettingsSubsystem::GetScreenResolutionOptions()` 统一生成，Screen 不直接查询平台显示模式，也不把任意当前值无条件塞回列表。项目最低支持 `1280 × 720`，固定常用档位为 `1280 × 720 / 1366 × 768 / 1600 × 900 / 1920 × 1080 / 2560 × 1440 / 3840 × 2160`：独占全屏只保留显卡和显示器实际报告支持的档位，窗口模式只保留不超过当前桌面工作区的档位；桌面原生分辨率和当前有效自定义分辨率只有在满足最低尺寸及对应模式限制时才作为额外选项保留。无边框窗口固定跟随当前桌面并禁用分辨率行；没有合法候选项时只禁用该行，不阻断其它设置。旧配置低于最低尺寸时可以显示原值，但不会继续作为选择目标，玩家首次步进会进入合法档位。

Subsystem 在 GameInstance 初始化时应用已保存的非分辨率配置，并在 Game / PIE World 就绪时应用音频总线；运行时消费者通过 native settings-changed delegate 订阅并在 teardown 时反订阅，不使用 Tick 查询设置。Run Tunnel 仍使用鼠标位置驱动视角，只叠加响应倍率和 Y 反转；镜头运动倍率同时缩放 Walk Bob 与 Camera Shake，`0` 会输出零 Bob 并停止已启动 Shake。`BP_WacomPlayerCharacter` 当前正式启用已有 `BP_RunTunnelWalkCameraShake`，并保持 `WalkBobComponent.bEnableWalkBob=false`，因此默认、半强度和关闭三档都作用于唯一的 CameraShake 路径，不叠加两套晃动。

`UWacomSettingsScreen` 是这套事务的唯一 CommonUI 页面协调器。页面覆盖显示、图形、音频、视角、辅助五类字段；App-private 字段描述表统一标签、离散 / 连续类型、格式与 Preview 白名单。Screen 在事务开始、窗口模式变化、恢复默认以及确认 / 回滚后的新事务中重新取得模式专属分辨率目录；切换模式时若当前 Draft 不合法，会在本地选择宽高距离最近的合法档位，不提前应用引擎视频模式。无边框窗口下分辨率仍显示，但禁用并标注“跟随桌面”。“恢复默认”从 Subsystem 获取完整项目平衡档，使用当前 token 只预览允许即时预览的字段并装入 Draft，不弹额外确认框、不会直接写盘；玩家仍需 Apply，视频变化继续走 15 秒确认。Apply 无视频模式变化时保存后留在页面并开启新 token；有视频模式变化时 Push `UWacomSettingsConfirmationDialog`，保留、恢复、超时和 Modal Push 失败都回到实际 Snapshot 并开启新 token。脏状态返回会先确认放弃；外部 teardown 会 Cancel 活动 edit 或 Revert 待确认视频模式。

主菜单与暂停菜单都只调用 App-private `FWacomSettingsScreenFlow`。该 flow 统一解析 `UI.Widget.SettingsScreen` 的软类、异步 Push 到 `UI.Layer.GameMenu`、拒绝重复打开，并在资产缺失时回退 `UWacomSettingsScreen`。主菜单 ViewData 现在开放 Settings，Journey History / Credits 仍隐藏；暂停菜单使用同一 Screen，关闭后由 CommonUI focus restoration 回到原入口。Screen、选项行和确认 Modal 均不访问 SaveGame。

主菜单继续使用 `L_MainMenu + AWacomMenuGameMode + UWacomMainMenuScreen`。菜单 travel 目标必须使用 UE package path：

- Exploration：`/Game/Wacom/Maps/L_Exploration`
- MainMenu：`/Game/Wacom/Maps/L_MainMenu`

不要把 travel 目标写成 `/Game/Wacom/Maps/L_Exploration.L_Exploration` 这类 ObjectPath。UE PIE 下 ObjectPath travel 曾触发 `FPackagePath::TryFromMountedName was passed an ObjectPath` 和 `!NewPIEWorld->bIsWorldInitialized` ensure。

主菜单采用 App flow 与 Screen 分离：`AWacomMenuGameMode` 构造 `FWacomMainMenuViewData`，绑定 `UWacomMainMenuScreen::OnActionRequestedNative`，并处理 `EWacomMainMenuAction`；Screen 只应用 ViewData、刷新 fallback / WBP 和上报玩家意图，不读取或删除 SaveGame，不调用 `OpenLevel()` 或退出 API。Screen class 配置已收紧为 `TSubclassOf<UWacomMainMenuScreen>`，C++ 默认加载 `/Game/Wacom/UI/Menus/WBP_MainMenuScreen`，资产缺失或加载失败时回退到原生 `UWacomMainMenuScreen`；travel 前和 `EndPlay` 都会显式解绑。当前存档总开关关闭，因此 GameMode 不访问磁盘，Continue、Journey History、Credits 保持隐藏；Settings 已开放并进入统一 Settings Screen flow，Start New Journey 仍直接走 `/Game/Wacom/Maps/L_Exploration`。未来档案服务接入后，只替换 GameMode 的 ViewData 构建与 Action flow，不让 Screen 重新拥有 slot 语义。

---

## §3 PlayerController 与 PlayerCharacter

`AWacomPlayerController` 是探索输入、RunSession 持有者、GameMenu 请求入口和世界目标路由者。

主要职责：

- BeginPlay 创建并持有 `URunSession`。
- 初始化 `UWacomInputContextCoordinatorSubsystem`，提供探索 / 战斗 mapping context 给 coordinator。
- 处理探索交互、暂停菜单、背包、商店、RunEvent 打开请求；具体 GameMenu 打开、关闭、异步 Push 和 Shop / RunEvent rollback 由私有 `FWacomExplorationScreenRouter` 承接。
- 转发战斗快捷键到当前 `UBattleHUD / UBattleSession`；Battle scene target click / probe 由 App-private `FWacomBattleSceneInteractionRouter` 承接，`AWacomPlayerController` 只保留 public façade、trace / flow protected seam 和输入入口。
- Run world hover / click / probe 由 App-private `FWacomRunWorldInteractionRouter` 承接；`AWacomPlayerController` 保留 timer、InputKey 顺序、trace protected seam、E 键 candidate list 和与 Run card drop coordinator 的 context 注入。
- 维护探索期 first-person Run Card Workspace source；默认 workspace 当前来自 Run `BattleDeck` 物理卡和可选投影入战卡，GameMenu 可通过 owned menu lease 临时接管候选持有卡显示和 Zone drop。
- 维护 Run first-person hover / inspect 详情：只在 Exploration 且 `UWacomRunFirstPersonCardSourceComponent` 判定当前 card layer source 属于 Run default source 或 active menu lease 时，通过 `URunSession::FindInstance()` 反查 owned card instance 并显示静态 `UWacomCardDetailPanel`；App-private `FWacomRunFirstPersonCardDetailController` 负责 Anchor hover delegate 绑定、hover / inspect / scrub 决策、Run detail 数据缓存和面板状态，并委托 Battle / Run 共享的 `FWacomFirstPersonCardDetailMotionController` 与 `FWacomFirstPersonCardDetailPanelHost` 处理预热、面板创建、AddToViewport、淡入淡出 / scale / follow motion、稳定换边和 teardown。`AWacomPlayerController` 只作为 UObject delegate endpoint 和 RunSession 查询入口保留。同一张卡的 inspect update / hover layout update 只更新 motion target，不重复构建详情；scrub 到另一张卡时才切换详情数据。`Inspecting` 期间详情保持并支持 scrub 切卡，正式拖拽、进入战斗、source 清理或 lease 结束时隐藏。
- 维护 Run first-person card drag：App-private `FWacomRunFirstPersonCardDragController` 负责 Anchor drag delegate 绑定、inspect 与正式拖拽分流、正式拖拽期间详情隐藏；`Inspecting` 刷新读牌详情、允许手牌内 scrub 切换 source，并用 drag view 持续驱动 Run Tunnel camera look，但不触发投放 probe。只有离开 scrub 手牌区域后，最后 Inspect 卡才能升级为正式拖拽；release / preview 统一交给 App-private `FWacomRunFirstPersonCardDropCoordinator`。Hover pointer 由 Run detail binding 转交 PlayerController，Inspect / Drag 由 drag delegate endpoint 转交 PlayerController，两者都直接使用已有 Run Tunnel 组件，不恢复旧 `FWacomFirstPersonCardCameraLookBridge`。Coordinator 以 drop transaction/router 形式按 menu lease zone adapter、world receiver adapter 处理 probe / preview / resolve / submit / debug；`AWacomPlayerController` 只保留 UObject delegate endpoint、drop target 注册转发和 RunSession / world hit 查询能力，创建 coordinator 时通过显式 context contract 注入这些能力，不再让 coordinator friend 读取 Controller 私有状态。menu owner / world receiver 继续决定提交结果。
- Run first-person source 使用 desired-state reconcile：PlayerController BeginPlay 会创建 / 绑定 RunSession 并请求探索期手牌 source 处于 active；如果当帧 RunSession、Pawn / `UWacomFirstPersonCardAnchorComponent`、GameMenu suppression 或 menu lease 条件尚未就绪，`UWacomRunFirstPersonCardSourceComponent` 会记录 pending/block reason，并在 RunSession 绑定、`SetPawn()`、GameMenu suppression 解除、menu lease 清理或 RunStateChanged 时自动重试。进入探索关、读档 bootstrap 完成和战斗返回探索都走 `PrepareExplorationRunFirstPersonCardLayer()`，该入口会清理 stale menu widgets / suppression / menu lease，再恢复默认 Run workspace source，不依赖背包关闭补刷。Run default、active menu lease、suppressed source 和 `BattleHand` 的处理权查询集中在 `UWacomRunFirstPersonCardSourceComponent`，PlayerController 不直接比较保留 source id。
- Run first-person source 写入默认 workspace 和 provider-backed menu lease 时使用 Battle / Run 共用的 `FWacomFirstPersonCardLayerSourceLifecycleFrame` 包住 presentation frame、interaction ownership 和必要清理，并统一经 `UWacomFirstPersonCardAnchorComponent::ApplyRuntimeCardLayerSourceLifecycleFrame()` 提交到 Anchor。组件通过 `URunSession::BuildRunCardWorkspaceSnapshot()` 读取 Run 层只读 workspace，并缓存 `CardInstanceId -> FRunCardWorkspaceEntry` 来源 metadata；App 不再自行遍历 RunState 四个物理区来决定候选卡。初次显示、从 GameMenu suppression 恢复、menu lease 交还默认 source、默认 source 新增卡，或 RunEvent / 菜单通过 `OwnedCardsFilter` 筛出候选卡接管 hand 时生成 `RunHandEntered` 入场 hints；普通刷新不重播。进入背包 / 事件等压制默认手牌的菜单场景写入 0 entries 的 `Suppressed` frame，让 card layer 清空手牌，而不是直接隐藏或硬清 slot motion；menu lease 不再有 entries-only raw 正式入口，必须通过 owned-card provider 给出候选卡。
- 在进入战斗、Controller EndPlay 或菜单 lease 结束时清理对应 first-person source / lease，避免 Run 展示层和 BattleHUD runtime hand 抢占同一 layer。
- GameMenu 可选走 first-person viewpoint staging：当前 Shop 和 RunEvent 使用该通用流程，Trigger 生成 entry stage request，PlayerController / router 在 UI 出现前锁定探索输入并清空 Run first-person hand，到位后再开始对应 `URunSession` 事务并 Push Screen；菜单关闭后 staged return 到 RunTunnel，完成后恢复手牌和交互提示。

`AWacomPlayerCharacter` 是第一人称探索 Pawn：

- 持有 Camera、CharacterMovement、Run Tunnel movement、cursor look driver、Battle camera look 和 `UWacomFirstPersonCardAnchorComponent`。
- 探索期由 Run Tunnel movement 消费移动输入；Run Tunnel 可读取 `UWacomFirstPersonWalkBobComponent` 的轻量本地 offset，在沿样条移动时叠加第一人称走路晃动。战斗时 suspend Run Tunnel，不 UnPossess。若 BattleTrigger 配置了 battle entry viewpoint，进入战斗时会构造 `FirstPersonViewStageRequest`，并交给 stage coordinator 把 Pawn 临时移动或平滑过渡到该第一人称镜头站位。
- 战斗期启用 Battle camera look，在战斗 base rotation 上叠加共享 cursor look offset。
- first-person anchor 只负责给 HUD / card layer 提供稳定锚点；projection/layout 细节见 [First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md)。

战斗时不 UnPossess 的原因是保持 PlayerController 的 InputComponent 活跃，让 Battle input profile 继续通过 Controller 路由到战斗 UI。

核心蓝图资产归档到 `/Game/Wacom/Core`：GameMode 放 `/Game/Wacom/Core/GameModes`，PlayerCharacter 放 `/Game/Wacom/Core/Player`，PlayerController 放 `/Game/Wacom/Core/Controllers`。Run Tunnel authoring 蓝图放 `/Game/Wacom/Run/Tunnel/Blueprints`。`/Game/Wacom/_GAME` 是历史临时目录，不再作为新增资产入口；具体目录规范见 [Content_Organization.md](./Content_Organization.md)。

`AWacomRunTunnelPaperLayerActor` 是探索纸片层的 C++ 父类：自带一个无碰撞、无阴影的 Plane StaticMeshComponent，蓝图子类可配置材质模板、Texture2D 数组、材质贴图参数名、固定索引或稳定随机种子。该 Actor 只做场景视觉 authoring，默认在 Construction / BeginPlay 创建动态材质实例并把选中的贴图写入材质参数；它不参与 Run Tunnel 移动、节点规则、世界交互或卡牌投放结算。

---

## §4 输入协调

`UWacomInputContextCoordinatorSubsystem` 是本地玩家输入上下文的唯一协调者。GameMode、PlayerController 和运行时组件只声明当前意图，Subsystem 统一应用：

- CommonUI input config。
- 鼠标显隐 / capture。
- Enhanced Input mapping context。
- PlayerController `bEnableClickEvents / bEnableMouseOverEvents` owner lease。

`UWacomGameViewportClient : UCommonGameViewportClient` 是极窄的 Slate 前置输入仲裁 owner，不替代 Coordinator 或 PlayerController。由于正式输入配置保持 `NoCapture`，ViewportClient 注册 `Game` priority 的 App-private `IInputProcessor`，在 Widget 路由前只处理“指针属于当前 GameViewport + 快捷键来源 first-person card active drag + 右键按下”的中性取消；其余输入返回未处理并继续正常路由。Viewport `HandleRerouteInput()` 和 PlayerController `InputKey()` 的同条件分支只作为其它捕获模式 fallback。

当前上下文：

| Context | CommonUI input config | IMC |
|---|---|---|
| `MainMenu` | `Menu + NoCapture`，鼠标可见 | 不启用探索 / 战斗 IMC |
| `Exploration` | `All + NoCapture`，鼠标可见 | `IMC_Exploration` |
| `Battle` | `All + NoCapture`，鼠标可见 | `IMC_Battle` |

PlayerController 上的 `PushMappingContext / PopMappingContext` helper 仍保留为兼容 / 调试入口；正式流程由 coordinator 管理。PlayerController BeginPlay、MenuGameMode BeginPlay 和 GameMode BeginPlay 都会初始化或重设 coordinator，防止 PIE 复用 Controller 时输入状态停留在上一关。

Run Tunnel 是探索期默认移动模型，不再有正式普通 FPS FreeLook 探索 profile。进入战斗时 `UWacomRunTunnelMovementComponent` 只 `Suspend`，保留当前 Segment / Distance；战斗结束后先停用 Battle camera look，再让 coordinator 回到 `Exploration`，Run Tunnel `Resume` 后继续沿原 tunnel path 移动。探索期 hover 或拖动 first-person 卡牌时，UMG 仍可处理 / 捕获鼠标；PlayerController 会把卡牌指针的归一化视口坐标临时写入 Run Tunnel cursor look override，让鼠标在卡牌上移动时视角也继续跟随。pointer leave、release、cancel 或清理卡层时恢复普通 cursor look。Run Tunnel 的 walking bob 是纯 App 表现层：正式玩家资产启用 UE `CameraShakeBase` 走路晃动，真实样条移动会启动 `BP_RunTunnelWalkCameraShake`，停步或卡在样条末端会在 `WalkCameraShakeStopGraceSeconds` 宽限后停止并走 shake blend out；suspend / segment switch / deactivate 会立即停止。该模式替代自定义 WalkBob offset，不与其叠加；镜头运动设置作为运行时倍率传给 shake，`1 / 0.5 / 0` 对应完整、半强度和完全关闭。组件仍保留“未启用 CameraShake 或未配置 class 时使用 `UWacomFirstPersonWalkBobComponent`”的通用 fallback，但 `BP_WacomPlayerCharacter` 明确关闭该 fallback，避免正式资产误叠加。若 PIE 中看不到 CameraShake 效果，可临时勾选 `bDebugWalkCameraShake`，屏幕左上角会显示运行时 class、scale、真实移动 delta、dead zone、grace、PlayerController / PlayerCameraManager 和 Start 返回实例。walking bob 不改变 `DistanceAlongSpline`、分支规则、trace 合法性或 Run 规则状态，suspend / segment switch / deactivate 会清理残留偏移。

当前 IA 口径：

| IA | 用途 |
|---|---|
| `IA_Move` | WASD 移动 |
| `IA_Look` | 鼠标视角 |
| `IA_Interact` | 探索期按 E 与最近世界交互对象互动 |
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
| 探索左键点击 | Battle scene router target click -> RunTunnel branch click -> Run world interaction router click -> `Super::InputKey()` | click 命中后仍回到 `TryInteract()` 或 BattleHUD target command |
| Run world card drop | first-person drag release -> Run world target probe -> `UWacomRunWorldCardDropReceiverComponent` | `URunSession::Validate/SubmitRunWorldCardInteraction()` |
| Run menu zone drop | active GameMenu + active menu lease -> `UWacomRunMenuDropTargetWidget` Zone probe | owning menu 决定 probe、prototype destroy 或 RunEvent payment submit |

正式 opt-in Run world clickable 对象包括 RunEventTrigger、ShopTrigger、BattleTrigger、RunPickup、RunCardPickup、RunRewardPickup 和 KeyChest。Actor 制作字段、debug summary、prototype sample button、Data Validation 和 target handle 细节见 [WacomWorldInteraction.md](./WacomWorldInteraction.md)。

打开 Backpack / Pause / Shop / RunEvent 等 GameMenu 时，不穿透点击场景目标；菜单 first-person card drag/drop 正在处理时，也会清理或抑制普通 Run world hover/click。

---

## §6 UI Shell 与菜单入口

WacomApp 负责调用 UI shell，但不在本文定义具体 UI 视觉和刷新规则。

当前 UI shell ownership：

- `UWacomGameUIManagerSubsystem` 持有 `UWacomPrimaryGameLayout`，负责 Ensure / TearDown PrimaryLayout 和按 layer Push / Pop。
- `UWacomUIDeveloperSettings` 是顶层 UI WBP 的项目级软类注册表。
- `FWacomExplorationScreenRouter` 统一背包、暂停菜单、商店和 RunEvent 的 GameMenu 打开 / 关闭 / async push / rollback。
- `UBattleHUD` 是战斗 UI 命令出口、WBP 绑定 owner、配置 owner 和 GC 引用 owner。

顶层 Screen / Toast 不再通过 `AWacomPlayerController` 暴露 ScreenClass 覆盖入口。Backpack / PauseMenu / Shop / RunEvent 通过 `UWacomUIDeveloperSettings.WidgetClasses` 的 `UI.Widget.*` tag 注册；缺失或加载失败时回到对应 C++ fallback。UI 数据流见 [WacomUI.md](./WacomUI.md)，CommonUI 层级 / Settings / Toast 见 [WacomUIFoundation.md](./WacomUIFoundation.md)，BattleHUD helper 和 Battle UI surface 见 [WacomBattleUI.md](./WacomBattleUI.md)。

菜单按钮不直接 OpenLevel；切关卡委托给 GameMode 或 PlayerController。主菜单和暂停菜单切关前先 `TearDownPrimaryLayout()`，再在下一帧 `OpenLevel()`，避免在按钮点击 / CommonUI deactivate 链中立即切关。

### GameMenu viewpoint staging

GameMenu viewpoint staging 是 Exploration GameMenu 的通用临时镜头站位流程，当前接入者是 Shop 和 RunEvent，共用同一套 PlayerController 状态和 RunTunnel return flow。`AWacomShopTriggerActor.ShopEntryViewpoint` 是商店入口的可选第一人称镜头站位；`AWacomRunEventTriggerActor.RunEventEntryViewpoint` 是事件入口的可选第一人称镜头站位。未配置时沿用普通路径：Shop 为 `BeginShopVisit -> Push ShopScreen`，RunEvent 为 `BeginRunEvent -> Push RunEventScreen`。配置后，Trigger 会构造 `FWacomFirstPersonViewStageRequest`：Shop 使用 `Reason=ShopEntry`，RunEvent 使用 `Reason=RunEventEntry`，`DebugSource` 优先使用 `PersistentId`，并复制 viewpoint 的 View Transform、blend 时间、曲线和 ease power。

staged GameMenu 打开时，`FWacomExplorationScreenRouter` 先锁定探索输入、清空 / suppress Run first-person hand，并通过 `FWacomFirstPersonViewStageCoordinator` 移动到目标 View Pose；到位后才执行对应 Screen 的 async push。ShopScreen / RunEventScreen 关闭时仍由自身 deactivate 流程执行 `EndShopVisit` / `EndRunEvent`，UI 立即退场；PlayerController 识别该 staged GameMenu 后保持手牌 suppression，调用 `FWacomFirstPersonViewStageReturnFlow::ReturnToRunTunnel()`，回到 RunTunnel 后恢复探索输入、刷新 Run first-person hand 和 interact toast。push / refresh 失败时走现有 rollback，并立即 return to RunTunnel。

Router 内部用共享私有 helper 承接 “stage first-person view -> open GameMenu -> arm return” 的握手；Shop / RunEvent 只提供各自的 async push 和 RunSession 事务，后续新 GameMenu viewpoint staging 应复用这条路径，不再复制 staging 生命周期。

GameMenu 打开前的通用 guard 同样收口在 Router 内部：Exploration 状态检查、UIManager / PrimaryLayout 就绪、关闭旧 GameMenu、pending async push 防重入由共享 prepare 处理；ShopId、PersistentId、Definition 等业务合法性仍由对应入口各自校验。

---

## §7 战斗进出流程

### EnterBattle

```text
BattleTrigger E键/点击
-> PlayerController.RequestEnterBattle(TriggerActor)
-> GameMode.EnterBattle(TriggerActor)
```

GameMode 进入战斗时：

1. 设置 `EGameFlowState::Battle`。
2. 清理探索期 Run first-person default workspace source 和 active menu lease。
3. Suspend PlayerCharacter 的 Run Tunnel 探索移动；若 Trigger 配置了 battle entry viewpoint，则先把第一人称摄像机 View Pose 对齐到该站位，再启用 Battle camera look。Viewpoint 可配置过渡时间，默认 0 秒立即对齐。
4. 由 RunSession 构造战斗玩家侧参数（角色、随机种子、备战卡组、撤离重入进度），由 Trigger 的 `EncounterDefinition` 构造敌人槽；若 RunSession 缺失或无法构造参数，GameMode 会拒绝进入战斗，不再用 `DefaultCharacter / DefaultRandomSeed` 拼 fallback 战斗。
5. 创建 / 初始化 `UBattleSession`。
6. 通过 UIManager Push `UBattleHUD` 到 Game 层。
7. 将 input coordinator 切到 `Battle` profile。
8. 记录触发战斗的 Trigger Actor，并把 Trigger 的 `SceneEnemyHostSlots` 映射传给 BattleHUD 场景目标 registry。

敌人入口只走 `EncounterDefinition + SceneEnemyHostSlots`。`EncounterDefinition.EnemySlots` 负责规则敌人槽，`SceneEnemyHostSlots` 负责 `EnemySlotId -> SceneEnemyHost` 的场景表现绑定；缺 Host、漏 slot 或多余 slot 会被编辑器验证阻止。场景敌人点击、hover 和拖卡目标路由只认当前 BattleHUD registry 中的 `EncounterId + EnemySlotId + PartSlotId`，不通过 Actor 名称、单 Host 缓存或旧第一敌人入口推断身份。

Battle entry viewpoint 是 `WacomApp` 的场景 / 镜头编排能力，不属于 `WacomRun` 或 `WacomBattle` 规则。关卡中可摆放 `AWacomFirstPersonViewpointActor`，并在 `ABattleTriggerActor.BattleEntryViewpoint` 引用它；该 Actor 的 transform 表示第一人称摄像机 View Pose，不是玩家 Capsule/root 位置。场景入口不直接操作 Pawn transform，而是先构造 `FWacomFirstPersonViewStageRequest`；BattleTrigger 是第一个 request producer，`FWacomFirstPersonViewStageCoordinator` 是 App 层 request consumer，GameMode 是当前 battle 入口调用者。进入战斗时 GameMode 会先 suspend Run Tunnel，再把 request 交给 coordinator：coordinator 按 View Pose 和摄像机相对偏移反推 Pawn root / Controller rotation，或启动平滑过渡，并在 staging 完成时激活 Battle camera look。`StageBlendTimeSeconds` 默认为 0 秒，此时立即对齐；若大于 0，则由 PlayerCharacter 的 first-person view stage blend component 从当前 View Pose 平滑移动到目标 View Pose。`StageBlendCurve` 和 `StageBlendEasePower` 控制过渡速度曲线；默认 `SmoothStep` 保持旧手感，`EaseOut` 适合快速靠近后柔和停下，`EaseInOut` 适合更明显的起止缓动。过渡期间组件使用 Battle camera look 的 yaw / pitch clamp、scale 和 interp 参数按鼠标位置叠加 cursor look offset；完成交接时 Battle camera look 的 base rotation 仍使用原始 Viewpoint，但当前 cursor look offset 会被保留，避免镜头从鼠标偏移角度回弹到原始 Viewpoint。

First-person card Anchor 在 `BattleCameraLook` 尚未激活但 stage blend active 时优先使用当前 staged base View Pose；suspended Run Tunnel 不再拥有 Anchor 空间，避免手牌锚点停在探索样条位置直到过渡结束。Anchor 在 `RunTunnel` / `BattleCamera` / `ViewStageBlend` 之间交接时使用 `CameraStageFollowInterpSpeed` 而不是普通 `FollowInterpSpeed`；该值默认 0，表示立即贴合目标镜头空间，避免 HUD 或 hand source 已刷新但锚点还在二次追随。BattleHUD / BattleSession 仍按 EnterBattle 当前时序创建；若镜头过渡被延后，BattleHUD 会先进入 `BattleInputReady=false` 且 first-person `BattleHand` runtime layer suppressed 的 staging 状态。suppressed 状态会关闭 `BattleHand` presentation gate、清空当前 card layer visual slot，并写入一个 0 entries 的空 `BattleHand` runtime source，用来阻止 Anchor 回退到 preview card layer；suppressed 期间的 Snapshot 刷新只更新普通 HUD / 场景目标，不显示或交互第一人称战斗手牌，也不推进 `FWacomBattleHandPresentationController` 的展示 baseline。

首回合 `CardsDrawn` 会作为 deferred entry reveal transaction 保留，等 Battle camera look 激活、GameMode 解除 suppression 后，再由 `FWacomBattleHandPresentationController` 用空手牌 baseline 从当前 Battle snapshot 生成一次显式 `entries + Drawn hints` presentation frame；解除后的正式 hand sync 会重新打开 presentation gate，Anchor 才能把 frame hints 交给 Layer 播放，消费后不会在后续普通刷新中重播。如果 entry reveal frame 已经提交到 Anchor 但还没被 Layer tick 消费时又进入 suppression，BattleHUD first-person hand bridge 会把该 frame 还原为 controller 的待播事件后再清空 visual source，避免卡牌在不可见阶段丢失动画或直接落位。Battle camera look 激活后，GameMode 重新从当前 Battle snapshot 刷新 first-person hand，并解锁玩家命令。未配置 viewpoint 或 blend 为 0 秒时保持旧行为，从当前探索位置立即进入可操作战斗。本阶段不做 CameraDirector 或 Level Sequence。

### ExitBattle

```text
BattleSession 结算完毕
-> BattleHUD.OnBattleEndedNative(EBattleOutcome) 通知 GameMode
-> GameMode.ExitBattle()
```

GameMode 退出战斗时：

1. Pop BattleHUD 并清理 BattleSession 引用。
2. 清空探索期 `UWacomRunFirstPersonCardSourceComponent` runtime layer，避免 Run 手牌在回程 staging 中从临时 Viewpoint 平移回 RunTunnel。
3. 将 input coordinator 切回 `Exploration` profile，但暂不恢复探索移动输入。
4. 停用 Battle camera look，但保留当前可见视角，不先拉回 battle base rotation。
5. 从当前 suspended Run Tunnel 构造 `RunTunnelReturn` stage request。
6. 通过 `FWacomFirstPersonViewStageReturnFlow` 将第一人称视角移动回 Run Tunnel 样条 View Pose。
7. stage 完成后恢复 PlayerCharacter 探索移动；若回程 blend 为 0 或无法构造 request，则同步恢复。
8. 从 `UBattleSession::BuildResultPacket()` 构造 `FBattleResultPacket`，调 RunSession 结算战斗结果，并在非 Undetermined 战斗结束后消耗 1 节点。GameMode 判断撤离异常全灭时只统计 packet 中有效的 `DestroyedPartKeys` / `EnemyResults.DestroyedPartKeys`，不再用 legacy `DestroyedParts` projection 触发场景 Trigger 销毁。
9. 真胜利时标记并销毁触发战斗的 BattleTrigger；撤离时不销毁 Trigger，允许玩家再次按 E 重入。
10. `EGameFlowState` 回到 `Exploration` 后，等待 return staging completion，再重新激活并刷新 Run first-person hand，同时刷新交互 Toast。

退出战斗回到 Exploration 后，PlayerController 会重新激活并刷新 `UWacomRunFirstPersonCardSourceComponent`，让 first-person card layer 再次显示当前默认 Run workspace。当前默认 workspace provider 仍读取 Run `BattleDeck` 物理卡和可选投影卡；这个刷新只读 Run snapshot，不提交 Run 命令；若回程是 deferred blend，刷新必须等镜头回到 RunTunnel 后再发生。

`AWacomPlayerController::RequestExitBattle(EBattleOutcome)` 仅作为外部手动结束战斗的 typed façade 保留，不再接受裸 `uint8` Outcome。正式 BattleEnd 主链路仍是 BattleHUD 根据 Snapshot BattleEnd 广播 `EBattleOutcome`，由 GameMode 在 Session 释放前生成 `FBattleResultPacket` 交给 Run 层。

若进入战斗时使用了 battle entry viewpoint，退出战斗不保存该临时站位。Run Tunnel 仍保留进入战斗前的 Segment / Distance，并负责生成 `RunTunnelReturn` stage request；`ReturnStageBlendTimeSeconds` 默认 0.35 秒，可在 `UWacomRunTunnelMovementComponent` 的 `Wacom|Run Tunnel|Staging` 分类中调整，`ReturnStageBlendCurve` 和 `ReturnStageBlendEasePower` 用同一套 stage blend 曲线语义控制回程节奏。回程过渡期间 BattleHUD 已退场、探索输入仍锁定、first-person Run 手牌为空；`FWacomFirstPersonViewStageReturnFlow` 负责调用 stage coordinator 回到样条 View Pose，并在完成后恢复探索输入。Battle exit return 恢复 RunTunnel 时会按当前鼠标位置保留 / 预热 cursor look offset，不先把镜头拉回样条中心角度，避免回程完成瞬间出现中心回弹。这个 return flow 以后也可被商店、剧情、RunEvent 等临时站位复用。

`UBattleHUD::NativeDestruct()` 可能在 Run first-person source 恢复之后才执行。BattleHUD 清理 first-person hand 时只允许清自己写入的 `BattleHand` runtime source；如果 Anchor 已经显示 `RunFirstPersonBattleDeck` 或 menu lease，只能解绑 BattleHUD delegate 和清战斗预览，不能关闭 Anchor 的 first-person card interaction。否则会出现回到 Exploration 后卡牌仍可见但无法拖拽 / 使用。

战斗结果包和 Run 结算规则见 [WacomRun.md](./WacomRun.md)；战斗内规则见 [WacomBattle.md](./WacomBattle.md)。

---

## §8 验证入口

| 场景 | 用途 |
|---|---|
| `L_Exploration` | 完整探索 -> 世界交互 -> 战斗 / 商店 / RunEvent 流程；PIE 战斗验证走正式 `AWacomGameMode` 生命周期 |
| `L_MainMenu` | 主菜单与启动流程 |

常用自动化前缀：

- App / 世界交互：`Wacom.UI.WorldInteraction`
- RunEvent UI：`Wacom.UI.Event`
- Shop UI：`Wacom.UI.Shop`
- Run menu drop target：`Wacom.UI.RunMenuDropTarget`
- Battle UI：`Wacom.UI.Battle`
