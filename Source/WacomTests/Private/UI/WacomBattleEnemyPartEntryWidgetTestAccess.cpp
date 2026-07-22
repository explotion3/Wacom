// Copyright Wacom. All Rights Reserved.

#include "UI/WacomBattleEnemyPartEntryWidgetTestAccess.h"

#include "../../../WacomApp/Private/UI/Battle/WacomBattleEnemyVitalsPresentation.h"
#include "Materials/MaterialInstanceDynamic.h"

void FWacomBattleEnemyPartEntryWidgetTestAccess::SetView(
	UWacomBattleEnemyPartEntryWidget& Widget,
	const FWacomBattleEnemyPartEntryViewData& View)
{
	Widget.SetPartEntryViewData(View);
}

void FWacomBattleEnemyPartEntryWidgetTestAccess::SetPreview(
	UWacomBattleEnemyPartEntryWidget& Widget,
	const FWacomBattleEnemyPartEntryViewData& View)
{
	Widget.SetActionPreview(View);
}

void FWacomBattleEnemyPartEntryWidgetTestAccess::ClearPreview(
	UWacomBattleEnemyPartEntryWidget& Widget)
{
	Widget.ClearActionPreview();
}

bool FWacomBattleEnemyPartEntryWidgetTestAccess::HasPreview(
	const UWacomBattleEnemyPartEntryWidget& Widget)
{
	return Widget.HasActionPreview();
}

const FWacomBattleEnemyPartEntryViewData&
FWacomBattleEnemyPartEntryWidgetTestAccess::GetCurrentView(
	const UWacomBattleEnemyPartEntryWidget& Widget)
{
	return Widget.GetPartEntryViewData();
}

void FWacomBattleEnemyPartEntryWidgetTestAccess::CancelPresentation(
	UWacomBattleEnemyPartEntryWidget& Widget)
{
	Widget.CancelPendingPresentation();
}

bool FWacomBattleEnemyPartEntryWidgetTestAccess::IsInspectionInteractionEnabled(
	const UWacomBattleEnemyPartEntryWidget& Widget)
{
	return Widget.IsInspectionInteractionEnabled();
}

EWacomBattleEnemySegmentRole
FWacomBattleEnemyPartEntryWidgetTestAccess::GetSegmentRole(
	const UWacomBattleEnemyPartEntryWidget& Widget)
{
	return Widget.PresentationState->GetSegmentRole();
}

int32 FWacomBattleEnemyPartEntryWidgetTestAccess::GetSegmentCount(
	const UWacomBattleEnemyPartEntryWidget& Widget)
{
	return Widget.PresentationState->GetSegmentCount();
}

float FWacomBattleEnemyPartEntryWidgetTestAccess::GetMaterialScalar(
	const UWacomBattleEnemyPartEntryWidget& Widget,
	const FName ParameterName)
{
	float Value = 0.0f;
	UMaterialInstanceDynamic* Material =
		Widget.VitalsMaterialAdapter->GetMaterialInstance();
	return Material && Material->GetScalarParameterValue(ParameterName, Value)
		? Value
		: TNumericLimits<float>::Lowest();
}

UMaterialInstanceDynamic*
FWacomBattleEnemyPartEntryWidgetTestAccess::GetVitalsMaterial(
	const UWacomBattleEnemyPartEntryWidget& Widget)
{
	return Widget.VitalsMaterialAdapter->GetMaterialInstance();
}

float FWacomBattleEnemyPartEntryWidgetTestAccess::GetDamageTrailStartPercent(
	const UWacomBattleEnemyPartEntryWidget& Widget)
{
	return Widget.PresentationState->GetDamageTrailStartPercent();
}

float FWacomBattleEnemyPartEntryWidgetTestAccess::GetRuntimeFlashIntensity(
	const UWacomBattleEnemyPartEntryWidget& Widget)
{
	return Widget.PresentationState->GetFlashIntensity();
}

bool FWacomBattleEnemyPartEntryWidgetTestAccess::IsUsingSimplifiedMotion(
	const UWacomBattleEnemyPartEntryWidget& Widget)
{
	return Widget.PresentationState->IsUsingSimplifiedMotion();
}

bool FWacomBattleEnemyPartEntryWidgetTestAccess::IsActionPreviewFrameActive(
	const UWacomBattleEnemyPartEntryWidget& Widget)
{
	return Widget.PresentationState->BuildActionPreviewFrame().bActive;
}

bool FWacomBattleEnemyPartEntryWidgetTestAccess::IsPerfectReleasePreviewVisible(
	const UWacomBattleEnemyPartEntryWidget& Widget)
{
	return Widget.PresentationState->BuildActionPreviewFrame().bPerfectRelease;
}

bool FWacomBattleEnemyPartEntryWidgetTestAccess::IsResistanceComparisonVisible(
	const UWacomBattleEnemyPartEntryWidget& Widget)
{
	return Widget.PresentationState->BuildActionPreviewFrame().bShowResistanceComparison;
}

bool FWacomBattleEnemyPartEntryWidgetTestAccess::IsResistancePreviewSuccessful(
	const UWacomBattleEnemyPartEntryWidget& Widget)
{
	return Widget.PresentationState->BuildActionPreviewFrame().ResistanceOutcome
		== EWacomBattleEnemyResistancePreviewOutcome::Success;
}

bool FWacomBattleEnemyPartEntryWidgetTestAccess::PreviewWillAct(
	const UWacomBattleEnemyPartEntryWidget& Widget)
{
	return Widget.PresentationState->BuildActionPreviewFrame().bWillAct;
}

bool FWacomBattleEnemyPartEntryWidgetTestAccess::PreviewWillSkipActionDueToStun(
	const UWacomBattleEnemyPartEntryWidget& Widget)
{
	return Widget.PresentationState->BuildActionPreviewFrame().bWillSkipActionDueToStun;
}

int32 FWacomBattleEnemyPartEntryWidgetTestAccess::GetPreviewPlayerPeakDamage(
	const UWacomBattleEnemyPartEntryWidget& Widget)
{
	return Widget.PresentationState->BuildActionPreviewFrame().PlayerPeakDamage;
}

int32 FWacomBattleEnemyPartEntryWidgetTestAccess::GetPreviewEnemyPeakDamage(
	const UWacomBattleEnemyPartEntryWidget& Widget)
{
	return Widget.PresentationState->BuildActionPreviewFrame().EnemyPeakDamage;
}

FString FWacomBattleEnemyPartEntryWidgetTestAccess::GetPreviewComparator(
	const UWacomBattleEnemyPartEntryWidget& Widget)
{
	return Widget.PresentationState->BuildActionPreviewFrame().ComparatorText.ToString();
}

void FWacomBattleEnemyPartEntryWidgetTestAccess::ApplyRuntimeSettings(
	UWacomBattleEnemyPartEntryWidget& Widget,
	const FWacomLocalSettingsSnapshot& Snapshot)
{
	float FlashIntensity = 1.0f;
	if (Snapshot.FlashEffectMode == EWacomFlashEffectMode::Reduced)
	{
		FlashIntensity = 0.35f;
	}
	else if (Snapshot.FlashEffectMode == EWacomFlashEffectMode::Off)
	{
		FlashIntensity = 0.0f;
	}
	Widget.ApplyRuntimePresentationPolicy(
		Snapshot.UIMotionMode == EWacomUIMotionMode::Simplified,
		FlashIntensity);
}
