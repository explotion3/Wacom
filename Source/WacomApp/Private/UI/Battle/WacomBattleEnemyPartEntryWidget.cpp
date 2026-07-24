// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleEnemyPartEntryWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/World.h"
#include "UI/Battle/WacomBattleEnemyIntentPresentation.h"
#include "UI/Battle/WacomBattleEnemyIntentPresentationStyle.h"
#include "UI/Battle/WacomBattleEnemyVitalsPresentation.h"
#include "UI/Battle/WacomBattleIntentTooltipWidget.h"
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
	IntentTooltipWidgetClass = UWacomBattleIntentTooltipWidget::StaticClass();
}

UWacomBattleEnemyPartEntryWidget::~UWacomBattleEnemyPartEntryWidget() = default;

void UWacomBattleEnemyPartEntryWidget::SetIntentTooltipWidgetClass(
	TSubclassOf<UWacomBattleIntentTooltipWidget> InClass)
{
	IntentTooltipWidgetClass = InClass
		? InClass
		: TSubclassOf<UWacomBattleIntentTooltipWidget>(
			UWacomBattleIntentTooltipWidget::StaticClass());
	CachedIntentTooltipWidget = nullptr;
}

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
	if (IntentIcon)
	{
		AuthoredIntentIconTint = IntentIcon->GetColorAndOpacity();
	}
	EnsureIntentTooltipBinding();
	ResetActionPreviewPresentation();
	if (InspectHitTarget)
	{
		InspectHitTarget->OnClicked.RemoveAll(this);
		InspectHitTarget->OnClicked.AddDynamic(this, &ThisClass::HandleInspectClicked);
	}
	if (IntentTooltipTarget)
	{
		IntentTooltipTarget->OnClicked.RemoveAll(this);
		IntentTooltipTarget->OnClicked.AddDynamic(
			this, &ThisClass::HandleInspectClicked);
		IntentTooltipTarget->SynchronizeProperties();
	}
	if (StatusList)
	{
		StatusList->SetMaxVisibleStatuses(3);
		StatusList->SetInspectionHost(
			EWacomBattleStatusInspectionHost::EnemyPart);
		StatusList->SetStatusIconActivationEnabled(true);
		StatusList->OnStatusIconActivatedNative.RemoveAll(this);
		StatusList->OnStatusIconActivatedNative.AddUObject(
			this,
			&ThisClass::HandleStatusIconActivated);
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
	if (IntentTooltipTarget)
	{
		IntentTooltipTarget->OnClicked.RemoveAll(this);
		IntentTooltipTarget->ToolTipWidgetDelegate.Unbind();
	}
	CachedIntentTooltipWidget = nullptr;
	if (IntentChangeAnimation)
	{
		UnbindAllFromAnimationFinished(IntentChangeAnimation);
	}
	if (StatusList)
	{
		StatusList->OnStatusIconActivatedNative.RemoveAll(this);
		StatusList->SetStatusInspectionEnabled(false);
	}
	OnInspectionRequestedNative.Clear();
	CancelPendingPresentation();
	ResetActionPreviewPresentation();
	PresentationState->ResetTransientPresentation();
	VitalsMaterialAdapter->RestoreAuthoredBrush();
	Super::NativeDestruct();
}

