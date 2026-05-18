# WacomApp 模块文档

> 本文是 WacomApp 模块的设计 + 实现文档。

---

## §1 模块职责

WacomApp 是**表现层和游戏主模块**（PRIMARY_GAME_MODULE）。

**负责**：
- GameMode / PlayerController / PlayerCharacter
- UI 架构（CommonUI 框架、Widget 体系、菜单系统）
- Enhanced Input 管理
- 探索场景交互
- 战斗 UI 路由
- 战斗触发 Actor

**不负责**：
- 修改战斗状态真相（只能通过 `UBattleSession` 提交命令、读取快照和事件）
- 单场战斗内规则细节
- 存档逻辑（属于 WacomRun）
- 静态数据定义（属于 WacomData）

---

## §2 关卡与 GameMode

### L_MainMenu + AWacomMenuGameMode

- 主菜单关卡
- 使用 `AWacomMenuGameMode`（不 Spawn Pawn，不做探索）
- 提供 New Game / Continue / Quit 入口

### L_Exploration + AWacomGameMode

- 探索关卡
- 使用 `AWacomGameMode`
- 管理 `EGameFlowState`（Exploration / Battle）
- 提供 `EnterBattle` / `ExitBattle` 接口
- DefaultPawnClass = `AWacomPlayerCharacter`

### L_TestBattle（独立测试）

- 纯战斗测试关卡
- 不走存档逻辑
- 保留作为开发验证用

### EGameFlowState

```cpp
enum class EGameFlowState : uint8
{
    Exploration,   // 玩家自由移动，可触发事件
    Battle,        // 战斗进行中，移动禁用，战斗 UI 激活
};
```

---

## §3 PlayerController

`AWacomPlayerController` 职责：

- 持有 `URunSession*`（BeginPlay 时创建）
- 管理 IMC Push/Pop（IMC_Exploration ↔ IMC_Battle）
- 接收 GameMode 的状态切换通知
- 战斗 IA 绑定（1-7 打牌、W 等待、E 结束回合等）
- ESC 路由（探索时打开暂停菜单，战斗时不响应或打开战斗暂停）

---

## §4 PlayerCharacter

`AWacomPlayerCharacter : ACharacter`

- 第一人称 Pawn
- `UCameraComponent`（第一人称摄像机）
- `UCharacterMovementComponent`（默认）
- 输入：IMC_Exploration 时响应 WASD + 鼠标视角（Move / Look）
- 战斗时：移动禁用（`SetExplorationInputEnabled(false)`），摄像机不动，Pawn 不 UnPossess

战斗时不 UnPossess 的原因：保持 PlayerController 的 InputComponent 活跃，IMC_Battle 的按键仍能通过 Controller 路由到战斗 UI。

---

## §5 BattleTriggerActor

`ABattleTriggerActor` 职责：

- 场景中的敌人触发器（use-key 交互模型，Stage 7 改）
- SphereCollision **仅做距离判定**：Begin/EndOverlap 维护 PlayerController 的候选 Trigger 列表
- 玩家按 IA_Interact（默认 E 键）→ PlayerController 从候选列表挑距离最近的调 `TryActivate`
- 持有 `UEnemyDefinition*` 配置
- `FName PersistentId`（必填，关卡级唯一）

### 为什么是 use-key 而非自动触发

旧模型（overlap 自动触发）有个致命漏洞：撤离回探索时玩家仍在 Sphere 内，永远不会有 EndOverlap → BeginOverlap 的循环，无法重入战斗（GDD §10.5 撤离重入）。use-key 模型用"在范围内"作为前置条件，按键作为触发点，重入天然支持。

### BeginPlay 逻辑

1. `PersistentId == NAME_None` → Warning，继续跑
2. RunSession 中 `DestroyedTriggerIds` 包含本 id → 立即 `Destroy()`（真胜利时被销毁）
3. 否则正常运行 + 注册 Begin/EndOverlap → PlayerController.Register/UnregisterCandidateTrigger

### EndPlay

向 PlayerController 反注册自己，避免悬空。

---

## §6 UI 架构

### UWacomGameInstance + UWacomGameUIManagerSubsystem

