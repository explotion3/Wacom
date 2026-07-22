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
#include "Resolution/BattleTargetValidationResult.h"
#include "Runtime/BattleEnemyKeys.h"
#include "RunSession.h"
#include "RunState.h"
#include "Session/BattleSession.h"
#include "Session/BattleResultPacket.h"
#include "Snapshots/BattleSnapshot.h"
#include "Types/WacomInteractionTargetTypes.h"

#include "UObject/StrongObjectPtr.h"

namespace
{
	UEnemyPartDefinition* MakeTargetIdentityPart(
		FWacomBattleFixture& Fx,
		FName PartId,
		int32 Hp,
		int32 Initiative)
	{
		UEnemyDefinition* TemplateEnemy = Fx.MakeSinglePartEnemy(Hp, Initiative);
		return TemplateEnemy && TemplateEnemy->Parts.Num() > 0
			? TemplateEnemy->Parts[0].PartDef
			: nullptr;
	}

	UEnemyDefinition* MakeSingleSlotEnemyWithSharedPartId(
		FWacomBattleFixture& Fx,
		FName EnemyId,
		FName PartId,
		FName PartSlotId)
	{
		UEnemyPartDefinition* Part = MakeTargetIdentityPart(Fx, PartId, /*Hp*/50, /*Initiative*/50);
		if (!Part)
		{
			return nullptr;
		}
		Part->PartId = PartId;

		UEnemyDefinition* Enemy = NewObject<UEnemyDefinition>(GetTransientPackage(), NAME_None, RF_Transient);
		Enemy->EnemyId = EnemyId;
		FEnemyPartSlot Slot;
		Slot.PartDef = Part;
		Slot.PartSlotId = PartSlotId;
		Enemy->Parts.Add(Slot);
		return Enemy;
	}

