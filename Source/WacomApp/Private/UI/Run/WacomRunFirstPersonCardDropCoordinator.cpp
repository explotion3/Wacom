// Copyright Wacom. All Rights Reserved.

#include "UI/Run/WacomRunFirstPersonCardDropCoordinator.h"

#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Components/WacomRunFirstPersonCardSourceComponent.h"
#include "Components/WacomRunWorldInteractionTargetBridgeComponent.h"
#include "GameFramework/WacomPlayerController.h"
#include "Interaction/WacomRunWorldCardDropReceiver.h"
#include "RunSession.h"
#include "UI/Foundation/WacomAppToastSubsystem.h"
#include "UI/Run/WacomRunMenuDropTargetWidget.h"
#include "UI/Run/WacomRunMenuWidgetBase.h"

#define LOCTEXT_NAMESPACE "WacomRunFirstPersonCardDropCoordinator"

namespace
{
	FString GetDebugObjectName(const UObject* Object)
	{
		return IsValid(Object) ? Object->GetName() : TEXT("None");
	}

	const TCHAR* ToRunMenuCardDropIntentString(EWacomRunMenuCardDropIntentKind Kind)
	{
		switch (Kind)
		{
		case EWacomRunMenuCardDropIntentKind::ProbeZoneTarget:
			return TEXT("ProbeZoneTarget");
		case EWacomRunMenuCardDropIntentKind::SubmitZoneTarget:
			return TEXT("SubmitZoneTarget");
		case EWacomRunMenuCardDropIntentKind::Reject:
			return TEXT("Reject");
		case EWacomRunMenuCardDropIntentKind::None:
		default:
			return TEXT("None");
		}
	}

	const TCHAR* ToRunMenuCardDropRejectString(EWacomRunMenuCardDropRejectReason Reason)
	{
		switch (Reason)
		{
		case EWacomRunMenuCardDropRejectReason::NotInExploration:
			return TEXT("NotInExploration");
		case EWacomRunMenuCardDropRejectReason::MissingGameMenu:
			return TEXT("MissingGameMenu");
		case EWacomRunMenuCardDropRejectReason::MissingMenuLease:
			return TEXT("MissingMenuLease");
		case EWacomRunMenuCardDropRejectReason::MissingSession:
			return TEXT("MissingSession");
		case EWacomRunMenuCardDropRejectReason::InvalidSourceCard:
			return TEXT("InvalidSourceCard");
		case EWacomRunMenuCardDropRejectReason::MissingZoneTarget:
			return TEXT("MissingZoneTarget");
		case EWacomRunMenuCardDropRejectReason::UnsupportedTargetKind:
			return TEXT("UnsupportedTargetKind");
		case EWacomRunMenuCardDropRejectReason::MenuNotFound:
			return TEXT("MenuNotFound");
		case EWacomRunMenuCardDropRejectReason::MenuDoesNotAccept:
			return TEXT("MenuDoesNotAccept");
		case EWacomRunMenuCardDropRejectReason::CardNotOwned:
			return TEXT("CardNotOwned");
		case EWacomRunMenuCardDropRejectReason::RunValidationFailed:
			return TEXT("RunValidationFailed");
		case EWacomRunMenuCardDropRejectReason::SubmitFailed:
			return TEXT("SubmitFailed");
		case EWacomRunMenuCardDropRejectReason::None:
		default:
			return TEXT("None");
		}
	}

	const TCHAR* ToRunMenuCardDropSubmitPolicyString(EWacomRunMenuCardDropSubmitPolicy Policy)
	{
		switch (Policy)
		{
		case EWacomRunMenuCardDropSubmitPolicy::ControllerDestroyOwnedCard:
			return TEXT("ControllerDestroyOwnedCard");
		case EWacomRunMenuCardDropSubmitPolicy::MenuHandled:
			return TEXT("MenuHandled");
		case EWacomRunMenuCardDropSubmitPolicy::None:
		default:
			return TEXT("None");
		}
	}

	void FinalizeRunMenuCardDropDebug(
		FWacomRunMenuCardDropResolveResult& Result,
		const FVector2D& PointerPosition,
		bool bReleased)
	{
		Result.DebugSummary = FString::Printf(
			TEXT("RunMenuCardDropIntent{CardId=%s LeaseId=%s LeaseSource=%s Intent=%s Reject=%s SubmitPolicy=%s SubmitReason=%s ZoneId=%s RunValidation=%s CanSubmit=%s Submitted=%s Pointer=%s Released=%s Target=%s}"),
			*Result.SourceCardInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
			*Result.LeaseId.ToString(),
			*Result.LeaseSourceId.ToString(),
			ToRunMenuCardDropIntentString(Result.IntentKind),
			ToRunMenuCardDropRejectString(Result.RejectReason),
			ToRunMenuCardDropSubmitPolicyString(Result.SubmitPolicy),
			*Result.SubmitReason.ToString(),
			*Result.ZoneId.ToString(),
			*Result.RunValidationReason.ToString(),
			Result.bCanSubmit ? TEXT("true") : TEXT("false"),
			Result.bSubmitted ? TEXT("true") : TEXT("false"),
			*PointerPosition.ToString(),
			bReleased ? TEXT("true") : TEXT("false"),
			*Result.TargetHandle.ToString());
	}

