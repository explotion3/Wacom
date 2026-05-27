// Copyright Wacom. All Rights Reserved.

#include "Components/WacomFirstPersonCardAnchorComponent.h"

#include "Actors/WacomRunTunnelSegmentActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/WacomBattleCameraLookComponent.h"
#include "Components/WacomCursorLookDriverComponent.h"
#include "Components/WacomRunTunnelMovementComponent.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "Cards/CardDefinition.h"
#include "UI/Card/WacomCardPresentationBuilder.h"
#include "UI/Card/WacomFirstPersonCardAnchorDebugWidget.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"

namespace
{
	const FName NoOwnerReason(TEXT("NoOwner"));
	const FName NoPlayerControllerReason(TEXT("NoPlayerController"));
	const FName NoCameraManagerReason(TEXT("NoCameraManager"));
	const FName CameraFallbackReason(TEXT("CameraFallback"));
}

UWacomFirstPersonCardAnchorComponent::UWacomFirstPersonCardAnchorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UWacomFirstPersonCardAnchorComponent::BeginPlay()
{
	Super::BeginPlay();
	SetComponentTickEnabled(true);
}

void UWacomFirstPersonCardAnchorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveStaticCardLayer();
	RemoveDebugWidget();
	Super::EndPlay(EndPlayReason);
}

void UWacomFirstPersonCardAnchorComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	RefreshAnchor(DeltaTime);
	UpdateDebugWidget();
	UpdateStaticCardLayer();
}

void UWacomFirstPersonCardAnchorComponent::RefreshAnchor(float DeltaTime)
{
	FTransform BaseTransform = FTransform::Identity;
	EWacomFirstPersonCardAnchorMode ResolvedMode = EWacomFirstPersonCardAnchorMode::Invalid;
	FName ResolvedFallbackReason = NAME_None;
	if (!ResolveBaseAnchor(BaseTransform, ResolvedMode, ResolvedFallbackReason))
	{
		bHasValidAnchor = false;
		CurrentMode = ResolvedMode;
		CurrentLookOffsetUsed = FRotator::ZeroRotator;
		LastFallbackReason = ResolvedFallbackReason;
		return;
	}

	FRotator LookOffset = FRotator::ZeroRotator;
	if (const AWacomPlayerCharacter* Character = GetOwnerCharacter())
	{
		if (const UWacomCursorLookDriverComponent* CursorLook = Character->GetCursorLookDriverComponent())
		{
			const FRotator SharedOffset = CursorLook->GetCurrentLookOffset();
			LookOffset.Pitch = SharedOffset.Pitch * LookInfluencePitch;
			LookOffset.Yaw = SharedOffset.Yaw * LookInfluenceYaw;
		}
	}

	FRotator AnchorRotation = BaseTransform.Rotator();
	AnchorRotation.Pitch += LookOffset.Pitch;
	AnchorRotation.Yaw += LookOffset.Yaw;
	AnchorRotation.Roll = 0.0f;

	const FRotationMatrix AnchorRotationMatrix(AnchorRotation);
	const FVector AnchorLocation =
		BaseTransform.GetLocation()
		+ AnchorRotationMatrix.GetScaledAxis(EAxis::X) * FMath::Max(0.0f, DistanceFromView)
		+ AnchorRotationMatrix.GetScaledAxis(EAxis::Y) * HorizontalOffset
		+ AnchorRotationMatrix.GetScaledAxis(EAxis::Z) * VerticalOffset;
	const FTransform TargetAnchorTransform(AnchorRotation, AnchorLocation, FVector::OneVector);

	if (!bHasInitializedAnchor || FollowInterpSpeed <= 0.0f || DeltaTime <= 0.0f)
	{
		CurrentAnchorTransform = TargetAnchorTransform;
	}
	else
	{
		const FVector SmoothedLocation = FMath::VInterpTo(
			CurrentAnchorTransform.GetLocation(),
			TargetAnchorTransform.GetLocation(),
			DeltaTime,
			FollowInterpSpeed);
		const FRotator SmoothedRotation = FMath::RInterpTo(
			CurrentAnchorTransform.Rotator(),
			TargetAnchorTransform.Rotator(),
			DeltaTime,
			FollowInterpSpeed);
		CurrentAnchorTransform = FTransform(SmoothedRotation, SmoothedLocation, FVector::OneVector);
	}

	bHasInitializedAnchor = true;
	bHasValidAnchor = true;
	CurrentMode = ResolvedMode;
	CurrentLookOffsetUsed = LookOffset;
	LastFallbackReason = ResolvedFallbackReason;
}

