// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/FireWriteCardContentBuilder.h"

#include "Cards/CardDefinition.h"
#include "ContentBuilders/ContentBuilderHelpers.h"
#include "ContentBuilders/FormalProductionContentSeedService.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "ObjectTools.h"
#include "Shops/ShopDefinition.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Battle/WacomBattleStatusPresentationCatalog.h"
#include "UI/Card/WacomCardExplanationLexicon.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "Validation/CardDefinitionValidation.h"

namespace Wacom::ContentBuilder::FireWritePrivate
{
	const FString Root = TEXT("/Game/Wacom/Data/Cards/FireWrite");
	const FString StatusCatalogPath =
		TEXT("/Game/Wacom/UI/Battle/Status/DA_BattleStatusPresentationCatalog");
	const FString LexiconPath =
		TEXT("/Game/Wacom/UI/Card/DA_CardExplanationLexicon_Default");
	const FString ShopPath = TEXT("/Game/Wacom/Data/Shops/DA_Shop_DebugSnake");
	const FString LegacyWhitePath =
		TEXT("/Game/Wacom/Data/Cards/Debug/ShopUpgrade/"
			"DA_Card_TestShopUpgrade_VenomProof_White");
	const FString LegacyBluePath =
		TEXT("/Game/Wacom/Data/Cards/Debug/ShopUpgrade/"
			"DA_Card_TestShopUpgrade_VenomProof_Blue");

	struct FSeedEntry
	{
		const TCHAR* EnglishName;
		const TCHAR* ChineseName;
	};

	const TArray<FSeedEntry>& SeedEntries()
	{
		static const TArray<FSeedEntry> Entries =
		{
			{ TEXT("OilCandle"), TEXT("虫油蜡烛") },
			{ TEXT("AshBug"), TEXT("灰烬虫") },
			{ TEXT("SaltMaggot"), TEXT("盐味熔蛆") },
			{ TEXT("WarmTinderbug"), TEXT("温热火绒虫") },
			{ TEXT("FireflySeed"), TEXT("流萤火种") },
			{ TEXT("HungryFireflyMaiden"), TEXT("饥饿的萤火侍女") },
			{ TEXT("BlazingEyeFirefly"), TEXT("灼眼·萤火虫") },
			{ TEXT("RottenFirefly"), TEXT("腐萤·萤火虫") },
			{ TEXT("GlimmerFirefly"), TEXT("微光·萤火虫") },
			{ TEXT("SlothFirefly"), TEXT("怠惰·萤火虫") },
			{ TEXT("EmptyBottle"), TEXT("空瓶") },
			{ TEXT("MoltenSalt"), TEXT("熔熔盐") },
			{ TEXT("JadeBeetle"), TEXT("翡翠甲虫") },
			{ TEXT("ObsidianBeetle"), TEXT("黑曜石甲虫") },
			{ TEXT("BlindSpider"), TEXT("盲眼蜘蛛") },
		};
		return Entries;
	}

	FString PackagePath(const FString& EnglishName)
	{
		return Root / (TEXT("DA_Card_") + EnglishName);
	}

	FString ObjectPath(const FString& Package)
	{
		return Wacom::ContentBuilder::MakeObjectPath(Package);
	}

	FCardEffect MakeEffect(
		const FGameplayTag EffectType,
		const int32 Magnitude,
		const FGameplayTag Target)
	{
		FCardEffect Effect;
		Effect.EffectType = EffectType;
		Effect.Magnitude = Magnitude;
		Effect.Target = Target;
		return Effect;
	}

	FCardEffect MakeDraw(const int32 Count)
	{
		FCardEffect Effect = MakeEffect(
			WacomTags::Effect_Draw,
			Count,
			WacomTags::Target_Player);
		Effect.TargetZone = WacomTags::CardLocation_Draw;
		return Effect;
	}

	FCardEffect MakeRuntimeEffect(
		const FGameplayTag EffectType,
		const int32 Magnitude,
		const FGameplayTag Target = WacomTags::Target_Self)
	{
		return MakeEffect(EffectType, Magnitude, Target);
	}

	FCardEffect MakeRuntimeEffectModifier(
		const FGameplayTag EffectType,
		const FGameplayTag AffectedEffectType,
		const int32 Magnitude,
		const FGameplayTag Target = WacomTags::Target_Self,
		const FGameplayTag RequiredTargetStatus = FGameplayTag())
	{
		FCardEffect Effect = MakeRuntimeEffect(
			EffectType,
			Magnitude,
			Target);
		Effect.AffectedEffectType = AffectedEffectType;
		Effect.RequiredTargetCardStatus = RequiredTargetStatus;
		return Effect;
	}

	FCardEffect MakeCreate(
		const FGameplayTag EffectType,
		const int32 Count,
		const TArray<UCardDefinition*>& Pool)
	{
		FCardEffect Effect = MakeRuntimeEffect(
			EffectType,
			Count,
			WacomTags::Target_Self);
		for (UCardDefinition* Card : Pool)
		{
			Effect.CardPool.Add(Card);
		}
		return Effect;
	}

	FCardPassive MakePassive(
		const FGameplayTag Trigger,
		const FString& DisplayText,
		const TArray<FCardEffect>& Effects,
		const FEffectCondition& Condition = FEffectCondition())
	{
		FCardPassive Passive;
		Passive.Trigger = Trigger;
		Passive.DisplayText = FText::FromString(DisplayText);
		Passive.Effects = Effects;
		Passive.Condition = Condition;
		return Passive;
	}

	FEffectCondition InCardLocation(const FGameplayTag Location)
	{
		FEffectCondition Condition;
		Condition.ConditionType = WacomTags::Condition_Self_InCardLocation;
		Condition.ParamTag = Location;
		return Condition;
	}

	FEffectCondition EverEnteredExhaust()
	{
		FEffectCondition Condition;
		Condition.ConditionType = WacomTags::Condition_Self_EverEnteredExhaust;
		return Condition;
	}

	FCardPhysique MakePhysique(
		const int32 MaxHp = 0,
		const int32 Durability = 0,
		const int32 Capacity = 0)
	{
		FCardPhysique Physique;
		Physique.MaxHpBonus = MaxHp;
		Physique.Durability = Durability;
		Physique.Capacity = Capacity;
		return Physique;
	}

	FGameplayTagContainer Keywords(
		std::initializer_list<FGameplayTag> Tags)
	{
		FGameplayTagContainer Result;
		for (const FGameplayTag Tag : Tags)
		{
			Result.AddTag(Tag);
		}
		return Result;
	}

	FText DefaultFireWriteEffectTemplate(const FCardEffect& Effect)
	{
		if (Effect.EffectType == WacomTags::Effect_Damage)
		{
			return FText::FromString(
				TEXT("{icon:EffectIcon} 造成 {value:Magnitude} 伤害。"));
		}
		if (Effect.EffectType == WacomTags::Effect_Heal)
		{
			return FText::FromString(
				TEXT("{icon:EffectIcon} 恢复 {value:Magnitude} 生命。"));
		}
		if (Effect.EffectType == WacomTags::Status_Shield)
		{
			return FText::FromString(
				TEXT("{icon:EffectIcon} 获得 {value:Magnitude} 护盾。"));
		}
		if (Effect.EffectType == WacomTags::Effect_Draw)
		{
			return FText::FromString(TEXT("抽 {value:Magnitude} 张牌。"));
		}
		if (Effect.EffectType == WacomTags::Effect_ApplyStatus_Poison
			|| Effect.EffectType == WacomTags::Effect_ApplyStatus_Slow
			|| Effect.EffectType == WacomTags::Effect_ApplyStatus_Freeze
			|| Effect.EffectType == WacomTags::Effect_ApplyStatus_Twilight
			|| Effect.EffectType == WacomTags::Effect_ApplyStatus_Burn)
		{
			return FText::FromString(
				TEXT("施加 {value:Magnitude} {status:EffectStatus}。"));
		}
		return FText::GetEmpty();
	}