	void FinalizeRunWorldCardDropDebugSummary(
		FString& OutDebugSummary,
		const FVector2D& PointerPosition,
		const FGuid& CardInstanceId,
		const FWacomInteractionTargetHandle& TargetHandle,
		const FRunWorldCardInteractionValidation& Validation,
		const AActor* TargetActor,
		const UWacomRunWorldCardDropReceiverComponent* Receiver,
		FName ResolveReason,
		bool bReleased,
		bool bSubmitted,
		FName ToastSource = NAME_None,
		const FText& ToastText = FText::GetEmpty())
	{
		const TCHAR* Phase = bReleased ? TEXT("Release") : TEXT("Preview");
		OutDebugSummary = FString::Printf(
			TEXT("RunWorldCardDrop{Phase=%s Card=%s TargetActor=%s Receiver=%s StableId=%s CanSubmit=%s Reason=%s Resolve=%s Submitted=%s Released=%s ToastSource=%s FailureToast=%s Pointer=%s Target=%s Validation=%s}"),
			Phase,
			*CardInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
			*GetDebugObjectName(TargetActor),
			*GetDebugObjectName(Receiver),
			*TargetHandle.StableTargetId.ToString(),
			Validation.bCanSubmit ? TEXT("true") : TEXT("false"),
			*Validation.DisabledReason.ToString(),
			*ResolveReason.ToString(),
			bSubmitted ? TEXT("true") : TEXT("false"),
			bReleased ? TEXT("true") : TEXT("false"),
			ToastSource.IsNone() ? TEXT("None") : *ToastSource.ToString(),
			ToastText.IsEmpty() ? TEXT("None") : *ToastText.ToString(),
			*PointerPosition.ToString(),
			*TargetHandle.ToString(),
			*Validation.DebugSummary);
	}

	FText BuildRunWorldCardDropConfigWarningToast(FName Reason)
	{
		if (Reason.IsNone())
		{
			return LOCTEXT("RunWorldCardDropConfigWarning", "场景交互配置异常");
		}
		return FText::Format(
			LOCTEXT("RunWorldCardDropConfigWarningWithReason", "场景交互配置异常：{0}"),
			FText::FromName(Reason));
	}
}

FWacomRunFirstPersonCardDropCoordinator::FWacomRunFirstPersonCardDropCoordinator(
	FContext InContext)
	: Context(MoveTemp(InContext))
{
}

class FWacomRunFirstPersonCardDropCoordinator::FTargetAdapter
{
public:
	explicit FTargetAdapter(FWacomRunFirstPersonCardDropCoordinator& InOwner)
		: Owner(InOwner)
	{
	}

	virtual ~FTargetAdapter() = default;

	virtual bool CanRoute(const FDropTransaction& Transaction) const = 0;
	virtual FDropProbeResult Probe(const FDropTransaction& Transaction) = 0;
	virtual FDropSubmitResult Submit(
		const FDropTransaction& Transaction,
		FDropProbeResult& Probe) = 0;
	virtual void ApplyPreview(
		const FDropTransaction& Transaction,
		const FDropProbeResult& Probe,
		const FDropSubmitResult& Submit) = 0;
	virtual void ClearPreview() = 0;
	virtual void StoreDebugSummary(const FString& DebugSummary) = 0;

protected:
	FWacomRunFirstPersonCardDropCoordinator& Owner;
};

class FWacomRunFirstPersonCardDropCoordinator::FMenuTargetAdapter final
	: public FWacomRunFirstPersonCardDropCoordinator::FTargetAdapter
{
public:
	using FTargetAdapter::FTargetAdapter;

	virtual bool CanRoute(const FDropTransaction& /*Transaction*/) const override
	{
		return Owner.ShouldHandleRunFirstPersonMenuDropProbe();
	}

	virtual FDropProbeResult Probe(const FDropTransaction& Transaction) override
	{
		FDropProbeResult Probe;
		Probe.MenuResult = Owner.ResolveRunMenuCardDropIntent(
			Transaction.CardInstanceId,
			Transaction.DragView);
		Probe.TargetHandle = Probe.MenuResult.TargetHandle;
		Probe.DebugSummary = Probe.MenuResult.DebugSummary;
		Probe.bCanSubmit = Probe.MenuResult.bCanSubmit;

		if (Probe.MenuResult.IntentKind == EWacomRunMenuCardDropIntentKind::None
			|| !Transaction.CardInstanceId.IsValid())
		{
			Probe.bShouldClearPreview = true;
			return Probe;
		}

		const UWacomRunMenuDropTargetWidget* NewTarget =
			Cast<UWacomRunMenuDropTargetWidget>(Probe.MenuResult.TargetHandle.SourceObject.Get());
		if (NewTarget)
		{
			Probe.FeedbackState =
				Probe.MenuResult.IntentKind == EWacomRunMenuCardDropIntentKind::Reject
					? EWacomFirstPersonCardDragTargetFeedbackState::Invalid
					: EWacomFirstPersonCardDragTargetFeedbackState::ZoneProbe;
			Probe.FeedbackTargetPosition = Transaction.ProbePosition;
		}
		else if (Transaction.bHasActiveDrag)
		{
			Probe.FeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::Invalid;
		}
		return Probe;
	}

	virtual FDropSubmitResult Submit(
		const FDropTransaction& Transaction,
		FDropProbeResult& Probe) override
	{
		FDropSubmitResult SubmitResult;
		if (Transaction.bReleased && Probe.MenuResult.bCanSubmit)
		{
			Owner.SubmitResolvedRunMenuCardDropIntent(Probe.MenuResult);
		}
		if (Transaction.bReleased)
		{
			FinalizeRunMenuCardDropDebug(
				Probe.MenuResult,
				Transaction.ProbePosition,
				/*bReleased*/ true);
		}
		Probe.DebugSummary = Probe.MenuResult.DebugSummary;
		SubmitResult.bSubmitted =
			Transaction.bReleased
			&& Probe.MenuResult.IntentKind == EWacomRunMenuCardDropIntentKind::SubmitZoneTarget
			&& Probe.MenuResult.bSubmitted;
		SubmitResult.bKeepPreviewAfterRelease =
			Transaction.bReleased
			&& Probe.MenuResult.TargetHandle.IsValid()
			&& Probe.MenuResult.IntentKind != EWacomRunMenuCardDropIntentKind::None;
		return SubmitResult;
	}

	virtual void ApplyPreview(
		const FDropTransaction& Transaction,
		const FDropProbeResult& Probe,
		const FDropSubmitResult& /*Submit*/) override
	{
		if (Probe.bShouldClearPreview)
		{
			ClearPreview();
			return;
		}

		UWacomRunMenuDropTargetWidget* NewTarget =
			Cast<UWacomRunMenuDropTargetWidget>(Probe.MenuResult.TargetHandle.SourceObject.Get());

		if (UWacomRunMenuDropTargetWidget* PreviousTarget = Owner.PreviewedRunMenuDropTarget.Get())
		{
			if (PreviousTarget != NewTarget)
			{
				PreviousTarget->ClearRunMenuDropPreviewState();
			}
		}
		Owner.PreviewedRunMenuDropTarget = NewTarget;

		if (!NewTarget)
		{
			return;
		}

		EWacomRunMenuDropTargetPreviewState PreviewState =
			EWacomRunMenuDropTargetPreviewState::Probe;
		if (Probe.MenuResult.IntentKind == EWacomRunMenuCardDropIntentKind::SubmitZoneTarget)
		{
			PreviewState = Transaction.bReleased && Probe.MenuResult.bSubmitted
				? EWacomRunMenuDropTargetPreviewState::Submitted
				: EWacomRunMenuDropTargetPreviewState::SubmitReady;
		}
		else if (Probe.MenuResult.IntentKind == EWacomRunMenuCardDropIntentKind::Reject)
		{
			PreviewState = EWacomRunMenuDropTargetPreviewState::Invalid;
		}
		else if (Transaction.bReleased)
		{
			PreviewState = EWacomRunMenuDropTargetPreviewState::ReleasedProbe;
		}

		NewTarget->SetRunMenuDropPreviewState(PreviewState);
	}

	virtual void ClearPreview() override
	{
		Owner.ClearRunMenuDropTargetProbe();
	}

	virtual void StoreDebugSummary(const FString& DebugSummary) override
	{
		Owner.LastRunMenuDropProbeDebugSummary = DebugSummary;
	}
};

