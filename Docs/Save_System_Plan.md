# 存档系统规划

本文定义 Wacom 项目的存档（SaveGame）系统骨架。当前阶段只落骨架和关键 invariant，
等策划文档到位后再按增量加字段。

---

## 1. 目标与非目标

### 目标

- Run 外层状态持久化：角色选择、已击败敌人、玩家在场景中的位置 / 朝向、已被触发过的 Trigger
- 战斗结束时自动存档、游戏退出前自动存档、玩家 ESC 菜单可手动存档
- 启动游戏自动尝试读档：读到就继续，读不到就新开
- 一份主存档 + 一份自动备份（主档损坏时回退）
- 版本号机制：加字段不破存档、必要时走迁移

### 非目标（第一版不做）

- 多槽位（Slot 1 / 2 / 3）
- 战斗中断档恢复（战斗必须一口气打完）
- 自动探索存档（走几步就存）
- 云存档
- 存档加密 / 防篡改
- 主菜单 / 新游戏·继续菜单（等策划 + UI 美术）

---

## 2. 决策摘要

| 项 | 决定 |
|---|---|
| 作用域 | 单存档覆盖式 + 自动备份（`Main.sav` + `Auto.sav`）|
| 触发时机 | 战斗结束后自动 + ESC 菜单手动 + 游戏退出前自动 |
| 场景 Actor 状态 | 手配 `FName PersistentId`，存 `TSet<FName> DestroyedTriggerIds` |
| 玩家位置 | 存 `FTransform`（下次启动回到离开位置）|
| 战斗中退出 | 丢弃战斗进度，下次从上一个存档继续 |
| 主菜单 | 不做，启动即尝试读档 |
| 存档格式 | UE 原生 `USaveGame` + `SaveGameToSlot`（二进制） |
| 版本机制 | 显式 `int32 SaveVersion` 字段 + `CurrentSaveVersion` 常量 |
| 资产引用 | SaveGame 中使用 `FSoftObjectPath`，避免硬引用保护资产 |

---

## 3. 数据分层

**三层分离原则**：运行时对象 ≠ 内存数据 ≠ 磁盘数据。

```
URunSession（UObject, Transient, 行为层）
    │ 持有 ↓
    ▼
FRunState（USTRUCT，内存数据层）
    │ Serialize / Deserialize ↕
    ▼
UWacomSaveGame（USaveGame，磁盘数据层）
```

**为什么分开：**

1. `FRunState` 内部可以用 `TObjectPtr<UCharacterDefinition>`（直接引用）；`UWacomSaveGame` 必须用
   `FSoftObjectPath`（按路径加载，不阻塞资产卸载）
2. SaveGame 可以比 FRunState 多一些只用于存档的字段（版本号、时间戳、调试字段）
3. SaveGame 的字段稳定性由版本号保证；FRunState 内部结构可以随时重构不影响存档兼容
4. SaveGame 序列化可以在不启动完整游戏的情况下做单元测试

**不要做的事：**
- 不要把 `FRunState` 塞进 `USaveGame` 直接 `UPROPERTY(SaveGame)`
- 不要让 `URunSession` 本身继承 `USaveGame`
- 不要在 `FBattleState`（战斗内部状态）上加 `SaveGame` 标记

---

## 4. 字段清单（第一版）

### `FRunState`（内存，运行时）

```
struct FRunState
{
    TObjectPtr<UCharacterDefinition> Character;              // 已存在（R5）
    int32 BattleSeed;                                        // 已存在（R5）
    TArray<TObjectPtr<UEnemyDefinition>> DefeatedEnemies;    // 已存在（R5）
    bool bRunActive;                                         // 已存在（R5）

    // 本次新增：
    TSet<FName> DestroyedTriggerIds;                         // 场景 Actor 销毁清单
    FTransform PlayerTransform;                              // 玩家在探索地图的位置 / 朝向
    bool bHasPlayerTransform;                                // PlayerTransform 是否有效
};
```

### `UWacomSaveGame`（磁盘）

```
class UWacomSaveGame : public USaveGame
{
    int32 SaveVersion;                                       // 版本号，见 §5
    FDateTime SavedAtUtc;                                    // 调试 / 显示用
    FString ClientBuildId;                                   // 可选，调试用

    FSoftObjectPath CharacterAssetPath;                      // 指向 UCharacterDefinition
    int32 BattleSeed;
    TArray<FSoftObjectPath> DefeatedEnemyAssetPaths;
    bool bRunActive;

    TArray<FName> DestroyedTriggerIds;                       // TSet 落盘用 Array
    FTransform PlayerTransform;
    bool bHasPlayerTransform;
};
```

