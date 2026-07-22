// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleEnemyVitalsPresentation.h"

#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UI/Battle/WacomBattleEnemyPartEntryWidget.h"

#define LOCTEXT_NAMESPACE "WacomBattleEnemyVitalsPresentation"

namespace
{
	const FName EnemyHpCurrentPercentParameterName(TEXT("HpCurrentPercent"));
	const FName EnemyHpTrailStartPercentParameterName(TEXT("HpTrailStartPercent"));
	const FName EnemyHpPreviewPercentParameterName(TEXT("HpPreviewPercent"));
	const FName EnemyHpPreviewModeParameterName(TEXT("HpPreviewMode"));
	const FName EnemyDamageStartTimeParameterName(TEXT("DamageStartTime"));
	const FName EnemyDamageTrailHoldParameterName(TEXT("DamageTrailHoldSeconds"));
	const FName EnemyDamageTrailRecoveryParameterName(TEXT("DamageTrailRecoverySeconds"));
	const FName EnemyShieldVisibleParameterName(TEXT("ShieldVisible"));
	const FName EnemyShieldPreviewModeParameterName(TEXT("ShieldPreviewMode"));
	const FName EnemyShieldImpactStartTimeParameterName(TEXT("ShieldImpactStartTime"));
	const FName EnemySegmentRoleParameterName(TEXT("SegmentRole"));
	const FName EnemyDestroyedAmountParameterName(TEXT("DestroyedAmount"));
	const FName EnemyLowHealthAmountParameterName(TEXT("LowHealthAmount"));
	const FName EnemyFlashIntensityParameterName(TEXT("FlashIntensity"));
	const FName EnemyReducedMotionParameterName(TEXT("ReducedMotion"));

	constexpr float EnemyLowHealthThreshold = 0.25f;

