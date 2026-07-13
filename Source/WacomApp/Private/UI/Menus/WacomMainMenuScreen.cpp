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
#include "Containers/Ticker.h"
#include "Engine/GameInstance.h"
#include "Settings/WacomSettingsSubsystem.h"

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
		Button->RefreshPresentationState();
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

void UWacomMainMenuButtonWidget::RefreshPresentationState()
{
	RefreshPresentationTarget();
}

TSharedRef<SWidget> UWacomMainMenuButtonWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		ButtonBackdrop = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			TEXT("ButtonBackdrop"));
		ButtonBackdrop->SetPadding(FMargin(14.0f, 8.0f));
		WidgetTree->RootWidget = ButtonBackdrop;

		ButtonText = WidgetTree->ConstructWidget<UCommonTextBlock>(
			UCommonTextBlock::StaticClass(),
			TEXT("ButtonText"));
		ButtonText->SetText(GetButtonText());
		ButtonText->SetJustification(ETextJustify::Left);
		ButtonBackdrop->AddChild(ButtonText);
	}

	return Super::RebuildWidget();
}

void UWacomMainMenuButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindRuntimeSettings();
	SetRenderTransformPivot(FVector2D(0.0f, 0.5f));
	RefreshPresentationTarget(/*bApplyImmediately*/true);
}

void UWacomMainMenuButtonWidget::NativeDestruct()
{
	UnbindRuntimeSettings();
	StopPresentationTicker();
	Super::NativeDestruct();
}

FReply UWacomMainMenuButtonWidget::NativeOnFocusReceived(
	const FGeometry& InGeometry,
	const FFocusEvent& InFocusEvent)
{
	FReply Reply = Super::NativeOnFocusReceived(InGeometry, InFocusEvent);
	RefreshPresentationTarget();
	return Reply;
}

void UWacomMainMenuButtonWidget::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnFocusLost(InFocusEvent);
	RefreshPresentationTarget();
}

void UWacomMainMenuButtonWidget::NativeOnHovered()
{
	Super::NativeOnHovered();
	RefreshPresentationTarget();
}

void UWacomMainMenuButtonWidget::NativeOnUnhovered()
{
	Super::NativeOnUnhovered();
	RefreshPresentationTarget();
}

void UWacomMainMenuButtonWidget::NativeOnPressed()
{
	Super::NativeOnPressed();
	bPresentationPressed = true;
	RefreshPresentationTarget();
}

void UWacomMainMenuButtonWidget::NativeOnReleased()
{
	Super::NativeOnReleased();
	bPresentationPressed = false;
	RefreshPresentationTarget();
}

void UWacomMainMenuButtonWidget::NativeOnEnabled()
{
	Super::NativeOnEnabled();
	RefreshPresentationTarget();
}

void UWacomMainMenuButtonWidget::NativeOnDisabled()
{
	Super::NativeOnDisabled();
	bPresentationPressed = false;
	RefreshPresentationTarget();
}

void UWacomMainMenuButtonWidget::RefreshPresentationTarget(bool bApplyImmediately)
{
	const bool bEmphasized = IsInteractionEnabled()
		&& (IsHovered() || HasAnyUserFocus() || GetSelected());
	TargetEmphasis = bEmphasized ? 1.0f : 0.0f;
	TargetPressedAmount = IsInteractionEnabled() && bPresentationPressed ? 1.0f : 0.0f;

	if (bApplyImmediately || bRuntimeSimplifiedMotion)
	{
		CurrentEmphasis = TargetEmphasis;
		CurrentPressedAmount = TargetPressedAmount;
		ApplyPresentation(CurrentEmphasis, CurrentPressedAmount);
		return;
	}

	if (!PresentationTickerHandle.IsValid())
	{
		PresentationTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateWeakLambda(this, [this](float DeltaTime)
			{
				return TickPresentation(DeltaTime);
			}));
	}
}

void UWacomMainMenuButtonWidget::BindRuntimeSettings()
{
	UGameInstance* GameInstance = GetGameInstance();
	UWacomSettingsSubsystem* SettingsSubsystem = GameInstance
		? GameInstance->GetSubsystem<UWacomSettingsSubsystem>()
		: nullptr;
	if (!SettingsSubsystem)
	{
		return;
	}
	RuntimeSettingsChangedHandle = SettingsSubsystem->OnRuntimeSettingsChangedNative().AddUObject(
		this,
		&UWacomMainMenuButtonWidget::HandleRuntimeSettingsChanged);
	HandleRuntimeSettingsChanged(
		SettingsSubsystem->GetCurrentSnapshot(),
		EWacomRuntimeSettingsChangeReason::Startup);
}

