// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Battle/BattleSessionTestAccess.h"
#include "Cards/CardDefinition.h"
#include "Commands/BattleCommand.h"
#include "Events/BattleEvent.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattlePileInspectionSnapshot.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"

#include "UObject/StrongObjectPtr.h"

namespace WacomBattleCardRuntimeLifecycleSpec
{
	UCardDefinition* LoadFireflySeed()
	{
		return LoadObject<UCardDefinition>(
			nullptr,
			TEXT("/Game/Wacom/Data/Cards/FireWrite/"
				"DA_Card_FireflySeed.DA_Card_FireflySeed"));
	}

	TStrongObjectPtr<UBattleSession> CreateEntrySession(
		FWacomBattleFixture& Fixture,
		const TArray<FBattleDeckEntry>& Entries,
		const int32 Seed = 81)
	{
		TStrongObjectPtr<UBattleSession> Session(
			NewObject<UBattleSession>());
		FBattleInitParams Params;
		Params.Character = Fixture.MakeCharacter(
			Fixture.MakeNoopCard(0),
			Fixture.MakeNoopCard(0),
			{});
		Params.RandomSeed = Seed;
		Params.BattleDeckEntries = Entries;
		FBattleEnemySlotInit EnemySlot;
		EnemySlot.EnemySlotId = TEXT("Enemy");
		EnemySlot.Enemy =
			Fixture.MakeSinglePartEnemyWithIntentDamage(200, 20, 0);
		Params.EnemySlots.Add(EnemySlot);
		const FBattleInitializationResult Initialization =
			Session->Initialize(Params);
		check(Initialization.IsOk());
		return Session;
	}

	FBattleDeckEntry MakeEntry(
		UCardDefinition* Definition,
		const EWacomCardUpgradeTier Tier =
			EWacomCardUpgradeTier::White)
	{
		FBattleDeckEntry Entry;
		Entry.Definition = Definition;
		Entry.UpgradeTier = Tier;
		return Entry;
	}

	const FBattlePileCardSnapshot* FindPileCard(
		const FBattlePileInspectionSnapshot& Snapshot,
		const FGuid& InstanceId)
	{
		for (const FBattlePileInspectionSectionSnapshot& Section :
			Snapshot.Sections)
		{
			for (const FBattlePileCardSnapshot& Card : Section.Cards)
			{
				if (Card.InstanceId == InstanceId)
				{
					return &Card;
				}
			}
		}
		return nullptr;
	}

