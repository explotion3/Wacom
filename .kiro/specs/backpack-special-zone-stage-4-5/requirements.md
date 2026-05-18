# Requirements Document — Stage 4.5 背包 B 类容器卡特殊存放区

## Introduction

本 spec 实现 Game_Design.md §11 中"B 类容器卡的特殊存放区"完整功能，把 Stage 4.3 起就位的"B 类容器卡数据契约骨架"扩展为可玩闭环：

- 每张玩家拥有的 B 类容器卡开辟独立的特殊存放区（容量 = `Capacity - 1`）
- 特殊存放区的卡可以参战，按 B 主卡的 `CapacityEffect` 修饰（首个具体效果：蛛茧绒囊给带武器关键词的卡 +3 攻击伤害）
- 负重区从"压力计算"升级为"独立数据数组"，超出其他区的卡溢出到此
- 现有 `TArray<TObjectPtr<UCardDefinition>>` 重构为 Instance ID 模型，使同款卡多张可被独立放入不同 zone
- BackpackScreen UI 全量重构为拖拽交互，渲染所有 zone（通量 / 备战 / N 个 SpecialZone / 负重 / 删牌）

本 stage 切成五个子切片（4.5.0 → 4.5.3b），每个子切片独立编译 + 测试通过。本文是整个 Stage 4.5 的需求合集，不是单切片需求。

## Glossary

- **CardInstance（FCardInstance）**：单张卡的运行时实例。结构 = `{ FGuid InstanceId; TObjectPtr<UCardDefinition> Definition; bool bBattleEnabledInSpecialZone; }`。本 stage 引入，替代 `TObjectPtr<UCardDefinition>` 直接作为存放槽元素。
- **InstanceId**：每张 CardInstance 的全局唯一 GUID。生成时机：进入背包系统时一次。同款 Definition 的多张卡 InstanceId 互不相同。
- **Backpack（背包）**：玩家拥有但未参战的卡牌池。GDD §11.1。
- **BattleDeck（备战卡组 / 备战区）**：从玩家拥有卡牌中选出的实际入战集合。GDD §11.6。
- **SpecialZone（特殊存放区）**：每张玩家拥有的 B 类容器卡独自开辟的存放区，容量 = `B 主卡 Capacity - 1`。GDD §11.3。
- **BurdenZone（负重区）**：固定存在的溢出存放区。其他区都装不下时卡进入此处；本 stage 起从 RunState 字段 `TArray<FCardInstance> BurdenZone` 持有，不再只是负重压力的中间计算量。
- **B 主卡（B Container Card / B 类容器卡）**：`Capacity > 0` 且 `CapacityEffect` 是有效 GameplayTag 的卡。每张 B 主卡在 RunState 中各对应一个 `FSpecialZone`。GDD §11.2。
- **A 主卡 / A 类容器卡**：`Capacity > 0` 且 `CapacityEffect` 为空的卡，只贡献通量公式。GDD §11.2。
- **FluxZone（通量存放区）**：背包内 A 类容量公式构成的"主存放区"。即当前 `RunState.Backpack` 中"非容器卡 + A 类容器卡"。GDD §11.4。
- **CapacityEffect**：B 主卡通过 `FCardPhysique.CapacityEffect` 字段挂的 GameplayTag，表示其特殊存放区会给放进去的卡施加的效果。命名空间 `Card.CapacityEffect.*`。本 stage 引入第一个具体 tag `Card.CapacityEffect.WeaponDamagePlus3`（替换占位 `Placeholder`）。
- **bBattleEnabledInSpecialZone**：FCardInstance 字段。仅当卡位于某 SpecialZone 时有意义；true 表示这张卡随对应 B 主卡入战参战，false 表示仅"被特殊收纳"不参战。
- **DropTarget**：UMG 拖拽目标 Widget，对应一个 zone（BackpackZone / BattleDeckZone / SpecialZone(BInstanceId) / BurdenZone / DeleteZone）。
- **DragOperation（UWacomCardDragOperation）**：UMG `UDragDropOperation` 子类，Payload = 起始 zone + 卡的 InstanceId。
- **EZoneKind**：枚举区分 zone 种类，至少包含 Backpack / BattleDeck / SpecialZone / BurdenZone。配合 OwnerInstanceId 共同定位某张卡所在区。
- **互斥四选一**：每个 InstanceId 同时**只能**在 Backpack / BattleDeck / SpecialZone(某 BInstanceId) / BurdenZone 之一中。

## Requirements

### Requirement 1：Instance ID 重构（4.5.0）

**User Story:** As a 程序，I want 把所有 zone 的元素从 `UCardDefinition*` 升级为 `FCardInstance`，so that 同款卡多张可被独立放置、查询、迁移，且为后续 SpecialZone 跟踪每张卡所属的特殊区提供基础。

#### Acceptance Criteria

