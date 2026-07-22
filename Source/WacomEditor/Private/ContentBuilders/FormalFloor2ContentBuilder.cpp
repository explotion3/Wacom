// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/FormalFloor2ContentBuilder.h"

#include "Cards/CardDefinition.h"
#include "Encounters/EncounterDefinition.h"
#include "Enemies/EnemyBehaviorDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Events/RunEventDefinition.h"
#include "Misc/PackageName.h"
#include "Pickups/RunPickupDefinition.h"
#include "Shops/ShopDefinition.h"
#include "Tags/WacomGameplayTags.h"
#include "UObject/StrongObjectPtr.h"

namespace Wacom::ContentBuilder::FormalFloor2Private
{
	const FString CardsRewardsRoot = TEXT("/Game/Wacom/Data/Cards/Rewards/MoltCavern");
	const FString CardsRunRoot = TEXT("/Game/Wacom/Data/Cards/Run/MoltCavern");
	const FString EnemiesRoot = TEXT("/Game/Wacom/Data/Enemies/MoltCavern");
	const FString EncountersRoot = TEXT("/Game/Wacom/Data/Encounters/MoltCavern");
	const FString EventsRoot = TEXT("/Game/Wacom/Data/Events/MoltCavern");
	const FString PickupsRoot = TEXT("/Game/Wacom/Data/Pickups/MoltCavern");
	const FString ShopsRoot = TEXT("/Game/Wacom/Data/Shops/MoltCavern");

	const FString HerbalPoulticePackage =
		TEXT("/Game/Wacom/Data/Cards/Rewards/SerpentWood/DA_Card_HerbalPoultice");
	const FString ChitinWardPackage =
		TEXT("/Game/Wacom/Data/Cards/BugGirl/StarterPack/DA_Card_Starter_ChitinWard");
	const FString MoltCutPackage =
		TEXT("/Game/Wacom/Data/Cards/BugGirl/StarterPack/DA_Card_Starter_MoltCut");

	enum class ESeedEffect : uint8
	{
		Damage,
		Heal,
		Draw,
		Poison,
		Slow,
		Shield,
	};

	enum class ESeedTarget : uint8
	{
		Player,
		Self,
		SingleEnemyPart,
		AllEnemyParts,
	};

	struct FCardEffectSeed
	{
		ESeedEffect Effect = ESeedEffect::Damage;
		int32 Magnitude = 0;
		ESeedTarget Target = ESeedTarget::Player;
		bool bDrawFromDrawPile = false;
	};

	struct FCardSeed
	{
		const TCHAR* CardId;
		const TCHAR* DisplayName;
		const TCHAR* Description;
		int32 Cost;
		const FNativeGameplayTag* Rarity;
		const FNativeGameplayTag* Keyword;
		ECardTargetMode TargetMode;
		TArray<FCardEffectSeed> Effects;
	};

	struct FIntentSeed
	{
		const TCHAR* Archetype;
		const TCHAR* PartSlot;
		const TCHAR* Suffix;
		const TCHAR* DisplayName;
		int32 Initiative;
		ESeedEffect Effect;
		int32 Magnitude;
		ESeedTarget Target;
	};

	struct FPartSeed
	{
		const TCHAR* Archetype;
		const TCHAR* PartSlot;
		const TCHAR* DisplayName;
		int32 MaxHp;
		int32 Experience;
	};

	struct FEnemySeed
	{
		const TCHAR* Archetype;
		const TCHAR* EnemyId;
		const TCHAR* DisplayName;
		TArray<const TCHAR*> PartSlots;
	};

	struct FEncounterSeed
	{
		const TCHAR* Id;
		const TCHAR* Leaf;
		const TCHAR* DisplayName;
		TArray<TPair<const TCHAR*, const TCHAR*>> Slots;
	};

	FString MakePackage(const FString& Root, const TCHAR* Leaf)
	{
		return Root / Leaf;
	}

	FString ArchetypeRoot(const TCHAR* Archetype)
	{
		return EnemiesRoot / Archetype;
	}

	FString BehaviorPackage(const TCHAR* Archetype)
	{
		return ArchetypeRoot(Archetype)
			/ FString::Printf(TEXT("DA_Behavior_%s"), Archetype);
	}

	FString PartPackage(const TCHAR* Archetype, const TCHAR* PartSlot)
	{
		return ArchetypeRoot(Archetype)
			/ FString::Printf(TEXT("DA_Part_%s_%s"), Archetype, PartSlot);
	}

	FString EnemyPackage(const TCHAR* Archetype)
	{
		return ArchetypeRoot(Archetype)
			/ FString::Printf(TEXT("DA_Enemy_%s"), Archetype);
	}

	FString BranchCardPackage(const TCHAR* Archetype, const TCHAR* Choice)
	{
		return CardsRewardsRoot / Archetype
			/ FString::Printf(TEXT("DA_Card_%s_%s"), Archetype, Choice);
	}

	FGameplayTag EffectTag(const ESeedEffect Effect)
	{
		using namespace WacomTags;
		switch (Effect)
		{
		case ESeedEffect::Damage: return Effect_Damage;
		case ESeedEffect::Heal: return Effect_Heal;
		case ESeedEffect::Draw: return Effect_Draw;
		case ESeedEffect::Poison: return Effect_ApplyStatus_Poison;
		case ESeedEffect::Slow: return Effect_ApplyStatus_Slow;
		case ESeedEffect::Shield: return Status_Shield;
		default: return FGameplayTag();
		}
	}

	FGameplayTag TargetTag(const ESeedTarget Target)
	{
		using namespace WacomTags;
		switch (Target)
		{
		case ESeedTarget::Player: return Target_Player;
		case ESeedTarget::Self: return Target_Self;
		case ESeedTarget::SingleEnemyPart: return Target_SingleEnemyPart;
		case ESeedTarget::AllEnemyParts: return Target_AllEnemyParts;
		default: return FGameplayTag();
		}
	}

