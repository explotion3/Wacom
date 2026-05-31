// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "RunStateTypes.generated.h"

class UCardDefinition;

/**
 * 单张卡的运行时实例。
 *
 * 为什么要 instance 化：同款 Definition 的多张卡（例如玩家拥有 3 张同名武器卡）需要被独立放进
 * 不同 zone（备战 / 通量 / 各 SpecialZone / 负重），单纯按 Definition 指针无法区分。Instance 引入后，
 * 每张卡有全局唯一 InstanceId，所有 zone 元素都升级为本结构。
 *
 * 字段语义：
 *   - `InstanceId`：全局唯一 GUID，进入背包系统时用 `FGuid::NewGuid()` 一次性分配，之后只读。
 *   - `Definition`：指向卡牌静态数据；一旦设置不再改写。
 *   - `bBattleEnabledInSpecialZone`：仅当本 instance 位于某 SpecialZone 时有意义。
 *       true  = 随对应 B 主卡入战参战；
 *       false = 仅"被特殊收纳"不参战。
 *     切换该 flag 不修改 instance 的物理归属；当 instance 从 SpecialZone 移出时会重置为 false。
 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FCardInstance
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck")
	FGuid InstanceId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck")
	TObjectPtr<UCardDefinition> Definition = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck")
	bool bBattleEnabledInSpecialZone = false;
};

/**
 * 战外压力八种类型。
 *
 * 压力是战外"血量"，对战内规则零影响。状态效果只是显示层。
 * 总值 = 8 条加和，达到 100% 触发 Run 失败。
 */
UENUM(BlueprintType)
enum class EWacomPressureType : uint8
{
	Hunger     UMETA(DisplayName = "饥饿"),
	Wound      UMETA(DisplayName = "伤口"),
	Fatigue    UMETA(DisplayName = "疲劳"),
	Burden     UMETA(DisplayName = "负重"),
	Decay      UMETA(DisplayName = "腐朽"),
	Misdeed    UMETA(DisplayName = "劣迹"),
	Bloodlust  UMETA(DisplayName = "嗜血"),
	Disability UMETA(DisplayName = "残疾"),
	Count      UMETA(Hidden),
};

/**
 * 一天内的五个时段。
 *
 * 推进顺序：Morning → Day → Dusk → Night → Sunrise → Morning（次日）。
 * 任一时段节点用完时自动推进到下一时段。
 * 露营特殊推进：Night → Morning（跳过 Sunrise），后续由特殊节点效果接入。
 */
UENUM(BlueprintType)
enum class ETimePhase : uint8
{
	Morning UMETA(DisplayName = "清晨"),
	Day     UMETA(DisplayName = "日间"),
	Dusk    UMETA(DisplayName = "黄昏"),
	Night   UMETA(DisplayName = "夜间"),
	Sunrise UMETA(DisplayName = "日出"),
	Count   UMETA(Hidden),
};

/**
 * 八种压力值容器。每条独立 0~100 的累计百分比。
 *
 * 字段直接拆开（而非 array / map）：
 *   - 每条压力名字直接对应领域概念，debug 友好
 *   - 字段稳定，避免 array 索引写错
 *   - 拆开比 array 索引更难写错
 *
 * Get / Set / Add 通过 EWacomPressureType 分派，调用方按枚举操作。
 * GetTotal() 用于 Run 失败判定。
 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FPressureValues
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pressure")
	int32 Hunger = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pressure")
	int32 Wound = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pressure")
	int32 Fatigue = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pressure")
	int32 Burden = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pressure")
	int32 Decay = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pressure")
	int32 Misdeed = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pressure")
	int32 Bloodlust = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pressure")
	int32 Disability = 0;

	/** 8 条加和。 */
	int32 GetTotal() const
	{
		return Hunger + Wound + Fatigue + Burden + Decay + Misdeed + Bloodlust + Disability;
	}

	/** 按枚举读取单条值。 */
	int32 Get(EWacomPressureType Type) const;

	/** 按枚举设值（Clamp 到 [0, 100]）。 */
	void Set(EWacomPressureType Type, int32 Value);

	/** 按枚举增量（可负，Clamp 到 [0, 100]）。 */
	void Add(EWacomPressureType Type, int32 Delta);
};

/**
 * 卡牌存放区种类。
 *
 * 配合 OwnerInstanceId 共同定位某张 FCardInstance 当前所属位置。
 * 互斥四选一：每个 InstanceId 同时只能在 Backpack / BattleDeck / SpecialZone(某 OwnerInstanceId) / BurdenZone 之一。
 *
 * - Backpack：通量存放区 + B 主卡所在的背包槽（玩家在背包中能直接看到的卡）
 * - BattleDeck：备战卡组
 * - SpecialZone：B 主卡开辟的特殊存放区（须配合 OwnerInstanceId 定位具体哪张 B 主卡的 SpecialZone）
 * - BurdenZone：负重区，其他三区溢出兜底
 */
UENUM(BlueprintType)
enum class EZoneKind : uint8
{
	Backpack    UMETA(DisplayName = "通量存放区 + B 主卡所在背包槽"),
	BattleDeck  UMETA(DisplayName = "备战卡组"),
	SpecialZone UMETA(DisplayName = "B 主卡的特殊存放区"),
	BurdenZone  UMETA(DisplayName = "负重区"),
};