1. THE WacomRun 模块 SHALL 定义 `FCardInstance`（USTRUCT，BlueprintType），含字段 `FGuid InstanceId`、`TObjectPtr<UCardDefinition> Definition`、`bool bBattleEnabledInSpecialZone`，三个字段均带 UPROPERTY 标记且蓝图可读；`bBattleEnabledInSpecialZone` 默认值为 false，`InstanceId` 默认 `FGuid()`，`Definition` 默认 nullptr。
2. THE FRunState SHALL 把 `Backpack` 与 `BattleDeck` 字段类型从 `TArray<TObjectPtr<UCardDefinition>>` 改为 `TArray<FCardInstance>`，构造态两数组 `Num() == 0`。
3. WHEN 通过 `URunSession::Initialize(InCharacter)` 把 `Character->StarterDeck` 灌入 Backpack / BattleDeck 时，THE URunSession SHALL 为每张非空 Definition 新建 `FCardInstance`，调用 `FGuid::NewGuid()` 生成 InstanceId，且生成的所有 InstanceId 在该次 Initialize 内互不相同；StarterDeck 中的 nullptr 条目跳过不创建 instance。
4. IF `URunSession::Initialize` 接收到 `InCharacter == nullptr` 或 `InCharacter->StarterDeck.Num() == 0`，THEN THE URunSession SHALL 把 Backpack 与 BattleDeck 设为空数组、不创建任何 instance、并通过返回值或日志可观察的方式表明此 fallback。
5. WHEN 通过 `URunSession::AddCardToBackpack(Card)` 加入一张卡时，THE URunSession SHALL 在 `Card != nullptr` 时新建 `FCardInstance` 包装它、分配新 InstanceId、并把该 instance 追加到 `RunState.Backpack` 末尾；`Card == nullptr` 时不修改 Backpack 并通过返回值或日志表明被拒绝。
6. THE URunSession SHALL 提供 `bool MoveInstance(FGuid InstanceId, EZoneKind ToZone, FGuid ToZoneOwnerInstanceId)`，用于在四种 zone 之间迁移单张 instance；迁移成功返回 true 且不改变 InstanceId，迁移失败（见 R1.7）返回 false 且 RunState 任何字段不被修改。
7. IF `MoveInstance` 接收到的 `InstanceId` 在所有四区中均不存在、或目标 zone 为 SpecialZone 但 `ToZoneOwnerInstanceId` 在 `RunState.SpecialZones` 中找不到对应 owner（即非法目标），THEN THE URunSession SHALL 返回 false 且不修改 RunState。
8. THE URunSession SHALL 提供 `bool FindInstance(FGuid InstanceId, FCardInstance& OutInstance, EZoneKind& OutZone, FGuid& OutZoneOwnerInstanceId) const`，给定 InstanceId 即可定位其当前所属 zone；命中时返回 true 并写入三个 out 参数，未命中时返回 false 且三个 out 参数保持调用方传入的初值不被覆写。
9. WHEN 调用既有 API `IsCardInBackpack(Card)` / `IsCardInBattleDeck(Card)`（按 Definition 查询）时，THE URunSession SHALL 在 `Card != nullptr` 时遍历对应 zone instances 并按 Definition 指针匹配，任意一张 instance 的 Definition 等于 Card 即返回 true；`Card == nullptr` 时返回 false。
10. WHEN 调用既有 API `AddCardToBattleDeck(Card)` / `RemoveCardFromBattleDeck(Card)` / `DestroyCardFromBackpack(Card)`（按 Definition 操作）时，THE URunSession SHALL 在源 zone 的 instance 集合中按下标升序选取**第一个** Definition 等于 Card 的 instance 执行操作；找不到匹配 instance 时不修改 RunState 并通过返回值或日志表明被拒绝；以保持现有 BackpackSpec 30+ 单测语义不变。
11. THE WacomSaveGame SHALL 把 `CurrentSaveVersion` 从 1 升到 2，新增字段持久化 `Backpack` / `BattleDeck` / `BurdenZone` 的 instance 列表（每条 = `FGuid` + `FSoftObjectPath` + `bool`）以及每张 B 主卡对应的 SpecialZone 内容；详细字段定义与迁移路径见 Requirement 7。
12. WHEN `UWacomSaveGame::MigrateIfNeeded` 处理 SaveVersion = 0 或 1 的旧档时（旧档没有 Backpack/BattleDeck 字段或字段为空），THE MigrateIfNeeded SHALL 把 SaveVersion 升到 2 后让 `URunSession::ApplySaveGameToRunState` 走"按 StarterDeck 重新生成 instances"路径，确保读档后每张卡都有合法 InstanceId（与 R7.3 / R7.4 一致）。
13. THE Stage 4.5.0 完成态 SHALL 保证除"Initialize / AddCardToBackpack 等会重新生成 InstanceId 的路径"外，所有 BackpackSpec 单测仍 105 条全过（按 Definition 的语义在 instance 模型下应等价）。
14. IF 任意 instance 的 InstanceId 为 `FGuid()`（无效），THEN THE URunSession SHALL 在加入任何 zone 前拒绝该 instance 并触发 `ensureMsgf` 断言（仅 Editor / Debug build 触发，Shipping 不崩）。

### Requirement 2：SpecialZone 数据层（4.5.1）

**User Story:** As a 程序，I want 让每张玩家拥有的 B 类容器卡在 RunState 中持有一个独立的存放区，so that 玩家可以把其他卡放进去，且容量受 B 主卡 `Capacity - 1` 约束。

#### Acceptance Criteria

