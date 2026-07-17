// Copyright Wacom. All Rights Reserved.

#include "UI/Menus/WacomJourneySummaryScreen.h"

#define LOCTEXT_NAMESPACE "WacomJourneySummary"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/Foundation/WacomMenuButtonWidget.h"

namespace
{
	UTextBlock* AddSummaryText(
		UWidgetTree& Tree,
		UVerticalBox& Parent,
		FName Name,
		int32 FontSize,
		const FMargin& Padding)
	{
		UTextBlock* Text = Tree.ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Text->SetJustification(ETextJustify::Center);
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = FontSize;
		Text->SetFont(Font);
		if (UVerticalBoxSlot* Slot = Parent.AddChildToVerticalBox(Text))
		{
			Slot->SetPadding(Padding);
			Slot->SetHorizontalAlignment(HAlign_Center);
		}
		return Text;
	}
}

UWacomJourneySummaryScreen::UWacomJourneySummaryScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoRestoreFocus = true;
}

TSharedRef<SWidget> UWacomJourneySummaryScreen::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
	}

	if (!WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("JourneySummaryRoot"));
		WidgetTree->RootWidget = Root;

		UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("JourneySummaryBackdrop"));
		Backdrop->SetBrushColor(FLinearColor(0.01f, 0.015f, 0.025f, 0.94f));
		if (UOverlaySlot* BackdropSlot = Root->AddChildToOverlay(Backdrop))
		{
			BackdropSlot->SetHorizontalAlignment(HAlign_Fill);
			BackdropSlot->SetVerticalAlignment(VAlign_Fill);
		}

		UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("JourneySummaryContent"));
		if (UOverlaySlot* ContentSlot = Root->AddChildToOverlay(Content))
		{
			ContentSlot->SetHorizontalAlignment(HAlign_Center);
			ContentSlot->SetVerticalAlignment(VAlign_Center);
			ContentSlot->SetPadding(FMargin(72.0f));
		}

		StatusTitleText = AddSummaryText(*WidgetTree, *Content, TEXT("StatusTitleText"), 44, FMargin(0.0f, 0.0f, 0.0f, 14.0f));
		JourneyTitleText = AddSummaryText(*WidgetTree, *Content, TEXT("JourneyTitleText"), 30, FMargin(0.0f, 0.0f, 0.0f, 28.0f));
		CompletionDayText = AddSummaryText(*WidgetTree, *Content, TEXT("CompletionDayText"), 22, FMargin(0.0f, 5.0f));
		FloorProgressText = AddSummaryText(*WidgetTree, *Content, TEXT("FloorProgressText"), 22, FMargin(0.0f, 5.0f));
		NodeProgressText = AddSummaryText(*WidgetTree, *Content, TEXT("NodeProgressText"), 22, FMargin(0.0f, 5.0f));
		FinalPressureText = AddSummaryText(*WidgetTree, *Content, TEXT("FinalPressureText"), 22, FMargin(0.0f, 5.0f, 0.0f, 28.0f));

		ContinueButton = WidgetTree->ConstructWidget<UWacomMenuButtonWidget>(
			UWacomMenuButtonWidget::StaticClass(), TEXT("ContinueButton"));
		ContinueButton->SetButtonText(LOCTEXT("Continue", "返回主菜单"));
		if (UVerticalBoxSlot* ButtonSlot = Content->AddChildToVerticalBox(ContinueButton))
		{
			ButtonSlot->SetPadding(FMargin(0.0f, 10.0f));
			ButtonSlot->SetHorizontalAlignment(HAlign_Center);
		}
	}

	RefreshView();
	return Super::RebuildWidget();
}

void UWacomJourneySummaryScreen::NativeConstruct()
{
	Super::NativeConstruct();
	bContinueIntentSent = false;
	if (ContinueButton)
	{
		ContinueButton->OnClicked().RemoveAll(this);
		ContinueButton->OnClicked().AddUObject(this, &UWacomJourneySummaryScreen::HandleContinueClicked);
	}
	RefreshView();
}

