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
		E.EffectType               = WacomTags::Effect_ApplyStatus_Poison;
		E.Magnitude                = 0;
		E.bMagnitudeFromRuntimeCost = true;
		E.Target                   = WacomTags::Target_SingleEnemyPart;
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
		TArray<FCardPassive> Passives)
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
		Card->Physique              = Physique;
		Card->Effects               = MoveTemp(Effects);
		Card->PerfectReleaseEffects = MoveTemp(PerfectReleaseEffects);
		Card->ZoneHooks             = MoveTemp(ZoneHooks);
		Card->Passives              = MoveTemp(Passives);

		SaveAssetPackage(Pkg, Card, PackagePath);
		return Card;
	}
}

namespace Wacom::ContentBuilder
{
	UCharacterDefinition* BuildBugGirlContent()
	{
		const FString CardsRoot = TEXT("/Game/Wacom/Cards/BugGirl/");

		// ==== 左手 ====
		// Data_Schema_Draft §8：第一阶段左手主动效果和完美释放效果留空，
		// 仅实现"合法可打出、打出后不入任何区域、保留关键字由手牌队列阶段处理"。
		UCardDefinition* LeftHand = BuildCard(
			CardsRoot + TEXT("DA_Card_LeftHand"),
			TEXT("DA_Card_LeftHand"),
			TEXT("LeftHand"),
			TEXT("Left Hand"),
			TEXT("Anchor card for hand zone. Details TBD."),
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
		// 主动：造成 8 伤害。其他"相邻右方伙伴代打"第一阶段不实现。
		UCardDefinition* RightHand = BuildCard(
			CardsRoot + TEXT("DA_Card_RightHand"),
			TEXT("DA_Card_RightHand"),
			TEXT("RightHand"),
			TEXT("Right Hand"),
			TEXT("Anchor card. Deal 8 damage."),
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
		// Zone hooks 第一阶段只建占位（S8+ 才消费）。
		FCardZoneHook ZM_Left;
		ZM_Left.Zone    = WacomTags::HandZone_Left;
		ZM_Left.Trigger = WacomTags::ZoneHook_Trigger_OnPerfectReleaseHit;
		FCardZoneHook ZM_Right;
		ZM_Right.Zone    = WacomTags::HandZone_Right;
		ZM_Right.Trigger = WacomTags::ZoneHook_Trigger_OnPlay;

		UCardDefinition* Zhaoguang = BuildCard(
			CardsRoot + TEXT("DA_Card_ZhaoguangMudie"),
			TEXT("DA_Card_ZhaoguangMudie"),
			TEXT("ZhaoguangMudie"),
			TEXT("Zhaoguang Mudie"),
			TEXT("Random shuffle 1 card; apply poison equal to current cost."),
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

		UCardDefinition* Fuxiao = BuildCard(
			CardsRoot + TEXT("DA_Card_FuxiaoFeie"),
			TEXT("DA_Card_FuxiaoFeie"),
			TEXT("FuxiaoFeie"),
			TEXT("Fuxiao Feie"),
			TEXT("Apply 1 slow to an enemy part."),
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
			CardsRoot + TEXT("DA_Card_ChifuGongyi"),
			TEXT("DA_Card_ChifuGongyi"),
			TEXT("ChifuGongyi"),
			TEXT("Chifu Gongyi"),
			TEXT("Retain. Shuffle a random card from the Both zone to another zone."),
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

		UCardDefinition* Shuoguang = BuildCard(
			CardsRoot + TEXT("DA_Card_ShuoguangDie"),
			TEXT("DA_Card_ShuoguangDie"),
			TEXT("ShuoguangDie"),
			TEXT("Shuoguang Die"),
			TEXT("Combo. Deal 7 damage. After play, shuffle to a random zone."),
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

		UCardDefinition* Muling = BuildCard(
			CardsRoot + TEXT("DA_Card_Muling"),
			TEXT("DA_Card_Muling"),
			TEXT("Muling"),
			TEXT("Muling"),
			TEXT("Freeze an enemy part for 1 turn."),
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

		// 检查：任一建造失败则放弃。
		if (!LeftHand || !RightHand || !Zhaoguang || !Fuxiao || !Chifu || !Shuoguang || !Muling)
		{
			return nullptr;
		}

		// ==== 角色 ====
		const FString CharPkgPath = TEXT("/Game/Wacom/Characters/DA_Character_BugGirl");
		UPackage* CharPkg = FindOrCreatePackage(CharPkgPath);
		if (!CharPkg) { return nullptr; }

		UCharacterDefinition* Char = CreateOrReplaceAsset<UCharacterDefinition>(CharPkg, TEXT("DA_Character_BugGirl"));
		if (!Char) { return nullptr; }

		Char->CharacterId    = TEXT("BugGirl");
		Char->DisplayName    = FText::FromString(TEXT("Bug Girl"));
		Char->BaseMaxHp      = 20;
		Char->LeftHandCard   = LeftHand;
		Char->RightHandCard  = RightHand;
		Char->StarterDeck    = { Zhaoguang, Fuxiao, Chifu, Shuoguang, Muling };

		SaveAssetPackage(CharPkg, Char, CharPkgPath);
		return Char;
	}
}