	FWacomCardExplanationTemplateSet BuildFireWriteExplanationTemplates(
		const UCardDefinition& Card)
	{
		FWacomCardExplanationTemplateSet Result;
		if (Card.TierProfiles.IsEmpty())
		{
			return Result;
		}

		const FWacomCardTierProfile& White = Card.TierProfiles[0];
		Result.EffectTemplates.SetNum(White.Effects.Num());
		for (int32 Index = 0; Index < White.Effects.Num(); ++Index)
		{
			Result.EffectTemplates[Index].Template =
				DefaultFireWriteEffectTemplate(White.Effects[Index]);
		}
		Result.PassiveTemplates.SetNum(White.Passives.Num());

		FString EnglishName = Card.CardId.ToString();
		int32 DotIndex = INDEX_NONE;
		if (EnglishName.FindLastChar(TEXT('.'), DotIndex))
		{
			EnglishName = EnglishName.Mid(DotIndex + 1);
		}

		auto SetEffect = [&Result](const int32 Index, const TCHAR* Template)
		{
			if (Result.EffectTemplates.IsValidIndex(Index))
			{
				Result.EffectTemplates[Index].Template =
					FText::FromString(Template);
			}
		};
		auto SetPassive = [&Result](const int32 Index, const TCHAR* Template)
		{
			if (Result.PassiveTemplates.IsValidIndex(Index))
			{
				Result.PassiveTemplates[Index].Template =
					FText::FromString(Template);
			}
		};
		auto SuppressEffect = [&Result](const int32 Index)
		{
			if (Result.EffectTemplates.IsValidIndex(Index))
			{
				Result.EffectTemplates[Index].Template = FText::GetEmpty();
				Result.EffectTemplates[Index].bSuppressInDetails = true;
			}
		};
		auto AddKeyword = [&Result](
			const FGameplayTag Keyword,
			const TCHAR* Template = TEXT("{keyword:Keyword}。"))
		{
			FWacomCardKeywordExplanationTemplate& Entry =
				Result.KeywordTemplates.AddDefaulted_GetRef();
			Entry.Keyword = Keyword;
			Entry.Template = FText::FromString(Template);
		};

		if (EnglishName == TEXT("OilCandle"))
		{
			SetPassive(
				0,
				TEXT("本场曾进入消耗区时：战斗胜利或撤离后，永久耐久 +{value:PassiveEffect[0].Magnitude}，灼烧 +{value:PassiveEffect[1].Magnitude}。"));
		}
		else if (EnglishName == TEXT("AshBug"))
		{
			AddKeyword(WacomTags::Card_Keyword_Exhaust);
			SetEffect(
				0,
				TEXT("对所有敌人施加 {value:Magnitude} {status:EffectStatus}。"));
			SetPassive(
				0,
				TEXT("回合结束时：若本卡在消耗区，免费自动打出，随后进入弃牌堆。"));
		}
		else if (EnglishName == TEXT("SaltMaggot"))
		{
			AddKeyword(WacomTags::Card_Keyword_Exhaust);
			SetEffect(
				0,
				TEXT("对所有敌人施加 {value:Magnitude} {status:EffectStatus}。"));
			SetEffect(
				1,
				TEXT("将 {value:Magnitude} 张同阶熔熔盐置入手牌。"));
		}
		else if (EnglishName == TEXT("WarmTinderbug"))
		{
			AddKeyword(WacomTags::Card_Keyword_Retain);
			SetEffect(
				0,
				TEXT("使手牌中所有卡（包含自身）的灼烧效果 +{value:Magnitude}；已有灼烧的卡牌获得双倍加成。"));
			SuppressEffect(1);
			Result.DynamicCostTemplate = FText::FromString(
				TEXT("手牌中每有一张 {status:CountedStatus} 卡牌，本卡费用 -{value:ReductionPerMatchingCard}。"));
		}
		else if (EnglishName == TEXT("FireflySeed"))
		{
			SetEffect(
				1,
				TEXT("将本卡的完整战斗复制品随机插入抽牌堆。"));
			SetPassive(
				0,
				TEXT("抽到时：生成 {value:PassiveEffect[0].Magnitude} 张同阶随机萤火虫。"));
		}
		else if (EnglishName == TEXT("HungryFireflyMaiden"))
		{
			SetEffect(0, TEXT("消耗目标伙伴手牌。"));
			SetEffect(
				1,
				TEXT("生成 {value:Magnitude} 张同阶随机萤火虫。"));
		}
		else if (EnglishName == TEXT("BlazingEyeFirefly"))
		{
			AddKeyword(WacomTags::Card_Keyword_Exhaust);
			SetPassive(
				0,
				TEXT("相邻伙伴被打出时：本场自身灼烧效果 +{value:PassiveEffect[0].Magnitude}。"));
		}
		else if (EnglishName == TEXT("RottenFirefly"))
		{
			AddKeyword(WacomTags::Card_Keyword_Exhaust);
			SetPassive(
				0,
				TEXT("相邻伙伴被打出时：本场自身暴击率 +{value:PassiveEffect[0].Magnitude}%。"));
		}
		else if (EnglishName == TEXT("GlimmerFirefly"))
		{
			AddKeyword(WacomTags::Card_Keyword_Exhaust);
		}
		else if (EnglishName == TEXT("SlothFirefly"))
		{
			AddKeyword(WacomTags::Card_Keyword_Exhaust);
			SetPassive(
				0,
				TEXT("相邻伙伴被打出时：本场自身费用 -{value:PassiveEffect[0].Magnitude}，最低 0。"));
		}
		else if (EnglishName == TEXT("EmptyBottle"))
		{
			AddKeyword(WacomTags::Card_Keyword_Exhaust);
		}
		else if (EnglishName == TEXT("MoltenSalt"))
		{
			AddKeyword(WacomTags::Card_Keyword_Exhaust);
			SetEffect(
				0,
				TEXT("对所有敌人施加 {value:Magnitude} {status:EffectStatus}。"));
			SetEffect(
				1,
				TEXT("对所有敌人施加 {value:Magnitude} {status:EffectStatus}。"));
		}
		else if (EnglishName == TEXT("JadeBeetle"))
		{
			SetPassive(
				0,
				TEXT("抽到时：本场自身费用 -{value:PassiveEffect[0].Magnitude}，最低 0。"));
		}
		else if (EnglishName == TEXT("ObsidianBeetle"))
		{
			SetPassive(
				0,
				TEXT("每次抽到本卡时：本场自身伤害翻倍。"));
		}
		else if (EnglishName == TEXT("BlindSpider"))
		{
			AddKeyword(WacomTags::Card_Keyword_Combo);
			SetPassive(
				0,
				TEXT("每打出一张其它伙伴：本场自身费用 -{value:PassiveEffect[0].Magnitude}，最低 0。"));
		}
		return Result;
	}

	bool AreExplanationTemplatesEquivalent(
		const FWacomCardExplanationTemplateSet& A,
		const FWacomCardExplanationTemplateSet& B)
	{
		auto AreLinesEquivalent = [](
			const TArray<FWacomCardExplanationLineTemplate>& Left,
			const TArray<FWacomCardExplanationLineTemplate>& Right)
		{
			if (Left.Num() != Right.Num())
			{
				return false;
			}
			for (int32 Index = 0; Index < Left.Num(); ++Index)
			{
				if (Left[Index].Template.ToString()
						!= Right[Index].Template.ToString()
					|| Left[Index].bSuppressInDetails
						!= Right[Index].bSuppressInDetails)
				{
					return false;
				}
			}
			return true;
		};

		if (!AreLinesEquivalent(A.EffectTemplates, B.EffectTemplates)
			|| !AreLinesEquivalent(A.PassiveTemplates, B.PassiveTemplates)
			|| A.DynamicCostTemplate.ToString()
				!= B.DynamicCostTemplate.ToString()
			|| A.KeywordTemplates.Num() != B.KeywordTemplates.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < A.KeywordTemplates.Num(); ++Index)
		{
			if (!A.KeywordTemplates[Index].Keyword.MatchesTagExact(
					B.KeywordTemplates[Index].Keyword)
				|| A.KeywordTemplates[Index].Template.ToString()
					!= B.KeywordTemplates[Index].Template.ToString())
			{
				return false;
			}
		}
		return true;
	}

