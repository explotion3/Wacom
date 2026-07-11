// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Commands/BattleCommand.h"
#include "Events/BattleEvent.h"
#include "Resolution/BattleCardActionPreview.h"
#include "Resolution/BattleCardTargetPreview.h"
#include "Resolution/BattleTargetValidationResult.h"
#include "Runtime/BattleEnemyKeys.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Snapshots/EnemySnapshot.h"
#include "Types/WacomEnums.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "Types/WacomResult.h"

namespace
{
	UCardDefinition* MakeEvaluationCard(
		FWacomBattleFixture& Fixture,
		const TCHAR* CardId,
		ECardTargetMode TargetMode,
		int32 Cost = 0)
	{
		UCardDefinition* Card = Fixture.MakeNoopCard(Cost);
		Card->CardId = CardId;
		Card->TargetMode = TargetMode;
		return Card;
	}

	UBattleSession* CreateEvaluationSession(
		FWacomBattleFixture& Fixture,
		const TArray<UCardDefinition*>& RequiredCards,
		UEnemyDefinition* Enemy,
		int32 Seed = 73)
	{
		TArray<UCardDefinition*> Deck = RequiredCards;
		while (Deck.Num() < 5)
		{
			Deck.Add(Fixture.MakeNoopCard(0));
		}

		return Fixture.CreateSession(
			Fixture.MakeCharacter(Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0), Deck),
			Enemy,
			Seed);
	}

	FWacomInteractionTargetHandle MakePartHandle(const FEnemyPartSnapshot& Part)
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

	void TestRejectedStatus(
		FAutomationTestBase& Test,
		const TCHAR* Label,
		const FBattleResolution& Resolution,
		EWacomError ExpectedCode,
		FName ExpectedDetail)
	{
		const FWacomStatus& Status = Resolution.Status;
		Test.TestFalse(FString::Printf(TEXT("%s rejects"), Label), Status.IsOk());
		Test.TestEqual(
			FString::Printf(TEXT("%s error code"), Label),
			static_cast<int32>(Status.Code),
			static_cast<int32>(ExpectedCode));
		Test.TestEqual(
			FString::Printf(TEXT("%s detail"), Label),
			Status.Detail,
			ExpectedDetail);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomPlayCardEvaluationPreviewFocusSpec,
	"Wacom.Battle.PlayCardEvaluation.PreviewFocus.DoesNotBlockImplicitTargetModes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomPlayCardEvaluationPreviewFocusSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* NoneCard = MakeEvaluationCard(
		Fixture,
		TEXT("PlayCardEvaluation.Focus.None"),
		ECardTargetMode::None);
	UCardDefinition* SelfCard = MakeEvaluationCard(
		Fixture,
		TEXT("PlayCardEvaluation.Focus.Self"),
		ECardTargetMode::Self);
	UCardDefinition* AllCard = MakeEvaluationCard(
		Fixture,
		TEXT("PlayCardEvaluation.Focus.AllEnemyParts"),
		ECardTargetMode::AllEnemyParts);
	UBattleSession* Session = CreateEvaluationSession(
		Fixture,
		{ NoneCard, SelfCard, AllCard },
		Fixture.MakeSinglePartEnemyWithIntentDamage(
			/*Hp*/20,
			/*Initiative*/10,
			/*IntentResist*/99,
			/*Damage*/0));

	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid NoneId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, NoneCard->CardId);
	const FGuid SelfId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, SelfCard->CardId);
	const FGuid AllId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, AllCard->CardId);
	const FEnemyPartSnapshot* Part = FWacomBattleFixture::GetEnemyPartSnapshot(Snapshot, 0);
	if (!TestTrue(
		TEXT("Focus fixture contains all source cards and an enemy part"),
		NoneId.IsValid() && SelfId.IsValid() && AllId.IsValid() && Part))
	{
		return false;
	}

	const FWacomInteractionTargetHandle ValidPartFocus = MakePartHandle(*Part);
	const FWacomInteractionTargetHandle WrongCardFocus =
		FWacomInteractionTargetHandle::ForCardTarget(NoneId, Session);
	const FWacomInteractionTargetHandle MalformedWorldFocus =
		FWacomInteractionTargetHandle::ForWorldTarget(FGuid::NewGuid(), nullptr);

	const FBattleCardTargetPreview NonePreview =
		Session->BuildCardTargetPreview(NoneId, ValidPartFocus);
	TestTrue(TEXT("None mode ignores a valid enemy-part focus"), NonePreview.bHasPreview);
	TestTrue(TEXT("None mode remains structurally valid"), NonePreview.Validation.bCanTarget);
	TestEqual(
		TEXT("None mode does not expose focus as an execution target"),
		NonePreview.TargetKind,
		EWacomBattleCardPreviewTargetKind::None);

	const FBattleCardTargetPreview SelfPreview =
		Session->BuildCardTargetPreview(SelfId, WrongCardFocus);
	TestTrue(TEXT("Self mode ignores a card focus"), SelfPreview.bHasPreview);
	TestTrue(TEXT("Self mode remains structurally valid"), SelfPreview.Validation.bCanTarget);
	TestEqual(
		TEXT("Self mode does not expose focus as an execution target"),
		SelfPreview.TargetKind,
		EWacomBattleCardPreviewTargetKind::None);

	const FWacomBattleTargetValidationResult NoneProbe =
		Session->ValidateTargetWithCard(NoneId, ValidPartFocus);
	TestFalse(TEXT("None mode does not accept an explicit world target"), NoneProbe.bCanTarget);
	TestEqual(
		TEXT("None explicit world target reports wrong target kind"),
		NoneProbe.RejectReason,
		EWacomBattleTargetRejectReason::UnsupportedWorldTarget);

	const FWacomBattleTargetValidationResult SelfProbe =
		Session->ValidateTargetWithCard(SelfId, WrongCardFocus);
	TestFalse(TEXT("Self mode does not accept an explicit card target"), SelfProbe.bCanTarget);
	TestEqual(
		TEXT("Self explicit card target reports wrong target kind"),
		SelfProbe.RejectReason,
		EWacomBattleTargetRejectReason::UnsupportedCardTarget);

	const FBattleCardTargetPreview AllMalformedPreview =
		Session->BuildCardTargetPreview(AllId, MalformedWorldFocus);
	TestTrue(TEXT("AllEnemyParts ignores malformed optional focus"), AllMalformedPreview.bHasPreview);
	TestTrue(TEXT("AllEnemyParts remains structurally valid"), AllMalformedPreview.Validation.bCanTarget);
	TestEqual(
		TEXT("Malformed AllEnemyParts focus produces no focus facts"),
		AllMalformedPreview.TargetKind,
		EWacomBattleCardPreviewTargetKind::None);

	const FBattleCardTargetPreview AllValidPreview =
		Session->BuildCardTargetPreview(AllId, ValidPartFocus);
	TestTrue(TEXT("AllEnemyParts accepts a valid optional focus"), AllValidPreview.bHasPreview);
	TestEqual(
		TEXT("Valid AllEnemyParts focus is exposed for presentation"),
		AllValidPreview.TargetKind,
		EWacomBattleCardPreviewTargetKind::EnemyPart);
	TestEqual(
		TEXT("AllEnemyParts presentation focus resolves the selected part"),
		AllValidPreview.TargetEnemyPartInstanceId,
		Part->InstanceId);

	TestTrue(
		TEXT("None action preview uses the normalized no-target command"),
		Session->BuildCardActionPreview(NoneId, ValidPartFocus).bHasPreview);
	TestTrue(
		TEXT("Self action preview uses the normalized no-target command"),
		Session->BuildCardActionPreview(SelfId, WrongCardFocus).bHasPreview);
	TestTrue(
		TEXT("AllEnemyParts action preview ignores malformed optional focus"),
		Session->BuildCardActionPreview(AllId, MalformedWorldFocus).bHasPreview);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomPlayCardEvaluationAllEnemyPartsProbeSpec,
	"Wacom.Battle.PlayCardEvaluation.TargetProbe.AllEnemyPartsIsStrict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomPlayCardEvaluationAllEnemyPartsProbeSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* AllCard = MakeEvaluationCard(
		Fixture,
		TEXT("PlayCardEvaluation.Probe.AllEnemyParts"),
		ECardTargetMode::AllEnemyParts);
	UCardDefinition* DestroyCard = Fixture.MakeSimpleDamageCard(/*Cost*/0, /*Damage*/99);
	DestroyCard->CardId = TEXT("PlayCardEvaluation.Probe.DestroyPart");
	UBattleSession* Session = CreateEvaluationSession(
		Fixture,
		{ AllCard, DestroyCard },
		Fixture.MakeThreePartEnemy(
			/*HeadHp*/5,
			/*BodyHp*/50,
			/*TailHp*/50,
			/*HeadInitiative*/10,
			/*BodyInitiative*/10,
			/*TailInitiative*/10));

	const FBattleSnapshot Before = Session->BuildSnapshot();
	const FGuid AllId = FWacomBattleFixture::FindHandInstanceByCardId(Before, AllCard->CardId);
	const FGuid DestroyId = FWacomBattleFixture::FindHandInstanceByCardId(Before, DestroyCard->CardId);
	const FEnemyPartSnapshot* Head = FWacomBattleFixture::GetEnemyPartSnapshot(Before, 0);
	if (!TestTrue(
		TEXT("Strict probe fixture contains both cards and the head part"),
		AllId.IsValid() && DestroyId.IsValid() && Head))
	{
		return false;
	}

	const FWacomInteractionTargetHandle HeadHandle = MakePartHandle(*Head);
	const FWacomBattleTargetValidationResult ValidProbe =
		Session->ValidateTargetWithCard(AllId, HeadHandle);
	TestTrue(TEXT("AllEnemyParts strict probe accepts a living part"), ValidProbe.bCanTarget);
	TestEqual(
		TEXT("Living probe resolves the focused part"),
		ValidProbe.ResolvedPartInstanceId,
		Head->InstanceId);

	const FWacomInteractionTargetHandle MalformedHandle =
		FWacomInteractionTargetHandle::ForWorldTarget(FGuid(), nullptr);
	const FWacomBattleTargetValidationResult MalformedProbe =
		Session->ValidateTargetWithCard(AllId, MalformedHandle);
	TestFalse(TEXT("AllEnemyParts strict probe rejects a malformed world handle"), MalformedProbe.bCanTarget);
	TestEqual(
		TEXT("Malformed world handle reports InvalidWorldTarget"),
		MalformedProbe.RejectReason,
		EWacomBattleTargetRejectReason::InvalidWorldTarget);

	const FWacomInteractionTargetHandle StaleHandle =
		FWacomInteractionTargetHandle::ForWorldTarget(
			FGuid::NewGuid(),
			nullptr,
			FVector::ZeroVector,
			FVector2D::ZeroVector,
			FGameplayTag(),
			NAME_None,
			Head->EncounterId,
			Head->EnemySlotId,
			TEXT("MissingPart"));
	const FWacomBattleTargetValidationResult StaleProbe =
		Session->ValidateTargetWithCard(AllId, StaleHandle);
	TestFalse(TEXT("AllEnemyParts strict probe rejects a stale slot identity"), StaleProbe.bCanTarget);
	TestEqual(
		TEXT("Stale slot identity reports InvalidWorldTarget"),
		StaleProbe.RejectReason,
		EWacomBattleTargetRejectReason::InvalidWorldTarget);

	const FBattleResolution DestroyStatus = Session->ResolveCommand(
		FBattleCommand::MakePlayCardOnEnemyPartKey(DestroyId, Head->PartKey));
	TestTrue(TEXT("Destroy setup card resolves"), DestroyStatus.IsOk());
	TestEqual(
		TEXT("Destroy setup enters pending knockdown choice"),
		Session->GetPhase(),
		EBattlePhase::PendingKnockdownChoice);

	const FWacomBattleTargetValidationResult DestroyedProbe =
		Session->ValidateTargetWithCard(AllId, HeadHandle);
	TestFalse(TEXT("AllEnemyParts strict probe rejects a destroyed part"), DestroyedProbe.bCanTarget);
	TestEqual(
		TEXT("Destroyed part reports InvalidWorldTarget"),
		DestroyedProbe.RejectReason,
		EWacomBattleTargetRejectReason::InvalidWorldTarget);

	const FBattleCardTargetPreview DestroyedFocusPreview =
		Session->BuildCardTargetPreview(AllId, HeadHandle);
	TestTrue(
		TEXT("AllEnemyParts Target Preview ignores the same destroyed optional focus"),
		DestroyedFocusPreview.bHasPreview);
	TestTrue(
		TEXT("Ignored destroyed focus does not invalidate structural preview"),
		DestroyedFocusPreview.Validation.bCanTarget);
	TestEqual(
		TEXT("Ignored destroyed focus produces no focus facts"),
		DestroyedFocusPreview.TargetKind,
		EWacomBattleCardPreviewTargetKind::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomPlayCardEvaluationStableKeyAuthoritySpec,
	"Wacom.Battle.PlayCardEvaluation.TargetProbe.UnresolvedRuntimeIdUsesStableKey",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomPlayCardEvaluationStableKeyAuthoritySpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* DamageCard = Fixture.MakeSimpleDamageCard(/*Cost*/0, /*Damage*/3);
	DamageCard->CardId = TEXT("PlayCardEvaluation.Identity.StableKeyAuthority");
	UBattleSession* Session = CreateEvaluationSession(
		Fixture,
		{ DamageCard },
		Fixture.MakeSinglePartEnemyWithIntentDamage(
			/*Hp*/20,
			/*Initiative*/10,
			/*IntentResist*/99,
			/*Damage*/0));

	const FBattleSnapshot Before = Session->BuildSnapshot();
	const FGuid CardId =
		FWacomBattleFixture::FindHandInstanceByCardId(Before, DamageCard->CardId);
	const FEnemyPartSnapshot* Part = FWacomBattleFixture::GetEnemyPartSnapshot(Before, 0);
	if (!TestTrue(
		TEXT("Stable-key fixture contains source card and enemy part"),
		CardId.IsValid() && Part))
	{
		return false;
	}

	const FWacomInteractionTargetHandle Handle =
		FWacomInteractionTargetHandle::ForWorldTarget(
			FGuid::NewGuid(),
			nullptr,
			FVector::ZeroVector,
			FVector2D::ZeroVector,
			FGameplayTag(),
			NAME_None,
			Part->EncounterId,
			Part->EnemySlotId,
			Part->PartSlotId);
	const FWacomBattleTargetValidationResult Probe =
		Session->ValidateTargetWithCard(CardId, Handle);
	TestTrue(TEXT("Unresolved runtime ID does not veto a valid stable key"), Probe.bCanTarget);
	TestEqual(TEXT("Probe resolves the stable-key part instance"), Probe.ResolvedPartInstanceId, Part->InstanceId);
	TestTrue(TEXT("Probe resolves the stable target key"), Probe.ResolvedPartKey == Part->PartKey);

	const FBattleCardTargetPreview Preview =
		Session->BuildCardTargetPreview(CardId, Handle);
	TestTrue(TEXT("Target Preview accepts the stable-key binding"), Preview.bHasPreview);
	TestEqual(
		TEXT("Target Preview binds the stable-key part"),
		Preview.TargetEnemyPartInstanceId,
		Part->InstanceId);
	TestTrue(TEXT("Target Preview exposes the stable key"), Preview.TargetEnemyPartKey == Part->PartKey);

	const FBattleResolution Status = Session->ResolveCommand(
		FBattleCommand::MakePlayCardOnEnemyPartKey(CardId, Probe.ResolvedPartKey));
	TestTrue(TEXT("Formal submit accepts the resolved stable key"), Status.IsOk());
	const FBattleSnapshot After = Session->BuildSnapshot();
	const FEnemyPartSnapshot* AfterPart =
		FWacomBattleFixture::GetEnemyPartSnapshot(After, 0);
	TestNotNull(TEXT("Target part remains after formal submit"), AfterPart);
	if (AfterPart)
	{
		TestEqual(TEXT("Formal submit damages the stable-key part"), AfterPart->CurrentHp, Part->CurrentHp - 3);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomPlayCardEvaluationTargetKindPrioritySpec,
	"Wacom.Battle.PlayCardEvaluation.TargetProbe.WrongKindPrecedesTargetIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomPlayCardEvaluationTargetKindPrioritySpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* EnemyPartCard = Fixture.MakeSimpleDamageCard(/*Cost*/0, /*Damage*/1);
	EnemyPartCard->CardId = TEXT("PlayCardEvaluation.Kind.EnemyPart");
	UCardDefinition* HandCard = Fixture.MakeHandCardCostModifierCard(
		/*Cost*/0,
		/*Magnitude*/1,
		/*bReduceCost*/false);
	HandCard->CardId = TEXT("PlayCardEvaluation.Kind.HandCard");
	UBattleSession* Session = CreateEvaluationSession(
		Fixture,
		{ EnemyPartCard, HandCard },
		Fixture.MakeSinglePartEnemyWithIntentDamage(
			/*Hp*/20,
			/*Initiative*/10,
			/*IntentResist*/99,
			/*Damage*/0));

	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid EnemyPartCardId =
		FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, EnemyPartCard->CardId);
	const FGuid HandCardId =
		FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, HandCard->CardId);
	if (!TestTrue(
		TEXT("Target-kind fixture contains both source cards"),
		EnemyPartCardId.IsValid() && HandCardId.IsValid()))
	{
		return false;
	}

	const FWacomBattleTargetValidationResult EnemyCardOnSelfCard =
		Session->ValidateTargetWithCard(
			EnemyPartCardId,
			FWacomInteractionTargetHandle::ForCardTarget(EnemyPartCardId, Session));
	TestFalse(TEXT("Enemy-part card rejects Card target kind"), EnemyCardOnSelfCard.bCanTarget);
	TestEqual(
		TEXT("Wrong Card kind wins over self-target identity"),
		EnemyCardOnSelfCard.RejectReason,
		EWacomBattleTargetRejectReason::UnsupportedCardTarget);

	const FWacomBattleTargetValidationResult HandCardOnMalformedWorld =
		Session->ValidateTargetWithCard(
			HandCardId,
			FWacomInteractionTargetHandle::ForWorldTarget(FGuid::NewGuid(), nullptr));
	TestFalse(TEXT("HandCard mode rejects World target kind"), HandCardOnMalformedWorld.bCanTarget);
	TestEqual(
		TEXT("Wrong World kind wins over malformed world identity"),
		HandCardOnMalformedWorld.RejectReason,
		EWacomBattleTargetRejectReason::UnsupportedWorldTarget);

	const FWacomBattleTargetValidationResult HandCardOnSelf =
		Session->ValidateTargetWithCard(
			HandCardId,
			FWacomInteractionTargetHandle::ForCardTarget(HandCardId, Session));
	TestFalse(TEXT("HandCard mode still rejects a same-kind self target"), HandCardOnSelf.bCanTarget);
	TestEqual(
		TEXT("Same-kind self target reports SelfTarget"),
		HandCardOnSelf.RejectReason,
		EWacomBattleTargetRejectReason::SelfTarget);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomPlayCardEvaluationCostParitySpec,
	"Wacom.Battle.PlayCardEvaluation.Cost.TargetPreviewRemainsStructural",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomPlayCardEvaluationCostParitySpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* ExpensiveCard = Fixture.MakeSimpleDamageCard(/*Cost*/5, /*Damage*/3);
	ExpensiveCard->CardId = TEXT("PlayCardEvaluation.Cost.Expensive");
	UBattleSession* Session = CreateEvaluationSession(
		Fixture,
		{ ExpensiveCard },
		Fixture.MakeSinglePartEnemyWithIntentDamage(
			/*Hp*/20,
			/*Initiative*/3,
			/*IntentResist*/99,
			/*Damage*/0));

	const FBattleSnapshot Before = Session->BuildSnapshot();
	const FGuid CardId =
		FWacomBattleFixture::FindHandInstanceByCardId(Before, ExpensiveCard->CardId);
	const FEnemyPartSnapshot* Part = FWacomBattleFixture::GetEnemyPartSnapshot(Before, 0);
	if (!TestTrue(TEXT("Cost fixture contains source card and target part"), CardId.IsValid() && Part))
	{
		return false;
	}

	const FWacomInteractionTargetHandle Target = MakePartHandle(*Part);
	const FBattleCardTargetPreview TargetPreview =
		Session->BuildCardTargetPreview(CardId, Target);
	TestTrue(TEXT("Insufficient initiative does not block Target Preview"), TargetPreview.bHasPreview);
	TestTrue(TEXT("Target Preview remains structurally valid"), TargetPreview.Validation.bCanTarget);

	const FBattleCardActionPreview ActionPreview =
		Session->BuildCardActionPreview(CardId, Target);
	TestFalse(TEXT("Insufficient initiative blocks Action Preview"), ActionPreview.bHasPreview);
	TestTrue(
		TEXT("Action Preview retains the structural target facts"),
		ActionPreview.TargetPreview.bHasPreview);
	TestFalse(
		TEXT("Action Preview marks the candidate as not currently committable"),
		ActionPreview.TargetPreview.Validation.bCanTarget);
	TestEqual(
		TEXT("Action Preview reports NotEnoughInitiative"),
		ActionPreview.TargetPreview.Validation.RejectReason,
		EWacomBattleTargetRejectReason::NotEnoughInitiative);

	const FBattleResolution Status = Session->ResolveCommand(
		FBattleCommand::MakePlayCardOnEnemyPartKey(CardId, Part->PartKey));
	TestRejectedStatus(
		*this,
		TEXT("Formal expensive PlayCard"),
		Status,
		EWacomError::NotEnoughInitiative,
		TEXT("CostExceedsInitiativeSum"));

	const FBattleSnapshot After = Session->BuildSnapshot();
	const FEnemyPartSnapshot* PartAfter = FWacomBattleFixture::GetEnemyPartSnapshot(After, 0);
	TestEqual(TEXT("Rejected cost keeps phase unchanged"), After.Phase, Before.Phase);
	TestEqual(TEXT("Rejected cost keeps hand size unchanged"), After.Hand.Cards.Num(), Before.Hand.Cards.Num());
	TestNotNull(TEXT("Rejected cost keeps target part"), PartAfter);
	if (PartAfter)
	{
		TestEqual(TEXT("Rejected cost keeps target HP unchanged"), PartAfter->CurrentHp, Part->CurrentHp);
		TestEqual(
			TEXT("Rejected cost keeps target initiative unchanged"),
			PartAfter->CurrentInitiative,
			Part->CurrentInitiative);
	}
	TestNotNull(
		TEXT("Rejected cost keeps source card in hand"),
		FWacomBattleFixture::FindHandCardByInstanceId(After, CardId));
	TestTrue(TEXT("Previews and rejected submit emit no events"), Status.Events.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomPlayCardEvaluationFormalStatusDetailSpec,
	"Wacom.Battle.PlayCardEvaluation.Commit.PreservesStatusDetails",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomPlayCardEvaluationFormalStatusDetailSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* EnemyPartCard = Fixture.MakeSimpleDamageCard(/*Cost*/0, /*Damage*/1);
	EnemyPartCard->CardId = TEXT("PlayCardEvaluation.Status.EnemyPart");
	UCardDefinition* HandTargetCard = Fixture.MakeHandCardCostModifierCard(
		/*Cost*/0,
		/*Magnitude*/1,
		/*bReduceCost*/false);
	HandTargetCard->CardId = TEXT("PlayCardEvaluation.Status.HandTarget");
	UCardDefinition* NoTargetCard = MakeEvaluationCard(
		Fixture,
		TEXT("PlayCardEvaluation.Status.NoTarget"),
		ECardTargetMode::None);
	UBattleSession* Session = CreateEvaluationSession(
		Fixture,
		{ EnemyPartCard, HandTargetCard, NoTargetCard },
		Fixture.MakeSinglePartEnemyWithIntentDamage(
			/*Hp*/20,
			/*Initiative*/10,
			/*IntentResist*/99,
			/*Damage*/0));

	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid EnemyPartCardId =
		FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, EnemyPartCard->CardId);
	const FGuid HandTargetCardId =
		FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, HandTargetCard->CardId);
	const FGuid NoTargetCardId =
		FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, NoTargetCard->CardId);
	if (!TestTrue(
		TEXT("Status fixture contains all source cards"),
		EnemyPartCardId.IsValid() && HandTargetCardId.IsValid() && NoTargetCardId.IsValid()))
	{
		return false;
	}

	TestRejectedStatus(
		*this,
		TEXT("Missing enemy target"),
		Session->ResolveCommand(FBattleCommand::MakePlayCard(EnemyPartCardId)),
		EWacomError::IllegalTarget,
		TEXT("MissingTarget"));

	TestRejectedStatus(
		*this,
		TEXT("Invalid enemy target key"),
		Session->ResolveCommand(FBattleCommand::MakePlayCardOnEnemyPartKey(
			EnemyPartCardId,
			FBattleEnemyPartKey::Make(TEXT("Encounter"), TEXT("Enemy"), TEXT("MissingPart")))),
		EWacomError::IllegalTarget,
		TEXT("TargetKeyInvalid"));

	TestRejectedStatus(
		*this,
		TEXT("Missing hand-card target"),
		Session->ResolveCommand(FBattleCommand::MakePlayCard(HandTargetCardId)),
		EWacomError::IllegalTarget,
		TEXT("MissingTargetCard"));

	TestRejectedStatus(
		*this,
		TEXT("Self hand-card target"),
		Session->ResolveCommand(FBattleCommand::MakePlayCardOnHandCard(
			HandTargetCardId,
			HandTargetCardId)),
		EWacomError::IllegalTarget,
		TEXT("SelfTargetCard"));

	TestRejectedStatus(
		*this,
		TEXT("Missing source instance"),
		Session->ResolveCommand(FBattleCommand::MakePlayCard(FGuid::NewGuid())),
		EWacomError::NotFound,
		TEXT("CardInstanceNotFound"));

	const FBattleResolution FirstPlay =
		Session->ResolveCommand(FBattleCommand::MakePlayCard(NoTargetCardId));
	TestTrue(TEXT("No-target card plays once to leave Hand"), FirstPlay.IsOk());
	TestRejectedStatus(
		*this,
		TEXT("Previously played source"),
		Session->ResolveCommand(FBattleCommand::MakePlayCard(NoTargetCardId)),
		EWacomError::IllegalCardZone,
		TEXT("CardNotInHand"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomPlayCardEvaluationCanonicalCommandSpec,
	"Wacom.Battle.PlayCardEvaluation.Commit.NormalizesUnusedTargetFields",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomPlayCardEvaluationCanonicalCommandSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* AllCard = MakeEvaluationCard(
		Fixture,
		TEXT("PlayCardEvaluation.Canonical.AllEnemyParts"),
		ECardTargetMode::AllEnemyParts);
	UCardDefinition* OtherCard = MakeEvaluationCard(
		Fixture,
		TEXT("PlayCardEvaluation.Canonical.Other"),
		ECardTargetMode::None);
	UBattleSession* Session = CreateEvaluationSession(
		Fixture,
		{ AllCard, OtherCard },
		Fixture.MakeSinglePartEnemyWithIntentDamage(
			/*Hp*/20,
			/*Initiative*/10,
			/*IntentResist*/99,
			/*Damage*/0));

	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid AllCardId =
		FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, AllCard->CardId);
	const FGuid OtherCardId =
		FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, OtherCard->CardId);
	const FEnemyPartSnapshot* Part = FWacomBattleFixture::GetEnemyPartSnapshot(Snapshot, 0);
	if (!TestTrue(
		TEXT("Canonical command fixture contains both cards and an enemy part"),
		AllCardId.IsValid() && OtherCardId.IsValid() && Part))
	{
		return false;
	}

	FBattleCommand Command =
		FBattleCommand::MakePlayCardOnEnemyPartKey(AllCardId, Part->PartKey);
	Command.TargetCardInstanceId = OtherCardId;
	const FBattleResolution Status = Session->ResolveCommand(Command);
	TestTrue(TEXT("AllEnemyParts accepts a command carrying unused target fields"), Status.IsOk());

	const TArray<FBattleEvent>& Events = Status.Events;
	const FBattleEvent* CardPlayed = Events.FindByPredicate(
		[AllCardId](const FBattleEvent& Event)
		{
			return Event.Type == EBattleEventType::CardPlayed
				&& Event.CardInstanceId == AllCardId;
		});
	TestNotNull(TEXT("Canonical command emits CardPlayed"), CardPlayed);
	if (CardPlayed)
	{
		TestFalse(
			TEXT("AllEnemyParts does not bind the supplied enemy-part focus"),
			CardPlayed->ActorInstanceId.IsValid());
		TestFalse(
			TEXT("AllEnemyParts clears the supplied stable target key"),
			CardPlayed->ActorEnemyPartKey.IsValidKey());
	}
	return true;
}
