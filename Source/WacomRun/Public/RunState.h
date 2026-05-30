// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Events/RunEventDefinition.h"
#include "GameplayTagContainer.h"
#include "Session/BattleSession.h"  // FBattleInitParams
#include "RunStateTypes.h"
#include "RunState.generated.h"

class UCharacterDefinition;
class UCardDefinition;
class UEnemyDefinition;
class UWacomRunEventDefinition;

/**
 * 单个战斗节点（Trigger）的进度快照。
 *
 * 撤离时 Run 层用 packet.DestroyedPartIds 写入 RunState.BattleProgress；
 * 下次进入同一 Trigger 时，BuildInitParamsForBattle 把 DestroyedPartIds
 * 灌进 BattleInitParams.PreDestroyedPartIds，BattleSession 应用为
 * Part.bDestroyed = true（不发经验、不入击倒队列）。
 *
 * 战斗胜利时清理对应 Trigger 的进度。
 *
 * 当前只记录"破坏部位列表"。如果后续需要保存中间血量、部位状态层数等，再扩字段。
 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FBattleProgressSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Battle")
	TArray<FName> DestroyedPartIds;
};

/** 调用方传入的一条商店商品配置。 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FRunShopOfferInput
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Shop")
	TObjectPtr<UCardDefinition> CardDefinition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Shop", meta = (ClampMin = "0", UIMin = "0", ToolTip = "商品价格，单位为金币。0 表示免费商品；负数输入会被跳过。"))
	int32 Price = 0;
};

/** Run 内保存的一条商店商品状态。 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FRunShopOffer
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Shop")
	FGuid OfferId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Shop")
	TObjectPtr<UCardDefinition> CardDefinition = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Shop")
	int32 Price = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Shop")
	bool bPurchased = false;
};

/** 单个商店节点在当前 Run 内的库存状态。 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FRunShopState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Shop")
	TArray<FRunShopOffer> Offers;
};

/** 当前商店 UI/测试可读取的只读快照。 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FRunShopSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Shop")
	FName ShopId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Shop")
	bool bIsActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Shop")
	bool bHasPurchaseThisVisit = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Shop")
	TArray<FRunShopOffer> Offers;
};

/** 单个场景事件节点在当前 Run 内的状态。 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FRunEventState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Event")
	bool bCompleted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Event")
	FName CurrentNodeId = NAME_None;
};

/** 当前事件选项的只读展示/校验快照。 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FRunEventChoiceSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Event")
	FName ChoiceId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Event")
	FText LabelText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Event")
	bool bAvailable = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Event")
	FName DisabledReason = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Event")
	bool bRequiresOwnedCardPayment = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Event")
	FName PaymentZoneId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Event")
	int32 PaymentCandidateCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Event")
	TArray<FGuid> PaymentCandidateInstanceIds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Event")
	FName PaymentDisabledReason = NAME_None;
};

/** 当前事件 UI/测试可读取的只读快照。 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FRunEventSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Event")
	FName PersistentId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Event")
	bool bIsActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Event")
	bool bCompleted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Event")
	FName EventId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Event")
	FName CurrentNodeId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Event")
	FText TitleText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Event")
	FText BodyText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Event")
	TArray<FRunEventChoiceSnapshot> Choices;
};

/** 单条 RunEvent 选项效果的实际执行结果，用于 UI/日志表现，不作为规则输入。 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FRunEventChoiceEffectResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Event")
	EWacomRunEventEffectType EffectType = EWacomRunEventEffectType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Event")
	TObjectPtr<UCardDefinition> CardDefinition = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Event")
	int32 Amount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Event")
	int32 ActualDelta = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Event")
	EWacomPressureType PressureType = EWacomPressureType::Count;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Event")
	bool bApplied = false;
};

/** RunEvent 选项提交结果。UI 只读该结果并展示，不根据它修改 RunState。 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FRunEventChoiceResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Event")
	bool bSucceeded = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Event")
	FName ChoiceId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Event")
	FName DisabledReason = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Event")
	FGuid PaidCardInstanceId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Event")
	TArray<FRunEventChoiceEffectResult> EffectResults;
};

/**
 * 一次冒险（Run）的持久状态。
 *
 * 行为约束：
 *   - 本结构是数据容器，不做业务逻辑；业务逻辑放 URunSession
 *   - URunSession 只读字段对外提供方法（GetXxx / AddPressure / RemoveFinger / ConsumeNode 等）
 *
 * 注意：FRunState 是内存数据层；磁盘格式见 UWacomSaveGame，两者之间做字段拷贝。
 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FRunState
{
	GENERATED_BODY()

	// ---- 玩家本体（手指 -> HP 上限）----

	/**
	 * 战外手指数量。
	 *
	 * 战内本体 HP 上限 = FingerCount × HpPerFinger。
	 * 战外失败：FingerCount = 0。
	 * 残疾压力同步：每缺 1 指 +5%（业务逻辑由 URunSession::RemoveFinger 处理）。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|HP")
	int32 FingerCount = 10;

	/** 每根手指对应的 HP（数值常量化，按角色配置）。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|HP")
	int32 HpPerFinger = 2;

	// ---- 压力系统 ----

	/** 八种压力值。详见 FPressureValues。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pressure")
	FPressureValues Pressure;

	/**
	 * 战内伤口阈值 1（CurrentHp/MaxHp 跨过时 +1% 伤口压力）。
	 * 当前默认 0.5。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pressure")
	float HighHpThreshold = 0.5f;

	/**
	 * 战内伤口阈值 2（CurrentHp/MaxHp 跨过时 +5% 伤口压力）。
	 * 当前默认 0.2。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pressure")
	float LowHpThreshold = 0.2f;

	// ---- 经验值与技能池 ----

	/** 累计经验值。满 ExperienceCapacity 自动入账并清零。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Skill")
	int32 ExperienceCurrent = 0;

	/** 经验值上限。当前默认 10。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Skill")
	int32 ExperienceCapacity = 10;

	/**
	 * 已获得的技能（按获得顺序）。
	 * 当前只占位，技能效果未实现。每次满经验入账一个 SkillSlot.Placeholder。
	 *
	 * 用 TArray 而不是 FGameplayTagContainer：
	 *   - Container 是 Set 语义，重复 AddTag 不增加 Num
	 *   - 占位期需要"重复累加"才能反映已得技能数
	 *   - 真技能上线（每种技能唯一）后可以平移到 GetUniqueSkillTags 投影
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Skill")
	TArray<FGameplayTag> AcquiredSkills;

	// ---- 压力辅助计数 ----

	/**
	 * 累计偷窃次数。OnTheftCommitted 调用时 ++ 后用于公式 n*(n+1)/2 + 1。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pressure")
	int32 TheftCount = 0;

	// ---- 经济：金币 ----

	/**
	 * 玩家持有的金币数量。
	 *
	 * 当前来源包括删牌区；用途包括商店购买和事件支付。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Economy")
	int32 Gold = 0;

	/** 当前正在访问的商店节点 ID。NAME_None 表示没有打开商店。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Shop")
	FName ActiveShopId = NAME_None;

	/** 当前商店访问内是否买过至少一件商品。关闭商店时据此消耗 1 节点。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Shop")
	bool bShopVisitHasPurchase = false;

	/** 当前正在访问的事件节点 ID。NAME_None 表示没有打开事件。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Event")
	FName ActiveRunEventId = NAME_None;

	/** 当前事件访问使用的事件定义。仅内存态引用。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Event")
	TObjectPtr<UWacomRunEventDefinition> ActiveRunEventDefinition = nullptr;

	// ---- 时间与昼夜 ----

	/** 当前 Run 的天数（从 1 开始）。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Time")
	int32 CurrentDayNumber = 1;

	/** 当前时段。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Time")
	ETimePhase CurrentTimePhase = ETimePhase::Morning;

	/** 当前时段剩余可用节点数（资源点）。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Time")
	int32 RemainingNodeCount = 2;

	/** 五时段初始节点数。可被技能 / 事件改。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Time")
	int32 InitialNodeCount_Morning = 2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Time")
	int32 InitialNodeCount_Day = 6;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Time")
	int32 InitialNodeCount_Dusk = 2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Time")
	int32 InitialNodeCount_Night = 2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Time")
	int32 InitialNodeCount_Sunrise = 1;

	// ---- 背包与备战卡组 ----

	/**
	 * 背包：永久卡牌池。
	 * 战斗结束所有战内卡回背包；事件 / 掉落进背包。
	 *
	 * 数组元素是 FCardInstance：同款 Definition 的多张卡可通过 InstanceId 区分，
	 * 并能独立放进备战 / 通量 / SpecialZone / 负重。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck")
	TArray<FCardInstance> Backpack;

	/**
	 * 备战卡组：从玩家拥有卡牌中选出，战斗实际读取的卡组。
	 * 不含左右手卡（左右手卡始终通过 Character 字段独立加载）。
	 * 容量由 URunSession::GetBattleDeckCapacity() 动态计算。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck")
	TArray<FCardInstance> BattleDeck;

	/**
	 * 负重区。
	 *
	 * 当通量 / 备战 / SpecialZone 都装不下时，溢出 instance 进入此处。
	 * RecomputeBurden 会先处理超容溢出，再按"通量 -> 备战 -> SpecialZones"优先序回填，
	 * 最后按 BurdenZone.Num() 写入负重压力。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck")
	TArray<FCardInstance> BurdenZone;

	/**
	 * 每张玩家拥有的 B 类容器卡 instance 各自开辟的特殊存放区集合。
	 *
	 * 不变量：
	 *   - 每条 entry 的 OwnerInstanceId 必须等于 Backpack ∪ BattleDeck 中某张 B 主卡 instance 的 InstanceId；
	 *   - 同一 OwnerInstanceId 在本数组中至多一条；
	 *   - SZ.Cards 与 Backpack / BattleDeck / BurdenZone 及其他 SpecialZone.Cards 互斥不重叠。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck")
	TArray<FSpecialZone> SpecialZones;

	// ---- 角色与场景进度 ----

	/** 玩家选择的角色。当前原型固定为 BugGirl。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run")
	TObjectPtr<UCharacterDefinition> Character = nullptr;

	/**
	 * 战斗随机种子。
	 * 0 表示每场战斗独立随机；非 0 时用于复现。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run")
	int32 BattleSeed = 0;

	/** 已击败的敌人 Definition 列表。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run")
	TArray<TObjectPtr<UEnemyDefinition>> DefeatedEnemies;

	/**
	 * 当前 Run 是否仍在进行。
	 * 战内失败：bRunActive = false。
	 * 战外失败（压力满 / 手指掉光）：bRunActive = false。
	 * URunSession::IsRunFailed() 提供综合判定。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run")
	bool bRunActive = true;

	/** 已被永久销毁的场景触发器 ID 列表。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run")
	TSet<FName> DestroyedTriggerIds;

	/**
	 * 战斗节点进度。
	 *
	 * Key = ABattleTriggerActor.PersistentId。Value = 该 Trigger 上次撤离时的破坏状态。
	 * 撤离写入 / 胜利清理 / 失败保留（Run 都结束了无意义保留与否）。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run")
	TMap<FName, FBattleProgressSnapshot> BattleProgress;

	/** 商店节点库存状态。Key = 场景商店/节点 PersistentId；当前只在 Run 内存态保留，不接 SaveGame。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Shop")
	TMap<FName, FRunShopState> ShopStates;

	/** 事件节点状态。Key = 场景事件 Actor 的 PersistentId；当前只在 Run 内存态保留，不接 SaveGame。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Event")
	TMap<FName, FRunEventState> RunEventStates;

	/** 玩家在探索地图的 Transform。仅当 bHasPlayerTransform == true 时有效。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run")
	FTransform PlayerTransform = FTransform::Identity;

	/** PlayerTransform 是否有效；新开 Run 时为 false。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run")
	bool bHasPlayerTransform = false;
};
