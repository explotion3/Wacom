// Copyright Wacom. All Rights Reserved.

#include "Validation/FirstPersonCardLayoutPresetValidation.h"

#include "UI/Card/WacomFirstPersonCardLayoutPreset.h"

#define LOCTEXT_NAMESPACE "WacomFirstPersonCardLayoutPresetValidation"

namespace
{
	void AddValidationError(FWacomFirstPersonCardLayoutPresetValidationReport& Report, const FText& Message)
	{
		Report.Errors.Add(Message);
	}

	void AddValidationWarning(FWacomFirstPersonCardLayoutPresetValidationReport& Report, const FText& Message)
	{
		Report.Warnings.Add(Message);
	}

	FText FormatValidationMessage(const TCHAR* Format, const FString& A)
	{
		return FText::FromString(FString::Format(Format, { A }));
	}

	FText FormatValidationMessage(const TCHAR* Format, const FString& A, const FString& B)
	{
		return FText::FromString(FString::Format(Format, { A, B }));
	}

	bool IsNearlyZero(float Value)
	{
		return FMath::IsNearlyZero(Value, KINDA_SMALL_NUMBER);
	}

	void ValidateNonNegativeFloat(
		FWacomFirstPersonCardLayoutPresetValidationReport& Report,
		const TCHAR* FieldName,
		float Value)
	{
		if (!FMath::IsFinite(Value) || Value < 0.0f)
		{
			AddValidationError(Report,
				FormatValidationMessage(TEXT("{0} 必须是大于等于 0 的有限数值。"), FieldName));
		}
	}

	void ValidatePositiveFloat(
		FWacomFirstPersonCardLayoutPresetValidationReport& Report,
		const TCHAR* FieldName,
		float Value)
	{
		if (!FMath::IsFinite(Value) || Value <= 0.0f)
		{
			AddValidationError(Report,
				FormatValidationMessage(TEXT("{0} 必须是大于 0 的有限数值。"), FieldName));
		}
	}

	void ValidateUnitFloat(
		FWacomFirstPersonCardLayoutPresetValidationReport& Report,
		const TCHAR* FieldName,
		float Value)
	{
		if (!FMath::IsFinite(Value) || Value < 0.0f || Value > 1.0f)
		{
			AddValidationError(Report,
				FormatValidationMessage(TEXT("{0} 必须位于 0 到 1 之间。"), FieldName));
		}
	}

	void ValidateViewportAnchor(
		FWacomFirstPersonCardLayoutPresetValidationReport& Report,
		const TCHAR* FieldName,
		const FVector2D& Value)
	{
		if (!FMath::IsFinite(Value.X) || !FMath::IsFinite(Value.Y)
			|| Value.X < 0.0f || Value.X > 1.0f
			|| Value.Y < 0.0f || Value.Y > 1.0f)
		{
			AddValidationError(Report,
				FormatValidationMessage(TEXT("{0} 必须位于归一化视口范围 0 到 1。"), FieldName));
		}
	}
}

