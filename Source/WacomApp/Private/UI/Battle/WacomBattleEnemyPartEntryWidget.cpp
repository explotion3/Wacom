// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleEnemyPartEntryWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Settings/WacomLocalSettingsTypes.h"
#include "Settings/WacomSettingsSubsystem.h"
#include "UI/Battle/WacomBattleStatusIconWidget.h"
#include "UI/Battle/WacomBattleEnemyIntentPresentationStyle.h"

namespace
{
	const FName HpCurrentPercentParameterName(TEXT("HpCurrentPercent"));
	const FName HpTrailStartPercentParameterName(TEXT("HpTrailStartPercent"));
	const FName HpPreviewPercentParameterName(TEXT("HpPreviewPercent"));
	const FName HpPreviewModeParameterName(TEXT("HpPreviewMode"));
	const FName DamageStartTimeParameterName(TEXT("DamageStartTime"));
	const FName DamageTrailHoldParameterName(TEXT("DamageTrailHoldSeconds"));
	const FName DamageTrailRecoveryParameterName(TEXT("DamageTrailRecoverySeconds"));
	const FName ShieldVisibleParameterName(TEXT("ShieldVisible"));
	const FName ShieldPreviewModeParameterName(TEXT("ShieldPreviewMode"));
	const FName ShieldImpactStartTimeParameterName(TEXT("ShieldImpactStartTime"));
	const FName SegmentRoleParameterName(TEXT("SegmentRole"));
	const FName DestroyedAmountParameterName(TEXT("DestroyedAmount"));
	const FName LowHealthAmountParameterName(TEXT("LowHealthAmount"));
	const FName FlashIntensityParameterName(TEXT("FlashIntensity"));
	const FName ReducedMotionParameterName(TEXT("ReducedMotion"));

	constexpr float LowHealthThreshold = 0.25f;
	constexpr float StandardDamageDurationSeconds = 0.22f;
	constexpr float StandardShieldDurationSeconds = 0.18f;
	constexpr float StandardShieldBreakDurationSeconds = 0.24f;
	constexpr float StandardInitiativeDurationSeconds = 0.12f;
	constexpr float StandardIntentDurationSeconds = 0.18f;
	constexpr float StandardDestroyedDurationSeconds = 0.30f;
	constexpr float StandardIntroDurationSeconds = 0.22f;

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
	if (bHadViewData && PreviousView.CurrentIntentId != InView.CurrentIntentId)
	{
		CaptureOutgoingIntent(PreviousView);
	}

