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

- 场景中的敌人触发器
- SphereCollision Overlap → 通知 Controller 进入战斗
- 持有 `UEnemyDefinition*` 配置
- `FName PersistentId`（必填，关卡级唯一）
- `bConsumeOnTrigger`：战斗结束后是否销毁（默认 true）

### BeginPlay 逻辑

1. `PersistentId == NAME_None` → Warning，继续跑
2. RunSession 中 `DestroyedTriggerIds` 包含本 id → 立即 `Destroy()`（不触发 Overlap）
3. 否则正常运行

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

---

## §8 输入协调

### CommonUI UIActionRouter leaf-most 机制

CommonUI 的 UIActionRouter 会把输入路由到"最前面的可激活 Widget"。战斗 UI 激活时，探索输入自然被屏蔽。

### IMC 资产

| IMC | 内容 |
|---|---|
| `IMC_Exploration` | WASD 移动 + 鼠标视角 + ESC 打开暂停菜单 |
| `IMC_Battle` | 1-7 打牌 + W 等待 + E 结束回合 + R 重启 + P 刷新 HUD |

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

### 切关卡时的 IMC 重新 Push

切关卡时 PlayerController 的 InputComponent 会被重建。GameMode 在 BeginPlay 后根据当前 `EGameFlowState` 重新 Push 对应 IMC。

---

## §9 战斗流程

### EnterBattle 完整步骤

```
玩家走进 ABattleTriggerActor 的 Overlap
→ ABattleTriggerActor::OnOverlapBegin
→ 通知 AWacomPlayerController::RequestEnterBattle(EnemyDef, TriggerActor)
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
    5. Destroy 触发战斗的 ABattleTriggerActor（bConsumeOnTrigger 时）
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