FTransform UWacomFirstPersonCardAnchorComponent::ComputeCardTransform(int32 NumCards, int32 CardIndex) const
{
	if (!bHasValidAnchor || NumCards <= 0 || CardIndex < 0 || CardIndex >= NumCards)
	{
		return CurrentAnchorTransform;
	}

	const float CenterOffset =
		static_cast<float>(CardIndex) - (static_cast<float>(NumCards - 1) * 0.5f);
	const FRotator AnchorRotation = CurrentAnchorTransform.Rotator();
	const FRotationMatrix AnchorRotationMatrix(AnchorRotation);
	const FVector CardLocation =
		CurrentAnchorTransform.GetLocation()
		+ AnchorRotationMatrix.GetScaledAxis(EAxis::Y) * (CenterOffset * FMath::Max(0.0f, CardSpacing));

	FRotator CardRotation = (AnchorRotation + FRotator(0.0f, 180.0f, 0.0f)).GetNormalized();
	CardRotation.Yaw += CenterOffset * FanYawDegrees;
	return FTransform(CardRotation, CardLocation, FVector::OneVector);
}

bool UWacomFirstPersonCardAnchorComponent::ProjectCardTransformToScreen(
	const FTransform& CardTransform,
	FWacomFirstPersonCardProjectedPoint& OutProjectedPoint,
	int32 PointIndex) const
{
	OutProjectedPoint = FWacomFirstPersonCardProjectedPoint();
	OutProjectedPoint.Index = PointIndex;
	OutProjectedPoint.WorldLocation = CardTransform.GetLocation();

	FVector2D ScreenPosition = FVector2D::ZeroVector;
	if (!ProjectWorldLocationForAnchor(CardTransform.GetLocation(), ScreenPosition))
	{
		return false;
	}

	FVector2D ViewportSize = FVector2D::ZeroVector;
	if (!GetViewportSizeForAnchor(ViewportSize)
		|| ViewportSize.X <= 0.0f
		|| ViewportSize.Y <= 0.0f)
	{
		return false;
	}

	const FVector2D UnclampedPosition = ScreenPosition;
	const float Padding = FMath::Max(0.0f, ProjectionPadding);
	ScreenPosition.X = FMath::Clamp(ScreenPosition.X, Padding, FMath::Max(Padding, ViewportSize.X - Padding));
	ScreenPosition.Y = FMath::Clamp(ScreenPosition.Y, Padding, FMath::Max(Padding, ViewportSize.Y - Padding));

	OutProjectedPoint.ScreenPosition = ScreenPosition;
	OutProjectedPoint.bProjected = true;
	OutProjectedPoint.bClamped = !UnclampedPosition.Equals(ScreenPosition, KINDA_SMALL_NUMBER);
	return true;
}

bool UWacomFirstPersonCardAnchorComponent::ResolveCameraTransformForAnchor(FTransform& OutCameraTransform) const
{
	const APlayerController* PC = GetOwnerPlayerController();
	if (!PC || !PC->PlayerCameraManager)
	{
		return false;
	}

	OutCameraTransform = FTransform(
		PC->PlayerCameraManager->GetCameraRotation(),
		PC->PlayerCameraManager->GetCameraLocation(),
		FVector::OneVector);
	return true;
}

bool UWacomFirstPersonCardAnchorComponent::ProjectWorldLocationForAnchor(
	const FVector& WorldLocation,
	FVector2D& OutScreenPosition) const
{
	APlayerController* PC = GetOwnerPlayerController();
	return PC && PC->ProjectWorldLocationToScreen(WorldLocation, OutScreenPosition, false);
}