	void ConfigureFireWriteExplanationTemplates(UCardDefinition& Card)
	{
		Card.ExplanationTemplates =
			BuildFireWriteExplanationTemplates(Card);
	}

	void SetProfiles(
		UCardDefinition& Card,
		const TFunction<FWacomCardTierProfile(int32)>& BuildProfile)
	{
		Card.TierProfiles.Reset(WacomCardUpgrade::TierCount);
		for (int32 TierIndex = 0;
			TierIndex < WacomCardUpgrade::TierCount;
			++TierIndex)
		{
			Card.TierProfiles.Add(BuildProfile(TierIndex));
		}

		// Keep the old flat fields as an authored White mirror for editor tools
		// that have not yet become tier-aware. Runtime resolution always reads
		// TierProfiles for these cards.
		const FWacomCardTierProfile& White = Card.TierProfiles[0];
		Card.Description = White.Description;
		Card.BaseCost = White.BaseCost;
		Card.Rarity = WacomTags::Card_Rarity_White;
		Card.Physique = White.Physique;
		Card.Effects = White.Effects;
		Card.PerfectReleaseEffects = White.PerfectReleaseEffects;
		Card.ZoneHooks = White.ZoneHooks;
		Card.Passives = White.Passives;
		ConfigureFireWriteExplanationTemplates(Card);
	}

	void SetCommon(
		UCardDefinition& Card,
		const FString& EnglishName,
		const FString& ChineseName,
		const ECardTargetMode TargetMode,
		const FGameplayTagContainer& CardKeywords)
	{
		Card.CardId = FName(*(TEXT("Card.FireWrite.") + EnglishName));
		Card.DisplayName = FText::FromString(ChineseName);
		Card.TargetMode = TargetMode;
		Card.Keywords = CardKeywords;
		Card.CardIllustration = nullptr;
		Card.CardIllustrationDepthMap = nullptr;
		Card.RunFace = {};
		Card.HandCardTargetFilter = {};
	}

	UCardDefinition* ResolveCard(
		const TMap<FString, UCardDefinition*>& Cards,
		const TCHAR* EnglishName,
		TArray<FString>& OutErrors)
	{
		if (UCardDefinition* const* Found = Cards.Find(EnglishName))
		{
			return *Found;
		}
		OutErrors.Add(FString::Printf(
			TEXT("Missing FireWrite reference: %s"),
			EnglishName));
		return nullptr;
	}

