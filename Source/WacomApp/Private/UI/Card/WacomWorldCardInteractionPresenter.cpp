// Copyright Wacom. All Rights Reserved.

#include "WacomWorldCardInteractionPresenter.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Cards/CardDefinition.h"
#include "Components/WidgetComponent.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/WacomPlayerController.h"
#include "UI/Card/WacomCardDetailPanel.h"
#include "UI/Card/WacomCardExplanationLexicon.h"
#include "UI/Card/WacomCardSemanticTooltipWidget.h"
#include "UI/Card/WacomCardView.h"
#include "WacomCardExplanationLexiconProvider.h"

namespace
{
	constexpr const TCHAR* CardDetailPanelClassPath =
		TEXT("/Game/Wacom/UI/Card/WBP_CardDetailPanel.WBP_CardDetailPanel_C");

	TSet<FName>& GetWarnedMissingSemanticIds()
	{
		static TSet<FName> WarnedIds;
		return WarnedIds;
	}
}

void FWacomWorldCardInteractionPresenter::SyncItems(
	const TArray<FWacomWorldCardInteractionItemView>& InItems)
{
	bool bPinnedIdentityChanged = false;
	bool bSemanticIdentityChanged = false;
	TArray<FItemRuntime> NewItems;
	NewItems.Reserve(InItems.Num());
	for (const FWacomWorldCardInteractionItemView& Incoming : InItems)
	{
		if (!Incoming.ItemId.IsValid()
			|| !Incoming.Definition.IsValid()
			|| !Incoming.WidgetComponent.IsValid()
			|| !Incoming.RootWidget.IsValid()
			|| !Incoming.CardView.IsValid())
		{
			continue;
		}

		FItemRuntime NewItem;
		NewItem.View = Incoming;
		if (FItemRuntime* Existing = FindItem(Incoming.ItemId);
			Existing
			&& Existing->View.WidgetComponent == Incoming.WidgetComponent)
		{
			NewItem.BaseRelativeTransform =
				Existing->BaseRelativeTransform;
			NewItem.HoverAlpha = Existing->HoverAlpha;
			if (Existing->View.Definition != Incoming.Definition)
			{
				bPinnedIdentityChanged |= PinnedItemId == Incoming.ItemId;
				bSemanticIdentityChanged |=
					HoveredSemanticItemId == Incoming.ItemId;
			}
		}
		else
		{
			NewItem.BaseRelativeTransform =
				Incoming.WidgetComponent->GetRelativeTransform();
		}
		NewItems.Add(MoveTemp(NewItem));
	}

	for (FItemRuntime& Existing : Items)
	{
		const bool bStillPresent = NewItems.ContainsByPredicate(
			[&Existing](const FItemRuntime& Candidate)
			{
				return Candidate.View.ItemId == Existing.View.ItemId
					&& Candidate.View.WidgetComponent
						== Existing.View.WidgetComponent;
			});
		if (!bStillPresent)
		{
			RestoreItemTransform(Existing);
		}
	}
	Items = MoveTemp(NewItems);

	if (!FindItem(PinnedItemId) || bPinnedIdentityChanged)
	{
		CloseInspect();
	}
	else if (const FItemRuntime* Pinned = FindItem(PinnedItemId);
		Pinned && InspectPanel.IsValid())
	{
		InspectPanel->SetCardDetailData(Pinned->View.DetailViewData);
	}
	if (!FindItem(HoveredSemanticItemId) || bSemanticIdentityChanged)
	{
		HoveredSemanticItemId.Invalidate();
		HoveredSemanticId = NAME_None;
		SemanticHoverElapsedSeconds = 0.0f;
		bCurrentSemanticMissing = false;
		HideTooltip();
	}
	if (!FindItem(HoveredItemId))
	{
		HoveredItemId.Invalidate();
	}

	// Keep already-open inspect geometry stable; only data refreshes here.
}

void FWacomWorldCardInteractionPresenter::Tick(
	AWacomPlayerController& PlayerController,
	const float DeltaTime,
	const FWacomWorldCardPointerSample& PointerSample,
	const FWacomWorldCardInteractionStyle& InStyle)
{
	const FWacomWorldCardInteractionStyle Style = InStyle.Sanitized();
	if (FItemRuntime* Hovered =
		FindItemForComponent(PointerSample.HoveredComponent.Get()))
	{
		HoveredItemId = Hovered->View.ItemId;
	}
	else
	{
		HoveredItemId.Invalidate();
	}
	UpdateHoverTransforms(
		PlayerController,
		FMath::Max(0.0f, DeltaTime),
		Style);
	UpdateSemanticTooltip(
		PlayerController,
		FMath::Max(0.0f, DeltaTime),
		PointerSample,
		Style);
}

