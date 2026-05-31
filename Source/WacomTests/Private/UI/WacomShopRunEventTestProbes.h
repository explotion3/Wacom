// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/WacomPlayerController.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "UI/Events/WacomRunEventScreen.h"
#include "UI/Foundation/WacomMenuWidgetBase.h"
#include "UI/Run/WacomRunMenuCardLeaseTestMenu.h"
#include "UI/Run/WacomRunMenuDropTargetWidget.h"
#include "UI/Shop/WacomShopScreen.h"
#include "Engine/HitResult.h"
#include "WacomShopRunEventTestProbes.generated.h"

class URunSession;
class UWacomAppToastSubsystem;
class UPrimitiveComponent;

UCLASS()
class UWacomRunMenuDropTargetWidgetProbe : public UWacomRunMenuDropTargetWidget
{
	GENERATED_BODY()

public:
	bool bProbeHitForTest = false;

	virtual bool ContainsWidgetPosition(FVector2D WidgetPosition) const override
	{
		LastWidgetPositionForTest = WidgetPosition;
		return bProbeHitForTest && CanProbeRunMenuDropTarget();
	}

	FVector2D GetLastWidgetPositionForTest() const { return LastWidgetPositionForTest; }

private:
	mutable FVector2D LastWidgetPositionForTest = FVector2D::ZeroVector;
};

UCLASS()
class AWacomPlayerControllerProbe : public AWacomPlayerController
{
	GENERATED_BODY()

public:
	AActor* ReadClosestInteractable() const
	{
		return PickClosestInteractable();
	}

	FText ReadCurrentInteractPrompt() const
	{
		return BuildCurrentInteractPrompt();
	}

	void SetRunSceneHitForTest(AActor* InActor, UPrimitiveComponent* InComponent = nullptr)
	{
		bHasRunSceneHitOverride = true;
		RunSceneHitOverride = FHitResult();
		RunSceneHitOverride.HitObjectHandle = FActorInstanceHandle(InActor);
		RunSceneHitOverride.Component = InComponent;
	}

	void ClearRunSceneHitForTest()
	{
		bHasRunSceneHitOverride = false;
		RunSceneHitOverride = FHitResult();
	}

	bool ProbeRunSceneTargetForTest(FWacomInteractionTargetHandle& OutHandle) const
	{
		return TryProbeRunSceneInteractionTarget(OutHandle);
	}

	bool ProbeRunSceneTargetAtWidgetPositionForTest(
		const FVector2D& WidgetPosition,
		FWacomInteractionTargetHandle& OutHandle) const
	{
		return TryProbeRunSceneInteractionTargetAtWidgetPosition(WidgetPosition, OutHandle);
	}

	void UpdateRunWorldTargetProbePreviewForTest()
	{
		UpdateRunWorldTargetProbePreview();
	}

	void ClearRunWorldTargetProbePreviewForTest()
	{
		ClearRunWorldTargetProbePreview();
	}

	void RegisterRunMenuDropTargetForTest(UWacomRunMenuDropTargetWidget* Target)
	{
		RegisterRunMenuDropTarget(Target);
	}

	void UnregisterRunMenuDropTargetForTest(UWacomRunMenuDropTargetWidget* Target)
	{
		UnregisterRunMenuDropTarget(Target);
	}

	bool ProbeRunMenuDropTargetAtWidgetPositionForTest(
		const FVector2D& WidgetPosition,
		FWacomInteractionTargetHandle& OutHandle) const
	{
		return TryProbeRunMenuDropTargetAtWidgetPosition(WidgetPosition, OutHandle);
	}

	void ClearRunMenuDropTargetProbeForTest()
	{
		ClearRunMenuDropTargetProbe();
	}

	bool ApplyRunMenuDropProbeFeedbackForTest(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView,
		bool bReleased)
	{
		return ApplyRunMenuDropProbeFeedback(CardInstanceId, DragView, bReleased);
	}

	FString ReadRunMenuDropProbeDebugSummaryForTest() const
	{
		return GetRunMenuDropProbeDebugSummaryForTest();
	}

	FWacomRunMenuCardDropResolveResult ResolveRunMenuCardDropIntentForTest(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView) const
	{
		return ResolveRunMenuCardDropIntent(CardInstanceId, DragView);
	}

	void SetRunSessionForTest(URunSession* InRunSession)
	{
		RunSessionForTest = InRunSession;
	}

protected:
	virtual bool IsInExplorationFlow() const override
	{
		return bRunProbeExplorationFlowForTest;
	}

	virtual URunSession* ResolveRunSessionForFirstPersonCardSource() const override
	{
		return RunSessionForTest
			? RunSessionForTest.Get()
			: Super::ResolveRunSessionForFirstPersonCardSource();
	}

