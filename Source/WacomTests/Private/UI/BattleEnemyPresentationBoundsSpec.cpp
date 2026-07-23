// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "../../../WacomApp/Private/Components/WacomBattleEnemyPartImpactFeedbackController.h"
#include "../../../WacomApp/Private/Components/WacomBattleEnemyPartPresentationBounds.h"
#include "../../../WacomApp/Private/Components/WacomBattleEnemyPartTargetPreviewFeedbackController.h"
#include "../../../WacomApp/Private/Components/WacomBattleEnemySceneRuntimeComponent.h"
#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartImpactStyle.h"
#include "Actors/WacomBattleEnemyPartTargetPreviewStyle.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ScopeExit.h"
#include "NiagaraComponent.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomBattleEnemyPresentationBoundsSpec
{
	UWorld* FindAutomationWorld()
	{
		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (UWorld* World = Context.World())
				{
					return World;
				}
			}
		}
		return GWorld;
	}

	void ResolveCameraPlane(
		const UWorld* World,
		FVector& OutRight,
		FVector& OutUp)
	{
		OutRight = FVector::RightVector;
		OutUp = FVector::UpVector;
		if (APlayerCameraManager* CameraManager =
			UGameplayStatics::GetPlayerCameraManager(World, 0))
		{
			const FRotationMatrix CameraMatrix(CameraManager->GetCameraRotation());
			OutRight = CameraMatrix.GetUnitAxis(EAxis::Y);
			OutUp = CameraMatrix.GetUnitAxis(EAxis::Z);
		}
	}

	template <typename TStyle>
	TStyle* DuplicateStyle(const TCHAR* ObjectPath)
	{
		TStyle* Source = LoadObject<TStyle>(nullptr, ObjectPath);
		return Source
			? DuplicateObject<TStyle>(Source, GetTransientPackage())
			: nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleEnemyPresentationBoundsProjectionSpec,
	"Wacom.UI.Battle.EnemyScene.PresentationBounds.OrientedProjectionPreservesPivotRotationAndScale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleEnemyPresentationBoundsProjectionSpec::RunTest(
	const FString& /*Parameters*/)
{
	const FBoxSphereBounds LocalBounds(
		FVector(7.0f, -4.0f, 9.0f),
		FVector(10.0f, 20.0f, 5.0f),
		22.0f);
	const FTransform WorldTransform(
		FRotator(0.0f, 90.0f, 0.0f),
		FVector(100.0f, 200.0f, 300.0f),
		FVector(2.0f, 3.0f, 4.0f));
	const FWacomBattleEnemyPartPresentationBounds Bounds =
		FWacomBattleEnemyPartPresentationBounds::FromLocalBounds(
			LocalBounds,
			WorldTransform,
			TEXT("AutomationStableVisual"));

	TestTrue(TEXT("Authored bounds are valid"), Bounds.IsValid());
	TestEqual(TEXT("Source is retained"), Bounds.GetSource(),
		FName(TEXT("AutomationStableVisual")));
	TestTrue(TEXT("Offset pivot becomes transformed world center"),
		Bounds.GetWorldCenter().Equals(
			WorldTransform.TransformPosition(LocalBounds.Origin)));
	TestTrue(TEXT("Non-uniform scale is retained on local X half axis"),
		FMath::IsNearlyEqual(Bounds.GetWorldHalfAxisX().Size(), 20.0f));
	TestTrue(TEXT("Non-uniform scale is retained on local Y half axis"),
		FMath::IsNearlyEqual(Bounds.GetWorldHalfAxisY().Size(), 60.0f));
	TestTrue(TEXT("Non-uniform scale is retained on local Z half axis"),
		FMath::IsNearlyEqual(Bounds.GetWorldHalfAxisZ().Size(), 20.0f));

	const FVector2D Projected = Bounds.ProjectSizeCentimeters(
		FVector::RightVector,
		FVector::UpVector);
	TestTrue(TEXT("Yaw rotation projects the authored X axis onto camera width"),
		FMath::IsNearlyEqual(Projected.X, 40.0f));
	TestTrue(TEXT("Authored Z scale controls camera height"),
		FMath::IsNearlyEqual(Projected.Y, 40.0f));
	TestTrue(TEXT("Diameter uses the longer camera-plane axis"),
		FMath::IsNearlyEqual(
			Bounds.ProjectDiameterCentimeters(
				FVector::RightVector,
				FVector::UpVector),
			40.0f));
	TestTrue(TEXT("Short axis uses the smaller camera-plane axis"),
		FMath::IsNearlyEqual(
			Bounds.ProjectShorterAxisCentimeters(
				FVector::RightVector,
				FVector::UpVector),
			40.0f));

	const FWacomBattleEnemyPartPresentationBounds Invalid =
		FWacomBattleEnemyPartPresentationBounds::FromLocalBounds(
			FBoxSphereBounds(FVector::ZeroVector, FVector::ZeroVector, 0.0f),
			FTransform::Identity,
			TEXT("Invalid"));
	TestFalse(TEXT("Zero authored bounds are rejected"), Invalid.IsValid());
	TestTrue(TEXT("Invalid bounds project to zero"),
		Invalid.ProjectSizeCentimeters(
			FVector::RightVector,
			FVector::UpVector).IsNearlyZero());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleEnemyPresentationFeedbackPlacementSpec,
	"Wacom.UI.Battle.EnemyScene.PresentationBounds.TargetCenterAndImpactAnchorRemainIndependent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleEnemyPresentationFeedbackPlacementSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyPresentationBoundsSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyActor* Host = World->SpawnActor<AWacomBattleEnemyActor>(
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParams);
	if (!TestNotNull(TEXT("Enemy host"), Host))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Host))
		{
			Host->Destroy();
		}
	};

	UWacomBattleEnemySceneRuntimeComponent* Runtime =
		Host->GetEnemySceneRuntimeComponent();
	if (!TestNotNull(TEXT("Enemy scene runtime"), Runtime))
	{
		return false;
	}
	USceneComponent* PartAttachment = NewObject<USceneComponent>(
		Host,
		TEXT("PresentationPart"),
		RF_Transient);
	Host->AddInstanceComponent(PartAttachment);
	PartAttachment->SetupAttachment(Host->GetRootComponent());
	PartAttachment->RegisterComponent();

	USceneComponent* ImpactAnchor = NewObject<USceneComponent>(
		Host,
		TEXT("PresentationImpactAnchor"),
		RF_Transient);
	Host->AddInstanceComponent(ImpactAnchor);
	ImpactAnchor->SetupAttachment(PartAttachment);
	ImpactAnchor->SetRelativeLocation(FVector(175.0f, 25.0f, -60.0f));
	ImpactAnchor->RegisterComponent();

	const FWacomBattleEnemyPartPresentationBounds Bounds =
		FWacomBattleEnemyPartPresentationBounds::FromLocalBounds(
			FBoxSphereBounds(
				FVector(20.0f, 0.0f, 15.0f),
				FVector(40.0f, 8.0f, 30.0f),
				50.0f),
			FTransform(
				FRotator(0.0f, 25.0f, 0.0f),
				FVector(-90.0f, 15.0f, 110.0f),
				FVector(1.2f, 1.0f, 0.8f)),
			TEXT("StableInteractionVisual"));
	if (!TestTrue(TEXT("Presentation bounds are valid"), Bounds.IsValid()))
	{
		return false;
	}

	TStrongObjectPtr<UWacomBattleEnemyPartTargetPreviewStyle> PreviewStyle(
		DuplicateStyle<UWacomBattleEnemyPartTargetPreviewStyle>(
			TEXT("/Game/Wacom/UI/Battle/WorldImpact/"
				"DA_BattleEnemyPartTargetPreviewStyle_PixelLock."
				"DA_BattleEnemyPartTargetPreviewStyle_PixelLock")));
	if (!TestNotNull(TEXT("Target preview style"), PreviewStyle.Get()))
	{
		return false;
	}
	PreviewStyle->MinimumAxisSizeCentimeters = 1.0f;
	PreviewStyle->MaximumAxisSizeCentimeters = 10000.0f;
	PreviewStyle->ValidCoverageMultiplier = 1.25f;
	PreviewStyle->AvailabilityIconSizeMultiplier = 0.25f;
	PreviewStyle->MinimumAvailabilityIconSizeCentimeters = 1.0f;
	PreviewStyle->MaximumAvailabilityIconSizeCentimeters = 10000.0f;
	PreviewStyle->CameraDepthOffsetCentimeters = 0.0f;

	FWacomBattleEnemyPartTargetPreviewPlaybackView PlaybackView;
	PlaybackView.bActive = true;
	PlaybackView.Kind = EWacomBattleEnemyPartTargetPreviewKind::Valid;
	PlaybackView.Phase = EWacomBattleEnemyPartTargetPreviewPhase::Holding;
	PlaybackView.Amount = 1.0f;

	FVector PlaneRight;
	FVector PlaneUp;
	ResolveCameraPlane(World, PlaneRight, PlaneUp);
	const FVector2D ExpectedPreviewSize =
		Bounds.ProjectSizeCentimeters(PlaneRight, PlaneUp) * 1.25f;

	FWacomBattleEnemyPartTargetPreviewFeedbackController PreviewController;
	TestTrue(TEXT("Target preview starts from presentation bounds"),
		PreviewController.BeginOrUpdate(
			*Runtime,
			PartAttachment,
			Bounds,
			PreviewStyle.Get(),
			PlaybackView,
			1.0f));
	const FWacomBattleEnemyPartTargetPreviewFeedbackDebugView& PreviewDebug =
		PreviewController.GetDebugView();
	TestTrue(TEXT("Target frame center stays on visual bounds"),
		PreviewDebug.TargetWorldCenter.Equals(Bounds.GetWorldCenter()));
	TestTrue(TEXT("Presentation debug retains the un-offset visual center"),
		PreviewDebug.PresentationBoundsWorldCenter.Equals(
			Bounds.GetWorldCenter()));
	TestFalse(TEXT("Target frame center does not use ImpactAnchor"),
		PreviewDebug.TargetWorldCenter.Equals(ImpactAnchor->GetComponentLocation()));
	TestTrue(TEXT("Target frame applies coverage to oriented projection"),
		PreviewDebug.TargetSizeCentimeters.Equals(ExpectedPreviewSize, 0.01f));
	TestEqual(TEXT("Target frame diagnoses presentation source"),
		PreviewDebug.PresentationBoundsSource,
		FName(TEXT("StableInteractionVisual")));

	PlaybackView.Kind = EWacomBattleEnemyPartTargetPreviewKind::Available;
	PreviewStyle->MinimumAvailabilityIconSizeCentimeters = 18.0f;
	PreviewStyle->MaximumAvailabilityIconSizeCentimeters = 24.0f;
	TestTrue(TEXT("Availability marker reuses the same stable bounds"),
		PreviewController.BeginOrUpdate(
			*Runtime,
			PartAttachment,
			Bounds,
			PreviewStyle.Get(),
			PlaybackView,
			1.0f));
	const float ExpectedAvailabilityIconSize = FMath::Clamp(
		Bounds.ProjectShorterAxisCentimeters(PlaneRight, PlaneUp)
			* PreviewStyle->AvailabilityIconSizeMultiplier,
		18.0f,
		24.0f);
	TestTrue(TEXT("Availability icon size uses stable short axis and clamps"),
		FMath::IsNearlyEqual(
			PreviewController.GetDebugView().AvailabilityIconSizeCentimeters,
			ExpectedAvailabilityIconSize,
			0.01f));

	PlaybackView.Kind = EWacomBattleEnemyPartTargetPreviewKind::Invalid;
	PreviewStyle->InvalidCoverageMultiplier = 10.0f;
	PreviewStyle->MinimumAxisSizeCentimeters = 45.0f;
	PreviewStyle->MaximumAxisSizeCentimeters = 70.0f;
	TestTrue(TEXT("Invalid target frame updates from the same bounds"),
		PreviewController.BeginOrUpdate(
			*Runtime,
			PartAttachment,
			Bounds,
			PreviewStyle.Get(),
			PlaybackView,
			1.0f));
	TestTrue(TEXT("Target width obeys the per-axis maximum clamp"),
		FMath::IsNearlyEqual(
			PreviewController.GetDebugView().TargetSizeCentimeters.X,
			70.0f));
	TestTrue(TEXT("Target height obeys the per-axis maximum clamp"),
		FMath::IsNearlyEqual(
			PreviewController.GetDebugView().TargetSizeCentimeters.Y,
			70.0f));

	TArray<UNiagaraComponent*> NiagaraComponents;
	Host->GetComponents(NiagaraComponents);
	UNiagaraComponent* const* PreviewMatch =
		NiagaraComponents.FindByPredicate([](const UNiagaraComponent* Component)
		{
			return Component
				&& Component->GetName().StartsWith(
					TEXT("WacomEnemyPartTargetPreviewNiagara"));
		});
	const UNiagaraComponent* PreviewNiagara =
		PreviewMatch ? *PreviewMatch : nullptr;
	if (TestNotNull(TEXT("Target preview Niagara exists"), PreviewNiagara))
	{
		TestTrue(TEXT("Target preview attaches to Part lifecycle parent"),
			PreviewNiagara->GetAttachParent() == PartAttachment);
	}

	TStrongObjectPtr<UWacomBattleEnemyPartImpactStyle> ImpactStyle(
		DuplicateStyle<UWacomBattleEnemyPartImpactStyle>(
			TEXT("/Game/Wacom/UI/Battle/WorldImpact/"
				"DA_BattleEnemyPartImpactStyle_Pixel."
				"DA_BattleEnemyPartImpactStyle_Pixel")));
	if (!TestNotNull(TEXT("Impact style"), ImpactStyle.Get()))
	{
		PreviewController.ResetImmediate(true);
		return false;
	}
	ImpactStyle->DamageCoverageMultiplier = 1.5f;
	ImpactStyle->MinimumImpactDiameterCentimeters = 1.0f;
	ImpactStyle->MaximumImpactDiameterCentimeters = 10000.0f;
	ImpactStyle->CameraDepthOffsetCentimeters = 0.0f;
	ImpactStyle->DamageSound = nullptr;

	FWacomBattlePresentationTargetCue Cue;
	Cue.Duration = 0.2f;
	Cue.Amount = 8;
	Cue.Seed = 37;
	FWacomBattleEnemyPartImpactFeedbackController ImpactController;
	TestTrue(TEXT("Impact feedback starts from authored anchor"),
		ImpactController.PlayAcceptedCue(
			*Runtime,
			ImpactAnchor,
			Bounds,
			ImpactStyle.Get(),
			EWacomBattleEnemyPartCuePlaybackKind::Damage,
			Cue,
			1.0f,
			false));
	const FWacomBattleEnemyPartImpactFeedbackDebugView& ImpactDebug =
		ImpactController.GetDebugView();
	TestTrue(TEXT("Impact effect remains at authored ImpactAnchor"),
		ImpactDebug.LastEffectWorldLocation.Equals(
			ImpactAnchor->GetComponentLocation()));
	TestFalse(TEXT("Impact effect does not move to target-frame center"),
		ImpactDebug.LastEffectWorldLocation.Equals(Bounds.GetWorldCenter()));
	TestTrue(TEXT("Impact diameter comes from stable presentation bounds"),
		FMath::IsNearlyEqual(
			ImpactDebug.LastTargetDiameterCentimeters,
			Bounds.ProjectDiameterCentimeters(PlaneRight, PlaneUp) * 1.5f,
			0.01f));
	TestEqual(TEXT("Impact diagnoses the same presentation source"),
		ImpactDebug.PresentationBoundsSource,
		FName(TEXT("StableInteractionVisual")));

	ImpactStyle->MinimumImpactDiameterCentimeters = 77.0f;
	ImpactStyle->MaximumImpactDiameterCentimeters = 77.0f;
	TestTrue(TEXT("Missing authored anchor can use the Part fallback parent"),
		ImpactController.PlayAcceptedCue(
			*Runtime,
			PartAttachment,
			Bounds,
			ImpactStyle.Get(),
			EWacomBattleEnemyPartCuePlaybackKind::Destroyed,
			Cue,
			1.0f,
			false));
	TestTrue(TEXT("Part fallback keeps impact at the Part origin"),
		ImpactController.GetDebugView().LastEffectWorldLocation.Equals(
			PartAttachment->GetComponentLocation()));
	TestTrue(TEXT("Impact diameter obeys the configured clamp"),
		FMath::IsNearlyEqual(
			ImpactController.GetDebugView().LastTargetDiameterCentimeters,
			77.0f));

	ImpactController.ResetImmediate(true);
	PreviewController.ResetImmediate(true);
	return true;
}