	const FEnemyPartSnapshot* FindPartBySlot(
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

	struct FTwoEnemySlotTargetFixture
	{
		TObjectPtr<UCharacterDefinition> Character = nullptr;
		TObjectPtr<UEnemyDefinition> LeftEnemy = nullptr;
		TObjectPtr<UEnemyDefinition> RightEnemy = nullptr;
		TObjectPtr<UCardDefinition> DamageCard = nullptr;
	};

	FTwoEnemySlotTargetFixture MakeTwoEnemySlotTargetFixture(
		FWacomBattleFixture& Fx,
		int32 Damage,
		FName PartDefinitionId = TEXT("Test.Part.Shared"))
	{
		FTwoEnemySlotTargetFixture Fixture;
		Fixture.DamageCard = Fx.MakeSimpleDamageCard(/*Cost*/0, Damage);
		Fixture.Character = Fx.MakeCharacter(
			Fx.MakeNoopCard(/*Cost*/0),
			Fixture.DamageCard,
			{ Fx.MakeNoopCard(/*Cost*/0), Fx.MakeNoopCard(/*Cost*/0), Fx.MakeNoopCard(/*Cost*/0) });

		Fixture.LeftEnemy = MakeSingleSlotEnemyWithSharedPartId(
			Fx,
			TEXT("Test.Enemy.Left"),
			PartDefinitionId,
			TEXT("Core"));
		Fixture.RightEnemy = MakeSingleSlotEnemyWithSharedPartId(
			Fx,
			TEXT("Test.Enemy.Right"),
			PartDefinitionId,
			TEXT("Core"));
		return Fixture;
	}

	void AppendTwoEnemySlots(
		FBattleInitParams& Params,
		const FTwoEnemySlotTargetFixture& Fixture)
	{
		FBattleEnemySlotInit LeftSlot;
		LeftSlot.Enemy = Fixture.LeftEnemy;
		LeftSlot.EnemySlotId = TEXT("LeftEnemy");
		Params.EnemySlots.Add(LeftSlot);

		FBattleEnemySlotInit RightSlot;
		RightSlot.Enemy = Fixture.RightEnemy;
		RightSlot.EnemySlotId = TEXT("RightEnemy");
		Params.EnemySlots.Add(RightSlot);
	}

	UBattleSession* CreateTwoEnemySlotTargetSession(
		FWacomBattleFixture& Fx,
		UCardDefinition*& OutDamageCard,
		FName PartDefinitionId = TEXT("Test.Part.Shared"))
	{
		const FTwoEnemySlotTargetFixture Fixture =
			MakeTwoEnemySlotTargetFixture(Fx, /*Damage*/10, PartDefinitionId);
		OutDamageCard = Fixture.DamageCard;

		FBattleInitParams Params;
		Params.Character = Fixture.Character;
		Params.EncounterId = TEXT("Encounter.TargetIdentity");
		Params.RandomSeed = 1;
		AppendTwoEnemySlots(Params, Fixture);

		UBattleSession* Session = NewObject<UBattleSession>(GetTransientPackage(), NAME_None, RF_Transient);
		const FBattleInitializationResult Status = Session->Initialize(Params);
		if (!ensure(Status.IsOk()))
		{
			return nullptr;
		}
		return Session;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleTargetIdentityPartKeyResolvesDuplicatePartIdsSpec,
	"Wacom.Battle.TargetIdentity.PartKeyResolvesDuplicatePartIds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleTargetIdentityPartKeyResolvesDuplicatePartIdsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* DamageCard = nullptr;
	TStrongObjectPtr<UBattleSession> Session(CreateTwoEnemySlotTargetSession(Fx, DamageCard));
	if (!TestNotNull(TEXT("Session created"), Session.Get()))
	{
		return false;
	}

	const FBattleSnapshot Before = Session->BuildSnapshot();
	const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Before, DamageCard->CardId);
	const FEnemyPartSnapshot* LeftPart = FindPartBySlot(Before, TEXT("LeftEnemy"), TEXT("Core"));
	const FEnemyPartSnapshot* RightPart = FindPartBySlot(Before, TEXT("RightEnemy"), TEXT("Core"));

	TestTrue(TEXT("Damage card in hand"), CardId.IsValid());
	TestNotNull(TEXT("Left part exists"), LeftPart);
	TestNotNull(TEXT("Right part exists"), RightPart);
	if (!CardId.IsValid() || !LeftPart || !RightPart)
	{
		return false;
	}
	TestNotEqual(TEXT("Duplicate part definition still has distinct runtime ids"),
		LeftPart->InstanceId,
		RightPart->InstanceId);

	FWacomInteractionTargetHandle RightHandle = FWacomInteractionTargetHandle::ForWorldTarget(
		FGuid(),
		nullptr,
		FVector::ZeroVector,
		FVector2D::ZeroVector,
		FGameplayTag(),
		TEXT("Test.Part.Shared"),
		TEXT("Encounter.TargetIdentity"),
		TEXT("RightEnemy"),
		TEXT("Core"));

	const FWacomBattleTargetValidationResult Validation =
		Session->ValidateTargetWithCard(CardId, RightHandle);
	TestTrue(TEXT("Slot-only world handle can target right enemy"), Validation.bCanTarget);
	TestEqual(TEXT("Validation resolves right runtime part"),
		Validation.ResolvedPartInstanceId,
		RightPart->InstanceId);
	TestEqual(TEXT("Validation preserves resolved enemy slot"),
		Validation.ResolvedPartIdentity.GetEffectiveEnemySlotId(),
		FName(TEXT("RightEnemy")));
	TestEqual(TEXT("Validation resolves right part key"),
		Validation.ResolvedPartKey,
		RightPart->PartKey);

	const FBattleResolution Status =
		Session->ResolveCommand(FBattleCommand::MakePlayCardOnEnemyPartKey(CardId, RightPart->PartKey));
	TestTrue(TEXT("PlayCard by part key succeeds"), Status.IsOk());

	const FBattleSnapshot After = Session->BuildSnapshot();
	const FEnemyPartSnapshot* LeftAfter = FindPartBySlot(After, TEXT("LeftEnemy"), TEXT("Core"));
	const FEnemyPartSnapshot* RightAfter = FindPartBySlot(After, TEXT("RightEnemy"), TEXT("Core"));
	TestNotNull(TEXT("Left part still exists after play"), LeftAfter);
	TestNotNull(TEXT("Right part still exists after play"), RightAfter);
	if (LeftAfter && RightAfter)
	{
		TestEqual(TEXT("Left enemy HP unchanged"), LeftAfter->CurrentHp, LeftPart->CurrentHp);
		TestEqual(TEXT("Right enemy HP took damage"), RightAfter->CurrentHp, RightPart->CurrentHp - 10);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleTargetIdentityMismatchedRuntimeAndKeyRejectsSpec,
	"Wacom.Battle.TargetIdentity.RuntimeIdAndKeyMismatchRejects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleTargetIdentityMismatchedRuntimeAndKeyRejectsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* DamageCard = nullptr;
	TStrongObjectPtr<UBattleSession> Session(CreateTwoEnemySlotTargetSession(Fx, DamageCard));
	if (!TestNotNull(TEXT("Session created"), Session.Get()))
	{
		return false;
	}

	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, DamageCard->CardId);
	const FEnemyPartSnapshot* LeftPart = FindPartBySlot(Snapshot, TEXT("LeftEnemy"), TEXT("Core"));
	TestTrue(TEXT("Damage card in hand"), CardId.IsValid());
	TestNotNull(TEXT("Left part exists"), LeftPart);
	if (!CardId.IsValid() || !LeftPart)
	{
		return false;
	}

	FWacomInteractionTargetHandle MismatchHandle = FWacomInteractionTargetHandle::ForWorldTarget(
		LeftPart->InstanceId,
		nullptr,
		FVector::ZeroVector,
		FVector2D::ZeroVector,
		FGameplayTag(),
		TEXT("Test.Part.Shared"),
		TEXT("Encounter.TargetIdentity"),
		TEXT("RightEnemy"),
		TEXT("Core"));

	const FWacomBattleTargetValidationResult Validation =
		Session->ValidateTargetWithCard(CardId, MismatchHandle);
	TestFalse(TEXT("Validation rejects mismatched runtime id and slot identity"), Validation.bCanTarget);
	TestEqual(TEXT("Validation reports identity mismatch"),
		Validation.RejectReason,
		EWacomBattleTargetRejectReason::TargetIdentityMismatch);

	const FBattleEnemyPartKey MissingKey =
		FBattleEnemyPartKey::Make(TEXT("Encounter.TargetIdentity"), TEXT("RightEnemy"), TEXT("Missing"));
	const FBattleResolution Status =
		Session->ResolveCommand(FBattleCommand::MakePlayCardOnEnemyPartKey(CardId, MissingKey));
	TestFalse(TEXT("PlayCard rejects stale or missing part key"), Status.IsOk());
	TestEqual(TEXT("Reject detail reports invalid key"),
		Status.Status.Detail,
		FName(TEXT("TargetKeyInvalid")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleTargetIdentityRunProgressRestoresOnlyMatchingEnemySlotSpec,
	"Wacom.Battle.TargetIdentity.RunBattleProgressRestoresOnlyMatchingEnemySlot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleTargetIdentityRunProgressRestoresOnlyMatchingEnemySlotSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	const FTwoEnemySlotTargetFixture Fixture =
		MakeTwoEnemySlotTargetFixture(Fx, /*Damage*/100, TEXT("Test.Part.Shared"));
	if (!TestNotNull(TEXT("Character"), Fixture.Character.Get())
		|| !TestNotNull(TEXT("Left enemy"), Fixture.LeftEnemy.Get())
		|| !TestNotNull(TEXT("Right enemy"), Fixture.RightEnemy.Get())
		|| !TestNotNull(TEXT("Damage card"), Fixture.DamageCard.Get()))
	{
		return false;
	}

	FWacomRunExplorationFixture Exploration;
	UWacomFloorMapDefinition* Floor =
		Exploration.MakeLinearFloor(TEXT("TargetIdentity.Progress.Floor"), 1);
	Floor->Nodes[0].NodeType = EWacomMapNodeType::Encounter;
	URunSession* Run = Exploration.CreateInitializedSession(
		Fixture.Character.Get(),
		Exploration.MakeJourney({ Floor }, TEXT("TargetIdentity.Progress.Journey"))).Session;
	const FRunExplorationResolution EncounterBegin =
		Run->BeginCurrentNodeActivity(ERunNodeActivityKind::Encounter);
	if (!TestTrue(TEXT("Encounter begins"),
		EncounterBegin.IsOk() && EncounterBegin.NodeActivityTicket.IsSet()))
	{
		return false;
	}

	{
		FBattleInitParams Params;
		Params.Character = Fixture.Character;
		Params.EncounterId = TEXT("ProgressTrigger");
		Params.RandomSeed = 1;
		AppendTwoEnemySlots(Params, Fixture);

		TStrongObjectPtr<UBattleSession> Session(NewObject<UBattleSession>());
		const FBattleInitializationResult InitStatus = Session->Initialize(Params);
		TestTrue(TEXT("Initial battle initializes"), InitStatus.IsOk());
		if (!InitStatus.IsOk())
		{
			return false;
		}

		const FBattleSnapshot Before = Session->BuildSnapshot();
		const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Before, Fixture.DamageCard->CardId);
		const FEnemyPartSnapshot* RightPart = FindPartBySlot(Before, TEXT("RightEnemy"), TEXT("Core"));
		TestTrue(TEXT("Damage card in hand"), CardId.IsValid());
		TestNotNull(TEXT("Right part exists"), RightPart);
		if (!CardId.IsValid() || !RightPart)
		{
			return false;
		}

		const FBattleResolution PlayStatus =
			Session->ResolveCommand(FBattleCommand::MakePlayCardOnEnemyPartKey(CardId, RightPart->PartKey));
		TestTrue(TEXT("PlayCard destroys right enemy slot"), PlayStatus.IsOk());
		TestEqual(TEXT("Pending knockdown choice"), Session->GetPhase(), EBattlePhase::PendingKnockdownChoice);

		const FBattleResolution WithdrawStatus =
			Session->ResolveCommand(FBattleCommand::MakeKnockdownChoice(EKnockdownChoice::Withdraw));
		TestTrue(TEXT("Withdraw succeeds"), WithdrawStatus.IsOk());

		const FBattleResultPacket Packet = Session->BuildResultPacket();
		TestTrue(TEXT("Packet withdrawn"), Packet.bWithdrawn);
		TestEqual(TEXT("Packet records one destroyed part key"), Packet.DestroyedPartKeys.Num(), 1);
		if (Packet.DestroyedPartKeys.Num() == 1)
		{
			TestEqual(TEXT("Destroyed key is right part"),
				Packet.DestroyedPartKeys[0],
				RightPart->PartKey);
		}
		TestEqual(TEXT("Packet records one destroyed part identity"), Packet.DestroyedParts.Num(), 1);
		if (Packet.DestroyedParts.Num() == 1)
		{
			TestEqual(TEXT("Destroyed identity enemy slot"),
				Packet.DestroyedParts[0].GetEffectiveEnemySlotId(),
				FName(TEXT("RightEnemy")));
			TestEqual(TEXT("Destroyed identity part slot"),
				Packet.DestroyedParts[0].GetEffectivePartSlotId(),
				FName(TEXT("Core")));
		}

		TestTrue(TEXT("Withdraw settlement succeeds"),
			Run->SettleEncounterNodeActivity(
				EncounterBegin.NodeActivityTicket.GetValue(), Packet).IsOk());
	}

	const FBattleProgressSnapshot* Progress =
		Run->GetRunState().BattleProgress.Find(
			FWacomMapNodeHandle{ TEXT("TargetIdentity.Progress.Floor"), TEXT("Node.01") });
	TestNotNull(TEXT("Run progress exists"), Progress);
	if (!Progress)
	{
		return false;
	}
	TestEqual(TEXT("Run progress stores one destroyed part key"), Progress->DestroyedPartKeys.Num(), 1);
	if (Progress->DestroyedPartKeys.Num() == 1)
	{
		TestEqual(TEXT("Run progress key is right part"),
			Progress->DestroyedPartKeys[0],
			FBattleEnemyPartKey::Make(TEXT("ProgressTrigger"), TEXT("RightEnemy"), TEXT("Core")));
	}
	TestEqual(TEXT("Run progress does not duplicate destroyed identity projection"),
		Progress->DestroyedParts.Num(),
		0);

	FBattleInitParams ReentryParams;
	const bool bBuildOk =
		Run->BuildInitParamsForBattle(FName(TEXT("ProgressTrigger")), ReentryParams);
	TestTrue(TEXT("Run builds reentry params"), bBuildOk);
	TestEqual(TEXT("Run reentry EncounterId uses trigger id"),
		ReentryParams.EncounterId,
		FName(TEXT("ProgressTrigger")));
	TestEqual(TEXT("Run reentry fills PreDestroyedParts"), ReentryParams.PreDestroyedParts.Num(), 1);

	ReentryParams.EnemySlots.Reset();
	AppendTwoEnemySlots(ReentryParams, Fixture);

	TStrongObjectPtr<UBattleSession> ReentrySession(NewObject<UBattleSession>());
	const FBattleInitializationResult ReentryStatus = ReentrySession->Initialize(ReentryParams);
	TestTrue(TEXT("Reentry battle initializes"), ReentryStatus.IsOk());
	if (!ReentryStatus.IsOk())
	{
		return false;
	}

	const FBattleSnapshot ReentrySnapshot = ReentrySession->BuildSnapshot();
	const FEnemyPartSnapshot* LeftAfter = FindPartBySlot(ReentrySnapshot, TEXT("LeftEnemy"), TEXT("Core"));
	const FEnemyPartSnapshot* RightAfter = FindPartBySlot(ReentrySnapshot, TEXT("RightEnemy"), TEXT("Core"));
	TestNotNull(TEXT("Left part exists after reentry"), LeftAfter);
	TestNotNull(TEXT("Right part exists after reentry"), RightAfter);
	if (LeftAfter && RightAfter)
	{
		TestFalse(TEXT("Left duplicate part remains alive"), LeftAfter->bDestroyed);
		TestTrue(TEXT("Right matching part is pre-destroyed"), RightAfter->bDestroyed);
		TestEqual(TEXT("Right HP is zero"), RightAfter->CurrentHp, 0);
	}

	return true;
}
