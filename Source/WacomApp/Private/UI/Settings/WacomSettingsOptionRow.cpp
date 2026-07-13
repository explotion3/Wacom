// Copyright Wacom. All Rights Reserved.

#include "UI/Settings/WacomSettingsOptionRow.h"

#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Input/Events.h"
#include "UI/Foundation/WacomMenuButtonWidget.h"

namespace
{
	USizeBox* WrapWidth(UWidgetTree& Tree, UWidget& Child, const TCHAR* Name, float Width)
	{
		USizeBox* Box = Tree.ConstructWidget<USizeBox>(USizeBox::StaticClass(), FName(Name));
		Box->SetWidthOverride(Width);
		Box->AddChild(&Child);
		return Box;
	}

	void AddRowChild(UHorizontalBox& Row, UWidget& Child, const FMargin& Padding)
	{
		if (UHorizontalBoxSlot* Slot = Row.AddChildToHorizontalBox(&Child))
		{
			Slot->SetPadding(Padding);
			Slot->SetVerticalAlignment(VAlign_Center);
		}
	}
}

UWacomSettingsOptionRow::UWacomSettingsOptionRow(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

TSharedRef<SWidget> UWacomSettingsOptionRow::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
	}
	if (!WidgetTree->RootWidget)
	{
		UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RowBackdrop"));
		Root->SetBrushColor(FLinearColor(0.025f, 0.038f, 0.060f, 0.88f));
		Root->SetPadding(FMargin(12.0f, 7.0f));
		WidgetTree->RootWidget = Root;

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), TEXT("Row"));
		Root->AddChild(Row);

		LabelText = WidgetTree->ConstructWidget<UCommonTextBlock>(
			UCommonTextBlock::StaticClass(), TEXT("LabelText"));
		LabelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.91f, 0.90f, 0.82f, 1.0f)));
		AddRowChild(*Row, *WrapWidth(*WidgetTree, *LabelText, TEXT("LabelSize"), 250.0f), FMargin(0.0f, 0.0f, 10.0f, 0.0f));

		DecreaseButton = WidgetTree->ConstructWidget<UWacomMenuButtonWidget>(
			UWacomMenuButtonWidget::StaticClass(), TEXT("DecreaseButton"));
		DecreaseButton->SetButtonText(FText::FromString(TEXT("<")));
		AddRowChild(*Row, *WrapWidth(*WidgetTree, *DecreaseButton, TEXT("DecreaseSize"), 48.0f), FMargin(0.0f, 0.0f, 6.0f, 0.0f));

		ValueSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("ValueSlider"));
		ValueSlider->SetStepSize(0.01f);
		AddRowChild(*Row, *WrapWidth(*WidgetTree, *ValueSlider, TEXT("SliderSize"), 220.0f), FMargin(0.0f, 0.0f, 10.0f, 0.0f));

		ValueText = WidgetTree->ConstructWidget<UCommonTextBlock>(
			UCommonTextBlock::StaticClass(), TEXT("ValueText"));
		ValueText->SetJustification(ETextJustify::Center);
		ValueText->SetColorAndOpacity(FSlateColor(FLinearColor(0.30f, 0.88f, 0.82f, 1.0f)));
		AddRowChild(*Row, *WrapWidth(*WidgetTree, *ValueText, TEXT("ValueSize"), 190.0f), FMargin(0.0f, 0.0f, 6.0f, 0.0f));

		IncreaseButton = WidgetTree->ConstructWidget<UWacomMenuButtonWidget>(
			UWacomMenuButtonWidget::StaticClass(), TEXT("IncreaseButton"));
		IncreaseButton->SetButtonText(FText::FromString(TEXT(">")));
		AddRowChild(*Row, *WrapWidth(*WidgetTree, *IncreaseButton, TEXT("IncreaseSize"), 48.0f), FMargin(0.0f));
	}
	return Super::RebuildWidget();
}

void UWacomSettingsOptionRow::NativeConstruct()
{
	Super::NativeConstruct();
	if (DecreaseButton)
	{
		DecreaseButton->OnClicked().RemoveAll(this);
		DecreaseButton->OnClicked().AddUObject(this, &UWacomSettingsOptionRow::HandleDecreaseClicked);
	}
	if (IncreaseButton)
	{
		IncreaseButton->OnClicked().RemoveAll(this);
		IncreaseButton->OnClicked().AddUObject(this, &UWacomSettingsOptionRow::HandleIncreaseClicked);
	}
	if (ValueSlider)
	{
		ValueSlider->OnValueChanged.RemoveAll(this);
		ValueSlider->OnValueChanged.AddDynamic(this, &UWacomSettingsOptionRow::HandleSliderValueChanged);
	}
	ApplyViewData(ViewData);
}

void UWacomSettingsOptionRow::NativeDestruct()
{
	if (DecreaseButton)
	{
		DecreaseButton->OnClicked().RemoveAll(this);
	}
	if (IncreaseButton)
	{
		IncreaseButton->OnClicked().RemoveAll(this);
	}
	if (ValueSlider)
	{
		ValueSlider->OnValueChanged.RemoveAll(this);
	}
	OnStepRequestedNative.Clear();
	OnNormalizedValueRequestedNative.Clear();
	Super::NativeDestruct();
}

void UWacomSettingsOptionRow::ApplyViewData(const FWacomSettingsOptionRowViewData& InViewData)
{
	ViewData = InViewData;
	bApplyingViewData = true;
	if (LabelText)
	{
		LabelText->SetText(ViewData.Label);
	}
	if (ValueText)
	{
		ValueText->SetText(ViewData.Value);
	}
	if (ValueSlider)
	{
		ValueSlider->SetVisibility(ViewData.Kind == EWacomSettingsOptionKind::Continuous
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed);
		ValueSlider->SetValue(FMath::Clamp(ViewData.NormalizedValue, 0.0f, 1.0f));
		ValueSlider->SetIsEnabled(ViewData.bEnabled);
	}
	if (DecreaseButton)
	{
		DecreaseButton->SetIsInteractionEnabled(ViewData.bEnabled);
	}
	if (IncreaseButton)
	{
		IncreaseButton->SetIsInteractionEnabled(ViewData.bEnabled);
	}
	SetIsEnabled(ViewData.bEnabled);
	bApplyingViewData = false;
}

FReply UWacomSettingsOptionRow::NativeOnKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	if (ViewData.bEnabled && InKeyEvent.GetKey() == EKeys::Left)
	{
		OnStepRequestedNative.Broadcast(-1);
		return FReply::Handled();
	}
	if (ViewData.bEnabled && InKeyEvent.GetKey() == EKeys::Right)
	{
		OnStepRequestedNative.Broadcast(1);
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UWacomSettingsOptionRow::HandleDecreaseClicked()
{
	if (ViewData.bEnabled)
	{
		OnStepRequestedNative.Broadcast(-1);
	}
}

void UWacomSettingsOptionRow::HandleIncreaseClicked()
{
	if (ViewData.bEnabled)
	{
		OnStepRequestedNative.Broadcast(1);
	}
}

void UWacomSettingsOptionRow::HandleSliderValueChanged(float Value)
{
	if (!bApplyingViewData && ViewData.bEnabled
		&& ViewData.Kind == EWacomSettingsOptionKind::Continuous)
	{
		OnNormalizedValueRequestedNative.Broadcast(FMath::Clamp(Value, 0.0f, 1.0f));
	}
}
