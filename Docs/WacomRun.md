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

#### 生命周期 / 状态访问

| 方法 | 职责 |
|---|---|
| `Initialize(UCharacterDefinition*)` | 初始化一次 Run（新开档时调用）。读 Character 的 FingerCount/HpPerFinger，复制 StarterDeck 到 Backpack/BattleDeck，重置时段 = Morning |
| `ResetRunState()` | 重置为"新 Run"默认值（保留 Character）|
| `GetRunState() const` | 只读访问当前 Run 状态 |
| `GetMutableRunState()` | 非 const 访问（仅 GameMode 内部写入用）|
| `IsRunActive() const` | 是否仍在 Run 中（bRunActive == true）|
| `IsRunFailed() const` | Run 是否失败（bRunActive=false OR 压力满 OR 手指=0）|

#### 手指（GDD §3.1 / §3.4）

| 方法 | 职责 |
|---|---|
| `GetFingerCount() const` | 当前手指数 |
| `IsFingerDepleted() const` | 手指是否 = 0 |
| `RemoveFinger(Count)` | 失去手指；同步增加 Disability 压力（每指 +5%）|

#### 压力（GDD §3.2）

| 方法 | 职责 |
|---|---|
| `GetPressureValue(Type) const` | 读单条压力 |
| `GetTotalPressure() const` | 读 8 条加和 |
| `AddPressure(Type, Delta)` | 增量压力（可负，clamp [0, 100]）|
| `SetPressure(Type, Value)` | 覆盖压力（用于幂等型，如负重）|
| `RemovePressure(Type, Amount)` | AddPressure 的负向命名别名 |
| `ClearPressure(Type)` | 单条归零 |
| `IsPressureCapReached() const` | 总和是否 ≥ 100 |

#### 战外行为触发（GDD §3.2）

| 方法 | 职责 |
|---|---|
| `OnRightHandDestructiveAction()` | 节点事件分支选"右手破坏" → 伤口 +1%（Stage 9 接入调用）|
| `OnCompanionCardPermanentlyDestroyed()` | 永久销毁伙伴卡 → 嗜血 +1%（Stage 4 背包接入调用）|
| `OnTheftCommitted()` | 完成偷窃 → 劣迹 +(n*(n+1)/2 +1)%（Stage 9 接入调用）|
| `RecomputeBurden()` | 背包变动后调用 → 负重 = (overCount * (overCount+1)) / 2（Stage 4 接入调用）|

#### 时段定时副作用（自动）

`AdvanceToNextPhase` 自动触发：
- 进入 Morning → 饥饿 +5%；若 PrevPhase=Sunrise（跨日）腐朽 +5%
- 进入 Dusk → 饥饿 +5%
- 进入 Sunrise → 疲劳 +10%

#### 经验 / 技能（GDD §3.3）

| 方法 | 职责 |
|---|---|
| `GetExperienceCurrent / Capacity()` | 读经验值 |
| `GetAcquiredSkillCount()` | 已获得技能数 |
| `AddExperience(Amount)` | 增加经验；满 Capacity 时入账技能（占位 SkillSlot.Placeholder）并扣减 Capacity（可多次）|

#### 时段 / 节点（GDD §8）

| 方法 | 职责 |
|---|---|
| `GetCurrentTimePhase()` | 当前时段 |
| `GetRemainingNodeCount()` | 当前时段剩余节点 |
| `GetCurrentDayNumber()` | 当前天数 |
| `ConsumeNode(Count=1)` | 消耗节点；归零时自动 AdvanceToNextPhase |
| `AdvanceToNextPhase()` | 推进时段（一般由 ConsumeNode 自动触发；留 public 调试用）|

#### 背包 / 备战卡组（GDD §11）