1. THE WacomRun 模块 SHALL 定义 `FSpecialZone`（USTRUCT，BlueprintType），含字段 `FGuid OwnerInstanceId`（B 主卡 InstanceId）、`TArray<FCardInstance> Cards`，两字段均带 UPROPERTY 蓝图可读；`OwnerInstanceId` 默认 `FGuid()`，`Cards` 默认空数组。
2. THE FRunState SHALL 含字段 `TArray<FSpecialZone> SpecialZones`，每张玩家拥有的 B 主卡 instance 在该数组中刚好对应一个 `OwnerInstanceId == 该 instance.InstanceId` 的条目；非 B 主卡的 instance 不在该数组中持有 entry。**B 主卡 instance 在生命周期内 SHALL 只能位于 `Backpack` 或 `BattleDeck`，永远不进入 `BurdenZone` 或任何 `SpecialZone.Cards`**（见 R2.2a 与 R5.6）。
2a. THE URunSession SHALL 把 "B 主卡 instance 限定在 Backpack ∪ BattleDeck" 视为全局不变量：a) `MoveInstance` 若 `InstanceId` 对应的 instance 是 B 主卡且 `ToZone ∈ {BurdenZone, SpecialZone}` → 拒绝（R2.7 扩展）；b) `RecomputeBurden` 步骤 ① 选择超容溢出 instance 时 SHALL 跳过 B 主卡（R2.12 扩展）；c) `DestroyCardFromBackpack` 退回 SpecialZone 内含卡时（R2.4）SHALL 不会把 B 主卡放入 BurdenZone — B 主卡内含卡按定义不存在（B 主卡不能放进任何 SpecialZone）；d) 任何其它代码路径（refill、SaveGame 还原、外部直接构造）若违反该不变量，对应路径 SHALL 拒绝并保持 RunState 不变。
3. WHEN 一张被判定为 B 主卡（`URunSession::IsTypeBContainerCard(Definition) == true`）的 instance 通过 `AddCardToBackpack` 或 `Initialize` 进入 Backpack / BattleDeck 时，IF `RunState.SpecialZones` 中尚无 `OwnerInstanceId == 该 instance.InstanceId` 的条目，THEN THE URunSession SHALL 在 `RunState.SpecialZones` 末尾追加 `FSpecialZone{ OwnerInstanceId = 该 InstanceId, Cards = {} }`；幂等：若已存在同 OwnerInstanceId 的 entry，不重复追加。
4. WHEN 一张 B 主卡 instance 通过 `DestroyCardFromBackpack` 永久销毁（且未被 §11.8 规则拒绝）时，THE URunSession SHALL 按 `FSpecialZone.Cards` 数组下标升序逐张处理：若 Backpack 当前占用 < `GetFluxCapacity()` 则把该 instance 追加到 Backpack 末尾，否则追加到 BurdenZone 末尾；处理完所有 instance 后从 `RunState.SpecialZones` 移除该 OwnerInstanceId 对应的 entry。
5. THE URunSession SHALL 提供 `int32 GetSpecialZoneCapacityFor(FGuid OwnerInstanceId) const`，返回 `FMath::Max(0, OwnerDefinition->Physique.Capacity - 1)`（与现有静态 `GetSpecialZoneCapacity(BCard)` 一致）；当 OwnerInstanceId 在 `RunState.SpecialZones` 中不存在时返回 0；当 OwnerDefinition->Physique.Capacity ≤ 1 时返回值钳到 0。
6. THE URunSession SHALL 提供 `bool GetSpecialZone(FGuid OwnerInstanceId, FSpecialZone& Out) const`，命中时写入 Out 并返回 true，未命中时不修改 Out 并返回 false。
7. WHEN 调用 `MoveInstance(InstanceId, EZoneKind::SpecialZone, OwnerInstanceId)` 把一张卡放入某 SpecialZone 时，IF 满足以下任一条件 a) `OwnerInstanceId` 不存在于 `RunState.SpecialZones`；b) `InstanceId == OwnerInstanceId`（B 主卡不能放进自己的 SpecialZone）；c) 该 SpecialZone 当前 `Cards.Num() >= GetSpecialZoneCapacityFor(OwnerInstanceId)`；d) `InstanceId` 在所有 zone 中均不存在；e) `InstanceId` 对应的 instance 是 B 主卡（`IsTypeBContainerCard(Inst.Definition) == true`，对应 R2.2a / R5.6 不变量：B 主卡不能进入任何 SpecialZone.Cards），THEN THE URunSession SHALL 返回 false、不修改 RunState 任何字段、且不广播 `OnRunStateChangedNative`。
7a. WHEN 调用 `MoveInstance(InstanceId, EZoneKind::BurdenZone, _)` 把一张卡放入 BurdenZone 时，IF `InstanceId` 对应的 instance 是 B 主卡（`IsTypeBContainerCard(Inst.Definition) == true`），THEN THE URunSession SHALL 返回 false、不修改 RunState 任何字段、且不广播 `OnRunStateChangedNative`（对应 R2.2a / R5.6：B 主卡不能进入 BurdenZone）。
8. WHEN 一张卡进入某 SpecialZone（R7 校验通过后）时，THE URunSession SHALL 在同一逻辑步骤内：先从其当前所在 zone（Backpack / BattleDeck / 另一 SpecialZone / BurdenZone）移除该 instance、再追加到目标 SpecialZone.Cards 末尾、再广播一次 `OnRunStateChangedNative`；此过程不允许在中间态广播 `OnRunStateChangedNative`。
9. WHILE 一张 instance 位于某 SpecialZone，THE URunSession SHALL 保证该 instance 的 `bBattleEnabledInSpecialZone` 字段在首次进入该 SpecialZone 时被设为 false（除非调用方通过 R10 显式切换）。
10. THE URunSession SHALL 提供 `bool SetSpecialZoneCardBattleEnabled(FGuid InstanceId, bool bEnabled)` 切换该 flag；若 instance 当前位于某 SpecialZone 则把其 `bBattleEnabledInSpecialZone` 设为 bEnabled、广播 `OnRunStateChangedNative` 并返回 true；否则不修改、不广播、返回 false。
11. THE FRunState SHALL 含字段 `TArray<FCardInstance> BurdenZone`，构造态 `Num() == 0`，且字段带 UPROPERTY 标记。
12. WHEN 容量公式（FluxCapacity / BattleDeckCapacity）变化导致已存在的卡超出对应区时，THE URunSession SHALL 从对应区下标最大端（数组末尾）向下标 0 方向**搜索第一个非 B 主卡 instance**摘出，追加到 `RunState.BurdenZone` 末尾，逐张处理直到对应区占用 ≤ 容量或对应区不再含任何非 B 主卡 instance。具体行为：a) 若数组末尾是非 B 主卡 → 直接 Pop 追加 BurdenZone（最常见路径）；b) 若数组末尾是 B 主卡 → 从该位置向下标 0 方向搜索第一个非 B 主卡 instance 移除并追加 BurdenZone（保持 B 主卡末位顺序不被破坏并不重要，重要的是 B 主卡不进 BurdenZone）；c) 若整个数组都是 B 主卡 instance（无任何非 B 主卡可摘）→ 该 zone 的溢出循环立即终止，**Backpack / BattleDeck 可能临时超容（数组 Num > Capacity）**，但绝不会有 B 主卡进入 BurdenZone（对应 R2.2a / R5.6 不变量）。该退化情形目前仅由"用户拥有数量超出容量公式的 B 主卡"触发，属于异常 RunState；UI 层在 Stage 4.5.3b 不主动暴露此入口。
13. THE URunSession SHALL 重写 `RecomputeBurden` 为：① 按 R12 把超容卡移到 BurdenZone；② 按 R14 回填；③ 按 `BurdenZone.Num()` 应用 GDD §3.2 公式 `n*(n+1)/2` 调用 `SetPressure(EPressureChannel::Burden, ...)` 设置压力；详细公式约束见 Requirement 9。
14. WHEN 调用 `RecomputeBurden` 后，IF BurdenZone 非空且通量 / 备战 / 任意 SpecialZone 有空余，THEN THE URunSession SHALL 按"通量 → 备战 → SpecialZones（按 SpecialZones 数组下标升序）"的优先顺序，把 BurdenZone 头部 instance 依次回填到第一个有空的目标区，回填到一张 instance 不再有目标可去为止；回填多次调用幂等（第二次调用不应再迁移任何 instance）。
15. THE URunSession SHALL 增加 4.5.1 单测覆盖：a) B 主卡加入 → 自动建空 SpecialZone；b) 放入合法卡 → SpecialZone.Cards 增加 + InstanceId 仅出现在该 SpecialZone；c) B 主卡自指放入 → 返回 false 且 RunState 不变；d) 容量满放入 → 返回 false 且 RunState 不变；e) B 主卡永久销毁（Backpack 仍有空时）→ 内含卡退回 Backpack 末尾；f) Backpack 满时 B 主卡销毁 → 内含卡进 BurdenZone；g) BurdenZone 非空 + 通量有空 → RecomputeBurden 按 R14 优先序回填、第二次调用幂等；h) 容量回退（如销毁 A 主卡）导致溢出 → 从对应区末尾摘卡进 BurdenZone。每条断言点为：操作返回值 + 操作后的 `RunState.Backpack/BattleDeck/SpecialZones/BurdenZone` 各数组的 InstanceId 集合。
16. THE URunSession SHALL 在所有改动 zone 内容的 public 入口（Add / Remove / Destroy / Move / SetSpecialZoneCardBattleEnabled / RecomputeBurden）成功路径尾部广播一次 `OnRunStateChangedNative`，被拒绝路径不广播。

