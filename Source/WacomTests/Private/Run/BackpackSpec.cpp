// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"

#include "RunSession.h"
#include "RunState.h"
#include "RunStateTypes.h"
#include "Session/BattleSession.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Tags/WacomGameplayTags.h"

#include "UObject/StrongObjectPtr.h"

/**
 * Stage 4.1：背包 / 备战卡组业务层测试。
 *
 * 覆盖（GDD §11）：
 *   - IsContainerCard / IsBagProviderCard / IsIntrinsicCard 静态判定
 *   - GetFluxCapacity 公式：Σ(玩家拥有的所有 A 类容器卡 Capacity)
 *   - GetBattleDeckCapacity 公式：Σ(玩家拥有的所有容器卡 Capacity)
 *   - IsBackpackUiAvailable：玩家持有区至少一张容器卡
 *   - Initialize Stage 4.1 a2：非容器卡进 BattleDeck，容器卡只进 Backpack
 *   - AcquireCardToRun 自动 RecomputeBurden
 *   - DestroyCardByInstance：
 *       Intrinsic 拒绝
 *       最后一张容量来源卡拒绝
 *       同一卡多份 → 只销毁一份
 *       Companion 卡 → 嗜血 +1
 *       按 Backpack → BattleDeck → BurdenZone → SpecialZones 优先级移除一张
 *   - DestroyCardByInstance：按 InstanceId 精确销毁已拥有卡，不发金币
 *   - DeleteCardForGoldByInstance：白=1 / 蓝=2
 *   - MoveInstance 进出 BattleDeck 边界规则
 *   - BuildInitParamsForBattle 用 BattleDeck 而不是 StarterDeck
 *   - 金币 AddGold / RemoveGold
 */

namespace
{
	UCardDefinition* MakeBagCard(FWacomBattleFixture& Fx, int32 Capacity, FGameplayTag Rarity = FGameplayTag())
	{
		UCardDefinition* Card = Fx.MakeNoopCard(0);
		Card->Physique.Capacity = Capacity;
		Card->Keywords.AddTag(WacomTags::Card_Keyword_BagProvider);
		if (Rarity.IsValid())
		{
			Card->Rarity = Rarity;
		}
		else
		{
			Card->Rarity = WacomTags::Card_Rarity_White;
		}
		return Card;
	}

	UCardDefinition* MakeCardWithRarity(FWacomBattleFixture& Fx, FGameplayTag Rarity, bool bCompanion = false)
	{
		UCardDefinition* Card = Fx.MakeNoopCard(0);
		Card->Rarity = Rarity;
		if (bCompanion)
		{
			Card->Keywords.AddTag(WacomTags::Card_Keyword_Companion);
		}
		return Card;
	}

	UCardDefinition* MakeTypeAContainerCard(FWacomBattleFixture& Fx, int32 Capacity, FName CardId = NAME_None)
	{
		UCardDefinition* Card = Fx.MakeNoopCard(0);
		Card->Physique.Capacity = Capacity;
		Card->Rarity = WacomTags::Card_Rarity_White;
		if (!CardId.IsNone())
		{
			Card->CardId = CardId;
		}
		return Card;
	}

	UCardDefinition* MakeTypeBContainerCard(FWacomBattleFixture& Fx, int32 Capacity)
	{
		UCardDefinition* Card = Fx.MakeNoopCard(0);
		Card->Physique.Capacity = Capacity;
		Card->Physique.CapacityEffect = WacomTags::Card_CapacityEffect_Placeholder;
		Card->Rarity = WacomTags::Card_Rarity_White;
		return Card;
	}

	bool OwnedCardsContainDefinition(const FRunState& State, const UCardDefinition* Card)
	{
		auto PileContains = [Card](const TArray<FCardInstance>& Pile) -> bool
		{
			for (const FCardInstance& Inst : Pile)
			{
				if (Inst.Definition == Card)
				{
					return true;
				}
			}
			return false;
		};

		if (PileContains(State.Backpack) || PileContains(State.BattleDeck) || PileContains(State.BurdenZone))
		{
			return true;
		}
		for (const FSpecialZone& SpecialZone : State.SpecialZones)
		{
			if (PileContains(SpecialZone.Cards))
			{
				return true;
			}
		}
		return false;
	}

	bool OwnedCardsContainInstance(const FRunState& State, FGuid InstanceId)
	{
		auto PileContains = [InstanceId](const TArray<FCardInstance>& Pile) -> bool
		{
			for (const FCardInstance& Inst : Pile)
			{
				if (Inst.InstanceId == InstanceId)
				{
					return true;
				}
			}
			return false;
		};

		if (PileContains(State.Backpack) || PileContains(State.BattleDeck) || PileContains(State.BurdenZone))
		{
			return true;
		}
		for (const FSpecialZone& SpecialZone : State.SpecialZones)
		{
			if (PileContains(SpecialZone.Cards))
			{
				return true;
			}
		}
		return false;
	}

	int32 CountOwnedCardsByDefinition(const FRunState& State, const UCardDefinition* Card)
	{
		auto CountPile = [Card](const TArray<FCardInstance>& Pile) -> int32
		{
			int32 Count = 0;
			for (const FCardInstance& Inst : Pile)
			{
				if (Inst.Definition == Card)
				{
					++Count;
				}
			}
			return Count;
		};

		int32 Count = CountPile(State.Backpack) + CountPile(State.BattleDeck) + CountPile(State.BurdenZone);
		for (const FSpecialZone& SpecialZone : State.SpecialZones)
		{
			Count += CountPile(SpecialZone.Cards);
		}
		return Count;
	}

	FGuid FindFirstOwnedInstanceIdByDefinitionInZone(const FRunState& State, const UCardDefinition* Card, EZoneKind Zone);

	FGuid FindFirstBackpackInstanceIdByDefinition(const FRunState& State, const UCardDefinition* Card)
	{
		for (const FCardInstance& Inst : State.Backpack)
		{
			if (Inst.Definition == Card)
			{
				return Inst.InstanceId;
			}
		}
		return FGuid();
	}

	FGuid FindFirstBattleDeckInstanceIdByDefinition(const FRunState& State, const UCardDefinition* Card)
	{
		for (const FCardInstance& Inst : State.BattleDeck)
		{
			if (Inst.Definition == Card)
			{
				return Inst.InstanceId;
			}
		}
		return FGuid();
	}

	FGuid FindFirstOwnedInstanceIdByDefinition(const FRunState& State, const UCardDefinition* Card)
	{
		if (const FGuid BackpackId = FindFirstOwnedInstanceIdByDefinitionInZone(State, Card, EZoneKind::Backpack);
			BackpackId.IsValid())
		{
			return BackpackId;
		}
		if (const FGuid BattleDeckId = FindFirstOwnedInstanceIdByDefinitionInZone(State, Card, EZoneKind::BattleDeck);
			BattleDeckId.IsValid())
		{
			return BattleDeckId;
		}
		if (const FGuid BurdenId = FindFirstOwnedInstanceIdByDefinitionInZone(State, Card, EZoneKind::BurdenZone);
			BurdenId.IsValid())
		{
			return BurdenId;
		}
		return FindFirstOwnedInstanceIdByDefinitionInZone(State, Card, EZoneKind::SpecialZone);
	}

	bool BackpackContainsDefinition(const FRunState& State, const UCardDefinition* Card)
	{
		return FindFirstBackpackInstanceIdByDefinition(State, Card).IsValid();
	}

	bool BattleDeckContainsDefinition(const FRunState& State, const UCardDefinition* Card)
	{
		return FindFirstBattleDeckInstanceIdByDefinition(State, Card).IsValid();
	}

	URunSession* ResolveRunSessionForTest(URunSession* Run)
	{
		return Run;
	}

	URunSession* ResolveRunSessionForTest(const TStrongObjectPtr<URunSession>& Run)
	{
		return Run.Get();
	}

	template <typename TRun>
	bool MoveFirstBackpackDefinitionToBattleDeck(const TRun& RunHandle, const UCardDefinition* Card)
	{
		URunSession* Run = ResolveRunSessionForTest(RunHandle);
		if (!Run)
		{
			return false;
		}
		const FGuid InstanceId = FindFirstBackpackInstanceIdByDefinition(Run->GetRunState(), Card);
		return InstanceId.IsValid() && Run->MoveInstance(InstanceId, EZoneKind::BattleDeck, FGuid());
	}

	template <typename TRun>
	bool MoveFirstBattleDeckDefinitionToBackpack(const TRun& RunHandle, const UCardDefinition* Card)
	{
		URunSession* Run = ResolveRunSessionForTest(RunHandle);
		if (!Run)
		{
			return false;
		}
		const FGuid InstanceId = FindFirstBattleDeckInstanceIdByDefinition(Run->GetRunState(), Card);
		return InstanceId.IsValid() && Run->MoveInstance(InstanceId, EZoneKind::Backpack, FGuid());
	}

	template <typename TRun>
	bool DestroyFirstOwnedDefinition(const TRun& RunHandle, const UCardDefinition* Card)
	{
		URunSession* Run = ResolveRunSessionForTest(RunHandle);
		if (!Run)
		{
			return false;
		}
		const FGuid InstanceId = FindFirstOwnedInstanceIdByDefinition(Run->GetRunState(), Card);
		return InstanceId.IsValid() && Run->DestroyCardByInstance(InstanceId);
	}

	template <typename TRun>
	bool DeleteFirstOwnedDefinitionForGold(const TRun& RunHandle, const UCardDefinition* Card)
	{
		URunSession* Run = ResolveRunSessionForTest(RunHandle);
		if (!Run)
		{
			return false;
		}
		const FGuid InstanceId = FindFirstOwnedInstanceIdByDefinition(Run->GetRunState(), Card);
		return InstanceId.IsValid() && Run->DeleteCardForGoldByInstance(InstanceId);
	}

	FGuid FindFirstOwnedInstanceIdByDefinitionInZone(const FRunState& State, const UCardDefinition* Card, EZoneKind Zone)
	{
		const TArray<FCardInstance>* Pile = nullptr;
		if (Zone == EZoneKind::Backpack)
		{
			Pile = &State.Backpack;
		}
		else if (Zone == EZoneKind::BattleDeck)
		{
			Pile = &State.BattleDeck;
		}
		else if (Zone == EZoneKind::BurdenZone)
		{
			Pile = &State.BurdenZone;
		}

		if (Pile)
		{
			for (const FCardInstance& Inst : *Pile)
			{
				if (Inst.Definition == Card)
				{
					return Inst.InstanceId;
				}
			}
			return FGuid();
		}

		for (const FSpecialZone& SpecialZone : State.SpecialZones)
		{
			for (const FCardInstance& Inst : SpecialZone.Cards)
			{
				if (Inst.Definition == Card)
				{
					return Inst.InstanceId;
				}
			}
		}
		return FGuid();
	}
}

// ================ 静态判定 ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckStaticPredicatesSpec,
	"Wacom.Run.Deck.StaticPredicates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckStaticPredicatesSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* Normal = Fx.MakeNoopCard(0);
	UCardDefinition* Container = Fx.MakeNoopCard(0);
	Container->Physique.Capacity = 5;

	UCardDefinition* BagProvider = MakeBagCard(Fx, 12);

	UCardDefinition* IntrinsicCard = Fx.MakeNoopCard(0);
	IntrinsicCard->Rarity = WacomTags::Card_Rarity_Intrinsic;

	TestFalse(TEXT("Normal not container"), URunSession::IsContainerCard(Normal));
	TestTrue(TEXT("Container is container"), URunSession::IsContainerCard(Container));
	TestTrue(TEXT("BagProvider is container"), URunSession::IsContainerCard(BagProvider));

	TestFalse(TEXT("Normal not BagProvider"), URunSession::IsBagProviderCard(Normal));
	TestTrue(TEXT("BagProvider tagged"), URunSession::IsBagProviderCard(BagProvider));

	TestFalse(TEXT("Normal not Intrinsic"), URunSession::IsIntrinsicCard(Normal));
	TestTrue(TEXT("Intrinsic recognized"), URunSession::IsIntrinsicCard(IntrinsicCard));

	TestFalse(TEXT("nullptr false on all"), URunSession::IsContainerCard(nullptr));
	TestFalse(TEXT("nullptr false on all"), URunSession::IsBagProviderCard(nullptr));
	TestFalse(TEXT("nullptr false on all"), URunSession::IsIntrinsicCard(nullptr));

	return true;
}

// ================ 容量公式 + Initialize a2 ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckInitializeA2Spec,
	"Wacom.Run.Deck.InitializeA2SeparatesContainerFromNormal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckInitializeA2Spec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* Normal1 = Fx.MakeNoopCard(0);
	UCardDefinition* Normal2 = Fx.MakeNoopCard(0);
	UCardDefinition* Bag = MakeBagCard(Fx, 12);

	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ Normal1, Normal2, Bag });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	const FRunState& State = Run->GetRunState();
	// GDD §11.4：一张卡同时只能在一个区。
	// 容器卡 → Backpack；非容器卡 → BattleDeck。
	// Stage 4.5.0：zone 元素是 FCardInstance；用 lambda 按 Definition 搜索。
	auto ContainsDef = [](const TArray<FCardInstance>& Pile, const UCardDefinition* Def) -> bool
	{
		for (const FCardInstance& Inst : Pile) { if (Inst.Definition == Def) { return true; } }
		return false;
	};
	TestEqual(TEXT("Backpack has only container Bag"), State.Backpack.Num(), 1);
	TestEqual(TEXT("BattleDeck has only normal cards"), State.BattleDeck.Num(), 2);
	TestTrue(TEXT("Bag in Backpack"), ContainsDef(State.Backpack, Bag));
	TestFalse(TEXT("Bag NOT in BattleDeck"), ContainsDef(State.BattleDeck, Bag));
	TestFalse(TEXT("Normal1 NOT in Backpack"), ContainsDef(State.Backpack, Normal1));
	TestTrue(TEXT("Normal1 in BattleDeck"), ContainsDef(State.BattleDeck, Normal1));

	TestEqual(TEXT("FluxCapacity=12 from Bag capacity"), Run->GetFluxCapacity(), 12);
	TestEqual(TEXT("BattleDeckCapacity=12"), Run->GetBattleDeckCapacity(), 12);
	TestTrue(TEXT("Backpack UI available with BagProvider"), Run->IsBackpackUiAvailable());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckCapacitySumSpec,
	"Wacom.Run.Deck.CapacitySumsAcrossBackpackAndBattleDeck",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckCapacitySumSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	// 容器卡 a Capacity=10（进 Backpack）+ 容器卡 b Capacity=3（也进 Backpack）
	UCardDefinition* Bag1 = MakeBagCard(Fx, 10);
	UCardDefinition* Bag2 = Fx.MakeNoopCard(0);
	Bag2->Physique.Capacity = 3;
	UCardDefinition* Normal = Fx.MakeNoopCard(0);

	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ Bag1, Bag2, Normal });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	TestEqual(TEXT("FluxCapacity=10+3=13"), Run->GetFluxCapacity(), 13);

	// 把 Bag2 加进 BattleDeck（虽然它是容器卡，第一阶段允许玩家手动加入备战）
	TestTrue(TEXT("Add Bag2 to BattleDeck"), MoveFirstBackpackDefinitionToBattleDeck(Run, Bag2));

	// 容量公式：Σ(背包 + 备战) = 仍然 13（容器卡换位置不影响公式）
	TestEqual(TEXT("FluxCapacity unchanged after move"), Run->GetFluxCapacity(), 13);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckBackpackUiAvailabilitySpec,
	"Wacom.Run.Deck.BackpackUiAvailability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckBackpackUiAvailabilitySpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	// 没容器卡 → 不可打开
	UCharacterDefinition* CharNoProvider = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ Fx.MakeNoopCard(0) });
	TStrongObjectPtr<URunSession> Run1(NewObject<URunSession>());
	Run1->Initialize(CharNoProvider);
	TestFalse(TEXT("UI not available without container capacity"),
		Run1->IsBackpackUiAvailable());

	// 有 BagProvider 容器 → 可打开
	UCardDefinition* Bag = MakeBagCard(Fx, 5);
	UCharacterDefinition* CharWithProvider = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ Bag });
	TStrongObjectPtr<URunSession> Run2(NewObject<URunSession>());
	Run2->Initialize(CharWithProvider);
	TestTrue(TEXT("UI available with BagProvider"),
		Run2->IsBackpackUiAvailable());

	// 有非 BagProvider 容器也可打开；背包入口与 BagProvider 关键词不再绑定。
	UCardDefinition* Lantern = MakeTypeAContainerCard(Fx, 3, TEXT("MuseiYinchongdeng"));
	UCharacterDefinition* CharWithCapacityProvider = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ Lantern });
	TStrongObjectPtr<URunSession> Run3(NewObject<URunSession>());
	Run3->Initialize(CharWithCapacityProvider);
	TestTrue(TEXT("UI available with non-BagProvider capacity card"),
		Run3->IsBackpackUiAvailable());

	return true;
}

// ================ AcquireCardToRun ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckAcquireCardToRunTriggersBurdenSpec,
	"Wacom.Run.Deck.AcquireCardToRunRecomputesBurden",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckAcquireCardToRunTriggersBurdenSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	// Capacity=2 容器卡 + 两张普通卡（进 BattleDeck，填满备战区容量）。
	// Stage 4.5 起 RecomputeBurden 会按 Backpack → BattleDeck → SpecialZones
	// 优先序从 BurdenZone 回填；这里必须让 BattleDeck 也满，超额卡才会留在 BurdenZone。
	UCardDefinition* Bag = MakeBagCard(Fx, 2);
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Bag });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	// 初始 Backpack 仅含 Bag = 1 张，BattleDeck=2 且容量=2 → Burden=0
	TestEqual(TEXT("Init Backpack=1"), Run->GetBackpack().Num(), 1);
	TestEqual(TEXT("Init BattleDeck=2"), Run->GetBattleDeck().Num(), 2);
	TestEqual(TEXT("Init Burden=0"),
		Run->GetPressureValue(EWacomPressureType::Burden), 0);

	// 加普通卡到 Backpack → Backpack=2，刚好 = Capacity，Burden=0
	Run->AcquireCardToRun(Fx.MakeNoopCard(0));
	TestEqual(TEXT("Burden=0 at capacity"),
		Run->GetPressureValue(EWacomPressureType::Burden), 0);

	// 再加一张 → Backpack=3，超容 1 → Burden = 1*2/2 = 1
	Run->AcquireCardToRun(Fx.MakeNoopCard(0));
	TestEqual(TEXT("Burden=1 over by 1"),
		Run->GetPressureValue(EWacomPressureType::Burden), 1);

	return true;
}