	CurrentView = InView;
	bHasReceivedViewData = true;
	if (!bHadViewData)
	{
		DamageTrailStartPercent = ResolveHpPercent(CurrentView);
		DamageStartTimeSeconds = -1000.0f;
		ShieldImpactStartTimeSeconds = -1000.0f;
	}
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

void UWacomBattleEnemyPartEntryWidget::SetSegmentLayout(
	const int32 InPartIndex,
	const int32 InPartCount)
{
	const int32 NewCount = FMath::Max(1, InPartCount);
	const int32 NewIndex = FMath::Clamp(InPartIndex, 0, NewCount - 1);
	const EWacomBattleEnemySegmentRole NewRole = ResolveSegmentRole(NewIndex, NewCount);
	if (SegmentIndex == NewIndex && SegmentCount == NewCount && SegmentRole == NewRole)
	{
		return;
	}

	SegmentIndex = NewIndex;
	SegmentCount = NewCount;
	SegmentRole = NewRole;
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
	if (IntentChangeAnimation)
	{
		UnbindAllFromAnimationFinished(IntentChangeAnimation);
		FWidgetAnimationDynamicEvent FinishedEvent;
		FinishedEvent.BindDynamic(this, &ThisClass::HandleIntentChangeAnimationFinished);
		BindToAnimationFinished(IntentChangeAnimation, FinishedEvent);
	}
	BindRuntimeSettings();
	if (PartEntryRoot)
	{
		PartEntryRoot->SetMinDesiredWidth(116.0f);
		PartEntryRoot->SetHeightOverride(92.0f);
	}
	EnsureVitalsMaterial();
	RefreshPresentation();
	RefreshInspectionInteraction();
}

void UWacomBattleEnemyPartEntryWidget::NativeDestruct()
{
	UnbindRuntimeSettings();
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
	RestoreVitalsMaterial();
	Super::NativeDestruct();
}

void UWacomBattleEnemyPartEntryWidget::RefreshPresentation()
{
	const FWacomBattleEnemyPartEntryViewData& View = GetEffectivePartEntryViewData();

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
		ContextSurface->SetVisibility((bContextHighlighted || bHasActionPreview)
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

bool UWacomBattleEnemyPartEntryWidget::EnsureVitalsMaterial()
{
	if (!VitalsTrackImage)
	{
		return false;
	}
	if (VitalsMaterialInstance)
	{
		return true;
	}

	UMaterialInterface* Source = Cast<UMaterialInterface>(
		VitalsTrackImage->GetBrush().GetResourceObject());
	if (!Source)
	{
		return false;
	}
	if (UMaterialInstanceDynamic* Existing = Cast<UMaterialInstanceDynamic>(Source))
	{
		VitalsMaterialInstance = Existing;
		return true;
	}

	VitalsSourceMaterial = Source;
	VitalsMaterialInstance = VitalsTrackImage->GetDynamicMaterial();
	return VitalsMaterialInstance != nullptr;
}

void UWacomBattleEnemyPartEntryWidget::ApplyVitalsMaterialPresentation()
{
	if (!bHasReceivedViewData || !EnsureVitalsMaterial())
	{
		return;
	}

	const FWacomBattleEnemyPartEntryViewData& DisplayView = GetEffectivePartEntryViewData();
	const float BaseHpPercent = ResolveHpPercent(CurrentView);
	const float DisplayHpPercent = ResolveHpPercent(DisplayView);
	const float PreviewMode = bHasActionPreview
		? (DisplayHpPercent < BaseHpPercent ? 1.0f
			: (DisplayHpPercent > BaseHpPercent ? 2.0f : 0.0f))
		: 0.0f;
	const float ShieldPreviewMode = bHasActionPreview
		? (DisplayView.Shield < CurrentView.Shield ? 1.0f
			: (DisplayView.Shield > CurrentView.Shield ? 2.0f : 0.0f))
		: 0.0f;
	const bool bShieldVisible = CurrentView.Shield > 0 || DisplayView.Shield > 0;
	const float LowHealthAmount = DisplayHpPercent > 0.0f
		? FMath::Clamp((LowHealthThreshold - DisplayHpPercent) / LowHealthThreshold, 0.0f, 1.0f)
		: 0.0f;
	const float RecoverySeconds = bRuntimeSimplifiedMotion
		? 0.001f
		: FMath::Max(0.001f, DamageTrailRecoverySeconds);
	const float HoldSeconds = bRuntimeSimplifiedMotion
		? 0.0f
		: FMath::Max(0.0f, DamageTrailHoldSeconds);

	VitalsMaterialInstance->SetScalarParameterValue(
		HpCurrentPercentParameterName, BaseHpPercent);
	VitalsMaterialInstance->SetScalarParameterValue(
		HpTrailStartPercentParameterName, FMath::Max(BaseHpPercent, DamageTrailStartPercent));
	VitalsMaterialInstance->SetScalarParameterValue(
		HpPreviewPercentParameterName, DisplayHpPercent);
	VitalsMaterialInstance->SetScalarParameterValue(HpPreviewModeParameterName, PreviewMode);
	VitalsMaterialInstance->SetScalarParameterValue(
		DamageStartTimeParameterName, DamageStartTimeSeconds);
	VitalsMaterialInstance->SetScalarParameterValue(DamageTrailHoldParameterName, HoldSeconds);
	VitalsMaterialInstance->SetScalarParameterValue(DamageTrailRecoveryParameterName, RecoverySeconds);
	VitalsMaterialInstance->SetScalarParameterValue(
		ShieldVisibleParameterName, bShieldVisible ? 1.0f : 0.0f);
	VitalsMaterialInstance->SetScalarParameterValue(
		ShieldPreviewModeParameterName, ShieldPreviewMode);
	VitalsMaterialInstance->SetScalarParameterValue(
		ShieldImpactStartTimeParameterName, ShieldImpactStartTimeSeconds);
	VitalsMaterialInstance->SetScalarParameterValue(
		SegmentRoleParameterName, static_cast<float>(SegmentRole));
	VitalsMaterialInstance->SetScalarParameterValue(
		DestroyedAmountParameterName, DisplayView.bDestroyed ? 1.0f : 0.0f);
	VitalsMaterialInstance->SetScalarParameterValue(
		LowHealthAmountParameterName, LowHealthAmount);
	VitalsMaterialInstance->SetScalarParameterValue(
		FlashIntensityParameterName, RuntimeFlashIntensity);
	VitalsMaterialInstance->SetScalarParameterValue(
		ReducedMotionParameterName, bRuntimeSimplifiedMotion ? 1.0f : 0.0f);
}

void UWacomBattleEnemyPartEntryWidget::RestoreVitalsMaterial()
{
	if (VitalsTrackImage && VitalsSourceMaterial)
	{
		VitalsTrackImage->SetBrushFromMaterial(VitalsSourceMaterial);
	}
	VitalsMaterialInstance = nullptr;
	VitalsSourceMaterial = nullptr;
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

float UWacomBattleEnemyPartEntryWidget::ResolveHpPercent(
	const FWacomBattleEnemyPartEntryViewData& View)
{
	return View.MaxHp > 0
		? FMath::Clamp(
			static_cast<float>(View.CurrentHp) / static_cast<float>(View.MaxHp),
			0.0f,
			1.0f)
		: 0.0f;
}

EWacomBattleEnemySegmentRole UWacomBattleEnemyPartEntryWidget::ResolveSegmentRole(
	const int32 PartIndex,
	const int32 PartCount)
{
	if (PartCount <= 1)
	{
		return EWacomBattleEnemySegmentRole::Single;
	}
	if (PartIndex <= 0)
	{
		return EWacomBattleEnemySegmentRole::First;
	}
	if (PartIndex >= PartCount - 1)
	{
		return EWacomBattleEnemySegmentRole::Last;
	}
	return EWacomBattleEnemySegmentRole::Middle;
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
	if (!PreviousView.bDestroyed && NewView.bDestroyed)
	{
		DamageTrailStartPercent = ResolveHpPercent(PreviousView);
		DamageStartTimeSeconds = ResolveWorldTimeSeconds();
		PlaySemanticAnimation(DestroyedAnimation, StandardDestroyedDurationSeconds);
	}
	else if (NewView.CurrentHp < PreviousView.CurrentHp)
	{
		DamageTrailStartPercent = ResolveHpPercent(PreviousView);
		DamageStartTimeSeconds = ResolveWorldTimeSeconds();
		PlaySemanticAnimation(DamageImpactAnimation, StandardDamageDurationSeconds);
	}
	else if (NewView.CurrentHp > PreviousView.CurrentHp)
	{
		DamageTrailStartPercent = ResolveHpPercent(NewView);
		DamageStartTimeSeconds = -1000.0f;
	}

	if (NewView.Shield != PreviousView.Shield)
	{
		ShieldImpactStartTimeSeconds = ResolveWorldTimeSeconds();
		const bool bShieldBroken = PreviousView.Shield > 0 && NewView.Shield <= 0;
		PlaySemanticAnimation(
			bShieldBroken ? ShieldBreakAnimation.Get() : ShieldImpactAnimation.Get(),
			bShieldBroken
				? StandardShieldBreakDurationSeconds
				: StandardShieldDurationSeconds);
	}
	ApplyVitalsMaterialPresentation();

	if (NewView.bDestroyed)
	{
		if (OutgoingIntentIcon)
		{
			OutgoingIntentIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}

	if (PreviousView.CurrentInitiative != NewView.CurrentInitiative)
	{
		PlaySemanticAnimation(InitiativeStepAnimation, StandardInitiativeDurationSeconds);
	}
	if (PreviousView.CurrentIntentId != NewView.CurrentIntentId)
	{
		UWidgetAnimation* Animation = IntentChangeAnimation;
		PlaySemanticAnimation(Animation, StandardIntentDurationSeconds);
		if (!Animation || bRuntimeSimplifiedMotion)
		{
			HandleIntentChangeAnimationFinished();
		}
	}
}

void UWacomBattleEnemyPartEntryWidget::RefreshContextPresentation(
	const bool bPreviousContextActive)
{
	const bool bContextActive = bContextHighlighted || bHasActionPreview;
	UWidgetAnimation* ResolvedAnimation = ContextAnimation;
	if (!ResolvedAnimation || bRuntimeSimplifiedMotion
		|| bContextActive == bPreviousContextActive)
	{
		return;
	}

	if (bContextActive)
	{
		PlayAnimationForward(ResolvedAnimation);
	}
	else
	{
		PlayAnimationReverse(ResolvedAnimation);
	}
}

void UWacomBattleEnemyPartEntryWidget::PlaySemanticAnimation(
	UWidgetAnimation* Animation,
	const float AuthoredDurationSeconds)
{
	if (!Animation || bRuntimeSimplifiedMotion)
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

void UWacomBattleEnemyPartEntryWidget::BindRuntimeSettings()
{
	UWacomSettingsSubsystem* Settings = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UWacomSettingsSubsystem>()
		: nullptr;
	if (BoundSettingsSubsystem.Get() == Settings && RuntimeSettingsChangedHandle.IsValid())
	{
		HandleRuntimeSettingsChanged(
			Settings->GetCurrentSnapshot(), EWacomRuntimeSettingsChangeReason::Startup);
		return;
	}

	UnbindRuntimeSettings();
	if (!Settings)
	{
		return;
	}
	BoundSettingsSubsystem = Settings;
	RuntimeSettingsChangedHandle = Settings->OnRuntimeSettingsChangedNative().AddUObject(
		this, &ThisClass::HandleRuntimeSettingsChanged);
	HandleRuntimeSettingsChanged(
		Settings->GetCurrentSnapshot(), EWacomRuntimeSettingsChangeReason::Startup);
}

void UWacomBattleEnemyPartEntryWidget::UnbindRuntimeSettings()
{
	if (UWacomSettingsSubsystem* Settings = BoundSettingsSubsystem.Get())
	{
		if (RuntimeSettingsChangedHandle.IsValid())
		{
			Settings->OnRuntimeSettingsChangedNative().Remove(RuntimeSettingsChangedHandle);
		}
	}
	BoundSettingsSubsystem.Reset();
	RuntimeSettingsChangedHandle.Reset();
}

void UWacomBattleEnemyPartEntryWidget::HandleRuntimeSettingsChanged(
	const FWacomLocalSettingsSnapshot& Snapshot,
	EWacomRuntimeSettingsChangeReason /*Reason*/)
{
	const bool bNextSimplified = Snapshot.UIMotionMode == EWacomUIMotionMode::Simplified;
	if (bNextSimplified && !bRuntimeSimplifiedMotion)
	{
		StopAllAnimations();
		HandleIntentChangeAnimationFinished();
		DamageTrailStartPercent = ResolveHpPercent(CurrentView);
		DamageStartTimeSeconds = -1000.0f;
	}
	bRuntimeSimplifiedMotion = bNextSimplified;
	switch (Snapshot.FlashEffectMode)
	{
	case EWacomFlashEffectMode::Reduced:
		RuntimeFlashIntensity = 0.35f;
		break;
	case EWacomFlashEffectMode::Off:
		RuntimeFlashIntensity = 0.0f;
		break;
	default:
		RuntimeFlashIntensity = 1.0f;
		break;
	}
	ApplyVitalsMaterialPresentation();
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