### `ABattleTriggerActor` 新增

```
UPROPERTY(EditAnywhere, Category = "Wacom|Persistence")
FName PersistentId;                                          // 必填，关卡级唯一
```

`BeginPlay` 时询问 `URunSession` 是否本地 id 已在 `DestroyedTriggerIds` 中；是则立即 `Destroy()`
（不触发 Overlap）。

---

## 5. 版本机制

### 约定

- 头文件里定义 `constexpr int32 UWacomSaveGame::CurrentSaveVersion = 1;`
- 每次加字段 / 改字段语义 → `CurrentSaveVersion++`
- 读档流程：
  1. `LoadGameFromSlot` 得到 `UWacomSaveGame*`
  2. 比较 `SaveGame->SaveVersion` 和 `CurrentSaveVersion`
  3. 相等 → 直接用
  4. `SaveGame->SaveVersion` 比 `CurrentSaveVersion` 小 → 走 `MigrateSaveGame(SaveGame)` 逐版本补齐
  5. 比当前大 → 拒绝读档（旧版本客户端读到新存档），回退到自动备份或新开

### 迁移函数骨架

```
void MigrateSaveGame(UWacomSaveGame* Save)
{
    switch (Save->SaveVersion)
    {
    case 1:
        // v1 → v2：例如加了 Gold 字段，默认 0
        Save->Gold = 0;
        Save->SaveVersion = 2;
        [[fallthrough]];
    case 2:
        // v2 → v3：...
    // 最后一个 case 不走 fallthrough，停在 CurrentSaveVersion
    default:
        break;
    }
}
```

每次升版本加一个 case，永远不改已存在的 case。这是版本机制的铁律。

---

## 6. 资产引用规则

**磁盘存 `FSoftObjectPath`，不存 `TObjectPtr`。**

读档时：
```
UCharacterDefinition* Char = Cast<UCharacterDefinition>(
    Save->CharacterAssetPath.TryLoad());
```

好处：
- 游戏运行时可以异步加载这些资产，不会因为存档引用而全部提前 Load
- 资产路径变更可以通过 Asset Redirector 处理
- 存档可以引用"尚未加载"的资产

**代价**：要处理 `TryLoad()` 返回 nullptr 的情况（资产被删了）——回退策略参见 §7。

---

## 7. 异常 / 回退策略

| 异常 | 处理 |
|---|---|
| `LoadGameFromSlot("Main")` 返回 nullptr | 尝试 `Auto.sav`；还失败就新开 Run |
| `SaveVersion > CurrentSaveVersion` | 拒绝读档；尝试 `Auto.sav`；还失败就新开 |
| `CharacterAssetPath.TryLoad()` 返回 nullptr | 新开 Run（角色资产消失说明项目更新，旧存档无意义） |
| `DefeatedEnemyAssetPaths` 中某项加载失败 | 跳过该项，继续加载（击败列表不影响玩法）|
| 写入磁盘失败 | 日志 Error，不崩溃；保留上次内存状态 |
| `DestroyedTriggerIds` 中的 id 在当前关卡找不到匹配 Actor | 静默忽略（关卡可能改了）|
| `PlayerTransform` 落地位置悬空或穿地 | 静默用关卡的 `APlayerStart` 重置 |

所有异常都走 `UE_LOG(LogTemp, Warning/Error)`，不用 `check` 不崩溃。玩家视角的
最差后果是"存档丢了，从头开始"——比崩溃好得多。

---

## 8. 存档时机详解

### 自动存档触发点

| 事件 | SlotName | 备份 | 触发位置 |
|---|---|---|---|
| ExitBattle 完成 | `Main` + `Auto` | ✓ | `AWacomGameMode::ExitBattle` 末尾 |
| 玩家退出游戏 | `Main` | | `AWacomGameMode::EndPlay` |

### 手动存档触发点

| 事件 | SlotName | 备份 |
|---|---|---|
| ESC 菜单 → 保存 | `Main` | |

### 读档时机

| 事件 | 顺序 |
|---|---|
| `AWacomGameMode::BeginPlay` 完成后 | 尝试 `Main` → 尝试 `Auto` → 新开 Run |

注意：`BeginPlay` 时玩家 Pawn 已 Spawn 在 `APlayerStart`。读档如果有
`bHasPlayerTransform == true`，把 Pawn 传送到 `PlayerTransform`。

