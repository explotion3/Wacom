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

FWacomFirstPersonCardSlotFeedbackConfig NormalizeSlotFeedbackConfig(
	const FWacomFirstPersonCardSlotFeedbackConfig& InConfig);
bool AreSlotFeedbackConfigsEquivalent(
	const FWacomFirstPersonCardSlotFeedbackConfig& A,
	const FWacomFirstPersonCardSlotFeedbackConfig& B);

FWacomFirstPersonCardDragConfig NormalizeCardDragConfig(
	const FWacomFirstPersonCardDragConfig& InConfig);
bool AreCardDragConfigsEquivalent(
	const FWacomFirstPersonCardDragConfig& A,
	const FWacomFirstPersonCardDragConfig& B);
