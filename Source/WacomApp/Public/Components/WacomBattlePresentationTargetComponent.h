// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "Events/BattleEvent.h"
#include "TimerManager.h"
#include "WacomBattlePresentationTargetComponent.generated.h"

class UBattleHUD;
class UPrimitiveComponent;

#if WITH_AUTOMATION_TESTS
class UWacomBattlePresentationTargetComponentProbe;
#endif

/**
 * Minimal scene-side target provider for BattleHUD presentation TargetCue.
 *
 * V0 owns no battle rules and performs no target selection. It only registers
 * an Actor/Component as a presentation target for an enemy part instance id.
 */
UCLASS(ClassGroup = (Wacom), meta = (BlueprintSpawnableComponent))
class WACOMAPP_API UWacomBattlePresentationTargetComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWacomBattlePresentationTargetComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Presentation", meta = (ToolTip = "Stable enemy part id from UEnemyPartDefinition::PartId, for prototype auto-binding to the current battle snapshot. Empty ids are ignored by auto-binding."))
	FName PartId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Presentation", meta = (ToolTip = "Enemy part instance id this scene object represents for BattleHUD presentation TargetCue routing. Empty ids are ignored."))
	FGuid PartInstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Presentation|Visual", meta = (ToolTip = "Optional primitive component that receives V0 scene target visual feedback. If unset, the component uses the owning actor's first primitive component."))
	TObjectPtr<UPrimitiveComponent> VisualTargetComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Presentation|Click", meta = (ToolTip = "Optional primitive component that receives PIE collision click events for this scene target. If unset, the component uses VisualTargetComponent, then the owning actor's first primitive component."))
	TObjectPtr<UPrimitiveComponent> ClickTargetComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Presentation|Click", meta = (ToolTip = "Enables the V0-B PIE collision click bridge. Clicks only forward target intent to BattleHUD; they do not submit battle commands directly."))
	bool bEnablePIECollisionClick = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Presentation|Click", meta = (ToolTip = "When enabled, registration temporarily configures the click target primitive to be queryable and block the Visibility trace channel, then restores its previous settings on unregister."))
	bool bConfigurePIEClickCollision = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Presentation|Visual", meta = (ToolTip = "Enables the V0 scale pulse when this scene target receives DamageDealt or EnemyPartHpEmptied presentation cues."))
	bool bEnableVisualFeedback = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Presentation|Visual", meta = (ClampMin = "1.0", UIMin = "1.0", UIMax = "1.5", ToolTip = "Relative scale multiplier used for DamageDealt cue pulse. 1.08 means 8 percent larger than the primitive's saved base relative scale."))
	float DamagePulseScale = 1.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Presentation|Visual", meta = (ClampMin = "1.0", UIMin = "1.0", UIMax = "1.8", ToolTip = "Relative scale multiplier used for EnemyPartHpEmptied cue pulse. This is intentionally stronger than normal damage feedback."))
	float DestroyedPulseScale = 1.16f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Presentation|Visual", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1.0", Units = "s", ToolTip = "Seconds to hold the DamageDealt scale pulse before restoring the primitive's saved base relative scale."))
	float DamagePulseSeconds = 0.14f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Presentation|Visual", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1.0", Units = "s", ToolTip = "Seconds to hold the EnemyPartHpEmptied scale pulse before restoring the primitive's saved base relative scale."))
	float DestroyedPulseSeconds = 0.28f;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Presentation")
	void SetPartId(FName InPartId);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Presentation")
	void SetPartInstanceId(const FGuid& InPartInstanceId);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Presentation")
	void SetVisualTargetComponent(UPrimitiveComponent* InVisualTargetComponent);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Presentation")
	void SetClickTargetComponent(UPrimitiveComponent* InClickTargetComponent);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Presentation", meta = (ToolTip = "V0-A click intent bridge. Forwards this target's PartInstanceId to the registered BattleHUD. Returns true only when the intent was forwarded to a valid HUD; it does not guarantee the battle command succeeds."))
	bool RequestSceneTargetClick();

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Presentation")
	bool RegisterWithBattleHUD(UBattleHUD* InHUD);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Presentation")
	void UnregisterFromBattleHUD();

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Presentation")
	bool IsRegisteredWithBattleHUD() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Presentation")
	FName GetPartId() const { return PartId; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Presentation")
	FGuid GetPartInstanceId() const { return PartInstanceId; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Presentation")
	EBattleEventType GetLastBattlePresentationCueType() const { return LastCueType; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Presentation")
	int32 GetLastBattlePresentationCueAmount() const { return LastCueAmount; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Presentation")
	int32 GetBattlePresentationCuePlayCount() const { return CuePlayCount; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	virtual void NativeOnBattlePresentationCue(EBattleEventType SourceEventType, int32 Amount);
	void RestoreVisualFeedback();
	bool IsVisualFeedbackActive() const { return bVisualFeedbackActive; }

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<UBattleHUD> RegisteredHUD;

	TWeakObjectPtr<UPrimitiveComponent> ActiveVisualFeedbackTarget;
	TWeakObjectPtr<UPrimitiveComponent> BoundClickTarget;
	FVector BaseVisualFeedbackScale = FVector::OneVector;
	EBattleEventType LastCueType = EBattleEventType::None;
	int32 LastCueAmount = 0;
	int32 CuePlayCount = 0;
	bool bVisualFeedbackActive = false;
	bool bHasAcquiredPlayerControllerClickEvents = false;
	bool bHasSavedClickTargetCollision = false;
	ECollisionEnabled::Type SavedClickTargetCollisionEnabled = ECollisionEnabled::NoCollision;
	ECollisionResponse SavedClickTargetVisibilityResponse = ECR_Block;
	FTimerHandle VisualFeedbackTimerHandle;

	void HandleBattlePresentationCue(EBattleEventType SourceEventType, int32 Amount);
	void BindPIEClickTarget();
	void UnbindPIEClickTarget();
	void SaveAndConfigureClickTargetCollision(UPrimitiveComponent& Target);
	void RestoreClickTargetCollision(UPrimitiveComponent& Target);
	UPrimitiveComponent* ResolveClickTargetComponent() const;
	void PlayVisualFeedback(EBattleEventType SourceEventType);
	void StopVisualFeedbackTimer();
	UPrimitiveComponent* ResolveVisualTargetComponent() const;

	UFUNCTION()
	void HandleClickTargetClicked(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed);

	friend class UBattleHUD;
#if WITH_AUTOMATION_TESTS
	friend class UWacomBattlePresentationTargetComponentProbe;
#endif
};
