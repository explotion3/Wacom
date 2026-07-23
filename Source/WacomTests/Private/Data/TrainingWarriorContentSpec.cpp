// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartAnimationStyle.h"
#include "Actors/WacomBattleSceneEnemyAuthoringReport.h"
#include "Data/EnemyHostComponentTestHelpers.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Cards/CardDefinition.h"
#include "Commands/BattleCommand.h"
#include "Encounters/EncounterDefinition.h"
#include "Engine/Blueprint.h"
#include "Engine/Texture2D.h"
#include "Enemies/EnemyBehaviorDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Events/BattleEvent.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Fixtures/GeneratedBattleContentTestAssets.h"
#include "Modules/ModuleManager.h"
#include "PaperFlipbook.h"
#include "PaperSprite.h"
#include "Session/BattleResultPacket.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "UObject/StrongObjectPtr.h"
#include "Validation/CardDefinitionValidation.h"
#include "Validation/EncounterDefinitionValidation.h"
#include "Validation/EnemyBehaviorDefinitionValidation.h"
#include "Validation/EnemyDefinitionValidation.h"
#include "Validation/EnemyPartDefinitionValidation.h"

namespace WacomTrainingWarriorContentSpec
{
	const TCHAR* EnemyPath =
		TEXT("/Game/Wacom/Data/Enemies/TrainingWarrior/DA_Enemy_TrainingWarrior.DA_Enemy_TrainingWarrior");
	const TCHAR* PartPath =
		TEXT("/Game/Wacom/Data/Enemies/TrainingWarrior/DA_Part_TrainingWarrior_Body.DA_Part_TrainingWarrior_Body");
	const TCHAR* BehaviorPath =
		TEXT("/Game/Wacom/Data/Enemies/TrainingWarrior/DA_Behavior_TrainingWarrior.DA_Behavior_TrainingWarrior");
	const TCHAR* EncounterPath =
		TEXT("/Game/Wacom/Data/Encounters/DA_Encounter_TrainingWarriorSingle.DA_Encounter_TrainingWarriorSingle");
	const TCHAR* AnimationStylePath =
		TEXT("/Game/Wacom/Data/Enemies/TrainingWarrior/DA_EnemyPartAnimation_TrainingWarrior.DA_EnemyPartAnimation_TrainingWarrior");
	const TCHAR* HostPath =
		TEXT("/Game/Wacom/Core/Enemy/BP_EnemyHost_TrainingWarrior.BP_EnemyHost_TrainingWarrior");
	const FName BodySequenceId(TEXT("TrainingWarrior.Body.Sequence"));

	template <typename T>
	T* LoadAsset(const TCHAR* Path)
	{
		return LoadObject<T>(nullptr, Path);
	}

	bool IsPackageUnderRoot(FName PackageName, const TCHAR* Root)
	{
		const FString PackageString = PackageName.ToString();
		const FString RootString(Root);
		return PackageString == RootString
			|| PackageString.StartsWith(RootString + TEXT("/"));
	}

	TStrongObjectPtr<UBattleSession> MakeTwoEnemySession(
		UCharacterDefinition* Character,
		UEnemyDefinition* Enemy,
		int32 Seed,
		bool& bOutInitialized)
	{
		TStrongObjectPtr<UBattleSession> Session(
			NewObject<UBattleSession>(GetTransientPackage()));
		FBattleInitParams Params;
		Params.Character = Character;
		Params.EncounterId = TEXT("Encounter.TrainingWarrior.Test");
		Params.RandomSeed = Seed;
		FBattleEnemySlotInit LeftSlot;
		LeftSlot.EnemySlotId = TEXT("Left");
		LeftSlot.Enemy = Enemy;
		FBattleEnemySlotInit RightSlot;
		RightSlot.EnemySlotId = TEXT("Right");
		RightSlot.Enemy = Enemy;
		Params.EnemySlots = { LeftSlot, RightSlot };
		bOutInitialized = Session->Initialize(Params).IsOk();
		return Session;
	}

