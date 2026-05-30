// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Commands/BattleCommand.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Types/WacomInteractionTargetTypes.h"

namespace
{
	const FHandCardSnapshot* FindHandCard(const FBattleSnapshot& Snapshot, const FGuid& CardId)
	{
		for (const FHandCardSnapshot& Card : Snapshot.Hand.Cards)
		{
			if (Card.InstanceId == CardId)
			{
				return &Card;
			}
		}
		return nullptr;
	}

	int32 GetRuntimeCostInHand(const FBattleSnapshot& Snapshot, const FGuid& CardId)
	{
		if (const FHandCardSnapshot* Card = FindHandCard(Snapshot, CardId))
		{
			return Card->RuntimeCost;
		}
		return INDEX_NONE;
	}

	UBattleSession* CreateCardToCardSession(
		FWacomBattleFixture& Fixture,
		UCardDefinition*& OutSourceCard,
		UCardDefinition*& OutTargetCard,
		bool bReduceCost = false)
	{
		OutSourceCard = Fixture.MakeHandCardCostModifierCard(/*Cost*/0, /*Magnitude*/2, bReduceCost);
		OutTargetCard = Fixture.MakeNoopCard(/*Cost*/3);
		TArray<UCardDefinition*> Deck = { OutSourceCard, OutTargetCard };
		for (int32 Index = 0; Index < 3; ++Index)
		{
			Deck.Add(Fixture.MakeNoopCard(0));
		}

		return Fixture.CreateSession(
			Fixture.MakeCharacter(Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0), Deck),
			Fixture.MakeSinglePartEnemy(/*Hp*/100, /*Initiative*/50, /*IntentResist*/0),
			1);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleHandCardTargetAcceptsAnotherHandCardSpec,
	"Wacom.Battle.CardToCardTarget.HandCardTargetAcceptsAnotherHandCard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleHandCardTargetAcceptsAnotherHandCardSpec::RunTest(const FString& Parameters)
{
	FWacomBattleFixture Fx;
	UCardDefinition* SourceDef = nullptr;
	UCardDefinition* TargetDef = nullptr;
	UBattleSession* Session = CreateCardToCardSession(Fx, SourceDef, TargetDef);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, SourceDef->CardId);
	const FGuid TargetId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, TargetDef->CardId);

	TestTrue(TEXT("Source exists"), SourceId.IsValid());
	TestTrue(TEXT("Target exists"), TargetId.IsValid());
	TestTrue(TEXT("Other hand card can be targeted"),
		Session->CanTargetWithCard(SourceId, FWacomInteractionTargetHandle::ForCardTarget(TargetId, Session)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleHandCardTargetRejectsSelfMissingOrNotInHandSpec,
	"Wacom.Battle.CardToCardTarget.HandCardTargetRejectsSelfMissingOrNotInHand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleHandCardTargetRejectsSelfMissingOrNotInHandSpec::RunTest(const FString& Parameters)
{
	FWacomBattleFixture Fx;
	UCardDefinition* SourceDef = nullptr;
	UCardDefinition* TargetDef = nullptr;
	UBattleSession* Session = CreateCardToCardSession(Fx, SourceDef, TargetDef);
	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, SourceDef->CardId);
	const FGuid TargetId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, TargetDef->CardId);
	TestTrue(TEXT("Source exists"), SourceId.IsValid());
	TestTrue(TEXT("Target exists"), TargetId.IsValid());

	TestFalse(TEXT("Self target rejects"),
		Session->CanTargetWithCard(SourceId, FWacomInteractionTargetHandle::ForCardTarget(SourceId, Session)));
	TestFalse(TEXT("Missing target rejects"),
		Session->CanTargetWithCard(SourceId, FWacomInteractionTargetHandle::ForCardTarget(FGuid::NewGuid(), Session)));

	TestTrue(TEXT("Play source to move it out of hand"),
		Session->SubmitCommand(FBattleCommand::MakePlayCardOnHandCard(SourceId, TargetId)).IsOk());
	Snapshot = Session->BuildSnapshot();
	TestFalse(TEXT("Source no longer in hand"), FindHandCard(Snapshot, SourceId) != nullptr);
	TestFalse(TEXT("Target card cannot target source once source left hand"),
		Session->CanTargetWithCard(TargetId, FWacomInteractionTargetHandle::ForCardTarget(SourceId, Session)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleSelectedHandCardAddCostSpec,
	"Wacom.Battle.CardToCardTarget.SelectedHandCardAddCostAppliesToExactTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleSelectedHandCardAddCostSpec::RunTest(const FString& Parameters)
{
	FWacomBattleFixture Fx;
	UCardDefinition* SourceDef = nullptr;
	UCardDefinition* TargetDef = nullptr;
	UBattleSession* Session = CreateCardToCardSession(Fx, SourceDef, TargetDef);
	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, SourceDef->CardId);
	const FGuid TargetId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, TargetDef->CardId);

	TestEqual(TEXT("Initial target cost"), GetRuntimeCostInHand(Snapshot, TargetId), 3);
	TestTrue(TEXT("Play card on selected target"),
		Session->SubmitCommand(FBattleCommand::MakePlayCardOnHandCard(SourceId, TargetId)).IsOk());
	Snapshot = Session->BuildSnapshot();
	TestEqual(TEXT("Selected target cost increased"), GetRuntimeCostInHand(Snapshot, TargetId), 5);
	TestFalse(TEXT("Source left hand after play"), FindHandCard(Snapshot, SourceId) != nullptr);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleSelectedHandCardReduceCostSpec,
	"Wacom.Battle.CardToCardTarget.SelectedHandCardReduceCostAppliesToExactTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleSelectedHandCardReduceCostSpec::RunTest(const FString& Parameters)
{
	FWacomBattleFixture Fx;
	UCardDefinition* SourceDef = nullptr;
	UCardDefinition* TargetDef = nullptr;
	UBattleSession* Session = CreateCardToCardSession(Fx, SourceDef, TargetDef, /*bReduceCost*/true);
	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, SourceDef->CardId);
	const FGuid TargetId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, TargetDef->CardId);

	TestEqual(TEXT("Initial target cost"), GetRuntimeCostInHand(Snapshot, TargetId), 3);
	TestTrue(TEXT("Play card on selected target"),
		Session->SubmitCommand(FBattleCommand::MakePlayCardOnHandCard(SourceId, TargetId)).IsOk());
	Snapshot = Session->BuildSnapshot();
	TestEqual(TEXT("Selected target cost reduced"), GetRuntimeCostInHand(Snapshot, TargetId), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleMissingTargetCardForHandCardPlayFailsSpec,
	"Wacom.Battle.CardToCardTarget.MissingTargetCardForHandCardPlayFails",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleMissingTargetCardForHandCardPlayFailsSpec::RunTest(const FString& Parameters)
{
	FWacomBattleFixture Fx;
	UCardDefinition* SourceDef = nullptr;
	UCardDefinition* TargetDef = nullptr;
	UBattleSession* Session = CreateCardToCardSession(Fx, SourceDef, TargetDef);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, SourceDef->CardId);

	const FWacomStatus MissingStatus = Session->SubmitCommand(FBattleCommand::MakePlayCard(SourceId, FGuid()));
	TestFalse(TEXT("Missing selected hand card target fails"), MissingStatus.IsOk());
	TestEqual(TEXT("Missing target is illegal target"),
		static_cast<int32>(MissingStatus.Code),
		static_cast<int32>(EWacomError::IllegalTarget));

	const FWacomStatus SelfStatus = Session->SubmitCommand(FBattleCommand::MakePlayCardOnHandCard(SourceId, SourceId));
	TestFalse(TEXT("Self selected hand card target fails"), SelfStatus.IsOk());
	TestEqual(TEXT("Self target is illegal target"),
		static_cast<int32>(SelfStatus.Code),
		static_cast<int32>(EWacomError::IllegalTarget));
	return true;
}
