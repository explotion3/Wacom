// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "Types/WacomEnums.h"
#include "Types/WacomResult.h"
#include "Commands/BattleCommand.h"
#include "Events/BattleEvent.h"
#include "Presentation/BattlePresentationJournal.h"
#include "Resolution/BattleTargetValidationResult.h"
#include "Resolution/BattleCardActionPreview.h"
#include "Resolution/BattleCardTargetPreview.h"
#include "Runtime/BattlePartSlotIdentity.h"
#include "Session/BattleInitializationResult.h"
#include "Session/BattleResultPacket.h"
#include "Session/BattleResolution.h"
#include "Snapshots/BattleSnapshot.h"
#include "Snapshots/BattlePileInspectionSnapshot.h"
#include "BattleSession.generated.h"

class UCharacterDefinition;
class UEnemyDefinition;
class UCardDefinition;
struct FWacomInteractionTargetHandle;

/**
 * 一张参战卡的入战清单条目。
 *
 * URunSession::BuildInitParamsForBattle 在战斗启动前填上备战区原生 instances
 * 与各 SpecialZone 中 bBattleEnabledInSpecialZone == true 的 instances；
 * BattleSession 在 Initialize 时按 entry 创建 FRuntimeCardInstance，把
 * CapacityEffectTags 拷入 RuntimeCardInstance，供 Effect Semantics 的
 * Damage magnitude 路径叠加修正（如蛛茧绒囊给武器卡 +3）。
 *
 * - 来自备战区原生位置：CapacityEffectTags = 空集合
 * - 来自 SpecialZone 的卡：CapacityEffectTags = { 主卡 Definition.Physique.CapacityEffect }
 */
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FBattleDeckEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle")
	TObjectPtr<const UCardDefinition> Definition = nullptr;

	/** 来自 SpecialZone 的卡：单元素集合 = 主卡 CapacityEffect。来自备战区原生位置：空集合。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle")
	FGameplayTagContainer CapacityEffectTags;
};

struct FBattleState;
struct FBattleEventBus;
#if WITH_AUTOMATION_TESTS
struct FWacomBattleSessionTestAccess;
#endif

/**
 * Encounter 内的单个敌人槽。
 */
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FBattleEnemySlotInit
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Encounter")
	FName EnemySlotId = TEXT("Enemy");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Encounter")
	TObjectPtr<const UEnemyDefinition> Enemy = nullptr;
};

/**
 * 击倒事件单个选项的只读展示/校验视图。
 *
 * DisabledReason 约定：
 * - None：可用或无禁用原因。
 * - NoLivingEnemyPart：撤离不可用，因为敌人已经没有存活部位。
 * - LeftHandMissing / RightHandMissing：预留，未来角色永久失去对应手牌时使用。
 */
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FKnockdownChoiceOptionView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Knockdown")
	EKnockdownChoice Choice = EKnockdownChoice::None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Knockdown")
	bool bAvailable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Knockdown")
	FName DisabledReason = NAME_None;

	/** 所选分支是否配置了可授予的奖励卡。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Knockdown")
	bool bHasRewardCard = false;

	/** 奖励卡稳定 ID；无奖励时为空。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Knockdown")
	FName RewardCardId = NAME_None;

	/** 奖励卡显示名；为空时 Battle 层优先回退到 RewardCardId。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Knockdown")
	FText RewardCardName;
};

/**
 * 当前待处理击倒事件的只读视图。
 *
 * UI 通过 UBattleSession::BuildPendingKnockdownChoiceView() 读取本结构，
 * 不再解析 FBattleEvent.Count 的临时位掩码。
 */
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FKnockdownChoiceView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Knockdown")
	bool bHasPendingChoice = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Knockdown")
	FGuid PartInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Knockdown")
	FName PartId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Knockdown")
	FBattlePartSlotIdentity Identity;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Knockdown")
	FText PartName;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Knockdown")
	FKnockdownChoiceOptionView AidOption;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Knockdown")
	FKnockdownChoiceOptionView WithdrawOption;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Knockdown")
	FKnockdownChoiceOptionView DestroyOption;
};