void UWacomBattleEnemyPartEntryWidget::SynchronizeProperties()
{
	EnsureIntentTooltipBinding();
	Super::SynchronizeProperties();
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
	RefreshIntentTooltipState();
	if (StatusList)
	{
		StatusList->SetMaxVisibleStatuses(3);
		StatusList->SetStatuses(View.RuntimeStatuses, View.RuntimeStatusStacks);
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
	RefreshActionPreviewPresentation();
	SetRenderOpacity(1.0f);
	ApplyVitalsMaterialPresentation();
	RefreshInspectionInteraction();
}

void UWacomBattleEnemyPartEntryWidget::RefreshActionPreviewPresentation()
{
	const FWacomBattleEnemyActionPreviewFrame Frame =
		PresentationState->BuildActionPreviewFrame();
	if (!Frame.bActive)
	{
		ResetActionPreviewPresentation();
		return;
	}

	if (PerfectReleaseSurface)
	{
		PerfectReleaseSurface->SetVisibility(Frame.bPerfectRelease
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	if (ActionPreviewComparisonRoot)
	{
		ActionPreviewComparisonRoot->SetVisibility(Frame.bShowResistanceComparison
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	if (InitiativeSocket)
	{
		InitiativeSocket->SetVisibility(Frame.bShowResistanceComparison
			? ESlateVisibility::Collapsed
			: ESlateVisibility::HitTestInvisible);
	}
	if (IntentSocket)
	{
		IntentSocket->SetVisibility(Frame.bShowResistanceComparison
			? ESlateVisibility::Collapsed
			: ESlateVisibility::SelfHitTestInvisible);
	}
	if (PreviewSkipMark)
	{
		PreviewSkipMark->SetVisibility(Frame.bWillSkipActionDueToStun
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	if (!Frame.bShowResistanceComparison)
	{
		if (IntentIcon)
		{
			FLinearColor IntentTint = AuthoredIntentIconTint;
			if (Frame.bWillSkipActionDueToStun)
			{
				IntentTint.A *= FMath::Clamp(ActionPreviewSkippedIntentOpacity, 0.0f, 1.0f);
			}
			else if (Frame.bWillAct)
			{
				IntentTint = ActionPreviewRiskIntentTint;
			}
			IntentIcon->SetColorAndOpacity(IntentTint);
		}
		return;
	}

	const bool bResistanceSuccess = Frame.ResistanceOutcome
		== EWacomBattleEnemyResistancePreviewOutcome::Success;
	const FLinearColor OutcomeTint = bResistanceSuccess
		? ResistancePreviewSuccessTint
		: ResistancePreviewFailureTint;
	if (PreviewPlayerDamageIcon)
	{
		PreviewPlayerDamageIcon->SetColorAndOpacity(OutcomeTint);
	}
	if (PreviewPlayerDamageText)
	{
		PreviewPlayerDamageText->SetText(FText::AsNumber(Frame.PlayerPeakDamage));
		PreviewPlayerDamageText->SetColorAndOpacity(FSlateColor(OutcomeTint));
	}
	if (PreviewComparatorText)
	{
		PreviewComparatorText->SetText(Frame.ComparatorText);
		PreviewComparatorText->SetColorAndOpacity(FSlateColor(OutcomeTint));
	}
	if (PreviewEnemyAttackText)
	{
		PreviewEnemyAttackText->SetText(FText::AsNumber(Frame.EnemyPeakDamage));
		PreviewEnemyAttackText->SetColorAndOpacity(FSlateColor(OutcomeTint));
	}
	if (PreviewEnemyIntentIcon)
	{
		if (IntentPresentationStyle)
		{
			if (const FSlateBrush* IntentBrush =
				IntentPresentationStyle->ResolveIntentIcon(
					PresentationState->GetDisplayView().CurrentIntentId))
			{
				PreviewEnemyIntentIcon->SetBrush(*IntentBrush);
			}
		}
		FLinearColor IntentTint = OutcomeTint;
		if (Frame.bWillSkipActionDueToStun)
		{
			IntentTint.A *= FMath::Clamp(ActionPreviewSkippedIntentOpacity, 0.0f, 1.0f);
		}
		PreviewEnemyIntentIcon->SetColorAndOpacity(IntentTint);
	}
}

void UWacomBattleEnemyPartEntryWidget::ResetActionPreviewPresentation()
{
	if (PerfectReleaseSurface)
	{
		PerfectReleaseSurface->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (ActionPreviewComparisonRoot)
	{
		ActionPreviewComparisonRoot->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (PreviewSkipMark)
	{
		PreviewSkipMark->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (InitiativeSocket)
	{
		InitiativeSocket->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (IntentSocket)
	{
		IntentSocket->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (IntentIcon)
	{
		IntentIcon->SetColorAndOpacity(AuthoredIntentIconTint);
	}
	if (PreviewPlayerDamageIcon)
	{
		PreviewPlayerDamageIcon->SetColorAndOpacity(FLinearColor::White);
	}
	if (PreviewEnemyIntentIcon)
	{
		PreviewEnemyIntentIcon->SetColorAndOpacity(FLinearColor::White);
	}
	if (PreviewPlayerDamageText)
	{
		PreviewPlayerDamageText->SetText(FText::GetEmpty());
	}
	if (PreviewComparatorText)
	{
		PreviewComparatorText->SetText(FText::GetEmpty());
	}
	if (PreviewEnemyAttackText)
	{
		PreviewEnemyAttackText->SetText(FText::GetEmpty());
	}
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
	if (StatusList)
	{
		StatusList->SetStatusInspectionEnabled(bCanInspect);
		StatusList->SetStatusIconActivationEnabled(bCanInspect);
	}
	RefreshIntentTooltipState();
}

void UWacomBattleEnemyPartEntryWidget::EnsureIntentTooltipBinding()
{
	if (IntentTooltipTarget
		&& !IntentTooltipTarget->ToolTipWidgetDelegate.IsBound())
	{
		IntentTooltipTarget->ToolTipWidgetDelegate.BindDynamic(
			this,
			&ThisClass::HandleBuildIntentTooltipWidget);
	}
}

void UWacomBattleEnemyPartEntryWidget::RefreshIntentTooltipState()
{
	if (!IntentTooltipTarget || !PresentationState->HasRealView())
	{
		return;
	}
	const FWacomBattleEnemyPartEntryViewData& View =
		PresentationState->GetRealView();
	const bool bCanShow = bInspectionInteractionEnabled
		&& !PresentationState->HasActionPreview()
		&& View.Identity.IsValidSlot()
		&& !View.bDestroyed
		&& !View.CurrentIntentId.IsNone();
	IntentTooltipTarget->SetIsEnabled(bCanShow);
	IntentTooltipTarget->SetVisibility(bCanShow
		? ESlateVisibility::Visible
		: ESlateVisibility::HitTestInvisible);
	if (CachedIntentTooltipWidget)
	{
		CachedIntentTooltipWidget->SetIntentViewData(
			bCanShow
				? FWacomBattleIntentPresentationBuilder::Build(
					View, IntentPresentationStyle, 5)
				: FWacomBattleIntentPresentationViewData());
	}
}

UWidget* UWacomBattleEnemyPartEntryWidget::HandleBuildIntentTooltipWidget()
{
	if (!bInspectionInteractionEnabled
		|| PresentationState->HasActionPreview()
		|| !PresentationState->HasRealView())
	{
		return nullptr;
	}
	const FWacomBattleEnemyPartEntryViewData& View =
		PresentationState->GetRealView();
	if (!View.Identity.IsValidSlot()
		|| View.bDestroyed
		|| View.CurrentIntentId.IsNone())
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
				View, IntentPresentationStyle, 5));
	}
	return CachedIntentTooltipWidget;
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

void UWacomBattleEnemyPartEntryWidget::HandleStatusIconActivated(
	const FWacomBattleStatusIconView& /*View*/)
{
	HandleInspectClicked();
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
