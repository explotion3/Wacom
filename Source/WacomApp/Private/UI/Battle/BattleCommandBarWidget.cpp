// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/BattleCommandBarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Widget.h"

#define LOCTEXT_NAMESPACE "WacomBattleCommandBar"

namespace
{
	ESlateVisibility TextVisibility(const FText& Text)
	{
		return Text.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible;
	}

	bool IsCommandIconBrushConfigured(const FSlateBrush& Brush)
	{
		return Brush.GetResourceObject() != nullptr
			|| !Brush.GetResourceName().IsNone();
	}
}

TSharedRef<SWidget> UWacomBattleCommandButtonWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Root"));
		Root->SetPadding(FMargin(12.0f, 7.0f));
		Root->SetBrushColor(FLinearColor(0.08f, 0.08f, 0.10f, 0.92f));
		WidgetTree->RootWidget = Root;

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ContentRow"));
		Root->AddChild(Row);

		IconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("IconImage"));
		IconImage->SetVisibility(ESlateVisibility::Collapsed);
		if (UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(IconImage))
		{
			IconSlot->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));
			IconSlot->SetVerticalAlignment(VAlign_Center);
		}

		ButtonText = WidgetTree->ConstructWidget<UCommonTextBlock>(
			UCommonTextBlock::StaticClass(),
			TEXT("ButtonText"));
		ButtonText->SetText(LOCTEXT("CommandButtonFallbackLabel", "Command"));
		ButtonText->SetJustification(ETextJustify::Center);
		if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(ButtonText))
		{
			LabelSlot->SetVerticalAlignment(VAlign_Center);
		}

		InputHintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InputHintText"));
		InputHintText->SetVisibility(ESlateVisibility::Collapsed);
		InputHintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.72f, 0.76f)));
		if (UHorizontalBoxSlot* HintSlot = Row->AddChildToHorizontalBox(InputHintText))
		{
			HintSlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
			HintSlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	return Super::RebuildWidget();
}

void UWacomBattleCommandButtonWidget::SetCommandView(
	const FWacomBattleCommandButtonView& InView)
{
	CurrentView = InView;
	SetVisibility(CurrentView.bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	SetIsEnabled(CurrentView.bEnabled);
	BP_OnInteractabilityChanged(CurrentView.bEnabled);
	SetButtonText(CurrentView.DisplayText);
	SetToolTipText(CurrentView.ToolTipText);

	if (IconImage)
	{
		if (CurrentView.bHasIconBrush)
		{
			IconImage->SetBrush(CurrentView.IconBrush);
			IconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			IconImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (InputHintText)
	{
		InputHintText->SetText(CurrentView.InputHintText);
		InputHintText->SetVisibility(TextVisibility(CurrentView.InputHintText));
	}

	if (PendingIndicator)
	{
		PendingIndicator->SetVisibility(CurrentView.bPending
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
}

void UWacomBattleCommandButtonWidget::NativeOnClicked()
{
	Super::NativeOnClicked();

	if (CurrentView.CommandId == EWacomBattleCommandId::None
		|| !CurrentView.bVisible
		|| !CurrentView.bEnabled)
	{
		return;
	}

	OnCommandButtonClicked.Broadcast(CurrentView.CommandId);
}

UBattleCommandBarWidget::UBattleCommandBarWidget(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CommandButtonWidgetClass = UWacomBattleCommandButtonWidget::StaticClass();
}

TSharedRef<SWidget> UBattleCommandBarWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		WaitValueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WaitValueText"));
		WaitValueText->SetText(LOCTEXT("WaitValueFallback", "Wait Value: 0"));
		WaitValueText->SetJustification(ETextJustify::Center);
		WaitValueText->SetColorAndOpacity(FSlateColor(FLinearColor(0.86f, 0.86f, 0.88f)));
		if (UVerticalBoxSlot* WaitSlot = Root->AddChildToVerticalBox(WaitValueText))
		{
			WaitSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
		}

		PendingText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PendingText"));
		PendingText->SetVisibility(ESlateVisibility::Collapsed);
		PendingText->SetJustification(ETextJustify::Center);
		PendingText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.78f, 0.38f)));
		if (UVerticalBoxSlot* PendingSlot = Root->AddChildToVerticalBox(PendingText))
		{
			PendingSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		}

		UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(),
			TEXT("CommandButtonContainer"));
		CommandButtonContainer = ButtonRow;
		Root->AddChildToVerticalBox(ButtonRow);
	}

	return Super::RebuildWidget();
}

void UBattleCommandBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RebuildCommandButtons();
}

void UBattleCommandBarWidget::NativeDestruct()
{
	if (WaitButton)
	{
		WaitButton->OnCommandButtonClicked.RemoveAll(this);
	}
	if (EndTurnButton)
	{
		EndTurnButton->OnCommandButtonClicked.RemoveAll(this);
	}
	ClearGeneratedCommandButtons();
	Super::NativeDestruct();
}

void UBattleCommandBarWidget::SetCommandBarViewData(
	const FWacomBattleCommandBarViewData& InViewData)
{
	CurrentViewData = InViewData;
	ApplyAuthoringIconBrushes(CurrentViewData);

	if (WaitValueText)
	{
		WaitValueText->SetText(CurrentViewData.WaitValueText);
		WaitValueText->SetVisibility(TextVisibility(CurrentViewData.WaitValueText));
	}

	if (PendingText)
	{
		PendingText->SetText(CurrentViewData.PendingCommandText);
		PendingText->SetVisibility(TextVisibility(CurrentViewData.PendingCommandText));
	}

	RebuildCommandButtons();
}

bool UBattleCommandBarWidget::FindCommandButtonView(
	EWacomBattleCommandId CommandId,
	FWacomBattleCommandButtonView& OutView) const
{
	for (const FWacomBattleCommandButtonView& CommandView : CurrentViewData.Commands)
	{
		if (CommandView.CommandId == CommandId)
		{
			OutView = CommandView;
			return true;
		}
	}
	return false;
}

bool UBattleCommandBarWidget::IsCommandEnabled(EWacomBattleCommandId CommandId) const
{
	FWacomBattleCommandButtonView CommandView;
	return FindCommandButtonView(CommandId, CommandView)
		&& CommandView.bVisible
		&& CommandView.bEnabled;
}

void UBattleCommandBarWidget::RequestCommand(EWacomBattleCommandId CommandId)
{
	if (CommandId == EWacomBattleCommandId::None)
	{
		return;
	}

	FWacomBattleCommandButtonView CommandView;
	if (!FindCommandButtonView(CommandId, CommandView)
		|| !CommandView.bVisible
		|| !CommandView.bEnabled)
	{
		return;
	}

	OnBattleCommandRequested.Broadcast(CommandId);
}

void UBattleCommandBarWidget::HandleCommandButtonClicked(
	EWacomBattleCommandId CommandId)
{
	RequestCommand(CommandId);
}

bool UBattleCommandBarWidget::UsesAuthoredCommandButtons() const
{
	return WaitButton || EndTurnButton;
}

void UBattleCommandBarWidget::ApplyAuthoringIconBrushes(
	FWacomBattleCommandBarViewData& ViewData) const
{
	for (FWacomBattleCommandButtonView& CommandView : ViewData.Commands)
	{
		const FSlateBrush* IconBrush = nullptr;
		switch (CommandView.CommandId)
		{
		case EWacomBattleCommandId::Wait:
			IconBrush = &WaitIconBrush;
			break;
		case EWacomBattleCommandId::EndTurn:
			IconBrush = &EndTurnIconBrush;
			break;
		default:
			break;
		}

		if (IconBrush && IsCommandIconBrushConfigured(*IconBrush))
		{
			CommandView.IconBrush = *IconBrush;
			CommandView.bHasIconBrush = true;
		}
	}
}