	const TArray<FCardSeed>& CardSeeds()
	{
		using namespace WacomTags;
		static const TArray<FCardSeed> Seeds =
		{
			{TEXT("Reward.MoltCavern.GlowcapPoultice"), TEXT("菌光药膏"),
				TEXT("恢复 {Effect.0} 点生命。"), 1, &Card_Rarity_Blue,
				&Card_Keyword_Tool, ECardTargetMode::None,
				{{ESeedEffect::Heal, 6, ESeedTarget::Player}}},
			{TEXT("Reward.MoltCavern.CrystalWard"), TEXT("晶脉护符"),
				TEXT("获得 {Effect.0} 点护盾。"), 0, &Card_Rarity_Blue,
				&Card_Keyword_Tool, ECardTargetMode::None,
				{{ESeedEffect::Shield, 5, ESeedTarget::Player}}},
			{TEXT("Reward.MoltCavern.VenomShard"), TEXT("毒晶尖刺"),
				TEXT("造成 {Effect.0} 点伤害，并施加 {Effect.1} 层中毒。"),
				1, &Card_Rarity_Blue, &Card_Keyword_Weapon,
				ECardTargetMode::SingleEnemyPart,
				{{ESeedEffect::Damage, 4, ESeedTarget::SingleEnemyPart},
				 {ESeedEffect::Poison, 2, ESeedTarget::SingleEnemyPart}}},
			{TEXT("Card.Run.MoltSeal"), TEXT("深窟蜕印"),
				TEXT("从抽牌堆抽取 {Effect.0} 张牌。"), 1, &Card_Rarity_Blue,
				nullptr, ECardTargetMode::None,
				{{ESeedEffect::Draw, 2, ESeedTarget::Player, true}}},
			{TEXT("Reward.MoltCavern.ScaleCrawler.Aid"), TEXT("鳞影潜行"),
				TEXT("获得 {Effect.0} 点护盾，并使一个敌方部位的当前意图延后 {Effect.1} 点先机。"),
				1, &Card_Rarity_Blue, &Card_Keyword_Tool,
				ECardTargetMode::SingleEnemyPart,
				{{ESeedEffect::Shield, 3, ESeedTarget::Player},
				 {ESeedEffect::Slow, 1, ESeedTarget::SingleEnemyPart}}},
			{TEXT("Reward.MoltCavern.ScaleCrawler.Destroy"), TEXT("裂鳞毒牙"),
				TEXT("造成 {Effect.0} 点伤害，并施加 {Effect.1} 层中毒。"),
				1, &Card_Rarity_Blue, &Card_Keyword_Weapon,
				ECardTargetMode::SingleEnemyPart,
				{{ESeedEffect::Damage, 4, ESeedTarget::SingleEnemyPart},
				 {ESeedEffect::Poison, 1, ESeedTarget::SingleEnemyPart}}},
			{TEXT("Reward.MoltCavern.StoneScaleGuard.Aid"), TEXT("石甲壁垒"),
				TEXT("获得 {Effect.0} 点护盾。"), 1, &Card_Rarity_Blue,
				&Card_Keyword_Tool, ECardTargetMode::None,
				{{ESeedEffect::Shield, 9, ESeedTarget::Player}}},
			{TEXT("Reward.MoltCavern.StoneScaleGuard.Destroy"), TEXT("崩岩重击"),
				TEXT("造成 {Effect.0} 点伤害。"), 1, &Card_Rarity_Blue,
				&Card_Keyword_Weapon, ECardTargetMode::SingleEnemyPart,
				{{ESeedEffect::Damage, 7, ESeedTarget::SingleEnemyPart}}},
			{TEXT("Reward.MoltCavern.VenomHunter.Aid"), TEXT("毒泉缠守"),
				TEXT("获得 {Effect.0} 点护盾，并使一个敌方部位的当前意图延后 {Effect.1} 点先机。"),
				1, &Card_Rarity_Blue, &Card_Keyword_Tool,
				ECardTargetMode::SingleEnemyPart,
				{{ESeedEffect::Shield, 4, ESeedTarget::Player},
				 {ESeedEffect::Slow, 2, ESeedTarget::SingleEnemyPart}}},
			{TEXT("Reward.MoltCavern.VenomHunter.Destroy"), TEXT("猎毒突刺"),
				TEXT("造成 {Effect.0} 点伤害，并施加 {Effect.1} 层中毒。"),
				1, &Card_Rarity_Blue, &Card_Keyword_Weapon,
				ECardTargetMode::SingleEnemyPart,
				{{ESeedEffect::Damage, 6, ESeedTarget::SingleEnemyPart},
				 {ESeedEffect::Poison, 2, ESeedTarget::SingleEnemyPart}}},
			{TEXT("Reward.MoltCavern.CavernGuardian.Aid"), TEXT("洞壳庇护"),
				TEXT("获得 {Effect.0} 点护盾。"), 1, &Card_Rarity_Yellow,
				&Card_Keyword_Tool, ECardTargetMode::None,
				{{ESeedEffect::Shield, 13, ESeedTarget::Player}}},
			{TEXT("Reward.MoltCavern.CavernGuardian.Destroy"), TEXT("碎窟毒潮"),
				TEXT("对所有存活敌方部位造成 {Effect.0} 点伤害，并施加 {Effect.1} 层中毒。"),
				2, &Card_Rarity_Yellow, &Card_Keyword_Weapon,
				ECardTargetMode::AllEnemyParts,
				{{ESeedEffect::Damage, 5, ESeedTarget::AllEnemyParts},
				 {ESeedEffect::Poison, 2, ESeedTarget::AllEnemyParts}}},
		};
		return Seeds;
	}

	const TArray<FIntentSeed>& IntentSeeds()
	{
		static const TArray<FIntentSeed> Seeds =
		{
			{TEXT("ScaleCrawler"), TEXT("Head"), TEXT("StoneBite"), TEXT("石牙啃咬"), 3, ESeedEffect::Damage, 4, ESeedTarget::Player},
			{TEXT("ScaleCrawler"), TEXT("Head"), TEXT("Venom"), TEXT("注毒"), 5, ESeedEffect::Poison, 1, ESeedTarget::Player},
			{TEXT("ScaleCrawler"), TEXT("Body"), TEXT("Skitter"), TEXT("疾爬"), 2, ESeedEffect::Damage, 3, ESeedTarget::Player},
			{TEXT("ScaleCrawler"), TEXT("Body"), TEXT("Castoff"), TEXT("蜕鳞"), 2, ESeedEffect::Shield, 3, ESeedTarget::Self},
			{TEXT("ScaleCrawler"), TEXT("Body"), TEXT("Coil"), TEXT("缠绕"), 4, ESeedEffect::Slow, 1, ESeedTarget::Player},
			{TEXT("StoneScaleGuard"), TEXT("Head"), TEXT("CrushBite"), TEXT("碎咬"), 3, ESeedEffect::Damage, 5, ESeedTarget::Player},
			{TEXT("StoneScaleGuard"), TEXT("Head"), TEXT("DustSpit"), TEXT("尘毒"), 5, ESeedEffect::Poison, 1, ESeedTarget::Player},
			{TEXT("StoneScaleGuard"), TEXT("Carapace"), TEXT("LithicHarden"), TEXT("岩甲硬化"), 2, ESeedEffect::Shield, 7, ESeedTarget::Self},
			{TEXT("StoneScaleGuard"), TEXT("Carapace"), TEXT("Ram"), TEXT("岩壳冲撞"), 4, ESeedEffect::Damage, 5, ESeedTarget::Player},
			{TEXT("StoneScaleGuard"), TEXT("Tail"), TEXT("Sweep"), TEXT("尾扫"), 2, ESeedEffect::Damage, 3, ESeedTarget::Player},
			{TEXT("StoneScaleGuard"), TEXT("Tail"), TEXT("Brace"), TEXT("支撑"), 2, ESeedEffect::Shield, 3, ESeedTarget::Self},
			{TEXT("VenomHunter"), TEXT("Head"), TEXT("Pounce"), TEXT("扑猎"), 4, ESeedEffect::Damage, 6, ESeedTarget::Player},
			{TEXT("VenomHunter"), TEXT("Head"), TEXT("Fang"), TEXT("毒牙"), 5, ESeedEffect::Poison, 2, ESeedTarget::Player},
			{TEXT("VenomHunter"), TEXT("Coil"), TEXT("Bind"), TEXT("束缚"), 4, ESeedEffect::Slow, 2, ESeedTarget::Player},
			{TEXT("VenomHunter"), TEXT("Coil"), TEXT("Crush"), TEXT("绞压"), 3, ESeedEffect::Damage, 5, ESeedTarget::Player},
			{TEXT("VenomHunter"), TEXT("Coil"), TEXT("Veil"), TEXT("毒幕"), 2, ESeedEffect::Shield, 4, ESeedTarget::Self},
			{TEXT("VenomHunter"), TEXT("VenomSac"), TEXT("VenomBurst"), TEXT("毒囊迸发"), 5, ESeedEffect::Poison, 2, ESeedTarget::Player},
			{TEXT("VenomHunter"), TEXT("VenomSac"), TEXT("GuardSac"), TEXT("护囊"), 2, ESeedEffect::Shield, 3, ESeedTarget::Self},
			{TEXT("CavernGuardian"), TEXT("Head"), TEXT("DeepBite"), TEXT("深窟噬咬"), 3, ESeedEffect::Damage, 7, ESeedTarget::Player},
			{TEXT("CavernGuardian"), TEXT("Head"), TEXT("VenomFlood"), TEXT("毒潮"), 5, ESeedEffect::Poison, 2, ESeedTarget::Player},
			{TEXT("CavernGuardian"), TEXT("Body"), TEXT("CaveCrush"), TEXT("洞窟碾压"), 4, ESeedEffect::Damage, 8, ESeedTarget::Player},
			{TEXT("CavernGuardian"), TEXT("Body"), TEXT("MoltWall"), TEXT("蜕壳之壁"), 2, ESeedEffect::Shield, 9, ESeedTarget::Self},
			{TEXT("CavernGuardian"), TEXT("Tail"), TEXT("RockSweep"), TEXT("岩尾横扫"), 2, ESeedEffect::Damage, 5, ESeedTarget::Player},
			{TEXT("CavernGuardian"), TEXT("Tail"), TEXT("Pin"), TEXT("镇压"), 4, ESeedEffect::Slow, 2, ESeedTarget::Player},
			{TEXT("CavernGuardian"), TEXT("MoltCore"), TEXT("CorePulse"), TEXT("蜕核脉冲"), 5, ESeedEffect::Poison, 2, ESeedTarget::Player},
			{TEXT("CavernGuardian"), TEXT("MoltCore"), TEXT("ShedWard"), TEXT("蜕核护持"), 2, ESeedEffect::Shield, 6, ESeedTarget::Self},
		};
		return Seeds;
	}