/**
 * 战斗初始化参数。
 *
 * Character 和 EnemySlots 中的 Enemy 都是 DataAsset。RandomSeed 为 0 时使用基于时间的 seed。
 * 测试可注入固定 seed 以得到可复现序列。
 *
 * 备战卡组来源（按优先级从高到低）：
 *   1) BattleDeckEntries 非空 → 使用 entries（含 CapacityEffectTags，RunSession 路径）
 *   2) 都为空 → 回退到 Character->StarterDeck（direct fixture / debug fallback）
 *
 * 左右手卡始终从 Character 加载，不通过 Override / Entries。
 */
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FBattleInitParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle")
	TObjectPtr<const UCharacterDefinition> Character = nullptr;

	/** Encounter 稳定 ID。Run 路径默认使用 Trigger PersistentId；空时回退为 Encounter。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Encounter")
	FName EncounterId = TEXT("Encounter");

	/** 敌人入口。按数组顺序创建敌人槽；必须至少有一个有效 Enemy。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Encounter")
	TArray<FBattleEnemySlotInit> EnemySlots;

	/** 0 表示基于时间。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle")
	int32 RandomSeed = 0;

	/**
	 * 备战卡组入战清单。
	 *
	 * 由 URunSession::BuildInitParamsForBattle 填充，包含：
	 *   - RunState.BattleDeck 中所有原生 instances（CapacityEffectTags 为空）
	 *   - 各 SpecialZone（其 OwnerInstanceId 当前位于 BattleDeck）中
	 *     bBattleEnabledInSpecialZone == true 的 instances
	 *     （CapacityEffectTags = { 主卡 Definition.Physique.CapacityEffect } 单元素集合）
	 *
	 * BattleSession::Initialize 在 BattleDeckEntries.Num() > 0 时优先按 entry 创建
	 * RuntimeCardInstance，把 entry 的 CapacityEffectTags 拷入 RuntimeCardInstance；
	 * Effect Semantics 的 Damage magnitude 路径读取 RuntimeCardInstance.CapacityEffectTags
	 * 决定是否叠加修正（如 Card.CapacityEffect.WeaponDamagePlus3 给武器卡 +3）。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle")
	TArray<FBattleDeckEntry> BattleDeckEntries;

	/**
	 * 战内伤口阈值。BattleSession 在玩家 HP 变更后维护跨越 flag。
	 * 默认值与 FRunState 一致（0.5 / 0.2）。RunSession::BuildInitParamsForBattle 灌入。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle")
	float HighHpThreshold = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle")
	float LowHpThreshold = 0.2f;

	/**
	 * 预先破坏的完整部位身份列表。Run 路径只填该字段。
	 *
	 * Initialize 时按此 list 把对应 RuntimeEnemyPart 设为 bDestroyed=true / HP=0 /
	 * Initiative=0。**不入 PendingKnockdownEvents 队列**（已经记过账了），
	 * **不发 KnockdownExpGain**（避免反复撤离刷经验）。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Encounter")
	TArray<FBattlePartSlotIdentity> PreDestroyedParts;
};

/**
 * 一场战斗的对外入口。
 *
 * 唯一职责：
 * - 持有 FBattleState（战斗真相）
 * - 持有 FBattleEventBus（事件 Sequence）
 * - 对外暴露 Initialize / ResolveCommand / BuildSnapshot
 *
 * UBattleSession 不写任何规则。规则在 Resolver / Executor / Service。
 *
 * GC 约束：FBattleState 是非反射结构，其中的 TObjectPtr 不会被 GC 追踪。
 * Session 在 Initialize 时把关键 UObject 引用（Character/EnemySlots/Parts/Cards 的 Def）
 * 镜像到 UPROPERTY 容器 ReferencedAssets，保证引用在 Session 存活期间不被 GC。
 */
UCLASS(BlueprintType)
class WACOMBATTLE_API UBattleSession : public UObject
{
	GENERATED_BODY()

public:
	UBattleSession();
	virtual ~UBattleSession() override;

	/**
	 * 原子初始化一场战斗。成功后 Phase 推进到 PlayerAction；失败时保留当前旧战斗。
	 * 初始化事件与命令后快照由返回值一次性交给调用方，不保存在 Session 消费队列中。
	 */
	FBattleInitializationResult Initialize(const FBattleInitParams& Params);

