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

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattlePresentationTargetDebugView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Presentation|Debug")
	FName PartId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Presentation|Debug")
	FGuid PartInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Presentation|Debug")
	bool bIsRegisteredWithBattleHUD = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Presentation|Debug")
	FString RegisteredHUDName;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Presentation|Debug")
	FString ResolvedVisualTargetName;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Presentation|Debug")
	FString ResolvedClickTargetName;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Presentation|Debug")
	FString BoundClickTargetName;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Presentation|Debug")
	TEnumAsByte<ECollisionEnabled::Type> ClickTargetCollisionEnabled = ECollisionEnabled::NoCollision;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Presentation|Debug")
	TEnumAsByte<ECollisionResponse> ClickTargetVisibilityResponse = ECR_Ignore;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Presentation|Debug")
	bool bClickTargetBlocksVisibility = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Presentation|Debug")
	EBattleEventType LastCueType = EBattleEventType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Presentation|Debug")
	int32 LastCueAmount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Presentation|Debug")
	int32 CuePlayCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Presentation|Debug")
	bool bVisualFeedbackActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Presentation|Debug")
	bool bTargetSelectionAffordanceActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Presentation|Debug")
	bool bTargetSelectionTargetable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Presentation|Debug")
	FName TargetSelectionDisabledReason = TEXT("NotAttempted");

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Presentation|Debug")
	FName LastRegistrationResult = TEXT("NotAttempted");

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Presentation|Debug")
	FName LastAutoBindResult = TEXT("NotAttempted");

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Presentation|Debug")
	FName LastClickResult = TEXT("NotAttempted");
};

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Presentation|Selection", meta = (ToolTip = "Enables the V0 target-selection affordance. When BattleHUD is selecting a valid enemy part target, this component plays a lightweight scale breathing hint on its visual primitive."))
	bool bEnableTargetSelectionAffordance = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Presentation|Selection", meta = (ClampMin = "1.0", UIMin = "1.0", UIMax = "1.3", ToolTip = "Base relative scale multiplier used while this scene target is selectable in BattleHUD TargetSelect. 1.06 means 6 percent larger than the primitive's saved base relative scale."))
	float TargetSelectionAffordanceScale = 1.06f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Presentation|Selection", meta = (ClampMin = "1.0", UIMin = "1.0", UIMax = "1.5", ToolTip = "Upper relative scale multiplier used by the V0 breathing hint while this scene target is selectable."))
	float TargetSelectionAffordancePulseScale = 1.10f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Presentation|Selection", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "2.0", Units = "s", ToolTip = "Seconds between V0 target-selection affordance scale toggles. Set to 0 to hold TargetSelectionAffordanceScale without breathing."))
	float TargetSelectionAffordancePulseSeconds = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Presentation|Debug", meta = (ToolTip = "When enabled, registration, auto-binding, click forwarding, and presentation cue state changes are written to the log. No tick or on-screen debug output is used."))
	bool bLogDebugStateChanges = false;

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

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Presentation|Debug")
	FWacomBattlePresentationTargetDebugView GetBattlePresentationTargetDebugView() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Presentation|Debug")
	FString GetBattlePresentationTargetDebugSummary() const;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Wacom|Battle|Presentation|Debug")
	void LogBattlePresentationTargetDebugSummary() const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Presentation|Debug")
	bool ValidateBattlePresentationTargetAuthoring(TArray<FString>& OutWarnings) const;

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
	bool bHasBaseVisualFeedbackScale = false;
	float ActiveVisualFeedbackScaleMultiplier = 1.0f;
	bool bTargetSelectionAffordanceActive = false;
	bool bTargetSelectionTargetable = false;
	bool bTargetSelectionAffordancePulseHigh = false;
	bool bHasAcquiredPlayerControllerClickEvents = false;
	bool bHasSavedClickTargetCollision = false;
	ECollisionEnabled::Type SavedClickTargetCollisionEnabled = ECollisionEnabled::NoCollision;
	ECollisionResponse SavedClickTargetVisibilityResponse = ECR_Block;
	FTimerHandle VisualFeedbackTimerHandle;
	FTimerHandle TargetSelectionAffordanceTimerHandle;
	FName LastRegistrationResult = TEXT("NotAttempted");
	FName LastAutoBindResult = TEXT("NotAttempted");
	FName LastClickResult = TEXT("NotAttempted");
	FName TargetSelectionDisabledReason = TEXT("NotAttempted");

	void SetTargetSelectionAffordance(bool bTargetable, FName DisabledReason);
	void HandleBattlePresentationCue(EBattleEventType SourceEventType, int32 Amount);
	void BindPIEClickTarget();
	void UnbindPIEClickTarget();
	void SaveAndConfigureClickTargetCollision(UPrimitiveComponent& Target);
	void RestoreClickTargetCollision(UPrimitiveComponent& Target);
	UPrimitiveComponent* ResolveClickTargetComponent() const;
	void PlayVisualFeedback(EBattleEventType SourceEventType);
	bool EnsureManagedVisualTarget();
	void ApplyCurrentVisualScale();
	void RestoreManagedVisualScaleIfIdle();
	void RestoreManagedVisualScale();
	void StopVisualFeedbackTimer();
	void StartTargetSelectionAffordance();
	void StopTargetSelectionAffordanceTimer();
	void StopTargetSelectionAffordance();
	void AdvanceTargetSelectionAffordancePulse();
	void StopAllVisualPresentation();
	UPrimitiveComponent* ResolveVisualTargetComponent() const;
	void MarkRegistrationResult(FName Result);
	void MarkAutoBindResult(FName Result);
	void MarkClickResult(FName Result);
	void LogDebugStateChange(const TCHAR* EventName, FName Result) const;

	UFUNCTION()
	void HandleClickTargetClicked(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed);

	friend class UBattleHUD;
#if WITH_AUTOMATION_TESTS
	friend class UWacomBattlePresentationTargetComponentProbe;
#endif
};
