// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleEnemyPartEntryWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/World.h"
#include "UI/Battle/WacomBattleStatusIconWidget.h"
#include "UI/Battle/WacomBattleEnemyIntentPresentationStyle.h"

namespace
{
	bool AreEnemyPartStatusStacksEquivalent(
		const TMap<FGameplayTag, int32>& Left,
		const TMap<FGameplayTag, int32>& Right)
	{
		if (Left.Num() != Right.Num())
		{
			return false;
		}

		for (const TPair<FGameplayTag, int32>& Pair : Left)
		{
			const int32* RightValue = Right.Find(Pair.Key);
			if (!RightValue || *RightValue != Pair.Value)
			{
				return false;
			}
		}
		return true;
	}

	bool ArePartEntryViewsEquivalent(
		const FWacomBattleEnemyPartEntryViewData& Left,
		const FWacomBattleEnemyPartEntryViewData& Right)
	{
		return Left.PartInstanceId == Right.PartInstanceId
			&& Left.Identity == Right.Identity
			&& Left.EnemySlotId == Right.EnemySlotId
			&& Left.PartSlotId == Right.PartSlotId
			&& Left.PartDisplayName.EqualTo(Right.PartDisplayName)
			&& Left.CurrentHp == Right.CurrentHp
			&& Left.MaxHp == Right.MaxHp
			&& Left.Shield == Right.Shield
			&& Left.CurrentInitiative == Right.CurrentInitiative
			&& Left.CurrentIntentId == Right.CurrentIntentId
			&& Left.CurrentIntentDisplayName.EqualTo(Right.CurrentIntentDisplayName)
			&& Left.CurrentIntentInitiative == Right.CurrentIntentInitiative
			&& Left.CurrentIntentResistanceValue == Right.CurrentIntentResistanceValue
			&& Left.RuntimeStatuses.Num() == Right.RuntimeStatuses.Num()
			&& Left.RuntimeStatuses.HasAllExact(Right.RuntimeStatuses)
			&& Right.RuntimeStatuses.HasAllExact(Left.RuntimeStatuses)
			&& AreEnemyPartStatusStacksEquivalent(Left.RuntimeStatusStacks, Right.RuntimeStatusStacks)
			&& Left.bDestroyed == Right.bDestroyed
			&& Left.bActionPreviewWillAct == Right.bActionPreviewWillAct;
	}
}

void UWacomBattleEnemyPartEntryWidget::SetPartEntryViewData(
	const FWacomBattleEnemyPartEntryViewData& InView)
{
	const FWacomBattleEnemyPartEntryViewData PreviousView = CurrentView;
	const bool bHadViewData = bHasReceivedViewData;

	CurrentView = InView;
	bHasReceivedViewData = true;
	RefreshPresentation();

	if (!bHadViewData)
	{
		ScheduleIntroAnimation();
	}
	else
	{
		PlayRealFactTransition(PreviousView, CurrentView);
	}
}

const FWacomBattleEnemyPartEntryViewData&
UWacomBattleEnemyPartEntryWidget::GetEffectivePartEntryViewData() const
{
	return bHasActionPreview ? ActionPreviewView : CurrentView;
}

void UWacomBattleEnemyPartEntryWidget::SetActionPreview(
	const FWacomBattleEnemyPartEntryViewData& InPreviewView)
{
	if (bHasActionPreview && ArePartEntryViewsEquivalent(ActionPreviewView, InPreviewView))
	{
		return;
	}

	const bool bPreviousContextActive = bContextHighlighted || bHasActionPreview;
	ActionPreviewView = InPreviewView;
	bHasActionPreview = true;
	RefreshPresentation();
	RefreshContextPresentation(bPreviousContextActive);
}

void UWacomBattleEnemyPartEntryWidget::ClearActionPreview()
{
	if (!bHasActionPreview)
	{
		return;
	}

	const bool bPreviousContextActive = true;
	bHasActionPreview = false;
	RefreshPresentation();
	RefreshContextPresentation(bPreviousContextActive);
}

void UWacomBattleEnemyPartEntryWidget::SetContextHighlighted(const bool bHighlighted)
{
	if (bContextHighlighted == bHighlighted)
	{
		return;
	}

	const bool bPreviousContextActive = bContextHighlighted || bHasActionPreview;
	bContextHighlighted = bHighlighted;
	RefreshPresentation();
	RefreshContextPresentation(bPreviousContextActive);
}

void UWacomBattleEnemyPartEntryWidget::SetInspectionInteractionEnabled(const bool bEnabled)
{
	if (bInspectionInteractionEnabled == bEnabled)
	{
		return;
	}

	bInspectionInteractionEnabled = bEnabled;
	RefreshInspectionInteraction();
}