### Requirement 3：备战卡组容量回归 GDD §11.4（4.5.1 同步）

**User Story:** As a 程序，I want `BattleDeckCapacity` 严格按 GDD §11.4 实现，so that 容器分类纠正后备战区容量与设计一致。

#### Acceptance Criteria

1. THE URunSession::GetBattleDeckCapacity SHALL 在玩家拥有零张容器卡时返回 0；含至少一张 A 类容器卡时返回 `Σ(玩家拥有的所有 A 类容器卡的 Capacity)`，且返回值类型与 `URunSession::GetFluxCapacity` 同为 int32。
2. WHEN 玩家拥有的所有容器卡均为 B 类（无任一 A 类），THE URunSession::GetBattleDeckCapacity SHALL 返回 0。
3. WHEN 玩家同时拥有 A 类和 B 类容器卡，THE URunSession::GetBattleDeckCapacity 返回值 SHALL 等于 A 类 Capacity 之和（与 GetFluxCapacity 相等），新增 / 移除 B 类容器卡前后该返回值差为 0。
4. WHEN 容器卡集合发生任何变化时，THE URunSession::GetBattleDeckCapacity 与 `GetFluxCapacity` SHALL 始终保持相等（防止公式漂移到 Flux 侧导致 R3 反向破坏）。
5. THE URunSession::CollectTypeBContainers SHALL 改为输出 `TArray<FGuid> OutOwnerInstanceIds`（B 主卡 InstanceId 集合），按 `RunState.SpecialZones` 数组下标升序排序，且不含重复 InstanceId、不含悬空 InstanceId（即每个返回的 InstanceId 都能在 `Backpack ∪ BattleDeck` 中找到对应 instance）。
6. WHEN 玩家未拥有任何 B 主卡，THE URunSession::CollectTypeBContainers SHALL 输出空数组（`OutOwnerInstanceIds.Num() == 0`）。
7. THE Stage 4.5.1 完成态 SHALL 同时保证 BackpackSpec 现有用例（含 R3 修正后受影响的"B 类容器卡进备战区是否提升备战区容量"用例）与本 stage 新增 4.5.1 单测两个集合都全部通过；任一集合存在 failing 用例都不算完成。

### Requirement 4：CapacityEffect 战斗集成 — 蛛茧绒囊（4.5.2）

**User Story:** As a 玩家，I want 蛛茧绒囊里参战的带武器关键词的卡，攻击伤害 +3，so that 能玩到首个具体的容量效果，让 B 类容器卡的差异性可被感知。

#### Acceptance Criteria

