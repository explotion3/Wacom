// Copyright Wacom. All Rights Reserved.

#include "UI/Menus/WacomMainMenuScreen.h"

#define LOCTEXT_NAMESPACE "WacomMainMenu"

#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
	bool IsConfiguredVisible(const UWidget* Widget)
	{
		if (!Widget)
		{
			return false;
		}
		const ESlateVisibility Visibility = Widget->GetVisibility();
		return Visibility != ESlateVisibility::Collapsed
			&& Visibility != ESlateVisibility::Hidden;
	}

	void SetButtonInteractionEnabled(
		UWacomMainMenuButtonWidget* Button,
		bool bEnabled)
	{
		if (!Button || Button->IsInteractionEnabled() == bEnabled)
		{
			return;
		}

		Button->SetIsInteractionEnabled(bEnabled);
		Button->BP_OnInteractabilityChanged(bEnabled);
	}

	UWacomMainMenuButtonWidget* MakeLabelButton(
		UWidgetTree* Tree,
		FName Name,
		const FText& Label,
		UVerticalBox* Parent)
	{
		UWacomMainMenuButtonWidget* Button = Tree->ConstructWidget<UWacomMainMenuButtonWidget>(
			UWacomMainMenuButtonWidget::StaticClass(),
			Name);
		Button->SetButtonText(Label);

		if (UVerticalBoxSlot* ButtonSlot = Parent->AddChildToVerticalBox(Button))
		{
			ButtonSlot->SetPadding(FMargin(0.0f, 7.0f));
			ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
		}
		return Button;
	}
}

UWacomMainMenuButtonWidget::UWacomMainMenuButtonWidget(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

TSharedRef<SWidget> UWacomMainMenuButtonWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UBorder* Root = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			TEXT("Root"));
		Root->SetPadding(FMargin(14.0f, 8.0f));
		Root->SetBrushColor(FLinearColor(0.06f, 0.07f, 0.09f, 0.88f));
		WidgetTree->RootWidget = Root;

		ButtonText = WidgetTree->ConstructWidget<UCommonTextBlock>(
			UCommonTextBlock::StaticClass(),
			TEXT("ButtonText"));
		ButtonText->SetText(GetButtonText());
		ButtonText->SetJustification(ETextJustify::Left);
		Root->AddChild(ButtonText);
	}

	return Super::RebuildWidget();
}

UWacomMainMenuScreen::UWacomMainMenuScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoRestoreFocus = true;
}

TSharedRef<SWidget> UWacomMainMenuScreen::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		UHorizontalBox* Content = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), TEXT("MainMenuContent"));
		if (UCanvasPanelSlot* ContentSlot = Root->AddChildToCanvas(Content))
		{
			ContentSlot->SetAnchors(FAnchors(0.08f, 0.15f, 0.92f, 0.85f));
			ContentSlot->SetOffsets(FMargin(0.0f));
			ContentSlot->SetAlignment(FVector2D::ZeroVector);
		}

		// Do not name this widget "Navigation": that shadows UWidget::Navigation and
		// makes the UE 5.8 Widget Blueprint Designer preview build invalid navigation metadata.
		UVerticalBox* NavigationBox = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("MainMenuNavigationBox"));
		if (UHorizontalBoxSlot* NavigationSlot = Content->AddChildToHorizontalBox(NavigationBox))
		{
			NavigationSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			NavigationSlot->SetPadding(FMargin(0.0f, 0.0f, 48.0f, 0.0f));
			NavigationSlot->SetHorizontalAlignment(HAlign_Fill);
			NavigationSlot->SetVerticalAlignment(VAlign_Center);
		}

		UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
		Title->SetText(LOCTEXT("Title", "Wacom"));
		FSlateFontInfo TitleFont = Title->GetFont();
		TitleFont.Size = 48;
		Title->SetFont(TitleFont);
		if (UVerticalBoxSlot* TitleSlot = NavigationBox->AddChildToVerticalBox(Title))
		{
			TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 32.0f));
			TitleSlot->SetHorizontalAlignment(HAlign_Left);
		}

		ContinueButton = MakeLabelButton(
			WidgetTree, TEXT("ContinueButton"), LOCTEXT("ContinueJourney", "继续旅程"), NavigationBox);
		NewJourneyButton = MakeLabelButton(
			WidgetTree, TEXT("NewJourneyButton"), LOCTEXT("StartNewJourney", "开始新旅程"), NavigationBox);
		JourneyHistoryButton = MakeLabelButton(
			WidgetTree, TEXT("JourneyHistoryButton"), LOCTEXT("JourneyHistory", "旅程记录"), NavigationBox);
		SettingsButton = MakeLabelButton(
			WidgetTree, TEXT("SettingsButton"), LOCTEXT("Settings", "设置"), NavigationBox);
		CreditsButton = MakeLabelButton(
			WidgetTree, TEXT("CreditsButton"), LOCTEXT("Credits", "制作人员"), NavigationBox);
		QuitButton = MakeLabelButton(
			WidgetTree, TEXT("QuitButton"), LOCTEXT("Quit", "退出游戏"), NavigationBox);

		UVerticalBox* Summary = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("JourneySummary"));
		if (UHorizontalBoxSlot* SummarySlot = Content->AddChildToHorizontalBox(Summary))
		{
			SummarySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			SummarySlot->SetPadding(FMargin(48.0f, 0.0f, 0.0f, 0.0f));
			SummarySlot->SetHorizontalAlignment(HAlign_Fill);
			SummarySlot->SetVerticalAlignment(VAlign_Center);
		}

		ActiveJourneyTitleText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("ActiveJourneyTitleText"));
		FSlateFontInfo SummaryTitleFont = ActiveJourneyTitleText->GetFont();
		SummaryTitleFont.Size = 30;
		ActiveJourneyTitleText->SetFont(SummaryTitleFont);
		if (UVerticalBoxSlot* SummaryTitleSlot = Summary->AddChildToVerticalBox(ActiveJourneyTitleText))
		{
			SummaryTitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
		}

		ActiveJourneySummaryText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("ActiveJourneySummaryText"));
		ActiveJourneySummaryText->SetAutoWrapText(true);
		Summary->AddChildToVerticalBox(ActiveJourneySummaryText);
	}

	return Super::RebuildWidget();
}