void UBattleCommandBarWidget::BindCommandButton(UWacomBattleCommandButtonWidget* Button)
{
	if (!Button)
	{
		return;
	}

	Button->OnCommandButtonClicked.RemoveAll(this);
	Button->OnCommandButtonClicked.AddDynamic(this, &UBattleCommandBarWidget::HandleCommandButtonClicked);
}

void UBattleCommandBarWidget::ClearGeneratedCommandButtons()
{
	for (TObjectPtr<UWacomBattleCommandButtonWidget>& Button : CommandButtons)
	{
		if (Button)
		{
			Button->OnCommandButtonClicked.RemoveAll(this);
			Button->RemoveFromParent();
		}
	}
	CommandButtons.Reset();
}

void UBattleCommandBarWidget::ApplyAuthoredCommandButtons()
{
	ClearGeneratedCommandButtons();

	int32 VisibleButtonCount = 0;
	auto ApplyCommandToButton =
		[this, &VisibleButtonCount](
			UWacomBattleCommandButtonWidget* Button,
			EWacomBattleCommandId CommandId)
		{
			if (!Button)
			{
				return;
			}

			FWacomBattleCommandButtonView CommandView;
			if (!FindCommandButtonView(CommandId, CommandView))
			{
				CommandView.CommandId = CommandId;
				CommandView.bVisible = false;
				CommandView.bEnabled = false;
			}

			Button->SetCommandView(CommandView);
			BindCommandButton(Button);
			if (CommandView.bVisible)
			{
				++VisibleButtonCount;
			}
		};

	ApplyCommandToButton(WaitButton, EWacomBattleCommandId::Wait);
	ApplyCommandToButton(EndTurnButton, EWacomBattleCommandId::EndTurn);

	SetVisibility(VisibleButtonCount > 0
		? ESlateVisibility::Visible
		: ESlateVisibility::Collapsed);
}

void UBattleCommandBarWidget::RebuildCommandButtons()
{
	if (UsesAuthoredCommandButtons())
	{
		ApplyAuthoredCommandButtons();
		return;
	}

	if (!CommandButtonContainer)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	ClearGeneratedCommandButtons();
	CommandButtonContainer->ClearChildren();

	TArray<FWacomBattleCommandButtonView> SortedCommands = CurrentViewData.Commands;
	SortedCommands.StableSort(
		[](const FWacomBattleCommandButtonView& Left, const FWacomBattleCommandButtonView& Right)
		{
			if (Left.SortOrder != Right.SortOrder)
			{
				return Left.SortOrder < Right.SortOrder;
			}
			return static_cast<uint8>(Left.CommandId) < static_cast<uint8>(Right.CommandId);
		});

	for (const FWacomBattleCommandButtonView& CommandView : SortedCommands)
	{
		if (!CommandView.bVisible)
		{
			continue;
		}

		UWacomBattleCommandButtonWidget* Button = CreateCommandButtonWidget();
		if (!Button)
		{
			continue;
		}

		Button->SetCommandView(CommandView);
		BindCommandButton(Button);
		CommandButtons.Add(Button);

		UPanelSlot* PanelSlot = CommandButtonContainer->AddChild(Button);
		if (UHorizontalBoxSlot* HorizontalSlot = Cast<UHorizontalBoxSlot>(PanelSlot))
		{
			HorizontalSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
			HorizontalSlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	SetVisibility(CommandButtons.IsEmpty()
		? ESlateVisibility::Collapsed
		: ESlateVisibility::Visible);
}

UWacomBattleCommandButtonWidget* UBattleCommandBarWidget::CreateCommandButtonWidget()
{
	TSubclassOf<UWacomBattleCommandButtonWidget> ButtonClass = CommandButtonWidgetClass;
	if (!ButtonClass)
	{
		ButtonClass = UWacomBattleCommandButtonWidget::StaticClass();
	}

	if (GetWorld())
	{
		return CreateWidget<UWacomBattleCommandButtonWidget>(this, ButtonClass);
	}

	return NewObject<UWacomBattleCommandButtonWidget>(this, ButtonClass);
}

#undef LOCTEXT_NAMESPACE