void UWacomBattleEnemyPartEntryWidget::SetIntroDelaySeconds(const float InDelaySeconds)
{
	IntroDelaySeconds = FMath::Max(0.0f, InDelaySeconds);
}

void UWacomBattleEnemyPartEntryWidget::CancelPendingPresentation()
{
	CancelIntroTimer();
	StopAllAnimations();
}

void UWacomBattleEnemyPartEntryWidget::SetIntentPresentationStyle(
	UWacomBattleEnemyIntentPresentationStyle* InStyle)
{
	if (IntentPresentationStyle == InStyle)
	{
		return;
	}

	IntentPresentationStyle = InStyle;
	RefreshPresentation();
}

void UWacomBattleEnemyPartEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (InspectHitTarget)
	{
		InspectHitTarget->OnClicked.RemoveAll(this);
		InspectHitTarget->OnClicked.AddDynamic(this, &ThisClass::HandleInspectClicked);
	}
	if (StatusList)
	{
		StatusList->SetMaxVisibleStatuses(3);
	}
	RefreshPresentation();
	RefreshInspectionInteraction();
}

void UWacomBattleEnemyPartEntryWidget::NativeDestruct()
{
	if (InspectHitTarget)
	{
		InspectHitTarget->OnClicked.RemoveAll(this);
	}
	OnInspectionRequestedNative.Clear();
	CancelPendingPresentation();
	Super::NativeDestruct();
}