	bool AreEnemyStatusStacksEquivalent(
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
}

FWacomBattleEnemyPartPresentationState::FWacomBattleEnemyPartPresentationState()
	: SegmentRole(EWacomBattleEnemySegmentRole::Single)
{
}

FWacomBattleEnemyPartPresentationUpdate
FWacomBattleEnemyPartPresentationState::SetRealView(
	const FWacomBattleEnemyPartEntryViewData& InView,
	const float WorldTimeSeconds)
{
	FWacomBattleEnemyPartPresentationUpdate Result;
	Result.bHadPreviousView = bHasRealView;
	if (bHasRealView)
	{
		Result.PreviousView = CurrentView;
		Result.bIntentChanged = CurrentView.CurrentIntentId != InView.CurrentIntentId;
	}

	if (!bHasRealView)
	{
		CurrentView = InView;
		bHasRealView = true;
		DamageTrailStartPercent = ResolveHpPercent(CurrentView);
		DamageStartTimeSeconds = -1000.0f;
		ShieldImpactStartTimeSeconds = -1000.0f;
		Result.Cues.Add(EWacomBattleEnemyMotionCue::Intro);
		return Result;
	}

	const FWacomBattleEnemyPartEntryViewData PreviousView = CurrentView;
	CurrentView = InView;
	if (!PreviousView.bDestroyed && InView.bDestroyed)
	{
		DamageTrailStartPercent = ResolveHpPercent(PreviousView);
		DamageStartTimeSeconds = WorldTimeSeconds;
		Result.Cues.Add(EWacomBattleEnemyMotionCue::Destroyed);
	}
	else if (InView.CurrentHp < PreviousView.CurrentHp)
	{
		DamageTrailStartPercent = ResolveHpPercent(PreviousView);
		DamageStartTimeSeconds = WorldTimeSeconds;
		Result.Cues.Add(EWacomBattleEnemyMotionCue::Damage);
	}
	else if (InView.CurrentHp > PreviousView.CurrentHp)
	{
		DamageTrailStartPercent = ResolveHpPercent(InView);
		DamageStartTimeSeconds = -1000.0f;
	}

	if (InView.Shield != PreviousView.Shield)
	{
		ShieldImpactStartTimeSeconds = WorldTimeSeconds;
		Result.Cues.Add(PreviousView.Shield > 0 && InView.Shield <= 0
			? EWacomBattleEnemyMotionCue::ShieldBreak
			: EWacomBattleEnemyMotionCue::ShieldImpact);
	}

	if (!InView.bDestroyed)
	{
		if (PreviousView.CurrentInitiative != InView.CurrentInitiative)
		{
			Result.Cues.Add(EWacomBattleEnemyMotionCue::InitiativeStep);
		}
		if (Result.bIntentChanged)
		{
			Result.Cues.Add(EWacomBattleEnemyMotionCue::IntentChange);
		}
	}
	return Result;
}

FWacomBattleEnemyPartPresentationUpdate
FWacomBattleEnemyPartPresentationState::SetActionPreview(
	const FWacomBattleEnemyPartEntryViewData& InPreviewView)
{
	FWacomBattleEnemyPartPresentationUpdate Result;
	if (bHasActionPreview && AreViewsEquivalent(ActionPreviewView, InPreviewView))
	{
		return Result;
	}
	const bool bWasContextActive = IsContextActive();
	ActionPreviewView = InPreviewView;
	bHasActionPreview = true;
	if (!bWasContextActive)
	{
		Result.Cues.Add(EWacomBattleEnemyMotionCue::ContextEnter);
	}
	return Result;
}

FWacomBattleEnemyPartPresentationUpdate
FWacomBattleEnemyPartPresentationState::ClearActionPreview()
{
	FWacomBattleEnemyPartPresentationUpdate Result;
	if (!bHasActionPreview)
	{
		return Result;
	}
	const bool bWasContextActive = IsContextActive();
	bHasActionPreview = false;
	if (bWasContextActive && !IsContextActive())
	{
		Result.Cues.Add(EWacomBattleEnemyMotionCue::ContextExit);
	}
	return Result;
}

FWacomBattleEnemyPartPresentationUpdate
FWacomBattleEnemyPartPresentationState::SetContextHighlighted(const bool bHighlighted)
{
	FWacomBattleEnemyPartPresentationUpdate Result;
	if (bContextHighlighted == bHighlighted)
	{
		return Result;
	}
	const bool bWasContextActive = IsContextActive();
	bContextHighlighted = bHighlighted;
	if (bWasContextActive != IsContextActive())
	{
		Result.Cues.Add(IsContextActive()
			? EWacomBattleEnemyMotionCue::ContextEnter
			: EWacomBattleEnemyMotionCue::ContextExit);
	}
	return Result;
}

void FWacomBattleEnemyPartPresentationState::SetSegmentLayout(
	const int32 PartIndex,
	const int32 PartCount)
{
	SegmentCount = FMath::Max(1, PartCount);
	SegmentIndex = FMath::Clamp(PartIndex, 0, SegmentCount - 1);
	SegmentRole = ResolveSegmentRole(SegmentIndex, SegmentCount);
}

FWacomBattleEnemyPartPresentationUpdate
FWacomBattleEnemyPartPresentationState::SetMotionPolicy(
	const bool bInSimplifiedMotion,
	const float InFlashIntensity)
{
	FWacomBattleEnemyPartPresentationUpdate Result;
	Result.bEnteredReducedMotion = bInSimplifiedMotion && !bSimplifiedMotion;
	bSimplifiedMotion = bInSimplifiedMotion;
	RuntimeFlashIntensity = FMath::Clamp(InFlashIntensity, 0.0f, 1.0f);
	if (Result.bEnteredReducedMotion)
	{
		DamageTrailStartPercent = ResolveHpPercent(CurrentView);
		DamageStartTimeSeconds = -1000.0f;
	}
	return Result;
}

FWacomBattleEnemyVitalsFrame
FWacomBattleEnemyPartPresentationState::BuildVitalsFrame(
	const float DamageTrailHoldSeconds,
	const float DamageTrailRecoverySeconds) const
{
	FWacomBattleEnemyVitalsFrame Frame;
	if (!bHasRealView)
	{
		return Frame;
	}
	const FWacomBattleEnemyPartEntryViewData& DisplayView = GetDisplayView();
	const float BaseHpPercent = ResolveHpPercent(CurrentView);
	const float DisplayHpPercent = ResolveHpPercent(DisplayView);
	Frame.HpCurrentPercent = BaseHpPercent;
	Frame.HpTrailStartPercent = FMath::Max(BaseHpPercent, DamageTrailStartPercent);
	Frame.HpPreviewPercent = DisplayHpPercent;
	Frame.HpPreviewMode = bHasActionPreview
		? (DisplayHpPercent < BaseHpPercent ? 1.0f
			: (DisplayHpPercent > BaseHpPercent ? 2.0f : 0.0f))
		: 0.0f;
	Frame.DamageStartTimeSeconds = DamageStartTimeSeconds;
	Frame.DamageTrailHoldSeconds = bSimplifiedMotion
		? 0.0f
		: FMath::Max(0.0f, DamageTrailHoldSeconds);
	Frame.DamageTrailRecoverySeconds = bSimplifiedMotion
		? 0.001f
		: FMath::Max(0.001f, DamageTrailRecoverySeconds);
	Frame.ShieldVisible = CurrentView.Shield > 0 || DisplayView.Shield > 0 ? 1.0f : 0.0f;
	Frame.ShieldPreviewMode = bHasActionPreview
		? (DisplayView.Shield < CurrentView.Shield ? 1.0f
			: (DisplayView.Shield > CurrentView.Shield ? 2.0f : 0.0f))
		: 0.0f;
	Frame.ShieldImpactStartTimeSeconds = ShieldImpactStartTimeSeconds;
	Frame.SegmentRole = static_cast<float>(SegmentRole);
	Frame.DestroyedAmount = DisplayView.bDestroyed ? 1.0f : 0.0f;
	Frame.LowHealthAmount = DisplayHpPercent > 0.0f
		? FMath::Clamp(
			(EnemyLowHealthThreshold - DisplayHpPercent) / EnemyLowHealthThreshold,
			0.0f,
			1.0f)
		: 0.0f;
	Frame.FlashIntensity = RuntimeFlashIntensity;
	Frame.ReducedMotion = bSimplifiedMotion ? 1.0f : 0.0f;
	return Frame;
}

FWacomBattleEnemyActionPreviewFrame
FWacomBattleEnemyPartPresentationState::BuildActionPreviewFrame() const
{
	FWacomBattleEnemyActionPreviewFrame Frame;
	if (!bHasActionPreview)
	{
		return Frame;
	}

	const FWacomBattleEnemyPartEntryViewData& View = ActionPreviewView;
	Frame.bActive = true;
	Frame.bPerfectRelease = View.bActionPreviewPerfectReleaseCandidate;
	Frame.bShowResistanceComparison = View.bHasResistancePreview;
	Frame.bWillAct = View.bActionPreviewWillAct;
	Frame.bWillSkipActionDueToStun = View.bActionPreviewWillSkipActionDueToStun;
	Frame.PlayerPeakDamage = View.ResistancePreviewPlayerPeakDamage;
	Frame.EnemyPeakDamage = View.ResistancePreviewEnemyPeakDamage;
	if (Frame.bShowResistanceComparison)
	{
		Frame.ResistanceOutcome = View.bResistancePreviewWillStun
			? EWacomBattleEnemyResistancePreviewOutcome::Success
			: EWacomBattleEnemyResistancePreviewOutcome::Failure;
		Frame.ComparatorText = View.bResistancePreviewWillStun
			? LOCTEXT("ResistanceGreaterThan", ">")
			: LOCTEXT("ResistanceLessThanOrEqual", "≤");
	}
	return Frame;
}

void FWacomBattleEnemyPartPresentationState::ResetTransientPresentation()
{
	bHasActionPreview = false;
	bContextHighlighted = false;
}

const FWacomBattleEnemyPartEntryViewData&
FWacomBattleEnemyPartPresentationState::GetDisplayView() const
{
	return bHasActionPreview ? ActionPreviewView : CurrentView;
}

bool FWacomBattleEnemyPartPresentationState::AreViewsEquivalent(
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
		&& Left.bCurrentIntentIsAttack == Right.bCurrentIntentIsAttack
		&& Left.CurrentIntentPeakAttackDamage == Right.CurrentIntentPeakAttackDamage
		&& Left.RuntimeStatuses.Num() == Right.RuntimeStatuses.Num()
		&& Left.RuntimeStatuses.HasAllExact(Right.RuntimeStatuses)
		&& Right.RuntimeStatuses.HasAllExact(Left.RuntimeStatuses)
		&& AreEnemyStatusStacksEquivalent(Left.RuntimeStatusStacks, Right.RuntimeStatusStacks)
		&& Left.bDestroyed == Right.bDestroyed
		&& Left.bActionPreviewWillAct == Right.bActionPreviewWillAct
		&& Left.bActionPreviewWillSkipActionDueToStun == Right.bActionPreviewWillSkipActionDueToStun
		&& Left.bActionPreviewPerfectReleaseCandidate == Right.bActionPreviewPerfectReleaseCandidate
		&& Left.bHasResistancePreview == Right.bHasResistancePreview
		&& Left.ResistancePreviewPlayerPeakDamage == Right.ResistancePreviewPlayerPeakDamage
		&& Left.ResistancePreviewEnemyPeakDamage == Right.ResistancePreviewEnemyPeakDamage
		&& Left.bResistancePreviewWillStun == Right.bResistancePreviewWillStun;
}

float FWacomBattleEnemyPartPresentationState::ResolveHpPercent(
	const FWacomBattleEnemyPartEntryViewData& View)
{
	return View.MaxHp > 0
		? FMath::Clamp(
			static_cast<float>(View.CurrentHp) / static_cast<float>(View.MaxHp),
			0.0f,
			1.0f)
		: 0.0f;
}

EWacomBattleEnemySegmentRole
FWacomBattleEnemyPartPresentationState::ResolveSegmentRole(
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
	return PartIndex >= PartCount - 1
		? EWacomBattleEnemySegmentRole::Last
		: EWacomBattleEnemySegmentRole::Middle;
}

bool FWacomBattleEnemyVitalsMaterialAdapter::Initialize(UImage* InVitalsImage)
{
	if (!InVitalsImage)
	{
		return false;
	}
	if (VitalsImage.Get() != InVitalsImage)
	{
		RestoreAuthoredBrush();
		VitalsImage = InVitalsImage;
	}
	if (MaterialInstance)
	{
		return true;
	}

	UMaterialInterface* SourceMaterial = Cast<UMaterialInterface>(
		InVitalsImage->GetBrush().GetResourceObject());
	if (!SourceMaterial)
	{
		return false;
	}
	if (UMaterialInstanceDynamic* Existing = Cast<UMaterialInstanceDynamic>(SourceMaterial))
	{
		MaterialInstance = Existing;
		return true;
	}
	AuthoredMaterial = SourceMaterial;
	MaterialInstance = InVitalsImage->GetDynamicMaterial();
	return MaterialInstance != nullptr;
}

void FWacomBattleEnemyVitalsMaterialAdapter::ApplyFrame(
	const FWacomBattleEnemyVitalsFrame& Frame)
{
	if (!MaterialInstance)
	{
		return;
	}
	MaterialInstance->SetScalarParameterValue(EnemyHpCurrentPercentParameterName, Frame.HpCurrentPercent);
	MaterialInstance->SetScalarParameterValue(EnemyHpTrailStartPercentParameterName, Frame.HpTrailStartPercent);
	MaterialInstance->SetScalarParameterValue(EnemyHpPreviewPercentParameterName, Frame.HpPreviewPercent);
	MaterialInstance->SetScalarParameterValue(EnemyHpPreviewModeParameterName, Frame.HpPreviewMode);
	MaterialInstance->SetScalarParameterValue(EnemyDamageStartTimeParameterName, Frame.DamageStartTimeSeconds);
	MaterialInstance->SetScalarParameterValue(EnemyDamageTrailHoldParameterName, Frame.DamageTrailHoldSeconds);
	MaterialInstance->SetScalarParameterValue(EnemyDamageTrailRecoveryParameterName, Frame.DamageTrailRecoverySeconds);
	MaterialInstance->SetScalarParameterValue(EnemyShieldVisibleParameterName, Frame.ShieldVisible);
	MaterialInstance->SetScalarParameterValue(EnemyShieldPreviewModeParameterName, Frame.ShieldPreviewMode);
	MaterialInstance->SetScalarParameterValue(EnemyShieldImpactStartTimeParameterName, Frame.ShieldImpactStartTimeSeconds);
	MaterialInstance->SetScalarParameterValue(EnemySegmentRoleParameterName, Frame.SegmentRole);
	MaterialInstance->SetScalarParameterValue(EnemyDestroyedAmountParameterName, Frame.DestroyedAmount);
	MaterialInstance->SetScalarParameterValue(EnemyLowHealthAmountParameterName, Frame.LowHealthAmount);
	MaterialInstance->SetScalarParameterValue(EnemyFlashIntensityParameterName, Frame.FlashIntensity);
	MaterialInstance->SetScalarParameterValue(EnemyReducedMotionParameterName, Frame.ReducedMotion);
}

void FWacomBattleEnemyVitalsMaterialAdapter::RestoreAuthoredBrush()
{
	if (UImage* Image = VitalsImage.Get(); Image && AuthoredMaterial)
	{
		Image->SetBrushFromMaterial(AuthoredMaterial);
	}
	VitalsImage.Reset();
	MaterialInstance = nullptr;
	AuthoredMaterial = nullptr;
}

void FWacomBattleEnemyVitalsMaterialAdapter::AddReferencedObjects(
	FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(MaterialInstance);
	Collector.AddReferencedObject(AuthoredMaterial);
}

FString FWacomBattleEnemyVitalsMaterialAdapter::GetReferencerName() const
{
	return TEXT("FWacomBattleEnemyVitalsMaterialAdapter");
}

#undef LOCTEXT_NAMESPACE
