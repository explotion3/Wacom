// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"

FWacomFirstPersonCardSlotMotionConfig NormalizeSlotMotionConfig(
	const FWacomFirstPersonCardSlotMotionConfig& InConfig);
bool AreSlotMotionConfigsEquivalent(
	const FWacomFirstPersonCardSlotMotionConfig& A,
	const FWacomFirstPersonCardSlotMotionConfig& B);

FWacomFirstPersonCardSlotVisualConfig NormalizeSlotVisualConfig(
	const FWacomFirstPersonCardSlotVisualConfig& InConfig);
bool AreSlotVisualConfigsEquivalent(
	const FWacomFirstPersonCardSlotVisualConfig& A,
	const FWacomFirstPersonCardSlotVisualConfig& B);

FWacomFirstPersonCardInteractionFeedbackConfig NormalizeInteractionFeedbackConfig(
	const FWacomFirstPersonCardInteractionFeedbackConfig& InConfig);
bool AreInteractionFeedbackConfigsEquivalent(
	const FWacomFirstPersonCardInteractionFeedbackConfig& A,
	const FWacomFirstPersonCardInteractionFeedbackConfig& B);

FWacomFirstPersonCardDragPickupConfig NormalizeDragPickupConfig(
	const FWacomFirstPersonCardDragPickupConfig& InConfig);
bool AreDragPickupConfigsEquivalent(
	const FWacomFirstPersonCardDragPickupConfig& A,
	const FWacomFirstPersonCardDragPickupConfig& B);

FWacomFirstPersonCardDragConfig NormalizeCardDragConfig(
	const FWacomFirstPersonCardDragConfig& InConfig);
bool AreCardDragConfigsEquivalent(
	const FWacomFirstPersonCardDragConfig& A,
	const FWacomFirstPersonCardDragConfig& B);

FWacomFirstPersonCardSlotRuntimeConfig NormalizeSlotRuntimeConfig(
	const FWacomFirstPersonCardSlotRuntimeConfig& InConfig);
bool AreSlotRuntimeConfigsEquivalent(
	const FWacomFirstPersonCardSlotRuntimeConfig& A,
	const FWacomFirstPersonCardSlotRuntimeConfig& B);