void UWacomMainMenuScreen::NativeConstruct()
{
	Super::NativeConstruct();

	if (ContinueButton)
	{
		ContinueButton->OnClicked().RemoveAll(this);
		ContinueButton->OnClicked().AddUObject(this, &UWacomMainMenuScreen::HandleContinueClicked);
	}
	if (NewJourneyButton)
	{
		NewJourneyButton->OnClicked().RemoveAll(this);
		NewJourneyButton->OnClicked().AddUObject(this, &UWacomMainMenuScreen::HandleNewJourneyClicked);
	}
	if (JourneyHistoryButton)
	{
		JourneyHistoryButton->OnClicked().RemoveAll(this);
		JourneyHistoryButton->OnClicked().AddUObject(this, &UWacomMainMenuScreen::HandleJourneyHistoryClicked);
	}
	if (SettingsButton)
	{
		SettingsButton->OnClicked().RemoveAll(this);
		SettingsButton->OnClicked().AddUObject(this, &UWacomMainMenuScreen::HandleSettingsClicked);
	}
	if (CreditsButton)
	{
		CreditsButton->OnClicked().RemoveAll(this);
		CreditsButton->OnClicked().AddUObject(this, &UWacomMainMenuScreen::HandleCreditsClicked);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked().RemoveAll(this);
		QuitButton->OnClicked().AddUObject(this, &UWacomMainMenuScreen::HandleQuitClicked);
	}

	RefreshFromViewData();
}

