// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Commands/BattleCommand.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Events/BattleEvent.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Session/BattleResultPacket.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"

namespace WacomKnockdownBranchRewardSpec
{
	struct FPendingChoiceFixture
	{
		FWacomBattleFixture Battle;
		UCardDefinition* AidReward = nullptr;
		UCardDefinition* DestroyReward = nullptr;
		UCardDefinition* LegacyReward = nullptr;
		UEnemyPartDefinition* TargetPart = nullptr;
		UBattleSession* Session = nullptr;
		FGuid TargetPartInstanceId;

		void Initialize(bool bUseExplicitRewards, bool bUseLegacyReward)
		{
			UCardDefinition* LeftHand = Battle.MakeNoopCard(0);
			UCardDefinition* Killer = Battle.MakeSimpleDamageCard(0, 100);
			TArray<UCardDefinition*> Deck;
			for (int32 Index = 0; Index < 5; ++Index)
			{
				Deck.Add(Battle.MakeNoopCard(0));
			}
			UCharacterDefinition* Character =
				Battle.MakeCharacter(LeftHand, Killer, Deck);

			UEnemyDefinition* Enemy =
				Battle.MakeThreePartEnemy(50, 50, 50, 7, 7, 7);
			TargetPart = Enemy->Parts[0].PartDef;

			AidReward = Battle.MakeNoopCard(0);
			AidReward->CardId = TEXT("Reward.Test.Branch.Aid");
			AidReward->DisplayName = FText::FromString(TEXT("援助奖励"));
			DestroyReward = Battle.MakeNoopCard(0);
			DestroyReward->CardId = TEXT("Reward.Test.Branch.Destroy");
			DestroyReward->DisplayName = FText::FromString(TEXT("破坏奖励"));
			LegacyReward = Battle.MakeNoopCard(0);
			LegacyReward->CardId = TEXT("Reward.Test.Legacy");
			LegacyReward->DisplayName = FText::FromString(TEXT("旧奖励"));

			if (bUseExplicitRewards)
			{
				TargetPart->AidRewardCard = AidReward;
				TargetPart->DestroyRewardCard = DestroyReward;
			}
			if (bUseLegacyReward)
			{
				TargetPart->KnockdownRewardCard = LegacyReward;
			}

			Session = Battle.CreateSession(Character, Enemy, 1201);
			const FBattleSnapshot Before = Session->BuildSnapshot();
			TargetPartInstanceId =
				FWacomBattleFixture::FindPartInstanceId(Before, 0);
			const FGuid KillerInstanceId =
				FWacomBattleFixture::FindHandInstanceByCardId(Before, Killer->CardId);
			Session->ResolveCommand(
				FWacomBattleFixture::MakePlayCardOnPartInstance(
					Before,
					KillerInstanceId,
					TargetPartInstanceId));
		}
	};

	int32 CountCardGainedEvents(const TArray<FBattleEvent>& Events)
	{
		int32 Count = 0;
		for (const FBattleEvent& Event : Events)
		{
			if (Event.Type == EBattleEventType::CardGained)
			{
				++Count;
			}
		}
		return Count;
	}

