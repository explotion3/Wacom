// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleEnemyPanelWidget.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "UI/Battle/WacomBattleEnemyPartEntryWidget.h"

namespace
{
	constexpr float EntryIntroStaggerSeconds = 0.045f;

	void SyncPanelChildAt(UPanelWidget* Panel, UWidget* Child, const int32 DesiredIndex)
	{
		if (!Panel || !Child)
		{
			return;
		}

		if (Panel->GetChildIndex(Child) == INDEX_NONE)
		{
			Panel->AddChild(Child);
		}
		Panel->ShiftChild(DesiredIndex, Child);
	}

	FName MakeWidgetObjectName(const FName StablePartKey)
	{
		FString ObjectName = StablePartKey.ToString();
		for (TCHAR& Character : ObjectName)
		{
			if (!FChar::IsAlnum(Character) && Character != TEXT('_'))
			{
				Character = TEXT('_');
			}
		}
		return FName(*FString::Printf(TEXT("EnemyPart_%s"), *ObjectName));
	}
}

void UWacomBattleEnemyPanelWidget::SetEnemyPanelViewData(
	const FWacomBattleEnemyPanelViewData& InView)
{
	CurrentView = InView;
	bHasCurrentView = true;
	RefreshHeader();
	SyncPartEntries();
}

void UWacomBattleEnemyPanelWidget::ClearEnemyPanelViewData()
{
	ClearActionPreview();
	HoveredPartSlotId = NAME_None;
	bHasCurrentView = false;
	CurrentView = FWacomBattleEnemyPanelViewData();
	ClearPartEntries();
	RefreshHeader();
	RefreshContextHighlight();
}

bool UWacomBattleEnemyPanelWidget::SetActionPreviewPartViews(
	const TArray<FWacomBattleEnemyPartEntryViewData>& InPreviewParts)
{
	for (TPair<FName, TObjectPtr<UWacomBattleEnemyPartEntryWidget>>& Pair : PartEntryWidgets)
	{
		if (Pair.Value)
		{
			Pair.Value->ClearActionPreview();
		}
	}

	bHasActionPreview = false;
	for (const FWacomBattleEnemyPartEntryViewData& PreviewPart : InPreviewParts)
	{
		if (!DoesPartBelongToCurrentEnemy(PreviewPart))
		{
			continue;
		}

		const FName PartKey = BuildPartEntryWidgetKey(PreviewPart);
		if (TObjectPtr<UWacomBattleEnemyPartEntryWidget>* PartWidget = PartEntryWidgets.Find(PartKey))
		{
			if (PartWidget->Get())
			{
				PartWidget->Get()->SetActionPreview(PreviewPart);
				bHasActionPreview = true;
			}
		}
	}
	RefreshContextHighlight();
	return bHasActionPreview;
}

void UWacomBattleEnemyPanelWidget::ClearActionPreview()
{
	for (TPair<FName, TObjectPtr<UWacomBattleEnemyPartEntryWidget>>& Pair : PartEntryWidgets)
	{
		if (Pair.Value)
		{
			Pair.Value->ClearActionPreview();
		}
	}
	bHasActionPreview = false;
	RefreshContextHighlight();
}

void UWacomBattleEnemyPanelWidget::SetHoveredPartSlotId(const FName InPartSlotId)
{
	if (HoveredPartSlotId == InPartSlotId)
	{
		return;
	}

	HoveredPartSlotId = InPartSlotId;
	for (const FWacomBattleEnemyPartEntryViewData& PartView : CurrentView.Parts)
	{
		if (TObjectPtr<UWacomBattleEnemyPartEntryWidget>* PartWidget =
			PartEntryWidgets.Find(BuildPartEntryWidgetKey(PartView)))
		{
			if (PartWidget->Get())
			{
				const FName EffectivePartSlotId = PartView.Identity.IsValidSlot()
					? PartView.Identity.GetEffectivePartSlotId()
					: PartView.PartSlotId;
				PartWidget->Get()->SetContextHighlighted(
					!HoveredPartSlotId.IsNone() && EffectivePartSlotId == HoveredPartSlotId);
			}
		}
	}
	RefreshContextHighlight();
}

void UWacomBattleEnemyPanelWidget::SetPartEntryWidgetClass(
	TSubclassOf<UWacomBattleEnemyPartEntryWidget> InWidgetClass)
{
	if (PartEntryWidgetClass == InWidgetClass)
	{
		return;
	}

	PartEntryWidgetClass = InWidgetClass;
	ClearPartEntries();
	SyncPartEntries();
}

void UWacomBattleEnemyPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshHeader();
	SyncPartEntries();
	RefreshContextHighlight();
}

void UWacomBattleEnemyPanelWidget::NativeDestruct()
{
	ClearPartEntries();
	Super::NativeDestruct();
}

void UWacomBattleEnemyPanelWidget::RefreshHeader()
{
	if (EnemyNameText)
	{
		EnemyNameText->SetText(bHasCurrentView
			? (CurrentView.EnemyDisplayName.IsEmpty()
				? FText::FromName(CurrentView.EnemySlotId)
				: CurrentView.EnemyDisplayName)
			: FText::GetEmpty());
	}
	if (EnemyInitiativeText)
	{
		EnemyInitiativeText->SetText(bHasCurrentView
			? FText::FromString(FString::Printf(
				TEXT("INIT %d"), CurrentView.EnemyInitiativeSum))
			: FText::GetEmpty());
	}
}

