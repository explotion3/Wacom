// Copyright Wacom. All Rights Reserved.

#include "Components/WacomBattleEnemyPartImpactFeedbackController.h"

#include "Actors/WacomBattleEnemyPartImpactStyle.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"
#include "UObject/UObjectGlobals.h"

namespace WacomBattleEnemyPartImpactFeedbackPrivate
{
	const FName EffectKindParameter(TEXT("User.EffectKind"));
	const FName DurationParameter(TEXT("User.Duration"));
	const FName IntensityParameter(TEXT("User.Intensity"));
	const FName SeedParameter(TEXT("User.Seed"));
	const FName DecorativeIntensityParameter(TEXT("User.DecorativeIntensity"));
	const FName ReducedMotionParameter(TEXT("User.ReducedMotion"));
	const FName ImpactMaterialParameter(TEXT("User.ImpactMaterial"));
	const FName PlaneRightParameter(TEXT("User.PlaneRight"));
	const FName PlaneUpParameter(TEXT("User.PlaneUp"));
	const FName TargetDiameterParameter(TEXT("User.TargetDiameter"));

	int32 ToNiagaraEffectKind(EWacomBattleEnemyPartImpactEffectKind EffectKind)
	{
		switch (EffectKind)
		{
		case EWacomBattleEnemyPartImpactEffectKind::Damage:
			return 1;
		case EWacomBattleEnemyPartImpactEffectKind::Destroyed:
			return 3;
		case EWacomBattleEnemyPartImpactEffectKind::TargetConfirmed:
		default:
			return 0;
		}
	}
}

FWacomBattleEnemyPartImpactRequest
FWacomBattleEnemyPartImpactFeedbackController::BuildRequest(
	const UWacomBattleEnemyPartImpactStyle& Style,
	EWacomBattleEnemyPartCuePlaybackKind PlaybackKind,
	const FWacomBattlePresentationTargetCue& Cue,
	float DecorativeIntensity,
	bool bReducedMotion)
{
	FWacomBattleEnemyPartImpactRequest Request;
	Request.DurationSeconds = FMath::Max(KINDA_SMALL_NUMBER, Cue.Duration);
	Request.Seed = Cue.Seed;
	Request.DecorativeIntensity = FMath::Clamp(DecorativeIntensity, 0.0f, 1.0f);
	Request.bReducedMotion = bReducedMotion;
	Request.CameraDepthOffsetCentimeters = FMath::Max(0.0f, Style.CameraDepthOffsetCentimeters);

	switch (PlaybackKind)
	{
	case EWacomBattleEnemyPartCuePlaybackKind::TargetConfirmed:
		Request.EffectKind = EWacomBattleEnemyPartImpactEffectKind::TargetConfirmed;
		Request.Intensity = FMath::Max(0.0f, Style.TargetConfirmedIntensity);
		Request.Sound = Style.TargetConfirmedSound;
		Request.SoundVolumeMultiplier = Style.TargetConfirmedSoundVolumeMultiplier;
		Request.SoundPitchMultiplier = Style.TargetConfirmedSoundPitchMultiplier;
		Request.SoundPitchVariation = Style.TargetConfirmedSoundPitchVariation;
		break;
	case EWacomBattleEnemyPartCuePlaybackKind::Damage:
		Request.EffectKind = EWacomBattleEnemyPartImpactEffectKind::Damage;
		Request.Intensity = Style.ResolveDamageIntensity(Cue.Amount);
		Request.Sound = Style.DamageSound;
		Request.SoundVolumeMultiplier = Style.DamageSoundVolumeMultiplier;
		Request.SoundPitchMultiplier = Style.DamageSoundPitchMultiplier;
		Request.SoundPitchVariation = Style.DamageSoundPitchVariation;
		break;
	case EWacomBattleEnemyPartCuePlaybackKind::Destroyed:
		Request.EffectKind = EWacomBattleEnemyPartImpactEffectKind::Destroyed;
		Request.Intensity = FMath::Max(0.0f, Style.DestroyedIntensity);
		Request.Sound = Style.DestroyedSound;
		Request.SoundVolumeMultiplier = Style.DestroyedSoundVolumeMultiplier;
		Request.SoundPitchMultiplier = Style.DestroyedSoundPitchMultiplier;
		Request.SoundPitchVariation = Style.DestroyedSoundPitchVariation;
		break;
	case EWacomBattleEnemyPartCuePlaybackKind::None:
	default:
		Request.EffectKind = EWacomBattleEnemyPartImpactEffectKind::None;
		break;
	}
	return Request;
}

