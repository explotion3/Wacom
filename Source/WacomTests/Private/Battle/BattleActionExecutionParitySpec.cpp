// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Cards/CardPassive.h"
#include "Characters/CharacterDefinition.h"
#include "Commands/BattleCommand.h"
#include "Resolution/BattleCardActionPreview.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Snapshots/EnemySnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "Types/WacomResult.h"

namespace
{
	FCardEffect MakeParityEffect(
		const FGameplayTag& EffectType,
		int32 Magnitude,
		const FGameplayTag& Target)
	{
		FCardEffect Effect;
		Effect.EffectType = EffectType;
		Effect.Magnitude = Magnitude;
		Effect.Target = Target;
		return Effect;
	}

	UCardDefinition* MakeParityCard(
		FWacomBattleFixture& Fixture,
		FName CardId,
		int32 Cost,
		ECardTargetMode TargetMode,
		const TArray<FCardEffect>& Effects)
	{
		UCardDefinition* Card = Fixture.MakeNoopCard(Cost);
		Card->CardId = CardId;
		Card->TargetMode = TargetMode;
		Card->Effects = Effects;
		return Card;
	}

	UBattleSession* CreateParitySession(
		FWacomBattleFixture& Fixture,
		const TArray<UCardDefinition*>& Cards,
		UEnemyDefinition* Enemy,
		int32 Seed)
	{
		TArray<UCardDefinition*> Deck = Cards;
		while (Deck.Num() < 5)
		{
			Deck.Add(Fixture.MakeNoopCard(0));
		}

		return Fixture.CreateSession(
			Fixture.MakeCharacter(Fixture.MakeNoopCard(5), Fixture.MakeNoopCard(5), Deck),
			Enemy,
			Seed);
	}

	FWacomInteractionTargetHandle MakePartTargetHandle(const FEnemyPartSnapshot& Part)
	{
		return FWacomInteractionTargetHandle::ForWorldTarget(
			Part.InstanceId,
			nullptr,
			FVector::ZeroVector,
			FVector2D::ZeroVector,
			FGameplayTag(),
			NAME_None,
			Part.EncounterId,
			Part.EnemySlotId,
			Part.PartSlotId);
	}

	const FBattleCardActionPreviewEnemyPartState* FindProjectedPart(
		const FBattleCardActionPreview& Preview,
		const FGuid& PartInstanceId)
	{
		return Preview.ProjectedEnemyParts.FindByPredicate(
			[PartInstanceId](const FBattleCardActionPreviewEnemyPartState& PartState)
			{
				return PartState.Snapshot.InstanceId == PartInstanceId;
			});
	}

