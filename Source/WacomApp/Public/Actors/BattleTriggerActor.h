// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BattleTriggerActor.generated.h"

class USphereComponent;
class UEnemyDefinition;

/**
 * 场景中的战斗触发器。
 *
 * Details 面板配置一个 UEnemyDefinition。玩家 Pawn 走进 Sphere 范围
 * 即触发 AWacomPlayerController::RequestEnterBattle。
 *
 * R3：只做 Overlap 转发 + 日志，真正的进入战斗逻辑（UI / Session / IMC 切换）在 R4。
 * 战斗结束后由 AWacomGameMode::ExitBattle 负责 Destroy 本触发器（避免重复战斗）。
 */
UCLASS(Blueprintable)
class WACOMAPP_API ABattleTriggerActor : public AActor
{
	GENERATED_BODY()

public:
	ABattleTriggerActor();

	/** 本触发器对应的敌人配置。必填。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle")
	TObjectPtr<UEnemyDefinition> EnemyDef = nullptr;

	/** 触发半径（cm）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle",
		meta = (ClampMin = "50.0", UIMin = "50.0"))
	float TriggerRadius = 200.f;

	/** 一次性触发：Overlap 之后禁用，避免在过渡期间再次触发。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle")
	bool bConsumeOnTrigger = true;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle")
	USphereComponent* GetTriggerSphere() const { return TriggerSphere; }

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> TriggerSphere = nullptr;

	/** 已触发标记；防止单帧多次 Overlap 重入。 */
	bool bTriggered = false;
};