---

## 9. 场景 Actor 状态详解

### PersistentId 规则

- 每个可被永久销毁的 Actor（目前是 `ABattleTriggerActor`）必须在 Details 面板填 `PersistentId`
- `PersistentId == NAME_None` 时视为"不参与存档"，会触发一条 Warning 日志
- 同一关卡内 PersistentId 不能重复；重复时 `BeginPlay` 报 Error

### 自动化校验（以后加）

可以写一个 Editor 工具 / Commandlet：遍历所有关卡的 `ABattleTriggerActor`，检测：
- NAME_None 的
- 重复的

第一版先靠肉眼。

### 和 `IWacomPersistent` 接口的关系

目前只有一种可销毁 Actor，没必要抽接口。以后种类变多（宝箱、门、拾取物）时，
抽一个接口：

```
class IWacomPersistent
{
    virtual FName GetPersistentId() const = 0;
    virtual void ApplyPersistedState(const FRunState& State) = 0;
};
```

然后 `GameMode::BeginPlay` 之后遍历所有实现了接口的 Actor 批量应用状态。这是
第二阶段的事。

---

## 10. 对现有代码的影响

| 位置 | 改动 | 切片 |
|---|---|---|
| `WacomRun/Public/WacomSaveGame.h` + `.cpp` | 新增 `UWacomSaveGame` | S1 |
| `WacomRun/Public/RunState.h` | 加 `DestroyedTriggerIds` / `PlayerTransform` / `bHasPlayerTransform` 字段 | S1 |
| `WacomRun/Public/RunSession.h` + `.cpp` | 加 `SaveToSlot` / `LoadFromSlot` / `HasSaveInSlot` / `ResetRunState` | S1, S2 |
| `WacomTests/.../SaveGameRoundtripSpec.cpp` | 新增：测试序列化往返无损 | S1 |
| `WacomApp/WacomGameMode.cpp` | `ExitBattle` 末尾触发存档；`BeginPlay` 后触发读档；`EndPlay` 退出前存档 | S2 |
| `WacomApp/Actors/BattleTriggerActor.h` + `.cpp` | 加 `FName PersistentId`；BeginPlay 时查询 RunSession 判断是否已被销毁 | S3 |
| `WacomApp/GameFramework/WacomPlayerCharacter.cpp` 或 GameMode | 读档后把 Pawn 传送到 `PlayerTransform` | S3 |
| `WacomApp/GameFramework/WacomGameMode.cpp` | `ExitBattle` 时把 `PendingTrigger->PersistentId` 加进 `DestroyedTriggerIds` | S3 |

---

## 11. 切片计划

### S1：SaveGame 基础设施（约 1 天）

**目标**：`FRunState` ↔ `UWacomSaveGame` 往返序列化无损，可被自动化测试覆盖。

- 新增 `UWacomSaveGame` 类，定义所有字段 + `CurrentSaveVersion = 1`
- `URunSession::SaveToSlot(SlotName)`：字段拷贝到 SaveGame，`SaveGameToSlot`
- `URunSession::LoadFromSlot(SlotName)`：`LoadGameFromSlot`，版本检查，字段拷贝回 FRunState
- `URunSession::HasSaveInSlot(SlotName)`：`DoesSaveGameExist`
- `URunSession::ResetRunState()`：新开 Run 用
- 自动化测试：`SaveGameRoundtripSpec.cpp`
  - 构造 FRunState（含 Character、DefeatedEnemies、DestroyedTriggerIds、PlayerTransform）
  - Save → Load 后字段逐项相等
  - Save 后反射修改 SaveVersion 到 999，Load 后应返回失败
  - Load 不存在的 Slot 应返回失败

**验收**：编译过 + 自动化测试全绿 + 测试包括版本号路径。

### S2：接入 RunSession + GameMode（约 0.5 天）

**目标**：战斗结束自动存档；启动自动读档；退出游戏前存档。

- `GameMode::BeginPlay` 末尾：尝试 `Main` → `Auto` → 新开
- `GameMode::ExitBattle` 末尾：`Save("Main")` + `Save("Auto")`
- `GameMode::EndPlay`：`Save("Main")`
- 日志：每次存档 / 读档打一条 Display 日志

**验收**：PIE 场景中击败 Snake，退出，重进，日志显示"读到存档"；DefeatedEnemies 包含 Snake。
手动删 `Main.sav`，再次进入应回退到 `Auto.sav`。

### S3：场景级状态（约 0.5–1 天）

