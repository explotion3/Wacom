// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Fixtures/WacomRunExplorationFixture.h"

#if WITH_AUTOMATION_TESTS

#include "../BackpackScreenTestAccess.h"
#include "../WacomUISettingsTestProbes.h"
#include "../../Settings/WacomSettingsTestAccess.h"
#include "BackpackScreenSpecFixture.h"
#include "CommonInputSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "RunSession.h"
#include "Settings/WacomGameUserSettings.h"
#include "Settings/WacomSettingsSubsystem.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UI/Foundation/WacomUITags.h"
#include "UI/ViewModels/WacomRunViewModelProvider.h"
#include "UObject/StrongObjectPtr.h"

using namespace WacomBackpackScreenSpecFixture;

namespace
{
	bool HasNoActiveSubscriptions(const FWacomBackpackScreenAutomationTestView& View)
	{
		return !View.bHasRunViewModelProviderSubscription
			&& !View.bHasRuntimeSettingsSubscription
			&& !View.bHasCommonInputSubscription
			&& !View.bHasOwningLayerTransitionSubscription;
	}

	bool HasAllActiveSubscriptions(const FWacomBackpackScreenAutomationTestView& View)
	{
		return View.bHasRunViewModelProviderSubscription
			&& View.bHasRuntimeSettingsSubscription
			&& View.bHasCommonInputSubscription
			&& View.bHasOwningLayerTransitionSubscription;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackScreenActiveSubscriptionLifecycleSpec,
	"Wacom.UI.Backpack.Workspace.ActiveSubscriptionLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackScreenActiveSubscriptionLifecycleSpec::RunTest(
	const FString& /*Parameters*/)
{
	UObject* Outer = GetTransientPackage();
	UCardDefinition* CapacityCard = MakeBackpackUiCardForTest(
		Outer, TEXT("Backpack.Lifecycle.Capacity"), 4);
	UCardDefinition* FirstCard = MakeBackpackUiCardForTest(
		Outer, TEXT("Backpack.Lifecycle.First"));
	UCardDefinition* SecondCard = MakeBackpackUiCardForTest(
		Outer, TEXT("Backpack.Lifecycle.Second"));
	UCharacterDefinition* Character = MakeBackpackUiCharacterForTest(
		Outer, { CapacityCard, FirstCard, SecondCard });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Lifecycle Run initializes"),
		InitializeRunSessionForTest(*Run, Character).IsOk());

	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<UWacomRunViewModelProvider> Provider(
		NewObject<UWacomRunViewModelProvider>(GameInstance.Get()));
	TStrongObjectPtr<UWacomGameUserSettings> UserSettings(
		NewObject<UWacomGameUserSettings>());
	UserSettings->SetToDefaults();
	TStrongObjectPtr<UWacomSettingsSubsystem> Settings(
		NewObject<UWacomSettingsSubsystem>(GameInstance.Get()));
	FWacomSettingsSubsystemTestAccess::ConfigureIsolatedSettings(
		*Settings, *UserSettings);
	TStrongObjectPtr<ULocalPlayer> LocalPlayer(NewObject<ULocalPlayer>(GEngine));
	TStrongObjectPtr<UCommonInputSubsystem> CommonInput(
		NewObject<UCommonInputSubsystem>(LocalPlayer.Get()));
	TStrongObjectPtr<UWacomUISettingsPrimaryLayoutProbe> PrimaryLayout(
		NewObject<UWacomUISettingsPrimaryLayoutProbe>(GameInstance.Get()));

	TStrongObjectPtr<UWacomBackpackScreen> Screen(
		FWacomBackpackScreenTestAccess::Create(Outer, Run.Get()));
	TestNotNull(TEXT("Backpack screen exists for subscription lifecycle"), Screen.Get());
	if (!Screen)
	{
		return false;
	}
	const TSharedRef<SWidget> RetainedScreenSlate = Screen->TakeWidget();
	FWacomBackpackScreenTestAccess::SetActiveSubscriptionSources(
		*Screen, Provider.Get(), Settings.Get(), CommonInput.Get(), PrimaryLayout.Get());