1. THE WacomCore SHALL 用 `UE_DECLARE_GAMEPLAY_TAG_EXTERN` + `UE_DEFINE_GAMEPLAY_TAG` 注册 tag `Card.CapacityEffect.WeaponDamagePlus3` 与 `Card.Keyword.Weapon`（如尚未存在），两 tag 在 Editor 启动后均通过 `FGameplayTag::IsValid()` 返回 true。
2. THE 蛛茧绒囊 DataAsset（`BugGirlBuilder.cpp`）SHALL 把 `CapacityEffect` 从 `Card.CapacityEffect.Placeholder` 改为 `Card.CapacityEffect.WeaponDamagePlus3`。
3. THE URunSession::BuildInitParamsForBattle SHALL 输出"备战区原生 instances + 各 SpecialZone 中 `bBattleEnabledInSpecialZone == true` 的 instances"合并列表，并对每张 instance 携带其"来源 CapacityEffect tag 集合"：来自 SpecialZone 的卡 tag 集合 = 主卡 CapacityEffect 单元素集合；来自备战区原生位置的卡 tag 集合 = 空集合。BattleSession Initialize 时按 instance 直接读取该集合，无需回访 RunSession。
4. WHEN BattleSession 计算一张卡的最终伤害时，IF 该卡携带的 CapacityEffect tag 集合含 `Card.CapacityEffect.WeaponDamagePlus3` 且其 Definition 含关键词 `Card.Keyword.Weapon`，THEN BattleSession SHALL 在该卡的 `Effect.Damage` 数值基础上 +3；同一卡同时被 N 个该 tag 来源覆盖时叠加 +3×N（语义清晰，但本 stage 设计上单卡至多被一个 SpecialZone 持有，N 通常 = 1）。
5. IF 一张卡携带的 CapacityEffect tag 集合含 `Card.CapacityEffect.WeaponDamagePlus3` 但其 Definition **不**含 `Card.Keyword.Weapon`，THEN BattleSession SHALL 不修改该卡的伤害（即 +3 仅作用于带武器关键词的卡）。
6. WHEN 一张卡的 `Effect.Damage` Magnitude 来源是表达式（非常量）时，THE BattleSession SHALL 在 magnitude resolve 之后、其它修正之后再叠加 +3，并在最终输出值上做 `FMath::Max(0, value)` clamp 防负数。
7. WHEN BuildInitParamsForBattle 输出某张来自 SpecialZone 的入战 instance 时，IF 该 instance 对应的 B 主卡当前不在 BattleDeck（即仍在 Backpack），THEN BuildInitParamsForBattle SHALL 不输出该 instance（即默认提案：主卡不在备战区 → 内含卡一律不入战，FBattleInitParams 不携带其 tag），与 R5.3 / R8.3 一致。
8. THE WacomTests SHALL 增加 BattleSpec / BackpackSpec 单测覆盖：a) 蛛茧绒囊 SpecialZone 内 `bBattleEnabledInSpecialZone=false` 的武器卡 → 不入战（不出现在 BattleSession 的 deck 中）；b) `bBattleEnabledInSpecialZone=true` 的武器卡入战伤害最终结算 = `Base + 3`；c) `bBattleEnabledInSpecialZone=true` 但 Definition 不含 `Card.Keyword.Weapon` 的卡入战伤害 = `Base`；d) 蛛茧绒囊不在备战区（仍在背包）时其内含 IsBattleEnabled 卡 → 不入战（按 R7 默认提案）。
9. THE Stage 4.5.2 完成态 SHALL 保证 BattleSpec / BackpackSpec 单测全过；编译命令为项目约定的 `Build.bat WacomEditor Win64 Development`，自动化命令为 `Automation RunTests Wacom`。

### Requirement 5：B 主卡跨 Backpack ↔ BattleDeck 移动时 SpecialZone 跟随（4.5.2 同步）

**User Story:** As a 玩家，I want B 主卡能在 Backpack 和 BattleDeck 之间移动且其 SpecialZone 内容跟随主卡而保留，so that 不会因为切换主卡所在 zone 丢失已收纳的卡。

#### Acceptance Criteria

1. WHEN B 主卡 instance 从 Backpack 移到 BattleDeck（或反向）成功后，THE URunSession SHALL 保证 `RunState.SpecialZones` 中 `OwnerInstanceId == 该 B 主卡 InstanceId` 的 `FSpecialZone.Cards` 数组在迁移前后：a) 长度不变；b) 元素顺序不变；c) 每个 `FCardInstance` 的 InstanceId / Definition / bBattleEnabledInSpecialZone 三字段值都与迁移前对应位置完全相等。
2. WHILE 某 B 主卡的 InstanceId 出现在 `RunState.BattleDeck` 任一 instance 的 InstanceId 字段中且其 SpecialZone 中存在 `bBattleEnabledInSpecialZone == true` 的 instance，WHEN `BuildInitParamsForBattle` 被调用，THE URunSession SHALL 把该 SpecialZone 中所有 `bBattleEnabledInSpecialZone == true` 的 instance 包含在返回的入战 deck 列表中（与 R4.3 一致）。
3. WHILE 某 B 主卡的 InstanceId 出现在 `RunState.Backpack` 任一 instance 的 InstanceId 字段中（即不在 BattleDeck），WHEN `BuildInitParamsForBattle` 被调用，THE URunSession SHALL 不把该 SpecialZone 中任何 instance 包含在返回的入战 deck 列表中（即 B 主卡需在备战区其特殊收纳的卡才会入战）。
4. WHEN `URunSession::DestroyCardFromBackpack` 试图销毁一张 B 主卡 instance 但被 §11.8 现有规则拒绝（最后 BagProvider 拒绝 / Intrinsic 拒绝）时，THE URunSession SHALL 保持 `RunState.Backpack` / `RunState.BattleDeck` / `RunState.SpecialZones` / `RunState.BurdenZone` 全部字段不变，并通过返回值或日志表明被拒绝；销毁通过时按 R2.4 处理 SpecialZone 内含卡退回。
5. WHEN `MoveInstance` 试图移动一张 instance 但失败（源 zone 不含该 InstanceId / 目标 zone 已满 / R2.7 拒绝条件），THE URunSession SHALL 保证 `RunState.Backpack` / `RunState.BattleDeck` / `RunState.SpecialZones` / `RunState.BurdenZone` 全部字段在调用前后逐字段相等（无任何"部分修改"中间态被持久化）。
6. THE URunSession SHALL 用 `OwnerInstanceId` 作为 SpecialZone 与 B 主卡 instance 的唯一关联键；当玩家拥有多张同一 CardId 的 B 主卡 instance（不同 InstanceId）时，每张主卡 instance 各自对应一个独立的 `FSpecialZone` entry，跨 Backpack ↔ BattleDeck 移动时各 SpecialZone 独立跟随其对应主卡 instance（不发生互相串台）。**B 主卡 instance 在生命周期内 SHALL 只可能位于 `Backpack` 或 `BattleDeck`，永远不进入 `BurdenZone` 或任何 `SpecialZone.Cards`**——任何代码路径（`MoveInstance` / `RecomputeBurden` 步骤 ① 溢出 / 回填 / SaveGame 还原 / 外部直接构造）若试图把 B 主卡 instance 放入 `BurdenZone` 或某 `SpecialZone.Cards` SHALL 被拒绝（对应 R2.2a / R2.7.e / R2.7a / R2.12 b-c）。该不变量保证 R2.2 双射（每个 `SpecialZones` entry 的 `OwnerInstanceId` 唯一对应 `Backpack ∪ BattleDeck` 中一个 instance）始终可解析、不会出现"主卡遗失到 BurdenZone 但 SpecialZone 仍存活"的悬空态。

