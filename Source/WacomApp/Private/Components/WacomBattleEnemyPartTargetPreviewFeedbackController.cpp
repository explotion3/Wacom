// Copyright Wacom. All Rights Reserved.

#include "Components/WacomBattleEnemyPartTargetPreviewFeedbackController.h"

#include "Actors/WacomBattleEnemyPartTargetPreviewStyle.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

namespace WacomBattleEnemyPartTargetPreviewFeedbackPrivate
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
	const FName TargetWidthParameter(TEXT("User.TargetWidth"));
	const FName TargetHeightParameter(TEXT("User.TargetHeight"));
	const FName PreviewAmountParameter(TEXT("User.PreviewAmount"));
	const FName PreviewValidityParameter(TEXT("User.PreviewValidity"));
	const FName PreviewPulseParameter(TEXT("User.PreviewPulse"));
}

using namespace WacomBattleEnemyPartTargetPreviewFeedbackPrivate;

bool FWacomBattleEnemyPartTargetPreviewFeedbackController::BeginOrUpdate(
	UActorComponent& OwnerComponent,
	USceneComponent* ImpactAnchor,
	UPrimitiveComponent* ImpactExtentSource,
	const UWacomBattleEnemyPartTargetPreviewStyle* Style,
	const FWacomBattleEnemyPartTargetPreviewPlaybackView& PlaybackView,
	float DecorativeIntensity)
{
	if (!PlaybackView.bActive || PlaybackView.Kind == EWacomBattleEnemyPartTargetPreviewKind::None
		|| !IsValid(Style) || !Style->HasValidVisualAssets() || !IsValid(ImpactAnchor))
	{
		DebugView.bEffectActive = false;
		DebugView.bNiagaraReady = NiagaraComponent.IsValid();
		return false;
	}

	UWorld* World = OwnerComponent.GetWorld();
	FVector PlaneRight = FVector::RightVector;
	FVector PlaneUp = FVector::UpVector;
	FVector CameraDirection = FVector::ForwardVector;
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

	UNiagaraComponent* Component = ResolveOrCreateComponent(
		OwnerComponent,
		*ImpactAnchor,
		*Style);
	if (!Component)
	{
		DebugView.bEffectActive = false;
		DebugView.bNiagaraReady = false;
		return false;
	}

	const FVector2D TargetSize = ResolveTargetSizeCentimeters(
		*Style,
		PlaybackView.Kind,
		ImpactExtentSource,
		PlaneRight,
		PlaneUp);
	const bool bWasActive = DebugView.bEffectActive;
	Component->SetWorldLocation(
		ImpactAnchor->GetComponentLocation()
		+ CameraDirection * FMath::Max(0.0f, Style->CameraDepthOffsetCentimeters));
	Component->SetVariableInt(EffectKindParameter, 2);
	Component->SetVariableFloat(DurationParameter, 3600.0f);
	Component->SetVariableFloat(IntensityParameter, 1.0f);
	Component->SetVariableInt(SeedParameter, GetTypeHash(OwnerComponent.GetOwner()));
	Component->SetVariableFloat(
		DecorativeIntensityParameter,
		FMath::Clamp(DecorativeIntensity, 0.0f, 1.0f));
	Component->SetVariableBool(ReducedMotionParameter, PlaybackView.bReducedMotion);
	Component->SetVariableMaterial(ImpactMaterialParameter, Style->PreviewMaterialInstance);
	Component->SetVariableVec3(PlaneRightParameter, PlaneRight);
	Component->SetVariableVec3(PlaneUpParameter, PlaneUp);
	Component->SetVariableFloat(TargetDiameterParameter, FMath::Max(TargetSize.X, TargetSize.Y));
	Component->SetVariableFloat(TargetWidthParameter, TargetSize.X);
	Component->SetVariableFloat(TargetHeightParameter, TargetSize.Y);
	Component->SetVariableFloat(PreviewAmountParameter, FMath::Clamp(PlaybackView.Amount, 0.0f, 1.0f));
	Component->SetVariableFloat(
		PreviewValidityParameter,
		PlaybackView.Kind == EWacomBattleEnemyPartTargetPreviewKind::Valid ? 1.0f : 0.0f);
	Component->SetVariableFloat(PreviewPulseParameter, FMath::Clamp(PlaybackView.Pulse, 0.0f, 1.0f));
	if (!bWasActive)
	{
		Component->Activate(true);
		++DebugView.ActivationCount;
	}

	DebugView.bNiagaraReady = true;
	DebugView.bEffectActive = true;
	DebugView.bValidTarget = PlaybackView.Kind == EWacomBattleEnemyPartTargetPreviewKind::Valid;
	DebugView.bReducedMotion = PlaybackView.bReducedMotion;
	DebugView.Amount = PlaybackView.Amount;
	DebugView.Pulse = PlaybackView.Pulse;
	DebugView.TargetSizeCentimeters = TargetSize;
	return true;
}

