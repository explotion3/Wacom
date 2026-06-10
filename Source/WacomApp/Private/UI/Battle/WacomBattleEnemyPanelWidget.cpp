// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleEnemyPanelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "UI/Battle/WacomBattleEnemyPartEntryWidget.h"

namespace
{
	FText BuildEnemyHeaderTextInternal(const FWacomBattleEnemyPanelViewData& View)
	{
		const FString EnemyName = View.EnemyDisplayName.IsEmpty() ? View.EnemySlotId.ToString() : View.EnemyDisplayName.ToString();
		return FText::FromString(FString::Printf(TEXT("%s [%s] INIT %d"), *EnemyName, *View.EnemySlotId.ToString(), View.EnemyInitiativeSum));
	}
}

void UWacomBattleEnemyPanelWidget::SetEnemyPanelViewData(const TArray<FWacomBattleEnemyPanelViewData>& InViews)
{
	CurrentViews = InViews;
	RebuildEnemyWidgets();
}

FWacomBattleEnemyPanelViewData UWacomBattleEnemyPanelWidget::BuildEnemyPanelViewDataFromSnapshot(
	const FBattleSnapshot& Snap,
	const FEnemySnapshot& Enemy)
{
	FWacomBattleEnemyPanelViewData View;
	View.EncounterId = Snap.EncounterId;
	View.EnemySlotId = Enemy.EnemySlotId;
	View.UnitKey = Enemy.UnitKey;
	View.EnemyDefinition = Enemy.Definition;
	View.EnemyDisplayName = Enemy.Definition ? Enemy.Definition->DisplayName : FText::FromName(Enemy.EnemySlotId);
	View.EnemyInitiativeSum = Enemy.InitiativeSum;
	View.bAllPartsDestroyed = Enemy.bAllPartsDestroyed;
	View.Parts.Reserve(Enemy.Parts.Num());

	for (const FEnemyPartSnapshot& Part : Enemy.Parts)
	{
		FWacomBattleEnemyPartEntryViewData PartView;
		PartView.PartInstanceId = Part.InstanceId;
		PartView.Identity = Part.Identity;
		PartView.EnemySlotId = Part.EnemySlotId;
		PartView.PartSlotId = Part.PartSlotId;
		PartView.PartDisplayName = Part.Definition ? Part.Definition->DisplayName : FText::FromName(Part.PartSlotId);
		PartView.CurrentHp = Part.CurrentHp;
		PartView.MaxHp = Part.MaxHp;
		PartView.Shield = Part.Shield;
		PartView.CurrentInitiative = Part.CurrentInitiative;
		PartView.CurrentIntentDisplayName = Part.CurrentIntent.DisplayName;
		PartView.CurrentIntentInitiative = Part.CurrentIntent.Initiative;
		PartView.CurrentIntentResistanceValue = Part.CurrentIntent.ResistanceValue;
		PartView.RuntimeStatuses = Part.Statuses;
		PartView.RuntimeStatusStacks = Part.StatusStacks;
		PartView.bDestroyed = Part.bDestroyed;
		View.Parts.Add(MoveTemp(PartView));
	}

	return View;
}

TArray<FWacomBattleEnemyPanelViewData> UWacomBattleEnemyPanelWidget::BuildEnemyPanelViewDataListFromSnapshot(
	const FBattleSnapshot& Snap)
{
	TArray<FWacomBattleEnemyPanelViewData> Views;
	Views.Reserve(Snap.Enemies.Num());
	for (const FEnemySnapshot& Enemy : Snap.Enemies)
	{
		Views.Add(BuildEnemyPanelViewDataFromSnapshot(Snap, Enemy));
	}
	return Views;
}

TSharedRef<SWidget> UWacomBattleEnemyPanelWidget::RebuildWidget()
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

		EmptyTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EmptyText"));
		EnemyListBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("EnemyListBox"));

		if (RootBox)
		{
			if (EmptyTextBlock)
			{
				EmptyTextBlock->SetText(FText::FromString(TEXT("No enemy data")));
				RootBox->AddChildToVerticalBox(EmptyTextBlock);
			}

			if (EnemyListBox)
			{
				RootBox->AddChildToVerticalBox(EnemyListBox);
			}
		}
	}

	RebuildEnemyWidgets();
	return Super::RebuildWidget();
}

void UWacomBattleEnemyPanelWidget::NativeRefreshFromSnapshot(const FBattleSnapshot& Snap)
{
	SetEnemyPanelViewData(BuildEnemyPanelViewDataListFromSnapshot(Snap));
}

void UWacomBattleEnemyPanelWidget::RebuildEnemyWidgets()
{
	if (!EnemyListBox)
	{
		return;
	}

	EnemyListBox->ClearChildren();

	for (int32 EnemyIndex = 0; EnemyIndex < CurrentViews.Num(); ++EnemyIndex)
	{
		const FWacomBattleEnemyPanelViewData& View = CurrentViews[EnemyIndex];
		UVerticalBox* EnemyBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),
			*FString::Printf(TEXT("EnemyBox_%d"), EnemyIndex));
		if (!EnemyBox)
		{
			continue;
		}

		UTextBlock* HeaderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
			*FString::Printf(TEXT("EnemyHeader_%d"), EnemyIndex));
		if (HeaderText)
		{
			HeaderText->SetText(BuildEnemyHeaderTextInternal(View));
			EnemyBox->AddChildToVerticalBox(HeaderText);
		}

		for (int32 PartIndex = 0; PartIndex < View.Parts.Num(); ++PartIndex)
		{
			const FWacomBattleEnemyPartEntryViewData& PartView = View.Parts[PartIndex];
			UWacomBattleEnemyPartEntryWidget* PartWidget =
				WidgetTree->ConstructWidget<UWacomBattleEnemyPartEntryWidget>(UWacomBattleEnemyPartEntryWidget::StaticClass(),
					*FString::Printf(TEXT("EnemyPartEntry_%d_%d"), EnemyIndex, PartIndex));
			if (!PartWidget)
			{
				continue;
			}

			PartWidget->SetPartEntryViewData(PartView);
			EnemyBox->AddChildToVerticalBox(PartWidget);
		}

		EnemyListBox->AddChildToVerticalBox(EnemyBox);
	}

	if (EmptyTextBlock)
	{
		EmptyTextBlock->SetVisibility(CurrentViews.IsEmpty() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

FText UWacomBattleEnemyPanelWidget::BuildEnemyHeaderText(const FWacomBattleEnemyPanelViewData& View) const
{
	return BuildEnemyHeaderTextInternal(View);
}