class FWacomRunFirstPersonCardDropCoordinator::FWorldTargetAdapter final
	: public FWacomRunFirstPersonCardDropCoordinator::FTargetAdapter
{
public:
	using FTargetAdapter::FTargetAdapter;

	virtual bool CanRoute(const FDropTransaction& /*Transaction*/) const override
	{
		return Owner.ShouldHandleRunWorldCardDropProbe();
	}

	virtual FDropProbeResult Probe(const FDropTransaction& Transaction) override
	{
		FDropProbeResult Probe;
		AActor* TargetActor = nullptr;
		UWacomRunWorldInteractionTargetBridgeComponent* TargetBridge = nullptr;
		UWacomRunWorldCardDropReceiverComponent* Receiver = nullptr;
		Probe.WorldValidation = Owner.ResolveRunWorldCardDropIntent(
			Transaction.CardInstanceId,
			Transaction.DragView,
			Probe.TargetHandle,
			TargetActor,
			TargetBridge,
			Receiver,
			Probe.DebugSummary);
		Probe.WorldTargetActor = TargetActor;
		Probe.WorldTargetBridge = TargetBridge;
		Probe.WorldReceiver = Receiver;
		Probe.bCanSubmit = Probe.WorldValidation.bCanSubmit;
		Probe.bValidAnchorTarget = Probe.WorldValidation.bCanSubmit;

		if (Probe.TargetHandle.IsValid())
		{
			Probe.FeedbackState = Probe.WorldValidation.bCanSubmit
				? EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget
				: EWacomFirstPersonCardDragTargetFeedbackState::Invalid;
			Probe.FeedbackTargetPosition = Probe.TargetHandle.ScreenPosition;
		}
		else if (Transaction.bHasActiveDrag)
		{
			Probe.FeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::Invalid;
			Probe.FeedbackTargetPosition = Transaction.ProbePosition;
		}
		return Probe;
	}

	virtual FDropSubmitResult Submit(
		const FDropTransaction& Transaction,
		FDropProbeResult& Probe) override
	{
		FDropSubmitResult SubmitResult;
		UWacomRunWorldCardDropReceiverComponent* Receiver = Probe.WorldReceiver.Get();
		bool bSubmitted = false;
		if (Transaction.bReleased && Probe.WorldValidation.bCanSubmit && Receiver)
		{
			bSubmitted = Owner.SubmitResolvedRunWorldCardDropIntent(
				Transaction.CardInstanceId,
				Receiver,
				Probe.TargetHandle.StableTargetId,
				Probe.WorldValidation);
		}

		FText FailureToastText = FText::GetEmpty();
		FName ToastSource = NAME_None;
		const bool bReleasedOnRunWorldTarget =
			Transaction.bReleased && Probe.TargetHandle.IsValid();
		if (bReleasedOnRunWorldTarget && !bSubmitted)
		{
			const FName FailureReason = Probe.WorldValidation.DisabledReason.IsNone()
				? FName(TEXT("SubmitFailed"))
				: Probe.WorldValidation.DisabledReason;
			if (Receiver)
			{
				FailureToastText = Receiver->BuildRunWorldCardDropFailureToastText(
					Owner.Context.GetPlayerController(),
					Probe.TargetHandle.StableTargetId,
					Transaction.CardInstanceId,
					FailureReason);
				ToastSource = TEXT("Receiver");
			}
			else
			{
				FailureToastText = BuildRunWorldCardDropConfigWarningToast(FailureReason);
				ToastSource = TEXT("ControllerFallback");
			}
			if (!FailureToastText.IsEmpty())
			{
				if (UWacomAppToastSubsystem* ToastSubsystem =
					Owner.Context.ResolveAppToastSubsystem())
				{
					ToastSubsystem->ShowWarning(FailureToastText);
				}
			}
		}

		FinalizeRunWorldCardDropDebugSummary(
			Probe.DebugSummary,
			Transaction.ProbePosition,
			Transaction.CardInstanceId,
			Probe.TargetHandle,
			Probe.WorldValidation,
			Probe.WorldTargetActor.Get(),
			Receiver,
			Probe.WorldValidation.DisabledReason.IsNone()
				? FName(TEXT("Ok"))
				: Probe.WorldValidation.DisabledReason,
			Transaction.bReleased,
			bSubmitted,
			ToastSource,
			FailureToastText);

		if (bSubmitted)
		{
			if (UWacomAppToastSubsystem* ToastSubsystem =
				Owner.Context.ResolveAppToastSubsystem())
			{
				const FRunWorldCardInteractionRequest Request = Receiver
					? Receiver->BuildRunWorldCardDropRequest_Implementation(
						Probe.TargetHandle.StableTargetId,
						Transaction.CardInstanceId)
					: FRunWorldCardInteractionRequest();
				for (const FWacomRunWorldCardInteractionReward& Reward : Request.Rewards)
				{
					switch (Reward.Type)
					{
					case EWacomRunWorldCardInteractionRewardType::Gold:
						ToastSubsystem->ShowGoldChanged(Reward.GoldAmount);
						break;
					case EWacomRunWorldCardInteractionRewardType::Card:
						ToastSubsystem->ShowCardGained(Reward.CardDefinition.Get());
						break;
					case EWacomRunWorldCardInteractionRewardType::None:
					default:
						break;
					}
				}
			}
			SubmitResult.bRefreshRunHand = true;
		}

		SubmitResult.bSubmitted = bSubmitted;
		return SubmitResult;
	}

	virtual void ApplyPreview(
		const FDropTransaction& /*Transaction*/,
		const FDropProbeResult& Probe,
		const FDropSubmitResult& /*Submit*/) override
	{
		UWacomRunWorldInteractionTargetBridgeComponent* PreviousBridge =
			Owner.PreviewedRunWorldCardDropBridge.Get();
		UWacomRunWorldInteractionTargetBridgeComponent* NewBridge =
			Probe.WorldTargetBridge.IsValid() && Probe.WorldValidation.bCanSubmit
				? Probe.WorldTargetBridge.Get()
				: nullptr;
		if (PreviousBridge != NewBridge)
		{
			if (PreviousBridge)
			{
				PreviousBridge->ClearProbePreview();
			}
			Owner.PreviewedRunWorldCardDropBridge = NewBridge;
			if (NewBridge)
			{
				NewBridge->SetProbePreviewActive(true);
			}
		}
	}

	virtual void ClearPreview() override
	{
		Owner.ClearRunWorldCardDropProbe();
	}

	virtual void StoreDebugSummary(const FString& DebugSummary) override
	{
		Owner.LastRunWorldCardDropDebugSummary = DebugSummary;
		if (Owner.Context.ShouldLogRunWorldCardDrop())
		{
			UE_LOG(LogTemp, Display,
				TEXT("[WacomRunWorldCardDrop] %s"),
				*Owner.LastRunWorldCardDropDebugSummary);
		}
	}
};