	TStrongObjectPtr<UCardDefinition> MakeFourTierCard(
		const FName CardId,
		const int32 Cost,
		const int32 Durability,
		const int32 MaxHpBonus = 0)
	{
		TStrongObjectPtr<UCardDefinition> Card(
			NewObject<UCardDefinition>());
		Card->CardId = CardId;
		Card->DisplayName = FText::FromName(CardId);
		Card->Rarity = WacomTags::Card_Rarity_White;
		Card->TargetMode = ECardTargetMode::None;
		for (int32 Tier = 0; Tier < WacomCardUpgrade::TierCount; ++Tier)
		{
			FWacomCardTierProfile& Profile =
				Card->TierProfiles.AddDefaulted_GetRef();
			Profile.Description = Card->DisplayName;
			Profile.BaseCost = Cost;
			Profile.Physique.Durability = Durability;
			Profile.Physique.MaxHpBonus = MaxHpBonus * (Tier + 1);
		}
		return Card;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCardRuntimeCloneBeforeDurabilitySpec,
	"Wacom.Battle.CardRuntimeLifecycle.CloneBeforeDurabilityAndIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCardRuntimeCloneBeforeDurabilitySpec::RunTest(
	const FString&)
{
	using namespace WacomBattleCardRuntimeLifecycleSpec;
	FWacomBattleFixture Fixture;
	UCardDefinition* SeedCard = LoadFireflySeed();
	if (!TestNotNull(TEXT("Firefly Seed asset loads"), SeedCard))
	{
		return false;
	}
	TArray<FBattleDeckEntry> Entries;
	FBattleDeckEntry SeedEntry =
		MakeEntry(SeedCard, EWacomCardUpgradeTier::Blue);
	SeedEntry.SourceRunInstanceId = FGuid::NewGuid();
	SeedEntry.PersistentModifiers.DurabilityBonus = 0;
	SeedEntry.PersistentModifiers.EffectMagnitudeBonuses.Add(
		WacomTags::Effect_ApplyStatus_Burn, 2);
	Entries.Add(SeedEntry);
	for (int32 Index = 0; Index < 4; ++Index)
	{
		Entries.Add(MakeEntry(Fixture.MakeNoopCard(0)));
	}
	TStrongObjectPtr<UBattleSession> Session =
		CreateEntrySession(Fixture, Entries);
	const FBattleSnapshot Before = Session->BuildSnapshot();
	const FGuid SourceId =
		FWacomBattleFixture::FindHandInstanceByCardId(
			Before, SeedCard->CardId);
	TestTrue(TEXT("Seed card is in opening hand"), SourceId.IsValid());
	TestTrue(TEXT("Source receives card Burn"),
		FWacomBattleSessionTestAccess::SetCardStatusStacks(
			Session.Get(), SourceId, WacomTags::Status_Burn, 2));
	TestTrue(TEXT("Source receives cost modifier"),
		FWacomBattleSessionTestAccess::SetCardRuntimeCostModifier(
			Session.Get(), SourceId, -1));
	TestTrue(TEXT("Source receives critical bonus"),
		FWacomBattleSessionTestAccess::SetCardCriticalChanceBonus(
			Session.Get(), SourceId, 75));

	const FBattleResolution Resolution = Session->ResolveCommand(
		FWacomBattleFixture::MakePlayCardOnPart(
			Before, SourceId, 0));
	TestTrue(TEXT("Firefly Seed resolves"), Resolution.IsOk());
	const FBattleEvent* Created = Resolution.Events.FindByPredicate(
		[](const FBattleEvent& Event)
		{
			return Event.Type == EBattleEventType::CardCreated;
		});
	if (!TestNotNull(TEXT("Clone event is emitted"), Created))
	{
		return false;
	}
	const FGuid CloneId = Created->CardInstanceId;
	TestTrue(TEXT("Clone receives a new identity"),
		CloneId.IsValid() && CloneId != SourceId);

	FRuntimeCardInstance SourceRuntime;
	FRuntimeCardInstance CloneRuntime;
	TestTrue(TEXT("Source runtime remains inspectable"),
		FWacomBattleSessionTestAccess::GetCardRuntimeState(
			Session.Get(), SourceId, SourceRuntime));
	TestTrue(TEXT("Clone runtime is inspectable"),
		FWacomBattleSessionTestAccess::GetCardRuntimeState(
			Session.Get(), CloneId, CloneRuntime));
	TestTrue(TEXT("Source durability exhausts after cloning"),
		SourceRuntime.bHasFiniteDurability
			&& SourceRuntime.CurrentDurability == 0
			&& SourceRuntime.Location == ECardLocation::Exhaust
			&& SourceRuntime.bEverEnteredExhaust);
	TestTrue(TEXT("Clone captures pre-deduction durability"),
		CloneRuntime.bHasFiniteDurability
			&& CloneRuntime.CurrentDurability == 1);
	TestTrue(TEXT("Clone enters draw pile"),
		CloneRuntime.Location == ECardLocation::Draw);
	TestTrue(TEXT("Clone inherits tier"),
		CloneRuntime.UpgradeTier == EWacomCardUpgradeTier::Blue);
	TestFalse(TEXT("Clone never inherits Run identity"),
		CloneRuntime.SourceRunInstanceId.IsValid());
	TestEqual(TEXT("Clone inherits runtime cost modifier"),
		CloneRuntime.RuntimeCostModifier, -1);
	TestEqual(TEXT("Clone inherits card Burn"),
		FWacomBattleFixture::GetStatusStacks(
			CloneRuntime.StatusStacks, WacomTags::Status_Burn),
		2);
	TestEqual(TEXT("Clone inherits critical bonus"),
		CloneRuntime.CriticalChanceBonusPercent, 75);
	TestEqual(TEXT("Clone inherits persistent durability projection"),
		CloneRuntime.PersistentModifiers.DurabilityBonus, 0);
	TestEqual(TEXT("Clone inherits persistent Burn bonus"),
		CloneRuntime.PersistentModifiers.EffectMagnitudeBonuses.FindRef(
			WacomTags::Effect_ApplyStatus_Burn),
		2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCardRuntimeDurabilityOverridesComboSpec,
	"Wacom.Battle.CardRuntimeLifecycle.DurabilityOverridesCombo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCardRuntimeDurabilityOverridesComboSpec::RunTest(
	const FString&)
{
	using namespace WacomBattleCardRuntimeLifecycleSpec;
	FWacomBattleFixture Fixture;
	TStrongObjectPtr<UCardDefinition> Card =
		MakeFourTierCard(TEXT("Card.Test.DurableCombo"), 0, 2);
	Card->Keywords.AddTag(WacomTags::Card_Keyword_Combo);
	TArray<FBattleDeckEntry> Entries = {
		MakeEntry(Card.Get()),
		MakeEntry(Fixture.MakeNoopCard(0)),
		MakeEntry(Fixture.MakeNoopCard(0)),
		MakeEntry(Fixture.MakeNoopCard(0)),
		MakeEntry(Fixture.MakeNoopCard(0)),
	};
	TStrongObjectPtr<UBattleSession> Session =
		CreateEntrySession(Fixture, Entries);
	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId =
		FWacomBattleFixture::FindHandInstanceByCardId(
			Snapshot, Card->CardId);
	TestTrue(TEXT("Durable Combo is in hand"), CardId.IsValid());

	TestTrue(TEXT("First successful play resolves"),
		Session->ResolveCommand(
			FBattleCommand::MakePlayCard(CardId)).IsOk());
	FRuntimeCardInstance Runtime;
	TestTrue(TEXT("Runtime after first play exists"),
		FWacomBattleSessionTestAccess::GetCardRuntimeState(
			Session.Get(), CardId, Runtime));
	TestEqual(TEXT("First play consumes one durability"),
		Runtime.CurrentDurability, 1);
	TestTrue(TEXT("Combo returns finite card to hand"),
		Runtime.Location == ECardLocation::Hand);

	TestTrue(TEXT("Second successful play resolves"),
		Session->ResolveCommand(
			FBattleCommand::MakePlayCard(CardId)).IsOk());
	TestTrue(TEXT("Runtime after second play exists"),
		FWacomBattleSessionTestAccess::GetCardRuntimeState(
			Session.Get(), CardId, Runtime));
	TestEqual(TEXT("Second play consumes final durability"),
		Runtime.CurrentDurability, 0);
	TestTrue(TEXT("Zero durability overrides Combo destination"),
		Runtime.Location == ECardLocation::Exhaust);
	TestTrue(TEXT("Exhaust history is latched"),
		Runtime.bEverEnteredExhaust);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCardRuntimeNamedCreationSpec,
	"Wacom.Battle.CardRuntimeLifecycle.NamedCreationTierBurnAndCompanionHp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCardRuntimeNamedCreationSpec::RunTest(const FString&)
{
	using namespace WacomBattleCardRuntimeLifecycleSpec;
	FWacomBattleFixture Fixture;
	TStrongObjectPtr<UCardDefinition> Companion =
		MakeFourTierCard(
			TEXT("Card.Test.GeneratedCompanion"),
			0,
			0,
			5);
	Companion->Keywords.AddTag(WacomTags::Card_Keyword_Companion);
	TStrongObjectPtr<UCardDefinition> Generator =
		MakeFourTierCard(TEXT("Card.Test.Generator"), 0, 0);
	for (FWacomCardTierProfile& Profile : Generator->TierProfiles)
	{
		FCardEffect Effect;
		Effect.EffectType = WacomTags::Effect_Card_GenerateToHand;
		Effect.Magnitude = 1;
		Effect.Target = WacomTags::Target_Self;
		Effect.CardPool.Add(Companion.Get());
		Profile.Effects.Add(Effect);
	}
	TArray<FBattleDeckEntry> Entries = {
		MakeEntry(Generator.Get(), EWacomCardUpgradeTier::Yellow),
		MakeEntry(Fixture.MakeNoopCard(0)),
		MakeEntry(Fixture.MakeNoopCard(0)),
		MakeEntry(Fixture.MakeNoopCard(0)),
		MakeEntry(Fixture.MakeNoopCard(0)),
	};
	TStrongObjectPtr<UBattleSession> Session =
		CreateEntrySession(Fixture, Entries);
	const FBattleSnapshot Before = Session->BuildSnapshot();
	const FGuid GeneratorId =
		FWacomBattleFixture::FindHandInstanceByCardId(
			Before, Generator->CardId);
	TestTrue(TEXT("Generator exists"), GeneratorId.IsValid());
	TestTrue(TEXT("Seed player Burn"),
		FWacomBattleSessionTestAccess::SetPlayerStatusStacks(
			Session.Get(), WacomTags::Status_Burn, 2));

	const FBattleResolution Resolution = Session->ResolveCommand(
		FBattleCommand::MakePlayCard(GeneratorId));
	TestTrue(TEXT("Generator play resolves"), Resolution.IsOk());
	const FBattleEvent* Created = Resolution.Events.FindByPredicate(
		[](const FBattleEvent& Event)
		{
			return Event.Type == EBattleEventType::CardCreated;
		});
	if (!TestNotNull(TEXT("Named creation emits event"), Created))
	{
		return false;
	}
	FRuntimeCardInstance Generated;
	TestTrue(TEXT("Generated runtime exists"),
		FWacomBattleSessionTestAccess::GetCardRuntimeState(
			Session.Get(), Created->CardInstanceId, Generated));
	TestTrue(TEXT("Named card inherits only source tier"),
		Generated.UpgradeTier == EWacomCardUpgradeTier::Yellow);
	TestFalse(TEXT("Named generated card has no Run identity"),
		Generated.SourceRunInstanceId.IsValid());
	TestEqual(TEXT("Direct creation does not receive card Burn"),
		FWacomBattleFixture::GetStatusStacks(
			Generated.StatusStacks, WacomTags::Status_Burn),
		0);
	const FBattleSnapshot After = Session->BuildSnapshot();
	TestEqual(TEXT("Direct creation does not consume player Burn"),
		FWacomBattleFixture::GetStatusStacks(
			After.Player.StatusStacks, WacomTags::Status_Burn),
		2);
	TestEqual(TEXT("Yellow companion raises MaxHP immediately"),
		After.Player.MaxHp, Before.Player.MaxHp + 15);
	TestEqual(TEXT("Yellow companion raises CurrentHP together"),
		After.Player.CurrentHp, Before.Player.CurrentHp + 15);
	return true;
}
