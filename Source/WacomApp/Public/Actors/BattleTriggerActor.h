// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/WacomWorldInteractable.h"
#include "BattleTriggerActor.generated.h"

class USphereComponent;
class UEnemyDefinition;

/**
 * 场景中的战斗触发器。
 *
 * 交互模型（Stage 7 之后）：use-key 模型。
 *   - Sphere 范围只用来判定"玩家是否在交互范围"
 *   - 进入范围 → 注册到 PlayerController.CandidateInteractables + ExplorationHUD 显示"按 E 战斗"
 *   - 离开范围 → 从 CandidateInteractables 移除
 *   - 玩家按 IA_Interact（E）→ PC 从候选列表挑最近的 interactable → 进战斗
 *
 * 旧模型（overlap 自动触发）已废弃，原因是撤离回探索后玩家仍在 Sphere 内，
 * 永远不会有 EndOverlap → BeginOverlap 的循环，无法重入战斗。
 */
UCLASS(Blueprintable)
class WACOMAPP_API ABattleTriggerActor : public AActor, public IWacomWorldInteractable
{
	GENERATED_BODY()

public:
	ABattleTriggerActor();

	/**
	 * 持久化 ID。存档层面唯一标识本触发器。
	 *
	 * - 关卡级别必须唯一；同 id 重复时 BeginPlay 会报错
	 * - 置空（NAME_None）视为"不参与存档"，BeginPlay 打 Warning
	 * - 真胜利时 RunSession 会把本 id 加入 DestroyedTriggerIds，下次关卡加载本 Actor 立即 Destroy
	 * - 撤离时不进 DestroyedTriggerIds，但 BattleProgress 会持久化已破坏部位
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Persistence")
	FName PersistentId;

	/** 本触发器对应的敌人配置。必填。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle")
	TObjectPtr<UEnemyDefinition> EnemyDef = nullptr;

	/** 触发半径（cm）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle",
		meta = (ToolTip = "玩家进入该半径后，探索期按 E 可以触发本战斗节点。单位：厘米。建议范围 50-1000。",
			ClampMin = "50.0", UIMin = "50.0", UIMax = "1000.0"))
	float TriggerRadius = 200.f;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle")
	USphereComponent* GetTriggerSphere() const { return TriggerSphere; }

	/**
	 * 玩家按 E 时由 PlayerController 调用。
	 * 转发到 PC->RequestEnterBattle。Trigger 自身不直接调 GameMode。
	 */
	void TryActivate(class AWacomPlayerController* PC);

	// ---- IWacomWorldInteractable ----
	virtual FText GetInteractPromptText_Implementation(AWacomPlayerController* PC) const override;
	virtual FVector GetInteractLocation_Implementation(AWacomPlayerController* PC) const override;
	virtual bool CanInteract_Implementation(AWacomPlayerController* PC) const override;
	virtual bool TryInteract_Implementation(AWacomPlayerController* PC) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> TriggerSphere = nullptr;
};