void FWacomRunFirstPersonCardDropCoordinator::RegisterRunMenuDropTarget(
	UWacomRunMenuDropTargetWidget* DropTarget)
{
	if (!DropTarget)
	{
		return;
	}

	RunMenuDropTargets.RemoveAll(
		[](const TWeakObjectPtr<UWacomRunMenuDropTargetWidget>& Existing)
		{
			return !Existing.IsValid();
		});
	if (!RunMenuDropTargets.ContainsByPredicate(
		[DropTarget](const TWeakObjectPtr<UWacomRunMenuDropTargetWidget>& Existing)
		{
			return Existing.Get() == DropTarget;
		}))
	{
		RunMenuDropTargets.Add(DropTarget);
	}
}

void FWacomRunFirstPersonCardDropCoordinator::UnregisterRunMenuDropTarget(
	UWacomRunMenuDropTargetWidget* DropTarget)
{
	if (PreviewedRunMenuDropTarget.Get() == DropTarget)
	{
		ClearRunMenuDropTargetProbe();
	}

	RunMenuDropTargets.RemoveAll(
		[DropTarget](const TWeakObjectPtr<UWacomRunMenuDropTargetWidget>& Existing)
		{
			return !Existing.IsValid() || Existing.Get() == DropTarget;
		});
}

bool FWacomRunFirstPersonCardDropCoordinator::TryProbeRunMenuDropTargetAtWidgetPosition(
	const FVector2D& WidgetPosition,
	FWacomInteractionTargetHandle& OutHandle) const
{
	OutHandle = FWacomInteractionTargetHandle();

	for (int32 Index = RunMenuDropTargets.Num() - 1; Index >= 0; --Index)
	{
		UWacomRunMenuDropTargetWidget* Target = RunMenuDropTargets[Index].Get();
		if (!Target || !Target->CanProbeRunMenuDropTarget())
		{
			continue;
		}

		if (Target->ContainsWidgetPosition(WidgetPosition))
		{
			OutHandle = Target->BuildZoneTargetHandle(WidgetPosition);
			return OutHandle.IsValid()
				&& OutHandle.TargetKind == EWacomInteractionTargetKind::Zone
				&& !OutHandle.ZoneId.IsNone();
		}
	}

	return false;
}