bool FWacomWorldCardInteractionPresenter::RouteRightClick(
	AWacomPlayerController& PlayerController,
	const FWacomWorldCardPointerSample& PointerSample,
	const FWacomWorldCardInteractionStyle& InStyle)
{
	const FWacomWorldCardInteractionStyle Style = InStyle.Sanitized();
	FItemRuntime* Item =
		FindItemForComponent(PointerSample.HoveredComponent.Get());
	HideTooltip();
	HoveredSemanticItemId.Invalidate();
	HoveredSemanticId = NAME_None;
	DisplayedSemanticItemId.Invalidate();
	DisplayedSemanticId = NAME_None;
	SemanticHoverElapsedSeconds = 0.0f;
	bCurrentSemanticMissing = false;

	if (!Item)
	{
		CloseInspect();
		return true;
	}
	if (PinnedItemId == Item->View.ItemId)
	{
		CloseInspect();
		return true;
	}
	OpenInspect(PlayerController, *Item, Style);
	return true;
}

void FWacomWorldCardInteractionPresenter::Reset()
{
	for (FItemRuntime& Item : Items)
	{
		RestoreItemTransform(Item);
	}
	Items.Reset();
	HoveredItemId.Invalidate();
	HoveredSemanticItemId.Invalidate();
	HoveredSemanticId = NAME_None;
	SemanticHoverElapsedSeconds = 0.0f;
	bCurrentSemanticMissing = false;
	HideTooltip();
	if (TooltipWidget.IsValid())
	{
		TooltipWidget->RemoveFromParent();
		TooltipWidget.Reset();
	}
	CloseInspect();
}

FWacomWorldCardInteractionPresenter::FItemRuntime*
FWacomWorldCardInteractionPresenter::FindItem(const FGuid& ItemId)
{
	if (!ItemId.IsValid())
	{
		return nullptr;
	}
	return Items.FindByPredicate([&ItemId](const FItemRuntime& Item)
	{
		return Item.View.ItemId == ItemId;
	});
}

const FWacomWorldCardInteractionPresenter::FItemRuntime*
FWacomWorldCardInteractionPresenter::FindItem(const FGuid& ItemId) const
{
	if (!ItemId.IsValid())
	{
		return nullptr;
	}
	return Items.FindByPredicate([&ItemId](const FItemRuntime& Item)
	{
		return Item.View.ItemId == ItemId;
	});
}

FWacomWorldCardInteractionPresenter::FItemRuntime*
FWacomWorldCardInteractionPresenter::FindItemForComponent(
	const UWidgetComponent* Component)
{
	if (!Component)
	{
		return nullptr;
	}
	return Items.FindByPredicate([Component](const FItemRuntime& Item)
	{
		return Item.View.WidgetComponent.Get() == Component;
	});
}

void FWacomWorldCardInteractionPresenter::RestoreItemTransform(
	FItemRuntime& Item)
{
	if (UWidgetComponent* Component = Item.View.WidgetComponent.Get())
	{
		Component->SetRelativeTransform(Item.BaseRelativeTransform);
	}
	Item.HoverAlpha = 0.0f;
}

void FWacomWorldCardInteractionPresenter::UpdateHoverTransforms(
	AWacomPlayerController& PlayerController,
	const float DeltaTime,
	const FWacomWorldCardInteractionStyle& Style)
{
	FVector CameraLocation = FVector::ZeroVector;
	FRotator CameraRotation = FRotator::ZeroRotator;
	PlayerController.GetPlayerViewPoint(CameraLocation, CameraRotation);

	const float AlphaSpeed =
		1.0f / FMath::Max(KINDA_SMALL_NUMBER, Style.HoverTransitionSeconds);
	for (FItemRuntime& Item : Items)
	{
		UWidgetComponent* Component = Item.View.WidgetComponent.Get();
		if (!Component)
		{
			continue;
		}
		const bool bShouldLift =
			Item.View.ItemId == HoveredItemId
			|| Item.View.ItemId == PinnedItemId;
		Item.HoverAlpha = FMath::FInterpConstantTo(
			Item.HoverAlpha,
			bShouldLift ? 1.0f : 0.0f,
			DeltaTime,
			AlphaSpeed);
		if (!bShouldLift && FMath::IsNearlyZero(Item.HoverAlpha))
		{
			RestoreItemTransform(Item);
			continue;
		}

		USceneComponent* Parent = Component->GetAttachParent();
		const FTransform BaseWorld = Parent
			? Item.BaseRelativeTransform * Parent->GetComponentTransform()
			: Item.BaseRelativeTransform;
		Component->SetWorldTransform(ComputeHoverWorldTransform(
			BaseWorld,
			CameraLocation,
			Item.HoverAlpha,
			Style));
	}
}

