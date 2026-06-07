// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Types/WacomEnums.h"
#include "RunState.h"
#include "RunStateTypes.h"
#include "Session/BattleResultPacket.h"
#include "RunSession.generated.h"

class UCharacterDefinition;
class UCardDefinition;
class UWacomRunEventDefinition;
class UWacomSaveGame;
struct FBattleInitParams;

#if WITH_AUTOMATION_TESTS
struct FWacomRunSessionTestAccess;
#endif

/**
 * 一次冒险（Run）的逻辑入口。
 *
 * 职责：
 *   - 持有 FRunState（战斗外的持久状态）
 *   - 初始化 / 重置 Run
 *   - 构造 FBattleInitParams 供 GameMode 打开一场战斗
 *   - 接收战斗结束通知，更新 Run 状态
 *   - 战外失败综合判定（压力满 / 手指掉光 / Defeat）
 *   - 存档 / 读档：FRunState <-> UWacomSaveGame <-> 磁盘
 *
 * AWacomPlayerController 在 BeginPlay 时创建并持有 URunSession。
 */
UCLASS(BlueprintType)
class WACOMRUN_API URunSession : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * RunState 任何字段被修改后广播一次。
	 *
	 * 设计：
	 *   - **原生委托**（不是 Dynamic），UI 用 AddUObject + RemoveAll 注册/反注册
	 *   - **粗粒度**：不区分变更字段，订阅方按需读 RunState 全量
	 *   - **组合事务合并**：RunSession 内部的玩家级组合操作会在事务末尾合并广播一次；
	 *     订阅方仍应保证刷新逻辑幂等
	 *   - **不 Push 数据**：参数为空，订阅方自己读 GetRunState() / Get* 接口
	 *
	 * 替代 Tick 拉数据。订阅生命周期管理见 ue5-ui-umg-slate skill 失效绑定章节。
	 *
	 * 后续如果迁 MVVM，这是 ViewModel 监听的入口。
	 */
	DECLARE_MULTICAST_DELEGATE(FOnRunStateChangedNative);
	FOnRunStateChangedNative OnRunStateChangedNative;

