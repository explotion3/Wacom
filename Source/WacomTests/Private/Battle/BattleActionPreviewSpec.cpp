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
	FCardEffect MakeActionPreviewEffect(
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

	UCardDefinition* MakeActionPreviewCard(
		FWacomBattleFixture& Fixture,
		int32 Cost,
		const TArray<FCardEffect>& Effects,
		const TArray<FGameplayTag>& Keywords = {})
	{
		UCardDefinition* Card = Fixture.MakeSimpleDamageCard(Cost, 0);
		Card->CardId = FName(*FString::Printf(
			TEXT("ActionPreview.%s"),
			*FGuid::NewGuid().ToString(EGuidFormats::Short)));
		Card->Effects = Effects;
		for (const FGameplayTag& Keyword : Keywords)
		{
			Card->Keywords.AddTag(Keyword);
		}
		return Card;
	}

	UBattleSession* CreateActionPreviewSession(
		FWacomBattleFixture& Fixture,
		const TArray<UCardDefinition*>& Cards,
		UEnemyDefinition* Enemy,
		int32 Seed = 11)
	{
		TArray<UCardDefinition*> Deck = Cards;
		while (Deck.Num() < 5)
		{
			Deck.Add(Fixture.MakeNoopCard(0));
		}

		return Fixture.CreateSession(
			Fixture.MakeCharacter(Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0), Deck),
			Enemy,
			Seed);
	}

	FWacomInteractionTargetHandle MakeActionPreviewPartTargetHandle(const FEnemyPartSnapshot& Part)
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

	FWacomInteractionTargetHandle MakeActionPreviewFirstPartTargetHandle(const FBattleSnapshot& Snapshot)
	{
		const FEnemyPartSnapshot* Part = FWacomBattleFixture::GetEnemyPartSnapshot(Snapshot, 0);
		return Part ? MakeActionPreviewPartTargetHandle(*Part) : FWacomInteractionTargetHandle();
	}

	const FBattleCardActionPreviewEnemyPartState* FindActionPreviewProjectedPart(
		const FBattleCardActionPreview& Preview,
		const FGuid& PartInstanceId)
	{
		for (const FBattleCardActionPreviewEnemyPartState& PartState : Preview.ProjectedEnemyParts)
		{
			if (PartState.Snapshot.InstanceId == PartInstanceId)
			{
				return &PartState;
			}
		}
		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleActionPreviewDamageAndEnemyActionSpec,
	"Wacom.Battle.ActionPreview.DamageAndEnemyActionProjectNetValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleActionPreviewDamageAndEnemyActionSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* DamageCard = MakeActionPreviewCard(
		Fixture,
		/*Cost*/3,
		{ MakeActionPreviewEffect(WacomTags::Effect_Damage, 3, WacomTags::Target_SingleEnemyPart) });
	UBattleSession* Session = CreateActionPreviewSession(
		Fixture,
		{ DamageCard },
		Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/20, /*Initiative*/3, /*Damage*/4));

	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, DamageCard->CardId);
	const FEnemyPartSnapshot* Part = FWacomBattleFixture::GetEnemyPartSnapshot(Snapshot, 0);
	if (!TestTrue(TEXT("Card and part exist"), CardId.IsValid() && Part))
	{
		return false;
	}

	const FBattleCardActionPreview Preview =
		Session->BuildCardActionPreview(CardId, MakeActionPreviewPartTargetHandle(*Part));

	TestTrue(TEXT("Action preview exists"), Preview.bHasPreview);
	TestTrue(TEXT("Player projection exists"), Preview.bHasProjectedPlayer);
	TestEqual(TEXT("Player HP accounts for enemy intent damage"), Preview.ProjectedPlayer.CurrentHp, 96);
	TestEqual(TEXT("Player shield stays zero"), Preview.ProjectedPlayer.Shield, 0);

	const FBattleCardActionPreviewEnemyPartState* ProjectedPart =
		FindActionPreviewProjectedPart(Preview, Part->InstanceId);
	if (!TestNotNull(TEXT("Projected enemy part exists"), ProjectedPart))
	{
		return false;
	}
	TestEqual(TEXT("Enemy HP projects damage"), ProjectedPart->Snapshot.CurrentHp, 17);
	TestEqual(TEXT("Enemy initiative displays action at zero"), ProjectedPart->Snapshot.CurrentInitiative, 0);
	TestTrue(TEXT("Part is marked as about to act"), ProjectedPart->bWillAct);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleActionPreviewShieldThenEnemyDamageSpec,
	"Wacom.Battle.ActionPreview.PlayerShieldShowsNetValueAfterEnemyAction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleActionPreviewShieldThenEnemyDamageSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* ShieldCard = MakeActionPreviewCard(
		Fixture,
		/*Cost*/3,
		{ MakeActionPreviewEffect(WacomTags::Status_Shield, 10, WacomTags::Target_Player) });
	UBattleSession* Session = CreateActionPreviewSession(
		Fixture,
		{ ShieldCard },
		Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/20, /*Initiative*/3, /*Damage*/3));

	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, ShieldCard->CardId);
	const FBattleCardActionPreview Preview =
		Session->BuildCardActionPreview(CardId, MakeActionPreviewFirstPartTargetHandle(Snapshot));

	TestTrue(TEXT("Action preview exists"), Preview.bHasPreview);
	TestTrue(TEXT("Player projection exists"), Preview.bHasProjectedPlayer);
	TestEqual(TEXT("Projected shield is shield gain minus incoming damage"), Preview.ProjectedPlayer.Shield, 7);
	TestEqual(TEXT("Player HP is unchanged while shield absorbs damage"), Preview.ProjectedPlayer.CurrentHp, 100);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleActionPreviewSwiftSkipsInitiativeSpec,
	"Wacom.Battle.ActionPreview.SwiftDoesNotProjectInitiativePushOrEnemyAction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleActionPreviewSwiftSkipsInitiativeSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* SwiftDamageCard = MakeActionPreviewCard(
		Fixture,
		/*Cost*/3,
		{ MakeActionPreviewEffect(WacomTags::Effect_Damage, 3, WacomTags::Target_SingleEnemyPart) },
		{ WacomTags::Card_Keyword_Swift });
	UBattleSession* Session = CreateActionPreviewSession(
		Fixture,
		{ SwiftDamageCard },
		Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/20, /*Initiative*/3, /*Damage*/4));

	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FEnemyPartSnapshot* Part = FWacomBattleFixture::GetEnemyPartSnapshot(Snapshot, 0);
	const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, SwiftDamageCard->CardId);
	const FBattleCardActionPreview Preview =
		Session->BuildCardActionPreview(CardId, MakeActionPreviewFirstPartTargetHandle(Snapshot));

	TestTrue(TEXT("Action preview exists"), Preview.bHasPreview);
	TestFalse(TEXT("No player projection because swift skips enemy action"), Preview.bHasProjectedPlayer);
	const FBattleCardActionPreviewEnemyPartState* ProjectedPart =
		Part ? FindActionPreviewProjectedPart(Preview, Part->InstanceId) : nullptr;
	if (!TestNotNull(TEXT("Projected swift damage part exists"), ProjectedPart))
	{
		return false;
	}
	TestEqual(TEXT("Swift damage still projects enemy HP"), ProjectedPart->Snapshot.CurrentHp, 17);
	TestEqual(TEXT("Swift does not push initiative"), ProjectedPart->Snapshot.CurrentInitiative, 3);
	TestFalse(TEXT("Swift does not mark part as acting"), ProjectedPart->bWillAct);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleActionPreviewProjectsAllInitiativeZeroEnemyActionsSpec,
	"Wacom.Battle.ActionPreview.ProjectsAllEnemyActionsTriggeredByInitiativePush",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleActionPreviewProjectsAllInitiativeZeroEnemyActionsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* DamageCard = MakeActionPreviewCard(
		Fixture,
		/*Cost*/3,
		{ MakeActionPreviewEffect(WacomTags::Effect_Damage, 3, WacomTags::Target_SingleEnemyPart) });
	UBattleSession* Session = CreateActionPreviewSession(
		Fixture,
		{ DamageCard },
		Fixture.MakeThreePartEnemy(
			/*HeadHp*/20,
			/*BodyHp*/20,
			/*TailHp*/20,
			/*HeadInitiative*/3,
			/*BodyInitiative*/2,
			/*TailInitiative*/1));

	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, DamageCard->CardId);
	const FEnemyPartSnapshot* TargetPart = FWacomBattleFixture::GetEnemyPartSnapshot(Snapshot, 0);
	if (!TestTrue(TEXT("Card and target part exist"), CardId.IsValid() && TargetPart))
	{
		return false;
	}

	const FBattleCardActionPreview Preview =
		Session->BuildCardActionPreview(CardId, MakeActionPreviewPartTargetHandle(*TargetPart));

	TestTrue(TEXT("Action preview exists"), Preview.bHasPreview);
	TestTrue(TEXT("Player projection includes all enemy actions"), Preview.bHasProjectedPlayer);
	TestEqual(TEXT("Non-stunned parts each deal 1 damage"), Preview.ProjectedPlayer.CurrentHp, 98);

	for (int32 PartIndex = 0; PartIndex < 3; ++PartIndex)
	{
		const FEnemyPartSnapshot* Part = FWacomBattleFixture::GetEnemyPartSnapshot(Snapshot, PartIndex);
		const FBattleCardActionPreviewEnemyPartState* ProjectedPart =
			Part ? FindActionPreviewProjectedPart(Preview, Part->InstanceId) : nullptr;
		if (!TestNotNull(FString::Printf(TEXT("Projected part %d exists"), PartIndex), ProjectedPart))
		{
			return false;
		}
		TestEqual(FString::Printf(TEXT("Part %d initiative displays action at zero"), PartIndex),
			ProjectedPart->Snapshot.CurrentInitiative,
			0);
		if (PartIndex == 0)
		{
			TestFalse(TEXT("Perfect-release target is not marked as an attack risk"),
				ProjectedPart->bWillAct);
			TestTrue(TEXT("Perfect-release target is marked as skipping due to stun"),
				ProjectedPart->bWillSkipActionDueToStun);
		}
		else
		{
			TestTrue(FString::Printf(TEXT("Part %d is marked as about to act"), PartIndex),
				ProjectedPart->bWillAct);
			TestFalse(FString::Printf(TEXT("Part %d does not skip"), PartIndex),
				ProjectedPart->bWillSkipActionDueToStun);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleActionPreviewRandomFollowUpSpec,
	"Wacom.Battle.ActionPreview.RandomFollowUpIsUnresolvedButDeterministicDamageProjects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleActionPreviewRandomFollowUpSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* RandomFollowUpCard = MakeActionPreviewCard(
		Fixture,
		/*Cost*/1,
		{
			MakeActionPreviewEffect(WacomTags::Effect_Damage, 3, WacomTags::Target_SingleEnemyPart),
			MakeActionPreviewEffect(WacomTags::Effect_Discard, 1, WacomTags::Target_Player)
		});
	UBattleSession* Session = CreateActionPreviewSession(
		Fixture,
		{ RandomFollowUpCard },
		Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/20, /*Initiative*/10, /*Damage*/0));

	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FEnemyPartSnapshot* Part = FWacomBattleFixture::GetEnemyPartSnapshot(Snapshot, 0);
	const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, RandomFollowUpCard->CardId);
	const FBattleCardActionPreview Preview =
		Session->BuildCardActionPreview(CardId, MakeActionPreviewFirstPartTargetHandle(Snapshot));

	TestTrue(TEXT("Action preview exists"), Preview.bHasPreview);
	TestTrue(TEXT("Random discard is marked unresolved"), Preview.bHasUnresolvedFacts);
	TestTrue(TEXT("Unresolved facts include discard"), Preview.UnresolvedEffectTypes.Contains(WacomTags::Effect_Discard));
	const FBattleCardActionPreviewEnemyPartState* ProjectedPart =
		Part ? FindActionPreviewProjectedPart(Preview, Part->InstanceId) : nullptr;
	if (!TestNotNull(TEXT("Projected enemy part exists"), ProjectedPart))
	{
		return false;
	}
	TestEqual(TEXT("Deterministic damage still projects"), ProjectedPart->Snapshot.CurrentHp, 17);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleActionPreviewAfterPlayedShieldSpec,
	"Wacom.Battle.ActionPreview.AfterPlayedDeterministicShieldProjects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleActionPreviewAfterPlayedShieldSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* Card = MakeActionPreviewCard(
		Fixture,
		/*Cost*/0,
		{ MakeActionPreviewEffect(WacomTags::Effect_Damage, 0, WacomTags::Target_SingleEnemyPart) });

	FCardPassive Passive;
	Passive.Trigger = WacomTags::Passive_Trigger_AfterPlayed;
	Passive.Effects.Add(MakeActionPreviewEffect(WacomTags::Status_Shield, 5, WacomTags::Target_Player));
	Card->Passives.Add(Passive);

	UBattleSession* Session = CreateActionPreviewSession(
		Fixture,
		{ Card },
		Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/20, /*Initiative*/10, /*Damage*/0));

	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, Card->CardId);
	const FBattleCardActionPreview Preview =
		Session->BuildCardActionPreview(CardId, MakeActionPreviewFirstPartTargetHandle(Snapshot));

	TestTrue(TEXT("Action preview exists"), Preview.bHasPreview);
	TestTrue(TEXT("AfterPlayed shield projects to player"), Preview.bHasProjectedPlayer);
	TestEqual(TEXT("Projected player shield"), Preview.ProjectedPlayer.Shield, 5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleActionPreviewInvalidTargetSpec,
	"Wacom.Battle.ActionPreview.InvalidTargetDoesNotProjectValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleActionPreviewInvalidTargetSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* Card = MakeActionPreviewCard(
		Fixture,
		/*Cost*/1,
		{ MakeActionPreviewEffect(WacomTags::Effect_Damage, 3, WacomTags::Target_SingleEnemyPart) });
	UBattleSession* Session = CreateActionPreviewSession(
		Fixture,
		{ Card },
		Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/20, /*Initiative*/10, /*Damage*/0));

	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, Card->CardId);
	const FBattleCardActionPreview Preview =
		Session->BuildCardActionPreview(CardId, FWacomInteractionTargetHandle());

	TestFalse(TEXT("Invalid target has no action preview"), Preview.bHasPreview);
	TestFalse(TEXT("Invalid target has no projected player"), Preview.bHasProjectedPlayer);
	TestEqual(TEXT("Invalid target has no projected parts"), Preview.ProjectedEnemyParts.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleActionPreviewNoTargetPlayerShieldSpec,
	"Wacom.Battle.ActionPreview.NoTargetPlayerShieldProjectsWithEmptyTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleActionPreviewNoTargetPlayerShieldSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* ShieldCard = MakeActionPreviewCard(
		Fixture,
		/*Cost*/0,
		{ MakeActionPreviewEffect(WacomTags::Status_Shield, 10, WacomTags::Target_Player) });
	ShieldCard->TargetMode = ECardTargetMode::None;
	UBattleSession* Session = CreateActionPreviewSession(
		Fixture,
		{ ShieldCard },
		Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/20, /*Initiative*/10, /*Damage*/0));

	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, ShieldCard->CardId);
	const FBattleCardActionPreview Preview =
		Session->BuildCardActionPreview(CardId, FWacomInteractionTargetHandle());

	TestTrue(TEXT("No-target action preview exists"), Preview.bHasPreview);
	TestTrue(TEXT("No-target target preview exists"), Preview.TargetPreview.bHasPreview);
	TestTrue(TEXT("No-target validation can target"), Preview.TargetPreview.Validation.bCanTarget);
	TestEqual(TEXT("No-target preview kind remains none"),
		Preview.TargetPreview.TargetKind,
		EWacomBattleCardPreviewTargetKind::None);
	TestTrue(TEXT("No-target player projection exists"), Preview.bHasProjectedPlayer);
	TestEqual(TEXT("No-target projected shield"), Preview.ProjectedPlayer.Shield, 10);
	TestEqual(TEXT("No-target preview does not project enemy parts"), Preview.ProjectedEnemyParts.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleActionPreviewSelectedDiscardPassiveParitySpec,
	"Wacom.Battle.ActionPreview.SelectedDiscardOnDiscardPassiveMatchesResolution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleActionPreviewSelectedDiscardPassiveParitySpec::RunTest(const FString& /*Parameters*/)
{
	const FName SourceCardId(TEXT("ActionPreview.Parity.SelectedDiscardSource"));
	const FName TargetCardId(TEXT("ActionPreview.Parity.OnDiscardShieldTarget"));
	constexpr int32 ShieldAmount = 6;
	constexpr int32 Seed = 17;

	auto CreateParitySession = [SourceCardId, TargetCardId, ShieldAmount, Seed](FWacomBattleFixture& Fixture)
	{
		UCardDefinition* SourceCard = Fixture.MakeSelectedHandCardZoneMoveCard(
			/*Cost*/0,
			/*bExhaust*/false);
		SourceCard->CardId = SourceCardId;

		UCardDefinition* TargetCard = Fixture.MakeOnDiscardShieldCard(
			/*Cost*/0,
			ShieldAmount);
		TargetCard->CardId = TargetCardId;

		return CreateActionPreviewSession(
			Fixture,
			{ SourceCard, TargetCard },
			Fixture.MakeSinglePartEnemyWithIntentDamage(
				/*Hp*/50,
				/*Initiative*/50,
				/*Damage*/0),
			Seed);
	};

	FWacomBattleFixture PreviewFixture;
	UBattleSession* PreviewSession = CreateParitySession(PreviewFixture);
	const FBattleSnapshot PreviewInitialSnapshot = PreviewSession->BuildSnapshot();
	const FGuid PreviewSourceId =
		FWacomBattleFixture::FindHandInstanceByCardId(PreviewInitialSnapshot, SourceCardId);
	const FGuid PreviewTargetId =
		FWacomBattleFixture::FindHandInstanceByCardId(PreviewInitialSnapshot, TargetCardId);

	FWacomBattleFixture SubmitFixture;
	UBattleSession* SubmitSession = CreateParitySession(SubmitFixture);
	const FBattleSnapshot SubmitInitialSnapshot = SubmitSession->BuildSnapshot();
	const FGuid SubmitSourceId =
		FWacomBattleFixture::FindHandInstanceByCardId(SubmitInitialSnapshot, SourceCardId);
	const FGuid SubmitTargetId =
		FWacomBattleFixture::FindHandInstanceByCardId(SubmitInitialSnapshot, TargetCardId);

	if (!TestTrue(
		TEXT("Equivalent sessions contain source and target cards"),
		PreviewSourceId.IsValid()
			&& PreviewTargetId.IsValid()
			&& SubmitSourceId.IsValid()
			&& SubmitTargetId.IsValid()))
	{
		return false;
	}
	TestEqual(
		TEXT("Equivalent sessions start with the same player shield"),
		PreviewInitialSnapshot.Player.Shield,
		SubmitInitialSnapshot.Player.Shield);

	const FBattleCardActionPreview Preview = PreviewSession->BuildCardActionPreview(
		PreviewSourceId,
		FWacomInteractionTargetHandle::ForCardTarget(PreviewTargetId, PreviewSession));
	const FBattleResolution SubmitStatus = SubmitSession->ResolveCommand(
		FBattleCommand::MakePlayCardOnHandCard(SubmitSourceId, SubmitTargetId));
	const FBattleSnapshot ResolvedSnapshot = SubmitSession->BuildSnapshot();

	TestTrue(TEXT("Selected discard action preview exists"), Preview.bHasPreview);
	TestTrue(TEXT("Selected discard resolves through command resolution"), SubmitStatus.IsOk());
	TestEqual(
		TEXT("Command resolution applies target OnDiscard shield"),
		ResolvedSnapshot.Player.Shield,
		ShieldAmount);
	TestTrue(TEXT("Selected discard preview projects the player"), Preview.bHasProjectedPlayer);
	TestEqual(
		TEXT("Selected discard preview shield matches command resolution"),
		Preview.ProjectedPlayer.Shield,
		ResolvedSnapshot.Player.Shield);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleActionPreviewSelectedDiscardUnresolvedPassiveSpec,
	"Wacom.Battle.ActionPreview.SelectedDiscardRandomOnDiscardFollowUpStaysUnresolved",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleActionPreviewSelectedDiscardUnresolvedPassiveSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* SourceCard = Fixture.MakeSelectedHandCardZoneMoveCard(
		/*Cost*/0,
		/*bExhaust*/false);
	UCardDefinition* TargetCard = Fixture.MakeOnDiscardShieldCard(/*Cost*/0, /*ShieldAmount*/6);
	if (!TestTrue(TEXT("Discard target has an OnDiscard passive"), TargetCard->Passives.IsValidIndex(0)))
	{
		return false;
	}
	TargetCard->Passives[0].Effects.Add(
		MakeActionPreviewEffect(WacomTags::Effect_Discard, 1, WacomTags::Target_Player));

	FCardPassive RandomDiscardVictimPassive;
	RandomDiscardVictimPassive.Trigger = WacomTags::Passive_Trigger_OnDiscard;
	RandomDiscardVictimPassive.Effects.Add(
		MakeActionPreviewEffect(WacomTags::Status_Shield, 9, WacomTags::Target_Player));
	SourceCard->Passives.Add(RandomDiscardVictimPassive);

	TArray<UCardDefinition*> Deck = { SourceCard, TargetCard };
	for (int32 Index = 0; Index < 3; ++Index)
	{
		Deck.Add(Fixture.MakeOnDiscardShieldCard(/*Cost*/0, /*ShieldAmount*/9));
	}

	UBattleSession* Session = CreateActionPreviewSession(
		Fixture,
		Deck,
		Fixture.MakeSinglePartEnemyWithIntentDamage(
			/*Hp*/50,
			/*Initiative*/50,
			/*Damage*/0),
		/*Seed*/23);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceId =
		FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, SourceCard->CardId);
	const FGuid TargetId =
		FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, TargetCard->CardId);
	if (!TestTrue(TEXT("Source and discard target exist"), SourceId.IsValid() && TargetId.IsValid()))
	{
		return false;
	}

	const FBattleCardActionPreview Preview = Session->BuildCardActionPreview(
		SourceId,
		FWacomInteractionTargetHandle::ForCardTarget(TargetId, Session));

	TestTrue(TEXT("Selected discard action preview exists"), Preview.bHasPreview);
	TestTrue(TEXT("Deterministic OnDiscard shield projects"), Preview.bHasProjectedPlayer);
	TestEqual(TEXT("Random discard result is not folded into shield"), Preview.ProjectedPlayer.Shield, 6);
	TestTrue(TEXT("Random OnDiscard follow-up is unresolved"), Preview.bHasUnresolvedFacts);
	TestTrue(
		TEXT("Unresolved effects include random discard"),
		Preview.UnresolvedEffectTypes.Contains(WacomTags::Effect_Discard));
	return true;
}
