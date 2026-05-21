// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Events/RunEventDefinition.h"
#include "Fixtures/BattleTestFixtures.h"
#include "RunSession.h"
#include "RunState.h"
#include "Tags/WacomGameplayTags.h"

#include "UObject/StrongObjectPtr.h"

namespace
{
	UCardDefinition* MakeRunEventTestTypeAContainer(FWacomBattleFixture& Fx, int32 Capacity)
	{
		UCardDefinition* Card = Fx.MakeNoopCard(0);
		Card->Physique.Capacity = Capacity;
		Card->Rarity = WacomTags::Card_Rarity_White;
		return Card;
	}

	UCardDefinition* MakeRunEventTestTypeBContainer(FWacomBattleFixture& Fx, int32 Capacity)
	{
		UCardDefinition* Card = Fx.MakeNoopCard(0);
		Card->Physique.Capacity = Capacity;
		Card->Physique.CapacityEffect = WacomTags::Card_CapacityEffect_Placeholder;
		Card->Rarity = WacomTags::Card_Rarity_White;
		return Card;
	}

	UWacomRunEventDefinition* MakeRunEventDefinition(UObject* Outer, UCardDefinition* RewardCard)
	{
		UWacomRunEventDefinition* Event = NewObject<UWacomRunEventDefinition>(Outer);
		Event->EventId = TEXT("Event.Test");
		Event->DisplayName = FText::FromString(TEXT("测试事件"));
		Event->StartNodeId = TEXT("Start");

		FWacomRunEventChoiceDefinition Jump;
		Jump.ChoiceId = TEXT("Jump");
		Jump.LabelText = FText::FromString(TEXT("跳转"));
		Jump.NextNodeId = TEXT("End");

		FWacomRunEventChoiceDefinition GoldLocked;
		GoldLocked.ChoiceId = TEXT("GoldLocked");
		GoldLocked.LabelText = FText::FromString(TEXT("金币选项"));
		FWacomRunEventConditionDefinition GoldCondition;
		GoldCondition.Type = EWacomRunEventConditionType::MinGold;
		GoldCondition.Value = 2;
		GoldLocked.Conditions.Add(GoldCondition);

		FWacomRunEventChoiceDefinition NodeLocked;
		NodeLocked.ChoiceId = TEXT("NodeLocked");
		NodeLocked.LabelText = FText::FromString(TEXT("节点选项"));
		FWacomRunEventConditionDefinition NodeCondition;
		NodeCondition.Type = EWacomRunEventConditionType::MinNodeCount;
		NodeCondition.Value = 3;
		NodeLocked.Conditions.Add(NodeCondition);

		FWacomRunEventChoiceDefinition PressureLocked;
		PressureLocked.ChoiceId = TEXT("PressureLocked");
		PressureLocked.LabelText = FText::FromString(TEXT("压力选项"));
		FWacomRunEventConditionDefinition PressureCondition;
		PressureCondition.Type = EWacomRunEventConditionType::MaxPressure;
		PressureCondition.PressureType = TEXT("Misdeed");
		PressureCondition.Value = 0;
		PressureLocked.Conditions.Add(PressureCondition);

		FWacomRunEventNodeDefinition Start;
		Start.NodeId = TEXT("Start");
		Start.TitleText = FText::FromString(TEXT("起点"));
		Start.BodyText = FText::FromString(TEXT("开始正文"));
		Start.Choices = { Jump, GoldLocked, NodeLocked, PressureLocked };

		FWacomRunEventChoiceDefinition Resolve;
		Resolve.ChoiceId = TEXT("Resolve");
		Resolve.LabelText = FText::FromString(TEXT("结算"));
		FWacomRunEventEffectDefinition GainCard;
		GainCard.Type = EWacomRunEventEffectType::GainCard;
		GainCard.CardDefinition = RewardCard;
		FWacomRunEventEffectDefinition Gold;
		Gold.Type = EWacomRunEventEffectType::AddGold;
		Gold.Value = 3;
		FWacomRunEventEffectDefinition Pressure;
		Pressure.Type = EWacomRunEventEffectType::AddPressure;
		Pressure.PressureType = TEXT("Misdeed");
		Pressure.Value = 4;
		FWacomRunEventEffectDefinition Node;
		Node.Type = EWacomRunEventEffectType::ConsumeNode;
		Node.Value = 1;
		Resolve.Effects = { GainCard, Gold, Pressure, Node };
		Resolve.bMarkEventCompleted = true;
		Resolve.bCloseEventAfterResolve = true;

		FWacomRunEventNodeDefinition End;
		End.NodeId = TEXT("End");
		End.TitleText = FText::FromString(TEXT("终点"));
		End.BodyText = FText::FromString(TEXT("结束正文"));
		End.Choices = { Resolve };

		Event->Nodes = { Start, End };
		return Event;
	}

