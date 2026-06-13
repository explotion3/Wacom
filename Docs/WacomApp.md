---
type: orchestration-spec
scope: wacom-app
status: active
updated: 2026-06-08
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
- first-person runtime source 所有权协调：探索期 Run BattleDeck 展示、GameMenu lease、进入战斗时清理、退出战斗后恢复。

不负责：

- 单场战斗规则真相；战斗命令必须提交给 `UBattleSession`。
- Run / 背包 / 商店 / RunEvent 规则真相；战外事务必须提交给 `URunSession`。
- 静态数据资产定义；卡牌、敌人、商店、RunEvent 等资产契约见 `WacomData.md`。
- WBP 视觉细节、卡牌展示规则和 UI 子控件刷新模型；这些见 `WacomUI.md`、UI 专题文档和 Binding 文档。

App / UI 对玩家已拥有卡提交精确 `InstanceId`，不以 Definition 指代某张具体卡；DataAsset / RunEvent 表达“某种卡”的资产语义时例外。

---

## §2 关卡与 GameMode

| 关卡 | GameMode | 职责 |
|---|---|---|
| `L_MainMenu` | `AWacomMenuGameMode` | 主菜单，不 Spawn 探索 Pawn，提供 New Game / Continue / Quit 入口 |
| `L_Exploration` | `AWacomGameMode` | 探索主流程，持有 GameFlowState，进入 / 退出战斗，初始化探索 HUD |

当前 `EGameFlowState` 只有 `Exploration` 与 `Battle`：

- `Exploration`：允许 Run Tunnel 移动、世界交互、打开暂停 / 背包 / 商店 / RunEvent 等 GameMenu。
- `Battle`：暂停 Run Tunnel 移动，启用 Battle camera look，切换输入 profile，并 Push `UBattleHUD` 到 Game 层。

当前 `AWacomGameMode::bSaveSystemEnabled == false`，自动存档和读取路径会静默 no-op。SaveGame schema 与恢复边界见 [WacomRun.md](./WacomRun.md)。

主菜单继续使用 `L_MainMenu + AWacomMenuGameMode + UWacomMainMenuScreen`。菜单 travel 目标必须使用 UE package path：

- Exploration：`/Game/Wacom/Maps/L_Exploration`
- MainMenu：`/Game/Wacom/Maps/L_MainMenu`

不要把 travel 目标写成 `/Game/Wacom/Maps/L_Exploration.L_Exploration` 这类 ObjectPath。UE 5.7 PIE 下 ObjectPath travel 曾触发 `FPackagePath::TryFromMountedName was passed an ObjectPath` 和 `!NewPIEWorld->bIsWorldInitialized` ensure。

---

## §3 PlayerController 与 PlayerCharacter

`AWacomPlayerController` 是探索输入、RunSession 持有者、GameMenu 请求入口和世界目标路由者。

主要职责：

- BeginPlay 创建并持有 `URunSession`。
- 初始化 `UWacomInputContextCoordinatorSubsystem`，提供探索 / 战斗 mapping context 给 coordinator。
- 处理探索交互、暂停菜单、背包、商店、RunEvent 打开请求；具体 GameMenu 打开、关闭、异步 Push 和 Shop / RunEvent rollback 由私有 `FWacomExplorationScreenRouter` 承接。
- 转发战斗快捷键和目标点击到当前 `UBattleHUD / UBattleSession`。
- 维护探索期 first-person Run BattleDeck source；GameMenu 可通过 owned menu lease 临时接管候选持有卡显示和 Zone drop。
- 在进入战斗、Controller EndPlay 或菜单 lease 结束时清理对应 first-person source / lease，避免 Run 展示层和 BattleHUD runtime hand 抢占同一 layer。

`AWacomPlayerCharacter` 是第一人称探索 Pawn：

- 持有 Camera、CharacterMovement、Run Tunnel movement、cursor look driver、Battle camera look 和 `UWacomFirstPersonCardAnchorComponent`。
- 探索期由 Run Tunnel movement 消费移动输入；战斗时 suspend Run Tunnel，不 UnPossess。若 BattleTrigger 配置了 battle entry viewpoint，进入战斗时会构造 `FirstPersonViewStageRequest`，并把 Pawn 临时移动到该第一人称镜头站位。
- 战斗期启用 Battle camera look，在战斗 base rotation 上叠加共享 cursor look offset。
- first-person anchor 只负责给 HUD / card layer 提供稳定锚点；projection/layout 细节见 [First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md)。

