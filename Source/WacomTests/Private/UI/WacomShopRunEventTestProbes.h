// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Actors/BattleTriggerActor.h"
#include "Actors/WacomRunPickupActor.h"
#include "Actors/WacomRunEventTriggerActor.h"
#include "Actors/WacomShopTriggerActor.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "Components/WacomRunWorldInteractionTargetBridgeComponent.h"
#include "GameFramework/WacomPlayerController.h"
#include "Interaction/WacomWorldInteractable.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "UI/Events/WacomRunEventChoiceButton.h"
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
class UWacomRunEventPaymentDropTargetWidgetClassProbe : public UWacomRunMenuDropTargetWidget
{
	GENERATED_BODY()

public:
	UWacomRunEventPaymentDropTargetWidgetClassProbe(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get())
		: Super(ObjectInitializer)
	{
		ProbePreviewScale = 1.111f;
	}
};

UCLASS()
class UWacomRunEventChoiceButtonClassProbe : public UWacomRunEventChoiceButton
{
	GENERATED_BODY()

public:
	int32 SnapshotAppliedCountForTest = 0;
	FName LastAppliedChoiceIdForTest = NAME_None;

protected:
	virtual void BP_OnRunEventChoiceSnapshotApplied_Implementation(
		const FRunEventChoiceSnapshot& AppliedChoiceSnapshot) override
	{
		++SnapshotAppliedCountForTest;
		LastAppliedChoiceIdForTest = AppliedChoiceSnapshot.ChoiceId;
	}
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

