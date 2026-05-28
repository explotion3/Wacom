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
#include "Blueprint/WidgetLayoutLibrary.h"
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
	ConfigureTickPrerequisites();
	SetComponentTickEnabled(true);
}

void UWacomFirstPersonCardAnchorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetBattleHandInteractionPrototypeEnabled(false);
	ResetAnchorScreenSmoothing();
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
		bCurrentLookOffsetAppliedToLayout = false;
		LastFallbackReason = ResolvedFallbackReason;
		ResetAnchorScreenSmoothing();
		return;
	}

	FRotator LookOffset = FRotator::ZeroRotator;
	if (ProjectionMode == EWacomFirstPersonCardProjectionMode::LegacyWorldProjected)
	{
		if (const AWacomPlayerCharacter* Character = GetOwnerCharacter())
		{
			if (const UWacomCursorLookDriverComponent* CursorLook = Character->GetCursorLookDriverComponent())
			{
				const FRotator SharedOffset = CursorLook->GetCurrentLookOffset();
				LookOffset.Pitch = SharedOffset.Pitch * LookInfluencePitch;
				LookOffset.Yaw = SharedOffset.Yaw * LookInfluenceYaw;
			}
		}
	}

	FRotator AnchorRotation = BaseTransform.Rotator();
	AnchorRotation.Roll = 0.0f;
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
	bCurrentLookOffsetAppliedToLayout = !LookOffset.IsNearlyZero();
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
	OutProjectedPoint.ProjectionMode = ProjectionMode;
	OutProjectedPoint.LayoutMode = CardLayoutMode;
	OutProjectedPoint.bBodyLockedLayout = ProjectionMode == EWacomFirstPersonCardProjectionMode::BodyLocked;
	OutProjectedPoint.bCurrentCameraProjection = true;
	OutProjectedPoint.bLookOffsetAppliedToLayout = bCurrentLookOffsetAppliedToLayout;

	FVector2D WidgetPosition = FVector2D::ZeroVector;
	FVector2D RawScreenPosition = FVector2D::ZeroVector;
	const bool bProjectionSucceeded = ProjectWorldLocationToWidgetPositionForAnchor(
		CardTransform.GetLocation(),
		WidgetPosition,
		RawScreenPosition);
	if (!bProjectionSucceeded)
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

	const float ViewportScale = FMath::Max(0.01f, GetViewportScaleForAnchor());
	const FVector2D WidgetViewportSize = ViewportSize / ViewportScale;
	const FVector2D UnclampedPosition = WidgetPosition;
	bool bClamped = false;
	bool bOutsideViewport = false;
	float OffscreenDistancePixels = 0.0f;
	WidgetPosition = ApplyViewportClampToWidgetPosition(
		UnclampedPosition,
		WidgetViewportSize,
		bClamped,
		bOutsideViewport,
		OffscreenDistancePixels);

	bool bPixelSnapped = false;
	const FVector2D SnappedPosition = SnapCardLayerPosition(WidgetPosition, bPixelSnapped);

	OutProjectedPoint.RawScreenPosition = RawScreenPosition;
	OutProjectedPoint.WidgetPosition = WidgetPosition;
	OutProjectedPoint.UnclampedWidgetPosition = UnclampedPosition;
	OutProjectedPoint.SnappedWidgetPosition = SnappedPosition;
	OutProjectedPoint.ScreenPosition = SnappedPosition;
	OutProjectedPoint.ViewportClampMode = ViewportClampMode;
	OutProjectedPoint.ViewportScale = ViewportScale;
	OutProjectedPoint.OffscreenDistancePixels = OffscreenDistancePixels;
	OutProjectedPoint.UnsmoothedAnchorWidgetPosition = WidgetPosition;
	OutProjectedPoint.SmoothedAnchorWidgetPosition = WidgetPosition;
	OutProjectedPoint.bProjected = true;
	OutProjectedPoint.bClamped = bClamped;
	OutProjectedPoint.bOutsideViewport = bOutsideViewport;
	OutProjectedPoint.bPixelSnapped = bPixelSnapped;
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

bool UWacomFirstPersonCardAnchorComponent::ProjectWorldLocationToWidgetPositionForAnchor(
	const FVector& WorldLocation,
	FVector2D& OutWidgetPosition,
	FVector2D& OutRawScreenPosition) const
{
	APlayerController* PC = GetOwnerPlayerController();
	if (!PC)
	{
		return false;
	}

	if (!ProjectWorldLocationForAnchor(WorldLocation, OutRawScreenPosition))
	{
		return false;
	}

	return UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
		PC,
		WorldLocation,
		OutWidgetPosition,
		false);
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

float UWacomFirstPersonCardAnchorComponent::GetViewportScaleForAnchor() const
{
	APlayerController* PC = GetOwnerPlayerController();
	if (!PC)
	{
		return 1.0f;
	}

	const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(PC);
	return ViewportScale > 0.0f ? ViewportScale : 1.0f;
}