	const TArray<FPartSeed>& PartSeeds()
	{
		static const TArray<FPartSeed> Seeds =
		{
			{TEXT("ScaleCrawler"), TEXT("Head"), TEXT("头部"), 9, 1},
			{TEXT("ScaleCrawler"), TEXT("Body"), TEXT("躯体"), 12, 1},
			{TEXT("StoneScaleGuard"), TEXT("Head"), TEXT("头部"), 10, 1},
			{TEXT("StoneScaleGuard"), TEXT("Carapace"), TEXT("石甲"), 18, 2},
			{TEXT("StoneScaleGuard"), TEXT("Tail"), TEXT("尾部"), 8, 1},
			{TEXT("VenomHunter"), TEXT("Head"), TEXT("头部"), 12, 2},
			{TEXT("VenomHunter"), TEXT("Coil"), TEXT("盘身"), 15, 2},
			{TEXT("VenomHunter"), TEXT("VenomSac"), TEXT("毒囊"), 7, 1},
			{TEXT("CavernGuardian"), TEXT("Head"), TEXT("头部"), 16, 2},
			{TEXT("CavernGuardian"), TEXT("Body"), TEXT("躯体"), 28, 4},
			{TEXT("CavernGuardian"), TEXT("Tail"), TEXT("尾部"), 14, 2},
			{TEXT("CavernGuardian"), TEXT("MoltCore"), TEXT("蜕核"), 12, 2},
		};
		return Seeds;
	}

	const TArray<FEnemySeed>& EnemySeeds()
	{
		static const TArray<FEnemySeed> Seeds =
		{
			{TEXT("ScaleCrawler"), TEXT("Enemy.MoltCavern.ScaleCrawler"), TEXT("鳞岩爬蛇"), {TEXT("Head"), TEXT("Body")}},
			{TEXT("StoneScaleGuard"), TEXT("Enemy.MoltCavern.StoneScaleGuard"), TEXT("石鳞守卫"), {TEXT("Head"), TEXT("Carapace"), TEXT("Tail")}},
			{TEXT("VenomHunter"), TEXT("Enemy.MoltCavern.VenomHunter"), TEXT("毒泉猎手"), {TEXT("Head"), TEXT("Coil"), TEXT("VenomSac")}},
			{TEXT("CavernGuardian"), TEXT("Enemy.MoltCavern.CavernGuardian"), TEXT("洞窟守卫"), {TEXT("Head"), TEXT("Body"), TEXT("Tail"), TEXT("MoltCore")}},
		};
		return Seeds;
	}

	const TArray<FEncounterSeed>& EncounterSeeds()
	{
		static const TArray<FEncounterSeed> Seeds =
		{
			{TEXT("Encounter.MoltCavern.ScaleScout"), TEXT("DA_Encounter_ScaleScout"), TEXT("鳞岩伏击"), {{TEXT("Scout"), TEXT("ScaleCrawler")}}},
			{TEXT("Encounter.MoltCavern.StoneScaleGuard"), TEXT("DA_Encounter_StoneScaleGuard"), TEXT("石鳞守地"), {{TEXT("Guard"), TEXT("StoneScaleGuard")}}},
			{TEXT("Encounter.MoltCavern.HatcheryAmbush"), TEXT("DA_Encounter_HatcheryAmbush"), TEXT("孵室伏击"), {{TEXT("Left"), TEXT("ScaleCrawler")}, {TEXT("Right"), TEXT("ScaleCrawler")}}},
			{TEXT("Encounter.MoltCavern.BridgeSentinel"), TEXT("DA_Encounter_BridgeSentinel"), TEXT("断桥守敌"), {{TEXT("Sentinel"), TEXT("StoneScaleGuard")}}},
			{TEXT("Encounter.MoltCavern.VenomHunter"), TEXT("DA_Encounter_VenomHunter"), TEXT("毒泉猎手"), {{TEXT("Hunter"), TEXT("VenomHunter")}}},
			{TEXT("Encounter.MoltCavern.EliteMolter"), TEXT("DA_Encounter_EliteMolter"), TEXT("蜕窟巡猎"), {{TEXT("Guard"), TEXT("StoneScaleGuard")}, {TEXT("Scout"), TEXT("ScaleCrawler")}}},
			{TEXT("Encounter.MoltCavern.CavernGuardian"), TEXT("DA_Encounter_CavernGuardian"), TEXT("洞窟守卫"), {{TEXT("Guardian"), TEXT("CavernGuardian")}}},
		};
		return Seeds;
	}

	const FCardSeed* FindCardSeed(const FName Id)
	{
		return CardSeeds().FindByPredicate(
			[Id](const FCardSeed& Seed) { return Id == FName(Seed.CardId); });
	}