void UWacomBattleEnemyPartEntryWidget::RefreshPresentation()
{
	const FWacomBattleEnemyPartEntryViewData& View = GetEffectivePartEntryViewData();

	if (PartNameText)
	{
		PartNameText->SetText(View.PartDisplayName.IsEmpty()
			? FText::FromName(View.PartSlotId)
			: View.PartDisplayName);
		PartNameText->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (HpBar)
	{
		const float HpFraction = View.MaxHp > 0
			? static_cast<float>(View.CurrentHp) / static_cast<float>(View.MaxHp)
			: 0.0f;
		HpBar->SetPercent(FMath::Clamp(HpFraction, 0.0f, 1.0f));
	}
	if (HpText)
	{
		HpText->SetText(FText::AsNumber(View.CurrentHp));
	}
	if (ShieldText)
	{
		ShieldText->SetText(FText::AsNumber(View.Shield));
	}
	if (ShieldContainer)
	{
		ShieldContainer->SetVisibility(View.Shield > 0
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	if (ShieldFrame)
	{
		ShieldFrame->SetVisibility(View.Shield > 0
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	if (ShieldBadge)
	{
		ShieldBadge->SetVisibility(View.Shield > 0
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	if (InitiativeText)
	{
		InitiativeText->SetText(FText::AsNumber(View.CurrentInitiative));
	}
	if (IntentIcon && IntentPresentationStyle)
	{
		if (const FSlateBrush* IntentBrush =
			IntentPresentationStyle->ResolveIntentIcon(View.CurrentIntentId))
		{
			IntentIcon->SetBrush(*IntentBrush);
		}
	}
	if (IntentText)
	{
		IntentText->SetText(View.CurrentIntentDisplayName.IsEmpty()
			? FText::FromString(TEXT("No intent"))
			: FText::FromString(FString::Printf(
				TEXT("%s  %d"),
				*View.CurrentIntentDisplayName.ToString(),
				View.CurrentIntentInitiative)));
		IntentText->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (ResistanceText)
	{
		ResistanceText->SetText(FText::FromString(FString::Printf(
			TEXT("RES %d"), View.CurrentIntentResistanceValue)));
		ResistanceText->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (StatusList)
	{
		// The compact-entry contract owns this limit independently of Slate
		// construction order. A panel can push ViewData before a newly-created
		// child has received NativeConstruct(), especially in WidgetComponent
		// and automation contexts.
		StatusList->SetMaxVisibleStatuses(3);
		StatusList->SetStatuses(View.RuntimeStatuses, View.RuntimeStatusStacks);
		StatusList->SetVisibility(View.RuntimeStatuses.IsEmpty()
			? ESlateVisibility::Collapsed
			: ESlateVisibility::HitTestInvisible);
	}
	if (StatusOverflowText)
	{
		const int32 OverflowCount = StatusList
			? StatusList->GetOverflowStatusCount()
			: FMath::Max(0, View.RuntimeStatuses.Num() - 3);
		StatusOverflowText->SetText(OverflowCount > 0
			? FText::FromString(FString::Printf(TEXT("+%d"), OverflowCount))
			: FText::GetEmpty());
		StatusOverflowText->SetVisibility(OverflowCount > 0
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	if (DetailsContainer)
	{
		DetailsContainer->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (ContextHighlight)
	{
		ContextHighlight->SetVisibility(bContextHighlighted
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	if (ActionPreviewOverlay)
	{
		ActionPreviewOverlay->SetVisibility(bHasActionPreview
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	if (DestroyedOverlay)
	{
		DestroyedOverlay->SetVisibility(View.bDestroyed
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	if (DestroyedMark)
	{
		DestroyedMark->SetVisibility(View.bDestroyed
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	const float PreviewOpacity = bHasActionPreview ? ActionPreviewRenderOpacity : 1.0f;
	SetRenderOpacity((View.bDestroyed ? 0.64f : 1.0f) * PreviewOpacity);
	RefreshInspectionInteraction();
}

void UWacomBattleEnemyPartEntryWidget::ScheduleIntroAnimation()
{
	CancelIntroTimer();
	if (IntroDelaySeconds <= 0.0f || !GetWorld())
	{
		PlayIntroAnimation();
		return;
	}

	bIntroPending = true;
	SetVisibility(ESlateVisibility::Hidden);
	FTimerDelegate Delegate = FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		PlayIntroAnimation();
	});
	GetWorld()->GetTimerManager().SetTimer(
		IntroTimerHandle,
		MoveTemp(Delegate),
		IntroDelaySeconds,
		false);
}

void UWacomBattleEnemyPartEntryWidget::PlayIntroAnimation()
{
	bIntroPending = false;
	RefreshInspectionInteraction();
	if (IntroAnimation)
	{
		PlayAnimation(IntroAnimation);
	}
}

void UWacomBattleEnemyPartEntryWidget::RefreshInspectionInteraction()
{
	if (bIntroPending)
	{
		SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	const bool bCanInspect = bInspectionInteractionEnabled
		&& !bHasActionPreview
		&& bHasReceivedViewData
		&& CurrentView.Identity.IsValidSlot();
	SetVisibility(bCanInspect
		? ESlateVisibility::SelfHitTestInvisible
		: ESlateVisibility::HitTestInvisible);
	if (InspectHitTarget)
	{
		InspectHitTarget->SetIsEnabled(bCanInspect);
		InspectHitTarget->SetVisibility(bCanInspect
			? ESlateVisibility::Visible
			: ESlateVisibility::HitTestInvisible);
	}
}

void UWacomBattleEnemyPartEntryWidget::HandleInspectClicked()
{
	if (!bInspectionInteractionEnabled
		|| bHasActionPreview
		|| !CurrentView.Identity.IsValidSlot())
	{
		return;
	}

	OnInspectionRequestedNative.Broadcast(CurrentView.Identity);
}

void UWacomBattleEnemyPartEntryWidget::PlayRealFactTransition(
	const FWacomBattleEnemyPartEntryViewData& PreviousView,
	const FWacomBattleEnemyPartEntryViewData& NewView)
{
	UWidgetAnimation* PrimaryAnimation = nullptr;
	if (!PreviousView.bDestroyed && NewView.bDestroyed)
	{
		PrimaryAnimation = DestroyedPulseAnimation;
	}
	else if (NewView.CurrentHp < PreviousView.CurrentHp)
	{
		PrimaryAnimation = DamagePulseAnimation;
	}
	else if (NewView.Shield != PreviousView.Shield)
	{
		PrimaryAnimation = ShieldPulseAnimation;
	}

	if (PrimaryAnimation)
	{
		StopAnimation(PrimaryAnimation);
		PlayAnimation(PrimaryAnimation, 0.0f, 1,
			EUMGSequencePlayMode::Forward, 1.0f, true);
	}

	if (NewView.bDestroyed)
	{
		return;
	}

	if (InitiativePulseAnimation
		&& PreviousView.CurrentInitiative != NewView.CurrentInitiative)
	{
		StopAnimation(InitiativePulseAnimation);
		PlayAnimation(InitiativePulseAnimation, 0.0f, 1,
			EUMGSequencePlayMode::Forward, 1.0f, true);
	}
	if (IntentChangedAnimation
		&& PreviousView.CurrentIntentId != NewView.CurrentIntentId)
	{
		StopAnimation(IntentChangedAnimation);
		PlayAnimation(IntentChangedAnimation, 0.0f, 1,
			EUMGSequencePlayMode::Forward, 1.0f, true);
	}
}

void UWacomBattleEnemyPartEntryWidget::RefreshContextPresentation(
	const bool bPreviousContextActive)
{
	const bool bContextActive = bContextHighlighted || bHasActionPreview;
	if (!ContextHighlightAnimation || bContextActive == bPreviousContextActive)
	{
		return;
	}

	if (bContextActive)
	{
		PlayAnimationForward(ContextHighlightAnimation);
	}
	else
	{
		PlayAnimationReverse(ContextHighlightAnimation);
	}
}

void UWacomBattleEnemyPartEntryWidget::CancelIntroTimer()
{
	if (IntroTimerHandle.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(IntroTimerHandle);
		}
		IntroTimerHandle.Invalidate();
	}
	bIntroPending = false;
}