战斗时不 UnPossess 的原因是保持 PlayerController 的 InputComponent 活跃，让 Battle input profile 继续通过 Controller 路由到战斗 UI。

核心蓝图资产归档到 `/Game/Wacom/Core`：GameMode 放 `/Game/Wacom/Core/GameModes`，PlayerCharacter 放 `/Game/Wacom/Core/Player`，PlayerController 放 `/Game/Wacom/Core/Controllers`。Run Tunnel authoring 蓝图放 `/Game/Wacom/Run/Tunnel/Blueprints`。`/Game/Wacom/_GAME` 是历史临时目录，不再作为新增资产入口；具体目录规范见 [Content_Organization.md](./Content_Organization.md)。

---

## §4 输入协调

`UWacomInputContextCoordinatorSubsystem` 是本地玩家输入上下文的唯一协调者。GameMode、PlayerController 和运行时组件只声明当前意图，Subsystem 统一应用：

- CommonUI input config。
- 鼠标显隐 / capture。
- Enhanced Input mapping context。
- PlayerController `bEnableClickEvents / bEnableMouseOverEvents` owner lease。

当前上下文：

| Context | CommonUI input config | IMC |
|---|---|---|
| `MainMenu` | `Menu + NoCapture`，鼠标可见 | 不启用探索 / 战斗 IMC |
| `Exploration` | `All + NoCapture`，鼠标可见 | `IMC_Exploration` |
| `Battle` | `All + NoCapture`，鼠标可见 | `IMC_Battle` |

PlayerController 上的 `PushMappingContext / PopMappingContext` helper 仍保留为兼容 / 调试入口；正式流程由 coordinator 管理。PlayerController BeginPlay、MenuGameMode BeginPlay 和 GameMode BeginPlay 都会初始化或重设 coordinator，防止 PIE 复用 Controller 时输入状态停留在上一关。

Run Tunnel 是探索期默认移动模型，不再有正式普通 FPS FreeLook 探索 profile。进入战斗时 `UWacomRunTunnelMovementComponent` 只 `Suspend`，保留当前 Segment / Distance；战斗结束后先停用 Battle camera look，再让 coordinator 回到 `Exploration`，Run Tunnel `Resume` 后继续沿原 tunnel path 移动。探索期 hover 或拖动 first-person 卡牌时，UMG 仍可处理 / 捕获鼠标；PlayerController 会把卡牌指针的归一化视口坐标临时写入 Run Tunnel cursor look override，让鼠标在卡牌上移动时视角也继续跟随。pointer leave、release、cancel 或清理卡层时恢复普通 cursor look。

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
| 鼠标 hover | cursor trace -> interaction target provider -> Run clickable resolver 或 Battle scene target probe | 只更新提示 / 预览，不提交规则 |
| 探索左键点击 | Battle target click -> RunTunnel branch click -> Run world interactable click -> `Super::InputKey()` | click 命中后仍回到 `TryInteract()` 或 BattleHUD target command |
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
2. 清理探索期 Run first-person BattleDeck source 和 active menu lease。
3. Suspend PlayerCharacter 的 Run Tunnel 探索移动；若 Trigger 配置了 battle entry viewpoint，则先把第一人称摄像机 View Pose 对齐到该站位，再启用 Battle camera look。Viewpoint 可配置过渡时间，默认 0 秒立即对齐。
4. 由 Trigger 的 `EncounterDefinition` 构造敌人槽，并由 RunSession 补齐撤离重入进度。
5. 创建 / 初始化 `UBattleSession`。
6. 通过 UIManager Push `UBattleHUD` 到 Game 层。
7. 将 input coordinator 切到 `Battle` profile。
8. 记录触发战斗的 Trigger Actor，并把 Trigger 的 `SceneEnemyHostSlots` 映射传给 BattleHUD 场景目标 registry。

