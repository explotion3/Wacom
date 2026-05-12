# WacomRun 模块文档

> 本文是 WacomRun 模块的设计 + 实现文档。

---

## §1 模块职责

WacomRun 负责**战斗外的持久状态和存档**。

**负责**：
- 一次冒险（Run）的逻辑入口
- 持有 FRunState（战斗外持久状态）
- 构造战斗初始化参数
- 接收战斗结束通知，更新 Run 状态
- 存档 / 读档：FRunState ↔ UWacomSaveGame ↔ 磁盘
- 场景 Actor 持久化（已销毁的触发器记录）

**不负责**：
- 单场战斗内规则细节（属于 WacomBattle）
- UI 展示（属于 WacomApp）
- 静态数据定义（属于 WacomData）

**依赖方向**：`WacomData ← WacomBattle ← WacomRun ← WacomApp`

---

## §2 URunSession

`URunSession` 是一次冒险的逻辑入口（UObject，Transient，行为层）。由 `AWacomPlayerController` 在 BeginPlay 时创建并持有。

### 公开接口

| 方法 | 职责 |
|---|---|
| `Initialize(UCharacterDefinition*)` | 初始化一次 Run（新开档时调用）|
| `ResetRunState()` | 重置为"新 Run"默认值（保留 Character）。死亡后重开用 |
| `GetRunState() const` | 只读访问当前 Run 状态 |
| `GetMutableRunState()` | 非 const 访问（仅 GameMode 内部写入用）|
| `IsRunActive() const` | 是否仍在 Run 中 |
| `BuildInitParamsForBattle(EnemyDef, OutParams)` | 构造一场战斗所需的 FBattleInitParams |
| `OnBattleFinished(Outcome, EnemyDef)` | 战斗结束通知（Victory 加入 DefeatedEnemies / Defeat 标记 bRunActive=false）|
| `MarkTriggerDestroyed(PersistentId)` | 标记一个触发器已被永久销毁 |
| `IsTriggerDestroyed(PersistentId) const` | 查询触发器是否已被销毁 |
| `SetPlayerTransform(InTransform)` | 记录玩家当前 Transform |
| `SaveToSlot(SlotName) const` | 写入指定 slot |
| `LoadFromSlot(SlotName)` | 从指定 slot 读档 |
| `HasSaveInSlot(SlotName) const` | 指定 slot 是否存在存档 |
| `BuildSaveGameFromRunState() const` | 把 RunState 拷贝到新建 UWacomSaveGame（公开以便测试）|
| `ApplySaveGameToRunState(SaveGame*)` | 把 SaveGame 字段应用到 RunState（含版本检查和资产 TryLoad）|

---

## §3 FRunState

`FRunState` 是内存数据层（USTRUCT），不直接序列化到磁盘。

### 字段清单

| 字段 | 类型 | 说明 |
|---|---|---|
| `Character` | `TObjectPtr<UCharacterDefinition>` | 玩家选择的角色（第一阶段固定为 BugGirl）|
| `BattleSeed` | `int32` | 战斗随机种子（0 = 每场独立随机）|
| `DefeatedEnemies` | `TArray<TObjectPtr<UEnemyDefinition>>` | 已击败的敌人列表 |
| `bRunActive` | `bool` | 当前 Run 是否仍在进行 |
| `DestroyedTriggerIds` | `TSet<FName>` | 已被永久销毁的场景触发器 ID 列表 |
| `PlayerTransform` | `FTransform` | 玩家在探索地图的位置/朝向 |
| `bHasPlayerTransform` | `bool` | PlayerTransform 是否有效 |

### 后续扩展（未实现）

- 跨战斗 HP 传递
- 当前卡组 / 金币 / 装备 / 各种 Buff / 事件标记

---

## §4 存档系统

### 三层分离

```
URunSession（UObject, Transient, 行为层）
    │ 持有 ↓
    ▼
FRunState（USTRUCT，内存数据层）
    │ Serialize / Deserialize ↕
    ▼
UWacomSaveGame（USaveGame，磁盘数据层）
```

**为什么分开**：
1. `FRunState` 内部用 `TObjectPtr`（直接引用）；`UWacomSaveGame` 用 `FSoftObjectPath`（按路径加载）
2. SaveGame 可以比 FRunState 多一些只用于存档的字段（版本号、时间戳、调试字段）
3. SaveGame 的字段稳定性由版本号保证；FRunState 内部结构可以随时重构
4. SaveGame 序列化可以在不启动完整游戏的情况下做单元测试

**不要做的事**：
- 不要把 FRunState 塞进 USaveGame 直接 `UPROPERTY(SaveGame)`
- 不要让 URunSession 本身继承 USaveGame
- 不要在 FBattleState 上加 SaveGame 标记

### UWacomSaveGame 字段

