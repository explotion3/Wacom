// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Events/RunEventDefinition.h"
#include "Fixtures/BattleTestFixtures.h"
#include "RunSession.h"
#include "RunState.h"
#include "Tags/WacomGameplayTags.h"

#include "UObject/StrongObjectPtr.h"

namespace
{
	struct FRunUiRevisionSnapshot
	{
		uint64 BackpackStorage = 0;
		uint64 Shop = 0;
		uint64 Economy = 0;
	};

	FRunUiRevisionSnapshot CaptureRunUiRevisions(const URunSession& Run)
	{
		return FRunUiRevisionSnapshot{
			Run.GetBackpackStorageSnapshotRevision(),
			Run.GetShopSnapshotRevision(),
			Run.GetEconomySnapshotRevision(),
		};
	}

	UCardDefinition* MakeRevisionCard(
		FWacomBattleFixture& Fx,
		FName CardId,
		int32 Capacity = 0,
		bool bTypeBContainer = false)
	{
		UCardDefinition* Card = Fx.MakeNoopCard(0);
		Card->CardId = CardId;
		Card->DisplayName = FText::FromName(CardId);
		Card->Rarity = WacomTags::Card_Rarity_White;
		Card->Physique.Capacity = Capacity;
		if (bTypeBContainer)
		{
			Card->Physique.CapacityEffect = WacomTags::Card_CapacityEffect_Placeholder;
		}
		return Card;
	}

	UCharacterDefinition* MakeRevisionCharacter(
		UObject* Outer,
		const TArray<UCardDefinition*>& StarterDeck)
	{
		UCharacterDefinition* Character = NewObject<UCharacterDefinition>(Outer);
		Character->CharacterId = TEXT("Revision.Character");
		Character->DisplayName = FText::FromString(TEXT("Revision Character"));
		Character->FingerCount = 10;
		Character->HpPerFinger = 2;
		for (UCardDefinition* Card : StarterDeck)
		{
			Character->StarterDeck.Add(Card);
		}
		return Character;
	}

	FRunShopOfferInput MakeShopOffer(UCardDefinition* Card, int32 Price)
	{
		FRunShopOfferInput Offer;
		Offer.CardDefinition = Card;
		Offer.Price = Price;
		return Offer;
	}

	FGuid FindStorageInstanceIdByDefinition(
		const FRunBackpackStorageSnapshot& Snapshot,
		const UCardDefinition* Card)
	{
		for (const FRunStorageCardView& View : Snapshot.Flux.ContentCards)
		{
			if (View.Instance.Definition.Get() == Card)
			{
				return View.Instance.InstanceId;
			}
		}
		for (const FRunStorageCardView& View : Snapshot.BattleDeckPhysicalCards)
		{
			if (View.Instance.Definition.Get() == Card)
			{
				return View.Instance.InstanceId;
			}
		}
		for (const FRunStorageCardView& View : Snapshot.BurdenCards)
		{
			if (View.Instance.Definition.Get() == Card)
			{
				return View.Instance.InstanceId;
			}
		}
		for (const FRunSpecialStorageView& Special : Snapshot.SpecialZones)
		{
			if (Special.OwnerCard.Instance.Definition.Get() == Card)
			{
				return Special.OwnerCard.Instance.InstanceId;
			}
			for (const FRunStorageCardView& View : Special.ContentCards)
			{
				if (View.Instance.Definition.Get() == Card)
				{
					return View.Instance.InstanceId;
				}
			}
		}
		return FGuid();
	}

	FGuid FindBattleDeckInstanceIdByDefinition(const URunSession& Run, const UCardDefinition* Card)
	{
		for (const FCardInstance& Instance : Run.GetBattleDeck())
		{
			if (Instance.Definition.Get() == Card)
			{
				return Instance.InstanceId;
			}
		}
		return FGuid();
	}

	FRunWorldCardInteractionRequest MakeWorldInteractionRequest(
		const URunSession& Run,
		UCardDefinition* SourceCard,
		FName PersistentId,
		const TArray<FWacomRunWorldCardInteractionReward>& Rewards,
		bool bConsumeCardOnSuccess = true)
	{
		FRunWorldCardInteractionRequest Request;
		Request.PersistentId = PersistentId;
		Request.SourceCardInstanceId = FindBattleDeckInstanceIdByDefinition(Run, SourceCard);
		Request.AllowedCardDefinitions = { SourceCard };
		Request.AllowedCardIds = { SourceCard ? SourceCard->CardId : NAME_None };
		Request.bConsumeCardOnSuccess = bConsumeCardOnSuccess;
		Request.Rewards = Rewards;
		return Request;
	}

	FWacomRunWorldCardInteractionReward MakeGoldReward(int32 Amount)
	{
		FWacomRunWorldCardInteractionReward Reward;
		Reward.Type = EWacomRunWorldCardInteractionRewardType::Gold;
		Reward.GoldAmount = Amount;
		return Reward;
	}