	/**
	 * 提交命令并原子返回该命令的事件、表现 checkpoint 与命令后快照。
	 * 这是唯一命令入口。
	 */
	FBattleResolution ResolveCommand(const FBattleCommand& Command);

	/** 构建当前战斗的只读快照。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle")
	FBattleSnapshot BuildSnapshot() const;

	/**
	 * 按需构建牌堆详情使用的只读快照。Draw 区不会暴露真实抽取顺序。
	 * 该接口不修改规则状态、事件序号或随机源。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Pile Inspection")
	FBattlePileInspectionSnapshot BuildPileInspectionSnapshot() const;

	/** 战斗是否已结束（Phase == BattleEnd）。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Battle")
	bool IsBattleEnded() const;

	/** 当前阶段，只读。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Battle")
	EBattlePhase GetPhase() const;

	/** 当前待处理击倒事件的可用性视图。没有待处理事件时 bHasPendingChoice=false。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Knockdown")
	FKnockdownChoiceView BuildPendingKnockdownChoiceView() const;

	/**
	 * 构造战斗结束时传给 Run 层的"战后包"。
	 *
	 * 调用约束：
	 *   - 仅在 Phase == BattleEnd 时调用。其他阶段返回的 packet 字段值未定义。
	 *   - 调用者应当在 Session 释放前完成；之后 Session 内部状态不再可用。
	 *
	 * 字段填充：
	 *   - Outcome：取自 BattleState.Outcome
	 *   - bCrossedHighHpThreshold / bCrossedLowHpThreshold：取自 BattleState 的同名 flag
	 *   - bMutualDestruction：取自 BattleState.bMutualDestruction（CheckAndApplyBattleEnd 维护）
	 */
	FBattleResultPacket BuildResultPacket() const;

	/**
	 * 判断给定的卡牌实例能否作用到给定目标，并返回可用于 UI/调试解释的拒绝原因。
	 *
	 * 只判断结构性兼容（TargetMode ↔ TargetKind 匹配）和目标基础资格，不校验费用、状态或回合阶段。
	 * 拖拽系统在 drop 前调用本函数做预览，提交 PlayCard 后 Resolver 做最终校验。
	 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Battle")
	FWacomBattleTargetValidationResult ValidateTargetWithCard(
		const FGuid& CardInstanceId,
		const FWacomInteractionTargetHandle& Target) const;

	/**
	 * 构造一张手牌针对候选目标的只读效果预览。
	 *
	 * 预览不提交命令、不发事件、不修改 BattleState。UI 只能消费返回的 facts，
	 * 不应自行重算 Magnitude、Condition 或目标筛选规则。
	 */
	FBattleCardTargetPreview BuildCardTargetPreview(
		const FGuid& CardInstanceId,
		const FWacomInteractionTargetHandle& Target) const;

	/**
	 * 构造一张手牌松手后的确定数值预览。
	 *
	 * 仅在目标合法且命令可提交语义成立时生成 projected values。随机或未决效果不
	 * 展开到净结果，只记录 debug 标记；UI 只能消费返回 facts，不应重算规则。
	 */
	FBattleCardActionPreview BuildCardActionPreview(
		const FGuid& CardInstanceId,
		const FWacomInteractionTargetHandle& Target) const;

private:
#if WITH_AUTOMATION_TESTS
	friend struct FWacomBattleSessionTestAccess;
	bool ValidateCardZoneInvariantsForAutomationTest(FString& OutError) const;
	int32 GetReferencedAssetCountForAutomationTest() const;
	bool ContainsReferencedAssetForAutomationTest(const UObject* Asset) const;
	int32 GetNextEventSequenceForAutomationTest() const;
	int32 GetRandomCurrentSeedForAutomationTest() const;
#endif

	/** 持有 FBattleState 和 FBattleEventBus。裸指针 + 手动管理，避免 TUniquePtr 在 UHT gen.cpp 里需要完整定义。 */
	FBattleState* State = nullptr;
	FBattleEventBus* EventBus = nullptr;

	/** 让 GC 追踪 Session 生命周期内引用到的资产。 */
	UPROPERTY()
	TArray<TObjectPtr<const UObject>> ReferencedAssets;
};