bool UWacomFirstPersonCardAnchorComponent::GetViewportSizeForAnchor(FVector2D& OutViewportSize) const
{
	const APlayerController* PC = GetOwnerPlayerController();
	if (!PC)
	{
		return false;
	}

	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	PC->GetViewportSize(ViewportSizeX, ViewportSizeY);
	OutViewportSize = FVector2D(ViewportSizeX, ViewportSizeY);
	return ViewportSizeX > 0 && ViewportSizeY > 0;
}

bool UWacomFirstPersonCardAnchorComponent::CanCreateStaticCardLayerForAnchor(
	APlayerController* PlayerController) const
{
	return PlayerController && PlayerController->IsLocalController();
}

UWacomFirstPersonCardLayerWidget* UWacomFirstPersonCardAnchorComponent::CreateStaticCardLayerWidgetForAnchor(
	APlayerController* PlayerController,
	TSubclassOf<UWacomFirstPersonCardLayerWidget> LayerClass) const
{
	if (!PlayerController || !LayerClass)
	{
		return nullptr;
	}

	return CreateWidget<UWacomFirstPersonCardLayerWidget>(PlayerController, LayerClass);
}

void UWacomFirstPersonCardAnchorComponent::AddStaticCardLayerWidgetToViewportForAnchor(
	UWacomFirstPersonCardLayerWidget* LayerWidget,
	int32 ZOrder) const
{
	if (LayerWidget)
	{
		LayerWidget->AddToViewport(ZOrder);
	}
}

FWacomFirstPersonCardAnchorDebugView UWacomFirstPersonCardAnchorComponent::GetFirstPersonCardAnchorDebugView(
	int32 NumDebugCards) const
{
	FWacomFirstPersonCardAnchorDebugView View;
	View.bHasValidAnchor = bHasValidAnchor;
	View.Mode = CurrentMode;
	View.AnchorTransform = CurrentAnchorTransform;
	View.LookOffsetUsed = CurrentLookOffsetUsed;
	View.LastFallbackReason = LastFallbackReason;

	if (!bHasValidAnchor)
	{
		return View;
	}

	const int32 ClampedCount = FMath::Clamp(NumDebugCards, 0, 32);
	View.ProjectedPoints.Reserve(ClampedCount);
	for (int32 Index = 0; Index < ClampedCount; ++Index)
	{
		FWacomFirstPersonCardProjectedPoint Point;
		ProjectCardTransformToScreen(ComputeCardTransform(ClampedCount, Index), Point, Index);
		View.ProjectedPoints.Add(Point);
	}
	return View;
}

TArray<FWacomFirstPersonStaticCardSlotView> UWacomFirstPersonCardAnchorComponent::BuildStaticCardSlotViews() const
{
	TArray<FWacomFirstPersonStaticCardSlotView> Slots;
	if (!bHasValidAnchor)
	{
		return Slots;
	}

	const int32 DesiredCount = StaticPreviewCardDefinitions.Num() > 0
		? StaticPreviewCardDefinitions.Num()
		: StaticCardCountFallback;
	const int32 ClampedCount = FMath::Clamp(DesiredCount, 0, 32);
	Slots.Reserve(ClampedCount);

	for (int32 Index = 0; Index < ClampedCount; ++Index)
	{
		FWacomFirstPersonStaticCardSlotView Slot;
		Slot.Index = Index;
		Slot.CardViewData = BuildStaticCardViewData(Index);
		Slot.RenderScale = FMath::Max(0.01f, StaticCardRenderScale);

		const float CenterOffset =
			static_cast<float>(Index) - (static_cast<float>(ClampedCount - 1) * 0.5f);
		Slot.RenderAngleDegrees = CenterOffset * FanYawDegrees;

		FWacomFirstPersonCardProjectedPoint Point;
		if (ProjectCardTransformToScreen(ComputeCardTransform(ClampedCount, Index), Point, Index))
		{
			Slot.ScreenPosition = Point.ScreenPosition;
			const float MaxAbsCenterOffset = FMath::Max(1.0f, static_cast<float>(ClampedCount - 1) * 0.5f);
			const float NormalizedEdgeDistance = FMath::Abs(CenterOffset) / MaxAbsCenterOffset;
			Slot.ScreenPosition.Y += FMath::Square(NormalizedEdgeDistance) * FMath::Max(0.0f, StaticCardEdgeDropPixels);
			Slot.bProjected = Point.bProjected;
			Slot.bClamped = Point.bClamped;
		}

		Slots.Add(Slot);
	}
	return Slots;
}

