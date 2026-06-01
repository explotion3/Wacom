// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif
#include "WacomBattleEnemyActor.generated.h"

class AWacomBattleEnemyPartActor;
class UEnemyDefinition;
class USceneComponent;

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattleSceneEnemyDebugView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FString ActorName;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FName EnemyDefinitionName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FName EnemyId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 AttachedPartActorCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	TArray<FName> AttachedPartIds;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	TArray<FName> UnknownPartIds;
};

/**
 * Battle 场景敌人 Host。
 *
 * Host 只负责分组、debug 和制作校验；实际命中和表现由子级
 * AWacomBattleEnemyPartActor 负责。当前 BattleSession 仍是单敌人规则层。
 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomBattleEnemyActor : public AActor
{
	GENERATED_BODY()

public:
	AWacomBattleEnemyActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy",
		meta = (ToolTip = "可选敌人定义。当前只用于制作校验和 debug，不作为运行时多敌人选择器。"))
	TObjectPtr<UEnemyDefinition> EnemyDefinition = nullptr;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy",
		meta = (ToolTip = "返回附着在当前 Host 下的战斗敌人部位 Actor。"))
	TArray<AWacomBattleEnemyPartActor*> GetAttachedBattleEnemyPartActors() const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Scene Enemy|Authoring",
		meta = (ToolTip = "刷新附着在当前 Host 下的部位 Actor facade。不会自动生成子 Actor。"))
	void RefreshAttachedPartAuthoringState() const;

	UFUNCTION(CallInEditor, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "绑定 Debug 蛇敌人定义。不会自动生成 Head/Body/Tail 部位 Actor。"))
	void ConfigureDebugSnakeHostSample();

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "读取战斗场景敌人 Host 的只读诊断信息。"))
	FWacomBattleSceneEnemyDebugView GetBattleSceneEnemyDebugView() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "返回战斗场景敌人 Host 的一行诊断摘要。"))
	FString GetBattleSceneEnemyDebugSummary() const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "将战斗场景敌人 Host 的诊断摘要写入日志。"))
	void LogBattleSceneEnemyDebugSummary() const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

private:
	TSet<FName> BuildDefinitionPartIdSet() const;
	TArray<FName> BuildUnknownAttachedPartIds() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy",
		meta = (AllowPrivateAccess = "true", ToolTip = "Host 根节点。部位 Actor 可附着到本 Actor 下进行分组。"))
	TObjectPtr<USceneComponent> SceneRoot = nullptr;
};