	UWacomRunEventDefinition* MakeSingleChoiceRunEvent(UObject* Outer, const FWacomRunEventChoiceDefinition& Choice)
	{
		UWacomRunEventDefinition* Event = NewObject<UWacomRunEventDefinition>(Outer);
		Event->EventId = TEXT("Event.SingleChoice");
		Event->DisplayName = FText::FromString(TEXT("单选事件"));
		Event->StartNodeId = TEXT("Start");

		FWacomRunEventNodeDefinition Start;
		Start.NodeId = TEXT("Start");
		Start.Choices.Add(Choice);
		Event->Nodes.Add(Start);
		return Event;
	}

	bool StorageContainsDefinition(const FRunBackpackStorageSnapshot& Snapshot, const UCardDefinition* Card)
	{
		for (const FRunStorageCardView& View : Snapshot.Flux.ContentCards)
		{
			if (View.Instance.Definition.Get() == Card)
			{
				return true;
			}
		}
		for (const FRunStorageCardView& View : Snapshot.BattleDeckPhysicalCards)
		{
			if (View.Instance.Definition.Get() == Card)
			{
				return true;
			}
		}
		for (const FRunStorageCardView& View : Snapshot.BurdenCards)
		{
			if (View.Instance.Definition.Get() == Card)
			{
				return true;
			}
		}
		for (const FRunSpecialStorageView& Special : Snapshot.SpecialZones)
		{
			if (Special.OwnerCard.Instance.Definition.Get() == Card)
			{
				return true;
			}
			for (const FRunStorageCardView& View : Special.ContentCards)
			{
				if (View.Instance.Definition.Get() == Card)
				{
					return true;
				}
			}
		}
		return false;
	}

	FGuid FindStorageInstanceIdByDefinition(const FRunBackpackStorageSnapshot& Snapshot, const UCardDefinition* Card)
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

	bool FindStorageZoneByDefinition(const FRunBackpackStorageSnapshot& Snapshot, const UCardDefinition* Card, EZoneKind& OutZone)
	{
		for (const FRunStorageCardView& View : Snapshot.Flux.ContentCards)
		{
			if (View.Instance.Definition.Get() == Card)
			{
				OutZone = EZoneKind::Backpack;
				return true;
			}
		}
		for (const FRunStorageCardView& View : Snapshot.BattleDeckPhysicalCards)
		{
			if (View.Instance.Definition.Get() == Card)
			{
				OutZone = EZoneKind::BattleDeck;
				return true;
			}
		}
		for (const FRunStorageCardView& View : Snapshot.BurdenCards)
		{
			if (View.Instance.Definition.Get() == Card)
			{
				OutZone = EZoneKind::BurdenZone;
				return true;
			}
		}
		for (const FRunSpecialStorageView& Special : Snapshot.SpecialZones)
		{
			if (Special.OwnerCard.Instance.Definition.Get() == Card)
			{
				OutZone = Special.bOwnerInBattleDeck ? EZoneKind::BattleDeck : EZoneKind::Backpack;
				return true;
			}
			for (const FRunStorageCardView& View : Special.ContentCards)
			{
				if (View.Instance.Definition.Get() == Card)
				{
					OutZone = EZoneKind::SpecialZone;
					return true;
				}
			}
		}
		return false;
	}