public:
	// ---- 生命周期 ----

	/**
	 * 初始化一次 Run。新开档时调用。
	 * 失败时 bRunActive 仍置 true 但 Character 为空——调用方应检查返回值。
	 *
	 * 行为：
	 *   - FingerCount / HpPerFinger 从 Character 字段读
	 *   - 非容器卡默认进 BattleDeck，容器卡默认进 Backpack；原型特例暮色引虫灯默认进 BattleDeck
	 *   - 时段重置为 Morning + 初始节点数
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run")
	bool Initialize(UCharacterDefinition* InCharacter);

	/** 重置为"新 Run"默认值（保留 Character）。死亡后重开用。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run")
	void ResetRunState();

	// ---- 状态访问 ----

	/** 只读：当前 Run 的状态。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run")
	const FRunState& GetRunState() const { return RunState; }

	/** Transient C++-only UI refresh revisions; not serialized and not exposed to Blueprint. */
	uint64 GetBackpackStorageSnapshotRevision() const { return BackpackStorageSnapshotRevision; }
	uint64 GetShopSnapshotRevision() const { return ShopSnapshotRevision; }
	uint64 GetEconomySnapshotRevision() const { return EconomySnapshotRevision; }

	/** 是否仍在 Run 中（bRunActive == true）。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run")
	bool IsRunActive() const { return RunState.bRunActive; }

	// ---- 手指 ----

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|HP")
	int32 GetFingerCount() const { return RunState.FingerCount; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|HP")
	bool IsFingerDepleted() const { return RunState.FingerCount <= 0; }

	/**
	 * 失去若干手指。
	 *
	 * 副作用：
	 *   - 残疾压力同步增加：每缺 1 指 +5%
	 *   - FingerCount 降到 0 时不主动改 bRunActive；调用 IsRunFailed() 综合判定
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|HP")
	void RemoveFinger(int32 Count = 1);

	// ---- 压力 ----

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Pressure")
	int32 GetPressureValue(EWacomPressureType Type) const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Pressure")
	int32 GetTotalPressure() const;

	/**
	 * 增量调整某种压力。
	 *   - Delta 可以为负（用于减少压力）
	 *   - Clamp 到 [0, 100]
	 *   - 不主动改 bRunActive；调用 IsRunFailed() 综合判定
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Pressure")
	void AddPressure(EWacomPressureType Type, int32 Delta);

	/**
	 * 直接设置压力值（覆盖语义）。
	 * 主要用于"幂等型"压力（如负重，由超容数量直接算出）。
	 * Clamp 到 [0, 100]。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Pressure")
	void SetPressure(EWacomPressureType Type, int32 Value);

	/**
	 * 减少某种压力（AddPressure(Type, -Amount) 的命名别名）。
	 * Amount 为负时不做任何处理。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Pressure")
	void RemovePressure(EWacomPressureType Type, int32 Amount);

	/** 单条压力归零。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Pressure")
	void ClearPressure(EWacomPressureType Type);

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Pressure")
	bool IsPressureCapReached() const;

	// ---- 战外行为触发器 ----

	/**
	 * 战外右手破坏行为：节点事件分支选"右手破坏"时调用。
	 * 伤口 +1%。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Pressure")
	void OnRightHandDestructiveAction();

	/**
	 * 永久销毁一张伙伴卡：嗜血 +1%。
	 *
	 * "永久销毁" = 消耗 / 战败丢弃 / 商店出售。
	 * 战斗内"被洗入消耗区"不算永久销毁，不触发本 API。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Pressure")
	void OnCompanionCardPermanentlyDestroyed();

	/**
	 * 完成一次偷窃行为：劣迹累加。
	 *
	 * 公式：
	 *   第 n 次偷窃完成时，劣迹 += n*(n+1)/2 + 1
	 *   n=1 → +2%；n=2 → +4%；n=3 → +7%；n=4 → +11%
	 *
	 * 内部维护 `TheftCount`，每调一次 ++。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Pressure")
	void OnTheftCommitted();

	/**
	 * 重算负重压力（幂等）。
	 *
	 * 公式：
	 *   超出"通量内容容量"的卡数 n → Burden = n*(n+1)/2
	 *   通量内容容量 = Σ(玩家拥有的所有 A 类容器卡 Capacity)
	 *
	 * 由 AcquireCardToRun / DestroyCardByInstance / MoveInstance 等改拥有卡数量的方法
	 * 自动调用，UI 一般不需要手动调。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Pressure")
	void RecomputeBurden();

	// ---- 失败综合判定 ----

	/**
	 * Run 是否失败。
	 *
	 * 任一为 true 即败：
	 *   - 战内 Defeat（bRunActive == false）
	 *   - 战外压力 8 条加和 ≥ 100%
	 *   - 战外手指 = 0
	 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run")
	bool IsRunFailed() const;

	// ---- 经验值与技能 ----

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Skill")
	int32 GetExperienceCurrent() const { return RunState.ExperienceCurrent; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Skill")
	int32 GetExperienceCapacity() const { return RunState.ExperienceCapacity; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Skill")
	int32 GetAcquiredSkillCount() const { return RunState.AcquiredSkills.Num(); }

	/**
	 * 增加经验值。
	 *   - Amount 可为负（用于事件扣经验等）
	 *   - 累计 >= Capacity 时入账：AcquiredSkills 计数 +1，ExperienceCurrent -= Capacity（可能多次入账）
	 *   - 当前技能内容占位：用一个固定 SkillSlot.Placeholder tag，不挂效果
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Skill")
	void AddExperience(int32 Amount);

	// ---- 时段 / 节点 ----

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Time")
	ETimePhase GetCurrentTimePhase() const { return RunState.CurrentTimePhase; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Time")
	int32 GetRemainingNodeCount() const { return RunState.RemainingNodeCount; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Time")
	int32 GetCurrentDayNumber() const { return RunState.CurrentDayNumber; }

	/**
	 * 消耗当前时段的节点数。
	 *   - Count <= 0 时视为成功且不修改状态
	 *   - 消耗后剩余 ≤ 0 时调用 AdvanceToNextPhase 自动推进
	 *   - 返回是否成功消耗（节点不足时返回 false 但仍会扣到 0 + 推进）
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Time")
	bool ConsumeNode(int32 Count = 1);

	/**
	 * 推进到下一时段。
	 * 一般不该手动调用，由 ConsumeNode 自动触发；留 public 调试用。
	 *
	 * 当前推进规则：
	 *   Morning → Day → Dusk → Night → Sunrise → Morning（次日，CurrentDayNumber++）
	 *
	 * 露营特殊推进（Night → Morning 跳过 Sunrise）等特殊节点效果后续单独接入。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Time")
	void AdvanceToNextPhase();

	// ---- 背包与备战卡组 ----

	/**
	 * 通量存放区当前容量。
	 *
	 * 公式：Σ(玩家拥有的所有 A 类容器卡的 Capacity)
	 *      = 遍历当前 RunState 全部物理持有区，对所有 IsTypeAContainerCard 的卡求和
	 * B 类容器卡（CapacityEffect 非空）不计入此公式，但其 Capacity 仍计入备战容量。
	 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Deck")
	int32 GetFluxCapacity() const;

	/**
	 * 备战区当前容量。
	 *
	 * 公式：Σ(玩家拥有的所有容器卡 Capacity)
	 *      = 遍历当前 RunState 全部物理持有区，对所有 IsContainerCard 的卡求和
	 * A 类和 B 类容器卡都计入备战区容量。
	 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Deck")
	int32 GetBattleDeckCapacity() const;

	/**
	 * 构建背包界面可直接读取的存放区 Snapshot。
	 *
	 * 只读查询：不修改 RunState、不触发广播、不创建新的 FCardInstance。
	 * 用于把物理四区重组为通量内容、特殊区、负重区、备战物理卡与备战投影卡。
	 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Deck|Snapshot")
	FRunBackpackStorageSnapshot BuildBackpackStorageSnapshot() const;

	/** 卡是否是容器卡（Capacity > 0）。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Deck")
	static bool IsContainerCard(const UCardDefinition* Card);

	/**
	 * A 类容器卡：容器卡且 CapacityEffect 为空。
	 * A 类容器卡的 Capacity 计入 GetFluxCapacity。
	 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Deck")
	static bool IsTypeAContainerCard(const UCardDefinition* Card);

	/**
	 * B 类容器卡：容器卡且 CapacityEffect 有效 tag。
	 * B 类容器卡自己开辟特殊存放区，不进通量公式。
	 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Deck")
	static bool IsTypeBContainerCard(const UCardDefinition* Card);

	/**
	 * B 类容器卡的特殊存放区容量。
	 * 公式：Capacity - 1。Capacity = 0 时返回 0。
	 *
	 * 调用方需要自己保证传入的是 B 类容器卡，否则结果无意义。
	 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Deck")
	static int32 GetSpecialZoneCapacity(const UCardDefinition* BCard);

	/**
	 * 枚举玩家拥有的所有 B 主卡 instance 的 InstanceId。
	 *
	 * 行为契约：
	 *   - 严格按 `RunState.SpecialZones` 数组下标升序输出每条 entry 的 `OwnerInstanceId`；
	 *   - 输出去重（不变量保证 SpecialZones 内 OwnerInstanceId 应唯一，
	 *     这里仍做一次防御性 dedupe）；
	 *   - 不含悬空 InstanceId：若某 OwnerInstanceId 在 Backpack ∪ BattleDeck 中找不到对应
	 *     instance，则跳过该 entry（防御性，正常状态下不应触发）；
	 *   - 玩家无任何 B 主卡（即 SpecialZones 为空）→ 输出空数组。
	 *
	 * 输出按值，调用方持有 InstanceId 的拷贝，跨帧使用安全。
	 */
	void CollectTypeBContainers(TArray<FGuid>& OutOwnerInstanceIds) const;

	/**
	 * 按 OwnerInstanceId 查询某 B 主卡的特殊存放区当前容量。
	 *
	 * 公式：`FMath::Max(0, OwnerDefinition->Physique.Capacity - 1)`
	 *
	 * 与已有的 `static GetSpecialZoneCapacity(BCard)` 数值一致；区别在于本函数按
	 * **OwnerInstanceId 关键字**查询当前 RunState 中实际存在的 SpecialZone：
	 *   - `OwnerInstanceId` 在 `RunState.SpecialZones` 中找不到对应 entry → 返回 0；
	 *   - 找到 entry 但 owner instance 在所有 zone 中都找不到（理论上违反 SpecialZone 不变量，
	 *     防御性返回）→ 返回 0；
	 *   - 找到 owner instance 但 `Definition == nullptr` → 走 `FMath::Max(0, 0 - 1)` = 0；
	 *   - 正常路径 → 返回 `FMath::Max(0, OwnerDefinition->Physique.Capacity - 1)`，
	 *     在 `Capacity <= 1` 时钳到 0。
	 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Deck")
	int32 GetSpecialZoneCapacityFor(FGuid OwnerInstanceId) const;

	/**
	 * 按 OwnerInstanceId 查询某 B 主卡的特殊存放区。
	 *
	 * 行为：
	 *   - 命中（`RunState.SpecialZones` 中存在 `OwnerInstanceId == InOwnerInstanceId` 的条目）
	 *     → 把该 entry 拷贝到 Out 后返回 true；
	 *   - 未命中（含传入 `FGuid()`）→ 返回 false 且**不修改 Out**，保持调用方传入的初值不被覆写。
	 *
	 * 注：返回的是按值拷贝（FSpecialZone 含 TArray<FCardInstance>），调用方拿到的是快照，
	 * 后续 RunState 写路径不会反向影响 Out。需要稳定别名时改用 GetRunState().SpecialZones。
	 */
	bool GetSpecialZone(FGuid OwnerInstanceId, FSpecialZone& Out) const;

	/**
	 * 切换 SpecialZone 中某张 instance 的 `bBattleEnabledInSpecialZone` 参战标记
	 * 行为契约（决策 Q-D：仅切 flag 不移卡）：
	 *   - InstanceId 当前位于某 `FSpecialZone.Cards`（任意 SpecialZone）：
	 *       a) 把该 instance 的 `bBattleEnabledInSpecialZone` 设为 bEnabled；
	 *       b) 不修改该 instance 的物理归属（仍由原 `FSpecialZone.Cards` 数组在原下标位置持有）；
	 *       c) 尾部广播一次 `OnRunStateChangedNative`；
	 *       d) 返回 true。
	 *   - InstanceId 不在任何 SpecialZone 中（含 Backpack / BattleDeck / BurdenZone 命中、
	 *     `FGuid()`、所有 zone 都未命中）：
	 *       a) 不修改 RunState 任何字段；
	 *       b) 不广播；
	 *       c) 返回 false。
	 *
	 * 注：bEnabled 与当前 flag 值相等时仍广播一次；订阅方刷新逻辑应保证幂等。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Deck")
	bool SetSpecialZoneCardBattleEnabled(FGuid InstanceId, bool bEnabled);

	/** 卡是否带 BagProvider 关键词。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Deck")
	static bool IsBagProviderCard(const UCardDefinition* Card);

	/** 卡是否带 DeleteProvider 关键词。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Deck")
	static bool IsDeleteProviderCard(const UCardDefinition* Card);

	/** 卡是否是固有稀有度（Rarity == Card.Rarity.Intrinsic）。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Deck")
	static bool IsIntrinsicCard(const UCardDefinition* Card);

	/** 玩家是否能打开背包 UI：玩家持有区至少存在一张容器卡（Capacity > 0）。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Deck")
	bool IsBackpackUiAvailable() const;

	/**
	 * 删牌功能是否可用：玩家持有区至少存在一张 DeleteProvider 卡。
	 *
	 * 当前 UI 始终显示删牌区；本接口保留给后续按能力开关删牌入口。
	 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Deck")
	bool IsDeleteFunctionAvailable() const;

	const TArray<FCardInstance>& GetBackpack() const { return RunState.Backpack; }
	const TArray<FCardInstance>& GetBattleDeck() const { return RunState.BattleDeck; }

	/**
	 * 全表查找入口。
	 *
	 * 给定 InstanceId 即可定位其当前所属 zone：
	 *   - 命中 → 写入三个 out 参数（OutInstance / OutZone / OutZoneOwnerInstanceId）后返回 true
	 *   - 未命中（含 InstanceId 为 `FGuid()`）→ 返回 false 且**不修改**三个 out 参数，
	 *     保持调用方传入的初值不被覆写。
	 *
	 * 注：来自 Backpack / BattleDeck / BurdenZone 的命中，`OutZoneOwnerInstanceId` 写为 `FGuid()`；
	 * 仅 SpecialZone 命中时才写主卡 InstanceId。
	 */
	bool FindInstance(FGuid InstanceId, FCardInstance& OutInstance, EZoneKind& OutZone, FGuid& OutZoneOwnerInstanceId) const;

	/**
	 * 通用迁移入口。
	 *
	 * 校验表：
	 *   - ToZone == Backpack            → 普通卡 / A 类容器卡需要通量内容区未满；B 主卡不占通量内容格
	 *   - ToZone == BattleDeck          → BattleDeck.Num() < GetBattleDeckCapacity()
	 *                                       （FromZone 已是 BattleDeck 的 in-place 移动不计入 capacity 检查）
	 *   - ToZone == SpecialZone         →
	 *                                       a) ToZoneOwnerInstanceId 在 SpecialZones 中存在
	 *                                       b) InstanceId != ToZoneOwnerInstanceId（B 主卡不能放进自己）
	 *                                       c) 目标 SpecialZone.Cards.Num() < GetSpecialZoneCapacityFor(ToZoneOwnerInstanceId)
	 *                                          （in-place 同 SpecialZone 不计入 capacity）
	 *                                       d) InstanceId 在所有 zone 中存在 — 由 FindInstance 校验
	 *   - ToZone == BurdenZone          → 无额外校验（API 允许；UI 不主动暴露此入口）
	 *   - InstanceId 在所有 zone 中均不存在 → 拒绝
	 *
	 * 行为：
	 *   - 校验失败 → return false 且 RunState 任何字段不修改、不广播 OnRunStateChangedNative
	 *   - 校验通过 → 从源 zone 删除该 instance、追加到目标 zone 末尾、尾部统一广播一次 NotifyRunStateChanged
	 *
	 * 注：成功移动到 BurdenZone 以外的 zone 后会调用 RecomputeBurdenInternal，
	 *      确保 A 类容器作为通量内容占格后的超容状态被及时整理。
	 *
	 * @param InstanceId               要移动的卡 InstanceId
	 * @param ToZone                   目标 zone 种类
	 * @param ToZoneOwnerInstanceId    仅当 ToZone == SpecialZone 时有意义；其他情况传 FGuid()
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Deck")
	bool MoveInstance(FGuid InstanceId, EZoneKind ToZone, FGuid ToZoneOwnerInstanceId);

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Deck")
	FRunDeckOperationValidation ValidateMoveInstance(FGuid InstanceId, EZoneKind ToZone, FGuid ToZoneOwnerInstanceId) const;

	/**
	 * 战外获得一张卡的统一入口。
	 *
	 * 当前实现等价于加入背包并重算负重；后续战斗奖励、节点事件、商店购买、
	 * 探险拾取都应优先走本入口，而不是各自生成 FCardInstance。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Deck")
	void AcquireCardToRun(UCardDefinition* Card);

	/**
	 * 按 InstanceId 精确永久销毁一张已拥有卡，不发金币。
	 *
	 * 遵守 Intrinsic / 最后容量来源 / SpecialZone / Companion 嗜血等永久销毁规则。
	 *
	 * 返回 true=销毁成功并广播一次；false=拒绝且不广播。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Deck")
	bool DestroyCardByInstance(FGuid InstanceId);

	/** 校验 DestroyCardByInstance 是否可执行；不修改 RunState。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Deck")
	FRunDeckOperationValidation ValidateDestroyCardByInstance(FGuid InstanceId) const;

	/** 删牌区入口：按 instance 精确销毁一张卡并按稀有度发金币。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Deck")
	bool DeleteCardForGoldByInstance(FGuid InstanceId);

	/** 删牌区统一金币奖励查询：白=1 / 蓝=2 / 其他=0。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Deck")
	static int32 GetDeleteGoldRewardForCard(const UCardDefinition* Card);

	/** 删牌区 instance 金币奖励查询：找不到 instance 时返回 0。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Deck")
	int32 GetDeleteGoldRewardForInstance(FGuid InstanceId) const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Deck")
	FRunDeckOperationValidation ValidateDeleteCardForGoldByInstance(FGuid InstanceId) const;

	// ---- 经济：金币 ----

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Economy")
	int32 GetGold() const { return RunState.Gold; }

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Economy")
	void AddGold(int32 Amount);

	/** 移除金币（不允许变负，超过则失败返回 false）。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Economy")
	bool RemoveGold(int32 Amount);

	/** 指定世界拾取物是否已在当前 Run 中被拾取。当前只保存在内存态，不接 SaveGame。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Pickup")
	bool IsPickupCollected(FName PersistentId) const;

	/**
	 * 拾取金币型世界拾取物。
	 *
	 * 成功时同一事务内增加金币并标记 PersistentId 已拾取；重复拾取、空 ID 或非正数金币会拒绝且不广播。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Pickup")
	bool CollectGoldPickup(FName PersistentId, int32 GoldAmount);

	/**
	 * 拾取固定卡牌型世界拾取物。
	 *
	 * 成功时复用获得卡牌入 Run 的规则并标记 PersistentId 已拾取；重复拾取、空 ID 或空卡牌会拒绝且不广播。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Pickup")
	bool CollectCardPickup(FName PersistentId, UCardDefinition* CardDefinition);

	/** 指定探索期世界卡牌交互是否已在当前 Run 中完成。当前只保存在内存态，不接 SaveGame。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|World Interaction")
	bool IsRunWorldInteractionCompleted(FName PersistentId) const;

	/** 校验一张精确卡牌 instance 是否可提交到指定 Run world card interaction；不修改 RunState。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|World Interaction")
	FRunWorldCardInteractionValidation ValidateRunWorldCardInteraction(
		const FRunWorldCardInteractionRequest& Request) const;

	/**
	 * 提交探索期世界卡牌交互事务。
	 *
	 * 成功时可选消耗精确 SourceCardInstanceId、增加金币奖励、标记 PersistentId 已完成，并统一广播一次。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|World Interaction")
	bool SubmitRunWorldCardInteraction(const FRunWorldCardInteractionRequest& Request);

	// ---- 商店购买 ----

	/**
	 * 开始一次商店访问。
	 *
	 * ShopId 使用场景商店/节点的 PersistentId。第一次打开该 ShopId 时用传入 Offers 初始化库存；
	 * 之后重复打开同一 ShopId 会保留既有库存和已购买状态，忽略新的 Offers。
	 * 打开商店不消耗节点，关闭时若本次访问买过至少一件商品才消耗 1 节点。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Shop")
	bool BeginShopVisit(FName ShopId, const TArray<FRunShopOfferInput>& Offers);

	/**
	 * 结束当前商店访问并清理访问标记。
	 *
	 * 如果本次访问买过至少一件商品，则在关闭时消耗 1 节点；未购买则不消耗。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Shop")
	void EndShopVisit();

	/** 当前是否处于一次商店访问。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Shop")
	bool IsShopVisitActive() const { return !RunState.ActiveShopId.IsNone(); }

	/** 当前商店访问是否已经买过至少一件商品。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Shop")
	bool HasCurrentShopVisitPurchase() const { return RunState.bShopVisitHasPurchase; }

	/** 构建当前商店的只读快照。无 active shop 时返回空快照。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Shop")
	FRunShopSnapshot BuildCurrentShopSnapshot() const;

	/**
	 * 购买当前商店中的一条商品。
	 *
	 * 成功后立即扣金币、获得卡牌并标记 Offer 已购买；节点消耗延迟到 EndShopVisit。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Shop")
	bool PurchaseShopOffer(FGuid OfferId);

	/**
	 * 从商店购买一张卡。
	 *
	 * 行为：
	 *   - Card 为空、Price < 0、金币不足时拒绝，不修改 RunState。
	 *   - 成功时扣金币，并通过 AcquireCardToRun 的同等内部路径把卡加入背包。
	 *   - 不处理商店库存或节点消耗；正式商店节点流程使用 PurchaseShopOffer。
	 *
	 * @return true=购买成功；false=拒绝。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Shop")
	bool PurchaseCardFromShop(UCardDefinition* Card, int32 Price);

	// ---- 探索事件 ----

	/**
	 * 开始一次轻量事件访问。
	 *
	 * PersistentId 使用场景事件 Actor 的持久化 ID；EventDefinition 只提供静态事件图。
	 * 已完成的 PersistentId 当前拒绝再次打开。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Event")
	bool BeginRunEvent(FName PersistentId, UWacomRunEventDefinition* EventDefinition);

	/** 结束当前事件访问，不改变完成状态。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Event")
	void EndRunEvent();

	/** 当前是否处于一次事件访问。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Event")
	bool IsRunEventActive() const { return !RunState.ActiveRunEventId.IsNone(); }

	/** 指定场景事件是否已完成。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Event")
	bool IsRunEventCompleted(FName PersistentId) const;

	/** 指定 Run 标记是否已设置。RunFlag 当前只在本次 Run 内存态保留，不接 SaveGame。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Event")
	bool IsRunFlagSet(FName FlagId) const;

	/** 构建当前事件的只读 UI 快照。无 active event 时返回空快照。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Event")
	FRunEventSnapshot BuildCurrentRunEventSnapshot() const;

	/**
	 * 选择当前节点中的一个选项。
	 *
	 * 成功后执行 Effects，并按 Choice 配置跳转、关闭或标记完成。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Event")
	bool ChooseRunEventOption(FName ChoiceId);

	/**
	 * 选择当前节点中的一个选项，并返回本次实际执行结果。
	 *
	 * UI/日志可读取 EffectResults 播放反馈；规则真相仍由 RunSession 内部执行。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Event")
	FRunEventChoiceResult ChooseRunEventOptionWithResult(FName ChoiceId);

	/** 校验当前 RunEvent 选项是否接受这张已持有卡作为支付；不修改 RunState。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Event")
	FRunDeckOperationValidation ValidateRunEventOptionCardPayment(FName ChoiceId, FGuid PaidCardInstanceId) const;

	/**
	 * 选择当前节点中的卡牌支付选项，并精确支付一张已持有卡。
	 *
	 * 支付卡移除、Effects、节点跳转、关闭/完成标记在同一事务中执行；任一步失败都会回滚。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Event")
	FRunEventChoiceResult ChooseRunEventOptionWithPaidCardResult(FName ChoiceId, FGuid PaidCardInstanceId);

	// ---- 战斗联动 ----

	/**
	 * 构造一场战斗所需的 FBattleInitParams。
	 * GameMode 在 EnterBattle 时调用。
	 *
	 * 行为：
	 *   - Params.Character = RunState.Character（共享同一份 Character DataAsset）
	 *   - Params.RandomSeed = RunState.BattleSeed
	 *   - Params.BattleDeckEntries = RunState.BattleDeck + 已启用入战的 SpecialZone 卡
	 *   - Params.HighHpThreshold / LowHpThreshold = RunState 字段
	 *   - Params.EncounterId = TriggerPersistentId（为空时使用默认 Encounter）
	 *   - **Params.PreDestroyedParts**：若 TriggerPersistentId 非空且 RunState.BattleProgress
	 *     中有该 Trigger 的进度，灌入已破坏的完整部位身份。
	 *     旧 DestroyedPartIds 进度会转换到默认 Enemy 槽作为兼容。
	 *
	 * RunSession 不读取、不接收、不写入敌人定义。调用方（当前为 GameMode / BattleTrigger）
	 * 负责把 EncounterDefinition 转成 Params.EnemySlots。
	 *
	 * @param TriggerPersistentId    触发战斗的 Trigger 的持久化 ID。NAME_None 表示没有持久化进度（如纯测试）
	 * @param OutParams              输出参数
	 */
	bool BuildInitParamsForBattle(FName TriggerPersistentId, FBattleInitParams& OutParams) const;

	/** 兼容老调用点（无 TriggerPersistentId）。等同于 TriggerPersistentId = NAME_None。 */
	bool BuildInitParamsForBattle(FBattleInitParams& OutParams) const;

	/**
	 * 一场战斗结束时由 GameMode::ExitBattle 调用。
	 *
	 * 战斗结果回传处理：
	 *   - Outcome 处理：
	 *       Victory：清理对应 Trigger 的 BattleProgress；场景完成由 MarkTriggerDestroyed 记录
	 *       Defeat ：标记 bRunActive = false
	 *       Undetermined：不改变状态（用于异常或玩家取消）
	 *   - 战斗结束疲劳压力 +1%（不分胜败）
	 *   - bCrossedHighHpThreshold → 伤口压力 +1%
	 *   - bCrossedLowHpThreshold  → 伤口压力 +5%
	 *   - bMutualDestruction      → 伤口压力 +10%
	 *
	 * 同归于尽（Outcome=Victory + bMutualDestruction=true）：
	 *   - 战外不触发失败（bRunActive 保持 true）
	 *   - 仅累计伤口压力
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run")
	void OnBattleFinished(const FBattleResultPacket& Packet);

	/**
	 * OnBattleFinished 扩展版：传入触发战斗的 Trigger 持久化 ID，让 Run 层能：
	 *   - 撤离时写 RunState.BattleProgress[TriggerId]
	 *   - 真胜利时清 RunState.BattleProgress[TriggerId]
	 *
	 * @param Packet                 战斗回传包
	 * @param TriggerPersistentId    Trigger 持久化 ID。NAME_None 表示无 Trigger（如纯测试或调试战斗）
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run")
	void OnBattleFinishedFromTrigger(const FBattleResultPacket& Packet, FName TriggerPersistentId);

	/** 场景：标记一个触发器已被永久销毁。 */
	void MarkTriggerDestroyed(FName PersistentId);

	/** 场景：某触发器是否已被销毁（关卡加载时查）。 */
	bool IsTriggerDestroyed(FName PersistentId) const;

	/** 场景：记录玩家当前 Transform（用于下次启动恢复）。 */
	void SetPlayerTransform(const FTransform& InTransform);

	// ---- 存档 / 读档 ----

	/**
	 * 写入指定 slot（文件位置 `Saved/SaveGames/{SlotName}.sav`）。
	 * 成功返回 true。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Save")
	bool SaveToSlot(const FString& SlotName) const;

	/**
	 * 从指定 slot 读档，并把字段应用到当前 RunState。
	 * 成功返回 true；失败时 RunState 不被修改。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Save")
	bool LoadFromSlot(const FString& SlotName);

	/** 指定 slot 是否存在存档文件。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Save")
	bool HasSaveInSlot(const FString& SlotName) const;

	// ---- 存档字段拷贝（公开以便测试）----

	/** 把当前 RunState 拷贝到一个新建的 UWacomSaveGame。 */
	UWacomSaveGame* BuildSaveGameFromRunState() const;

	/** 把 SaveGame 字段应用到当前 RunState（含版本检查 + 资产 TryLoad）。 */
	bool ApplySaveGameToRunState(UWacomSaveGame* SaveGame);