FTransform FWacomWorldCardInteractionPresenter::ComputeHoverWorldTransform(
	const FTransform& BaseWorld,
	const FVector& CameraLocation,
	const float HoverAlpha,
	const FWacomWorldCardInteractionStyle& Style)
{
	const float SafeAlpha = FMath::Clamp(HoverAlpha, 0.0f, 1.0f);
	FVector ToCamera = CameraLocation - BaseWorld.GetLocation();
	if (!ToCamera.Normalize())
	{
		ToCamera = FVector::ForwardVector * -1.0f;
	}
	const FVector HoveredLocation =
		BaseWorld.GetLocation()
		+ ToCamera * (Style.HoverForwardDistanceCm * SafeAlpha);
	const float ScaleMultiplier = FMath::Lerp(
		1.0f,
		Style.HoverScale,
		SafeAlpha);
	return FTransform(
		BaseWorld.GetRotation(),
		HoveredLocation,
		BaseWorld.GetScale3D() * ScaleMultiplier);
}

bool FWacomWorldCardInteractionPresenter::ResolveSemanticUnderPointer(
	const FWacomWorldCardPointerSample& PointerSample,
	FItemRuntime*& OutItem,
	FWacomCardFaceSemanticTokenView& OutToken)
{
	OutItem = FindItemForComponent(
		PointerSample.HoveredComponent.Get());
	if (!OutItem)
	{
		return false;
	}
	UUserWidget* RootWidget = OutItem->View.RootWidget.Get();
	UWacomCardView* CardView = OutItem->View.CardView.Get();
	if (!RootWidget || !CardView)
	{
		return false;
	}

	const FGeometry& RootGeometry = RootWidget->GetCachedGeometry();
	const FGeometry& CardGeometry = CardView->GetCachedGeometry();
	if (RootGeometry.GetLocalSize().IsNearlyZero()
		|| CardGeometry.GetLocalSize().IsNearlyZero())
	{
		return false;
	}
	const FVector2D AbsolutePosition = RootGeometry.LocalToAbsolute(
		PointerSample.LocalHitLocation);
	const FVector2D CardLocalPosition =
		CardGeometry.AbsoluteToLocal(AbsolutePosition);
	return CardView->TryResolveTypeSemanticTokenAtLocalPosition(
		CardLocalPosition,
		OutToken);
}

void FWacomWorldCardInteractionPresenter::UpdateSemanticTooltip(
	AWacomPlayerController& PlayerController,
	const float DeltaTime,
	const FWacomWorldCardPointerSample& PointerSample,
	const FWacomWorldCardInteractionStyle& Style)
{
	FItemRuntime* Item = nullptr;
	FWacomCardFaceSemanticTokenView Token;
	if (!ResolveSemanticUnderPointer(PointerSample, Item, Token))
	{
		HoveredSemanticItemId.Invalidate();
		HoveredSemanticId = NAME_None;
		SemanticHoverElapsedSeconds = 0.0f;
		bCurrentSemanticMissing = false;
		HideTooltip();
		return;
	}

	const bool bSameSemantic =
		HoveredSemanticItemId == Item->View.ItemId
		&& HoveredSemanticId == Token.SemanticId;
	if (!bSameSemantic)
	{
		HoveredSemanticItemId = Item->View.ItemId;
		HoveredSemanticId = Token.SemanticId;
		SemanticHoverElapsedSeconds = 0.0f;
		bCurrentSemanticMissing = false;
		HideTooltip();
	}
	SemanticHoverElapsedSeconds += DeltaTime;
	if (SemanticHoverElapsedSeconds < Style.TooltipDelaySeconds
		|| bCurrentSemanticMissing)
	{
		return;
	}

	FWacomCardFaceSemanticLexiconEntry Entry;
	if (!WacomCardExplanationLexiconProvider::FindCardFaceSemantic(
		Token.SemanticId,
		Token.SourceTag,
		Entry)
		|| Entry.DisplayName.IsEmpty()
		|| Entry.Description.IsEmpty())
	{
		bCurrentSemanticMissing = true;
#if !UE_BUILD_SHIPPING
		if (!GetWarnedMissingSemanticIds().Contains(Token.SemanticId))
		{
			GetWarnedMissingSemanticIds().Add(Token.SemanticId);
			UE_LOG(LogTemp, Warning,
				TEXT("[WorldCardInteraction] 缺少卡面语义说明 SemanticId=%s SourceTag=%s"),
				*Token.SemanticId.ToString(),
				*Token.SourceTag.ToString());
		}
#endif
		HideTooltip();
		return;
	}

	EnsureTooltip(PlayerController);
	if (!TooltipWidget.IsValid())
	{
		return;
	}
	if (DisplayedSemanticItemId != Item->View.ItemId
		|| DisplayedSemanticId != Token.SemanticId)
	{
		TooltipWidget->SetSemanticTooltip(
			Entry.DisplayName,
			Entry.Description,
			Style.TooltipWidthPixels);
		TooltipWidget->ForceLayoutPrepass();
		DisplayedSemanticItemId = Item->View.ItemId;
		DisplayedSemanticId = Token.SemanticId;
	}
	PositionTooltip(PlayerController, Style);
}