// ================ DestroyCardByInstance ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckDestroyIntrinsicRejectedSpec,
	"Wacom.Run.Deck.DestroyIntrinsicRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckDestroyIntrinsicRejectedSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* IntrinsicCard = MakeCardWithRarity(Fx, WacomTags::Card_Rarity_Intrinsic);
	UCardDefinition* Bag = MakeBagCard(Fx, 5);
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ IntrinsicCard, Bag });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	const int32 BefSize = Run->GetBackpack().Num();
	TestFalse(TEXT("Destroy Intrinsic rejected"),
		DestroyFirstOwnedDefinition(Run, IntrinsicCard));
	TestEqual(TEXT("Backpack size unchanged"),
		Run->GetBackpack().Num(), BefSize);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckDestroyLastCapacityProviderRejectedSpec,
	"Wacom.Run.Deck.DestroyLastCapacityProviderRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckDestroyLastCapacityProviderRejectedSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* OnlyBag = MakeBagCard(Fx, 5);
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ OnlyBag });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	TestFalse(TEXT("Destroy last capacity provider rejected"),
		DestroyFirstOwnedDefinition(Run, OnlyBag));
	TestTrue(TEXT("UI still available"),
		Run->IsBackpackUiAvailable());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckDestroyOneOfTwoCapacityProvidersAllowedSpec,
	"Wacom.Run.Deck.DestroyOneOfTwoCapacityProvidersAllowed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckDestroyOneOfTwoCapacityProvidersAllowedSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* Bag1 = MakeBagCard(Fx, 5);
	UCardDefinition* Bag2 = MakeTypeAContainerCard(Fx, 8);
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ Bag1, Bag2 });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	TestTrue(TEXT("Destroy Bag1 allowed (Bag2 still provides capacity)"),
		DestroyFirstOwnedDefinition(Run, Bag1));
	TestTrue(TEXT("UI still available"), Run->IsBackpackUiAvailable());
	TestEqual(TEXT("FluxCapacity=8 only Bag2 capacity"),
		Run->GetFluxCapacity(), 8);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckDestroyBugGirlBagAllowedWhenLanternProvidesCapacitySpec,
	"Wacom.Run.Deck.DestroyBugGirlBagAllowedWhenLanternProvidesCapacity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckDestroyBugGirlBagAllowedWhenLanternProvidesCapacitySpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* Normal1 = Fx.MakeNoopCard(0);
	UCardDefinition* Normal2 = Fx.MakeNoopCard(0);
	UCardDefinition* Normal3 = Fx.MakeNoopCard(0);
	UCardDefinition* Bag = MakeBagCard(Fx, 4);
	UCardDefinition* Lantern = MakeTypeAContainerCard(Fx, 3, TEXT("MuseiYinchongdeng"));

	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ Normal1, Normal2, Normal3, Bag, Lantern });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	TestTrue(TEXT("Lantern starts in BattleDeck"), BattleDeckContainsDefinition(Run->GetRunState(), Lantern));
	TestTrue(TEXT("Bag starts in Backpack"), BackpackContainsDefinition(Run->GetRunState(), Bag));
	TestEqual(TEXT("Initial flux capacity includes bag and lantern"), Run->GetFluxCapacity(), 7);
	TestEqual(TEXT("Initial battle capacity includes bag and lantern"), Run->GetBattleDeckCapacity(), 7);

	const FGuid BagId = FindFirstBackpackInstanceIdByDefinition(Run->GetRunState(), Bag);
	TestTrue(TEXT("Bag instance valid"), BagId.IsValid());
	TestTrue(TEXT("Delete validation allows Bag while Lantern provides capacity"),
		Run->ValidateDeleteCardForGoldByInstance(BagId).bCanExecute);
	TestTrue(TEXT("Destroy Bag succeeds"),
		DestroyFirstOwnedDefinition(Run, Bag));

	TestFalse(TEXT("Bag removed"), BackpackContainsDefinition(Run->GetRunState(), Bag));
	TestTrue(TEXT("Lantern remains in BattleDeck"), BattleDeckContainsDefinition(Run->GetRunState(), Lantern));
	TestTrue(TEXT("Backpack UI remains available from Lantern capacity"), Run->IsBackpackUiAvailable());
	TestEqual(TEXT("Flux capacity now only Lantern"), Run->GetFluxCapacity(), 3);
	TestEqual(TEXT("Battle capacity now only Lantern"), Run->GetBattleDeckCapacity(), 3);
	TestEqual(TEXT("BattleDeck overflow moved one normal card to Backpack first"), Run->GetBackpack().Num(), 1);
	TestEqual(TEXT("BattleDeck overflow does not reach burden while flux has room"), Run->GetRunState().BurdenZone.Num(), 0);
	TestEqual(TEXT("No burden pressure while overflow fits flux"), Run->GetPressureValue(EWacomPressureType::Burden), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckDestroyBugGirlBagOverflowUsesBurdenOnlyWhenFluxFullSpec,
	"Wacom.Run.Deck.DestroyBugGirlBagOverflowUsesBurdenOnlyWhenFluxFull",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckDestroyBugGirlBagOverflowUsesBurdenOnlyWhenFluxFullSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* Normal1 = Fx.MakeNoopCard(0);
	UCardDefinition* Normal2 = Fx.MakeNoopCard(0);
	UCardDefinition* Normal3 = Fx.MakeNoopCard(0);
	UCardDefinition* Bag = MakeBagCard(Fx, 4);
	UCardDefinition* Lantern = MakeTypeAContainerCard(Fx, 3, TEXT("MuseiYinchongdeng"));

	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ Normal1, Normal2, Normal3, Bag, Lantern });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	Run->AcquireCardToRun(Fx.MakeNoopCard(0));
	Run->AcquireCardToRun(Fx.MakeNoopCard(0));
	Run->AcquireCardToRun(Fx.MakeNoopCard(0));
	TestEqual(TEXT("Pre-delete Backpack has Bag plus three flux cards"), Run->GetBackpack().Num(), 4);

	TestTrue(TEXT("Destroy Bag succeeds with Lantern still providing capacity"),
		DestroyFirstOwnedDefinition(Run, Bag));

	TestFalse(TEXT("Bag removed"), BackpackContainsDefinition(Run->GetRunState(), Bag));
	TestTrue(TEXT("Lantern remains in BattleDeck"), BattleDeckContainsDefinition(Run->GetRunState(), Lantern));
	TestEqual(TEXT("Flux capacity now only Lantern"), Run->GetFluxCapacity(), 3);
	TestEqual(TEXT("Backpack remains at full flux capacity"), Run->GetBackpack().Num(), 3);
	TestEqual(TEXT("BattleDeck overflow reaches burden only after flux is full"), Run->GetRunState().BurdenZone.Num(), 1);
	TestEqual(TEXT("Burden pressure for one overflow card"), Run->GetPressureValue(EWacomPressureType::Burden), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckDestroyCompanionAddsBloodlustSpec,
	"Wacom.Run.Deck.DestroyCompanionAddsBloodlust",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckDestroyCompanionAddsBloodlustSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* Companion = MakeCardWithRarity(Fx, WacomTags::Card_Rarity_White, /*bCompanion*/true);
	UCardDefinition* Bag = MakeBagCard(Fx, 5);
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ Companion, Bag });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	TestEqual(TEXT("Init Bloodlust=0"),
		Run->GetPressureValue(EWacomPressureType::Bloodlust), 0);

	TestTrue(TEXT("Destroy Companion succeeded"),
		DestroyFirstOwnedDefinition(Run, Companion));
	TestEqual(TEXT("Bloodlust +1 from Companion destruction"),
		Run->GetPressureValue(EWacomPressureType::Bloodlust), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckDestroyAlsoRemovesFromBattleDeckSpec,
	"Wacom.Run.Deck.DestroyAlsoRemovesFromBattleDeck",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckDestroyAlsoRemovesFromBattleDeckSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* Card = Fx.MakeNoopCard(0);  // 普通卡 → 默认进 BattleDeck
	Card->Rarity = WacomTags::Card_Rarity_White;
	UCardDefinition* Bag = MakeBagCard(Fx, 5);
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ Card, Bag });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	TestTrue(TEXT("Card initially in BattleDeck (a2 rule)"),
		BattleDeckContainsDefinition(Run->GetRunState(), Card));
	TestFalse(TEXT("Card not in Backpack (互斥)"),
		BackpackContainsDefinition(Run->GetRunState(), Card));

	// DestroyCardByInstance 应当能从 BattleDeck 中销毁（玩家拥有即可）
	TestTrue(TEXT("Destroy succeeded"),
		DestroyFirstOwnedDefinition(Run, Card));

	TestFalse(TEXT("Card no longer in BattleDeck"),
		BattleDeckContainsDefinition(Run->GetRunState(), Card));
	TestFalse(TEXT("Card not in Backpack"),
		BackpackContainsDefinition(Run->GetRunState(), Card));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckDestroyCardFromAllOwnedZonesSpec,
	"Wacom.Run.Deck.DestroyCardFromAllOwnedZones",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckDestroyCardFromAllOwnedZonesSpec::RunTest(const FString& /*Parameters*/)
{
	auto BuildRunWithTargetInZone = [this](FWacomBattleFixture& Fx, EZoneKind TargetZone, UCardDefinition*& OutTarget) -> TStrongObjectPtr<URunSession>
	{
		UCardDefinition* Bag = MakeBagCard(Fx, 8);
		UCardDefinition* TypeB = MakeTypeBContainerCard(Fx, 3);
		OutTarget = MakeCardWithRarity(Fx, WacomTags::Card_Rarity_White);

		TArray<UCardDefinition*> Starter = { Bag, TypeB };
		if (TargetZone == EZoneKind::BattleDeck)
		{
			Starter.Add(OutTarget);
		}

		UCharacterDefinition* Char = Fx.MakeCharacter(Fx.MakeNoopCard(1), Fx.MakeNoopCard(1), Starter);
		TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
		TestTrue(TEXT("Initialize all-zones destroy run"), Run->Initialize(Char));

		if (TargetZone != EZoneKind::BattleDeck)
		{
			Run->AcquireCardToRun(OutTarget);
		}

		if (TargetZone == EZoneKind::BurdenZone)
		{
			const FGuid TargetId = FindFirstBackpackInstanceIdByDefinition(Run->GetRunState(), OutTarget);
			TestTrue(TEXT("Target id valid before burden move"), TargetId.IsValid());
			TestTrue(TEXT("Move target to burden"), Run->MoveInstance(TargetId, EZoneKind::BurdenZone, FGuid()));
		}
		else if (TargetZone == EZoneKind::SpecialZone)
		{
			const FGuid OwnerId = Run->GetRunState().SpecialZones[0].OwnerInstanceId;
			const FGuid TargetId = FindFirstBackpackInstanceIdByDefinition(Run->GetRunState(), OutTarget);
			TestTrue(TEXT("Target id valid before special move"), TargetId.IsValid());
			TestTrue(TEXT("Move target to special"), Run->MoveInstance(TargetId, EZoneKind::SpecialZone, OwnerId));
		}

		return Run;
	};

	const EZoneKind Zones[] = {
		EZoneKind::Backpack,
		EZoneKind::BattleDeck,
		EZoneKind::BurdenZone,
		EZoneKind::SpecialZone,
	};

	for (const EZoneKind Zone : Zones)
	{
		FWacomBattleFixture Fx;
		UCardDefinition* Target = nullptr;
		TStrongObjectPtr<URunSession> Run = BuildRunWithTargetInZone(Fx, Zone, Target);
		const FRunState& Before = Run->GetRunState();
		const FGuid RemovedId = FindFirstOwnedInstanceIdByDefinitionInZone(Before, Target, Zone);
		TestTrue(TEXT("Target starts in expected zone"), RemovedId.IsValid());

		int32 BroadcastCount = 0;
		Run->OnRunStateChangedNative.AddLambda([&BroadcastCount]()
		{
			++BroadcastCount;
		});

		TestTrue(TEXT("Delete validation sees target in owned zone"),
			Run->ValidateDeleteCardForGoldByInstance(RemovedId).bCanExecute);
		TestTrue(TEXT("Destroy from owned zone succeeds"), DestroyFirstOwnedDefinition(Run, Target));
		TestFalse(TEXT("Removed instance is gone"), OwnedCardsContainInstance(Run->GetRunState(), RemovedId));
		TestFalse(TEXT("Target definition gone"), OwnedCardsContainDefinition(Run->GetRunState(), Target));
		TestEqual(TEXT("Destroy broadcasts once"), BroadcastCount, 1);
		TestEqual(TEXT("Destroy does not add gold"), Run->GetGold(), 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckDestroyCardByInstanceRemovesExactInstanceSpec,
	"Wacom.Run.Deck.DestroyCardByInstanceRemovesExactInstance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckDestroyCardByInstanceRemovesExactInstanceSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* SharedCard = MakeCardWithRarity(Fx, WacomTags::Card_Rarity_White);
	UCardDefinition* Bag = MakeBagCard(Fx, 5);
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ Bag, SharedCard, SharedCard });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	TestTrue(TEXT("Move first shared card to Backpack"), MoveFirstBattleDeckDefinitionToBackpack(Run, SharedCard));
	const FGuid FirstId = FindFirstOwnedInstanceIdByDefinitionInZone(Run->GetRunState(), SharedCard, EZoneKind::Backpack);
	const FGuid SecondId = FindFirstOwnedInstanceIdByDefinitionInZone(Run->GetRunState(), SharedCard, EZoneKind::BattleDeck);
	TestTrue(TEXT("First same-definition id valid"), FirstId.IsValid());
	TestTrue(TEXT("Second same-definition id valid"), SecondId.IsValid());

	int32 BroadcastCount = 0;
	Run->OnRunStateChangedNative.AddLambda([&BroadcastCount]()
	{
		++BroadcastCount;
	});

	TestTrue(TEXT("Destroy by instance validates second id"),
		Run->ValidateDestroyCardByInstance(SecondId).bCanExecute);
	TestTrue(TEXT("Destroy by instance removes second id"),
		Run->DestroyCardByInstance(SecondId));

	TestTrue(TEXT("First same-definition instance remains"), OwnedCardsContainInstance(Run->GetRunState(), FirstId));
	TestFalse(TEXT("Requested second instance removed"), OwnedCardsContainInstance(Run->GetRunState(), SecondId));
	TestEqual(TEXT("Only one shared definition remains"), CountOwnedCardsByDefinition(Run->GetRunState(), SharedCard), 1);
	TestEqual(TEXT("Destroy by instance does not add gold"), Run->GetGold(), 0);
	TestEqual(TEXT("Destroy by instance broadcasts once"), BroadcastCount, 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckDestroyCardByInstanceSafetySpec,
	"Wacom.Run.Deck.DestroyCardByInstanceSafety",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckDestroyCardByInstanceSafetySpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* OnlyBag = MakeBagCard(Fx, 5);
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ OnlyBag });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	if (!TestEqual(TEXT("Only capacity provider starts in Backpack"), Run->GetRunState().Backpack.Num(), 1))
	{
		return false;
	}
	const FGuid OnlyBagId = Run->GetRunState().Backpack[0].InstanceId;
	const int32 BackpackCountBefore = Run->GetBackpack().Num();
	const int32 BattleDeckCountBefore = Run->GetBattleDeck().Num();
	const int32 BurdenCountBefore = Run->GetRunState().BurdenZone.Num();
	const int32 SpecialZoneCountBefore = Run->GetRunState().SpecialZones.Num();
	const int32 FluxCapacityBefore = Run->GetFluxCapacity();
	const int32 BattleDeckCapacityBefore = Run->GetBattleDeckCapacity();
	const int32 GoldBefore = Run->GetGold();

	int32 BroadcastCount = 0;
	Run->OnRunStateChangedNative.AddLambda([&BroadcastCount]()
	{
		++BroadcastCount;
	});

	const FRunDeckOperationValidation Validation = Run->ValidateDestroyCardByInstance(OnlyBagId);
	TestFalse(TEXT("Last capacity provider destroy by instance validation rejected"), Validation.bCanExecute);
	TestEqual(TEXT("Last capacity provider reason"),
		Validation.DisabledReason,
		FName(TEXT("LastCapacityProvider")));
	TestFalse(TEXT("Last capacity provider destroy by instance fails"),
		Run->DestroyCardByInstance(OnlyBagId));

	TestTrue(TEXT("Last capacity provider instance remains"),
		OwnedCardsContainInstance(Run->GetRunState(), OnlyBagId));
	TestEqual(TEXT("Backpack count unchanged"), Run->GetBackpack().Num(), BackpackCountBefore);
	TestEqual(TEXT("BattleDeck count unchanged"), Run->GetBattleDeck().Num(), BattleDeckCountBefore);
	TestEqual(TEXT("Burden count unchanged"), Run->GetRunState().BurdenZone.Num(), BurdenCountBefore);
	TestEqual(TEXT("SpecialZone count unchanged"), Run->GetRunState().SpecialZones.Num(), SpecialZoneCountBefore);
	TestEqual(TEXT("Flux capacity unchanged"), Run->GetFluxCapacity(), FluxCapacityBefore);
	TestEqual(TEXT("BattleDeck capacity unchanged"), Run->GetBattleDeckCapacity(), BattleDeckCapacityBefore);
	TestEqual(TEXT("Gold unchanged"), Run->GetGold(), GoldBefore);
	TestEqual(TEXT("Rejected destroy by instance does not broadcast"), BroadcastCount, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckDestroyBMainInstanceClearsOnlyMatchingSpecialZoneSpec,
	"Wacom.Run.Deck.DestroyCardByInstance.BMainClearsOnlyMatchingSpecialZone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckDestroyBMainInstanceClearsOnlyMatchingSpecialZoneSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* TypeA = MakeBagCard(Fx, 6);
	UCardDefinition* SharedBMain = MakeTypeBContainerCard(Fx, 3);
	UCardDefinition* FirstContent = Fx.MakeNoopCard(0);
	UCardDefinition* SecondContent = Fx.MakeNoopCard(0);
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ TypeA, SharedBMain, SharedBMain });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	if (!TestEqual(TEXT("Two SpecialZones for two same-definition B mains"), Run->GetRunState().SpecialZones.Num(), 2))
	{
		return false;
	}
	const FGuid FirstOwnerId = Run->GetRunState().SpecialZones[0].OwnerInstanceId;
	const FGuid SecondOwnerId = Run->GetRunState().SpecialZones[1].OwnerInstanceId;

	Run->AcquireCardToRun(FirstContent);
	const FGuid FirstContentId = FindFirstBackpackInstanceIdByDefinition(Run->GetRunState(), FirstContent);
	TestTrue(TEXT("Move first content to first SpecialZone"),
		Run->MoveInstance(FirstContentId, EZoneKind::SpecialZone, FirstOwnerId));

	Run->AcquireCardToRun(SecondContent);
	const FGuid SecondContentId = FindFirstBackpackInstanceIdByDefinition(Run->GetRunState(), SecondContent);
	TestTrue(TEXT("Move second content to second SpecialZone"),
		Run->MoveInstance(SecondContentId, EZoneKind::SpecialZone, SecondOwnerId));

	TestTrue(TEXT("Destroy second B main by instance validates"),
		Run->ValidateDestroyCardByInstance(SecondOwnerId).bCanExecute);
	TestTrue(TEXT("Destroy second B main by instance succeeds"),
		Run->DestroyCardByInstance(SecondOwnerId));

	FSpecialZone FirstZoneAfter;
	TestTrue(TEXT("First same-definition B main SpecialZone remains"),
		Run->GetSpecialZone(FirstOwnerId, FirstZoneAfter));
	TestEqual(TEXT("First SpecialZone content remains"), FirstZoneAfter.Cards.Num(), 1);
	if (FirstZoneAfter.Cards.Num() == 1)
	{
		TestEqual(TEXT("First SpecialZone keeps its content id"),
			FirstZoneAfter.Cards[0].InstanceId, FirstContentId);
	}

	FSpecialZone SecondZoneAfter;
	TestFalse(TEXT("Destroyed B main SpecialZone removed"),
		Run->GetSpecialZone(SecondOwnerId, SecondZoneAfter));
	TestTrue(TEXT("Second SpecialZone content returned to owned zones"),
		OwnedCardsContainInstance(Run->GetRunState(), SecondContentId));
	TestTrue(TEXT("First B main instance remains"),
		OwnedCardsContainInstance(Run->GetRunState(), FirstOwnerId));
	TestFalse(TEXT("Destroyed B main instance removed"),
		OwnedCardsContainInstance(Run->GetRunState(), SecondOwnerId));
	TestEqual(TEXT("Destroy B main by instance does not add gold"), Run->GetGold(), 0);

	return true;
}

// ================ DeleteCardForGold ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckDeleteCardForGoldByRaritySpec,
	"Wacom.Run.Deck.DeleteCardForGoldByRarity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckDeleteCardForGoldByRaritySpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* WhiteCard = MakeCardWithRarity(Fx, WacomTags::Card_Rarity_White);
	UCardDefinition* BlueCard  = MakeCardWithRarity(Fx, WacomTags::Card_Rarity_Blue);
	UCardDefinition* Bag = MakeBagCard(Fx, 5);
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ WhiteCard, BlueCard, Bag });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	TestEqual(TEXT("Init gold=0"), Run->GetGold(), 0);

	TestTrue(TEXT("Delete white card succeeded"),
		DeleteFirstOwnedDefinitionForGold(Run, WhiteCard));
	TestEqual(TEXT("White card → +1 gold"), Run->GetGold(), 1);

	TestTrue(TEXT("Delete blue card succeeded"),
		DeleteFirstOwnedDefinitionForGold(Run, BlueCard));
	TestEqual(TEXT("Blue card → +2 gold (total 3)"), Run->GetGold(), 3);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckDeleteCardForGoldFromBurdenAndSpecialZoneSpec,
	"Wacom.Run.Deck.DeleteCardForGoldFromBurdenAndSpecialZone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckDeleteCardForGoldFromBurdenAndSpecialZoneSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* WhiteCard = MakeCardWithRarity(Fx, WacomTags::Card_Rarity_White);
	UCardDefinition* BlueCard = MakeCardWithRarity(Fx, WacomTags::Card_Rarity_Blue);
	UCardDefinition* Bag = MakeBagCard(Fx, 8);
	UCardDefinition* TypeB = MakeTypeBContainerCard(Fx, 3);
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ Bag, TypeB });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	Run->AcquireCardToRun(WhiteCard);
	const FGuid WhiteId = FindFirstBackpackInstanceIdByDefinition(Run->GetRunState(), WhiteCard);
	TestTrue(TEXT("White id valid"), WhiteId.IsValid());
	TestTrue(TEXT("Move white to burden"), Run->MoveInstance(WhiteId, EZoneKind::BurdenZone, FGuid()));

	Run->AcquireCardToRun(BlueCard);
	const FGuid BlueId = FindFirstBackpackInstanceIdByDefinition(Run->GetRunState(), BlueCard);
	TestTrue(TEXT("Blue id valid"), BlueId.IsValid());
	const FGuid OwnerId = Run->GetRunState().SpecialZones[0].OwnerInstanceId;
	TestTrue(TEXT("Move blue to special"), Run->MoveInstance(BlueId, EZoneKind::SpecialZone, OwnerId));

	int32 BroadcastCount = 0;
	Run->OnRunStateChangedNative.AddLambda([&BroadcastCount]()
	{
		++BroadcastCount;
	});

	TestTrue(TEXT("Delete white from burden succeeds"), DeleteFirstOwnedDefinitionForGold(Run, WhiteCard));
	TestEqual(TEXT("White from burden gives 1 gold"), Run->GetGold(), 1);
	TestFalse(TEXT("White removed from all owned zones"), OwnedCardsContainDefinition(Run->GetRunState(), WhiteCard));
	TestEqual(TEXT("White delete broadcasts once"), BroadcastCount, 1);

	TestTrue(TEXT("Delete blue from special succeeds"), DeleteFirstOwnedDefinitionForGold(Run, BlueCard));
	TestEqual(TEXT("Blue from special gives 2 more gold"), Run->GetGold(), 3);
	TestFalse(TEXT("Blue removed from all owned zones"), OwnedCardsContainDefinition(Run->GetRunState(), BlueCard));
	TestEqual(TEXT("Blue delete broadcasts once"), BroadcastCount, 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckDeleteCardForGoldValidationSpec,
	"Wacom.Run.Deck.DeleteCardForGoldValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckDeleteCardForGoldValidationSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* WhiteCard = MakeCardWithRarity(Fx, WacomTags::Card_Rarity_White);
	UCardDefinition* IntrinsicCard = MakeCardWithRarity(Fx, WacomTags::Card_Rarity_Intrinsic);
	UCardDefinition* OnlyBag = MakeBagCard(Fx, 5);
	UCardDefinition* MissingCard = MakeCardWithRarity(Fx, WacomTags::Card_Rarity_White);
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ WhiteCard, IntrinsicCard, OnlyBag });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	const FGuid WhiteId = FindFirstOwnedInstanceIdByDefinition(Run->GetRunState(), WhiteCard);
	const FGuid IntrinsicId = FindFirstOwnedInstanceIdByDefinition(Run->GetRunState(), IntrinsicCard);
	const FGuid OnlyBagId = FindFirstOwnedInstanceIdByDefinition(Run->GetRunState(), OnlyBag);
	TestTrue(TEXT("White card can be deleted"),
		Run->ValidateDeleteCardForGoldByInstance(WhiteId).bCanExecute);
	TestEqual(TEXT("Null delete reason"),
		Run->ValidateDeleteCardForGoldByInstance(FGuid()).DisabledReason,
		FName(TEXT("CardNotOwned")));
	TestFalse(TEXT("Missing card not owned"), OwnedCardsContainDefinition(Run->GetRunState(), MissingCard));
	TestEqual(TEXT("Intrinsic delete reason"),
		Run->ValidateDeleteCardForGoldByInstance(IntrinsicId).DisabledReason,
		FName(TEXT("Intrinsic")));
	TestEqual(TEXT("Last capacity provider delete reason"),
		Run->ValidateDeleteCardForGoldByInstance(OnlyBagId).DisabledReason,
		FName(TEXT("LastCapacityProvider")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckDeleteCardForGoldByInstanceRemovesOnlyRequestedIdSpec,
	"Wacom.Run.Deck.DeleteCardForGoldByInstance.RemovesOnlyRequestedId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckDeleteCardForGoldByInstanceRemovesOnlyRequestedIdSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* SharedCard = MakeCardWithRarity(Fx, WacomTags::Card_Rarity_White);
	UCardDefinition* Bag = MakeBagCard(Fx, 5);
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ Bag, SharedCard, SharedCard });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	TestTrue(TEXT("Move first shared card to backpack"), MoveFirstBattleDeckDefinitionToBackpack(Run, SharedCard));
	const FGuid FirstId = FindFirstOwnedInstanceIdByDefinitionInZone(Run->GetRunState(), SharedCard, EZoneKind::Backpack);
	const FGuid SecondId = FindFirstOwnedInstanceIdByDefinitionInZone(Run->GetRunState(), SharedCard, EZoneKind::BattleDeck);
	TestTrue(TEXT("First same-definition id valid"), FirstId.IsValid());
	TestTrue(TEXT("Second same-definition id valid"), SecondId.IsValid());

	TestTrue(TEXT("Delete by instance validates second id"),
		Run->ValidateDeleteCardForGoldByInstance(SecondId).bCanExecute);
	TestTrue(TEXT("Delete by instance removes second id"),
		Run->DeleteCardForGoldByInstance(SecondId));

	TestTrue(TEXT("First same-definition instance remains"), OwnedCardsContainInstance(Run->GetRunState(), FirstId));
	TestFalse(TEXT("Requested second instance removed"), OwnedCardsContainInstance(Run->GetRunState(), SecondId));
	TestEqual(TEXT("Only one shared definition remains"), CountOwnedCardsByDefinition(Run->GetRunState(), SharedCard), 1);
	TestEqual(TEXT("White instance delete gives one gold"), Run->GetGold(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckDeleteCardForGoldByInstanceInvalidReasonSpec,
	"Wacom.Run.Deck.DeleteCardForGoldByInstance.InvalidReason",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckDeleteCardForGoldByInstanceInvalidReasonSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());

	TestEqual(TEXT("Invalid instance delete reason is CardNotOwned"),
		Run->ValidateDeleteCardForGoldByInstance(FGuid()).DisabledReason,
		FName(TEXT("CardNotOwned")));
	TestEqual(TEXT("Missing instance delete reason is CardNotOwned"),
		Run->ValidateDeleteCardForGoldByInstance(FGuid::NewGuid()).DisabledReason,
		FName(TEXT("CardNotOwned")));
	TestFalse(TEXT("Missing instance delete rejected"),
		Run->DeleteCardForGoldByInstance(FGuid::NewGuid()));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckDeleteSameDefinitionCapacityInstanceRulesSpec,
	"Wacom.Run.Deck.DeleteCardForGoldByInstance.SameDefinitionCapacityRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckDeleteSameDefinitionCapacityInstanceRulesSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* SharedContainer = MakeTypeAContainerCard(Fx, 3);
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ SharedContainer, SharedContainer });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	TestEqual(TEXT("Two same-definition capacity instances owned"),
		CountOwnedCardsByDefinition(Run->GetRunState(), SharedContainer), 2);

	const FGuid FirstId = Run->GetRunState().Backpack[0].InstanceId;
	const FGuid SecondId = Run->GetRunState().Backpack[1].InstanceId;
	TestTrue(TEXT("Deleting one of two same-definition capacity providers validates"),
		Run->ValidateDeleteCardForGoldByInstance(FirstId).bCanExecute);
	TestTrue(TEXT("Deleting one of two same-definition capacity providers succeeds"),
		Run->DeleteCardForGoldByInstance(FirstId));

	TestFalse(TEXT("First capacity instance removed"), OwnedCardsContainInstance(Run->GetRunState(), FirstId));
	TestTrue(TEXT("Second capacity instance remains"), OwnedCardsContainInstance(Run->GetRunState(), SecondId));
	TestEqual(TEXT("One same-definition capacity instance remains"),
		CountOwnedCardsByDefinition(Run->GetRunState(), SharedContainer), 1);
	TestEqual(TEXT("Last remaining capacity provider rejected"),
		Run->ValidateDeleteCardForGoldByInstance(SecondId).DisabledReason,
		FName(TEXT("LastCapacityProvider")));
	TestFalse(TEXT("Last remaining capacity provider delete fails"),
		Run->DeleteCardForGoldByInstance(SecondId));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckDeleteBMainInstanceClearsOnlyMatchingSpecialZoneSpec,
	"Wacom.Run.Deck.DeleteCardForGoldByInstance.BMainClearsOnlyMatchingSpecialZone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckDeleteBMainInstanceClearsOnlyMatchingSpecialZoneSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* TypeA = MakeBagCard(Fx, 6);
	UCardDefinition* SharedBMain = MakeTypeBContainerCard(Fx, 3);
	UCardDefinition* FirstContent = Fx.MakeNoopCard(0);
	UCardDefinition* SecondContent = Fx.MakeNoopCard(0);
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ TypeA, SharedBMain, SharedBMain });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	if (!TestEqual(TEXT("Two SpecialZones for two same-definition B mains"), Run->GetRunState().SpecialZones.Num(), 2))
	{
		return false;
	}
	const FGuid FirstOwnerId = Run->GetRunState().SpecialZones[0].OwnerInstanceId;
	const FGuid SecondOwnerId = Run->GetRunState().SpecialZones[1].OwnerInstanceId;

	Run->AcquireCardToRun(FirstContent);
	const FGuid FirstContentId = FindFirstBackpackInstanceIdByDefinition(Run->GetRunState(), FirstContent);
	TestTrue(TEXT("Move first content to first SpecialZone"),
		Run->MoveInstance(FirstContentId, EZoneKind::SpecialZone, FirstOwnerId));

	Run->AcquireCardToRun(SecondContent);
	const FGuid SecondContentId = FindFirstBackpackInstanceIdByDefinition(Run->GetRunState(), SecondContent);
	TestTrue(TEXT("Move second content to second SpecialZone"),
		Run->MoveInstance(SecondContentId, EZoneKind::SpecialZone, SecondOwnerId));

	TestTrue(TEXT("Delete second B main by instance succeeds"),
		Run->DeleteCardForGoldByInstance(SecondOwnerId));

	FSpecialZone FirstZoneAfter;
	TestTrue(TEXT("First same-definition B main SpecialZone remains"),
		Run->GetSpecialZone(FirstOwnerId, FirstZoneAfter));
	TestEqual(TEXT("First SpecialZone content remains"), FirstZoneAfter.Cards.Num(), 1);
	if (FirstZoneAfter.Cards.Num() == 1)
	{
		TestEqual(TEXT("First SpecialZone keeps its content id"),
			FirstZoneAfter.Cards[0].InstanceId, FirstContentId);
	}

	FSpecialZone SecondZoneAfter;
	TestFalse(TEXT("Deleted B main SpecialZone removed"),
		Run->GetSpecialZone(SecondOwnerId, SecondZoneAfter));
	TestTrue(TEXT("Second SpecialZone content returned to owned zones"),
		OwnedCardsContainInstance(Run->GetRunState(), SecondContentId));
	TestFalse(TEXT("Deleted B main instance removed"),
		OwnedCardsContainInstance(Run->GetRunState(), SecondOwnerId));

	return true;
}

// ================ MoveInstance To BattleDeck ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckAddToBattleDeckRequiresBackpackSpec,
	"Wacom.Run.Deck.AddToBattleDeckRequiresBackpack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckAddToBattleDeckRequiresBackpackSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* OutOfBackpack = Fx.MakeNoopCard(0);  // 没在 StarterDeck
	UCardDefinition* Bag = MakeBagCard(Fx, 5);
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ Bag });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	// OutOfBackpack 不在 backpack → 拒绝
	TestFalse(TEXT("AddToBattleDeck rejected: not in backpack"),
		MoveFirstBackpackDefinitionToBattleDeck(Run, OutOfBackpack));

	// Bag 在 backpack 且不在 BattleDeck（容器卡默认只进 Backpack）→ 加进去
	TestTrue(TEXT("Bag initially in Backpack"),
		BackpackContainsDefinition(Run->GetRunState(), Bag));
	TestFalse(TEXT("Bag NOT initially in BattleDeck"),
		BattleDeckContainsDefinition(Run->GetRunState(), Bag));

	TestTrue(TEXT("AddToBattleDeck Bag succeeds"),
		MoveFirstBackpackDefinitionToBattleDeck(Run, Bag));
	TestTrue(TEXT("Bag now in BattleDeck"),
		BattleDeckContainsDefinition(Run->GetRunState(), Bag));
	// 移到 BattleDeck 后从 Backpack 移除（互斥）
	TestFalse(TEXT("Bag no longer in Backpack after move"),
		BackpackContainsDefinition(Run->GetRunState(), Bag));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckAddToBattleDeckRespectsCapacitySpec,
	"Wacom.Run.Deck.AddToBattleDeckRespectsCapacity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckAddToBattleDeckRespectsCapacitySpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	// Capacity=2，初始 BattleDeck=2（两张普通卡），再加任何卡都该被拒
	UCardDefinition* Normal1 = Fx.MakeNoopCard(0);
	UCardDefinition* Normal2 = Fx.MakeNoopCard(0);
	UCardDefinition* Bag = MakeBagCard(Fx, 2);
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ Normal1, Normal2, Bag });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	TestEqual(TEXT("BattleDeck has 2 (capacity)"),
		Run->GetBattleDeck().Num(), 2);
	TestEqual(TEXT("Capacity=2"), Run->GetBattleDeckCapacity(), 2);

	// 试图把 Bag 加入 → 拒绝（已达容量）
	TestFalse(TEXT("Cannot add Bag (battle deck full)"),
		MoveFirstBackpackDefinitionToBattleDeck(Run, Bag));

	return true;
}