float UWacomFirstPersonCardAnchorComponent::GetAnchorSmoothingDeltaTimeForAnchor() const
{
	const UWorld* World = GetWorld();
	return World ? FMath::Max(0.0f, World->GetDeltaSeconds()) : 0.0f;
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
	View.ProjectionMode = ProjectionMode;
	View.LayoutMode = CardLayoutMode;
	View.ViewportClampMode = ViewportClampMode;
	View.LookOffsetUsed = CurrentLookOffsetUsed;
	View.LastFallbackReason = LastFallbackReason;
	View.bBodyLockedLayout = ProjectionMode == EWacomFirstPersonCardProjectionMode::BodyLocked;
	View.bCurrentCameraProjection = true;
	View.bLookOffsetAppliedToLayout = bCurrentLookOffsetAppliedToLayout;
	View.bAnchorScreenSmoothed = bLastAnchorScreenSmoothed;
	if (StaticCardLayerWidget)
	{
		View.LayerMotionDebugView = StaticCardLayerWidget->GetSlotMotionDebugView();
	}

	if (!bHasValidAnchor)
	{
		return View;
	}

	const int32 ClampedCount = FMath::Clamp(NumDebugCards, 0, 32);
	View.ProjectedPoints.Reserve(ClampedCount);
	if (CardLayoutMode == EWacomFirstPersonCardLayoutMode::Authored2D)
	{
		TArray<FWacomFirstPersonCardLayerEntry> DebugEntries;
		DebugEntries.SetNum(ClampedCount);
		const TArray<FWacomFirstPersonCardLayerSlotView> DebugSlots = BuildCardSlotViewsFromEntries(DebugEntries);
		for (const FWacomFirstPersonCardLayerSlotView& Slot : DebugSlots)
		{
			FWacomFirstPersonCardProjectedPoint Point;
			Point.Index = Slot.Index;
			Point.WorldLocation = CurrentAnchorTransform.GetLocation();
			Point.RawScreenPosition = Slot.RawScreenPosition;
			Point.WidgetPosition = Slot.WidgetPosition;
			Point.UnclampedWidgetPosition = Slot.UnclampedWidgetPosition;
			Point.SnappedWidgetPosition = Slot.SnappedWidgetPosition;
			Point.ScreenPosition = Slot.ScreenPosition;
			Point.ProjectionMode = Slot.ProjectionMode;
			Point.LayoutMode = Slot.LayoutMode;
			Point.ViewportClampMode = Slot.ViewportClampMode;
			Point.AnchorWidgetPosition = Slot.AnchorWidgetPosition;
			Point.UnsmoothedAnchorWidgetPosition = Slot.UnsmoothedAnchorWidgetPosition;
			Point.SmoothedAnchorWidgetPosition = Slot.SmoothedAnchorWidgetPosition;
			Point.AuthoredLayoutOffset = Slot.AuthoredLayoutOffset;
			Point.NormalizedHandOffset = Slot.NormalizedHandOffset;
			Point.ViewportScale = Slot.ViewportScale;
			Point.OffscreenDistancePixels = Slot.OffscreenDistancePixels;
			Point.AnchorScreenSmoothingDistancePixels = Slot.AnchorScreenSmoothingDistancePixels;
			Point.bProjected = Slot.bProjected;
			Point.bClamped = Slot.bClamped;
			Point.bOutsideViewport = Slot.bOutsideViewport;
			Point.bPixelSnapped = Slot.bPixelSnapped;
			Point.bAnchorScreenSmoothed = Slot.bAnchorScreenSmoothed;
			Point.bBodyLockedLayout = Slot.bBodyLockedLayout;
			Point.bCurrentCameraProjection = Slot.bCurrentCameraProjection;
			Point.bLookOffsetAppliedToLayout = Slot.bLookOffsetAppliedToLayout;
			View.ProjectedPoints.Add(Point);
		}
		View.bAnchorScreenSmoothed = bLastAnchorScreenSmoothed;
		return View;
	}

	for (int32 Index = 0; Index < ClampedCount; ++Index)
	{
		FWacomFirstPersonCardProjectedPoint Point;
		ProjectCardTransformToScreen(ComputeCardTransform(ClampedCount, Index), Point, Index);
		View.ProjectedPoints.Add(Point);
	}
	return View;
}

TArray<FWacomFirstPersonCardLayerSlotView> UWacomFirstPersonCardAnchorComponent::BuildStaticCardSlotViews() const
{
	return BuildCardSlotViewsFromEntries(BuildStaticCardLayerEntries());
}

void UWacomFirstPersonCardAnchorComponent::SetRuntimeCardLayerEntries(
	FName SourceId,
	const TArray<FWacomFirstPersonCardLayerEntry>& Entries)
{
	if (SourceId.IsNone())
	{
		return;
	}

	RuntimeCardLayerSourceId = SourceId;
	RuntimeCardLayerEntries.Reset(Entries.Num());
	RuntimeCardLayerData.Reset(Entries.Num());
	for (FWacomFirstPersonCardLayerEntry Entry : Entries)
	{
		Entry.bIsPlayable = Entry.bIsPlayable && !Entry.CardViewData.bDisabled;
		Entry.CardViewData.bDisabled = !Entry.bIsPlayable;
		RuntimeCardLayerEntries.Add(Entry);
		RuntimeCardLayerData.Add(Entry.CardViewData);
	}
	bHasRuntimeCardLayerData = true;
}

void UWacomFirstPersonCardAnchorComponent::SetRuntimeCardLayerData(
	FName SourceId,
	const TArray<FWacomCardViewData>& Cards)
{
	SetRuntimeCardLayerEntries(SourceId, BuildCardLayerEntriesFromData(Cards));
}

void UWacomFirstPersonCardAnchorComponent::ClearRuntimeCardLayerData(FName SourceId)
{
	if (SourceId.IsNone() || RuntimeCardLayerSourceId != SourceId)
	{
		return;
	}

	RuntimeCardLayerSourceId = NAME_None;
	RuntimeCardLayerData.Reset();
	RuntimeCardLayerEntries.Reset();
	HoveredCardInstanceId.Invalidate();
	bHasRuntimeCardLayerData = false;
	ResetAnchorScreenSmoothing();
	if (StaticCardLayerWidget)
	{
		StaticCardLayerWidget->ClearSlotMotionState();
	}
}