	UWacomRunEventDefinition* MakeRemoveCardRunEvent(UObject* Outer, UCardDefinition* Card)
	{
		FWacomRunEventChoiceDefinition RemoveChoice;
		RemoveChoice.ChoiceId = TEXT("Remove");
		FWacomRunEventEffectDefinition Remove;
		Remove.Type = EWacomRunEventEffectType::RemoveCard;
		Remove.CardDefinition = Card;
		RemoveChoice.Effects.Add(Remove);
		return MakeSingleChoiceRunEvent(Outer, RemoveChoice);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunEventGraphFlowSpec,
	"Wacom.Run.Event.GraphFlow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunEventGraphFlowSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Reward = Fx.MakeNoopCard(0);
	Reward->DisplayName = FText::FromString(TEXT("事件奖励卡"));
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TStrongObjectPtr<UWacomRunEventDefinition> Event(MakeRunEventDefinition(Run.Get(), Reward));

	TestTrue(TEXT("Begin event succeeds"), Run->BeginRunEvent(TEXT("Event.Actor.1"), Event.Get()));
	FRunEventSnapshot Snapshot = Run->BuildCurrentRunEventSnapshot();
	TestTrue(TEXT("Snapshot active"), Snapshot.bIsActive);
	TestEqual(TEXT("Starts at start node"), Snapshot.CurrentNodeId, FName(TEXT("Start")));
	TestEqual(TEXT("Start title"), Snapshot.TitleText.ToString(), FString(TEXT("起点")));

	TestTrue(TEXT("Jump choice succeeds"), Run->ChooseRunEventOption(TEXT("Jump")));
	Snapshot = Run->BuildCurrentRunEventSnapshot();
	TestEqual(TEXT("Jump reaches end node"), Snapshot.CurrentNodeId, FName(TEXT("End")));

	const int32 NodesBefore = Run->GetRemainingNodeCount();
	const FRunEventChoiceResult Result = Run->ChooseRunEventOptionWithResult(TEXT("Resolve"));
	TestTrue(TEXT("Resolve succeeds"), Result.bSucceeded);
	TestEqual(TEXT("Resolve records choice id"), Result.ChoiceId, FName(TEXT("Resolve")));
	TestEqual(TEXT("Resolve records four effect results"), Result.EffectResults.Num(), 4);
	if (Result.EffectResults.Num() == 4)
	{
		TestTrue(TEXT("Gain card result applied"), Result.EffectResults[0].bApplied);
		TestTrue(TEXT("Gain card records card"), Result.EffectResults[0].CardDefinition.Get() == Reward);
		TestEqual(TEXT("Gold result actual delta"), Result.EffectResults[1].ActualDelta, 3);
		TestEqual(TEXT("Pressure result actual delta"), Result.EffectResults[2].ActualDelta, 4);
		TestEqual(TEXT("Pressure result type"), Result.EffectResults[2].PressureType, EWacomPressureType::Misdeed);
		TestEqual(TEXT("Node result actual consumed"), Result.EffectResults[3].ActualDelta, -1);
	}
	TestFalse(TEXT("Event closed after resolve"), Run->IsRunEventActive());
	TestTrue(TEXT("Event marked completed"), Run->IsRunEventCompleted(TEXT("Event.Actor.1")));
	TestFalse(TEXT("Completed event cannot reopen"), Run->BeginRunEvent(TEXT("Event.Actor.1"), Event.Get()));
	const FRunBackpackStorageSnapshot StorageSnapshot = Run->BuildBackpackStorageSnapshot();
	TestTrue(TEXT("Reward card acquired"),
		StorageSnapshot.Flux.ContentCards.ContainsByPredicate([Reward](const FRunStorageCardView& CardView)
		{
			return CardView.Instance.Definition.Get() == Reward;
		})
		|| StorageSnapshot.BurdenCards.ContainsByPredicate([Reward](const FRunStorageCardView& CardView)
		{
			return CardView.Instance.Definition.Get() == Reward;
		}));
	TestEqual(TEXT("Gold gained"), Run->GetGold(), 3);
	TestEqual(TEXT("Misdeed pressure gained"), Run->GetPressureValue(EWacomPressureType::Misdeed), 4);
	TestEqual(TEXT("Node consumed"), Run->GetRemainingNodeCount(), NodesBefore - 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunEventCardAndEventStateConditionsSpec,
	"Wacom.Run.Event.CardAndEventStateConditions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunEventCardAndEventStateConditionsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Bag = MakeRunEventTestTypeAContainer(Fx, 8);
	UCardDefinition* BackpackCard = Fx.MakeNoopCard(0);
	UCardDefinition* BattleDeckCard = Fx.MakeNoopCard(0);
	UCardDefinition* BurdenCard = Fx.MakeNoopCard(0);
	UCardDefinition* TypeB = MakeRunEventTestTypeBContainer(Fx, 3);
	UCardDefinition* SpecialCard = Fx.MakeNoopCard(0);
	UCardDefinition* MissingCard = Fx.MakeNoopCard(0);

	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1),
		Fx.MakeNoopCard(1),
		{ Bag, BackpackCard, BattleDeckCard, TypeB });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Initialize"), Run->Initialize(Char));

	TestTrue(TEXT("Move backpack card out of battle deck"), Run->RemoveCardFromBattleDeck(BackpackCard));
	Run->AddCardToBackpack(BurdenCard);
	FRunBackpackStorageSnapshot Snapshot = Run->BuildBackpackStorageSnapshot();
	const FGuid BurdenInstanceId = FindStorageInstanceIdByDefinition(Snapshot, BurdenCard);
	TestTrue(TEXT("Burden card instance found before move"), BurdenInstanceId.IsValid());
	TestTrue(TEXT("Move card to burden"), Run->MoveInstance(BurdenInstanceId, EZoneKind::BurdenZone, FGuid()));

