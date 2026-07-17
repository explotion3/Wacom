// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Deck/RunDeckBatchTypes.h"
#include "Exploration/RunCampActivity.h"
#include "Exploration/RunExplorationCommand.h"
#include "Exploration/RunExplorationResolution.h"
#include "Exploration/RunFloorMapSnapshot.h"
#include "UObject/Object.h"
#include "Types/WacomEnums.h"
#include "RunState.h"
#include "RunStateTypes.h"
#include "Session/BattleResultPacket.h"
#include "RunSession.generated.h"

class UCharacterDefinition;
class UCardDefinition;
class UWacomRunPickupDefinition;
class UWacomRunEventDefinition;
class UWacomSaveGame;
struct FBattleInitParams;

enum class ERunUiSnapshotDirtyFlags : uint8;

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
	 * 以 Character + Journey 初始化完整 Run working state。
	 * 成功后一次提交并返回版本 1 的显式 Snapshot/Events；失败时旧 Session 完全不变。
	 */
	FRunInitializationResult Initialize(const FRunInitializationParams& Params);

	/** 唯一探索命令入口。命令结果不会暂存在 Session 输出队列。 */
	FRunExplorationResolution ResolveExplorationCommand(const FRunExplorationCommand& Command);

	/** 构建当前探索只读事实；不改变状态、不广播。 */
	FRunExplorationSnapshot BuildExplorationSnapshot() const;

	/** 构建当前 Floor 的完整地图只读事实；不改变状态、不广播。 */
	FRunFloorMapSnapshot BuildCurrentFloorMapSnapshot() const;

	/** 为当前 typed Map Node 开始唯一 NodeActivity；成本由规则类型决定，调用方不能提供。 */
	FRunExplorationResolution BeginCurrentNodeActivity(ERunNodeActivityKind Kind);

	/** 仅持有同一票据的调用方可取消；取消释放逻辑预留且不消费 Action Point。 */
	FRunExplorationResolution CancelNodeActivity(const FRunNodeActivityTicket& Ticket);

	/** 完成一个类型化 Camp Activity；handler 只能读取上下文并返回 outcome。 */
	FRunExplorationResolution CompleteCampActivity(
		const FRunCampTicket& Ticket,
		const IRunCampActivityHandler& Handler);

	/** Encounter 战果、预留、压力/奖励和节点生命周期的唯一原子结算入口。 */
	FRunExplorationResolution SettleEncounterNodeActivity(
		const FRunNodeActivityTicket& Ticket,
		const FBattleResultPacket& Packet);

	// ---- 状态访问 ----

	/** 只读：当前 Run 的状态。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run")
	const FRunState& GetRunState() const { return RunState; }

	/** Transient C++-only UI refresh revisions; not serialized and not exposed to Blueprint. */
	uint64 GetBackpackStorageSnapshotRevision() const { return BackpackStorageSnapshotRevision; }
	uint64 GetShopSnapshotRevision() const { return ShopSnapshotRevision; }
	uint64 GetEconomySnapshotRevision() const { return EconomySnapshotRevision; }

	/** 是否仍可继续 Run；Succeeded/Failed 和既有失败门槛均返回 false。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run")
	bool IsRunActive() const;

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
	 *   - FingerCount 降到 0 时由 IsRunFailed() 兼容查询派生 Failed
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
	 *   - 达到失败线时由 IsRunFailed() 兼容查询派生 Failed
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
	 *   - 战内 Defeat（Outcome == Failed）
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

	// ---- 时段 / Action Point ----

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Time")
	ETimePhase GetCurrentTimePhase() const { return RunState.TimeState.CurrentTimePhase; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Time")
	int32 GetRemainingActionPoints() const { return RunState.TimeState.RemainingActionPoints; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Time")
	int32 GetCurrentDayNumber() const { return RunState.TimeState.CurrentDayNumber; }

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

	/**
	 * 构建 Run 第一人称卡牌 workspace 快照。
	 *
	 * Workspace 是展示/操作视图，不改变 Backpack / BattleDeck / BurdenZone /
	 * SpecialZones 的物理归属。App 层用它生成 first-person card layer entries。
	 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Deck|Snapshot")
	FRunCardWorkspaceSnapshot BuildRunCardWorkspaceSnapshot(
		const FRunCardWorkspaceRequest& Request) const;

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

	/**
	 * 校验 SetSpecialZoneCardBattleEnabled 是否可执行；不修改 RunState。
	 *
	 * 当前规则只要求 InstanceId 当前位于任意 SpecialZone。bEnabled 预留给未来按目标状态
	 * 区分校验的规则，调用方不应自行推断 SpecialZone 归属失败原因。
	 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Deck")
	FRunDeckOperationValidation ValidateSetSpecialZoneCardBattleEnabled(FGuid InstanceId, bool bEnabled) const;

	/**
	 * 切换 SpecialZone 中某张 instance 的入战标记。
	 *
	 * App/UI 层用于提交“右键切换”意图；当前值读取、合法性校验和 flag mutation 均留在 Run 层。
	 */
	bool ToggleSpecialZoneCardBattleEnabled(FGuid InstanceId);

	/** 校验 ToggleSpecialZoneCardBattleEnabled 是否可执行；不修改 RunState。 */
	FRunDeckOperationValidation ValidateToggleSpecialZoneCardBattleEnabled(FGuid InstanceId) const;

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
	 *   - ToZone == BurdenZone          → B 主卡拒绝进入；其他卡 API 允许（UI 不主动暴露此入口）
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
	 * C++-only 原子批量移动预检。
	 *
	 * 校验整个 InstanceId 集合、共同来源、目标区、容量和严格 storage revision；不修改 RunState，
	 * 不广播。被动 WBP 不直接调用，由 App Screen command flow 转发意图。
	 */
	FRunDeckBatchOperationValidation ValidateMoveInstancesAtomic(
		const FRunDeckBatchMoveRequest& Request) const;

	/**
	 * C++-only 原子批量移动提交。
	 *
	 * 全部成功才一次替换权威状态、推进背包 storage revision 并广播一次；任一项失败则零修改、
	 * 零 revision 推进、零广播。
	 */
	FRunDeckBatchOperationResult MoveInstancesAtomic(const FRunDeckBatchMoveRequest& Request);

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

	/** C++-only 原子批量删牌预览；返回整组校验和预计总金币，不保留提交授权。 */
	FRunDeckBatchDeletePreview ValidateDeleteCardsForGoldAtomic(
		const FRunDeckBatchDeleteRequest& Request) const;

	/**
	 * C++-only 原子批量删牌提交；确认后重新校验严格 revision，成功只发一次金币、revision 和广播。
	 */
	FRunDeckBatchOperationResult DeleteCardsForGoldAtomic(
		const FRunDeckBatchDeleteRequest& Request);

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

	/** C++ 只读查询：CredentialId 非空且已由 Run 规则授予时返回 true。 */
	bool HasCredential(FName CredentialId) const;

	/**
	 * 数据驱动世界拾取物的正式原子结算入口。
	 *
	 * 主奖励、Definition 声明的全部 Credential、PersistentId、Treasure 节点与 AP
	 * 在同一 working-state 中提交；失败时零修改、零广播。
	 */
	FRunTreasureSettlementResult CollectPickupFromDefinition(
		FName PersistentId,
		const UWacomRunPickupDefinition* PickupDefinition);

	/**
	 * 拾取金币型世界拾取物。
	 *
	 * 成功时同一事务内增加金币并标记 PersistentId 已拾取；重复拾取、空 ID 或非正数金币会拒绝且不广播。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Pickup")
	FRunTreasureSettlementResult CollectGoldPickup(FName PersistentId, int32 GoldAmount);

	/**
	 * 拾取固定卡牌型世界拾取物。
	 *
	 * 成功时复用获得卡牌入 Run 的规则并标记 PersistentId 已拾取；重复拾取、空 ID 或空卡牌会拒绝且不广播。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Pickup")
	FRunTreasureSettlementResult CollectCardPickup(FName PersistentId, UCardDefinition* CardDefinition);

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
	FRunTreasureSettlementResult SubmitRunWorldCardInteraction(const FRunWorldCardInteractionRequest& Request);

	// ---- 商店购买 ----

	/**
	 * 开始一次商店访问。
	 *
	 * ShopId 使用场景商店/节点的 PersistentId。第一次打开该 ShopId 时用传入 Offers 初始化库存；
	 * 之后重复打开同一 ShopId 会保留既有库存和已购买状态，忽略新的 Offers。
	 * 已有 active shop visit 时拒绝重入，必须先结束当前访问。
	 * 打开商店和浏览免费；本次 visit 第一次成功购买原子消耗 1 行动点，后续购买免费。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Shop")
	bool BeginShopVisit(FName ShopId, const TArray<FRunShopOfferInput>& Offers);

	/** C++ 正式入口：同时返回 visit ownership 与探索版本结果。 */
	FRunShopVisitResult BeginShopVisitWithResult(
		FName ShopId,
		const TArray<FRunShopOfferInput>& Offers);

	/** C++ UI ownership token for the currently active shop visit. */
	FGuid GetActiveShopVisitToken() const { return ActiveShopVisitToken; }

	/** Ends the shop visit only when the caller still owns the active visit token. */
	bool EndShopVisitIfOwned(FGuid VisitToken);

	/** C++ 正式入口：只结束调用方仍拥有的 visit，并显式返回探索版本结果。 */
	FRunShopVisitResult EndShopVisitIfOwnedWithResult(FGuid VisitToken);

	/**
	 * 结束当前商店访问并清理访问标记。
	 *
	 * 关闭只结束访问；不会追加行动点成本。未购买时会取消 pending Shop activity。
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
	 * 成功后原子提交金币、卡牌、Offer 状态；本次 visit 第一次成功购买同时消耗 1 行动点。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Shop")
	FRunShopPurchaseResult PurchaseShopOffer(FGuid OfferId);

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
	 * 已有 active RunEvent visit 时拒绝重入，必须先结束当前访问。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Event")
	bool BeginRunEvent(FName PersistentId, UWacomRunEventDefinition* EventDefinition);

	/** C++ 正式入口：开始事件访问并显式返回探索版本结果。 */
	FRunExplorationResolution BeginRunEventWithExplorationResult(
		FName PersistentId,
		UWacomRunEventDefinition* EventDefinition);

	/** C++ UI ownership token for the currently active RunEvent visit. */
	FGuid GetActiveRunEventVisitToken() const { return ActiveRunEventVisitToken; }

	/** Ends the RunEvent only when the caller still owns the active visit token. */
	bool EndRunEventIfOwned(FGuid VisitToken);

	/** C++ 正式入口：只结束调用方仍拥有的事件访问，并显式返回探索版本结果。 */
	FRunExplorationResolution EndRunEventIfOwnedWithExplorationResult(FGuid VisitToken);

	/** 结束当前事件访问，不改变完成状态。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Event")
	void EndRunEvent();

	/** C++ 入口：结束当前事件访问，并显式返回探索版本结果。正式 UI 应优先使用 owned 入口。 */
	FRunExplorationResolution EndRunEventWithExplorationResult();

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
	 *
	 * RunSession 不读取、不接收、不写入敌人定义。调用方（当前为 GameMode / BattleTrigger）
	 * 负责把 EncounterDefinition 转成 Params.EnemySlots。
	 *
	 * @param TriggerPersistentId    触发战斗的 Trigger 的持久化 ID。NAME_None 表示没有持久化进度
	 * @param OutParams              输出参数
	 */
	bool BuildInitParamsForBattle(FName TriggerPersistentId, FBattleInitParams& OutParams) const;

	/** SaveGame v3 兼容投影：记录已由 Map Node Resolved 判定完成的 Trigger。 */
	void MarkTriggerDestroyed(FName PersistentId);

	/** SaveGame v3 兼容投影：关卡加载时查询对应完成 Trigger。 */
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

	void MarkRunUiSnapshotsDirty(ERunUiSnapshotDirtyFlags DirtyFlags);

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

	/** RunEvent 选项、卡牌支付、行动点与地图生命周期的单次 working-state 事务。 */
	FRunEventChoiceResult ResolveRunEventChoiceInternal(
		FName ChoiceId,
		TOptional<FGuid> PaidCardInstanceId);

	/** 商店关闭的无 ownership 内部实现；公开 UI 路径应使用 owned result 入口。 */
	FRunShopVisitResult EndShopVisitWithResult();
	/** 私有路径：AcquireCardToRun 的"不广播"版本，供复合 Run 操作统一尾部广播。 */
	bool AcquireCardToRunInternal(UCardDefinition* Card);

	/** 私有路径：DestroyCardByInstance 的"不广播"版本。 */
	bool DestroyCardByInstanceInternal(FGuid InstanceId);

	/** Gold/Card/Definition Pickup 共用的 working-state 原子结算路径。 */
	FRunTreasureSettlementResult CollectPickupInternal(
		FName PersistentId,
		int32 GoldAmount,
		UCardDefinition* CardDefinition,
		const TArray<FName>& GrantedCredentialIds);

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
	 *
	 * 不在本函数内广播 OnRunStateChangedNative：调用方公共入口在末尾统一广播一次。
	 * `FRunDeckRules::MoveInstance` 也会在 B 主卡跨入 Backpack/BattleDeck 成功后
	 * 直接调用 Deck helper 侧的同名静态规则函数作为防御性保底。
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

	/** Transient UI ownership; never serialized as RunState/SaveGame data. */
	TOptional<FRunTraversalTicket> ActiveTraversalTicket;
	TOptional<FRunNodeActivityTicket> ActiveNodeActivityTicket;
	TOptional<FRunCampTicket> ActiveCampTicket;
	TOptional<FRunFloorTransitionConfirmation> ActiveFloorTransitionConfirmation;
	FGuid ActiveShopVisitToken;
	FGuid ActiveRunEventVisitToken;

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
