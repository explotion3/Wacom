// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleEnemyPartEntryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "UI/Battle/WacomBattleEventPresentationBuilder.h"

void UWacomBattleEnemyPartEntryWidget::SetPartEntryViewData(const FWacomBattleEnemyPartEntryViewData& InView)
{
	CurrentView = InView;
	RefreshText();
}

TSharedRef<SWidget> UWacomBattleEnemyPartEntryWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
	}

	if (!WidgetTree->RootWidget)
	{
		UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;
		RootBox = Root;

		PartNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PartNameText"));
		StatsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatsText"));
		IntentText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("IntentText"));
		StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));

		if (RootBox)
		{
			if (PartNameText) RootBox->AddChildToVerticalBox(PartNameText);
			if (StatsText) RootBox->AddChildToVerticalBox(StatsText);
			if (IntentText) RootBox->AddChildToVerticalBox(IntentText);
			if (StatusText) RootBox->AddChildToVerticalBox(StatusText);
		}
	}

	RefreshText();
	return Super::RebuildWidget();
}

void UWacomBattleEnemyPartEntryWidget::NativeRefreshFromSnapshot(const FBattleSnapshot& /*Snap*/)
{
	RefreshText();
}

void UWacomBattleEnemyPartEntryWidget::RefreshText()
{
	if (PartNameText)
	{
		PartNameText->SetText(CurrentView.PartDisplayName.IsEmpty() ? FText::FromName(CurrentView.PartSlotId) : CurrentView.PartDisplayName);
	}

	if (StatsText)
	{
		StatsText->SetText(FText::FromString(FString::Printf(TEXT("HP %d/%d  SH %d  INIT %d"), CurrentView.CurrentHp, CurrentView.MaxHp, CurrentView.Shield, CurrentView.CurrentInitiative)));
	}

	if (IntentText)
	{
		if (CurrentView.CurrentIntentDisplayName.IsEmpty())
		{
			IntentText->SetText(FText::FromString(TEXT("意图：无")));
		}
		else
		{
			IntentText->SetText(FText::FromString(FString::Printf(TEXT("意图：%s  %d"), *CurrentView.CurrentIntentDisplayName.ToString(), CurrentView.CurrentIntentInitiative)));
		}
	}

	if (StatusText)
	{
		StatusText->SetText(BuildStatusText());
	}
}

FText UWacomBattleEnemyPartEntryWidget::BuildStatusText() const
{
	TArray<FString> Parts;
	TArray<FGameplayTag> Tags;
	CurrentView.RuntimeStatuses.GetGameplayTagArray(Tags);
	Tags.Sort([](const FGameplayTag& A, const FGameplayTag& B)
	{
		return A.GetTagName().LexicalLess(B.GetTagName());
	});

	for (const FGameplayTag& Tag : Tags)
	{
		const int32* Stack = CurrentView.RuntimeStatusStacks.Find(Tag);
		const int32 StackCount = Stack ? *Stack : 0;
		const FString StatusName = UWacomBattleEventPresentationBuilder::FormatStatusName(Tag);
		Parts.Add(StackCount > 1 ? FString::Printf(TEXT("%s x%d"), *StatusName, StackCount) : StatusName);
	}

	return Parts.IsEmpty()
		? FText::FromString(TEXT("状态：无"))
		: FText::FromString(FString::Printf(TEXT("状态：%s"), *FString::Join(Parts, TEXT(" / "))));
}
