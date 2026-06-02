// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/BugGirlBuilder.h"
#include "ContentBuilders/ContentBuilderHelpers.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Cards/CardPassive.h"
#include "Cards/CardPhysique.h"
#include "Cards/CardZoneHook.h"
#include "Characters/CharacterDefinition.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomEnums.h"

namespace
{
	using namespace Wacom::ContentBuilder;

	// ---- 通用 effect 工厂 ----

	FCardEffect Damage(int32 Amount, const FGameplayTag& Target = FGameplayTag())
	{
		FCardEffect E;
		E.EffectType = WacomTags::Effect_Damage;
		E.Magnitude  = Amount;
		E.Target     = Target.IsValid() ? Target : WacomTags::Target_SingleEnemyPart;
		return E;
	}

	FCardEffect ApplyStatus(const FGameplayTag& EffectType, int32 Stacks, int32 Duration,
	                        const FGameplayTag& Target = FGameplayTag())
	{
		FCardEffect E;
		E.EffectType = EffectType;
		E.Magnitude  = Stacks;
		E.Duration   = Duration;
		E.Target     = Target.IsValid() ? Target : WacomTags::Target_SingleEnemyPart;
		return E;
	}

	FCardEffect ApplyPoisonFromCost()
	{
		FCardEffect E;
		E.EffectType       = WacomTags::Effect_ApplyStatus_Poison;
		E.Magnitude        = 0;
		E.MagnitudeSource  = WacomTags::Magnitude_Source_RuntimeCost;
		E.Target           = WacomTags::Target_SingleEnemyPart;
		return E;
	}

	FCardEffect ShieldPlayer(int32 Amount)
	{
		FCardEffect E;
		E.EffectType = WacomTags::Status_Shield;
		E.Magnitude  = Amount;
		E.Target     = WacomTags::Target_Player;
		return E;
	}

	FCardEffect HealPlayer(int32 Amount)
	{
		FCardEffect E;
		E.EffectType = WacomTags::Effect_Heal;
		E.Magnitude  = Amount;
		E.Target     = WacomTags::Target_Player;
		return E;
	}

	FCardEffect DrawFromPile(int32 Count)
	{
		FCardEffect E;
		E.EffectType = WacomTags::Effect_Draw;
		E.Magnitude  = Count;
		E.Target     = WacomTags::Target_Player;
		E.TargetZone = WacomTags::CardLocation_Draw;
		return E;
	}

	FCardEffect DiscardRandomHandCard(int32 Count)
	{
		FCardEffect E;
		E.EffectType = WacomTags::Effect_Discard;
		E.Magnitude  = Count;
		E.Target     = WacomTags::Target_Player;
		return E;
	}

	FCardEffect RemoveEnemyStatus(const FGameplayTag& StatusTag, int32 Stacks)
	{
		FCardEffect E;
		E.EffectType = WacomTags::Effect_RemoveStatus;
		E.Magnitude  = Stacks;
		E.Target     = WacomTags::Target_SingleEnemyPart;
		E.TargetZone = StatusTag;
		return E;
	}

	FCardEffect ModifyEnemyInitiative(int32 Amount)
	{
		FCardEffect E;
		E.EffectType = WacomTags::Effect_ModifyInitiative;
		E.Magnitude  = Amount;
		E.Target     = WacomTags::Target_SingleEnemyPart;
		return E;
	}

	FCardEffect ShuffleRandom()
	{
		FCardEffect E;
		E.EffectType = WacomTags::Effect_Shuffle_Random;
		E.Target     = WacomTags::Target_RandomHandCard;
		return E;
	}

	FCardEffect ShuffleFromBothToOther()
	{
		FCardEffect E;
		E.EffectType = WacomTags::Effect_Shuffle_FromBothToOther;
		E.Target     = WacomTags::Target_ZoneHandCard;
		E.TargetZone = WacomTags::HandZone_Both;
		return E;
	}

	FCardEffect ShuffleSelfToRandomZone()
	{
		FCardEffect E;
		E.EffectType = WacomTags::Effect_Shuffle_ToRandomZone;
		E.Target     = WacomTags::Target_Self;
		return E;
	}

	FCardEffect ModifySelectedHandCardCost(bool bReduceCost, int32 Amount)
	{
		FCardEffect E;
		E.EffectType = bReduceCost
			? WacomTags::Effect_Card_ReduceCost
			: WacomTags::Effect_Card_AddCost;
		E.Magnitude = Amount;
		E.Target = WacomTags::Target_SelectedHandCard;
		return E;
	}

	FCardEffect MoveSelectedHandCardZone(bool bExhaust)
	{
		FCardEffect E;
		E.EffectType = bExhaust
			? WacomTags::Effect_Card_ExhaustSelected
			: WacomTags::Effect_Card_DiscardSelected;
		E.Magnitude = 1;
		E.Target = WacomTags::Target_SelectedHandCard;
		return E;
	}

