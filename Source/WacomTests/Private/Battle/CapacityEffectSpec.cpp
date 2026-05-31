// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"

#include "Commands/BattleCommand.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Characters/CharacterDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Events/RunEventDefinition.h"
#include "Shops/ShopDefinition.h"
#include "Tags/WacomGameplayTags.h"

#include "UObject/StrongObjectPtr.h"

namespace
{
	TStrongObjectPtr<UBattleSession> CreateCapacityEffectSession(
		FWacomBattleFixture& Fx,
		UCardDefinition* Card,
		const FGameplayTagContainer& CapacityEffectTags)
	{
		UCharacterDefinition* Character = Fx.MakeCharacter(
			Fx.MakeNoopCard(0),
			Fx.MakeNoopCard(0),
			{});
		UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(/*Hp*/20, /*Initiative*/10, /*IntentResist*/0);

		TStrongObjectPtr<UBattleSession> Session(NewObject<UBattleSession>());
		FBattleInitParams Params;
		Params.Character = Character;
		Params.Enemy = Enemy;
		Params.RandomSeed = 11;

		FBattleDeckEntry Entry;
		Entry.Definition = Card;
		Entry.CapacityEffectTags = CapacityEffectTags;
		Params.BattleDeckEntries.Add(Entry);

		const FWacomStatus Status = Session->Initialize(Params);
		check(Status.IsOk());
		return Session;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCapacityEffectTagsValidSpec,
	"Wacom.Battle.CapacityEffect.TagsValid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCapacityEffectTagsValidSpec::RunTest(const FString& /*Parameters*/)
{
	const FGameplayTag CapacityTag = WacomTags::Card_CapacityEffect_WeaponDamagePlus3;
	const FGameplayTag WeaponTag = WacomTags::Card_Keyword_Weapon;
	TestTrue(TEXT("WeaponDamagePlus3 tag valid"),
		CapacityTag.IsValid());
	TestTrue(TEXT("Weapon keyword tag valid"),
		WeaponTag.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCapacityEffectBugGirlCocoonAssetSpec,
	"Wacom.Battle.CapacityEffect.BugGirlCocoonAsset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCapacityEffectBugGirlCocoonAssetSpec::RunTest(const FString& /*Parameters*/)
{
	// 蛛茧绒囊落盘资产必须使用首个具体容量效果，而不是早期 Placeholder。
	UCardDefinition* Cocoon = LoadObject<UCardDefinition>(
		nullptr,
		TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_ZhujianRongnang.DA_Card_ZhujianRongnang"));

	if (!TestNotNull(TEXT("BugGirl cocoon card asset loads"), Cocoon))
	{
		return false;
	}

	const FGameplayTag ExpectedCapacityEffect = WacomTags::Card_CapacityEffect_WeaponDamagePlus3;
	TestEqual(TEXT("ZhujianRongnang CapacityEffect == WeaponDamagePlus3"),
		Cocoon->Physique.CapacityEffect,
		ExpectedCapacityEffect);
	TestEqual(TEXT("ZhujianRongnang SpecialZone capacity source = 3"),
		Cocoon->Physique.Capacity,
		3);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleKnockdownRewardPoisonFangAssetSpec,
	"Wacom.Battle.KnockdownReward.PoisonFangAsset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleKnockdownRewardPoisonFangAssetSpec::RunTest(const FString& /*Parameters*/)
{
	UCardDefinition* PoisonFang = LoadObject<UCardDefinition>(
		nullptr,
		TEXT("/Game/Wacom/Data/Cards/Rewards/DA_Card_PoisonFang.DA_Card_PoisonFang"));

	if (!TestNotNull(TEXT("PoisonFang card asset loads"), PoisonFang))
	{
		return false;
	}

	TestEqual(TEXT("PoisonFang CardId"), PoisonFang->CardId, FName(TEXT("PoisonFang")));
	TestEqual(TEXT("PoisonFang DisplayName"), PoisonFang->DisplayName.ToString(), FString(TEXT("毒牙")));
	TestEqual(TEXT("PoisonFang BaseCost"), PoisonFang->BaseCost, 0);
	TestEqual(TEXT("PoisonFang TargetMode"), PoisonFang->TargetMode, ECardTargetMode::SingleEnemyPart);
	TestEqual(TEXT("PoisonFang has one effect"), PoisonFang->Effects.Num(), 1);
	if (PoisonFang->Effects.IsValidIndex(0))
	{
		const FCardEffect& Effect = PoisonFang->Effects[0];
		TestEqual(TEXT("PoisonFang effect type is poison"),
			Effect.EffectType,
			FGameplayTag(WacomTags::Effect_ApplyStatus_Poison));
		TestEqual(TEXT("PoisonFang applies one poison stack"), Effect.Magnitude, 1);
		TestEqual(TEXT("PoisonFang targets one enemy part"),
			Effect.Target,
			FGameplayTag(WacomTags::Target_SingleEnemyPart));
	}

	const TCHAR* PartPaths[] =
	{
		TEXT("/Game/Wacom/Data/Enemies/Snake/DA_Part_Snake_Head.DA_Part_Snake_Head"),
		TEXT("/Game/Wacom/Data/Enemies/Snake/DA_Part_Snake_Body.DA_Part_Snake_Body"),
		TEXT("/Game/Wacom/Data/Enemies/Snake/DA_Part_Snake_Tail.DA_Part_Snake_Tail"),
	};

	for (const TCHAR* PartPath : PartPaths)
	{
		UEnemyPartDefinition* Part = LoadObject<UEnemyPartDefinition>(nullptr, PartPath);
		if (!TestNotNull(FString::Printf(TEXT("Snake part asset loads: %s"), PartPath), Part))
		{
			return false;
		}

		TestEqual(
			FString::Printf(TEXT("Snake part reward is PoisonFang: %s"), PartPath),
			Part->KnockdownRewardCard.Get(),
			PoisonFang);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataShopDebugSnakeAssetSpec,
	"Wacom.Data.Shop.DebugSnakeAsset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataShopDebugSnakeAssetSpec::RunTest(const FString& /*Parameters*/)
{
	UShopDefinition* DebugShop = LoadObject<UShopDefinition>(
		nullptr,
		TEXT("/Game/Wacom/Data/Shops/DA_Shop_DebugSnake.DA_Shop_DebugSnake"));

	if (!TestNotNull(TEXT("DebugSnake shop asset loads"), DebugShop))
	{
		return false;
	}

	TestEqual(TEXT("DebugSnake ShopId"), DebugShop->ShopId, FName(TEXT("Shop.DebugSnake")));
	TestEqual(TEXT("DebugSnake DisplayName"), DebugShop->DisplayName.ToString(), FString(TEXT("蛇巢调试商店")));
	TestEqual(TEXT("DebugSnake offer count"), DebugShop->Offers.Num(), 4);

	struct FExpectedOffer
	{
		const TCHAR* ObjectPath;
		int32 Price;
	};
	const FExpectedOffer ExpectedOffers[] =
	{
		{ TEXT("/Game/Wacom/Data/Cards/Rewards/DA_Card_PoisonFang.DA_Card_PoisonFang"), 0 },
		{ TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_ChifuGongyi.DA_Card_ChifuGongyi"), 2 },
		{ TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_ZhaoguangMudie.DA_Card_ZhaoguangMudie"), 2 },
		{ TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_BugGirlBag.DA_Card_BugGirlBag"), 3 },
	};

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(ExpectedOffers); ++Index)
	{
		if (!DebugShop->Offers.IsValidIndex(Index))
		{
			return false;
		}

		UCardDefinition* ExpectedCard = LoadObject<UCardDefinition>(nullptr, ExpectedOffers[Index].ObjectPath);
		if (!TestNotNull(FString::Printf(TEXT("Offer card asset loads %d"), Index), ExpectedCard))
		{
			return false;
		}

		TestEqual(FString::Printf(TEXT("Offer card %d"), Index),
			DebugShop->Offers[Index].CardDefinition.Get(),
			ExpectedCard);
		TestEqual(FString::Printf(TEXT("Offer price %d"), Index),
			DebugShop->Offers[Index].Price,
			ExpectedOffers[Index].Price);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataRunEventDebugSnakeGiftAssetSpec,
	"Wacom.Data.RunEvent.DebugSnakeGiftAsset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataRunEventDebugSnakeGiftAssetSpec::RunTest(const FString& /*Parameters*/)
{
	UWacomRunEventDefinition* Event = LoadObject<UWacomRunEventDefinition>(
		nullptr,
		TEXT("/Game/Wacom/Data/Events/DA_Event_DebugSnakeGift.DA_Event_DebugSnakeGift"));

	if (!TestNotNull(TEXT("DebugSnakeGift event asset loads"), Event))
	{
		return false;
	}

	TestEqual(TEXT("EventId"), Event->EventId, FName(TEXT("Event.DebugSnakeGift")));
	TestEqual(TEXT("DisplayName"), Event->DisplayName.ToString(), FString(TEXT("蛇巢遗物")));
	TestEqual(TEXT("StartNodeId"), Event->StartNodeId, FName(TEXT("Start")));

	const FWacomRunEventNodeDefinition* StartNode = Event->Nodes.FindByPredicate(
		[](const FWacomRunEventNodeDefinition& Node)
		{
			return Node.NodeId == TEXT("Start");
		});
	const FWacomRunEventNodeDefinition* EndNode = Event->Nodes.FindByPredicate(
		[](const FWacomRunEventNodeDefinition& Node)
		{
			return Node.NodeId == TEXT("End");
		});
	if (!TestNotNull(TEXT("Start node exists"), StartNode)
		|| !TestNotNull(TEXT("End node exists"), EndNode))
	{
		return false;
	}

	const FWacomRunEventChoiceDefinition* TakeGift = StartNode->Choices.FindByPredicate(
		[](const FWacomRunEventChoiceDefinition& Choice)
		{
			return Choice.ChoiceId == TEXT("TakeGift");
		});
	const FWacomRunEventChoiceDefinition* PayRespect = StartNode->Choices.FindByPredicate(
		[](const FWacomRunEventChoiceDefinition& Choice)
		{
			return Choice.ChoiceId == TEXT("PayRespect");
		});
	const FWacomRunEventChoiceDefinition* HandOverFang = StartNode->Choices.FindByPredicate(
		[](const FWacomRunEventChoiceDefinition& Choice)
		{
			return Choice.ChoiceId == TEXT("HandOverFang");
		});
	const FWacomRunEventChoiceDefinition* Leave = StartNode->Choices.FindByPredicate(
		[](const FWacomRunEventChoiceDefinition& Choice)
		{
			return Choice.ChoiceId == TEXT("Leave");
		});
	const FWacomRunEventChoiceDefinition* Close = EndNode->Choices.FindByPredicate(
		[](const FWacomRunEventChoiceDefinition& Choice)
		{
			return Choice.ChoiceId == TEXT("Close");
		});

	if (!TestNotNull(TEXT("TakeGift choice exists"), TakeGift)
		|| !TestNotNull(TEXT("PayRespect choice exists"), PayRespect)
		|| !TestNotNull(TEXT("HandOverFang choice exists"), HandOverFang)
		|| !TestNotNull(TEXT("Leave choice exists"), Leave)
		|| !TestNotNull(TEXT("Close choice exists"), Close))
	{
		return false;
	}

	UCardDefinition* PoisonFang = LoadObject<UCardDefinition>(
		nullptr,
		TEXT("/Game/Wacom/Data/Cards/Rewards/DA_Card_PoisonFang.DA_Card_PoisonFang"));
	if (!TestNotNull(TEXT("PoisonFang card asset loads"), PoisonFang))
	{
		return false;
	}

	TestTrue(TEXT("TakeGift gains PoisonFang"),
		TakeGift->Effects.ContainsByPredicate([PoisonFang](const FWacomRunEventEffectDefinition& Effect)
		{
			return Effect.Type == EWacomRunEventEffectType::GainCard
				&& Effect.CardDefinition.Get() == PoisonFang;
		}));
	TestTrue(TEXT("TakeGift consumes one node"),
		TakeGift->Effects.ContainsByPredicate([](const FWacomRunEventEffectDefinition& Effect)
		{
			return Effect.Type == EWacomRunEventEffectType::ConsumeNode
				&& Effect.Value == 1;
		}));

	TestTrue(TEXT("PayRespect requires one gold"),
		PayRespect->Conditions.ContainsByPredicate([](const FWacomRunEventConditionDefinition& Condition)
		{
			return Condition.Type == EWacomRunEventConditionType::MinGold
				&& Condition.Value == 1;
		}));
	TestTrue(TEXT("PayRespect removes one gold"),
		PayRespect->Effects.ContainsByPredicate([](const FWacomRunEventEffectDefinition& Effect)
		{
			return Effect.Type == EWacomRunEventEffectType::AddGold
				&& Effect.Value == -1;
		}));
	TestTrue(TEXT("PayRespect reduces Misdeed pressure"),
		PayRespect->Effects.ContainsByPredicate([](const FWacomRunEventEffectDefinition& Effect)
		{
			return Effect.Type == EWacomRunEventEffectType::AddPressure
				&& Effect.PressureType == TEXT("Misdeed")
				&& Effect.Value == -1;
		}));
	TestTrue(TEXT("PayRespect consumes one node"),
		PayRespect->Effects.ContainsByPredicate([](const FWacomRunEventEffectDefinition& Effect)
		{
			return Effect.Type == EWacomRunEventEffectType::ConsumeNode
				&& Effect.Value == 1;
		}));

	TestTrue(TEXT("HandOverFang requires PoisonFang"),
		HandOverFang->Conditions.ContainsByPredicate([PoisonFang](const FWacomRunEventConditionDefinition& Condition)
		{
			return Condition.Type == EWacomRunEventConditionType::HasCard
				&& Condition.CardDefinition.Get() == PoisonFang;
		}));
	TestTrue(TEXT("HandOverFang requires card payment"),
		HandOverFang->CardPayment.bRequiresOwnedCardPayment);
	TestTrue(TEXT("HandOverFang payment accepts PoisonFang"),
		HandOverFang->CardPayment.AllowedCardDefinitions.Contains(PoisonFang));
	TestEqual(TEXT("HandOverFang payment zone"),
		HandOverFang->CardPayment.PaymentZoneId,
		FName(TEXT("RunEvent.Pay.Fang")));
	TestFalse(TEXT("HandOverFang does not also remove PoisonFang by effect"),
		HandOverFang->Effects.ContainsByPredicate([](const FWacomRunEventEffectDefinition& Effect)
		{
			return Effect.Type == EWacomRunEventEffectType::RemoveCard;
		}));
	TestTrue(TEXT("HandOverFang consumes one node"),
		HandOverFang->Effects.ContainsByPredicate([](const FWacomRunEventEffectDefinition& Effect)
		{
			return Effect.Type == EWacomRunEventEffectType::ConsumeNode
				&& Effect.Value == 1;
		}));

	TestTrue(TEXT("Leave closes event"), Leave->bCloseEventAfterResolve);
	TestTrue(TEXT("Close closes event"), Close->bCloseEventAfterResolve);
	TestTrue(TEXT("Close marks event completed"), Close->bMarkEventCompleted);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataRunEventDebugFlagRewardAssetSpec,
	"Wacom.Data.RunEvent.DebugFlagRewardAsset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataRunEventDebugFlagRewardAssetSpec::RunTest(const FString& /*Parameters*/)
{
	const FName InspectedFlag = TEXT("DebugFlagReward.Inspected");
	const FName GoldGrantedFlag = TEXT("DebugFlagReward.GoldGranted");
	const FName RewardClaimedFlag = TEXT("DebugFlagReward.RewardClaimed");

	UWacomRunEventDefinition* Event = LoadObject<UWacomRunEventDefinition>(
		nullptr,
		TEXT("/Game/Wacom/Data/Events/DA_Event_DebugFlagReward.DA_Event_DebugFlagReward"));

	if (!TestNotNull(TEXT("DebugFlagReward event asset loads"), Event))
	{
		return false;
	}

	TestEqual(TEXT("EventId"), Event->EventId, FName(TEXT("Event.DebugFlagReward")));
	TestEqual(TEXT("DisplayName"), Event->DisplayName.ToString(), FString(TEXT("标记奖励样例")));
	TestEqual(TEXT("StartNodeId"), Event->StartNodeId, FName(TEXT("Start")));

	const FWacomRunEventNodeDefinition* StartNode = Event->Nodes.FindByPredicate(
		[](const FWacomRunEventNodeDefinition& Node)
		{
			return Node.NodeId == TEXT("Start");
		});
	const FWacomRunEventNodeDefinition* RewardedNode = Event->Nodes.FindByPredicate(
		[](const FWacomRunEventNodeDefinition& Node)
		{
			return Node.NodeId == TEXT("Rewarded");
		});
	if (!TestNotNull(TEXT("Start node exists"), StartNode)
		|| !TestNotNull(TEXT("Rewarded node exists"), RewardedNode))
	{
		return false;
	}

	const FWacomRunEventChoiceDefinition* InspectMark = StartNode->Choices.FindByPredicate(
		[](const FWacomRunEventChoiceDefinition& Choice)
		{
			return Choice.ChoiceId == TEXT("InspectMark");
		});
	const FWacomRunEventChoiceDefinition* DebugGrantGold = StartNode->Choices.FindByPredicate(
		[](const FWacomRunEventChoiceDefinition& Choice)
		{
			return Choice.ChoiceId == TEXT("DebugGrantGold");
		});
	const FWacomRunEventChoiceDefinition* ClaimGoldReward = StartNode->Choices.FindByPredicate(
		[](const FWacomRunEventChoiceDefinition& Choice)
		{
			return Choice.ChoiceId == TEXT("ClaimGoldReward");
		});
	const FWacomRunEventChoiceDefinition* Leave = StartNode->Choices.FindByPredicate(
		[](const FWacomRunEventChoiceDefinition& Choice)
		{
			return Choice.ChoiceId == TEXT("Leave");
		});
	const FWacomRunEventChoiceDefinition* TryClaimAgain = RewardedNode->Choices.FindByPredicate(
		[](const FWacomRunEventChoiceDefinition& Choice)
		{
			return Choice.ChoiceId == TEXT("TryClaimAgain");
		});
	const FWacomRunEventChoiceDefinition* ResetFlags = RewardedNode->Choices.FindByPredicate(
		[](const FWacomRunEventChoiceDefinition& Choice)
		{
			return Choice.ChoiceId == TEXT("ResetFlags");
		});
	const FWacomRunEventChoiceDefinition* Close = RewardedNode->Choices.FindByPredicate(
		[](const FWacomRunEventChoiceDefinition& Choice)
		{
			return Choice.ChoiceId == TEXT("Close");
		});
	if (!TestNotNull(TEXT("InspectMark choice exists"), InspectMark)
		|| !TestNotNull(TEXT("DebugGrantGold choice exists"), DebugGrantGold)
		|| !TestNotNull(TEXT("ClaimGoldReward choice exists"), ClaimGoldReward)
		|| !TestNotNull(TEXT("Leave choice exists"), Leave)
		|| !TestNotNull(TEXT("TryClaimAgain choice exists"), TryClaimAgain)
		|| !TestNotNull(TEXT("ResetFlags choice exists"), ResetFlags)
		|| !TestNotNull(TEXT("Close choice exists"), Close))
	{
		return false;
	}

	UCardDefinition* PoisonFang = LoadObject<UCardDefinition>(
		nullptr,
		TEXT("/Game/Wacom/Data/Cards/Rewards/DA_Card_PoisonFang.DA_Card_PoisonFang"));
	if (!TestNotNull(TEXT("PoisonFang card asset loads"), PoisonFang))
	{
		return false;
	}

	TestTrue(TEXT("InspectMark requires missing inspected flag"),
		InspectMark->Conditions.ContainsByPredicate([InspectedFlag](const FWacomRunEventConditionDefinition& Condition)
		{
			return Condition.Type == EWacomRunEventConditionType::RunFlagNotSet
				&& Condition.FlagId == InspectedFlag;
		}));
	TestTrue(TEXT("InspectMark sets inspected flag"),
		InspectMark->Effects.ContainsByPredicate([InspectedFlag](const FWacomRunEventEffectDefinition& Effect)
		{
			return Effect.Type == EWacomRunEventEffectType::SetRunFlag
				&& Effect.FlagId == InspectedFlag;
		}));

	TestTrue(TEXT("DebugGrantGold requires missing gold-granted flag"),
		DebugGrantGold->Conditions.ContainsByPredicate([GoldGrantedFlag](const FWacomRunEventConditionDefinition& Condition)
		{
			return Condition.Type == EWacomRunEventConditionType::RunFlagNotSet
				&& Condition.FlagId == GoldGrantedFlag;
		}));
	TestTrue(TEXT("DebugGrantGold adds three gold"),
		DebugGrantGold->Effects.ContainsByPredicate([](const FWacomRunEventEffectDefinition& Effect)
		{
			return Effect.Type == EWacomRunEventEffectType::AddGold
				&& Effect.Value == 3;
		}));
	TestTrue(TEXT("DebugGrantGold sets gold-granted flag"),
		DebugGrantGold->Effects.ContainsByPredicate([GoldGrantedFlag](const FWacomRunEventEffectDefinition& Effect)
		{
			return Effect.Type == EWacomRunEventEffectType::SetRunFlag
				&& Effect.FlagId == GoldGrantedFlag;
		}));

	TestTrue(TEXT("ClaimGoldReward requires inspected flag"),
		ClaimGoldReward->Conditions.ContainsByPredicate([InspectedFlag](const FWacomRunEventConditionDefinition& Condition)
		{
			return Condition.Type == EWacomRunEventConditionType::RunFlagSet
				&& Condition.FlagId == InspectedFlag;
		}));
	TestTrue(TEXT("ClaimGoldReward requires missing reward flag"),
		ClaimGoldReward->Conditions.ContainsByPredicate([RewardClaimedFlag](const FWacomRunEventConditionDefinition& Condition)
		{
			return Condition.Type == EWacomRunEventConditionType::RunFlagNotSet
				&& Condition.FlagId == RewardClaimedFlag;
		}));
	TestTrue(TEXT("ClaimGoldReward requires three gold"),
		ClaimGoldReward->Conditions.ContainsByPredicate([](const FWacomRunEventConditionDefinition& Condition)
		{
			return Condition.Type == EWacomRunEventConditionType::MinGold
				&& Condition.Value == 3;
		}));
	TestTrue(TEXT("ClaimGoldReward removes three gold"),
		ClaimGoldReward->Effects.ContainsByPredicate([](const FWacomRunEventEffectDefinition& Effect)
		{
			return Effect.Type == EWacomRunEventEffectType::AddGold
				&& Effect.Value == -3;
		}));
	TestTrue(TEXT("ClaimGoldReward gains PoisonFang"),
		ClaimGoldReward->Effects.ContainsByPredicate([PoisonFang](const FWacomRunEventEffectDefinition& Effect)
		{
			return Effect.Type == EWacomRunEventEffectType::GainCard
				&& Effect.CardDefinition.Get() == PoisonFang;
		}));
	TestTrue(TEXT("ClaimGoldReward sets reward flag"),
		ClaimGoldReward->Effects.ContainsByPredicate([RewardClaimedFlag](const FWacomRunEventEffectDefinition& Effect)
		{
			return Effect.Type == EWacomRunEventEffectType::SetRunFlag
				&& Effect.FlagId == RewardClaimedFlag;
		}));
	TestEqual(TEXT("ClaimGoldReward goes to rewarded node"),
		ClaimGoldReward->NextNodeId,
		FName(TEXT("Rewarded")));

	TestTrue(TEXT("TryClaimAgain is blocked by claimed flag"),
		TryClaimAgain->Conditions.ContainsByPredicate([RewardClaimedFlag](const FWacomRunEventConditionDefinition& Condition)
		{
			return Condition.Type == EWacomRunEventConditionType::RunFlagNotSet
				&& Condition.FlagId == RewardClaimedFlag;
		}));
	TestTrue(TEXT("TryClaimAgain still documents gold gate"),
		TryClaimAgain->Conditions.ContainsByPredicate([](const FWacomRunEventConditionDefinition& Condition)
		{
			return Condition.Type == EWacomRunEventConditionType::MinGold
				&& Condition.Value == 3;
		}));

	TestTrue(TEXT("ResetFlags requires reward flag"),
		ResetFlags->Conditions.ContainsByPredicate([RewardClaimedFlag](const FWacomRunEventConditionDefinition& Condition)
		{
			return Condition.Type == EWacomRunEventConditionType::RunFlagSet
				&& Condition.FlagId == RewardClaimedFlag;
		}));
	TestTrue(TEXT("ResetFlags clears inspected flag"),
		ResetFlags->Effects.ContainsByPredicate([InspectedFlag](const FWacomRunEventEffectDefinition& Effect)
		{
			return Effect.Type == EWacomRunEventEffectType::ClearRunFlag
				&& Effect.FlagId == InspectedFlag;
		}));
	TestTrue(TEXT("ResetFlags clears gold-granted flag"),
		ResetFlags->Effects.ContainsByPredicate([GoldGrantedFlag](const FWacomRunEventEffectDefinition& Effect)
		{
			return Effect.Type == EWacomRunEventEffectType::ClearRunFlag
				&& Effect.FlagId == GoldGrantedFlag;
		}));
	TestTrue(TEXT("ResetFlags clears reward flag"),
		ResetFlags->Effects.ContainsByPredicate([RewardClaimedFlag](const FWacomRunEventEffectDefinition& Effect)
		{
			return Effect.Type == EWacomRunEventEffectType::ClearRunFlag
				&& Effect.FlagId == RewardClaimedFlag;
		}));
	TestEqual(TEXT("ResetFlags returns to start"),
		ResetFlags->NextNodeId,
		FName(TEXT("Start")));

	TestTrue(TEXT("Leave closes event without completing"), Leave->bCloseEventAfterResolve);
	TestFalse(TEXT("Leave does not mark completed"), Leave->bMarkEventCompleted);
	TestTrue(TEXT("Close closes event without completing"), Close->bCloseEventAfterResolve);
	TestFalse(TEXT("Close does not mark completed"), Close->bMarkEventCompleted);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCapacityEffectWeaponDamagePlus3Spec,
	"Wacom.Battle.CapacityEffect.WeaponDamagePlus3AppliesToWeapon",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCapacityEffectWeaponDamagePlus3Spec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Weapon = Fx.MakeDamageCardWithKeywords(
		/*Cost*/1,
		/*Damage*/4,
		{ WacomTags::Card_Keyword_Weapon });

	FGameplayTagContainer Tags;
	Tags.AddTag(WacomTags::Card_CapacityEffect_WeaponDamagePlus3);
	TStrongObjectPtr<UBattleSession> Session = CreateCapacityEffectSession(Fx, Weapon, Tags);

	FBattleSnapshot Before = Session->BuildSnapshot();
	const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Before, Weapon->CardId);
	const FGuid TargetId = FWacomBattleFixture::FindPartInstanceId(Before, 0);
	TestTrue(TEXT("Weapon card in hand"), CardId.IsValid());
	TestTrue(TEXT("Target valid"), TargetId.IsValid());

	TestTrue(TEXT("Play weapon"), Session->SubmitCommand(FBattleCommand::MakePlayCard(CardId, TargetId)).IsOk());
	const FBattleSnapshot After = Session->BuildSnapshot();
	TestEqual(TEXT("Damage 4 + WeaponDamagePlus3 = 7"),
		FWacomBattleFixture::FindPartHp(After, 0), 13);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCapacityEffectNonWeaponNoBonusSpec,
	"Wacom.Battle.CapacityEffect.WeaponDamagePlus3IgnoresNonWeapon",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCapacityEffectNonWeaponNoBonusSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* NonWeapon = Fx.MakeSimpleDamageCard(/*Cost*/1, /*Damage*/4);

	FGameplayTagContainer Tags;
	Tags.AddTag(WacomTags::Card_CapacityEffect_WeaponDamagePlus3);
	TStrongObjectPtr<UBattleSession> Session = CreateCapacityEffectSession(Fx, NonWeapon, Tags);

	FBattleSnapshot Before = Session->BuildSnapshot();
	const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Before, NonWeapon->CardId);
	const FGuid TargetId = FWacomBattleFixture::FindPartInstanceId(Before, 0);
	TestTrue(TEXT("Non-weapon card in hand"), CardId.IsValid());
	TestTrue(TEXT("Target valid"), TargetId.IsValid());

	TestTrue(TEXT("Play non-weapon"), Session->SubmitCommand(FBattleCommand::MakePlayCard(CardId, TargetId)).IsOk());
	const FBattleSnapshot After = Session->BuildSnapshot();
	TestEqual(TEXT("Damage remains 4 without Weapon keyword"),
		FWacomBattleFixture::FindPartHp(After, 0), 16);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCapacityEffectWeaponWithoutTagNoBonusSpec,
	"Wacom.Battle.CapacityEffect.WeaponDamagePlus3RequiresTag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCapacityEffectWeaponWithoutTagNoBonusSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Weapon = Fx.MakeDamageCardWithKeywords(
		/*Cost*/1,
		/*Damage*/4,
		{ WacomTags::Card_Keyword_Weapon });

	FGameplayTagContainer EmptyTags;
	TStrongObjectPtr<UBattleSession> Session = CreateCapacityEffectSession(Fx, Weapon, EmptyTags);

	FBattleSnapshot Before = Session->BuildSnapshot();
	const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Before, Weapon->CardId);
	const FGuid TargetId = FWacomBattleFixture::FindPartInstanceId(Before, 0);
	TestTrue(TEXT("Weapon card in hand"), CardId.IsValid());
	TestTrue(TEXT("Target valid"), TargetId.IsValid());

	TestTrue(TEXT("Play weapon without capacity tag"), Session->SubmitCommand(FBattleCommand::MakePlayCard(CardId, TargetId)).IsOk());
	const FBattleSnapshot After = Session->BuildSnapshot();
	TestEqual(TEXT("Damage remains 4 without WeaponDamagePlus3 tag"),
		FWacomBattleFixture::FindPartHp(After, 0), 16);

	return true;
}