	bool ConfigureCard(
		UCardDefinition& Card,
		const FString& EnglishName,
		const TMap<FString, UCardDefinition*>& Cards,
		TArray<FString>& OutErrors)
	{
		const auto Four = [](const int32 A, const int32 B, const int32 C, const int32 D)
		{
			return TArray<int32>{ A, B, C, D };
		};
		const FGameplayTag Single = WacomTags::Target_SingleEnemyPart;
		const FGameplayTag All = WacomTags::Target_AllEnemyParts;

		if (EnglishName == TEXT("OilCandle"))
		{
			const TArray<int32> Burn = Four(3, 6, 9, 12);
			SetCommon(Card, EnglishName, TEXT("虫油蜡烛"),
				ECardTargetMode::SingleEnemyPart,
				Keywords({WacomTags::Card_Keyword_Tool}));
			SetProfiles(Card, [&](const int32 Tier)
			{
				FWacomCardTierProfile Profile;
				Profile.BaseCost = 1;
				Profile.Physique = MakePhysique(0, 3);
				Profile.Description = FText::FromString(FString::Printf(
					TEXT("施加 %d 层灼烧。耐久 3。本场曾进入消耗区时，胜利或撤离后永久耐久 +1，灼烧 +1。"),
					Burn[Tier]));
				Profile.Effects =
				{
					MakeEffect(WacomTags::Effect_ApplyStatus_Burn, Burn[Tier], Single)
				};
				Profile.Passives =
				{
					MakePassive(
						WacomTags::Passive_Trigger_OnBattleSettlement,
						TEXT("本场曾进入消耗区时：战斗胜利或撤离后，永久耐久 +1，灼烧 +1。"),
						{
							MakeRuntimeEffect(
								WacomTags::Effect_Card_AddPersistentDurability,
								1),
							MakeRuntimeEffectModifier(
								WacomTags::Effect_Card_AddPersistentEffectMagnitude,
								WacomTags::Effect_ApplyStatus_Burn,
								1)
						},
						EverEnteredExhaust())
				};
				return Profile;
			});
			return true;
		}

		if (EnglishName == TEXT("AshBug"))
		{
			const TArray<int32> Hp = Four(18, 24, 30, 36);
			const TArray<int32> Burn = Four(10, 15, 20, 25);
			SetCommon(Card, EnglishName, TEXT("灰烬虫"),
				ECardTargetMode::AllEnemyParts,
				Keywords({
					WacomTags::Card_Keyword_Companion,
					WacomTags::Card_Keyword_Exhaust}));
			SetProfiles(Card, [&](const int32 Tier)
			{
				FWacomCardTierProfile Profile;
				Profile.BaseCost = 3;
				Profile.Physique = MakePhysique(Hp[Tier]);
				Profile.Description = FText::FromString(FString::Printf(
					TEXT("体质 %d。对所有敌方部位施加 %d 层灼烧。回合结束时若在消耗区，免费自动打出并进入弃牌堆。"),
					Hp[Tier], Burn[Tier]));
				Profile.Effects =
				{
					MakeEffect(WacomTags::Effect_ApplyStatus_Burn, Burn[Tier], All)
				};
				Profile.Passives =
				{
					MakePassive(
						WacomTags::Passive_Trigger_OnTurnEnd,
						TEXT("回合结束时：若本卡在消耗区，免费自动打出，随后进入弃牌堆。"),
						{ MakeRuntimeEffect(WacomTags::Effect_Card_AutoPlaySelf, 0) },
						InCardLocation(WacomTags::CardLocation_Exhaust))
				};
				return Profile;
			});
			return true;
		}

		if (EnglishName == TEXT("SaltMaggot"))
		{
			UCardDefinition* MoltenSalt =
				ResolveCard(Cards, TEXT("MoltenSalt"), OutErrors);
			if (!MoltenSalt) return false;
			const TArray<int32> Hp = Four(6, 12, 18, 24);
			const TArray<int32> Burn = Four(5, 8, 12, 15);
			SetCommon(Card, EnglishName, TEXT("盐味熔蛆"),
				ECardTargetMode::AllEnemyParts,
				Keywords({
					WacomTags::Card_Keyword_Companion,
					WacomTags::Card_Keyword_Exhaust}));
			SetProfiles(Card, [&](const int32 Tier)
			{
				FWacomCardTierProfile Profile;
				Profile.BaseCost = 1;
				Profile.Physique = MakePhysique(Hp[Tier]);
				Profile.Description = FText::FromString(FString::Printf(
					TEXT("体质 %d。对所有敌方部位施加 %d 层灼烧。生成 2 张同阶熔熔盐到手牌。"),
					Hp[Tier], Burn[Tier]));
				Profile.Effects =
				{
					MakeEffect(WacomTags::Effect_ApplyStatus_Burn, Burn[Tier], All),
					MakeCreate(
						WacomTags::Effect_Card_GenerateToHand,
						2,
						{ MoltenSalt })
				};
				return Profile;
			});
			return true;
		}

		if (EnglishName == TEXT("WarmTinderbug"))
		{
			const TArray<int32> Hp = Four(25, 30, 35, 40);
			const TArray<int32> Bonus = Four(1, 2, 3, 4);
			SetCommon(Card, EnglishName, TEXT("温热火绒虫"),
				ECardTargetMode::None,
				Keywords({
					WacomTags::Card_Keyword_Companion,
					WacomTags::Card_Keyword_Retain}));
			SetProfiles(Card, [&](const int32 Tier)
			{
				FWacomCardTierProfile Profile;
				Profile.BaseCost = 5;
				Profile.Physique = MakePhysique(Hp[Tier]);
				Profile.DynamicCostRule.CountHandCardsWithStatus =
					WacomTags::Status_Burn;
				Profile.DynamicCostRule.ReductionPerMatchingCard = 1;
				Profile.DynamicCostRule.MinimumCost = 0;
				Profile.Description = FText::FromString(FString::Printf(
					TEXT("体质 %d。提交前手牌中所有卡（包含自身）的灼烧效果 +%d；已有卡牌灼烧者再获得一次该加值。手牌每有 1 张灼烧卡，本卡费用 -1。"),
					Hp[Tier], Bonus[Tier]));
				Profile.Effects =
				{
					MakeRuntimeEffectModifier(
						WacomTags::Effect_Card_AddEffectMagnitude,
						WacomTags::Effect_ApplyStatus_Burn,
						Bonus[Tier],
						WacomTags::Target_AllHandCards),
					MakeRuntimeEffectModifier(
						WacomTags::Effect_Card_AddEffectMagnitude,
						WacomTags::Effect_ApplyStatus_Burn,
						Bonus[Tier],
						WacomTags::Target_AllHandCards,
						WacomTags::Status_Burn)
				};
				return Profile;
			});
			return true;
		}

		if (EnglishName == TEXT("FireflySeed"))
		{
			UCardDefinition* Blazing =
				ResolveCard(Cards, TEXT("BlazingEyeFirefly"), OutErrors);
			UCardDefinition* Rotten =
				ResolveCard(Cards, TEXT("RottenFirefly"), OutErrors);
			UCardDefinition* Glimmer =
				ResolveCard(Cards, TEXT("GlimmerFirefly"), OutErrors);
			UCardDefinition* Sloth =
				ResolveCard(Cards, TEXT("SlothFirefly"), OutErrors);
			if (!Blazing || !Rotten || !Glimmer || !Sloth) return false;
			const TArray<UCardDefinition*> Pool = { Blazing, Rotten, Glimmer, Sloth };
			const TArray<int32> Burn = Four(2, 3, 4, 5);
			SetCommon(Card, EnglishName, TEXT("流萤火种"),
				ECardTargetMode::SingleEnemyPart,
				Keywords({WacomTags::Card_Keyword_Tool}));
			SetProfiles(Card, [&](const int32 Tier)
			{
				FWacomCardTierProfile Profile;
				Profile.BaseCost = 1;
				Profile.Physique = MakePhysique(0, 1);
				Profile.Description = FText::FromString(FString::Printf(
					TEXT("施加 %d 层灼烧。先将本卡的完整战斗复制品随机插入抽牌堆，再扣除 1 点耐久。抽到时生成 1 张同阶随机萤火虫。"),
					Burn[Tier]));
				Profile.Effects =
				{
					MakeEffect(WacomTags::Effect_ApplyStatus_Burn, Burn[Tier], Single),
					MakeRuntimeEffect(
						WacomTags::Effect_Card_CloneSelfIntoDraw,
						0)
				};
				Profile.Passives =
				{
					MakePassive(
						WacomTags::Passive_Trigger_OnDraw,
						TEXT("抽到时：生成 1 张同阶随机萤火虫。"),
						{
							MakeCreate(
								WacomTags::Effect_Card_GenerateRandomFromPoolToHand,
								1,
								Pool)
						})
				};
				return Profile;
			});
			return true;
		}

		if (EnglishName == TEXT("HungryFireflyMaiden"))
		{
			UCardDefinition* Blazing =
				ResolveCard(Cards, TEXT("BlazingEyeFirefly"), OutErrors);
			UCardDefinition* Rotten =
				ResolveCard(Cards, TEXT("RottenFirefly"), OutErrors);
			UCardDefinition* Glimmer =
				ResolveCard(Cards, TEXT("GlimmerFirefly"), OutErrors);
			UCardDefinition* Sloth =
				ResolveCard(Cards, TEXT("SlothFirefly"), OutErrors);
			if (!Blazing || !Rotten || !Glimmer || !Sloth) return false;
			const TArray<UCardDefinition*> Pool = { Blazing, Rotten, Glimmer, Sloth };
			const TArray<int32> Count = Four(2, 3, 4, 5);
			SetCommon(Card, EnglishName, TEXT("饥饿的萤火侍女"),
				ECardTargetMode::HandCard,
				Keywords({WacomTags::Card_Keyword_Companion}));
			Card.HandCardTargetFilter.bUseExplicitHandCardTargetFilter = true;
			Card.HandCardTargetFilter.bAllowNormalHandCards = true;
			Card.HandCardTargetFilter.bAllowHandAnchors = false;
			Card.HandCardTargetFilter.RequiredTargetKeywords.AddTag(
				WacomTags::Card_Keyword_Companion);
			SetProfiles(Card, [&](const int32 Tier)
			{
				FWacomCardTierProfile Profile;
				Profile.BaseCost = 2;
				Profile.Description = FText::FromString(FString::Printf(
					TEXT("消耗另一张普通伙伴手牌；生成 %d 张同阶随机萤火虫。"),
					Count[Tier]));
				Profile.Effects =
				{
					MakeEffect(
						WacomTags::Effect_Card_ExhaustSelected,
						1,
						WacomTags::Target_SelectedHandCard),
					MakeCreate(
						WacomTags::Effect_Card_GenerateRandomFromPoolToHand,
						Count[Tier],
						Pool)
				};
				return Profile;
			});
			return true;
		}

		if (EnglishName == TEXT("BlazingEyeFirefly"))
		{
			const TArray<int32> Hp = Four(1, 2, 4, 8);
			const TArray<int32> Burn = Four(1, 2, 3, 4);
			SetCommon(Card, EnglishName, TEXT("灼眼·萤火虫"),
				ECardTargetMode::SingleEnemyPart,
				Keywords({
					WacomTags::Card_Keyword_Companion,
					WacomTags::Card_Keyword_Exhaust}));
			SetProfiles(Card, [&](const int32 Tier)
			{
				FWacomCardTierProfile Profile;
				Profile.BaseCost = 0;
				Profile.Physique = MakePhysique(Hp[Tier]);
				Profile.Description = FText::FromString(FString::Printf(
					TEXT("体质 %d。施加 %d 层灼烧。相邻伙伴被打出时，本场自身灼烧效果永久 +%d。"),
					Hp[Tier], Burn[Tier], Burn[Tier]));
				Profile.Effects =
				{
					MakeEffect(WacomTags::Effect_ApplyStatus_Burn, Burn[Tier], Single)
				};
				Profile.Passives =
				{
					MakePassive(
						WacomTags::Passive_Trigger_OnAdjacentCompanionPlayed,
						FString::Printf(
							TEXT("相邻伙伴被打出时：本场自身灼烧效果 +%d。"),
							Burn[Tier]),
						{
							MakeRuntimeEffectModifier(
								WacomTags::Effect_Card_AddEffectMagnitude,
								WacomTags::Effect_ApplyStatus_Burn,
								Burn[Tier])
						})
				};
				return Profile;
			});
			return true;
		}

		if (EnglishName == TEXT("RottenFirefly"))
		{
			const TArray<int32> Hp = Four(1, 2, 4, 8);
			const TArray<int32> Critical = Four(25, 50, 75, 100);
			SetCommon(Card, EnglishName, TEXT("腐萤·萤火虫"),
				ECardTargetMode::SingleEnemyPart,
				Keywords({
					WacomTags::Card_Keyword_Companion,
					WacomTags::Card_Keyword_Exhaust}));
			SetProfiles(Card, [&](const int32 Tier)
			{
				FWacomCardTierProfile Profile;
				Profile.BaseCost = 1;
				Profile.Physique = MakePhysique(Hp[Tier]);
				Profile.Description = FText::FromString(FString::Printf(
					TEXT("体质 %d。施加 1 层中毒。相邻伙伴被打出时，本场自身暴击率 +%d%%。"),
					Hp[Tier], Critical[Tier]));
				Profile.Effects =
				{
					MakeEffect(WacomTags::Effect_ApplyStatus_Poison, 1, Single)
				};
				Profile.Passives =
				{
					MakePassive(
						WacomTags::Passive_Trigger_OnAdjacentCompanionPlayed,
						FString::Printf(
							TEXT("相邻伙伴被打出时：本场自身暴击率 +%d%%。"),
							Critical[Tier]),
						{
							MakeRuntimeEffect(
								WacomTags::Effect_Card_AddCriticalChance,
								Critical[Tier])
						})
				};
				return Profile;
			});
			return true;
		}

		if (EnglishName == TEXT("GlimmerFirefly"))
		{
			const TArray<int32> Hp = Four(1, 2, 4, 8);
			SetCommon(Card, EnglishName, TEXT("微光·萤火虫"),
				ECardTargetMode::None,
				Keywords({
					WacomTags::Card_Keyword_Companion,
					WacomTags::Card_Keyword_Exhaust}));
			SetProfiles(Card, [&](const int32 Tier)
			{
				FWacomCardTierProfile Profile;
				Profile.BaseCost = 2;
				Profile.Physique = MakePhysique(Hp[Tier]);
				Profile.Description = FText::FromString(FString::Printf(
					TEXT("体质 %d。抽 2 张牌。"),
					Hp[Tier]));
				Profile.Effects = { MakeDraw(2) };
				return Profile;
			});
			return true;
		}

		if (EnglishName == TEXT("SlothFirefly"))
		{
			const TArray<int32> Hp = Four(1, 2, 4, 8);
			const TArray<int32> Slow = Four(1, 2, 3, 4);
			SetCommon(Card, EnglishName, TEXT("怠惰·萤火虫"),
				ECardTargetMode::SingleEnemyPart,
				Keywords({
					WacomTags::Card_Keyword_Companion,
					WacomTags::Card_Keyword_Exhaust}));
			SetProfiles(Card, [&](const int32 Tier)
			{
				FWacomCardTierProfile Profile;
				Profile.BaseCost = 3;
				Profile.Physique = MakePhysique(Hp[Tier]);
				Profile.Description = FText::FromString(FString::Printf(
					TEXT("体质 %d。施加 %d 层减速。相邻伙伴被打出时，本场自身费用 -1，最低 0。"),
					Hp[Tier], Slow[Tier]));
				Profile.Effects =
				{
					MakeEffect(WacomTags::Effect_ApplyStatus_Slow, Slow[Tier], Single)
				};
				Profile.Passives =
				{
					MakePassive(
						WacomTags::Passive_Trigger_OnAdjacentCompanionPlayed,
						TEXT("相邻伙伴被打出时：本场自身费用 -1，最低 0。"),
						{
							MakeRuntimeEffect(
								WacomTags::Effect_Card_ReduceCost,
								1)
						})
				};
				return Profile;
			});
			return true;
		}

		if (EnglishName == TEXT("EmptyBottle"))
		{
			SetCommon(Card, EnglishName, TEXT("空瓶"),
				ECardTargetMode::None,
				Keywords({
					WacomTags::Card_Keyword_Tool,
					WacomTags::Card_Keyword_Container,
					WacomTags::Card_Keyword_Exhaust}));
			SetProfiles(Card, [](const int32)
			{
				FWacomCardTierProfile Profile;
				Profile.BaseCost = 1;
				Profile.Physique = MakePhysique(0, 1, 1);
				Profile.Description =
					FText::FromString(TEXT("容器。容量 1。耐久 1。"));
				return Profile;
			});
			return true;
		}

		if (EnglishName == TEXT("MoltenSalt"))
		{
			const TArray<int32> Amount = Four(1, 2, 3, 4);
			SetCommon(Card, EnglishName, TEXT("熔熔盐"),
				ECardTargetMode::AllEnemyParts,
				Keywords({
					WacomTags::Card_Keyword_Food,
					WacomTags::Card_Keyword_Exhaust}));
			SetProfiles(Card, [&](const int32 Tier)
			{
				FWacomCardTierProfile Profile;
				Profile.BaseCost = 0;
				Profile.Description = FText::FromString(FString::Printf(
					TEXT("对所有敌方部位施加 %d 层减速，随后施加 %d 层灼烧。"),
					Amount[Tier], Amount[Tier]));
				Profile.Effects =
				{
					MakeEffect(WacomTags::Effect_ApplyStatus_Slow, Amount[Tier], All),
					MakeEffect(WacomTags::Effect_ApplyStatus_Burn, Amount[Tier], All)
				};
				return Profile;
			});
			return true;
		}

		if (EnglishName == TEXT("JadeBeetle"))
		{
			const TArray<int32> Hp = Four(22, 27, 32, 37);
			const TArray<int32> Poison = Four(2, 4, 6, 8);
			SetCommon(Card, EnglishName, TEXT("翡翠甲虫"),
				ECardTargetMode::SingleEnemyPart,
				Keywords({WacomTags::Card_Keyword_Companion}));
			SetProfiles(Card, [&](const int32 Tier)
			{
				FWacomCardTierProfile Profile;
				Profile.BaseCost = 4;
				Profile.Physique = MakePhysique(Hp[Tier]);
				Profile.Description = FText::FromString(FString::Printf(
					TEXT("体质 %d。施加 %d 层中毒。每次正式抽到本卡，本场自身费用 -1，最低 0。"),
					Hp[Tier], Poison[Tier]));
				Profile.Effects =
				{
					MakeEffect(WacomTags::Effect_ApplyStatus_Poison, Poison[Tier], Single)
				};
				Profile.Passives =
				{
					MakePassive(
						WacomTags::Passive_Trigger_OnDraw,
						TEXT("抽到时：本场自身费用 -1，最低 0。"),
						{
							MakeRuntimeEffect(
								WacomTags::Effect_Card_ReduceCost,
								1)
						})
				};
				return Profile;
			});
			return true;
		}

		if (EnglishName == TEXT("ObsidianBeetle"))
		{
			const TArray<int32> Cost = Four(4, 3, 2, 1);
			const TArray<int32> Hp = Four(28, 56, 112, 224);
			SetCommon(Card, EnglishName, TEXT("黑曜石甲虫"),
				ECardTargetMode::SingleEnemyPart,
				Keywords({
					WacomTags::Card_Keyword_Companion,
					WacomTags::Card_Keyword_Weapon}));
			SetProfiles(Card, [&](const int32 Tier)
			{
				FWacomCardTierProfile Profile;
				Profile.BaseCost = Cost[Tier];
				Profile.Physique = MakePhysique(Hp[Tier]);
				Profile.Description = FText::FromString(FString::Printf(
					TEXT("体质 %d。造成 5 点伤害。每次正式抽到本卡，本场自身伤害翻倍。"),
					Hp[Tier]));
				Profile.Effects =
				{
					MakeEffect(WacomTags::Effect_Damage, 5, Single)
				};
				Profile.Passives =
				{
					MakePassive(
						WacomTags::Passive_Trigger_OnDraw,
						TEXT("每次抽到本卡时：本场自身伤害翻倍。"),
						{
							MakeRuntimeEffectModifier(
								WacomTags::Effect_Card_MultiplyEffectMagnitude,
								WacomTags::Effect_Damage,
								2)
						})
				};
				return Profile;
			});
			return true;
		}

		if (EnglishName == TEXT("BlindSpider"))
		{
			const TArray<int32> Hp = Four(30, 38, 46, 54);
			const TArray<int32> Damage = Four(8, 16, 32, 64);
			SetCommon(Card, EnglishName, TEXT("盲眼蜘蛛"),
				ECardTargetMode::SingleEnemyPart,
				Keywords({
					WacomTags::Card_Keyword_Companion,
					WacomTags::Card_Keyword_Weapon,
					WacomTags::Card_Keyword_Combo}));
			SetProfiles(Card, [&](const int32 Tier)
			{
				FWacomCardTierProfile Profile;
				Profile.BaseCost = 3;
				Profile.Physique = MakePhysique(Hp[Tier]);
				Profile.Description = FText::FromString(FString::Printf(
					TEXT("体质 %d。造成 %d 点伤害。每打出一张其它伙伴，本场自身费用 -1，最低 0。"),
					Hp[Tier], Damage[Tier]));
				Profile.Effects =
				{
					MakeEffect(WacomTags::Effect_Damage, Damage[Tier], Single)
				};
				Profile.Passives =
				{
					MakePassive(
						WacomTags::Passive_Trigger_OnOtherCompanionPlayed,
						TEXT("每打出一张其它伙伴：本场自身费用 -1，最低 0。"),
						{
							MakeRuntimeEffect(
								WacomTags::Effect_Card_ReduceCost,
								1)
						})
				};
				return Profile;
			});
			return true;
		}

		OutErrors.Add(TEXT("Unknown FireWrite seed entry: ") + EnglishName);
		return false;
	}