**目标**：Trigger 被销毁后永远不再出现；玩家位置持久化。

- `ABattleTriggerActor` 加 `FName PersistentId`
- `BeginPlay` 时：
  - `PersistentId == NAME_None` → Warning，继续跑
  - RunSession 中 `DestroyedTriggerIds` 包含本 id → 立即 Destroy
- `GameMode::ExitBattle` 时把 `PendingTrigger->PersistentId` 加入 `DestroyedTriggerIds`
- `GameMode::BeginPlay` 读档后（存档有 `bHasPlayerTransform`）把 Pawn Teleport 到 `PlayerTransform`
- 玩家退出游戏 / 手动存档时把 Pawn 当前 `GetActorTransform()` 写入 FRunState

**验收**：
- 在 L_Exploration 放两个 BattleTriggerActor（PersistentId = `Trigger_Snake_A` / `Trigger_Snake_B`），
  各配一个 Snake
- 击败 A → 退出 → 重进：A 不再存在，B 仍在原位
- 击败 B → 退出 → 重进：两个都不再存在
- 在探索中走到 (X, Y) 退出，重进时 Pawn 出现在 (X, Y)

### S4：版本迁移骨架（约 0.5 天）

**目标**：验证版本机制可用，为未来加字段做准备。

- 在 `UWacomSaveGame` 内部实现 `MigrateSaveGame(UWacomSaveGame*)` 骨架
- 手写一个假的 "v1 → v2" 迁移示例（即使当前版本还是 v1，代码路径存在）
- 自动化测试：构造 `SaveVersion = 0` 的 SaveGame，走迁移后变成 `CurrentSaveVersion`
- 文档示例写法加到本文 §5

**验收**：迁移测试绿，本文档 §5 的范例和代码一致。

---

## 12. 测试清单

### 单元测试（WacomTests）

S1 必须覆盖：
- [ ] Save/Load 字段完全相同（Character / DefeatedEnemies / DestroyedTriggerIds / PlayerTransform / BattleSeed / bRunActive）
- [ ] `HasSaveInSlot` 在 Save 前后的返回值
- [ ] 版本号大于 Current 时读档失败
- [ ] 不存在 Slot 读档失败
- [ ] `CharacterAssetPath` 指向已删除资产时 `TryLoad` 返回 nullptr，处理不崩

S4 必须覆盖：
- [ ] SaveVersion = 0 的 SaveGame 经过 MigrateSaveGame 后 SaveVersion == CurrentSaveVersion

### 手动测试（PIE）

S2 / S3 阶段在 L_Exploration 手动验证：
- [ ] 打败敌人后退出游戏，重进仍记得
- [ ] 手动删除 `Main.sav`，启动后自动从 `Auto.sav` 恢复
- [ ] 删除全部存档，启动正常开新 Run，无 ensure / check
- [ ] 两个 Trigger 销毁状态相互独立
- [ ] 玩家位置 / 朝向正确恢复

---

## 13. 和文档的同步

本阶段完成后同步：

- `Architecture.md`：模块职责 WacomRun 一栏加一行"存档持久化"
- `Dev_Log.md`：追加 Phase 3.5 存档骨架完成条目
- `Phase2_Temporary_Decisions.md`：把"SaveGame：先不写"删除或标记为已正式化

---

## 14. 可能踩的坑

- **`FTransform` 默认不会被 UPROPERTY(SaveGame) 自动序列化**：要确认；如果不行改成拆三个字段（Location / Rotation / Scale）存
- **`FSoftObjectPath::TryLoad` 在 Game Thread 里是同步的**：启动读档时玩家感受不到，但要避免在战斗内每帧调
- **`SaveGameToSlot` 是阻塞 IO**：在 `EndPlay` 里调用会略微延长退出时间；可接受但要知道。异步版本 `AsyncSaveGameToSlot` 存在但第一版不用
- **`TSet<FName>` 在 `UPROPERTY(SaveGame)` 上的支持**：不确定，所以 `UWacomSaveGame` 里存 `TArray<FName>`；内存里再转 `TSet<FName>` 用
- **Editor 里"重新启动游戏"会把 GameInstance 保留**：测试读档时注意区分"PIE 重启"和"Editor 重启"
- **`DefaultGameMode` 在 L_TestBattle 和 L_Exploration 之间不同**：只有 L_Exploration 跑存档，L_TestBattle 还是纯战斗测试；确保 GameMode Save 逻辑只在 `AWacomGameMode` 而不是父类
