// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleEnemyPanelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
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
		if (UHorizontalBoxSlot* HorizontalSlot = Cast<UHorizontalBoxSlot>(Child->Slot))
		{
			HorizontalSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			HorizontalSlot->SetPadding(FMargin(0.0f));
			HorizontalSlot->SetHorizontalAlignment(HAlign_Fill);
			HorizontalSlot->SetVerticalAlignment(VAlign_Fill);
		}
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

	template <typename WidgetType>
	void ResolveWidgetBinding(
		UWidgetTree* WidgetTree,
		TObjectPtr<WidgetType>& Binding,
		const FName WidgetName)
	{
		if (!Binding && WidgetTree)
		{
			Binding = Cast<WidgetType>(WidgetTree->FindWidget(WidgetName));
		}
	}
}

void UWacomBattleEnemyPanelWidget::SetEnemyPanelViewData(
	const FWacomBattleEnemyPanelViewData& InView)
{
	CurrentView = InView;
	bHasCurrentView = true;
	SyncPartEntries();
}

void UWacomBattleEnemyPanelWidget::ClearEnemyPanelViewData()
{
	ClearActionPreview();
	HoveredPartSlotId = NAME_None;
	bHasCurrentView = false;
	CurrentView = FWacomBattleEnemyPanelViewData();
	ClearPartEntries();
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
	ApplyInspectionInteractionState();
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
	ApplyInspectionInteractionState();
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

void UWacomBattleEnemyPanelWidget::SetInspectionInteractionEnabled(const bool bEnabled)
{
	if (bInspectionInteractionEnabled == bEnabled)
	{
		return;
	}

	bInspectionInteractionEnabled = bEnabled;
	ApplyInspectionInteractionState();
}

void UWacomBattleEnemyPanelWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	ResolveAuthoredBindings();
}

void UWacomBattleEnemyPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ResolveAuthoredBindings();
	ApplyAuthoredGeometry();
	SyncPartEntries();
}

void UWacomBattleEnemyPanelWidget::ResolveAuthoredBindings()
{
	ResolveWidgetBinding(WidgetTree, PanelRoot, TEXT("PanelRoot"));
	ResolveWidgetBinding(WidgetTree, PartList, TEXT("PartList"));
}

void UWacomBattleEnemyPanelWidget::NativeDestruct()
{
	ClearPartEntries();
	OnInspectionRequestedNative.Clear();
	Super::NativeDestruct();
}

void UWacomBattleEnemyPanelWidget::ApplyAuthoredGeometry()
{
	if (!PanelRoot)
	{
		return;
	}
	PanelRoot->SetMinDesiredHeight(92.0f);
	if (FixedPanelWidth > 0.0f)
	{
		PanelRoot->SetWidthOverride(FixedPanelWidth);
	}
	else
	{
		PanelRoot->ClearWidthOverride();
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

		PartWidget->SetSegmentLayout(PartIndex, CurrentView.Parts.Num());
		PartWidget->SetPartEntryViewData(PartView);
		const FName EffectivePartSlotId = PartView.Identity.IsValidSlot()
			? PartView.Identity.GetEffectivePartSlotId()
			: PartView.PartSlotId;
		PartWidget->SetContextHighlighted(
			!HoveredPartSlotId.IsNone() && EffectivePartSlotId == HoveredPartSlotId);
		PartWidget->SetInspectionInteractionEnabled(
			bInspectionInteractionEnabled && !bHasActionPreview);
		SyncPanelChildAt(PartList, PartWidget, PartIndex);
	}

	for (auto It = PartEntryWidgets.CreateIterator(); It; ++It)
	{
		if (!ActivePartKeys.Contains(It.Key()))
		{
			if (It.Value())
			{
				It.Value()->OnInspectionRequestedNative.RemoveAll(this);
				It.Value()->CancelPendingPresentation();
				PartList->RemoveChild(It.Value());
			}
			AnimatedPartEntryKeys.Remove(It.Key());
			It.RemoveCurrent();
		}
	}
}

void UWacomBattleEnemyPanelWidget::ClearPartEntries()
{
	for (TPair<FName, TObjectPtr<UWacomBattleEnemyPartEntryWidget>>& Pair : PartEntryWidgets)
	{
		if (Pair.Value)
		{
			Pair.Value->OnInspectionRequestedNative.RemoveAll(this);
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
		PartWidget->OnInspectionRequestedNative.RemoveAll(this);
		PartWidget->OnInspectionRequestedNative.AddUObject(
			this, &ThisClass::HandlePartInspectionRequested);
		PartEntryWidgets.Add(PartKey, PartWidget);
	}
	return PartWidget;
}

void UWacomBattleEnemyPanelWidget::ApplyInspectionInteractionState()
{
	const bool bEnableEntries = bInspectionInteractionEnabled
		&& bHasCurrentView
		&& !bHasActionPreview;
	for (TPair<FName, TObjectPtr<UWacomBattleEnemyPartEntryWidget>>& Pair : PartEntryWidgets)
	{
		if (Pair.Value)
		{
			Pair.Value->SetInspectionInteractionEnabled(bEnableEntries);
		}
	}
}

void UWacomBattleEnemyPanelWidget::HandlePartInspectionRequested(
	const FBattlePartSlotIdentity& PartIdentity)
{
	if (!bInspectionInteractionEnabled
		|| bHasActionPreview
		|| !PartIdentity.IsValidSlot())
	{
		return;
	}

	OnInspectionRequestedNative.Broadcast(PartIdentity);
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
