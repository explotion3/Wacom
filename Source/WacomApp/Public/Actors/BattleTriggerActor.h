// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/WacomRunWorldClickableInteractable.h"
#include "Interaction/WacomWorldInteractable.h"
#include "Misc/DataValidation.h"
#include "BattleTriggerActor.generated.h"

class USphereComponent;
class UBoxComponent;
class UEnemyDefinition;
class UWacomInteractionTargetComponent;
class UWacomRunWorldInteractionTargetBridgeComponent;

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattleTriggerDebugView
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Debug")
	FString ActorName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Debug")
	FName PersistentId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Debug")
	FString EnemyDefinitionName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Debug")
	bool bCanInteract = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Debug")
	bool bIsDestroyed = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Debug")
	bool bClickTargetConfigured = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Debug")
	FName ClickTargetStableId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Debug")
	FString HoverPrompt;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Debug")
	FString DestroyedHoverPrompt;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Debug")
	FName LastDebugResult = NAME_None;
};

/**
 * 场景中的战斗触发器。
 *
 * 交互模型：use-key 模型。
 *   - Sphere 范围只用来判定"玩家是否在交互范围"
 *   - 进入范围 → 注册到 PlayerController.CandidateInteractables + ExplorationHUD 显示"按 E 战斗"
 *   - 离开范围 → 从 CandidateInteractables 移除
 *   - 玩家按 IA_Interact（E）→ PC 从候选列表挑最近的 interactable → 进战斗
 *
 * overlap 自动触发已废弃，原因是撤离回探索后玩家仍在 Sphere 内，
 * 永远不会有 EndOverlap → BeginOverlap 的循环，无法重入战斗。
 */
UCLASS(Blueprintable)
class WACOMAPP_API ABattleTriggerActor : public AActor, public IWacomWorldInteractable, public IWacomRunWorldClickableInteractable
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

	/** 鼠标指向 ClickBounds 时探索 HUD 上显示的点击提示文本。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Click",
		meta = (ToolTip = "鼠标指向战斗触发器点击命中体时显示的提示文本。只影响 hover 提示；点击后仍走 IWacomWorldInteractable。"))
	FText HoverPromptText;

	/** 已销毁战斗触发器仍能被调试或 probe 到时显示的弱点击提示文本。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Click",
		meta = (ToolTip = "战斗已在当前 Run 中结束，但 Actor 仍能被调试或 probe 到时显示的弱提示文本。"))
	FText DestroyedHoverPromptText;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle")
	USphereComponent* GetTriggerSphere() const { return TriggerSphere; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle")
	UBoxComponent* GetClickBounds() const { return ClickBounds; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle")
	UWacomInteractionTargetComponent* GetClickInteractionTargetComponent() const
	{
		return ClickInteractionTargetComponent;
	}

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle")
	UWacomRunWorldInteractionTargetBridgeComponent* GetClickTargetBridgeComponent() const
	{
		return ClickTargetBridgeComponent;
	}

	/**
	 * 玩家按 E 时由 PlayerController 调用。
	 * 转发到 PC->RequestEnterBattle。Trigger 自身不直接调 GameMode。
	 */
	void TryActivate(class AWacomPlayerController* PC);

	/** 返回鼠标 hover 到 ClickBounds 时应显示的提示文本；已销毁触发器返回弱提示。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Click",
		meta = (ToolTip = "返回鼠标 hover 到 ClickBounds 时应显示的提示文本；已销毁触发器返回弱提示。"))
	FText GetHoverPromptText(AWacomPlayerController* PC) const;

	/** 读取当前战斗触发器配置和点击目标绑定的只读诊断信息。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Debug",
		meta = (ToolTip = "读取当前战斗触发器配置、销毁状态和点击目标绑定的只读诊断信息；不会修改 RunState。"))
	FWacomBattleTriggerDebugView GetBattleTriggerDebugView(AWacomPlayerController* PC) const;

	/** 返回适合复制到日志或 PIE Details 面板查看的一行诊断摘要。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Debug",
		meta = (ToolTip = "返回适合复制到日志或 PIE Details 面板查看的一行战斗触发器诊断摘要。"))
	FString GetBattleTriggerDebugSummary(AWacomPlayerController* PC) const;

	/** 将当前战斗触发器诊断摘要写入日志。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Debug",
		meta = (ToolTip = "将当前战斗触发器诊断摘要写入日志，便于 PIE 排查战斗配置和点击目标绑定。"))
	void LogBattleTriggerDebugSummary(AWacomPlayerController* PC) const;

	// ---- IWacomWorldInteractable ----
	virtual FText GetInteractPromptText_Implementation(AWacomPlayerController* PC) const override;
	virtual FVector GetInteractLocation_Implementation(AWacomPlayerController* PC) const override;
	virtual bool CanInteract_Implementation(AWacomPlayerController* PC) const override;
	virtual bool TryInteract_Implementation(AWacomPlayerController* PC) override;

	// ---- IWacomRunWorldClickableInteractable ----
	virtual FText GetRunWorldClickHoverPrompt_Implementation(AWacomPlayerController* PC) const override;
	virtual FWacomRunWorldClickableInteractableDebugView GetRunWorldClickableDebugView_Implementation(
		AWacomPlayerController* PC) const override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
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
	void RefreshClickTargetBinding();
	bool HasDuplicatePersistentIdInWorld() const;
	bool IsDestroyedFor(AWacomPlayerController* PC) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> TriggerSphere = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Click",
		meta = (AllowPrivateAccess = "true", ToolTip = "鼠标点击命中体。只用于 Visibility trace，不产生 overlap；点击后仍走 IWacomWorldInteractable。"))
	TObjectPtr<UBoxComponent> ClickBounds = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Click",
		meta = (AllowPrivateAccess = "true", ToolTip = "BattleTrigger 默认携带的通用交互目标身份组件。"))
	TObjectPtr<UWacomInteractionTargetComponent> ClickInteractionTargetComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Click",
		meta = (AllowPrivateAccess = "true", ToolTip = "把 BattleTrigger 标记为 Run World Target，供鼠标 probe 和点击桥接识别。"))
	TObjectPtr<UWacomRunWorldInteractionTargetBridgeComponent> ClickTargetBridgeComponent = nullptr;
};