TArray<FWacomFirstPersonCardLayerEntry> UWacomFirstPersonCardAnchorComponent::BuildStaticCardLayerEntries() const
{
	TArray<FWacomCardViewData> CardData;
	const int32 DesiredCount = StaticPreviewCardDefinitions.Num() > 0
		? StaticPreviewCardDefinitions.Num()
		: StaticCardCountFallback;
	const int32 ClampedCount = FMath::Clamp(DesiredCount, 0, 32);
	CardData.Reserve(ClampedCount);
	for (int32 Index = 0; Index < ClampedCount; ++Index)
	{
		CardData.Add(BuildStaticCardViewData(Index));
	}
	return BuildCardLayerEntriesFromData(CardData);
}

TArray<FWacomFirstPersonCardLayerEntry> UWacomFirstPersonCardAnchorComponent::BuildCardLayerEntriesFromData(
	const TArray<FWacomCardViewData>& CardData)
{
	TArray<FWacomFirstPersonCardLayerEntry> Entries;
	Entries.Reserve(CardData.Num());
	for (const FWacomCardViewData& Data : CardData)
	{
		FWacomFirstPersonCardLayerEntry Entry;
		Entry.CardViewData = Data;
		Entry.bIsPlayable = !Data.bDisabled;
		Entries.Add(MoveTemp(Entry));
	}
	return Entries;
}

