// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Commands/BattleCommand.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Map/WacomFloorMapDefinition.h"
#include "RunSession.h"
#include "RunState.h"
#include "Session/BattleResultPacket.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"

#include "UObject/StrongObjectPtr.h"

namespace
{
	UEnemyDefinition* MakeRunProgressSinglePartEnemy(
		FWacomBattleFixture& Fx,
		FName EnemyId,
		FName SharedPartId,
		FName PartSlotId)
	{
		UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(/*Hp*/30, /*Initiative*/50);
		if (!Enemy || Enemy->Parts.Num() == 0 || !Enemy->Parts[0].PartDef)
		{
			return nullptr;
		}

		Enemy->EnemyId = EnemyId;
		Enemy->Parts[0].PartSlotId = PartSlotId;
		Enemy->Parts[0].PartDef->PartId = SharedPartId;
		return Enemy;
	}

	void AppendRunProgressEnemySlots(
		FBattleInitParams& Params,
		UEnemyDefinition* LeftEnemy,
		UEnemyDefinition* RightEnemy)
	{
		FBattleEnemySlotInit LeftSlot;
		LeftSlot.EnemySlotId = TEXT("LeftEnemy");
		LeftSlot.Enemy = LeftEnemy;
		Params.EnemySlots.Add(LeftSlot);

		FBattleEnemySlotInit RightSlot;
		RightSlot.EnemySlotId = TEXT("RightEnemy");
		RightSlot.Enemy = RightEnemy;
		Params.EnemySlots.Add(RightSlot);
	}

	const FEnemyPartSnapshot* FindRunProgressPartBySlot(
		const FBattleSnapshot& Snapshot,
		FName EnemySlotId,
		FName PartSlotId)
	{
		for (const FEnemySnapshot& Enemy : Snapshot.Enemies)
		{
			if (Enemy.EnemySlotId != EnemySlotId)
			{
				continue;
			}

			for (const FEnemyPartSnapshot& Part : Enemy.Parts)
			{
				if (Part.PartSlotId == PartSlotId)
				{
					return &Part;
				}
			}
		}
		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunBattleProgressDestroyedPartKeysRestoreOnlyMatchingEnemySlotSpec,
	"Wacom.Run.BattleProgress.DestroyedPartKeysRestoreOnlyMatchingEnemySlot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunBattleProgressDestroyedPartKeysRestoreOnlyMatchingEnemySlotSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* KillerCard = Fx.MakeSimpleDamageCard(/*Cost*/0, /*Damage*/100);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(/*Cost*/0),
		Fx.MakeNoopCard(/*Cost*/0),
		{
			KillerCard,
			Fx.MakeNoopCard(/*Cost*/0),
			Fx.MakeNoopCard(/*Cost*/0),
			Fx.MakeNoopCard(/*Cost*/0),
			Fx.MakeNoopCard(/*Cost*/0),
		});
	UEnemyDefinition* LeftEnemy = MakeRunProgressSinglePartEnemy(
		Fx,
		TEXT("Test.Enemy.Left"),
		TEXT("Test.Part.Shared"),
		TEXT("Core"));
	UEnemyDefinition* RightEnemy = MakeRunProgressSinglePartEnemy(
		Fx,
		TEXT("Test.Enemy.Right"),
		TEXT("Test.Part.Shared"),
		TEXT("Core"));

	if (!TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Killer card"), KillerCard)
		|| !TestNotNull(TEXT("Left enemy"), LeftEnemy)
		|| !TestNotNull(TEXT("Right enemy"), RightEnemy))
	{
		return false;
	}

	FWacomRunExplorationFixture Exploration;
	UWacomFloorMapDefinition* Floor =
		Exploration.MakeLinearFloor(TEXT("BattleProgress.Floor"), 1);
	Floor->Nodes[0].NodeType = EWacomMapNodeType::Encounter;
	URunSession* Run = Exploration.CreateInitializedSession(
		Character,
		Exploration.MakeJourney({ Floor }, TEXT("BattleProgress.Journey"))).Session;
	const FRunExplorationResolution EncounterBegin =
		Run->BeginCurrentNodeActivity(ERunNodeActivityKind::Encounter);
	if (!TestTrue(TEXT("Encounter begins"),
		EncounterBegin.IsOk() && EncounterBegin.NodeActivityTicket.IsSet()))
	{
		return false;
	}