	void AppendTextErrors(
		const FString& Prefix,
		const TArray<FText>& Errors,
		TArray<FString>& OutErrors)
	{
		for (const FText& Error : Errors)
		{
			OutErrors.Add(Prefix + Error.ToString());
		}
	}

	bool ValidateCard(
		const FString& EnglishName,
		const UCardDefinition& Card,
		TArray<FString>& OutErrors)
	{
		TArray<FText> Errors;
		const bool bValid =
			FWacomCardDefinitionValidation::Validate(&Card, Errors);
		AppendTextErrors(EnglishName + TEXT(": "), Errors, OutErrors);
		return bValid;
	}

	bool CompareCardToSeed(
		const FString& EnglishName,
		const UCardDefinition& Actual,
		const TMap<FString, UCardDefinition*>& ReferenceCards,
		TArray<FString>& OutErrors)
	{
		TStrongObjectPtr<UCardDefinition> Expected(
			NewObject<UCardDefinition>(GetTransientPackage()));
		TArray<FString> ConfigureErrors;
		if (!ConfigureCard(
			*Expected.Get(),
			EnglishName,
			ReferenceCards,
			ConfigureErrors))
		{
			for (const FString& Error : ConfigureErrors)
			{
				OutErrors.Add(EnglishName + TEXT(": ") + Error);
			}
			return false;
		}

		TArray<FString> CompareErrors;
		if (!CompareFormalProductionEditableProperties(
			Actual,
			*Expected.Get(),
			/*bStrict=*/true,
			CompareErrors))
		{
			for (const FString& Error : CompareErrors)
			{
				OutErrors.Add(EnglishName + TEXT(": ") + Error);
			}
			return false;
		}
		return true;
	}