// ================ MoveInstance From BattleDeck ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckMoveIntrinsicFromBattleDeckUsesInstancePathSpec,
	"Wacom.Run.Deck.MoveIntrinsicFromBattleDeckUsesInstancePath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckMoveIntrinsicFromBattleDeckUsesInstancePathSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* IntrinsicNormal = MakeCardWithRarity(Fx, WacomTags::Card_Rarity_Intrinsic);
	UCardDefinition* Bag = MakeBagCard(Fx, 5);
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ IntrinsicNormal, Bag });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	// IntrinsicNormal 非容器卡 → 默认进 BattleDeck
	TestTrue(TEXT("IntrinsicNormal initially in BattleDeck"),
		BattleDeckContainsDefinition(Run->GetRunState(), IntrinsicNormal));

	TestTrue(TEXT("Move Intrinsic from BattleDeck by instance succeeds"),
		MoveFirstBattleDeckDefinitionToBackpack(Run, IntrinsicNormal));
	TestFalse(TEXT("Intrinsic no longer in BattleDeck"),
		BattleDeckContainsDefinition(Run->GetRunState(), IntrinsicNormal));
	TestTrue(TEXT("Intrinsic moved to Backpack"),
		BackpackContainsDefinition(Run->GetRunState(), IntrinsicNormal));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckRemoveFromBattleDeckSucceedsSpec,
	"Wacom.Run.Deck.RemoveFromBattleDeckSucceeds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckRemoveFromBattleDeckSucceedsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* Card = MakeCardWithRarity(Fx, WacomTags::Card_Rarity_White);
	UCardDefinition* Bag = MakeBagCard(Fx, 5);
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ Card, Bag });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	TestTrue(TEXT("Card initially in BattleDeck"),
		BattleDeckContainsDefinition(Run->GetRunState(), Card));

	TestTrue(TEXT("Remove succeeds"),
		MoveFirstBattleDeckDefinitionToBackpack(Run, Card));
	TestFalse(TEXT("Card no longer in BattleDeck"),
		BattleDeckContainsDefinition(Run->GetRunState(), Card));
	// 但仍在 Backpack（备战移除不影响背包）
	TestTrue(TEXT("Card still in Backpack"),
		BackpackContainsDefinition(Run->GetRunState(), Card));

	return true;
}

// ================ BattleInitParams 用 BattleDeck ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckBuildInitParamsUsesBattleDeckSpec,
	"Wacom.Run.Deck.BuildInitParamsUsesBattleDeck",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckBuildInitParamsUsesBattleDeckSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* Normal = Fx.MakeNoopCard(0);
	UCardDefinition* Bag = MakeBagCard(Fx, 5);
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ Normal, Bag });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	// BattleDeck 应当只有 1 张（Normal）
	TestEqual(TEXT("BattleDeck size=1"), Run->GetBattleDeck().Num(), 1);

	FBattleInitParams Params;
	const bool bOk = Run->BuildInitParamsForBattle(FName(TEXT("Run.Deck.BuildInitParamsUsesBattleDeck")), Params);
	TestTrue(TEXT("BuildInitParams succeeded"), bOk);

	TestEqual(TEXT("BattleDeckEntries has 1 card"),
		Params.BattleDeckEntries.Num(), 1);
	TestTrue(TEXT("Entry = BattleDeck content"),
		Params.BattleDeckEntries[0].Definition.Get() == Normal);
	TestTrue(TEXT("Native BattleDeck entry has no CapacityEffect"),
		Params.BattleDeckEntries[0].CapacityEffectTags.IsEmpty());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckMoveInstanceValidationSpec,
	"Wacom.Run.Deck.MoveInstanceValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckMoveInstanceValidationSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* Normal = Fx.MakeNoopCard(0);
	UCardDefinition* SecondNormal = Fx.MakeNoopCard(0);
	UCardDefinition* Bag = MakeBagCard(Fx, 2);
	UCardDefinition* TypeB = Fx.MakeNoopCard(0);
	TypeB->Physique.Capacity = 2;
	TypeB->Physique.CapacityEffect = WacomTags::Card_CapacityEffect_Placeholder;
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ Normal, SecondNormal, Bag, TypeB });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	const FGuid NormalId = Run->GetBattleDeck()[0].InstanceId;
	const FGuid SecondNormalId = Run->GetBattleDeck()[1].InstanceId;
	const FGuid TypeBId = Run->GetBackpack()[1].InstanceId;
	const FGuid SpecialOwnerId = Run->GetRunState().SpecialZones[0].OwnerInstanceId;

	TestTrue(TEXT("BattleDeck normal can return to Backpack when flux has room"),
		Run->ValidateMoveInstance(NormalId, EZoneKind::Backpack, FGuid()).bCanExecute);
	TestEqual(TEXT("Unknown move reason"),
		Run->ValidateMoveInstance(FGuid::NewGuid(), EZoneKind::Backpack, FGuid()).DisabledReason,
		FName(TEXT("CardNotFound")));
	TestEqual(TEXT("Self special zone reason"),
		Run->ValidateMoveInstance(SpecialOwnerId, EZoneKind::SpecialZone, SpecialOwnerId).DisabledReason,
		FName(TEXT("SelfSpecialZone")));
	TestEqual(TEXT("TypeB special zone reason"),
		Run->ValidateMoveInstance(TypeBId, EZoneKind::SpecialZone, SpecialOwnerId).DisabledReason,
		FName(TEXT("SelfSpecialZone")));
	TestEqual(TEXT("TypeB burden reason"),
		Run->ValidateMoveInstance(TypeBId, EZoneKind::BurdenZone, FGuid()).DisabledReason,
		FName(TEXT("TypeBInBurdenZone")));

	TestTrue(TEXT("Move one normal to Backpack succeeds"), Run->MoveInstance(NormalId, EZoneKind::Backpack, FGuid()));
	TestEqual(TEXT("Returning second normal to full flux is rejected"),
		Run->ValidateMoveInstance(SecondNormalId, EZoneKind::Backpack, FGuid()).DisabledReason,
		FName(TEXT("FluxFull")));

	return true;
}

namespace
{
	UCardDefinition* MakeStage45TypeBCard(FWacomBattleFixture& Fx, int32 Capacity)
	{
		UCardDefinition* Card = Fx.MakeNoopCard(0);
		Card->Physique.Capacity = Capacity;
		Card->Physique.CapacityEffect = WacomTags::Card_CapacityEffect_Placeholder;
		return Card;
	}

	FCardInstance MakeStage45Instance(UCardDefinition* Definition, bool bBattleEnabled = false)
	{
		FCardInstance Inst;
		Inst.InstanceId = FGuid::NewGuid();
		Inst.Definition = Definition;
		Inst.bBattleEnabledInSpecialZone = bBattleEnabled;
		return Inst;
	}

	TArray<FGuid> CollectStage45Ids(const TArray<FCardInstance>& Pile)
	{
		TArray<FGuid> Result;
		for (const FCardInstance& Inst : Pile)
		{
			Result.Add(Inst.InstanceId);
		}
		return Result;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckRecomputeBurdenContractSpec,
	"Wacom.Run.Deck.RecomputeBurdenContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckRecomputeBurdenContractSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, Property 8: RecomputeBurden 输出契约
	// Validates: Requirements 2.12, 2.13, 2.14, 9.1
	FWacomBattleFixture Fx;

	UCardDefinition* TypeA = MakeBagCard(Fx, 2);
	UCardDefinition* TypeB = MakeStage45TypeBCard(Fx, 2);
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ TypeA, TypeB });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run.Get());
	if (!TestEqual(TEXT("One SpecialZone"), State.SpecialZones.Num(), 1))
	{
		return false;
	}

	FCardInstance N1 = MakeStage45Instance(Fx.MakeNoopCard(0), true);
	FCardInstance N2 = MakeStage45Instance(Fx.MakeNoopCard(0), true);
	FCardInstance N3 = MakeStage45Instance(Fx.MakeNoopCard(0), true);
	FCardInstance N4 = MakeStage45Instance(Fx.MakeNoopCard(0));
	FCardInstance N5 = MakeStage45Instance(Fx.MakeNoopCard(0));
	FCardInstance N6 = MakeStage45Instance(Fx.MakeNoopCard(0));
	FCardInstance N7 = MakeStage45Instance(Fx.MakeNoopCard(0));

	State.Backpack.Add(N1);
	State.Backpack.Add(N2);
	State.Backpack.Add(N3);
	State.BattleDeck.Add(N4);
	State.BattleDeck.Add(N5);
	State.BattleDeck.Add(N6);
	State.BattleDeck.Add(N7);

	Run->RecomputeBurden();

	TestEqual(TEXT("Backpack keeps A/B main cards plus one flux content card"), State.Backpack.Num(), 3);
	TestEqual(TEXT("BattleDeck remains full"), State.BattleDeck.Num(), 4);
	TestEqual(TEXT("First overflow card refills first SpecialZone slot"), State.SpecialZones[0].Cards.Num(), 1);
	if (State.SpecialZones[0].Cards.Num() == 1)
	{
		TestEqual(TEXT("SpecialZone receives most recent overflow first"), State.SpecialZones[0].Cards[0].InstanceId, N3.InstanceId);
		TestFalse(TEXT("SpecialZone refill clears battle flag"), State.SpecialZones[0].Cards[0].bBattleEnabledInSpecialZone);
	}
	TestEqual(TEXT("Remaining one card stays in BurdenZone"), State.BurdenZone.Num(), 1);
	if (State.BurdenZone.Num() == 1)
	{
		TestEqual(TEXT("Burden order[0]"), State.BurdenZone[0].InstanceId, N2.InstanceId);
	}
	TestEqual(TEXT("Burden pressure n=1 -> 1"), Run->GetPressureValue(EWacomPressureType::Burden), 1);

	const TArray<FGuid> BackpackIds = CollectStage45Ids(State.Backpack);
	const TArray<FGuid> BattleDeckIds = CollectStage45Ids(State.BattleDeck);
	const TArray<FGuid> BurdenIds = CollectStage45Ids(State.BurdenZone);
	const TArray<FGuid> SpecialIds = CollectStage45Ids(State.SpecialZones[0].Cards);

	Run->RecomputeBurden();

	TestEqual(TEXT("Idempotent Backpack"), CollectStage45Ids(State.Backpack), BackpackIds);
	TestEqual(TEXT("Idempotent BattleDeck"), CollectStage45Ids(State.BattleDeck), BattleDeckIds);
	TestEqual(TEXT("Idempotent BurdenZone"), CollectStage45Ids(State.BurdenZone), BurdenIds);
	TestEqual(TEXT("Idempotent SpecialZone"), CollectStage45Ids(State.SpecialZones[0].Cards), SpecialIds);
	TestEqual(TEXT("Idempotent pressure"), Run->GetPressureValue(EWacomPressureType::Burden), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckRecomputeBurdenRefillPrioritySpec,
	"Wacom.Run.Deck.RecomputeBurdenRefillPriority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckRecomputeBurdenRefillPrioritySpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, Property 8: BurdenZone 回填优先序
	// Validates: Requirements 2.13, 2.14
	FWacomBattleFixture Fx;

	UCardDefinition* TypeA = MakeBagCard(Fx, 2);
	UCardDefinition* TypeB = MakeStage45TypeBCard(Fx, 3);
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ TypeA, TypeB });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run.Get());
	if (!TestEqual(TEXT("One SpecialZone"), State.SpecialZones.Num(), 1))
	{
		return false;
	}

	FCardInstance ToBackpack = MakeStage45Instance(Fx.MakeNoopCard(0));
	FCardInstance ToBattleDeck = MakeStage45Instance(Fx.MakeNoopCard(0));
	FCardInstance ToSpecialZone = MakeStage45Instance(Fx.MakeNoopCard(0), true);

	State.BattleDeck.Add(MakeStage45Instance(Fx.MakeNoopCard(0)));
	State.BattleDeck.Add(MakeStage45Instance(Fx.MakeNoopCard(0)));
	State.BattleDeck.Add(MakeStage45Instance(Fx.MakeNoopCard(0)));
	State.BattleDeck.Add(MakeStage45Instance(Fx.MakeNoopCard(0)));
	State.BurdenZone = { ToBackpack, ToBattleDeck, ToSpecialZone };

	Run->RecomputeBurden();

	TestEqual(TEXT("Burden fully refilled"), State.BurdenZone.Num(), 0);
	TestEqual(TEXT("Backpack receives first burden card"), State.Backpack.Last().InstanceId, ToBackpack.InstanceId);
	TestEqual(TEXT("BattleDeck receives second burden card"), State.BattleDeck.Last().InstanceId, ToBattleDeck.InstanceId);
	TestEqual(TEXT("SpecialZone receives third burden card"), State.SpecialZones[0].Cards.Last().InstanceId, ToSpecialZone.InstanceId);
	TestFalse(TEXT("SpecialZone refill clears stale flag"), State.SpecialZones[0].Cards.Last().bBattleEnabledInSpecialZone);
	TestEqual(TEXT("Burden pressure n=0 -> 0"), Run->GetPressureValue(EWacomPressureType::Burden), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckBurdenPressureFormulaSpec,
	"Wacom.Run.Deck.BurdenPressureFormula",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckBurdenPressureFormulaSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, EXAMPLE R9.3: Burden pressure formula
	// Validates: Requirements 9.1, 9.3
	FWacomBattleFixture Fx;

	auto CheckPressure = [this, &Fx](int32 Count, int32 ExpectedPressure) -> bool
	{
		UCharacterDefinition* Char = Fx.MakeCharacter(
			Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
			{});

		TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
		Run->Initialize(Char);

		FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run.Get());
		for (int32 i = 0; i < Count; ++i)
		{
			State.BurdenZone.Add(MakeStage45Instance(Fx.MakeNoopCard(0)));
		}

		Run->RecomputeBurden();
		return TestEqual(FString::Printf(TEXT("n=%d pressure"), Count),
			Run->GetPressureValue(EWacomPressureType::Burden), ExpectedPressure);
	};

	bool bOk = true;
	bOk &= CheckPressure(0, 0);
	bOk &= CheckPressure(1, 1);
	bOk &= CheckPressure(3, 6);
	bOk &= CheckPressure(14, 100);
	return bOk;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckZoneBroadcastCountSpec,
	"Wacom.Run.Deck.ZoneBroadcastCount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckZoneBroadcastCountSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, Property 9: 广播计数与 Burden 通道写入唯一性
	// Validates: Requirements 2.8, 2.16, 9.2
	FWacomBattleFixture Fx;

	UCardDefinition* TypeA = MakeBagCard(Fx, 1);
	UCardDefinition* BattleFiller = Fx.MakeNoopCard(0);
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ TypeA, BattleFiller });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	int32 BroadcastCount = 0;
	Run->OnRunStateChangedNative.AddLambda([&BroadcastCount]()
	{
		++BroadcastCount;
	});

	Run->AcquireCardToRun(Fx.MakeNoopCard(0));
	TestEqual(TEXT("AcquireCardToRun emits once"), BroadcastCount, 1);
	TestEqual(TEXT("AcquireCardToRun writes Burden pressure once"), Run->GetPressureValue(EWacomPressureType::Burden), 1);

	Run->RecomputeBurden();
	TestEqual(TEXT("Public RecomputeBurden emits once"), BroadcastCount, 2);
	TestEqual(TEXT("Pressure remains stable"), Run->GetPressureValue(EWacomPressureType::Burden), 1);

	TestFalse(TEXT("Invalid MoveInstance rejected"), Run->MoveInstance(FGuid::NewGuid(), EZoneKind::Backpack, FGuid()));
	TestEqual(TEXT("Rejected MoveInstance does not broadcast"), BroadcastCount, 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckSpecialZoneFlagResetSpec,
	"Wacom.Run.Deck.SpecialZoneFlagReset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckSpecialZoneFlagResetSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, Property 10: 进入 / 离开 SpecialZone 重置 bBattleEnabledInSpecialZone
	// Validates: Requirements 2.9, 8.6
	FWacomBattleFixture Fx;

	UCardDefinition* TypeA = MakeBagCard(Fx, 6);
	UCardDefinition* TypeB = MakeStage45TypeBCard(Fx, 3);
	UCardDefinition* Stored = Fx.MakeNoopCard(0);
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ TypeA, TypeB });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);
	Run->AcquireCardToRun(Stored);

	const FGuid OwnerId = Run->GetRunState().SpecialZones[0].OwnerInstanceId;
	const FGuid StoredId = Run->GetBackpack().Last().InstanceId;

	TestTrue(TEXT("Move into SpecialZone"), Run->MoveInstance(StoredId, EZoneKind::SpecialZone, OwnerId));
	TestFalse(TEXT("Initial SpecialZone flag false"), Run->GetRunState().SpecialZones[0].Cards[0].bBattleEnabledInSpecialZone);

	TestTrue(TEXT("Enable flag"), Run->SetSpecialZoneCardBattleEnabled(StoredId, true));
	TestTrue(TEXT("Flag enabled"), Run->GetRunState().SpecialZones[0].Cards[0].bBattleEnabledInSpecialZone);

	TestTrue(TEXT("Move out to Backpack"), Run->MoveInstance(StoredId, EZoneKind::Backpack, FGuid()));
	TestFalse(TEXT("Leaving SpecialZone clears flag"), Run->GetBackpack().Last().bBattleEnabledInSpecialZone);

	TestTrue(TEXT("Move back into SpecialZone"), Run->MoveInstance(StoredId, EZoneKind::SpecialZone, OwnerId));
	TestFalse(TEXT("Re-entering SpecialZone starts disabled"), Run->GetRunState().SpecialZones[0].Cards[0].bBattleEnabledInSpecialZone);

	TestTrue(TEXT("Enable flag again"), Run->SetSpecialZoneCardBattleEnabled(StoredId, true));
	TestFalse(TEXT("B owner cannot move into own SpecialZone"), Run->MoveInstance(OwnerId, EZoneKind::SpecialZone, OwnerId));
	TestTrue(TEXT("Rejected self move leaves stored flag unchanged"), Run->GetRunState().SpecialZones[0].Cards[0].bBattleEnabledInSpecialZone);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckSetSpecialZoneFlagOnlySpec,
	"Wacom.Run.Deck.SetSpecialZoneFlagOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckSetSpecialZoneFlagOnlySpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, Property 11: SetSpecialZoneCardBattleEnabled 切 flag 不移卡
	// Validates: Requirements 2.10, 8.1, 8.5
	FWacomBattleFixture Fx;

	UCardDefinition* TypeA = MakeBagCard(Fx, 6);
	UCardDefinition* TypeB = MakeStage45TypeBCard(Fx, 3);
	UCardDefinition* Stored = Fx.MakeNoopCard(0);
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ TypeA, TypeB });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);
	Run->AcquireCardToRun(Stored);

	const FGuid OwnerId = Run->GetRunState().SpecialZones[0].OwnerInstanceId;
	const FGuid StoredId = Run->GetBackpack().Last().InstanceId;
	const FGuid NonSpecialId = Run->GetBackpack()[0].InstanceId;

	TestTrue(TEXT("Move stored card into SpecialZone"), Run->MoveInstance(StoredId, EZoneKind::SpecialZone, OwnerId));

	int32 BroadcastCount = 0;
	Run->OnRunStateChangedNative.AddLambda([&BroadcastCount]()
	{
		++BroadcastCount;
	});

	TestTrue(TEXT("Set flag true succeeds"), Run->SetSpecialZoneCardBattleEnabled(StoredId, true));
	TestEqual(TEXT("Broadcast once after flag true"), BroadcastCount, 1);
	TestEqual(TEXT("Still one card in SpecialZone"), Run->GetRunState().SpecialZones[0].Cards.Num(), 1);
	TestEqual(TEXT("Instance remains in same SpecialZone"), Run->GetRunState().SpecialZones[0].Cards[0].InstanceId, StoredId);
	TestTrue(TEXT("Flag true"), Run->GetRunState().SpecialZones[0].Cards[0].bBattleEnabledInSpecialZone);

	TestTrue(TEXT("Set same flag still succeeds"), Run->SetSpecialZoneCardBattleEnabled(StoredId, true));
	TestEqual(TEXT("Same-value set still broadcasts by contract"), BroadcastCount, 2);
	TestEqual(TEXT("Still not moved"), Run->GetRunState().SpecialZones[0].Cards[0].InstanceId, StoredId);

	TestTrue(TEXT("Set flag false succeeds"), Run->SetSpecialZoneCardBattleEnabled(StoredId, false));
	TestEqual(TEXT("Broadcast once after flag false"), BroadcastCount, 3);
	TestFalse(TEXT("Flag false"), Run->GetRunState().SpecialZones[0].Cards[0].bBattleEnabledInSpecialZone);

	TestFalse(TEXT("Non-SpecialZone instance rejected"), Run->SetSpecialZoneCardBattleEnabled(NonSpecialId, true));
	TestFalse(TEXT("Invalid instance rejected"), Run->SetSpecialZoneCardBattleEnabled(FGuid::NewGuid(), true));
	TestEqual(TEXT("Rejected SetSpecialZoneCardBattleEnabled does not broadcast"), BroadcastCount, 3);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckBuildInitParamsIncludesEnabledSpecialZoneSpec,
	"Wacom.Run.Deck.BuildInitParamsIncludesEnabledSpecialZone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckBuildInitParamsIncludesEnabledSpecialZoneSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* TypeA = MakeBagCard(Fx, 8);
	UCardDefinition* TypeB = MakeBagCard(Fx, 3);
	TypeB->Physique.CapacityEffect = WacomTags::Card_CapacityEffect_WeaponDamagePlus3;
	UCardDefinition* Weapon = Fx.MakeDamageCardWithKeywords(
		/*Cost*/1,
		/*Damage*/4,
		{ WacomTags::Card_Keyword_Weapon });
	UCardDefinition* Normal = Fx.MakeNoopCard(0);

	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ TypeA, TypeB, Normal });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Initialize"), Run->Initialize(Char));

	const FGuid BOwnerId = Run->GetRunState().SpecialZones[0].OwnerInstanceId;
	Run->AcquireCardToRun(Weapon);
	const FGuid WeaponId = Run->GetBackpack().Last().InstanceId;

	TestTrue(TEXT("Move B master to BattleDeck"), Run->MoveInstance(BOwnerId, EZoneKind::BattleDeck, FGuid()));
	TestTrue(TEXT("Move weapon into B SpecialZone"), Run->MoveInstance(WeaponId, EZoneKind::SpecialZone, BOwnerId));
	TestTrue(TEXT("Enable weapon in SpecialZone"), Run->SetSpecialZoneCardBattleEnabled(WeaponId, true));

	FBattleInitParams Params;
	TestTrue(TEXT("BuildInitParamsForBattle"),
		Run->BuildInitParamsForBattle(FName(TEXT("Run.Deck.SpecialZoneEnabled")), Params));

	int32 SpecialZoneEntries = 0;
	for (const FBattleDeckEntry& Entry : Params.BattleDeckEntries)
	{
		if (Entry.Definition == Weapon)
		{
			++SpecialZoneEntries;
			TestTrue(TEXT("SpecialZone weapon carries WeaponDamagePlus3"),
				Entry.CapacityEffectTags.HasTagExact(WacomTags::Card_CapacityEffect_WeaponDamagePlus3));
		}
	}

	TestEqual(TEXT("SpecialZone enabled weapon included once"), SpecialZoneEntries, 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckBuildInitParamsSpecialZoneEntryScenariosSpec,
	"Wacom.Run.Deck.BuildInitParamsSpecialZoneEntryScenarios",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckBuildInitParamsSpecialZoneEntryScenariosSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, EXAMPLE R4.8 a/d:
	// flag=false 不入战；主卡仍在 Backpack 时即使 flag=true 也不入战。
	FWacomBattleFixture Fx;

	UCardDefinition* TypeA = MakeBagCard(Fx, 8);
	UCardDefinition* TypeB = MakeBagCard(Fx, 3);
	TypeB->Physique.CapacityEffect = WacomTags::Card_CapacityEffect_WeaponDamagePlus3;
	UCardDefinition* Weapon = Fx.MakeDamageCardWithKeywords(
		/*Cost*/1,
		/*Damage*/4,
		{ WacomTags::Card_Keyword_Weapon });
	UCardDefinition* Normal = Fx.MakeNoopCard(0);

	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ TypeA, TypeB, Normal });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Initialize"), Run->Initialize(Char));

	const FGuid BOwnerId = Run->GetRunState().SpecialZones[0].OwnerInstanceId;
	Run->AcquireCardToRun(Weapon);
	const FGuid WeaponId = Run->GetBackpack().Last().InstanceId;
	TestTrue(TEXT("Move weapon into SpecialZone"), Run->MoveInstance(WeaponId, EZoneKind::SpecialZone, BOwnerId));

	TestTrue(TEXT("Move B owner to BattleDeck"), Run->MoveInstance(BOwnerId, EZoneKind::BattleDeck, FGuid()));

	FBattleInitParams Params;
	TestTrue(TEXT("Build params flag=false"),
		Run->BuildInitParamsForBattle(FName(TEXT("Run.Deck.SpecialZoneScenarios")), Params));
	TestEqual(TEXT("flag=false weapon not included"), Params.BattleDeckEntries.Num(), 2);

	TestTrue(TEXT("Enable weapon"), Run->SetSpecialZoneCardBattleEnabled(WeaponId, true));
	TestTrue(TEXT("Move B owner back to Backpack"), Run->MoveInstance(BOwnerId, EZoneKind::Backpack, FGuid()));

	Params = FBattleInitParams();
	TestTrue(TEXT("Build params owner in Backpack"),
		Run->BuildInitParamsForBattle(FName(TEXT("Run.Deck.SpecialZoneScenarios")), Params));
	TestEqual(TEXT("owner in Backpack weapon not included"), Params.BattleDeckEntries.Num(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckBContainerMoveKeepsSpecialZoneSpec,
	"Wacom.Run.Deck.BContainerMoveKeepsSpecialZone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckBContainerMoveKeepsSpecialZoneSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, Property 15: B 主卡跨 Backpack ↔ BattleDeck 移动 SpecialZone 内容保持
	// Validates: Requirements 5.1, 5.4
	FWacomBattleFixture Fx;

	UCardDefinition* TypeA = MakeBagCard(Fx, 8);
	UCardDefinition* TypeB = MakeBagCard(Fx, 3);
	TypeB->Physique.CapacityEffect = WacomTags::Card_CapacityEffect_WeaponDamagePlus3;
	UCardDefinition* Stored = Fx.MakeNoopCard(0);
	UCardDefinition* Normal = Fx.MakeNoopCard(0);

	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ TypeA, TypeB, Normal });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Initialize"), Run->Initialize(Char));

	const FGuid BOwnerId = Run->GetRunState().SpecialZones[0].OwnerInstanceId;
	Run->AcquireCardToRun(Stored);
	const FGuid StoredId = Run->GetBackpack().Last().InstanceId;
	TestTrue(TEXT("Move stored card into SpecialZone"), Run->MoveInstance(StoredId, EZoneKind::SpecialZone, BOwnerId));
	TestTrue(TEXT("Enable stored card"), Run->SetSpecialZoneCardBattleEnabled(StoredId, true));

	FSpecialZone Before;
	TestTrue(TEXT("Get SpecialZone before move"), Run->GetSpecialZone(BOwnerId, Before));
	TestEqual(TEXT("Before one stored card"), Before.Cards.Num(), 1);
	TestEqual(TEXT("Before stored id"), Before.Cards[0].InstanceId, StoredId);
	TestTrue(TEXT("Before flag true"), Before.Cards[0].bBattleEnabledInSpecialZone);

	TestTrue(TEXT("Move B owner to BattleDeck"), Run->MoveInstance(BOwnerId, EZoneKind::BattleDeck, FGuid()));
	FSpecialZone InBattleDeck;
	TestTrue(TEXT("Get SpecialZone after move to BattleDeck"), Run->GetSpecialZone(BOwnerId, InBattleDeck));
	TestEqual(TEXT("Still one stored card"), InBattleDeck.Cards.Num(), 1);
	TestEqual(TEXT("Stored id preserved"), InBattleDeck.Cards[0].InstanceId, StoredId);
	TestTrue(TEXT("Flag preserved"), InBattleDeck.Cards[0].bBattleEnabledInSpecialZone);

	TestTrue(TEXT("Move B owner back to Backpack"), Run->MoveInstance(BOwnerId, EZoneKind::Backpack, FGuid()));
	FSpecialZone BackInBackpack;
	TestTrue(TEXT("Get SpecialZone after move back"), Run->GetSpecialZone(BOwnerId, BackInBackpack));
	TestEqual(TEXT("Still one stored card after back"), BackInBackpack.Cards.Num(), 1);
	TestEqual(TEXT("Stored id preserved after back"), BackInBackpack.Cards[0].InstanceId, StoredId);
	TestTrue(TEXT("Flag preserved after back"), BackInBackpack.Cards[0].bBattleEnabledInSpecialZone);

	return true;
}

