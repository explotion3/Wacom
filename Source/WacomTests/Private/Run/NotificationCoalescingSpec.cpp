// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Fixtures/WacomRunExplorationFixture.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Events/RunEventDefinition.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Interactions/RunWorldCardInteractionDefinition.h"
#include "Map/WacomFloorMapDefinition.h"
#include "RunSession.h"
#include "RunState.h"
#include "RunStateTypes.h"
#include "Runtime/BattleEnemyKeys.h"
#include "Session/BattleResultPacket.h"
#include "Tags/WacomGameplayTags.h"

#include "UObject/StrongObjectPtr.h"

namespace
{
	struct FNotificationRevisionSnapshot
	{
		uint64 BackpackStorage = 0;
		uint64 Shop = 0;
		uint64 Economy = 0;
	};

	FNotificationRevisionSnapshot CaptureNotificationRevisions(const URunSession& Run)
	{
		return FNotificationRevisionSnapshot{
			Run.GetBackpackStorageSnapshotRevision(),
			Run.GetShopSnapshotRevision(),
			Run.GetEconomySnapshotRevision(),
		};
	}

	UCardDefinition* MakeNotificationCard(FWacomBattleFixture& Fx, FName CardId, int32 Capacity = 0)
	{
		UCardDefinition* Card = Fx.MakeNoopCard(0);
		Card->CardId = CardId;
		Card->DisplayName = FText::FromName(CardId);
		Card->Rarity = WacomTags::Card_Rarity_White;
		Card->Physique.Capacity = Capacity;
		return Card;
	}

	UCharacterDefinition* MakeNotificationCharacter(UObject* Outer, const TArray<UCardDefinition*>& StarterDeck)
	{
		UCharacterDefinition* Character = NewObject<UCharacterDefinition>(Outer);
		Character->CharacterId = TEXT("Notification.Character");
		Character->DisplayName = FText::FromString(TEXT("Notification Character"));
		Character->FingerCount = 10;
		Character->HpPerFinger = 2;
		for (UCardDefinition* Card : StarterDeck)
		{
			Character->StarterDeck.Add(Card);
		}
		return Character;
	}

	FKnockdownExpGain MakeNotificationExpGain(FName PartId, int32 ExpAmount)
	{
		FKnockdownExpGain Gain;
		Gain.PartId = PartId;
		Gain.ExpAmount = ExpAmount;
		return Gain;
	}

	FRunShopOfferInput MakeNotificationShopOffer(UCardDefinition* Card, int32 Price)
	{
		FRunShopOfferInput Offer;
		Offer.CardDefinition = Card;
		Offer.Price = Price;
		return Offer;
	}

	bool NotificationStorageContainsDefinition(
		const FRunBackpackStorageSnapshot& Snapshot,
		const UCardDefinition* Card)
	{
		auto ContainsInCards = [Card](const TArray<FRunStorageCardView>& Cards)
		{
			return Cards.ContainsByPredicate(
				[Card](const FRunStorageCardView& View)
				{
					return View.Instance.Definition.Get() == Card;
				});
		};

		if (ContainsInCards(Snapshot.Flux.ContentCards)
			|| ContainsInCards(Snapshot.BattleDeckPhysicalCards)
			|| ContainsInCards(Snapshot.BattleDeckProjectedCards)
			|| ContainsInCards(Snapshot.BurdenCards))
		{
			return true;
		}

		for (const FRunSpecialStorageView& Special : Snapshot.SpecialZones)
		{
			if (Special.OwnerCard.Instance.Definition.Get() == Card
				|| ContainsInCards(Special.ContentCards))
			{
				return true;
			}
		}
		return false;
	}