	TestTrue(TEXT("Constructed inactive Screen has no external subscriptions"),
		HasNoActiveSubscriptions(FWacomBackpackScreenTestAccess::View(*Screen)));

	FWacomBackpackScreenTestAccess::ActivateWorkspaceScreen(*Screen);
	const FWacomBackpackScreenAutomationTestView FirstActiveView =
		FWacomBackpackScreenTestAccess::View(*Screen);
	TestTrue(TEXT("Activate binds every external source"),
		HasAllActiveSubscriptions(FirstActiveView));
	TestEqual(TEXT("Activate requests focus for the Workspace root"),
		FirstActiveView.WorkspaceFocusRequestCount, 1);

	FWacomBackpackScreenTestAccess::ActivateWorkspaceScreen(*Screen);
	const int32 ActiveSkipCountBeforeBroadcast =
		FWacomBackpackScreenTestAccess::SnapshotRevisionSkipCount(*Screen);
	Provider->OnRunViewModelRefreshedNative.Broadcast();
	TestEqual(TEXT("One active Provider broadcast triggers exactly one refresh"),
		FWacomBackpackScreenTestAccess::SnapshotRevisionSkipCount(*Screen),
		ActiveSkipCountBeforeBroadcast + 1);

	PrimaryLayout->OnLayerTransitioningChangedNative.Broadcast(
		WacomUITags::UI_Layer_GameMenu.GetTag(), true);
	TestFalse(TEXT("Active layer transition disables retained card rendering"),
		FWacomBackpackScreenTestAccess::WorkspaceView(*Screen)
			.bCardFaceRetainedRenderingEnabled);

	const FWacomBackpackScreenAutomationTestView BeforeDeactivate =
		FWacomBackpackScreenTestAccess::View(*Screen);
	const int32 WorkspaceCardCountBeforeDeactivate = BeforeDeactivate.WorkspaceCardCount;
	FWacomBackpackScreenTestAccess::DeactivateWorkspaceScreen(*Screen);
	TestTrue(TEXT("Deactivate removes every external subscription"),
		HasNoActiveSubscriptions(FWacomBackpackScreenTestAccess::View(*Screen)));

	const int32 InactiveSkipCountBeforeBroadcast =
		FWacomBackpackScreenTestAccess::SnapshotRevisionSkipCount(*Screen);
	Provider->OnRunViewModelRefreshedNative.Broadcast();
	TestEqual(TEXT("Inactive Provider broadcast does not refresh the Screen"),
		FWacomBackpackScreenTestAccess::SnapshotRevisionSkipCount(*Screen),
		InactiveSkipCountBeforeBroadcast);

	FWacomLocalSettingsSnapshot SimplifiedSnapshot = UserSettings->MakeSnapshot();
	SimplifiedSnapshot.UIMotionMode = EWacomUIMotionMode::Simplified;
	UserSettings->SetFromSnapshot(SimplifiedSnapshot);
	CommonInput->SetCurrentInputType(ECommonInputType::Gamepad);
	PrimaryLayout->OnLayerTransitioningChangedNative.Broadcast(
		WacomUITags::UI_Layer_GameMenu.GetTag(), false);