void FWacomBattleEnemyPartTargetPreviewFeedbackController::FinishNaturally()
{
	if (UNiagaraComponent* Component = NiagaraComponent.Get())
	{
		Component->DeactivateImmediate();
	}
	DebugView.bEffectActive = false;
	DebugView.Amount = 0.0f;
	DebugView.Pulse = 0.0f;
}

void FWacomBattleEnemyPartTargetPreviewFeedbackController::ResetImmediate(bool bDestroyComponent)
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
	DebugView.Amount = 0.0f;
	DebugView.Pulse = 0.0f;
}

UNiagaraComponent* FWacomBattleEnemyPartTargetPreviewFeedbackController::ResolveOrCreateComponent(
	UActorComponent& OwnerComponent,
	USceneComponent& ImpactAnchor,
	const UWacomBattleEnemyPartTargetPreviewStyle& Style)
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
		Component = NewObject<UNiagaraComponent>(
			Owner,
			TEXT("WacomEnemyPartTargetPreviewNiagara"),
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
		Component->AttachToComponent(&ImpactAnchor, FAttachmentTransformRules::KeepWorldTransform);
	}

	if (Component->GetAsset() != Style.PreviewSystem)
	{
		Component->SetAsset(Style.PreviewSystem);
	}
	AttachedImpactAnchor = &ImpactAnchor;
	ActiveStyle = &Style;
	return Component;
}

FVector2D FWacomBattleEnemyPartTargetPreviewFeedbackController::ResolveTargetSizeCentimeters(
	const UWacomBattleEnemyPartTargetPreviewStyle& Style,
	EWacomBattleEnemyPartTargetPreviewKind Kind,
	const UPrimitiveComponent* ImpactExtentSource,
	const FVector& PlaneRight,
	const FVector& PlaneUp)
{
	FVector2D Size = Style.FallbackSizeCentimeters.GetAbs();
	if (IsValid(ImpactExtentSource))
	{
		const FVector Extent = ImpactExtentSource->Bounds.BoxExtent.GetAbs();
		const float ProjectedWidth = 2.0f * FVector::DotProduct(PlaneRight.GetAbs(), Extent);
		const float ProjectedHeight = 2.0f * FVector::DotProduct(PlaneUp.GetAbs(), Extent);
		if (FMath::IsFinite(ProjectedWidth) && FMath::IsFinite(ProjectedHeight)
			&& ProjectedWidth > KINDA_SMALL_NUMBER && ProjectedHeight > KINDA_SMALL_NUMBER)
		{
			Size = FVector2D(ProjectedWidth, ProjectedHeight);
		}
	}

	const float Coverage = Kind == EWacomBattleEnemyPartTargetPreviewKind::Invalid
		? Style.InvalidCoverageMultiplier
		: Style.ValidCoverageMultiplier;
	Size *= FMath::Max(0.0f, Coverage);
	const float LowerBound = FMath::Max(
		KINDA_SMALL_NUMBER,
		FMath::Min(Style.MinimumAxisSizeCentimeters, Style.MaximumAxisSizeCentimeters));
	const float UpperBound = FMath::Max(
		LowerBound,
		FMath::Max(Style.MinimumAxisSizeCentimeters, Style.MaximumAxisSizeCentimeters));
	Size.X = FMath::Clamp(Size.X, LowerBound, UpperBound);
	Size.Y = FMath::Clamp(Size.Y, LowerBound, UpperBound);
	return Size;
}