	FGuid FindNotificationStorageInstanceIdByDefinition(
		const FRunBackpackStorageSnapshot& Snapshot,
		const UCardDefinition* Card)
	{
		auto FindInCards = [Card](const TArray<FRunStorageCardView>& Cards)
		{
			for (const FRunStorageCardView& View : Cards)
			{
				if (View.Instance.Definition.Get() == Card)
				{
					return View.Instance.InstanceId;
				}
			}
			return FGuid();
		};

		FGuid Result = FindInCards(Snapshot.Flux.ContentCards);
		if (Result.IsValid()) { return Result; }
		Result = FindInCards(Snapshot.BattleDeckPhysicalCards);
		if (Result.IsValid()) { return Result; }
		Result = FindInCards(Snapshot.BattleDeckProjectedCards);
		if (Result.IsValid()) { return Result; }
		Result = FindInCards(Snapshot.BurdenCards);
		if (Result.IsValid()) { return Result; }

		for (const FRunSpecialStorageView& Special : Snapshot.SpecialZones)
		{
			if (Special.OwnerCard.Instance.Definition.Get() == Card)
			{
				return Special.OwnerCard.Instance.InstanceId;
			}
			Result = FindInCards(Special.ContentCards);
			if (Result.IsValid())
			{
				return Result;
			}
		}
		return FGuid();
	}

	UWacomRunEventDefinition* MakeNotificationRunEvent(UObject* Outer, UCardDefinition* RewardCard)
	{
		UWacomRunEventDefinition* Event = NewObject<UWacomRunEventDefinition>(Outer);
		Event->EventId = TEXT("Notification.Event");
		Event->DisplayName = FText::FromString(TEXT("Notification Event"));
		Event->StartNodeId = TEXT("Start");

		FWacomRunEventChoiceDefinition Choice;
		Choice.ChoiceId = TEXT("Resolve");
		Choice.LabelText = FText::FromString(TEXT("Resolve"));

		FWacomRunEventEffectDefinition Gold;
		Gold.Type = EWacomRunEventEffectType::AddGold;
		Gold.Value = 3;

		FWacomRunEventEffectDefinition GainCard;
		GainCard.Type = EWacomRunEventEffectType::GainCard;
		GainCard.CardDefinition = RewardCard;

		Choice.Effects = { Gold, GainCard };
		Choice.bMarkEventCompleted = true;
		Choice.bCloseEventAfterResolve = true;

		FWacomRunEventNodeDefinition Start;
		Start.NodeId = TEXT("Start");
		Start.Choices.Add(Choice);
		Event->Nodes.Add(Start);
		return Event;
	}