void UWacomMainMenuScreen::NativeDestruct()
{
	if (ContinueButton)
	{
		ContinueButton->OnClicked().RemoveAll(this);
	}
	if (NewJourneyButton)
	{
		NewJourneyButton->OnClicked().RemoveAll(this);
	}
	if (JourneyHistoryButton)
	{
		JourneyHistoryButton->OnClicked().RemoveAll(this);
	}
	if (SettingsButton)
	{
		SettingsButton->OnClicked().RemoveAll(this);
	}
	if (CreditsButton)
	{
		CreditsButton->OnClicked().RemoveAll(this);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked().RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UWacomMainMenuScreen::ApplyViewData(const FWacomMainMenuViewData& InViewData)
{
	ViewData = InViewData;
	RefreshFromViewData();
	BP_OnMainMenuViewDataApplied(ViewData);
}

void UWacomMainMenuScreen::RefreshFromViewData()
{
	if (ContinueButton)
	{
		ContinueButton->SetVisibility(ViewData.bHasActiveJourney
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed);
		SetButtonInteractionEnabled(
			ContinueButton,
			ViewData.bHasActiveJourney && ViewData.bCanContinueJourney);
	}
	if (JourneyHistoryButton)
	{
		JourneyHistoryButton->SetVisibility(ViewData.bShowJourneyHistory
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed);
		SetButtonInteractionEnabled(JourneyHistoryButton, ViewData.bShowJourneyHistory);
	}
	if (SettingsButton)
	{
		SettingsButton->SetVisibility(ViewData.bShowSettings
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed);
		SetButtonInteractionEnabled(SettingsButton, ViewData.bShowSettings);
	}
	if (CreditsButton)
	{
		CreditsButton->SetVisibility(ViewData.bShowCredits
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed);
		SetButtonInteractionEnabled(CreditsButton, ViewData.bShowCredits);
	}

	if (ActiveJourneyTitleText)
	{
		ActiveJourneyTitleText->SetText(
			ViewData.bHasActiveJourney && !ViewData.ActiveJourneyTitle.IsEmpty()
				? ViewData.ActiveJourneyTitle
				: LOCTEXT("NewJourneySummaryTitle", "准备启程"));
	}
	if (ActiveJourneySummaryText)
	{
		ActiveJourneySummaryText->SetText(
			ViewData.bHasActiveJourney && !ViewData.ActiveJourneySummary.IsEmpty()
				? ViewData.ActiveJourneySummary
				: LOCTEXT("NewJourneySummary", "开始一段新的旅程。"));
	}
}

UWidget* UWacomMainMenuScreen::NativeGetDesiredFocusTarget() const
{
	if (ContinueButton
		&& IsConfiguredVisible(ContinueButton)
		&& ContinueButton->IsInteractionEnabled())
	{
		return ContinueButton;
	}
	if (NewJourneyButton
		&& IsConfiguredVisible(NewJourneyButton)
		&& NewJourneyButton->IsInteractionEnabled())
	{
		return NewJourneyButton;
	}
	return Super::NativeGetDesiredFocusTarget();
}

bool UWacomMainMenuScreen::IsActionAvailable(EWacomMainMenuAction Action) const
{
	switch (Action)
	{
	case EWacomMainMenuAction::ContinueJourney:
		return ViewData.bHasActiveJourney && ViewData.bCanContinueJourney;
	case EWacomMainMenuAction::StartNewJourney:
	case EWacomMainMenuAction::Quit:
		return true;
	case EWacomMainMenuAction::JourneyHistory:
		return ViewData.bShowJourneyHistory;
	case EWacomMainMenuAction::Settings:
		return ViewData.bShowSettings;
	case EWacomMainMenuAction::Credits:
		return ViewData.bShowCredits;
	default:
		return false;
	}
}

void UWacomMainMenuScreen::RequestAction(EWacomMainMenuAction Action)
{
	if (!IsActionAvailable(Action))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[MainMenu] Reject unavailable action: %d"),
			static_cast<int32>(Action));
		return;
	}

	OnActionRequestedNative.Broadcast(Action);
}

void UWacomMainMenuScreen::HandleContinueClicked()
{
	RequestAction(EWacomMainMenuAction::ContinueJourney);
}

void UWacomMainMenuScreen::HandleNewJourneyClicked()
{
	RequestAction(EWacomMainMenuAction::StartNewJourney);
}

void UWacomMainMenuScreen::HandleJourneyHistoryClicked()
{
	RequestAction(EWacomMainMenuAction::JourneyHistory);
}

void UWacomMainMenuScreen::HandleSettingsClicked()
{
	RequestAction(EWacomMainMenuAction::Settings);
}

void UWacomMainMenuScreen::HandleCreditsClicked()
{
	RequestAction(EWacomMainMenuAction::Credits);
}

void UWacomMainMenuScreen::HandleQuitClicked()
{
	RequestAction(EWacomMainMenuAction::Quit);
}

#if WITH_AUTOMATION_TESTS
FWacomMainMenuScreenAutomationTestView UWacomMainMenuScreen::GetAutomationTestViewForTest() const
{
	FWacomMainMenuScreenAutomationTestView TestView;
	TestView.bContinueVisible = IsConfiguredVisible(ContinueButton);
	TestView.bContinueEnabled = ContinueButton && ContinueButton->IsInteractionEnabled();
	TestView.bNewJourneyVisible = IsConfiguredVisible(NewJourneyButton);
	TestView.bJourneyHistoryVisible = IsConfiguredVisible(JourneyHistoryButton);
	TestView.bSettingsVisible = IsConfiguredVisible(SettingsButton);
	TestView.bCreditsVisible = IsConfiguredVisible(CreditsButton);
	TestView.bQuitVisible = IsConfiguredVisible(QuitButton);
	TestView.bAutoRestoreFocus = bAutoRestoreFocus;
	if (const UWidget* FocusTarget = GetDesiredFocusTarget())
	{
		TestView.DesiredFocusTargetName = FocusTarget->GetFName();
	}
	if (ActiveJourneyTitleText)
	{
		TestView.SummaryTitle = ActiveJourneyTitleText->GetText();
	}
	if (ActiveJourneySummaryText)
	{
		TestView.SummaryBody = ActiveJourneySummaryText->GetText();
	}
	return TestView;
}
#endif

#undef LOCTEXT_NAMESPACE