	const FBattleEvent* FindFirstEnemyAction(
		const TArray<FBattleEvent>& Events)
	{
		return Events.FindByPredicate(
			[](const FBattleEvent& Event)
			{
				return Event.Type == EBattleEventType::EnemyPartActed;
			});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataTrainingWarriorAssetContractSpec,
	"Wacom.Data.Enemy.TrainingWarrior.AssetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataTrainingWarriorAssetContractSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomTrainingWarriorContentSpec;
	UCardDefinition* Card = FWacomGeneratedBattleContentAssets::LoadBrokenCleave(*this);
	UEnemyPartDefinition* Part = LoadAsset<UEnemyPartDefinition>(PartPath);
	UEnemyBehaviorDefinition* Behavior =
		LoadAsset<UEnemyBehaviorDefinition>(BehaviorPath);
	UEnemyDefinition* Enemy = LoadAsset<UEnemyDefinition>(EnemyPath);
	UEncounterDefinition* Encounter = LoadAsset<UEncounterDefinition>(EncounterPath);
	if (!TestNotNull(TEXT("TrainingWarrior reward card loads"), Card)
		|| !TestNotNull(TEXT("TrainingWarrior part loads"), Part)
		|| !TestNotNull(TEXT("TrainingWarrior behavior loads"), Behavior)
		|| !TestNotNull(TEXT("TrainingWarrior enemy loads"), Enemy)
		|| !TestNotNull(TEXT("TrainingWarrior encounter loads"), Encounter))
	{
		return false;
	}

	TArray<FText> Errors;
	TestTrue(TEXT("Reward card passes validation"),
		FWacomCardDefinitionValidation::Validate(Card, Errors));
	TestEqual(TEXT("Reward card validation errors"), Errors.Num(), 0);
	Errors.Reset();
	TestTrue(TEXT("Body part passes validation"),
		FWacomEnemyPartDefinitionValidation::Validate(Part, Errors));
	TestEqual(TEXT("Body part validation errors"), Errors.Num(), 0);
	Errors.Reset();
	TestTrue(TEXT("Enemy passes validation"),
		FWacomEnemyDefinitionValidation::Validate(Enemy, Errors));
	TestEqual(TEXT("Enemy validation errors"), Errors.Num(), 0);
	Errors.Reset();
	TestTrue(TEXT("Behavior passes validation"),
		FWacomEnemyBehaviorDefinitionValidation::Validate(Behavior, Errors, Enemy));
	TestEqual(TEXT("Behavior validation errors"), Errors.Num(), 0);
	Errors.Reset();
	TestTrue(TEXT("Encounter passes validation"),
		FWacomEncounterDefinitionValidation::Validate(Encounter, Errors));
	TestEqual(TEXT("Encounter validation errors"), Errors.Num(), 0);

	TestEqual(TEXT("Reward CardId"), Card->CardId, FName(TEXT("Reward.BrokenCleave")));
	TestEqual(TEXT("Reward display name"), Card->DisplayName.ToString(), FString(TEXT("残缺横斩")));
	TestEqual(TEXT("Reward cost"), Card->BaseCost, 1);
	TestTrue(TEXT("Reward rarity is White"), Card->Rarity == WacomTags::Card_Rarity_White);
	TestTrue(TEXT("Reward has Weapon keyword"),
		Card->Keywords.HasTagExact(WacomTags::Card_Keyword_Weapon));
	TestEqual(TEXT("Reward has only one keyword"), Card->Keywords.Num(), 1);
	TestEqual(TEXT("Reward target mode"), Card->TargetMode, ECardTargetMode::AllEnemyParts);
	TestNull(TEXT("Reward uses fallback illustration"), Card->CardIllustration.Get());
	TestEqual(TEXT("Reward has one effect"), Card->Effects.Num(), 1);
	if (Card->Effects.Num() == 1)
	{
		TestTrue(TEXT("Reward effect is damage"),
			Card->Effects[0].EffectType == WacomTags::Effect_Damage);
		TestEqual(TEXT("Reward damage"), Card->Effects[0].Magnitude, 3);
		TestTrue(TEXT("Reward effect targets all enemy parts"),
			Card->Effects[0].Target == WacomTags::Target_AllEnemyParts);
	}
	TestEqual(TEXT("Reward has no perfect release effects"), Card->PerfectReleaseEffects.Num(), 0);
	TestEqual(TEXT("Reward has no zone hooks"), Card->ZoneHooks.Num(), 0);
	TestEqual(TEXT("Reward has no passives"), Card->Passives.Num(), 0);

	TestEqual(TEXT("Body PartId"), Part->PartId, FName(TEXT("TrainingWarrior.Body")));
	TestEqual(TEXT("Body HP"), Part->MaxHp, 24);
	TestEqual(TEXT("Body experience"), Part->ExperienceReward, 3);
	TestNull(TEXT("Legacy Body reward field is cleared"),
		Part->KnockdownRewardCard.Get());
	TestTrue(TEXT("Body Aid reward references BrokenCleave"),
		Part->AidRewardCard.Get() == Card);
	TestTrue(TEXT("Body Destroy reward references BrokenCleave"),
		Part->DestroyRewardCard.Get() == Card);
	TestTrue(TEXT("Unified Aid query reads explicit BrokenCleave"),
		Part->ResolveKnockdownRewardCard(EKnockdownChoice::Aid) == Card);
	TestTrue(TEXT("Unified Destroy query reads explicit BrokenCleave"),
		Part->ResolveKnockdownRewardCard(EKnockdownChoice::Destroy) == Card);
	TestNull(TEXT("Unified Withdraw query returns no reward"),
		Part->ResolveKnockdownRewardCard(EKnockdownChoice::Withdraw));

	TestEqual(TEXT("BehaviorId"), Behavior->BehaviorId, FName(TEXT("TrainingWarrior.Behavior")));
	TestEqual(TEXT("Behavior initial phase"), Behavior->InitialPhaseId, FName(TEXT("Default")));
	TestEqual(TEXT("Behavior phase count"), Behavior->Phases.Num(), 1);
	if (Behavior->Phases.Num() == 1)
	{
		const FWacomEnemyPhaseDefinition& Phase = Behavior->Phases[0];
		TestEqual(TEXT("Default phase id"), Phase.PhaseId, FName(TEXT("Default")));
		TestEqual(TEXT("Intent set count"), Phase.IntentSets.Num(), 1);
		if (Phase.IntentSets.Num() == 1)
		{
			const FWacomEnemyIntentSetDefinition& IntentSet = Phase.IntentSets[0];
			TestEqual(TEXT("Body sequence id"), IntentSet.IntentSetId, BodySequenceId);
			TestEqual(TEXT("Body sequence slot"), IntentSet.AppliesToPartSlotId, FName(TEXT("Body")));
			TestEqual(TEXT("Body sequence mode"), IntentSet.SelectorMode, EWacomEnemyIntentSelectorMode::Sequence);
			TestEqual(TEXT("Body sequence intent count"), IntentSet.Intents.Num(), 3);
			const TArray<FName> ExpectedIds = {
				TEXT("TrainingWarrior.Body.Attack"),
				TEXT("TrainingWarrior.Body.Guard"),
				TEXT("TrainingWarrior.Body.Cleave")
			};
			const TArray<int32> ExpectedInitiatives = { 3, 2, 4 };
			for (int32 Index = 0; Index < IntentSet.Intents.Num(); ++Index)
			{
				TestEqual(FString::Printf(TEXT("Intent %d id"), Index),
					IntentSet.Intents[Index].Intent.IntentId, ExpectedIds[Index]);
				TestEqual(FString::Printf(TEXT("Intent %d initiative"), Index),
					IntentSet.Intents[Index].Intent.Initiative, ExpectedInitiatives[Index]);
			}
		}
	}

	TestEqual(TEXT("EnemyId"), Enemy->EnemyId, FName(TEXT("Enemy.TrainingWarrior")));
	TestTrue(TEXT("Enemy references behavior"), Enemy->DefaultBehavior.Get() == Behavior);
	TestEqual(TEXT("Enemy has one part slot"), Enemy->Parts.Num(), 1);
	if (Enemy->Parts.Num() == 1)
	{
		TestEqual(TEXT("Enemy Body PartSlotId"), Enemy->Parts[0].PartSlotId, FName(TEXT("Body")));
		TestTrue(TEXT("Enemy Body references part"), Enemy->Parts[0].PartDef.Get() == Part);
		TestEqual(TEXT("Enemy Body initial intent set"), Enemy->Parts[0].InitialIntentSetId, BodySequenceId);
	}
	TestEqual(TEXT("Encounter id"), Encounter->EncounterDefinitionId,
		FName(TEXT("Encounter.TrainingWarrior.Single")));
	TestEqual(TEXT("Encounter has one enemy"), Encounter->EnemySlots.Num(), 1);
	if (Encounter->EnemySlots.Num() == 1)
	{
		TestEqual(TEXT("Encounter enemy slot"), Encounter->EnemySlots[0].EnemySlotId, FName(TEXT("Enemy")));
		TestTrue(TEXT("Encounter references TrainingWarrior"),
			Encounter->EnemySlots[0].EnemyDefinition.Get() == Enemy);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataTrainingWarriorAllEnemyPartsRuntimeSpec,
	"Wacom.Data.Enemy.TrainingWarrior.BrokenCleaveDamagesEveryLivingPart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataTrainingWarriorAllEnemyPartsRuntimeSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomTrainingWarriorContentSpec;
	FWacomBattleFixture Fixture;
	UCardDefinition* RewardCard = FWacomGeneratedBattleContentAssets::LoadBrokenCleave(*this);
	UEnemyDefinition* Enemy = LoadAsset<UEnemyDefinition>(EnemyPath);
	if (!RewardCard || !Enemy)
	{
		return false;
	}
	UCardDefinition* LeftHand = Fixture.MakeNoopCard(0);
	UCardDefinition* RightHand = Fixture.MakeNoopCard(0);
	UCharacterDefinition* Character = Fixture.MakeCharacter(
		LeftHand, RightHand,
		{ RewardCard, RewardCard, RewardCard, RewardCard, RewardCard });
	bool bInitialized = false;
	TStrongObjectPtr<UBattleSession> Session = MakeTwoEnemySession(
		Character, Enemy, 17, bInitialized);
	if (!TestTrue(TEXT("Two TrainingWarrior session initializes"), bInitialized))
	{
		return false;
	}

	const FBattleSnapshot Before = Session->BuildSnapshot();
	const FGuid CardInstanceId =
		FWacomBattleFixture::FindHandInstanceByCardId(Before, RewardCard->CardId);
	if (!TestTrue(TEXT("BrokenCleave is drawn"), CardInstanceId.IsValid()))
	{
		return false;
	}
	const FBattleResolution Resolution = Session->ResolveCommand(
		FBattleCommand::MakePlayCard(CardInstanceId));
	TestTrue(TEXT("BrokenCleave requires no single target"), Resolution.IsOk());
	const FBattleSnapshot After = Session->BuildSnapshot();
	const FEnemyPartSnapshot* LeftPart =
		FWacomBattleFixture::GetEnemyPartSnapshot(After, 0, 0);
	const FEnemyPartSnapshot* RightPart =
		FWacomBattleFixture::GetEnemyPartSnapshot(After, 1, 0);
	if (TestNotNull(TEXT("Left Body snapshot"), LeftPart)
		&& TestNotNull(TEXT("Right Body snapshot"), RightPart))
	{
		TestEqual(TEXT("Left living Body loses 3 HP"), LeftPart->CurrentHp, 21);
		TestEqual(TEXT("Right living Body loses 3 HP"), RightPart->CurrentHp, 21);
	}
	TestEqual(TEXT("Two DamageDealt events emitted"),
		FWacomBattleFixture::CountEvents(
			Resolution.Events, EBattleEventType::DamageDealt), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataTrainingWarriorBehaviorRuntimeSpec,
	"Wacom.Data.Enemy.TrainingWarrior.AttackGuardCleaveSequence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataTrainingWarriorBehaviorRuntimeSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomTrainingWarriorContentSpec;
	FWacomBattleFixture Fixture;
	UEnemyDefinition* Enemy = LoadAsset<UEnemyDefinition>(EnemyPath);
	if (!TestNotNull(TEXT("TrainingWarrior enemy loads"), Enemy))
	{
		return false;
	}
	UCardDefinition* LeftHand = Fixture.MakeNoopCard(0);
	UCardDefinition* RightHand = Fixture.MakeNoopCard(0);
	UCardDefinition* DeckCard = Fixture.MakeNoopCard(0);
	UCharacterDefinition* Character = Fixture.MakeCharacter(
		LeftHand, RightHand,
		{ DeckCard, DeckCard, DeckCard, DeckCard, DeckCard });
	UBattleSession* Session = Fixture.CreateSession(Character, Enemy, 23);

	const TArray<FName> ExpectedActions = {
		TEXT("TrainingWarrior.Body.Attack"),
		TEXT("TrainingWarrior.Body.Guard"),
		TEXT("TrainingWarrior.Body.Cleave"),
		TEXT("TrainingWarrior.Body.Attack"),
	};
	for (int32 Index = 0; Index < ExpectedActions.Num(); ++Index)
	{
		const FBattleResolution Resolution = Session->ResolveCommand(
			FBattleCommand::MakeEndTurn());
		TestTrue(FString::Printf(TEXT("EndTurn %d resolves"), Index + 1),
			Resolution.IsOk());
		const FBattleEvent* Action = FindFirstEnemyAction(Resolution.Events);
		if (TestNotNull(
			FString::Printf(TEXT("EndTurn %d emits EnemyPartActed"), Index + 1),
			Action))
		{
			TestEqual(
				FString::Printf(TEXT("EndTurn %d action"), Index + 1),
				Action->IntentId,
				ExpectedActions[Index]);
		}
	}
	const FBattleSnapshot After = Session->BuildSnapshot();
	TestEqual(TEXT("Attack + Cleave + Attack deal 15 player damage"),
		After.Player.CurrentHp, 85);
	const FEnemyPartSnapshot* Body =
		FWacomBattleFixture::GetEnemyPartSnapshot(After, 0);
	if (TestNotNull(TEXT("TrainingWarrior Body snapshot"), Body))
	{
		TestEqual(TEXT("Guard grants 4 shield"), Body->Shield, 4);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataTrainingWarriorRewardChoicesSpec,
	"Wacom.Data.Enemy.TrainingWarrior.KnockdownRewardChoices",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataTrainingWarriorRewardChoicesSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomTrainingWarriorContentSpec;
	UEnemyDefinition* Enemy = LoadAsset<UEnemyDefinition>(EnemyPath);
	UCardDefinition* RewardCard = FWacomGeneratedBattleContentAssets::LoadBrokenCleave(*this);
	if (!Enemy || !RewardCard)
	{
		return false;
	}

	const TArray<EKnockdownChoice> Choices = {
		EKnockdownChoice::Aid,
		EKnockdownChoice::Destroy,
		EKnockdownChoice::Withdraw,
	};
	for (EKnockdownChoice Choice : Choices)
	{
		FWacomBattleFixture Fixture;
		UCardDefinition* Killer = Fixture.MakeSimpleDamageCard(0, 50);
		UCharacterDefinition* Character = Fixture.MakeCharacter(
			Fixture.MakeNoopCard(0),
			Fixture.MakeNoopCard(0),
			{ Killer, Killer, Killer, Killer, Killer });
		bool bInitialized = false;
		TStrongObjectPtr<UBattleSession> Session = MakeTwoEnemySession(
			Character, Enemy, 31 + static_cast<int32>(Choice), bInitialized);
		if (!TestTrue(TEXT("Reward choice session initializes"), bInitialized))
		{
			continue;
		}
		const FBattleSnapshot Before = Session->BuildSnapshot();
		const FGuid KillerInstanceId =
			FWacomBattleFixture::FindHandInstanceByCardId(Before, Killer->CardId);
		const FEnemyPartSnapshot* Target =
			FWacomBattleFixture::GetEnemyPartSnapshot(Before, 0, 0);
		if (!KillerInstanceId.IsValid()
			|| !TestNotNull(TEXT("Reward choice target"), Target))
		{
			continue;
		}
		const FBattleResolution KillResult = Session->ResolveCommand(
			FBattleCommand::MakePlayCardOnEnemyPartKey(
				KillerInstanceId, Target->PartKey));
		TestTrue(TEXT("TrainingWarrior Body is knocked down"), KillResult.IsOk());
		TestEqual(TEXT("Knockdown enters pending choice"),
			Session->GetPhase(), EBattlePhase::PendingKnockdownChoice);
		const FBattleResolution ChoiceResult = Session->ResolveCommand(
			FBattleCommand::MakeKnockdownChoice(Choice));
		TestTrue(TEXT("TrainingWarrior knockdown choice resolves"),
			ChoiceResult.IsOk());
		const FBattleResultPacket Packet = Session->BuildResultPacket();
		const int32 ExpectedRewardCount =
			Choice == EKnockdownChoice::Withdraw ? 0 : 1;
		TestEqual(
			FString::Printf(TEXT("Choice %d reward count"), static_cast<int32>(Choice)),
			Packet.GainedCards.Num(),
			ExpectedRewardCount);
		if (ExpectedRewardCount == 1 && Packet.GainedCards.Num() == 1)
		{
			TestTrue(TEXT("Reward definition is BrokenCleave"),
				Packet.GainedCards[0].Definition.Get() == RewardCard);
			TestEqual(TEXT("Reward source choice is preserved"),
				Packet.GainedCards[0].SourceChoice, Choice);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataTrainingWarriorHostAndArtSpec,
	"Wacom.Data.Enemy.TrainingWarrior.HostAndFormalArt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataTrainingWarriorHostAndArtSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomTrainingWarriorContentSpec;
	UBlueprint* HostBlueprint = LoadAsset<UBlueprint>(HostPath);
	UWacomBattleEnemyPartAnimationStyle* Style =
		LoadAsset<UWacomBattleEnemyPartAnimationStyle>(AnimationStylePath);
	if (!TestNotNull(TEXT("TrainingWarrior Host Blueprint loads"), HostBlueprint)
		|| !TestNotNull(TEXT("TrainingWarrior animation Style loads"), Style)
		|| !TestNotNull(TEXT("TrainingWarrior Host generated class"),
			HostBlueprint ? HostBlueprint->GeneratedClass.Get() : nullptr))
	{
		return false;
	}
	AWacomBattleEnemyActor* Host = Cast<AWacomBattleEnemyActor>(
		HostBlueprint->GeneratedClass->GetDefaultObject());
	if (!TestNotNull(TEXT("TrainingWarrior Host CDO"), Host))
	{
		return false;
	}
	TestEqual(TEXT("Host enemy slot"), Host->EnemySlotId, FName(TEXT("Enemy")));
	TestNotNull(TEXT("Host formal impact Style"), Host->DefaultImpactStyle.Get());
	TestNotNull(TEXT("Host formal target preview Style"),
		Host->DefaultTargetPreviewStyle.Get());

	const FWacomBattleSceneEnemyHostAuthoringReport Report =
		FWacomBattleSceneEnemyHostAuthoringEvaluator::Build(*Host);
	TestTrue(TEXT("Host authoring report is Ready"), Report.bAuthoringReady);
	TestEqual(TEXT("Host has one Body Part component"), Report.PartComponentCount, 1);
	const Wacom::Tests::EnemyHostComponents::FPartTemplates Body =
		Wacom::Tests::EnemyHostComponents::Find(*HostBlueprint, TEXT("Body"));
	if (TestNotNull(TEXT("Host Body Part component"), Body.Part)
		&& TestNotNull(TEXT("Host Body Flipbook layer"), Body.Flipbook)
		&& TestNotNull(TEXT("Host Body ImpactAnchor"), Body.ImpactAnchor))
	{
		TestEqual(TEXT("Host PartSlotId"), Body.Part->PartSlotId, FName(TEXT("Body")));
		TestEqual(TEXT("Host derived PartId"), Body.Part->PartId,
			FName(TEXT("TrainingWarrior.Body")));
		TestEqual(TEXT("Host Body ImpactAnchor has no offset"),
			Body.ImpactAnchor->GetRelativeLocation(), FVector::ZeroVector);
		TestTrue(TEXT("Body owns semantic animation Style"),
			Body.Part->PartAnimationStyle.Get() == Style);
		TestNotNull(TEXT("Body Idle Flipbook"), Body.Flipbook->GetFlipbook());
		TestEqual(TEXT("Body stable LayerId"), Body.Flipbook->LayerId,
			FName(TEXT("TrainingWarrior.Body.Main")));
		TestEqual(TEXT("Body Idle play rate"), Body.Flipbook->GetPlayRate(), 1.0f);
		TestTrue(TEXT("Body Idle loops"), Body.Flipbook->IsLooping());
	}

	const FWacomBattleEnemyPartAnimationClip* Attack =
		Style->ResolveActionClip(TEXT("TrainingWarrior.Body.Attack"));
	const FWacomBattleEnemyPartAnimationClip* Guard =
		Style->ResolveActionClip(TEXT("TrainingWarrior.Body.Guard"));
	const FWacomBattleEnemyPartAnimationClip* Cleave =
		Style->ResolveActionClip(TEXT("TrainingWarrior.Body.Cleave"));
	const FWacomBattleEnemyPartAnimationClip* Destroyed =
		Style->ResolveEnemyDestroyedClip();
	if (TestNotNull(TEXT("Attack resolves through Default Action"), Attack)
		&& TestNotNull(TEXT("Guard resolves explicit Block"), Guard)
		&& TestNotNull(TEXT("Cleave resolves explicit Cleave"), Cleave)
		&& TestNotNull(TEXT("Destroyed clip resolves"), Destroyed))
	{
		TestTrue(TEXT("Attack is the default clip"),
			Attack == &Style->DefaultActionClip);
		TestFalse(TEXT("Attack has no explicit intent mapping"),
			Style->ActionClipsByIntentId.Contains(TEXT("TrainingWarrior.Body.Attack")));
		TestEqual(TEXT("Attack play rate"), Attack->PlayRate, 0.75f);
		TestEqual(TEXT("Guard play rate"), Guard->PlayRate, 1.0f);
		TestEqual(TEXT("Cleave play rate"), Cleave->PlayRate, 0.75f);
		TestEqual(TEXT("Destroyed play rate"), Destroyed->PlayRate, 0.75f);
		for (const FWacomBattleEnemyPartAnimationClip* Clip :
			{ Attack, Guard, Cleave, Destroyed })
		{
			TestTrue(TEXT("Semantic clip is runtime usable"), Clip->IsRuntimeUsable());
			TestTrue(TEXT("Semantic clip has positive duration"),
				Clip->Flipbook && Clip->Flipbook->GetTotalDuration() > 0.0f);
		}
	}

	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	AssetRegistry.SearchAllAssets(/*bSynchronousSearch*/ true);
	const FName FormalRoot(TEXT("/Game/Wacom/Art/Enemies/TrainingWarrior"));
	TArray<FAssetData> FormalAssets;
	AssetRegistry.GetAssetsByPath(
		FormalRoot, FormalAssets,
		/*bRecursive*/ true,
		/*bIncludeOnlyOnDiskAssets*/ true);
	TestEqual(TEXT("Formal art contains exactly the selected dependency closure"),
		FormalAssets.Num(), 36);
	int32 FlipbookCount = 0;
	int32 SpriteCount = 0;
	int32 TextureCount = 0;
	for (const FAssetData& Asset : FormalAssets)
	{
		FlipbookCount += Asset.AssetClassPath ==
			UPaperFlipbook::StaticClass()->GetClassPathName();
		SpriteCount += Asset.AssetClassPath ==
			UPaperSprite::StaticClass()->GetClassPathName();
		TextureCount += Asset.AssetClassPath ==
			UTexture2D::StaticClass()->GetClassPathName();

		TArray<FAssetDependency> Dependencies;
		AssetRegistry.GetDependencies(
			FAssetIdentifier(Asset.PackageName),
			Dependencies,
			UE::AssetRegistry::EDependencyCategory::Package);
		for (const FAssetDependency& Dependency : Dependencies)
		{
			const FName PackageName = Dependency.AssetId.PackageName;
			TestFalse(TEXT("Formal art does not depend on /Game/Art"),
				IsPackageUnderRoot(PackageName, TEXT("/Game/Art")));
			TestFalse(TEXT("Formal art does not depend on /Game/Asset"),
				IsPackageUnderRoot(PackageName, TEXT("/Game/Asset")));
			TestFalse(TEXT("Formal art does not depend on /Game/DreamMaterials"),
				IsPackageUnderRoot(PackageName, TEXT("/Game/DreamMaterials")));
		}
	}
	TestEqual(TEXT("Formal art Flipbook count"), FlipbookCount, 5);
	TestEqual(TEXT("Formal art Sprite count"), SpriteCount, 30);
	TestEqual(TEXT("Formal art Texture count"), TextureCount, 1);
	return true;
}