	FWacomRunWorldCardInteractionReward MakeCardReward(UCardDefinition* Card)
	{
		FWacomRunWorldCardInteractionReward Reward;
		Reward.Type = EWacomRunWorldCardInteractionRewardType::Card;
		Reward.CardDefinition = Card;
		return Reward;
	}

	UWacomRunEventDefinition* MakeSingleChoiceRunEvent(
		UObject* Outer,
		const FWacomRunEventChoiceDefinition& Choice)
	{
		UWacomRunEventDefinition* Event = NewObject<UWacomRunEventDefinition>(Outer);
		Event->EventId = TEXT("Revision.Event");
		Event->DisplayName = FText::FromString(TEXT("Revision Event"));
		Event->StartNodeId = TEXT("Start");

		FWacomRunEventNodeDefinition Start;
		Start.NodeId = TEXT("Start");
		Start.Choices = { Choice };
		Event->Nodes = { Start };
		return Event;
	}

	FRunEventChoiceResult RunSingleChoiceEvent(
		URunSession& Run,
		UWacomRunEventDefinition* Event,
		FName PersistentId,
		FName ChoiceId = TEXT("Resolve"))
	{
		if (!Run.BeginRunEvent(PersistentId, Event))
		{
			return FRunEventChoiceResult{};
		}
		return Run.ChooseRunEventOptionWithResult(ChoiceId);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunSnapshotRevisionsBackpackStorageMutationPathsSpec,
	"Wacom.Run.SnapshotRevisions.BackpackStorageMutationPaths",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunSnapshotRevisionsBackpackStorageMutationPathsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Bag = MakeRevisionCard(Fx, TEXT("Revision.Bag"), 6);
	UCardDefinition* Normal = MakeRevisionCard(Fx, TEXT("Revision.Normal"));
	UCardDefinition* Added = MakeRevisionCard(Fx, TEXT("Revision.Added"));
	UCharacterDefinition* Character = MakeRevisionCharacter(GetTransientPackage(), { Bag, Normal });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Initialize bumps all tracked revisions"), Run->Initialize(Character));
	const FRunUiRevisionSnapshot Initial = CaptureRunUiRevisions(*Run);
	TestTrue(TEXT("Backpack revision starts marked"), Initial.BackpackStorage > 0);
	TestTrue(TEXT("Shop revision starts marked"), Initial.Shop > 0);
	TestTrue(TEXT("Economy revision starts marked"), Initial.Economy > 0);

	const FRunUiRevisionSnapshot BeforeAcquire = CaptureRunUiRevisions(*Run);
	Run->AcquireCardToRun(Added);
	const FRunUiRevisionSnapshot AfterAcquire = CaptureRunUiRevisions(*Run);
	TestTrue(TEXT("AcquireCardToRun bumps backpack"), AfterAcquire.BackpackStorage > BeforeAcquire.BackpackStorage);
	TestEqual(TEXT("AcquireCardToRun leaves shop"), AfterAcquire.Shop, BeforeAcquire.Shop);
	TestEqual(TEXT("AcquireCardToRun leaves economy"), AfterAcquire.Economy, BeforeAcquire.Economy);

	const FGuid NormalId = FindStorageInstanceIdByDefinition(Run->BuildBackpackStorageSnapshot(), Normal);
	TestTrue(TEXT("Normal instance exists"), NormalId.IsValid());
	const FRunUiRevisionSnapshot BeforeMove = CaptureRunUiRevisions(*Run);
	TestTrue(TEXT("Move to backpack succeeds"), Run->MoveInstance(NormalId, EZoneKind::Backpack, FGuid()));
	const FRunUiRevisionSnapshot AfterMove = CaptureRunUiRevisions(*Run);
	TestTrue(TEXT("MoveInstance bumps backpack"), AfterMove.BackpackStorage > BeforeMove.BackpackStorage);
	TestEqual(TEXT("MoveInstance leaves shop"), AfterMove.Shop, BeforeMove.Shop);
	TestEqual(TEXT("MoveInstance leaves economy"), AfterMove.Economy, BeforeMove.Economy);

	const FRunUiRevisionSnapshot BeforeDelete = CaptureRunUiRevisions(*Run);
	TestTrue(TEXT("Destroy by instance succeeds"), Run->DestroyCardByInstance(NormalId));
	const FRunUiRevisionSnapshot AfterDelete = CaptureRunUiRevisions(*Run);
	TestTrue(TEXT("DestroyCardByInstance bumps backpack"), AfterDelete.BackpackStorage > BeforeDelete.BackpackStorage);
	TestEqual(TEXT("DestroyCardByInstance leaves shop"), AfterDelete.Shop, BeforeDelete.Shop);
	TestEqual(TEXT("DestroyCardByInstance leaves economy"), AfterDelete.Economy, BeforeDelete.Economy);

	UCardDefinition* TypeB = MakeRevisionCard(Fx, TEXT("Revision.TypeB"), 3, true);
	UCardDefinition* SpecialContent = MakeRevisionCard(Fx, TEXT("Revision.SpecialContent"));
	UCharacterDefinition* SpecialCharacter = MakeRevisionCharacter(GetTransientPackage(), { TypeB, SpecialContent });
	TStrongObjectPtr<URunSession> SpecialRun(NewObject<URunSession>());
	TestTrue(TEXT("Special run initializes"), SpecialRun->Initialize(SpecialCharacter));
	const FGuid OwnerId = SpecialRun->GetBackpack().IsValidIndex(0)
		? SpecialRun->GetBackpack()[0].InstanceId
		: FGuid();
	const FGuid ContentId = FindStorageInstanceIdByDefinition(
		SpecialRun->BuildBackpackStorageSnapshot(),
		SpecialContent);
	TestTrue(TEXT("Owner id valid"), OwnerId.IsValid());
	TestTrue(TEXT("Content id valid"), ContentId.IsValid());
	TestTrue(TEXT("Move content to special zone succeeds"),
		SpecialRun->MoveInstance(ContentId, EZoneKind::SpecialZone, OwnerId));
	TestTrue(TEXT("Move owner to battle deck succeeds"),
		SpecialRun->MoveInstance(OwnerId, EZoneKind::BattleDeck, FGuid()));
	const FRunUiRevisionSnapshot BeforeProjection = CaptureRunUiRevisions(*SpecialRun);
	TestTrue(TEXT("Enable special projection succeeds"),
		SpecialRun->SetSpecialZoneCardBattleEnabled(ContentId, true));
	const FRunUiRevisionSnapshot AfterProjection = CaptureRunUiRevisions(*SpecialRun);
	TestTrue(TEXT("Special projection bumps backpack"),
		AfterProjection.BackpackStorage > BeforeProjection.BackpackStorage);
	TestEqual(TEXT("Special projection leaves shop"), AfterProjection.Shop, BeforeProjection.Shop);
	TestEqual(TEXT("Special projection leaves economy"), AfterProjection.Economy, BeforeProjection.Economy);

	UCardDefinition* PickupCard = MakeRevisionCard(Fx, TEXT("Revision.PickupCard"));
	const FRunUiRevisionSnapshot BeforePickup = CaptureRunUiRevisions(*Run);
	TestTrue(TEXT("Card pickup succeeds"), Run->CollectCardPickup(TEXT("Revision.Pickup.Card"), PickupCard));
	const FRunUiRevisionSnapshot AfterPickup = CaptureRunUiRevisions(*Run);
	TestTrue(TEXT("Card pickup bumps backpack"), AfterPickup.BackpackStorage > BeforePickup.BackpackStorage);
	TestEqual(TEXT("Card pickup leaves shop"), AfterPickup.Shop, BeforePickup.Shop);
	TestEqual(TEXT("Card pickup leaves economy"), AfterPickup.Economy, BeforePickup.Economy);

	UCardDefinition* Key = MakeRevisionCard(Fx, TEXT("Revision.World.Key"));
	UCardDefinition* WorldBag = MakeRevisionCard(Fx, TEXT("Revision.World.Bag"), 3);
	UCardDefinition* WorldReward = MakeRevisionCard(Fx, TEXT("Revision.World.CardReward"));
	UCharacterDefinition* WorldCharacter = MakeRevisionCharacter(GetTransientPackage(), { WorldBag, Key });
	TStrongObjectPtr<URunSession> WorldRun(NewObject<URunSession>());
	TestTrue(TEXT("World run initializes"), WorldRun->Initialize(WorldCharacter));
	const FRunUiRevisionSnapshot BeforeWorld = CaptureRunUiRevisions(*WorldRun);
	TestTrue(TEXT("World card interaction succeeds"),
		WorldRun->SubmitRunWorldCardInteraction(
			MakeWorldInteractionRequest(
				*WorldRun,
				Key,
				TEXT("Revision.World.Card"),
				{ MakeCardReward(WorldReward) },
				true)));
	const FRunUiRevisionSnapshot AfterWorld = CaptureRunUiRevisions(*WorldRun);
	TestTrue(TEXT("World consume/card reward bumps backpack"),
		AfterWorld.BackpackStorage > BeforeWorld.BackpackStorage);
	TestEqual(TEXT("World card-only reward leaves shop"), AfterWorld.Shop, BeforeWorld.Shop);
	TestEqual(TEXT("World card-only reward leaves economy"), AfterWorld.Economy, BeforeWorld.Economy);

	UCardDefinition* BattleReward = MakeRevisionCard(Fx, TEXT("Revision.BattleReward"));
	UCharacterDefinition* BattleCharacter = MakeRevisionCharacter(GetTransientPackage(), { Bag });
	TStrongObjectPtr<URunSession> BattleRun(NewObject<URunSession>());
	TestTrue(TEXT("Battle run initializes"), BattleRun->Initialize(BattleCharacter));
	FBattleResultPacket Packet;
	Packet.Outcome = EBattleOutcome::Victory;
	FBattleGainedCard GainedCard;
	GainedCard.Definition = BattleReward;
	Packet.GainedCards.Add(GainedCard);
	const FRunUiRevisionSnapshot BeforeBattle = CaptureRunUiRevisions(*BattleRun);
	BattleRun->OnBattleFinishedFromTrigger(Packet, NewObject<UEnemyDefinition>(), TEXT("Revision.Battle.Trigger"));
	const FRunUiRevisionSnapshot AfterBattle = CaptureRunUiRevisions(*BattleRun);
	TestTrue(TEXT("Battle gained card bumps backpack"),
		AfterBattle.BackpackStorage > BeforeBattle.BackpackStorage);
	TestEqual(TEXT("Battle gained card leaves shop"), AfterBattle.Shop, BeforeBattle.Shop);
	TestEqual(TEXT("Battle gained card leaves economy"), AfterBattle.Economy, BeforeBattle.Economy);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunSnapshotRevisionsEconomyMutationPathsSpec,
	"Wacom.Run.SnapshotRevisions.EconomyMutationPaths",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunSnapshotRevisionsEconomyMutationPathsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	const FRunUiRevisionSnapshot Initial = CaptureRunUiRevisions(*Run);

	Run->AddGold(5);
	const FRunUiRevisionSnapshot AfterAddGold = CaptureRunUiRevisions(*Run);
	TestTrue(TEXT("AddGold bumps economy"), AfterAddGold.Economy > Initial.Economy);
	TestEqual(TEXT("AddGold leaves backpack"), AfterAddGold.BackpackStorage, Initial.BackpackStorage);
	TestEqual(TEXT("AddGold leaves shop"), AfterAddGold.Shop, Initial.Shop);

	const FRunUiRevisionSnapshot BeforeRemoveGold = CaptureRunUiRevisions(*Run);
	TestTrue(TEXT("RemoveGold succeeds"), Run->RemoveGold(2));
	const FRunUiRevisionSnapshot AfterRemoveGold = CaptureRunUiRevisions(*Run);
	TestTrue(TEXT("RemoveGold bumps economy"), AfterRemoveGold.Economy > BeforeRemoveGold.Economy);
	TestEqual(TEXT("RemoveGold leaves backpack"), AfterRemoveGold.BackpackStorage, BeforeRemoveGold.BackpackStorage);
	TestEqual(TEXT("RemoveGold leaves shop"), AfterRemoveGold.Shop, BeforeRemoveGold.Shop);

	const FRunUiRevisionSnapshot BeforePickup = CaptureRunUiRevisions(*Run);
	TestTrue(TEXT("Gold pickup succeeds"), Run->CollectGoldPickup(TEXT("Revision.Pickup.Gold"), 3));
	const FRunUiRevisionSnapshot AfterPickup = CaptureRunUiRevisions(*Run);
	TestTrue(TEXT("Gold pickup bumps economy"), AfterPickup.Economy > BeforePickup.Economy);
	TestEqual(TEXT("Gold pickup leaves backpack"), AfterPickup.BackpackStorage, BeforePickup.BackpackStorage);
	TestEqual(TEXT("Gold pickup leaves shop"), AfterPickup.Shop, BeforePickup.Shop);

	UCardDefinition* Key = MakeRevisionCard(Fx, TEXT("Revision.World.GoldKey"));
	UCardDefinition* WorldBag = MakeRevisionCard(Fx, TEXT("Revision.World.GoldBag"), 3);
	UCharacterDefinition* Character = MakeRevisionCharacter(GetTransientPackage(), { WorldBag, Key });
	TStrongObjectPtr<URunSession> WorldRun(NewObject<URunSession>());
	TestTrue(TEXT("World economy run initializes"), WorldRun->Initialize(Character));
	const FRunUiRevisionSnapshot BeforeWorld = CaptureRunUiRevisions(*WorldRun);
	TestTrue(TEXT("World gold interaction succeeds"),
		WorldRun->SubmitRunWorldCardInteraction(
			MakeWorldInteractionRequest(
				*WorldRun,
				Key,
				TEXT("Revision.World.Gold"),
				{ MakeGoldReward(4) },
				false)));
	const FRunUiRevisionSnapshot AfterWorld = CaptureRunUiRevisions(*WorldRun);
	TestTrue(TEXT("World gold reward bumps economy"), AfterWorld.Economy > BeforeWorld.Economy);
	TestEqual(TEXT("World gold reward leaves backpack"), AfterWorld.BackpackStorage, BeforeWorld.BackpackStorage);
	TestEqual(TEXT("World gold reward leaves shop"), AfterWorld.Shop, BeforeWorld.Shop);

	FWacomRunEventChoiceDefinition Choice;
	Choice.ChoiceId = TEXT("Resolve");
	FWacomRunEventEffectDefinition AddGold;
	AddGold.Type = EWacomRunEventEffectType::AddGold;
	AddGold.Value = 3;
	Choice.Effects = { AddGold };
	TStrongObjectPtr<UWacomRunEventDefinition> Event(MakeSingleChoiceRunEvent(Run.Get(), Choice));
	const FRunUiRevisionSnapshot BeforeEvent = CaptureRunUiRevisions(*Run);
	const FRunEventChoiceResult Result =
		RunSingleChoiceEvent(*Run, Event.Get(), TEXT("Revision.Event.AddGold"));
	const FRunUiRevisionSnapshot AfterEvent = CaptureRunUiRevisions(*Run);
	TestTrue(TEXT("AddGold RunEvent succeeds"), Result.bSucceeded);
	TestTrue(TEXT("RunEvent AddGold bumps economy"), AfterEvent.Economy > BeforeEvent.Economy);
	TestEqual(TEXT("RunEvent AddGold leaves backpack"), AfterEvent.BackpackStorage, BeforeEvent.BackpackStorage);
	TestEqual(TEXT("RunEvent AddGold leaves shop"), AfterEvent.Shop, BeforeEvent.Shop);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunSnapshotRevisionsShopMutationPathsSpec,
	"Wacom.Run.SnapshotRevisions.ShopMutationPaths",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunSnapshotRevisionsShopMutationPathsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* OfferCard = MakeRevisionCard(Fx, TEXT("Revision.Shop.Card"));
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->AddGold(5);

	const FRunUiRevisionSnapshot BeforeInvalidBegin = CaptureRunUiRevisions(*Run);
	TArray<FRunShopOfferInput> Offers = { MakeShopOffer(OfferCard, 2) };
	TestFalse(TEXT("Invalid shop id fails"), Run->BeginShopVisit(NAME_None, Offers));
	const FRunUiRevisionSnapshot AfterInvalidBegin = CaptureRunUiRevisions(*Run);
	TestEqual(TEXT("Invalid shop begin leaves backpack"),
		AfterInvalidBegin.BackpackStorage,
		BeforeInvalidBegin.BackpackStorage);
	TestEqual(TEXT("Invalid shop begin leaves shop"), AfterInvalidBegin.Shop, BeforeInvalidBegin.Shop);
	TestEqual(TEXT("Invalid shop begin leaves economy"), AfterInvalidBegin.Economy, BeforeInvalidBegin.Economy);

	const FRunUiRevisionSnapshot BeforeBegin = CaptureRunUiRevisions(*Run);
	TestTrue(TEXT("BeginShopVisit succeeds"), Run->BeginShopVisit(TEXT("Revision.Shop"), Offers));
	const FRunUiRevisionSnapshot AfterBegin = CaptureRunUiRevisions(*Run);
	TestTrue(TEXT("BeginShopVisit bumps shop"), AfterBegin.Shop > BeforeBegin.Shop);
	TestEqual(TEXT("BeginShopVisit leaves backpack"), AfterBegin.BackpackStorage, BeforeBegin.BackpackStorage);
	TestEqual(TEXT("BeginShopVisit leaves economy"), AfterBegin.Economy, BeforeBegin.Economy);

	const FRunUiRevisionSnapshot BeforeFailedPurchase = CaptureRunUiRevisions(*Run);
	TestFalse(TEXT("Unknown offer purchase fails"), Run->PurchaseShopOffer(FGuid::NewGuid()));
	const FRunUiRevisionSnapshot AfterFailedPurchase = CaptureRunUiRevisions(*Run);
	TestEqual(TEXT("Failed purchase leaves backpack"),
		AfterFailedPurchase.BackpackStorage,
		BeforeFailedPurchase.BackpackStorage);
	TestEqual(TEXT("Failed purchase leaves shop"), AfterFailedPurchase.Shop, BeforeFailedPurchase.Shop);
	TestEqual(TEXT("Failed purchase leaves economy"), AfterFailedPurchase.Economy, BeforeFailedPurchase.Economy);

	const FRunShopSnapshot Snapshot = Run->BuildCurrentShopSnapshot();
	TestTrue(TEXT("Shop has offer"), Snapshot.Offers.Num() == 1);
	const FGuid OfferId = Snapshot.Offers.IsValidIndex(0) ? Snapshot.Offers[0].OfferId : FGuid();
	const FRunUiRevisionSnapshot BeforePurchase = CaptureRunUiRevisions(*Run);
	TestTrue(TEXT("PurchaseShopOffer succeeds"), Run->PurchaseShopOffer(OfferId));
	const FRunUiRevisionSnapshot AfterPurchase = CaptureRunUiRevisions(*Run);
	TestTrue(TEXT("Purchase bumps backpack"), AfterPurchase.BackpackStorage > BeforePurchase.BackpackStorage);
	TestTrue(TEXT("Purchase bumps shop"), AfterPurchase.Shop > BeforePurchase.Shop);
	TestTrue(TEXT("Purchase bumps economy"), AfterPurchase.Economy > BeforePurchase.Economy);

	const FRunUiRevisionSnapshot BeforeRepeatPurchase = CaptureRunUiRevisions(*Run);
	TestFalse(TEXT("Repeat purchase fails"), Run->PurchaseShopOffer(OfferId));
	const FRunUiRevisionSnapshot AfterRepeatPurchase = CaptureRunUiRevisions(*Run);
	TestEqual(TEXT("Repeat purchase leaves backpack"),
		AfterRepeatPurchase.BackpackStorage,
		BeforeRepeatPurchase.BackpackStorage);
	TestEqual(TEXT("Repeat purchase leaves shop"), AfterRepeatPurchase.Shop, BeforeRepeatPurchase.Shop);
	TestEqual(TEXT("Repeat purchase leaves economy"), AfterRepeatPurchase.Economy, BeforeRepeatPurchase.Economy);

	const FRunUiRevisionSnapshot BeforeEnd = CaptureRunUiRevisions(*Run);
	Run->EndShopVisit();
	const FRunUiRevisionSnapshot AfterEnd = CaptureRunUiRevisions(*Run);
	TestTrue(TEXT("EndShopVisit bumps shop"), AfterEnd.Shop > BeforeEnd.Shop);
	TestEqual(TEXT("EndShopVisit leaves backpack"), AfterEnd.BackpackStorage, BeforeEnd.BackpackStorage);
	TestEqual(TEXT("EndShopVisit leaves economy"), AfterEnd.Economy, BeforeEnd.Economy);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunSnapshotRevisionsRunEventMutationPathsSpec,
	"Wacom.Run.SnapshotRevisions.RunEventMutationPaths",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunSnapshotRevisionsRunEventMutationPathsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Bag = MakeRevisionCard(Fx, TEXT("Revision.Event.Bag"), 8);
	UCardDefinition* PaidCard = MakeRevisionCard(Fx, TEXT("Revision.Event.Paid"));
	UCardDefinition* RewardCard = MakeRevisionCard(Fx, TEXT("Revision.Event.Reward"));
	UCardDefinition* RemovedCard = MakeRevisionCard(Fx, TEXT("Revision.Event.Removed"));
	UCharacterDefinition* Character = MakeRevisionCharacter(GetTransientPackage(), { Bag, PaidCard, RemovedCard });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Event run initializes"), Run->Initialize(Character));

	FWacomRunEventChoiceDefinition PayChoice;
	PayChoice.ChoiceId = TEXT("Pay");
	PayChoice.CardPayment.bRequiresOwnedCardPayment = true;
	PayChoice.CardPayment.AllowedCardDefinitions = { PaidCard };
	TStrongObjectPtr<UWacomRunEventDefinition> PayEvent(MakeSingleChoiceRunEvent(Run.Get(), PayChoice));
	TestTrue(TEXT("Begin pay event"), Run->BeginRunEvent(TEXT("Revision.Event.Pay"), PayEvent.Get()));
	const FGuid PaidId = FindStorageInstanceIdByDefinition(Run->BuildBackpackStorageSnapshot(), PaidCard);
	TestTrue(TEXT("Paid card id valid"), PaidId.IsValid());
	const FRunUiRevisionSnapshot BeforePay = CaptureRunUiRevisions(*Run);
	const FRunEventChoiceResult PayResult = Run->ChooseRunEventOptionWithPaidCardResult(TEXT("Pay"), PaidId);
	const FRunUiRevisionSnapshot AfterPay = CaptureRunUiRevisions(*Run);
	TestTrue(TEXT("Paid card RunEvent succeeds"), PayResult.bSucceeded);
	TestTrue(TEXT("Paid card bumps backpack"), AfterPay.BackpackStorage > BeforePay.BackpackStorage);
	TestEqual(TEXT("Paid card leaves shop"), AfterPay.Shop, BeforePay.Shop);
	TestEqual(TEXT("Paid card leaves economy"), AfterPay.Economy, BeforePay.Economy);
	Run->EndRunEvent();

	FWacomRunEventChoiceDefinition GainChoice;
	GainChoice.ChoiceId = TEXT("Resolve");
	FWacomRunEventEffectDefinition GainCard;
	GainCard.Type = EWacomRunEventEffectType::GainCard;
	GainCard.CardDefinition = RewardCard;
	GainChoice.Effects = { GainCard };
	TStrongObjectPtr<UWacomRunEventDefinition> GainEvent(MakeSingleChoiceRunEvent(Run.Get(), GainChoice));
	const FRunUiRevisionSnapshot BeforeGain = CaptureRunUiRevisions(*Run);
	const FRunEventChoiceResult GainResult =
		RunSingleChoiceEvent(*Run, GainEvent.Get(), TEXT("Revision.Event.Gain"));
	const FRunUiRevisionSnapshot AfterGain = CaptureRunUiRevisions(*Run);
	TestTrue(TEXT("GainCard RunEvent succeeds"), GainResult.bSucceeded);
	TestTrue(TEXT("GainCard bumps backpack"), AfterGain.BackpackStorage > BeforeGain.BackpackStorage);
	TestEqual(TEXT("GainCard leaves shop"), AfterGain.Shop, BeforeGain.Shop);
	TestEqual(TEXT("GainCard leaves economy"), AfterGain.Economy, BeforeGain.Economy);
	Run->EndRunEvent();

	FWacomRunEventChoiceDefinition RemoveChoice;
	RemoveChoice.ChoiceId = TEXT("Resolve");
	FWacomRunEventEffectDefinition RemoveCard;
	RemoveCard.Type = EWacomRunEventEffectType::RemoveCard;
	RemoveCard.CardDefinition = RemovedCard;
	RemoveChoice.Effects = { RemoveCard };
	TStrongObjectPtr<UWacomRunEventDefinition> RemoveEvent(MakeSingleChoiceRunEvent(Run.Get(), RemoveChoice));
	const FRunUiRevisionSnapshot BeforeRemove = CaptureRunUiRevisions(*Run);
	const FRunEventChoiceResult RemoveResult =
		RunSingleChoiceEvent(*Run, RemoveEvent.Get(), TEXT("Revision.Event.Remove"));
	const FRunUiRevisionSnapshot AfterRemove = CaptureRunUiRevisions(*Run);
	TestTrue(TEXT("RemoveCard RunEvent succeeds"), RemoveResult.bSucceeded);
	TestTrue(TEXT("RemoveCard bumps backpack"), AfterRemove.BackpackStorage > BeforeRemove.BackpackStorage);
	TestEqual(TEXT("RemoveCard leaves shop"), AfterRemove.Shop, BeforeRemove.Shop);
	TestEqual(TEXT("RemoveCard leaves economy"), AfterRemove.Economy, BeforeRemove.Economy);
	Run->EndRunEvent();

	FWacomRunEventChoiceDefinition GoldChoice;
	GoldChoice.ChoiceId = TEXT("Resolve");
	FWacomRunEventEffectDefinition AddGold;
	AddGold.Type = EWacomRunEventEffectType::AddGold;
	AddGold.Value = 2;
	GoldChoice.Effects = { AddGold };
	TStrongObjectPtr<UWacomRunEventDefinition> GoldEvent(MakeSingleChoiceRunEvent(Run.Get(), GoldChoice));
	const FRunUiRevisionSnapshot BeforeGold = CaptureRunUiRevisions(*Run);
	const FRunEventChoiceResult GoldResult =
		RunSingleChoiceEvent(*Run, GoldEvent.Get(), TEXT("Revision.Event.Gold"));
	const FRunUiRevisionSnapshot AfterGold = CaptureRunUiRevisions(*Run);
	TestTrue(TEXT("AddGold RunEvent succeeds"), GoldResult.bSucceeded);
	TestTrue(TEXT("AddGold bumps economy"), AfterGold.Economy > BeforeGold.Economy);
	TestEqual(TEXT("AddGold leaves backpack"), AfterGold.BackpackStorage, BeforeGold.BackpackStorage);
	TestEqual(TEXT("AddGold leaves shop"), AfterGold.Shop, BeforeGold.Shop);
	Run->EndRunEvent();

	FWacomRunEventChoiceDefinition NonSnapshotChoice;
	NonSnapshotChoice.ChoiceId = TEXT("Resolve");
	FWacomRunEventEffectDefinition AddPressure;
	AddPressure.Type = EWacomRunEventEffectType::AddPressure;
	AddPressure.PressureType = TEXT("Wound");
	AddPressure.Value = 1;
	FWacomRunEventEffectDefinition ConsumeNode;
	ConsumeNode.Type = EWacomRunEventEffectType::ConsumeNode;
	ConsumeNode.Value = 1;
	FWacomRunEventEffectDefinition SetFlag;
	SetFlag.Type = EWacomRunEventEffectType::SetRunFlag;
	SetFlag.FlagId = TEXT("Revision.Flag");
	FWacomRunEventEffectDefinition ClearFlag;
	ClearFlag.Type = EWacomRunEventEffectType::ClearRunFlag;
	ClearFlag.FlagId = TEXT("Revision.Flag");
	NonSnapshotChoice.Effects = { AddPressure, ConsumeNode, SetFlag, ClearFlag };
	TStrongObjectPtr<UWacomRunEventDefinition> NonSnapshotEvent(
		MakeSingleChoiceRunEvent(Run.Get(), NonSnapshotChoice));
	const FRunUiRevisionSnapshot BeforeNonSnapshot = CaptureRunUiRevisions(*Run);
	const FRunEventChoiceResult NonSnapshotResult =
		RunSingleChoiceEvent(*Run, NonSnapshotEvent.Get(), TEXT("Revision.Event.NonSnapshot"));
	const FRunUiRevisionSnapshot AfterNonSnapshot = CaptureRunUiRevisions(*Run);
	TestTrue(TEXT("Non-snapshot RunEvent succeeds"), NonSnapshotResult.bSucceeded);
	TestEqual(TEXT("Pressure/node/flags leave backpack"),
		AfterNonSnapshot.BackpackStorage,
		BeforeNonSnapshot.BackpackStorage);
	TestEqual(TEXT("Pressure/node/flags leave shop"), AfterNonSnapshot.Shop, BeforeNonSnapshot.Shop);
	TestEqual(TEXT("Pressure/node/flags leave economy"),
		AfterNonSnapshot.Economy,
		BeforeNonSnapshot.Economy);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunSnapshotRevisionsNonSnapshotMutationsSpec,
	"Wacom.Run.SnapshotRevisions.NonSnapshotMutationsDoNotBumpTrackedRevisions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunSnapshotRevisionsNonSnapshotMutationsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());

	FRunUiRevisionSnapshot Before = CaptureRunUiRevisions(*Run);
	Run->AddPressure(EWacomPressureType::Wound, 1);
	FRunUiRevisionSnapshot After = CaptureRunUiRevisions(*Run);
	TestEqual(TEXT("AddPressure leaves backpack"), After.BackpackStorage, Before.BackpackStorage);
	TestEqual(TEXT("AddPressure leaves shop"), After.Shop, Before.Shop);
	TestEqual(TEXT("AddPressure leaves economy"), After.Economy, Before.Economy);

	Before = CaptureRunUiRevisions(*Run);
	Run->ConsumeNode(1);
	After = CaptureRunUiRevisions(*Run);
	TestEqual(TEXT("ConsumeNode leaves backpack"), After.BackpackStorage, Before.BackpackStorage);
	TestEqual(TEXT("ConsumeNode leaves shop"), After.Shop, Before.Shop);
	TestEqual(TEXT("ConsumeNode leaves economy"), After.Economy, Before.Economy);

	Before = CaptureRunUiRevisions(*Run);
	Run->AdvanceToNextPhase();
	After = CaptureRunUiRevisions(*Run);
	TestEqual(TEXT("AdvanceToNextPhase leaves backpack"), After.BackpackStorage, Before.BackpackStorage);
	TestEqual(TEXT("AdvanceToNextPhase leaves shop"), After.Shop, Before.Shop);
	TestEqual(TEXT("AdvanceToNextPhase leaves economy"), After.Economy, Before.Economy);

	Before = CaptureRunUiRevisions(*Run);
	Run->MarkTriggerDestroyed(TEXT("Revision.Trigger"));
	After = CaptureRunUiRevisions(*Run);
	TestEqual(TEXT("MarkTriggerDestroyed leaves backpack"), After.BackpackStorage, Before.BackpackStorage);
	TestEqual(TEXT("MarkTriggerDestroyed leaves shop"), After.Shop, Before.Shop);
	TestEqual(TEXT("MarkTriggerDestroyed leaves economy"), After.Economy, Before.Economy);

	FWacomRunEventChoiceDefinition CloseChoice;
	CloseChoice.ChoiceId = TEXT("Resolve");
	CloseChoice.bCloseEventAfterResolve = true;
	CloseChoice.bMarkEventCompleted = true;
	TStrongObjectPtr<UWacomRunEventDefinition> Event(MakeSingleChoiceRunEvent(Run.Get(), CloseChoice));
	Before = CaptureRunUiRevisions(*Run);
	TestTrue(TEXT("BeginRunEvent succeeds"), Run->BeginRunEvent(TEXT("Revision.Event.OpenClose"), Event.Get()));
	After = CaptureRunUiRevisions(*Run);
	TestEqual(TEXT("BeginRunEvent leaves backpack"), After.BackpackStorage, Before.BackpackStorage);
	TestEqual(TEXT("BeginRunEvent leaves shop"), After.Shop, Before.Shop);
	TestEqual(TEXT("BeginRunEvent leaves economy"), After.Economy, Before.Economy);

	Before = CaptureRunUiRevisions(*Run);
	const FRunEventChoiceResult Result = Run->ChooseRunEventOptionWithResult(TEXT("Resolve"));
	After = CaptureRunUiRevisions(*Run);
	TestTrue(TEXT("Close/complete RunEvent choice succeeds"), Result.bSucceeded);
	TestEqual(TEXT("Close/complete RunEvent leaves backpack"), After.BackpackStorage, Before.BackpackStorage);
	TestEqual(TEXT("Close/complete RunEvent leaves shop"), After.Shop, Before.Shop);
	TestEqual(TEXT("Close/complete RunEvent leaves economy"), After.Economy, Before.Economy);

	Before = CaptureRunUiRevisions(*Run);
	Run->EndRunEvent();
	After = CaptureRunUiRevisions(*Run);
	TestEqual(TEXT("EndRunEvent leaves backpack"), After.BackpackStorage, Before.BackpackStorage);
	TestEqual(TEXT("EndRunEvent leaves shop"), After.Shop, Before.Shop);
	TestEqual(TEXT("EndRunEvent leaves economy"), After.Economy, Before.Economy);

	return true;
}