	const FRunBackpackStorageSnapshot BeforeDelete = Run->BuildBackpackStorageSnapshot();
	const FRunStorageCardView* CardToDelete = !BeforeDelete.BattleDeckPhysicalCards.IsEmpty()
		? &BeforeDelete.BattleDeckPhysicalCards[0]
		: (!BeforeDelete.Flux.ContentCards.IsEmpty()
			? &BeforeDelete.Flux.ContentCards[0]
			: nullptr);
	TestNotNull(TEXT("Lifecycle fixture has a movable physical card to remove"),
		CardToDelete);
	if (!CardToDelete)
	{
		return false;
	}
	TestTrue(TEXT("Inactive Run mutation succeeds"),
		Run->DeleteCardForGoldByInstance(CardToDelete->Instance.InstanceId));
	TestEqual(TEXT("Inactive Run mutation leaves the hidden Scene untouched"),
		FWacomBackpackScreenTestAccess::WorkspaceCardCount(*Screen),
		WorkspaceCardCountBeforeDeactivate);

	const int32 ApplyCountBeforeCatchUp =
		FWacomBackpackScreenTestAccess::RefreshApplyCount(*Screen);
	FWacomBackpackScreenTestAccess::ActivateWorkspaceScreen(*Screen);
	TestTrue(TEXT("Reactivation restores every external subscription"),
		HasAllActiveSubscriptions(FWacomBackpackScreenTestAccess::View(*Screen)));
	TestEqual(TEXT("Reactivation applies the missed Run revision exactly once"),
		FWacomBackpackScreenTestAccess::RefreshApplyCount(*Screen),
		ApplyCountBeforeCatchUp + 1);
	TestEqual(TEXT("Reactivation catches up the physical card removal"),
		FWacomBackpackScreenTestAccess::WorkspaceCardCount(*Screen),
		WorkspaceCardCountBeforeDeactivate - 1);
	TestTrue(TEXT("Reactivation applies the latest simplified-motion setting"),
		FWacomBackpackScreenTestAccess::WorkspaceView(*Screen).bSimplifiedMotion);
	TestEqual(TEXT("Reactivation applies the latest CommonInput type"),
		FWacomBackpackScreenTestAccess::View(*Screen).CurrentInputType,
		ECommonInputType::Gamepad);
	TestTrue(TEXT("Reactivation catches up the current non-transitioning layer state"),
		FWacomBackpackScreenTestAccess::WorkspaceView(*Screen)
			.bCardFaceRetainedRenderingEnabled);
	TestEqual(TEXT("Reactivation requests Workspace focus again"),
		FWacomBackpackScreenTestAccess::View(*Screen).WorkspaceFocusRequestCount,
		2);

	const int32 SceneBindCountBeforeUnchangedCycle =
		FWacomBackpackScreenTestAccess::WorkspaceView(*Screen).WorkspaceSceneBindCount;
	FWacomBackpackScreenTestAccess::DeactivateWorkspaceScreen(*Screen);
	FWacomBackpackScreenTestAccess::ActivateWorkspaceScreen(*Screen);
	TestEqual(TEXT("Unchanged reactivation does not reconcile the Scene"),
		FWacomBackpackScreenTestAccess::WorkspaceView(*Screen).WorkspaceSceneBindCount,
		SceneBindCountBeforeUnchangedCycle);

	const int32 FinalSkipCountBeforeBroadcast =
		FWacomBackpackScreenTestAccess::SnapshotRevisionSkipCount(*Screen);
	Provider->OnRunViewModelRefreshedNative.Broadcast();
	TestEqual(TEXT("Repeated activation cycles do not duplicate Provider bindings"),
		FWacomBackpackScreenTestAccess::SnapshotRevisionSkipCount(*Screen),
		FinalSkipCountBeforeBroadcast + 1);

	FWacomBackpackScreenTestAccess::DeactivateWorkspaceScreen(*Screen);
	TestTrue(TEXT("Final deactivation remains idempotently unbound"),
		HasNoActiveSubscriptions(FWacomBackpackScreenTestAccess::View(*Screen)));
	FWacomBackpackScreenTestAccess::DestructWorkspaceScreen(*Screen);
	TestTrue(TEXT("Destruct cleanup remains idempotently unbound"),
		HasNoActiveSubscriptions(FWacomBackpackScreenTestAccess::View(*Screen)));
	return true;
}

#endif