敌人入口只走 `EncounterDefinition + SceneEnemyHostSlots`。`EncounterDefinition.EnemySlots` 负责规则敌人槽，`SceneEnemyHostSlots` 负责 `EnemySlotId -> SceneEnemyHost` 的场景表现绑定；缺 Host、漏 slot 或多余 slot 会被编辑器验证阻止。场景敌人点击、hover 和拖卡目标路由只认当前 BattleHUD registry 中的 `EncounterId + EnemySlotId + PartSlotId`，不通过 Actor 名称、单 Host 缓存或旧第一敌人入口推断身份。

Battle entry viewpoint 是 `WacomApp` 的场景 / 镜头编排能力，不属于 `WacomRun` 或 `WacomBattle` 规则。关卡中可摆放 `AWacomFirstPersonViewpointActor`，并在 `ABattleTriggerActor.BattleEntryViewpoint` 引用它；该 Actor 的 transform 表示第一人称摄像机 View Pose，不是玩家 Capsule/root 位置。场景入口不直接操作 Pawn transform，而是先构造 `FWacomFirstPersonViewStageRequest`；BattleTrigger 是第一个 request producer，GameMode 是当前 request consumer。进入战斗时 GameMode 会先 suspend Run Tunnel，再按 request 的 View Pose 和摄像机相对偏移反推 Pawn root / Controller rotation。`StageBlendTimeSeconds` 默认为 0 秒，此时立即对齐；若大于 0，则由 PlayerCharacter 的 first-person view stage blend component 从当前 View Pose 平滑移动到目标 View Pose。过渡期间组件使用 Battle camera look 的 yaw / pitch clamp、scale 和 interp 参数按鼠标位置叠加 cursor look offset；完成交接时 Battle camera look 的 base rotation 仍使用原始 Viewpoint，但当前 cursor look offset 会被保留，避免镜头从鼠标偏移角度回弹到原始 Viewpoint。First-person card Anchor 在 `BattleCameraLook` 尚未激活但 stage blend active 时优先使用当前 staged base View Pose；suspended Run Tunnel 不再拥有 Anchor 空间，避免手牌锚点停在探索样条位置直到过渡结束。BattleHUD / BattleSession 仍按 EnterBattle 当前时序创建；若镜头过渡被延后，BattleHUD 会先进入 `BattleInputReady=false` 且 first-person `BattleHand` runtime layer suppressed 的 staging 状态。suppressed 状态会清空当前 card layer visual slot，并写入一个 0 entries 的空 `BattleHand` runtime source，用来阻止 Anchor 回退到 preview card layer；初始 Snapshot 刷新只更新 HUD / 场景目标，不显示或交互第一人称战斗手牌。Battle camera look 激活后，GameMode 解除 suppression、重新从当前 Battle snapshot 刷新 first-person hand，并解锁玩家命令。未配置 viewpoint 或 blend 为 0 秒时保持旧行为，从当前探索位置立即进入可操作战斗。本阶段不做 CameraDirector 或 Level Sequence。

### ExitBattle

```text
BattleSession 结算完毕
-> BattleHUD / Controller 通知 GameMode
-> GameMode.ExitBattle()
```

GameMode 退出战斗时：

1. 设置 `EGameFlowState::Exploration`。
2. Pop BattleHUD。
3. 停用 Battle camera look，并恢复 PlayerCharacter 探索移动。
4. 将 input coordinator 切回 `Exploration` profile。
5. 调 RunSession 结算战斗结果。
6. 真胜利时标记并销毁触发战斗的 BattleTrigger。
7. 撤离时不销毁 Trigger，允许玩家再次按 E 重入。
8. 非 Undetermined 战斗结束后消耗 1 节点。

退出战斗回到 Exploration 后，PlayerController 会重新激活并刷新 `UWacomRunFirstPersonCardSourceComponent`，让 first-person card layer 再次显示当前 Run BattleDeck。这个刷新只读 Run snapshot，不提交 Run 命令。

若进入战斗时使用了 battle entry viewpoint，退出战斗不保存该临时站位。`ResumeRunTunnel()` 会重新应用进入战斗前保留的 Segment / Distance，把 Pawn 和 Controller rotation 恢复到 Run Tunnel 样条进度。

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