	virtual bool BuildRunSceneClickHitResult(FHitResult& OutHitResult) const override
	{
		if (!bHasRunSceneHitOverride)
		{
			return false;
		}

		OutHitResult = RunSceneHitOverride;
		return OutHitResult.GetActor() || OutHitResult.GetComponent();
	}

	virtual bool BuildRunSceneInteractionTargetHitResultAtWidgetPosition(
		const FVector2D& WidgetPosition,
		FHitResult& OutHitResult) const override
	{
		if (!bHasRunSceneHitOverride)
		{
			return false;
		}

		OutHitResult = RunSceneHitOverride;
		OutHitResult.Location = FVector(WidgetPosition.X, WidgetPosition.Y, 0.0f);
		return OutHitResult.GetActor() || OutHitResult.GetComponent();
	}

private:
	bool bRunProbeExplorationFlowForTest = true;
	FHitResult RunSceneHitOverride;
	bool bHasRunSceneHitOverride = false;

	UPROPERTY(Transient)
	TObjectPtr<URunSession> RunSessionForTest = nullptr;
};

UCLASS()
class UWacomShopScreenProbe : public UWacomShopScreen
{
	GENERATED_BODY()

public:
	void SetRunSession(URunSession* InRunSession)
	{
		RunSession = InRunSession;
	}

	void SetToastSubsystem(UWacomAppToastSubsystem* InToastSubsystem)
	{
		ToastSubsystem = InToastSubsystem;
	}

	FText ReadGoldText() const
	{
		return GetDisplayedGoldText();
	}

	int32 ReadOfferCount() const
	{
		return GetCachedOfferCount();
	}

	FWacomShopOfferPresentationView ReadOfferPresentationView(int32 Index) const
	{
		return GetCachedOfferView(Index);
	}

	bool PurchaseOfferAt(int32 Index)
	{
		return PurchaseOfferByIndex(Index);
	}

	void SuppressEndOnNextDeactivateForTest()
	{
		SuppressEndShopVisitOnNextDeactivate();
	}

	static FText FormatPurchaseFailureToast(FName DisabledReason)
	{
		return BuildPurchaseFailureToastText(DisabledReason);
	}

protected:
	virtual URunSession* ResolveRunSession() const override
	{
		return RunSession ? RunSession.Get() : UWacomShopScreen::ResolveRunSession();
	}

	virtual UWacomAppToastSubsystem* ResolveToastSubsystem() const override
	{
		return ToastSubsystem ? ToastSubsystem.Get() : UWacomShopScreen::ResolveToastSubsystem();
	}

private:
	UPROPERTY(Transient)
	TObjectPtr<URunSession> RunSession = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UWacomAppToastSubsystem> ToastSubsystem = nullptr;
};

UCLASS()
class UWacomMenuWidgetBaseProbe : public UWacomMenuWidgetBase
{
	GENERATED_BODY()

public:
	bool bAcceptRunMenuFirstPersonCardDropForTest = false;
	EWacomRunMenuCardDropSubmitPolicy SubmitPolicyForTest =
		EWacomRunMenuCardDropSubmitPolicy::None;
	bool bMenuSubmitSucceedsForTest = true;
	FName AcceptedZoneIdForTest = NAME_None;
	FWacomRunMenuCardDropResolveResult LastDropResultForTest;

	void SetOwningWacomPlayerControllerForTest(AWacomPlayerController* InPlayerController)
	{
		OwningPlayerControllerForTest = InPlayerController;
	}

	void DeactivateForTest()
	{
		NativeOnDeactivated();
	}

protected:
	virtual AWacomPlayerController* ResolveOwningWacomPlayerController() const override
	{
		return OwningPlayerControllerForTest
			? OwningPlayerControllerForTest.Get()
			: Super::ResolveOwningWacomPlayerController();
	}

	virtual FWacomRunMenuCardDropResolveResult ResolveRunMenuFirstPersonCardDropIntent_Implementation(
		const FWacomRunMenuCardDropResolveResult& Candidate) const override
	{
		FWacomRunMenuCardDropResolveResult Result = Candidate;
		if (!bAcceptRunMenuFirstPersonCardDropForTest
			|| (!AcceptedZoneIdForTest.IsNone() && Result.ZoneId != AcceptedZoneIdForTest))
		{
			Result.IntentKind = EWacomRunMenuCardDropIntentKind::ProbeZoneTarget;
			Result.RejectReason = EWacomRunMenuCardDropRejectReason::MenuDoesNotAccept;
			Result.SubmitPolicy = EWacomRunMenuCardDropSubmitPolicy::None;
			Result.bCanSubmit = false;
			return Result;
		}

		Result.IntentKind = EWacomRunMenuCardDropIntentKind::SubmitZoneTarget;
		Result.RejectReason = EWacomRunMenuCardDropRejectReason::None;
		Result.SubmitPolicy = SubmitPolicyForTest;
		Result.bCanSubmit = SubmitPolicyForTest != EWacomRunMenuCardDropSubmitPolicy::None;
		return Result;
	}

