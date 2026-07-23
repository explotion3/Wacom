// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WacomBattleEnemyPartPresentationBounds.h"
#include "WacomBattleEnemyPartTargetPreviewPlayback.h"

class UActorComponent;
class UNiagaraComponent;
class USceneComponent;
class UWacomBattleEnemyPartTargetPreviewStyle;

struct FWacomBattleEnemyPartTargetPreviewFeedbackDebugView
{
	bool bNiagaraReady = false;
	bool bEffectActive = false;
	EWacomBattleEnemyPartTargetPreviewKind Kind = EWacomBattleEnemyPartTargetPreviewKind::None;
	bool bReducedMotion = false;
	float Amount = 0.0f;
	float Pulse = 0.0f;
	FVector2D TargetSizeCentimeters = FVector2D::ZeroVector;
	float AvailabilityIconSizeCentimeters = 0.0f;
	FName PresentationBoundsSource = NAME_None;
	FVector PresentationBoundsWorldCenter = FVector::ZeroVector;
	FVector TargetWorldCenter = FVector::ZeroVector;
	int32 ActivationCount = 0;
};

/** App-private owner of the reusable target-preview Niagara component for one enemy part. */
class WACOMAPP_API FWacomBattleEnemyPartTargetPreviewFeedbackController
{
public:
	bool BeginOrUpdate(
		UActorComponent& OwnerComponent,
		USceneComponent* AttachmentParent,
		const FWacomBattleEnemyPartPresentationBounds& PresentationBounds,
		const UWacomBattleEnemyPartTargetPreviewStyle* Style,
		const FWacomBattleEnemyPartTargetPreviewPlaybackView& PlaybackView,
		float DecorativeIntensity);

	void FinishNaturally();
	void ResetImmediate(bool bDestroyComponent);

	const FWacomBattleEnemyPartTargetPreviewFeedbackDebugView& GetDebugView() const
	{
		return DebugView;
	}

private:
	UNiagaraComponent* ResolveOrCreateComponent(
		UActorComponent& OwnerComponent,
		USceneComponent& AttachmentParent,
		const UWacomBattleEnemyPartTargetPreviewStyle& Style);
	static FVector2D ResolveTargetSizeCentimeters(
		const UWacomBattleEnemyPartTargetPreviewStyle& Style,
		EWacomBattleEnemyPartTargetPreviewKind Kind,
		const FWacomBattleEnemyPartPresentationBounds& PresentationBounds,
		const FVector& PlaneRight,
		const FVector& PlaneUp);
	static float ResolveAvailabilityIconSizeCentimeters(
		const UWacomBattleEnemyPartTargetPreviewStyle& Style,
		const FVector2D& TargetSizeCentimeters);

	TWeakObjectPtr<UNiagaraComponent> NiagaraComponent;
	TWeakObjectPtr<USceneComponent> AttachedPart;
	TWeakObjectPtr<const UWacomBattleEnemyPartTargetPreviewStyle> ActiveStyle;
	FWacomBattleEnemyPartTargetPreviewFeedbackDebugView DebugView;
};