	const FPartSeed* FindPartSeed(const FName Id)
	{
		return PartSeeds().FindByPredicate([Id](const FPartSeed& Seed)
		{
			return Id == FName(*FString::Printf(
				TEXT("MoltCavern.%s.%s"), Seed.Archetype, Seed.PartSlot));
		});
	}

	const FEnemySeed* FindEnemySeed(const FName Id)
	{
		return EnemySeeds().FindByPredicate(
			[Id](const FEnemySeed& Seed) { return Id == FName(Seed.EnemyId); });
	}

	const FEnemySeed* FindBehaviorSeed(const FName Id)
	{
		return EnemySeeds().FindByPredicate([Id](const FEnemySeed& Seed)
		{
			return Id == FName(*FString::Printf(
				TEXT("MoltCavern.%s.Behavior"), Seed.Archetype));
		});
	}

	const FEncounterSeed* FindEncounterSeed(const FName Id)
	{
		return EncounterSeeds().FindByPredicate(
			[Id](const FEncounterSeed& Seed) { return Id == FName(Seed.Id); });
	}

	bool ConfigureCard(UCardDefinition& Card, const FCardSeed& Seed)
	{
		Card.CardId = Seed.CardId;
		Card.DisplayName = FText::FromString(Seed.DisplayName);
		Card.Description = FText::FromString(Seed.Description);
		Card.CardIllustration = nullptr;
		Card.CardIllustrationDepthMap = nullptr;
		Card.BaseCost = Seed.Cost;
		Card.Rarity = Seed.Rarity ? Seed.Rarity->GetTag() : FGameplayTag();
		Card.Keywords.Reset();
		if (Seed.Keyword)
		{
			Card.Keywords.AddTag(Seed.Keyword->GetTag());
		}
		Card.TargetMode = Seed.TargetMode;
		Card.HandCardTargetFilter = FWacomHandCardTargetFilter();
		Card.Physique = FCardPhysique();
		Card.Effects.Reset();
		for (const FCardEffectSeed& EffectSeed : Seed.Effects)
		{
			FCardEffect& Effect = Card.Effects.AddDefaulted_GetRef();
			Effect.EffectType = EffectTag(EffectSeed.Effect);
			Effect.Magnitude = EffectSeed.Magnitude;
			Effect.Target = TargetTag(EffectSeed.Target);
			Effect.TargetZone = EffectSeed.bDrawFromDrawPile
				? WacomTags::CardLocation_Draw.GetTag() : FGameplayTag();
			Effect.Duration = 0;
			Effect.MagnitudeSource = FGameplayTag();
		}
		Card.PerfectReleaseEffects.Reset();
		Card.ZoneHooks.Reset();
		Card.Passives.Reset();
		return true;
	}

	bool ConfigureBehavior(UEnemyBehaviorDefinition& Behavior, const FEnemySeed& Seed)
	{
		Behavior.BehaviorId = *FString::Printf(
			TEXT("MoltCavern.%s.Behavior"), Seed.Archetype);
		Behavior.InitialPhaseId = TEXT("Default");
		Behavior.Phases.Reset();
		FWacomEnemyPhaseDefinition& Phase = Behavior.Phases.AddDefaulted_GetRef();
		Phase.PhaseId = TEXT("Default");
		for (const TCHAR* PartSlot : Seed.PartSlots)
		{
			FWacomEnemyIntentSetDefinition& Set = Phase.IntentSets.AddDefaulted_GetRef();
			Set.IntentSetId = *FString::Printf(
				TEXT("MoltCavern.%s.%s.Sequence"), Seed.Archetype, PartSlot);
			Set.AppliesToPartSlotId = PartSlot;
			Set.SelectorMode = EWacomEnemyIntentSelectorMode::Sequence;
			Set.SelectorRules.Reset();
			Set.FallbackIntentId = NAME_None;
			for (const FIntentSeed& IntentSeed : IntentSeeds())
			{
				if (FCString::Strcmp(IntentSeed.Archetype, Seed.Archetype) != 0
					|| FCString::Strcmp(IntentSeed.PartSlot, PartSlot) != 0)
				{
					continue;
				}
				FWacomEnemyBehaviorIntent& BehaviorIntent = Set.Intents.AddDefaulted_GetRef();
				BehaviorIntent.CooldownGroup = NAME_None;
				BehaviorIntent.CooldownSelections = 0;
				FIntentDefinition& Intent = BehaviorIntent.Intent;
				Intent.IntentId = *FString::Printf(TEXT("MoltCavern.%s.%s.%s"),
					Seed.Archetype, PartSlot, IntentSeed.Suffix);
				Intent.DisplayName = FText::FromString(IntentSeed.DisplayName);
				Intent.Initiative = IntentSeed.Initiative;
				FIntentEffect& Effect = Intent.Effects.AddDefaulted_GetRef();
				Effect.EffectType = EffectTag(IntentSeed.Effect);
				Effect.Magnitude = IntentSeed.Magnitude;
				Effect.Target = TargetTag(IntentSeed.Target);
				Effect.Duration = 0;
				Effect.HandAffliction.Selection = EHandAfflictionSelection::Default;
				Effect.HandAffliction.TargetCardCount = 1;
			}
		}
		return true;
	}

	bool ConfigurePart(
		UEnemyPartDefinition& Part,
		const FPartSeed& Seed,
		const FFormalProductionResolveObject& Resolve,
		TArray<FString>& OutErrors)
	{
		Part.PartId = *FString::Printf(
			TEXT("MoltCavern.%s.%s"), Seed.Archetype, Seed.PartSlot);
		Part.DisplayName = FText::FromString(Seed.DisplayName);
		Part.MaxHp = Seed.MaxHp;
		Part.ExperienceReward = Seed.Experience;
		Part.AidRewardCard = Cast<UCardDefinition>(
			Resolve(BranchCardPackage(Seed.Archetype, TEXT("Aid"))));
		Part.DestroyRewardCard = Cast<UCardDefinition>(
			Resolve(BranchCardPackage(Seed.Archetype, TEXT("Destroy"))));
		Part.KnockdownRewardCard = nullptr;
		if (!Part.AidRewardCard || !Part.DestroyRewardCard)
		{
			OutErrors.Add(FString::Printf(TEXT("Could not resolve branch rewards for %s"),
				*Part.PartId.ToString()));
		}
		return OutErrors.IsEmpty();
	}

	bool ConfigureEnemy(
		UEnemyDefinition& Enemy,
		const FEnemySeed& Seed,
		const FFormalProductionResolveObject& Resolve,
		TArray<FString>& OutErrors)
	{
		Enemy.EnemyId = Seed.EnemyId;
		Enemy.DisplayName = FText::FromString(Seed.DisplayName);
		Enemy.DefaultBehavior = Cast<UEnemyBehaviorDefinition>(
			Resolve(BehaviorPackage(Seed.Archetype)));
		Enemy.DefaultPhaseId = TEXT("Default");
		Enemy.Parts.Reset();
		for (const TCHAR* PartSlot : Seed.PartSlots)
		{
			FEnemyPartSlot& Slot = Enemy.Parts.AddDefaulted_GetRef();
			Slot.PartSlotId = PartSlot;
			Slot.PartDef = Cast<UEnemyPartDefinition>(
				Resolve(PartPackage(Seed.Archetype, PartSlot)));
			Slot.BehaviorOverride = nullptr;
			Slot.InitialIntentSetId = *FString::Printf(
				TEXT("MoltCavern.%s.%s.Sequence"), Seed.Archetype, PartSlot);
			if (!Slot.PartDef)
			{
				OutErrors.Add(FString::Printf(TEXT("Could not resolve %s.%s Part"),
					Seed.Archetype, PartSlot));
			}
		}
		if (!Enemy.DefaultBehavior)
		{
			OutErrors.Add(FString::Printf(TEXT("Could not resolve %s Behavior"),
				Seed.Archetype));
		}
		return OutErrors.IsEmpty();
	}