	Snapshot = Run->BuildBackpackStorageSnapshot();
	TestTrue(TEXT("Has special owner"), Snapshot.SpecialZones.Num() > 0);
	const FGuid SpecialOwnerId = Snapshot.SpecialZones[0].OwnerCard.Instance.InstanceId;
	Run->AddCardToBackpack(SpecialCard);
	Snapshot = Run->BuildBackpackStorageSnapshot();
	const FGuid SpecialCardId = FindStorageInstanceIdByDefinition(Snapshot, SpecialCard);
	TestTrue(TEXT("Special card instance found before move"), SpecialCardId.IsValid());
	TestTrue(TEXT("Move card to special zone"), Run->MoveInstance(SpecialCardId, EZoneKind::SpecialZone, SpecialOwnerId));
	Snapshot = Run->BuildBackpackStorageSnapshot();
	const FGuid BurdenAfterRefillInstanceId = FindStorageInstanceIdByDefinition(Snapshot, BurdenCard);
	TestTrue(TEXT("Burden card instance found after refill"), BurdenAfterRefillInstanceId.IsValid());
	TestTrue(TEXT("Move card back to burden after refill"), Run->MoveInstance(BurdenAfterRefillInstanceId, EZoneKind::BurdenZone, FGuid()));

	FWacomRunEventChoiceDefinition Probe;
	Probe.ChoiceId = TEXT("Probe");
	auto AddHasCardCondition = [&Probe](UCardDefinition* Card)
	{
		FWacomRunEventConditionDefinition Condition;
		Condition.Type = EWacomRunEventConditionType::HasCard;
		Condition.CardDefinition = Card;
		Probe.Conditions.Add(Condition);
	};
	AddHasCardCondition(BackpackCard);
	AddHasCardCondition(BattleDeckCard);
	AddHasCardCondition(BurdenCard);
	AddHasCardCondition(SpecialCard);
	FWacomRunEventConditionDefinition MissingCondition;
	MissingCondition.Type = EWacomRunEventConditionType::MissingCard;
	MissingCondition.CardDefinition = MissingCard;
	Probe.Conditions.Add(MissingCondition);
	FWacomRunEventConditionDefinition EventNotDone;
	EventNotDone.Type = EWacomRunEventConditionType::EventNotCompleted;
	EventNotDone.TargetPersistentId = TEXT("Event.Dependency");
	Probe.Conditions.Add(EventNotDone);
	FWacomRunEventEffectDefinition MarkDependency;
	MarkDependency.Type = EWacomRunEventEffectType::MarkEventCompleted;
	MarkDependency.TargetPersistentId = TEXT("Event.Dependency");
	Probe.Effects.Add(MarkDependency);

	TStrongObjectPtr<UWacomRunEventDefinition> Event(MakeSingleChoiceRunEvent(Run.Get(), Probe));
	TestTrue(TEXT("Begin event succeeds"), Run->BeginRunEvent(TEXT("Event.CardConditions"), Event.Get()));
	Snapshot = FRunBackpackStorageSnapshot();
	FRunEventSnapshot EventSnapshot = Run->BuildCurrentRunEventSnapshot();
	TestTrue(TEXT("Probe exists"), EventSnapshot.Choices.Num() == 1);
	if (EventSnapshot.Choices.Num() == 1)
	{
		TestTrue(TEXT("All card zone conditions available"), EventSnapshot.Choices[0].bAvailable);
	}

	const FRunEventChoiceResult Result = Run->ChooseRunEventOptionWithResult(TEXT("Probe"));
	TestTrue(TEXT("Probe succeeds"), Result.bSucceeded);
	TestTrue(TEXT("Dependency marked completed"), Run->IsRunEventCompleted(TEXT("Event.Dependency")));
	TestEqual(TEXT("Mark effect recorded"), Result.EffectResults.Num(), 1);
	if (Result.EffectResults.Num() == 1)
	{
		TestEqual(TEXT("Mark effect type"), Result.EffectResults[0].EffectType, EWacomRunEventEffectType::MarkEventCompleted);
		TestEqual(TEXT("Mark effect delta"), Result.EffectResults[0].ActualDelta, 1);
	}

