// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleEnemyInspectionWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/World.h"
#include "UI/Battle/WacomBattleEnemyInspectionPartRowWidget.h"
#include "UI/Battle/WacomBattleEnemyIntentPresentation.h"
#include "UI/Battle/WacomBattleEnemyIntentPresentationStyle.h"
#include "UI/Battle/WacomBattleIntentTooltipWidget.h"
#include "UI/Battle/WacomBattleStatusIconWidget.h"

namespace
{
	FString MakeInspectionRowObjectName(const FName StableKey)
	{
		FString Name = StableKey.ToString();
		for (TCHAR& Character : Name)
		{
			if (!FChar::IsAlnum(Character) && Character != TEXT('_'))
			{
				Character = TEXT('_');
			}
		}
		return FString::Printf(TEXT("EnemyInspectionPart_%s"), *Name);
	}
}

bool UWacomBattleEnemyInspectionWidget::SetInspectionViewData(
	const FWacomBattleEnemyInspectionViewData& InView)
{
	CurrentView = InView;
	bHasViewData = !CurrentView.Enemy.Parts.IsEmpty();
	if (!bHasViewData || !EnsureValidSelection())
	{
		ClearInspectionViewData();
		return false;
	}

	if (EnemyNameText)
	{
		EnemyNameText->SetText(CurrentView.Enemy.EnemyDisplayName.IsEmpty()
			? FText::FromName(CurrentView.Enemy.EnemySlotId)
			: CurrentView.Enemy.EnemyDisplayName);
	}
	if (EnemyStateText)
	{
		int32 DestroyedPartCount = 0;
		for (const FWacomBattleEnemyPartEntryViewData& Part : CurrentView.Enemy.Parts)
		{
			DestroyedPartCount += Part.bDestroyed ? 1 : 0;
		}
		EnemyStateText->SetText(DestroyedPartCount == CurrentView.Enemy.Parts.Num()
			? FText::FromString(TEXT("已击破"))
			: FText::FromString(FString::Printf(
				TEXT("部位 %d / %d"),
				CurrentView.Enemy.Parts.Num() - DestroyedPartCount,
				CurrentView.Enemy.Parts.Num())));
	}
	SyncPartRows();
	RefreshSelectedPartDetails();
	return true;
}

void UWacomBattleEnemyInspectionWidget::ClearInspectionViewData()
{
	bHasViewData = false;
	CurrentView = FWacomBattleEnemyInspectionViewData();
	ClearPartRows();
	if (StatusList)
	{
		StatusList->SetStatusIconViews({});
	}
	ClearIntentPresentation();
}

void UWacomBattleEnemyInspectionWidget::OpenInspection()
{
	if (!bHasViewData)
	{
		return;
	}

	if (CloseAnimation)
	{
		UnbindAllFromAnimationFinished(CloseAnimation);
	}
	StopAllAnimations();
	bClosing = false;
	bOpen = true;
	SetIsEnabled(true);
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (StatusList)
	{
		StatusList->SetStatusInspectionEnabled(true);
	}
	RefreshIntentPresentation(FindSelectedPart());
	if (OpenLeftAnimation)
	{
		PlayAnimation(OpenLeftAnimation);
	}
	ScheduleRightPanelOpen();
}