	bool ConfigureEncounter(
		UEncounterDefinition& Encounter,
		const FEncounterSeed& Seed,
		const FFormalProductionResolveObject& Resolve,
		TArray<FString>& OutErrors)
	{
		Encounter.EncounterDefinitionId = Seed.Id;
		Encounter.DisplayName = FText::FromString(Seed.DisplayName);
		Encounter.EnemySlots.Reset();
		for (const auto& SeedSlot : Seed.Slots)
		{
			FEncounterEnemySlot& Slot = Encounter.EnemySlots.AddDefaulted_GetRef();
			Slot.EnemySlotId = SeedSlot.Key;
			Slot.EnemyDefinition = Cast<UEnemyDefinition>(
				Resolve(EnemyPackage(SeedSlot.Value)));
			if (!Slot.EnemyDefinition)
			{
				OutErrors.Add(FString::Printf(TEXT("Could not resolve Encounter enemy %s"),
					SeedSlot.Value));
			}
		}
		return OutErrors.IsEmpty();
	}

	FWacomRunEventConditionDefinition MinGold(const int32 Value)
	{
		FWacomRunEventConditionDefinition Result;
		Result.Type = EWacomRunEventConditionType::MinGold;
		Result.Value = Value;
		return Result;
	}

	FWacomRunEventConditionDefinition RunFlagSet(const TCHAR* FlagId)
	{
		FWacomRunEventConditionDefinition Result;
		Result.Type = EWacomRunEventConditionType::RunFlagSet;
		Result.FlagId = FlagId;
		return Result;
	}

	FWacomRunEventEffectDefinition AddGold(const int32 Value)
	{
		FWacomRunEventEffectDefinition Result;
		Result.Type = EWacomRunEventEffectType::AddGold;
		Result.Value = Value;
		return Result;
	}

	FWacomRunEventEffectDefinition AddPressure(const TCHAR* Type, const int32 Value)
	{
		FWacomRunEventEffectDefinition Result;
		Result.Type = EWacomRunEventEffectType::AddPressure;
		Result.PressureType = Type;
		Result.Value = Value;
		return Result;
	}

	FWacomRunEventEffectDefinition SetRunFlag(const TCHAR* FlagId)
	{
		FWacomRunEventEffectDefinition Result;
		Result.Type = EWacomRunEventEffectType::SetRunFlag;
		Result.FlagId = FlagId;
		return Result;
	}

	FWacomRunEventChoiceDefinition Choice(
		const TCHAR* Id,
		const TCHAR* Label,
		TArray<FWacomRunEventConditionDefinition> Conditions = {},
		TArray<FWacomRunEventEffectDefinition> Effects = {})
	{
		FWacomRunEventChoiceDefinition Result;
		Result.ChoiceId = Id;
		Result.LabelText = FText::FromString(Label);
		Result.Conditions = MoveTemp(Conditions);
		Result.ActionPointPolicy = EWacomRunEventActionPointPolicy::Automatic;
		Result.FixedActionPointCost = 1;
		Result.Effects = MoveTemp(Effects);
		Result.NextNodeId = NAME_None;
		Result.bCloseEventAfterResolve = true;
		Result.bMarkEventCompleted = true;
		return Result;
	}

	bool ConfigureEvent(UWacomRunEventDefinition& Event, const FName EventId)
	{
		static const TCHAR* RitePatternKnown = TEXT("MoltCavern.RitePatternKnown");
		static const TCHAR* DelverRouteKnown = TEXT("MoltCavern.DelverRouteKnown");
		Event.EventId = EventId;
		Event.StartNodeId = TEXT("Start");
		Event.Nodes.Reset();
		FWacomRunEventNodeDefinition& Node = Event.Nodes.AddDefaulted_GetRef();
		Node.NodeId = TEXT("Start");
		if (EventId == TEXT("Event.MoltCavern.CastoffEcho"))
		{
			Event.DisplayName = FText::FromString(TEXT("残蜕回声"));
			Node.TitleText = FText::FromString(TEXT("残蜕回声"));
			Node.BodyText = FText::FromString(TEXT("洞壁上的旧蜕记录着仪式与尘埃。"));
			Node.Choices =
			{
				Choice(TEXT("ReadRitePattern"), TEXT("辨读仪式纹路"), {}, {SetRunFlag(RitePatternKnown)}),
				Choice(TEXT("GatherScaleDust"), TEXT("收集鳞尘"), {}, {AddGold(3), AddPressure(TEXT("Misdeed"), 2)}),
				Choice(TEXT("RestAmongCastoffs"), TEXT("在残蜕间休息"), {}, {AddPressure(TEXT("Fatigue"), -2)}),
			};
			return true;
		}
		if (EventId == TEXT("Event.MoltCavern.LostDelver"))
		{
			Event.DisplayName = FText::FromString(TEXT("失踪探路者"));
			Node.TitleText = FText::FromString(TEXT("旧井边的探路者"));
			Node.BodyText = FText::FromString(TEXT("遗留的路标、背包与口粮仍能改变前路。"));
			Node.Choices =
			{
				Choice(TEXT("GuideToOldWell"), TEXT("标记旧井路线"), {}, {SetRunFlag(DelverRouteKnown), AddPressure(TEXT("Misdeed"), -2)}),
				Choice(TEXT("TakeAbandonedPack"), TEXT("拿走遗弃背包"), {}, {AddGold(4), AddPressure(TEXT("Misdeed"), 3)}),
				Choice(TEXT("ShareRations"), TEXT("分享口粮"), {}, {AddPressure(TEXT("Fatigue"), -3)}),
			};
			return true;
		}
		if (EventId == TEXT("Event.MoltCavern.MoltingRite"))
		{
			Event.DisplayName = FText::FromString(TEXT("蜕壳仪式"));
			Node.TitleText = FText::FromString(TEXT("蜕壳仪式"));
			Node.BodyText = FText::FromString(TEXT("仪式门槛接受知识、路标、金币或硬闯。"));
			Node.Choices =
			{
				Choice(TEXT("RepeatKnownRite"), TEXT("复述已知仪式"), {RunFlagSet(RitePatternKnown)}, {AddPressure(TEXT("Fatigue"), -3)}),
				Choice(TEXT("FollowDelverMarks"), TEXT("沿探路标记"), {RunFlagSet(DelverRouteKnown)}, {AddPressure(TEXT("Wound"), -2)}),
				Choice(TEXT("OfferCoin"), TEXT("献上金币"), {MinGold(3)}, {AddGold(-3), AddPressure(TEXT("Misdeed"), -2)}),
				Choice(TEXT("ForcePassage"), TEXT("强行通过"), {}, {AddPressure(TEXT("Fatigue"), 5), AddPressure(TEXT("Wound"), 1)}),
			};
			return true;
		}
		return false;
	}

