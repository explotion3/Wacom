// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleEnemyPanelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "UI/Battle/WacomBattleEnemyPartEntryWidget.h"

namespace
{
	constexpr float FallbackEntryIntroStaggerSeconds = 0.045f;

	FText BuildEnemyHeaderTextInternal(const FWacomBattleEnemyPanelViewData& View)
	{
		const FString EnemyName =
			View.EnemyDisplayName.IsEmpty() ? View.EnemySlotId.ToString() : View.EnemyDisplayName.ToString();
		return FText::FromString(FString::Printf(TEXT("%s   INIT %d"), *EnemyName, View.EnemyInitiativeSum));
	}

	FString SanitizeEnemyPanelWidgetObjectName(const FName Key)
	{
		FString Name = Key.ToString();
		for (TCHAR& Character : Name)
		{
			if (!FChar::IsAlnum(Character) && Character != TEXT('_'))
			{
				Character = TEXT('_');
			}
		}
		return Name.IsEmpty() ? FString(TEXT("None")) : Name;
	}

	void SyncPanelChildAt(UPanelWidget* Panel, UWidget* Child, const int32 DesiredIndex)
	{
		if (!Panel || !Child)
		{
			return;
		}

		const int32 CurrentIndex = Panel->GetChildIndex(Child);
		if (CurrentIndex == INDEX_NONE)
		{
			Panel->AddChild(Child);
		}

		Panel->ShiftChild(DesiredIndex, Child);
	}

	void RemoveInactivePanelChildren(UPanelWidget* Panel, const TSet<UWidget*>& ActiveChildren)
	{
		if (!Panel)
		{
			return;
		}

		for (int32 ChildIndex = Panel->GetChildrenCount() - 1; ChildIndex >= 0; --ChildIndex)
		{
			UWidget* Child = Panel->GetChildAt(ChildIndex);
			if (!ActiveChildren.Contains(Child))
			{
				Panel->RemoveChildAt(ChildIndex);
			}
		}
	}

	void StylePanelText(UTextBlock* TextBlock, const int32 Size, const FLinearColor& Color)
	{
		if (!TextBlock)
		{
			return;
		}

		FSlateFontInfo Font = TextBlock->GetFont();
		Font.Size = Size;
		TextBlock->SetFont(Font);
		TextBlock->SetColorAndOpacity(FSlateColor(Color));
		TextBlock->SetShadowOffset(FVector2D(0.0f, 1.0f));
		TextBlock->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f));
		TextBlock->SetAutoWrapText(false);
	}
}

void UWacomBattleEnemyPanelWidget::SetEnemyPanelViewData(const TArray<FWacomBattleEnemyPanelViewData>& InViews)
{
	CurrentViews = InViews;
	SyncEnemyWidgets();
}

void UWacomBattleEnemyPanelWidget::SetPartEntryWidgetClass(
	TSubclassOf<UWacomBattleEnemyPartEntryWidget> InWidgetClass)
{
	if (PartEntryWidgetClass == InWidgetClass)
	{
		return;
	}

	PartEntryWidgetClass = InWidgetClass;
	EnemyWidgetStates.Reset();
	SyncEnemyWidgets();
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
		UBorder* PanelFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PanelFrame"));
		PanelFrame->SetBrushColor(FLinearColor(0.018f, 0.022f, 0.028f, 0.78f));
		PanelFrame->SetContentColorAndOpacity(FLinearColor::White);
		PanelFrame->SetPadding(FMargin(10.0f, 8.0f));
		WidgetTree->RootWidget = PanelFrame;

		RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Root"));
		PanelFrame->SetContent(RootBox);

		EmptyTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EmptyText"));
		EnemyListBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("EnemyListBox"));

		if (RootBox)
		{
			if (EmptyTextBlock)
			{
				EmptyTextBlock->SetText(FText::FromString(TEXT("No enemy data")));
				StylePanelText(EmptyTextBlock, 16, FLinearColor(0.62f, 0.67f, 0.74f, 1.0f));
				if (UVerticalBoxSlot* EmptySlot = RootBox->AddChildToVerticalBox(EmptyTextBlock))
				{
					EmptySlot->SetHorizontalAlignment(HAlign_Center);
					EmptySlot->SetPadding(FMargin(0.0f, 4.0f));
				}
			}

			if (EnemyListBox)
			{
				RootBox->AddChildToVerticalBox(EnemyListBox);
			}
		}
	}

	SyncEnemyWidgets();
	return Super::RebuildWidget();
}