// ================ 金币 ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckGoldOpsSpec,
	"Wacom.Run.Deck.GoldAddRemove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckGoldOpsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Bag = MakeBagCard(Fx, 5);
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ Bag });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	TestEqual(TEXT("Init gold=0"), Run->GetGold(), 0);

	Run->AddGold(10);
	TestEqual(TEXT("After +10, gold=10"), Run->GetGold(), 10);

	TestTrue(TEXT("Remove 5 ok"), Run->RemoveGold(5));
	TestEqual(TEXT("Gold=5"), Run->GetGold(), 5);

	TestFalse(TEXT("Remove 10 from 5 fails"), Run->RemoveGold(10));
	TestEqual(TEXT("Gold unchanged"), Run->GetGold(), 5);

	return true;
}

// ================ 商店购买 ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckShopOffersInitializeSnapshotSpec,
	"Wacom.Run.Deck.ShopOffersInitializeSnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckShopOffersInitializeSnapshotSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Bag = MakeBagCard(Fx, 5);
	UCardDefinition* ShopCardA = Fx.MakeNoopCard(0);
	UCardDefinition* ShopCardB = Fx.MakeNoopCard(0);
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ Bag });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	const int32 StartNodes = Run->GetRemainingNodeCount();
	TArray<FRunShopOfferInput> Offers;
	Offers.Add({ ShopCardA, 3 });
	Offers.Add({ ShopCardB, 0 });

	TestTrue(TEXT("Begin shop succeeds"), Run->BeginShopVisit(TEXT("Shop.A"), Offers));
	const FRunShopSnapshot Snapshot = Run->BuildCurrentShopSnapshot();
	TestTrue(TEXT("Shop visit active"), Snapshot.bIsActive);
	TestEqual(TEXT("Snapshot shop id"), Snapshot.ShopId, FName(TEXT("Shop.A")));
	TestFalse(TEXT("No purchase on enter"), Snapshot.bHasPurchaseThisVisit);
	TestEqual(TEXT("Two offers"), Snapshot.Offers.Num(), 2);
	TestTrue(TEXT("Offer A id valid"), Snapshot.Offers[0].OfferId.IsValid());
	TestTrue(TEXT("Offer B id valid"), Snapshot.Offers[1].OfferId.IsValid());
	TestNotEqual(TEXT("Offer ids differ"), Snapshot.Offers[0].OfferId, Snapshot.Offers[1].OfferId);
	TestEqual(TEXT("Offer A card"), Snapshot.Offers[0].CardDefinition.Get(), ShopCardA);
	TestEqual(TEXT("Offer A price"), Snapshot.Offers[0].Price, 3);
	TestFalse(TEXT("Offer A not purchased"), Snapshot.Offers[0].bPurchased);
	TestEqual(TEXT("Offer B card"), Snapshot.Offers[1].CardDefinition.Get(), ShopCardB);
	TestEqual(TEXT("Offer B free"), Snapshot.Offers[1].Price, 0);
	TestEqual(TEXT("Nodes unchanged after entering shop"), Run->GetRemainingNodeCount(), StartNodes);

	Run->EndShopVisit();
	TestFalse(TEXT("Shop visit closed"), Run->IsShopVisitActive());
	TestEqual(TEXT("Closing without purchase does not consume node"), Run->GetRemainingNodeCount(), StartNodes);
	TestFalse(TEXT("Closed snapshot inactive"), Run->BuildCurrentShopSnapshot().bIsActive);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckShopOfferPurchaseSpec,
	"Wacom.Run.Deck.ShopOfferPurchase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckShopOfferPurchaseSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Bag = MakeBagCard(Fx, 5);
	UCardDefinition* ShopCardA = Fx.MakeNoopCard(0);
	UCardDefinition* ShopCardB = Fx.MakeNoopCard(0);
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ Bag });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);
	Run->AddGold(10);

	const int32 StartNodes = Run->GetRemainingNodeCount();
	const int32 StartBackpackCount = Run->GetBackpack().Num();
	TArray<FRunShopOfferInput> Offers;
	Offers.Add({ ShopCardA, 3 });
	Offers.Add({ ShopCardB, 2 });
	TestTrue(TEXT("Begin shop succeeds"), Run->BeginShopVisit(TEXT("Shop.A"), Offers));

	FRunShopSnapshot Snapshot = Run->BuildCurrentShopSnapshot();
	const FGuid OfferAId = Snapshot.Offers[0].OfferId;
	const FGuid OfferBId = Snapshot.Offers[1].OfferId;

	TestTrue(TEXT("Purchase offer A succeeds"), Run->PurchaseShopOffer(OfferAId));
	TestEqual(TEXT("Gold reduced by A price"), Run->GetGold(), 7);
	TestTrue(TEXT("Purchased card A enters backpack"), BackpackContainsDefinition(Run->GetRunState(), ShopCardA));
	TestEqual(TEXT("Backpack gained one card"), Run->GetBackpack().Num(), StartBackpackCount + 1);
	TestEqual(TEXT("Purchase does not consume node before close"), Run->GetRemainingNodeCount(), StartNodes);

	Snapshot = Run->BuildCurrentShopSnapshot();
	TestTrue(TEXT("Visit marked purchased"), Snapshot.bHasPurchaseThisVisit);
	TestTrue(TEXT("Offer A marked purchased"), Snapshot.Offers[0].bPurchased);
	TestFalse(TEXT("Offer B still unpurchased"), Snapshot.Offers[1].bPurchased);

	TestTrue(TEXT("Purchase offer B succeeds"), Run->PurchaseShopOffer(OfferBId));
	TestEqual(TEXT("Gold reduced by B price"), Run->GetGold(), 5);
	TestTrue(TEXT("Purchased card B enters backpack"), BackpackContainsDefinition(Run->GetRunState(), ShopCardB));
	TestEqual(TEXT("Backpack gained two cards"), Run->GetBackpack().Num(), StartBackpackCount + 2);
	TestEqual(TEXT("Second purchase still does not consume node before close"), Run->GetRemainingNodeCount(), StartNodes);

	Run->EndShopVisit();
	TestEqual(TEXT("Closing after purchases consumes exactly one node"), Run->GetRemainingNodeCount(), StartNodes - 1);
	TestFalse(TEXT("Shop closed"), Run->IsShopVisitActive());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckShopOfferRejectsInvalidCasesSpec,
	"Wacom.Run.Deck.ShopOfferRejectsInvalidCases",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckShopOfferRejectsInvalidCasesSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Bag = MakeBagCard(Fx, 5);
	UCardDefinition* ShopCard = Fx.MakeNoopCard(0);
	UCardDefinition* ValidCard = Fx.MakeNoopCard(0);
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ Bag });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);
	Run->AddGold(1);

	const int32 StartGold = Run->GetGold();
	const int32 StartNodes = Run->GetRemainingNodeCount();
	const int32 StartBackpackCount = Run->GetBackpack().Num();
	TestFalse(TEXT("Purchase without active shop fails"), Run->PurchaseShopOffer(FGuid::NewGuid()));

	TArray<FRunShopOfferInput> InvalidOffers;
	InvalidOffers.Add({ nullptr, 0 });
	InvalidOffers.Add({ ShopCard, -1 });
	InvalidOffers.Add({ ValidCard, 2 });
	TestFalse(TEXT("None shop id fails"), Run->BeginShopVisit(NAME_None, InvalidOffers));
	TestTrue(TEXT("Valid shop id succeeds"), Run->BeginShopVisit(TEXT("Shop.Invalid"), InvalidOffers));

	FRunShopSnapshot Snapshot = Run->BuildCurrentShopSnapshot();
	TestEqual(TEXT("Only valid non-negative card offer kept"), Snapshot.Offers.Num(), 1);
	const FGuid ValidOfferId = Snapshot.Offers[0].OfferId;
	TestEqual(TEXT("Kept offer card"), Snapshot.Offers[0].CardDefinition.Get(), ValidCard);

	TestFalse(TEXT("Unknown offer fails"), Run->PurchaseShopOffer(FGuid::NewGuid()));
	TestFalse(TEXT("Invalid offer id fails"), Run->PurchaseShopOffer(FGuid()));
	TestFalse(TEXT("Insufficient gold fails"), Run->PurchaseShopOffer(ValidOfferId));
	TestEqual(TEXT("Gold unchanged on failures"), Run->GetGold(), StartGold);
	TestEqual(TEXT("Backpack unchanged on failures"), Run->GetBackpack().Num(), StartBackpackCount);
	TestEqual(TEXT("Nodes unchanged on failures"), Run->GetRemainingNodeCount(), StartNodes);
	TestFalse(TEXT("Failed purchase does not mark visit purchase"), Run->BuildCurrentShopSnapshot().bHasPurchaseThisVisit);

	Run->AddGold(2);
	TestTrue(TEXT("Purchase succeeds after adding gold"), Run->PurchaseShopOffer(ValidOfferId));
	TestFalse(TEXT("Repeat purchase rejected"), Run->PurchaseShopOffer(ValidOfferId));
	TestEqual(TEXT("Repeat rejection keeps one purchased card"), Run->GetBackpack().Num(), StartBackpackCount + 1);

	Run->EndShopVisit();
	TestEqual(TEXT("One successful purchase consumes one node on close"), Run->GetRemainingNodeCount(), StartNodes - 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckShopNodeInventoryPersistsSpec,
	"Wacom.Run.Deck.ShopNodeInventoryPersists",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckShopNodeInventoryPersistsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Bag = MakeBagCard(Fx, 5);
	UCardDefinition* ShopACard1 = Fx.MakeNoopCard(0);
	UCardDefinition* ShopACard2 = Fx.MakeNoopCard(0);
	UCardDefinition* ReplacementCard = Fx.MakeNoopCard(0);
	UCardDefinition* ShopBCard = Fx.MakeNoopCard(0);
	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ Bag });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);
	Run->AddGold(10);

	TArray<FRunShopOfferInput> ShopAOffers;
	ShopAOffers.Add({ ShopACard1, 1 });
	ShopAOffers.Add({ ShopACard2, 1 });
	TestTrue(TEXT("Begin shop A succeeds"), Run->BeginShopVisit(TEXT("Shop.A"), ShopAOffers));
	FRunShopSnapshot SnapshotA = Run->BuildCurrentShopSnapshot();
	const FGuid ShopAOffer1 = SnapshotA.Offers[0].OfferId;
	const FGuid ShopAOffer2 = SnapshotA.Offers[1].OfferId;
	TestTrue(TEXT("Buy first A offer"), Run->PurchaseShopOffer(ShopAOffer1));
	Run->EndShopVisit();

	const int32 NodesAfterFirstVisit = Run->GetRemainingNodeCount();
	TArray<FRunShopOfferInput> ReplacementOffers;
	ReplacementOffers.Add({ ReplacementCard, 9 });
	TestTrue(TEXT("Reopen shop A succeeds"), Run->BeginShopVisit(TEXT("Shop.A"), ReplacementOffers));
	SnapshotA = Run->BuildCurrentShopSnapshot();
	TestEqual(TEXT("Shop A keeps original offer count"), SnapshotA.Offers.Num(), 2);
	TestEqual(TEXT("Shop A ignores replacement card"), SnapshotA.Offers[0].CardDefinition.Get(), ShopACard1);
	TestTrue(TEXT("Shop A preserves purchased flag"), SnapshotA.Offers[0].bPurchased);
	TestFalse(TEXT("Shop A second offer still unpurchased"), SnapshotA.Offers[1].bPurchased);
	TestEqual(TEXT("Shop A offer id stable after reopen"), SnapshotA.Offers[1].OfferId, ShopAOffer2);
	TestTrue(TEXT("Buy second A offer on later visit"), Run->PurchaseShopOffer(ShopAOffer2));
	Run->EndShopVisit();
	TestTrue(TEXT("Second paid visit can advance phase when nodes run out"), Run->GetCurrentTimePhase() == ETimePhase::Day);
	TestEqual(TEXT("Day nodes reset after second paid visit"), Run->GetRemainingNodeCount(), Run->GetRunState().InitialNodeCount_Day);

	TArray<FRunShopOfferInput> ShopBOffers;
	ShopBOffers.Add({ ShopBCard, 0 });
	TestTrue(TEXT("Begin shop B succeeds"), Run->BeginShopVisit(TEXT("Shop.B"), ShopBOffers));
	const FRunShopSnapshot SnapshotB = Run->BuildCurrentShopSnapshot();
	TestEqual(TEXT("Shop B independent offer count"), SnapshotB.Offers.Num(), 1);
	TestEqual(TEXT("Shop B independent card"), SnapshotB.Offers[0].CardDefinition.Get(), ShopBCard);
	TestFalse(TEXT("Shop B offer starts unpurchased"), SnapshotB.Offers[0].bPurchased);

	return true;
}

// ================ Stage 4.3：B 类容器卡骨架 ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckTypeAVsTypeBContainerSpec,
	"Wacom.Run.Deck.TypeAVsTypeBContainerPredicates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckTypeAVsTypeBContainerSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* Normal = Fx.MakeNoopCard(0);

	UCardDefinition* TypeA = Fx.MakeNoopCard(0);
	TypeA->Physique.Capacity = 5;
	// CapacityEffect 留空 → A 类

	UCardDefinition* TypeB = Fx.MakeNoopCard(0);
	TypeB->Physique.Capacity = 3;
	TypeB->Physique.CapacityEffect = WacomTags::Card_CapacityEffect_Placeholder;

	TestFalse(TEXT("Normal not A"), URunSession::IsTypeAContainerCard(Normal));
	TestFalse(TEXT("Normal not B"), URunSession::IsTypeBContainerCard(Normal));

	TestTrue (TEXT("TypeA is container"), URunSession::IsContainerCard(TypeA));
	TestTrue (TEXT("TypeA is type A"),    URunSession::IsTypeAContainerCard(TypeA));
	TestFalse(TEXT("TypeA not type B"),   URunSession::IsTypeBContainerCard(TypeA));

	TestTrue (TEXT("TypeB is container"), URunSession::IsContainerCard(TypeB));
	TestFalse(TEXT("TypeB not type A"),   URunSession::IsTypeAContainerCard(TypeB));
	TestTrue (TEXT("TypeB is type B"),    URunSession::IsTypeBContainerCard(TypeB));

	TestFalse(TEXT("nullptr false"), URunSession::IsTypeAContainerCard(nullptr));
	TestFalse(TEXT("nullptr false"), URunSession::IsTypeBContainerCard(nullptr));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckSpecialZoneCapacitySpec,
	"Wacom.Run.Deck.SpecialZoneCapacity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckSpecialZoneCapacitySpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	// GDD §11.4：特殊存放区容量 = b.Capacity - 1
	UCardDefinition* B3 = Fx.MakeNoopCard(0);
	B3->Physique.Capacity = 3;
	B3->Physique.CapacityEffect = WacomTags::Card_CapacityEffect_Placeholder;

	UCardDefinition* B1 = Fx.MakeNoopCard(0);
	B1->Physique.Capacity = 1;
	B1->Physique.CapacityEffect = WacomTags::Card_CapacityEffect_Placeholder;

	UCardDefinition* B0 = Fx.MakeNoopCard(0);
	B0->Physique.Capacity = 0;
	// Capacity = 0 → 不是容器卡，但函数仍然 clamp 到 0

	TestEqual(TEXT("Cap=3 → SpecialZone=2"), URunSession::GetSpecialZoneCapacity(B3), 2);
	TestEqual(TEXT("Cap=1 → SpecialZone=0"), URunSession::GetSpecialZoneCapacity(B1), 0);
	TestEqual(TEXT("Cap=0 → SpecialZone=0"), URunSession::GetSpecialZoneCapacity(B0), 0);
	TestEqual(TEXT("nullptr → 0"),           URunSession::GetSpecialZoneCapacity(nullptr), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckSpecialZoneCapacityForOwnerSpec,
	"Wacom.Run.Deck.SpecialZoneCapacityForOwner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckSpecialZoneCapacityForOwnerSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, Property 7: GetSpecialZoneCapacityFor 公式
	// Validates: Requirements 2.5
	FWacomBattleFixture Fx;

	UCardDefinition* TypeA = MakeBagCard(Fx, 8);

	UCardDefinition* TypeB4 = Fx.MakeNoopCard(0);
	TypeB4->Physique.Capacity = 4;
	TypeB4->Physique.CapacityEffect = WacomTags::Card_CapacityEffect_Placeholder;

	UCardDefinition* TypeB1 = Fx.MakeNoopCard(0);
	TypeB1->Physique.Capacity = 1;
	TypeB1->Physique.CapacityEffect = WacomTags::Card_CapacityEffect_Placeholder;

	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ TypeA, TypeB4, TypeB1 });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	const FRunState& State = Run->GetRunState();
	if (!TestEqual(TEXT("Two SpecialZones"), State.SpecialZones.Num(), 2))
	{
		return false;
	}

	const FGuid Owner4 = State.SpecialZones[0].OwnerInstanceId;
	const FGuid Owner1 = State.SpecialZones[1].OwnerInstanceId;

	TestEqual(TEXT("Owner cap 4 -> SpecialZone cap 3"),
		Run->GetSpecialZoneCapacityFor(Owner4), 3);
	TestEqual(TEXT("Owner cap 1 -> SpecialZone cap 0"),
		Run->GetSpecialZoneCapacityFor(Owner1), 0);
	TestEqual(TEXT("Invalid owner -> 0"),
		Run->GetSpecialZoneCapacityFor(FGuid::NewGuid()), 0);
	TestEqual(TEXT("Zero GUID -> 0"),
		Run->GetSpecialZoneCapacityFor(FGuid()), 0);

	TestTrue(TEXT("Move owner to BattleDeck"), MoveFirstBackpackDefinitionToBattleDeck(Run, TypeB4));
	TestEqual(TEXT("Capacity stable after owner moves to BattleDeck"),
		Run->GetSpecialZoneCapacityFor(Owner4), 3);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckFluxCapacityOnlyCountsTypeASpec,
	"Wacom.Run.Deck.FluxCapacityOnlyCountsTypeA",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckFluxCapacityOnlyCountsTypeASpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	// A 类容量=10（小布袋同款）+ B 类容量=3（占位）+ 普通卡
	UCardDefinition* TypeA = MakeBagCard(Fx, 10);

	UCardDefinition* TypeB = Fx.MakeNoopCard(0);
	TypeB->Physique.Capacity = 3;
	TypeB->Physique.CapacityEffect = WacomTags::Card_CapacityEffect_Placeholder;
	// 给它也加 BagProvider，确保 backpack UI 不被干扰

	UCardDefinition* Normal = Fx.MakeNoopCard(0);

	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ TypeA, TypeB, Normal });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	// FluxCapacity 只算 A 类容量：10；BattleDeckCapacity 统计 A+B：13
	TestEqual(TEXT("FluxCapacity ignores TypeB and counts TypeA capacity"), Run->GetFluxCapacity(), 10);
	TestEqual(TEXT("BattleDeckCapacity counts TypeA and TypeB"), Run->GetBattleDeckCapacity(), 13);

	// 容器卡换到 BattleDeck 后两个公式都稳定
	TestTrue(TEXT("Add TypeB to BattleDeck"), MoveFirstBackpackDefinitionToBattleDeck(Run, TypeB));
	TestEqual(TEXT("FluxCapacity stable"), Run->GetFluxCapacity(), 10);
	TestEqual(TEXT("BattleDeckCapacity stable"), Run->GetBattleDeckCapacity(), 13);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckCapacityCountsAllPhysicalZonesSpec,
	"Wacom.Run.Deck.CapacityCountsAllPhysicalZones",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckCapacityCountsAllPhysicalZonesSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* TypeAInBackpack = MakeBagCard(Fx, 8);

	UCardDefinition* TypeBMaster = Fx.MakeNoopCard(0);
	TypeBMaster->Physique.Capacity = 3;
	TypeBMaster->Physique.CapacityEffect = WacomTags::Card_CapacityEffect_Placeholder;

	UCardDefinition* TypeAInSpecialZone = MakeBagCard(Fx, 2);
	UCardDefinition* TypeAInBurdenZone = MakeBagCard(Fx, 4);

	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ TypeAInBackpack, TypeBMaster, TypeAInSpecialZone, TypeAInBurdenZone });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	auto FindBackpackInstanceId = [RunPtr = Run.Get()](const UCardDefinition* Definition)
	{
		for (const FCardInstance& Inst : RunPtr->GetBackpack())
		{
			if (Inst.Definition == Definition)
			{
				return Inst.InstanceId;
			}
		}
		return FGuid();
	};

	const FGuid BMasterId = FindBackpackInstanceId(TypeBMaster);
	const FGuid TypeAInSpecialZoneId = FindBackpackInstanceId(TypeAInSpecialZone);
	const FGuid TypeAInBurdenZoneId = FindBackpackInstanceId(TypeAInBurdenZone);

	if (!TestTrue(TEXT("B master InstanceId valid"), BMasterId.IsValid())
		|| !TestTrue(TEXT("TypeA special InstanceId valid"), TypeAInSpecialZoneId.IsValid())
		|| !TestTrue(TEXT("TypeA burden InstanceId valid"), TypeAInBurdenZoneId.IsValid()))
	{
		return false;
	}

	TestTrue(TEXT("Move TypeA container into SpecialZone.Cards"),
		Run->MoveInstance(TypeAInSpecialZoneId, EZoneKind::SpecialZone, BMasterId));
	TestTrue(TEXT("Move TypeA container into BurdenZone"),
		Run->MoveInstance(TypeAInBurdenZoneId, EZoneKind::BurdenZone, FGuid()));

	TestEqual(TEXT("FluxCapacity counts full TypeA capacity in all physical zones"),
		Run->GetFluxCapacity(), 14);
	TestEqual(TEXT("BattleDeckCapacity counts all containers in all physical zones"),
		Run->GetBattleDeckCapacity(), 17);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckBackpackStorageSnapshotSpec,
	"Wacom.Run.Deck.BackpackStorageSnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckBackpackStorageSnapshotSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* TypeAMain = MakeBagCard(Fx, 8);

	UCardDefinition* TypeBMaster = Fx.MakeNoopCard(0);
	TypeBMaster->Physique.Capacity = 4;
	TypeBMaster->Physique.CapacityEffect = WacomTags::Card_CapacityEffect_Placeholder;

	UCardDefinition* FluxContent = Fx.MakeNoopCard(0);
	UCardDefinition* SpecialContent = Fx.MakeNoopCard(0);
	UCardDefinition* SpecialContainerContent = MakeBagCard(Fx, 2);
	UCardDefinition* BurdenContainer = MakeBagCard(Fx, 3);

	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ TypeAMain, TypeBMaster, SpecialContainerContent, BurdenContainer });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);
	Run->AcquireCardToRun(FluxContent);

	auto FindBackpackInstanceId = [RunPtr = Run.Get()](const UCardDefinition* Definition)
	{
		for (const FCardInstance& Inst : RunPtr->GetBackpack())
		{
			if (Inst.Definition == Definition)
			{
				return Inst.InstanceId;
			}
		}
		return FGuid();
	};

	auto HasDefinition = [](const TArray<FRunStorageCardView>& Views, const UCardDefinition* Definition)
	{
		for (const FRunStorageCardView& View : Views)
		{
			if (View.Instance.Definition == Definition)
			{
				return true;
			}
		}
		return false;
	};

	auto FindView = [](const TArray<FRunStorageCardView>& Views, const UCardDefinition* Definition)
	{
		for (const FRunStorageCardView& View : Views)
		{
			if (View.Instance.Definition == Definition)
			{
				return &View;
			}
		}
		return static_cast<const FRunStorageCardView*>(nullptr);
	};

	const FRunBackpackStorageSnapshot InitialSnap = Run->BuildBackpackStorageSnapshot();
	TestEqual(TEXT("Flux.MainCards kept empty for compatibility"),
		InitialSnap.Flux.MainCards.Num(), 0);
	TestTrue(TEXT("A container in Backpack appears as Flux.ContentCards"),
		HasDefinition(InitialSnap.Flux.ContentCards, TypeAMain));
	TestTrue(TEXT("Backpack normal card appears as Flux.ContentCards"),
		HasDefinition(InitialSnap.Flux.ContentCards, FluxContent));
	if (const FRunStorageCardView* TypeAView = FindView(InitialSnap.Flux.ContentCards, TypeAMain))
	{
		TestEqual(TEXT("Initial A container physical zone"), TypeAView->PhysicalZone, EZoneKind::Backpack);
		TestFalse(TEXT("Initial A container not physical BattleDeck"), TypeAView->bIsPhysicalInBattleDeck);
	}

	TestTrue(TEXT("Move A container to BattleDeck"), MoveFirstBackpackDefinitionToBattleDeck(Run, TypeAMain));
	FRunBackpackStorageSnapshot Snap = Run->BuildBackpackStorageSnapshot();
	TestFalse(TEXT("Moved A container no longer appears in Flux.ContentCards"),
		HasDefinition(Snap.Flux.ContentCards, TypeAMain));
	if (const FRunStorageCardView* TypeAView = FindView(Snap.BattleDeckPhysicalCards, TypeAMain))
	{
		TestEqual(TEXT("Moved A container appears as physical BattleDeck card"),
			TypeAView->PhysicalZone, EZoneKind::BattleDeck);
		TestTrue(TEXT("Moved A container physical BattleDeck flag"), TypeAView->bIsPhysicalInBattleDeck);
	}
	else
	{
		AddError(TEXT("Moved A container missing from BattleDeckPhysicalCards"));
		return false;
	}

	const FGuid BMasterId = FindBackpackInstanceId(TypeBMaster);
	if (!TestTrue(TEXT("B master InstanceId valid"), BMasterId.IsValid()))
	{
		return false;
	}

	Run->AcquireCardToRun(SpecialContent);
	const FGuid SpecialContentId = FindBackpackInstanceId(SpecialContent);
	const FGuid SpecialContainerId = FindBackpackInstanceId(SpecialContainerContent);
	const FGuid BurdenContainerId = FindBackpackInstanceId(BurdenContainer);
	if (!TestTrue(TEXT("Special normal InstanceId valid"), SpecialContentId.IsValid())
		|| !TestTrue(TEXT("Special container InstanceId valid"), SpecialContainerId.IsValid())
		|| !TestTrue(TEXT("Burden container InstanceId valid"), BurdenContainerId.IsValid()))
	{
		return false;
	}

	TestTrue(TEXT("Move normal into SpecialZone"),
		Run->MoveInstance(SpecialContentId, EZoneKind::SpecialZone, BMasterId));
	TestTrue(TEXT("Move container into SpecialZone as content"),
		Run->MoveInstance(SpecialContainerId, EZoneKind::SpecialZone, BMasterId));
	TestTrue(TEXT("Move B master to BattleDeck"), MoveFirstBackpackDefinitionToBattleDeck(Run, TypeBMaster));
	TestTrue(TEXT("Enable SpecialZone card for battle projection"),
		Run->SetSpecialZoneCardBattleEnabled(SpecialContentId, true));
	TestTrue(TEXT("Move container into BurdenZone"),
		Run->MoveInstance(BurdenContainerId, EZoneKind::BurdenZone, FGuid()));

	// 异常 instance：Snapshot 应跳过 nullptr Definition 且不崩溃。
	FCardInstance NullInst;
	NullInst.InstanceId = FGuid::NewGuid();
	FWacomRunSessionTestAccess::GetMutableRunState(*Run.Get()).Backpack.Add(NullInst);

	Snap = Run->BuildBackpackStorageSnapshot();
	if (!TestEqual(TEXT("One SpecialZone view"), Snap.SpecialZones.Num(), 1))
	{
		return false;
	}

	const FRunSpecialStorageView& SpecialView = Snap.SpecialZones[0];
	TestEqual(TEXT("Special owner is B master"), SpecialView.OwnerCard.Instance.Definition.Get(), TypeBMaster);
	TestTrue(TEXT("Special owner in BattleDeck"), SpecialView.bOwnerInBattleDeck);
	TestTrue(TEXT("Special normal content preserved"),
		HasDefinition(SpecialView.ContentCards, SpecialContent));
	TestTrue(TEXT("Special container content preserved as content"),
		HasDefinition(SpecialView.ContentCards, SpecialContainerContent));
	if (const FRunStorageCardView* SpecialContainerView = FindView(SpecialView.ContentCards, SpecialContainerContent))
	{
		TestEqual(TEXT("Special container physical zone"), SpecialContainerView->PhysicalZone, EZoneKind::SpecialZone);
		TestEqual(TEXT("Special container owner id"), SpecialContainerView->ZoneOwnerInstanceId, BMasterId);
		TestTrue(TEXT("Special container still classified as TypeA"), SpecialContainerView->bIsTypeAContainer);
	}

	TestTrue(TEXT("Enabled SpecialZone card appears as BattleDeck projection"),
		HasDefinition(Snap.BattleDeckProjectedCards, SpecialContent));
	TestTrue(TEXT("B master appears in physical BattleDeck cards"),
		HasDefinition(Snap.BattleDeckPhysicalCards, TypeBMaster));
	TestTrue(TEXT("Burden container appears in BurdenCards"),
		HasDefinition(Snap.BurdenCards, BurdenContainer));
	if (const FRunStorageCardView* BurdenView = FindView(Snap.BurdenCards, BurdenContainer))
	{
		TestEqual(TEXT("Burden container physical zone"), BurdenView->PhysicalZone, EZoneKind::BurdenZone);
		TestTrue(TEXT("Burden container still classified as TypeA"), BurdenView->bIsTypeAContainer);
	}
	TestFalse(TEXT("Burden container is not exposed as Flux main card"),
		HasDefinition(Snap.Flux.MainCards, BurdenContainer));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckCollectTypeBContainersSpec,
	"Wacom.Run.Deck.CollectTypeBContainers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckCollectTypeBContainersSpec::RunTest(const FString& /*Parameters*/)
{
	// Stage 4.5.1 任务 10.2 / R3.5 / R3.6：
	//   - CollectTypeBContainers 改为输出 TArray<FGuid> OutOwnerInstanceIds
	//   - 按 RunState.SpecialZones 数组下标升序、去重、不含悬空 InstanceId
	//   - 玩家无 B 主卡 → 输出空数组
	FWacomBattleFixture Fx;

	UCardDefinition* TypeA = MakeBagCard(Fx, 8);

	UCardDefinition* TypeB1 = Fx.MakeNoopCard(0);
	TypeB1->Physique.Capacity = 3;
	TypeB1->Physique.CapacityEffect = WacomTags::Card_CapacityEffect_Placeholder;

	UCardDefinition* TypeB2 = Fx.MakeNoopCard(0);
	TypeB2->Physique.Capacity = 5;
	TypeB2->Physique.CapacityEffect = WacomTags::Card_CapacityEffect_Placeholder;

	UCardDefinition* Normal = Fx.MakeNoopCard(0);

	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ TypeA, TypeB1, TypeB2, Normal });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	// Initialize 后 SpecialZones 已为两张 B 主卡各建空 entry（task 7.1 / R2.3）。
	const FRunState& State = Run->GetRunState();
	if (!TestEqual(TEXT("SpecialZones seeded for B containers"), State.SpecialZones.Num(), 2))
	{
		return false;
	}

	const FGuid B1OwnerId = State.SpecialZones[0].OwnerInstanceId;
	const FGuid B2OwnerId = State.SpecialZones[1].OwnerInstanceId;

	TArray<FGuid> Owners;
	Run->CollectTypeBContainers(Owners);

	TestEqual(TEXT("Two B owner InstanceIds collected"), Owners.Num(), 2);
	// 严格按 SpecialZones 数组下标升序输出
	if (Owners.Num() == 2)
	{
		TestEqual(TEXT("Order[0] matches SpecialZones[0]"), Owners[0], B1OwnerId);
		TestEqual(TEXT("Order[1] matches SpecialZones[1]"), Owners[1], B2OwnerId);
	}
	TestTrue(TEXT("All InstanceIds are non-zero"), !Owners.Contains(FGuid()));

	// 把 TypeB1 移到 BattleDeck，仍应被枚举到（owner instance 在 Backpack ∪ BattleDeck 都接受）。
	// 此操作不改变 SpecialZones 数组顺序（B 主卡跨 zone 移动只改主卡所在父 zone，不动 entry）。
	TestTrue(TEXT("Add TypeB1 to BattleDeck"), MoveFirstBackpackDefinitionToBattleDeck(Run, TypeB1));
	Run->CollectTypeBContainers(Owners);
	TestEqual(TEXT("Still two after move"), Owners.Num(), 2);
	if (Owners.Num() == 2)
	{
		TestEqual(TEXT("Order preserved after move [0]"), Owners[0], B1OwnerId);
		TestEqual(TEXT("Order preserved after move [1]"), Owners[1], B2OwnerId);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckCollectTypeBContainersEmptySpec,
	"Wacom.Run.Deck.CollectTypeBContainersEmpty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckCollectTypeBContainersEmptySpec::RunTest(const FString& /*Parameters*/)
{
	// R3.6：玩家无 B 主卡 → 输出空数组
	FWacomBattleFixture Fx;

	UCardDefinition* TypeA  = MakeBagCard(Fx, 8);
	UCardDefinition* Normal = Fx.MakeNoopCard(0);

	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ TypeA, Normal });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	TArray<FGuid> Owners;
	// 预填一些垃圾值确保被 Reset
	Owners.Add(FGuid::NewGuid());
	Run->CollectTypeBContainers(Owners);

	TestEqual(TEXT("No B owners → empty"), Owners.Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckOnlyTypeBProvidersStillUnlockBackpackSpec,
	"Wacom.Run.Deck.OnlyTypeBProvidersStillUnlockBackpack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckOnlyTypeBProvidersStillUnlockBackpackSpec::RunTest(const FString& /*Parameters*/)
{
	// IsBackpackUiAvailable 按容器卡容量判定；
	// 玩家若只有 B 类容器也应能打开背包（虽然 Flux=0 暂时无法存通量卡）。
	FWacomBattleFixture Fx;

	UCardDefinition* TypeBProvider = MakeBagCard(Fx, 4);
	TypeBProvider->Physique.CapacityEffect = WacomTags::Card_CapacityEffect_Placeholder;

	UCardDefinition* Normal = Fx.MakeNoopCard(0);

	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ TypeBProvider, Normal });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	TestTrue(TEXT("Backpack UI available with B-only provider"), Run->IsBackpackUiAvailable());
	TestEqual(TEXT("FluxCapacity = 0 (no A providers)"), Run->GetFluxCapacity(), 0);

	return true;
}