	bool ConfigurePickup(
		UWacomRunPickupDefinition& Pickup,
		const FName PickupId,
		const FFormalProductionResolveObject& Resolve,
		TArray<FString>& OutErrors)
	{
		Pickup.PickupId = PickupId;
		Pickup.RewardType = EWacomRunPickupRewardType::Card;
		Pickup.GoldAmount = 1;
		Pickup.GrantedCredentialIds.Reset();
		FString CardPackage;
		if (PickupId == TEXT("Pickup.MoltCavern.FungalCache"))
		{
			CardPackage = MakePackage(CardsRewardsRoot, TEXT("DA_Card_GlowcapPoultice"));
		}
		else if (PickupId == TEXT("Pickup.MoltCavern.MineralCache"))
		{
			CardPackage = MakePackage(CardsRewardsRoot, TEXT("DA_Card_CrystalWard"));
		}
		else if (PickupId == TEXT("Pickup.MoltCavern.VenomCrystalCache"))
		{
			CardPackage = MakePackage(CardsRewardsRoot, TEXT("DA_Card_VenomShard"));
		}
		else if (PickupId == TEXT("Pickup.MoltCavern.MoltSeal"))
		{
			CardPackage = MakePackage(CardsRunRoot, TEXT("DA_Card_MoltSeal"));
			Pickup.GrantedCredentialIds.Add(TEXT("Credential.Run.MoltSeal"));
		}
		Pickup.CardDefinition = Cast<UCardDefinition>(Resolve(CardPackage));
		if (!Pickup.CardDefinition)
		{
			OutErrors.Add(FString::Printf(TEXT("Could not resolve Pickup card for %s"),
				*PickupId.ToString()));
		}
		return OutErrors.IsEmpty();
	}

	bool ConfigureShop(
		UShopDefinition& Shop,
		const FFormalProductionResolveObject& Resolve,
		TArray<FString>& OutErrors)
	{
		Shop.ShopId = TEXT("Shop.MoltCavern.DeepWayfarer");
		Shop.DisplayName = FText::FromString(TEXT("深窟行商"));
		Shop.Offers.Reset();
		const TArray<TPair<FString, int32>> OfferSeeds =
		{
			{HerbalPoulticePackage, 3},
			{ChitinWardPackage, 3},
			{MoltCutPackage, 4},
			{MakePackage(CardsRewardsRoot, TEXT("DA_Card_GlowcapPoultice")), 4},
			{MakePackage(CardsRewardsRoot, TEXT("DA_Card_VenomShard")), 5},
		};
		for (const auto& OfferSeed : OfferSeeds)
		{
			FShopOfferDefinition& Offer = Shop.Offers.AddDefaulted_GetRef();
			Offer.CardDefinition = Cast<UCardDefinition>(Resolve(OfferSeed.Key));
			Offer.Price = OfferSeed.Value;
			if (!Offer.CardDefinition)
			{
				OutErrors.Add(FString::Printf(TEXT("Could not resolve Shop card %s"),
					*OfferSeed.Key));
			}
		}
		return OutErrors.IsEmpty();
	}

	bool ConfigureExpected(
		UObject& Object,
		const FFormalProductionContentManifestEntry& Entry,
		const FFormalProductionResolveObject& Resolve,
		TArray<FString>& OutErrors)
	{
		if (UCardDefinition* Card = Cast<UCardDefinition>(&Object))
		{
			const FCardSeed* Seed = FindCardSeed(Entry.StableId);
			return Seed && ConfigureCard(*Card, *Seed);
		}
		if (UEnemyBehaviorDefinition* Behavior = Cast<UEnemyBehaviorDefinition>(&Object))
		{
			const FEnemySeed* Seed = FindBehaviorSeed(Entry.StableId);
			return Seed && ConfigureBehavior(*Behavior, *Seed);
		}
		if (UEnemyPartDefinition* Part = Cast<UEnemyPartDefinition>(&Object))
		{
			const FPartSeed* Seed = FindPartSeed(Entry.StableId);
			return Seed && ConfigurePart(*Part, *Seed, Resolve, OutErrors);
		}
		if (UEnemyDefinition* Enemy = Cast<UEnemyDefinition>(&Object))
		{
			const FEnemySeed* Seed = FindEnemySeed(Entry.StableId);
			return Seed && ConfigureEnemy(*Enemy, *Seed, Resolve, OutErrors);
		}
		if (UEncounterDefinition* Encounter = Cast<UEncounterDefinition>(&Object))
		{
			const FEncounterSeed* Seed = FindEncounterSeed(Entry.StableId);
			return Seed && ConfigureEncounter(*Encounter, *Seed, Resolve, OutErrors);
		}
		if (UWacomRunEventDefinition* Event = Cast<UWacomRunEventDefinition>(&Object))
		{
			return ConfigureEvent(*Event, Entry.StableId);
		}
		if (UWacomRunPickupDefinition* Pickup = Cast<UWacomRunPickupDefinition>(&Object))
		{
			return ConfigurePickup(*Pickup, Entry.StableId, Resolve, OutErrors);
		}
		if (UShopDefinition* Shop = Cast<UShopDefinition>(&Object))
		{
			return ConfigureShop(*Shop, Resolve, OutErrors);
		}
		OutErrors.Add(FString::Printf(TEXT("Unsupported manifest class for %s"),
			*Entry.PackagePath));
		return false;
	}

	void AddEnemyGraphManifest(
		TArray<FFormalProductionContentManifestEntry>& Manifest,
		const FEnemySeed& Seed)
	{
		Manifest.Add({EFormalProductionContentGroup::EnemyGraph,
			BehaviorPackage(Seed.Archetype),
			*FString::Printf(TEXT("MoltCavern.%s.Behavior"), Seed.Archetype),
			UEnemyBehaviorDefinition::StaticClass()});
		for (const TCHAR* PartSlot : Seed.PartSlots)
		{
			Manifest.Add({EFormalProductionContentGroup::EnemyGraph,
				PartPackage(Seed.Archetype, PartSlot),
				*FString::Printf(TEXT("MoltCavern.%s.%s"), Seed.Archetype, PartSlot),
				UEnemyPartDefinition::StaticClass()});
		}
		Manifest.Add({EFormalProductionContentGroup::EnemyGraph,
			EnemyPackage(Seed.Archetype), Seed.EnemyId,
			UEnemyDefinition::StaticClass()});
	}