USTRUCT(BlueprintType)
struct WACOMRUN_API FRunDeckOperationValidation
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck|Validation")
	bool bCanExecute = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck|Validation")
	FName DisabledReason = NAME_None;
};

/**
 * 探索期“拖一张已拥有卡到场景物体”事务请求。
 *
 * PersistentId 来自场景 Actor，是本次 Run 内防重复完成 key；SourceCardInstanceId 必须是玩家
 * 当前真实持有区中的精确 instance。V1 只支持可选消耗这张卡、获得固定金币并标记完成。
 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FRunWorldCardInteractionRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|World Interaction")
	FName PersistentId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|World Interaction")
	FGuid SourceCardInstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|World Interaction")
	TArray<TObjectPtr<UCardDefinition>> AllowedCardDefinitions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|World Interaction")
	TArray<FName> AllowedCardIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|World Interaction")
	FGameplayTagContainer RequiredKeywords;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|World Interaction")
	FGameplayTagContainer BlockedKeywords;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|World Interaction")
	bool bConsumeCardOnSuccess = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|World Interaction", meta = (ClampMin = "0", UIMin = "0"))
	int32 GoldReward = 0;
};

/** 只读校验结果；失败路径不修改 RunState。 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FRunWorldCardInteractionValidation
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|World Interaction|Validation")
	bool bCanSubmit = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|World Interaction|Validation")
	FName DisabledReason = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|World Interaction|Validation")
	TObjectPtr<UCardDefinition> SourceCardDefinition = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|World Interaction|Validation")
	FName SourceCardId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|World Interaction|Validation")
	FString DebugSummary;
};

/**
 * 单个 B 主卡 instance 在 RunState 中开辟的特殊存放区。
 *
 * 每张玩家拥有的 B 类容器卡（`Capacity > 0` 且 `CapacityEffect` 为有效 GameplayTag）
 * 在 `FRunState.SpecialZones` 数组中刚好对应一条本结构。容量 = 主卡 `Physique.Capacity - 1`。
 *
 * 字段语义：
 *   - `OwnerInstanceId`：主卡 instance 的 InstanceId。同一 OwnerInstanceId 在 SpecialZones 中至多一条。
 *   - `Cards`：本特殊区当前持有的 instance 列表，按下标顺序持久化。
 *
 * 不把 SpecialZone 嵌进 B 主卡 instance，而是 RunState 字段平铺；
 * 这样 UI / 测试 / Save 单独遍历更直观，B 主卡 instance 在 BattleDeck 数组里仍是一个干净的 invariant。
 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FSpecialZone
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck")
	FGuid OwnerInstanceId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck")
	TArray<FCardInstance> Cards;
};

/**
 * 背包 UI 查询用的单卡视图。
 *
 * 这是只读 Snapshot，不是新的物理归属模型；真实归属仍由 FRunState 四个 zone 数组决定。
 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FRunStorageCardView
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck|Snapshot")
	FCardInstance Instance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck|Snapshot")
	EZoneKind PhysicalZone = EZoneKind::Backpack;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck|Snapshot")
	FGuid ZoneOwnerInstanceId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck|Snapshot")
	bool bIsContainer = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck|Snapshot")
	bool bIsTypeAContainer = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck|Snapshot")
	bool bIsTypeBContainer = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck|Snapshot")
	bool bIsPhysicalInBattleDeck = false;
};

/** 通量存放区查询视图：MainCards 仅保留兼容，当前通量内容包含 A 类容器和普通卡。 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FRunFluxStorageView
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck|Snapshot")
	TArray<FRunStorageCardView> MainCards;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck|Snapshot")
	TArray<FRunStorageCardView> ContentCards;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck|Snapshot")
	int32 FluxCapacity = 0;
};

/** 单个 B 类特殊存放区查询视图。 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FRunSpecialStorageView
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck|Snapshot")
	FRunStorageCardView OwnerCard;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck|Snapshot")
	TArray<FRunStorageCardView> ContentCards;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck|Snapshot")
	int32 Capacity = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck|Snapshot")
	bool bOwnerInBattleDeck = false;
};

/** 背包界面用的 Run 层只读存放区 Snapshot。 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FRunBackpackStorageSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck|Snapshot")
	FRunFluxStorageView Flux;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck|Snapshot")
	TArray<FRunSpecialStorageView> SpecialZones;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck|Snapshot")
	TArray<FRunStorageCardView> BurdenCards;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck|Snapshot")
	TArray<FRunStorageCardView> BattleDeckPhysicalCards;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck|Snapshot")
	TArray<FRunStorageCardView> BattleDeckProjectedCards;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck|Snapshot")
	int32 FluxCapacity = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck|Snapshot")
	int32 BattleDeckCapacity = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck|Snapshot")
	int32 BackpackPhysicalCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck|Snapshot")
	int32 FluxContentCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck|Snapshot")
	int32 BattleDeckPhysicalCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck|Snapshot")
	int32 BurdenCount = 0;
};