FString UWacomFirstPersonCardAnchorComponent::GetDebugSummary() const
{
	return FString::Printf(
		TEXT("FirstPersonCardAnchor Mode=%s Valid=%s Anchor=%s LookOffset=%s Fallback=%s"),
		*AnchorModeToString(CurrentMode),
		bHasValidAnchor ? TEXT("true") : TEXT("false"),
		*CurrentAnchorTransform.ToHumanReadableString(),
		*CurrentLookOffsetUsed.ToString(),
		*LastFallbackReason.ToString());
}

AWacomPlayerCharacter* UWacomFirstPersonCardAnchorComponent::GetOwnerCharacter() const
{
	return Cast<AWacomPlayerCharacter>(GetOwner());
}

APlayerController* UWacomFirstPersonCardAnchorComponent::GetOwnerPlayerController() const
{
	const AWacomPlayerCharacter* Character = GetOwnerCharacter();
	return Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
}

bool UWacomFirstPersonCardAnchorComponent::ResolveBaseAnchor(
	FTransform& OutBaseTransform,
	EWacomFirstPersonCardAnchorMode& OutMode,
	FName& OutFallbackReason) const
{
	const AWacomPlayerCharacter* Character = GetOwnerCharacter();
	if (!Character)
	{
		OutMode = EWacomFirstPersonCardAnchorMode::Invalid;
		OutFallbackReason = NoOwnerReason;
		return false;
	}

	if (const UWacomBattleCameraLookComponent* BattleCamera = Character->GetBattleCameraLookComponent())
	{
		if (BattleCamera->IsBattleCameraLookActive())
		{
			if (!GetOwnerPlayerController())
			{
				OutMode = EWacomFirstPersonCardAnchorMode::Invalid;
				OutFallbackReason = NoPlayerControllerReason;
				return false;
			}

			FTransform CameraTransform = FTransform::Identity;
			if (!ResolveCameraTransformForAnchor(CameraTransform))
			{
				OutMode = EWacomFirstPersonCardAnchorMode::Invalid;
				OutFallbackReason = NoCameraManagerReason;
				return false;
			}

			OutBaseTransform = FTransform(
				BattleCamera->GetBaseBattleRotation(),
				CameraTransform.GetLocation(),
				FVector::OneVector);
			OutMode = EWacomFirstPersonCardAnchorMode::BattleCamera;
			OutFallbackReason = NAME_None;
			return true;
		}
	}

	if (const UWacomRunTunnelMovementComponent* RunTunnel = Character->GetRunTunnelMovementComponent())
	{
		if (RunTunnel->IsRunTunnelActive())
		{
			if (const AWacomRunTunnelSegmentActor* Segment = RunTunnel->GetActiveSegment())
			{
				OutBaseTransform = Segment->GetSplineTransformAtDistance(RunTunnel->GetDistanceAlongSpline());
				OutMode = EWacomFirstPersonCardAnchorMode::RunTunnel;
				OutFallbackReason = NAME_None;
				return true;
			}
		}
	}

	if (!GetOwnerPlayerController())
	{
		OutMode = EWacomFirstPersonCardAnchorMode::Invalid;
		OutFallbackReason = NoPlayerControllerReason;
		return false;
	}
	if (!ResolveCameraTransformForAnchor(OutBaseTransform))
	{
		OutMode = EWacomFirstPersonCardAnchorMode::Invalid;
		OutFallbackReason = NoCameraManagerReason;
		return false;
	}

	OutMode = EWacomFirstPersonCardAnchorMode::CameraFallback;
	OutFallbackReason = CameraFallbackReason;
	return true;
}