	virtual bool SubmitRunMenuFirstPersonCardDropIntent_Implementation(
		const FWacomRunMenuCardDropResolveResult& Resolved,
		FWacomRunMenuCardDropResolveResult& OutSubmitted) override
	{
		OutSubmitted = Resolved;
		OutSubmitted.bSubmitted = bMenuSubmitSucceedsForTest;
		if (!bMenuSubmitSucceedsForTest)
		{
			OutSubmitted.IntentKind = EWacomRunMenuCardDropIntentKind::Reject;
			OutSubmitted.RejectReason = EWacomRunMenuCardDropRejectReason::SubmitFailed;
			OutSubmitted.SubmitPolicy = EWacomRunMenuCardDropSubmitPolicy::None;
			OutSubmitted.bCanSubmit = false;
		}
		LastDropResultForTest = OutSubmitted;
		return bMenuSubmitSucceedsForTest;
	}

private:
	UPROPERTY(Transient)
	TObjectPtr<AWacomPlayerController> OwningPlayerControllerForTest = nullptr;
};

UCLASS()
class UWacomRunMenuCardLeaseTestMenuProbe : public UWacomRunMenuCardLeaseTestMenu
{
	GENERATED_BODY()

public:
	void SetOwningWacomPlayerControllerForTest(AWacomPlayerController* InPlayerController)
	{
		OwningPlayerControllerForTest = InPlayerController;
	}

	void DeactivateForTest()
	{
		NativeOnDeactivated();
	}

protected:
	virtual AWacomPlayerController* ResolveOwningWacomPlayerController() const override
	{
		return OwningPlayerControllerForTest
			? OwningPlayerControllerForTest.Get()
			: Super::ResolveOwningWacomPlayerController();
	}

private:
	UPROPERTY(Transient)
	TObjectPtr<AWacomPlayerController> OwningPlayerControllerForTest = nullptr;
};

UCLASS()
class UWacomRunEventScreenProbe : public UWacomRunEventScreen
{
	GENERATED_BODY()

public:
	void SetRunSession(URunSession* InRunSession)
	{
		RunSession = InRunSession;
	}

	void SetToastSubsystem(UWacomAppToastSubsystem* InToastSubsystem)
	{
		ToastSubsystem = InToastSubsystem;
	}

	int32 ReadChoiceCount() const
	{
		return GetChoiceCount();
	}

	FRunEventChoiceSnapshot ReadChoiceSnapshot(int32 Index) const
	{
		return GetCachedChoiceSnapshot(Index);
	}

	FWacomRunMenuCardDropResolveResult ResolveDropForTest(
		const FWacomRunMenuCardDropResolveResult& Candidate) const
	{
		return ResolveRunMenuFirstPersonCardDropIntent_Implementation(Candidate);
	}

	bool SubmitDropForTest(
		const FWacomRunMenuCardDropResolveResult& Resolved,
		FWacomRunMenuCardDropResolveResult& OutSubmitted)
	{
		return SubmitRunMenuFirstPersonCardDropIntent_Implementation(Resolved, OutSubmitted);
	}

	bool ChooseChoiceAt(int32 Index)
	{
		return ChooseChoiceByIndex(Index);
	}

	void SuppressEndOnNextDeactivateForTest()
	{
		SuppressEndRunEventOnNextDeactivate();
	}

	FText ReadTitleText() const
	{
		return GetDisplayedTitleText();
	}

	FText ReadBodyText() const
	{
		return GetDisplayedBodyText();
	}

	FWacomRunEventScreenDebugView ReadRunEventScreenDebugView() const
	{
		return GetRunEventScreenDebugView();
	}

	FString ReadRunEventScreenDebugSummary() const
	{
		return GetRunEventScreenDebugSummary();
	}

protected:
	virtual URunSession* ResolveRunSession() const override
	{
		return RunSession ? RunSession.Get() : UWacomRunEventScreen::ResolveRunSession();
	}

	virtual UWacomAppToastSubsystem* ResolveToastSubsystem() const override
	{
		return ToastSubsystem ? ToastSubsystem.Get() : UWacomRunEventScreen::ResolveToastSubsystem();
	}

private:
	UPROPERTY(Transient)
	TObjectPtr<URunSession> RunSession = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UWacomAppToastSubsystem> ToastSubsystem = nullptr;
};