private:
#if WITH_AUTOMATION_TESTS
	friend struct FWacomRunSessionTestAccess;
#endif

	struct FScopedRunStateNotificationBatch;

	void MarkRunUiSnapshotsDirty(uint8 DirtyFlags);

	/**
	 * 当 ExperienceCurrent ≥ ExperienceCapacity 时入账技能（可多次）。
	 * 当前用占位 tag `SkillSlot.Placeholder`，不挂效果。
	 */
	void TryConsumeExperienceForSkills();

	/**
	 * 通知 OnRunStateChangedNative 订阅者。
	 *
	 * 调用约定：每个修改 RunState 字段的 public 入口在末尾调用一次。
	 * 失败/拒绝路径（提前 return）不调用。
	 * 会改变 Backpack / Shop / Economy UI Snapshot 事实的事务，必须先调用
	 * MarkRunUiSnapshotsDirty(...) 标记对应 transient revision。
	 *
	 * 内部互调（例如 AcquireCardToRun 调 RecomputeBurden）由外层入口统一发，
	 * 内部辅助不发，避免一次操作多次广播的尾部串。
	 */
	void NotifyRunStateChanged();

	void BroadcastRunStateChangedImmediately();
	void BeginRunStateNotificationBatch();
	void EndRunStateNotificationBatch();

	/**
	 * 私有路径：RecomputeBurden 的"不广播"版本。
	 *
	 * 行为与 public `RecomputeBurden` 完全一致（步骤 ① 超容溢出 + ② 回填 + ③ 写 Burden 压力），
	 * 区别仅在 ③ 直接写 `RunState.Pressure.Set(EWacomPressureType::Burden, ...)`（绕过
	 * public `SetPressure` 内部的 NotifyRunStateChanged 调用），且本函数不在末尾广播。
	 *
	 * 调用约定：
	 *   - 由其他 public 入口（Initialize / AcquireCardToRun / DestroyCardByInstance /
	 *     MoveInstance）内部链式调用，避免该 public 入口在尾部
	 *     NotifyRunStateChanged 之外多发一次广播。
	 *   - 直接被外部蓝图 / 测试调用的 `RecomputeBurden()` 是公开入口，内部先调本函数再
	 *     `NotifyRunStateChanged()` 一次。
	 */
	void RecomputeBurdenInternal(bool bAllowBurdenRefill = true);

	/** 私有路径：AcquireCardToRun 的"不广播"版本，供复合 Run 操作统一尾部广播。 */
	bool AcquireCardToRunInternal(UCardDefinition* Card);

	/** 私有路径：DestroyCardByInstance 的"不广播"版本。 */
	bool DestroyCardByInstanceInternal(FGuid InstanceId);

	/**
	 * 幂等保证：B 主卡 instance 进入 Backpack/BattleDeck 时 `RunState.SpecialZones`
	 * 中存在一条 `OwnerInstanceId == Inst.InstanceId` 的 entry。
	 *
	 * 行为：
	 *   - `Inst.Definition` 不是 B 主卡（`IsTypeBContainerCard == false`） → 直接返回，不修改 RunState；
	 *   - `Inst.InstanceId` 为 zero GUID（防御性）→ 直接返回，不写入 SpecialZones；
	 *   - 已存在同 OwnerInstanceId 的 entry → 直接返回（幂等）；
	 *   - 否则在 `RunState.SpecialZones` 末尾追加 `FSpecialZone{ OwnerInstanceId = Inst.InstanceId, Cards = {} }`。
	 *
	 * 调用点：
	 *   - `Initialize` 把 StarterDeck 灌入两区时；
	 *   - `AcquireCardToRun` 新加 B 主卡时；
	 *   - `MoveInstance` 把 B 主卡 instance 跨入 Backpack/BattleDeck 成功后（防御性保底，
	 *     正常路径上 entry 在 Initialize / AcquireCardToRun 阶段已创建）。
	 *
	 * 不在本函数内广播 OnRunStateChangedNative：调用方公共入口在末尾统一广播一次。
	 */
	void EnsureSpecialZoneEntryFor(const FCardInstance& Inst);

	/** 遍历 RunState 全部物理持有区，按过滤条件累计卡牌 Capacity。 */
	int32 SumOwnedCardCapacity(bool bTypeAOnly) const;

	/** 统计指定列表中真正占用通量内容格的卡：A 类容器和普通卡计入，B 主卡不计入。 */
	int32 CountFluxContentCards(const TArray<FCardInstance>& Pile) const;

	UPROPERTY(VisibleAnywhere, Category = "Wacom|Run", Transient)
	FRunState RunState;

	uint64 BackpackStorageSnapshotRevision = 0;
	uint64 ShopSnapshotRevision = 0;
	uint64 EconomySnapshotRevision = 0;

	int32 RunStateNotificationDeferralDepth = 0;
	bool bRunStateNotificationPending = false;
};

#if WITH_AUTOMATION_TESTS
/**
 * Tests-only access for constructing invalid or boundary RunState fixtures.
 *
 * This deliberately stays outside UFUNCTION/Blueprint exposure so production
 * callers keep using narrow RunSession command APIs.
 */
struct FWacomRunSessionTestAccess
{
	static FRunState& GetMutableRunState(URunSession& Session)
	{
		return Session.RunState;
	}
};
#endif