	FWacomRunEventChoiceDefinition Blocked;
	Blocked.ChoiceId = TEXT("Blocked");
	FWacomRunEventConditionDefinition RequiresNotDone;
	RequiresNotDone.Type = EWacomRunEventConditionType::EventNotCompleted;
	RequiresNotDone.TargetPersistentId = TEXT("Event.Dependency");
	Blocked.Conditions.Add(RequiresNotDone);
	TStrongObjectPtr<UWacomRunEventDefinition> BlockedEvent(MakeSingleChoiceRunEvent(Run.Get(), Blocked));
	TestTrue(TEXT("Begin second event succeeds"), Run->BeginRunEvent(TEXT("Event.CardConditions.2"), BlockedEvent.Get()));
	EventSnapshot = Run->BuildCurrentRunEventSnapshot();
	if (EventSnapshot.Choices.Num() == 1)
	{
		TestFalse(TEXT("Completed dependency blocks EventNotCompleted"), EventSnapshot.Choices[0].bAvailable);
		TestEqual(TEXT("EventNotCompleted disabled reason"), EventSnapshot.Choices[0].DisabledReason, FName(TEXT("RequiredEventAlreadyCompleted")));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunEventRemoveCardEffectSpec,
	"Wacom.Run.Event.RemoveCardEffect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunEventRemoveCardEffectSpec::RunTest(const FString& /*Parameters*/)
{
	auto ExecuteRemove = [this](URunSession* Run, UCardDefinition* Card, const TCHAR* EventId) -> FRunEventChoiceResult
	{
		UWacomRunEventDefinition* Event = MakeRemoveCardRunEvent(Run, Card);
		TestTrue(TEXT("Begin remove event"), Run->BeginRunEvent(FName(EventId), Event));
		return Run->ChooseRunEventOptionWithResult(TEXT("Remove"));
	};

	{
		FWacomBattleFixture Fx;
		UCardDefinition* Bag = MakeRunEventTestTypeAContainer(Fx, 8);
		UCardDefinition* BackpackCard = Fx.MakeNoopCard(0);
		UCharacterDefinition* Char = Fx.MakeCharacter(Fx.MakeNoopCard(1), Fx.MakeNoopCard(1), { Bag, BackpackCard });
		TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
		TestTrue(TEXT("Initialize backpack remove"), Run->Initialize(Char));
		TestTrue(TEXT("Move card to backpack"), Run->RemoveCardFromBattleDeck(BackpackCard));
		EZoneKind Zone = EZoneKind::BattleDeck;
		TestTrue(TEXT("Backpack card zone found"), FindStorageZoneByDefinition(Run->BuildBackpackStorageSnapshot(), BackpackCard, Zone));
		TestEqual(TEXT("Backpack card starts in backpack"), Zone, EZoneKind::Backpack);

		const FRunEventChoiceResult Result = ExecuteRemove(Run.Get(), BackpackCard, TEXT("Event.Remove.Backpack"));
		TestTrue(TEXT("Backpack remove succeeds"), Result.bSucceeded);
		TestEqual(TEXT("One remove effect recorded"), Result.EffectResults.Num(), 1);
		if (Result.EffectResults.Num() == 1)
		{
			TestEqual(TEXT("Remove effect type"), Result.EffectResults[0].EffectType, EWacomRunEventEffectType::RemoveCard);
			TestEqual(TEXT("Remove effect delta"), Result.EffectResults[0].ActualDelta, -1);
			TestTrue(TEXT("Remove effect applied"), Result.EffectResults[0].bApplied);
		}
		TestFalse(TEXT("Backpack card removed"), StorageContainsDefinition(Run->BuildBackpackStorageSnapshot(), BackpackCard));
	}

	{
		FWacomBattleFixture Fx;
		UCardDefinition* Bag = MakeRunEventTestTypeAContainer(Fx, 8);
		UCardDefinition* BattleDeckCard = Fx.MakeNoopCard(0);
		UCharacterDefinition* Char = Fx.MakeCharacter(Fx.MakeNoopCard(1), Fx.MakeNoopCard(1), { Bag, BattleDeckCard });
		TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
		TestTrue(TEXT("Initialize battle deck remove"), Run->Initialize(Char));
		EZoneKind Zone = EZoneKind::Backpack;
		TestTrue(TEXT("Battle deck card zone found"), FindStorageZoneByDefinition(Run->BuildBackpackStorageSnapshot(), BattleDeckCard, Zone));
		TestEqual(TEXT("Battle deck card starts in battle deck"), Zone, EZoneKind::BattleDeck);

		const FRunEventChoiceResult Result = ExecuteRemove(Run.Get(), BattleDeckCard, TEXT("Event.Remove.BattleDeck"));
		TestTrue(TEXT("Battle deck remove succeeds"), Result.bSucceeded);
		TestFalse(TEXT("Battle deck card removed"), StorageContainsDefinition(Run->BuildBackpackStorageSnapshot(), BattleDeckCard));
	}

	{
		FWacomBattleFixture Fx;
		UCardDefinition* Bag = MakeRunEventTestTypeAContainer(Fx, 8);
		UCardDefinition* BurdenCard = Fx.MakeNoopCard(0);
		UCharacterDefinition* Char = Fx.MakeCharacter(Fx.MakeNoopCard(1), Fx.MakeNoopCard(1), { Bag });
		TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
		TestTrue(TEXT("Initialize burden remove"), Run->Initialize(Char));
		Run->AddCardToBackpack(BurdenCard);
		FRunBackpackStorageSnapshot Snapshot = Run->BuildBackpackStorageSnapshot();
		const FGuid BurdenInstanceId = FindStorageInstanceIdByDefinition(Snapshot, BurdenCard);
		TestTrue(TEXT("Burden card instance found"), BurdenInstanceId.IsValid());
		TestTrue(TEXT("Move card to burden"), Run->MoveInstance(BurdenInstanceId, EZoneKind::BurdenZone, FGuid()));
		EZoneKind Zone = EZoneKind::Backpack;
		TestTrue(TEXT("Burden card zone found"), FindStorageZoneByDefinition(Run->BuildBackpackStorageSnapshot(), BurdenCard, Zone));
		TestEqual(TEXT("Burden card starts in burden"), Zone, EZoneKind::BurdenZone);

		const FRunEventChoiceResult Result = ExecuteRemove(Run.Get(), BurdenCard, TEXT("Event.Remove.Burden"));
		TestTrue(TEXT("Burden remove succeeds"), Result.bSucceeded);
		TestFalse(TEXT("Burden card removed"), StorageContainsDefinition(Run->BuildBackpackStorageSnapshot(), BurdenCard));
	}

	{
		FWacomBattleFixture Fx;
		UCardDefinition* Bag = MakeRunEventTestTypeAContainer(Fx, 8);
		UCardDefinition* TypeB = MakeRunEventTestTypeBContainer(Fx, 3);
		UCardDefinition* SpecialCard = Fx.MakeNoopCard(0);
		UCharacterDefinition* Char = Fx.MakeCharacter(Fx.MakeNoopCard(1), Fx.MakeNoopCard(1), { Bag, TypeB });
		TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
		TestTrue(TEXT("Initialize special remove"), Run->Initialize(Char));
		FRunBackpackStorageSnapshot Snapshot = Run->BuildBackpackStorageSnapshot();
		TestTrue(TEXT("Has special owner"), Snapshot.SpecialZones.Num() > 0);
		const FGuid SpecialOwnerId = Snapshot.SpecialZones[0].OwnerCard.Instance.InstanceId;
		Run->AddCardToBackpack(SpecialCard);
		Snapshot = Run->BuildBackpackStorageSnapshot();
		const FGuid SpecialCardId = FindStorageInstanceIdByDefinition(Snapshot, SpecialCard);
		TestTrue(TEXT("Special card instance found"), SpecialCardId.IsValid());
		TestTrue(TEXT("Move card to special zone"), Run->MoveInstance(SpecialCardId, EZoneKind::SpecialZone, SpecialOwnerId));
		EZoneKind Zone = EZoneKind::Backpack;
		TestTrue(TEXT("Special card zone found"), FindStorageZoneByDefinition(Run->BuildBackpackStorageSnapshot(), SpecialCard, Zone));
		TestEqual(TEXT("Special card starts in special"), Zone, EZoneKind::SpecialZone);

		const FRunEventChoiceResult Result = ExecuteRemove(Run.Get(), SpecialCard, TEXT("Event.Remove.Special"));
		TestTrue(TEXT("Special remove succeeds"), Result.bSucceeded);
		TestFalse(TEXT("Special card removed"), StorageContainsDefinition(Run->BuildBackpackStorageSnapshot(), SpecialCard));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunEventRemoveCardSafetySpec,
	"Wacom.Run.Event.RemoveCardSafety",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunEventRemoveCardSafetySpec::RunTest(const FString& /*Parameters*/)
{
	{
		FWacomBattleFixture Fx;
		UCardDefinition* OnlyBag = MakeRunEventTestTypeAContainer(Fx, 3);
		UCharacterDefinition* Char = Fx.MakeCharacter(Fx.MakeNoopCard(1), Fx.MakeNoopCard(1), { OnlyBag });
		TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
		TestTrue(TEXT("Initialize only bag"), Run->Initialize(Char));

		FWacomRunEventChoiceDefinition RemoveBag;
		RemoveBag.ChoiceId = TEXT("RemoveBag");
		FWacomRunEventEffectDefinition Effect;
		Effect.Type = EWacomRunEventEffectType::RemoveCard;
		Effect.CardDefinition = OnlyBag;
		RemoveBag.Effects.Add(Effect);
		TStrongObjectPtr<UWacomRunEventDefinition> Event(MakeSingleChoiceRunEvent(Run.Get(), RemoveBag));
		TestTrue(TEXT("Begin only bag event"), Run->BeginRunEvent(TEXT("Event.Safety.Bag"), Event.Get()));
		const FRunEventChoiceResult Result = Run->ChooseRunEventOptionWithResult(TEXT("RemoveBag"));
		TestFalse(TEXT("Last capacity provider rejected"), Result.bSucceeded);
		TestEqual(TEXT("Last capacity provider reason"), Result.DisabledReason, FName(TEXT("LastCapacityProvider")));
		TestTrue(TEXT("Only bag remains"), StorageContainsDefinition(Run->BuildBackpackStorageSnapshot(), OnlyBag));
	}

	{
		FWacomBattleFixture Fx;
		UCardDefinition* Bag = MakeRunEventTestTypeAContainer(Fx, 3);
		UCardDefinition* Intrinsic = Fx.MakeNoopCard(0);
		Intrinsic->Rarity = WacomTags::Card_Rarity_Intrinsic;
		UCharacterDefinition* Char = Fx.MakeCharacter(Fx.MakeNoopCard(1), Fx.MakeNoopCard(1), { Bag, Intrinsic });
		TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
		TestTrue(TEXT("Initialize intrinsic"), Run->Initialize(Char));

		FWacomRunEventChoiceDefinition RemoveIntrinsic;
		RemoveIntrinsic.ChoiceId = TEXT("RemoveIntrinsic");
		FWacomRunEventEffectDefinition Effect;
		Effect.Type = EWacomRunEventEffectType::RemoveCard;
		Effect.CardDefinition = Intrinsic;
		RemoveIntrinsic.Effects.Add(Effect);
		TStrongObjectPtr<UWacomRunEventDefinition> Event(MakeSingleChoiceRunEvent(Run.Get(), RemoveIntrinsic));
		TestTrue(TEXT("Begin intrinsic event"), Run->BeginRunEvent(TEXT("Event.Safety.Intrinsic"), Event.Get()));
		const FRunEventChoiceResult Result = Run->ChooseRunEventOptionWithResult(TEXT("RemoveIntrinsic"));
		TestFalse(TEXT("Intrinsic rejected"), Result.bSucceeded);
		TestEqual(TEXT("Intrinsic reason"), Result.DisabledReason, FName(TEXT("ProtectedCard")));
		TestTrue(TEXT("Intrinsic remains"), StorageContainsDefinition(Run->BuildBackpackStorageSnapshot(), Intrinsic));
	}

	{
		FWacomBattleFixture Fx;
		UCardDefinition* Bag = MakeRunEventTestTypeAContainer(Fx, 3);
		UCardDefinition* Companion = Fx.MakeNoopCard(0);
		Companion->Keywords.AddTag(WacomTags::Card_Keyword_Companion);
		UCharacterDefinition* Char = Fx.MakeCharacter(Fx.MakeNoopCard(1), Fx.MakeNoopCard(1), { Bag, Companion });
		TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
		TestTrue(TEXT("Initialize companion"), Run->Initialize(Char));

		FWacomRunEventChoiceDefinition RemoveCompanion;
		RemoveCompanion.ChoiceId = TEXT("RemoveCompanion");
		FWacomRunEventEffectDefinition Effect;
		Effect.Type = EWacomRunEventEffectType::RemoveCard;
		Effect.CardDefinition = Companion;
		RemoveCompanion.Effects.Add(Effect);
		TStrongObjectPtr<UWacomRunEventDefinition> Event(MakeSingleChoiceRunEvent(Run.Get(), RemoveCompanion));
		TestTrue(TEXT("Begin companion event"), Run->BeginRunEvent(TEXT("Event.Safety.Companion"), Event.Get()));
		const FRunEventChoiceResult Result = Run->ChooseRunEventOptionWithResult(TEXT("RemoveCompanion"));
		TestTrue(TEXT("Companion remove succeeds"), Result.bSucceeded);
		TestEqual(TEXT("Bloodlust increments"), Run->GetPressureValue(EWacomPressureType::Bloodlust), 1);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunEventChoiceResultClampSpec,
	"Wacom.Run.Event.ChoiceResultActualDeltaClamp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunEventChoiceResultClampSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TStrongObjectPtr<UWacomRunEventDefinition> Event(NewObject<UWacomRunEventDefinition>(Run.Get()));
	Event->EventId = TEXT("Event.Clamp");
	Event->DisplayName = FText::FromString(TEXT("Clamp事件"));
	Event->StartNodeId = TEXT("Start");

	FWacomRunEventChoiceDefinition ClampChoice;
	ClampChoice.ChoiceId = TEXT("Clamp");
	FWacomRunEventEffectDefinition LoseGold;
	LoseGold.Type = EWacomRunEventEffectType::AddGold;
	LoseGold.Value = -5;
	FWacomRunEventEffectDefinition AddPressure;
	AddPressure.Type = EWacomRunEventEffectType::AddPressure;
	AddPressure.PressureType = TEXT("Wound");
	AddPressure.Value = 10;
	ClampChoice.Effects = { LoseGold, AddPressure };

	FWacomRunEventNodeDefinition Start;
	Start.NodeId = TEXT("Start");
	Start.Choices = { ClampChoice };
	Event->Nodes = { Start };

	Run->AddGold(2);
	Run->AddPressure(EWacomPressureType::Wound, 95);
	TestTrue(TEXT("Begin event succeeds"), Run->BeginRunEvent(TEXT("Event.Clamp.Actor"), Event.Get()));
	const FRunEventChoiceResult Result = Run->ChooseRunEventOptionWithResult(TEXT("Clamp"));
	TestTrue(TEXT("Clamp choice succeeds"), Result.bSucceeded);
	TestEqual(TEXT("Two effect results"), Result.EffectResults.Num(), 2);
	if (Result.EffectResults.Num() == 2)
	{
		TestEqual(TEXT("Gold actual delta clamps to available gold"), Result.EffectResults[0].ActualDelta, -2);
		TestEqual(TEXT("Pressure actual delta clamps to 100"), Result.EffectResults[1].ActualDelta, 5);
	}
	TestEqual(TEXT("Gold reaches zero"), Run->GetGold(), 0);
	TestEqual(TEXT("Wound reaches 100"), Run->GetPressureValue(EWacomPressureType::Wound), 100);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunEventConditionsSpec,
	"Wacom.Run.Event.Conditions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunEventConditionsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TStrongObjectPtr<UWacomRunEventDefinition> Event(MakeRunEventDefinition(Run.Get(), Fx.MakeNoopCard(0)));

	TestTrue(TEXT("Begin event succeeds"), Run->BeginRunEvent(TEXT("Event.Actor.2"), Event.Get()));
	FRunEventSnapshot Snapshot = Run->BuildCurrentRunEventSnapshot();
	const FRunEventChoiceSnapshot* GoldLocked = Snapshot.Choices.FindByPredicate(
		[](const FRunEventChoiceSnapshot& Choice) { return Choice.ChoiceId == TEXT("GoldLocked"); });
	const FRunEventChoiceSnapshot* NodeLocked = Snapshot.Choices.FindByPredicate(
		[](const FRunEventChoiceSnapshot& Choice) { return Choice.ChoiceId == TEXT("NodeLocked"); });
	const FRunEventChoiceSnapshot* PressureLocked = Snapshot.Choices.FindByPredicate(
		[](const FRunEventChoiceSnapshot& Choice) { return Choice.ChoiceId == TEXT("PressureLocked"); });

	if (!TestNotNull(TEXT("Gold locked choice exists"), GoldLocked)
		|| !TestNotNull(TEXT("Node locked choice exists"), NodeLocked)
		|| !TestNotNull(TEXT("Pressure locked choice exists"), PressureLocked))
	{
		return false;
	}

	TestFalse(TEXT("Gold condition disabled"), GoldLocked->bAvailable);
	TestEqual(TEXT("Gold disabled reason"), GoldLocked->DisabledReason, FName(TEXT("InsufficientGold")));
	const int32 GoldBeforeRejectedChoice = Run->GetGold();
	const FRunEventChoiceResult RejectedResult = Run->ChooseRunEventOptionWithResult(TEXT("GoldLocked"));
	TestFalse(TEXT("Rejected choice result fails"), RejectedResult.bSucceeded);
	TestEqual(TEXT("Rejected choice reason"), RejectedResult.DisabledReason, FName(TEXT("InsufficientGold")));
	TestEqual(TEXT("Rejected choice does not change gold"), Run->GetGold(), GoldBeforeRejectedChoice);
	TestFalse(TEXT("Node condition disabled"), NodeLocked->bAvailable);
	TestEqual(TEXT("Node disabled reason"), NodeLocked->DisabledReason, FName(TEXT("InsufficientNode")));
	TestTrue(TEXT("Pressure condition initially available"), PressureLocked->bAvailable);

	Run->AddGold(2);
	Run->AddPressure(EWacomPressureType::Misdeed, 1);
	Snapshot = Run->BuildCurrentRunEventSnapshot();
	GoldLocked = Snapshot.Choices.FindByPredicate(
		[](const FRunEventChoiceSnapshot& Choice) { return Choice.ChoiceId == TEXT("GoldLocked"); });
	PressureLocked = Snapshot.Choices.FindByPredicate(
		[](const FRunEventChoiceSnapshot& Choice) { return Choice.ChoiceId == TEXT("PressureLocked"); });
	TestTrue(TEXT("Gold condition becomes available"), GoldLocked && GoldLocked->bAvailable);
	TestFalse(TEXT("Pressure condition becomes disabled"), PressureLocked && PressureLocked->bAvailable);
	if (PressureLocked)
	{
		TestEqual(TEXT("Pressure disabled reason"), PressureLocked->DisabledReason, FName(TEXT("PressureTooHigh")));
	}

	return true;
}