void UWacomMainMenuButtonWidget::UnbindRuntimeSettings()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UWacomSettingsSubsystem* SettingsSubsystem =
			GameInstance->GetSubsystem<UWacomSettingsSubsystem>())
		{
			SettingsSubsystem->OnRuntimeSettingsChangedNative().Remove(RuntimeSettingsChangedHandle);
		}
	}
	RuntimeSettingsChangedHandle.Reset();
}

void UWacomMainMenuButtonWidget::HandleRuntimeSettingsChanged(
	const FWacomLocalSettingsSnapshot& Snapshot,
	EWacomRuntimeSettingsChangeReason /*Reason*/)
{
	bRuntimeSimplifiedMotion = Snapshot.UIMotionMode == EWacomUIMotionMode::Simplified;
	if (bRuntimeSimplifiedMotion)
	{
		StopPresentationTicker();
		RefreshPresentationTarget(/*bApplyImmediately*/true);
	}
}

bool UWacomMainMenuButtonWidget::TickPresentation(float DeltaTime)
{
	CurrentEmphasis = FMath::FInterpConstantTo(
		CurrentEmphasis, TargetEmphasis, DeltaTime, 7.5f);
	CurrentPressedAmount = FMath::FInterpConstantTo(
		CurrentPressedAmount, TargetPressedAmount, DeltaTime, 12.0f);
	ApplyPresentation(CurrentEmphasis, CurrentPressedAmount);

	const bool bSettled = FMath::IsNearlyEqual(CurrentEmphasis, TargetEmphasis, 0.001f)
		&& FMath::IsNearlyEqual(CurrentPressedAmount, TargetPressedAmount, 0.001f);
	if (bSettled)
	{
		PresentationTickerHandle.Reset();
		return false;
	}
	return true;
}

void UWacomMainMenuButtonWidget::ApplyPresentation(float Emphasis, float PressedAmount)
{
	const bool bInteractable = IsInteractionEnabled();
	const FLinearColor RestingBackdrop(0.025f, 0.035f, 0.055f, 0.84f);
	const FLinearColor FocusedBackdrop(0.075f, 0.115f, 0.145f, 0.96f);
	const FLinearColor DisabledBackdrop(0.02f, 0.025f, 0.035f, 0.58f);
	const FLinearColor RestingText(0.72f, 0.76f, 0.78f, 1.0f);
	const FLinearColor FocusedText(0.98f, 0.91f, 0.61f, 1.0f);
	const FLinearColor DisabledText(0.36f, 0.39f, 0.42f, 1.0f);
	const FLinearColor AccentColor(0.30f, 0.88f, 0.82f, 1.0f);

	FLinearColor BackdropColor = bInteractable
		? FMath::Lerp(RestingBackdrop, FocusedBackdrop, Emphasis)
		: DisabledBackdrop;
	BackdropColor *= 1.0f - PressedAmount * 0.14f;
	BackdropColor.A = bInteractable
		? FMath::Lerp(RestingBackdrop.A, FocusedBackdrop.A, Emphasis)
		: DisabledBackdrop.A;
	if (ButtonBackdrop)
	{
		ButtonBackdrop->SetBrushColor(BackdropColor);
	}

	if (ButtonAccent)
	{
		ButtonAccent->SetBrushColor(AccentColor);
		ButtonAccent->SetRenderOpacity(bInteractable ? 0.22f + 0.78f * Emphasis : 0.10f);
	}

	const FLinearColor TextColor = bInteractable
		? FMath::Lerp(RestingText, FocusedText, Emphasis)
		: DisabledText;
	if (ButtonText)
	{
		ButtonText->SetColorAndOpacity(FSlateColor(TextColor));
	}
	if (ButtonGlyph)
	{
		ButtonGlyph->SetColorAndOpacity(FSlateColor(AccentColor));
		ButtonGlyph->SetRenderOpacity(bInteractable ? 0.18f + 0.82f * Emphasis : 0.08f);
	}

	SetRenderTranslation(FVector2D(10.0f * Emphasis, 1.5f * PressedAmount));
	const float PressedScale = 1.0f - 0.012f * PressedAmount;
	SetRenderScale(FVector2D(PressedScale));
}

void UWacomMainMenuButtonWidget::StopPresentationTicker()
{
	if (PresentationTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(PresentationTickerHandle);
		PresentationTickerHandle.Reset();
	}
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
			UHorizontalBox::StaticClass(), TEXT("MenuContentRoot"));
		MenuContentRoot = Content;
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
			UVerticalBox::StaticClass(), TEXT("JourneySummaryPanel"));
		JourneySummaryPanel = Summary;
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
	BindRuntimeSettings();

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
	UnbindRuntimeSettings();
	StopIntroPresentation();
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

void UWacomMainMenuScreen::NativeOnActivated()
{
	Super::NativeOnActivated();
	StartIntroPresentation();
}