- `UWacomGameInstance`：GameInstance 子类
- `UWacomGameUIManagerSubsystem`：GameInstance Subsystem，管理 UI 生命周期
- PrimaryLayout 生命周期：由 Subsystem 持有，切关卡时 TearDown + 重建（跟随当前 PC）

### UWacomPrimaryGameLayout（四层 Stack）

| Layer | 用途 |
|---|---|
| Game | 战斗 HUD / 探索 HUD |
| GameMenu | 主菜单、暂停菜单、背包 |
| Modal | 确认框 |
| Overlay | Toast |

### Widget 基类体系

| 基类 | 职责 |
|---|---|
| `UWacomActivatableWidget` | 项目根基类（动画钩子、通用生命周期）|
| `UWacomBattleWidgetBase` | 战斗血统（Session + Snapshot 刷新）|
| `UWacomMenuWidgetBase` | 菜单血统（焦点管理、UIInputConfig(Menu)、Back 委托）|

### UWacomExplorationHUD

- Game 层锚点
- 探索状态下的 HUD（当前为 placeholder）

### MVVM 数据流（M1+M2）

Run 域 widget 用 **ViewModel + Provider 订阅模型**，避免 widget 直接订阅业务层导致的生命周期错位 bug（CommonUI Stack Activate/Deactivate vs widget Construct/Destruct 不一致）。

```
RunSession 写 API（AddPressure / OnBattleFinished / DeleteCardForGold ...）
  ↓ 末尾 NotifyRunStateChanged() 内部 Broadcast
URunSession::OnRunStateChangedNative（粗粒度多播）
  ↓ 唯一订阅者
UWacomRunViewModelProvider（GameInstance Subsystem）
  ↓ 读 RunState 字段
  ↓ 调 ViewModel.SetXxx()（UE_MVVM_SET_PROPERTY_VALUE 内置 dedupe + FieldNotify）
  ↓ 末尾 OnRunViewModelRefreshedNative.Broadcast()（粗粒度，给非 ViewBinding 的订阅方）
UWacomRunViewModel（UMVVMViewModelBase）
  ↓ 21 个 FieldNotify 字段（Phase / NodeCount / Pressure 8 条 / Capacity / Gold / ...）
Widget（C++ 直接订阅 OnRunViewModelRefreshedNative，未来 WBP 可走 ViewBinding）
  ↓ 收到事件 → 读 ViewModel → SetText
```

**当前状态（M1+M2）**：Widget 端用 C++ 父类硬编码布局 + 订阅 Provider 粗粒度事件 + 手动 SetText。FieldNotify 字段已就位但暂未被 WBP ViewBinding 消费。

**美术阶段切 WBP 时**：在 WBP Designer 里把 ViewModel 加到 widget（创建模式 = Global Viewmodel Collection，Identifier = `WacomRunViewModel`），用 View Bindings 编辑器把字段绑到 TextBlock/ProgressBar 等。WBP 启用后 C++ 父类的 SetText 路径作 fallback 保留，逐步删除。

**关键设计**：

- ViewModel 是**纯数据**，不持有 Session 指针、不订阅事件。便于单测。
- Provider 是 **GameInstance Subsystem**，跨关卡持久。声明 `Collection.InitializeDependency(UMVVMGameSubsystem)` 保证销毁顺序。
- Widget 订阅 **Provider 的粗粒度 multicast**，不订阅 RunSession——隔离业务层。
- Widget 在 NativeOnActivated 也调 `TrySubscribeAndRefresh`：CommonUI Reactivate 时补刷一次，防漏更新。

参考 DevLog：`Docs/DevLog/UI架构MVVM迁移M1M2.md`

### 战斗 UI（保持原有 Snapshot 推送模型）

战斗 UI 用 **Snapshot + Controller 推送**而非 ViewModel 订阅模型。理由：

- BattleHUD 战斗开始 Push、结束 Pop，无 Reactivate 风险
- 9 个子 widget 在同一棵树里，BattleHUD 作为 Controller 递归 RefreshFromSnapshot 自然
- Snapshot 是值类型快照，子 widget 各读不同字段，结构稳定
- Hand / EnemyParts 动态列表 ViewModel 不擅长，保留 ChildBattleWidgets 白名单递归

