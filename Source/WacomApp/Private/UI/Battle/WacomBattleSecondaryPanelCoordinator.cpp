// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleSecondaryPanelCoordinator.h"

#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "GameFramework/PlayerController.h"
#include "UI/Battle/WacomBattleCombatLogDetailsScreen.h"
#include "UI/Battle/WacomBattleHUDRuntime.h"
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

	// Menu ownership begins only after all active targeting/drag state was neutral-cancelled.
	Runtime.CancelTargetSelect();
	if (UWacomFirstPersonCardAnchorComponent* Anchor = Runtime.ResolveActiveFirstPersonCardAnchor())
	{
		Anchor->CancelFirstPersonCardDragGesture(true);
	}
	Runtime.HideCardDetailPanel();
	Runtime.SetSecondaryPanelOpen(true);

	bShuttingDown = false;
	bPushPending = true;
	const int32 RequestGeneration = ++Generation;
	TWeakPtr<FWacomBattleSecondaryPanelCoordinator> WeakThis = AsShared();

	FWacomAsyncWidgetPushRequest Request;
	Request.LayerTag = LayerTag;
	Request.WidgetTag = WacomUITags::UI_Widget_BattleCombatLogDetailsScreen.GetTag();
	Request.FallbackClass = UWacomBattleCombatLogDetailsScreen::StaticClass();
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
		UWacomBattleCombatLogDetailsScreen* Screen = Cast<UWacomBattleCombatLogDetailsScreen>(&Pushed);
		if (!Screen)
		{
			OutFailureReason = TEXT("WrongCombatLogDetailsScreenClass");
			return false;
		}
		StrongThis->AttachScreen(*Screen);
		return true;
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

	if (UWacomBattleCombatLogDetailsScreen* Screen = ActiveScreen.Get())
	{
		Screen->OnSecondaryPanelClosedNative().RemoveAll(this);
		Screen->OnDetailsModeChangedNative().RemoveAll(this);
		ActiveScreen.Reset();
		if (Screen->IsActivated())
		{
			Screen->DeactivateWidget();
		}
	}
	ReleaseCommandGate();
	if (bResetBattlePreference)
	{
		bShowCombatLogDetails = false;
	}
	bShuttingDown = false;
}

void FWacomBattleSecondaryPanelCoordinator::AttachScreen(
	UWacomBattleCombatLogDetailsScreen& Screen)
{
	ActiveScreen = &Screen;
	Screen.OnSecondaryPanelClosedNative().RemoveAll(this);
	Screen.OnSecondaryPanelClosedNative().AddSP(AsShared(), &FWacomBattleSecondaryPanelCoordinator::HandleScreenClosed);
	Screen.OnDetailsModeChangedNative().RemoveAll(this);
	Screen.OnDetailsModeChangedNative().AddSP(AsShared(), &FWacomBattleSecondaryPanelCoordinator::HandleDetailsModeChanged);
	Screen.SetCombatLogContext(Runtime.GetBattleCombatLogDetailsHistory(), bShowCombatLogDetails);
}

void FWacomBattleSecondaryPanelCoordinator::HandleScreenClosed()
{
	if (bShuttingDown)
	{
		return;
	}
	if (UWacomBattleCombatLogDetailsScreen* Screen = ActiveScreen.Get())
	{
		Screen->OnSecondaryPanelClosedNative().RemoveAll(this);
		Screen->OnDetailsModeChangedNative().RemoveAll(this);
	}
	ActiveScreen.Reset();
	bPushPending = false;
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
			TEXT("[BattleSecondaryPanel] Combat Log details push failed: %s"),
			*FailureReason.ToString());
	}
	ActiveScreen.Reset();
	ReleaseCommandGate();
}

void FWacomBattleSecondaryPanelCoordinator::ReleaseCommandGate()
{
	Runtime.SetSecondaryPanelOpen(false);
}

bool FWacomBattleSecondaryPanelCoordinator::IsCurrentRequest(int32 RequestGeneration) const
{
	return !bShuttingDown && bPushPending && Generation == RequestGeneration;
}