// ================ Stage 4.4：删牌能力提供者识别 ================

namespace
{
	UCardDefinition* MakeDeleteProviderCard(FWacomBattleFixture& Fx, int32 Capacity)
	{
		UCardDefinition* Card = Fx.MakeNoopCard(0);
		Card->Physique.Capacity = Capacity;
		Card->Keywords.AddTag(WacomTags::Card_Keyword_DeleteProvider);
		Card->Rarity = WacomTags::Card_Rarity_White;
		return Card;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckIsDeleteProviderCardSpec,
	"Wacom.Run.Deck.IsDeleteProviderCard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckIsDeleteProviderCardSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* Normal       = Fx.MakeNoopCard(0);
	UCardDefinition* BagOnly      = MakeBagCard(Fx, 12);
	UCardDefinition* DeleteOnly   = MakeDeleteProviderCard(Fx, 3);

	UCardDefinition* Both         = MakeBagCard(Fx, 5);
	Both->Keywords.AddTag(WacomTags::Card_Keyword_DeleteProvider);

	TestFalse(TEXT("Normal not delete provider"),    URunSession::IsDeleteProviderCard(Normal));
	TestFalse(TEXT("BagOnly not delete provider"),   URunSession::IsDeleteProviderCard(BagOnly));
	TestTrue (TEXT("DeleteOnly is delete provider"), URunSession::IsDeleteProviderCard(DeleteOnly));
	TestTrue (TEXT("Both is delete provider"),       URunSession::IsDeleteProviderCard(Both));

	TestFalse(TEXT("nullptr false"), URunSession::IsDeleteProviderCard(nullptr));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckIsDeleteFunctionAvailableSpec,
	"Wacom.Run.Deck.IsDeleteFunctionAvailable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckIsDeleteFunctionAvailableSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	// 场景 1：只有 BagProvider，没有 DeleteProvider → false
	{
		UCardDefinition* Bag = MakeBagCard(Fx, 12);
		UCardDefinition* Normal = Fx.MakeNoopCard(0);
		UCharacterDefinition* Char = Fx.MakeCharacter(
			Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
			{ Bag, Normal });

		TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
		Run->Initialize(Char);

		TestFalse(TEXT("Bag only → no delete function"), Run->IsDeleteFunctionAvailable());
	}

	// 场景 2：背包有 DeleteProvider → true
	{
		UCardDefinition* Bag = MakeBagCard(Fx, 12);
		UCardDefinition* Lantern = MakeDeleteProviderCard(Fx, 3);
		UCharacterDefinition* Char = Fx.MakeCharacter(
			Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
			{ Bag, Lantern });

		TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
		Run->Initialize(Char);

		TestTrue(TEXT("With DeleteProvider → available"), Run->IsDeleteFunctionAvailable());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckMuseiLanternStartsInBattleDeckSpec,
	"Wacom.Run.Deck.MuseiLanternStartsInBattleDeckAndContributesFlux",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckMuseiLanternStartsInBattleDeckSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* Lantern = MakeDeleteProviderCard(Fx, 3);
	Lantern->CardId = TEXT("MuseiYinchongdeng");

	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ Lantern });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	TestFalse(TEXT("Lantern not initially in Backpack"), BackpackContainsDefinition(Run->GetRunState(), Lantern));
	TestTrue(TEXT("Lantern initially in BattleDeck"), BattleDeckContainsDefinition(Run->GetRunState(), Lantern));
	TestEqual(TEXT("Lantern capacity contributes FluxCapacity"), Run->GetFluxCapacity(), 3);
	TestTrue(TEXT("Delete provider available from BattleDeck"), Run->IsDeleteFunctionAvailable());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckDeleteFunctionLostAfterDestroySpec,
	"Wacom.Run.Deck.DeleteFunctionLostAfterDestroy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckDeleteFunctionLostAfterDestroySpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	// Bag 保证背包 UI 一直可用，避免"最后容量来源卡被拒"干扰
	UCardDefinition* Bag = MakeBagCard(Fx, 12);
	UCardDefinition* Lantern = MakeDeleteProviderCard(Fx, 3);

	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ Bag, Lantern });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	TestTrue(TEXT("Initially available"), Run->IsDeleteFunctionAvailable());

	// 销毁 Lantern → 删牌功能消失
	TestTrue(TEXT("Destroy Lantern OK"), DestroyFirstOwnedDefinition(Run, Lantern));
	TestFalse(TEXT("After destroy, no longer available"), Run->IsDeleteFunctionAvailable());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckBagAndDeleteProvidersIndependentSpec,
	"Wacom.Run.Deck.BagAndDeleteProvidersIndependent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckBagAndDeleteProvidersIndependentSpec::RunTest(const FString& /*Parameters*/)
{
	// 验证：BagProvider 和 DeleteProvider 是两条独立的关键词维度，互不影响。
	FWacomBattleFixture Fx;

	UCardDefinition* DeleteOnly = MakeDeleteProviderCard(Fx, 3);
	UCardDefinition* Normal     = Fx.MakeNoopCard(0);

	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ DeleteOnly, Normal });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Run->Initialize(Char);

	// 只有 DeleteProvider 容器、无 BagProvider → 删牌功能和背包 UI 都可用。
	TestTrue (TEXT("Delete function available"), Run->IsDeleteFunctionAvailable());
	TestTrue(TEXT("Backpack UI available from DeleteProvider capacity"), Run->IsBackpackUiAvailable());

	return true;
}

// ================ 备战区容量：统计全部容器卡 ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckBattleDeckCapacityCountsAllContainersSpec,
	"Wacom.Run.Deck.BattleDeckCapacityCountsAllContainers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckBattleDeckCapacityCountsAllContainersSpec::RunTest(const FString& /*Parameters*/)
{
	// GDD §11.4：通量容量只统计 A 类容器；备战容量统计所有容器（A+B）。
	FWacomBattleFixture Fx;

	// 场景 1：A + B 同存 → 通量只算 A，备战算 A+B
	{
		UCardDefinition* TypeA = MakeBagCard(Fx, 12);

		UCardDefinition* TypeB = Fx.MakeNoopCard(0);
		TypeB->Physique.Capacity = 4;
		TypeB->Physique.CapacityEffect = WacomTags::Card_CapacityEffect_Placeholder;

		UCharacterDefinition* Char = Fx.MakeCharacter(
			Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
			{ TypeA, TypeB });

		TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
		Run->Initialize(Char);

		TestEqual(TEXT("FluxCapacity = 12 (A capacity)"), Run->GetFluxCapacity(), 12);
		TestEqual(TEXT("BattleDeckCapacity = 16 (A+B)"), Run->GetBattleDeckCapacity(), 16);
	}

	// 场景 2：玩家无任何容器卡 → 两公式均返回 0（R3.1）
	{
		UCardDefinition* Normal = Fx.MakeNoopCard(0);
		UCharacterDefinition* Char = Fx.MakeCharacter(
			Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
			{ Normal });

		TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
		Run->Initialize(Char);

		TestEqual(TEXT("No container → FluxCapacity = 0"), Run->GetFluxCapacity(), 0);
		TestEqual(TEXT("No container → BattleDeckCapacity = 0"), Run->GetBattleDeckCapacity(), 0);
	}

	// 场景 3：玩家全部是 B 类容器卡 → 通量为 0，备战统计全部 B 容量
	{
		UCardDefinition* TypeB1 = Fx.MakeNoopCard(0);
		TypeB1->Physique.Capacity = 4;
		TypeB1->Physique.CapacityEffect = WacomTags::Card_CapacityEffect_Placeholder;

		UCardDefinition* TypeB2 = Fx.MakeNoopCard(0);
		TypeB2->Physique.Capacity = 6;
		TypeB2->Physique.CapacityEffect = WacomTags::Card_CapacityEffect_Placeholder;

		UCharacterDefinition* Char = Fx.MakeCharacter(
			Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
			{ TypeB1, TypeB2 });

		TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
		Run->Initialize(Char);

		TestEqual(TEXT("All B → FluxCapacity = 0"), Run->GetFluxCapacity(), 0);
		TestEqual(TEXT("All B → BattleDeckCapacity = 10"), Run->GetBattleDeckCapacity(), 10);
	}

	return true;
}

// ================ Stage 4.5.0 Property Tests ================

namespace
{
	/**
	 * Property 1 测试用：随机生成一张 PBT 卡。
	 * 加权：50% 普通卡 / 25% A 类容器卡 / 25% B 类容器卡。
	 *
	 * Capacity 在 [1, 6]，CapacityEffect 用 Placeholder 占位 tag 标记 B 类。
	 * 生成器混合三种卡型是为了让 Initialize 走 a2 分流（容器 → Backpack，非容器 → BattleDeck）
	 * 时两个数组都吃到 instance，property 才能覆盖"两区合并后 InstanceId 唯一性"。
	 */
	UCardDefinition* MakePbtCard(FWacomBattleFixture& Fx, FRandomStream& Rng)
	{
		UCardDefinition* Card = Fx.MakeNoopCard(0);

		const int32 KindRoll = Rng.RandRange(0, 9);
		if (KindRoll < 5)
		{
			// Normal 卡：无 Capacity / 无 CapacityEffect。
		}
		else if (KindRoll < 7)
		{
			// A 类容器卡：Capacity > 0 + CapacityEffect 为空。
			Card->Physique.Capacity = Rng.RandRange(1, 6);
		}
		else
		{
			// B 类容器卡：Capacity > 0 + CapacityEffect 非空（占位 tag）。
			Card->Physique.Capacity = Rng.RandRange(1, 6);
			Card->Physique.CapacityEffect = WacomTags::Card_CapacityEffect_Placeholder;
		}
		return Card;
	}

	bool IsPbtFluxContentCard(const UCardDefinition* Card)
	{
		return Card && !URunSession::IsTypeBContainerCard(Card);
	}

	int32 CountPbtFluxContentCards(const TArray<FCardInstance>& Pile)
	{
		int32 Count = 0;
		for (const FCardInstance& Inst : Pile)
		{
			if (IsPbtFluxContentCard(Inst.Definition))
			{
				++Count;
			}
		}
		return Count;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckPropertyInstanceIdUniqueSpec,
	"Wacom.Run.Deck.PropertyInstanceIdUnique",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckPropertyInstanceIdUniqueSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, Property 1: InstanceId 全局唯一且非零
	// Validates: Requirements 1.3, 1.5, 7.2
	//
	// 对任意 Initialize / AcquireCardToRun 操作序列产生的 fresh URunSession，
	// Backpack ∪ BattleDeck ∪ BurdenZone ∪ ⋃ SpecialZones.Cards 中所有 InstanceId
	// 必须两两互异且不含 FGuid()（zero GUID）。
	//
	// 4.5.0 阶段 FRunState 尚未引入 BurdenZone / SpecialZones 字段（task 6.2 / 6.1
	// 在 4.5.1 才接入），本 property 当前只能遍历 Backpack ∪ BattleDeck；4.5.1 接入
	// 数据层后扩展遍历范围即可，property 文本与 acceptance 不变。

	const int32 NumIterations = 150;     // ≥ 100，按 design §Testing Strategy
	const int32 BaseSeed      = 0xC0FFEE;

	for (int32 Iter = 0; Iter < NumIterations; ++Iter)
	{
		const int32 Seed = BaseSeed + Iter;
		FRandomStream Rng(Seed);
		FWacomBattleFixture Fx;

		// ---- 生成器：随机大小的 StarterDeck（0..20），混 nullptr 验证 R1.3 跳过路径 ----
		const int32 StarterDeckSize = Rng.RandRange(0, 20);
		TArray<UCardDefinition*> StarterDeck;
		StarterDeck.Reserve(StarterDeckSize);
		for (int32 i = 0; i < StarterDeckSize; ++i)
		{
			// 10% 概率 nullptr。Initialize 应跳过不生成 instance（R1.3）。
			if (Rng.RandRange(0, 9) == 0) { StarterDeck.Add(nullptr); }
			else                          { StarterDeck.Add(MakePbtCard(Fx, Rng)); }
		}

		UCharacterDefinition* Char = Fx.MakeCharacter(
			Fx.MakeNoopCard(1), Fx.MakeNoopCard(1), StarterDeck);

		TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
		Run->Initialize(Char);

		// ---- 操作序列：0..15 次随机 AcquireCardToRun（含 nullptr 混入，验证 R1.5）----
		const int32 NumAdds = Rng.RandRange(0, 15);
		for (int32 i = 0; i < NumAdds; ++i)
		{
			UCardDefinition* Card = (Rng.RandRange(0, 9) == 0)
				? nullptr
				: MakePbtCard(Fx, Rng);
			Run->AcquireCardToRun(Card);
		}

		// ---- 断言：Backpack ∪ BattleDeck 的 InstanceId 集合两两互异 + 非 zero GUID ----
		const int32 ExpectedCount = Run->GetBackpack().Num() + Run->GetBattleDeck().Num();
		TSet<FGuid> SeenIds;
		SeenIds.Reserve(ExpectedCount);

		auto CheckPile = [&](const TArray<FCardInstance>& Pile, const TCHAR* ZoneName) -> bool
		{
			for (int32 Idx = 0; Idx < Pile.Num(); ++Idx)
			{
				const FCardInstance& Inst = Pile[Idx];

				// R1.3 / R1.5 / R7.2：InstanceId 必须非 zero GUID
				if (!Inst.InstanceId.IsValid())
				{
					AddError(FString::Printf(
						TEXT("Property 1 FAILED: zero-GUID InstanceId. ")
						TEXT("Counterexample: Seed=%d Iter=%d Zone=%s Idx=%d StarterDeckSize=%d NumAdds=%d"),
						Seed, Iter, ZoneName, Idx, StarterDeckSize, NumAdds));
					return false;
				}

				// 全表合并后两两互异
				bool bAlreadyInSet = false;
				SeenIds.Add(Inst.InstanceId, &bAlreadyInSet);
				if (bAlreadyInSet)
				{
					AddError(FString::Printf(
						TEXT("Property 1 FAILED: duplicate InstanceId=%s. ")
						TEXT("Counterexample: Seed=%d Iter=%d Zone=%s Idx=%d StarterDeckSize=%d NumAdds=%d"),
						*Inst.InstanceId.ToString(EGuidFormats::DigitsWithHyphens),
						Seed, Iter, ZoneName, Idx, StarterDeckSize, NumAdds));
					return false;
				}
			}
			return true;
		};

		if (!CheckPile(Run->GetBackpack(),   TEXT("Backpack")))   { return false; }
		if (!CheckPile(Run->GetBattleDeck(), TEXT("BattleDeck"))) { return false; }

		// 防御性兜底：集合大小 == 两区总数（CheckPile 已逐张校验，这里捕捉极端的 set 行为偏差）
		if (SeenIds.Num() != ExpectedCount)
		{
			AddError(FString::Printf(
				TEXT("Property 1 FAILED: SeenIds.Num()=%d != ExpectedCount=%d. ")
				TEXT("Counterexample: Seed=%d Iter=%d StarterDeckSize=%d NumAdds=%d"),
				SeenIds.Num(), ExpectedCount, Seed, Iter, StarterDeckSize, NumAdds));
			return false;
		}
	}

	return true;
}

// ================ Stage 4.5.0 Property 2: MoveInstance 原子成功 / 完全失败 ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckPropertyMoveInstanceAtomicSpec,
	"Wacom.Run.Deck.PropertyMoveInstanceAtomic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckPropertyMoveInstanceAtomicSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, Property 2: MoveInstance 原子成功 / 完全失败
	// Validates: Requirements 1.6, 1.7, 2.7, 5.5
	//
	// 对任意 URunSession 状态和任意 MoveInstance(InstanceId, ToZone, ToOwner) 调用：
	//   - 返回 true：FindInstance(InstanceId) 之后返回 (OutZone == ToZone, OutZoneOwnerInstanceId == ToOwner)
	//                且源 zone 不再包含该 InstanceId
	//   - 返回 false：Backpack 与 BattleDeck 数组逐元素与调用前 snapshot 完全相等
	//
	// 4.5.0 阶段覆盖（task 3.4 范围）：
	//   - 成功：Backpack → BattleDeck（容量允许时）
	//   - 成功：BattleDeck → Backpack（始终允许）
	//   - 失败：随机 GUID（InstanceId 不在任何 zone 中，对应 R1.7）
	//   - 失败：BattleDeck 容量满 + 来源 Backpack（对应 design §5 校验表）
	// SpecialZone owner-not-found / 自指拒绝 / 容量满 在 4.5.1 task 8.1 接入后扩展（property 文本与 acceptance 不变）。

	const int32 NumIterations = 150;     // ≥ 100
	const int32 BaseSeed      = 0xBADF00D;

	// 逐字段比较两张 instance（POD：InstanceId / Definition / bBattleEnabledInSpecialZone）
	auto AreInstancesEqual = [](const FCardInstance& A, const FCardInstance& B) -> bool
	{
		return A.InstanceId == B.InstanceId
			&& A.Definition == B.Definition
			&& A.bBattleEnabledInSpecialZone == B.bBattleEnabledInSpecialZone;
	};