	int32 GetStatusStacks(const TMap<FGameplayTag, int32>& StatusStacks, const FGameplayTag& StatusTag)
	{
		const int32* Stacks = StatusStacks.Find(StatusTag);
		return Stacks ? *Stacks : 0;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleActionExecutionDeterministicParitySpec,
	"Wacom.Battle.ActionPreview.Parity.DeterministicTransactionMatchesSubmitCommand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleActionExecutionDeterministicParitySpec::RunTest(const FString& /*Parameters*/)
{
	const FName CardId(TEXT("ActionPreview.Parity.DeterministicTransaction"));
	constexpr int32 Seed = 37;

	auto CreateSession = [CardId, Seed](FWacomBattleFixture& Fixture)
	{
		UCardDefinition* Card = MakeParityCard(
			Fixture,
			CardId,
			/*Cost*/2,
			ECardTargetMode::SingleEnemyPart,
			{
				MakeParityEffect(WacomTags::Effect_Damage, 4, WacomTags::Target_SingleEnemyPart),
				MakeParityEffect(WacomTags::Status_Shield, 5, WacomTags::Target_Player),
				MakeParityEffect(WacomTags::Effect_ApplyStatus_Poison, 2, WacomTags::Target_SingleEnemyPart),
				MakeParityEffect(WacomTags::Effect_ModifyInitiative, 1, WacomTags::Target_SingleEnemyPart),
			});
		return CreateParitySession(
			Fixture,
			{ Card },
			Fixture.MakeSinglePartEnemyWithIntentDamage(
				/*Hp*/30,
				/*Initiative*/20,
				/*IntentResist*/99,
				/*Damage*/0),
			Seed);
	};

	FWacomBattleFixture PreviewFixture;
	UBattleSession* PreviewSession = CreateSession(PreviewFixture);
	const FBattleSnapshot PreviewInitial = PreviewSession->BuildSnapshot();
	const FGuid PreviewCardId = FWacomBattleFixture::FindHandInstanceByCardId(PreviewInitial, CardId);
	const FEnemyPartSnapshot* PreviewPart = FWacomBattleFixture::GetEnemyPartSnapshot(PreviewInitial, 0);

	FWacomBattleFixture SubmitFixture;
	UBattleSession* SubmitSession = CreateSession(SubmitFixture);
	const FBattleSnapshot SubmitInitial = SubmitSession->BuildSnapshot();
	const FGuid SubmitCardId = FWacomBattleFixture::FindHandInstanceByCardId(SubmitInitial, CardId);
	const FEnemyPartSnapshot* SubmitPart = FWacomBattleFixture::GetEnemyPartSnapshot(SubmitInitial, 0);

	if (!TestTrue(
		TEXT("Equivalent sessions contain source cards and enemy parts"),
		PreviewCardId.IsValid() && PreviewPart && SubmitCardId.IsValid() && SubmitPart))
	{
		return false;
	}

	const FBattleCardActionPreview Preview = PreviewSession->BuildCardActionPreview(
		PreviewCardId,
		MakePartTargetHandle(*PreviewPart));
	const FWacomStatus SubmitStatus = SubmitSession->SubmitCommand(
		FBattleCommand::MakePlayCardOnEnemyPartKey(SubmitCardId, SubmitPart->PartKey));
	const FBattleSnapshot Resolved = SubmitSession->BuildSnapshot();
	const FEnemyPartSnapshot* ResolvedPart = FWacomBattleFixture::GetEnemyPartSnapshot(Resolved, 0);
	const FBattleCardActionPreviewEnemyPartState* ProjectedPart =
		FindProjectedPart(Preview, PreviewPart->InstanceId);

	TestTrue(TEXT("Deterministic preview exists"), Preview.bHasPreview);
	TestFalse(TEXT("Deterministic transaction has no unresolved facts"), Preview.bHasUnresolvedFacts);
	TestTrue(TEXT("Equivalent SubmitCommand succeeds"), SubmitStatus.IsOk());
	if (!TestNotNull(TEXT("Projected enemy part exists"), ProjectedPart)
		|| !TestNotNull(TEXT("Resolved enemy part exists"), ResolvedPart))
	{
		return false;
	}

	TestTrue(TEXT("Player projection exists"), Preview.bHasProjectedPlayer);
	TestEqual(TEXT("Projected player HP matches submit"), Preview.ProjectedPlayer.CurrentHp, Resolved.Player.CurrentHp);
	TestEqual(TEXT("Projected player shield matches submit"), Preview.ProjectedPlayer.Shield, Resolved.Player.Shield);
	TestEqual(TEXT("Projected enemy HP matches submit"), ProjectedPart->Snapshot.CurrentHp, ResolvedPart->CurrentHp);
	TestEqual(TEXT("Projected enemy shield matches submit"), ProjectedPart->Snapshot.Shield, ResolvedPart->Shield);
	TestEqual(
		TEXT("Projected enemy initiative matches submit"),
		ProjectedPart->Snapshot.CurrentInitiative,
		ResolvedPart->CurrentInitiative);
	TestEqual(
		TEXT("Projected poison stacks match submit"),
		GetStatusStacks(ProjectedPart->Snapshot.StatusStacks, WacomTags::Status_Poison),
		GetStatusStacks(ResolvedPart->StatusStacks, WacomTags::Status_Poison));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleActionPreviewRequiresPlayerActionPhaseSpec,
	"Wacom.Battle.ActionPreview.Eligibility.RequiresPlayerActionPhase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleActionPreviewRequiresPlayerActionPhaseSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* LethalCard = MakeParityCard(
		Fixture,
		TEXT("ActionPreview.Eligibility.Lethal"),
		/*Cost*/0,
		ECardTargetMode::SingleEnemyPart,
		{ MakeParityEffect(WacomTags::Effect_Damage, 999, WacomTags::Target_SingleEnemyPart) });
	UCardDefinition* FollowUpCard = MakeParityCard(
		Fixture,
		TEXT("ActionPreview.Eligibility.FollowUp"),
		/*Cost*/0,
		ECardTargetMode::None,
		{ MakeParityEffect(WacomTags::Status_Shield, 3, WacomTags::Target_Player) });
	UBattleSession* Session = CreateParitySession(
		Fixture,
		{ LethalCard, FollowUpCard },
		Fixture.MakeThreePartEnemy(
			/*HeadHp*/10,
			/*BodyHp*/10,
			/*TailHp*/10,
			/*HeadInitiative*/50,
			/*BodyInitiative*/50,
			/*TailInitiative*/50),
		/*Seed*/41);

	const FBattleSnapshot Initial = Session->BuildSnapshot();
	const FGuid LethalCardId = FWacomBattleFixture::FindHandInstanceByCardId(Initial, LethalCard->CardId);
	const FGuid FollowUpCardId = FWacomBattleFixture::FindHandInstanceByCardId(Initial, FollowUpCard->CardId);
	const FEnemyPartSnapshot* FirstPart = FWacomBattleFixture::GetEnemyPartSnapshot(Initial, 0);
	if (!TestTrue(
		TEXT("Phase fixture contains both cards and a target part"),
		LethalCardId.IsValid() && FollowUpCardId.IsValid() && FirstPart))
	{
		return false;
	}

	const FWacomStatus SubmitStatus = Session->SubmitCommand(
		FBattleCommand::MakePlayCardOnEnemyPartKey(LethalCardId, FirstPart->PartKey));
	const FBattleSnapshot PendingSnapshot = Session->BuildSnapshot();
	const FBattleCardActionPreview Preview = Session->BuildCardActionPreview(
		FollowUpCardId,
		FWacomInteractionTargetHandle());

	TestTrue(TEXT("Lethal card submit succeeds"), SubmitStatus.IsOk());
	TestEqual(
		TEXT("One destroyed part enters knockdown choice phase"),
		PendingSnapshot.Phase,
		EBattlePhase::PendingKnockdownChoice);
	TestTrue(TEXT("Target preview remains independently valid"), Preview.TargetPreview.bHasPreview);
	TestFalse(TEXT("Action preview is unavailable outside PlayerAction"), Preview.bHasPreview);
	TestFalse(TEXT("No player projection is returned outside PlayerAction"), Preview.bHasProjectedPlayer);
	TestEqual(TEXT("No enemy projections are returned outside PlayerAction"), Preview.ProjectedEnemyParts.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleActionPreviewCompanionRandomBranchSpec,
	"Wacom.Battle.ActionPreview.Unresolved.CompanionReturnDoesNotLeakRandomDiscardEffects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleActionPreviewCompanionRandomBranchSpec::RunTest(const FString& /*Parameters*/)
{
	bool bCovered = false;
	for (int32 Seed = 1; Seed <= 64 && !bCovered; ++Seed)
	{
		FWacomBattleFixture Fixture;
		UCardDefinition* SourceCard = MakeParityCard(
			Fixture,
			TEXT("ActionPreview.Unresolved.CompanionSource"),
			/*Cost*/0,
			ECardTargetMode::None,
			{ MakeParityEffect(WacomTags::Status_Shield, 2, WacomTags::Target_Player) });
		SourceCard->Keywords.AddTag(WacomTags::Card_Keyword_Companion);

		TArray<UCardDefinition*> Deck = { SourceCard };
		for (int32 Index = 0; Index < 12; ++Index)
		{
			UCardDefinition* ReturnCard = Fixture.MakeNoopCard(0);
			ReturnCard->Keywords.AddTag(WacomTags::Card_Keyword_Companion);

			FCardPassive ReturnPassive;
			ReturnPassive.Trigger = WacomTags::Passive_Trigger_OnCompanionCount;
			ReturnPassive.TriggerThreshold = 1;
			ReturnCard->Passives.Add(ReturnPassive);

			FCardPassive OnDiscardPassive;
			OnDiscardPassive.Trigger = WacomTags::Passive_Trigger_OnDiscard;
			OnDiscardPassive.Effects.Add(
				MakeParityEffect(WacomTags::Status_Shield, 9, WacomTags::Target_Player));
			ReturnCard->Passives.Add(OnDiscardPassive);
			Deck.Add(ReturnCard);
		}

		UBattleSession* Session = CreateParitySession(
			Fixture,
			Deck,
			Fixture.MakeSinglePartEnemyWithIntentDamage(
				/*Hp*/500,
				/*Initiative*/50,
				/*IntentResist*/0,
				/*Damage*/0),
			Seed);
		const FBattleSnapshot Initial = Session->BuildSnapshot();
		const FGuid SourceCardId =
			FWacomBattleFixture::FindHandInstanceByCardId(Initial, SourceCard->CardId);
		if (!SourceCardId.IsValid())
		{
			continue;
		}

		const FBattleCardActionPreview Preview = Session->BuildCardActionPreview(
			SourceCardId,
			FWacomInteractionTargetHandle());
		TestTrue(FString::Printf(TEXT("Seed=%d action preview exists"), Seed), Preview.bHasPreview);
		TestTrue(FString::Printf(TEXT("Seed=%d player projection exists"), Seed), Preview.bHasProjectedPlayer);
		TestEqual(
			FString::Printf(TEXT("Seed=%d random companion discards do not alter projected shield"), Seed),
			Preview.ProjectedPlayer.Shield,
			2);
		TestTrue(
			FString::Printf(TEXT("Seed=%d companion return is marked unresolved"), Seed),
			Preview.bHasUnresolvedFacts);
		bCovered = true;
	}

	TestTrue(TEXT("At least one seed draws the companion source card"), bCovered);
	return true;
}
