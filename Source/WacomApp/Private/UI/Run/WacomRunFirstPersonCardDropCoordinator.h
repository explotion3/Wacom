// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunStateTypes.h"
#include "Templates/Function.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "UI/Run/WacomRunMenuCardDropIntentTypes.h"

class AActor;
class AWacomPlayerController;
class URunSession;
class UWacomAppToastSubsystem;
class UWacomFirstPersonCardAnchorComponent;
class UWacomRunMenuDropTargetWidget;
class UWacomRunWorldCardDropReceiverComponent;
class UWacomRunWorldInteractionTargetBridgeComponent;
class UWacomRunFirstPersonCardSourceComponent;
class UWacomMenuWidgetBase;

/**
 * App-private coordinator for Run first-person card drop.
 *
 * PlayerController owns the lifetime and input context; this Module owns the
 * menu/world target registry, probe preview, resolve, submit and debug state.
 */
class FWacomRunFirstPersonCardDropCoordinator
{
public:
	struct FContext
	{
		TWeakObjectPtr<AWacomPlayerController> PlayerController;

		TFunction<bool()> IsInExplorationFlowFunc;
		TFunction<bool()> HasActiveRunGameMenuOrTransitionSuppressionFunc;
		TFunction<bool()> IsRunWorldCardDropEnabledFunc;
		TFunction<bool()> ShouldLogRunWorldCardDropFunc;
		TFunction<UWacomRunFirstPersonCardSourceComponent*()> ResolveRunFirstPersonCardSourceFunc;
		TFunction<UWacomFirstPersonCardAnchorComponent*()> ResolveFirstPersonCardAnchorFunc;
		TFunction<URunSession*()> ResolveRunSessionFunc;
		TFunction<UWacomMenuWidgetBase*(FName)> ResolveOwningMenuForLeaseFunc;
		TFunction<UWacomAppToastSubsystem*()> ResolveAppToastSubsystemFunc;
		TFunction<void()> RefreshRunFirstPersonCardLayerFunc;
		TFunction<bool(const FVector2D&, FWacomInteractionTargetHandle&)>
			TryProbeRunSceneInteractionTargetAtWidgetPositionFunc;
		TFunction<bool(
			const FWacomInteractionTargetHandle&,
			AActor*&,
			UWacomRunWorldInteractionTargetBridgeComponent*&,
			FName&)>
			ResolveRunWorldClickableInteractableFromHandleFunc;
		TFunction<UWacomRunWorldCardDropReceiverComponent*(
			const FWacomInteractionTargetHandle&)>
			ResolveRunWorldCardDropReceiverFromHandleFunc;

		AWacomPlayerController* GetPlayerController() const
		{
			return PlayerController.Get();
		}

		bool IsInExplorationFlow() const
		{
			return IsInExplorationFlowFunc && IsInExplorationFlowFunc();
		}

		bool HasActiveRunGameMenuOrTransitionSuppression() const
		{
			return HasActiveRunGameMenuOrTransitionSuppressionFunc
				&& HasActiveRunGameMenuOrTransitionSuppressionFunc();
		}

		bool IsRunWorldCardDropEnabled() const
		{
			return IsRunWorldCardDropEnabledFunc && IsRunWorldCardDropEnabledFunc();
		}

		bool ShouldLogRunWorldCardDrop() const
		{
			return ShouldLogRunWorldCardDropFunc && ShouldLogRunWorldCardDropFunc();
		}

		UWacomRunFirstPersonCardSourceComponent* ResolveRunFirstPersonCardSource() const
		{
			return ResolveRunFirstPersonCardSourceFunc
				? ResolveRunFirstPersonCardSourceFunc()
				: nullptr;
		}

		UWacomFirstPersonCardAnchorComponent* ResolveFirstPersonCardAnchor() const
		{
			return ResolveFirstPersonCardAnchorFunc
				? ResolveFirstPersonCardAnchorFunc()
				: nullptr;
		}

		URunSession* ResolveRunSession() const
		{
			return ResolveRunSessionFunc ? ResolveRunSessionFunc() : nullptr;
		}

		UWacomMenuWidgetBase* ResolveOwningMenuForLease(FName LeaseId) const
		{
			return ResolveOwningMenuForLeaseFunc
				? ResolveOwningMenuForLeaseFunc(LeaseId)
				: nullptr;
		}

		UWacomAppToastSubsystem* ResolveAppToastSubsystem() const
		{
			return ResolveAppToastSubsystemFunc
				? ResolveAppToastSubsystemFunc()
				: nullptr;
		}

		void RefreshRunFirstPersonCardLayer() const
		{
			if (RefreshRunFirstPersonCardLayerFunc)
			{
				RefreshRunFirstPersonCardLayerFunc();
			}
		}

		bool TryProbeRunSceneInteractionTargetAtWidgetPosition(
			const FVector2D& WidgetPosition,
			FWacomInteractionTargetHandle& OutHandle) const
		{
			return TryProbeRunSceneInteractionTargetAtWidgetPositionFunc
				&& TryProbeRunSceneInteractionTargetAtWidgetPositionFunc(WidgetPosition, OutHandle);
		}

		bool ResolveRunWorldClickableInteractableFromHandle(
			const FWacomInteractionTargetHandle& Handle,
			AActor*& OutInteractableActor,
			UWacomRunWorldInteractionTargetBridgeComponent*& OutBridge,
			FName& OutRejectReason) const
		{
			return ResolveRunWorldClickableInteractableFromHandleFunc
				&& ResolveRunWorldClickableInteractableFromHandleFunc(
					Handle,
					OutInteractableActor,
					OutBridge,
					OutRejectReason);
		}