void UWacomBattleEnemyInspectionWidget::CloseInspection(const bool bImmediate)
{
	if (!bOpen && !bClosing)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SetIsEnabled(false);
	if (StatusList)
	{
		StatusList->SetStatusInspectionEnabled(false);
	}
	ClearIntentPresentation();
	CancelRightPanelOpenTimer();
	if (bImmediate || !CloseAnimation)
	{
		StopAllAnimations();
		bOpen = false;
		bClosing = false;
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	bClosing = true;
	UnbindAllFromAnimationFinished(CloseAnimation);
	FWidgetAnimationDynamicEvent FinishedEvent;
	FinishedEvent.BindDynamic(this, &ThisClass::HandleCloseAnimationFinished);
	BindToAnimationFinished(CloseAnimation, FinishedEvent);
	PlayAnimation(CloseAnimation);
}

void UWacomBattleEnemyInspectionWidget::SetPartRowWidgetClass(
	TSubclassOf<UWacomBattleEnemyInspectionPartRowWidget> InClass)
{
	if (PartRowWidgetClass == InClass)
	{
		return;
	}
	PartRowWidgetClass = InClass;
	ClearPartRows();
	SyncPartRows();
}

void UWacomBattleEnemyInspectionWidget::SetIntentPresentationStyle(
	UWacomBattleEnemyIntentPresentationStyle* InStyle)
{
	if (IntentPresentationStyle == InStyle)
	{
		return;
	}
	IntentPresentationStyle = InStyle;
	RefreshSelectedPartDetails();
}

void UWacomBattleEnemyInspectionWidget::SetIntentTooltipWidgetClass(
	TSubclassOf<UWacomBattleIntentTooltipWidget> InClass)
{
	IntentTooltipWidgetClass = InClass
		? InClass
		: TSubclassOf<UWacomBattleIntentTooltipWidget>(
			UWacomBattleIntentTooltipWidget::StaticClass());
	CachedIntentTooltipWidget = nullptr;
}

void UWacomBattleEnemyInspectionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!IntentTooltipWidgetClass)
	{
		IntentTooltipWidgetClass =
			UWacomBattleIntentTooltipWidget::StaticClass();
	}
	EnsureIntentTooltipBinding();
	if (IntentTooltipTarget)
	{
		IntentTooltipTarget->SynchronizeProperties();
	}
	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveAll(this);
		CloseButton->OnClicked.AddDynamic(this, &ThisClass::HandleCloseClicked);
	}
	if (StatusList)
	{
		StatusList->SetMaxVisibleStatuses(0);
		StatusList->SetInspectionHost(
			EWacomBattleStatusInspectionHost::EnemyPart);
		StatusList->SetStatusIconActivationEnabled(false);
		StatusList->SetStatusInspectionEnabled(false);
	}
	SetVisibility(ESlateVisibility::Collapsed);
	SyncPartRows();
	RefreshSelectedPartDetails();
}

void UWacomBattleEnemyInspectionWidget::NativeDestruct()
{
	CancelRightPanelOpenTimer();
	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveAll(this);
	}
	if (CloseAnimation)
	{
		UnbindAllFromAnimationFinished(CloseAnimation);
	}
	StopAllAnimations();
	if (IntentTooltipTarget)
	{
		IntentTooltipTarget->ToolTipWidgetDelegate.Unbind();
	}
	ClearIntentPresentation();
	ClearPartRows();
	if (StatusList)
	{
		StatusList->SetStatusInspectionEnabled(false);
	}
	OnCloseRequestedNative.Clear();
	OnSelectionRequestedNative.Clear();
	bOpen = false;
	bClosing = false;
	Super::NativeDestruct();
}

void UWacomBattleEnemyInspectionWidget::SynchronizeProperties()
{
	EnsureIntentTooltipBinding();
	Super::SynchronizeProperties();
}

void UWacomBattleEnemyInspectionWidget::SyncPartRows()
{
	if (!bHasViewData || !PartNavigator || !PartRowWidgetClass)
	{
		return;
	}

	TSet<FName> ActiveKeys;
	for (int32 Index = 0; Index < CurrentView.Enemy.Parts.Num(); ++Index)
	{
		const FWacomBattleEnemyPartEntryViewData& Part = CurrentView.Enemy.Parts[Index];
		const FName Key = BuildPartRowKey(Part);
		ActiveKeys.Add(Key);
		UWacomBattleEnemyInspectionPartRowWidget* Row = nullptr;
		if (TObjectPtr<UWacomBattleEnemyInspectionPartRowWidget>* Existing = PartRows.Find(Key))
		{
			Row = Existing->Get();
		}
		if (!Row)
		{
			Row = CreateWidget<UWacomBattleEnemyInspectionPartRowWidget>(
				this, PartRowWidgetClass, FName(*MakeInspectionRowObjectName(Key)));
			if (!Row)
			{
				continue;
			}
			Row->OnPartSelectedNative.AddUObject(
				this, &ThisClass::HandlePartRowSelected);
			PartRows.Add(Key, Row);
		}

		Row->SetPartViewData(Part);
		Row->SetSelected(Part.Identity == CurrentView.SelectedPartIdentity);
		if (PartNavigator->GetChildIndex(Row) == INDEX_NONE)
		{
			PartNavigator->AddChild(Row);
		}
		PartNavigator->ShiftChild(Index, Row);
	}

	for (auto It = PartRows.CreateIterator(); It; ++It)
	{
		if (!ActiveKeys.Contains(It.Key()))
		{
			if (It.Value())
			{
				It.Value()->OnPartSelectedNative.RemoveAll(this);
				PartNavigator->RemoveChild(It.Value());
			}
			It.RemoveCurrent();
		}
	}
}