	bool SaveChangedAsset(
		UObject& Asset,
		const FString& PackagePath,
		FFireWriteCardContentReport& Report)
	{
		UPackage* Package = Asset.GetPackage();
		if (!Wacom::ContentBuilder::SaveAssetPackage(
			Package,
			&Asset,
			PackagePath))
		{
			Report.Errors.Add(TEXT("Failed to save ") + PackagePath);
			return false;
		}
		UPackage::WaitForAsyncFileWrites();
		++Report.SavedCount;
		return true;
	}

	bool MigrateStatusCatalog(FFireWriteCardContentReport& Report)
	{
		UWacomBattleStatusPresentationCatalog* Catalog =
			LoadObject<UWacomBattleStatusPresentationCatalog>(
				nullptr,
				*ObjectPath(StatusCatalogPath));
		if (!Catalog)
		{
			Report.Errors.Add(TEXT("Could not load status catalog: ")
				+ StatusCatalogPath);
			return false;
		}

		const UWacomBattleStatusPresentationCatalog* Defaults =
			GetDefault<UWacomBattleStatusPresentationCatalog>();
		const FWacomBattleStatusPresentationEntry* DefaultBurn =
			Defaults ? Defaults->FindEntry(WacomTags::Status_Burn) : nullptr;
		if (!DefaultBurn)
		{
			Report.Errors.Add(
				TEXT("Status catalog CDO does not contain Status.Burn."));
			return false;
		}

		bool bChanged = false;
		FWacomBattleStatusPresentationEntry* ExistingBurn =
			Catalog->Entries.FindByPredicate([](const auto& Entry)
			{
				return Entry.StatusTag == WacomTags::Status_Burn;
			});
		if (!ExistingBurn)
		{
			Catalog->Modify();
			Catalog->Entries.Add(*DefaultBurn);
			bChanged = true;
		}
		else
		{
			// Preserve an authored Burn icon if content later supplies one.
			const FSlateBrush ExistingIcon = ExistingBurn->IconBrush;
			const bool bNeedsRules =
				!ExistingBurn->DisplayName.EqualTo(DefaultBurn->DisplayName)
				|| ExistingBurn->LookupAliases != DefaultBurn->LookupAliases
				|| ExistingBurn->SortPriority != DefaultBurn->SortPriority
				|| !ExistingBurn->PlayerRules.IsComplete()
				|| !ExistingBurn->EnemyPartRules.IsComplete()
				|| !ExistingBurn->CardRules.IsComplete();
			if (bNeedsRules)
			{
				Catalog->Modify();
				*ExistingBurn = *DefaultBurn;
				ExistingBurn->IconBrush = ExistingIcon;
				bChanged = true;
			}
		}

		if (!bChanged)
		{
			return true;
		}
		return SaveChangedAsset(*Catalog, StatusCatalogPath, Report);
	}

	template <typename EntryType, typename PredicateType>
	bool MergeMissingEntries(
		TArray<EntryType>& Target,
		const TArray<EntryType>& Defaults,
		PredicateType&& Exists)
	{
		bool bChanged = false;
		for (const EntryType& DefaultEntry : Defaults)
		{
			if (!Exists(Target, DefaultEntry))
			{
				Target.Add(DefaultEntry);
				bChanged = true;
			}
		}
		return bChanged;
	}