		UWacomRunWorldCardDropReceiverComponent* ResolveRunWorldCardDropReceiverFromHandle(
			const FWacomInteractionTargetHandle& Handle) const
		{
			return ResolveRunWorldCardDropReceiverFromHandleFunc
				? ResolveRunWorldCardDropReceiverFromHandleFunc(Handle)
				: nullptr;
		}
	};

	explicit FWacomRunFirstPersonCardDropCoordinator(FContext InContext);

	void RegisterRunMenuDropTarget(UWacomRunMenuDropTargetWidget* DropTarget);
	void UnregisterRunMenuDropTarget(UWacomRunMenuDropTargetWidget* DropTarget);
	bool TryProbeRunMenuDropTargetAtWidgetPosition(
		const FVector2D& WidgetPosition,
		FWacomInteractionTargetHandle& OutHandle) const;

	bool ShouldHandleRunFirstPersonMenuDropProbe() const;
	bool ShouldHandleRunWorldCardDropProbe() const;
	bool ShouldBindRunFirstPersonCardDropDelegates() const;

	bool HandleFormalDrag(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView,
		bool bReleased);

	void ClearRunMenuDropTargetProbe();
	void ClearRunWorldCardDropProbe();
	void ClearAllDropProbes();

	const FString& GetRunMenuDropProbeDebugSummary() const
	{
		return LastRunMenuDropProbeDebugSummary;
	}

	const FString& GetRunWorldCardDropDebugSummary() const
	{
		return LastRunWorldCardDropDebugSummary;
	}

#if WITH_AUTOMATION_TESTS
	bool ApplyRunMenuDropProbeFeedbackForTest(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView,
		bool bReleased);
	bool ApplyRunWorldCardDropProbeFeedbackForTest(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView,
		bool bReleased);
	FWacomRunMenuCardDropResolveResult ResolveRunMenuCardDropIntentForTest(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView) const;
	FRunWorldCardInteractionValidation ResolveRunWorldCardDropIntentForTest(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView,
		FWacomInteractionTargetHandle& OutTargetHandle,
		AActor*& OutTargetActor,
		UWacomRunWorldInteractionTargetBridgeComponent*& OutTargetBridge,
		UWacomRunWorldCardDropReceiverComponent*& OutReceiver,
		FString& OutDebugSummary) const;
#endif

private:
	struct FDropTransaction
	{
		FGuid CardInstanceId;
		FWacomFirstPersonCardDragView DragView;
		FVector2D ProbePosition = FVector2D::ZeroVector;
		bool bReleased = false;
		bool bHasActiveDrag = false;
	};

	struct FDropTransactionResult
	{
		bool bHandled = false;
		bool bSubmitted = false;
		bool bKeepPreviewAfterRelease = false;
	};

	struct FDropProbeResult
	{
		FWacomInteractionTargetHandle TargetHandle;
		EWacomFirstPersonCardDragTargetFeedbackState FeedbackState =
			EWacomFirstPersonCardDragTargetFeedbackState::None;
		TOptional<FVector2D> FeedbackTargetPosition;
		FString DebugSummary;
		bool bValidAnchorTarget = false;
		bool bCanSubmit = false;
		bool bShouldClearPreview = false;

		FWacomRunMenuCardDropResolveResult MenuResult;
		FRunWorldCardInteractionValidation WorldValidation;
		TWeakObjectPtr<AActor> WorldTargetActor;
		TWeakObjectPtr<UWacomRunWorldInteractionTargetBridgeComponent> WorldTargetBridge;
		TWeakObjectPtr<UWacomRunWorldCardDropReceiverComponent> WorldReceiver;
	};

	struct FDropSubmitResult
	{
		bool bSubmitted = false;
		bool bKeepPreviewAfterRelease = false;
		bool bRefreshRunHand = false;
	};

	class FTargetAdapter;
	class FMenuTargetAdapter;
	class FWorldTargetAdapter;

	FDropTransaction BuildDropTransaction(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView,
		bool bReleased) const;
	FDropTransactionResult RouteDropTransaction(
		const FDropTransaction& Transaction);
	FDropTransactionResult ApplyTargetAdapter(
		FTargetAdapter& Adapter,
		const FDropTransaction& Transaction);
	void ApplyAnchorFeedback(const FDropProbeResult& Probe);
	bool ApplyRunMenuDropProbeFeedback(
		const FDropTransaction& Transaction);
	bool ApplyRunWorldCardDropProbeFeedback(
		const FDropTransaction& Transaction);
	FWacomRunMenuCardDropResolveResult ResolveRunMenuCardDropIntent(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView) const;
	bool SubmitResolvedRunMenuCardDropIntent(
		FWacomRunMenuCardDropResolveResult& Result);
	FRunWorldCardInteractionValidation ResolveRunWorldCardDropIntent(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView,
		FWacomInteractionTargetHandle& OutTargetHandle,
		AActor*& OutTargetActor,
		UWacomRunWorldInteractionTargetBridgeComponent*& OutTargetBridge,
		UWacomRunWorldCardDropReceiverComponent*& OutReceiver,
		FString& OutDebugSummary) const;
	bool SubmitResolvedRunWorldCardDropIntent(
		const FGuid& CardInstanceId,
		UWacomRunWorldCardDropReceiverComponent* Receiver,
		FName PersistentId,
		FRunWorldCardInteractionValidation& InOutValidation);

	FContext Context;
	TArray<TWeakObjectPtr<UWacomRunMenuDropTargetWidget>> RunMenuDropTargets;
	TWeakObjectPtr<UWacomRunMenuDropTargetWidget> PreviewedRunMenuDropTarget;
	TWeakObjectPtr<UWacomRunWorldInteractionTargetBridgeComponent> PreviewedRunWorldCardDropBridge;
	FString LastRunMenuDropProbeDebugSummary;
	FString LastRunWorldCardDropDebugSummary;
};