TArray<FWacomFirstPersonCardLayerSlotView> UWacomFirstPersonCardAnchorComponent::BuildCardSlotViewsFromEntries(
	const TArray<FWacomFirstPersonCardLayerEntry>& CardEntries) const
{
	TArray<FWacomFirstPersonCardLayerSlotView> Slots;
	if (!bHasValidAnchor)
	{
		return Slots;
	}

	const int32 ClampedCount = FMath::Clamp(CardEntries.Num(), 0, 32);
	Slots.Reserve(ClampedCount);

	FWacomFirstPersonCardProjectedPoint AnchorPoint;
	const bool bUseAuthoredLayout = CardLayoutMode == EWacomFirstPersonCardLayoutMode::Authored2D;
	const bool bAuthoredAnchorProjected = !bUseAuthoredLayout
		|| ProjectCardTransformToScreen(CurrentAnchorTransform, AnchorPoint, INDEX_NONE);
	if (bUseAuthoredLayout && bAuthoredAnchorProjected)
	{
		ApplyAnchorScreenSmoothing(AnchorPoint);
	}
	else if (bUseAuthoredLayout)
	{
		ResetAnchorScreenSmoothing();
	}

	for (int32 Index = 0; Index < ClampedCount; ++Index)
	{
		FWacomFirstPersonCardLayerSlotView Slot;
		Slot.Index = Index;
		Slot.Entry = CardEntries[Index];
		Slot.Entry.bIsPlayable = Slot.Entry.bIsPlayable && !Slot.Entry.CardViewData.bDisabled;
		Slot.Entry.CardViewData.bDisabled = !Slot.Entry.bIsPlayable;
		Slot.RenderScale = FMath::Max(0.01f, StaticCardRenderScale);
		Slot.RenderOpacity = Slot.Entry.bIsPlayable
			? 1.0f
			: FMath::Clamp(DisabledRenderOpacity, 0.0f, 1.0f);
		Slot.ZOrder = Index;
		Slot.ProjectionMode = ProjectionMode;
		Slot.LayoutMode = CardLayoutMode;
		Slot.ViewportClampMode = ViewportClampMode;
		Slot.bBodyLockedLayout = ProjectionMode == EWacomFirstPersonCardProjectionMode::BodyLocked;
		Slot.bCurrentCameraProjection = true;
		Slot.bLookOffsetAppliedToLayout = bCurrentLookOffsetAppliedToLayout;

		const float CenterOffset =
			static_cast<float>(Index) - (static_cast<float>(ClampedCount - 1) * 0.5f);
		const float MaxAbsCenterOffset = FMath::Max(1.0f, static_cast<float>(ClampedCount - 1) * 0.5f);
		const float NormalizedHandOffset = CenterOffset / MaxAbsCenterOffset;
		const float NormalizedEdgeDistance = FMath::Abs(NormalizedHandOffset);
		const float FanCurveExponent = FMath::Max(0.01f, AuthoredFanCurveExponent);
		const float FanDirection = CenterOffset < 0.0f ? -1.0f : 1.0f;
		const float AuthoredFanMagnitude = FMath::Pow(NormalizedEdgeDistance, FanCurveExponent) * MaxAbsCenterOffset;
		const float FanOffset = bUseAuthoredLayout
			? FanDirection * AuthoredFanMagnitude
			: CenterOffset;
		Slot.NormalizedHandOffset = NormalizedHandOffset;
		Slot.RenderAngleDegrees = ClampCardLayerRenderAngle(FanOffset * FanYawDegrees);
		if (bUseAuthoredLayout && bAuthoredCenterCardsDrawOnTop)
		{
			Slot.ZOrder = FMath::RoundToInt((1.0f - NormalizedEdgeDistance) * 100.0f);
		}
		if (Slot.Entry.bIsHandAnchor)
		{
			Slot.RenderScale *= FMath::Max(0.01f, HandAnchorScale);
		}
		if (Slot.Entry.bIsPendingTargeting)
		{
			Slot.RenderScale *= FMath::Max(0.01f, PendingTargetingScale);
			Slot.ZOrder += 1000;
		}
		if (Slot.Entry.CardInstanceId.IsValid() && Slot.Entry.CardInstanceId == HoveredCardInstanceId)
		{
			Slot.bIsHovered = true;
			Slot.RenderScale *= FMath::Max(0.01f, HoverScale);
			Slot.ZOrder += FMath::Max(0, HoverZOrderBoost);
		}

		if (bUseAuthoredLayout)
		{
			Slot.RawScreenPosition = AnchorPoint.RawScreenPosition;
			Slot.UnclampedWidgetPosition = AnchorPoint.UnclampedWidgetPosition;
			Slot.ViewportScale = AnchorPoint.ViewportScale;
			Slot.ProjectionMode = AnchorPoint.ProjectionMode;
			Slot.ViewportClampMode = AnchorPoint.ViewportClampMode;
			Slot.AnchorWidgetPosition = AnchorPoint.WidgetPosition;
			Slot.UnsmoothedAnchorWidgetPosition = AnchorPoint.UnsmoothedAnchorWidgetPosition;
			Slot.SmoothedAnchorWidgetPosition = AnchorPoint.SmoothedAnchorWidgetPosition;
			Slot.OffscreenDistancePixels = AnchorPoint.OffscreenDistancePixels;
			Slot.AnchorScreenSmoothingDistancePixels = AnchorPoint.AnchorScreenSmoothingDistancePixels;
			Slot.bBodyLockedLayout = ProjectionMode == EWacomFirstPersonCardProjectionMode::BodyLocked;
			Slot.bCurrentCameraProjection = true;
			Slot.bLookOffsetAppliedToLayout = bCurrentLookOffsetAppliedToLayout;
			Slot.bClamped = AnchorPoint.bClamped;
			Slot.bOutsideViewport = AnchorPoint.bOutsideViewport;
			Slot.bAnchorScreenSmoothed = AnchorPoint.bAnchorScreenSmoothed;
			Slot.bProjected = false;

			if (bAuthoredAnchorProjected)
			{
				const float NaturalHandWidth = FMath::Max(0, ClampedCount - 1) * FMath::Max(0.0f, AuthoredCardSpacingPixels);
				const float MaxHandWidth = FMath::Max(0.0f, AuthoredMaxHandWidthPixels);
				const float WidthScale = (MaxHandWidth > 0.0f && NaturalHandWidth > MaxHandWidth)
					? MaxHandWidth / NaturalHandWidth
					: 1.0f;
				const float XOffset = CenterOffset * FMath::Max(0.0f, AuthoredCardSpacingPixels) * WidthScale;
				const float DropMagnitude =
					FMath::Pow(NormalizedEdgeDistance, FMath::Max(0.01f, AuthoredDropCurveExponent))
					* FMath::Max(0.0f, StaticCardEdgeDropPixels);
				const float CenterLiftMagnitude =
					(1.0f - NormalizedEdgeDistance) * AuthoredCenterLiftPixels;

				FVector2D FinalPosition =
					AnchorPoint.WidgetPosition
					+ AuthoredHandScreenOffset
					+ FVector2D(XOffset, DropMagnitude - CenterLiftMagnitude);
				Slot.AuthoredLayoutOffset = FinalPosition - AnchorPoint.WidgetPosition;
				Slot.bBodyLockedLayout = AnchorPoint.bBodyLockedLayout;
				Slot.bCurrentCameraProjection = AnchorPoint.bCurrentCameraProjection;
				Slot.bLookOffsetAppliedToLayout = AnchorPoint.bLookOffsetAppliedToLayout;
				if (Slot.Entry.bIsPendingTargeting)
				{
					FinalPosition.Y -= FMath::Max(0.0f, PendingTargetingLiftPixels);
				}
				if (Slot.Entry.CardInstanceId.IsValid() && Slot.Entry.CardInstanceId == HoveredCardInstanceId)
				{
					FinalPosition.Y -= FMath::Max(0.0f, HoverLiftPixels);
				}
				bool bPixelSnapped = false;
				Slot.WidgetPosition = FinalPosition;
				Slot.SnappedWidgetPosition = SnapCardLayerPosition(FinalPosition, bPixelSnapped);
				Slot.ScreenPosition = Slot.SnappedWidgetPosition;
				Slot.bPixelSnapped = bPixelSnapped;
				Slot.bProjected = AnchorPoint.bProjected;
			}
		}
		else
		{
			FWacomFirstPersonCardProjectedPoint Point;
			if (ProjectCardTransformToScreen(ComputeCardTransform(ClampedCount, Index), Point, Index))
			{
				FVector2D FinalPosition = Point.WidgetPosition;
				Slot.RawScreenPosition = Point.RawScreenPosition;
				Slot.UnclampedWidgetPosition = Point.UnclampedWidgetPosition;
				Slot.ViewportScale = Point.ViewportScale;
				Slot.ProjectionMode = Point.ProjectionMode;
				Slot.LayoutMode = EWacomFirstPersonCardLayoutMode::LegacyProjectedFan2D;
				Slot.ViewportClampMode = Point.ViewportClampMode;
				Slot.AnchorWidgetPosition = Point.WidgetPosition;
				Slot.UnsmoothedAnchorWidgetPosition = Point.WidgetPosition;
				Slot.SmoothedAnchorWidgetPosition = Point.WidgetPosition;
				Slot.OffscreenDistancePixels = Point.OffscreenDistancePixels;
				Slot.AnchorScreenSmoothingDistancePixels = 0.0f;
				Slot.bBodyLockedLayout = Point.bBodyLockedLayout;
				Slot.bCurrentCameraProjection = Point.bCurrentCameraProjection;
				Slot.bLookOffsetAppliedToLayout = Point.bLookOffsetAppliedToLayout;
				FinalPosition.Y += FMath::Square(NormalizedEdgeDistance) * FMath::Max(0.0f, StaticCardEdgeDropPixels);
				if (Slot.Entry.bIsPendingTargeting)
				{
					FinalPosition.Y -= FMath::Max(0.0f, PendingTargetingLiftPixels);
				}
				if (Slot.Entry.CardInstanceId.IsValid() && Slot.Entry.CardInstanceId == HoveredCardInstanceId)
				{
					FinalPosition.Y -= FMath::Max(0.0f, HoverLiftPixels);
				}
				bool bPixelSnapped = false;
				Slot.WidgetPosition = FinalPosition;
				Slot.SnappedWidgetPosition = SnapCardLayerPosition(FinalPosition, bPixelSnapped);
				Slot.ScreenPosition = Slot.SnappedWidgetPosition;
				Slot.bPixelSnapped = bPixelSnapped;
				Slot.bProjected = Point.bProjected;
				Slot.bClamped = Point.bClamped;
				Slot.bOutsideViewport = Point.bOutsideViewport;
				Slot.bAnchorScreenSmoothed = false;
			}
		}

		Slots.Add(Slot);
	}
	return Slots;
}