	TArray<FFormalProductionContentManifestEntry> BuildManifest()
	{
		TArray<FFormalProductionContentManifestEntry> Manifest;
		Manifest.Reserve(47);
		const TArray<TPair<FString, FName>> CardEntries =
		{
			{MakePackage(CardsRewardsRoot, TEXT("DA_Card_GlowcapPoultice")), TEXT("Reward.MoltCavern.GlowcapPoultice")},
			{MakePackage(CardsRewardsRoot, TEXT("DA_Card_CrystalWard")), TEXT("Reward.MoltCavern.CrystalWard")},
			{MakePackage(CardsRewardsRoot, TEXT("DA_Card_VenomShard")), TEXT("Reward.MoltCavern.VenomShard")},
			{MakePackage(CardsRunRoot, TEXT("DA_Card_MoltSeal")), TEXT("Card.Run.MoltSeal")},
			{BranchCardPackage(TEXT("ScaleCrawler"), TEXT("Aid")), TEXT("Reward.MoltCavern.ScaleCrawler.Aid")},
			{BranchCardPackage(TEXT("ScaleCrawler"), TEXT("Destroy")), TEXT("Reward.MoltCavern.ScaleCrawler.Destroy")},
			{BranchCardPackage(TEXT("StoneScaleGuard"), TEXT("Aid")), TEXT("Reward.MoltCavern.StoneScaleGuard.Aid")},
			{BranchCardPackage(TEXT("StoneScaleGuard"), TEXT("Destroy")), TEXT("Reward.MoltCavern.StoneScaleGuard.Destroy")},
			{BranchCardPackage(TEXT("VenomHunter"), TEXT("Aid")), TEXT("Reward.MoltCavern.VenomHunter.Aid")},
			{BranchCardPackage(TEXT("VenomHunter"), TEXT("Destroy")), TEXT("Reward.MoltCavern.VenomHunter.Destroy")},
			{BranchCardPackage(TEXT("CavernGuardian"), TEXT("Aid")), TEXT("Reward.MoltCavern.CavernGuardian.Aid")},
			{BranchCardPackage(TEXT("CavernGuardian"), TEXT("Destroy")), TEXT("Reward.MoltCavern.CavernGuardian.Destroy")},
		};
		for (const auto& Entry : CardEntries)
		{
			Manifest.Add({EFormalProductionContentGroup::Cards,
				Entry.Key, Entry.Value, UCardDefinition::StaticClass()});
		}
		for (const FEnemySeed& Seed : EnemySeeds())
		{
			AddEnemyGraphManifest(Manifest, Seed);
		}
		for (const FEncounterSeed& Seed : EncounterSeeds())
		{
			Manifest.Add({EFormalProductionContentGroup::NodeDefinitions,
				MakePackage(EncountersRoot, Seed.Leaf), Seed.Id,
				UEncounterDefinition::StaticClass()});
		}
		const TArray<TPair<const TCHAR*, const TCHAR*>> EventEntries =
		{
			{TEXT("Event.MoltCavern.CastoffEcho"), TEXT("DA_Event_CastoffEcho")},
			{TEXT("Event.MoltCavern.LostDelver"), TEXT("DA_Event_LostDelver")},
			{TEXT("Event.MoltCavern.MoltingRite"), TEXT("DA_Event_MoltingRite")},
		};
		for (const auto& Entry : EventEntries)
		{
			Manifest.Add({EFormalProductionContentGroup::NodeDefinitions,
				MakePackage(EventsRoot, Entry.Value), Entry.Key,
				UWacomRunEventDefinition::StaticClass()});
		}
		const TArray<TPair<const TCHAR*, const TCHAR*>> PickupEntries =
		{
			{TEXT("Pickup.MoltCavern.FungalCache"), TEXT("DA_Pickup_FungalCache")},
			{TEXT("Pickup.MoltCavern.MineralCache"), TEXT("DA_Pickup_MineralCache")},
			{TEXT("Pickup.MoltCavern.VenomCrystalCache"), TEXT("DA_Pickup_VenomCrystalCache")},
			{TEXT("Pickup.MoltCavern.MoltSeal"), TEXT("DA_Pickup_MoltSeal")},
		};
		for (const auto& Entry : PickupEntries)
		{
			Manifest.Add({EFormalProductionContentGroup::NodeDefinitions,
				MakePackage(PickupsRoot, Entry.Value), Entry.Key,
				UWacomRunPickupDefinition::StaticClass()});
		}
		Manifest.Add({EFormalProductionContentGroup::NodeDefinitions,
			MakePackage(ShopsRoot, TEXT("DA_Shop_DeepWayfarer")),
			TEXT("Shop.MoltCavern.DeepWayfarer"), UShopDefinition::StaticClass()});
		return Manifest;
	}

	const FFormalProductionContentProfile& GetProfile()
	{
		static const FFormalProductionContentProfile Profile = []
		{
			FFormalProductionContentProfile Result;
			Result.LogLabel = TEXT("FormalFloor2Content");
			Result.ReportFolder = TEXT("FormalFloor2Content");
			Result.Manifest = &GetFormalFloor2ContentManifest();
			Result.ExpectedCardsCount = 12;
			Result.ExpectedEnemyGraphCount = 20;
			Result.ExpectedNodeDefinitionsCount = 15;
			Result.ExpectedClassCounts =
			{
				{UCardDefinition::StaticClass(), 12},
				{UEnemyBehaviorDefinition::StaticClass(), 4},
				{UEnemyPartDefinition::StaticClass(), 12},
				{UEnemyDefinition::StaticClass(), 4},
				{UEncounterDefinition::StaticClass(), 7},
				{UWacomRunEventDefinition::StaticClass(), 3},
				{UWacomRunPickupDefinition::StaticClass(), 4},
				{UShopDefinition::StaticClass(), 1},
			};
			Result.ReadOnlyDependencies =
			{
				HerbalPoulticePackage,
				ChitinWardPackage,
				MoltCutPackage,
			};
			Result.ConfigureExpected = ConfigureExpected;
			Result.ValidateProfileSpecific = [](TArray<FString>& OutErrors)
			{
				if (CardSeeds().Num() != 12 || PartSeeds().Num() != 12
					|| EnemySeeds().Num() != 4 || IntentSeeds().Num() != 26
					|| EncounterSeeds().Num() != 7)
				{
					OutErrors.Add(TEXT("Floor 2 seed table counts do not match Spec 017"));
				}
				return OutErrors.IsEmpty();
			};
			return Result;
		}();
		return Profile;
	}
}

namespace Wacom::ContentBuilder
{
	const TArray<FFormalFloor2ContentManifestEntry>& GetFormalFloor2ContentManifest()
	{
		static const TArray<FFormalFloor2ContentManifestEntry> Manifest =
			FormalFloor2Private::BuildManifest();
		return Manifest;
	}

	bool ParseFormalFloor2ContentOptions(
		const TArray<FString>& Arguments,
		FFormalFloor2ContentOptions& OutOptions,
		TArray<FString>& OutErrors)
	{
		return ParseFormalProductionContentOptions(Arguments, OutOptions, OutErrors);
	}

	bool ValidateFormalFloor2ContentManifest(TArray<FString>& OutErrors)
	{
		return ValidateFormalProductionContentManifest(
			FormalFloor2Private::GetProfile(), OutErrors);
	}

