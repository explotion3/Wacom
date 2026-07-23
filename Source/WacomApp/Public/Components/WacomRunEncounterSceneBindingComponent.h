// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Session/BattleSession.h"
#include "Types/WacomResult.h"
#include "WacomRunEncounterSceneBindingComponent.generated.h"

class UEncounterDefinition;
class AWacomBattleEnemyActor;
class AWacomFirstPersonViewpointActor;
struct FWacomFirstPersonViewStageRequest;

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattleSceneEnemyHostSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy",
		meta = (ToolTip = "Encounter 内敌人槽位 ID。必须与 Floor Node 的 EncounterDefinition.EnemySlots[].EnemySlotId 精确对应。"))
	FName EnemySlotId = TEXT("Enemy");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy",
		meta = (ToolTip = "该敌人槽对应的场景 Enemy Host。Host 仍负责部位命中、描边、Badge 与战斗表现。"))
	TObjectPtr<AWacomBattleEnemyActor> SceneEnemyHost = nullptr;
};

/**
 * Encounter Node Anchor 的场景绑定。
 *
 * 静态规则只来自 Floor Node payload；本组件只保存场景 Host 与可选战斗镜头引用。
 * NodeId 来自 Owner AWacomRunMapNodeAnchorActor，不在组件内复制。
 */
UCLASS(ClassGroup = (Wacom), BlueprintType, Blueprintable,
	meta = (BlueprintSpawnableComponent, DisplayName = "Wacom Run Encounter Scene Binding"))
class WACOMAPP_API UWacomRunEncounterSceneBindingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWacomRunEncounterSceneBindingComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Encounter",
		meta = (ToolTip = "Encounter 场景 Host 映射。必须完整且唯一覆盖 Floor Node EncounterDefinition 的全部有效 EnemySlotId。"))
	TArray<FWacomBattleSceneEnemyHostSlot> SceneEnemyHostSlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Encounter|Camera",
		meta = (ToolTip = "进入该 Encounter 战斗时使用的可选第一人称镜头站位。Actor Transform 表示摄像机 View Pose；未配置时保持当前探索位置。"))
	TObjectPtr<AWacomFirstPersonViewpointActor> BattleEntryViewpoint = nullptr;

	/** 只读验证 Owner、规则槽与场景 Host 的完整一一映射。 */
	FWacomStatus ValidateForEncounter(const UEncounterDefinition& EncounterDefinition) const;

	/** 将 Floor Node EncounterDefinition 转成 Battle init 敌人槽。 */
	void BuildBattleEnemySlots(
		const UEncounterDefinition& EncounterDefinition,
		TArray<FBattleEnemySlotInit>& OutEnemySlots) const;

	/** 按规则槽顺序返回场景 Host，并临时写入 Host.EnemySlotId。 */
	void BuildBattleSceneEnemyHosts(
		const UEncounterDefinition& EncounterDefinition,
		TArray<AWacomBattleEnemyActor*>& OutSceneEnemyHosts) const;

	/** 构造可选 Battle Entry 第一人称镜头请求。 */
	bool TryBuildBattleEntryViewStageRequest(FWacomFirstPersonViewStageRequest& OutRequest) const;

	/** 真胜利结算成功后立即进入待退役态；Anchor 保留。 */
	void BeginResolvedEncounterSceneRetirement();

	/** 双 barrier 完成后退役本 Encounter 的 Host；Anchor 与组件保留。 */
	void CompleteResolvedEncounterSceneRetirement(const UEncounterDefinition& EncounterDefinition);

	bool IsRetirementPending() const { return bResolvedSceneRetirementPending; }
	bool IsRetirementCompleted() const { return bResolvedSceneRetirementCompleted; }

private:
	bool bResolvedSceneRetirementPending = false;
	bool bResolvedSceneRetirementCompleted = false;
};