战斗 UI 的命令出口唯一在 BattleHUD（`Session->SubmitCommand`），子 widget 只发委托。

---

## §7 菜单系统

### MainMenuScreen

- New Game：初始化 RunSession → OpenLevel(L_Exploration)
- Continue：LoadFromSlot → OpenLevel(L_Exploration)
- Quit：退出游戏

### PauseMenuScreen

- Resume：关闭暂停菜单
- Save：调用 RunSession::SaveToSlot("Main")
- Quit to Menu：OpenLevel(L_MainMenu)

### ConfirmDialog

- 静态工厂 `UWacomConfirmDialog::Show(Title, Message, OnConfirm, OnCancel)`
- 推入 Modal 层
- 用于"确认退出"、"确认覆盖存档"等

### 按钮委托约定

- 菜单按钮不直接 OpenLevel：委托给 GameMode 控制切关卡
- Widget 不直接调用 SubmitCommand：通过委托通知 HUD，HUD 统一提交

### BackpackScreen（GDD §11）

- 入口：探索期 B 键（IA_OpenBackpack 资产手动建后接入）→ Push 到 GameMenu 层
- 临时 console command：`Wacom.OpenBackpack` / `Wacom.CloseBackpack`
- 战斗 IMC 不绑定 IA_OpenBackpack（战斗内 B 不可打开背包；OnOpenBackpackPressed 内部还有 GameMode 状态防御）
- ESC 关闭：复用 OnOpenMenuPressed 的"GameMenu 顶层 widget 直接 Deactivate"逻辑

UI 结构（垂直堆叠）：

| 区 | 容器 | 说明 |
|---|---|---|
| 顶部行 | HorizontalBox | 标题 / 金币 / 关闭按钮 |
| 删牌区 | `UWacomDeleteZoneDropTarget` | 拖入卡牌调用 `DeleteCardForGold` |
| 备战区 | `UWacomZoneDropTarget + WrapBox` | BattleDeck 卡，标题显示 N/Capacity；同时显示已入战 SpecialZone 投影卡 |
| 背包区 | `UWacomZoneDropTarget + WrapBox` | Backpack 卡，标题显示 N/FluxCapacity |
| SpecialZone 区块 | 动态 `UWacomZoneDropTarget + WrapBox` | 每张 B 主卡一个区块，标题显示主卡名与 `n/(Capacity-1)` |
| 负重区 | `UWacomZoneDropTarget + WrapBox` | 渲染 `RunState.BurdenZone` |

子控件：`UWacomDeckCardWidget`

- 主体区域：左键拖拽，生成 `UWacomCardDragOperation`（InstanceId / FromZone / FromZoneOwnerInstanceId / Definition）
- 右上角 X：保留旧入口，点击 → 弹 ConfirmDialog → DeleteCardForGold
- SpecialZone 内卡：右键切换 `bBattleEnabledInSpecialZone`
- `BattleEnabledBadge`：SpecialZone 内已选择入战的卡显示“已选”
- `ProjectedFromBadge`：BattleDeck 视觉投影卡显示“来自 [B 主卡名]”
- Delete 按钮启用规则：Intrinsic / 最后 BagProvider 禁用

DropTarget 规则：
- 普通 zone drop 调 `RunSession->MoveInstance`。
- DeleteZone drop 调 `RunSession->DeleteCardForGold`。
- `NativeOnDragOver` 只做视觉预判，例如 BattleDeck 已满且来源 Backpack 时返回 false；最终规则仍以 RunSession 返回值为准。

刷新模型：
- 操作命令 → RunSession 写状态 → `OnRunStateChangedNative` → Provider 刷 ViewModel → `OnRunViewModelRefreshedNative` → `BackpackScreen::RebuildAll()`。
- RebuildAll 已拆为 `RebuildTopStats / RebuildBattleDeckZone / RebuildBackpackZone / RebuildSpecialZones / RebuildBurdenZone`。
- UI 不做局部 patch，成功操作后从 RunState 全量重建。