	FRunWorldCardInteractionRequest MakeNotificationWorldInteractionRequest(
		const URunSession& Run,
		UCardDefinition* SourceCard,
		UCardDefinition* RewardCard)
	{
		FRunWorldCardInteractionRequest Request;
		Request.PersistentId = TEXT("Notification.WorldInteraction");
		Request.bConsumeCardOnSuccess = true;
		Request.AllowedCardDefinitions.Add(SourceCard);

		FWacomRunWorldCardInteractionReward GoldReward;
		GoldReward.Type = EWacomRunWorldCardInteractionRewardType::Gold;
		GoldReward.GoldAmount = 2;
		Request.Rewards.Add(GoldReward);

		FWacomRunWorldCardInteractionReward CardReward;
		CardReward.Type = EWacomRunWorldCardInteractionRewardType::Card;
		CardReward.CardDefinition = RewardCard;
		Request.Rewards.Add(CardReward);

		Request.SourceCardInstanceId =
			FindNotificationStorageInstanceIdByDefinition(Run.BuildBackpackStorageSnapshot(), SourceCard);
		return Request;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunNotificationBattleSettlementCoalescesSpec,
	"Wacom.Run.NotificationCoalescing.BattleSettlementCoalescesPressureExperienceAndGainedCardNotifications",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunNotificationBattleSettlementCoalescesSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Starter = MakeNotificationCard(Fx, TEXT("Notification.Starter"));
	UCardDefinition* Reward = MakeNotificationCard(Fx, TEXT("Notification.BattleReward"));
	UCharacterDefinition* Character = MakeNotificationCharacter(GetTransientPackage(), { Starter });

	FWacomRunExplorationFixture Exploration;
	UWacomFloorMapDefinition* Floor =
		Exploration.MakeLinearFloor(TEXT("Notification.Battle.Floor"), 1);
	Floor->Nodes[0].NodeType = EWacomMapNodeType::Encounter;
	URunSession* Run = Exploration.CreateInitializedSession(
		Character,
		Exploration.MakeJourney({ Floor }, TEXT("Notification.Battle.Journey"))).Session;
	const FRunExplorationResolution Begin =
		Run->BeginCurrentNodeActivity(ERunNodeActivityKind::Encounter);
	if (!TestTrue(TEXT("Encounter begins"), Begin.IsOk() && Begin.NodeActivityTicket.IsSet()))
	{
		return false;
	}

	int32 BroadcastCount = 0;
	Run->OnRunStateChangedNative.AddLambda([&BroadcastCount]()
	{
		++BroadcastCount;
	});

	FBattleResultPacket Packet;
	Packet.Outcome = EBattleOutcome::Victory;
	Packet.bCrossedHighHpThreshold = true;
	Packet.bCrossedLowHpThreshold = true;
	Packet.KnockdownExpGains.Add(MakeNotificationExpGain(TEXT("Notification.Part.A"), 3));
	FBattleGainedCard GainedCard;
	GainedCard.Definition = Reward;
	GainedCard.SourcePartKey = FBattleEnemyPartKey::Make(
		TEXT("Notification.Battle.Trigger"),
		TEXT("Enemy"),
		TEXT("Notification.Part.A"));
	GainedCard.SourceChoice = EKnockdownChoice::Aid;
	Packet.GainedCards.Add(GainedCard);

	const FNotificationRevisionSnapshot Before = CaptureNotificationRevisions(*Run);
	const FRunExplorationResolution Settlement = Run->SettleEncounterNodeActivity(
		Begin.NodeActivityTicket.GetValue(), Packet);
	TestTrue(TEXT("Battle settlement succeeds"), Settlement.IsOk());
	const FNotificationRevisionSnapshot After = CaptureNotificationRevisions(*Run);

	TestEqual(TEXT("Composite battle settlement broadcasts once"), BroadcastCount, 1);
	TestEqual(TEXT("Fatigue +1"), Run->GetPressureValue(EWacomPressureType::Fatigue), 1);
	TestEqual(TEXT("Wound +6"), Run->GetPressureValue(EWacomPressureType::Wound), 6);
	TestEqual(TEXT("Experience applied"), Run->GetExperienceCurrent(), 3);
	TestTrue(TEXT("Battle reward enters run"),
		NotificationStorageContainsDefinition(Run->BuildBackpackStorageSnapshot(), Reward));
	TestTrue(TEXT("Battle card reward bumps backpack revision"), After.BackpackStorage > Before.BackpackStorage);
	TestEqual(TEXT("Battle settlement leaves shop revision"), After.Shop, Before.Shop);
	TestEqual(TEXT("Battle settlement leaves economy revision"), After.Economy, Before.Economy);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunNotificationBattleUndeterminedSpec,
	"Wacom.Run.NotificationCoalescing.BattleSettlementUndeterminedDoesNotBroadcast",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunNotificationBattleUndeterminedSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Starter = MakeNotificationCard(Fx, TEXT("Notification.Undetermined.Starter"));
	UCharacterDefinition* Character = MakeNotificationCharacter(GetTransientPackage(), { Starter });

	FWacomRunExplorationFixture Exploration;
	UWacomFloorMapDefinition* Floor =
		Exploration.MakeLinearFloor(TEXT("Notification.Undetermined.Floor"), 1);
	Floor->Nodes[0].NodeType = EWacomMapNodeType::Encounter;
	URunSession* Run = Exploration.CreateInitializedSession(
		Character,
		Exploration.MakeJourney({ Floor }, TEXT("Notification.Undetermined.Journey"))).Session;
	const FRunExplorationResolution Begin =
		Run->BeginCurrentNodeActivity(ERunNodeActivityKind::Encounter);
	if (!TestTrue(TEXT("Encounter begins"), Begin.IsOk() && Begin.NodeActivityTicket.IsSet()))
	{
		return false;
	}

	int32 BroadcastCount = 0;
	Run->OnRunStateChangedNative.AddLambda([&BroadcastCount]()
	{
		++BroadcastCount;
	});

	FBattleResultPacket Packet;
	Packet.Outcome = EBattleOutcome::Undetermined;
	const FNotificationRevisionSnapshot Before = CaptureNotificationRevisions(*Run);
	const FRunExplorationResolution Settlement = Run->SettleEncounterNodeActivity(
		Begin.NodeActivityTicket.GetValue(), Packet);
	TestFalse(TEXT("Undetermined settlement is rejected"), Settlement.IsOk());
	const FNotificationRevisionSnapshot After = CaptureNotificationRevisions(*Run);

	TestEqual(TEXT("Undetermined battle does not broadcast"), BroadcastCount, 0);
	TestEqual(TEXT("Undetermined leaves backpack revision"), After.BackpackStorage, Before.BackpackStorage);
	TestEqual(TEXT("Undetermined leaves shop revision"), After.Shop, Before.Shop);
	TestEqual(TEXT("Undetermined leaves economy revision"), After.Economy, Before.Economy);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunNotificationEndShopVisitCoalescesSpec,
	"Wacom.Run.NotificationCoalescing.EndShopVisitBroadcastsOnceWithDeferredPhaseAdvance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunNotificationEndShopVisitCoalescesSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Bag = MakeNotificationCard(Fx, TEXT("Notification.Shop.Bag"), 5);
	UCardDefinition* OfferCard = MakeNotificationCard(Fx, TEXT("Notification.Shop.Offer"));
	UCharacterDefinition* Character = MakeNotificationCharacter(GetTransientPackage(), { Bag });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), InitializeRunSessionForTest(*Run, Character, EWacomMapNodeType::Shop).IsOk());
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run);
	State.TimeState.CurrentTimePhase = ETimePhase::Morning;
	State.TimeState.RemainingActionPoints = 1;
	const int32 DayActionPoints = State.TimeState.PhaseBudgets.Day;
	Run->AddGold(5);
	TestTrue(TEXT("Begin shop succeeds"),
		Run->BeginShopVisit(
			TEXT("Notification.Shop"),
			{ MakeNotificationShopOffer(OfferCard, 1) }));
	const FRunShopSnapshot Snapshot = Run->BuildCurrentShopSnapshot();
	TestTrue(TEXT("Shop has offer"), Snapshot.Offers.Num() == 1);
	const FGuid OfferId = Snapshot.Offers.IsValidIndex(0) ? Snapshot.Offers[0].OfferId : FGuid();
	TestTrue(TEXT("Purchase succeeds"), Run->PurchaseShopOffer(OfferId).bSucceeded);
	TestTrue(TEXT("Purchase keeps shop open at zero AP"), Run->IsShopVisitActive());
	TestEqual(TEXT("Purchase defers phase"), Run->GetCurrentTimePhase(), ETimePhase::Morning);
	TestEqual(TEXT("Purchase consumes final AP"), Run->GetRemainingActionPoints(), 0);

	int32 BroadcastCount = 0;
	Run->OnRunStateChangedNative.AddLambda([&BroadcastCount]()
	{
		++BroadcastCount;
	});

	const FNotificationRevisionSnapshot Before = CaptureNotificationRevisions(*Run);
	Run->EndShopVisit();
	const FNotificationRevisionSnapshot After = CaptureNotificationRevisions(*Run);

	TestEqual(TEXT("EndShopVisit broadcasts once"), BroadcastCount, 1);
	TestEqual(TEXT("EndShopVisit applies deferred phase"), Run->GetCurrentTimePhase(), ETimePhase::Day);
	TestEqual(TEXT("EndShopVisit loads Day budget without extra cost"), Run->GetRemainingActionPoints(), DayActionPoints);
	TestFalse(TEXT("Shop closed"), Run->IsShopVisitActive());
	TestTrue(TEXT("EndShopVisit bumps shop revision"), After.Shop > Before.Shop);
	TestEqual(TEXT("EndShopVisit leaves backpack revision"), After.BackpackStorage, Before.BackpackStorage);
	TestEqual(TEXT("EndShopVisit leaves economy revision"), After.Economy, Before.Economy);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunNotificationRunEventChoiceSpec,
	"Wacom.Run.NotificationCoalescing.RunEventChoiceStillBroadcastsOnceAndKeepsRevisionSemantics",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunNotificationRunEventChoiceSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Starter = MakeNotificationCard(Fx, TEXT("Notification.Event.Starter"));
	UCardDefinition* Reward = MakeNotificationCard(Fx, TEXT("Notification.Event.Reward"));
	UCharacterDefinition* Character = MakeNotificationCharacter(GetTransientPackage(), { Starter });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), InitializeRunSessionForTest(*Run, Character, EWacomMapNodeType::RunEvent).IsOk());
	TStrongObjectPtr<UWacomRunEventDefinition> Event(MakeNotificationRunEvent(Run.Get(), Reward));
	TestTrue(TEXT("Begin event succeeds"), Run->BeginRunEvent(TEXT("Notification.Event.Actor"), Event.Get()));

	int32 BroadcastCount = 0;
	Run->OnRunStateChangedNative.AddLambda([&BroadcastCount]()
	{
		++BroadcastCount;
	});

	const FNotificationRevisionSnapshot Before = CaptureNotificationRevisions(*Run);
	const FRunEventChoiceResult Result = Run->ChooseRunEventOptionWithResult(TEXT("Resolve"));
	const FNotificationRevisionSnapshot After = CaptureNotificationRevisions(*Run);

	TestTrue(TEXT("Choice succeeds"), Result.bSucceeded);
	TestEqual(TEXT("RunEvent choice broadcasts once"), BroadcastCount, 1);
	TestTrue(TEXT("RunEvent gain card bumps backpack"), After.BackpackStorage > Before.BackpackStorage);
	TestEqual(TEXT("RunEvent leaves shop revision"), After.Shop, Before.Shop);
	TestTrue(TEXT("RunEvent add gold bumps economy"), After.Economy > Before.Economy);
	TestTrue(TEXT("Reward card enters run"),
		NotificationStorageContainsDefinition(Run->BuildBackpackStorageSnapshot(), Reward));
	TestEqual(TEXT("Gold applied"), Run->GetGold(), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunNotificationWorldInteractionSpec,
	"Wacom.Run.NotificationCoalescing.RunWorldCardInteractionStillBroadcastsOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunNotificationWorldInteractionSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Key = MakeNotificationCard(Fx, TEXT("Notification.World.Key"));
	UCardDefinition* Reward = MakeNotificationCard(Fx, TEXT("Notification.World.Reward"));
	UCharacterDefinition* Character = MakeNotificationCharacter(GetTransientPackage(), { Key });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), InitializeRunSessionForTest(*Run, Character, EWacomMapNodeType::Treasure).IsOk());

	int32 BroadcastCount = 0;
	Run->OnRunStateChangedNative.AddLambda([&BroadcastCount]()
	{
		++BroadcastCount;
	});

	const FNotificationRevisionSnapshot Before = CaptureNotificationRevisions(*Run);
	const FRunWorldCardInteractionRequest Request =
		MakeNotificationWorldInteractionRequest(*Run, Key, Reward);
	TestTrue(TEXT("Request has source card"), Request.SourceCardInstanceId.IsValid());
	TestTrue(TEXT("World interaction succeeds"), Run->SubmitRunWorldCardInteraction(Request).bSucceeded);
	const FNotificationRevisionSnapshot After = CaptureNotificationRevisions(*Run);

	TestEqual(TEXT("World interaction broadcasts once"), BroadcastCount, 1);
	TestTrue(TEXT("World consume/card reward bumps backpack"), After.BackpackStorage > Before.BackpackStorage);
	TestEqual(TEXT("World interaction leaves shop revision"), After.Shop, Before.Shop);
	TestTrue(TEXT("World gold reward bumps economy"), After.Economy > Before.Economy);
	TestTrue(TEXT("Reward card enters run"),
		NotificationStorageContainsDefinition(Run->BuildBackpackStorageSnapshot(), Reward));
	TestEqual(TEXT("Gold applied"), Run->GetGold(), 2);
	return true;
}