	FBattleEnemyPartKey DestroyedRightPartKey;
	{
		FBattleInitParams Params;
		Params.Character = Character;
		Params.EncounterId = TEXT("ProgressTrigger");
		Params.RandomSeed = 1;
		AppendRunProgressEnemySlots(Params, LeftEnemy, RightEnemy);

		TStrongObjectPtr<UBattleSession> Session(NewObject<UBattleSession>());
		const FBattleInitializationResult InitStatus = Session->Initialize(Params);
		TestTrue(TEXT("Initial battle initializes"), InitStatus.IsOk());
		if (!InitStatus.IsOk())
		{
			return false;
		}

		const FBattleSnapshot Before = Session->BuildSnapshot();
		const FGuid KillerCardId = FWacomBattleFixture::FindHandInstanceByCardId(Before, KillerCard->CardId);
		const FEnemyPartSnapshot* RightPart =
			FindRunProgressPartBySlot(Before, TEXT("RightEnemy"), TEXT("Core"));
		TestTrue(TEXT("Killer card is in hand"), KillerCardId.IsValid());
		TestNotNull(TEXT("Right enemy part exists"), RightPart);
		if (!KillerCardId.IsValid() || !RightPart)
		{
			return false;
		}

		DestroyedRightPartKey = RightPart->PartKey;
		const FBattleResolution PlayStatus =
			Session->ResolveCommand(FBattleCommand::MakePlayCardOnEnemyPartKey(KillerCardId, DestroyedRightPartKey));
		TestTrue(TEXT("Play card destroys right part"), PlayStatus.IsOk());
		TestEqual(TEXT("Pending knockdown choice"), Session->GetPhase(), EBattlePhase::PendingKnockdownChoice);

		const FBattleResolution WithdrawStatus =
			Session->ResolveCommand(FBattleCommand::MakeKnockdownChoice(EKnockdownChoice::Withdraw));
		TestTrue(TEXT("Withdraw succeeds"), WithdrawStatus.IsOk());

		const FBattleResultPacket Packet = Session->BuildResultPacket();
		TestTrue(TEXT("Packet is withdrawn"), Packet.bWithdrawn);
		TestEqual(TEXT("Packet has one destroyed part key"), Packet.DestroyedPartKeys.Num(), 1);
		if (Packet.DestroyedPartKeys.Num() == 1)
		{
			TestEqual(TEXT("Packet key is right part"), Packet.DestroyedPartKeys[0], DestroyedRightPartKey);
		}

		TestTrue(TEXT("Withdraw settlement succeeds"),
			Run->SettleEncounterNodeActivity(
				EncounterBegin.NodeActivityTicket.GetValue(), Packet).IsOk());
	}

	const FBattleProgressSnapshot* Progress =
		Run->GetRunState().BattleProgress.Find(
			FWacomMapNodeHandle{ TEXT("BattleProgress.Floor"), TEXT("Node.01") });
	TestNotNull(TEXT("Run progress exists"), Progress);
	if (!Progress)
	{
		return false;
	}

	TestEqual(TEXT("Run progress stores one destroyed part key"), Progress->DestroyedPartKeys.Num(), 1);
	if (Progress->DestroyedPartKeys.Num() == 1)
	{
		TestEqual(TEXT("Run progress key is right enemy part"),
			Progress->DestroyedPartKeys[0],
			DestroyedRightPartKey);
	}
	TestEqual(TEXT("Run progress does not duplicate destroyed identity projection"),
		Progress->DestroyedParts.Num(),
		0);

	FBattleInitParams ReentryParams;
	const bool bBuildOk = Run->BuildInitParamsForBattle(
		Run->BuildExplorationSnapshot().CurrentNode,
		FName(TEXT("ProgressTrigger")),
		ReentryParams);
	TestTrue(TEXT("Run builds reentry params"), bBuildOk);
	TestEqual(TEXT("Reentry EncounterId uses trigger id"),
		ReentryParams.EncounterId,
		FName(TEXT("ProgressTrigger")));
	TestEqual(TEXT("Reentry fills one PreDestroyedParts identity"),
		ReentryParams.PreDestroyedParts.Num(),
		1);

	ReentryParams.EnemySlots.Reset();
	AppendRunProgressEnemySlots(ReentryParams, LeftEnemy, RightEnemy);

	TStrongObjectPtr<UBattleSession> ReentrySession(NewObject<UBattleSession>());
	const FBattleInitializationResult ReentryStatus = ReentrySession->Initialize(ReentryParams);
	TestTrue(TEXT("Reentry battle initializes"), ReentryStatus.IsOk());
	if (!ReentryStatus.IsOk())
	{
		return false;
	}

	const FBattleSnapshot ReentrySnapshot = ReentrySession->BuildSnapshot();
	const FEnemyPartSnapshot* LeftAfter =
		FindRunProgressPartBySlot(ReentrySnapshot, TEXT("LeftEnemy"), TEXT("Core"));
	const FEnemyPartSnapshot* RightAfter =
		FindRunProgressPartBySlot(ReentrySnapshot, TEXT("RightEnemy"), TEXT("Core"));
	TestNotNull(TEXT("Left part exists after reentry"), LeftAfter);
	TestNotNull(TEXT("Right part exists after reentry"), RightAfter);
	if (LeftAfter && RightAfter)
	{
		TestFalse(TEXT("Left shared part stays alive"), LeftAfter->bDestroyed);
		TestTrue(TEXT("Right matching part is pre-destroyed"), RightAfter->bDestroyed);
		TestEqual(TEXT("Right HP is zero"), RightAfter->CurrentHp, 0);
	}

	return true;
}