---

## §8 输入协调

### CommonUI UIActionRouter leaf-most 机制

CommonUI 的 UIActionRouter 会把输入路由到"最前面的可激活 Widget"。战斗 UI 激活时，探索输入自然被屏蔽。

### IMC 资产

| IMC | 内容 |
|---|---|
| `IMC_Exploration` | WASD 移动 + 鼠标视角 + ESC 打开暂停菜单 + B 打开背包（IA 资产建好后） |
| `IMC_Battle` | 1-7 打牌 + W 等待 + E 结束回合 + R 重启 + P 刷新 HUD（不绑 IA_OpenBackpack） |

### IA 资产

| IA | 用途 |
|---|---|
| `IA_Move` | WASD 移动 |
| `IA_Look` | 鼠标视角 |
| `IA_OpenMenu` | ESC 打开菜单 |
| `IA_PlayCard1~7` | 打出手牌 1-7 |
| `IA_Wait` | 等待 |
| `IA_EndTurn` | 结束回合 |
| `IA_Restart` | 重启战斗 |
| `IA_RefreshHUD` | 刷新 HUD |
| `IA_OpenBackpack` | 打开背包（探索 IMC 绑定，资产由用户手动创建后启用） |

### 切关卡时的 IMC 重新 Push

切关卡时 PlayerController 的 InputComponent 会被重建。GameMode 在 BeginPlay 后根据当前 `EGameFlowState` 重新 Push 对应 IMC。

---

## §9 战斗流程

### EnterBattle 完整步骤

```
玩家走进 ABattleTriggerActor 的 Sphere 范围
→ ABattleTriggerActor::HandleBeginOverlap
→ AWacomPlayerController::RegisterCandidateTrigger(this)
→ ExplorationHUD 显示 Toast"按 E 战斗"

玩家按 E（IA_Interact 或 console `Wacom.Interact`）
→ AWacomPlayerController::OnInteractPressed
→ PickClosestCandidate → ABattleTriggerActor::TryActivate
→ AWacomPlayerController::RequestEnterBattle(EnemyDef, TriggerActor)
→ Controller 调 GameMode::EnterBattle(EnemyDef)
→ GameMode:
    1. 设 State = Battle
    2. 禁用玩家移动（Character->SetExplorationInputEnabled(false)）
    3. Pop IMC_Exploration → Push IMC_Battle
    4. 创建/激活 BattleSession + 战斗 UI
    5. 记录触发的 TriggerActor 引用
```

### ExitBattle 完整步骤

```
BattleSession 结算完毕 → Phase = BattleEnd
→ 战斗 UI 检测到 BattleEnd → 通知 Controller::OnBattleFinished(Outcome)
→ Controller 调 GameMode::ExitBattle(Outcome)
→ GameMode:
    1. 设 State = Exploration
    2. 销毁战斗 UI
    3. Pop IMC_Battle → Push IMC_Exploration
    4. 恢复玩家移动（Character->SetExplorationInputEnabled(true)）
    5. 真胜利时 Destroy 触发战斗的 ABattleTriggerActor（撤离时不销毁，玩家可重入）
    6. 通知 RunSession::OnBattleFinished(Outcome)
    7. MarkTriggerDestroyed(PersistentId)
    8. SaveToSlot("Main") + SaveToSlot("Auto")
```

---

## §10 测试约定

### Standalone vs PIE

- `L_TestBattle`：纯战斗测试，不走存档，不走 GameMode 状态切换
- `L_Exploration`：完整流程测试（探索 → 战斗 → 存档 → 读档）
- PIE 中"重新启动游戏"会保留 GameInstance，测试读档时注意区分

### ESC 设置

- 探索时 ESC → 打开暂停菜单
- 战斗时 ESC → 当前不响应（后续可加战斗暂停）
- 菜单中 ESC → 关闭当前菜单（Back 委托）

### DefaultGameMode 区分

- `L_Exploration` / `L_MainMenu` 各有自己的 GameMode
- `L_TestBattle` 使用默认 GameMode 或 BattleTestActor 直接管理
- 存档逻辑只在 `AWacomGameMode` 中，不在父类