### Requirement 6：BackpackScreen 拖拽 UI 重构（4.5.3a + 4.5.3b）

**User Story:** As a 玩家，I want 通过拖拽在背包界面里把卡片在通量 / 备战 / 各 SpecialZone / 负重 / 删牌区之间移动，so that 操作更直观，且能看到所有 zone 的当前状态。

#### Acceptance Criteria

1. THE WacomApp 模块 SHALL 定义 `UWacomCardDragOperation`（继承 `UDragDropOperation`），Payload 含 `FGuid InstanceId`、`EZoneKind FromZone`、`FGuid FromZoneOwnerInstanceId`、`TObjectPtr<UCardDefinition> Definition`；当 `FromZone != EZoneKind::SpecialZone` 时 `FromZoneOwnerInstanceId` 必须为 invalid GUID（`FGuid()`）。
2. THE WacomDeckCardWidget SHALL 重构为支持拖拽源：覆写 `NativeOnMouseButtonDown`（仅响应鼠标左键 + 调用 `DetectDragIfPressed`）+ `NativeOnDragDetected`（构造并返回非空的 `UWacomCardDragOperation`）。
3. THE WacomBackpackScreen SHALL 定义独立的 DropTarget Widget 类（如 `UWacomZoneDropTarget`），覆写 `NativeOnDragOver` / `NativeOnDrop`；每个 zone 容器（BackpackZone / BattleDeckZone / 每个 SpecialZone / BurdenZone / DeleteZone）实例化一个独立 DropTarget；DropTarget 在 `NativeOnDrop` 中仅当传入的 Operation `Cast<UWacomCardDragOperation>` 成功时才调用对应 RunSession 迁移 API，否则忽略并返回 false。
4. WHEN 一次合法 Drop 完成（DropTarget 调用的 RunSession API 返回 true）后，THE WacomBackpackScreen SHALL 通过 `OnRunViewModelRefreshedNative` 回调路径重新渲染所有 zone（清空原 widget 子项 + 按当前 RunState 重建），且不通过手动 patch（`AddChildToWrapBox` / `RemoveChild` 在已存在 widget 上）的方式做局部增删；执行完成后视觉 zone 内容必须等同于在该状态下重新打开 Screen。
5. IF 一次 Drop 完成时 RunSession 迁移 API 返回 false，THEN THE WacomBackpackScreen SHALL 不修改任何 zone 内容（widget 子项数量、顺序、实例引用全保持不变），且原拖拽源卡 widget 保持在原 zone 容器中的原索引位置。
6. **4.5.3a：** THE WacomBackpackScreen SHALL 先实现"BattleDeckZone ↔ BackpackZone（通量区）"两个 DropTarget 之间的拖拽闭环；同时删除原"点击主按钮 Move"语义，卡 widget 主按钮在 4.5.3a 起仅作纯展示，对鼠标点击不绑定 Move 委托。
7. **4.5.3b：** THE WacomBackpackScreen SHALL 在 4.5.3a 基础上为每张玩家拥有的 B 主卡渲染独立的 SpecialZone 区块（一个 WrapBox + 标题"特殊存放区 [主卡名]"+ 容量计数显示为格式 `n/(Capacity-1)` 其中 n = 当前 `FSpecialZone.Cards.Num()`、Capacity 取自主卡 `Physique.Capacity`），并显示其内容卡 widget；在该 SpecialZone 中 `bBattleEnabledInSpecialZone == true` 的卡 widget SHALL 显示一个 widget tree 中可被名称 / 类型查询识别的"已选"角标元素（具体视觉样式由 design / 美术决定，但元素必须存在使单测可校验）。
8. **4.5.3b：** THE WacomBackpackScreen SHALL 渲染独立的 BurdenZone 区块（标题"负重区"+ 计数 `RunState.BurdenZone.Num()`），其 WrapBox 子项与 `RunState.BurdenZone` 数组逐张对应（顺序、Definition、InstanceId 一致）。
9. **4.5.3b：** THE WacomBackpackScreen SHALL 把删牌区做成独立 DropTarget，其 `NativeOnDrop` 在合法 Operation 上调用 `URunSession::DeleteCardForGold(Card)`（按 instance.Definition 取出 Card）；IF `DeleteCardForGold` 返回 false（金币不足 / Intrinsic / 最后 BagProvider 等失败路径），THEN DropTarget SHALL 按 R6.5 处理（zone 内容不变 + 原卡 widget 保持原位）。
10. **4.5.3b：** THE WacomBackpackScreen 的 BattleDeckZone 视觉显示 SHALL 包含其本身 instances + 所有 B 主卡（`OwnerInstanceId` 出现在 `RunState.BattleDeck`）SpecialZone 中 `bBattleEnabledInSpecialZone == true` 的卡，后者带角标"来自 [B 主卡名]"；同时这些卡在原 SpecialZone 区块中 SHALL 同时显示"已选"标记（不消失，与 R6.7 一致）。
11. WHEN 玩家在某 SpecialZone 区块中的一张卡 widget 上点击鼠标右键（单击），THE WacomBackpackScreen SHALL 调用 `URunSession::SetSpecialZoneCardBattleEnabled(Instance.InstanceId, !FCardInstance.bBattleEnabledInSpecialZone)` 切换该 flag；切换成功后该卡 widget 的"已选"角标可见性 SHALL 严格等于 `RunState` 中该 instance 切换后的 `bBattleEnabledInSpecialZone` 值。
12. IF 一次 Drop 的目标 zone 是 BattleDeckZone 且当前 BattleDeck 占用 ≥ `GetBattleDeckCapacity()` 而源 zone 是 BackpackZone（即来自 Backpack 的 Drop），THEN DropTarget SHALL 按 R6.5 拒绝并保留视觉位置（不调用 `MoveInstance` 或 `MoveInstance` 因容量满返回 false 时同样按 R6.5 处理）。
13. WHILE B 主卡 instance 当前位于 BattleDeck，THE WacomBackpackScreen SHALL 在该主卡对应的 SpecialZone 区块标题上显示一个 widget tree 中可被名称 / 类型查询识别的"已入战"标记元素（与 GDD §11.3 末注一致）；主卡回到 Backpack 时该元素 visibility 切换为 collapsed。