	FWacomHandCardTargetFilter MakeHandCardTargetFilter(
		bool bAllowNormalHandCards,
		bool bAllowHandAnchors,
		const TArray<FGameplayTag>& RequiredTargetKeywords = {},
		const TArray<FGameplayTag>& BlockedTargetKeywords = {})
	{
		FWacomHandCardTargetFilter Filter;
		Filter.bUseExplicitHandCardTargetFilter = true;
		Filter.bAllowNormalHandCards = bAllowNormalHandCards;
		Filter.bAllowHandAnchors = bAllowHandAnchors;
		for (const FGameplayTag& Tag : RequiredTargetKeywords)
		{
			Filter.RequiredTargetKeywords.AddTag(Tag);
		}
		for (const FGameplayTag& Tag : BlockedTargetKeywords)
		{
			Filter.BlockedTargetKeywords.AddTag(Tag);
		}
		return Filter;
	}

	// ---- 卡 Builder ----

	UCardDefinition* BuildCard(
		const FString& PackagePath,
		FName AssetName,
		FName CardId,
		const FString& DisplayName,
		const FString& Description,
		int32 BaseCost,
		const FGameplayTag& Rarity,
		TArray<FGameplayTag> Keywords,
		ECardTargetMode TargetMode,
		FCardPhysique Physique,
		TArray<FCardEffect> Effects,
		TArray<FCardEffect> PerfectReleaseEffects,
		TArray<FCardZoneHook> ZoneHooks,
		TArray<FCardPassive> Passives,
		const FWacomHandCardTargetFilter& HandCardTargetFilter = FWacomHandCardTargetFilter())
	{
		UPackage* Pkg = FindOrCreatePackage(PackagePath);
		if (!Pkg) { return nullptr; }

		UCardDefinition* Card = CreateOrReplaceAsset<UCardDefinition>(Pkg, AssetName);
		if (!Card) { return nullptr; }

		Card->CardId      = CardId;
		Card->DisplayName = FText::FromString(DisplayName);
		Card->Description = FText::FromString(Description);
		Card->BaseCost    = BaseCost;
		Card->Rarity      = Rarity;
		Card->Keywords.Reset();
		for (const FGameplayTag& Tag : Keywords) { Card->Keywords.AddTag(Tag); }
		Card->TargetMode            = TargetMode;
		Card->HandCardTargetFilter  = HandCardTargetFilter;
		Card->Physique              = Physique;
		Card->Effects               = MoveTemp(Effects);
		Card->PerfectReleaseEffects = MoveTemp(PerfectReleaseEffects);
		Card->ZoneHooks             = MoveTemp(ZoneHooks);
		Card->Passives              = MoveTemp(Passives);

		SaveAssetPackage(Pkg, Card, PackagePath);
		return Card;
	}