void UWacomMainMenuScreen::NativeOnDeactivated()
{
	StopIntroPresentation();
	FinishIntroPresentation();
	Super::NativeOnDeactivated();
}

void UWacomMainMenuScreen::StartIntroPresentation()
{
	StopIntroPresentation();
	IntroElapsedSeconds = 0.0f;
	if (bRuntimeSimplifiedMotion)
	{
		FinishIntroPresentation();
		return;
	}

	if (!MenuContentRoot && !JourneySummaryPanel)
	{
		return;
	}

	if (MenuContentRoot)
	{
		MenuContentRoot->SetRenderOpacity(0.0f);
		MenuContentRoot->SetRenderTranslation(FVector2D(-42.0f, 0.0f));
	}
	if (JourneySummaryPanel)
	{
		JourneySummaryPanel->SetRenderOpacity(0.0f);
		JourneySummaryPanel->SetRenderTranslation(FVector2D(28.0f, 0.0f));
	}

	IntroTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateWeakLambda(this, [this](float DeltaTime)
		{
			return TickIntroPresentation(DeltaTime);
		}));
}

void UWacomMainMenuScreen::BindRuntimeSettings()
{
	UGameInstance* GameInstance = GetGameInstance();
	UWacomSettingsSubsystem* SettingsSubsystem = GameInstance
		? GameInstance->GetSubsystem<UWacomSettingsSubsystem>()
		: nullptr;
	if (!SettingsSubsystem)
	{
		return;
	}
	RuntimeSettingsChangedHandle = SettingsSubsystem->OnRuntimeSettingsChangedNative().AddUObject(
		this,
		&UWacomMainMenuScreen::HandleRuntimeSettingsChanged);
	HandleRuntimeSettingsChanged(
		SettingsSubsystem->GetCurrentSnapshot(),
		EWacomRuntimeSettingsChangeReason::Startup);
}

void UWacomMainMenuScreen::UnbindRuntimeSettings()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UWacomSettingsSubsystem* SettingsSubsystem =
			GameInstance->GetSubsystem<UWacomSettingsSubsystem>())
		{
			SettingsSubsystem->OnRuntimeSettingsChangedNative().Remove(RuntimeSettingsChangedHandle);
		}
	}
	RuntimeSettingsChangedHandle.Reset();
}

void UWacomMainMenuScreen::HandleRuntimeSettingsChanged(
	const FWacomLocalSettingsSnapshot& Snapshot,
	EWacomRuntimeSettingsChangeReason /*Reason*/)
{
	bRuntimeSimplifiedMotion = Snapshot.UIMotionMode == EWacomUIMotionMode::Simplified;
	if (bRuntimeSimplifiedMotion)
	{
		StopIntroPresentation();
		FinishIntroPresentation();
	}
}

bool UWacomMainMenuScreen::TickIntroPresentation(float DeltaTime)
{
	IntroElapsedSeconds += DeltaTime;
	const float ContentAlpha = FMath::Clamp(IntroElapsedSeconds / 0.38f, 0.0f, 1.0f);
	const float SummaryAlpha = FMath::Clamp((IntroElapsedSeconds - 0.10f) / 0.38f, 0.0f, 1.0f);
	const float ContentEase = 1.0f - FMath::Pow(1.0f - ContentAlpha, 3.0f);
	const float SummaryEase = 1.0f - FMath::Pow(1.0f - SummaryAlpha, 3.0f);

	if (MenuContentRoot)
	{
		MenuContentRoot->SetRenderOpacity(ContentEase);
		MenuContentRoot->SetRenderTranslation(FVector2D(-42.0f * (1.0f - ContentEase), 0.0f));
	}
	if (JourneySummaryPanel)
	{
		JourneySummaryPanel->SetRenderOpacity(SummaryEase);
		JourneySummaryPanel->SetRenderTranslation(FVector2D(28.0f * (1.0f - SummaryEase), 0.0f));
	}

	if (ContentAlpha >= 1.0f && SummaryAlpha >= 1.0f)
	{
		FinishIntroPresentation();
		IntroTickerHandle.Reset();
		return false;
	}
	return true;
}

void UWacomMainMenuScreen::FinishIntroPresentation()
{
	if (MenuContentRoot)
	{
		MenuContentRoot->SetRenderOpacity(1.0f);
		MenuContentRoot->SetRenderTranslation(FVector2D::ZeroVector);
	}
	if (JourneySummaryPanel)
	{
		JourneySummaryPanel->SetRenderOpacity(1.0f);
		JourneySummaryPanel->SetRenderTranslation(FVector2D::ZeroVector);
	}
}

void UWacomMainMenuScreen::StopIntroPresentation()
{
	if (IntroTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(IntroTickerHandle);
		IntroTickerHandle.Reset();
	}
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
