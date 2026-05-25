// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/EventToast.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Events/BattleEvent.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/Battle/WacomBattleEventPresentationBuilder.h"

UEventToast::UEventToast()
{
	SetIsFocusable(false);
}

TSharedRef<SWidget> UEventToast::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Frame"));
		Frame->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.35f));
		Frame->SetPadding(FMargin(8, 4));
		WidgetTree->RootWidget = Frame;

		Container = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Container"));
		Frame->SetContent(Container);
	}
	return Super::RebuildWidget();
}

void UEventToast::NativeOnInitialized()
{
	Super::NativeOnInitialized();
}

void UEventToast::NativeTick(const FGeometry& InGeometry, float InDeltaTime)
{
	Super::NativeTick(InGeometry, InDeltaTime);

	// 倒计时并移除过期消息（从后往前扫避免索引错位）
	for (int32 i = ActiveRemaining.Num() - 1; i >= 0; --i)
	{
		ActiveRemaining[i] -= InDeltaTime;

		if (ActiveRemaining[i] <= 0.0f)
		{
			RemoveAt(i);
			continue;
		}

		// 最后 0.8 秒淡出
		if (UTextBlock* T = ActiveTexts.IsValidIndex(i) ? ActiveTexts[i].Get() : nullptr)
		{
			const float FadeDuration = 0.8f;
			const float Opacity = FMath::Clamp(ActiveRemaining[i] / FadeDuration, 0.0f, 1.0f);
			FLinearColor C = T->GetColorAndOpacity().GetSpecifiedColor();
			C.A = Opacity;
			T->SetColorAndOpacity(FSlateColor(C));
		}
	}
}

void UEventToast::EnqueueEvents(const TArray<FBattleEvent>& Events)
{
	for (const FBattleEvent& E : Events)
	{
		const FBattleEventPresentationView View = UWacomBattleEventPresentationBuilder::BuildEventPresentationView(E);
		EnqueuePresentationView(View);
	}
}

void UEventToast::EnqueuePresentationView(const FBattleEventPresentationView& View)
{
	if (View.bShouldDisplay)
	{
		PushMessage(View.MessageText.ToString());
	}
}

void UEventToast::PushMessage(const FString& Message)
{
	if (!Container) { return; }

	// 限制条数：超出就先挤掉最旧
	while (ActiveTexts.Num() >= MaxVisibleMessages)
	{
		RemoveAt(0);
	}

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), NAME_None);
	Text->SetText(FText::FromString(Message));
	Text->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.95f, 0.85f, 1.0f)));
	{
		FSlateFontInfo F = Text->GetFont();
		F.Size = 12;
		Text->SetFont(F);
	}

	Container->AddChildToVerticalBox(Text);
	ActiveTexts.Add(Text);
	ActiveRemaining.Add(MessageLifetime);
}

void UEventToast::RemoveAt(int32 Index)
{
	if (!ActiveTexts.IsValidIndex(Index)) { return; }
	UTextBlock* T = ActiveTexts[Index].Get();
	if (T && Container)
	{
		Container->RemoveChild(T);
	}
	ActiveTexts.RemoveAt(Index);
	if (ActiveRemaining.IsValidIndex(Index))
	{
		ActiveRemaining.RemoveAt(Index);
	}
}

// ---- Event → 人类可读文本 ----

FString UEventToast::FormatEventForPlayer(const FBattleEvent& E)
{
	return UWacomBattleEventPresentationBuilder::FormatEventForPlayer(E);
}