### Requirement 7：存档迁移与版本管理（贯穿 4.5.0 → 4.5.3b）

**User Story:** As a 玩家，I want 从老版本（v0 / v1 存档，不存背包数据）切到 Stage 4.5 后玩家仍能继续 Run，so that 不会因结构升级丢档。

#### Acceptance Criteria

1. THE UWacomSaveGame::CurrentSaveVersion SHALL 设为编译期常量值 2，且每次结构升级时单调递增、旧版本号永不复用。
2. THE UWacomSaveGame SHALL 新增 SaveGame 字段持久化 `Backpack` / `BattleDeck` / `BurdenZone` 的 instance 列表，每条 = `{ FGuid InstanceId; FSoftObjectPath DefinitionAssetPath; bool bBattleEnabledInSpecialZone; }`，以及 `TArray<FSpecialZoneSaveEntry>`（每条含 `FGuid OwnerInstanceId` + `TArray<FCardInstanceSaveEntry>`），且写入时三个 instance 列表合并去重后所有 InstanceId 必须全局唯一、非 zero GUID。
3. WHEN MigrateIfNeeded 检测到加载的 SaveVersion ∈ {0, 1}，THE MigrateIfNeeded SHALL 在不修改任何已存在字段原值的前提下，把第 R7.2 条新增的所有 instance 列表与 SpecialZones 列表初始化为空 TArray 并把 SaveVersion 写为 2。
4. WHEN ApplySaveGameToRunState 接收到 SaveVersion = 2 且 Backpack / BattleDeck / BurdenZone / SpecialZones 全部为空容器的存档（含来自 R7.3 迁移路径的存档），THE ApplySaveGameToRunState SHALL 按当前角色 StarterDeck 重新生成 Backpack 与 BattleDeck instances（每条新分配 InstanceId）、把 BurdenZone 置为空、并按 RunState 中的 B 主卡 instance 列表为每个 B 主卡建立一条 SpecialZones entry（OwnerInstanceId = 该 B 主卡 instance、CardInstances 为空）。
5. WHEN ApplySaveGameToRunState 接收到 SaveVersion = 2 且 instance 列表非空的存档，THE ApplySaveGameToRunState SHALL 按存档中的 InstanceId、DefinitionAssetPath、bBattleEnabledInSpecialZone 还原 Backpack / BattleDeck / BurdenZone 三区，并按 SpecialZones entry 还原每个 OwnerInstanceId 与其 CardInstances 的归属关系，使内存中的 RunState 与磁盘上对应字段在 InstanceId / Definition / 归属 / flag 四个维度逐项相等。
6. IF ApplySaveGameToRunState 检测到任一损坏档情况：a) 某条 entry 的 DefinitionAssetPath 无法解析为有效 CardDefinition；b) 某 SpecialZone entry 的 OwnerInstanceId 在还原后的 Backpack/BattleDeck 中找不到对应 owner instance；c) 同一 InstanceId 在合并后的 Backpack ∪ BattleDeck ∪ BurdenZone ∪ ⋃SpecialZones.Cards 列表中出现多次，THEN THE ApplySaveGameToRunState SHALL 拒绝加载该存档、保持调用前 RunState 不变、并向调用方返回加载失败结果。
7. IF MigrateIfNeeded 接收到 SaveVersion > 2 的存档（来自更新版本客户端），THEN THE MigrateIfNeeded SHALL 拒绝迁移、保持存档对象不被改写、并向调用方返回版本不兼容错误。
8. THE WacomSaveGame Migration 单测 SHALL 覆盖以下四条路径：(a) v0 → v2 与 v1 → v2 迁移后第 R7.2 条所述新字段全部为空容器且 SaveVersion 等于 2；(b) v2 → v2 round-trip 在 InstanceId 集合、每个 InstanceId 对应的 DefinitionAssetPath、每个 InstanceId 的 bBattleEnabledInSpecialZone、每个 SpecialZone 的 OwnerInstanceId 及其 CardInstances 集合上完全相等；(c) DefinitionAssetPath 失效 / OwnerInstanceId dangling / InstanceId 重复 三类损坏档分别触发第 R7.6 条所述拒绝加载结果；(d) SaveVersion = 3 触发第 R7.7 条所述版本不兼容错误。