FWacomFirstPersonCardLayoutPresetValidationReport FWacomFirstPersonCardLayoutPresetValidation::BuildReport(
	const UWacomFirstPersonCardLayoutPreset* Preset)
{
	FWacomFirstPersonCardLayoutPresetValidationReport Report;

	if (!Preset)
	{
		AddValidationError(Report,
			LOCTEXT("MissingPreset", "FirstPersonCardLayoutPreset 为空。"));
		return Report;
	}

	ValidateNonNegativeFloat(Report, TEXT("AuthoredCardSpacingPixels"), Preset->AuthoredCardSpacingPixels);
	ValidateNonNegativeFloat(Report, TEXT("AuthoredMaxHandWidthPixels"), Preset->AuthoredMaxHandWidthPixels);
	ValidatePositiveFloat(Report, TEXT("AuthoredDropCurveExponent"), Preset->AuthoredDropCurveExponent);
	ValidatePositiveFloat(Report, TEXT("AuthoredFanCurveExponent"), Preset->AuthoredFanCurveExponent);
	ValidateNonNegativeFloat(Report, TEXT("AuthoredCardBodyBottomViewportPaddingPixels"),
		Preset->AuthoredCardBodyBottomViewportPaddingPixels);
	ValidateNonNegativeFloat(Report, TEXT("ProjectionPadding"), Preset->ProjectionPadding);
	ValidateNonNegativeFloat(Report, TEXT("SoftClampOffscreenAllowancePixels"),
		Preset->SoftClampOffscreenAllowancePixels);
	ValidateNonNegativeFloat(Report, TEXT("SoftClampBlendRangePixels"), Preset->SoftClampBlendRangePixels);
	ValidatePositiveFloat(Report, TEXT("CardLayerPixelSnapGrid"), Preset->CardLayerPixelSnapGrid);
	ValidatePositiveFloat(Report, TEXT("StaticCardRenderScale"), Preset->StaticCardRenderScale);
	ValidateNonNegativeFloat(Report, TEXT("StaticCardEdgeDropPixels"), Preset->StaticCardEdgeDropPixels);
	ValidateNonNegativeFloat(Report, TEXT("ShortHandEdgeDropPixels"), Preset->ShortHandEdgeDropPixels);
	ValidateNonNegativeFloat(Report, TEXT("AnchorScreenSmoothingSpeed"), Preset->AnchorScreenSmoothingSpeed);
	ValidateNonNegativeFloat(Report, TEXT("AnchorScreenSmoothingResetDistancePixels"),
		Preset->AnchorScreenSmoothingResetDistancePixels);
	ValidateNonNegativeFloat(Report, TEXT("CardSlotMotionSpeed"), Preset->CardSlotMotionSpeed);
	ValidateNonNegativeFloat(Report, TEXT("CardSlotOpacitySpeed"), Preset->CardSlotOpacitySpeed);
	ValidateUnitFloat(Report, TEXT("CardSlotEnterOpacity"), Preset->CardSlotEnterOpacity);
	ValidateNonNegativeFloat(Report, TEXT("CardSlotExitDuration"), Preset->CardSlotExitDuration);
	ValidateNonNegativeFloat(Report, TEXT("CardSlotMotionResetDistancePixels"),
		Preset->CardSlotMotionResetDistancePixels);
	ValidateViewportAnchor(Report, TEXT("DrawnCardEnterViewportAnchor"), Preset->DrawnCardEnterViewportAnchor);
	ValidateViewportAnchor(Report, TEXT("GainedCardEnterViewportAnchor"), Preset->GainedCardEnterViewportAnchor);
	ValidateViewportAnchor(Report, TEXT("PlayedCardExitViewportAnchor"), Preset->PlayedCardExitViewportAnchor);
	ValidateViewportAnchor(Report, TEXT("DiscardedCardExitViewportAnchor"), Preset->DiscardedCardExitViewportAnchor);
	ValidatePositiveFloat(Report, TEXT("DrawnCardEnterScaleMultiplier"), Preset->DrawnCardEnterScaleMultiplier);
	ValidatePositiveFloat(Report, TEXT("GainedCardEnterScaleMultiplier"), Preset->GainedCardEnterScaleMultiplier);
	ValidatePositiveFloat(Report, TEXT("PlayedCardExitScaleMultiplier"), Preset->PlayedCardExitScaleMultiplier);
	ValidatePositiveFloat(Report, TEXT("DiscardedCardExitScaleMultiplier"), Preset->DiscardedCardExitScaleMultiplier);
	ValidateNonNegativeFloat(Report, TEXT("PendingTargetingLiftPixels"), Preset->PendingTargetingLiftPixels);
	ValidatePositiveFloat(Report, TEXT("PendingTargetingScale"), Preset->PendingTargetingScale);
	ValidateUnitFloat(Report, TEXT("PendingTargetingAngleBlend"), Preset->PendingTargetingAngleBlend);
	ValidateUnitFloat(Report, TEXT("TargetSelectNonPendingOpacityMultiplier"),
		Preset->TargetSelectNonPendingOpacityMultiplier);
	ValidatePositiveFloat(Report, TEXT("HandAnchorScale"), Preset->HandAnchorScale);
	ValidateUnitFloat(Report, TEXT("DisabledRenderOpacity"), Preset->DisabledRenderOpacity);
	ValidateNonNegativeFloat(Report, TEXT("HoverLiftPixels"), Preset->HoverLiftPixels);
	ValidatePositiveFloat(Report, TEXT("HoverScale"), Preset->HoverScale);
	ValidateNonNegativeFloat(Report, TEXT("HoverHitHysteresisPixels"), Preset->HoverHitHysteresisPixels);
	ValidateNonNegativeFloat(Report, TEXT("DragCardTargetFocusLiftPixels"),
		Preset->DragCardTargetFocusLiftPixels);
	ValidatePositiveFloat(Report, TEXT("DragCardTargetFocusScale"), Preset->DragCardTargetFocusScale);
	ValidateNonNegativeFloat(Report, TEXT("CardPointerCameraLookScale"), Preset->CardPointerCameraLookScale);
	ValidatePositiveFloat(Report, TEXT("PressedFeedbackScale"), Preset->PressedFeedbackScale);
	ValidateUnitFloat(Report, TEXT("PlayableHoverFeedbackOpacity"), Preset->PlayableHoverFeedbackOpacity);
	ValidateUnitFloat(Report, TEXT("PressedFeedbackOpacity"), Preset->PressedFeedbackOpacity);
	ValidateNonNegativeFloat(Report, TEXT("ConfirmFeedbackDuration"), Preset->ConfirmFeedbackDuration);
	ValidateUnitFloat(Report, TEXT("ConfirmFeedbackOpacity"), Preset->ConfirmFeedbackOpacity);
	ValidateNonNegativeFloat(Report, TEXT("DenyFeedbackDuration"), Preset->DenyFeedbackDuration);
	ValidateNonNegativeFloat(Report, TEXT("DenyFeedbackShakePixels"), Preset->DenyFeedbackShakePixels);
	ValidateUnitFloat(Report, TEXT("DenyFeedbackOpacity"), Preset->DenyFeedbackOpacity);

	if (Preset->EdgeDropScaleMinCardCount < 1)
	{
		AddValidationError(Report,
			LOCTEXT("InvalidEdgeDropScaleMinCardCount", "EdgeDropScaleMinCardCount 必须大于等于 1。"));
	}
	if (Preset->EdgeDropScaleMaxCardCount < 1)
	{
		AddValidationError(Report,
			LOCTEXT("InvalidEdgeDropScaleMaxCardCount", "EdgeDropScaleMaxCardCount 必须大于等于 1。"));
	}
	if (Preset->bScaleEdgeDropByHandCount
		&& Preset->EdgeDropScaleMaxCardCount <= Preset->EdgeDropScaleMinCardCount)
	{
		AddValidationError(Report,
			LOCTEXT("InvalidEdgeDropScaleRange",
				"按手牌数量缩放边缘下坠时，EdgeDropScaleMaxCardCount 必须大于 EdgeDropScaleMinCardCount。"));
	}

	if (Preset->PendingTargetingZOrderBoost < 0
		|| Preset->HoverZOrderBoost < 0
		|| Preset->DragCardTargetFocusZOrderBoost < 0)
	{
		AddValidationError(Report,
			LOCTEXT("NegativeZOrderBoost", "Pending / Hover / Drag target ZOrder boost 不能为负数。"));
	}

	if (Preset->ProjectionMode == EWacomFirstPersonCardProjectionMode::BodyLocked
		&& (!IsNearlyZero(Preset->LookInfluenceYaw) || !IsNearlyZero(Preset->LookInfluencePitch)))
	{
		AddValidationWarning(Report,
			LOCTEXT("BodyLockedLookInfluenceIgnored",
				"ProjectionMode 为 BodyLocked 时 LookInfluenceYaw / LookInfluencePitch 不参与手牌锚点计算；资产仍有效，但这些值只有切到 Look Responsive Projected 后才会生效。"));
	}

	if (Preset->ProjectionMode == EWacomFirstPersonCardProjectionMode::LegacyWorldProjected)
	{
		if (Preset->LookInfluenceYaw > 0.35f)
		{
			AddValidationWarning(Report,
				FormatValidationMessage(
					TEXT("Look Responsive Projected 的 LookInfluenceYaw={0} 偏高：建议先控制在 0.05-0.35，过高会让手牌随鼠标漂移明显。"),
					FString::SanitizeFloat(Preset->LookInfluenceYaw)));
		}
		if (Preset->LookInfluencePitch > 0.20f)
		{
			AddValidationWarning(Report,
				FormatValidationMessage(
					TEXT("Look Responsive Projected 的 LookInfluencePitch={0} 偏高：建议先控制在 0.03-0.20，过高会影响读牌稳定性。"),
					FString::SanitizeFloat(Preset->LookInfluencePitch)));
		}
	}

	if (Preset->StaticCardRenderScale > 1.2f)
	{
		AddValidationWarning(Report,
			FormatValidationMessage(TEXT("StaticCardRenderScale={0} 偏大：第一人称手牌可能遮挡视野或重叠严重。"),
				FString::SanitizeFloat(Preset->StaticCardRenderScale)));
	}

	if (Preset->StaticCardEdgeDropPixels > 180.0f)
	{
		AddValidationWarning(Report,
			FormatValidationMessage(TEXT("StaticCardEdgeDropPixels={0} 偏大：大手牌边缘卡可能掉得过低。"),
				FString::SanitizeFloat(Preset->StaticCardEdgeDropPixels)));
	}

	if (Preset->bScaleEdgeDropByHandCount
		&& Preset->ShortHandEdgeDropPixels > Preset->StaticCardEdgeDropPixels)
	{
		AddValidationWarning(Report,
			LOCTEXT("ShortHandDropGreaterThanStaticDrop",
				"ShortHandEdgeDropPixels 大于 StaticCardEdgeDropPixels：少牌边缘会比大手牌掉得更低，通常不是预期。"));
	}

	if (FMath::Abs(Preset->FanYawDegrees) > 12.0f)
	{
		AddValidationWarning(Report,
			FormatValidationMessage(TEXT("FanYawDegrees={0} 偏大：卡面旋转锯齿和遮挡风险较高。"),
				FString::SanitizeFloat(Preset->FanYawDegrees)));
	}

	if (Preset->ViewportClampMode == EWacomFirstPersonCardViewportClampMode::SoftClampToViewport
		&& Preset->SoftClampBlendRangePixels <= 0.0f)
	{
		AddValidationWarning(Report,
			LOCTEXT("SoftClampNoBlendRange",
				"ViewportClampMode 为 SoftClampToViewport 但 SoftClampBlendRangePixels 为 0：资产仍有效，但越界后会接近硬停。"));
	}

	if (Preset->bEnableAnchorScreenSmoothing
		&& Preset->AnchorScreenSmoothingSpeed <= 0.0f)
	{
		AddValidationWarning(Report,
			LOCTEXT("AnchorSmoothingImmediate",
				"已开启 Anchor screen smoothing 但 AnchorScreenSmoothingSpeed 为 0：资产仍有效，实际表现会立即贴合。"));
	}

	if (Preset->bEnableCardSlotMotion
		&& Preset->CardSlotMotionSpeed <= 0.0f)
	{
		AddValidationWarning(Report,
			LOCTEXT("SlotMotionImmediate",
				"已开启 CardSlotMotion 但 CardSlotMotionSpeed 为 0：资产仍有效，卡牌槽位置/角度/缩放会立即贴合。"));
	}

	return Report;
}

bool FWacomFirstPersonCardLayoutPresetValidation::Validate(
	const UWacomFirstPersonCardLayoutPreset* Preset,
	TArray<FText>& OutErrors)
{
	const FWacomFirstPersonCardLayoutPresetValidationReport Report = BuildReport(Preset);
	OutErrors = Report.Errors;
	return Report.IsValid();
}

#undef LOCTEXT_NAMESPACE
