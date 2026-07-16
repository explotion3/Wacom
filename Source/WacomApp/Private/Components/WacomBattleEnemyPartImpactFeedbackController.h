// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WacomBattleEnemyPartCuePlayback.h"

class UActorComponent;
class UMaterialInterface;
class UNiagaraComponent;
class UPrimitiveComponent;
class USceneComponent;
class USoundBase;
class UWacomBattleEnemyPartImpactStyle;
struct FWacomBattlePresentationTargetCue;

enum class EWacomBattleEnemyPartImpactEffectKind : uint8
{
	None,
	TargetConfirmed,
	Damage,
	Destroyed,
};

struct FWacomBattleEnemyPartImpactRequest
{
	EWacomBattleEnemyPartImpactEffectKind EffectKind = EWacomBattleEnemyPartImpactEffectKind::None;
	float DurationSeconds = 0.0f;
	float Intensity = 0.0f;
	int32 Seed = 0;
	float DecorativeIntensity = 1.0f;
	bool bReducedMotion = false;
	float TargetDiameterCentimeters = 96.0f;
	float CameraDepthOffsetCentimeters = 0.0f;
	TObjectPtr<USoundBase> Sound = nullptr;
	float SoundVolumeMultiplier = 1.0f;
	float SoundPitchMultiplier = 1.0f;
	float SoundPitchVariation = 0.0f;
};

struct FWacomBattleEnemyPartImpactFeedbackDebugView
{
	bool bNiagaraReady = false;
	bool bEffectActive = false;
	float LastIntensity = 0.0f;
	int32 LastSeed = 0;
	float LastDecorativeIntensity = 1.0f;
	float LastTargetDiameterCentimeters = 0.0f;
	bool bLastReducedMotion = false;
	int32 EffectPlayCount = 0;
	int32 SoundRequestCount = 0;
	FName LastEffectKind = TEXT("None");
	int32 LastEffectKindValue = INDEX_NONE;
};

/** App-private owner of the one reusable Niagara component for an enemy part. */
class FWacomBattleEnemyPartImpactFeedbackController
{
public:
	bool PlayAcceptedCue(
		UActorComponent& OwnerComponent,
		USceneComponent* ImpactAnchor,
		UPrimitiveComponent* ImpactExtentSource,
		const UWacomBattleEnemyPartImpactStyle* Style,
		EWacomBattleEnemyPartCuePlaybackKind PlaybackKind,
		const FWacomBattlePresentationTargetCue& Cue,
		float DecorativeIntensity,
		bool bReducedMotion);

	void FinishNaturally();
	void ResetImmediate(bool bDestroyComponent);

	const FWacomBattleEnemyPartImpactFeedbackDebugView& GetDebugView() const
	{
		return DebugView;
	}

	static FWacomBattleEnemyPartImpactRequest BuildRequest(
		const UWacomBattleEnemyPartImpactStyle& Style,
		EWacomBattleEnemyPartCuePlaybackKind PlaybackKind,
		const FWacomBattlePresentationTargetCue& Cue,
		float DecorativeIntensity,
		bool bReducedMotion);

private:
	UNiagaraComponent* ResolveOrCreateComponent(
		UActorComponent& OwnerComponent,
		USceneComponent& ImpactAnchor,
		const UWacomBattleEnemyPartImpactStyle& Style);
	static FName EffectKindToName(EWacomBattleEnemyPartImpactEffectKind Kind);
	static float ResolveStablePitch(const FWacomBattleEnemyPartImpactRequest& Request);
	static float ResolveTargetDiameterCentimeters(
		const UWacomBattleEnemyPartImpactStyle& Style,
		EWacomBattleEnemyPartImpactEffectKind EffectKind,
		const UPrimitiveComponent* ImpactExtentSource,
		const FVector& PlaneRight,
		const FVector& PlaneUp);

	TWeakObjectPtr<UNiagaraComponent> NiagaraComponent;
	TWeakObjectPtr<USceneComponent> AttachedImpactAnchor;
	TWeakObjectPtr<const UWacomBattleEnemyPartImpactStyle> ActiveStyle;
	FWacomBattleEnemyPartImpactFeedbackDebugView DebugView;
};
