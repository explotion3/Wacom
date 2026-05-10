// Copyright Wacom. All Rights Reserved.

#include "UI/Common/WacomProgressBar.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Blueprint/WidgetTree.h"

TSharedRef<SWidget> UWacomProgressBar::RebuildWidget()
{
	// 如果 WBP 子类已经给了 WidgetTree root（RootWidget 被蓝图 Designer 构造），让它来。
	// 否则程序构造默认外观。
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Wacom_Default"));
		}

		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("DefaultRoot"));
		WidgetTree->RootWidget = Root;

		// Layer 0：进度条
		Fill = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("Fill"));
		Fill->SetFillColorAndOpacity(FLinearColor(0.15f, 0.8f, 0.25f));
		Fill->SetPercent(0.0f);
		if (UOverlaySlot* FillSlot = Root->AddChildToOverlay(Fill))
		{
			FillSlot->SetHorizontalAlignment(HAlign_Fill);
			FillSlot->SetVerticalAlignment(VAlign_Fill);
		}

		// Layer 1：居中文字
		ValueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ValueText"));
		ValueText->SetJustification(ETextJustify::Center);
		ValueText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		ValueText->SetText(FText::FromString(TEXT("0/0")));
		if (UOverlaySlot* TextSlot = Root->AddChildToOverlay(ValueText))
		{
			TextSlot->SetHorizontalAlignment(HAlign_Center);
			TextSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
	return Super::RebuildWidget();
}

void UWacomProgressBar::NativePreConstruct()
{
	Super::NativePreConstruct();
	RefreshDisplay();
}

void UWacomProgressBar::SetValue(int32 InCurrent, int32 InMax)
{
	Current  = InCurrent;
	MaxValue = InMax;
	RefreshDisplay();
}

void UWacomProgressBar::SetFillColor(FLinearColor Color)
{
	if (Fill) { Fill->SetFillColorAndOpacity(Color); }
}

void UWacomProgressBar::SetShowText(bool bShow)
{
	bShowText = bShow;
	if (ValueText)
	{
		ValueText->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UWacomProgressBar::SetTextFormat(FText InFormat)
{
	TextFormat = InFormat;
	RefreshDisplay();
}

void UWacomProgressBar::RefreshDisplay()
{
	if (Fill)
	{
		const float Percent = (MaxValue > 0)
			? FMath::Clamp(static_cast<float>(Current) / static_cast<float>(MaxValue), 0.0f, 1.0f)
			: 0.0f;
		Fill->SetPercent(Percent);
	}

	if (ValueText)
	{
		if (bShowText)
		{
			ValueText->SetVisibility(ESlateVisibility::HitTestInvisible);
			FFormatOrderedArguments Args;
			Args.Add(FFormatArgumentValue(Current));
			Args.Add(FFormatArgumentValue(MaxValue));
			ValueText->SetText(FText::Format(TextFormat, Args));
		}
		else
		{
			ValueText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}
