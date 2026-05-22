// Copyright Wacom. All Rights Reserved.

#include "UI/Events/WacomRunEventScreen.h"

#define LOCTEXT_NAMESPACE "WacomRunEventScreen"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#include "Engine/GameInstance.h"
#include "GameFramework/WacomPlayerController.h"
#include "RunSession.h"
#include "UI/Events/WacomRunEventChoiceButton.h"
#include "UI/Events/WacomRunEventScreenFlow.h"
#include "UI/Foundation/WacomAppToastSubsystem.h"

namespace
{
	UTextBlock* MakeRunEventText(UWidgetTree* Tree, FName Name, const FText& Text, int32 FontSize)
	{
		UTextBlock* Block = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Block->SetText(Text);
		FSlateFontInfo Font = Block->GetFont();
		Font.Size = FontSize;
		Block->SetFont(Font);
		Block->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.95f, 0.95f, 1.f)));
		return Block;
	}
}

TSharedRef<SWidget> UWacomRunEventScreen::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		UBorder* PanelBg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PanelBg"));
		PanelBg->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.025f, 0.94f));
		PanelBg->SetPadding(FMargin(22.f));
		if (UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(PanelBg))
		{
			PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			PanelSlot->SetOffsets(FMargin(-420.f, -280.f, 840.f, 560.f));
			PanelSlot->SetAutoSize(false);
		}

		UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootBox"));
		PanelBg->AddChild(RootBox);

		TitleText = MakeRunEventText(WidgetTree, TEXT("TitleText"), LOCTEXT("Title", "事件"), 30);
		TitleText->SetJustification(ETextJustify::Center);
		if (UVerticalBoxSlot* TitleSlot = RootBox->AddChildToVerticalBox(TitleText))
		{
			TitleSlot->SetHorizontalAlignment(HAlign_Center);
			TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 14.f));
		}

		BodyText = MakeRunEventText(WidgetTree, TEXT("BodyText"), FText::GetEmpty(), 18);
		BodyText->SetAutoWrapText(true);
		if (UVerticalBoxSlot* BodySlot = RootBox->AddChildToVerticalBox(BodyText))
		{
			BodySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 18.f));
		}

		EmptyText = MakeRunEventText(WidgetTree, TEXT("EmptyText"), LOCTEXT("Empty", "暂无可选行动"), 16);
		EmptyText->SetJustification(ETextJustify::Center);
		if (UVerticalBoxSlot* EmptySlot = RootBox->AddChildToVerticalBox(EmptyText))
		{
			EmptySlot->SetHorizontalAlignment(HAlign_Center);
			EmptySlot->SetPadding(FMargin(0.f, 8.f));
		}

		UScrollBox* ChoiceScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ChoiceScroll"));
		if (UVerticalBoxSlot* ScrollSlot = RootBox->AddChildToVerticalBox(ChoiceScroll))
		{
			ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		ChoiceList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ChoiceList"));
		ChoiceScroll->AddChild(ChoiceList);

		CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
		UTextBlock* CloseText = MakeRunEventText(WidgetTree, TEXT("CloseText"), LOCTEXT("Close", "关闭"), 18);
		CloseText->SetJustification(ETextJustify::Center);
		CloseButton->AddChild(CloseText);
		if (UVerticalBoxSlot* CloseSlot = RootBox->AddChildToVerticalBox(CloseButton))
		{
			CloseSlot->SetHorizontalAlignment(HAlign_Center);
			CloseSlot->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
		}
	}
	return Super::RebuildWidget();
}

void UWacomRunEventScreen::NativeConstruct()
{
	Super::NativeConstruct();
	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UWacomRunEventScreen::HandleCloseClicked);
	}
	RefreshEvent();
}

void UWacomRunEventScreen::NativeOnActivated()
{
	Super::NativeOnActivated();
	bDidEndRunEvent = false;
	RefreshEvent();
}

void UWacomRunEventScreen::NativeOnDeactivated()
{
	FWacomRunEventScreenFlow::EndRunEventOnDeactivate(ResolveRunSession(), bDidEndRunEvent);
	Super::NativeOnDeactivated();
}

