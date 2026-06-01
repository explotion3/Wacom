// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/BattlePresentationStackEntryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "UI/Card/WacomCardView.h"

#define LOCTEXT_NAMESPACE "WacomBattlePresentationStackEntry"

namespace
{
	constexpr float DefaultCardDesignWidth = 260.0f;
	constexpr float DefaultCardDesignHeight = 380.0f;
	constexpr float StackExitDurationSeconds = 0.16f;
	constexpr float StackExitYOffsetPixels = -24.0f;
	constexpr float StackExitScaleMultiplier = 0.92f;
}

TSharedRef<SWidget> UBattlePresentationStackEntryWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		UScaleBox* ScaleBox = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("MiniCardScaleBox"));
		ScaleBox->SetStretch(EStretch::ScaleToFit);
		if (UOverlaySlot* ScaleSlot = Root->AddChildToOverlay(ScaleBox))
		{
			ScaleSlot->SetHorizontalAlignment(HAlign_Fill);
			ScaleSlot->SetVerticalAlignment(VAlign_Fill);
		}

		MiniCardScaleHost = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MiniCardScaleHost"));
		MiniCardScaleHost->SetWidthOverride(DefaultCardDesignWidth);
		MiniCardScaleHost->SetHeightOverride(DefaultCardDesignHeight);
		ScaleBox->AddChild(MiniCardScaleHost);

		CardHost = MiniCardScaleHost;
	}

	return Super::RebuildWidget();
}

void UBattlePresentationStackEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	EnsureMiniCardView();
	ApplyCurrentEntry();
}

void UBattlePresentationStackEntryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!CurrentEntry.bIsExiting)
	{
		return;
	}

	ExitElapsedSeconds += FMath::Max(0.0f, InDeltaTime);
	const float Alpha = FMath::Clamp(ExitElapsedSeconds / StackExitDurationSeconds, 0.0f, 1.0f);
	ApplyExitVisual(Alpha);
}

#if WITH_AUTOMATION_TESTS
void UBattlePresentationStackEntryWidget::TickExitForTest(float DeltaSeconds)
{
	NativeTick(FGeometry(), DeltaSeconds);
}
#endif

void UBattlePresentationStackEntryWidget::SetPresentationStackEntryData(
	const FWacomBattlePresentationStackEntryView& InEntry)
{
	CurrentEntry = InEntry;
	if (CurrentEntry.bIsExiting != bPreviousExitingState)
	{
		ExitElapsedSeconds = 0.0f;
		bPreviousExitingState = CurrentEntry.bIsExiting;
	}
	ApplyCurrentEntry();
	BP_OnEntryUpdated(InEntry);
}

void UBattlePresentationStackEntryWidget::SetMiniCardViewClass(TSubclassOf<UWacomCardView> InClass)
{
	if (RuntimeMiniCardViewClass == InClass)
	{
		return;
	}

	RuntimeMiniCardViewClass = InClass;
	if (MiniCardView && CardHost)
	{
		CardHost->RemoveChild(MiniCardView);
		MiniCardView = nullptr;
	}
	EnsureMiniCardView();
	ApplyCurrentEntry();
}

void UBattlePresentationStackEntryWidget::EnsureMiniCardView()
{
	if (MiniCardView || !CardHost)
	{
		return;
	}

	UClass* WidgetClass = RuntimeMiniCardViewClass
		? RuntimeMiniCardViewClass.Get()
		: UWacomCardView::StaticClass();
	UWacomCardView* CreatedView = GetWorld()
		? CreateWidget<UWacomCardView>(this, WidgetClass)
		: NewObject<UWacomCardView>(this, WidgetClass);
	if (!CreatedView)
	{
		return;
	}

	CreatedView->SetVisibility(ESlateVisibility::HitTestInvisible);
	CardHost->AddChild(CreatedView);
	MiniCardView = CreatedView;
}

void UBattlePresentationStackEntryWidget::ApplyCurrentEntry()
{
	EnsureMiniCardView();
	if (MiniCardView)
	{
		MiniCardView->SetCardViewData(CurrentEntry.CardViewData);
	}

	ApplyExitVisual(CurrentEntry.bIsExiting
		? FMath::Clamp(ExitElapsedSeconds / StackExitDurationSeconds, 0.0f, 1.0f)
		: 0.0f);
	SetVisibility(CurrentEntry.EntryId != INDEX_NONE
		? ESlateVisibility::HitTestInvisible
		: ESlateVisibility::Collapsed);
}

void UBattlePresentationStackEntryWidget::ApplyExitVisual(float NormalizedAlpha)
{
	const float Alpha = CurrentEntry.bIsExiting ? FMath::Clamp(NormalizedAlpha, 0.0f, 1.0f) : 0.0f;
	SetRenderOpacity(1.0f - Alpha);
	SetRenderTranslation(FVector2D(0.0f, StackExitYOffsetPixels * Alpha));
	const float Scale = FMath::Lerp(1.0f, StackExitScaleMultiplier, Alpha);
	SetRenderScale(FVector2D(Scale, Scale));
}

#undef LOCTEXT_NAMESPACE
