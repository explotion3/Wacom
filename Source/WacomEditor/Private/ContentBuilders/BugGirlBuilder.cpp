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
			TEXT("左手"),
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
			TEXT("右手"),
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

		// ZoneHook：左手区 + OnPerfectReleaseHit = 不推进先机。P3.3 起生效。
		// ExtraEffects 留空——"跳过先机推进"由 PlayCardResolver 在 Hook 存在即触发。
		FCardZoneHook ZM_Left;
		ZM_Left.Zone    = WacomTags::HandZone_Left;
		ZM_Left.Trigger = WacomTags::ZoneHook_Trigger_OnPerfectReleaseHit;

		// ZoneHook：右手区 + OnPlay = 费用转移。P3.3 起生效。
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
			CardsRoot + TEXT("DA_Card_ZhaoguangMudie"),
			TEXT("DA_Card_ZhaoguangMudie"),
			TEXT("ZhaoguangMudie"),
			TEXT("朝光暮蝶"),
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
			TEXT("拂晓飞蛾"),
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
			TEXT("赤腹工蚁"),
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
			TEXT("烁光蝶"),
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
			TEXT("暮蛉"),
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

		// ==== 虫妹的小布袋 BugGirlBag（基础容器卡，GDD §11）====
		// Stage 4.0 / 4.1 引入：背包能力提供者，Capacity = 12。
		// 自身打出无意义但合法（Cost=0，无主动效果）。
		// 玩家可以选择把它放进备战卡组（Initialize a2：默认只放 Backpack 不进 BattleDeck）。
		FCardPhysique BagPhysique;
		BagPhysique.Capacity = 12;

		UCardDefinition* BugGirlBag = BuildCard(
			CardsRoot + TEXT("DA_Card_BugGirlBag"),
			TEXT("DA_Card_BugGirlBag"),
			TEXT("BugGirlBag"),
			TEXT("虫妹的小布袋"),
			TEXT("Provides backpack capacity. Bag UI is unlocked while at least one BagProvider card is in the backpack."),
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

		// ==== 蛛茧绒囊 ZhujianRongnang（B 类容器卡，GDD §11.2 / Stage 4.5.2）====
		// Stage 4.3 引入：演示 B 类容器卡机制；Stage 4.5.2 把 CapacityEffect 从占位升级为首个具体效果。
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
			CardsRoot + TEXT("DA_Card_ZhujianRongnang"),
			TEXT("DA_Card_ZhujianRongnang"),
			TEXT("ZhujianRongnang"),
			TEXT("蛛茧绒囊"),
			TEXT("Type-B container card. Opens a special storage zone (capacity = 2)."
			     " Weapon cards stored there gain +3 damage when battle-enabled."),
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

		// ==== 暮色引虫灯 MuseiYinchongdeng（删牌能力提供者，GDD §4.4 / §11.5 / §11.7）====
		// Stage 4.4 引入：背包删牌能力的"卡牌承载体"。
		//   Capacity = 3，A 类容器卡（无 CapacityEffect）→ 计入 Flux 公式
		//   关键词：Card.Keyword.DeleteProvider（背包至少一张就有删牌功能）
		//   不带 BagProvider，与小布袋职责正交
		// 第一阶段简化：
		//   - 不读耐久，自身打出无意义但合法（Cost=0，无主动效果）
		//   - 删牌区始终显示，不绑定具体卡（GDD §11.7）
		//   - 任务后升级远期不做
		FCardPhysique LanternPhysique;
		LanternPhysique.Capacity = 3;

		UCardDefinition* MuseiLantern = BuildCard(
			CardsRoot + TEXT("DA_Card_MuseiYinchongdeng"),
			TEXT("DA_Card_MuseiYinchongdeng"),
			TEXT("MuseiYinchongdeng"),
			TEXT("暮色引虫灯"),
			TEXT("Type-A container with capacity 3. Carries the delete-card ability"
			     " (DeleteProvider keyword). Stage 1 ignores durability and combat effects."),
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

		// 检查：任一建造失败则放弃。
		if (!LeftHand || !RightHand || !Zhaoguang || !Fuxiao || !Chifu || !Shuoguang || !Muling || !BugGirlBag || !ZhujianRongnang || !MuseiLantern)
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
		// 默认 10 指 × 2 HP = 20 本体 HP。和重构前 BaseMaxHp = 20 一致。
		Char->FingerCount    = 10;
		Char->HpPerFinger    = 2;
		Char->LeftHandCard   = LeftHand;
		Char->RightHandCard  = RightHand;
		// 顺序：5 张参战伙伴卡 + 虫妹的小布袋（A 类）+ 蛛茧绒囊（B 类占位）+ 暮色引虫灯（A 类，DeleteProvider）。
		// Initialize 时按 Stage 4.1 a2 规则：非容器卡进 BattleDeck，容器卡只进 Backpack。
		Char->StarterDeck    = { Zhaoguang, Fuxiao, Chifu, Shuoguang, Muling, BugGirlBag, ZhujianRongnang, MuseiLantern };

		SaveAssetPackage(CharPkg, Char, CharPkgPath);
		return Char;
	}
}