void UWacomBattleEnemyPanelWidget::NativeRefreshFromSnapshot(const FBattleSnapshot& Snap)
{
	SetEnemyPanelViewData(BuildEnemyPanelViewDataListFromSnapshot(Snap));
}

void UWacomBattleEnemyPanelWidget::SyncEnemyWidgets()
{
	if (bSyncingEnemyWidgets || !EnemyListBox || !WidgetTree)
	{
		return;
	}

	TGuardValue<bool> SyncGuard(bSyncingEnemyWidgets, true);
	TSet<FName> ActiveEnemyKeys;
	TSet<UWidget*> ActiveEnemyWidgets;

	for (int32 EnemyIndex = 0; EnemyIndex < CurrentViews.Num(); ++EnemyIndex)
	{
		const FWacomBattleEnemyPanelViewData& View = CurrentViews[EnemyIndex];
		const FName EnemyKey = BuildEnemyWidgetKey(View);
		ActiveEnemyKeys.Add(EnemyKey);
		FWacomBattleEnemyPanelEnemyWidgetState& EnemyState = FindOrCreateEnemyWidgetState(View);
		if (!EnemyState.EnemyBox)
		{
			continue;
		}

		TSet<UWidget*> ActivePartWidgets;
		if (EnemyState.HeaderText)
		{
			EnemyState.HeaderText->SetText(BuildEnemyHeaderTextInternal(View));
			SyncPanelChildAt(EnemyState.EnemyBox, EnemyState.HeaderText, 0);
			ActivePartWidgets.Add(EnemyState.HeaderText);
		}

		TSet<FName> ActivePartKeys;
		for (int32 PartIndex = 0; PartIndex < View.Parts.Num(); ++PartIndex)
		{
			const FWacomBattleEnemyPartEntryViewData& PartView = View.Parts[PartIndex];
			const FName PartKey = BuildPartEntryWidgetKey(PartView);
			ActivePartKeys.Add(PartKey);
			UWacomBattleEnemyPartEntryWidget* PartWidget = FindOrCreatePartEntryWidget(EnemyState, PartView);
			if (!PartWidget)
			{
				continue;
			}

			PartWidget->SetPartEntryViewData(PartView);
			SyncPanelChildAt(EnemyState.EnemyBox, PartWidget, ActivePartWidgets.Num());
			ActivePartWidgets.Add(PartWidget);
		}

		for (auto It = EnemyState.PartEntryWidgets.CreateIterator(); It; ++It)
		{
			if (!ActivePartKeys.Contains(It.Key()))
			{
				EnemyState.AnimatedPartEntryKeys.Remove(It.Key());
				It.RemoveCurrent();
			}
		}
		RemoveInactivePanelChildren(EnemyState.EnemyBox, ActivePartWidgets);

		UWidget* EnemyGroupWidget = EnemyState.EnemyBorder ? Cast<UWidget>(EnemyState.EnemyBorder) : Cast<UWidget>(EnemyState.EnemyBox);
		SyncPanelChildAt(EnemyListBox, EnemyGroupWidget, EnemyIndex);
		if (UVerticalBoxSlot* EnemySlot = EnemyGroupWidget ? Cast<UVerticalBoxSlot>(EnemyGroupWidget->Slot) : nullptr)
		{
			EnemySlot->SetPadding(FMargin(0.0f, EnemyIndex > 0 ? 6.0f : 0.0f, 0.0f, 0.0f));
		}
		ActiveEnemyWidgets.Add(EnemyGroupWidget);
	}

	for (auto It = EnemyWidgetStates.CreateIterator(); It; ++It)
	{
		if (!ActiveEnemyKeys.Contains(It.Key()))
		{
			It.RemoveCurrent();
		}
	}
	RemoveInactivePanelChildren(EnemyListBox, ActiveEnemyWidgets);

	if (EmptyTextBlock)
	{
		EmptyTextBlock->SetVisibility(CurrentViews.IsEmpty() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

FWacomBattleEnemyPanelEnemyWidgetState&
UWacomBattleEnemyPanelWidget::FindOrCreateEnemyWidgetState(const FWacomBattleEnemyPanelViewData& View)
{
	const FName EnemyKey = BuildEnemyWidgetKey(View);
	FWacomBattleEnemyPanelEnemyWidgetState& State = EnemyWidgetStates.FindOrAdd(EnemyKey);
	if (!State.EnemyBorder && WidgetTree)
	{
		const FString SafeKey = SanitizeEnemyPanelWidgetObjectName(EnemyKey);
		State.EnemyBorder = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			*FString::Printf(TEXT("EnemyGroup_%s"), *SafeKey));
		if (State.EnemyBorder)
		{
			State.EnemyBorder->SetPadding(FMargin(8.0f, 7.0f));
			State.EnemyBorder->SetBrushColor(FLinearColor(0.06f, 0.073f, 0.088f, 0.88f));
		}
	}
	if (!State.EnemyBox && WidgetTree)
	{
		const FString SafeKey = SanitizeEnemyPanelWidgetObjectName(EnemyKey);
		State.EnemyBox = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(),
			*FString::Printf(TEXT("EnemyBox_%s"), *SafeKey));
		if (State.EnemyBorder && State.EnemyBox)
		{
			State.EnemyBorder->SetContent(State.EnemyBox);
		}
	}
	if (!State.HeaderText && WidgetTree)
	{
		const FString SafeKey = SanitizeEnemyPanelWidgetObjectName(EnemyKey);
		State.HeaderText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			*FString::Printf(TEXT("EnemyHeader_%s"), *SafeKey));
		StylePanelText(State.HeaderText, 16, FLinearColor(0.92f, 0.84f, 0.62f, 1.0f));
	}
	return State;
}