| 字段 | 类型 | 说明 |
|---|---|---|
| `SaveVersion` | `int32` | 版本号 |
| `SavedAtUtc` | `FDateTime` | 写入时间戳（调试/显示用）|
| `ClientBuildId` | `FString` | 可选 build 标识 |
| `CharacterAssetPath` | `FSoftObjectPath` | 当前角色资产路径 |
| `BattleSeed` | `int32` | 战斗随机种子 |
| `DefeatedEnemyAssetPaths` | `TArray<FSoftObjectPath>` | 已击败敌人资产路径列表 |
| `bRunActive` | `bool` | Run 是否活跃 |
| `DestroyedTriggerIds` | `TArray<FName>` | 已销毁触发器 ID（TArray 避免 TSet 序列化兼容问题）|
| `PlayerTransform` | `FTransform` | 玩家位置 |
| `bHasPlayerTransform` | `bool` | 位置是否有效 |

### 存档时机

| 事件 | SlotName | 备份 | 触发位置 |
|---|---|---|---|
| ExitBattle 完成 | `Main` + `Auto` | ✓ | `AWacomGameMode::ExitBattle` 末尾 |
| 玩家退出游戏 | `Main` | | `AWacomGameMode::EndPlay` |
| ESC 菜单 → 保存 | `Main` | | 手动触发 |

### 读档时机

| 事件 | 顺序 |
|---|---|
| `AWacomGameMode::BeginPlay` 完成后（延一帧）| 尝试 `Main` → 尝试 `Auto` → 新开 Run |

注意：`BeginPlay` 时玩家 Pawn 已 Spawn 在 `APlayerStart`。读档如果有 `bHasPlayerTransform == true`，把 Pawn 传送到 `PlayerTransform`。

### 双 Slot 策略

- `Main.sav`：主存档
- `Auto.sav`：自动备份
- 战斗结束后同时写入两个 slot
- 主档损坏时回退到 Auto

### 版本迁移（MigrateIfNeeded switch 链）

```cpp
static bool MigrateIfNeeded(UWacomSaveGame* SaveGame)
{
    if (SaveGame->SaveVersion > CurrentSaveVersion) return false; // 拒绝
    if (SaveGame->SaveVersion == CurrentSaveVersion) return true; // 无需迁移

    switch (SaveGame->SaveVersion)
    {
    case 0:
        // v0 → v1：初始版本，无需迁移字段
        SaveGame->SaveVersion = 1;
        [[fallthrough]];
    // case 1:
    //     // v1 → v2：例如加了 Gold 字段
    //     SaveGame->Gold = 0;
    //     SaveGame->SaveVersion = 2;
    //     [[fallthrough]];
    default:
        break;
    }
    return SaveGame->SaveVersion == CurrentSaveVersion;
}
```

**铁律**：每次升版本加一个 case，永远不改已存在的 case。

---

## §5 场景 Actor 持久化

### PersistentId

- 每个可被永久销毁的 Actor（目前是 `ABattleTriggerActor`）必须在 Details 面板填 `PersistentId`
- `PersistentId == NAME_None` 时视为"不参与存档"，触发 Warning 日志
- 同一关卡内 PersistentId 不能重复

### DestroyedTriggerIds

- `FRunState::DestroyedTriggerIds` 记录已被永久销毁的触发器 ID
- `GameMode::ExitBattle` 时把 `PendingTrigger->PersistentId` 加入列表

### Bootstrap 清理顺序

1. `ABattleTriggerActor::BeginPlay` 时询问 `URunSession` 本 id 是否已在 `DestroyedTriggerIds` 中
2. 是 → 立即 `Destroy()`（不触发 Overlap）
3. 否 → 正常运行

### 后续扩展

种类变多（宝箱、门、拾取物）时，抽 `IWacomPersistent` 接口：
```cpp
class IWacomPersistent
{
    virtual FName GetPersistentId() const = 0;
    virtual void ApplyPersistedState(const FRunState& State) = 0;
};
```

---

## §6 异常处理

| 异常 | 处理 |
|---|---|
| `LoadGameFromSlot("Main")` 返回 nullptr | 尝试 `Auto.sav`；还失败就新开 Run |
| `SaveVersion > CurrentSaveVersion` | 拒绝读档；尝试 `Auto.sav`；还失败就新开 |
| `CharacterAssetPath.TryLoad()` 返回 nullptr | 新开 Run（角色资产消失说明项目更新）|
| `DefeatedEnemyAssetPaths` 中某项加载失败 | 跳过该项，继续加载 |
| 写入磁盘失败 | 日志 Error，不崩溃；保留上次内存状态 |
| `DestroyedTriggerIds` 中的 id 在当前关卡找不到匹配 Actor | 静默忽略 |
| `PlayerTransform` 落地位置悬空或穿地 | 用关卡的 `APlayerStart` 重置 |

所有异常都走 `UE_LOG`，不用 `check` 不崩溃。玩家视角的最差后果是"存档丢了，从头开始"。
