// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleSecondaryPanelCoordinator.h"

#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "GameFramework/PlayerController.h"
#include "Session/BattleSession.h"
#include "UI/Battle/WacomBattleCardPileDetailsScreen.h"
#include "UI/Battle/WacomBattleCombatLogDetailsScreen.h"
#include "UI/Battle/WacomBattleViewportLayerPolicy.h"
#include "UI/Battle/WacomBattleHUDRuntime.h"
#include "UI/Battle/WacomBattleSecondaryPanelScreenBase.h"
#include "UI/Foundation/WacomGameUIManagerSubsystem.h"
#include "UI/Foundation/WacomUITags.h"

FWacomBattleSecondaryPanelCoordinator::FWacomBattleSecondaryPanelCoordinator(
	FWacomBattleHUDRuntime& InRuntime)
	: Runtime(InRuntime)
{
}

FWacomBattleSecondaryPanelCoordinator::~FWacomBattleSecondaryPanelCoordinator()
{
	Shutdown(false);
}

bool FWacomBattleSecondaryPanelCoordinator::RequestOpenCombatLogDetails()
{
	return BeginPush(
		EWacomBattleSecondaryPanelKind::CombatLog,
		WacomUITags::UI_Widget_BattleCombatLogDetailsScreen.GetTag(),
		UWacomBattleCombatLogDetailsScreen::StaticClass());
}

bool FWacomBattleSecondaryPanelCoordinator::RequestOpenCardPileDetails(
	EWacomBattlePileDetailsTab InitialTab)
{
	UBattleSession* Session = Runtime.GetSession();
	if (!Session
		|| Runtime.IsBattlePresentationBusy()
		|| !Runtime.CanSubmitPlayerActionCommand())
	{
		return false;
	}
	PendingPileSnapshot = Session->BuildPileInspectionSnapshot();
	PendingPileTab = InitialTab;
	const bool bStarted = BeginPush(
		EWacomBattleSecondaryPanelKind::CardPile,
		WacomUITags::UI_Widget_BattleCardPileDetailsScreen.GetTag(),
		UWacomBattleCardPileDetailsScreen::StaticClass());
	if (!bStarted)
	{
		PendingPileSnapshot = FBattlePileInspectionSnapshot();
	}
	return bStarted;
}

bool FWacomBattleSecondaryPanelCoordinator::BeginPush(
	EWacomBattleSecondaryPanelKind Kind,
	FGameplayTag WidgetTag,
	TSubclassOf<UCommonActivatableWidget> FallbackClass)
{
	if (bPushPending || ActiveScreen.IsValid() || !Runtime.GetSession())
	{
		return false;
	}

	APlayerController* PlayerController = Runtime.GetOwningPlayer();
	UGameInstance* GameInstance = PlayerController ? PlayerController->GetGameInstance() : nullptr;
	UWacomGameUIManagerSubsystem* UIManager = GameInstance
		? GameInstance->GetSubsystem<UWacomGameUIManagerSubsystem>()
		: nullptr;
	if (!PlayerController || !UIManager)
	{
		return false;
	}

	UIManager->EnsurePrimaryLayout(PlayerController);
	const FGameplayTag LayerTag = WacomUITags::UI_Layer_GameMenu.GetTag();
	if (!UIManager->GetPrimaryLayout() || UIManager->HasPendingAsyncPushToLayer(LayerTag))
	{
		return false;
	}

	Runtime.CancelTargetSelect();
	UWacomFirstPersonCardAnchorComponent* ActiveAnchor =
		Runtime.ResolveActiveFirstPersonCardAnchor();
	if (ActiveAnchor)
	{
		ActiveAnchor->CancelFirstPersonCardDragGesture(true);
	}
	Runtime.HideCardDetailPanel();

	const int32 RequestedViewportZOrder = ActiveAnchor
		? WacomBattleViewportLayerPolicy::ResolveSecondaryPanelZOrder(
			ActiveAnchor->CardLayerZOrder)
		: WacomBattleViewportLayerPolicy::SecondaryPanelZOrder;
	ViewportDepthLeaseOwner = UIManager;
	ViewportDepthLeaseId =
		UIManager->AcquirePrimaryLayoutViewportZOrderLease(RequestedViewportZOrder);
	Runtime.SetSecondaryPanelOpen(true);

	bShuttingDown = false;
	bPushPending = true;
	PendingKind = Kind;
	const int32 RequestGeneration = ++Generation;
	TWeakPtr<FWacomBattleSecondaryPanelCoordinator> WeakThis = AsShared();

	FWacomAsyncWidgetPushRequest Request;
	Request.LayerTag = LayerTag;
	Request.WidgetTag = WidgetTag;
	Request.FallbackClass = FallbackClass;
	Request.OwningPlayer = PlayerController;
	Request.bLogMissingEntry = true;
	Request.CanPush = [WeakThis, RequestGeneration]()
	{
		const TSharedPtr<FWacomBattleSecondaryPanelCoordinator> StrongThis = WeakThis.Pin();
		return StrongThis.IsValid() && StrongThis->IsCurrentRequest(RequestGeneration)
			&& StrongThis->Runtime.GetSession() != nullptr;
	};
	Request.AfterPush = [WeakThis, RequestGeneration](UCommonActivatableWidget& Pushed, FName& OutFailureReason)
	{
		const TSharedPtr<FWacomBattleSecondaryPanelCoordinator> StrongThis = WeakThis.Pin();
		if (!StrongThis.IsValid() || !StrongThis->IsCurrentRequest(RequestGeneration))
		{
			OutFailureReason = TEXT("StaleBattleSecondaryPanelRequest");
			return false;
		}
		return StrongThis->AttachPushedScreen(Pushed, OutFailureReason);
	};
	Request.Rollback = [WeakThis, RequestGeneration](FName FailureReason)
	{
		if (const TSharedPtr<FWacomBattleSecondaryPanelCoordinator> StrongThis = WeakThis.Pin())
		{
			if (StrongThis->IsCurrentRequest(RequestGeneration))
			{
				StrongThis->HandlePushCompleted(false, FailureReason);
			}
		}
	};
	Request.OnComplete = [WeakThis, RequestGeneration](const FWacomAsyncWidgetPushResult& Result)
	{
		if (const TSharedPtr<FWacomBattleSecondaryPanelCoordinator> StrongThis = WeakThis.Pin())
		{
			if (StrongThis->IsCurrentRequest(RequestGeneration))
			{
				StrongThis->HandlePushCompleted(Result.bSucceeded, Result.FailureReason);
			}
		}
	};
	UIManager->PushRegisteredWidgetToLayerAsync(MoveTemp(Request));
	return true;
}