void UWacomBattleEnemyPanelWidget::SyncPartEntries()
{
	if (bSyncingPartEntries || !bHasCurrentView || !PartList || !PartEntryWidgetClass)
	{
		return;
	}

	TGuardValue<bool> SyncGuard(bSyncingPartEntries, true);
	TSet<FName> ActivePartKeys;
	for (int32 PartIndex = 0; PartIndex < CurrentView.Parts.Num(); ++PartIndex)
	{
		const FWacomBattleEnemyPartEntryViewData& PartView = CurrentView.Parts[PartIndex];
		const FName PartKey = BuildPartEntryWidgetKey(PartView);
		ActivePartKeys.Add(PartKey);
		UWacomBattleEnemyPartEntryWidget* PartWidget = FindOrCreatePartEntryWidget(PartView);
		if (!PartWidget)
		{
			continue;
		}

		PartWidget->SetPartEntryViewData(PartView);
		const FName EffectivePartSlotId = PartView.Identity.IsValidSlot()
			? PartView.Identity.GetEffectivePartSlotId()
			: PartView.PartSlotId;
		PartWidget->SetContextHighlighted(
			!HoveredPartSlotId.IsNone() && EffectivePartSlotId == HoveredPartSlotId);
		SyncPanelChildAt(PartList, PartWidget, PartIndex);
	}

	for (auto It = PartEntryWidgets.CreateIterator(); It; ++It)
	{
		if (!ActivePartKeys.Contains(It.Key()))
		{
			if (It.Value())
			{
				It.Value()->CancelPendingPresentation();
				PartList->RemoveChild(It.Value());
			}
			AnimatedPartEntryKeys.Remove(It.Key());
			It.RemoveCurrent();
		}
	}
	RefreshContextHighlight();
}

void UWacomBattleEnemyPanelWidget::ClearPartEntries()
{
	for (TPair<FName, TObjectPtr<UWacomBattleEnemyPartEntryWidget>>& Pair : PartEntryWidgets)
	{
		if (Pair.Value)
		{
			Pair.Value->CancelPendingPresentation();
		}
	}
	if (PartList)
	{
		PartList->ClearChildren();
	}
	PartEntryWidgets.Reset();
	AnimatedPartEntryKeys.Reset();
	bHasActionPreview = false;
}

void UWacomBattleEnemyPanelWidget::RefreshContextHighlight()
{
	if (PanelContextHighlight)
	{
		PanelContextHighlight->SetVisibility(
			(!HoveredPartSlotId.IsNone() || bHasActionPreview)
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed);
	}
}

UWacomBattleEnemyPartEntryWidget*
UWacomBattleEnemyPanelWidget::FindOrCreatePartEntryWidget(
	const FWacomBattleEnemyPartEntryViewData& PartView)
{
	const FName PartKey = BuildPartEntryWidgetKey(PartView);
	if (TObjectPtr<UWacomBattleEnemyPartEntryWidget>* Existing = PartEntryWidgets.Find(PartKey))
	{
		return Existing->Get();
	}

	UWacomBattleEnemyPartEntryWidget* PartWidget =
		CreateWidget<UWacomBattleEnemyPartEntryWidget>(
			this, PartEntryWidgetClass, MakeWidgetObjectName(PartKey));
	if (PartWidget)
	{
		PartWidget->SetIntroDelaySeconds(
			AnimatedPartEntryKeys.Num() * EntryIntroStaggerSeconds);
		AnimatedPartEntryKeys.Add(PartKey);
		PartEntryWidgets.Add(PartKey, PartWidget);
	}
	return PartWidget;
}

FName UWacomBattleEnemyPanelWidget::BuildPartEntryWidgetKey(
	const FWacomBattleEnemyPartEntryViewData& PartView) const
{
	const FName EnemySlotId = PartView.Identity.IsValidSlot()
		? PartView.Identity.GetEffectiveEnemySlotId()
		: (PartView.EnemySlotId.IsNone() ? CurrentView.EnemySlotId : PartView.EnemySlotId);
	const FName PartSlotId = PartView.Identity.IsValidSlot()
		? PartView.Identity.GetEffectivePartSlotId()
		: PartView.PartSlotId;
	return FName(*FString::Printf(
		TEXT("%s.%s"), *EnemySlotId.ToString(), *PartSlotId.ToString()));
}

bool UWacomBattleEnemyPanelWidget::DoesPartBelongToCurrentEnemy(
	const FWacomBattleEnemyPartEntryViewData& PartView) const
{
	if (!bHasCurrentView)
	{
		return false;
	}

	const FName EnemySlotId = PartView.Identity.IsValidSlot()
		? PartView.Identity.GetEffectiveEnemySlotId()
		: PartView.EnemySlotId;
	return EnemySlotId == CurrentView.EnemySlotId;
}