	bool MigrateLexicon(FFireWriteCardContentReport& Report)
	{
		UWacomCardExplanationLexicon* Lexicon =
			LoadObject<UWacomCardExplanationLexicon>(
				nullptr,
				*ObjectPath(LexiconPath));
		if (!Lexicon)
		{
			Report.Errors.Add(TEXT("Could not load explanation lexicon: ")
				+ LexiconPath);
			return false;
		}

		const UWacomCardExplanationLexicon* Defaults =
			GetDefault<UWacomCardExplanationLexicon>();
		if (!Defaults)
		{
			Report.Errors.Add(TEXT("Card explanation lexicon CDO is missing."));
			return false;
		}

		Lexicon->Modify();
		bool bChanged = false;
		const auto TemplateExists = [](
			const TArray<FWacomCardExplanationTemplateEntry>& Entries,
			const FWacomCardExplanationTemplateEntry& Candidate)
		{
			return Entries.ContainsByPredicate([&](const auto& Entry)
			{
				return Entry.KeyTag == Candidate.KeyTag;
			});
		};
		bChanged |= MergeMissingEntries(
			Lexicon->EffectTemplates,
			Defaults->EffectTemplates,
			TemplateExists);
		bChanged |= MergeMissingEntries(
			Lexicon->PassiveTriggerTemplates,
			Defaults->PassiveTriggerTemplates,
			TemplateExists);
		bChanged |= MergeMissingEntries(
			Lexicon->PassiveOutcomeTemplates,
			Defaults->PassiveOutcomeTemplates,
			TemplateExists);
		bChanged |= MergeMissingEntries(
			Lexicon->MagnitudeSourceTemplates,
			Defaults->MagnitudeSourceTemplates,
			TemplateExists);
		bChanged |= MergeMissingEntries(
			Lexicon->TagDisplayNames,
			Defaults->TagDisplayNames,
			[](const TArray<FWacomCardExplanationTagDisplayEntry>& Entries,
				const FWacomCardExplanationTagDisplayEntry& Candidate)
			{
				return Entries.ContainsByPredicate([&](const auto& Entry)
				{
					return Entry.KeyTag == Candidate.KeyTag;
				});
			});
		bChanged |= MergeMissingEntries(
			Lexicon->NamedTexts,
			Defaults->NamedTexts,
			[](const TArray<FWacomCardExplanationNamedTextEntry>& Entries,
				const FWacomCardExplanationNamedTextEntry& Candidate)
			{
				return Entries.ContainsByPredicate([&](const auto& Entry)
				{
					return Entry.Key == Candidate.Key;
				});
			});
		bChanged |= MergeMissingEntries(
			Lexicon->CardFaceSemantics,
			Defaults->CardFaceSemantics,
			[](const TArray<FWacomCardFaceSemanticLexiconEntry>& Entries,
				const FWacomCardFaceSemanticLexiconEntry& Candidate)
			{
				return Entries.ContainsByPredicate([&](const auto& Entry)
				{
					return Entry.SemanticId == Candidate.SemanticId;
				});
			});

		if (!bChanged)
		{
			Lexicon->GetPackage()->SetDirtyFlag(false);
			return true;
		}
		return SaveChangedAsset(*Lexicon, LexiconPath, Report);
	}

	bool SyncExplanationLexiconDefaults(
		FFireWriteCardContentReport& Report)
	{
		UWacomCardExplanationLexicon* Lexicon =
			LoadObject<UWacomCardExplanationLexicon>(
				nullptr,
				*ObjectPath(LexiconPath));
		const UWacomCardExplanationLexicon* Defaults =
			GetDefault<UWacomCardExplanationLexicon>();
		if (!Lexicon || !Defaults)
		{
			Report.Errors.Add(
				TEXT("Could not load card explanation lexicon defaults."));
			return false;
		}

		const TArray<FGameplayTag> ApprovedCommonEffects = {
			WacomTags::Effect_Damage,
			WacomTags::Effect_Heal,
			WacomTags::Status_Shield,
			WacomTags::Effect_Draw,
			WacomTags::Effect_ApplyStatus_Poison,
			WacomTags::Effect_ApplyStatus_Slow,
			WacomTags::Effect_ApplyStatus_Freeze,
			WacomTags::Effect_ApplyStatus_Twilight,
			WacomTags::Effect_ApplyStatus_Burn
		};

		bool bChanged = false;
		for (const FGameplayTag EffectTag : ApprovedCommonEffects)
		{
			const FWacomCardExplanationTemplateEntry* DefaultEntry =
				Defaults->EffectTemplates.FindByPredicate(
					[EffectTag](
						const FWacomCardExplanationTemplateEntry& Entry)
					{
						return Entry.KeyTag.MatchesTagExact(EffectTag);
					});
			if (!DefaultEntry)
			{
				Report.Errors.Add(FString::Printf(
					TEXT("Missing C++ explanation default for %s."),
					*EffectTag.ToString()));
				continue;
			}

			FWacomCardExplanationTemplateEntry* ExistingEntry =
				Lexicon->EffectTemplates.FindByPredicate(
					[EffectTag](
						const FWacomCardExplanationTemplateEntry& Entry)
					{
						return Entry.KeyTag.MatchesTagExact(EffectTag);
					});
			if (ExistingEntry
				&& ExistingEntry->Template.ToString()
					== DefaultEntry->Template.ToString())
			{
				continue;
			}

			if (!bChanged)
			{
				Lexicon->Modify();
				bChanged = true;
			}
			if (ExistingEntry)
			{
				ExistingEntry->Template = DefaultEntry->Template;
			}
			else
			{
				Lexicon->EffectTemplates.Add(*DefaultEntry);
			}
		}

		if (!Report.Errors.IsEmpty())
		{
			return false;
		}
		if (!bChanged)
		{
			return true;
		}
		return SaveChangedAsset(*Lexicon, LexiconPath, Report);
	}

	bool MigrateShop(FFireWriteCardContentReport& Report)
	{
		UShopDefinition* Shop = LoadObject<UShopDefinition>(
			nullptr,
			*ObjectPath(ShopPath));
		if (!Shop)
		{
			Report.Errors.Add(TEXT("Could not load debug shop: ") + ShopPath);
			return false;
		}

		const int32 Before = Shop->Offers.Num();
		Shop->Modify();
		Shop->Offers.RemoveAll([](const FShopOfferDefinition& Offer)
		{
			const UCardDefinition* Definition = Offer.CardDefinition;
			if (!Definition)
			{
				return false;
			}
			const FString Package = Definition->GetPackage()->GetName();
			return Package == LegacyWhitePath || Package == LegacyBluePath;
		});
		if (Shop->Offers.Num() == Before)
		{
			Shop->GetPackage()->SetDirtyFlag(false);
			return true;
		}
		return SaveChangedAsset(*Shop, ShopPath, Report);
	}

	bool DeleteLegacyAsset(
		const FString& PackagePath,
		FFireWriteCardContentReport& Report)
	{
		if (!FPackageName::DoesPackageExist(PackagePath))
		{
			return true;
		}
		UObject* Asset = LoadObject<UObject>(nullptr, *ObjectPath(PackagePath));
		if (!Asset)
		{
			Report.Errors.Add(TEXT("Legacy package exists but asset failed to load: ")
				+ PackagePath);
			return false;
		}
		if (ObjectTools::DeleteObjectsUnchecked({ Asset }) != 1)
		{
			Report.Errors.Add(TEXT("Could not delete legacy asset: ")
				+ PackagePath);
			return false;
		}
		++Report.DeletedCount;
		return true;
	}

	bool MigrateLegacyAssets(FFireWriteCardContentReport& Report)
	{
		if (!MigrateStatusCatalog(Report)
			|| !MigrateLexicon(Report)
			|| !MigrateShop(Report))
		{
			return false;
		}
		return DeleteLegacyAsset(LegacyWhitePath, Report)
			&& DeleteLegacyAsset(LegacyBluePath, Report);
	}
}

namespace Wacom::ContentBuilder
{
	const TArray<FString>& GetFireWriteCardPackagePaths()
	{
		static TArray<FString> Paths;
		if (Paths.IsEmpty())
		{
			for (const FireWritePrivate::FSeedEntry& Entry :
				FireWritePrivate::SeedEntries())
			{
				Paths.Add(FireWritePrivate::PackagePath(Entry.EnglishName));
			}
		}
		return Paths;
	}