bool FWacomRunFirstPersonCardDropCoordinator::ShouldHandleRunFirstPersonMenuDropProbe() const
{
	const UWacomRunFirstPersonCardSourceComponent* Source =
		Context.ResolveRunFirstPersonCardSource();
	return Context.IsInExplorationFlow()
		&& Context.HasActiveRunGameMenuOrTransitionSuppression()
		&& Source
		&& Source->HasActiveMenuLease();
}

bool FWacomRunFirstPersonCardDropCoordinator::ShouldHandleRunWorldCardDropProbe() const
{
	const UWacomRunFirstPersonCardSourceComponent* Source =
		Context.ResolveRunFirstPersonCardSource();
	if (!Context.IsRunWorldCardDropEnabled()
		|| !Context.IsInExplorationFlow()
		|| !Source
		|| !Source->IsRunFirstPersonCardLayerActive()
		|| Source->HasActiveMenuLease())
	{
		return false;
	}

	return !Context.HasActiveRunGameMenuOrTransitionSuppression();
}

bool FWacomRunFirstPersonCardDropCoordinator::ShouldBindRunFirstPersonCardDropDelegates() const
{
	const UWacomRunFirstPersonCardSourceComponent* Source =
		Context.ResolveRunFirstPersonCardSource();
	return Source
		&& (Source->HasActiveMenuLease()
			|| ShouldHandleRunWorldCardDropProbe());
}

bool FWacomRunFirstPersonCardDropCoordinator::HandleFormalDrag(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView,
	bool bReleased)
{
	const FDropTransaction Transaction =
		BuildDropTransaction(CardInstanceId, DragView, bReleased);
	const FDropTransactionResult Result = RouteDropTransaction(Transaction);
	if (!Result.bHandled && bReleased)
	{
		ClearAllDropProbes();
	}
	return Result.bSubmitted;
}

FWacomRunFirstPersonCardDropCoordinator::FDropTransaction
FWacomRunFirstPersonCardDropCoordinator::BuildDropTransaction(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView,
	bool bReleased) const
{
	FDropTransaction Transaction;
	Transaction.CardInstanceId = CardInstanceId;
	Transaction.DragView = DragView;
	Transaction.ProbePosition = DragView.bHasPointerViewportPosition
		? DragView.PointerViewportPosition
		: DragView.CurrentScreenPosition;
	Transaction.bReleased = bReleased;
	Transaction.bHasActiveDrag =
		DragView.GestureState == EWacomFirstPersonCardGestureState::DraggingNoTargetCard
		|| DragView.GestureState == EWacomFirstPersonCardGestureState::AimingTargetedCard;
	return Transaction;
}

FWacomRunFirstPersonCardDropCoordinator::FDropTransactionResult
FWacomRunFirstPersonCardDropCoordinator::RouteDropTransaction(
	const FDropTransaction& Transaction)
{
	FMenuTargetAdapter MenuAdapter(*this);
	if (MenuAdapter.CanRoute(Transaction))
	{
		return ApplyTargetAdapter(MenuAdapter, Transaction);
	}

	FWorldTargetAdapter WorldAdapter(*this);
	if (WorldAdapter.CanRoute(Transaction))
	{
		return ApplyTargetAdapter(WorldAdapter, Transaction);
	}

	return FDropTransactionResult();
}

FWacomRunFirstPersonCardDropCoordinator::FDropTransactionResult
FWacomRunFirstPersonCardDropCoordinator::ApplyTargetAdapter(
	FTargetAdapter& Adapter,
	const FDropTransaction& Transaction)
{
	FDropTransactionResult Result;
	Result.bHandled = true;

	FDropProbeResult Probe = Adapter.Probe(Transaction);
	FDropSubmitResult Submit;
	if (Transaction.bReleased)
	{
		Submit = Adapter.Submit(Transaction, Probe);
	}
	Adapter.ApplyPreview(Transaction, Probe, Submit);

	if (Transaction.bReleased && !Submit.bKeepPreviewAfterRelease)
	{
		Adapter.ClearPreview();
		Adapter.StoreDebugSummary(Probe.DebugSummary);
	}
	else
	{
		Adapter.StoreDebugSummary(Probe.DebugSummary);
		ApplyAnchorFeedback(Probe);
	}

	if (Submit.bRefreshRunHand)
	{
		Context.RefreshRunFirstPersonCardLayer();
	}

	Result.bSubmitted = Submit.bSubmitted;
	Result.bKeepPreviewAfterRelease = Submit.bKeepPreviewAfterRelease;
	return Result;
}

void FWacomRunFirstPersonCardDropCoordinator::ApplyAnchorFeedback(
	const FDropProbeResult& Probe)
{
	if (UWacomFirstPersonCardAnchorComponent* Anchor =
		Context.ResolveFirstPersonCardAnchor())
	{
		Anchor->SetFirstPersonCardDragFeedbackTarget(
			Probe.TargetHandle,
			Probe.bValidAnchorTarget,
			Probe.FeedbackState,
			Probe.FeedbackTargetPosition,
			Probe.DebugSummary);
	}
}