### Requirement 8：BattleDeck 解释口径（决策 Q-D）

**User Story:** As a 玩家，I want 在 SpecialZone 中标记某张卡"参战" = 仅切 flag 不物理移卡，so that 同一张卡同时是"被特殊收纳"和"参战"两种身份兼容。

#### Acceptance Criteria

1. WHEN URunSession 收到"将位于 SpecialZone 中的某 FCardInstance 标记为参战"的操作请求时，THE URunSession SHALL 把该 instance 的 `FCardInstance.bBattleEnabledInSpecialZone` 设为 true，且 SHALL 不修改该 instance 在 SpecialZone 中的物理归属（操作前后该 instance 仍由同一 `FSpecialZone.Cards` 数组在同一下标位置持有，未进入 Backpack / BattleDeck / BurdenZone）。
2. WHILE 某 FCardInstance 同时满足"位于 SpecialZone"且"`bBattleEnabledInSpecialZone == true`"且"该 instance 对应的 B 主卡 instance 当前位于 BattleDeck 中"，WHEN `URunSession::BuildInitParamsForBattle` 被调用，THE URunSession SHALL 把该 SpecialZone 中的 instance 包含在返回的入战 deck 列表中（与 R4.3 / R5.2 一致）。
3. WHILE 某 FCardInstance 同时满足"位于 SpecialZone"且"`bBattleEnabledInSpecialZone == true`"且"该 instance 对应的 B 主卡 instance 当前位于 Backpack 中"，WHEN `URunSession::BuildInitParamsForBattle` 被调用，THE URunSession SHALL 不把该 SpecialZone 中的 instance 包含在返回的入战 deck 列表中（与 R5.3 一致）。
4. WHILE 某 FCardInstance 位于 BurdenZone，WHEN `URunSession::BuildInitParamsForBattle` 被调用，THE URunSession SHALL 不读取该 instance 的 `bBattleEnabledInSpecialZone` 字段，且 SHALL 不把该 instance 包含在返回的入战 deck 列表中（无论 `bBattleEnabledInSpecialZone` 字段值为 true 还是 false）。
5. WHEN URunSession 收到"取消位于 SpecialZone 中的某 FCardInstance 的参战标记"的操作请求时，THE URunSession SHALL 把该 instance 的 `FCardInstance.bBattleEnabledInSpecialZone` 设为 false，且 SHALL 不修改该 instance 在 SpecialZone 中的物理归属。
6. WHEN 某 FCardInstance 因任何原因从 SpecialZone 移出（包括但不限于：被取回 Backpack、移入 BurdenZone、Run 结束清算、所属 B 主卡被销毁退回流），THE URunSession SHALL 在移出生效前或同一操作内把该 instance 的 `FCardInstance.bBattleEnabledInSpecialZone` 重置为 false，使后续若该 instance 再次进入 SpecialZone 时不残留旧的参战标记。

### Requirement 9：负重压力公式保持（贯穿 4.5.1）

**User Story:** As a 玩家，I want 负重压力沿用 GDD §3.2 现有公式，so that 数据层重构不引发玩家可见的压力变化。

#### Acceptance Criteria

1. WHEN `URunSession::RecomputeBurden` 被调用，THE URunSession SHALL 通过 `SetPressure(EPressureChannel::Burden, FMath::Clamp(n*(n+1)/2, 0, 100))` 写入 Burden 通道压力值，其中 `n = RunState.BurdenZone.Num()`；当 n ≥ 14 时（n*(n+1)/2 ≥ 105 > 100）写入值会被 clamp 到 100。
2. IF 调用栈不在 `RecomputeBurden` 路径上（例如 `AddPressure` / `SetPressure` 其他通道、`AdvanceToNextPhase` / `OnBattleFinishedFromTrigger` 等流程内），THEN THE URunSession SHALL 不修改 Burden 通道值（即 Burden 通道的写入唯一入口为 `RecomputeBurden`，反之则为不变量违反）。
3. THE BackpackSpec 单测 SHALL 增加用例：a) BurdenZone 0 / 1 / 3 / 14 张分别触发 `RecomputeBurden` 后，Burden 通道值分别为 0 / 1 / 6 / 100（n=14 触发 clamp）；b) `RecomputeBurden` 连续两次调用幂等（第二次调用后 Burden 通道值与第一次后完全相等，且通过观察不广播额外的 `OnRunStateChangedNative` 或 Burden 通道变更通知验证）。

## 切片实现顺序（参考，非可测需求）

| 切片 | 主要要求 | 验收 |
|---|---|---|
| 4.5.0 Instance ID 重构 | R1 全部 + R7 SaveVersion 升档 | 编译通过；BackpackSpec 105 全过；新增 v0/v1→v2 迁移单测过 |
| 4.5.1 SpecialZone + BurdenZone 数据层 | R2 + R3 + R9 | 4.5.1 新增单测全过 |
| 4.5.2 BattleSession 集成 + 蛛茧绒囊武器+3 | R4 + R5 + R8 | BattleSpec 新增单测过；蛛茧绒囊改 tag 后旧测试不引入回归 |
| 4.5.3a 拖拽框架（备战↔背包） | R6.1 / R6.2 / R6.3 / R6.4 / R6.5 / R6.6 | PIE 手测拖拽正常；现有 105 单测保持全绿（UI 层无单测） |
| 4.5.3b SpecialZone / BurdenZone 渲染 + 删牌拖拽 | R6.7 / R6.8 / R6.9 / R6.10 / R6.11 / R6.12 / R6.13 | PIE 手测全 zone 拖拽正常 |