| 方法 | 职责 |
|---|---|
| `GetBackpack() / GetBattleDeck()` | 只读访问 |
| `GetFluxCapacity() const` | 通量内容容量（动态：Σ(玩家拥有所有 A 类容器卡 `max(Capacity - 1, 0)`））|
| `GetBattleDeckCapacity() const` | 备战区容量（动态：Σ 玩家拥有的所有容器卡 Capacity，A/B 类都计入）|
| `IsContainerCard(Card) static` | 卡是否容器（Capacity > 0）|
| `IsTypeAContainerCard(Card) static` | 卡是 A 类容器卡（容器 + CapacityEffect 为空，计入 Flux）|
| `IsTypeBContainerCard(Card) static` | 卡是 B 类容器卡（容器 + CapacityEffect 有效，开辟特殊存放区）|
| `GetSpecialZoneCapacity(BCard) static` | B 类容器卡的特殊存放区容量 = `Capacity - 1`（clamp 到 0）|
| `CollectTypeBContainers(OutOwnerInstanceIds) const` | 按 `RunState.SpecialZones` 顺序枚举玩家拥有的 B 主卡 instance id，输出不含悬空 owner |
| `GetSpecialZoneCapacityFor(OwnerInstanceId) const` | 按 B 主卡 instance 查询 SpecialZone 容量，公式 `Max(0, Capacity - 1)` |
| `GetSpecialZone(OwnerInstanceId, Out) const` | 按 owner instance 查询 SpecialZone 快照 |
| `FindInstance(InstanceId, OutInstance, OutZone, OutOwnerInstanceId) const` | 全区查找 instance 当前所在 zone |
| `MoveInstance(InstanceId, ToZone, ToOwnerInstanceId)` | 通用迁移入口，支持 Backpack / BattleDeck / SpecialZone / BurdenZone |
| `SetSpecialZoneCardBattleEnabled(InstanceId, bEnabled)` | SpecialZone 内卡牌切换是否随 B 主卡入战 |
| `IsBagProviderCard(Card) static` | 卡是否带 BagProvider 关键词 |
| `IsDeleteProviderCard(Card) static` | 卡是否带 DeleteProvider 关键词（GDD §11.7）|
| `IsDeleteFunctionAvailable() const` | 删牌功能是否可用（Backpack 至少一张 DeleteProvider）。第一阶段 UI 不读 |
| `IsIntrinsicCard(Card) static` | 卡是否固有（Rarity = Intrinsic）|
| `IsBackpackUiAvailable() const` | 背包 UI 是否可打开（至少一张 BagProvider）|
| `IsCardInBackpack(Card) / IsCardInBattleDeck(Card)` | 查询 |
| `AddCardToBackpack(Card)` | 加卡进背包 + RecomputeBurden |
| `AcquireCardToRun(Card)` | 战外获得卡统一入口；当前等价于加入背包并重算负重，后续战斗奖励、节点事件、商店、探险奖励都优先走这里 |
| `DestroyCardFromBackpack(Card)` | 永久销毁（含 Intrinsic / 最后 BagProvider 拒绝 / Companion 嗜血）|
| `DeleteCardForGold(Card)` | 删牌区入口：销毁 + 按稀有度发金币（白=1 / 蓝=2）|
| `AddCardToBattleDeck(Card)` | 从 Backpack 移到 BattleDeck（互斥）|
| `RemoveCardFromBattleDeck(Card)` | 从 BattleDeck 移回 Backpack（Intrinsic 拒绝）|

#### 经济（GDD §11.7）

| 方法 | 职责 |
|---|---|
| `GetGold() const` | 当前金币 |
| `AddGold(Amount)` | 增加金币 |
| `RemoveGold(Amount)` | 减少金币（余额不足返回 false）|

#### 商店购买

| 方法 | 职责 |
|---|---|
| `BeginShopVisit(ShopId, Offers)` | 开始访问指定商店节点。`ShopId` 第一次出现时用调用方传入的 Offers 初始化库存；已存在时保留原库存和已购买状态，忽略新 Offers |
| `EndShopVisit()` | 结束商店访问；如果本次访问买过至少一件商品，则消耗 1 节点，否则不消耗 |
| `IsShopVisitActive()` | 当前是否处于商店访问 |
| `HasCurrentShopVisitPurchase()` | 当前商店访问是否已买过至少一件商品 |
| `BuildCurrentShopSnapshot()` | 构建当前商店只读快照；无 active shop 时返回空快照 |
| `PurchaseShopOffer(OfferId)` | 购买当前商店中未购买的 Offer；成功后扣金币、获得卡牌、标记商品已购买，不立刻扣节点 |
| `PurchaseCardFromShop(Card, Price)` | 低层兼容购买事务：只负责扣金币并获得卡牌，不处理库存和节点消耗 |

商店第一版规则：
- 购买卡牌直接进入背包，不自动加入备战区。
- 场景商店入口使用 `AWacomShopTriggerActor.PersistentId` 作为 `ShopId`。
- 商店库存按 `ShopId` 在当前 Run 内存态保留；本轮不接 SaveGame。
- 商品列表由调用方传入；本轮不硬编码商品资产路径。
- 进入商店但不购买不消耗节点；买过任意商品后，关闭商店时统一消耗 1 节点。
- `ShopId == NAME_None`、未知 Offer、重复购买、金币不足等失败路径不修改 RunState。
- UI 入口为 `UWacomShopScreen`：打开时只读取快照，购买时提交 `PurchaseShopOffer`，关闭时调用 `EndShopVisit`。

#### 战斗联动 / 场景持久化 / 存档

| 方法 | 职责 |
|---|---|
| `BuildInitParamsForBattle(EnemyDef, OutParams)` | 构造一场战斗所需的 FBattleInitParams（向后兼容签名，TriggerPersistentId=NAME_None）|
| `BuildInitParamsForBattle(EnemyDef, TriggerPersistentId, OutParams)` | 同上，传入 Trigger 持久化 ID 让 Run 层灌入 PreDestroyedPartIds（GDD §10.5 撤离重入）|
| `OnBattleFinished(const FBattleResultPacket&, EnemyDef)` | 战斗结束通知（向后兼容签名）|
| `OnBattleFinishedFromTrigger(Packet, EnemyDef, TriggerPersistentId)` | 同上，传入 TriggerPersistentId 让 Run 层撤离时写 BattleProgress、真胜利时清理（Stage 7）|
| `MarkTriggerDestroyed(PersistentId)` | 标记一个触发器已被永久销毁 |
| `IsTriggerDestroyed(PersistentId) const` | 查询触发器是否已被销毁 |
| `SetPlayerTransform(InTransform)` | 记录玩家当前 Transform |
| `SaveToSlot(SlotName) const` / `LoadFromSlot(SlotName)` / `HasSaveInSlot(SlotName) const` | 存档接口（Stage 0.1 暂停）|
| `BuildSaveGameFromRunState() const` / `ApplySaveGameToRunState(SaveGame*)` | 存档字段拷贝（公开以便测试）|

`OnBattleFinishedFromTrigger(Packet, EnemyDef, TriggerPersistentId)` 行为（GDD §9.2 / §3.3 / §6 / §10.5）：
- Outcome=Victory + bWithdrawn=true（撤离）：敌人**不**进 DefeatedEnemies；写 RunState.BattleProgress[TriggerId]
- Outcome=Victory + bWithdrawn=false（真胜利）：敌人进 DefeatedEnemies；清理 BattleProgress[TriggerId]
- Outcome=Defeat：终止 Run
- Outcome=Undetermined：跳过结算
- 任一非 Undetermined 结果加 +1% 疲劳
- `bCrossedHighHpThreshold` → +1% 伤口
- `bCrossedLowHpThreshold` → +5% 伤口
- `bMutualDestruction` → +10% 伤口（不影响 bRunActive）
- `KnockdownExpGains[]`：Victory（含同归于尽 / 撤离）累加经验；Defeat 不结算
- `GainedCards[]`：Victory（含撤离）通过 `AcquireCardToRun()` 归入 Run；Defeat / Undetermined 不结算。第一版来源是击倒事件 Aid / Destroy 的部位奖励卡
- `KnockdownChoices[]`：第一阶段仅日志，Stage 9 节点事件接入时按 Choice 触发分支

---

## §3 FRunState

`FRunState` 是内存数据层（USTRUCT），不直接序列化到磁盘。

Stage 1.1 起本结构覆盖 GDD §3 / §8 / §11 描述的全部战外字段。

### 字段清单

#### §3.1 / §3.4：本体 HP（手指）

| 字段 | 类型 | 说明 |
|---|---|---|
| `FingerCount` | `int32` | 战外手指数量。Initialize 时从 Character 读取 |
| `HpPerFinger` | `int32` | 每指对应 HP（默认 2）|

战内 MaxHp = `FingerCount * HpPerFinger` + Companion 卡的 `MaxHpBonus` 累加（在 `BattleSession::Initialize` 中计算）。

#### §3.2：压力（数值化常量 / 八种状态）

| 字段 | 类型 | 说明 |
|---|---|---|
| `Pressure` | `FPressureValues` | 八种压力值容器 |
| `HighHpThreshold` | `float` | 战内伤口阈值 1（默认 0.5）|
| `LowHpThreshold` | `float` | 战内伤口阈值 2（默认 0.2）|

`FPressureValues` 字段直接拆 8 个 int32（不是 array / map）。提供 `Get / Set / Add / GetTotal` 接口。

#### §3.2：辅助计数

| 字段 | 类型 | 说明 |
|---|---|---|
| `TheftCount` | `int32` | 累计偷窃次数。`OnTheftCommitted` 用于劣迹增量公式 |

#### §3.3：经验值与技能

| 字段                   | 类型                     | 说明                                      |
| -------------------- | ---------------------- | --------------------------------------- |
| `ExperienceCurrent`  | `int32`                | 累计经验                                    |
| `ExperienceCapacity` | `int32`                | 经验值上限（默认 10）                            |
| `AcquiredSkills`     | `TArray<FGameplayTag>` | 已获得技能。第一阶段全用 `SkillSlot.Placeholder` 占位 |

#### §8：时间与昼夜

| 字段 | 类型 | 说明 |
|---|---|---|
| `CurrentDayNumber` | `int32` | 当前天数（从 1 开始）|
| `CurrentTimePhase` | `ETimePhase` | 当前时段 |
| `RemainingNodeCount` | `int32` | 当前时段剩余节点数 |
| `InitialNodeCount_Morning/Day/Dusk/Night/Sunrise` | `int32` | 五时段初始节点数（数值常量化）|

`ETimePhase` 枚举：`Morning / Day / Dusk / Night / Sunrise`。

#### §11：背包与备战卡组

| 字段 | 类型 | 说明 |
|---|---|---|
| `Backpack` | `TArray<FCardInstance>` | 通量/背包区，持有独立卡牌 instance |
| `BattleDeck` | `TArray<FCardInstance>` | 备战区，战斗入场的基础卡组 |
| `BurdenZone` | `TArray<FCardInstance>` | 负重区，容量不足时的溢出卡牌 |
| `SpecialZones` | `TArray<FSpecialZone>` | 每张 B 主卡 instance 对应一个特殊存放区 |

**容量公式**（GDD §11.4）由 `URunSession::GetFluxCapacity()` / `GetBattleDeckCapacity()` 动态计算：
- 通量内容容量 = `Σ(玩家拥有的所有 A 类容器卡 max(Capacity - 1, 0))`
- 备战容量 = `Σ(Backpack + BattleDeck 里所有容器卡 Capacity)`，A 类与 B 类都计入
- B 类容器卡（`Physique.CapacityEffect` 非空）不计入通量公式，但计入备战容量；每张 B 主卡自己开辟一个特殊存放区，内容区容量 = `Capacity - 1`
- A 类主卡只占通量主卡区，不额外占用通量内容容量
- B 类特殊存放区内容区实际可收纳数量 = `B.Capacity - 1`

**instance 互斥**：同一个 `FCardInstance.InstanceId` 同时只能位于 `Backpack / BattleDeck / BurdenZone / 任一 SpecialZone.Cards` 之一。`MoveInstance` 是通用迁移入口，失败路径不修改 RunState、不广播。

**主卡投影**：主卡进入 `BattleDeck` 时，物理 instance 位于备战区；背包区对应的主卡槽仍应显示该主卡投影，并标记"已出战"。投影只用于表现和交互提示，不新增 instance，不参与存档和规则归属。

**B 类容器卡与 SpecialZone**：
- B 主卡 instance 进入 `Backpack` 或 `BattleDeck` 时，`RunState.SpecialZones` 中会幂等存在一条 `OwnerInstanceId == 主卡 InstanceId` 的 entry。
- SpecialZone 容量 = 主卡 `Physique.Capacity - 1`，最小为 0。
- SpecialZone 内卡牌默认不入战；`bBattleEnabledInSpecialZone == true` 时，且主卡位于 `BattleDeck`，该卡会被 `BuildInitParamsForBattle` 输出到 `BattleDeckEntries`，并携带主卡 `Physique.CapacityEffect`。
- B 主卡从 `Backpack` 移到 `BattleDeck` 或移回时，SpecialZone 内容保持；销毁 B 主卡时，内含卡退回 `Backpack`，装不下则进入 `BurdenZone`，并清除入战标记。

**Initialize 行为**（Stage 4.1 a2 规则）：
- 容器卡（Capacity > 0）→ 进 Backpack
- 非容器卡（Capacity = 0）→ 进 BattleDeck
- 玩家可用 AddToBattleDeck / RemoveFromBattleDeck 调整

#### §11.7 / 经济：金币

| 字段 | 类型 | 说明 |
|---|---|---|
| `Gold` | `int32` | 玩家金币（GDD §11.7）。Run 内资源，存档第一阶段不持久化。 |

#### 商店访问状态

| 字段 | 类型 | 说明 |
|---|---|---|
| `ActiveShopId` | `FName` | 当前正在访问的商店节点 ID；`NAME_None` 表示没有打开商店 |
| `bShopVisitHasPurchase` | `bool` | 当前商店访问内是否买过至少一件商品；关闭商店时据此消耗 1 节点 |
| `ShopStates` | `TMap<FName, FRunShopState>` | 商店节点库存状态。Key 为商店/节点 `PersistentId`；当前只在 Run 内存态保留 |

#### 既有字段（R5 / S1 骨架）

| 字段 | 类型 | 说明 |
|---|---|---|
| `Character` | `TObjectPtr<UCharacterDefinition>` | 玩家选择的角色（第一阶段固定为 BugGirl）|
| `BattleSeed` | `int32` | 战斗随机种子（0 = 每场独立随机）|
| `DefeatedEnemies` | `TArray<TObjectPtr<UEnemyDefinition>>` | 已击败的敌人列表 |
| `bRunActive` | `bool` | 当前 Run 是否仍在进行（战内 Defeat 置 false）|
| `DestroyedTriggerIds` | `TSet<FName>` | 已被永久销毁的场景触发器 ID 列表 |
| `PlayerTransform` | `FTransform` | 玩家在探索地图的位置/朝向 |
| `bHasPlayerTransform` | `bool` | PlayerTransform 是否有效 |

### 后续扩展（未实现）

- 地图运行时状态（GDD §10 §10.7 引入 `MapNodeStates: TMap<FName, FMapNodeState>`）
- 战内 → 战外回传契约（Stage 1.2 引入 `FBattleResultPacket`）

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
| `Backpack` | `TArray<FCardInstanceSaveEntry>` | v2 起保存 Backpack instances |
| `BattleDeck` | `TArray<FCardInstanceSaveEntry>` | v2 起保存 BattleDeck instances |
| `BurdenZone` | `TArray<FCardInstanceSaveEntry>` | v2 起保存 BurdenZone instances |
| `SpecialZones` | `TArray<FSpecialZoneSaveEntry>` | v2 起保存 owner 与 SpecialZone 内卡 |
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
    case 1:
        // v1 → v2：新增 Backpack/BattleDeck/BurdenZone/SpecialZones instance 列表。
        // 新数组保持空，ApplySaveGameToRunState 会按 StarterDeck 重建 instance。
        SaveGame->SaveVersion = 2;
        [[fallthrough]];
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