void FWacomRunFirstPersonCardDropCoordinator::ClearRunMenuDropTargetProbe()
{
	if (UWacomRunMenuDropTargetWidget* Target = PreviewedRunMenuDropTarget.Get())
	{
		Target->ClearRunMenuDropPreviewState();
	}
	PreviewedRunMenuDropTarget.Reset();
	LastRunMenuDropProbeDebugSummary = TEXT("RunMenuDropProbe{State=Cleared}");

	if (UWacomFirstPersonCardAnchorComponent* Anchor =
		Context.ResolveFirstPersonCardAnchor())
	{
		Anchor->SetFirstPersonCardDragFeedbackTarget(
			FWacomInteractionTargetHandle(),
			false,
			EWacomFirstPersonCardDragTargetFeedbackState::None,
			TOptional<FVector2D>(),
			LastRunMenuDropProbeDebugSummary);
	}
}

void FWacomRunFirstPersonCardDropCoordinator::ClearRunWorldCardDropProbe()
{
	if (UWacomRunWorldInteractionTargetBridgeComponent* Bridge =
		PreviewedRunWorldCardDropBridge.Get())
	{
		Bridge->ClearProbePreview();
	}
	PreviewedRunWorldCardDropBridge.Reset();
	LastRunWorldCardDropDebugSummary = TEXT("RunWorldCardDrop{State=Cleared}");

	if (UWacomFirstPersonCardAnchorComponent* Anchor =
		Context.ResolveFirstPersonCardAnchor())
	{
		Anchor->SetFirstPersonCardDragFeedbackTarget(
			FWacomInteractionTargetHandle(),
			false,
			EWacomFirstPersonCardDragTargetFeedbackState::None,
			TOptional<FVector2D>(),
			LastRunWorldCardDropDebugSummary);
	}
}

void FWacomRunFirstPersonCardDropCoordinator::ClearAllDropProbes()
{
	ClearRunMenuDropTargetProbe();
	ClearRunWorldCardDropProbe();
}

#if WITH_AUTOMATION_TESTS
bool FWacomRunFirstPersonCardDropCoordinator::ApplyRunMenuDropProbeFeedbackForTest(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView,
	bool bReleased)
{
	return ApplyRunMenuDropProbeFeedback(
		BuildDropTransaction(CardInstanceId, DragView, bReleased));
}

bool FWacomRunFirstPersonCardDropCoordinator::ApplyRunWorldCardDropProbeFeedbackForTest(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView,
	bool bReleased)
{
	return ApplyRunWorldCardDropProbeFeedback(
		BuildDropTransaction(CardInstanceId, DragView, bReleased));
}

FWacomRunMenuCardDropResolveResult
FWacomRunFirstPersonCardDropCoordinator::ResolveRunMenuCardDropIntentForTest(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView) const
{
	return ResolveRunMenuCardDropIntent(CardInstanceId, DragView);
}

FRunWorldCardInteractionValidation
FWacomRunFirstPersonCardDropCoordinator::ResolveRunWorldCardDropIntentForTest(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView,
	FWacomInteractionTargetHandle& OutTargetHandle,
	AActor*& OutTargetActor,
	UWacomRunWorldInteractionTargetBridgeComponent*& OutTargetBridge,
	UWacomRunWorldCardDropReceiverComponent*& OutReceiver,
	FString& OutDebugSummary) const
{
	return ResolveRunWorldCardDropIntent(
		CardInstanceId,
		DragView,
		OutTargetHandle,
		OutTargetActor,
		OutTargetBridge,
		OutReceiver,
		OutDebugSummary);
}
#endif

bool FWacomRunFirstPersonCardDropCoordinator::ApplyRunMenuDropProbeFeedback(
	const FDropTransaction& Transaction)
{
	FMenuTargetAdapter Adapter(*this);
	return ApplyTargetAdapter(Adapter, Transaction).bSubmitted;
}

bool FWacomRunFirstPersonCardDropCoordinator::ApplyRunWorldCardDropProbeFeedback(
	const FDropTransaction& Transaction)
{
	FWorldTargetAdapter Adapter(*this);
	return ApplyTargetAdapter(Adapter, Transaction).bSubmitted;
}