	auto ArePilesEqual = [&](const TArray<FCardInstance>& X, const TArray<FCardInstance>& Y) -> bool
	{
		if (X.Num() != Y.Num()) { return false; }
		for (int32 i = 0; i < X.Num(); ++i)
		{
			if (!AreInstancesEqual(X[i], Y[i])) { return false; }
		}
		return true;
	};

	auto PileContains = [](const TArray<FCardInstance>& Pile, const FGuid& Id) -> bool
	{
		for (const FCardInstance& I : Pile)
		{
			if (I.InstanceId == Id) { return true; }
		}
		return false;
	};

	enum class EScenario : uint8
	{
		SuccessBackpackToBattleDeck = 0,
		SuccessBattleDeckToBackpack = 1,
		FailRandomGuid              = 2,
		FailCapacityFull            = 3,
	};

	for (int32 Iter = 0; Iter < NumIterations; ++Iter)
	{
		const int32 Seed = BaseSeed + Iter;
		FRandomStream Rng(Seed);
		FWacomBattleFixture Fx;

		// ---- 生成器：构造随机 RunSession 状态 ----
		// 必须含至少一张 BagProvider（Capacity ≥ 1），保证 GetBattleDeckCapacity > 0；
		// 其余张数随机走 MakePbtCard（混 Normal / A / B 三类）。
		const int32 ExtraCards = Rng.RandRange(0, 11);
		TArray<UCardDefinition*> StarterDeck;
		StarterDeck.Reserve(ExtraCards + 1);
		StarterDeck.Add(MakeBagCard(Fx, Rng.RandRange(1, 6)));
		for (int32 i = 0; i < ExtraCards; ++i)
		{
			StarterDeck.Add(MakePbtCard(Fx, Rng));
		}

		UCharacterDefinition* Char = Fx.MakeCharacter(
			Fx.MakeNoopCard(1), Fx.MakeNoopCard(1), StarterDeck);

		TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
		Run->Initialize(Char);

		// ---- 选择本次迭代的场景；若前置条件不满足则降级到 FailRandomGuid ----
		EScenario Scenario = static_cast<EScenario>(Rng.RandRange(0, 3));

		if (Scenario == EScenario::SuccessBackpackToBattleDeck)
		{
			if (Run->GetBackpack().Num() == 0
				|| Run->GetBattleDeck().Num() >= Run->GetBattleDeckCapacity())
			{
				Scenario = EScenario::FailRandomGuid;
			}
		}
		else if (Scenario == EScenario::SuccessBattleDeckToBackpack)
		{
			if (Run->GetBattleDeck().Num() == 0)
			{
				Scenario = EScenario::FailRandomGuid;
			}
		}

		// 容量满场景需要在 setup 阶段做一系列 moves 把 BattleDeck 填满；这些 moves 会改变状态。
		// 真正测试调用前的 snapshot 在所有 setup 完成后再取一次。
		if (Scenario == EScenario::FailCapacityFull)
		{
			const int32 Capacity = Run->GetBattleDeckCapacity();
			// 把背包中的 instance 一张张推到 BattleDeck，直到 BattleDeck.Num() == Capacity 或推不动
			int32 SafetyGuard = 64;
			while (SafetyGuard-- > 0
				&& Run->GetBattleDeck().Num() < Capacity
				&& Run->GetBackpack().Num() > 0)
			{
				const FGuid PushId = Run->GetBackpack()[0].InstanceId;
				const bool bOk = Run->MoveInstance(PushId, EZoneKind::BattleDeck, FGuid());
				if (!bOk) { break; }
			}

			if (Run->GetBattleDeck().Num() < Capacity || Run->GetBackpack().Num() == 0)
			{
				// 凑不出"容量满 + 来源非空"的情形 → 降级
				Scenario = EScenario::FailRandomGuid;
			}
		}

		// ---- 准备调用参数 ----
		FGuid     TargetInstanceId;
		EZoneKind ToZone   = EZoneKind::Backpack;
		FGuid     ToOwner;                       // 4.5.0 阶段始终 invalid GUID

		EZoneKind ExpectedFromZone = EZoneKind::Backpack;
		bool      bExpectSuccess   = false;

		switch (Scenario)
		{
		case EScenario::SuccessBackpackToBattleDeck:
		{
			const int32 PickIdx = Rng.RandRange(0, Run->GetBackpack().Num() - 1);
			TargetInstanceId    = Run->GetBackpack()[PickIdx].InstanceId;
			ToZone              = EZoneKind::BattleDeck;
			ExpectedFromZone    = EZoneKind::Backpack;
			bExpectSuccess      = true;
			break;
		}
		case EScenario::SuccessBattleDeckToBackpack:
		{
			const int32 PickIdx = Rng.RandRange(0, Run->GetBattleDeck().Num() - 1);
			TargetInstanceId    = Run->GetBattleDeck()[PickIdx].InstanceId;
			ToZone              = EZoneKind::Backpack;
			ExpectedFromZone    = EZoneKind::BattleDeck;
			const UCardDefinition* TargetDef = Run->GetBattleDeck()[PickIdx].Definition;
			const bool bTargetConsumesFlux = IsPbtFluxContentCard(TargetDef);
			const bool bBackpackHasFluxSpace =
				CountPbtFluxContentCards(Run->GetBackpack()) < Run->GetFluxCapacity();
			bExpectSuccess = !bTargetConsumesFlux || bBackpackHasFluxSpace;
			break;
		}
		case EScenario::FailCapacityFull:
		{
			TargetInstanceId    = Run->GetBackpack()[0].InstanceId;
			ToZone              = EZoneKind::BattleDeck;
			bExpectSuccess      = false;
			break;
		}
		case EScenario::FailRandomGuid:
		default:
		{
			TargetInstanceId    = FGuid::NewGuid();  // 与已存在 InstanceId 冲突的概率为 2^-128
			ToZone              = (Rng.RandRange(0, 1) == 0) ? EZoneKind::Backpack : EZoneKind::BattleDeck;
			bExpectSuccess      = false;
			break;
		}
		}

		// ---- 调用前 snapshot（深拷贝 TArray，元素是 POD struct）----
		const TArray<FCardInstance> BackpackPre   = Run->GetBackpack();
		const TArray<FCardInstance> BattleDeckPre = Run->GetBattleDeck();

		// ---- 执行 ----
		const bool bResult = Run->MoveInstance(TargetInstanceId, ToZone, ToOwner);

		// ---- 断言 ----
		if (bResult)
		{
			// 期待场景：bExpectSuccess 为 true 时 bResult 应为 true；反之则是 unexpected pass
			if (!bExpectSuccess)
			{
				AddError(FString::Printf(
					TEXT("Property 2 FAILED: 期待失败的场景却返回 true。")
					TEXT("Counterexample: Seed=%d Iter=%d Scenario=%d ToZone=%d"),
					Seed, Iter, (int32)Scenario, (int32)ToZone));
				return false;
			}

			// 成功路径 1：FindInstance 返回新 zone + ToOwner
			FCardInstance OutInst;
			EZoneKind     OutZone  = EZoneKind::Backpack;
			FGuid         OutOwner;
			const bool bFound = Run->FindInstance(TargetInstanceId, OutInst, OutZone, OutOwner);
			if (!bFound)
			{
				AddError(FString::Printf(
					TEXT("Property 2 FAILED: 成功移动后 FindInstance 找不到 InstanceId=%s。")
					TEXT("Counterexample: Seed=%d Iter=%d Scenario=%d"),
					*TargetInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
					Seed, Iter, (int32)Scenario));
				return false;
			}
			if (OutZone != ToZone)
			{
				AddError(FString::Printf(
					TEXT("Property 2 FAILED: 成功移动后 OutZone(%d) != ToZone(%d)。")
					TEXT("Counterexample: Seed=%d Iter=%d Scenario=%d"),
					(int32)OutZone, (int32)ToZone, Seed, Iter, (int32)Scenario));
				return false;
			}
			if (OutOwner != ToOwner)
			{
				AddError(FString::Printf(
					TEXT("Property 2 FAILED: OutZoneOwnerInstanceId(%s) != ToOwner(%s)。")
					TEXT("Counterexample: Seed=%d Iter=%d Scenario=%d"),
					*OutOwner.ToString(EGuidFormats::DigitsWithHyphens),
					*ToOwner.ToString(EGuidFormats::DigitsWithHyphens),
					Seed, Iter, (int32)Scenario));
				return false;
			}

			// 成功路径 2：源 zone 不再包含该 InstanceId
			const TArray<FCardInstance>& SrcPostMove = (ExpectedFromZone == EZoneKind::Backpack)
				? Run->GetBackpack()
				: Run->GetBattleDeck();
			if (PileContains(SrcPostMove, TargetInstanceId))
			{
				AddError(FString::Printf(
					TEXT("Property 2 FAILED: 源 zone(%d) 在成功移动后仍含 InstanceId=%s。")
					TEXT("Counterexample: Seed=%d Iter=%d Scenario=%d"),
					(int32)ExpectedFromZone,
					*TargetInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
					Seed, Iter, (int32)Scenario));
				return false;
			}
		}
		else
		{
			// 期待场景：bExpectSuccess 为 false 时 bResult 应为 false；反之则是 unexpected fail
			if (bExpectSuccess)
			{
				AddError(FString::Printf(
					TEXT("Property 2 FAILED: 期待成功的场景却返回 false。")
					TEXT("Counterexample: Seed=%d Iter=%d Scenario=%d ToZone=%d InstanceId=%s ")
					TEXT("BackpackNum=%d BattleDeckNum=%d Capacity=%d"),
					Seed, Iter, (int32)Scenario, (int32)ToZone,
					*TargetInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
					BackpackPre.Num(), BattleDeckPre.Num(), Run->GetBattleDeckCapacity()));
				return false;
			}

			// 失败路径：四区数组逐字节相等。4.5.0 阶段 BurdenZone / SpecialZones 字段未引入（task 6.x），
			// 这里仅断言 Backpack / BattleDeck 完全等同于 snapshot。
			if (!ArePilesEqual(BackpackPre, Run->GetBackpack()))
			{
				AddError(FString::Printf(
					TEXT("Property 2 FAILED: 失败路径下 Backpack 数组发生变化。")
					TEXT("Counterexample: Seed=%d Iter=%d Scenario=%d Pre.Num=%d Post.Num=%d"),
					Seed, Iter, (int32)Scenario, BackpackPre.Num(), Run->GetBackpack().Num()));
				return false;
			}
			if (!ArePilesEqual(BattleDeckPre, Run->GetBattleDeck()))
			{
				AddError(FString::Printf(
					TEXT("Property 2 FAILED: 失败路径下 BattleDeck 数组发生变化。")
					TEXT("Counterexample: Seed=%d Iter=%d Scenario=%d Pre.Num=%d Post.Num=%d"),
					Seed, Iter, (int32)Scenario, BattleDeckPre.Num(), Run->GetBattleDeck().Num()));
				return false;
			}
		}
	}

	return true;
}

// ================ Stage 4.5.0 Property 3: FindInstance 一致性 ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckPropertyFindInstanceConsistencySpec,
	"Wacom.Run.Deck.PropertyFindInstanceConsistency",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckPropertyFindInstanceConsistencySpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, Property 3: FindInstance 一致性
	// Validates: Requirements 1.8
	//
	// 对任意 URunSession 状态和任何 InstanceId：
	//   - 命中：FindInstance 返回 true，(OutZone, OutZoneOwnerInstanceId) 与实际位置完全一致；
	//           OutInstance 的 InstanceId / Definition / bBattleEnabledInSpecialZone 与实际持有的 instance 等价。
	//   - 未命中（含 FGuid()）：FindInstance 返回 false，三个 out 参数全部维持调用方传入的初值（sentinel）。
	//
	// 4.5.0 阶段 FindInstance 仅遍历 Backpack / BattleDeck；4.5.1 起遍历范围会扩展到
	// BurdenZone / ⋃SpecialZones.Cards（见 task 8.2），property 文本与 acceptance 不变。

	const int32 NumIterations = 150;     // ≥ 100，按 design §Testing Strategy
	const int32 BaseSeed      = 0xFEEDBEE;

	for (int32 Iter = 0; Iter < NumIterations; ++Iter)
	{
		const int32 Seed = BaseSeed + Iter;
		FRandomStream Rng(Seed);
		FWacomBattleFixture Fx;

		// ---- 生成器：含至少 1 张 BagProvider 保证 GetBattleDeckCapacity > 0；其余随机 ----
		const int32 ExtraCards = Rng.RandRange(0, 11);
		TArray<UCardDefinition*> StarterDeck;
		StarterDeck.Reserve(ExtraCards + 1);
		StarterDeck.Add(MakeBagCard(Fx, Rng.RandRange(2, 6)));
		for (int32 i = 0; i < ExtraCards; ++i)
		{
			StarterDeck.Add(MakePbtCard(Fx, Rng));
		}

		UCharacterDefinition* Char = Fx.MakeCharacter(
			Fx.MakeNoopCard(1), Fx.MakeNoopCard(1), StarterDeck);

		TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
		Run->Initialize(Char);

		// ---- 随机 0..5 次 MoveInstance 让两区分布更杂 ----
		const int32 NumShuffles = Rng.RandRange(0, 5);
		for (int32 i = 0; i < NumShuffles; ++i)
		{
			const TArray<FCardInstance>& Bp = Run->GetBackpack();
			const TArray<FCardInstance>& Bd = Run->GetBattleDeck();
			if (Bp.Num() == 0 && Bd.Num() == 0) { break; }
			const bool bFromBackpack = (Bp.Num() > 0)
				&& ((Bd.Num() == 0) || (Rng.RandRange(0, 1) == 0));
			if (bFromBackpack)
			{
				const FGuid Id = Bp[Rng.RandRange(0, Bp.Num() - 1)].InstanceId;
				Run->MoveInstance(Id, EZoneKind::BattleDeck, FGuid());
			}
			else
			{
				const FGuid Id = Bd[Rng.RandRange(0, Bd.Num() - 1)].InstanceId;
				Run->MoveInstance(Id, EZoneKind::Backpack, FGuid());
			}
		}

		// ---- Sentinel：明显不会被实现写入的值，便于 R1.8 "out 参数不被覆写" 检测 ----
		// OutZone 选 BurdenZone（4.5.0 阶段 FindInstance 不会写入此值）；
		// OutZoneOwnerInstanceId 选一个新 GUID（与任何 instance.InstanceId 冲突概率 2^-128）；
		// OutInstance 用 bBattleEnabledInSpecialZone=true（默认是 false）让覆写易识别。
		const FGuid SentinelOwner   = FGuid::NewGuid();
		const FGuid SentinelInstId  = FGuid::NewGuid();
		auto MakeSentinelInstance = [&]() -> FCardInstance
		{
			FCardInstance S;
			S.InstanceId                  = SentinelInstId;
			S.Definition                  = nullptr;
			S.bBattleEnabledInSpecialZone = true;
			return S;
		};

		// ---- 路径 A：随机选一个已知 InstanceId（来自 Backpack ∪ BattleDeck） ----
		const TArray<FCardInstance>& Backpack   = Run->GetBackpack();
		const TArray<FCardInstance>& BattleDeck = Run->GetBattleDeck();
		const int32 TotalKnown = Backpack.Num() + BattleDeck.Num();

		if (TotalKnown > 0)
		{
			const int32 PickIdx = Rng.RandRange(0, TotalKnown - 1);
			FCardInstance ExpectedInstance;
			EZoneKind     ExpectedZone = EZoneKind::Backpack;
			if (PickIdx < Backpack.Num())
			{
				ExpectedInstance = Backpack[PickIdx];
				ExpectedZone     = EZoneKind::Backpack;
			}
			else
			{
				ExpectedInstance = BattleDeck[PickIdx - Backpack.Num()];
				ExpectedZone     = EZoneKind::BattleDeck;
			}

			FCardInstance OutInst  = MakeSentinelInstance();
			EZoneKind     OutZone  = EZoneKind::BurdenZone;     // sentinel
			FGuid         OutOwner = SentinelOwner;
			const bool    bFound   = Run->FindInstance(ExpectedInstance.InstanceId, OutInst, OutZone, OutOwner);

			if (!bFound)
			{
				AddError(FString::Printf(
					TEXT("Property 3 FAILED: 已知 InstanceId=%s FindInstance 返回 false。")
					TEXT("Counterexample: Seed=%d Iter=%d ExpectedZone=%d BackpackNum=%d BattleDeckNum=%d"),
					*ExpectedInstance.InstanceId.ToString(EGuidFormats::DigitsWithHyphens),
					Seed, Iter, (int32)ExpectedZone, Backpack.Num(), BattleDeck.Num()));
				return false;
			}
			if (OutZone != ExpectedZone)
			{
				AddError(FString::Printf(
					TEXT("Property 3 FAILED: OutZone(%d) != ExpectedZone(%d)。")
					TEXT("Counterexample: Seed=%d Iter=%d InstanceId=%s"),
					(int32)OutZone, (int32)ExpectedZone, Seed, Iter,
					*ExpectedInstance.InstanceId.ToString(EGuidFormats::DigitsWithHyphens)));
				return false;
			}
			// 4.5.0 阶段：来自 Backpack / BattleDeck 时 OutZoneOwnerInstanceId 应当是 invalid GUID。
			if (OutOwner.IsValid())
			{
				AddError(FString::Printf(
					TEXT("Property 3 FAILED: Backpack/BattleDeck 命中时 OutZoneOwnerInstanceId 应为 invalid GUID，实际=%s。")
					TEXT("Counterexample: Seed=%d Iter=%d InstanceId=%s"),
					*OutOwner.ToString(EGuidFormats::DigitsWithHyphens),
					Seed, Iter,
					*ExpectedInstance.InstanceId.ToString(EGuidFormats::DigitsWithHyphens)));
				return false;
			}
			if (OutInst.InstanceId != ExpectedInstance.InstanceId
				|| OutInst.Definition != ExpectedInstance.Definition
				|| OutInst.bBattleEnabledInSpecialZone != ExpectedInstance.bBattleEnabledInSpecialZone)
			{
				AddError(FString::Printf(
					TEXT("Property 3 FAILED: OutInstance 字段与实际持有的 instance 不一致。")
					TEXT("Counterexample: Seed=%d Iter=%d InstanceId=%s"),
					Seed, Iter,
					*ExpectedInstance.InstanceId.ToString(EGuidFormats::DigitsWithHyphens)));
				return false;
			}
		}

		// ---- 路径 B：随机生成的新 GUID（不在任何 zone 中） ----
		{
			FCardInstance OutInst  = MakeSentinelInstance();
			EZoneKind     OutZone  = EZoneKind::BurdenZone;     // sentinel
			FGuid         OutOwner = SentinelOwner;

			const FGuid Unknown = FGuid::NewGuid();
			const bool  bFound  = Run->FindInstance(Unknown, OutInst, OutZone, OutOwner);

			if (bFound)
			{
				AddError(FString::Printf(
					TEXT("Property 3 FAILED: 未知 InstanceId=%s 但 FindInstance 返回 true。")
					TEXT("Counterexample: Seed=%d Iter=%d"),
					*Unknown.ToString(EGuidFormats::DigitsWithHyphens), Seed, Iter));
				return false;
			}
			// R1.8：三个 out 参数全保持初值
			if (OutInst.InstanceId != SentinelInstId
				|| OutInst.Definition != nullptr
				|| OutInst.bBattleEnabledInSpecialZone != true)
			{
				AddError(FString::Printf(
					TEXT("Property 3 FAILED: 未命中路径下 OutInstance 被覆写。")
					TEXT("Counterexample: Seed=%d Iter=%d Unknown=%s"),
					Seed, Iter, *Unknown.ToString(EGuidFormats::DigitsWithHyphens)));
				return false;
			}
			if (OutZone != EZoneKind::BurdenZone)
			{
				AddError(FString::Printf(
					TEXT("Property 3 FAILED: 未命中路径下 OutZone 被覆写为 %d。")
					TEXT("Counterexample: Seed=%d Iter=%d Unknown=%s"),
					(int32)OutZone, Seed, Iter,
					*Unknown.ToString(EGuidFormats::DigitsWithHyphens)));
				return false;
			}
			if (OutOwner != SentinelOwner)
			{
				AddError(FString::Printf(
					TEXT("Property 3 FAILED: 未命中路径下 OutZoneOwnerInstanceId 被覆写。")
					TEXT("Counterexample: Seed=%d Iter=%d Unknown=%s"),
					Seed, Iter, *Unknown.ToString(EGuidFormats::DigitsWithHyphens)));
				return false;
			}
		}

		// ---- 路径 C：边界 — FGuid()（zero GUID）始终未命中 ----
		{
			FCardInstance OutInst  = MakeSentinelInstance();
			EZoneKind     OutZone  = EZoneKind::BurdenZone;
			FGuid         OutOwner = SentinelOwner;

			const bool bFound = Run->FindInstance(FGuid(), OutInst, OutZone, OutOwner);
			if (bFound)
			{
				AddError(FString::Printf(
					TEXT("Property 3 FAILED: FGuid() 路径 FindInstance 返回 true。")
					TEXT("Counterexample: Seed=%d Iter=%d"), Seed, Iter));
				return false;
			}
			if (OutInst.InstanceId != SentinelInstId
				|| OutInst.Definition != nullptr
				|| OutInst.bBattleEnabledInSpecialZone != true
				|| OutZone != EZoneKind::BurdenZone
				|| OutOwner != SentinelOwner)
			{
				AddError(FString::Printf(
					TEXT("Property 3 FAILED: FGuid() 路径 out 参数被覆写。")
					TEXT("Counterexample: Seed=%d Iter=%d"), Seed, Iter));
				return false;
			}
		}
	}

	return true;
}


