// Copyright Wacom. All Rights Reserved.

#include "UI/Foundation/WacomAppToastWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

UWacomAppToastWidget::UWacomAppToastWidget()
{
	SetIsFocusable(false);
	SetVisibility(ESlateVisibility::Collapsed);
}

TSharedRef<SWidget> UWacomAppToastWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		Root->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = Root;

		UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Frame"));
		Frame->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.35f));
		Frame->SetPadding(FMargin(10.f, 6.f));
		Frame->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UCanvasPanelSlot* FrameSlot = Root->AddChildToCanvas(Frame))
		{
			FrameSlot->SetAnchors(FAnchors(0.5f, 0.f));
			FrameSlot->SetAlignment(FVector2D(0.5f, 0.f));
			FrameSlot->SetOffsets(FMargin(0.f, 40.f, 0.f, 0.f));
			FrameSlot->SetAutoSize(true);
		}

		Container = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Container"));
		Container->SetVisibility(ESlateVisibility::HitTestInvisible);
		Frame->SetContent(Container);
	}

	return Super::RebuildWidget();
}

void UWacomAppToastWidget::NativeDestruct()
{
	ActiveViews.Reset();
	ActiveTexts.Reset();
	ActiveRemaining.Reset();
	Super::NativeDestruct();
}

void UWacomAppToastWidget::NativeTick(const FGeometry& InGeometry, float InDeltaTime)
{
	Super::NativeTick(InGeometry, InDeltaTime);
	TickToasts(InDeltaTime);
}

TOptional<FUIInputConfig> UWacomAppToastWidget::GetDesiredInputConfig() const
{
	return TOptional<FUIInputConfig>();
}

void UWacomAppToastWidget::EnqueueToast(const FWacomAppToastView& View)
{
	if (View.MessageText.IsEmpty())
	{
		return;
	}

	if (!Container)
	{
		TakeWidget();
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
	PushToast(View);
}

void UWacomAppToastWidget::TickToastsForTest(float DeltaTime)
{
	TickToasts(DeltaTime);
}

void UWacomAppToastWidget::PushToast(const FWacomAppToastView& View)
{
	if (!Container || !WidgetTree)
	{
		return;
	}

	while (ActiveViews.Num() >= MaxVisibleMessages)
	{
		RemoveAt(0);
	}

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), NAME_None);
	Text->SetText(View.MessageText);
	Text->SetAutoWrapText(true);
	Text->SetVisibility(ESlateVisibility::HitTestInvisible);
	Text->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.95f, 0.85f, 1.0f)));
	{
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = 14;
		Text->SetFont(Font);
	}

	if (UVerticalBoxSlot* TextSlot = Container->AddChildToVerticalBox(Text))
	{
		TextSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
	}

	ActiveViews.Add(View);
	ActiveTexts.Add(Text);
	ActiveRemaining.Add(View.LifetimeOverride > 0.0f ? View.LifetimeOverride : DefaultMessageLifetime);
}

void UWacomAppToastWidget::RemoveAt(int32 Index)
{
	if (!ActiveViews.IsValidIndex(Index))
	{
		return;
	}

	if (UTextBlock* Text = ActiveTexts.IsValidIndex(Index) ? ActiveTexts[Index].Get() : nullptr)
	{
		if (Container)
		{
			Container->RemoveChild(Text);
		}
	}

	ActiveViews.RemoveAt(Index);
	if (ActiveTexts.IsValidIndex(Index))
	{
		ActiveTexts.RemoveAt(Index);
	}
	if (ActiveRemaining.IsValidIndex(Index))
	{
		ActiveRemaining.RemoveAt(Index);
	}
	HandleQueueEmpty();
}

void UWacomAppToastWidget::TickToasts(float DeltaTime)
{
	for (int32 Index = ActiveRemaining.Num() - 1; Index >= 0; --Index)
	{
		ActiveRemaining[Index] -= DeltaTime;

		if (ActiveRemaining[Index] <= 0.0f)
		{
			RemoveAt(Index);
			continue;
		}

		if (FadeDuration > 0.0f)
		{
			if (UTextBlock* Text = ActiveTexts.IsValidIndex(Index) ? ActiveTexts[Index].Get() : nullptr)
			{
				const float Opacity = FMath::Clamp(ActiveRemaining[Index] / FadeDuration, 0.0f, 1.0f);
				FLinearColor Color = Text->GetColorAndOpacity().GetSpecifiedColor();
				Color.A = Opacity;
				Text->SetColorAndOpacity(FSlateColor(Color));
			}
		}
	}
}

void UWacomAppToastWidget::HandleQueueEmpty()
{
	if (ActiveViews.Num() == 0)
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
}
