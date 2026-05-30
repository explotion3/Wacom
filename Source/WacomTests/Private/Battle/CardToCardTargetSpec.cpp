// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Commands/BattleCommand.h"
#include "Resolution/BattleTargetValidationResult.h"
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

	FGuid FindFirstHandAnchor(const FBattleSnapshot& Snapshot)
	{
		for (const FHandCardSnapshot& Card : Snapshot.Hand.Cards)
		{
			if (Card.bIsHandAnchor)
			{
				return Card.InstanceId;
			}
		}
		return FGuid();
	}

	int32 GetRuntimeCostInHand(const FBattleSnapshot& Snapshot, const FGuid& CardId)
	{
		if (const FHandCardSnapshot* Card = FindHandCard(Snapshot, CardId))
		{
			return Card->RuntimeCost;
		}
		return INDEX_NONE;
	}

	bool HasBattleEvent(UBattleSession* Session, EBattleEventType EventType, const FGuid& CardId)
	{
		if (!Session)
		{
			return false;
		}
		for (const FBattleEvent& Event : Session->ConsumeEvents())
		{
			if (Event.Type == EventType && (!CardId.IsValid() || Event.CardInstanceId == CardId))
			{
				return true;
			}
		}
		return false;
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

	UBattleSession* CreateSelectedHandCardZoneMoveSession(
		FWacomBattleFixture& Fixture,
		UCardDefinition*& OutSourceCard,
		UCardDefinition*& OutTargetCard,
		bool bExhaust)
	{
		OutSourceCard = Fixture.MakeSelectedHandCardZoneMoveCard(/*Cost*/0, bExhaust);
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

	UBattleSession* CreateExplicitFilterSession(
		FWacomBattleFixture& Fixture,
		UCardDefinition*& OutSourceCard,
		UCardDefinition*& OutNormalTargetCard,
		bool bAllowNormalHandCards,
		bool bAllowHandAnchors)
	{
		OutSourceCard = Fixture.MakeHandCardCostModifierCard(/*Cost*/0, /*Magnitude*/2, /*bReduceCost*/false);
		OutSourceCard->HandCardTargetFilter.bUseExplicitHandCardTargetFilter = true;
		OutSourceCard->HandCardTargetFilter.bAllowNormalHandCards = bAllowNormalHandCards;
		OutSourceCard->HandCardTargetFilter.bAllowHandAnchors = bAllowHandAnchors;
		OutNormalTargetCard = Fixture.MakeNoopCard(/*Cost*/3);

		TArray<UCardDefinition*> Deck = { OutSourceCard, OutNormalTargetCard };
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
	FWacomBattleExplicitFilterAllowsNormalHandCardsSpec,
	"Wacom.Battle.CardToCardTarget.ExplicitFilterAllowsNormalHandCards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleExplicitFilterAllowsNormalHandCardsSpec::RunTest(const FString& Parameters)
{
	FWacomBattleFixture Fx;
	UCardDefinition* SourceDef = nullptr;
	UCardDefinition* TargetDef = nullptr;
	UBattleSession* Session = CreateExplicitFilterSession(
		Fx,
		SourceDef,
		TargetDef,
		/*bAllowNormalHandCards*/ true,
		/*bAllowHandAnchors*/ false);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, SourceDef->CardId);
	const FGuid TargetId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, TargetDef->CardId);
	const FWacomInteractionTargetHandle Target = FWacomInteractionTargetHandle::ForCardTarget(TargetId, Session);

	const FWacomBattleTargetValidationResult Result = Session->ValidateTargetWithCard(SourceId, Target);
	TestTrue(TEXT("Explicit filter allows normal hand card"), Result.bCanTarget);
	TestTrue(TEXT("CanTarget mirrors explicit normal allow"), Session->CanTargetWithCard(SourceId, Target) == Result.bCanTarget);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleExplicitFilterAllowsHandAnchorsSpec,
	"Wacom.Battle.CardToCardTarget.ExplicitFilterAllowsHandAnchors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleExplicitFilterAllowsHandAnchorsSpec::RunTest(const FString& Parameters)
{
	FWacomBattleFixture Fx;
	UCardDefinition* SourceDef = nullptr;
	UCardDefinition* TargetDef = nullptr;
	UBattleSession* Session = CreateExplicitFilterSession(
		Fx,
		SourceDef,
		TargetDef,
		/*bAllowNormalHandCards*/ false,
		/*bAllowHandAnchors*/ true);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, SourceDef->CardId);
	const FGuid AnchorId = FindFirstHandAnchor(Snapshot);
	const FWacomInteractionTargetHandle Target = FWacomInteractionTargetHandle::ForCardTarget(AnchorId, Session);

	const FWacomBattleTargetValidationResult Result = Session->ValidateTargetWithCard(SourceId, Target);
	TestTrue(TEXT("Explicit filter allows hand anchor"), Result.bCanTarget);
	TestTrue(TEXT("CanTarget mirrors explicit anchor allow"), Session->CanTargetWithCard(SourceId, Target) == Result.bCanTarget);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleExplicitFilterRejectsNormalHandCardsSpec,
	"Wacom.Battle.CardToCardTarget.ExplicitFilterRejectsNormalHandCards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleExplicitFilterRejectsNormalHandCardsSpec::RunTest(const FString& Parameters)
{
	FWacomBattleFixture Fx;
	UCardDefinition* SourceDef = nullptr;
	UCardDefinition* TargetDef = nullptr;
	UBattleSession* Session = CreateExplicitFilterSession(
		Fx,
		SourceDef,
		TargetDef,
		/*bAllowNormalHandCards*/ false,
		/*bAllowHandAnchors*/ true);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, SourceDef->CardId);
	const FGuid TargetId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, TargetDef->CardId);
	const FWacomInteractionTargetHandle Target = FWacomInteractionTargetHandle::ForCardTarget(TargetId, Session);

	const FWacomBattleTargetValidationResult Result = Session->ValidateTargetWithCard(SourceId, Target);
	TestFalse(TEXT("Explicit filter rejects normal hand card"), Result.bCanTarget);
	TestEqual(TEXT("Explicit filter explains normal hand card reject"),
		Result.RejectReason,
		EWacomBattleTargetRejectReason::UnsupportedNormalHandCardTarget);
	TestFalse(TEXT("Submit on rejected normal target fails"),
		Session->SubmitCommand(FBattleCommand::MakePlayCardOnHandCard(SourceId, TargetId)).IsOk());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleExplicitFilterRejectsHandAnchorsSpec,
	"Wacom.Battle.CardToCardTarget.ExplicitFilterRejectsHandAnchors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleExplicitFilterRejectsHandAnchorsSpec::RunTest(const FString& Parameters)
{
	FWacomBattleFixture Fx;
	UCardDefinition* SourceDef = nullptr;
	UCardDefinition* TargetDef = nullptr;
	UBattleSession* Session = CreateExplicitFilterSession(
		Fx,
		SourceDef,
		TargetDef,
		/*bAllowNormalHandCards*/ true,
		/*bAllowHandAnchors*/ false);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, SourceDef->CardId);
	const FGuid AnchorId = FindFirstHandAnchor(Snapshot);
	const FWacomInteractionTargetHandle Target = FWacomInteractionTargetHandle::ForCardTarget(AnchorId, Session);

	const FWacomBattleTargetValidationResult Result = Session->ValidateTargetWithCard(SourceId, Target);
	TestFalse(TEXT("Explicit filter rejects hand anchor"), Result.bCanTarget);
	TestEqual(TEXT("Explicit filter explains hand anchor reject"),
		Result.RejectReason,
		EWacomBattleTargetRejectReason::UnsupportedHandAnchorTarget);
	TestFalse(TEXT("Submit on rejected anchor target fails"),
		Session->SubmitCommand(FBattleCommand::MakePlayCardOnHandCard(SourceId, AnchorId)).IsOk());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleImplicitFilterPreservesCostModifierAnchorBehaviorSpec,
	"Wacom.Battle.CardToCardTarget.ImplicitFilterPreservesCostModifierAnchorBehavior",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleImplicitFilterPreservesCostModifierAnchorBehaviorSpec::RunTest(const FString& Parameters)
{
	FWacomBattleFixture Fx;
	UCardDefinition* SourceDef = Fx.MakeHandCardCostModifierCard(/*Cost*/0, /*Magnitude*/2, /*bReduceCost*/false);
	SourceDef->HandCardTargetFilter.bUseExplicitHandCardTargetFilter = false;
	SourceDef->HandCardTargetFilter.bAllowNormalHandCards = false;
	SourceDef->HandCardTargetFilter.bAllowHandAnchors = false;
	UCardDefinition* LeftDef = Fx.MakeNoopCard(0);
	UBattleSession* Session = Fx.CreateSession(
		Fx.MakeCharacter(LeftDef, Fx.MakeNoopCard(0), { SourceDef, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) }),
		Fx.MakeSinglePartEnemy(/*Hp*/100, /*Initiative*/50, /*IntentResist*/0),
		1);
	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, SourceDef->CardId);
	const FGuid LeftId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, LeftDef->CardId);
	const FWacomInteractionTargetHandle Target = FWacomInteractionTargetHandle::ForCardTarget(LeftId, Session);

	const FWacomBattleTargetValidationResult Result = Session->ValidateTargetWithCard(SourceId, Target);
	TestTrue(TEXT("Implicit cost-modifier filter keeps anchor target valid"), Result.bCanTarget);
	TestTrue(TEXT("Submit on implicit cost-modifier anchor target succeeds"),
		Session->SubmitCommand(FBattleCommand::MakePlayCardOnHandCard(SourceId, LeftId)).IsOk());
	Snapshot = Session->BuildSnapshot();
	TestEqual(TEXT("Implicit cost-modifier still changes anchor cost"), GetRuntimeCostInHand(Snapshot, LeftId), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleImplicitFilterPreservesSelectedDiscardExhaustAnchorRejectSpec,
	"Wacom.Battle.CardToCardTarget.ImplicitFilterPreservesSelectedDiscardExhaustAnchorReject",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleImplicitFilterPreservesSelectedDiscardExhaustAnchorRejectSpec::RunTest(const FString& Parameters)
{
	FWacomBattleFixture Fx;
	UCardDefinition* SourceDef = Fx.MakeSelectedHandCardZoneMoveCard(/*Cost*/0, /*bExhaust*/false);
	SourceDef->HandCardTargetFilter.bUseExplicitHandCardTargetFilter = false;
	SourceDef->HandCardTargetFilter.bAllowNormalHandCards = true;
	SourceDef->HandCardTargetFilter.bAllowHandAnchors = true;
	UCardDefinition* LeftDef = Fx.MakeNoopCard(0);
	UBattleSession* Session = Fx.CreateSession(
		Fx.MakeCharacter(LeftDef, Fx.MakeNoopCard(0), { SourceDef, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) }),
		Fx.MakeSinglePartEnemy(/*Hp*/100, /*Initiative*/50, /*IntentResist*/0),
		1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, SourceDef->CardId);
	const FGuid LeftId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, LeftDef->CardId);
	const FWacomInteractionTargetHandle Target = FWacomInteractionTargetHandle::ForCardTarget(LeftId, Session);

	const FWacomBattleTargetValidationResult Result = Session->ValidateTargetWithCard(SourceId, Target);
	TestFalse(TEXT("Implicit selected zone move filter rejects anchor target"), Result.bCanTarget);
	TestEqual(TEXT("Implicit selected zone move explains anchor reject"),
		Result.RejectReason,
		EWacomBattleTargetRejectReason::UnsupportedHandAnchorTarget);
	TestFalse(TEXT("Submit on implicit selected zone move anchor target fails"),
		Session->SubmitCommand(FBattleCommand::MakePlayCardOnHandCard(SourceId, LeftId)).IsOk());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattlePlayCardResolverMatchesValidationForHandCardFilterSpec,
	"Wacom.Battle.CardToCardTarget.PlayCardResolverMatchesValidationForHandCardFilter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattlePlayCardResolverMatchesValidationForHandCardFilterSpec::RunTest(const FString& Parameters)
{
	FWacomBattleFixture Fx;
	UCardDefinition* SourceDef = nullptr;
	UCardDefinition* TargetDef = nullptr;
	UBattleSession* Session = CreateExplicitFilterSession(
		Fx,
		SourceDef,
		TargetDef,
		/*bAllowNormalHandCards*/ false,
		/*bAllowHandAnchors*/ true);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, SourceDef->CardId);
	const FGuid TargetId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, TargetDef->CardId);
	const FWacomInteractionTargetHandle Target = FWacomInteractionTargetHandle::ForCardTarget(TargetId, Session);

	const FWacomBattleTargetValidationResult Result = Session->ValidateTargetWithCard(SourceId, Target);
	const FWacomStatus Status = Session->SubmitCommand(FBattleCommand::MakePlayCardOnHandCard(SourceId, TargetId));
	TestFalse(TEXT("Validation rejects normal hand-card target"), Result.bCanTarget);
	TestEqual(TEXT("Validation reject reason is filter-specific"),
		Result.RejectReason,
		EWacomBattleTargetRejectReason::UnsupportedNormalHandCardTarget);
	TestFalse(TEXT("PlayCard submit rejects same filter case"), Status.IsOk());
	TestEqual(TEXT("PlayCard resolver reports matching filter detail"),
		Status.Detail,
		FName(TEXT("TargetNormalHandCardUnsupported")));
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
	FWacomBattleSelectedHandCardDiscardSpec,
	"Wacom.Battle.CardToCardTarget.SelectedHandCardDiscardMovesExactTargetToDiscard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleSelectedHandCardDiscardSpec::RunTest(const FString& Parameters)
{
	FWacomBattleFixture Fx;
	UCardDefinition* SourceDef = nullptr;
	UCardDefinition* TargetDef = nullptr;
	UBattleSession* Session = CreateSelectedHandCardZoneMoveSession(Fx, SourceDef, TargetDef, /*bExhaust*/false);
	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, SourceDef->CardId);
	const FGuid TargetId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, TargetDef->CardId);
	const int32 DiscardBefore = Snapshot.PileCounts.DiscardCount;
	const int32 ExhaustBefore = Snapshot.PileCounts.ExhaustCount;

	TestTrue(TEXT("Can target normal hand card"),
		Session->CanTargetWithCard(SourceId, FWacomInteractionTargetHandle::ForCardTarget(TargetId, Session)));
	TestTrue(TEXT("Play discard selected hand card"),
		Session->SubmitCommand(FBattleCommand::MakePlayCardOnHandCard(SourceId, TargetId)).IsOk());
	Snapshot = Session->BuildSnapshot();

	TestFalse(TEXT("Target left hand"), FindHandCard(Snapshot, TargetId) != nullptr);
	TestFalse(TEXT("Source left hand"), FindHandCard(Snapshot, SourceId) != nullptr);
	TestEqual(TEXT("Discard pile gained source and target"), Snapshot.PileCounts.DiscardCount, DiscardBefore + 2);
	TestEqual(TEXT("Exhaust pile unchanged"), Snapshot.PileCounts.ExhaustCount, ExhaustBefore);
	TestTrue(TEXT("HandZoneChanged emitted for selected target"),
		HasBattleEvent(Session, EBattleEventType::HandZoneChanged, TargetId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleSelectedHandCardExhaustSpec,
	"Wacom.Battle.CardToCardTarget.SelectedHandCardExhaustMovesExactTargetToExhaust",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleSelectedHandCardExhaustSpec::RunTest(const FString& Parameters)
{
	FWacomBattleFixture Fx;
	UCardDefinition* SourceDef = nullptr;
	UCardDefinition* TargetDef = nullptr;
	UBattleSession* Session = CreateSelectedHandCardZoneMoveSession(Fx, SourceDef, TargetDef, /*bExhaust*/true);
	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, SourceDef->CardId);
	const FGuid TargetId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, TargetDef->CardId);
	const int32 DiscardBefore = Snapshot.PileCounts.DiscardCount;
	const int32 ExhaustBefore = Snapshot.PileCounts.ExhaustCount;

	TestTrue(TEXT("Can target normal hand card"),
		Session->CanTargetWithCard(SourceId, FWacomInteractionTargetHandle::ForCardTarget(TargetId, Session)));
	TestTrue(TEXT("Play exhaust selected hand card"),
		Session->SubmitCommand(FBattleCommand::MakePlayCardOnHandCard(SourceId, TargetId)).IsOk());
	Snapshot = Session->BuildSnapshot();

	TestFalse(TEXT("Target left hand"), FindHandCard(Snapshot, TargetId) != nullptr);
	TestFalse(TEXT("Source left hand"), FindHandCard(Snapshot, SourceId) != nullptr);
	TestEqual(TEXT("Discard pile gained source only"), Snapshot.PileCounts.DiscardCount, DiscardBefore + 1);
	TestEqual(TEXT("Exhaust pile gained target"), Snapshot.PileCounts.ExhaustCount, ExhaustBefore + 1);
	TestTrue(TEXT("HandZoneChanged emitted for selected target"),
		HasBattleEvent(Session, EBattleEventType::HandZoneChanged, TargetId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleSelectedHandCardZoneMoveRejectsAnchorsSpec,
	"Wacom.Battle.CardToCardTarget.SelectedHandCardZoneMoveRejectsHandAnchors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleSelectedHandCardZoneMoveRejectsAnchorsSpec::RunTest(const FString& Parameters)
{
	FWacomBattleFixture Fx;
	UCardDefinition* SourceDef = Fx.MakeSelectedHandCardZoneMoveCard(/*Cost*/0, /*bExhaust*/false);
	UCardDefinition* TargetDef = Fx.MakeNoopCard(/*Cost*/3);
	UCardDefinition* LeftDef = Fx.MakeNoopCard(0);
	UCardDefinition* RightDef = Fx.MakeNoopCard(0);
	UBattleSession* Session = Fx.CreateSession(
		Fx.MakeCharacter(LeftDef, RightDef, { SourceDef, TargetDef, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) }),
		Fx.MakeSinglePartEnemy(/*Hp*/100, /*Initiative*/50, /*IntentResist*/0),
		1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, SourceDef->CardId);
	const FGuid LeftId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, LeftDef->CardId);
	const FGuid RightId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, RightDef->CardId);

	TestFalse(TEXT("Selected discard rejects left anchor"),
		Session->CanTargetWithCard(SourceId, FWacomInteractionTargetHandle::ForCardTarget(LeftId, Session)));
	TestFalse(TEXT("Selected discard rejects right anchor"),
		Session->CanTargetWithCard(SourceId, FWacomInteractionTargetHandle::ForCardTarget(RightId, Session)));
	TestFalse(TEXT("Submit on left anchor fails"),
		Session->SubmitCommand(FBattleCommand::MakePlayCardOnHandCard(SourceId, LeftId)).IsOk());
	TestFalse(TEXT("Submit on right anchor fails"),
		Session->SubmitCommand(FBattleCommand::MakePlayCardOnHandCard(SourceId, RightId)).IsOk());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleSelectedHandCardZoneMoveRejectsInvalidTargetsSpec,
	"Wacom.Battle.CardToCardTarget.SelectedHandCardZoneMoveRejectsSelfMissingAndNotInHand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleSelectedHandCardZoneMoveRejectsInvalidTargetsSpec::RunTest(const FString& Parameters)
{
	FWacomBattleFixture Fx;
	UCardDefinition* SourceDef = Fx.MakeSelectedHandCardZoneMoveCard(/*Cost*/0, /*bExhaust*/false);
	UCardDefinition* TargetDef = Fx.MakeNoopCard(/*Cost*/0);
	UBattleSession* Session = Fx.CreateSession(
		Fx.MakeCharacter(
			Fx.MakeNoopCard(0),
			Fx.MakeNoopCard(0),
			{ SourceDef, TargetDef, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) }),
		Fx.MakeSinglePartEnemy(/*Hp*/100, /*Initiative*/50, /*IntentResist*/0),
		1);
	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, SourceDef->CardId);
	const FGuid TargetId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, TargetDef->CardId);
	const FGuid MissingId = FGuid::NewGuid();

	TestFalse(TEXT("Selected zone move rejects self target"),
		Session->CanTargetWithCard(SourceId, FWacomInteractionTargetHandle::ForCardTarget(SourceId, Session)));
	TestFalse(TEXT("Selected zone move rejects missing target"),
		Session->CanTargetWithCard(SourceId, FWacomInteractionTargetHandle::ForCardTarget(MissingId, Session)));
	TestFalse(TEXT("Submit missing target fails"),
		Session->SubmitCommand(FBattleCommand::MakePlayCardOnHandCard(SourceId, MissingId)).IsOk());

	TestTrue(TEXT("Move target card out of hand"),
		Session->SubmitCommand(FBattleCommand::MakePlayCard(TargetId)).IsOk());
	Snapshot = Session->BuildSnapshot();
	TestFalse(TEXT("Moved target no longer in hand"), FindHandCard(Snapshot, TargetId) != nullptr);
	TestFalse(TEXT("Selected zone move rejects not-in-hand target"),
		Session->CanTargetWithCard(SourceId, FWacomInteractionTargetHandle::ForCardTarget(TargetId, Session)));
	TestFalse(TEXT("Submit not-in-hand target fails"),
		Session->SubmitCommand(FBattleCommand::MakePlayCardOnHandCard(SourceId, TargetId)).IsOk());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCostModifierStillAllowsHandAnchorTargetsSpec,
	"Wacom.Battle.CardToCardTarget.CostModifierStillAllowsHandAnchorTargets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCostModifierStillAllowsHandAnchorTargetsSpec::RunTest(const FString& Parameters)
{
	FWacomBattleFixture Fx;
	UCardDefinition* SourceDef = Fx.MakeHandCardCostModifierCard(/*Cost*/0, /*Magnitude*/2, /*bReduceCost*/false);
	UCardDefinition* LeftDef = Fx.MakeNoopCard(0);
	UCardDefinition* RightDef = Fx.MakeNoopCard(0);
	UBattleSession* Session = Fx.CreateSession(
		Fx.MakeCharacter(LeftDef, RightDef, { SourceDef, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) }),
		Fx.MakeSinglePartEnemy(/*Hp*/100, /*Initiative*/50, /*IntentResist*/0),
		1);
	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, SourceDef->CardId);
	const FGuid LeftId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, LeftDef->CardId);

	TestTrue(TEXT("Cost modifier still accepts anchor target"),
		Session->CanTargetWithCard(SourceId, FWacomInteractionTargetHandle::ForCardTarget(LeftId, Session)));
	TestTrue(TEXT("Cost modifier can submit on anchor target"),
		Session->SubmitCommand(FBattleCommand::MakePlayCardOnHandCard(SourceId, LeftId)).IsOk());
	Snapshot = Session->BuildSnapshot();
	TestEqual(TEXT("Anchor cost increased"), GetRuntimeCostInHand(Snapshot, LeftId), 2);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleValidateTargetExplainsValidHandCardTargetSpec,
	"Wacom.Battle.CardToCardTarget.ValidateTargetWithCardExplainsValidHandCardTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleValidateTargetExplainsValidHandCardTargetSpec::RunTest(const FString& Parameters)
{
	FWacomBattleFixture Fx;
	UCardDefinition* SourceDef = nullptr;
	UCardDefinition* TargetDef = nullptr;
	UBattleSession* Session = CreateCardToCardSession(Fx, SourceDef, TargetDef);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, SourceDef->CardId);
	const FGuid TargetId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, TargetDef->CardId);
	const FWacomInteractionTargetHandle Target = FWacomInteractionTargetHandle::ForCardTarget(TargetId, Session);

	const FWacomBattleTargetValidationResult Result = Session->ValidateTargetWithCard(SourceId, Target);
	TestTrue(TEXT("Validation accepts valid hand card target"), Result.bCanTarget);
	TestEqual(TEXT("Validation has no reject reason"), Result.RejectReason, EWacomBattleTargetRejectReason::None);
	TestTrue(TEXT("CanTarget mirrors validation"), Session->CanTargetWithCard(SourceId, Target) == Result.bCanTarget);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleValidateTargetExplainsSelfTargetSpec,
	"Wacom.Battle.CardToCardTarget.ValidateTargetWithCardExplainsSelfTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleValidateTargetExplainsSelfTargetSpec::RunTest(const FString& Parameters)
{
	FWacomBattleFixture Fx;
	UCardDefinition* SourceDef = nullptr;
	UCardDefinition* TargetDef = nullptr;
	UBattleSession* Session = CreateCardToCardSession(Fx, SourceDef, TargetDef);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, SourceDef->CardId);
	const FWacomInteractionTargetHandle Target = FWacomInteractionTargetHandle::ForCardTarget(SourceId, Session);

	const FWacomBattleTargetValidationResult Result = Session->ValidateTargetWithCard(SourceId, Target);
	TestFalse(TEXT("Validation rejects self target"), Result.bCanTarget);
	TestEqual(TEXT("Validation explains self target"), Result.RejectReason, EWacomBattleTargetRejectReason::SelfTarget);
	TestTrue(TEXT("CanTarget mirrors validation"), Session->CanTargetWithCard(SourceId, Target) == Result.bCanTarget);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleValidateTargetExplainsUnsupportedAnchorForSelectedZoneMoveSpec,
	"Wacom.Battle.CardToCardTarget.ValidateTargetWithCardExplainsUnsupportedAnchorForSelectedZoneMove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleValidateTargetExplainsUnsupportedAnchorForSelectedZoneMoveSpec::RunTest(const FString& Parameters)
{
	FWacomBattleFixture Fx;
	UCardDefinition* SourceDef = Fx.MakeSelectedHandCardZoneMoveCard(/*Cost*/0, /*bExhaust*/false);
	UCardDefinition* LeftDef = Fx.MakeNoopCard(0);
	UBattleSession* Session = Fx.CreateSession(
		Fx.MakeCharacter(LeftDef, Fx.MakeNoopCard(0), { SourceDef, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) }),
		Fx.MakeSinglePartEnemy(/*Hp*/100, /*Initiative*/50, /*IntentResist*/0),
		1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, SourceDef->CardId);
	const FGuid LeftId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, LeftDef->CardId);
	const FWacomInteractionTargetHandle Target = FWacomInteractionTargetHandle::ForCardTarget(LeftId, Session);

	const FWacomBattleTargetValidationResult Result = Session->ValidateTargetWithCard(SourceId, Target);
	TestFalse(TEXT("Validation rejects selected zone move anchor target"), Result.bCanTarget);
	TestEqual(TEXT("Validation explains unsupported hand anchor"), Result.RejectReason, EWacomBattleTargetRejectReason::UnsupportedHandAnchorTarget);
	TestTrue(TEXT("CanTarget mirrors validation"), Session->CanTargetWithCard(SourceId, Target) == Result.bCanTarget);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleValidateTargetExplainsNonHandCardSourceSpec,
	"Wacom.Battle.CardToCardTarget.ValidateTargetWithCardExplainsNonHandCardSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleValidateTargetExplainsNonHandCardSourceSpec::RunTest(const FString& Parameters)
{
	FWacomBattleFixture Fx;
	UCardDefinition* SourceDef = Fx.MakeNoopCard(0);
	UCardDefinition* TargetDef = Fx.MakeNoopCard(0);
	UBattleSession* Session = Fx.CreateSession(
		Fx.MakeCharacter(Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), { SourceDef, TargetDef, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) }),
		Fx.MakeSinglePartEnemy(/*Hp*/100, /*Initiative*/50, /*IntentResist*/0),
		1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, SourceDef->CardId);
	const FGuid TargetId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, TargetDef->CardId);
	const FWacomInteractionTargetHandle Target = FWacomInteractionTargetHandle::ForCardTarget(TargetId, Session);

	const FWacomBattleTargetValidationResult Result = Session->ValidateTargetWithCard(SourceId, Target);
	TestFalse(TEXT("Validation rejects non-HandCard source"), Result.bCanTarget);
	TestEqual(TEXT("Validation explains unsupported card target"), Result.RejectReason, EWacomBattleTargetRejectReason::UnsupportedCardTarget);
	TestTrue(TEXT("CanTarget mirrors validation"), Session->CanTargetWithCard(SourceId, Target) == Result.bCanTarget);
	return true;
}