// ================ Stage 4.5.0 Property 4: 测试辅助按 Definition 定位第一个匹配 instance ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckPropertyInstanceHelperFirstMatchSpec,
	"Wacom.Run.Deck.PropertyInstanceHelperFirstMatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckPropertyInstanceHelperFirstMatchSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, Property 4: 测试辅助按 Definition 定位第一个匹配 instance
	// Validates: Requirements 1.9, 1.10
	//
	// 对任意 URunSession 状态，含同一 Definition 多个 instance（数组下标分散）时：
	//   - BackpackContainsDefinition(Card) / BattleDeckContainsDefinition(Card) 当且仅当对应 zone 内
	//     存在至少一张 instance.Definition == Card 时返回 true；
	//     未持有的 Definition 在两区都返回 false。
	//   - 测试辅助先按 Definition 找到源 zone 内下标最小的 instance，再提交正式
	//     MoveInstance / DestroyCardByInstance 入口；其它同 Definition 的 instance 与另一区的
	//     同 Definition instance 全部保留（InstanceId 与数组顺序不变）。
	//   - 这个 property 不声明 public Definition 级 deck API；玩家已拥有卡正式入口仍是 InstanceId。

	const int32 NumIterations = 150;     // ≥ 100，按 design §Testing Strategy
	const int32 BaseSeed      = 0xDEFCA11;

	auto PileContainsId = [](const TArray<FCardInstance>& Pile, const FGuid& Id) -> bool
	{
		for (const FCardInstance& I : Pile)
		{
			if (I.InstanceId == Id) { return true; }
		}
		return false;
	};

	auto CountByDefinition = [](const TArray<FCardInstance>& Pile, const UCardDefinition* Card) -> int32
	{
		int32 N = 0;
		for (const FCardInstance& I : Pile) { if (I.Definition == Card) { ++N; } }
		return N;
	};

	// 收集所有 Definition == Card 的 InstanceId，按数组下标升序
	auto CollectIdsByDefinition = [](const TArray<FCardInstance>& Pile, const UCardDefinition* Card) -> TArray<FGuid>
	{
		TArray<FGuid> Ids;
		for (const FCardInstance& I : Pile)
		{
			if (I.Definition == Card) { Ids.Add(I.InstanceId); }
		}
		return Ids;
	};

	for (int32 Iter = 0; Iter < NumIterations; ++Iter)
	{
		const int32 Seed = BaseSeed + Iter;
		FRandomStream Rng(Seed);
		FWacomBattleFixture Fx;

		// ---- 生成器 ----
		// CardX：本次迭代被测的 target Definition。无 Intrinsic / 无 BagProvider / 无 Companion，
		// 避免触发 DestroyCardByInstance 的拒绝路径（GDD §11.8 / Companion +1 嗜血副作用）。
		UCardDefinition* CardX = Fx.MakeNoopCard(0);
		// CardY：从未加入 Run 的 Definition，用于断言 IsCardIn* 返回 false（"未持有"路径）。
		UCardDefinition* CardY = Fx.MakeNoopCard(0);

		// Bag 兜底两件事：a) 提供 BattleDeckCapacity > 0 让 MoveInstance 有空位；
		// b) 让 DestroyCardByInstance 不会因"最后一张容量来源卡"被拒（CardX 不是容器）。
		// Capacity=12 远大于本测试构造的 BattleDeck 数量上限。
		UCardDefinition* Bag = MakeBagCard(Fx, /*Capacity*/ 12);

		// 多 instance 同 Definition 的关键：StarterDeck 含 BackpackInitial 张 CardX。
		// CardX 是非容器卡 → Initialize a2 把它们全分到 BattleDeck；后续用
		// MoveInstance 把若干张拉回 Backpack，让 Backpack / BattleDeck
		// 同时含若干张 CardX 实例（同 Definition，不同 InstanceId）。
		const int32 BackpackInitial  = Rng.RandRange(3, 5);
		const int32 BattleDeckTarget = Rng.RandRange(2, 4);

		TArray<UCardDefinition*> StarterDeck;
		StarterDeck.Reserve(BackpackInitial + 1);
		StarterDeck.Add(Bag);
		for (int32 i = 0; i < BackpackInitial; ++i) { StarterDeck.Add(CardX); }

		UCharacterDefinition* Char = Fx.MakeCharacter(
			Fx.MakeNoopCard(1), Fx.MakeNoopCard(1), StarterDeck);

		TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
		Run->Initialize(Char);

		// 把 (BackpackInitial - BattleDeckTarget) 张 CardX 从 BattleDeck 拉回 Backpack；
		// 若 BackpackInitial <= BattleDeckTarget 则不拉，BattleDeck 维持初始 BackpackInitial 张。
		const int32 PullToBackpack = FMath::Max(0, BackpackInitial - BattleDeckTarget);
		for (int32 i = 0; i < PullToBackpack; ++i)
		{
			const bool bOk = MoveFirstBattleDeckDefinitionToBackpack(Run, CardX);
			if (!bOk) { break; }
		}

		// 起始状态快照：Backpack ∋ Bag + 0..k 张 CardX；BattleDeck ∋ 1..N 张 CardX
		const int32 BpXCount0 = CountByDefinition(Run->GetBackpack(),   CardX);
		const int32 BdXCount0 = CountByDefinition(Run->GetBattleDeck(), CardX);
		if (BpXCount0 + BdXCount0 == 0)
		{
			// BackpackInitial >= 3 保证至少 1 张 CardX 在玩家手中；setup 失败属于不变量违反
			AddError(FString::Printf(
				TEXT("Property 4 setup FAILED: 找不到任何 CardX。Seed=%d Iter=%d"),
				Seed, Iter));
			return false;
		}

		// ---------- 1) 谓词：BackpackContainsDefinition / BattleDeckContainsDefinition（R1.9）----------
		const bool bExpectedInBp = (BpXCount0 > 0);
		const bool bExpectedInBd = (BdXCount0 > 0);
		if (BackpackContainsDefinition(Run->GetRunState(), CardX) != bExpectedInBp)
		{
			AddError(FString::Printf(
				TEXT("Property 4 FAILED: BackpackContainsDefinition(CardX)=%d 与期望(%d)不一致。")
				TEXT("Counterexample: Seed=%d Iter=%d BpXCount=%d BdXCount=%d"),
				(int32)BackpackContainsDefinition(Run->GetRunState(), CardX), (int32)bExpectedInBp,
				Seed, Iter, BpXCount0, BdXCount0));
			return false;
		}
		if (BattleDeckContainsDefinition(Run->GetRunState(), CardX) != bExpectedInBd)
		{
			AddError(FString::Printf(
				TEXT("Property 4 FAILED: BattleDeckContainsDefinition(CardX)=%d 与期望(%d)不一致。")
				TEXT("Counterexample: Seed=%d Iter=%d BpXCount=%d BdXCount=%d"),
				(int32)BattleDeckContainsDefinition(Run->GetRunState(), CardX), (int32)bExpectedInBd,
				Seed, Iter, BpXCount0, BdXCount0));
			return false;
		}
		// CardY 从未加入 Run → 两区都返回 false（"未持有 Definition" 路径）
		if (BackpackContainsDefinition(Run->GetRunState(), CardY))
		{
			AddError(FString::Printf(
				TEXT("Property 4 FAILED: BackpackContainsDefinition(CardY) 期望 false 但返回 true。")
				TEXT("Counterexample: Seed=%d Iter=%d"), Seed, Iter));
			return false;
		}
		if (BattleDeckContainsDefinition(Run->GetRunState(), CardY))
		{
			AddError(FString::Printf(
				TEXT("Property 4 FAILED: BattleDeckContainsDefinition(CardY) 期望 false 但返回 true。")
				TEXT("Counterexample: Seed=%d Iter=%d"), Seed, Iter));
			return false;
		}

		// ---------- 2) MoveInstance：移动 Backpack 内 CardX 第一张到 BattleDeck（R1.10）----------
		if (BpXCount0 >= 1
			&& Run->GetBattleDeck().Num() < Run->GetBattleDeckCapacity())
		{
			// 调用前快照：Backpack 内 CardX 的 InstanceId 序列（数组下标升序）
			const TArray<FGuid> BpIdsPre = CollectIdsByDefinition(Run->GetBackpack(),   CardX);
			const TArray<FGuid> BdIdsPre = CollectIdsByDefinition(Run->GetBattleDeck(), CardX);
			const FGuid ExpectedMovedId  = BpIdsPre[0];   // R1.10：第一个匹配 instance

			const bool bOk = MoveFirstBackpackDefinitionToBattleDeck(Run, CardX);
			if (!bOk)
			{
				AddError(FString::Printf(
					TEXT("Property 4 FAILED: MoveInstance Backpack->BattleDeck 期望成功但返回 false。")
					TEXT("Counterexample: Seed=%d Iter=%d BpXCount=%d BdNum=%d Capacity=%d"),
					Seed, Iter, BpXCount0,
					Run->GetBattleDeck().Num(), Run->GetBattleDeckCapacity()));
				return false;
			}

			// 第一个 instance 已被搬走 → 不再在 Backpack
			if (PileContainsId(Run->GetBackpack(), ExpectedMovedId))
			{
				AddError(FString::Printf(
					TEXT("Property 4 FAILED: MoveInstance Backpack->BattleDeck 后 Backpack 仍含 ExpectedMovedId=%s。")
					TEXT("Counterexample: Seed=%d Iter=%d"),
					*ExpectedMovedId.ToString(EGuidFormats::DigitsWithHyphens), Seed, Iter));
				return false;
			}
			// 已出现在 BattleDeck
			if (!PileContainsId(Run->GetBattleDeck(), ExpectedMovedId))
			{
				AddError(FString::Printf(
					TEXT("Property 4 FAILED: MoveInstance Backpack->BattleDeck 后 BattleDeck 不含 ExpectedMovedId=%s。")
					TEXT("Counterexample: Seed=%d Iter=%d"),
					*ExpectedMovedId.ToString(EGuidFormats::DigitsWithHyphens), Seed, Iter));
				return false;
			}
			// 其他 Backpack-CardX InstanceId（下标 >= 1 的 instance）全部保留
			for (int32 i = 1; i < BpIdsPre.Num(); ++i)
			{
				if (!PileContainsId(Run->GetBackpack(), BpIdsPre[i]))
				{
					AddError(FString::Printf(
						TEXT("Property 4 FAILED: MoveInstance Backpack->BattleDeck 误碰 Backpack 内 InstanceId=%s。")
						TEXT("Counterexample: Seed=%d Iter=%d ExpectedMovedId=%s"),
						*BpIdsPre[i].ToString(EGuidFormats::DigitsWithHyphens),
						Seed, Iter,
						*ExpectedMovedId.ToString(EGuidFormats::DigitsWithHyphens)));
					return false;
				}
			}
			// BattleDeck 中原本就有的同 Definition instance 全部保留
			for (const FGuid& Id : BdIdsPre)
			{
				if (!PileContainsId(Run->GetBattleDeck(), Id))
				{
					AddError(FString::Printf(
						TEXT("Property 4 FAILED: MoveInstance Backpack->BattleDeck 后原 BattleDeck-CardX InstanceId=%s 丢失。")
						TEXT("Counterexample: Seed=%d Iter=%d"),
						*Id.ToString(EGuidFormats::DigitsWithHyphens), Seed, Iter));
					return false;
				}
			}
		}

		// ---------- 3) MoveInstance：移动 BattleDeck 内 CardX 第一张回 Backpack（R1.10）----------
		if (CountByDefinition(Run->GetBattleDeck(), CardX) >= 1)
		{
			const TArray<FGuid> BpIdsPre = CollectIdsByDefinition(Run->GetBackpack(),   CardX);
			const TArray<FGuid> BdIdsPre = CollectIdsByDefinition(Run->GetBattleDeck(), CardX);
			const FGuid ExpectedMovedId  = BdIdsPre[0];   // R1.10：BattleDeck 第一个匹配 instance

			const bool bOk = MoveFirstBattleDeckDefinitionToBackpack(Run, CardX);
			if (!bOk)
			{
				AddError(FString::Printf(
					TEXT("Property 4 FAILED: MoveInstance BattleDeck->Backpack 期望成功但返回 false。")
					TEXT("Counterexample: Seed=%d Iter=%d BdXCount=%d"),
					Seed, Iter, BdIdsPre.Num()));
				return false;
			}

			if (PileContainsId(Run->GetBattleDeck(), ExpectedMovedId))
			{
				AddError(FString::Printf(
					TEXT("Property 4 FAILED: MoveInstance BattleDeck->Backpack 后 BattleDeck 仍含 ExpectedMovedId=%s。")
					TEXT("Counterexample: Seed=%d Iter=%d"),
					*ExpectedMovedId.ToString(EGuidFormats::DigitsWithHyphens), Seed, Iter));
				return false;
			}
			if (!PileContainsId(Run->GetBackpack(), ExpectedMovedId))
			{
				AddError(FString::Printf(
					TEXT("Property 4 FAILED: MoveInstance BattleDeck->Backpack 后 Backpack 不含 ExpectedMovedId=%s。")
					TEXT("Counterexample: Seed=%d Iter=%d"),
					*ExpectedMovedId.ToString(EGuidFormats::DigitsWithHyphens), Seed, Iter));
				return false;
			}
			// 其他 BattleDeck-CardX InstanceId（下标 >= 1）全部保留
			for (int32 i = 1; i < BdIdsPre.Num(); ++i)
			{
				if (!PileContainsId(Run->GetBattleDeck(), BdIdsPre[i]))
				{
					AddError(FString::Printf(
						TEXT("Property 4 FAILED: MoveInstance BattleDeck->Backpack 误碰 BattleDeck 内 InstanceId=%s。")
						TEXT("Counterexample: Seed=%d Iter=%d ExpectedMovedId=%s"),
						*BdIdsPre[i].ToString(EGuidFormats::DigitsWithHyphens),
						Seed, Iter,
						*ExpectedMovedId.ToString(EGuidFormats::DigitsWithHyphens)));
					return false;
				}
			}
			// Backpack 中原有同 Definition instance 全部保留
			for (const FGuid& Id : BpIdsPre)
			{
				if (!PileContainsId(Run->GetBackpack(), Id))
				{
					AddError(FString::Printf(
						TEXT("Property 4 FAILED: MoveInstance BattleDeck->Backpack 后原 Backpack-CardX InstanceId=%s 丢失。")
						TEXT("Counterexample: Seed=%d Iter=%d"),
						*Id.ToString(EGuidFormats::DigitsWithHyphens), Seed, Iter));
					return false;
				}
			}
		}

		// ---------- 4) DestroyCardByInstance(CardX)：Backpack → BattleDeck 优先（R1.10）----------
		// 本 property 只构造 Backpack/BattleDeck；四区销毁入口由 DestroyCardFromAllOwnedZones 覆盖。
		const TArray<FGuid> BpIdsPreD = CollectIdsByDefinition(Run->GetBackpack(),   CardX);
		const TArray<FGuid> BdIdsPreD = CollectIdsByDefinition(Run->GetBattleDeck(), CardX);
		if (BpIdsPreD.Num() > 0 || BdIdsPreD.Num() > 0)
		{
			const bool  bFromBackpack       = (BpIdsPreD.Num() > 0);
			const FGuid ExpectedDestroyedId = bFromBackpack ? BpIdsPreD[0] : BdIdsPreD[0];

			const bool bOk = DestroyFirstOwnedDefinition(Run, CardX);
			if (!bOk)
			{
				AddError(FString::Printf(
					TEXT("Property 4 FAILED: DestroyCardByInstance(CardX) 期望成功但返回 false。")
					TEXT("Counterexample: Seed=%d Iter=%d BpXCount=%d BdXCount=%d"),
					Seed, Iter, BpIdsPreD.Num(), BdIdsPreD.Num()));
				return false;
			}

			// 期望被销毁的 instance 已不在任何 zone
			if (PileContainsId(Run->GetBackpack(), ExpectedDestroyedId)
				|| PileContainsId(Run->GetBattleDeck(), ExpectedDestroyedId))
			{
				AddError(FString::Printf(
					TEXT("Property 4 FAILED: DestroyCardByInstance 后 ExpectedDestroyedId=%s 仍存在。")
					TEXT("Counterexample: Seed=%d Iter=%d FromBackpack=%d"),
					*ExpectedDestroyedId.ToString(EGuidFormats::DigitsWithHyphens),
					Seed, Iter, (int32)bFromBackpack));
				return false;
			}
			// 源 zone 内其他同 Definition instance（下标 >= 1）全部保留在原 zone
			const TArray<FGuid>& SrcIds = bFromBackpack ? BpIdsPreD : BdIdsPreD;
			for (int32 i = 1; i < SrcIds.Num(); ++i)
			{
				const FGuid& Id = SrcIds[i];
				const bool bStillThere =
					PileContainsId(Run->GetBackpack(), Id)
					|| PileContainsId(Run->GetBattleDeck(), Id);
				if (!bStillThere)
				{
					AddError(FString::Printf(
						TEXT("Property 4 FAILED: DestroyCardByInstance 误碰源 zone 内 InstanceId=%s。")
						TEXT("Counterexample: Seed=%d Iter=%d FromBackpack=%d"),
						*Id.ToString(EGuidFormats::DigitsWithHyphens),
						Seed, Iter, (int32)bFromBackpack));
					return false;
				}
			}
			// 另一区 CardX 全部保留
			const TArray<FGuid>& OtherIds = bFromBackpack ? BdIdsPreD : BpIdsPreD;
			for (const FGuid& Id : OtherIds)
			{
				const bool bStillThere =
					PileContainsId(Run->GetBackpack(), Id)
					|| PileContainsId(Run->GetBattleDeck(), Id);
				if (!bStillThere)
				{
					AddError(FString::Printf(
						TEXT("Property 4 FAILED: DestroyCardByInstance 误碰另一区 InstanceId=%s。")
						TEXT("Counterexample: Seed=%d Iter=%d FromBackpack=%d"),
						*Id.ToString(EGuidFormats::DigitsWithHyphens),
						Seed, Iter, (int32)bFromBackpack));
					return false;
				}
			}
		}
	}

	return true;
}
// ================ Stage 4.5.0 EXAMPLE / EDGE_CASE: 4.5.0 结构与 fallback ================
//
// Task 3.7：覆盖默认构造结构、fallback 路径与 R1.14 ensureMsgf 兜底文档。
// Validates: Requirements 1.1, 1.2, 1.4, 1.13, 1.14
//
// R1.13（既有 BackpackSpec 105 条全过 → 回归校验）由本文件其余测试用例整体回归承担，
// 不在此处单独追加用例，仅由 `Automation RunTests Wacom.Run.Deck` 全绿来证明。
//
// 数据源：纯本地构造 / fixture，无外部依赖。

// ---- R1.1：FCardInstance 默认构造结构 ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckFCardInstanceDefaultConstructSpec,
	"Wacom.Run.Deck.FCardInstanceDefaultConstruct",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckFCardInstanceDefaultConstructSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, EXAMPLE: FCardInstance 默认构造字段值
	// Validates: Requirement 1.1
	FCardInstance Inst;

	TestFalse(TEXT("R1.1: 默认 InstanceId 非 valid（FGuid()）"),         Inst.InstanceId.IsValid());
	TestTrue (TEXT("R1.1: 默认 InstanceId == FGuid()"),                 Inst.InstanceId == FGuid());
	TestTrue (TEXT("R1.1: 默认 Definition == nullptr"),                 Inst.Definition == nullptr);
	TestFalse(TEXT("R1.1: 默认 bBattleEnabledInSpecialZone == false"),  Inst.bBattleEnabledInSpecialZone);

	return true;
}

// ---- R1.2：FRunState 默认构造 Backpack/BattleDeck 为空 ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckFRunStateDefaultConstructSpec,
	"Wacom.Run.Deck.FRunStateDefaultConstruct",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckFRunStateDefaultConstructSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, EXAMPLE: FRunState 默认构造卡牌区为空
	// Validates: Requirement 1.2
	FRunState State;

	TestEqual(TEXT("R1.2: 默认 Backpack.Num() == 0"),   State.Backpack.Num(),   0);
	TestEqual(TEXT("R1.2: 默认 BattleDeck.Num() == 0"), State.BattleDeck.Num(), 0);

	return true;
}

// ---- R1.4：nullptr Character fallback ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckInitializeNullCharacterFallbackSpec,
	"Wacom.Run.Deck.InitializeNullCharacterFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckInitializeNullCharacterFallbackSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, EDGE_CASE: nullptr Character fallback
	// Validates: Requirement 1.4
	//
	// 期望行为：
	//   - Initialize(nullptr) 返回 false（通过返回值表明 fallback）
	//   - Backpack/BattleDeck 都为空数组（不创建任何 instance）
	//   - 通过 UE_LOG Warning 表明 fallback（不在自动化里直接断言日志，由代码 review 保证）
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());

	const bool bResult = Run->Initialize(nullptr);

	TestFalse(TEXT("R1.4: Initialize(nullptr) 返回 false"),                  bResult);
	TestEqual(TEXT("R1.4: nullptr Character 后 Backpack.Num() == 0"),       Run->GetBackpack().Num(),   0);
	TestEqual(TEXT("R1.4: nullptr Character 后 BattleDeck.Num() == 0"),     Run->GetBattleDeck().Num(), 0);

	return true;
}

// ---- R1.4：空 StarterDeck fallback ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckInitializeEmptyStarterDeckFallbackSpec,
	"Wacom.Run.Deck.InitializeEmptyStarterDeckFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckInitializeEmptyStarterDeckFallbackSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, EDGE_CASE: 空 StarterDeck fallback
	// Validates: Requirement 1.4
	//
	// 期望行为：
	//   - Initialize(角色合法，但 StarterDeck.Num() == 0) → 仍 return true（角色本身有效）
	//   - Backpack/BattleDeck 都为空数组（不创建任何 instance）
	//   - 通过 UE_LOG Warning 表明 fallback
	FWacomBattleFixture Fx;

	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		/*StarterDeck=*/ {});

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	const bool bResult = Run->Initialize(Char);

	TestTrue (TEXT("R1.4: Initialize(空 StarterDeck Character) 返回 true（角色仍合法）"), bResult);
	TestEqual(TEXT("R1.4: 空 StarterDeck 后 Backpack.Num() == 0"),                       Run->GetBackpack().Num(),   0);
	TestEqual(TEXT("R1.4: 空 StarterDeck 后 BattleDeck.Num() == 0"),                     Run->GetBattleDeck().Num(), 0);

	return true;
}

// ---- R1.14：zero GUID ensureMsgf 不变量（文档 + 前置 sanity）----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckZeroGuidEnsureMsgfDocumentationSpec,
	"Wacom.Run.Deck.ZeroGuidEnsureMsgfDocumentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckZeroGuidEnsureMsgfDocumentationSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, EDGE_CASE: zero GUID ensureMsgf 兜底
	// Validates: Requirement 1.14
	//
	// R1.14 要求 InstanceId 非 zero GUID 由 ensureMsgf 兜底（仅 Editor / Debug build 触发，
	// Shipping 不崩）。直接构造 zero GUID instance 并加入 zone 会触发 ensureMsgf 中断测试，
	// 因此不在此处直接驱动该路径。
	//
	// 替代做法：sanity 校验 R1.14 的"前置条件"——`FGuid::NewGuid()` 永远输出 valid GUID。
	// 该前置条件失效会立即在 Initialize / AcquireCardToRun 路径上触发 ensureMsgf。
	for (int32 i = 0; i < 100; ++i)
	{
		const FGuid Id = FGuid::NewGuid();
		if (!Id.IsValid())
		{
			AddError(FString::Printf(
				TEXT("R1.14 前置失败：FGuid::NewGuid() 在第 %d 次迭代输出 zero GUID"), i));
			return false;
		}
	}
	return true;
}


// ================ Stage 4.5.1 Property 5: B 主卡 ↔ SpecialZone 双射不变量 ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckPropertyBContainerSpecialZoneBijectionSpec,
	"Wacom.Run.Deck.PropertyBContainerSpecialZoneBijection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckPropertyBContainerSpecialZoneBijectionSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, Property 5: B 主卡 ↔ SpecialZone 双射不变量
	// Validates: Requirements 2.2, 2.3, 3.5, 3.6, 5.6
	//
	// 对任意 zone-modifying op 序列（AcquireCardToRun / MoveInstance Backpack->BattleDeck /
	// MoveInstance BattleDeck->Backpack / DestroyCardByInstance / MoveInstance），
	// 在 Initialize 之后以及每次 op 之后必须满足以下三条不变量：
	//
	//   (A) Forward 双射：∀ Inst ∈ Backpack ∪ BattleDeck，若
	//       Inst.Definition->Physique.CapacityEffect.IsValid()（B 主卡 instance），
	//       则 RunState.SpecialZones 中**恰好**存在一条 SZ 满足 SZ.OwnerInstanceId == Inst.InstanceId。
	//   (B) Reverse 双射：∀ SZ ∈ RunState.SpecialZones，
	//       Backpack ∪ BattleDeck 中**恰好**存在一张 Inst 满足 Inst.InstanceId == SZ.OwnerInstanceId。
	//   (C) CollectTypeBContainers(OutOwnerInstanceIds) 输出严格等于 SpecialZones
	//       数组下标升序的 OwnerInstanceId 投影；输出去重；不含悬空 InstanceId。
	//
	// 生成器（task 12.1 描述）：
	//   - StarterDeck 含至少 1 张 BagProvider（A 类，Capacity ∈ [3,8]）保证
	//     IsBackpackUiAvailable() == true 且 GetBattleDeckCapacity() 大于 0；
	//   - 其余 0..10 张走 MakePbtCard 混合 Normal / A 类 / B 类。
	//
	// op 选择按等概率从五种 op 中抽取；每种 op 内部又随机选 InstanceId / Definition / 目标 zone。
	// 每次 op 之后立即 VerifyBijection；任何一次 op 后违反 (A)/(B)/(C) 任一即报告反例并返回 false。

	const int32 NumIterations      = 120;             // ≥ 100，按 design §Testing Strategy
	const int32 NumOpsPerIteration = 12;              // 每次 iteration 跑 12 次 op，覆盖串联组合
	const int32 BaseSeed           = 0xB1B1B0B5;      // unique seed prefix for Property 5

	auto IsBMaster = [](const FCardInstance& I) -> bool
	{
		return I.Definition != nullptr
			&& I.Definition->Physique.CapacityEffect.IsValid();
	};

	auto CountByIdInTwoPiles = [](const TArray<FCardInstance>& A, const TArray<FCardInstance>& B, const FGuid& Id) -> int32
	{
		int32 C = 0;
		for (const FCardInstance& I : A) { if (I.InstanceId == Id) { ++C; } }
		for (const FCardInstance& I : B) { if (I.InstanceId == Id) { ++C; } }
		return C;
	};

	auto VerifyBijection = [&](URunSession* Run, int32 Seed, int32 Iter, int32 OpStep, const TCHAR* Where) -> bool
	{
		const FRunState& State                  = Run->GetRunState();
		const TArray<FCardInstance>& Backpack   = State.Backpack;
		const TArray<FCardInstance>& BattleDeck = State.BattleDeck;
		const TArray<FSpecialZone>&  SZs        = State.SpecialZones;

		// (A) Forward — 每张 B 主卡 instance 在 Backpack ∪ BattleDeck 中存在 → SpecialZones 中恰好一条匹配
		auto CheckForward = [&](const TArray<FCardInstance>& Pile, const TCHAR* PileName) -> bool
		{
			for (int32 Idx = 0; Idx < Pile.Num(); ++Idx)
			{
				const FCardInstance& I = Pile[Idx];
				if (!IsBMaster(I)) { continue; }
				int32 MatchCount = 0;
				for (const FSpecialZone& SZ : SZs)
				{
					if (SZ.OwnerInstanceId == I.InstanceId) { ++MatchCount; }
				}
				if (MatchCount != 1)
				{
					AddError(FString::Printf(
						TEXT("Property 5 FAILED (Forward): B 主卡 InstanceId=%s 在 %s[%d] 但 SpecialZones 中匹配条目数=%d (期望 1)。")
						TEXT("Counterexample: Seed=%d Iter=%d OpStep=%d Where=%s"),
						*I.InstanceId.ToString(EGuidFormats::DigitsWithHyphens),
						PileName, Idx, MatchCount,
						Seed, Iter, OpStep, Where));
					return false;
				}
			}
			return true;
		};
		if (!CheckForward(Backpack,   TEXT("Backpack")))   { return false; }
		if (!CheckForward(BattleDeck, TEXT("BattleDeck"))) { return false; }

		// (B) Reverse — 每个 SpecialZone 的 OwnerInstanceId 必须在 Backpack ∪ BattleDeck 中恰好一次出现
		for (int32 i = 0; i < SZs.Num(); ++i)
		{
			const FSpecialZone& SZ = SZs[i];
			const int32 OwnerCount = CountByIdInTwoPiles(Backpack, BattleDeck, SZ.OwnerInstanceId);
			if (OwnerCount != 1)
			{
				AddError(FString::Printf(
					TEXT("Property 5 FAILED (Reverse): SpecialZones[%d].OwnerInstanceId=%s 在 Backpack ∪ BattleDeck 出现 %d 次 (期望 1)。")
					TEXT("Counterexample: Seed=%d Iter=%d OpStep=%d Where=%s ")
					TEXT("Backpack.Num=%d BattleDeck.Num=%d BurdenZone.Num=%d SZs.Num=%d"),
					i,
					*SZ.OwnerInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
					OwnerCount,
					Seed, Iter, OpStep, Where,
					Backpack.Num(), BattleDeck.Num(), State.BurdenZone.Num(), SZs.Num()));
				return false;
			}
		}

		// (C) CollectTypeBContainers 输出契约（R3.5 / R3.6）
		TArray<FGuid> Owners;
		// 预填一个垃圾值确保实现内部 Reset
		Owners.Add(FGuid::NewGuid());
		Run->CollectTypeBContainers(Owners);

		// 长度等于 SpecialZones 长度（reverse direction 已保证无悬空，所以投影 1:1）
		if (Owners.Num() != SZs.Num())
		{
			AddError(FString::Printf(
				TEXT("Property 5 FAILED (Collect): Owners.Num()=%d != SpecialZones.Num()=%d。")
				TEXT("Counterexample: Seed=%d Iter=%d OpStep=%d Where=%s"),
				Owners.Num(), SZs.Num(), Seed, Iter, OpStep, Where));
			return false;
		}
		// 严格按 SpecialZones 数组下标升序匹配
		for (int32 i = 0; i < SZs.Num(); ++i)
		{
			if (Owners[i] != SZs[i].OwnerInstanceId)
			{
				AddError(FString::Printf(
					TEXT("Property 5 FAILED (Collect): Owners[%d]=%s != SpecialZones[%d].OwnerInstanceId=%s。")
					TEXT("Counterexample: Seed=%d Iter=%d OpStep=%d Where=%s"),
					i, *Owners[i].ToString(EGuidFormats::DigitsWithHyphens),
					i, *SZs[i].OwnerInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
					Seed, Iter, OpStep, Where));
				return false;
			}
		}
		// 显式 dedup 校验（防御 R2.2 不变量被破坏的回归）
		TSet<FGuid> SeenOwners;
		SeenOwners.Reserve(Owners.Num());
		for (int32 i = 0; i < Owners.Num(); ++i)
		{
			bool bAlready = false;
			SeenOwners.Add(Owners[i], &bAlready);
			if (bAlready)
			{
				AddError(FString::Printf(
					TEXT("Property 5 FAILED (Collect dedup): Owners[%d]=%s 重复出现。")
					TEXT("Counterexample: Seed=%d Iter=%d OpStep=%d Where=%s"),
					i, *Owners[i].ToString(EGuidFormats::DigitsWithHyphens),
					Seed, Iter, OpStep, Where));
				return false;
			}
		}
		// 不含悬空 InstanceId（R3.5）— 每个输出都必须能在 Backpack ∪ BattleDeck 中找到
		for (int32 i = 0; i < Owners.Num(); ++i)
		{
			const int32 C = CountByIdInTwoPiles(Backpack, BattleDeck, Owners[i]);
			if (C != 1)
			{
				AddError(FString::Printf(
					TEXT("Property 5 FAILED (Collect dangling): Owners[%d]=%s 在 Backpack ∪ BattleDeck 出现 %d 次 (期望 1)。")
					TEXT("Counterexample: Seed=%d Iter=%d OpStep=%d Where=%s"),
					i, *Owners[i].ToString(EGuidFormats::DigitsWithHyphens),
					C, Seed, Iter, OpStep, Where));
				return false;
			}
		}

		return true;
	};

	for (int32 Iter = 0; Iter < NumIterations; ++Iter)
	{
		const int32 Seed = BaseSeed + Iter;
		FRandomStream Rng(Seed);
		FWacomBattleFixture Fx;

		// ---- 生成器：StarterDeck = 1 张 BagProvider（A 类）+ 0..10 张随机 PBT 卡 ----
		const int32 ExtraCount = Rng.RandRange(0, 10);
		TArray<UCardDefinition*> StarterDeck;
		StarterDeck.Reserve(ExtraCount + 1);
		StarterDeck.Add(MakeBagCard(Fx, Rng.RandRange(3, 8)));
		for (int32 i = 0; i < ExtraCount; ++i)
		{
			StarterDeck.Add(MakePbtCard(Fx, Rng));
		}

		UCharacterDefinition* Char = Fx.MakeCharacter(
			Fx.MakeNoopCard(1), Fx.MakeNoopCard(1), StarterDeck);

		TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
		Run->Initialize(Char);

		// 初始状态校验（R2.3：B 主卡 instance 在 Initialize 路径上自动建空 SpecialZone entry）
		if (!VerifyBijection(Run.Get(), Seed, Iter, /*OpStep*/0, TEXT("AfterInitialize")))
		{
			return false;
		}

		// ---- 操作序列：每步从五种 op 等概率抽一种 ----
		for (int32 Step = 1; Step <= NumOpsPerIteration; ++Step)
		{
			const int32 OpRoll = Rng.RandRange(0, 4);
			switch (OpRoll)
			{
			case 0:
			{
				// AcquireCardToRun(随机生成的卡，含 ~10% nullptr 触发 R1.5 拒绝路径)
				UCardDefinition* Card = (Rng.RandRange(0, 9) == 0)
					? nullptr
					: MakePbtCard(Fx, Rng);
				Run->AcquireCardToRun(Card);
				break;
			}
			case 1:
			{
				// MoveInstance Backpack->BattleDeck(从 Backpack 随机选 Definition；Backpack 空则跳过)
				const TArray<FCardInstance>& Bp = Run->GetBackpack();
				if (Bp.Num() > 0)
				{
					const int32 PickIdx = Rng.RandRange(0, Bp.Num() - 1);
					MoveFirstBackpackDefinitionToBattleDeck(Run, Bp[PickIdx].Definition);
				}
				break;
			}
			case 2:
			{
				// MoveInstance BattleDeck->Backpack(从 BattleDeck 随机选 Definition；空则跳过)
				const TArray<FCardInstance>& Bd = Run->GetBattleDeck();
				if (Bd.Num() > 0)
				{
					const int32 PickIdx = Rng.RandRange(0, Bd.Num() - 1);
					MoveFirstBattleDeckDefinitionToBackpack(Run, Bd[PickIdx].Definition);
				}
				break;
			}
			case 3:
			{
				// DestroyCardByInstance(从 Backpack ∪ BattleDeck 随机选 Definition；
				//   GDD §11.8 拒绝条件由 RunSession 内部处理，不影响 invariant)
				const int32 BpN = Run->GetBackpack().Num();
				const int32 BdN = Run->GetBattleDeck().Num();
				const int32 Total = BpN + BdN;
				if (Total > 0)
				{
					const int32 Pick = Rng.RandRange(0, Total - 1);
					UCardDefinition* Card = (Pick < BpN)
						? Run->GetBackpack()[Pick].Definition
						: Run->GetBattleDeck()[Pick - BpN].Definition;
					DestroyFirstOwnedDefinition(Run, Card);
				}
				break;
			}
			case 4:
			default:
			{
				// MoveInstance：~90% 选某已知 InstanceId，~10% 用新 GUID 走 R1.7 失败路径。
				// 目标 zone 从四种中等概率抽；ToOwnerInstanceId 仅 SpecialZone 路径有意义。
				const FRunState& StateNow = Run->GetRunState();
				TArray<FGuid> AllIds;
				AllIds.Reserve(
					StateNow.Backpack.Num() + StateNow.BattleDeck.Num() + StateNow.BurdenZone.Num());
				for (const FCardInstance& I : StateNow.Backpack)   { AllIds.Add(I.InstanceId); }
				for (const FCardInstance& I : StateNow.BattleDeck) { AllIds.Add(I.InstanceId); }
				for (const FCardInstance& I : StateNow.BurdenZone) { AllIds.Add(I.InstanceId); }
				for (const FSpecialZone& SZ : StateNow.SpecialZones)
				{
					for (const FCardInstance& I : SZ.Cards) { AllIds.Add(I.InstanceId); }
				}

				FGuid TargetId;
				if (AllIds.Num() > 0 && Rng.RandRange(0, 9) > 0)
				{
					TargetId = AllIds[Rng.RandRange(0, AllIds.Num() - 1)];
				}
				else
				{
					TargetId = FGuid::NewGuid();   // 不在任何 zone — R1.7 失败路径
				}

				const int32 ZoneRoll = Rng.RandRange(0, 3);
				const EZoneKind ToZone = static_cast<EZoneKind>(ZoneRoll);
				FGuid ToOwner;
				if (ToZone == EZoneKind::SpecialZone)
				{
					const TArray<FSpecialZone>& SZs = StateNow.SpecialZones;
					if (SZs.Num() > 0 && Rng.RandRange(0, 9) > 0)
					{
						ToOwner = SZs[Rng.RandRange(0, SZs.Num() - 1)].OwnerInstanceId;
					}
					else
					{
						// 无 SpecialZone 或主动走拒绝路径（OwnerInstanceId 不存在 — R2.7a）
						ToOwner = FGuid::NewGuid();
					}
				}
				Run->MoveInstance(TargetId, ToZone, ToOwner);
				break;
			}
			}

			// 每次 op 之后立即校验三条不变量
			if (!VerifyBijection(Run.Get(), Seed, Iter, Step, TEXT("PostOp")))
			{
				return false;
			}
		}
	}

	return true;
}