bool FWacomBattleEnemyPartImpactFeedbackController::PlayAcceptedCue(
	UActorComponent& OwnerComponent,
	USceneComponent* ImpactAnchor,
	const FWacomBattleEnemyPartPresentationBounds& PresentationBounds,
	const UWacomBattleEnemyPartImpactStyle* Style,
	EWacomBattleEnemyPartCuePlaybackKind PlaybackKind,
	const FWacomBattlePresentationTargetCue& Cue,
	float DecorativeIntensity,
	bool bReducedMotion)
{
	if (!IsValid(Style))
	{
		ResetImmediate(false);
		return false;
	}

	FWacomBattleEnemyPartImpactRequest Request = BuildRequest(
		*Style,
		PlaybackKind,
		Cue,
		DecorativeIntensity,
		bReducedMotion);
	if (Request.EffectKind == EWacomBattleEnemyPartImpactEffectKind::None)
	{
		ResetImmediate(false);
		return false;
	}

	UWorld* World = OwnerComponent.GetWorld();
	if (Request.Sound && World && IsValid(ImpactAnchor))
	{
		UGameplayStatics::PlaySoundAtLocation(
			World,
			Request.Sound,
			ImpactAnchor->GetComponentLocation(),
			FMath::Max(0.0f, Request.SoundVolumeMultiplier),
			ResolveStablePitch(Request));
		++DebugView.SoundRequestCount;
	}

	FVector PlaneRight = FVector::RightVector;
	FVector PlaneUp = FVector::UpVector;
	FVector CameraDirection = FVector::ForwardVector;
	if (IsValid(ImpactAnchor))
	{
		if (APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(World, 0))
		{
			const FRotationMatrix CameraMatrix(CameraManager->GetCameraRotation());
			PlaneRight = CameraMatrix.GetUnitAxis(EAxis::Y);
			PlaneUp = CameraMatrix.GetUnitAxis(EAxis::Z);
			CameraDirection = (CameraManager->GetCameraLocation()
				- ImpactAnchor->GetComponentLocation()).GetSafeNormal();
			if (CameraDirection.IsNearlyZero())
			{
				CameraDirection = -CameraMatrix.GetUnitAxis(EAxis::X);
			}
		}
	}
	Request.TargetDiameterCentimeters = ResolveTargetDiameterCentimeters(
		*Style,
		Request.EffectKind,
		PresentationBounds,
		PlaneRight,
		PlaneUp);
	DebugView.LastEffectKind = EffectKindToName(Request.EffectKind);
	DebugView.LastEffectKindValue =
		WacomBattleEnemyPartImpactFeedbackPrivate::ToNiagaraEffectKind(Request.EffectKind);
	DebugView.LastIntensity = Request.Intensity;
	DebugView.LastSeed = Request.Seed;
	DebugView.LastDecorativeIntensity = Request.DecorativeIntensity;
	DebugView.LastTargetDiameterCentimeters = Request.TargetDiameterCentimeters;
	DebugView.bLastReducedMotion = Request.bReducedMotion;
	DebugView.PresentationBoundsSource = PresentationBounds.GetSource();

	if (!Style->HasValidVisualAssets() || !IsValid(ImpactAnchor))
	{
		DebugView.bNiagaraReady = false;
		DebugView.bEffectActive = false;
		return false;
	}

	UNiagaraComponent* Component = ResolveOrCreateComponent(
		OwnerComponent,
		*ImpactAnchor,
		*Style);
	if (!Component)
	{
		DebugView.bNiagaraReady = false;
		DebugView.bEffectActive = false;
		return false;
	}

	DebugView.LastEffectWorldLocation =
		ImpactAnchor->GetComponentLocation()
		+ CameraDirection * Request.CameraDepthOffsetCentimeters;
	Component->SetWorldLocation(DebugView.LastEffectWorldLocation);
	Component->SetVariableInt(WacomBattleEnemyPartImpactFeedbackPrivate::EffectKindParameter,
		WacomBattleEnemyPartImpactFeedbackPrivate::ToNiagaraEffectKind(Request.EffectKind));
	Component->SetVariableFloat(
		WacomBattleEnemyPartImpactFeedbackPrivate::DurationParameter,
		Request.DurationSeconds);
	Component->SetVariableFloat(
		WacomBattleEnemyPartImpactFeedbackPrivate::IntensityParameter,
		Request.Intensity);
	Component->SetVariableInt(WacomBattleEnemyPartImpactFeedbackPrivate::SeedParameter, Request.Seed);
	Component->SetVariableFloat(
		WacomBattleEnemyPartImpactFeedbackPrivate::DecorativeIntensityParameter,
		Request.DecorativeIntensity);
	Component->SetVariableBool(
		WacomBattleEnemyPartImpactFeedbackPrivate::ReducedMotionParameter,
		Request.bReducedMotion);
	Component->SetVariableMaterial(
		WacomBattleEnemyPartImpactFeedbackPrivate::ImpactMaterialParameter,
		Style->ImpactMaterialInstance);
	Component->SetVariableVec3(
		WacomBattleEnemyPartImpactFeedbackPrivate::PlaneRightParameter,
		PlaneRight);
	Component->SetVariableVec3(
		WacomBattleEnemyPartImpactFeedbackPrivate::PlaneUpParameter,
		PlaneUp);
	Component->SetVariableFloat(
		WacomBattleEnemyPartImpactFeedbackPrivate::TargetDiameterParameter,
		Request.TargetDiameterCentimeters);
	Component->Activate(true);

	DebugView.bNiagaraReady = true;
	DebugView.bEffectActive = true;
	++DebugView.EffectPlayCount;
	return true;
}