UWacomBattleEnemyPartEntryWidget* UWacomBattleEnemyPanelWidget::FindOrCreatePartEntryWidget(
	FWacomBattleEnemyPanelEnemyWidgetState& EnemyState,
	const FWacomBattleEnemyPartEntryViewData& PartView)
{
	const FName PartKey = BuildPartEntryWidgetKey(PartView);
	if (TObjectPtr<UWacomBattleEnemyPartEntryWidget>* Existing = EnemyState.PartEntryWidgets.Find(PartKey))
	{
		return Existing->Get();
	}

	if (!WidgetTree)
	{
		return nullptr;
	}

	const TSubclassOf<UWacomBattleEnemyPartEntryWidget> EntryClass =
		PartEntryWidgetClass ? PartEntryWidgetClass.Get() : UWacomBattleEnemyPartEntryWidget::StaticClass();
	UWacomBattleEnemyPartEntryWidget* PartWidget =
		WidgetTree->ConstructWidget<UWacomBattleEnemyPartEntryWidget>(
			EntryClass,
			*FString::Printf(TEXT("EnemyPartEntry_%s"), *SanitizeEnemyPanelWidgetObjectName(PartKey)));
	if (PartWidget)
	{
		PartWidget->SetFallbackIntroDelaySeconds(EnemyState.AnimatedPartEntryKeys.Num() * FallbackEntryIntroStaggerSeconds);
		EnemyState.AnimatedPartEntryKeys.Add(PartKey);
		EnemyState.PartEntryWidgets.Add(PartKey, PartWidget);
	}
	return PartWidget;
}

FText UWacomBattleEnemyPanelWidget::BuildEnemyHeaderText(const FWacomBattleEnemyPanelViewData& View) const
{
	return BuildEnemyHeaderTextInternal(View);
}

FName UWacomBattleEnemyPanelWidget::BuildEnemyWidgetKey(const FWacomBattleEnemyPanelViewData& View) const
{
	const FName EnemySlotId = View.UnitKey.IsValidKey()
		? View.UnitKey.GetEffectiveEnemyUnitSlotId()
		: (View.EnemySlotId.IsNone() ? FName(TEXT("Enemy")) : View.EnemySlotId);
	return EnemySlotId;
}

FName UWacomBattleEnemyPanelWidget::BuildPartEntryWidgetKey(const FWacomBattleEnemyPartEntryViewData& PartView) const
{
	const FName EnemySlotId = PartView.Identity.IsValidSlot()
		? PartView.Identity.GetEffectiveEnemySlotId()
		: (PartView.EnemySlotId.IsNone() ? FName(TEXT("Enemy")) : PartView.EnemySlotId);
	const FName PartSlotId = PartView.Identity.IsValidSlot()
		? PartView.Identity.GetEffectivePartSlotId()
		: PartView.PartSlotId;
	return FName(*FString::Printf(TEXT("%s.%s"), *EnemySlotId.ToString(), *PartSlotId.ToString()));
}
