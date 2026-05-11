# Phase 3：Run 骨架（探索 ↔ 战斗切换）

## 1. 目标

最小可行版本：第一人称玩家在 3D 场景中自由探索，走进敌人 Overlap 范围自动触发战斗，战斗结束后敌人消失、玩家恢复探索。

**不做**：节点图、背包、奖励、存档、商店、事件、HD-2D 美术。

## 2. 设计决策

| 项 | 决定 |
|---|---|
| 战斗视角 | 保持第一人称不动，战斗 UI 覆盖在屏幕上 |
| 战斗触发 | Overlap（走进范围自动触发）|
| 战斗结束 | 敌人 Actor 消失（Destroy），玩家留在原地恢复探索 |
| 关卡切换 | 不切关卡，同一关卡内完成（隐藏/显示 + UI 切换）|
| GameMode | 自定义 `AWacomGameMode`，管理 Exploration ↔ Battle 状态 |
| Pawn | 第一人称 `AWacomPlayerCharacter`（移动 + 视角）|
| Controller | `AWacomPlayerController`，持有 RunSession 引用，管理 IMC 切换 |

## 3. 游戏状态

```
enum class EGameFlowState : uint8
{
    Exploration,   // 玩家自由移动，可触发事件
    Battle,        // 战斗进行中，移动禁用，战斗 UI 激活
};
```

## 4. 组件清单

### WacomApp（表现层）

| 类 | 职责 |
|---|---|
| `AWacomGameMode` | 管理 EGameFlowState；决定 DefaultPawnClass / HUDClass；提供 `EnterBattle` / `ExitBattle` 接口 |
| `AWacomPlayerController` | 持有 `URunSession*`；管理 IMC Push/Pop（IMC_Exploration ↔ IMC_Battle）；接收 GameMode 的状态切换通知 |
| `AWacomPlayerCharacter` | 第一人称 Pawn：Camera + Movement（WASD + 鼠标视角）；战斗时禁用移动输入但不 UnPossess |
| `ABattleTriggerActor` | 场景中的敌人触发器：SphereCollision Overlap → 通知 Controller 进入战斗；持有 `UEnemyDefinition*` 配置 |
| `ABattleTestActor`（改造）| 保留作为"战斗 UI + 战斗输入路由"的管理器，由 GameMode 在 EnterBattle 时激活 |

### WacomRun（流程逻辑）

| 类 | 职责 |
|---|---|
| `URunSession` | 一次冒险的逻辑入口：持有 `FRunState`；提供 `StartBattle(EnemyDef)` / `OnBattleFinished(Outcome)` |
| `FRunState` | 持久状态：当前 HP、当前卡组（第一阶段固定为虫妹 StarterDeck）、已击败的敌人列表 |

### WacomData（新增）

| 类 | 职责 |
|---|---|
| （暂无新增）| 第一阶段用现有 `UCharacterDefinition` + `UEnemyDefinition` 即可 |

### Enhanced Input

| 资产 | 内容 |
|---|---|
| `IMC_Exploration` | WASD 移动 + 鼠标视角（新建）|
| `IMC_Battle` | 1-7 打牌 + W/E/R/P（已有）|

## 5. 状态切换流程

### 进入战斗

```
玩家走进 ABattleTriggerActor 的 Overlap
→ ABattleTriggerActor::OnOverlapBegin
→ 通知 AWacomPlayerController::RequestEnterBattle(EnemyDef, TriggerActor)
→ Controller 调 GameMode::EnterBattle(EnemyDef)
→ GameMode:
    1. 设 State = Battle
    2. 禁用玩家移动（Character->DisableMovement）
    3. Pop IMC_Exploration → Push IMC_Battle
    4. 创建/激活 BattleSession + 战斗 UI（复用现有 BattleTestActor 逻辑或抽出来）
    5. 记录触发的 TriggerActor 引用
```

### 战斗结束