void UWacomBattleEnemyInspectionWidget::ClearPartRows()
{
	for (TPair<FName, TObjectPtr<UWacomBattleEnemyInspectionPartRowWidget>>& Pair : PartRows)
	{
		if (Pair.Value)
		{
			Pair.Value->OnPartSelectedNative.RemoveAll(this);
		}
	}
	if (PartNavigator)
	{
		PartNavigator->ClearChildren();
	}
	PartRows.Reset();
}

void UWacomBattleEnemyInspectionWidget::RefreshSelectedPartDetails()
{
	const FWacomBattleEnemyPartEntryViewData* Part = FindSelectedPart();
	if (!Part)
	{
		ClearIntentPresentation();
		return;
	}

	if (SelectedPartNameText)
	{
		SelectedPartNameText->SetText(Part->PartDisplayName.IsEmpty()
			? FText::FromName(Part->PartSlotId)
			: Part->PartDisplayName);
	}
	if (HpBar)
	{
		HpBar->SetPercent(Part->MaxHp > 0
			? FMath::Clamp(static_cast<float>(Part->CurrentHp) / static_cast<float>(Part->MaxHp), 0.0f, 1.0f)
			: 0.0f);
	}
	if (HpText)
	{
		HpText->SetText(FText::FromString(FString::Printf(
			TEXT("%d / %d"), Part->CurrentHp, Part->MaxHp)));
	}
	if (ShieldText)
	{
		ShieldText->SetText(FText::AsNumber(Part->Shield));
	}
	if (ShieldContainer)
	{
		ShieldContainer->SetVisibility(Part->Shield > 0
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	if (InitiativeText)
	{
		InitiativeText->SetText(FText::AsNumber(Part->CurrentInitiative));
	}
	if (IntentText)
	{
		IntentText->SetText(Part->CurrentIntentDisplayName.IsEmpty()
			? FText::FromName(Part->CurrentIntentId)
			: Part->CurrentIntentDisplayName);
	}
	if (IntentIcon && IntentPresentationStyle)
	{
		if (const FSlateBrush* IntentBrush =
			IntentPresentationStyle->ResolveIntentIcon(Part->CurrentIntentId))
		{
			IntentIcon->SetBrush(*IntentBrush);
		}
	}
	if (ResistanceText)
	{
		ResistanceText->SetText(FText::FromString(
			Part->bCurrentIntentIsAttack
				? FString::Printf(
					TEXT("INIT %d   ATK %d"),
					Part->CurrentIntentInitiative,
					Part->CurrentIntentPeakAttackDamage)
				: FString::Printf(
					TEXT("INIT %d"),
					Part->CurrentIntentInitiative)));
	}
	if (StatusList)
	{
		StatusList->SetStatuses(Part->RuntimeStatuses, Part->RuntimeStatusStacks);
	}
	if (DestroyedOverlay)
	{
		DestroyedOverlay->SetVisibility(Part->bDestroyed
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	RefreshIntentPresentation(Part);
}

void UWacomBattleEnemyInspectionWidget::EnsureIntentTooltipBinding()
{
	if (IntentTooltipTarget
		&& !IntentTooltipTarget->ToolTipWidgetDelegate.IsBound())
	{
		IntentTooltipTarget->ToolTipWidgetDelegate.BindDynamic(
			this,
			&ThisClass::HandleBuildIntentTooltipWidget);
	}
}

void UWacomBattleEnemyInspectionWidget::RefreshIntentPresentation(
	const FWacomBattleEnemyPartEntryViewData* Part)
{
	ClearIntentPresentation();
	if (!Part || Part->bDestroyed || Part->CurrentIntentId.IsNone())
	{
		return;
	}

	if (IntentTooltipTarget)
	{
		const bool bCanInspectIntent = bOpen && !bClosing;
		IntentTooltipTarget->SetIsEnabled(bCanInspectIntent);
		IntentTooltipTarget->SetVisibility(bCanInspectIntent
			? ESlateVisibility::Visible
			: ESlateVisibility::HitTestInvisible);
	}
	if (CachedIntentTooltipWidget)
	{
		CachedIntentTooltipWidget->SetIntentViewData(
			FWacomBattleIntentPresentationBuilder::Build(
				*Part, IntentPresentationStyle, 5));
	}
}

void UWacomBattleEnemyInspectionWidget::ClearIntentPresentation()
{
	if (IntentTooltipTarget)
	{
		IntentTooltipTarget->SetIsEnabled(false);
		IntentTooltipTarget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (CachedIntentTooltipWidget)
	{
		CachedIntentTooltipWidget->SetIntentViewData(
			FWacomBattleIntentPresentationViewData());
	}
}

UWidget* UWacomBattleEnemyInspectionWidget::HandleBuildIntentTooltipWidget()
{
	const FWacomBattleEnemyPartEntryViewData* Part = FindSelectedPart();
	if (!bOpen || bClosing || !Part
		|| Part->bDestroyed || Part->CurrentIntentId.IsNone())
	{
		return nullptr;
	}
	if (!CachedIntentTooltipWidget)
	{
		UClass* TooltipClass = IntentTooltipWidgetClass
			? IntentTooltipWidgetClass.Get()
			: UWacomBattleIntentTooltipWidget::StaticClass();
		CachedIntentTooltipWidget = GetWorld()
			? CreateWidget<UWacomBattleIntentTooltipWidget>(
				GetWorld(), TooltipClass)
			: NewObject<UWacomBattleIntentTooltipWidget>(
				this, TooltipClass);
	}
	if (CachedIntentTooltipWidget)
	{
		CachedIntentTooltipWidget->SetIntentViewData(
			FWacomBattleIntentPresentationBuilder::Build(
				*Part, IntentPresentationStyle, 5));
	}
	return CachedIntentTooltipWidget;
}

const FWacomBattleEnemyPartEntryViewData*
UWacomBattleEnemyInspectionWidget::FindSelectedPart() const
{
	return CurrentView.Enemy.Parts.FindByPredicate(
		[this](const FWacomBattleEnemyPartEntryViewData& Part)
		{
			return Part.Identity == CurrentView.SelectedPartIdentity;
		});
}

bool UWacomBattleEnemyInspectionWidget::EnsureValidSelection()
{
	if (FindSelectedPart())
	{
		return true;
	}

	for (const FWacomBattleEnemyPartEntryViewData& Part : CurrentView.Enemy.Parts)
	{
		if (Part.Identity.IsValidSlot())
		{
			CurrentView.SelectedPartIdentity = Part.Identity;
			return true;
		}
	}
	return false;
}

FName UWacomBattleEnemyInspectionWidget::BuildPartRowKey(
	const FWacomBattleEnemyPartEntryViewData& PartView) const
{
	const FName EnemySlotId = PartView.Identity.IsValidSlot()
		? PartView.Identity.GetEffectiveEnemySlotId()
		: PartView.EnemySlotId;
	const FName PartSlotId = PartView.Identity.IsValidSlot()
		? PartView.Identity.GetEffectivePartSlotId()
		: PartView.PartSlotId;
	return FName(*FString::Printf(
		TEXT("%s.%s"),
		*EnemySlotId.ToString(),
		*PartSlotId.ToString()));
}

void UWacomBattleEnemyInspectionWidget::HandlePartRowSelected(
	const FBattlePartSlotIdentity& PartIdentity)
{
	if (!PartIdentity.IsValidSlot())
	{
		return;
	}
	CurrentView.SelectedPartIdentity = PartIdentity;
	SyncPartRows();
	RefreshSelectedPartDetails();
	OnSelectionRequestedNative.Broadcast(PartIdentity);
}

void UWacomBattleEnemyInspectionWidget::HandleCloseClicked()
{
	OnCloseRequestedNative.Broadcast();
}

void UWacomBattleEnemyInspectionWidget::HandleCloseAnimationFinished()
{
	if (CloseAnimation)
	{
		UnbindAllFromAnimationFinished(CloseAnimation);
	}
	bOpen = false;
	bClosing = false;
	SetVisibility(ESlateVisibility::Collapsed);
}

void UWacomBattleEnemyInspectionWidget::ScheduleRightPanelOpen()
{
	CancelRightPanelOpenTimer();
	if (!OpenRightAnimation)
	{
		return;
	}
	if (!GetWorld())
	{
		PlayRightPanelOpen();
		return;
	}

	FTimerDelegate Delegate = FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		PlayRightPanelOpen();
	});
	GetWorld()->GetTimerManager().SetTimer(
		RightPanelOpenTimerHandle,
		MoveTemp(Delegate),
		0.04f,
		false);
}

void UWacomBattleEnemyInspectionWidget::PlayRightPanelOpen()
{
	RightPanelOpenTimerHandle.Invalidate();
	if (bOpen && !bClosing && OpenRightAnimation)
	{
		PlayAnimation(OpenRightAnimation);
	}
}

void UWacomBattleEnemyInspectionWidget::CancelRightPanelOpenTimer()
{
	if (RightPanelOpenTimerHandle.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(RightPanelOpenTimerHandle);
		}
		RightPanelOpenTimerHandle.Invalidate();
	}
}
