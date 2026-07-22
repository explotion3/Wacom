// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackControlsHelpWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "InputCoreTypes.h"

#define LOCTEXT_NAMESPACE "WacomBackpackControlsHelp"

TSharedRef<SWidget> UWacomBackpackControlsHelpWidget::RebuildWidget()
{
	EnsureFallbackTree();
	SetIsFocusable(true);
	return Super::RebuildWidget();
}

void UWacomBackpackControlsHelpWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (CloseHelpButton)
	{
		CloseHelpButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleCloseClicked);
	}
	if (HelpText)
	{
		HelpText->SetText(PendingHelpText);
	}
}

void UWacomBackpackControlsHelpWidget::NativeDestruct()
{
	if (CloseHelpButton)
	{
		CloseHelpButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleCloseClicked);
	}
	Super::NativeDestruct();
}

FReply UWacomBackpackControlsHelpWidget::NativeOnKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::F1 || Key == EKeys::Gamepad_FaceButton_Right)
	{
		OnCloseRequestedNative.Broadcast();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UWacomBackpackControlsHelpWidget::SetHelpText(const FText& InText)
{
	PendingHelpText = InText;
	if (HelpText)
	{
		HelpText->SetText(InText);
	}
}

void UWacomBackpackControlsHelpWidget::HandleCloseClicked()
{
	OnCloseRequestedNative.Broadcast();
}

void UWacomBackpackControlsHelpWidget::EnsureFallbackTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;
	UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dim"));
	Dim->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.72f));
	Root->AddChildToOverlay(Dim);

	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelSize"));
	PanelSize->SetWidthOverride(620.0f);
	PanelSize->SetMinDesiredHeight(420.0f);
	if (UOverlaySlot* PanelSlot = Root->AddChildToOverlay(PanelSize))
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
		PanelSlot->SetVerticalAlignment(VAlign_Center);
		PanelSlot->SetPadding(FMargin(24.0f));
	}
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	Panel->SetBrushColor(FLinearColor(0.035f, 0.05f, 0.075f, 0.99f));
	Panel->SetPadding(FMargin(28.0f));
	PanelSize->AddChild(Panel);
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	Panel->AddChild(Column);

	HelpText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HelpText"));
	HelpText->SetAutoWrapText(true);
	FSlateFontInfo HelpFont = HelpText->GetFont();
	HelpFont.Size = 19;
	HelpText->SetFont(HelpFont);
	if (UVerticalBoxSlot* HelpSlot = Column->AddChildToVerticalBox(HelpText))
	{
		HelpSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		HelpSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));
	}

	CloseHelpButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseHelpButton"));
	UTextBlock* CloseLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CloseHelpLabel"));
	CloseLabel->SetText(LOCTEXT("CloseHelp", "关闭说明"));
	CloseLabel->SetJustification(ETextJustify::Center);
	CloseHelpButton->AddChild(CloseLabel);
	if (UVerticalBoxSlot* CloseSlot = Column->AddChildToVerticalBox(CloseHelpButton))
	{
		CloseSlot->SetHorizontalAlignment(HAlign_Right);
	}
}

#undef LOCTEXT_NAMESPACE
