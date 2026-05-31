// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/WacomRunWorldClickableInteractable.h"
#include "Interaction/WacomWorldInteractable.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif
#include "WacomRunPickupActorBase.generated.h"

class AWacomPlayerController;
class UBoxComponent;
class USphereComponent;
class UStaticMeshComponent;
class UWacomInteractionTargetComponent;
class UWacomRunWorldInteractionTargetBridgeComponent;

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomRunPickupBaseDebugView
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	FString ActorName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	FName PersistentId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	bool bHasRunSession = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	bool bCanInteract = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	bool bIsCollected = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	float TriggerRadius = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	FVector ClickBoundsExtent = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	FName VisualName = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	bool bConfigValid = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	FName ConfigWarningReason = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	bool bDuplicatePersistentIdDetected = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	bool bHasRenderableVisual = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	bool bClickTargetConfigured = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	FName ClickTargetStableId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	FString HoverPrompt;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	FString CollectedHoverPrompt;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	FName LastDebugResult = NAME_None;
};

/**
 * Run world pickup 的共享交互壳。
 *
 * 子类只负责奖励配置、RunSession 结算和成功反馈；E 键、鼠标点击、hover、视觉 probe、
 * PersistentId 绑定、重复 ID 诊断和 collected lifecycle 都在这里统一维护。
 */
UCLASS(Abstract, Blueprintable)
class WACOMAPP_API AWacomRunPickupActorBase
	: public AActor
	, public IWacomWorldInteractable
	, public IWacomRunWorldClickableInteractable
{
	GENERATED_BODY()

public:
	AWacomRunPickupActorBase();

	/** 拾取物在当前 Run 内的唯一 ID。RunSession 使用它防止重复拾取；None 会拒绝拾取。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Pickup",
		meta = (ToolTip = "拾取物在当前 Run 内的唯一 ID。RunSession 使用它防止重复拾取；None 会拒绝拾取。"))
	FName PersistentId;

	/** 触发半径（cm）。玩家进入该范围后，探索期按 E 可以拾取。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Pickup",
		meta = (ToolTip = "玩家进入该半径后，探索期按 E 可以拾取。单位：厘米。建议范围 50-1000。",
			ClampMin = "50.0", UIMin = "50.0", UIMax = "1000.0"))
	float TriggerRadius = 160.f;

	/** 探索 HUD 上显示的 E 键拾取提示文本。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Pickup",
		meta = (ToolTip = "玩家处于拾取范围内时显示在探索 HUD 上的提示文本。"))
	FText InteractPromptText;

	/** 鼠标指向 ClickBounds 时探索 HUD 上显示的点击提示文本。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Pickup|Click",
		meta = (ToolTip = "鼠标指向拾取物点击命中体时显示的提示文本。只影响 hover 提示；点击后仍走 IWacomWorldInteractable。"))
	FText HoverPromptText;

	/** 已拾取但 Actor 仍可被 debug/probe 到时显示的弱点击提示文本。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Pickup|Click",
		meta = (ToolTip = "拾取物已在当前 Run 中被拾取，但 Actor 仍可被调试或 probe 到时显示的弱提示文本。"))
	FText CollectedHoverPromptText;

	/** 成功拾取后是否销毁 Actor。关闭时只会隐藏并禁用碰撞，便于调试。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Pickup",
		meta = (ToolTip = "成功拾取后是否销毁 Actor。关闭时只隐藏并禁用碰撞，便于调试。"))
	bool bDestroyWhenCollected = true;

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Pickup")
	USphereComponent* GetTriggerSphere() const { return TriggerSphere; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Pickup")
	UBoxComponent* GetClickBounds() const { return ClickBounds; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Pickup")
	UStaticMeshComponent* GetPickupVisual() const { return PickupVisual; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Pickup")
	UWacomInteractionTargetComponent* GetClickInteractionTargetComponent() const
	{
		return ClickInteractionTargetComponent;
	}

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Pickup")
	UWacomRunWorldInteractionTargetBridgeComponent* GetClickTargetBridgeComponent() const
	{
		return ClickTargetBridgeComponent;
	}

	/** 返回鼠标 hover 到 ClickBounds 时应显示的提示文本；已拾取时返回弱提示。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Pickup|Click",
		meta = (ToolTip = "返回鼠标 hover 到 ClickBounds 时应显示的提示文本；已拾取时返回弱提示。"))
	FText GetHoverPromptText(AWacomPlayerController* PC) const;

	/** 读取 Run Pickup 共享交互壳的只读诊断信息。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Pickup|Debug",
		meta = (ToolTip = "读取 Run Pickup 共享交互壳的配置、收集状态和点击目标绑定诊断；不会修改 RunState。"))
	FWacomRunPickupBaseDebugView GetRunPickupBaseDebugView(AWacomPlayerController* PC) const;

	/** 返回 Run Pickup 共享交互壳的一行诊断摘要。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Pickup|Debug",
		meta = (ToolTip = "返回 Run Pickup 共享交互壳的一行诊断摘要。"))
	FString GetRunPickupBaseDebugSummary(AWacomPlayerController* PC) const;

	/** 将 Run Pickup 共享交互壳诊断摘要写入日志。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Pickup|Debug",
		meta = (ToolTip = "将 Run Pickup 共享交互壳诊断摘要写入日志。"))
	void LogRunPickupBaseDebugSummary(AWacomPlayerController* PC) const;

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

	void RefreshClickTargetBinding();
	void RefreshClickTargetBindingAndRuntimeTarget();
	void RefreshPickupAuthoringState();
	void ApplyDebugPickupAuthoringDefaults(
		FName InPersistentId,
		float InTriggerRadius = 160.f,
		bool bInDestroyWhenCollected = true);
	FName BuildConfigWarningReason() const;
	bool HasDuplicatePersistentIdInWorld() const;
	bool IsCollectedFor(AWacomPlayerController* PC) const;
	void ApplyCollectedLifecycle(AWacomPlayerController* PC);

	static FName BuildDebugPersistentIdFromActorName(
		const FString& ActorName,
		const TCHAR* Prefix,
		const TCHAR* EmptyFallback);

	virtual FName GetRewardConfigWarningReason() const { return NAME_None; }
	virtual bool TryCollectPickupReward(AWacomPlayerController* PC) PURE_VIRTUAL(
		AWacomRunPickupActorBase::TryCollectPickupReward, return false;);
	virtual FText GetDefaultInteractPromptText() const;
	virtual FText GetDefaultHoverPromptText() const;
	virtual FText GetDefaultCollectedHoverPromptText() const;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> TriggerSphere = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Click",
		meta = (AllowPrivateAccess = "true", ToolTip = "鼠标点击命中体。只用于 Visibility trace，不产生 overlap；点击后仍走 IWacomWorldInteractable。"))
	TObjectPtr<UBoxComponent> ClickBounds = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup",
		meta = (AllowPrivateAccess = "true", ToolTip = "拾取物的 C++ 占位可见物。正式美术可在 Blueprint 或子类里替换。"))
	TObjectPtr<UStaticMeshComponent> PickupVisual = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Click",
		meta = (AllowPrivateAccess = "true", ToolTip = "拾取物默认携带的通用交互目标身份组件。"))
	TObjectPtr<UWacomInteractionTargetComponent> ClickInteractionTargetComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Click",
		meta = (AllowPrivateAccess = "true", ToolTip = "把拾取物标记为 Run World Target，供鼠标 probe 和点击桥接识别。"))
	TObjectPtr<UWacomRunWorldInteractionTargetBridgeComponent> ClickTargetBridgeComponent = nullptr;
};