void FWacomBattleEnemyPartImpactFeedbackController::FinishNaturally()
{
	if (UNiagaraComponent* Component = NiagaraComponent.Get())
	{
		Component->Deactivate();
	}
	DebugView.bEffectActive = false;
}

void FWacomBattleEnemyPartImpactFeedbackController::ResetImmediate(bool bDestroyComponent)
{
	if (UNiagaraComponent* Component = NiagaraComponent.Get())
	{
		Component->DeactivateImmediate();
		if (bDestroyComponent)
		{
			Component->DestroyComponent();
			NiagaraComponent.Reset();
			AttachedImpactAnchor.Reset();
			ActiveStyle.Reset();
		}
	}
	DebugView.bEffectActive = false;
	DebugView.bNiagaraReady = NiagaraComponent.IsValid();
}

UNiagaraComponent* FWacomBattleEnemyPartImpactFeedbackController::ResolveOrCreateComponent(
	UActorComponent& OwnerComponent,
	USceneComponent& ImpactAnchor,
	const UWacomBattleEnemyPartImpactStyle& Style)
{
	AActor* Owner = OwnerComponent.GetOwner();
	UWorld* World = OwnerComponent.GetWorld();
	if (!Owner || !World || !Style.HasValidVisualAssets())
	{
		return nullptr;
	}

	UNiagaraComponent* Component = NiagaraComponent.Get();
	if (!Component)
	{
		const FName ComponentName = MakeUniqueObjectName(
			Owner,
			UNiagaraComponent::StaticClass(),
			TEXT("WacomEnemyPartImpactNiagara"));
		Component = NewObject<UNiagaraComponent>(
			Owner,
			ComponentName,
			RF_Transient);
		if (!Component)
		{
			return nullptr;
		}
		Component->SetAutoActivate(false);
		Component->SetAutoDestroy(false);
		Component->SetupAttachment(&ImpactAnchor);
		Owner->AddInstanceComponent(Component);
		Component->RegisterComponentWithWorld(World);
		NiagaraComponent = Component;
	}
	else if (AttachedImpactAnchor.Get() != &ImpactAnchor)
	{
		Component->AttachToComponent(
			&ImpactAnchor,
			FAttachmentTransformRules::KeepWorldTransform);
	}

	if (Component->GetAsset() != Style.ImpactSystem)
	{
		Component->SetAsset(Style.ImpactSystem);
	}
	AttachedImpactAnchor = &ImpactAnchor;
	ActiveStyle = &Style;
	return Component;
}