bool FWacomBattleSecondaryPanelCoordinator::AttachPushedScreen(
	UCommonActivatableWidget& Pushed,
	FName& OutFailureReason)
{
	switch (PendingKind)
	{
	case EWacomBattleSecondaryPanelKind::CombatLog:
		if (UWacomBattleCombatLogDetailsScreen* Screen = Cast<UWacomBattleCombatLogDetailsScreen>(&Pushed))
		{
			AttachCombatLogScreen(*Screen);
			return true;
		}
		OutFailureReason = TEXT("WrongCombatLogDetailsScreenClass");
		return false;
	case EWacomBattleSecondaryPanelKind::CardPile:
		if (UWacomBattleCardPileDetailsScreen* Screen = Cast<UWacomBattleCardPileDetailsScreen>(&Pushed))
		{
			AttachCardPileScreen(*Screen);
			return true;
		}
		OutFailureReason = TEXT("WrongCardPileDetailsScreenClass");
		return false;
	default:
		OutFailureReason = TEXT("MissingBattleSecondaryPanelKind");
		return false;
	}
}

void FWacomBattleSecondaryPanelCoordinator::Shutdown(bool bResetBattlePreference)
{
	bShuttingDown = true;
	++Generation;
	if (bPushPending)
	{
		if (APlayerController* PlayerController = Runtime.GetOwningPlayer())
		{
			if (UGameInstance* GameInstance = PlayerController->GetGameInstance())
			{
				if (UWacomGameUIManagerSubsystem* UIManager = GameInstance->GetSubsystem<UWacomGameUIManagerSubsystem>())
				{
					UIManager->CancelPendingAsyncPushToLayer(WacomUITags::UI_Layer_GameMenu.GetTag());
				}
			}
		}
	}
	bPushPending = false;

	if (UWacomBattleSecondaryPanelScreenBase* Screen = ActiveScreen.Get())
	{
		Screen->OnSecondaryPanelClosedNative().RemoveAll(this);
		if (UWacomBattleCombatLogDetailsScreen* CombatLogScreen = Cast<UWacomBattleCombatLogDetailsScreen>(Screen))
		{
			CombatLogScreen->OnDetailsModeChangedNative().RemoveAll(this);
		}
		ActiveScreen.Reset();
		if (Screen->IsActivated())
		{
			Screen->DeactivateWidget();
		}
	}
	PendingKind = EWacomBattleSecondaryPanelKind::None;
	PendingPileSnapshot = FBattlePileInspectionSnapshot();
	ReleaseCommandGate();
	if (bResetBattlePreference)
	{
		bShowCombatLogDetails = false;
	}
	bShuttingDown = false;
}

