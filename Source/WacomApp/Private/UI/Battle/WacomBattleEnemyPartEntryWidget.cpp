// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleEnemyPartEntryWidget.h"

#include "Animation/WidgetAnimation.h"
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
	RefreshPresentation();
}

void UWacomBattleEnemyPartEntryWidget::NativeDestruct()
{
	CancelPendingPresentation();
	Super::NativeDestruct();
}

void UWacomBattleEnemyPartEntryWidget::RefreshPresentation()
{
	const FWacomBattleEnemyPartEntryViewData& View = GetEffectivePartEntryViewData();
	const bool bContextActive = bContextHighlighted || bHasActionPreview;

	if (PartNameText)
	{
		PartNameText->SetText(View.PartDisplayName.IsEmpty()
			? FText::FromName(View.PartSlotId)
			: View.PartDisplayName);
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
		HpText->SetText(bDisplayCurrentHpOnly
			? FText::AsNumber(View.CurrentHp)
			: FText::FromString(FString::Printf(
				TEXT("%d/%d"), View.CurrentHp, View.MaxHp)));
	}
	if (ShieldBar)
	{
		const float ShieldFraction = View.MaxHp > 0
			? static_cast<float>(View.Shield) / static_cast<float>(View.MaxHp)
			: 0.0f;
		ShieldBar->SetPercent(FMath::Clamp(ShieldFraction, 0.0f, 1.0f));
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
	}
	if (ResistanceText)
	{
		ResistanceText->SetText(FText::FromString(FString::Printf(
			TEXT("RES %d"), View.CurrentIntentResistanceValue)));
	}
	if (StatusList)
	{
		StatusList->SetStatuses(View.RuntimeStatuses, View.RuntimeStatusStacks);
		StatusList->SetVisibility(View.RuntimeStatuses.IsEmpty()
			? ESlateVisibility::Collapsed
			: ESlateVisibility::HitTestInvisible);
	}
	if (DetailsContainer)
	{
		DetailsContainer->SetVisibility(bContextActive
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
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
}

void UWacomBattleEnemyPartEntryWidget::ScheduleIntroAnimation()
{
	CancelIntroTimer();
	if (IntroDelaySeconds <= 0.0f || !GetWorld())
	{
		PlayIntroAnimation();
		return;
	}

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
	SetVisibility(ESlateVisibility::HitTestInvisible);
	if (IntroAnimation)
	{
		PlayAnimation(IntroAnimation);
	}
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
}
