// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleEnemyPartEntryWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/World.h"
#include "UI/Battle/WacomBattleEnemyIntentPresentationStyle.h"
#include "UI/Battle/WacomBattleEnemyVitalsPresentation.h"
#include "UI/Battle/WacomBattleStatusIconWidget.h"

namespace
{
	constexpr float StandardDamageDurationSeconds = 0.22f;
	constexpr float StandardShieldDurationSeconds = 0.18f;
	constexpr float StandardShieldBreakDurationSeconds = 0.24f;
	constexpr float StandardInitiativeDurationSeconds = 0.12f;
	constexpr float StandardIntentDurationSeconds = 0.18f;
	constexpr float StandardDestroyedDurationSeconds = 0.30f;
	constexpr float StandardIntroDurationSeconds = 0.22f;
}

UWacomBattleEnemyPartEntryWidget::UWacomBattleEnemyPartEntryWidget(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, PresentationState(MakePimpl<FWacomBattleEnemyPartPresentationState>())
	, VitalsMaterialAdapter(MakePimpl<FWacomBattleEnemyVitalsMaterialAdapter>())
{
}

UWacomBattleEnemyPartEntryWidget::~UWacomBattleEnemyPartEntryWidget() = default;

void UWacomBattleEnemyPartEntryWidget::SetPartEntryViewData(
	const FWacomBattleEnemyPartEntryViewData& InView)
{
	FWacomBattleEnemyPartPresentationUpdate Update =
		PresentationState->SetRealView(InView, ResolveWorldTimeSeconds());
	if (Update.bIntentChanged)
	{
		CaptureOutgoingIntent(Update.PreviousView);
	}
	RefreshPresentation();
	RouteMotionCues(Update);
}

const FWacomBattleEnemyPartEntryViewData&
UWacomBattleEnemyPartEntryWidget::GetPartEntryViewData() const
{
	return PresentationState->GetRealView();
}

const FWacomBattleEnemyPartEntryViewData&
UWacomBattleEnemyPartEntryWidget::GetEffectivePartEntryViewData() const
{
	return PresentationState->GetDisplayView();
}

void UWacomBattleEnemyPartEntryWidget::SetActionPreview(
	const FWacomBattleEnemyPartEntryViewData& InPreviewView)
{
	FWacomBattleEnemyPartPresentationUpdate Update =
		PresentationState->SetActionPreview(InPreviewView);
	RefreshPresentation();
	RouteMotionCues(Update);
}

void UWacomBattleEnemyPartEntryWidget::ClearActionPreview()
{
	if (!PresentationState->HasActionPreview())
	{
		return;
	}
	FWacomBattleEnemyPartPresentationUpdate Update =
		PresentationState->ClearActionPreview();
	RefreshPresentation();
	RouteMotionCues(Update);
}

bool UWacomBattleEnemyPartEntryWidget::HasActionPreview() const
{
	return PresentationState->HasActionPreview();
}

void UWacomBattleEnemyPartEntryWidget::SetContextHighlighted(const bool bHighlighted)
{
	FWacomBattleEnemyPartPresentationUpdate Update =
		PresentationState->SetContextHighlighted(bHighlighted);
	RefreshPresentation();
	RouteMotionCues(Update);
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

void UWacomBattleEnemyPartEntryWidget::SetSegmentLayout(
	const int32 InPartIndex,
	const int32 InPartCount)
{
	PresentationState->SetSegmentLayout(InPartIndex, InPartCount);
	ApplyVitalsMaterialPresentation();
}

void UWacomBattleEnemyPartEntryWidget::CancelPendingPresentation()
{
	CancelIntroTimer();
	StopAllAnimations();
	if (OutgoingIntentIcon)
	{
		OutgoingIntentIcon->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UWacomBattleEnemyPartEntryWidget::ApplyRuntimePresentationPolicy(
	const bool bSimplifiedMotion,
	const float FlashIntensity)
{
	const FWacomBattleEnemyPartPresentationUpdate Update =
		PresentationState->SetMotionPolicy(bSimplifiedMotion, FlashIntensity);
	if (Update.bEnteredReducedMotion)
	{
		StopAllAnimations();
		HandleIntentChangeAnimationFinished();
	}
	ApplyVitalsMaterialPresentation();
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
	if (IntentChangeAnimation)
	{
		UnbindAllFromAnimationFinished(IntentChangeAnimation);
		FWidgetAnimationDynamicEvent FinishedEvent;
		FinishedEvent.BindDynamic(this, &ThisClass::HandleIntentChangeAnimationFinished);
		BindToAnimationFinished(IntentChangeAnimation, FinishedEvent);
	}
	if (PartEntryRoot)
	{
		PartEntryRoot->SetMinDesiredWidth(116.0f);
		PartEntryRoot->SetHeightOverride(92.0f);
	}
	VitalsMaterialAdapter->Initialize(VitalsTrackImage);
	RefreshPresentation();
	RefreshInspectionInteraction();
}

void UWacomBattleEnemyPartEntryWidget::NativeDestruct()
{
	if (InspectHitTarget)
	{
		InspectHitTarget->OnClicked.RemoveAll(this);
	}
	if (IntentChangeAnimation)
	{
		UnbindAllFromAnimationFinished(IntentChangeAnimation);
	}
	OnInspectionRequestedNative.Clear();
	CancelPendingPresentation();
	PresentationState->ResetTransientPresentation();
	VitalsMaterialAdapter->RestoreAuthoredBrush();
	Super::NativeDestruct();
}

void UWacomBattleEnemyPartEntryWidget::RefreshPresentation()
{
	if (!PresentationState->HasRealView())
	{
		return;
	}
	const FWacomBattleEnemyPartEntryViewData& View =
		PresentationState->GetDisplayView();
	if (HpText)
	{
		HpText->SetText(FText::AsNumber(View.CurrentHp));
	}
	if (ShieldText)
	{
		ShieldText->SetText(FText::AsNumber(View.Shield));
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
	if (StatusList)
	{
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
	if (DestroyedMark)
	{
		DestroyedMark->SetVisibility(View.bDestroyed
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	if (ShieldValueRoot)
	{
		ShieldValueRoot->SetVisibility(View.Shield > 0
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	if (ContextSurface)
	{
		ContextSurface->SetVisibility(PresentationState->IsContextActive()
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	if (DestroyedSurface)
	{
		DestroyedSurface->SetVisibility(View.bDestroyed
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	SetRenderOpacity(1.0f);
	ApplyVitalsMaterialPresentation();
	RefreshInspectionInteraction();
}

void UWacomBattleEnemyPartEntryWidget::ApplyVitalsMaterialPresentation()
{
	if (!PresentationState->HasRealView()
		|| !VitalsMaterialAdapter->Initialize(VitalsTrackImage))
	{
		return;
	}
	VitalsMaterialAdapter->ApplyFrame(PresentationState->BuildVitalsFrame(
		DamageTrailHoldSeconds,
		DamageTrailRecoverySeconds));
}

void UWacomBattleEnemyPartEntryWidget::CaptureOutgoingIntent(
	const FWacomBattleEnemyPartEntryViewData& PreviousView)
{
	if (!OutgoingIntentIcon || !IntentPresentationStyle)
	{
		return;
	}
	if (const FSlateBrush* PreviousBrush =
		IntentPresentationStyle->ResolveIntentIcon(PreviousView.CurrentIntentId))
	{
		OutgoingIntentIcon->SetBrush(*PreviousBrush);
		OutgoingIntentIcon->SetRenderOpacity(1.0f);
		OutgoingIntentIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

float UWacomBattleEnemyPartEntryWidget::ResolveWorldTimeSeconds() const
{
	return GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
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
	PlaySemanticAnimation(IntroAnimation, StandardIntroDurationSeconds);
}

void UWacomBattleEnemyPartEntryWidget::RouteMotionCues(
	const FWacomBattleEnemyPartPresentationUpdate& Update)
{
	for (const EWacomBattleEnemyMotionCue Cue : Update.Cues)
	{
		RouteMotionCue(Cue);
	}
}

void UWacomBattleEnemyPartEntryWidget::RouteMotionCue(
	const EWacomBattleEnemyMotionCue Cue)
{
	switch (Cue)
	{
	case EWacomBattleEnemyMotionCue::Intro:
		ScheduleIntroAnimation();
		break;
	case EWacomBattleEnemyMotionCue::Damage:
		PlaySemanticAnimation(DamageImpactAnimation, StandardDamageDurationSeconds);
		break;
	case EWacomBattleEnemyMotionCue::ShieldImpact:
		PlaySemanticAnimation(ShieldImpactAnimation, StandardShieldDurationSeconds);
		break;
	case EWacomBattleEnemyMotionCue::ShieldBreak:
		PlaySemanticAnimation(ShieldBreakAnimation, StandardShieldBreakDurationSeconds);
		break;
	case EWacomBattleEnemyMotionCue::InitiativeStep:
		PlaySemanticAnimation(InitiativeStepAnimation, StandardInitiativeDurationSeconds);
		break;
	case EWacomBattleEnemyMotionCue::IntentChange:
		PlaySemanticAnimation(IntentChangeAnimation, StandardIntentDurationSeconds);
		if (!IntentChangeAnimation || PresentationState->IsUsingSimplifiedMotion())
		{
			HandleIntentChangeAnimationFinished();
		}
		break;
	case EWacomBattleEnemyMotionCue::ContextEnter:
		if (ContextAnimation && !PresentationState->IsUsingSimplifiedMotion())
		{
			PlayAnimationForward(ContextAnimation);
		}
		break;
	case EWacomBattleEnemyMotionCue::ContextExit:
		if (ContextAnimation && !PresentationState->IsUsingSimplifiedMotion())
		{
			PlayAnimationReverse(ContextAnimation);
		}
		break;
	case EWacomBattleEnemyMotionCue::Destroyed:
		PlaySemanticAnimation(DestroyedAnimation, StandardDestroyedDurationSeconds);
		HandleIntentChangeAnimationFinished();
		break;
	default:
		break;
	}
}

void UWacomBattleEnemyPartEntryWidget::RefreshInspectionInteraction()
{
	if (bIntroPending)
	{
		SetVisibility(ESlateVisibility::Hidden);
		return;
	}
	const FWacomBattleEnemyPartEntryViewData& CurrentView =
		PresentationState->GetRealView();
	const bool bCanInspect = bInspectionInteractionEnabled
		&& !PresentationState->HasActionPreview()
		&& PresentationState->HasRealView()
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
	const FWacomBattleEnemyPartEntryViewData& CurrentView =
		PresentationState->GetRealView();
	if (!bInspectionInteractionEnabled
		|| PresentationState->HasActionPreview()
		|| !CurrentView.Identity.IsValidSlot())
	{
		return;
	}
	OnInspectionRequestedNative.Broadcast(CurrentView.Identity);
}

void UWacomBattleEnemyPartEntryWidget::PlaySemanticAnimation(
	UWidgetAnimation* Animation,
	const float AuthoredDurationSeconds)
{
	if (!Animation || PresentationState->IsUsingSimplifiedMotion())
	{
		return;
	}
	const float SourceDurationSeconds = FMath::Max(0.001f, Animation->GetEndTime());
	const float PlaybackSpeed = SourceDurationSeconds
		/ FMath::Max(0.001f, AuthoredDurationSeconds);
	StopAnimation(Animation);
	PlayAnimation(
		Animation,
		0.0f,
		1,
		EUMGSequencePlayMode::Forward,
		PlaybackSpeed,
		false);
}

void UWacomBattleEnemyPartEntryWidget::HandleIntentChangeAnimationFinished()
{
	if (OutgoingIntentIcon)
	{
		OutgoingIntentIcon->SetVisibility(ESlateVisibility::Collapsed);
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
