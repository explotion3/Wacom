// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyPartActor.h"
#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Commands/BattleCommand.h"
#include "Components/WacomBattleEnemyPartPresentationComponent.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Events/BattleEvent.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Snapshots/EnemySnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "UI/Battle/BattleCombatLogFeedWidget.h"
#include "UI/BattleWidgetSpecReceiver.h"
#include "BattleHUDTestHarness.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomBattlePresentationQueueSpec
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

	FGuid FindFirstHandCardByTargetMode(const FBattleSnapshot& Snapshot, ECardTargetMode TargetMode)
	{
		for (const FHandCardSnapshot& Card : Snapshot.Hand.Cards)
		{
			if (Card.Definition && Card.Definition->TargetMode == TargetMode)
			{
				return Card.InstanceId;
			}
		}
		return FGuid();
	}

	FWacomInteractionTargetHandle MakeWorldTargetHandleForPart(
		const FBattleSnapshot& Snapshot,
		const FGuid& PartInstanceId)
	{
		for (const FEnemySnapshot& Enemy : Snapshot.Enemies)
		{
			for (const FEnemyPartSnapshot& Part : Enemy.Parts)
			{
				if (Part.InstanceId == PartInstanceId)
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
			}
		}
		return FWacomInteractionTargetHandle();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationQueueIgnoresTextOnlyEventsSpec,
	"Wacom.UI.Battle.PresentationQueue.IgnoresTextOnlyEvents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationQueueIgnoresTextOnlyEventsSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattlePresentationQueueSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity,
		SpawnParams);
	if (!TestNotNull(TEXT("PlayerController spawned"), PC))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);

	FBattleEvent First;
	First.Type = EBattleEventType::BattleStarted;
	First.Sequence = 1;

	FBattleEvent Second;
	Second.Type = EBattleEventType::DamageDealt;
	Second.Sequence = 2;
	Second.Amount = 3;

	HUD->EnqueueBattlePresentationEventsForTest({ First, Second });

	World->GetTimerManager().Tick(0.01f);
	TestFalse(TEXT("Text-only battle events do not create presentation steps"), HUD->IsBattlePresentationBusy());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationQueueNonblockingInputSpec,
	"Wacom.UI.Battle.PresentationQueue.NonblockingInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationQueueNonblockingInputSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattlePresentationQueueSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(0, 1);
	UCardDefinition* NoTargetCard = Fx.MakeNoopCard(0);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ TargetCard, NoTargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 50, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDWithPlayer(World);
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get())
		|| !TestNotNull(TEXT("PlayerController spawned"), Harness->PlayerController()))
	{
		return false;
	}

	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	UBattleCombatLogFeedWidget* CombatLogFeed = Harness->AttachCombatLogFeed();
	Harness->AttachPresentationStack();
	UWacomBattleCommandBarTestProbe* CommandBar = Harness->AttachCommandBar();
	Harness->SetSession(Session);
	if (!TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("CombatLogFeed"), CombatLogFeed)
		|| !TestNotNull(TEXT("CommandBar"), CommandBar))
	{
		return false;
	}
	TestFalse(TEXT("Initial session presentation has settled before focused blocking check"),
		HUD->IsBattlePresentationBusy());
	TestEqual(TEXT("HUD returns idle after initial session presentation"), HUD->GetUIState(), EBattleUIState::Idle);
	TestTrue(TEXT("Command bar wait starts enabled"), CommandBar->IsWaitCommandEnabledForTest());
	TestTrue(TEXT("Command bar end turn starts enabled"), CommandBar->IsEndTurnCommandEnabledForTest());

	const FBattleSnapshot InitialSnapshot = Session->BuildSnapshot();
	const FGuid TargetCardId = WacomBattlePresentationQueueSpec::FindFirstHandCardByTargetMode(
		InitialSnapshot,
		ECardTargetMode::SingleEnemyPart);
	const FGuid NoTargetCardId = WacomBattlePresentationQueueSpec::FindFirstHandCardByTargetMode(
		InitialSnapshot,
		ECardTargetMode::None);
	const FGuid TargetPartId = FWacomBattleFixture::FindPartInstanceId(InitialSnapshot, 0);
	if (!TestTrue(TEXT("Target card exists"), TargetCardId.IsValid())
		|| !TestTrue(TEXT("No target card exists"), NoTargetCardId.IsValid())
		|| !TestTrue(TEXT("Target part exists"), TargetPartId.IsValid()))
	{
		return false;
	}

	FBattleEvent Event;
	Event.Type = EBattleEventType::DamageDealt;
	Event.Sequence = 1;
	Event.ActorEnemyPartKey = FWacomBattleFixture::FindPartKeyByInstanceId(InitialSnapshot, TargetPartId);
	Event.Amount = 1;
	HUD->EnqueueBattlePresentationEventsForTest({ Event });
	World->GetTimerManager().Tick(0.01f);

	TestTrue(TEXT("Queue reports busy"), HUD->IsBattlePresentationBusy());
	TestEqual(TEXT("HUD stays idle while presenting"), HUD->GetUIState(), EBattleUIState::Idle);
	TestTrue(TEXT("Command bar wait stays enabled while presenting"), CommandBar->IsWaitCommandEnabledForTest());
	TestTrue(TEXT("Command bar end turn stays enabled while presenting"), CommandBar->IsEndTurnCommandEnabledForTest());

	const int32 CombatLogCountBeforeTargetSelect = HUD->GetBattleCombatLogBlockCount();
	HUD->OnCardClickedByUser(TargetCardId);
	TestEqual(TEXT("Target card can enter target select while presenting"), HUD->GetUIState(), EBattleUIState::TargetSelect);
	TestEqual(TEXT("Target card becomes pending while presenting"), HUD->GetPendingTargetingCardId(), TargetCardId);
	TestEqual(TEXT("Target select alone does not append combat log"), HUD->GetBattleCombatLogBlockCount(), CombatLogCountBeforeTargetSelect);

	const int32 VersionBeforeTargetSubmit = Session->BuildSnapshot().Version;
	HUD->OnEnemyPartClickedByUser(
		WacomBattlePresentationQueueSpec::MakeWorldTargetHandleForPart(Session->BuildSnapshot(), TargetPartId));
	TestEqual(TEXT("Target submit returns idle while presenting"), HUD->GetUIState(), EBattleUIState::Idle);
	TestTrue(TEXT("Target submit resolves while presenting"),
		Session->BuildSnapshot().Version > VersionBeforeTargetSubmit);
	TestTrue(TEXT("Presentation queue remains busy after appended card events"), HUD->IsBattlePresentationBusy());
	TestEqual(TEXT("Target submit appends presentation stack entry"), HUD->GetPresentationStackEntryCountForTest(), 1);
	TestEqual(TEXT("Oldest stack entry is target card"), HUD->GetPresentationStackEntriesForTest()[0].CardInstanceId, TargetCardId);
	TestEqual(TEXT("Target submit appends one combat log block"),
		HUD->GetBattleCombatLogBlockCount(),
		CombatLogCountBeforeTargetSelect + 1);
	TestTrue(TEXT("Target submit block uses PlayCard header"),
		HUD->GetBattleCombatLogHistoryForTest().Last().HeaderText.ToString().Contains(TEXT("打出")));

	HUD->OnCardClickedByUser(NoTargetCardId);
	TestEqual(TEXT("No target card can submit while presenting"), HUD->GetUIState(), EBattleUIState::Idle);
	TestFalse(TEXT("No target submit clears pending while presenting"), HUD->GetPendingTargetingCardId().IsValid());
	TestTrue(TEXT("Presentation queue still has appended events after no-target card"), HUD->IsBattlePresentationBusy());
	TestEqual(TEXT("No-target submit appends second presentation stack entry"), HUD->GetPresentationStackEntryCountForTest(), 2);
	TestEqual(TEXT("Second stack entry is newest card"), HUD->GetPresentationStackEntriesForTest()[1].CardInstanceId, NoTargetCardId);
	TestEqual(TEXT("No-target submit appends one more combat log block"),
		HUD->GetBattleCombatLogBlockCount(),
		CombatLogCountBeforeTargetSelect + 2);

	const int32 WaitValueBefore = Session->BuildSnapshot().CurrentWaitValue;
	HUD->OnWaitRequested();
	TestEqual(TEXT("Wait does not resolve while presentation stack has cards"), Session->BuildSnapshot().CurrentWaitValue, WaitValueBefore);
	TestTrue(TEXT("Wait becomes pending"), HUD->HasPendingTurnBoundaryCommandForTest());
	TestFalse(TEXT("Command bar wait disabled while pending"), CommandBar->IsWaitCommandEnabledForTest());
	TestFalse(TEXT("Command bar end turn disabled while pending"), CommandBar->IsEndTurnCommandEnabledForTest());

	const int32 VersionBeforeBlockedCard = Session->BuildSnapshot().Version;
	HUD->OnCardClickedByUser(NoTargetCardId);
	TestEqual(TEXT("Pending turn boundary blocks further card submits"), Session->BuildSnapshot().Version, VersionBeforeBlockedCard);
	TestEqual(TEXT("Pending turn boundary does not append another stack entry"), HUD->GetPresentationStackEntryCountForTest(), 2);

	while (HUD->IsBattlePresentationBusy() && !HUD->GetPresentationStackEntriesForTest().IsEmpty()
		&& !HUD->GetPresentationStackEntriesForTest()[0].bIsExiting)
	{
		HUD->AdvanceBattlePresentationQueueForTest();
	}
	TestTrue(TEXT("Boundary marks oldest stack entry exiting"), HUD->GetPresentationStackEntriesForTest()[0].bIsExiting);
	TestTrue(TEXT("Pending wait remains while exit motion plays"), HUD->HasPendingTurnBoundaryCommandForTest());
	TestEqual(TEXT("Pending wait still does not mutate during exit motion"), Session->BuildSnapshot().CurrentWaitValue, WaitValueBefore);

	HUD->FinishPresentationStackEntryExitForTest(HUD->GetPresentationStackEntriesForTest()[0].EntryId);
	Harness->SettlePresentationQueueAndExitStack();
	TestFalse(TEXT("Pending wait runs after stack drains"), HUD->HasPendingTurnBoundaryCommandForTest());
	TestEqual(TEXT("Wait resolves after stack drains"), Session->BuildSnapshot().CurrentWaitValue, WaitValueBefore + 1);
	TestEqual(TEXT("Wait appends after stack drains"),
		HUD->GetBattleCombatLogBlockCount(),
		CombatLogCountBeforeTargetSelect + 3);
	TestEqual(TEXT("Presentation stack drained"), HUD->GetPresentationStackEntryCountForTest(), 0);

	const int32 VersionBeforeEndTurn = Session->BuildSnapshot().Version;
	HUD->OnEndTurnRequested();
	TestTrue(TEXT("End turn resolves immediately when stack is empty"), Session->BuildSnapshot().Version > VersionBeforeEndTurn);
	TestEqual(TEXT("Scrollable feed mirrors combat log history"),
		CombatLogFeed->GetVisibleBlockCount(),
		HUD->GetBattleCombatLogBlockCount());

	Harness->SettlePresentationQueue();
	TestFalse(TEXT("Queue no longer busy"), HUD->IsBattlePresentationBusy());
	TestTrue(TEXT("HUD remains in a non-battle-end command state after presentation"),
		HUD->GetUIState() == EBattleUIState::Idle || HUD->GetUIState() == EBattleUIState::BattleEnd);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationStackEndTurnBarrierSpec,
	"Wacom.UI.Battle.EndTurnWhilePresentationStackPendingLocksFurtherPlayerCommandsAndRunsAfterDrain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationStackEndTurnBarrierSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattlePresentationQueueSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(0, 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ TargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 50, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDWithPlayer(World);
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get())
		|| !TestNotNull(TEXT("PlayerController spawned"), Harness->PlayerController()))
	{
		return false;
	}

	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	Harness->AttachPresentationStack();
	Harness->SetSession(Session);
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}

	const FBattleSnapshot InitialSnapshot = Session->BuildSnapshot();
	const FGuid TargetCardId = WacomBattlePresentationQueueSpec::FindFirstHandCardByTargetMode(
		InitialSnapshot,
		ECardTargetMode::SingleEnemyPart);
	const FGuid TargetPartId = FWacomBattleFixture::FindPartInstanceId(InitialSnapshot, 0);
	if (!TestTrue(TEXT("Target card exists"), TargetCardId.IsValid())
		|| !TestTrue(TEXT("Target part exists"), TargetPartId.IsValid()))
	{
		return false;
	}

	HUD->OnCardClickedByUser(TargetCardId);
	HUD->OnEnemyPartClickedByUser(
		WacomBattlePresentationQueueSpec::MakeWorldTargetHandleForPart(Session->BuildSnapshot(), TargetPartId));
	TestEqual(TEXT("PlayCard appends one stack entry"), HUD->GetPresentationStackEntryCountForTest(), 1);
	const int32 VersionBeforeEndTurn = Session->BuildSnapshot().Version;

	HUD->OnEndTurnRequested();
	TestTrue(TEXT("EndTurn becomes pending"), HUD->HasPendingTurnBoundaryCommandForTest());
	TestEqual(TEXT("EndTurn does not mutate while stack pending"), Session->BuildSnapshot().Version, VersionBeforeEndTurn);

	HUD->OnEndTurnRequested();
	TestEqual(TEXT("Repeated EndTurn remains ignored while pending"), Session->BuildSnapshot().Version, VersionBeforeEndTurn);

	while (HUD->IsBattlePresentationBusy() && !HUD->GetPresentationStackEntriesForTest().IsEmpty()
		&& !HUD->GetPresentationStackEntriesForTest()[0].bIsExiting)
	{
		HUD->AdvanceBattlePresentationQueueForTest();
	}
	TestTrue(TEXT("EndTurn waits while stack entry is exiting"), HUD->HasPendingTurnBoundaryCommandForTest());
	HUD->FinishPresentationStackEntryExitForTest(HUD->GetPresentationStackEntriesForTest()[0].EntryId);
	Harness->SettlePresentationQueueAndExitStack();
	TestFalse(TEXT("Pending EndTurn clears after drain"), HUD->HasPendingTurnBoundaryCommandForTest());
	TestTrue(TEXT("EndTurn runs after stack drains"), Session->BuildSnapshot().Version > VersionBeforeEndTurn);
	TestEqual(TEXT("Presentation stack drained"), HUD->GetPresentationStackEntryCountForTest(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationQueueDamageCueSpec,
	"Wacom.UI.Battle.PresentationQueue.DamageCue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationQueueDamageCueSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattlePresentationQueueSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FGuid TargetPartId = FWacomBattleFixture::FindPartInstanceId(Session->BuildSnapshot(), 0);

	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDWithPlayer(World);
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get())
		|| !TestNotNull(TEXT("PlayerController spawned"), Harness->PlayerController()))
	{
		return false;
	}

	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	FWacomBattleHUDTestSceneEnemyHost& SceneEnemy =
		Harness->AttachSceneEnemyHost(Enemy, { TEXT("Test.Part.Solo") });
	if (!TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestTrue(TEXT("Scene enemy has one part"), SceneEnemy.Parts.Num() == 1)
		|| !TestNotNull(TEXT("Scene enemy part"), SceneEnemy.Parts.IsValidIndex(0) ? SceneEnemy.Parts[0] : nullptr))
	{
		return false;
	}

	Harness->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ SceneEnemy.Host });
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());
	Harness->SettlePresentationQueue();

	FBattleEvent Event;
	Event.Type = EBattleEventType::DamageDealt;
	Event.Sequence = 1;
	Event.ActorEnemyPartKey = FWacomBattleFixture::FindPartKeyByInstanceId(Session->BuildSnapshot(), TargetPartId);
	Event.Amount = 7;
	HUD->EnqueueBattlePresentationEventsForTest({ Event });

	World->GetTimerManager().Tick(0.01f);
	const FWacomBattleEnemyPartPresentationDebugView View =
		SceneEnemy.Parts[0]->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView();
	TestEqual(TEXT("Target cue plays for damage event"), View.CuePlayCount, 1);
	TestEqual(TEXT("Target cue kind is damage"), View.LastCueKind, FName(TEXT("DamageDealt")));
	TestEqual(TEXT("Target cue type is damage"), View.LastCueType, EBattleEventType::DamageDealt);
	TestEqual(TEXT("Target cue carries damage amount"), View.LastCueAmount, 7);

	HUD->AdvanceBattlePresentationQueueForTest();
	TestFalse(TEXT("Queue finishes after target cue pacing"), HUD->IsBattlePresentationBusy());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationQueueBlocksPlayerActionOutsidePlayerPhaseSpec,
	"Wacom.UI.Battle.PresentationQueue.BlocksPlayerActionOutsidePlayerPhase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationQueueBlocksPlayerActionOutsidePlayerPhaseSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* KillerCard = nullptr;
	UCharacterDefinition* Character = [&Fx, &KillerCard]()
	{
		UCardDefinition* LeftHand = Fx.MakeNoopCard(0);
		UCardDefinition* RightHand = Fx.MakeNoopCard(0);
		KillerCard = Fx.MakeSimpleDamageCard(0, 100);
		TArray<UCardDefinition*> Deck = { KillerCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) };
		return Fx.MakeCharacter(LeftHand, RightHand, Deck);
	}();
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(50, 50, 50, 7, 7, 7);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetSession(Session);
	while (HUD->IsBattlePresentationBusy())
	{
		HUD->AdvanceBattlePresentationQueueForTest();
	}

	const FBattleSnapshot InitialSnapshot = Session->BuildSnapshot();
	const FGuid KillerCardId = FWacomBattleFixture::FindHandInstanceByCardId(InitialSnapshot, KillerCard->CardId);
	const FGuid TargetPartId = FWacomBattleFixture::FindPartInstanceId(InitialSnapshot, 0);
	if (!TestTrue(TEXT("Killer card exists"), KillerCardId.IsValid())
		|| !TestTrue(TEXT("Target part exists"), TargetPartId.IsValid()))
	{
		return false;
	}

	TestTrue(TEXT("Submit killer card"),
		Session->SubmitCommand(FWacomBattleFixture::MakePlayCardOnPartInstance(
			InitialSnapshot,
			KillerCardId,
			TargetPartId)).IsOk());
	TestEqual(TEXT("Session enters pending knockdown"), Session->BuildSnapshot().Phase, EBattlePhase::PendingKnockdownChoice);
	TestFalse(TEXT("HUD command gate blocks pending knockdown"), HUD->CanSubmitPlayerActionCommand());

	const int32 VersionBeforeWait = Session->BuildSnapshot().Version;
	HUD->OnWaitRequested();
	TestEqual(TEXT("Wait does not resolve during pending knockdown"), Session->BuildSnapshot().Version, VersionBeforeWait);

	FGuid FillerCardId;
	for (const FHandCardSnapshot& Card : Session->BuildSnapshot().Hand.Cards)
	{
		if (Card.Definition && Card.Definition->TargetMode == ECardTargetMode::None)
		{
			FillerCardId = Card.InstanceId;
			break;
		}
	}
	TestTrue(TEXT("Filler card exists"), FillerCardId.IsValid());
	HUD->OnCardClickedByUser(FillerCardId);
	TestEqual(TEXT("Card click does not submit during pending knockdown"), Session->BuildSnapshot().Version, VersionBeforeWait);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationQueueInvalidTargetCueSkippedSpec,
	"Wacom.UI.Battle.PresentationQueue.InvalidTargetCueSkipped",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationQueueInvalidTargetCueSkippedSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattlePresentationQueueSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity,
		SpawnParams);
	if (!TestNotNull(TEXT("PlayerController spawned"), PC))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);

	FBattleEvent Event;
	Event.Type = EBattleEventType::DamageDealt;
	Event.Sequence = 1;
	Event.Amount = 5;
	HUD->EnqueueBattlePresentationEventsForTest({ Event });

	World->GetTimerManager().Tick(0.01f);
	TestFalse(TEXT("Invalid target damage does not create presentation steps"), HUD->IsBattlePresentationBusy());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationQueueClearsOnSessionChangeSpec,
	"Wacom.UI.Battle.PresentationQueue.ClearsOnSessionChange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationQueueClearsOnSessionChangeSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattlePresentationQueueSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity,
		SpawnParams);
	if (!TestNotNull(TEXT("PlayerController spawned"), PC))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->SetSession(Session);

	FBattleEvent First;
	First.Type = EBattleEventType::DamageDealt;
	First.Sequence = 1;
	First.ActorEnemyPartKey = FBattleEnemyPartKey::Make(TEXT("Encounter"), TEXT("Enemy"), TEXT("QueuedA"));
	First.Amount = 4;
	FBattleEvent Second;
	Second.Type = EBattleEventType::DamageDealt;
	Second.Sequence = 2;
	Second.ActorEnemyPartKey = FBattleEnemyPartKey::Make(TEXT("Encounter"), TEXT("Enemy"), TEXT("QueuedB"));
	Second.Amount = 4;
	HUD->EnqueueBattlePresentationEventsForTest({ First, Second });

	World->GetTimerManager().Tick(0.01f);
	TestTrue(TEXT("Queue is busy before session change"), HUD->IsBattlePresentationBusy());

	HUD->SetSession(nullptr);
	TestFalse(TEXT("Session change clears queue"), HUD->IsBattlePresentationBusy());

	World->GetTimerManager().Tick(0.50f);
	TestFalse(TEXT("Cleared queue does not resume queued target cue"), HUD->IsBattlePresentationBusy());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationQueueBattleEndClearsQueueSafelySpec,
	"Wacom.UI.Battle.PresentationQueue.BattleEndClearsQueueSafely",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationQueueBattleEndClearsQueueSafelySpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattlePresentationQueueSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* Killer = Fx.MakeSimpleDamageCard(0, 100);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Killer, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(10, 5, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	const FBattleSnapshot InitialSnapshot = Session->BuildSnapshot();
	const FGuid KillerId = FWacomBattleFixture::FindHandInstanceByCardId(InitialSnapshot, Killer->CardId);
	const FGuid TargetPartId = FWacomBattleFixture::FindPartInstanceId(InitialSnapshot, 0);
	TestTrue(TEXT("Play killer card"),
		Session->SubmitCommand(FWacomBattleFixture::MakePlayCardOnPartInstance(
			InitialSnapshot,
			KillerId,
			TargetPartId)).IsOk());
	TestTrue(TEXT("Submit final Aid"), Session->SubmitCommand(FBattleCommand::MakeKnockdownChoice(EKnockdownChoice::Aid)).IsOk());
	TestTrue(TEXT("Session reached BattleEnd"), Session->GetPhase() == EBattlePhase::BattleEnd);
	Session->ConsumeEvents();

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity,
		SpawnParams);
	if (!TestNotNull(TEXT("PlayerController spawned"), PC))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->SetSession(Session);
	HUD->OnBattleEndedNative.AddUObject(
		HUD.Get(),
		&UWacomBattleHUDDetailTest::ClearPresentationQueueOnBattleEndedForTest);

	FBattleEvent VictorySignal;
	VictorySignal.Type = EBattleEventType::BattleEnded;
	VictorySignal.Sequence = 1;
	VictorySignal.Count = 1;

	FBattleEvent ShouldNotPlayAfterClear;
	ShouldNotPlayAfterClear.Type = EBattleEventType::DamageDealt;
	ShouldNotPlayAfterClear.Sequence = 2;
	ShouldNotPlayAfterClear.ActorEnemyPartKey = FBattleEnemyPartKey::Make(TEXT("Encounter"), TEXT("Enemy"), TEXT("Cleared"));
	ShouldNotPlayAfterClear.Amount = 9;

	HUD->EnqueueBattlePresentationEventsForTest({ VictorySignal, ShouldNotPlayAfterClear });

	World->GetTimerManager().Tick(0.01f);
	TestTrue(TEXT("BattleEnd callback clears queue during presentation"),
		HUD->GetBattleEndedCallbackCountForTest() > 0);
	TestFalse(TEXT("Queue no longer busy after battle end callback clears it"), HUD->IsBattlePresentationBusy());
	TestEqual(TEXT("HUD is in BattleEnd after battle end step"), HUD->GetUIState(), EBattleUIState::BattleEnd);

	HUD->AdvanceBattlePresentationQueueForTest();
	World->GetTimerManager().Tick(1.0f);
	TestFalse(TEXT("Cleared queue does not play trailing event"), HUD->IsBattlePresentationBusy());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationQueueKnockdownDialogDelayedAndGuardedSpec,
	"Wacom.UI.Battle.PresentationQueue.KnockdownDialogDelayedAndGuarded",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationQueueKnockdownDialogDelayedAndGuardedSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattlePresentationQueueSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* Killer = Fx.MakeSimpleDamageCard(0, 100);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Killer, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	const FBattleSnapshot InitialSnapshot = Session->BuildSnapshot();
	const FGuid KillerId = FWacomBattleFixture::FindHandInstanceByCardId(InitialSnapshot, Killer->CardId);
	const FGuid HeadId = FWacomBattleFixture::FindPartInstanceId(InitialSnapshot, 0);
	TestTrue(TEXT("Play killer card"),
		Session->SubmitCommand(FWacomBattleFixture::MakePlayCardOnPartInstance(
			InitialSnapshot,
			KillerId,
			HeadId)).IsOk());
	TestTrue(TEXT("Session is pending knockdown"), Session->BuildPendingKnockdownChoiceView().bHasPendingChoice);
	Session->ConsumeEvents();

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity,
		SpawnParams);
	if (!TestNotNull(TEXT("PlayerController spawned"), PC))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->SetSession(Session);

	FBattleEvent IntroCue;
	IntroCue.Type = EBattleEventType::DamageDealt;
	IntroCue.Sequence = 1;
	IntroCue.ActorEnemyPartKey = FWacomBattleFixture::FindPartKeyByInstanceId(Session->BuildSnapshot(), HeadId);
	IntroCue.Amount = 100;

	FBattleEvent KnockdownRequest;
	KnockdownRequest.Type = EBattleEventType::KnockdownChoiceRequested;
	KnockdownRequest.Sequence = 2;

	HUD->EnqueueBattlePresentationEventsForTest({ IntroCue, KnockdownRequest });

	World->GetTimerManager().Tick(0.01f);
	TestTrue(TEXT("Target cue delays knockdown modal step"), HUD->IsBattlePresentationBusy());

	HUD->AdvanceBattlePresentationQueueForTest();
	TestFalse(TEXT("Knockdown step is consumed after the pacing delay"), HUD->IsBattlePresentationBusy());
	TestTrue(TEXT("Valid pending choice is still available for the dialog path"),
		Session->BuildPendingKnockdownChoiceView().bHasPendingChoice);

	HUD->ClearBattlePresentationQueueForTest();
	TestTrue(TEXT("Resolve pending knockdown choice"),
		Session->SubmitCommand(FBattleCommand::MakeKnockdownChoice(EKnockdownChoice::Aid)).IsOk());
	Session->ConsumeEvents();
	TestFalse(TEXT("No pending choice remains after Aid"),
		Session->BuildPendingKnockdownChoiceView().bHasPendingChoice);

	HUD->EnqueueBattlePresentationEventsForTest({ KnockdownRequest });
	World->GetTimerManager().Tick(0.01f);
	TestFalse(TEXT("Stale knockdown request is guarded and finishes without a modal dependency"),
		HUD->IsBattlePresentationBusy());

	return true;
}