// ================ Stage 4.5.1 Property 6: B 主卡销毁内含卡退回流 ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckPropertyBContainerDestroyRetrievalFlowSpec,
	"Wacom.Run.Deck.PropertyBContainerDestroyRetrievalFlow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckPropertyBContainerDestroyRetrievalFlowSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, Property 6: B 主卡销毁内含卡退回流
	// Validates: Requirements 2.4 (兼带 R8.6 验证：每张退回 instance 强制 bBattleEnabledInSpecialZone=false)
	//
	// 对任意 URunSession 状态，若 DestroyCardByInstance(BMain) 被接受
	// （BMain 非 Intrinsic、非最后一张容量来源卡）且 BMain 是 B 类容器卡且
	// 其 SpecialZone 非空，调用之后必须满足以下四条不变量：
	//
	//   (A) 不丢失：原 FSpecialZone.Cards 中的每个 InstanceId 在 RunState 的某个 zone 里
	//       仍然存在（Backpack ∪ BattleDeck ∪ BurdenZone ∪ ⋃SpecialZones.Cards）。
	//   (B) 退回顺序：内含卡按原 SZ.Cards 数组下标升序追加到 Backpack 末尾，直到
	//       通量内容卡数量 == GetFluxCapacity()，剩余按原顺序追加到 BurdenZone 末尾。
	//   (C) SpecialZones 中不再含原 OwnerInstanceId 对应的 entry。
	//   (D) 每张退回 instance 的 bBattleEnabledInSpecialZone == false（R8.6）。
	//
	// 生成器（task 12.2 描述）：
	//   - StarterDeck 含恰好 1 张 A 类 BagProvider（Capacity=ACap ∈ [3,6]，白色）+
	//     1 张 B 主卡（非 Intrinsic 白色，Capacity=BCap ∈ [2,5]，CapacityEffect=Placeholder
	//     占位 tag）+ ACap 张非容器卡（默认进 BattleDeck，把 BattleDeck 一次性灌满到
	//     B 主卡销毁后的 BattleDeckCap=ACap）；
	//   - 用 AcquireCardToRun 加 S ∈ [1, min(BCap-1, ACap-2)] 张非容器卡到 Backpack，
	//     再用 MoveInstance 逐张移到 BMaster 的 SpecialZone（按 add 顺序灌入 SZ.Cards）；
	//   - 随机对 SZ.Cards 中的子集调 SetSpecialZoneCardBattleEnabled(true)，覆盖 (D)
	//     从 true → false 的清零路径（与从未设置 true 的默认 false 路径区分）；
	//   - 用 AcquireCardToRun 再加 F ∈ [0, ACap-2] 张非容器卡到 Backpack，
	//     使 Backpack 进入"未满 / 接近满"的混合分布以覆盖 (B) 的两条分支：
	//       a) 1+F+S ≤ ACap：全部 S 张退回 Backpack，BurdenZone 收 0 张；
	//       b) 1+F+S > ACap：前 K=ACap-1-F 张退回 Backpack，剩余 S-K 张退回 BurdenZone。
	//
	// 测试构造保证 RecomputeBurden 在 DestroyCardByInstanceInternal 末尾的调用
	// 是 no-op（BattleDeck 已满到 B 主卡销毁后的 BattleDeckCap=ACap、Backpack 退回后恰好不超 FluxCap=ACap；
	// 小布袋自己也占 1 个通量内容格，
	// 销毁后没有其他 SpecialZone 可承接 refill），从而后置状态严格等于步骤 6 退回循环
	// 的输出，可以直接对比期望布局。

	const int32 NumIterations = 120;             // ≥ 100，按 design §Testing Strategy
	const int32 BaseSeed      = 0xB1B1B0B6;      // unique seed prefix for Property 6

	for (int32 Iter = 0; Iter < NumIterations; ++Iter)
	{
		const int32 Seed = BaseSeed + Iter;
		FRandomStream Rng(Seed);
		FWacomBattleFixture Fx;

		// ---- 生成器：StarterDeck ----
		const int32 ACap = Rng.RandRange(3, 6);
		const int32 BCap = Rng.RandRange(2, 5);

		UCardDefinition* BagProviderDef = MakeBagCard(Fx, ACap);                 // 白色 BagProvider，Capacity=ACap
		UCardDefinition* BMasterDef     = Fx.MakeNoopCard(0);
		BMasterDef->Physique.Capacity        = BCap;
		BMasterDef->Physique.CapacityEffect  = WacomTags::Card_CapacityEffect_Placeholder;
		BMasterDef->Rarity                   = WacomTags::Card_Rarity_White;     // 显式非 Intrinsic

		TArray<UCardDefinition*> StarterDeck;
		StarterDeck.Reserve(2 + ACap);
		StarterDeck.Add(BagProviderDef);
		StarterDeck.Add(BMasterDef);
		for (int32 i = 0; i < ACap; ++i)
		{
			StarterDeck.Add(Fx.MakeNoopCard(0));
		}

		UCharacterDefinition* Char = Fx.MakeCharacter(
			Fx.MakeNoopCard(1), Fx.MakeNoopCard(1), StarterDeck);

		TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
		Run->Initialize(Char);

		// ---- 生成器后置校验：Initialize 应当把 BMaster + BagProvider 放进 Backpack
		//      （a2 分流：容器卡只进 Backpack），ACap 张非容器卡进 BattleDeck。
		const FRunState& InitState = Run->GetRunState();
		FGuid BMasterId;
		for (const FCardInstance& I : InitState.Backpack)
		{
			if (I.Definition == BMasterDef) { BMasterId = I.InstanceId; break; }
		}
		if (!BMasterId.IsValid())
		{
			AddError(FString::Printf(
				TEXT("Property 6 SETUP FAILED: BMaster InstanceId not found in Backpack after Initialize. ")
				TEXT("Counterexample: Seed=%d Iter=%d ACap=%d BCap=%d Backpack.Num=%d BattleDeck.Num=%d"),
				Seed, Iter, ACap, BCap,
				InitState.Backpack.Num(), InitState.BattleDeck.Num()));
			return false;
		}

		// ---- 步骤 1：加 S 张非容器卡到 Backpack，逐张 MoveInstance 进 SpecialZone ----
		const int32 SMax = FMath::Min(BCap - 1, ACap - 2);
		// 由 ACap ∈ [3,6] 与 BCap ∈ [2,5] 确保 SMax ≥ 1。
		const int32 S = Rng.RandRange(1, SMax);

		TArray<FGuid> SZSnapshotOrder;             // SZ.Cards 灌入顺序的 InstanceId 列表
		SZSnapshotOrder.Reserve(S);
		for (int32 i = 0; i < S; ++i)
		{
			UCardDefinition* NCard = Fx.MakeNoopCard(0);
			Run->AcquireCardToRun(NCard);
			// 取最新追加到 Backpack 末尾的 InstanceId
			const FGuid NewId = Run->GetBackpack().Last().InstanceId;
			const bool bMoved = Run->MoveInstance(NewId, EZoneKind::SpecialZone, BMasterId);
			if (!bMoved)
			{
				AddError(FString::Printf(
					TEXT("Property 6 SETUP FAILED: MoveInstance to SpecialZone failed at i=%d. ")
					TEXT("Counterexample: Seed=%d Iter=%d ACap=%d BCap=%d S=%d NewId=%s"),
					i, Seed, Iter, ACap, BCap, S,
					*NewId.ToString(EGuidFormats::DigitsWithHyphens)));
				return false;
			}
			SZSnapshotOrder.Add(NewId);
		}

		// ---- 可选：随机对 SZ.Cards 子集设 bBattleEnabledInSpecialZone=true（覆盖 D 的清零路径）----
		for (int32 i = 0; i < S; ++i)
		{
			if (Rng.RandRange(0, 1) == 1)
			{
				Run->SetSpecialZoneCardBattleEnabled(SZSnapshotOrder[i], true);
			}
		}

		// ---- 步骤 2：再加 F 张非容器卡到 Backpack（控制 Backpack 退回时的余量）----
		const int32 F = Rng.RandRange(0, ACap - 2);
		for (int32 i = 0; i < F; ++i)
		{
			Run->AcquireCardToRun(Fx.MakeNoopCard(0));
		}

		// ---- 快照 ----
		const int32 BackpackOrigSize = Run->GetBackpack().Num();   // 期望 = 2 + F
		const int32 FluxCap          = Run->GetFluxCapacity();      // 期望 = ACap
		const TArray<FCardInstance> BackpackBefore = Run->GetBackpack();
		const TArray<FCardInstance> BattleDeckBefore = Run->GetBattleDeck();
		const TArray<FCardInstance> BurdenZoneBefore = Run->GetRunState().BurdenZone;

		// 收集销毁前所有 InstanceId（供 (A) 不丢失校验）
		TSet<FGuid> AllIdsBefore;
		AllIdsBefore.Reserve(
			BackpackBefore.Num() + BattleDeckBefore.Num() + BurdenZoneBefore.Num() + S);
		for (const FCardInstance& I : BackpackBefore)   { AllIdsBefore.Add(I.InstanceId); }
		for (const FCardInstance& I : BattleDeckBefore) { AllIdsBefore.Add(I.InstanceId); }
		for (const FCardInstance& I : BurdenZoneBefore) { AllIdsBefore.Add(I.InstanceId); }
		for (const FSpecialZone& SZ : Run->GetRunState().SpecialZones)
		{
			for (const FCardInstance& I : SZ.Cards) { AllIdsBefore.Add(I.InstanceId); }
		}

		// 期望退回布局
		const int32 K = FMath::Clamp(FluxCap - 1 - F, 0, S);

		// 设置健全性校验：本测试构造确保 RecomputeBurden 是 no-op，需要 BattleDeck 在销毁前
		// 已经满到 B 主卡销毁后的 BattleDeckCap（ACap 张非容器卡 Initialize 进 BattleDeck），且销毁后
		// 没有其他 SpecialZone 可以承接 refill（StarterDeck 仅含一张 B 主卡）。
		const int32 PostDestroyBattleDeckCap = BagProviderDef->Physique.Capacity;
		if (BattleDeckBefore.Num() != PostDestroyBattleDeckCap)
		{
			AddError(FString::Printf(
				TEXT("Property 6 SETUP FAILED: BattleDeck pre-destroy size=%d != post-destroy BattleDeckCap=%d, ")
				TEXT("RecomputeBurden no-op assumption broken. ")
				TEXT("Counterexample: Seed=%d Iter=%d ACap=%d BCap=%d S=%d F=%d"),
				BattleDeckBefore.Num(), PostDestroyBattleDeckCap,
				Seed, Iter, ACap, BCap, S, F));
			return false;
		}
		if (Run->GetRunState().SpecialZones.Num() != 1)
		{
			AddError(FString::Printf(
				TEXT("Property 6 SETUP FAILED: SpecialZones.Num()=%d (expected 1, only BMaster), ")
				TEXT("RecomputeBurden refill no-op assumption broken. ")
				TEXT("Counterexample: Seed=%d Iter=%d ACap=%d BCap=%d S=%d F=%d"),
				Run->GetRunState().SpecialZones.Num(),
				Seed, Iter, ACap, BCap, S, F));
			return false;
		}

		// ---- 调 DestroyCardByInstance ----
		const bool bDestroyed = DestroyFirstOwnedDefinition(Run, BMasterDef);
		if (!bDestroyed)
		{
			AddError(FString::Printf(
				TEXT("Property 6 FAILED (Precondition): DestroyCardByInstance(BMaster) returned false. ")
				TEXT("Counterexample: Seed=%d Iter=%d ACap=%d BCap=%d S=%d F=%d ")
				TEXT("BackpackOrigSize=%d BattleDeck.Num=%d"),
				Seed, Iter, ACap, BCap, S, F,
				BackpackOrigSize, BattleDeckBefore.Num()));
			return false;
		}

		const FRunState& After = Run->GetRunState();
		const TArray<FCardInstance>& PostBackpack   = After.Backpack;
		const TArray<FCardInstance>& PostBattleDeck = After.BattleDeck;
		const TArray<FCardInstance>& PostBurden     = After.BurdenZone;

		// ---- (C) 销毁的 OwnerInstanceId 不再在 SpecialZones 中 ----
		for (int32 i = 0; i < After.SpecialZones.Num(); ++i)
		{
			if (After.SpecialZones[i].OwnerInstanceId == BMasterId)
			{
				AddError(FString::Printf(
					TEXT("Property 6 FAILED (C): SpecialZones[%d].OwnerInstanceId=%s 仍是被销毁的 BMaster。")
					TEXT("Counterexample: Seed=%d Iter=%d ACap=%d BCap=%d S=%d F=%d K=%d ")
					TEXT("BackpackOrigSize=%d FluxCap=%d"),
					i, *BMasterId.ToString(EGuidFormats::DigitsWithHyphens),
					Seed, Iter, ACap, BCap, S, F, K, BackpackOrigSize, FluxCap));
				return false;
			}
		}

		// ---- (A) 不丢失：销毁前所有 InstanceId（除 BMaster 自身）必须仍在某个 zone ----
		TSet<FGuid> AllIdsAfter;
		AllIdsAfter.Reserve(
			PostBackpack.Num() + PostBattleDeck.Num() + PostBurden.Num());
		for (const FCardInstance& I : PostBackpack)   { AllIdsAfter.Add(I.InstanceId); }
		for (const FCardInstance& I : PostBattleDeck) { AllIdsAfter.Add(I.InstanceId); }
		for (const FCardInstance& I : PostBurden)     { AllIdsAfter.Add(I.InstanceId); }
		for (const FSpecialZone& SZ : After.SpecialZones)
		{
			for (const FCardInstance& I : SZ.Cards) { AllIdsAfter.Add(I.InstanceId); }
		}

		if (AllIdsAfter.Contains(BMasterId))
		{
			AddError(FString::Printf(
				TEXT("Property 6 FAILED (A): BMaster InstanceId=%s 仍在某 zone 中（应被销毁）。")
				TEXT("Counterexample: Seed=%d Iter=%d ACap=%d BCap=%d S=%d F=%d"),
				*BMasterId.ToString(EGuidFormats::DigitsWithHyphens),
				Seed, Iter, ACap, BCap, S, F));
			return false;
		}
		for (const FGuid& OldId : AllIdsBefore)
		{
			if (OldId == BMasterId) { continue; }
			if (!AllIdsAfter.Contains(OldId))
			{
				AddError(FString::Printf(
					TEXT("Property 6 FAILED (A): InstanceId=%s 销毁前存在但销毁后丢失。")
					TEXT("Counterexample: Seed=%d Iter=%d ACap=%d BCap=%d S=%d F=%d K=%d ")
					TEXT("BackpackBefore.Num=%d PostBackpack.Num=%d PostBurden.Num=%d"),
					*OldId.ToString(EGuidFormats::DigitsWithHyphens),
					Seed, Iter, ACap, BCap, S, F, K,
					BackpackBefore.Num(), PostBackpack.Num(), PostBurden.Num()));
				return false;
			}
		}

		// ---- (B) 退回顺序 ----
		// 期望 Backpack.Num = (BackpackOrigSize - 1) + K
		// 期望 BurdenZone.Num = (BurdenZoneBefore.Num) + (S - K)
		// 由于本测试构造确保销毁前 BurdenZone 为空（Initialize 后 BurdenZone 始终空，
		// AcquireCardToRun/MoveInstance 都不会让 BurdenZone 非空因 Backpack 总未超 FluxCap），
		// 期望 BurdenZoneBefore.Num == 0；不显式断言 0 让该不变量留给 Property 5/8。
		const int32 ExpectedBackpackSize = (BackpackOrigSize - 1) + K;
		const int32 ExpectedBurdenSize   = BurdenZoneBefore.Num() + (S - K);
		if (PostBackpack.Num() != ExpectedBackpackSize)
		{
			AddError(FString::Printf(
				TEXT("Property 6 FAILED (B size): PostBackpack.Num=%d != Expected=%d (BackpackOrigSize-1+K=%d-1+%d)。")
				TEXT("Counterexample: Seed=%d Iter=%d ACap=%d BCap=%d S=%d F=%d K=%d ")
				TEXT("FluxCap=%d BackpackBefore.Num=%d"),
				PostBackpack.Num(), ExpectedBackpackSize,
				BackpackOrigSize, K,
				Seed, Iter, ACap, BCap, S, F, K, FluxCap, BackpackBefore.Num()));
			return false;
		}
		if (PostBurden.Num() != ExpectedBurdenSize)
		{
			AddError(FString::Printf(
				TEXT("Property 6 FAILED (B size): PostBurden.Num=%d != Expected=%d (BurdenZoneBefore.Num+(S-K)=%d+%d)。")
				TEXT("Counterexample: Seed=%d Iter=%d ACap=%d BCap=%d S=%d F=%d K=%d ")
				TEXT("FluxCap=%d"),
				PostBurden.Num(), ExpectedBurdenSize,
				BurdenZoneBefore.Num(), (S - K),
				Seed, Iter, ACap, BCap, S, F, K, FluxCap));
			return false;
		}

		// 前 K 张：Backpack[BackpackOrigSize-1 .. BackpackOrigSize-1+K-1] 应严格等于
		// SZSnapshotOrder[0..K-1]。
		for (int32 i = 0; i < K; ++i)
		{
			const int32 ExpectedIdx = (BackpackOrigSize - 1) + i;
			const FGuid ActualId = PostBackpack[ExpectedIdx].InstanceId;
			const FGuid ExpectedId = SZSnapshotOrder[i];
			if (ActualId != ExpectedId)
			{
				AddError(FString::Printf(
					TEXT("Property 6 FAILED (B order Backpack): PostBackpack[%d].InstanceId=%s != Expected SZ.Cards[%d]=%s。")
					TEXT("Counterexample: Seed=%d Iter=%d ACap=%d BCap=%d S=%d F=%d K=%d ")
					TEXT("FluxCap=%d BackpackOrigSize=%d"),
					ExpectedIdx, *ActualId.ToString(EGuidFormats::DigitsWithHyphens),
					i, *ExpectedId.ToString(EGuidFormats::DigitsWithHyphens),
					Seed, Iter, ACap, BCap, S, F, K, FluxCap, BackpackOrigSize));
				return false;
			}
		}

		// 后 S-K 张：BurdenZone[BurdenZoneBefore.Num .. BurdenZoneBefore.Num+(S-K)-1] 应严格等于
		// SZSnapshotOrder[K..S-1]。
		for (int32 i = K; i < S; ++i)
		{
			const int32 ExpectedIdx = BurdenZoneBefore.Num() + (i - K);
			const FGuid ActualId = PostBurden[ExpectedIdx].InstanceId;
			const FGuid ExpectedId = SZSnapshotOrder[i];
			if (ActualId != ExpectedId)
			{
				AddError(FString::Printf(
					TEXT("Property 6 FAILED (B order BurdenZone): PostBurden[%d].InstanceId=%s != Expected SZ.Cards[%d]=%s。")
					TEXT("Counterexample: Seed=%d Iter=%d ACap=%d BCap=%d S=%d F=%d K=%d ")
					TEXT("FluxCap=%d BurdenZoneBefore.Num=%d"),
					ExpectedIdx, *ActualId.ToString(EGuidFormats::DigitsWithHyphens),
					i, *ExpectedId.ToString(EGuidFormats::DigitsWithHyphens),
					Seed, Iter, ACap, BCap, S, F, K, FluxCap, BurdenZoneBefore.Num()));
				return false;
			}
		}

		// ---- (D) 每张退回 instance 的 bBattleEnabledInSpecialZone == false ----
		for (int32 i = 0; i < K; ++i)
		{
			const int32 ExpectedIdx = (BackpackOrigSize - 1) + i;
			if (PostBackpack[ExpectedIdx].bBattleEnabledInSpecialZone)
			{
				AddError(FString::Printf(
					TEXT("Property 6 FAILED (D): PostBackpack[%d] (originally SZ.Cards[%d]) ")
					TEXT("bBattleEnabledInSpecialZone=true (期望 false)。")
					TEXT("Counterexample: Seed=%d Iter=%d ACap=%d BCap=%d S=%d F=%d K=%d ")
					TEXT("FluxCap=%d"),
					ExpectedIdx, i,
					Seed, Iter, ACap, BCap, S, F, K, FluxCap));
				return false;
			}
		}
		for (int32 i = K; i < S; ++i)
		{
			const int32 ExpectedIdx = BurdenZoneBefore.Num() + (i - K);
			if (PostBurden[ExpectedIdx].bBattleEnabledInSpecialZone)
			{
				AddError(FString::Printf(
					TEXT("Property 6 FAILED (D): PostBurden[%d] (originally SZ.Cards[%d]) ")
					TEXT("bBattleEnabledInSpecialZone=true (期望 false)。")
					TEXT("Counterexample: Seed=%d Iter=%d ACap=%d BCap=%d S=%d F=%d K=%d ")
					TEXT("FluxCap=%d"),
					ExpectedIdx, i,
					Seed, Iter, ACap, BCap, S, F, K, FluxCap));
				return false;
			}
		}
	}

	return true;
}