FWacomCardViewData UWacomFirstPersonCardAnchorComponent::BuildStaticCardViewData(int32 CardIndex) const
{
	if (StaticPreviewCardDefinitions.IsValidIndex(CardIndex))
	{
		const TSoftObjectPtr<UCardDefinition>& CardPtr = StaticPreviewCardDefinitions[CardIndex];
		if (const UCardDefinition* Card = CardPtr.LoadSynchronous())
		{
			return UWacomCardPresentationBuilder::BuildCardViewData(Card);
		}
	}

	FWacomCardViewData Data;
	Data.Name = FText::Format(
		NSLOCTEXT("Wacom.FirstPersonCardLayer", "StaticPlaceholderName", "Anchor Card {0}"),
		FText::AsNumber(CardIndex + 1));
	Data.TypeText = NSLOCTEXT("Wacom.FirstPersonCardLayer", "StaticPlaceholderType", "Static Preview");
	Data.Description = NSLOCTEXT(
		"Wacom.FirstPersonCardLayer",
		"StaticPlaceholderDescription",
		"HUD card view driven by the first-person anchor.");
	Data.Cost = CardIndex;
	Data.bShowCost = true;
	Data.Value = CardIndex + 1;
	Data.bShowValue = true;
	return Data;
}

void UWacomFirstPersonCardAnchorComponent::UpdateDebugWidget()
{
	if (!bDrawDebugProjection)
	{
		RemoveDebugWidget();
		return;
	}

	APlayerController* PC = GetOwnerPlayerController();
	if (!PC || !PC->IsLocalController())
	{
		RemoveDebugWidget();
		return;
	}

	if (!DebugWidget)
	{
		DebugWidget = CreateWidget<UWacomFirstPersonCardAnchorDebugWidget>(
			PC,
			UWacomFirstPersonCardAnchorDebugWidget::StaticClass());
		if (DebugWidget)
		{
			DebugWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
			DebugWidget->AddToViewport(DebugWidgetZOrder);
		}
	}

	if (DebugWidget)
	{
		DebugWidget->SetDebugView(GetFirstPersonCardAnchorDebugView(5));
	}
}

void UWacomFirstPersonCardAnchorComponent::UpdateStaticCardLayer()
{
	if (!bDrawStaticCardLayer)
	{
		RemoveStaticCardLayer();
		return;
	}

	APlayerController* PC = GetOwnerPlayerController();
	if (!CanCreateStaticCardLayerForAnchor(PC))
	{
		RemoveStaticCardLayer();
		return;
	}

	if (!StaticCardLayerWidget)
	{
		TSubclassOf<UWacomFirstPersonCardLayerWidget> LayerClass = StaticCardLayerWidgetClass;
		if (!LayerClass)
		{
			LayerClass = UWacomFirstPersonCardLayerWidget::StaticClass();
		}

		StaticCardLayerWidget = CreateStaticCardLayerWidgetForAnchor(PC, LayerClass);
		if (StaticCardLayerWidget)
		{
			StaticCardLayerWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
			StaticCardLayerWidget->SetCardViewClass(StaticCardViewClass);
			AddStaticCardLayerWidgetToViewportForAnchor(StaticCardLayerWidget, StaticCardLayerZOrder);
		}
	}

	if (StaticCardLayerWidget)
	{
		StaticCardLayerWidget->SetCardViewClass(StaticCardViewClass);
		StaticCardLayerWidget->SetStaticCardSlots(BuildStaticCardSlotViews());
	}
}

void UWacomFirstPersonCardAnchorComponent::RemoveDebugWidget()
{
	if (DebugWidget)
	{
		DebugWidget->RemoveFromParent();
		DebugWidget = nullptr;
	}
}

void UWacomFirstPersonCardAnchorComponent::RemoveStaticCardLayer()
{
	if (StaticCardLayerWidget)
	{
		StaticCardLayerWidget->RemoveFromParent();
		StaticCardLayerWidget = nullptr;
	}
}

FString UWacomFirstPersonCardAnchorComponent::AnchorModeToString(EWacomFirstPersonCardAnchorMode Mode)
{
	switch (Mode)
	{
	case EWacomFirstPersonCardAnchorMode::BattleCamera:
		return TEXT("BattleCamera");
	case EWacomFirstPersonCardAnchorMode::RunTunnel:
		return TEXT("RunTunnel");
	case EWacomFirstPersonCardAnchorMode::CameraFallback:
		return TEXT("CameraFallback");
	case EWacomFirstPersonCardAnchorMode::Invalid:
	default:
		return TEXT("Invalid");
	}
}