FName FWacomBattleEnemyPartImpactFeedbackController::EffectKindToName(
	EWacomBattleEnemyPartImpactEffectKind Kind)
{
	switch (Kind)
	{
	case EWacomBattleEnemyPartImpactEffectKind::TargetConfirmed:
		return TEXT("TargetConfirmed");
	case EWacomBattleEnemyPartImpactEffectKind::Damage:
		return TEXT("Damage");
	case EWacomBattleEnemyPartImpactEffectKind::Destroyed:
		return TEXT("Destroyed");
	case EWacomBattleEnemyPartImpactEffectKind::None:
	default:
		return TEXT("None");
	}
}

float FWacomBattleEnemyPartImpactFeedbackController::ResolveStablePitch(
	const FWacomBattleEnemyPartImpactRequest& Request)
{
	const uint32 Hash = HashCombineFast(
		GetTypeHash(Request.Seed),
		GetTypeHash(static_cast<uint8>(Request.EffectKind)));
	const float UnitValue = static_cast<float>(Hash & 0x00FFFFFFu)
		/ static_cast<float>(0x00FFFFFFu);
	const float Variation = FMath::Max(0.0f, Request.SoundPitchVariation);
	return FMath::Max(
		0.01f,
		Request.SoundPitchMultiplier * (1.0f + FMath::Lerp(-Variation, Variation, UnitValue)));
}

float FWacomBattleEnemyPartImpactFeedbackController::ResolveTargetDiameterCentimeters(
	const UWacomBattleEnemyPartImpactStyle& Style,
	EWacomBattleEnemyPartImpactEffectKind EffectKind,
	const FWacomBattleEnemyPartPresentationBounds& PresentationBounds,
	const FVector& PlaneRight,
	const FVector& PlaneUp)
{
	float Diameter = Style.FallbackImpactDiameterCentimeters;
	bool bHasProjectedDiameter = false;
	const float ProjectedDiameter =
		PresentationBounds.ProjectDiameterCentimeters(PlaneRight, PlaneUp);
	if (FMath::IsFinite(ProjectedDiameter)
		&& ProjectedDiameter > KINDA_SMALL_NUMBER)
	{
		Diameter = ProjectedDiameter;
		bHasProjectedDiameter = true;
	}

	float CoverageMultiplier = Style.TargetConfirmedCoverageMultiplier;
	if (EffectKind == EWacomBattleEnemyPartImpactEffectKind::Damage)
	{
		CoverageMultiplier = Style.DamageCoverageMultiplier;
	}
	else if (EffectKind == EWacomBattleEnemyPartImpactEffectKind::Destroyed)
	{
		CoverageMultiplier = Style.DestroyedCoverageMultiplier;
	}
	if (bHasProjectedDiameter)
	{
		Diameter *= FMath::Max(0.0f, CoverageMultiplier);
	}

	const float LowerBound = FMath::Max(
		KINDA_SMALL_NUMBER,
		FMath::Min(Style.MinimumImpactDiameterCentimeters, Style.MaximumImpactDiameterCentimeters));
	const float UpperBound = FMath::Max(
		LowerBound,
		FMath::Max(Style.MinimumImpactDiameterCentimeters, Style.MaximumImpactDiameterCentimeters));
	return FMath::Clamp(Diameter, LowerBound, UpperBound);
}