void UWacomRunEventScreen::RefreshEvent()
{
	if (URunSession* Run = ResolveRunSession())
	{
		const FRunEventSnapshot Snapshot = Run->BuildCurrentRunEventSnapshot();
		if (TitleText)
		{
			TitleText->SetText(Snapshot.TitleText.IsEmpty() ? LOCTEXT("Title", "事件") : Snapshot.TitleText);
		}
		if (BodyText)
		{
			BodyText->SetText(Snapshot.BodyText);
		}
	}

	RebuildChoices();
}

void UWacomRunEventScreen::HandleCloseClicked()
{
	DeactivateWidget();
}

#if WITH_AUTOMATION_TESTS
FRunEventChoiceSnapshot UWacomRunEventScreen::GetCachedChoiceSnapshot(int32 Index) const
{
	return CachedChoices.IsValidIndex(Index) ? CachedChoices[Index] : FRunEventChoiceSnapshot();
}

bool UWacomRunEventScreen::ChooseChoiceByIndex(int32 Index)
{
	if (!CachedChoices.IsValidIndex(Index))
	{
		return false;
	}
	return ChooseChoice(CachedChoices[Index].ChoiceId);
}

FText UWacomRunEventScreen::GetDisplayedTitleText() const
{
	return TitleText ? TitleText->GetText() : FText::GetEmpty();
}

FText UWacomRunEventScreen::GetDisplayedBodyText() const
{
	return BodyText ? BodyText->GetText() : FText::GetEmpty();
}
#endif

URunSession* UWacomRunEventScreen::ResolveRunSession() const
{
	AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(GetOwningPlayer());
	return WacomPC ? WacomPC->GetRunSession() : nullptr;
}

UWacomAppToastSubsystem* UWacomRunEventScreen::ResolveToastSubsystem() const
{
	const UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UWacomAppToastSubsystem>() : nullptr;
}

void UWacomRunEventScreen::RebuildChoices()
{
	CachedChoices.Reset();
	if (ChoiceList)
	{
		ChoiceList->ClearChildren();
	}

	URunSession* Run = ResolveRunSession();
	const FRunEventSnapshot Snapshot = Run ? Run->BuildCurrentRunEventSnapshot() : FRunEventSnapshot();
	CachedChoices = Snapshot.Choices;

	if (EmptyText)
	{
		EmptyText->SetVisibility(Snapshot.Choices.Num() == 0
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	if (!ChoiceList)
	{
		return;
	}

	for (const FRunEventChoiceSnapshot& Choice : Snapshot.Choices)
	{
		AddChoiceButton(Choice);
	}
}

void UWacomRunEventScreen::AddChoiceButton(const FRunEventChoiceSnapshot& Choice)
{
	if (!WidgetTree || !ChoiceList)
	{
		return;
	}

	UWacomRunEventChoiceButton* ChoiceButtonWidget = WidgetTree->ConstructWidget<UWacomRunEventChoiceButton>(
		UWacomRunEventChoiceButton::StaticClass());
	ChoiceButtonWidget->SetChoiceSnapshot(Choice);
	ChoiceButtonWidget->OnChoiceClickedNative.AddUObject(this, &UWacomRunEventScreen::HandleChoiceClicked);
	if (UVerticalBoxSlot* ChoiceSlot = ChoiceList->AddChildToVerticalBox(ChoiceButtonWidget))
	{
		ChoiceSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	}
}

void UWacomRunEventScreen::HandleChoiceClicked(FName ChoiceId)
{
	ChooseChoice(ChoiceId);
}

bool UWacomRunEventScreen::ChooseChoice(FName ChoiceId)
{
	URunSession* Run = ResolveRunSession();
	if (!Run)
	{
		return false;
	}

	return FWacomRunEventScreenFlow::ChooseChoice(
		*this,
		Run,
		ResolveToastSubsystem(),
		ChoiceId,
		CachedChoices,
		bDidEndRunEvent);
}

#undef LOCTEXT_NAMESPACE