FWacomRunMenuCardDropResolveResult
FWacomRunFirstPersonCardDropCoordinator::ResolveRunMenuCardDropIntent(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView) const
{
	const FVector2D ProbePosition = DragView.bHasPointerViewportPosition
		? DragView.PointerViewportPosition
		: DragView.CurrentScreenPosition;

	FWacomRunMenuCardDropResolveResult Result;
	Result.SourceCardInstanceId = CardInstanceId;
	const UWacomRunFirstPersonCardSourceComponent* Source =
		Context.ResolveRunFirstPersonCardSource();
	Result.LeaseId = Source
		? Source->GetActiveMenuLeaseId()
		: NAME_None;
	Result.LeaseSourceId = Source
		? Source->GetActiveMenuLeaseSourceId()
		: NAME_None;

	auto RejectWith = [&Result, &ProbePosition](EWacomRunMenuCardDropRejectReason Reason)
	{
		Result.IntentKind = EWacomRunMenuCardDropIntentKind::Reject;
		Result.RejectReason = Reason;
		Result.bCanSubmit = false;
		FinalizeRunMenuCardDropDebug(Result, ProbePosition, /*bReleased*/ false);
		return Result;
	};

	if (!Context.IsInExplorationFlow())
	{
		return RejectWith(EWacomRunMenuCardDropRejectReason::NotInExploration);
	}

	if (!Context.HasActiveRunGameMenuOrTransitionSuppression())
	{
		return RejectWith(EWacomRunMenuCardDropRejectReason::MissingGameMenu);
	}
	if (!Source || !Source->HasActiveMenuLease())
	{
		return RejectWith(EWacomRunMenuCardDropRejectReason::MissingMenuLease);
	}
	if (!CardInstanceId.IsValid())
	{
		return RejectWith(EWacomRunMenuCardDropRejectReason::InvalidSourceCard);
	}
	FWacomInteractionTargetHandle TargetHandle;
	const bool bHasZoneTarget =
		TryProbeRunMenuDropTargetAtWidgetPosition(ProbePosition, TargetHandle);
	if (!bHasZoneTarget)
	{
		return RejectWith(EWacomRunMenuCardDropRejectReason::MissingZoneTarget);
	}
	if (TargetHandle.TargetKind != EWacomInteractionTargetKind::Zone)
	{
		Result.TargetHandle = TargetHandle;
		return RejectWith(EWacomRunMenuCardDropRejectReason::UnsupportedTargetKind);
	}

	Result.TargetHandle = TargetHandle;
	Result.ZoneId = TargetHandle.ZoneId;
	Result.IntentKind = EWacomRunMenuCardDropIntentKind::ProbeZoneTarget;
	Result.RejectReason = EWacomRunMenuCardDropRejectReason::None;
	Result.SubmitPolicy = EWacomRunMenuCardDropSubmitPolicy::None;
	Result.bCanSubmit = false;
	Result.bSubmitted = false;

	UWacomRunMenuWidgetBase* OwningMenu =
		Context.ResolveOwningMenuForLease(Result.LeaseId);
	if (!OwningMenu)
	{
		Result.IntentKind = EWacomRunMenuCardDropIntentKind::ProbeZoneTarget;
		Result.RejectReason = EWacomRunMenuCardDropRejectReason::MenuNotFound;
		FinalizeRunMenuCardDropDebug(Result, ProbePosition, /*bReleased*/ false);
		return Result;
	}

	Result = OwningMenu->ResolveRunMenuCardDropIntent(Result);

	if (Result.IntentKind != EWacomRunMenuCardDropIntentKind::SubmitZoneTarget
		|| Result.SubmitPolicy != EWacomRunMenuCardDropSubmitPolicy::ControllerDestroyOwnedCard)
	{
		FinalizeRunMenuCardDropDebug(Result, ProbePosition, /*bReleased*/ false);
		return Result;
	}

	URunSession* ResolvedRunSession =
		Context.ResolveRunSession();
	if (!ResolvedRunSession)
	{
		Result.IntentKind = EWacomRunMenuCardDropIntentKind::Reject;
		Result.RejectReason = EWacomRunMenuCardDropRejectReason::MissingSession;
		Result.SubmitPolicy = EWacomRunMenuCardDropSubmitPolicy::None;
		Result.bCanSubmit = false;
		FinalizeRunMenuCardDropDebug(Result, ProbePosition, /*bReleased*/ false);
		return Result;
	}

	const FRunDeckOperationValidation Validation =
		ResolvedRunSession->ValidateDestroyCardByInstance(CardInstanceId);
	Result.RunValidationReason = Validation.DisabledReason;
	if (!Validation.bCanExecute)
	{
		Result.IntentKind = EWacomRunMenuCardDropIntentKind::Reject;
		Result.RejectReason = Validation.DisabledReason == FName(TEXT("CardNotOwned"))
			? EWacomRunMenuCardDropRejectReason::CardNotOwned
			: EWacomRunMenuCardDropRejectReason::RunValidationFailed;
		Result.SubmitPolicy = EWacomRunMenuCardDropSubmitPolicy::None;
		Result.bCanSubmit = false;
		FinalizeRunMenuCardDropDebug(Result, ProbePosition, /*bReleased*/ false);
		return Result;
	}

	Result.IntentKind = EWacomRunMenuCardDropIntentKind::SubmitZoneTarget;
	Result.RejectReason = EWacomRunMenuCardDropRejectReason::None;
	Result.SubmitPolicy = EWacomRunMenuCardDropSubmitPolicy::ControllerDestroyOwnedCard;
	Result.bCanSubmit = true;
	FinalizeRunMenuCardDropDebug(Result, ProbePosition, /*bReleased*/ false);
	return Result;
}

bool FWacomRunFirstPersonCardDropCoordinator::SubmitResolvedRunMenuCardDropIntent(
	FWacomRunMenuCardDropResolveResult& Result)
{
	if (Result.IntentKind != EWacomRunMenuCardDropIntentKind::SubmitZoneTarget
		|| !Result.bCanSubmit)
	{
		Result.bSubmitted = false;
		return false;
	}

	if (Result.SubmitPolicy == EWacomRunMenuCardDropSubmitPolicy::MenuHandled)
	{
		UWacomRunMenuWidgetBase* OwningMenu =
			Context.ResolveOwningMenuForLease(Result.LeaseId);
		if (!OwningMenu)
		{
			Result.IntentKind = EWacomRunMenuCardDropIntentKind::Reject;
			Result.RejectReason = EWacomRunMenuCardDropRejectReason::MenuNotFound;
			Result.SubmitPolicy = EWacomRunMenuCardDropSubmitPolicy::None;
			Result.bCanSubmit = false;
			Result.bSubmitted = false;
			return false;
		}

		FWacomRunMenuCardDropResolveResult SubmittedResult = Result;
		const bool bSubmitted =
			OwningMenu->SubmitRunMenuCardDropIntent(Result, SubmittedResult);
		Result = SubmittedResult;
		Result.bSubmitted = bSubmitted && Result.bSubmitted;
		if (!Result.bSubmitted)
		{
			Result.IntentKind = EWacomRunMenuCardDropIntentKind::Reject;
			if (Result.RejectReason == EWacomRunMenuCardDropRejectReason::None)
			{
				Result.RejectReason = EWacomRunMenuCardDropRejectReason::SubmitFailed;
			}
			Result.SubmitPolicy = EWacomRunMenuCardDropSubmitPolicy::None;
			Result.bCanSubmit = false;
		}
		return Result.bSubmitted;
	}

	if (Result.SubmitPolicy != EWacomRunMenuCardDropSubmitPolicy::ControllerDestroyOwnedCard
		|| !Context.ResolveRunSession())
	{
		Result.bSubmitted = false;
		return false;
	}

	URunSession* ResolvedRunSession =
		Context.ResolveRunSession();
	const bool bDestroyed =
		ResolvedRunSession->DestroyCardByInstance(Result.SourceCardInstanceId);
	Result.bSubmitted = bDestroyed;
	if (!bDestroyed)
	{
		Result.IntentKind = EWacomRunMenuCardDropIntentKind::Reject;
		Result.RejectReason = EWacomRunMenuCardDropRejectReason::SubmitFailed;
		Result.SubmitPolicy = EWacomRunMenuCardDropSubmitPolicy::None;
		Result.bCanSubmit = false;
	}
	return bDestroyed;
}