	TArray<UCardDefinition*> BuildBugGirlStarterPackCards()
	{
		const FString StarterPackRoot = BugGirlStarterPackCardsRoot();
		TArray<UCardDefinition*> Cards;
		Cards.Reserve(6);

		FCardEffect PoisonNeedleDamage = Damage(4);
		FCardEffect PoisonNeedleBonusDamage = Damage(5);
		PoisonNeedleBonusDamage.Condition.ConditionType = WacomTags::Condition_Target_HasStatus;
		PoisonNeedleBonusDamage.Condition.ParamTag = WacomTags::Status_Poison;
		Cards.Add(BuildCard(
			MakePackagePath(StarterPackRoot, TEXT("DA_Card_Starter_PoisonNeedle")),
			TEXT("DA_Card_Starter_PoisonNeedle"),
			TEXT("Starter.PoisonNeedle"),
			TEXT("毒针"),
			TEXT("造成 4 伤害。\n如果目标有中毒，额外造成 5 伤害。"),
			/*BaseCost*/ 1,
			WacomTags::Card_Rarity_White,
			/*Keywords*/ { WacomTags::Card_Keyword_Weapon },
			ECardTargetMode::SingleEnemyPart,
			FCardPhysique{},
			/*Effects*/ { PoisonNeedleDamage, PoisonNeedleBonusDamage },
			/*PerfectRelease*/ {},
			/*ZoneHooks*/ {},
			/*Passives*/ {}
		));

		Cards.Add(BuildCard(
			MakePackagePath(StarterPackRoot, TEXT("DA_Card_Starter_ChitinWard")),
			TEXT("DA_Card_Starter_ChitinWard"),
			TEXT("Starter.ChitinWard"),
			TEXT("几丁护片"),
			TEXT("获得 5 护盾。\n恢复 2 生命。"),
			/*BaseCost*/ 1,
			WacomTags::Card_Rarity_White,
			/*Keywords*/ { WacomTags::Card_Keyword_Tool },
			ECardTargetMode::None,
			FCardPhysique{},
			/*Effects*/ { ShieldPlayer(5), HealPlayer(2) },
			/*PerfectRelease*/ {},
			/*ZoneHooks*/ {},
			/*Passives*/ {}
		));

		Cards.Add(BuildCard(
			MakePackagePath(StarterPackRoot, TEXT("DA_Card_Starter_AntennaSearch")),
			TEXT("DA_Card_Starter_AntennaSearch"),
			TEXT("Starter.AntennaSearch"),
			TEXT("触须探路"),
			TEXT("抽 2 张牌，随机弃置 1 张普通手牌。"),
			/*BaseCost*/ 0,
			WacomTags::Card_Rarity_White,
			/*Keywords*/ { WacomTags::Card_Keyword_Tool },
			ECardTargetMode::None,
			FCardPhysique{},
			/*Effects*/ { DrawFromPile(2), DiscardRandomHandCard(1) },
			/*PerfectRelease*/ {},
			/*ZoneHooks*/ {},
			/*Passives*/ {}
		));

		Cards.Add(BuildCard(
			MakePackagePath(StarterPackRoot, TEXT("DA_Card_Starter_MoltCut")),
			TEXT("DA_Card_Starter_MoltCut"),
			TEXT("Starter.MoltCut"),
			TEXT("蜕壳切"),
			TEXT("移除目标 1 层减速。\n目标当前先机 -2。"),
			/*BaseCost*/ 1,
			WacomTags::Card_Rarity_Blue,
			/*Keywords*/ { WacomTags::Card_Keyword_Tool },
			ECardTargetMode::SingleEnemyPart,
			FCardPhysique{},
			/*Effects*/ { RemoveEnemyStatus(WacomTags::Status_Slow, 1), ModifyEnemyInitiative(-2) },
			/*PerfectRelease*/ {},
			/*ZoneHooks*/ {},
			/*Passives*/ {}
		));

		FCardPassive HuskOnDiscard;
		HuskOnDiscard.Trigger = WacomTags::Passive_Trigger_OnDiscard;
		HuskOnDiscard.DisplayText = FText::FromString(TEXT("被弃置时：获得 4 护盾。"));
		HuskOnDiscard.Effects = { ShieldPlayer(4) };
		Cards.Add(BuildCard(
			MakePackagePath(StarterPackRoot, TEXT("DA_Card_Starter_LightHusk")),
			TEXT("DA_Card_Starter_LightHusk"),
			TEXT("Starter.LightHusk"),
			TEXT("轻蜕壳"),
			TEXT("被弃置时：获得 4 护盾。"),
			/*BaseCost*/ 0,
			WacomTags::Card_Rarity_White,
			/*Keywords*/ { WacomTags::Card_Keyword_Companion },
			ECardTargetMode::None,
			FCardPhysique{},
			/*Effects*/ {},
			/*PerfectRelease*/ {},
			/*ZoneHooks*/ {},
			/*Passives*/ { HuskOnDiscard }
		));

		FCardZoneHook SilklineLeftHook;
		SilklineLeftHook.Zone = WacomTags::HandZone_Left;
		SilklineLeftHook.Trigger = WacomTags::ZoneHook_Trigger_OnPerfectReleaseHit;
		Cards.Add(BuildCard(
			MakePackagePath(StarterPackRoot, TEXT("DA_Card_Starter_SilklineFeint")),
			TEXT("DA_Card_Starter_SilklineFeint"),
			TEXT("Starter.SilklineFeint"),
			TEXT("丝线佯攻"),
			TEXT("造成 3 伤害。\n处于左手区时：完美释放不推进目标先机。"),
			/*BaseCost*/ 1,
			WacomTags::Card_Rarity_White,
			/*Keywords*/ { WacomTags::Card_Keyword_Weapon },
			ECardTargetMode::SingleEnemyPart,
			FCardPhysique{},
			/*Effects*/ { Damage(3) },
			/*PerfectRelease*/ {},
			/*ZoneHooks*/ { SilklineLeftHook },
			/*Passives*/ {}
		));

		return Cards;
	}
}