void FWacomBattleSecondaryPanelCoordinator::AttachCombatLogScreen(
	UWacomBattleCombatLogDetailsScreen& Screen)
{
	ActiveScreen = &Screen;
	Screen.OnSecondaryPanelClosedNative().RemoveAll(this);
	Screen.OnSecondaryPanelClosedNative().AddSP(AsShared(), &FWacomBattleSecondaryPanelCoordinator::HandleScreenClosed);
	Screen.OnDetailsModeChangedNative().RemoveAll(this);
	Screen.OnDetailsModeChangedNative().AddSP(AsShared(), &FWacomBattleSecondaryPanelCoordinator::HandleDetailsModeChanged);
	Screen.SetCombatLogContext(Runtime.GetBattleCombatLogDetailsHistory(), bShowCombatLogDetails);
}

void FWacomBattleSecondaryPanelCoordinator::AttachCardPileScreen(
	UWacomBattleCardPileDetailsScreen& Screen)
{
	ActiveScreen = &Screen;
	Screen.OnSecondaryPanelClosedNative().RemoveAll(this);
	Screen.OnSecondaryPanelClosedNative().AddSP(AsShared(), &FWacomBattleSecondaryPanelCoordinator::HandleScreenClosed);
	if (const UWacomFirstPersonCardAnchorComponent* ActiveAnchor =
		Runtime.ResolveActiveFirstPersonCardAnchor())
	{
		Screen.SetRestingHandCardPresentationProfile(
			ActiveAnchor->BuildRestingCardPresentationProfile());
	}
	else
	{
		Screen.SetRestingHandCardPresentationProfile({});
	}
	Screen.SetPileDetailsContext(PendingPileSnapshot, PendingPileTab);
	bCardPileHandHidden = Runtime.SetFirstPersonBattleHandPresentationVisible(false);
}

void FWacomBattleSecondaryPanelCoordinator::HandleScreenClosed()
{
	if (bShuttingDown)
	{
		return;
	}
	if (UWacomBattleSecondaryPanelScreenBase* Screen = ActiveScreen.Get())
	{
		Screen->OnSecondaryPanelClosedNative().RemoveAll(this);
		if (UWacomBattleCombatLogDetailsScreen* CombatLogScreen = Cast<UWacomBattleCombatLogDetailsScreen>(Screen))
		{
			CombatLogScreen->OnDetailsModeChangedNative().RemoveAll(this);
		}
	}
	ActiveScreen.Reset();
	bPushPending = false;
	PendingKind = EWacomBattleSecondaryPanelKind::None;
	PendingPileSnapshot = FBattlePileInspectionSnapshot();
	ReleaseCommandGate();
}

void FWacomBattleSecondaryPanelCoordinator::HandleDetailsModeChanged(bool bShowDetails)
{
	bShowCombatLogDetails = bShowDetails;
}

void FWacomBattleSecondaryPanelCoordinator::HandlePushCompleted(
	bool bSucceeded,
	FName FailureReason)
{
	bPushPending = false;
	if (bSucceeded && ActiveScreen.IsValid())
	{
		return;
	}
	if (!FailureReason.IsNone())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[BattleSecondaryPanel] Secondary panel push failed: %s"),
			*FailureReason.ToString());
	}
	ActiveScreen.Reset();
	PendingKind = EWacomBattleSecondaryPanelKind::None;
	PendingPileSnapshot = FBattlePileInspectionSnapshot();
	ReleaseCommandGate();
}

void FWacomBattleSecondaryPanelCoordinator::ReleaseCommandGate()
{
	if (bCardPileHandHidden)
	{
		Runtime.SetFirstPersonBattleHandPresentationVisible(true);
		bCardPileHandHidden = false;
	}
	ReleaseViewportDepthLease();
	Runtime.SetSecondaryPanelOpen(false);
}

void FWacomBattleSecondaryPanelCoordinator::ReleaseViewportDepthLease()
{
	if (ViewportDepthLeaseId != 0)
	{
		if (UWacomGameUIManagerSubsystem* UIManager = ViewportDepthLeaseOwner.Get())
		{
			UIManager->ReleasePrimaryLayoutViewportZOrderLease(ViewportDepthLeaseId);
		}
	}
	ViewportDepthLeaseOwner.Reset();
	ViewportDepthLeaseId = 0;
}

bool FWacomBattleSecondaryPanelCoordinator::IsCurrentRequest(int32 RequestGeneration) const
{
	return !bShuttingDown && bPushPending && Generation == RequestGeneration;
}