FRunWorldCardInteractionValidation
FWacomRunFirstPersonCardDropCoordinator::ResolveRunWorldCardDropIntent(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView,
	FWacomInteractionTargetHandle& OutTargetHandle,
	AActor*& OutTargetActor,
	UWacomRunWorldInteractionTargetBridgeComponent*& OutTargetBridge,
	UWacomRunWorldCardDropReceiverComponent*& OutReceiver,
	FString& OutDebugSummary) const
{
	OutTargetHandle = FWacomInteractionTargetHandle();
	OutTargetActor = nullptr;
	OutTargetBridge = nullptr;
	OutReceiver = nullptr;

	auto RejectWith = [&](
		FName Reason,
		const FWacomInteractionTargetHandle& TargetHandle =
			FWacomInteractionTargetHandle())
	{
		FRunWorldCardInteractionValidation Result;
		Result.bCanSubmit = false;
		Result.DisabledReason = Reason;
		Result.DebugSummary = FString::Printf(
			TEXT("RunWorldCardInteraction{CanSubmit=false Reason=%s}"),
			*Reason.ToString());
		FinalizeRunWorldCardDropDebugSummary(
			OutDebugSummary,
			DragView.bHasPointerViewportPosition
				? DragView.PointerViewportPosition
				: DragView.CurrentScreenPosition,
			CardInstanceId,
			TargetHandle,
			Result,
			OutTargetActor,
			OutReceiver,
			Reason,
			/*bReleased*/ false,
			/*bSubmitted*/ false);
		return Result;
	};

	if (!Context.IsRunWorldCardDropEnabled())
	{
		return RejectWith(TEXT("Disabled"));
	}
	if (!ShouldHandleRunWorldCardDropProbe())
	{
		return RejectWith(TEXT("Blocked"));
	}
	if (!CardInstanceId.IsValid())
	{
		return RejectWith(TEXT("InvalidSourceCard"));
	}

	const FVector2D ProbePosition = DragView.bHasPointerViewportPosition
		? DragView.PointerViewportPosition
		: DragView.CurrentScreenPosition;
	if (!Context.TryProbeRunSceneInteractionTargetAtWidgetPosition(
		ProbePosition,
		OutTargetHandle))
	{
		return RejectWith(TEXT("MissingRunWorldTarget"));
	}

	FName ResolveRejectReason = NAME_None;
	if (!Context.ResolveRunWorldClickableInteractableFromHandle(
		OutTargetHandle,
		OutTargetActor,
		OutTargetBridge,
		ResolveRejectReason))
	{
		return RejectWith(ResolveRejectReason, OutTargetHandle);
	}

	OutReceiver =
		Context.ResolveRunWorldCardDropReceiverFromHandle(OutTargetHandle);
	if (!OutReceiver)
	{
		return RejectWith(TEXT("MissingCardDropReceiver"), OutTargetHandle);
	}

	AWacomPlayerController* PlayerController = Context.GetPlayerController();
	FRunWorldCardInteractionValidation Validation =
		OutReceiver->ValidateRunWorldCardDrop_Implementation(
			PlayerController,
			OutTargetHandle.StableTargetId,
			CardInstanceId);
	FinalizeRunWorldCardDropDebugSummary(
		OutDebugSummary,
		ProbePosition,
		CardInstanceId,
		OutTargetHandle,
		Validation,
		OutTargetActor,
		OutReceiver,
		Validation.DisabledReason.IsNone()
			? FName(TEXT("Ok"))
			: Validation.DisabledReason,
		/*bReleased*/ false,
		/*bSubmitted*/ false);
	return Validation;
}

bool FWacomRunFirstPersonCardDropCoordinator::SubmitResolvedRunWorldCardDropIntent(
	const FGuid& CardInstanceId,
	UWacomRunWorldCardDropReceiverComponent* Receiver,
	FName PersistentId,
	FRunWorldCardInteractionValidation& InOutValidation)
{
	if (!Receiver || PersistentId.IsNone() || !CardInstanceId.IsValid())
	{
		InOutValidation.bCanSubmit = false;
		InOutValidation.DisabledReason = TEXT("InvalidSubmitContext");
		return false;
	}

	AWacomPlayerController* PlayerController = Context.GetPlayerController();
	if (!PlayerController)
	{
		InOutValidation.bCanSubmit = false;
		InOutValidation.DisabledReason = TEXT("MissingPlayerController");
		return false;
	}

	const bool bSubmitted = Receiver->SubmitRunWorldCardDrop_Implementation(
		PlayerController,
		PersistentId,
		CardInstanceId,
		InOutValidation);
	if (!bSubmitted && InOutValidation.DisabledReason.IsNone())
	{
		InOutValidation.DisabledReason = TEXT("SubmitFailed");
	}
	return bSubmitted;
}

#undef LOCTEXT_NAMESPACE
