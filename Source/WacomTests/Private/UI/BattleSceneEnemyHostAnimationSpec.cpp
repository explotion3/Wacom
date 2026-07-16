// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyHostAnimationStyle.h"
#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Events/BattleEvent.h"
#include "Fixtures/BattleTestFixtures.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "PaperSprite.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "BattleHUDTestHarness.h"
#include "UI/BattleWidgetSpecReceiver.h"

#include "Engine/Engine.h"
#include "EngineGlobals.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"

namespace WacomBattleSceneEnemyHostAnimationSpec
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

	FWacomInteractionTargetHandle MakeWorldTargetHandle(const FEnemyPartSnapshot& Part)
	{
		return FWacomInteractionTargetHandle::ForWorldTarget(
			Part.InstanceId,
			nullptr,
			FVector::ZeroVector,
			FVector2D::ZeroVector,
			WacomTags::Interaction_Target_Battle_EnemyPart,
			Part.Definition ? Part.Definition->PartId : NAME_None,
			Part.EncounterId,
			Part.EnemySlotId,
			Part.PartSlotId);
	}

	UWacomBattleEnemyHostAnimationStyle* ConfigureAnimatedHost(
		AWacomBattleEnemyActor& Host,
		UPaperFlipbook*& OutIdle,
		UPaperFlipbook*& OutAttack,
		UPaperFlipbook*& OutBlock,
		UPaperFlipbook*& OutDestroyed)
	{
		OutIdle = MakeFlipbook(&Host, TEXT("Idle_Test"));
		OutAttack = MakeFlipbook(&Host, TEXT("Attack_Test"));
		OutBlock = MakeFlipbook(&Host, TEXT("Block_Test"));
		OutDestroyed = MakeFlipbook(&Host, TEXT("Destroyed_Test"));

		UWacomBattleEnemyHostAnimationStyle* Style =
			NewObject<UWacomBattleEnemyHostAnimationStyle>(&Host);
		Style->DefaultActionClip.Flipbook = OutAttack;
		Style->DefaultActionClip.PlayRate = 1.0f;
		FWacomBattleEnemyHostAnimationClip BlockClip;
		BlockClip.Flipbook = OutBlock;
		BlockClip.PlayRate = 1.25f;
		Style->ActionClipsByIntentId.Add(TEXT("Intent.Block"), BlockClip);
		Style->DestroyedClip.Flipbook = OutDestroyed;
		Style->DestroyedClip.PlayRate = 1.0f;

		Host.HostAuthoringMode = EWacomBattleEnemyHostAuthoringMode::SimpleHostVisual;
		Host.HostVisualMode = EWacomBattleEnemyHostVisualMode::Flipbook;
		Host.HostFlipbook = OutIdle;
		Host.HostFlipbookPlayRate = 0.75f;
		Host.bLoopHostFlipbook = true;
		Host.bAutoPlayHostFlipbook = true;
		Host.bHostVisualVisible = true;
		Host.HostAnimationStyle = Style;
		Host.RefreshBattleEnemyPartAuthoringState();
		return Style;
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

	FBattleEvent MakeActionEvent(
		const FBattleEnemyPartKey& PartKey,
		int32 Sequence,
		FName IntentId,
		int32 Count = 1)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::EnemyPartActed;
		Event.Sequence = Sequence;
		Event.ActorEnemyPartKey = PartKey;
		Event.IntentId = IntentId;
		Event.Count = Count;
		return Event;
	}

	FBattleEvent MakeDestroyedEvent(const FBattleEnemyPartKey& PartKey, int32 Sequence)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::EnemyPartHpEmptied;
		Event.Sequence = Sequence;
		Event.ActorEnemyPartKey = PartKey;
		return Event;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostAnimationStyleResolutionSpec,
	"Wacom.UI.Battle.BattleSceneEnemyHostAnimation.StyleResolutionIsExplicit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostAnimationStyleResolutionSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleSceneEnemyHostAnimationSpec;
	TStrongObjectPtr<UWacomBattleEnemyHostAnimationStyle> Style(
		NewObject<UWacomBattleEnemyHostAnimationStyle>());
	UPaperFlipbook* DefaultFlipbook = MakeFlipbook(Style.Get(), TEXT("DefaultAction_Test"));
	UPaperFlipbook* ExplicitFlipbook = MakeFlipbook(Style.Get(), TEXT("IntentAttack_Test"));
	Style->DefaultActionClip.Flipbook = DefaultFlipbook;
	FWacomBattleEnemyHostAnimationClip ExplicitClip;
	ExplicitClip.Flipbook = ExplicitFlipbook;
	Style->ActionClipsByIntentId.Add(TEXT("Intent.Attack"), ExplicitClip);

	const FWacomBattleEnemyHostAnimationClip* ResolvedExplicit =
		Style->ResolveActionClip(TEXT("Intent.Attack"));
	const FWacomBattleEnemyHostAnimationClip* ResolvedFallback =
		Style->ResolveActionClip(TEXT("Intent.Unknown"));
	TestTrue(TEXT("Explicit Intent clip wins"),
		ResolvedExplicit && ResolvedExplicit->Flipbook == ExplicitFlipbook);
	TestTrue(TEXT("Missing Intent falls back to default"),
		ResolvedFallback && ResolvedFallback->Flipbook == DefaultFlipbook);

	Style->DefaultActionClip.Flipbook = nullptr;
	Style->ActionClipsByIntentId.Reset();
	TestNull(TEXT("Asset-like Intent name is not inferred"),
		Style->ResolveActionClip(FName(*ExplicitFlipbook->GetName())));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostAnimationComponentLifecycleSpec,
	"Wacom.UI.Battle.BattleSceneEnemyHostAnimation.ComponentLifecycleAndWatchdog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostAnimationComponentLifecycleSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleSceneEnemyHostAnimationSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyActor* Host = World->SpawnActor<AWacomBattleEnemyActor>(
		AWacomBattleEnemyActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Scene enemy Host"), Host))
	{
		return false;
	}
	ON_SCOPE_EXIT { if (IsValid(Host)) { Host->Destroy(); } };

	UPaperFlipbook* Idle = nullptr;
	UPaperFlipbook* Attack = nullptr;
	UPaperFlipbook* Block = nullptr;
	UPaperFlipbook* Destroyed = nullptr;
	ConfigureAnimatedHost(*Host, Idle, Attack, Block, Destroyed);
	UPaperFlipbookComponent* Visual = Host->GetGeneratedHostFlipbookVisualComponent();
	if (!TestNotNull(TEXT("Generated Host Flipbook component"), Visual))
	{
		return false;
	}

	bool bActionCompleted = false;
	Host->PlayRuntimeHostActionAnimation(TEXT("Intent.Attack"), [&bActionCompleted]()
	{
		bActionCompleted = true;
	});
	TestTrue(TEXT("Action switches the existing component in place"),
		Host->GetGeneratedHostFlipbookVisualComponent() == Visual);
	TestEqual(TEXT("Action clip is active"), Visual->GetFlipbook(), Attack);
	TestTrue(TEXT("Action is a real completion barrier"),
		Host->GetBattleSceneEnemyDebugView().bHostAnimationPlaybackActive && !bActionCompleted);
	Visual->OnFinishedPlaying.Broadcast();
	TestTrue(TEXT("Action completion callback fires"), bActionCompleted);
	TestEqual(TEXT("Action completion restores authored Idle"), Visual->GetFlipbook(), Idle);
	TestTrue(TEXT("Authored Idle looping is restored"), Visual->IsLooping());
	TestTrue(TEXT("Authored Idle play rate is restored"),
		FMath::IsNearlyEqual(Visual->GetPlayRate(), 0.75f));

	bool bWatchdogCompleted = false;
	const int32 WatchdogCountBefore =
		Host->GetBattleSceneEnemyDebugView().HostAnimationWatchdogCompletionCount;
	Host->PlayRuntimeHostActionAnimation(TEXT("Intent.Block"), [&bWatchdogCompleted]()
	{
		bWatchdogCompleted = true;
	});
	++GFrameCounter;
	World->GetTimerManager().Tick(0.40f);
	// Timers created before this world's first TimerManager tick enter PendingTimerSet;
	// a second frame activates and evaluates them, matching normal UWorld ticking.
	++GFrameCounter;
	World->GetTimerManager().Tick(0.40f);
	TestTrue(TEXT("Duration watchdog completes a stalled Flipbook"), bWatchdogCompleted);
	TestEqual(TEXT("Watchdog restores Idle"), Visual->GetFlipbook(), Idle);
	TestEqual(TEXT("Watchdog completion is diagnosed"),
		Host->GetBattleSceneEnemyDebugView().HostAnimationWatchdogCompletionCount,
		WatchdogCountBefore + 1);

	bool bDestroyedCompleted = false;
	Host->PlayRuntimeHostDestroyedAnimation([&bDestroyedCompleted]()
	{
		bDestroyedCompleted = true;
	});
	TestEqual(TEXT("Destroyed clip uses the same component"),
		Host->GetGeneratedHostFlipbookVisualComponent(), Visual);
	Visual->OnFinishedPlaying.Broadcast();
	const FWacomBattleSceneEnemyDebugView TerminalView = Host->GetBattleSceneEnemyDebugView();
	TestTrue(TEXT("Destroyed completion callback fires"), bDestroyedCompleted);
	TestTrue(TEXT("Destroyed state is terminal"), TerminalView.bHostAnimationTerminalState);
	TestEqual(TEXT("Destroyed holds its clip"), Visual->GetFlipbook(), Destroyed);
	TestTrue(TEXT("Destroyed holds the last frame"),
		FMath::IsNearlyEqual(Visual->GetPlaybackPosition(), Visual->GetFlipbookLength()));

	Host->ResetRuntimeHostAnimation();
	TestFalse(TEXT("New battle reset clears terminal state"),
		Host->GetBattleSceneEnemyDebugView().bHostAnimationTerminalState);
	TestEqual(TEXT("New battle reset restores Idle"), Visual->GetFlipbook(), Idle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostAnimationQueueBarrierSpec,
	"Wacom.UI.Battle.BattleSceneEnemyHostAnimation.ActionQueueIsSerialAndStable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostAnimationQueueBarrierSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleSceneEnemyHostAnimationSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UCharacterDefinition* Character = Fixture.MakeCharacter(
		Fixture.MakeNoopCard(0),
		Fixture.MakeNoopCard(0),
		{ Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemy(20, 5, 0);
	UBattleSession* Session = Fixture.CreateSession(Character, Enemy, 1);
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

	UPaperFlipbook* Idle = nullptr;
	UPaperFlipbook* Attack = nullptr;
	UPaperFlipbook* Block = nullptr;
	UPaperFlipbook* Destroyed = nullptr;
	UWacomBattleEnemyHostAnimationStyle* Style = ConfigureAnimatedHost(
		*SceneEnemy.Host, Idle, Attack, Block, Destroyed);
	Harness->SetSession(Session);
	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	HUD->SetBattleSceneEnemyHostsForTest({ SceneEnemy.Host });
	const FBattleEnemyPartKey PartKey = Session->BuildSnapshot().Enemies[0].Parts[0].PartKey;
	UPaperFlipbookComponent* Visual = SceneEnemy.Host->GetGeneratedHostFlipbookVisualComponent();
	if (!TestNotNull(TEXT("Host Flipbook component"), Visual))
	{
		return false;
	}

	const int32 PlayCountBefore =
		SceneEnemy.Host->GetBattleSceneEnemyDebugView().HostAnimationPlayCount;
	HUD->EnqueueBattlePresentationEventsForTest({
		MakeActionEvent(PartKey, 1, TEXT("Intent.Attack"), 0) });
	TestFalse(TEXT("Count zero action does not block the queue"), HUD->IsBattlePresentationBusy());
	TestEqual(TEXT("Count zero action does not play"),
		SceneEnemy.Host->GetBattleSceneEnemyDebugView().HostAnimationPlayCount,
		PlayCountBefore);

	SceneEnemy.Host->HostAnimationStyle = nullptr;
	HUD->EnqueueBattlePresentationEventsForTest({
		MakeActionEvent(PartKey, 2, TEXT("Intent.Attack")) });
	TestFalse(TEXT("Missing Style completes immediately"), HUD->IsBattlePresentationBusy());
	SceneEnemy.Host->HostAnimationStyle = Style;

	HUD->EnqueueBattlePresentationEventsForTest({
		MakeActionEvent(PartKey, 3, TEXT("Intent.Attack")),
		MakeActionEvent(PartKey, 4, TEXT("Intent.Block")) });
	TestTrue(TEXT("First action holds the presentation queue"), HUD->IsBattlePresentationBusy());
	TestEqual(TEXT("First action uses default clip"), Visual->GetFlipbook(), Attack);
	TestEqual(TEXT("Only first action has started"),
		SceneEnemy.Host->GetBattleSceneEnemyDebugView().HostAnimationPlayCount,
		PlayCountBefore + 1);

	Visual->SetPlaybackPosition(0.05f, false);
	const float ProgressBeforeRefresh = Visual->GetPlaybackPosition();
	const int32 RegistryRevisionBefore =
		HUD->GetBattleSceneEnemyTargetRegistryRevisionForTest();
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());
	HUD->SetBattleSceneEnemyHostsForTest({ SceneEnemy.Host });
	TestEqual(TEXT("Snapshot and repeated Host set keep registry revision"),
		HUD->GetBattleSceneEnemyTargetRegistryRevisionForTest(),
		RegistryRevisionBefore);
	TestEqual(TEXT("Snapshot keeps action clip"), Visual->GetFlipbook(), Attack);
	TestTrue(TEXT("Snapshot keeps action progress"),
		FMath::IsNearlyEqual(Visual->GetPlaybackPosition(), ProgressBeforeRefresh));

	SceneEnemy.Host->InvalidateRuntimePartTopology();
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());
	TestTrue(TEXT("Topology rebuild keeps the visual component"),
		SceneEnemy.Host->GetGeneratedHostFlipbookVisualComponent() == Visual);
	TestEqual(TEXT("Topology rebuild does not reset the active clip"),
		Visual->GetFlipbook(), Attack);

	Visual->OnFinishedPlaying.Broadcast();
	TestTrue(TEXT("Second action starts only after first completes"),
		HUD->IsBattlePresentationBusy());
	TestEqual(TEXT("Explicit second Intent clip is selected"), Visual->GetFlipbook(), Block);
	TestEqual(TEXT("Second action increments play count"),
		SceneEnemy.Host->GetBattleSceneEnemyDebugView().HostAnimationPlayCount,
		PlayCountBefore + 2);
	Visual->OnFinishedPlaying.Broadcast();
	TestFalse(TEXT("Queue drains after second action completes"), HUD->IsBattlePresentationBusy());
	TestEqual(TEXT("Queue drain restores Idle"), Visual->GetFlipbook(), Idle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostAnimationFinalPartPlayCardPlanSpec,
	"Wacom.UI.Battle.BattleSceneEnemyHostAnimation.FinalPartDestroyedThroughPlayCardPlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostAnimationFinalPartPlayCardPlanSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleSceneEnemyHostAnimationSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UCardDefinition* Killer = Fixture.MakeSimpleDamageCard(0, 100);
	UCharacterDefinition* Character = Fixture.MakeCharacter(
		Fixture.MakeNoopCard(0),
		Fixture.MakeNoopCard(0),
		{ Killer, Killer, Killer, Killer, Killer });
	UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemy(10, 5, 0);
	UBattleSession* Session = Fixture.CreateSession(Character, Enemy, 17);
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

	UPaperFlipbook* Idle = nullptr;
	UPaperFlipbook* Attack = nullptr;
	UPaperFlipbook* Block = nullptr;
	UPaperFlipbook* Destroyed = nullptr;
	ConfigureAnimatedHost(*SceneEnemy.Host, Idle, Attack, Block, Destroyed);
	Harness->SetSession(Session);
	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	HUD->SetBattleSceneEnemyHostsForTest({ SceneEnemy.Host });
	UPaperFlipbookComponent* Visual =
		SceneEnemy.Host->GetGeneratedHostFlipbookVisualComponent();
	if (!TestNotNull(TEXT("Host Flipbook component"), Visual))
	{
		return false;
	}

	const FBattleSnapshot Before = Session->BuildSnapshot();
	const FGuid KillerId =
		FWacomBattleFixture::FindHandInstanceByCardId(Before, Killer->CardId);
	const FEnemyPartSnapshot* Target =
		FWacomBattleFixture::GetEnemyPartSnapshot(Before, 0, 0);
	if (!TestTrue(TEXT("Killer is in hand"), KillerId.IsValid())
		|| !TestNotNull(TEXT("Target part"), Target))
	{
		return false;
	}

	HUD->SetTargetSelectionStateForTest(KillerId);
	HUD->OnEnemyPartClickedByUser(MakeWorldTargetHandle(*Target));
	const FBattleSnapshot After = Session->BuildSnapshot();
	TestTrue(TEXT("HUD-routed killing PlayCard advances the session"),
		After.Version > Before.Version);
	TestTrue(TEXT("HUD-routed PlayCard marks all parts destroyed"),
		After.Enemies.Num() == 1 && After.Enemies[0].bAllPartsDestroyed);
	TestTrue(TEXT("HUD-routed PlayCard enters pending knockdown choice"),
		After.Phase == EBattlePhase::PendingKnockdownChoice);

	for (int32 Step = 0;
		Step < 32 && HUD->IsBattlePresentationBusy()
			&& Visual->GetFlipbook() != Destroyed;
		++Step)
	{
		HUD->AdvanceBattlePresentationQueueForTest();
	}
	TestEqual(TEXT("Real PlayCard plan starts Destroyed clip"),
		Visual->GetFlipbook(), Destroyed);
	TestTrue(TEXT("Destroyed clip holds the presentation queue"),
		HUD->IsBattlePresentationBusy());

	Visual->OnFinishedPlaying.Broadcast();
	TestTrue(TEXT("Completed Destroyed remains terminal before knockdown choice"),
		SceneEnemy.Host->GetBattleSceneEnemyDebugView().bHostAnimationTerminalState);
	TestEqual(TEXT("Completed Destroyed remains on its terminal clip"),
		Visual->GetFlipbook(), Destroyed);

	bool bBattleEndBroadcast = false;
	const FDelegateHandle BattleEndHandle = HUD->OnBattleEndedNative.AddLambda(
		[&bBattleEndBroadcast](EBattleOutcome)
		{
			bBattleEndBroadcast = true;
		});
	ON_SCOPE_EXIT
	{
		HUD->OnBattleEndedNative.Remove(BattleEndHandle);
	};
	HUD->OnKnockdownChoiceSelected(EKnockdownChoice::Destroy);
	for (int32 Step = 0; Step < 32 && !bBattleEndBroadcast; ++Step)
	{
		HUD->AdvanceBattlePresentationQueueForTest();
	}
	TestTrue(TEXT("Destroy choice reaches BattleEnd presentation signal"),
		bBattleEndBroadcast);
	HUD->SetBattleSceneEnemyHostsForTest({});
	TestTrue(TEXT("BattleEnd Host clear preserves Destroyed terminal state"),
		SceneEnemy.Host->GetBattleSceneEnemyDebugView().bHostAnimationTerminalState);
	TestEqual(TEXT("BattleEnd Host clear preserves Destroyed terminal clip"),
		Visual->GetFlipbook(), Destroyed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostAnimationDestroyedRetirementSpec,
	"Wacom.UI.Battle.BattleSceneEnemyHostAnimation.DestroyedUsesRetiringHost",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostAnimationDestroyedRetirementSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleSceneEnemyHostAnimationSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UCharacterDefinition* Character = Fixture.MakeCharacter(
		Fixture.MakeNoopCard(0),
		Fixture.MakeNoopCard(0),
		{ Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fixture.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fixture.CreateSession(Character, Enemy, 1);
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

	UPaperFlipbook* Idle = nullptr;
	UPaperFlipbook* Attack = nullptr;
	UPaperFlipbook* Block = nullptr;
	UPaperFlipbook* Destroyed = nullptr;
	ConfigureAnimatedHost(*SceneEnemy.Host, Idle, Attack, Block, Destroyed);
	Harness->SetSession(Session);
	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	HUD->SetBattleSceneEnemyHostsForTest({ SceneEnemy.Host });
	const FBattleSnapshot InitialSnapshot = Session->BuildSnapshot();
	UPaperFlipbookComponent* Visual = SceneEnemy.Host->GetGeneratedHostFlipbookVisualComponent();
	if (!TestNotNull(TEXT("Host Flipbook component"), Visual))
	{
		return false;
	}

	const int32 PlayCountBefore =
		SceneEnemy.Host->GetBattleSceneEnemyDebugView().HostAnimationPlayCount;
	HUD->EnqueueBattlePresentationEventsForTest({
		MakeDestroyedEvent(InitialSnapshot.Enemies[0].Parts[0].PartKey, 10) });
	HUD->AdvanceBattlePresentationQueueForTest();
	HUD->AdvanceBattlePresentationQueueForTest();
	TestFalse(TEXT("Partial multi-part destruction does not play Host Downed"),
		SceneEnemy.Host->GetBattleSceneEnemyDebugView().bHostAnimationPlaybackActive);
	TestEqual(TEXT("Partial destruction does not increment Host play count"),
		SceneEnemy.Host->GetBattleSceneEnemyDebugView().HostAnimationPlayCount,
		PlayCountBefore);
	TestFalse(TEXT("Partial destruction queue drains"), HUD->IsBattlePresentationBusy());

	FBattleSnapshot BattleEndSnapshot = InitialSnapshot;
	BattleEndSnapshot.Phase = EBattlePhase::BattleEnd;
	BattleEndSnapshot.Enemies[0].bAllPartsDestroyed = true;
	for (FEnemyPartSnapshot& Part : BattleEndSnapshot.Enemies[0].Parts)
	{
		Part.bDestroyed = true;
		Part.CurrentHp = 0;
	}
	HUD->RefreshFromSnapshotForTest(BattleEndSnapshot);
	TestEqual(TEXT("BattleEnd immediately clears world target registry"),
		HUD->GetBattleSceneEnemyPartWorldTargetBridgeCountForTest(), 0);
	TestEqual(TEXT("BattleEnd immediately clears presentation target registry"),
		HUD->GetBattlePresentationTargetCountForTest(), 0);

	FBattleEvent BattleEnded;
	BattleEnded.Type = EBattleEventType::BattleEnded;
	BattleEnded.Sequence = 21;
	HUD->EnqueueBattlePresentationEventsForTest({
		MakeDestroyedEvent(InitialSnapshot.Enemies[0].Parts.Last().PartKey, 20),
		BattleEnded });
	HUD->AdvanceBattlePresentationQueueForTest();
	HUD->AdvanceBattlePresentationQueueForTest();
	TestTrue(TEXT("Retiring Host starts Destroyed after target registry cleanup"),
		SceneEnemy.Host->GetBattleSceneEnemyDebugView().bHostAnimationPlaybackActive);
	TestEqual(TEXT("Retiring Host uses Destroyed clip"), Visual->GetFlipbook(), Destroyed);
	TestTrue(TEXT("Destroyed remains a queue barrier"), HUD->IsBattlePresentationBusy());

	Visual->OnFinishedPlaying.Broadcast();
	TestFalse(TEXT("BattleEnd queue drains after Destroyed completion"),
		HUD->IsBattlePresentationBusy());
	TestTrue(TEXT("Destroyed remains on terminal frame after retiring refs clear"),
		SceneEnemy.Host->GetBattleSceneEnemyDebugView().bHostAnimationTerminalState);
	TestEqual(TEXT("Destroyed terminal clip remains visible"), Visual->GetFlipbook(), Destroyed);

	HUD->SetBattleSceneEnemyHostsForTest({ SceneEnemy.Host });
	TestFalse(TEXT("Reentry clears prior terminal state"),
		SceneEnemy.Host->GetBattleSceneEnemyDebugView().bHostAnimationTerminalState);
	TestEqual(TEXT("Reentry restores authored Idle in place"), Visual->GetFlipbook(), Idle);
	TestTrue(TEXT("Reentry keeps the generated component"),
		SceneEnemy.Host->GetGeneratedHostFlipbookVisualComponent() == Visual);
	return true;
}