	FString ReadRunWorldInteractableHoverDebugSummaryForTest() const
	{
		return GetRunWorldInteractableHoverDebugSummary();
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

	void SetRunProbeExplorationFlowForTest(bool bInExploration)
	{
		bRunProbeExplorationFlowForTest = bInExploration;
	}

	bool RouteRunWorldInteractableClickForTest()
	{
		return TryRouteRunWorldInteractableClick();
	}

	bool InputLeftMouseReleasedForTest()
	{
		FInputKeyEventArgs Args;
		Args.Key = EKeys::LeftMouseButton;
		Args.Event = IE_Released;
		return InputKey(Args);
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

	void SetRunFirstPersonMenuLeaseForTest(FName LeaseId = TEXT("Test.MenuLease"))
	{
		TArray<FWacomFirstPersonCardLayerEntry> Entries;
		FWacomFirstPersonCardLayerEntry Entry;
		Entry.CardInstanceId = FGuid::NewGuid();
		Entries.Add(Entry);
		SetRunFirstPersonCardLayerMenuLease(LeaseId, TEXT("Test.Source"), Entries);
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
class AWacomRunEventTriggerClickProbe : public AWacomRunEventTriggerActor
{
	GENERATED_BODY()

public:
	int32 TryInteractCountForTest = 0;
	bool bInteractResultForTest = true;

	void SyncClickTargetForTest()
	{
		OnConstruction(FTransform::Identity);
	}

	virtual bool TryInteract_Implementation(AWacomPlayerController* PC) override
	{
		++TryInteractCountForTest;
		LastInteractingPlayerControllerForTest = PC;
		return bInteractResultForTest;
	}

	AWacomPlayerController* GetLastInteractingPlayerControllerForTest() const
	{
		return LastInteractingPlayerControllerForTest.Get();
	}

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<AWacomPlayerController> LastInteractingPlayerControllerForTest;
};

UCLASS()
class AWacomShopTriggerClickProbe : public AWacomShopTriggerActor
{
	GENERATED_BODY()

public:
	int32 TryInteractCountForTest = 0;
	bool bInteractResultForTest = true;

	void SyncClickTargetForTest()
	{
		OnConstruction(FTransform::Identity);
	}

	virtual bool TryInteract_Implementation(AWacomPlayerController* PC) override
	{
		++TryInteractCountForTest;
		LastInteractingPlayerControllerForTest = PC;
		return bInteractResultForTest;
	}

	AWacomPlayerController* GetLastInteractingPlayerControllerForTest() const
	{
		return LastInteractingPlayerControllerForTest.Get();
	}

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<AWacomPlayerController> LastInteractingPlayerControllerForTest;
};

UCLASS()
class AWacomBattleTriggerClickProbe : public ABattleTriggerActor
{
	GENERATED_BODY()

public:
	int32 TryInteractCountForTest = 0;
	bool bInteractResultForTest = true;

	void SyncClickTargetForTest()
	{
		OnConstruction(FTransform::Identity);
	}

	virtual bool TryInteract_Implementation(AWacomPlayerController* PC) override
	{
		++TryInteractCountForTest;
		LastInteractingPlayerControllerForTest = PC;
		return bInteractResultForTest;
	}

	AWacomPlayerController* GetLastInteractingPlayerControllerForTest() const
	{
		return LastInteractingPlayerControllerForTest.Get();
	}

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<AWacomPlayerController> LastInteractingPlayerControllerForTest;
};

UCLASS()
class AWacomRunPickupClickProbe : public AWacomRunPickupActor
{
	GENERATED_BODY()

public:
	void SyncClickTargetForTest()
	{
		OnConstruction(FTransform::Identity);
	}
};

UCLASS()
class AWacomRunWorldNonClickableInteractableProbe : public AActor, public IWacomWorldInteractable
{
	GENERATED_BODY()

public:
	int32 TryInteractCountForTest = 0;
	bool bCanInteractForTest = true;
	FText PromptForTest = FText::FromString(TEXT("按 E 测试"));

	virtual FText GetInteractPromptText_Implementation(AWacomPlayerController* /*PC*/) const override
	{
		return PromptForTest;
	}

	virtual FVector GetInteractLocation_Implementation(AWacomPlayerController* /*PC*/) const override
	{
		return GetActorLocation();
	}

	virtual bool CanInteract_Implementation(AWacomPlayerController* /*PC*/) const override
	{
		return bCanInteractForTest;
	}

	virtual bool TryInteract_Implementation(AWacomPlayerController* /*PC*/) override
	{
		++TryInteractCountForTest;
		return true;
	}
};

UCLASS()
class AWacomGenericRunWorldClickableInteractableProbe
	: public AActor
	, public IWacomWorldInteractable
	, public IWacomRunWorldClickableInteractable
{
	GENERATED_BODY()

public:
	AWacomGenericRunWorldClickableInteractableProbe()
	{
		Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
		SetRootComponent(Root);

		ClickBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("ClickBounds"));
		ClickBounds->SetupAttachment(Root);
		FWacomRunWorldClickableInteractableHelper::ConfigureClickBounds(ClickBounds);

		Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
		Visual->SetupAttachment(Root);

		ClickInteractionTarget = CreateDefaultSubobject<UWacomInteractionTargetComponent>(
			TEXT("ClickInteractionTarget"));
		ClickTargetBridge = CreateDefaultSubobject<UWacomRunWorldInteractionTargetBridgeComponent>(
			TEXT("ClickTargetBridge"));
		SyncClickTargetForTest();
	}

	UPROPERTY()
	TObjectPtr<USceneComponent> Root = nullptr;

	UPROPERTY()
	TObjectPtr<UBoxComponent> ClickBounds = nullptr;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Visual = nullptr;

	UPROPERTY()
	TObjectPtr<UWacomInteractionTargetComponent> ClickInteractionTarget = nullptr;

	UPROPERTY()
	TObjectPtr<UWacomRunWorldInteractionTargetBridgeComponent> ClickTargetBridge = nullptr;

	FName StableIdForTest = TEXT("Run.Generic.Clickable");
	FText InteractPromptForTest = FText::FromString(TEXT("按 E 测试通用交互"));
	FText HoverPromptForTest = FText::FromString(TEXT("点击通用交互"));
	bool bCanInteractForTest = true;
	bool bInteractResultForTest = true;
	FName LastDebugResultForTest = TEXT("Ok");
	int32 TryInteractCountForTest = 0;

	void SyncClickTargetForTest()
	{
		FWacomRunWorldClickableInteractableHelper::BindClickTarget(
			StableIdForTest,
			ClickBounds,
			ClickInteractionTarget,
			ClickTargetBridge);
	}

	virtual FText GetInteractPromptText_Implementation(AWacomPlayerController* /*PC*/) const override
	{
		return InteractPromptForTest;
	}

	virtual FVector GetInteractLocation_Implementation(AWacomPlayerController* /*PC*/) const override
	{
		return GetActorLocation();
	}

	virtual bool CanInteract_Implementation(AWacomPlayerController* /*PC*/) const override
	{
		return bCanInteractForTest;
	}

	virtual bool TryInteract_Implementation(AWacomPlayerController* PC) override
	{
		++TryInteractCountForTest;
		LastInteractingPlayerControllerForTest = PC;
		return bInteractResultForTest;
	}

	virtual FText GetRunWorldClickHoverPrompt_Implementation(AWacomPlayerController* /*PC*/) const override
	{
		return HoverPromptForTest;
	}

	virtual FWacomRunWorldClickableInteractableDebugView
	GetRunWorldClickableDebugView_Implementation(AWacomPlayerController* /*PC*/) const override
	{
		return FWacomRunWorldClickableInteractableHelper::BuildDebugView(
			this,
			StableIdForTest,
			HoverPromptForTest,
			bCanInteractForTest,
			/*bHasCompletionState*/false,
			/*bIsCompleted*/false,
			LastDebugResultForTest,
			ClickInteractionTarget,
			ClickTargetBridge,
			ClickBounds);
	}

	AWacomPlayerController* GetLastInteractingPlayerControllerForTest() const
	{
		return LastInteractingPlayerControllerForTest.Get();
	}

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<AWacomPlayerController> LastInteractingPlayerControllerForTest;
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

	void SetChoiceButtonClassForTest(TSubclassOf<UWacomRunEventChoiceButton> InClass)
	{
		ChoiceButtonWidgetClass = InClass;
	}

	void SetPaymentDropTargetClassForTest(TSubclassOf<UWacomRunMenuDropTargetWidget> InClass)
	{
		PaymentDropTargetWidgetClass = InClass;
	}

	void SetPaymentChoiceMinDesiredWidthForTest(float InMinDesiredWidth)
	{
		PaymentChoiceMinDesiredWidth = InMinDesiredWidth;
	}

	int32 ReadChoiceCount() const
	{
		return GetChoiceCount();
	}

	FRunEventChoiceSnapshot ReadChoiceSnapshot(int32 Index) const
	{
		return GetCachedChoiceSnapshot(Index);
	}

	UWacomRunEventChoiceButton* ReadChoiceButtonWidget(int32 Index) const
	{
		return GetChoiceButtonWidgetForTest(Index);
	}

	TSubclassOf<UWacomRunEventChoiceButton> ReadChoiceButtonWidgetClass() const
	{
		return GetChoiceButtonWidgetClassForTest();
	}

	TSubclassOf<UWacomRunMenuDropTargetWidget> ReadPaymentDropTargetWidgetClass() const
	{
		return GetPaymentDropTargetWidgetClassForTest();
	}

	UWacomRunMenuDropTargetWidget* ReadPaymentDropTarget(int32 Index) const
	{
		return GetPaymentDropTargetForTest(Index);
	}

	float ReadPaymentChoiceMinDesiredWidth() const
	{
		return GetPaymentChoiceMinDesiredWidthForTest();
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
