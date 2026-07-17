// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

class APlayerController;
class AWacomPlayerCharacter;
class UWorld;
class UWacomFirstPersonCardAnchorSpecProbeComponent;

namespace WacomFirstPersonCardLayerSpec
{
	UWorld* FindAutomationWorld();
	UWacomFirstPersonCardAnchorSpecProbeComponent* AddProbe(AWacomPlayerCharacter* Character);
	void PrimeFallbackAnchor(
		APlayerController* PC,
		AWacomPlayerCharacter* Character,
		UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor);
	FWacomFirstPersonCardLayerSlotView MakeProjectedInteractionSlot(
		const FGuid& CardInstanceId,
		bool bPlayable = true,
		bool bProjected = true);
	FWacomFirstPersonCardLayerSlotView MakeMotionSlot(
		const FGuid& CardInstanceId,
		int32 Index,
		const FVector2D& Position,
		float Angle = 0.0f,
		float Scale = 1.0f,
		float Opacity = 1.0f);
	FWacomFirstPersonCardSlotMotionConfig MakeFastSlotMotionConfig();
	FWacomFirstPersonCardInteractionFeedbackConfig MakeTestFeedbackConfig();
	void SetSlotInteractionIntent(
		FWacomFirstPersonCardLayerSlotView& Slot,
		EWacomFirstPersonCardInteractionIntent InteractionIntent);

	class FLayerInteractionReceiver
	{
	public:
		int32 HoverCount = 0;
		int32 UnhoverCount = 0;
		FGuid LastCardId;

		void HandleHovered(
			const FGuid& CardInstanceId,
			const FWacomFirstPersonCardLayerSlotView&)
		{
			++HoverCount;
			LastCardId = CardInstanceId;
		}

		void HandleUnhovered(
			const FGuid& CardInstanceId,
			const FWacomFirstPersonCardLayerSlotView&)
		{
			++UnhoverCount;
			LastCardId = CardInstanceId;
		}
	};

	class FLayerDragReceiver
	{
	public:
		int32 StartedCount = 0;
		int32 UpdatedCount = 0;
		int32 ReleasedCount = 0;
		int32 CancelledCount = 0;
		FGuid LastCardId;
		FWacomFirstPersonCardDragView LastDragView;

		void HandleStarted(
			const FGuid& CardInstanceId,
			const FWacomFirstPersonCardDragView& DragView)
		{
			++StartedCount;
			LastCardId = CardInstanceId;
			LastDragView = DragView;
		}

		void HandleUpdated(
			const FGuid& CardInstanceId,
			const FWacomFirstPersonCardDragView& DragView)
		{
			++UpdatedCount;
			LastCardId = CardInstanceId;
			LastDragView = DragView;
		}

		void HandleReleased(
			const FGuid& CardInstanceId,
			const FWacomFirstPersonCardDragView& DragView)
		{
			++ReleasedCount;
			LastCardId = CardInstanceId;
			LastDragView = DragView;
		}

		void HandleCancelled(
			const FGuid& CardInstanceId,
			const FWacomFirstPersonCardDragView& DragView)
		{
			++CancelledCount;
			LastCardId = CardInstanceId;
			LastDragView = DragView;
		}
	};
}