void UWacomJourneySummaryScreen::NativeDestruct()
{
	if (ContinueButton)
	{
		ContinueButton->OnClicked().RemoveAll(this);
	}
	OnContinueRequestedNative.Clear();
	Super::NativeDestruct();
}

void UWacomJourneySummaryScreen::ApplyViewData(const FWacomJourneySummaryViewData& InViewData)
{
	ViewData = InViewData;
	RefreshView();
	BP_OnJourneySummaryViewDataApplied(ViewData);
}

void UWacomJourneySummaryScreen::RequestContinue()
{
	if (bContinueIntentSent)
	{
		return;
	}
	bContinueIntentSent = true;
	OnContinueRequestedNative.Broadcast();
}

void UWacomJourneySummaryScreen::HandleContinueClicked()
{
	RequestContinue();
}

UWidget* UWacomJourneySummaryScreen::NativeGetDesiredFocusTarget() const
{
	return ContinueButton;
}

FReply UWacomJourneySummaryScreen::NativeHandleBackRequested()
{
	RequestContinue();
	return FReply::Handled();
}

void UWacomJourneySummaryScreen::RefreshView()
{
	const FText Status = ViewData.StatusTitle.IsEmpty()
		? LOCTEXT("DefaultStatusTitle", "Journey 成功")
		: ViewData.StatusTitle;
	const FText Journey = ViewData.JourneyTitle.IsEmpty()
		? LOCTEXT("UnknownJourney", "未知 Journey")
		: ViewData.JourneyTitle;

	if (StatusTitleText) { StatusTitleText->SetText(Status); }
	if (JourneyTitleText) { JourneyTitleText->SetText(Journey); }
	if (CompletionDayText)
	{
		CompletionDayText->SetText(FText::Format(
			LOCTEXT("CompletionDayFormat", "完成天数：{0}"),
			FText::AsNumber(ViewData.CompletionDay)));
	}
	if (FloorProgressText)
	{
		FloorProgressText->SetText(FText::Format(
			LOCTEXT("FloorProgressFormat", "Floor 进度：{0} / {1}"),
			FText::AsNumber(ViewData.EnteredFloorCount),
			FText::AsNumber(ViewData.TotalFloorCount)));
	}
	if (NodeProgressText)
	{
		NodeProgressText->SetText(FText::Format(
			LOCTEXT("NodeProgressFormat", "Node 进度：{0} / {1}"),
			FText::AsNumber(ViewData.ResolvedNodeCount),
			FText::AsNumber(ViewData.TotalNodeCount)));
	}
	if (FinalPressureText)
	{
		FinalPressureText->SetText(FText::Format(
			LOCTEXT("PressureFormat", "最终压力：{0}"),
			FText::AsNumber(ViewData.FinalPressure)));
	}
}

#if WITH_AUTOMATION_TESTS
FWacomJourneySummaryScreenAutomationTestView UWacomJourneySummaryScreen::GetAutomationTestViewForTest() const
{
	FWacomJourneySummaryScreenAutomationTestView View;
	View.StatusTitle = StatusTitleText ? StatusTitleText->GetText() : FText();
	View.JourneyTitle = JourneyTitleText ? JourneyTitleText->GetText() : FText();
	View.DayText = CompletionDayText ? CompletionDayText->GetText() : FText();
	View.FloorProgressText = FloorProgressText ? FloorProgressText->GetText() : FText();
	View.NodeProgressText = NodeProgressText ? NodeProgressText->GetText() : FText();
	View.PressureText = FinalPressureText ? FinalPressureText->GetText() : FText();
	View.bHasContinueButton = ContinueButton != nullptr;
	View.bContinueButtonFocusable = ContinueButton && ContinueButton->GetIsFocusable();
	View.bAutoRestoreFocus = bAutoRestoreFocus;
	View.bContinueIntentSent = bContinueIntentSent;
	View.DesiredFocusTargetName = NativeGetDesiredFocusTarget()
		? NativeGetDesiredFocusTarget()->GetFName()
		: NAME_None;
	return View;
}
#endif

#undef LOCTEXT_NAMESPACE