```
BattleSession 结算完毕 → Phase = BattleEnd
→ 战斗 UI 检测到 BattleEnd → 通知 Controller::OnBattleFinished(Outcome)
→ Controller 调 GameMode::ExitBattle(Outcome)
→ GameMode:
    1. 设 State = Exploration
    2. 销毁战斗 UI
    3. Pop IMC_Battle → Push IMC_Exploration
    4. 恢复玩家移动（Character->EnableMovement）
    5. Destroy 触发战斗的 ABattleTriggerActor（敌人消失）
    6. 通知 RunSession::OnBattleFinished(Outcome)（未来做奖励/推进）
```

## 6. 第一人称 Pawn

```
AWacomPlayerCharacter : ACharacter
├── UCameraComponent（第一人称摄像机）
├── UCharacterMovementComponent（默认）
├── 输入：IMC_Exploration 时响应 WASD + 鼠标
└── 战斗时：移动禁用，摄像机不动，Pawn 不 UnPossess
```

战斗时不 UnPossess 的原因：保持 PlayerController 的 InputComponent 活跃，这样 IMC_Battle 的按键仍然能通过 Controller 路由到战斗 UI。

## 7. 切片计划

| 切片 | 内容 | 验收 |
|---|---|---|
| R1 | `AWacomGameMode` + `AWacomPlayerController` + `EGameFlowState` 骨架 | PIE 启动后 GameMode 正确 Spawn Pawn，状态为 Exploration |
| R2 | `AWacomPlayerCharacter`（第一人称移动 + 视角）+ `IMC_Exploration` | PIE 中 WASD 移动 + 鼠标转视角 |
| R3 | `ABattleTriggerActor`（Overlap + EnemyDef 配置）| 走进范围后 Log 输出"触发战斗" |
| R4 | `EnterBattle` / `ExitBattle` 状态切换 | Overlap 后禁用移动 + 显示战斗 UI + 战斗可玩；战斗结束后恢复移动 + 敌人消失 |
| R5 | `URunSession` + `FRunState` 最小骨架 | RunSession 持有玩家 HP 和卡组，战斗参数从 RunState 读取而非硬编码 |

## 8. 文件位置预期

```
Source/WacomApp/
├── Public/
│   ├── GameFramework/
│   │   ├── WacomGameMode.h
│   │   ├── WacomPlayerController.h
│   │   └── WacomPlayerCharacter.h
│   └── Actors/
│       ├── BattleTriggerActor.h
│       └── BattleTestActor.h（保留，后续可能改名）
└── Private/
    ├── GameFramework/
    │   ├── WacomGameMode.cpp
    │   ├── WacomPlayerController.cpp
    │   └── WacomPlayerCharacter.cpp
    └── Actors/
        ├── BattleTriggerActor.cpp
        └── BattleTestActor.cpp

Source/WacomRun/
├── Public/
│   ├── RunSession.h
│   └── RunState.h
└── Private/
    └── RunSession.cpp

Content/Wacom/Input/
├── IMC_Exploration.uasset（新建）
├── IA_MoveForward.uasset（新建）
├── IA_MoveRight.uasset（新建）
├── IA_Look.uasset（新建）
└── ...（已有 IMC_Battle + 11 个 IA）
```

## 9. 临时决定

- **第一阶段 RunState 固定**：HP = 虫妹 BaseMaxHp + 身材加成，卡组 = StarterDeck。不做背包修改。
- **敌人配置硬编码**：`ABattleTriggerActor` 的 Details 面板直接指定 `UEnemyDefinition`。不做随机敌人生成。
- **战斗结束后不做奖励**：`OnBattleFinished` 只销毁敌人，不分发卡牌/金币。
- **不做存档**：关闭游戏 = 进度丢失。
- **BattleTestActor 保留**：作为"战斗 UI 管理器"继续使用，由 GameMode 在 EnterBattle 时动态 Spawn 或激活。

## 10. 和现有系统的关系

- `UBattleSession` 不变——仍然是战斗内核的唯一入口
- `IMC_Battle` + 11 个 IA 不变——战斗输入复用
- `PrimaryGameLayout` + `BattleHUD` 不变——战斗 UI 复用
- 现有 `L_TestBattle` 关卡可以保留作为纯战斗测试；新建一个 `L_Exploration` 关卡用于探索测试