	bool ValidateFormalFloor2TransientDefaults(TArray<FString>& OutErrors)
	{
		if (!ValidateFormalFloor2ContentManifest(OutErrors))
		{
			return false;
		}
		TMap<FString, UObject*> ObjectsByPackage;
		TArray<TStrongObjectPtr<UObject>> KeepAlive;
		KeepAlive.Reserve(47);
		for (const auto& Entry : GetFormalFloor2ContentManifest())
		{
			TStrongObjectPtr<UObject> Expected(nullptr);
			TArray<FString> EntryErrors;
			if (!BuildFormalProductionExpectedObject(
				FormalFloor2Private::GetProfile(), Entry, ObjectsByPackage,
				Expected, EntryErrors))
			{
				for (const FString& Error : EntryErrors)
				{
					OutErrors.Add(Entry.StableId.ToString() + TEXT(": ") + Error);
				}
				return false;
			}
			ObjectsByPackage.Add(Entry.PackagePath, Expected.Get());
			KeepAlive.Add(MoveTemp(Expected));
		}

		int32 IntentCount = 0;
		int32 ChoiceCount = 0;
		int32 ExplicitPartRewardCount = 0;
		for (const TStrongObjectPtr<UObject>& Object : KeepAlive)
		{
			if (const UEnemyBehaviorDefinition* Behavior =
				Cast<UEnemyBehaviorDefinition>(Object.Get()))
			{
				for (const FWacomEnemyPhaseDefinition& Phase : Behavior->Phases)
				{
					for (const FWacomEnemyIntentSetDefinition& Set : Phase.IntentSets)
					{
						IntentCount += Set.Intents.Num();
					}
				}
			}
			else if (const UWacomRunEventDefinition* Event =
				Cast<UWacomRunEventDefinition>(Object.Get()))
			{
				for (const FWacomRunEventNodeDefinition& Node : Event->Nodes)
				{
					ChoiceCount += Node.Choices.Num();
				}
			}
			else if (const UEnemyPartDefinition* Part =
				Cast<UEnemyPartDefinition>(Object.Get()))
			{
				if (Part->AidRewardCard && Part->DestroyRewardCard
					&& Part->AidRewardCard != Part->DestroyRewardCard
					&& !Part->KnockdownRewardCard)
				{
					++ExplicitPartRewardCount;
				}
			}
		}
		if (IntentCount != 26)
		{
			OutErrors.Add(FString::Printf(TEXT("Expected 26 configured Intents, got %d"),
				IntentCount));
		}
		if (ChoiceCount != 10)
		{
			OutErrors.Add(FString::Printf(TEXT("Expected 10 configured Choices, got %d"),
				ChoiceCount));
		}
		if (ExplicitPartRewardCount != 12)
		{
			OutErrors.Add(FString::Printf(
				TEXT("Expected 12 explicit branch-reward Parts, got %d"),
				ExplicitPartRewardCount));
		}

		const FString GuardianPackage = FormalFloor2Private::BranchCardPackage(
			TEXT("CavernGuardian"), TEXT("Destroy"));
		const UCardDefinition* GuardianDestroy =
			Cast<UCardDefinition>(ObjectsByPackage.FindRef(GuardianPackage));
		if (!GuardianDestroy || GuardianDestroy->BaseCost != 2
			|| GuardianDestroy->Rarity != WacomTags::Card_Rarity_Yellow
			|| GuardianDestroy->TargetMode != ECardTargetMode::AllEnemyParts
			|| GuardianDestroy->Effects.Num() != 2
			|| GuardianDestroy->Effects[0].Magnitude != 5
			|| GuardianDestroy->Effects[1].Magnitude != 2
			|| GuardianDestroy->Effects[0].Target != WacomTags::Target_AllEnemyParts
			|| GuardianDestroy->Effects[1].Target != WacomTags::Target_AllEnemyParts)
		{
			OutErrors.Add(TEXT("CavernGuardian Destroy contract mismatch"));
		}

		const UWacomRunPickupDefinition* MoltSeal = Cast<UWacomRunPickupDefinition>(
			ObjectsByPackage.FindRef(FormalFloor2Private::MakePackage(
				FormalFloor2Private::PickupsRoot, TEXT("DA_Pickup_MoltSeal"))));
		if (!MoltSeal || !MoltSeal->CardDefinition
			|| MoltSeal->CardDefinition->CardId != TEXT("Card.Run.MoltSeal")
			|| MoltSeal->GrantedCredentialIds.Num() != 1
			|| MoltSeal->GrantedCredentialIds[0] != TEXT("Credential.Run.MoltSeal"))
		{
			OutErrors.Add(TEXT("MoltSeal Pickup card/credential contract mismatch"));
		}
		return OutErrors.IsEmpty();
	}

	bool ValidateFormalFloor2ComparatorBoundaries(TArray<FString>& OutErrors)
	{
		const auto* Entry = GetFormalFloor2ContentManifest().FindByPredicate(
			[](const auto& Candidate)
			{
				return Candidate.StableId == TEXT("Reward.MoltCavern.GlowcapPoultice");
			});
		if (!Entry)
		{
			OutErrors.Add(TEXT("GlowcapPoultice manifest entry is missing"));
			return false;
		}
		TMap<FString, UObject*> ObjectsByPackage;
		TStrongObjectPtr<UObject> Expected(nullptr);
		if (!BuildFormalProductionExpectedObject(
			FormalFloor2Private::GetProfile(), *Entry, ObjectsByPackage,
			Expected, OutErrors))
		{
			return false;
		}
		TStrongObjectPtr<UCardDefinition> Tuned(
			DuplicateObject(CastChecked<UCardDefinition>(Expected.Get()),
				GetTransientPackage()));
		Tuned->DisplayName = FText::FromString(TEXT("人工调优名称"));
		Tuned->Description = FText::FromString(TEXT("人工调优描述"));
		Tuned->BaseCost = 2;
		Tuned->Effects[0].Magnitude = 7;
		TArray<FString> StructuralErrors;
		if (!CompareFormalProductionEditableProperties(
			*Tuned.Get(), *Expected.Get(), false, StructuralErrors))
		{
			OutErrors.Add(TEXT("Structural comparison rejected approved tunable drift"));
			OutErrors.Append(StructuralErrors);
		}
		TArray<FString> StrictErrors;
		if (CompareFormalProductionEditableProperties(
			*Tuned.Get(), *Expected.Get(), true, StrictErrors))
		{
			OutErrors.Add(TEXT("Strict comparison accepted seed-default drift"));
		}
		Tuned->CardId = TEXT("Reward.MoltCavern.InvalidIdentity");
		StructuralErrors.Reset();
		if (CompareFormalProductionEditableProperties(
			*Tuned.Get(), *Expected.Get(), false, StructuralErrors))
		{
			OutErrors.Add(TEXT("Structural comparison accepted stable identity drift"));
		}
		return OutErrors.IsEmpty();
	}

	bool ValidateFormalFloor2LoadedAssets(
		const bool bCompareSeedDefaults,
		TArray<FString>& OutErrors)
	{
		return ValidateFormalProductionLoadedAssets(
			FormalFloor2Private::GetProfile(), bCompareSeedDefaults, OutErrors);
	}

	bool ValidateFormalFloor2DependencyClosure(TArray<FString>& OutErrors)
	{
		return ValidateFormalProductionDependencyClosure(
			FormalFloor2Private::GetProfile(), OutErrors);
	}

	int32 RunFormalFloor2ContentBuilder(
		const TArray<FString>& Arguments,
		FFormalFloor2ContentBuildReport* OutReport)
	{
		return RunFormalProductionContentSeedService(
			FormalFloor2Private::GetProfile(), Arguments, OutReport);
	}
}