TArray<FWacomFirstPersonCardLayerSlotView> UWacomFirstPersonCardAnchorComponent::BuildActiveCardLayerSlotViews() const
{
	return bHasRuntimeCardLayerData
		? BuildCardSlotViewsFromEntries(RuntimeCardLayerEntries)
		: BuildStaticCardSlotViews();
}

void UWacomFirstPersonCardAnchorComponent::SetBattleHandInteractionPrototypeEnabled(bool bEnabled)
{
	if (bEnableBattleHandInteractionPrototype == bEnabled)
	{
		return;
	}

	bEnableBattleHandInteractionPrototype = bEnabled;
	if (!bEnableBattleHandInteractionPrototype)
	{
		HoveredCardInstanceId.Invalidate();
		if (StaticCardLayerWidget)
		{
			StaticCardLayerWidget->ClearSlotMotionState();
		}
	}
	if (StaticCardLayerWidget)
	{
		StaticCardLayerWidget->SetCardLayerInteractionEnabled(bEnableBattleHandInteractionPrototype);
	}
}

FString UWacomFirstPersonCardAnchorComponent::GetDebugSummary() const
{
	const FString LayerMotionSummary = StaticCardLayerWidget
		? StaticCardLayerWidget->GetSlotMotionDebugSummary()
		: TEXT("SlotMotion Inactive");
	return FString::Printf(
		TEXT("FirstPersonCardAnchor Mode=%s ProjectionMode=%s LayoutMode=%s ViewportClampMode=%s BodyLockedLayout=%s CurrentCameraProjection=true LookUsedForLayout=%s Valid=%s Anchor=%s LookOffset=%s Fallback=%s PixelSnap=%s SnapGrid=%.2f AngleClamp=%s MaxAngle=%.2f ViewportScale=%.2f SoftAllowance=%.2f SoftBlend=%.2f AnchorScreenSmoothing=%s AnchorScreenSmoothingSpeed=%.2f AnchorScreenSmoothingReset=%.2f AnchorScreenSmoothed=%s AnchorScreenSmoothingDistance=%.2f %s"),
		*AnchorModeToString(CurrentMode),
		*ProjectionModeToString(ProjectionMode),
		*LayoutModeToString(CardLayoutMode),
		*ViewportClampModeToString(ViewportClampMode),
		ProjectionMode == EWacomFirstPersonCardProjectionMode::BodyLocked ? TEXT("true") : TEXT("false"),
		bCurrentLookOffsetAppliedToLayout ? TEXT("true") : TEXT("false"),
		bHasValidAnchor ? TEXT("true") : TEXT("false"),
		*CurrentAnchorTransform.ToHumanReadableString(),
		*CurrentLookOffsetUsed.ToString(),
		*LastFallbackReason.ToString(),
		bEnableCardLayerPixelSnapping ? TEXT("true") : TEXT("false"),
		CardLayerPixelSnapGrid,
		bClampCardLayerRenderAngle ? TEXT("true") : TEXT("false"),
		MaxCardLayerRenderAngleDegrees,
		GetViewportScaleForAnchor(),
		SoftClampOffscreenAllowancePixels,
		SoftClampBlendRangePixels,
		bEnableAnchorScreenSmoothing ? TEXT("true") : TEXT("false"),
		AnchorScreenSmoothingSpeed,
		AnchorScreenSmoothingResetDistancePixels,
		bLastAnchorScreenSmoothed ? TEXT("true") : TEXT("false"),
		LastAnchorScreenSmoothingDistancePixels,
		*LayerMotionSummary);
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

void UWacomFirstPersonCardAnchorComponent::ConfigureTickPrerequisites()
{
	if (const AWacomPlayerCharacter* Character = GetOwnerCharacter())
	{
		if (UWacomRunTunnelMovementComponent* RunTunnel = Character->GetRunTunnelMovementComponent())
		{
			PrimaryComponentTick.AddPrerequisite(RunTunnel, RunTunnel->PrimaryComponentTick);
		}
		if (UWacomBattleCameraLookComponent* BattleCamera = Character->GetBattleCameraLookComponent())
		{
			PrimaryComponentTick.AddPrerequisite(BattleCamera, BattleCamera->PrimaryComponentTick);
		}
	}
}

void UWacomFirstPersonCardAnchorComponent::ResetAnchorScreenSmoothing() const
{
	bHasSmoothedAnchorWidgetPosition = false;
	bLastAnchorScreenSmoothed = false;
	LastAnchorScreenSmoothingDistancePixels = 0.0f;
	SmoothedAnchorWidgetPosition = FVector2D::ZeroVector;
	LastAnchorScreenSmoothingTargetWidgetPosition = FVector2D::ZeroVector;
	LastAnchorScreenSmoothingFrame = 0;
	SmoothedAnchorLayoutMode = CardLayoutMode;
	SmoothedAnchorProjectionMode = ProjectionMode;
	SmoothedAnchorViewportClampMode = ViewportClampMode;
	SmoothedAnchorMode = CurrentMode;
}

void UWacomFirstPersonCardAnchorComponent::ApplyAnchorScreenSmoothing(
	FWacomFirstPersonCardProjectedPoint& AnchorPoint) const
{
	AnchorPoint.UnsmoothedAnchorWidgetPosition = AnchorPoint.WidgetPosition;
	AnchorPoint.SmoothedAnchorWidgetPosition = AnchorPoint.WidgetPosition;
	AnchorPoint.AnchorScreenSmoothingDistancePixels = 0.0f;
	AnchorPoint.bAnchorScreenSmoothed = false;
	bLastAnchorScreenSmoothed = false;
	LastAnchorScreenSmoothingDistancePixels = 0.0f;

	if (!AnchorPoint.bProjected
		|| CardLayoutMode != EWacomFirstPersonCardLayoutMode::Authored2D
		|| !bEnableAnchorScreenSmoothing
		|| AnchorScreenSmoothingSpeed <= 0.0f)
	{
		SmoothedAnchorWidgetPosition = AnchorPoint.WidgetPosition;
		bHasSmoothedAnchorWidgetPosition = AnchorPoint.bProjected;
		SmoothedAnchorLayoutMode = CardLayoutMode;
		SmoothedAnchorProjectionMode = ProjectionMode;
		SmoothedAnchorViewportClampMode = ViewportClampMode;
		SmoothedAnchorMode = CurrentMode;
		return;
	}

	const bool bModeChanged =
		SmoothedAnchorLayoutMode != CardLayoutMode
		|| SmoothedAnchorProjectionMode != ProjectionMode
		|| SmoothedAnchorViewportClampMode != ViewportClampMode
		|| SmoothedAnchorMode != CurrentMode;
	const bool bSameFrameTarget =
		LastAnchorScreenSmoothingFrame == GFrameCounter
		&& LastAnchorScreenSmoothingTargetWidgetPosition.Equals(AnchorPoint.WidgetPosition, KINDA_SMALL_NUMBER);
	const float TargetJumpDistance = bHasSmoothedAnchorWidgetPosition
		? FVector2D::Distance(LastAnchorScreenSmoothingTargetWidgetPosition, AnchorPoint.WidgetPosition)
		: 0.0f;
	const float PreviousDistance = bHasSmoothedAnchorWidgetPosition
		? FVector2D::Distance(SmoothedAnchorWidgetPosition, AnchorPoint.WidgetPosition)
		: 0.0f;
	const float ResetDistance = FMath::Max(0.0f, AnchorScreenSmoothingResetDistancePixels);
	const bool bLargeJump = bHasSmoothedAnchorWidgetPosition
		&& ResetDistance > 0.0f
		&& TargetJumpDistance > ResetDistance;

	if (!bHasSmoothedAnchorWidgetPosition || bModeChanged || bLargeJump)
	{
		SmoothedAnchorWidgetPosition = AnchorPoint.WidgetPosition;
		bHasSmoothedAnchorWidgetPosition = true;
		SmoothedAnchorLayoutMode = CardLayoutMode;
		SmoothedAnchorProjectionMode = ProjectionMode;
		SmoothedAnchorViewportClampMode = ViewportClampMode;
		SmoothedAnchorMode = CurrentMode;
		LastAnchorScreenSmoothingTargetWidgetPosition = AnchorPoint.WidgetPosition;
		LastAnchorScreenSmoothingFrame = GFrameCounter;
		return;
	}

	if (bSameFrameTarget)
	{
		AnchorPoint.WidgetPosition = SmoothedAnchorWidgetPosition;
		AnchorPoint.SmoothedAnchorWidgetPosition = SmoothedAnchorWidgetPosition;
		AnchorPoint.AnchorScreenSmoothingDistancePixels = PreviousDistance;
		AnchorPoint.bAnchorScreenSmoothed = PreviousDistance > KINDA_SMALL_NUMBER;
		bLastAnchorScreenSmoothed = AnchorPoint.bAnchorScreenSmoothed;
		LastAnchorScreenSmoothingDistancePixels = PreviousDistance;
		return;
	}

	const float DeltaTime = GetAnchorSmoothingDeltaTimeForAnchor();
	if (DeltaTime <= 0.0f)
	{
		return;
	}

	SmoothedAnchorWidgetPosition = FMath::Vector2DInterpTo(
		SmoothedAnchorWidgetPosition,
		AnchorPoint.WidgetPosition,
		DeltaTime,
		AnchorScreenSmoothingSpeed);
	const float SmoothedDistance = FVector2D::Distance(SmoothedAnchorWidgetPosition, AnchorPoint.WidgetPosition);
	AnchorPoint.WidgetPosition = SmoothedAnchorWidgetPosition;
	AnchorPoint.SmoothedAnchorWidgetPosition = SmoothedAnchorWidgetPosition;
	AnchorPoint.AnchorScreenSmoothingDistancePixels = SmoothedDistance;
	AnchorPoint.bAnchorScreenSmoothed = SmoothedDistance > KINDA_SMALL_NUMBER;
	bLastAnchorScreenSmoothed = AnchorPoint.bAnchorScreenSmoothed;
	LastAnchorScreenSmoothingDistancePixels = SmoothedDistance;
	SmoothedAnchorLayoutMode = CardLayoutMode;
	SmoothedAnchorProjectionMode = ProjectionMode;
	SmoothedAnchorViewportClampMode = ViewportClampMode;
	SmoothedAnchorMode = CurrentMode;
	LastAnchorScreenSmoothingTargetWidgetPosition = AnchorPoint.UnsmoothedAnchorWidgetPosition;
	LastAnchorScreenSmoothingFrame = GFrameCounter;
}

FVector2D UWacomFirstPersonCardAnchorComponent::ApplyViewportClampToWidgetPosition(
	FVector2D UnclampedPosition,
	FVector2D WidgetViewportSize,
	bool& bOutClamped,
	bool& bOutOutsideViewport,
	float& OutOffscreenDistancePixels) const
{
	bOutClamped = false;
	bOutOutsideViewport = false;
	OutOffscreenDistancePixels = 0.0f;

	const float Padding = FMath::Max(0.0f, ProjectionPadding);
	const FVector2D SafeMin(Padding, Padding);
	const FVector2D SafeMax(
		FMath::Max(Padding, WidgetViewportSize.X - Padding),
		FMath::Max(Padding, WidgetViewportSize.Y - Padding));
	const FVector2D NearestSafePoint(
		FMath::Clamp(UnclampedPosition.X, SafeMin.X, SafeMax.X),
		FMath::Clamp(UnclampedPosition.Y, SafeMin.Y, SafeMax.Y));
	OutOffscreenDistancePixels = FVector2D::Distance(UnclampedPosition, NearestSafePoint);
	bOutOutsideViewport = OutOffscreenDistancePixels > KINDA_SMALL_NUMBER;

	if (ViewportClampMode == EWacomFirstPersonCardViewportClampMode::AllowOffscreen)
	{
		return UnclampedPosition;
	}

	if (ViewportClampMode == EWacomFirstPersonCardViewportClampMode::HardClampToViewport)
	{
		bOutClamped = bOutOutsideViewport;
		return NearestSafePoint;
	}

	const float Allowance = FMath::Max(0.0f, SoftClampOffscreenAllowancePixels);
	const FVector2D SoftMin = SafeMin - FVector2D(Allowance, Allowance);
	const FVector2D SoftMax = SafeMax + FVector2D(Allowance, Allowance);
	const FVector2D NearestSoftPoint(
		FMath::Clamp(UnclampedPosition.X, SoftMin.X, SoftMax.X),
		FMath::Clamp(UnclampedPosition.Y, SoftMin.Y, SoftMax.Y));
	const float SoftOvershootDistance = FVector2D::Distance(UnclampedPosition, NearestSoftPoint);
	if (SoftOvershootDistance <= KINDA_SMALL_NUMBER)
	{
		return UnclampedPosition;
	}

	const float BlendRange = FMath::Max(0.0f, SoftClampBlendRangePixels);
	const float Alpha = BlendRange <= KINDA_SMALL_NUMBER
		? 1.0f
		: FMath::SmoothStep(0.0f, 1.0f, FMath::Clamp(SoftOvershootDistance / BlendRange, 0.0f, 1.0f));
	const FVector2D ClampedPosition = FMath::Lerp(UnclampedPosition, NearestSoftPoint, Alpha);
	bOutClamped = !UnclampedPosition.Equals(ClampedPosition, KINDA_SMALL_NUMBER);
	return ClampedPosition;
}

FVector2D UWacomFirstPersonCardAnchorComponent::SnapCardLayerPosition(
	FVector2D Position,
	bool& bOutPixelSnapped) const
{
	bOutPixelSnapped = false;
	if (!bEnableCardLayerPixelSnapping)
	{
		return Position;
	}

	const float Grid = FMath::Max(0.01f, CardLayerPixelSnapGrid);
	FVector2D SnappedPosition(
		FMath::RoundToFloat(Position.X / Grid) * Grid,
		FMath::RoundToFloat(Position.Y / Grid) * Grid);
	bOutPixelSnapped = !SnappedPosition.Equals(Position, KINDA_SMALL_NUMBER);
	return SnappedPosition;
}

float UWacomFirstPersonCardAnchorComponent::ClampCardLayerRenderAngle(float AngleDegrees) const
{
	if (!bClampCardLayerRenderAngle)
	{
		return AngleDegrees;
	}

	const float MaxAbsAngle = FMath::Max(0.0f, MaxCardLayerRenderAngleDegrees);
	return FMath::Clamp(AngleDegrees, -MaxAbsAngle, MaxAbsAngle);
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

FString UWacomFirstPersonCardAnchorComponent::ProjectionModeToString(EWacomFirstPersonCardProjectionMode Mode)
{
	switch (Mode)
	{
	case EWacomFirstPersonCardProjectionMode::LegacyWorldProjected:
		return TEXT("LegacyWorldProjected");
	case EWacomFirstPersonCardProjectionMode::BodyLocked:
	default:
		return TEXT("BodyLocked");
	}
}

FString UWacomFirstPersonCardAnchorComponent::LayoutModeToString(EWacomFirstPersonCardLayoutMode Mode)
{
	switch (Mode)
	{
	case EWacomFirstPersonCardLayoutMode::LegacyProjectedFan2D:
		return TEXT("LegacyProjectedFan2D");
	case EWacomFirstPersonCardLayoutMode::Authored2D:
	default:
		return TEXT("Authored2D");
	}
}

FString UWacomFirstPersonCardAnchorComponent::ViewportClampModeToString(
	EWacomFirstPersonCardViewportClampMode Mode)
{
	switch (Mode)
	{
	case EWacomFirstPersonCardViewportClampMode::HardClampToViewport:
		return TEXT("HardClampToViewport");
	case EWacomFirstPersonCardViewportClampMode::AllowOffscreen:
		return TEXT("AllowOffscreen");
	case EWacomFirstPersonCardViewportClampMode::SoftClampToViewport:
	default:
		return TEXT("SoftClampToViewport");
	}
}

void UWacomFirstPersonCardAnchorComponent::UpdateStaticCardLayer()
{
	if (!bDrawStaticCardLayer && !bHasRuntimeCardLayerData)
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
			StaticCardLayerWidget->SetCardViewClass(FirstPersonCardViewClass);
			FWacomFirstPersonCardSlotMotionConfig MotionConfig;
			MotionConfig.bEnabled = bEnableCardSlotMotion;
			MotionConfig.MotionSpeed = CardSlotMotionSpeed;
			MotionConfig.OpacitySpeed = CardSlotOpacitySpeed;
			MotionConfig.EnterOffsetPixels = CardSlotEnterOffsetPixels;
			MotionConfig.EnterOpacity = CardSlotEnterOpacity;
			MotionConfig.ExitOffsetPixels = CardSlotExitOffsetPixels;
			MotionConfig.ExitDuration = CardSlotExitDuration;
			MotionConfig.ResetDistancePixels = CardSlotMotionResetDistancePixels;
			StaticCardLayerWidget->SetSlotMotionConfig(MotionConfig);
			StaticCardLayerWidget->SetLogSlotMotionDiagnostics(bLogCardLayerMotionDiagnostics);
			StaticCardLayerWidget->SetCardLayerInteractionEnabled(bEnableBattleHandInteractionPrototype);
			BindStaticCardLayerWidget(StaticCardLayerWidget);
			AddStaticCardLayerWidgetToViewportForAnchor(StaticCardLayerWidget, StaticCardLayerZOrder);
		}
	}

	if (StaticCardLayerWidget)
	{
		FWacomFirstPersonCardSlotMotionConfig MotionConfig;
		MotionConfig.bEnabled = bEnableCardSlotMotion;
		MotionConfig.MotionSpeed = CardSlotMotionSpeed;
		MotionConfig.OpacitySpeed = CardSlotOpacitySpeed;
		MotionConfig.EnterOffsetPixels = CardSlotEnterOffsetPixels;
		MotionConfig.EnterOpacity = CardSlotEnterOpacity;
		MotionConfig.ExitOffsetPixels = CardSlotExitOffsetPixels;
		MotionConfig.ExitDuration = CardSlotExitDuration;
		MotionConfig.ResetDistancePixels = CardSlotMotionResetDistancePixels;
		StaticCardLayerWidget->SetSlotMotionConfig(MotionConfig);
		StaticCardLayerWidget->SetLogSlotMotionDiagnostics(bLogCardLayerMotionDiagnostics);
		StaticCardLayerWidget->SetCardViewClass(FirstPersonCardViewClass);
		StaticCardLayerWidget->SetStaticCardSlots(BuildActiveCardLayerSlotViews());
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
		StaticCardLayerWidget->ClearSlotMotionState();
		UnbindStaticCardLayerWidget(StaticCardLayerWidget);
		StaticCardLayerWidget->RemoveFromParent();
		StaticCardLayerWidget = nullptr;
	}
	HoveredCardInstanceId.Invalidate();
}

