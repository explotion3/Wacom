// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyActionPlayback.h"
#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyHostAnimationStyle.h"
#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Fixtures/BattleTestFixtures.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "PaperSprite.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/Battle/PlayerStatusBar.h"
#include "UI/BattleWidgetSpecReceiver.h"
#include "UI/Common/WacomProgressBar.h"
#include "BattleHUDTestHarness.h"

#include "Blueprint/WidgetTree.h"
#include "Engine/Engine.h"
#include "EngineGlobals.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"

namespace WacomBattleEnemyActionImpactSpec
{
	UWorld* FindAutomationWorld()
	{
		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (UWorld* World = Context.World())
				{
					return World;
				}
			}
		}
		return GWorld;
	}

	UPaperFlipbook* MakeFlipbook(UObject* Outer, FName Name, int32 FrameRun = 2)
	{
		UPaperSprite* FrameSprite = NewObject<UPaperSprite>(Outer);
		UPaperFlipbook* Flipbook = NewObject<UPaperFlipbook>(Outer, Name);
		FScopedFlipbookMutator Mutator(Flipbook);
		Mutator.FramesPerSecond = 10.0f;
		FPaperFlipbookKeyFrame KeyFrame;
		KeyFrame.Sprite = FrameSprite;
		KeyFrame.FrameRun = FrameRun;
		Mutator.KeyFrames.Add(KeyFrame);
		return Flipbook;
	}

	UPaperFlipbook* ConfigureAnimatedHost(AWacomBattleEnemyActor& Host)
	{
		Host.HostAuthoringMode = EWacomBattleEnemyHostAuthoringMode::SimpleHostVisual;
		Host.HostVisualMode = EWacomBattleEnemyHostVisualMode::Flipbook;
		Host.HostFlipbook = MakeFlipbook(&Host, TEXT("Impact_Idle"), 4);
		Host.bLoopHostFlipbook = true;
		Host.bAutoPlayHostFlipbook = true;
		Host.HostAnimationStyle = NewObject<UWacomBattleEnemyHostAnimationStyle>(&Host);
		UPaperFlipbook* Action = MakeFlipbook(&Host, TEXT("Impact_Action"), 2);
		Host.HostAnimationStyle->DefaultActionClip.Flipbook = Action;
		Host.HostAnimationStyle->DefaultActionClip.PlayRate = 1.0f;
		Host.HostAnimationStyle->DefaultActionClip.ImpactNormalizedTime = 0.5f;
		Host.RefreshBattleEnemyPartAuthoringState();
		return Action;
	}

	AWacomBattleEnemyActor* SpawnAnimatedHost(UWorld& World)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.ObjectFlags |= RF_Transient;
		AWacomBattleEnemyActor* Host = World.SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
		if (!Host)
		{
			return nullptr;
		}

		ConfigureAnimatedHost(*Host);
		return Host;
	}

	TArray<FName> BuildDefinitionPartIds(const UEnemyDefinition& Enemy)
	{
		TArray<FName> PartIds;
		for (const FEnemyPartSlot& PartSlot : Enemy.Parts)
		{
			if (PartSlot.PartDef)
			{
				PartIds.Add(PartSlot.PartDef->PartId);
			}
		}
		return PartIds;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyActionImpactDoubleBarrierSpec,
	"Wacom.UI.Battle.EnemyActionImpact.ImpactPrecedesCompletionExactlyOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyActionImpactDoubleBarrierSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyActionImpactSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleEnemyActor* Host = SpawnAnimatedHost(*World);
	if (!TestNotNull(TEXT("Animated Host"), Host))
	{
		return false;
	}
	ON_SCOPE_EXIT { if (IsValid(Host)) { Host->Destroy(); } };

	TArray<FName> CallbackOrder;
	Host->PlayRuntimeHostActionAnimation(
		TEXT("Intent.Attack"),
		FWacomBattleEnemyActionPlaybackCallbacks{
			[&CallbackOrder]() { CallbackOrder.Add(TEXT("Impact")); },
			[&CallbackOrder]() { CallbackOrder.Add(TEXT("Complete")); }});

	const FWacomBattleSceneEnemyDebugView StartedView = Host->GetBattleSceneEnemyDebugView();
	TestTrue(TEXT("Playback is pending"), StartedView.bHostAnimationPlaybackActive);
	TestFalse(TEXT("Impact has not fired on start"), StartedView.bHostAnimationImpactFired);
	TestEqual(TEXT("No callback fires synchronously for a valid clip"), CallbackOrder.Num(), 0);

	++GFrameCounter;
	World->GetTimerManager().Tick(0.01f);
	TestEqual(TEXT("Impact remains pending before its marker"), CallbackOrder.Num(), 0);
	++GFrameCounter;
	World->GetTimerManager().Tick(0.11f);
	TestEqual(TEXT("Marker fires Impact once"), CallbackOrder.Num(), 1);
	if (CallbackOrder.Num() == 1)
	{
		TestEqual(TEXT("First callback is Impact"), CallbackOrder[0], FName(TEXT("Impact")));
	}

	UPaperFlipbookComponent* Visual = Host->GetGeneratedHostFlipbookVisualComponent();
	if (!TestNotNull(TEXT("Host Flipbook component"), Visual))
	{
		return false;
	}
	Visual->OnFinishedPlaying.Broadcast();
	Visual->OnFinishedPlaying.Broadcast();
	TestEqual(TEXT("Completion and duplicate engine signals execute callbacks exactly once"), CallbackOrder.Num(), 2);
	if (CallbackOrder.Num() == 2)
	{
		TestEqual(TEXT("Second callback is Complete"), CallbackOrder[1], FName(TEXT("Complete")));
	}
	const FWacomBattleSceneEnemyDebugView CompletedView = Host->GetBattleSceneEnemyDebugView();
	TestEqual(TEXT("Impact counter increments once"), CompletedView.HostAnimationImpactCount, 1);
	TestFalse(TEXT("Playback is no longer active"), CompletedView.bHostAnimationPlaybackActive);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyActionImpactFallbackSpec,
	"Wacom.UI.Battle.EnemyActionImpact.FallbackAndWatchdogDoNotBlock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyActionImpactFallbackSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyActionImpactSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleEnemyActor* Host = SpawnAnimatedHost(*World);
	if (!TestNotNull(TEXT("Animated Host"), Host))
	{
		return false;
	}
	ON_SCOPE_EXIT { if (IsValid(Host)) { Host->Destroy(); } };

	TArray<FName> WatchdogOrder;
	Host->HostAnimationStyle->DefaultActionClip.ImpactNormalizedTime = 1.0f;
	Host->PlayRuntimeHostActionAnimation(
		TEXT("Intent.Attack"),
		FWacomBattleEnemyActionPlaybackCallbacks{
			[&WatchdogOrder]() { WatchdogOrder.Add(TEXT("Impact")); },
			[&WatchdogOrder]() { WatchdogOrder.Add(TEXT("Complete")); }});
	++GFrameCounter;
	World->GetTimerManager().Tick(0.01f);
	++GFrameCounter;
	World->GetTimerManager().Tick(0.50f);
	TestEqual(TEXT("Watchdog forces both barriers"), WatchdogOrder.Num(), 2);
	if (WatchdogOrder.Num() == 2)
	{
		TestEqual(TEXT("Watchdog forces Impact first"), WatchdogOrder[0], FName(TEXT("Impact")));
		TestEqual(TEXT("Watchdog completes second"), WatchdogOrder[1], FName(TEXT("Complete")));
	}
	const FWacomBattleSceneEnemyDebugView WatchdogView = Host->GetBattleSceneEnemyDebugView();
	TestEqual(TEXT("Watchdog completion is diagnosed"), WatchdogView.HostAnimationWatchdogCompletionCount, 1);
	TestEqual(TEXT("Impact remains exactly once through watchdog completion"), WatchdogView.HostAnimationImpactCount, 1);

	TArray<FName> CancelOrder;
	Host->HostAnimationStyle->DefaultActionClip.ImpactNormalizedTime = 0.5f;
	Host->PlayRuntimeHostActionAnimation(
		TEXT("Intent.Attack"),
		FWacomBattleEnemyActionPlaybackCallbacks{
			[&CancelOrder]() { CancelOrder.Add(TEXT("Impact")); },
			[&CancelOrder]() { CancelOrder.Add(TEXT("Complete")); }});
	Host->CancelRuntimeHostAnimation();
	TestEqual(TEXT("Lifecycle cancellation releases completion without applying stale Impact"),
		CancelOrder.Num(),
		1);
	if (CancelOrder.Num() == 1)
	{
		TestEqual(TEXT("Cancellation only completes the barrier"),
			CancelOrder[0],
			FName(TEXT("Complete")));
	}

	Host->HostAnimationStyle = nullptr;
	TArray<FName> MissingStyleOrder;
	Host->PlayRuntimeHostActionAnimation(
		TEXT("Intent.Attack"),
		FWacomBattleEnemyActionPlaybackCallbacks{
			[&MissingStyleOrder]() { MissingStyleOrder.Add(TEXT("Impact")); },
			[&MissingStyleOrder]() { MissingStyleOrder.Add(TEXT("Complete")); }});
	TestEqual(TEXT("Missing Style completes both callbacks synchronously"), MissingStyleOrder.Num(), 2);
	if (MissingStyleOrder.Num() == 2)
	{
		TestEqual(TEXT("Fallback still delivers Impact first"), MissingStyleOrder[0], FName(TEXT("Impact")));
		TestEqual(TEXT("Fallback delivers Complete second"), MissingStyleOrder[1], FName(TEXT("Complete")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyActionImpactWaitIntegrationSpec,
	"Wacom.UI.Battle.EnemyActionImpact.WaitAppliesCombatSnapshotAtImpact",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyActionImpactWaitIntegrationSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyActionImpactSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UCardDefinition* Noop = Fixture.MakeNoopCard(0);
	UCharacterDefinition* Character = Fixture.MakeCharacter(
		Fixture.MakeNoopCard(0),
		Fixture.MakeNoopCard(0),
		{ Noop, Noop, Noop, Noop, Noop });
	UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemyWithIntentDamage(
		/*Hp*/30,
		/*Initiative*/2,
		/*IntentResist*/0,
		/*Damage*/4);
	UBattleSession* Session = Fixture.CreateSession(Character, Enemy, /*Seed*/31);

	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDOnly(World);
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get()))
	{
		return false;
	}
	FWacomBattleHUDTestSceneEnemyHost& SceneEnemy =
		Harness->AttachSceneEnemyHost(Enemy, BuildDefinitionPartIds(*Enemy));
	if (!TestNotNull(TEXT("Scene enemy Host"), SceneEnemy.Host))
	{
		return false;
	}
	UPaperFlipbook* Action = ConfigureAnimatedHost(*SceneEnemy.Host);
	UPlayerStatusBar* StatusBar = Harness->AttachPlayerStatusBar();
	if (!TestNotNull(TEXT("Player status bar"), StatusBar))
	{
		return false;
	}
	Harness->SetSession(Session);
	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	HUD->SetBattleSceneEnemyHostsForTest({ SceneEnemy.Host });
	UWacomProgressBar* HpBar = Cast<UWacomProgressBar>(
		StatusBar->WidgetTree
			? StatusBar->WidgetTree->FindWidget(TEXT("HpBar"))
			: nullptr);
	UPaperFlipbookComponent* Visual =
		SceneEnemy.Host->GetGeneratedHostFlipbookVisualComponent();
	if (!TestNotNull(TEXT("Player HP bar"), HpBar)
		|| !TestNotNull(TEXT("Host Flipbook component"), Visual))
	{
		return false;
	}

	const FBattleSnapshot Before = Session->BuildSnapshot();
	HUD->OnWaitRequested();
	const FBattleSnapshot After = Session->BuildSnapshot();
	TestEqual(TEXT("Wait resolves enemy damage in the rule session"),
		After.Player.CurrentHp,
		Before.Player.CurrentHp - 4);
	for (int32 Step = 0;
		Step < 32 && HUD->IsBattlePresentationBusy() && Visual->GetFlipbook() != Action;
		++Step)
	{
		HUD->AdvanceBattlePresentationQueueForTest();
	}
	TestEqual(TEXT("Wait starts the Host action clip"), Visual->GetFlipbook(), Action);
	TestEqual(TEXT("Wait keeps the action-before HP visible"),
		HpBar->GetCurrent(),
		Before.Player.CurrentHp);

	++GFrameCounter;
	World->GetTimerManager().Tick(0.01f);
	++GFrameCounter;
	World->GetTimerManager().Tick(0.11f);
	TestEqual(TEXT("Wait applies the action-after HP at Impact"),
		HpBar->GetCurrent(),
		After.Player.CurrentHp);
	TestTrue(TEXT("Wait still holds the queue until animation completion"),
		HUD->IsBattlePresentationBusy());
	Visual->OnFinishedPlaying.Broadcast();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyActionImpactEndTurnIntegrationSpec,
	"Wacom.UI.Battle.EnemyActionImpact.EndTurnPreservesCombatBaselineUntilImpact",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyActionImpactEndTurnIntegrationSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyActionImpactSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UCardDefinition* Noop = Fixture.MakeNoopCard(0);
	UCharacterDefinition* Character = Fixture.MakeCharacter(
		Fixture.MakeNoopCard(0),
		Fixture.MakeNoopCard(0),
		{ Noop, Noop, Noop, Noop, Noop });
	UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemyWithIntentDamage(
		/*Hp*/30,
		/*Initiative*/20,
		/*IntentResist*/0,
		/*Damage*/4);
	UBattleSession* Session = Fixture.CreateSession(Character, Enemy, /*Seed*/32);

	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDOnly(World);
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get()))
	{
		return false;
	}
	FWacomBattleHUDTestSceneEnemyHost& SceneEnemy =
		Harness->AttachSceneEnemyHost(Enemy, BuildDefinitionPartIds(*Enemy));
	if (!TestNotNull(TEXT("Scene enemy Host"), SceneEnemy.Host))
	{
		return false;
	}
	UPaperFlipbook* Action = ConfigureAnimatedHost(*SceneEnemy.Host);
	UPlayerStatusBar* StatusBar = Harness->AttachPlayerStatusBar();
	if (!TestNotNull(TEXT("Player status bar"), StatusBar))
	{
		return false;
	}
	Harness->SetSession(Session);
	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	HUD->SetBattleSceneEnemyHostsForTest({ SceneEnemy.Host });
	UWacomProgressBar* HpBar = Cast<UWacomProgressBar>(
		StatusBar->WidgetTree
			? StatusBar->WidgetTree->FindWidget(TEXT("HpBar"))
			: nullptr);
	UPaperFlipbookComponent* Visual =
		SceneEnemy.Host->GetGeneratedHostFlipbookVisualComponent();
	if (!TestNotNull(TEXT("Player HP bar"), HpBar)
		|| !TestNotNull(TEXT("Host Flipbook component"), Visual))
	{
		return false;
	}

	const FBattleSnapshot Before = Session->BuildSnapshot();
	HUD->OnEndTurnRequested();
	const FBattleSnapshot After = Session->BuildSnapshot();
	TestEqual(TEXT("EndTurn resolves enemy damage in the rule session"),
		After.Player.CurrentHp,
		Before.Player.CurrentHp - 4);
	for (int32 Step = 0;
		Step < 64 && HUD->IsBattlePresentationBusy() && Visual->GetFlipbook() != Action;
		++Step)
	{
		HUD->AdvanceBattlePresentationQueueForTest();
	}
	TestEqual(TEXT("EndTurn starts the Host action clip"), Visual->GetFlipbook(), Action);
	TestEqual(TEXT("EndTurn keeps the action-before HP visible"),
		HpBar->GetCurrent(),
		Before.Player.CurrentHp);

	++GFrameCounter;
	World->GetTimerManager().Tick(0.01f);
	++GFrameCounter;
	World->GetTimerManager().Tick(0.11f);
	TestEqual(TEXT("EndTurn applies the action-after HP at Impact"),
		HpBar->GetCurrent(),
		After.Player.CurrentHp);
	TestTrue(TEXT("EndTurn still holds the action barrier until completion"),
		SceneEnemy.Host->GetBattleSceneEnemyDebugView().bHostAnimationPlaybackActive);
	Visual->OnFinishedPlaying.Broadcast();
	TestFalse(TEXT("EndTurn releases the action playback barrier on completion"),
		SceneEnemy.Host->GetBattleSceneEnemyDebugView().bHostAnimationPlaybackActive);
	return true;
}
