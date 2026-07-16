// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Fixtures/WacomRunExplorationFixture.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Events/RunEventDefinition.h"
#include "Interactions/RunWorldCardInteractionDefinition.h"
#include "Pickups/RunPickupDefinition.h"
#include "RunSession.h"
#include "RunState.h"
#include "Tags/WacomGameplayTags.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	const FName SerpentCredential(TEXT("Credential.Run.SerpentSigil"));

	UCardDefinition* MakeCredentialCard(UObject* Outer, const FName CardId)
	{
		UCardDefinition* Card = NewObject<UCardDefinition>(Outer);
		Card->CardId = CardId;
		Card->DisplayName = FText::FromName(CardId);
		Card->Rarity = WacomTags::Card_Rarity_White;
		return Card;
	}

	UWacomRunPickupDefinition* MakeCredentialCardPickup(
		UObject* Outer,
		UCardDefinition* Card,
		const FName PickupId,
		TArray<FName> CredentialIds = { SerpentCredential })
	{
		UWacomRunPickupDefinition* Definition = NewObject<UWacomRunPickupDefinition>(Outer);
		Definition->PickupId = PickupId;
		Definition->RewardType = EWacomRunPickupRewardType::Card;
		Definition->CardDefinition = Card;
		Definition->GrantedCredentialIds = MoveTemp(CredentialIds);
		return Definition;
	}

	UWacomRunPickupDefinition* MakeCredentialGoldPickup(
		UObject* Outer,
		const FName PickupId,
		const TArray<FName>& CredentialIds)
	{
		UWacomRunPickupDefinition* Definition = NewObject<UWacomRunPickupDefinition>(Outer);
		Definition->PickupId = PickupId;
		Definition->RewardType = EWacomRunPickupRewardType::Gold;
		Definition->GoldAmount = 2;
		Definition->GrantedCredentialIds = CredentialIds;
		return Definition;
	}

	FGuid FindOwnedInstance(const FRunState& State, const UCardDefinition* Card)
	{
		auto FindIn = [Card](const TArray<FCardInstance>& Cards)
		{
			for (const FCardInstance& Instance : Cards)
			{
				if (Instance.Definition == Card)
				{
					return Instance.InstanceId;
				}
			}
			return FGuid();
		};

		if (const FGuid Id = FindIn(State.Backpack); Id.IsValid())
		{
			return Id;
		}
		if (const FGuid Id = FindIn(State.BattleDeck); Id.IsValid())
		{
			return Id;
		}
		if (const FGuid Id = FindIn(State.BurdenZone); Id.IsValid())
		{
			return Id;
		}
		for (const FSpecialZone& Zone : State.SpecialZones)
		{
			if (const FGuid Id = FindIn(Zone.Cards); Id.IsValid())
			{
				return Id;
			}
		}
		return FGuid();
	}

	int32 CountOwnedCard(const FRunState& State, const UCardDefinition* Card)
	{
		int32 Count = 0;
		auto CountIn = [&Count, Card](const TArray<FCardInstance>& Cards)
		{
			for (const FCardInstance& Instance : Cards)
			{
				if (Instance.Definition == Card)
				{
					++Count;
				}
			}
		};
		CountIn(State.Backpack);
		CountIn(State.BattleDeck);
		CountIn(State.BurdenZone);
		for (const FSpecialZone& Zone : State.SpecialZones)
		{
			CountIn(Zone.Cards);
		}
		return Count;
	}

	UWacomRunEventDefinition* MakeCredentialPaymentEvent(UObject* Outer, UCardDefinition* Card)
	{
		UWacomRunEventDefinition* Event = NewObject<UWacomRunEventDefinition>(Outer);
		Event->EventId = TEXT("Event.Credential.Payment");
		Event->DisplayName = FText::FromString(TEXT("Credential payment test"));
		Event->StartNodeId = TEXT("Start");

		FWacomRunEventChoiceDefinition Choice;
		Choice.ChoiceId = TEXT("Pay");
		Choice.CardPayment.bRequiresOwnedCardPayment = true;
		Choice.CardPayment.AllowedCardDefinitions = { Card };

		FWacomRunEventNodeDefinition Node;
		Node.NodeId = TEXT("Start");
		Node.Choices = { Choice };
		Event->Nodes = { Node };
		return Event;
	}

	UCharacterDefinition* MakeCredentialCharacter(
		FWacomRunExplorationFixture& Fixture,
		UCardDefinition* Card)
	{
		UCharacterDefinition* Character = Fixture.MakeCharacter(TEXT("Credential.Test.Character"));
		UCardDefinition* Bag = MakeCredentialCard(Character, TEXT("Credential.Test.Bag"));
		Bag->Physique.Capacity = 4;
		Character->StarterDeck = { Card, Bag };
		return Character;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCredentialAtomicPickupSpec,
	"Wacom.Run.Credential.AtomicCardPickupGrantsRewardCredentialsAndNotifiesOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunCredentialAtomicPickupSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	UCardDefinition* Card = MakeCredentialCard(Run.Get(), TEXT("Card.Run.SerpentSigil"));
	UWacomRunPickupDefinition* Definition = MakeCredentialCardPickup(
		Run.Get(), Card, TEXT("Pickup.Run.SerpentSigil"),
		{ SerpentCredential, TEXT("Credential.Run.Secondary") });
	int32 BroadcastCount = 0;
	Run->OnRunStateChangedNative.AddLambda([&BroadcastCount]() { ++BroadcastCount; });

	const FRunTreasureSettlementResult Result =
		Run->CollectPickupFromDefinition(TEXT("Floor.Main.01.Node.Key.01"), Definition);

	TestTrue(TEXT("Definition pickup succeeds"), Result.bSucceeded);
	TestTrue(TEXT("Serpent credential granted"), Run->HasCredential(SerpentCredential));
	TestTrue(TEXT("Secondary credential granted"),
		Run->HasCredential(TEXT("Credential.Run.Secondary")));
	TestEqual(TEXT("Credential set has two ids"), Run->GetRunState().GrantedCredentialIds.Num(), 2);
	TestEqual(TEXT("Presentation card acquired"), CountOwnedCard(Run->GetRunState(), Card), 1);
	TestTrue(TEXT("Pickup marked collected"),
		Run->IsPickupCollected(TEXT("Floor.Main.01.Node.Key.01")));
	TestEqual(TEXT("Atomic pickup broadcasts once"), BroadcastCount, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCredentialIdempotentGrantSpec,
	"Wacom.Run.Credential.RepeatedGrantIsIdempotentWithoutBlockingOtherReward",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunCredentialIdempotentGrantSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	UCardDefinition* Card = MakeCredentialCard(Run.Get(), TEXT("Card.Credential.First"));
	UWacomRunPickupDefinition* CardPickup = MakeCredentialCardPickup(
		Run.Get(), Card, TEXT("Pickup.Credential.First"));
	UWacomRunPickupDefinition* GoldPickup = MakeCredentialGoldPickup(
		Run.Get(), TEXT("Pickup.Credential.Second"),
		{ SerpentCredential, TEXT("Credential.Run.Secondary") });

	TestTrue(TEXT("First grant succeeds"),
		Run->CollectPickupFromDefinition(TEXT("Credential.Source.First"), CardPickup).bSucceeded);
	TestTrue(TEXT("Second source with repeated grant succeeds"),
		Run->CollectPickupFromDefinition(TEXT("Credential.Source.Second"), GoldPickup).bSucceeded);
	TestEqual(TEXT("Repeated id remains unique"), Run->GetRunState().GrantedCredentialIds.Num(), 2);
	TestEqual(TEXT("Other main reward still applied"), Run->GetGold(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCredentialInvalidDefinitionAtomicSpec,
	"Wacom.Run.Credential.InvalidCredentialIdsRejectWholePickup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunCredentialInvalidDefinitionAtomicSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	UWacomRunPickupDefinition* Definition = MakeCredentialGoldPickup(
		Run.Get(), TEXT("Pickup.Credential.Invalid"),
		{ SerpentCredential, SerpentCredential });
	int32 BroadcastCount = 0;
	Run->OnRunStateChangedNative.AddLambda([&BroadcastCount]() { ++BroadcastCount; });

	const FRunTreasureSettlementResult Result =
		Run->CollectPickupFromDefinition(TEXT("Credential.Source.Invalid"), Definition);

	TestFalse(TEXT("Duplicate credential ids reject pickup"), Result.bSucceeded);
	TestEqual(TEXT("Gold unchanged"), Run->GetGold(), 0);
	TestFalse(TEXT("Credential not granted"), Run->HasCredential(SerpentCredential));
	TestFalse(TEXT("Pickup not collected"),
		Run->IsPickupCollected(TEXT("Credential.Source.Invalid")));
	TestEqual(TEXT("Rejected pickup does not broadcast"), BroadcastCount, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCredentialExplorationRollbackSpec,
	"Wacom.Run.Credential.ExplorationFailureRejectsRewardCredentialAndPickup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunCredentialExplorationRollbackSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomRunExplorationFixture Fixture;
	UCharacterDefinition* Character = Fixture.MakeCharacter(TEXT("Credential.Rollback.Character"));
	FWacomInitializedRunExplorationSession Initialized =
		Fixture.CreateInitializedSession(Character, Fixture.MakeJourney({ Fixture.MakeLinearFloor() }));
	if (!TestTrue(TEXT("Run initializes"), Initialized.Initialization.IsOk())
		|| !TestNotNull(TEXT("Session exists"), Initialized.Session))
	{
		return false;
	}
	URunSession* Run = Initialized.Session;
	UCardDefinition* Card = MakeCredentialCard(Run, TEXT("Card.Credential.Rollback"));
	UWacomRunPickupDefinition* Definition = MakeCredentialCardPickup(
		Run, Card, TEXT("Pickup.Credential.Rollback"));
	int32 BroadcastCount = 0;
	Run->OnRunStateChangedNative.AddLambda([&BroadcastCount]() { ++BroadcastCount; });

	const FRunTreasureSettlementResult Result =
		Run->CollectPickupFromDefinition(TEXT("Credential.Source.Rollback"), Definition);

	TestFalse(TEXT("Navigation node rejects Treasure settlement"), Result.bSucceeded);
	TestEqual(TEXT("Card reward rolled back"), CountOwnedCard(Run->GetRunState(), Card), 0);
	TestFalse(TEXT("Credential rolled back"), Run->HasCredential(SerpentCredential));
	TestFalse(TEXT("Pickup marker rolled back"),
		Run->IsPickupCollected(TEXT("Credential.Source.Rollback")));
	TestEqual(TEXT("Rejected transaction does not broadcast"), BroadcastCount, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCredentialSurvivesDestroySpec,
	"Wacom.Run.Credential.SurvivesDirectCardDestroy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunCredentialSurvivesDestroySpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	UCardDefinition* Card = MakeCredentialCard(Run.Get(), TEXT("Card.Credential.Destroy"));
	UWacomRunPickupDefinition* Pickup = MakeCredentialCardPickup(
		Run.Get(), Card, TEXT("Pickup.Credential.Destroy"));
	TestTrue(TEXT("Credential pickup succeeds"),
		Run->CollectPickupFromDefinition(TEXT("Credential.Source.Destroy"), Pickup).bSucceeded);
	const FGuid InstanceId = FindOwnedInstance(Run->GetRunState(), Card);
	TestTrue(TEXT("Card instance exists"), InstanceId.IsValid());
	TestTrue(TEXT("Direct destroy succeeds"), Run->DestroyCardByInstance(InstanceId));
	TestTrue(TEXT("Credential survives direct destroy"), Run->HasCredential(SerpentCredential));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCredentialSurvivesDeleteForGoldSpec,
	"Wacom.Run.Credential.SurvivesDeleteCardForGold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunCredentialSurvivesDeleteForGoldSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	UCardDefinition* Card = MakeCredentialCard(Run.Get(), TEXT("Card.Credential.Delete"));
	UWacomRunPickupDefinition* Pickup = MakeCredentialCardPickup(
		Run.Get(), Card, TEXT("Pickup.Credential.Delete"));
	TestTrue(TEXT("Credential pickup succeeds"),
		Run->CollectPickupFromDefinition(TEXT("Credential.Source.Delete"), Pickup).bSucceeded);
	const FGuid InstanceId = FindOwnedInstance(Run->GetRunState(), Card);
	TestTrue(TEXT("Card instance exists"), InstanceId.IsValid());
	TestTrue(TEXT("Delete for gold succeeds"), Run->DeleteCardForGoldByInstance(InstanceId));
	TestTrue(TEXT("Credential survives delete for gold"), Run->HasCredential(SerpentCredential));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCredentialSurvivesEventPaymentSpec,
	"Wacom.Run.Credential.SurvivesRunEventCardPayment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunCredentialSurvivesEventPaymentSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomRunExplorationFixture Fixture;
	UCardDefinition* Card = MakeCredentialCard(GetTransientPackage(), TEXT("Card.Credential.Payment"));
	UCharacterDefinition* Character = MakeCredentialCharacter(Fixture, Card);
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"),
		InitializeRunSessionForTest(*Run, Character, EWacomMapNodeType::RunEvent).IsOk());
	FWacomRunSessionTestAccess::GetMutableRunState(*Run).GrantedCredentialIds.Add(SerpentCredential);
	const FGuid InstanceId = FindOwnedInstance(Run->GetRunState(), Card);
	UWacomRunEventDefinition* Event = MakeCredentialPaymentEvent(Run.Get(), Card);
	TestTrue(TEXT("Payment event begins"),
		Run->BeginRunEvent(TEXT("Event.Credential.Payment.Host"), Event));
	TestTrue(TEXT("Card payment succeeds"),
		Run->ChooseRunEventOptionWithPaidCardResult(TEXT("Pay"), InstanceId).bSucceeded);
	TestTrue(TEXT("Credential survives RunEvent payment"), Run->HasCredential(SerpentCredential));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCredentialSurvivesWorldConsumeSpec,
	"Wacom.Run.Credential.SurvivesWorldInteractionCardConsume",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunCredentialSurvivesWorldConsumeSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomRunExplorationFixture Fixture;
	UCardDefinition* Card = MakeCredentialCard(GetTransientPackage(), TEXT("Card.Credential.WorldConsume"));
	UCharacterDefinition* Character = MakeCredentialCharacter(Fixture, Card);
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"),
		InitializeRunSessionForTest(*Run, Character, EWacomMapNodeType::Treasure).IsOk());
	FWacomRunSessionTestAccess::GetMutableRunState(*Run).GrantedCredentialIds.Add(SerpentCredential);

	FRunWorldCardInteractionRequest Request;
	Request.PersistentId = TEXT("WorldInteraction.Credential.Consume");
	Request.SourceCardInstanceId = FindOwnedInstance(Run->GetRunState(), Card);
	Request.AllowedCardDefinitions = { Card };
	Request.bConsumeCardOnSuccess = true;
	FWacomRunWorldCardInteractionReward Reward;
	Reward.Type = EWacomRunWorldCardInteractionRewardType::Gold;
	Reward.GoldAmount = 1;
	Request.Rewards = { Reward };

	TestTrue(TEXT("World interaction consumes card"),
		Run->SubmitRunWorldCardInteraction(Request).bSucceeded);
	TestTrue(TEXT("Credential survives world card consume"), Run->HasCredential(SerpentCredential));
	return true;
}