	bool ResultContainsOnly(
		const FBattleResultPacket& Packet,
		const UCardDefinition* Expected,
		EKnockdownChoice Choice)
	{
		return Packet.GainedCards.Num() == 1
			&& Packet.GainedCards[0].Definition.Get() == Expected
			&& Packet.GainedCards[0].SourceChoice == Choice;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomKnockdownExplicitBranchRewardSpec,
	"Wacom.Battle.KnockdownReward.ExplicitBranchesDoNotCross",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomKnockdownExplicitBranchRewardSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomKnockdownBranchRewardSpec;

	for (const EKnockdownChoice Choice :
		{ EKnockdownChoice::Aid, EKnockdownChoice::Destroy })
	{
		FPendingChoiceFixture Fixture;
		Fixture.Initialize(/*bUseExplicitRewards*/ true, /*bUseLegacyReward*/ false);
		const FKnockdownChoiceView View =
			Fixture.Session->BuildPendingKnockdownChoiceView();
		TestTrue(TEXT("Aid preview has a reward"), View.AidOption.bHasRewardCard);
		TestEqual(TEXT("Aid preview CardId"),
			View.AidOption.RewardCardId, Fixture.AidReward->CardId);
		TestEqual(TEXT("Aid preview name"),
			View.AidOption.RewardCardName.ToString(), FString(TEXT("援助奖励")));
		TestTrue(TEXT("Destroy preview has a reward"),
			View.DestroyOption.bHasRewardCard);
		TestEqual(TEXT("Destroy preview CardId"),
			View.DestroyOption.RewardCardId, Fixture.DestroyReward->CardId);
		TestEqual(TEXT("Destroy preview name"),
			View.DestroyOption.RewardCardName.ToString(), FString(TEXT("破坏奖励")));
		TestTrue(TEXT("Reward presence does not disable Aid"),
			View.AidOption.bAvailable);
		TestTrue(TEXT("Reward presence does not disable Destroy"),
			View.DestroyOption.bAvailable);

		const FBattleResolution Resolution = Fixture.Session->ResolveCommand(
			FBattleCommand::MakeKnockdownChoice(Choice));
		TestTrue(TEXT("Explicit branch choice resolves"), Resolution.IsOk());
		const UCardDefinition* Expected = Choice == EKnockdownChoice::Aid
			? Fixture.AidReward
			: Fixture.DestroyReward;
		TestTrue(TEXT("Only selected branch reward is recorded"),
			ResultContainsOnly(
				Fixture.Session->BuildResultPacket(), Expected, Choice));
		TestEqual(TEXT("Exactly one CardGained event is emitted"),
			CountCardGainedEvents(Resolution.Events), 1);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomKnockdownLegacyRewardFallbackSpec,
	"Wacom.Battle.KnockdownReward.LegacyFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomKnockdownLegacyRewardFallbackSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomKnockdownBranchRewardSpec;
	for (const EKnockdownChoice Choice :
		{ EKnockdownChoice::Aid, EKnockdownChoice::Destroy })
	{
		FPendingChoiceFixture Fixture;
		Fixture.Initialize(/*bUseExplicitRewards*/ false, /*bUseLegacyReward*/ true);
		const FKnockdownChoiceView View =
			Fixture.Session->BuildPendingKnockdownChoiceView();
		const FKnockdownChoiceOptionView& Option = Choice == EKnockdownChoice::Aid
			? View.AidOption
			: View.DestroyOption;
		TestTrue(TEXT("Legacy preview reports reward"), Option.bHasRewardCard);
		TestEqual(TEXT("Legacy preview CardId"),
			Option.RewardCardId, Fixture.LegacyReward->CardId);

		const FBattleResolution Resolution = Fixture.Session->ResolveCommand(
			FBattleCommand::MakeKnockdownChoice(Choice));
		TestTrue(TEXT("Legacy branch choice resolves"), Resolution.IsOk());
		TestTrue(TEXT("Legacy reward is recorded with selected source choice"),
			ResultContainsOnly(
				Fixture.Session->BuildResultPacket(),
				Fixture.LegacyReward,
				Choice));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomKnockdownEmptyBranchRewardSpec,
	"Wacom.Battle.KnockdownReward.EmptyRewardRemainsAvailable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomKnockdownEmptyBranchRewardSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomKnockdownBranchRewardSpec;
	FPendingChoiceFixture Fixture;
	Fixture.Initialize(/*bUseExplicitRewards*/ false, /*bUseLegacyReward*/ false);
	const FKnockdownChoiceView View =
		Fixture.Session->BuildPendingKnockdownChoiceView();
	TestTrue(TEXT("Aid stays available without a reward"),
		View.AidOption.bAvailable);
	TestFalse(TEXT("Aid preview reports no reward"),
		View.AidOption.bHasRewardCard);
	TestTrue(TEXT("Destroy stays available without a reward"),
		View.DestroyOption.bAvailable);
	TestFalse(TEXT("Destroy preview reports no reward"),
		View.DestroyOption.bHasRewardCard);

	const FBattleResolution Resolution = Fixture.Session->ResolveCommand(
		FBattleCommand::MakeKnockdownChoice(EKnockdownChoice::Aid));
	TestTrue(TEXT("Empty-reward Aid still resolves"), Resolution.IsOk());
	TestEqual(TEXT("Empty reward emits no CardGained"),
		CountCardGainedEvents(Resolution.Events), 0);
	TestEqual(TEXT("Empty reward records no gained cards"),
		Fixture.Session->BuildResultPacket().GainedCards.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomKnockdownWithdrawIgnoresBranchRewardsSpec,
	"Wacom.Battle.KnockdownReward.WithdrawDoesNotGrant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomKnockdownWithdrawIgnoresBranchRewardsSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomKnockdownBranchRewardSpec;
	FPendingChoiceFixture Fixture;
	Fixture.Initialize(/*bUseExplicitRewards*/ true, /*bUseLegacyReward*/ false);
	TestTrue(TEXT("Withdraw is available while other parts live"),
		Fixture.Session->BuildPendingKnockdownChoiceView().WithdrawOption.bAvailable);

	const FBattleResolution Resolution = Fixture.Session->ResolveCommand(
		FBattleCommand::MakeKnockdownChoice(EKnockdownChoice::Withdraw));
	TestTrue(TEXT("Withdraw resolves"), Resolution.IsOk());
	TestEqual(TEXT("Withdraw emits no CardGained"),
		CountCardGainedEvents(Resolution.Events), 0);
	const FBattleResultPacket Packet = Fixture.Session->BuildResultPacket();
	TestTrue(TEXT("Packet is withdrawn"), Packet.bWithdrawn);
	TestEqual(TEXT("Withdraw records no gained cards"), Packet.GainedCards.Num(), 0);
	return true;
}