void FWacomWorldCardInteractionPresenter::EnsureTooltip(
	AWacomPlayerController& PlayerController)
{
	if (TooltipWidget.IsValid())
	{
		return;
	}
	UWacomCardSemanticTooltipWidget* Created =
		CreateWidget<UWacomCardSemanticTooltipWidget>(
			&PlayerController,
			UWacomCardSemanticTooltipWidget::StaticClass());
	if (!Created)
	{
		return;
	}
	TooltipWidget.Reset(Created);
	Created->AddToPlayerScreen(90);
	Created->SetVisibility(ESlateVisibility::Collapsed);
}

void FWacomWorldCardInteractionPresenter::HideTooltip()
{
	if (TooltipWidget.IsValid())
	{
		TooltipWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	DisplayedSemanticItemId.Invalidate();
	DisplayedSemanticId = NAME_None;
}

void FWacomWorldCardInteractionPresenter::PositionTooltip(
	AWacomPlayerController& PlayerController,
	const FWacomWorldCardInteractionStyle& Style)
{
	if (!TooltipWidget.IsValid())
	{
		return;
	}
	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (!PlayerController.GetMousePosition(MouseX, MouseY))
	{
		HideTooltip();
		return;
	}
	const float ViewportScale = FMath::Max(
		0.01f,
		UWidgetLayoutLibrary::GetViewportScale(&PlayerController));
	const FVector2D MousePosition(MouseX / ViewportScale, MouseY / ViewportScale);
	FVector2D TooltipSize = TooltipWidget->GetDesiredSize();
	TooltipSize.X = Style.TooltipWidthPixels;
	TooltipSize.Y = FMath::Max(1.0f, TooltipSize.Y);
	TooltipWidget->SetDesiredSizeInViewport(TooltipSize);
	TooltipWidget->SetAlignmentInViewport(FVector2D::ZeroVector);
	TooltipWidget->SetPositionInViewport(
		ComputeTooltipPosition(
			MousePosition,
			TooltipSize,
			GetViewportLogicalSize(PlayerController),
			Style),
		false);
	TooltipWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void FWacomWorldCardInteractionPresenter::OpenInspect(
	AWacomPlayerController& PlayerController,
	const FItemRuntime& Item,
	const FWacomWorldCardInteractionStyle& Style)
{
	if (!InspectPanel.IsValid())
	{
		UClass* PanelClass = LoadClass<UWacomCardDetailPanel>(
			nullptr,
			CardDetailPanelClassPath);
		if (!PanelClass
			|| !PanelClass->IsChildOf(UWacomCardDetailPanel::StaticClass()))
		{
			PanelClass = UWacomCardDetailPanel::StaticClass();
		}
		UWacomCardDetailPanel* Created =
			CreateWidget<UWacomCardDetailPanel>(
				&PlayerController,
				PanelClass);
		if (!Created)
		{
			return;
		}
		InspectPanel.Reset(Created);
		Created->AddToPlayerScreen(89);
	}

	UWidgetComponent* Component = Item.View.WidgetComponent.Get();
	FVector2D TargetScreen = FVector2D::ZeroVector;
	bool bProjected = Component
		&& PlayerController.ProjectWorldLocationToScreen(
			Component->GetComponentLocation(),
			TargetScreen,
			true);
	const float ViewportScale = FMath::Max(
		0.01f,
		UWidgetLayoutLibrary::GetViewportScale(&PlayerController));
	TargetScreen /= ViewportScale;
	const FVector2D ViewportSize =
		GetViewportLogicalSize(PlayerController);
	const bool bTargetIsOnLeft = !bProjected
		|| TargetScreen.X <= ViewportSize.X * 0.5f;

	InspectPanel->SetCardDetailData(Item.View.DetailViewData);
	InspectPanel->SetDesiredSizeInViewport(
		Style.InspectPanelSizePixels);
	InspectPanel->SetAlignmentInViewport(FVector2D::ZeroVector);
	InspectPanel->SetPositionInViewport(
		ComputeInspectPosition(
			bTargetIsOnLeft,
			ViewportSize,
			Style),
		false);
	InspectPanel->SetRenderOpacity(1.0f);
	InspectPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	PinnedItemId = Item.View.ItemId;
}

void FWacomWorldCardInteractionPresenter::CloseInspect()
{
	PinnedItemId.Invalidate();
	if (InspectPanel.IsValid())
	{
		InspectPanel->RemoveFromParent();
		InspectPanel.Reset();
	}
}

FVector2D FWacomWorldCardInteractionPresenter::GetViewportLogicalSize(
	AWacomPlayerController& PlayerController)
{
	int32 ViewportX = 0;
	int32 ViewportY = 0;
	PlayerController.GetViewportSize(ViewportX, ViewportY);
	const float ViewportScale = FMath::Max(
		0.01f,
		UWidgetLayoutLibrary::GetViewportScale(&PlayerController));
	const FVector2D Result(
		static_cast<float>(ViewportX) / ViewportScale,
		static_cast<float>(ViewportY) / ViewportScale);
	return Result.X > 0.0f && Result.Y > 0.0f
		? Result
		: FVector2D(1920.0f, 1080.0f);
}

FVector2D FWacomWorldCardInteractionPresenter::ComputeTooltipPosition(
	const FVector2D& MousePosition,
	const FVector2D& TooltipSize,
	const FVector2D& ViewportSize,
	const FWacomWorldCardInteractionStyle& Style)
{
	const float Margin = Style.ViewportSafeMarginPixels;
	FVector2D Position(
		MousePosition.X + Style.TooltipMouseOffsetPixels.X,
		MousePosition.Y + Style.TooltipMouseOffsetPixels.Y - TooltipSize.Y);
	if (Position.X + TooltipSize.X + Margin > ViewportSize.X)
	{
		Position.X =
			MousePosition.X
			- FMath::Abs(Style.TooltipMouseOffsetPixels.X)
			- TooltipSize.X;
	}
	if (Position.Y < Margin)
	{
		Position.Y =
			MousePosition.Y
			+ FMath::Abs(Style.TooltipMouseOffsetPixels.Y);
	}
	Position.X = FMath::Clamp(
		Position.X,
		Margin,
		FMath::Max(Margin, ViewportSize.X - Margin - TooltipSize.X));
	Position.Y = FMath::Clamp(
		Position.Y,
		Margin,
		FMath::Max(Margin, ViewportSize.Y - Margin - TooltipSize.Y));
	return Position;
}

FVector2D FWacomWorldCardInteractionPresenter::ComputeInspectPosition(
	const bool bTargetIsOnLeft,
	const FVector2D& ViewportSize,
	const FWacomWorldCardInteractionStyle& Style)
{
	const float Margin = Style.InspectPanelMarginPixels;
	const FVector2D PanelSize = Style.InspectPanelSizePixels;
	const float X = bTargetIsOnLeft
		? ViewportSize.X - Margin - PanelSize.X
		: Margin;
	return FVector2D(
		FMath::Clamp(
			X,
			Margin,
			FMath::Max(Margin, ViewportSize.X - Margin - PanelSize.X)),
		FMath::Clamp(
			(ViewportSize.Y - PanelSize.Y) * 0.5f,
			Margin,
			FMath::Max(Margin, ViewportSize.Y - Margin - PanelSize.Y)));
}

#if WITH_AUTOMATION_TESTS
bool FWacomWorldCardInteractionPresenter::IsTooltipVisibleForTest() const
{
	return TooltipWidget.IsValid()
		&& TooltipWidget->GetVisibility() != ESlateVisibility::Collapsed;
}

bool FWacomWorldCardInteractionPresenter::IsInspectVisibleForTest() const
{
	return InspectPanel.IsValid()
		&& InspectPanel->GetVisibility() != ESlateVisibility::Collapsed;
}

FTransform
FWacomWorldCardInteractionPresenter::ComputeHoverWorldTransformForTest(
	const FTransform& BaseWorld,
	const FVector& CameraLocation,
	const float HoverAlpha,
	const FWacomWorldCardInteractionStyle& Style)
{
	return ComputeHoverWorldTransform(
		BaseWorld,
		CameraLocation,
		HoverAlpha,
		Style.Sanitized());
}
#endif