void UWacomFirstPersonCardAnchorComponent::BindStaticCardLayerWidget(UWacomFirstPersonCardLayerWidget* LayerWidget)
{
	if (!LayerWidget)
	{
		return;
	}

	LayerWidget->OnCardClickedNative.RemoveAll(this);
	LayerWidget->OnCardHoveredNative.RemoveAll(this);
	LayerWidget->OnCardUnhoveredNative.RemoveAll(this);
	LayerWidget->OnHoveredCardSlotUpdatedNative.RemoveAll(this);
	LayerWidget->OnCardClickedNative.AddUObject(this, &UWacomFirstPersonCardAnchorComponent::HandleLayerCardClicked);
	LayerWidget->OnCardHoveredNative.AddUObject(this, &UWacomFirstPersonCardAnchorComponent::HandleLayerCardHovered);
	LayerWidget->OnCardUnhoveredNative.AddUObject(this, &UWacomFirstPersonCardAnchorComponent::HandleLayerCardUnhovered);
	LayerWidget->OnHoveredCardSlotUpdatedNative.AddUObject(
		this,
		&UWacomFirstPersonCardAnchorComponent::HandleLayerHoveredCardSlotUpdated);
}

void UWacomFirstPersonCardAnchorComponent::UnbindStaticCardLayerWidget(UWacomFirstPersonCardLayerWidget* LayerWidget)
{
	if (!LayerWidget)
	{
		return;
	}

	LayerWidget->OnCardClickedNative.RemoveAll(this);
	LayerWidget->OnCardHoveredNative.RemoveAll(this);
	LayerWidget->OnCardUnhoveredNative.RemoveAll(this);
	LayerWidget->OnHoveredCardSlotUpdatedNative.RemoveAll(this);
}

void UWacomFirstPersonCardAnchorComponent::HandleLayerCardClicked(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	OnFirstPersonCardLayerCardClicked.Broadcast(CardInstanceId, SlotView);
}

void UWacomFirstPersonCardAnchorComponent::HandleLayerCardHovered(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	HoveredCardInstanceId = CardInstanceId;
	OnFirstPersonCardLayerCardHovered.Broadcast(CardInstanceId, SlotView);
}

void UWacomFirstPersonCardAnchorComponent::HandleLayerCardUnhovered(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	if (HoveredCardInstanceId == CardInstanceId)
	{
		HoveredCardInstanceId.Invalidate();
	}
	OnFirstPersonCardLayerCardUnhovered.Broadcast(CardInstanceId, SlotView);
}

void UWacomFirstPersonCardAnchorComponent::HandleLayerHoveredCardSlotUpdated(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	OnFirstPersonCardLayerHoveredCardLayoutUpdated.Broadcast(CardInstanceId, SlotView);
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
