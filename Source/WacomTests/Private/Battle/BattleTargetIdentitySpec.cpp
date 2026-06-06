// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Commands/BattleCommand.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Resolution/BattleTargetValidationResult.h"
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
		UEnemyDefinition* TemplateEnemy = Fx.MakeSinglePartEnemy(Hp, Initiative, /*IntentResist*/0);
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
		const FWacomStatus Status = Session->Initialize(Params);
		if (!ensure(Status.IsOk()))
		{
			return nullptr;
		}
		return Session;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleTargetIdentitySlotHandleResolvesDuplicatePartIdsSpec,
	"Wacom.Battle.TargetIdentity.SlotHandleResolvesDuplicatePartIds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleTargetIdentitySlotHandleResolvesDuplicatePartIdsSpec::RunTest(const FString& /*Parameters*/)
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

	const FWacomStatus Status =
		Session->SubmitCommand(FBattleCommand::MakePlayCardOnPartSlot(CardId, TEXT("RightEnemy"), TEXT("Core")));
	TestTrue(TEXT("PlayCard by slot succeeds"), Status.IsOk());

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
	FWacomBattleTargetIdentityMismatchedRuntimeAndSlotRejectsSpec,
	"Wacom.Battle.TargetIdentity.RuntimeIdAndSlotMismatchRejects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleTargetIdentityMismatchedRuntimeAndSlotRejectsSpec::RunTest(const FString& /*Parameters*/)
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

	FBattleCommand Command = FBattleCommand::MakePlayCard(CardId, LeftPart->InstanceId);
	Command.TargetEnemySlotId = TEXT("RightEnemy");
	Command.TargetPartSlotId = TEXT("Core");
	const FWacomStatus Status = Session->SubmitCommand(Command);
	TestFalse(TEXT("PlayCard rejects mismatched runtime id and slot identity"), Status.IsOk());
	TestEqual(TEXT("Reject detail reports identity mismatch"),
		Status.Detail,
		FName(TEXT("TargetIdentityMismatch")));

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

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Fixture.Character);

	{
		FBattleInitParams Params;
		Params.Character = Fixture.Character;
		Params.EncounterId = TEXT("ProgressTrigger");
		Params.RandomSeed = 1;
		AppendTwoEnemySlots(Params, Fixture);

		TStrongObjectPtr<UBattleSession> Session(NewObject<UBattleSession>());
		const FWacomStatus InitStatus = Session->Initialize(Params);
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

		const FWacomStatus PlayStatus =
			Session->SubmitCommand(FBattleCommand::MakePlayCardOnPartSlot(CardId, TEXT("RightEnemy"), TEXT("Core")));
		TestTrue(TEXT("PlayCard destroys right enemy slot"), PlayStatus.IsOk());
		TestEqual(TEXT("Pending knockdown choice"), Session->GetPhase(), EBattlePhase::PendingKnockdownChoice);

		const FWacomStatus WithdrawStatus =
			Session->SubmitCommand(FBattleCommand::MakeKnockdownChoice(EKnockdownChoice::Withdraw));
		TestTrue(TEXT("Withdraw succeeds"), WithdrawStatus.IsOk());

		const FBattleResultPacket Packet = Session->BuildResultPacket();
		TestTrue(TEXT("Packet withdrawn"), Packet.bWithdrawn);
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

		Run->OnBattleFinishedFromTrigger(
			Packet,
			Fixture.RightEnemy.Get(),
			FName(TEXT("ProgressTrigger")));
	}

	const FBattleProgressSnapshot* Progress =
		Run->GetRunState().BattleProgress.Find(FName(TEXT("ProgressTrigger")));
	TestNotNull(TEXT("Run progress exists"), Progress);
	if (!Progress)
	{
		return false;
	}
	TestEqual(TEXT("Run progress stores one destroyed identity"), Progress->DestroyedParts.Num(), 1);
	TestEqual(TEXT("Run progress keeps legacy projection"), Progress->DestroyedPartIds.Num(), 1);

	FBattleInitParams ReentryParams;
	const bool bBuildOk =
		Run->BuildInitParamsForBattle(Fixture.RightEnemy.Get(), FName(TEXT("ProgressTrigger")), ReentryParams);
	TestTrue(TEXT("Run builds reentry params"), bBuildOk);
	TestEqual(TEXT("Run reentry EncounterId uses trigger id"),
		ReentryParams.EncounterId,
		FName(TEXT("ProgressTrigger")));
	TestEqual(TEXT("Run reentry fills PreDestroyedParts"), ReentryParams.PreDestroyedParts.Num(), 1);
	TestEqual(TEXT("Run reentry leaves legacy PreDestroyedPartIds empty"), ReentryParams.PreDestroyedPartIds.Num(), 0);

	ReentryParams.Enemy = nullptr;
	ReentryParams.EnemySlots.Reset();
	AppendTwoEnemySlots(ReentryParams, Fixture);

	TStrongObjectPtr<UBattleSession> ReentrySession(NewObject<UBattleSession>());
	const FWacomStatus ReentryStatus = ReentrySession->Initialize(ReentryParams);
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