	bool ValidateFireWriteTransientDefaults(TArray<FString>& OutErrors)
	{
		TMap<FString, UCardDefinition*> Cards;
		TArray<TStrongObjectPtr<UCardDefinition>> KeepAlive;
		for (const FireWritePrivate::FSeedEntry& Entry :
			FireWritePrivate::SeedEntries())
		{
			TStrongObjectPtr<UCardDefinition> Card(
				NewObject<UCardDefinition>(GetTransientPackage()));
			Cards.Add(Entry.EnglishName, Card.Get());
			KeepAlive.Add(MoveTemp(Card));
		}

		for (const FireWritePrivate::FSeedEntry& Entry :
			FireWritePrivate::SeedEntries())
		{
			UCardDefinition* Card = Cards.FindRef(Entry.EnglishName);
			if (!Card
				|| !FireWritePrivate::ConfigureCard(
					*Card,
					Entry.EnglishName,
					Cards,
					OutErrors)
				|| !FireWritePrivate::ValidateCard(
					Entry.EnglishName,
					*Card,
					OutErrors))
			{
				continue;
			}
		}
		return OutErrors.IsEmpty();
	}

	bool ValidateFireWriteLoadedAssets(
		const bool bCompareSeedDefaults,
		TArray<FString>& OutErrors)
	{
		TMap<FString, UCardDefinition*> Cards;
		for (const FireWritePrivate::FSeedEntry& Entry :
			FireWritePrivate::SeedEntries())
		{
			const FString Package =
				FireWritePrivate::PackagePath(Entry.EnglishName);
			UCardDefinition* Card = LoadObject<UCardDefinition>(
				nullptr,
				*FireWritePrivate::ObjectPath(Package));
			if (!Card)
			{
				OutErrors.Add(TEXT("Missing FireWrite card: ") + Package);
				continue;
			}
			Cards.Add(Entry.EnglishName, Card);
		}
		if (Cards.Num() != FireWritePrivate::SeedEntries().Num())
		{
			return false;
		}

		for (const FireWritePrivate::FSeedEntry& Entry :
			FireWritePrivate::SeedEntries())
		{
			const UCardDefinition* Card = Cards.FindRef(Entry.EnglishName);
			if (!Card)
			{
				continue;
			}
			FireWritePrivate::ValidateCard(
				Entry.EnglishName,
				*Card,
				OutErrors);
			if (bCompareSeedDefaults)
			{
				FireWritePrivate::CompareCardToSeed(
					Entry.EnglishName,
					*Card,
					Cards,
					OutErrors);
			}
		}
		return OutErrors.IsEmpty();
	}

	int32 RunFireWriteCardContentBuilder(
		const FFireWriteCardContentOptions& Options,
		FFireWriteCardContentReport* OutReport)
	{
		using namespace FireWritePrivate;
		FFireWriteCardContentReport Report;

		TArray<FString> TransientErrors;
		if (!ValidateFireWriteTransientDefaults(TransientErrors))
		{
			Report.Errors.Append(TransientErrors);
		}

		TMap<FString, UCardDefinition*> Cards;
		TMap<FString, UPackage*> CreatedPackages;
		TSet<FString> ChangedCards;
		for (const FSeedEntry& Entry : SeedEntries())
		{
			const FString Package = PackagePath(Entry.EnglishName);
			if (FPackageName::DoesPackageExist(Package))
			{
				UObject* Existing = LoadObject<UObject>(
					nullptr,
					*ObjectPath(Package));
				UCardDefinition* Card = Cast<UCardDefinition>(Existing);
				if (!Card)
				{
					Report.Errors.Add(FString::Printf(
						TEXT("%s exists with wrong class %s"),
						*Package,
						*GetNameSafe(Existing ? Existing->GetClass() : nullptr)));
					continue;
				}
				Cards.Add(Entry.EnglishName, Card);
				++Report.ExistingCount;
				continue;
			}

			if (!Options.bSeedMissing)
			{
				Report.Errors.Add(TEXT("Missing FireWrite card: ") + Package);
				++Report.MissingCount;
				continue;
			}

			UPackage* NewPackage = CreatePackage(*Package);
			const FName AssetName(
				*FPackageName::GetLongPackageAssetName(Package));
			UCardDefinition* Card = NewPackage
				? NewObject<UCardDefinition>(
					NewPackage,
					AssetName,
					RF_Public | RF_Standalone | RF_Transactional)
				: nullptr;
			if (!Card)
			{
				Report.Errors.Add(TEXT("Could not allocate ") + Package);
				continue;
			}
			Cards.Add(Entry.EnglishName, Card);
			CreatedPackages.Add(Entry.EnglishName, NewPackage);
			++Report.CreatedCount;
		}

		if (Cards.Num() == SeedEntries().Num() && Report.Errors.IsEmpty())
		{
			for (const FSeedEntry& Entry : SeedEntries())
			{
				UCardDefinition* Card = Cards.FindRef(Entry.EnglishName);
				if (CreatedPackages.Contains(Entry.EnglishName)
					&& !ConfigureCard(
						*Card,
						Entry.EnglishName,
						Cards,
						Report.Errors))
				{
					break;
				}
				if (Options.bSyncSeedDefaults
					&& !CreatedPackages.Contains(Entry.EnglishName))
				{
					TArray<FString> Differences;
					if (!CompareCardToSeed(
						Entry.EnglishName,
						*Card,
						Cards,
						Differences))
					{
						Card->Modify();
						if (!ConfigureCard(
							*Card,
							Entry.EnglishName,
							Cards,
							Report.Errors))
						{
							break;
						}
						ChangedCards.Add(Entry.EnglishName);
					}
				}
				if (Options.bWriteExplanationTemplates
					&& !CreatedPackages.Contains(Entry.EnglishName))
				{
					FWacomCardExplanationTemplateSet DesiredTemplates =
						BuildFireWriteExplanationTemplates(*Card);
					if (!AreExplanationTemplatesEquivalent(
						Card->ExplanationTemplates,
						DesiredTemplates))
					{
						Card->Modify();
						Card->ExplanationTemplates = MoveTemp(DesiredTemplates);
						ChangedCards.Add(Entry.EnglishName);
					}
				}
				ValidateCard(
					Entry.EnglishName,
					*Card,
					Report.Errors);
				if (Options.bCompareSeedDefaults
					|| CreatedPackages.Contains(Entry.EnglishName))
				{
					CompareCardToSeed(
						Entry.EnglishName,
						*Card,
						Cards,
						Report.Errors);
				}
			}
		}

		if (Report.Errors.IsEmpty())
		{
			for (const FSeedEntry& Entry : SeedEntries())
			{
				if (!CreatedPackages.Contains(Entry.EnglishName)
					&& !ChangedCards.Contains(Entry.EnglishName))
				{
					continue;
				}
				UCardDefinition* Card = Cards.FindRef(Entry.EnglishName);
				if (!SaveChangedAsset(
					*Card,
					PackagePath(Entry.EnglishName),
					Report))
				{
					break;
				}
			}
		}

		if (Options.bMigrateLegacyUpgrade && Report.Errors.IsEmpty())
		{
			MigrateLegacyAssets(Report);
		}
		if (Options.bSyncExplanationLexiconDefaults
			&& Report.Errors.IsEmpty())
		{
			SyncExplanationLexiconDefaults(Report);
		}

		for (const FString& Error : Report.Errors)
		{
			UE_LOG(LogTemp, Error, TEXT("[WacomBuildFireWriteCardContent] %s"),
				*Error);
		}
		UE_LOG(LogTemp, Display,
			TEXT("[WacomBuildFireWriteCardContent] Created=%d Existing=%d "
				"Missing=%d Saved=%d Deleted=%d Errors=%d"),
			Report.CreatedCount,
			Report.ExistingCount,
			Report.MissingCount,
			Report.SavedCount,
			Report.DeletedCount,
			Report.Errors.Num());

		const int32 ExitCode = Report.IsOk() ? 0 : 1;
		if (OutReport)
		{
			*OutReport = Report;
		}
		return ExitCode;
	}
}
