// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WacomBattleEnemyPartTargetPreviewPlayback.h"

class UActorComponent;
class UNiagaraComponent;
class UPrimitiveComponent;
class USceneComponent;
class UWacomBattleEnemyPartTargetPreviewStyle;

struct FWacomBattleEnemyPartTargetPreviewFeedbackDebugView
{
	bool bNiagaraReady = false;
	bool bEffectActive = false;
	bool bValidTarget = false;
	bool bReducedMotion = false;
	float Amount = 0.0f;
	float Pulse = 0.0f;
	FVector2D TargetSizeCentimeters = FVector2D::ZeroVector;
	int32 ActivationCount = 0;
};

/** App-private owner of the reusable target-preview Niagara component for one enemy part. */
class FWacomBattleEnemyPartTargetPreviewFeedbackController
{
public:
	bool BeginOrUpdate(
		UActorComponent& OwnerComponent,
		USceneComponent* ImpactAnchor,
		UPrimitiveComponent* ImpactExtentSource,
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
		USceneComponent& ImpactAnchor,
		const UWacomBattleEnemyPartTargetPreviewStyle& Style);
	static FVector2D ResolveTargetSizeCentimeters(
		const UWacomBattleEnemyPartTargetPreviewStyle& Style,
		EWacomBattleEnemyPartTargetPreviewKind Kind,
		const UPrimitiveComponent* ImpactExtentSource,
		const FVector& PlaneRight,
		const FVector& PlaneUp);

	TWeakObjectPtr<UNiagaraComponent> NiagaraComponent;
	TWeakObjectPtr<USceneComponent> AttachedImpactAnchor;
	TWeakObjectPtr<const UWacomBattleEnemyPartTargetPreviewStyle> ActiveStyle;
	FWacomBattleEnemyPartTargetPreviewFeedbackDebugView DebugView;
};
