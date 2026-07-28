// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "RunState.h"
#include "Session/BattleResultPacket.h"
#include "Tags/WacomGameplayTags.h"
#include "Testing/WacomRunBattleSettlementAutomationTestView.h"

namespace WacomRunCardPersistentMutationSpec
{
	FGuid AddTrackedCard(FRunState& State)
	{
		FCardInstance Card;
		Card.InstanceId = FGuid::NewGuid();
		State.BattleDeck.Add(Card);
		return Card.InstanceId;
	}

	FBattlePersistentCardMutation MakeMutation(
		const FGuid& SourceRunInstanceId,
		const int32 DurabilityBonus)
	{
		FBattlePersistentCardMutation Mutation;
		Mutation.SourceRunInstanceId = SourceRunInstanceId;
		Mutation.PersistentModifiers.DurabilityBonus = DurabilityBonus;
		Mutation.PersistentModifiers.EffectMagnitudeBonuses.Add(
			WacomTags::Effect_ApplyStatus_Burn,
			DurabilityBonus);
		return Mutation;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCardPersistentMutationVictorySpec,
	"Wacom.Run.CardPersistentMutation.VictoryAndWithdrawApplyOnce",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

bool FWacomRunCardPersistentMutationVictorySpec::RunTest(
	const FString&)
{
	using namespace WacomRunCardPersistentMutationSpec;

	for (const bool bWithdrawn : { false, true })
	{
		FRunState State;
		const FGuid TrackedId = AddTrackedCard(State);
		FBattleResultPacket Packet;
		Packet.Outcome = EBattleOutcome::Victory;
		Packet.bWithdrawn = bWithdrawn;
		Packet.PersistentCardMutations = {
			MakeMutation(TrackedId, 1),
			MakeMutation(TrackedId, 2),
			MakeMutation(FGuid(), 99),
		};
		TestTrue(TEXT("Victory settlement succeeds"),
			FWacomRunBattleSettlementAutomationTestView::Resolve(
				State,
				Packet));
		TestEqual(
			TEXT("Duplicate mutation is applied once using stable packet order"),
			State.BattleDeck[0].PersistentModifiers.DurabilityBonus,
			1);
		TestEqual(TEXT("Persistent Burn growth travels with the same mutation"),
			State.BattleDeck[0].PersistentModifiers.EffectMagnitudeBonuses.FindRef(
				WacomTags::Effect_ApplyStatus_Burn),
			1);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCardPersistentMutationDefeatSpec,
	"Wacom.Run.CardPersistentMutation.DefeatDoesNotApply",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

bool FWacomRunCardPersistentMutationDefeatSpec::RunTest(
	const FString&)
{
	using namespace WacomRunCardPersistentMutationSpec;

	FRunState State;
	const FGuid TrackedId = AddTrackedCard(State);
	FBattleResultPacket Packet;
	Packet.Outcome = EBattleOutcome::Defeat;
	Packet.PersistentCardMutations = {
		MakeMutation(TrackedId, 4),
	};
	TestTrue(TEXT("Defeat settlement still completes"),
		FWacomRunBattleSettlementAutomationTestView::Resolve(
			State,
			Packet));
	TestEqual(TEXT("Defeat ignores persistent battle mutations"),
		State.BattleDeck[0].PersistentModifiers.DurabilityBonus,
		0);
	TestEqual(TEXT("Defeat also ignores persistent effect growth"),
		State.BattleDeck[0].PersistentModifiers.EffectMagnitudeBonuses.FindRef(
			WacomTags::Effect_ApplyStatus_Burn),
		0);
	return true;
}
