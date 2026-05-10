// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Types/WacomResult.h"
#include "Commands/BattleCommand.h"
#include "Events/BattleEvent.h"
#include "Snapshots/BattleSnapshot.h"
#include "BattleSession.generated.h"

class UCharacterDefinition;
class UEnemyDefinition;

struct FBattleState;
struct FBattleEventBus;

/**
 * 战斗初始化参数。
 *
 * Character 和 Enemy 都是 DataAsset。RandomSeed 为 0 时使用基于时间的 seed。
 * 测试可注入固定 seed 以得到可复现序列。
 */
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FBattleInitParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle")
	TObjectPtr<const UCharacterDefinition> Character = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle")
	TObjectPtr<const UEnemyDefinition> Enemy = nullptr;

	/** 0 表示基于时间。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle")
	int32 RandomSeed = 0;
};

/**
 * 一场战斗的对外入口。
 *
 * 唯一职责：
 * - 持有 FBattleState（战斗真相）
 * - 持有 FBattleEventBus（事件流）
 * - 对外暴露 SubmitCommand / BuildSnapshot / ConsumeEvents
 *
 * UBattleSession 不写任何规则。规则在 Resolver / Executor / Service。
 *
 * GC 约束：FBattleState 是非反射结构，其中的 TObjectPtr 不会被 GC 追踪。
 * Session 在 Initialize 时把关键 UObject 引用（Character/Enemy/Parts/Cards 的 Def）
 * 镜像到 UPROPERTY 容器 ReferencedAssets，保证引用在 Session 存活期间不被 GC。
 */
UCLASS(BlueprintType)
class WACOMBATTLE_API UBattleSession : public UObject
{
	GENERATED_BODY()

public:
	UBattleSession();
	virtual ~UBattleSession() override;

	/** 初始化一场战斗。成功后 Phase 推进到 PlayerAction，等待 SubmitCommand。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle")
	FWacomStatus Initialize(const FBattleInitParams& Params);

	/**
	 * 提交一条命令。
	 * 命令非法时 BattleState 不变，返回 Fail。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle")
	FWacomStatus SubmitCommand(const FBattleCommand& Command);

	/** 构建当前战斗的只读快照。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle")
	FBattleSnapshot BuildSnapshot() const;

	/** 取走自上次调用以来累积的事件并清空。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle")
	TArray<FBattleEvent> ConsumeEvents();

	/** 战斗是否已结束（Phase == BattleEnd）。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Battle")
	bool IsBattleEnded() const;

	/** 当前阶段，只读。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Battle")
	EBattlePhase GetPhase() const;

private:
	/** 持有 FBattleState 和 FBattleEventBus。裸指针 + 手动管理，避免 TUniquePtr 在 UHT gen.cpp 里需要完整定义。 */
	FBattleState* State = nullptr;
	FBattleEventBus* EventBus = nullptr;

	/** 让 GC 追踪 Session 生命周期内引用到的资产。 */
	UPROPERTY()
	TArray<TObjectPtr<const UObject>> ReferencedAssets;
};