namespace Wacom::ContentBuilder
{
	UCharacterDefinition* BuildBugGirlContent()
	{
		const FString BugGirlCards = BugGirlCardsRoot();

		// ==== 左手 ====
		// 当前左手主动效果和完美释放效果留空；打出后进入 Limbo，保留由手牌队列阶段处理。
		UCardDefinition* LeftHand = BuildCard(
			MakePackagePath(BugGirlCards, TEXT("DA_Card_LeftHand")),
			TEXT("DA_Card_LeftHand"),
			TEXT("LeftHand"),
			TEXT("左手"),
			TEXT("完美释放：闪避敌人攻击意图。\n如果左方没有卡牌，从背包随机抽取一张牌并使其迅捷。\n回合结束时：双手皆在手牌中则保留其区间内的手牌。"),
			/*BaseCost*/ 2,
			WacomTags::Card_Rarity_Intrinsic,
			/*Keywords*/ { WacomTags::Card_Keyword_Hand, WacomTags::Card_Keyword_Weapon, WacomTags::Card_Keyword_Tool },
			ECardTargetMode::None,
			FCardPhysique{},
			/*Effects*/ {},
			/*PerfectRelease*/ {},
			/*ZoneHooks*/ {},
			/*Passives*/ {}
		);

		// ==== 右手 ====
		// 主动：造成 8 伤害。"相邻右方伙伴代打"仍待正式规则。
		UCardDefinition* RightHand = BuildCard(
			MakePackagePath(BugGirlCards, TEXT("DA_Card_RightHand")),
			TEXT("DA_Card_RightHand"),
			TEXT("RightHand"),
			TEXT("右手"),
			TEXT("造成 8 伤害。\n如果相邻右方有伙伴，则改为使用它并减少 1 点费用，随后腾挪到左手。\n回合结束时：双手皆在手牌中则保留其区间内的手牌。"),
			/*BaseCost*/ 2,
			WacomTags::Card_Rarity_Intrinsic,
			/*Keywords*/ { WacomTags::Card_Keyword_Hand, WacomTags::Card_Keyword_Weapon, WacomTags::Card_Keyword_Tool },
			ECardTargetMode::SingleEnemyPart,
			FCardPhysique{},
			/*Effects*/ { Damage(8) },
			/*PerfectRelease*/ {},
			/*ZoneHooks*/ {},
			/*Passives*/ {}
		);

		// ==== 朝光暮蝶 Zhaoguang Mudie ====
		FCardPhysique ZMPhysique; ZMPhysique.MaxHpBonus = 1;

		// ZoneHook：左手区 + OnPerfectReleaseHit = 不推进先机。
		// ExtraEffects 留空——"跳过先机推进"由 PlayCardResolver 在 Hook 存在即触发。
		FCardZoneHook ZM_Left;
		ZM_Left.Zone    = WacomTags::HandZone_Left;
		ZM_Left.Trigger = WacomTags::ZoneHook_Trigger_OnPerfectReleaseHit;

		// ZoneHook：右手区 + OnPlay = 费用转移。
		// 组合三条通用效果：
		//   [0] Shuffle.Random           → 随机腾挪一张手牌，写 LastShuffledCardId
		//   [1] Card.ReduceCost Mag=1    → 对被腾挪卡 -1 RuntimeCostModifier
		//   [2] Card.AddCost    Mag=1    → 对本卡 +1 RuntimeCostModifier（可叠加）
		FCardEffect ZH_Shuffle; ZH_Shuffle.EffectType = WacomTags::Effect_Shuffle_Random;
		ZH_Shuffle.Target                           = WacomTags::Target_RandomHandCard;

		FCardEffect ZH_ReduceCostOnShuffled;
		ZH_ReduceCostOnShuffled.EffectType = WacomTags::Effect_Card_ReduceCost;
		ZH_ReduceCostOnShuffled.Magnitude  = 1;
		ZH_ReduceCostOnShuffled.Target     = WacomTags::Target_LastShuffledCard;

		FCardEffect ZH_AddCostOnSelf;
		ZH_AddCostOnSelf.EffectType = WacomTags::Effect_Card_AddCost;
		ZH_AddCostOnSelf.Magnitude  = 1;
		ZH_AddCostOnSelf.Target     = WacomTags::Target_Self;

		FCardZoneHook ZM_Right;
		ZM_Right.Zone    = WacomTags::HandZone_Right;
		ZM_Right.Trigger = WacomTags::ZoneHook_Trigger_OnPlay;
		ZM_Right.ExtraEffects = { ZH_Shuffle, ZH_ReduceCostOnShuffled, ZH_AddCostOnSelf };

		UCardDefinition* Zhaoguang = BuildCard(
			MakePackagePath(BugGirlCards, TEXT("DA_Card_ZhaoguangMudie")),
			TEXT("DA_Card_ZhaoguangMudie"),
			TEXT("ZhaoguangMudie"),
			TEXT("朝光暮蝶"),
			TEXT("随机腾挪 1 张我方卡牌。\n施加等于此卡当前费用的中毒。\n处于左手区时：完美释放使此卡获得迅捷。\n处于右手区时：通过此卡腾挪的卡牌费用 -1，减少的费用转移给本卡。"),
			/*BaseCost*/ 0,
			WacomTags::Card_Rarity_White,
			/*Keywords*/ { WacomTags::Card_Keyword_Companion },
			ECardTargetMode::SingleEnemyPart,
			ZMPhysique,
			/*Effects*/ { ShuffleRandom(), ApplyPoisonFromCost() },
			/*PerfectRelease*/ {},
			/*ZoneHooks*/ { ZM_Left, ZM_Right },
			/*Passives*/ {}
		);

		// ==== 拂晓飞蛾 Fuxiao Feie ====
		FCardPhysique FFPhysique; FFPhysique.MaxHpBonus = 1;
		FCardPassive FF_OnCompanion;
		FF_OnCompanion.Trigger          = WacomTags::Passive_Trigger_OnCompanionCount;
		FF_OnCompanion.TriggerThreshold = 3;
		FF_OnCompanion.DisplayText      = FText::FromString(TEXT("每当你打出 3 张伙伴时，使此牌回到手中。"));

		UCardDefinition* Fuxiao = BuildCard(
			MakePackagePath(BugGirlCards, TEXT("DA_Card_FuxiaoFeie")),
			TEXT("DA_Card_FuxiaoFeie"),
			TEXT("FuxiaoFeie"),
			TEXT("拂晓飞蛾"),
			TEXT("对一个敌方部位施加 1 减速。"),
			/*BaseCost*/ 0,
			WacomTags::Card_Rarity_Blue,
			/*Keywords*/ { WacomTags::Card_Keyword_Companion },
			ECardTargetMode::SingleEnemyPart,
			FFPhysique,
			/*Effects*/ { ApplyStatus(WacomTags::Effect_ApplyStatus_Slow, 1, 0) },
			/*PerfectRelease*/ {},
			/*ZoneHooks*/ {},
			/*Passives*/ { FF_OnCompanion }
		);

		// ==== 赤腹工蚁 Chifu Gongyi ====
		FCardPhysique CGPhysique; CGPhysique.MaxHpBonus = 1;

		UCardDefinition* Chifu = BuildCard(
			MakePackagePath(BugGirlCards, TEXT("DA_Card_ChifuGongyi")),
			TEXT("DA_Card_ChifuGongyi"),
			TEXT("ChifuGongyi"),
			TEXT("赤腹工蚁"),
			TEXT("保留。\n将双手区的随机 1 张卡牌腾挪至其他区域。"),
			/*BaseCost*/ 0,
			WacomTags::Card_Rarity_White,
			/*Keywords*/ { WacomTags::Card_Keyword_Companion, WacomTags::Card_Keyword_Retain },
			ECardTargetMode::None,
			CGPhysique,
			/*Effects*/ { ShuffleFromBothToOther() },
			/*PerfectRelease*/ {},
			/*ZoneHooks*/ {},
			/*Passives*/ {}
		);

		// ==== 烁光蝶 Shuoguang Die ====
		FCardPhysique SDPhysique; SDPhysique.MaxHpBonus = 6;
		FCardPassive SD_AfterPlayed;
		SD_AfterPlayed.Trigger = WacomTags::Passive_Trigger_AfterPlayed;
		SD_AfterPlayed.Effects = { ShuffleSelfToRandomZone() };
		SD_AfterPlayed.DisplayText = FText::FromString(TEXT("打出后：此牌腾挪至随机区域。"));

		UCardDefinition* Shuoguang = BuildCard(
			MakePackagePath(BugGirlCards, TEXT("DA_Card_ShuoguangDie")),
			TEXT("DA_Card_ShuoguangDie"),
			TEXT("ShuoguangDie"),
			TEXT("烁光蝶"),
			TEXT("连击。\n造成 7 伤害。"),
			/*BaseCost*/ 1,
			WacomTags::Card_Rarity_White,
			/*Keywords*/ { WacomTags::Card_Keyword_Companion, WacomTags::Card_Keyword_Weapon, WacomTags::Card_Keyword_Combo },
			ECardTargetMode::SingleEnemyPart,
			SDPhysique,
			/*Effects*/ { Damage(7) },
			/*PerfectRelease*/ {},
			/*ZoneHooks*/ {},
			/*Passives*/ { SD_AfterPlayed }
		);

		// ==== 暮蛉 Muling ====
		FCardPhysique MLPhysique; MLPhysique.MaxHpBonus = 23;
		FCardPassive ML_OnTwilight;
		ML_OnTwilight.Trigger = WacomTags::Passive_Trigger_OnTwilightTriggered;
		ML_OnTwilight.DisplayText = FText::FromString(TEXT("当触发暮气时，使一张中毒卡牌效果 +1。"));

		UCardDefinition* Muling = BuildCard(
			MakePackagePath(BugGirlCards, TEXT("DA_Card_Muling")),
			TEXT("DA_Card_Muling"),
			TEXT("Muling"),
			TEXT("暮蛉"),
			TEXT("突袭。\n冻结一个敌方部位 1 回合。"),
			/*BaseCost*/ 5,
			WacomTags::Card_Rarity_Blue,
			/*Keywords*/ { WacomTags::Card_Keyword_Companion },
			ECardTargetMode::SingleEnemyPart,
			MLPhysique,
			/*Effects*/ { ApplyStatus(WacomTags::Effect_ApplyStatus_Freeze, 1, 1) },
			/*PerfectRelease*/ {},
			/*ZoneHooks*/ {},
			/*Passives*/ { ML_OnTwilight }
		);

		// ==== 虫妹的小布袋 BugGirlBag（基础容器卡）====
		// 背包能力提供者，Capacity = 12。
		// 自身打出无意义但合法（Cost=0，无主动效果）。
		// 玩家可以选择把它放进备战卡组（Initialize a2：默认只放 Backpack 不进 BattleDeck）。
		FCardPhysique BagPhysique;
		BagPhysique.Capacity = 12;

		UCardDefinition* BugGirlBag = BuildCard(
			MakePackagePath(BugGirlCards, TEXT("DA_Card_BugGirlBag")),
			TEXT("DA_Card_BugGirlBag"),
			TEXT("BugGirlBag"),
			TEXT("虫妹的小布袋"),
			TEXT("提供背包容量。\n背包中至少有一张背包能力提供卡时，背包界面可用。"),
			/*BaseCost*/ 0,
			WacomTags::Card_Rarity_White,
			/*Keywords*/ { WacomTags::Card_Keyword_BagProvider },
			ECardTargetMode::None,
			BagPhysique,
			/*Effects*/ {},
			/*PerfectRelease*/ {},
			/*ZoneHooks*/ {},
			/*Passives*/ {}
		);

		// ==== 蛛茧绒囊 ZhujianRongnang（B 类容器卡）====
		// 演示 B 类容器卡机制和首个具体 CapacityEffect。
		//   Capacity = 3 → 不计入通量公式（CapacityEffect 非空）
		//   特殊存放区容量 = Capacity - 1 = 2
		//   CapacityEffect = Card.CapacityEffect.WeaponDamagePlus3
		//     → 放进特殊存放区且 bBattleEnabledInSpecialZone=true 的"武器关键词卡"入战伤害 +3
		// 玩家手上同时有小布袋（A，12）+ 蛛茧绒囊（B，3）时：
		//   Flux 仍 = 12，蛛茧绒囊不增加通量
		FCardPhysique CocoonPhysique;
		CocoonPhysique.Capacity       = 3;
		CocoonPhysique.CapacityEffect = WacomTags::Card_CapacityEffect_WeaponDamagePlus3;

		UCardDefinition* ZhujianRongnang = BuildCard(
			MakePackagePath(BugGirlCards, TEXT("DA_Card_ZhujianRongnang")),
			TEXT("DA_Card_ZhujianRongnang"),
			TEXT("ZhujianRongnang"),
			TEXT("蛛茧绒囊"),
			TEXT("B 类容器卡，开启一个特殊存放区，内容容量为 2。\n放入其中且选择入战的武器卡，战斗中伤害 +3。"),
			/*BaseCost*/ 0,
			WacomTags::Card_Rarity_White,
			/*Keywords*/ {}, // 不带 BagProvider，专门测 B 类与 BagProvider 互相独立
			ECardTargetMode::None,
			CocoonPhysique,
			/*Effects*/ {},
			/*PerfectRelease*/ {},
			/*ZoneHooks*/ {},
			/*Passives*/ {}
		);

		// ==== 暮色引虫灯 MuseiYinchongdeng（删牌能力提供者）====
		// 背包删牌能力的"卡牌承载体"。
		//   Capacity = 3，A 类容器卡（无 CapacityEffect）→ 计入 Flux 公式
		//   关键词：Card.Keyword.DeleteProvider（玩家持有区至少一张就有删牌功能）
		//   不带 BagProvider，与小布袋职责正交
		//   原型规则：Run 初始化时默认进入 BattleDeck，但仍贡献通量容量。
		// 当前简化：
		//   - 不读耐久，自身打出无意义但合法（Cost=0，无主动效果）
		//   - 删牌区始终显示，不绑定具体卡
		//   - 耐久和战斗主动效果暂不接入
		FCardPhysique LanternPhysique;
		LanternPhysique.Capacity = 3;

		UCardDefinition* MuseiLantern = BuildCard(
			MakePackagePath(BugGirlCards, TEXT("DA_Card_MuseiYinchongdeng")),
			TEXT("DA_Card_MuseiYinchongdeng"),
			TEXT("MuseiYinchongdeng"),
			TEXT("暮色引虫灯"),
			TEXT("A 类容器卡，容量为 3。\n携带删牌能力。\n第一阶段暂不处理耐久与战斗主动效果。"),
			/*BaseCost*/ 0,
			WacomTags::Card_Rarity_White,
			/*Keywords*/ { WacomTags::Card_Keyword_DeleteProvider },
			ECardTargetMode::None,
			LanternPhysique,
			/*Effects*/ {},
			/*PerfectRelease*/ {},
			/*ZoneHooks*/ {},
			/*Passives*/ {}
		);

		// ==== 调试钥匙 DebugKey ====
		// V0-BU 原型：探索期拖到调试宝箱上，走 Run world card drop receiver 事务。
		UCardDefinition* DebugKey = BuildCard(
			MakePackagePath(BugGirlCards, TEXT("DA_Card_DebugKey")),
			TEXT("DA_Card_DebugKey"),
			TEXT("DebugKey"),
			TEXT("钥匙"),
			TEXT("调试用工具卡：拖到调试宝箱上可打开宝箱。"),
			/*BaseCost*/ 0,
			WacomTags::Card_Rarity_White,
			/*Keywords*/ { WacomTags::Card_Keyword_Tool },
			ECardTargetMode::None,
			FCardPhysique{},
			/*Effects*/ {},
			/*PerfectRelease*/ {},
			/*ZoneHooks*/ {},
			/*Passives*/ {}
		);

		// ==== V0-AD 卡对卡测试卡 ====
		// 用于 PIE 验证：拖到另一张 first-person 手牌上，命中 Target.SelectedHandCard。
		UCardDefinition* TestAddCostToSelectedHand = BuildCard(
			MakePackagePath(BugGirlCards, TEXT("DA_Card_Test_AddCostToSelectedHand")),
			TEXT("DA_Card_Test_AddCostToSelectedHand"),
			TEXT("Test.AddCostToSelectedHand"),
			TEXT("加费测试"),
			TEXT("测试卡：拖到另一张手牌上，使目标卡费用 +2。"),
			/*BaseCost*/ 0,
			WacomTags::Card_Rarity_White,
			/*Keywords*/ {},
			ECardTargetMode::HandCard,
			FCardPhysique{},
			/*Effects*/ { ModifySelectedHandCardCost(/*bReduceCost*/ false, 2) },
			/*PerfectRelease*/ {},
			/*ZoneHooks*/ {},
			/*Passives*/ {},
			MakeHandCardTargetFilter(/*bAllowNormalHandCards*/ true, /*bAllowHandAnchors*/ true)
		);

		UCardDefinition* TestReduceCostToSelectedHand = BuildCard(
			MakePackagePath(BugGirlCards, TEXT("DA_Card_Test_ReduceCostToSelectedHand")),
			TEXT("DA_Card_Test_ReduceCostToSelectedHand"),
			TEXT("Test.ReduceCostToSelectedHand"),
			TEXT("减费测试"),
			TEXT("测试卡：拖到另一张手牌上，使目标卡费用 -1。"),
			/*BaseCost*/ 0,
			WacomTags::Card_Rarity_White,
			/*Keywords*/ {},
			ECardTargetMode::HandCard,
			FCardPhysique{},
			/*Effects*/ { ModifySelectedHandCardCost(/*bReduceCost*/ true, 1) },
			/*PerfectRelease*/ {},
			/*ZoneHooks*/ {},
			/*Passives*/ {},
			MakeHandCardTargetFilter(/*bAllowNormalHandCards*/ true, /*bAllowHandAnchors*/ true)
		);

		UCardDefinition* TestTargetCost3 = BuildCard(
			MakePackagePath(BugGirlCards, TEXT("DA_Card_Test_TargetCost3")),
			TEXT("DA_Card_Test_TargetCost3"),
			TEXT("Test.TargetCost3"),
			TEXT("目标卡 3费"),
			TEXT("测试目标卡：用于验证被 Target.SelectedHandCard 精确命中后的费用刷新、弃置或消耗。"),
			/*BaseCost*/ 3,
			WacomTags::Card_Rarity_White,
			/*Keywords*/ {},
			ECardTargetMode::None,
			FCardPhysique{},
			/*Effects*/ {},
			/*PerfectRelease*/ {},
			/*ZoneHooks*/ {},
			/*Passives*/ {}
		);

		UCardDefinition* TestTargetCompanion = BuildCard(
			MakePackagePath(BugGirlCards, TEXT("DA_Card_Test_TargetCompanion")),
			TEXT("DA_Card_Test_TargetCompanion"),
			TEXT("Test.TargetCompanion"),
			TEXT("目标伙伴测试"),
			TEXT("测试目标卡：带伙伴关键词，用于验证 HandCard 关键词筛选。"),
			/*BaseCost*/ 1,
			WacomTags::Card_Rarity_White,
			/*Keywords*/ { WacomTags::Card_Keyword_Companion },
			ECardTargetMode::None,
			FCardPhysique{},
			/*Effects*/ {},
			/*PerfectRelease*/ {},
			/*ZoneHooks*/ {},
			/*Passives*/ {}
		);

		UCardDefinition* TestRequireCompanionTarget = BuildCard(
			MakePackagePath(BugGirlCards, TEXT("DA_Card_Test_RequireCompanionTarget")),
			TEXT("DA_Card_Test_RequireCompanionTarget"),
			TEXT("Test.RequireCompanionTarget"),
			TEXT("只作用伙伴测试"),
			TEXT("测试卡：只能拖到带伙伴关键词的手牌上，使目标卡费用 +1。"),
			/*BaseCost*/ 0,
			WacomTags::Card_Rarity_White,
			/*Keywords*/ {},
			ECardTargetMode::HandCard,
			FCardPhysique{},
			/*Effects*/ { ModifySelectedHandCardCost(/*bReduceCost*/ false, 1) },
			/*PerfectRelease*/ {},
			/*ZoneHooks*/ {},
			/*Passives*/ {},
			MakeHandCardTargetFilter(
				/*bAllowNormalHandCards*/ true,
				/*bAllowHandAnchors*/ true,
				/*RequiredTargetKeywords*/ { WacomTags::Card_Keyword_Companion })
		);

		UCardDefinition* TestBlockWeaponTarget = BuildCard(
			MakePackagePath(BugGirlCards, TEXT("DA_Card_Test_BlockWeaponTarget")),
			TEXT("DA_Card_Test_BlockWeaponTarget"),
			TEXT("Test.BlockWeaponTarget"),
			TEXT("不能作用武器测试"),
			TEXT("测试卡：不能拖到带武器关键词的手牌上，其他合法手牌目标费用 +1。"),
			/*BaseCost*/ 0,
			WacomTags::Card_Rarity_White,
			/*Keywords*/ {},
			ECardTargetMode::HandCard,
			FCardPhysique{},
			/*Effects*/ { ModifySelectedHandCardCost(/*bReduceCost*/ false, 1) },
			/*PerfectRelease*/ {},
			/*ZoneHooks*/ {},
			/*Passives*/ {},
			MakeHandCardTargetFilter(
				/*bAllowNormalHandCards*/ true,
				/*bAllowHandAnchors*/ true,
				/*RequiredTargetKeywords*/ {},
				/*BlockedTargetKeywords*/ { WacomTags::Card_Keyword_Weapon })
		);

		// ==== V0-AE 指定手牌弃置 / 消耗测试卡 ====
		UCardDefinition* TestDiscardSelectedHandCard = BuildCard(
			MakePackagePath(BugGirlCards, TEXT("DA_Card_Test_DiscardSelectedHandCard")),
			TEXT("DA_Card_Test_DiscardSelectedHandCard"),
			TEXT("Test.DiscardSelectedHandCard"),
			TEXT("弃置目标手牌测试"),
			TEXT("测试卡：拖到另一张普通手牌上，使目标卡进入弃牌堆。不能选择左右手。"),
			/*BaseCost*/ 0,
			WacomTags::Card_Rarity_White,
			/*Keywords*/ {},
			ECardTargetMode::HandCard,
			FCardPhysique{},
			/*Effects*/ { MoveSelectedHandCardZone(/*bExhaust*/ false) },
			/*PerfectRelease*/ {},
			/*ZoneHooks*/ {},
			/*Passives*/ {},
			MakeHandCardTargetFilter(/*bAllowNormalHandCards*/ true, /*bAllowHandAnchors*/ false)
		);

		UCardDefinition* TestExhaustSelectedHandCard = BuildCard(
			MakePackagePath(BugGirlCards, TEXT("DA_Card_Test_ExhaustSelectedHandCard")),
			TEXT("DA_Card_Test_ExhaustSelectedHandCard"),
			TEXT("Test.ExhaustSelectedHandCard"),
			TEXT("消耗目标手牌测试"),
			TEXT("测试卡：拖到另一张普通手牌上，使目标卡进入消耗区。不能选择左右手。"),
			/*BaseCost*/ 0,
			WacomTags::Card_Rarity_White,
			/*Keywords*/ {},
			ECardTargetMode::HandCard,
			FCardPhysique{},
			/*Effects*/ { MoveSelectedHandCardZone(/*bExhaust*/ true) },
			/*PerfectRelease*/ {},
			/*ZoneHooks*/ {},
			/*Passives*/ {},
			MakeHandCardTargetFilter(/*bAllowNormalHandCards*/ true, /*bAllowHandAnchors*/ false)
		);

		TArray<UCardDefinition*> StarterPackCards = BuildBugGirlStarterPackCards();

		// 检查：任一建造失败则放弃。
		if (!LeftHand || !RightHand || !Zhaoguang || !Fuxiao || !Chifu || !Shuoguang || !Muling || !BugGirlBag || !ZhujianRongnang || !MuseiLantern
			|| !DebugKey
			|| !TestAddCostToSelectedHand || !TestReduceCostToSelectedHand || !TestTargetCost3
			|| !TestTargetCompanion || !TestRequireCompanionTarget || !TestBlockWeaponTarget
			|| !TestDiscardSelectedHandCard || !TestExhaustSelectedHandCard
			|| StarterPackCards.Num() != 6 || StarterPackCards.Contains(nullptr))
		{
			return nullptr;
		}

		// ==== 角色 ====
		const FString CharPkgPath = MakePackagePath(CharactersRoot(), TEXT("DA_Character_BugGirl"));
		UPackage* CharPkg = FindOrCreatePackage(CharPkgPath);
		if (!CharPkg) { return nullptr; }

		UCharacterDefinition* Char = CreateOrReplaceAsset<UCharacterDefinition>(CharPkg, TEXT("DA_Character_BugGirl"));
		if (!Char) { return nullptr; }

		Char->CharacterId    = TEXT("BugGirl");
		Char->DisplayName    = FText::FromString(TEXT("Bug Girl"));
		// 默认 10 指 × 2 HP = 20 本体 HP。和重构前 BaseMaxHp = 20 一致。
		Char->FingerCount    = 10;
		Char->HpPerFinger    = 2;
		Char->LeftHandCard   = LeftHand;
		Char->RightHandCard  = RightHand;
		// 顺序：5 张参战伙伴卡 + V0-AD 卡对卡测试卡 + 虫妹的小布袋（A 类）+ 蛛茧绒囊（B 类占位）+ 暮色引虫灯（A 类，DeleteProvider）。
		// Initialize 时：非容器卡进 BattleDeck，容器卡默认进 Backpack；
		// 暮色引虫灯按原型特例默认进 BattleDeck。
		Char->StarterDeck    = {
			Zhaoguang,
			Fuxiao,
			Chifu,
			Shuoguang,
			Muling,
			TestAddCostToSelectedHand,
			TestReduceCostToSelectedHand,
			TestDiscardSelectedHandCard,
			TestExhaustSelectedHandCard,
			TestTargetCost3,
			DebugKey,
			BugGirlBag,
			ZhujianRongnang,
			MuseiLantern
		};
		SaveAssetPackage(CharPkg, Char, CharPkgPath);
		return Char;
	}
}
