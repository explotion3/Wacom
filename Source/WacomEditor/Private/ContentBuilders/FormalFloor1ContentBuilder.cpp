// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/FormalFloor1ContentBuilder.h"

#include "Cards/CardDefinition.h"
#include "ContentBuilders/ContentBuilderHelpers.h"
#include "Dom/JsonObject.h"
#include "Encounters/EncounterDefinition.h"
#include "Enemies/EnemyBehaviorDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Events/RunEventDefinition.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Misc/DateTime.h"
#include "Pickups/RunPickupDefinition.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Shops/ShopDefinition.h"
#include "Tags/WacomGameplayTags.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "Validation/CardDefinitionValidation.h"
#include "Validation/EncounterDefinitionValidation.h"
#include "Validation/EnemyBehaviorDefinitionValidation.h"
#include "Validation/EnemyDefinitionValidation.h"
#include "Validation/EnemyPartDefinitionValidation.h"
#include "Validation/RunEventDefinitionValidation.h"
#include "Validation/RunPickupDefinitionValidation.h"
#include "Validation/ShopDefinitionValidation.h"

namespace
{
	using namespace Wacom::ContentBuilder;

	constexpr int32 ReportSchemaVersion = 1;

	const FString CardsRewardsRoot = TEXT("/Game/Wacom/Data/Cards/Rewards/SerpentWood");
	const FString CardsRunRoot = TEXT("/Game/Wacom/Data/Cards/Run/SerpentWood");
	const FString EnemiesRoot = TEXT("/Game/Wacom/Data/Enemies/SerpentWood");
	const FString EncountersRoot = TEXT("/Game/Wacom/Data/Encounters/SerpentWood");
	const FString EventsRoot = TEXT("/Game/Wacom/Data/Events/SerpentWood");
	const FString PickupsRoot = TEXT("/Game/Wacom/Data/Pickups/SerpentWood");
	const FString ShopsRoot = TEXT("/Game/Wacom/Data/Shops/SerpentWood");

	const FString ChitinWardPackage =
		TEXT("/Game/Wacom/Data/Cards/BugGirl/StarterPack/DA_Card_Starter_ChitinWard");
	const FString AntennaSearchPackage =
		TEXT("/Game/Wacom/Data/Cards/BugGirl/StarterPack/DA_Card_Starter_AntennaSearch");
	const FString MoltCutPackage =
		TEXT("/Game/Wacom/Data/Cards/BugGirl/StarterPack/DA_Card_Starter_MoltCut");
	const FString PoisonFangPackage =
		TEXT("/Game/Wacom/Data/Cards/Rewards/DA_Card_PoisonFang");

	enum class ESeedEffect : uint8
	{
		Damage,
		Heal,
		Draw,
		Poison,
		Slow,
		Twilight,
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
		int32 Resistance;
		ESeedEffect Effect;
		int32 Magnitude;
		ESeedTarget Target;
		EHandAfflictionSelection HandSelection = EHandAfflictionSelection::Default;
		int32 TargetCardCount = 1;
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

	FString ObjectPathForPackage(const FString& PackagePath)
	{
		return PackagePath + TEXT(".")
			+ FPackageName::GetLongPackageAssetName(PackagePath);
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
		case ESeedEffect::Twilight: return Effect_ApplyStatus_Twilight;
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
			{ TEXT("Reward.SerpentWood.HerbalPoultice"), TEXT("草药敷剂"),
				TEXT("恢复 {Effect.0} 点生命。"), 1, &Card_Rarity_White,
				&Card_Keyword_Tool, ECardTargetMode::None,
				{{ESeedEffect::Heal, 4, ESeedTarget::Player}} },
			{ TEXT("Reward.SerpentWood.HunterSnare"), TEXT("猎人绊索"),
				TEXT("使一个敌方部位的当前意图延后 {Effect.0} 点先机。"), 1,
				&Card_Rarity_White, &Card_Keyword_Tool,
				ECardTargetMode::SingleEnemyPart,
				{{ESeedEffect::Slow, 2, ESeedTarget::SingleEnemyPart}} },
			{ TEXT("Reward.SerpentWood.MoltWard"), TEXT("蜕壳护符"),
				TEXT("获得 {Effect.0} 点护盾。"), 0, &Card_Rarity_Blue,
				&Card_Keyword_Tool, ECardTargetMode::None,
				{{ESeedEffect::Shield, 3, ESeedTarget::Player}} },
			{ TEXT("Card.Run.SerpentSigil"), TEXT("浅巢蛇印"),
				TEXT("从抽牌堆抽取 {Effect.0} 张牌。"), 1, &Card_Rarity_White,
				nullptr, ECardTargetMode::None,
				{{ESeedEffect::Draw, 1, ESeedTarget::Player, true}} },
			{ TEXT("Reward.SerpentWood.BrushSnake.Aid"), TEXT("伏草藏身"),
				TEXT("获得 {Effect.0} 护盾，使一个敌方部位的当前意图延后 {Effect.1} 点先机。"),
				1, &Card_Rarity_White, &Card_Keyword_Tool,
				ECardTargetMode::SingleEnemyPart,
				{{ESeedEffect::Shield, 2, ESeedTarget::Player},
				 {ESeedEffect::Slow, 1, ESeedTarget::SingleEnemyPart}} },
			{ TEXT("Reward.SerpentWood.BrushSnake.Destroy"), TEXT("断牙毒刺"),
				TEXT("对一个敌方部位造成 {Effect.0} 点伤害并施加 {Effect.1} 层中毒。"),
				1, &Card_Rarity_White, &Card_Keyword_Weapon,
				ECardTargetMode::SingleEnemyPart,
				{{ESeedEffect::Damage, 3, ESeedTarget::SingleEnemyPart},
				 {ESeedEffect::Poison, 1, ESeedTarget::SingleEnemyPart}} },
			{ TEXT("Reward.SerpentWood.MoltGuard.Aid"), TEXT("蜕甲壁垒"),
				TEXT("获得 {Effect.0} 护盾。"), 1, &Card_Rarity_Blue,
				&Card_Keyword_Tool, ECardTargetMode::None,
				{{ESeedEffect::Shield, 7, ESeedTarget::Player}} },
			{ TEXT("Reward.SerpentWood.MoltGuard.Destroy"), TEXT("裂壳重击"),
				TEXT("对一个敌方部位造成 {Effect.0} 点伤害。"), 1,
				&Card_Rarity_Blue, &Card_Keyword_Weapon,
				ECardTargetMode::SingleEnemyPart,
				{{ESeedEffect::Damage, 6, ESeedTarget::SingleEnemyPart}} },
			{ TEXT("Reward.SerpentWood.RootStalker.Aid"), TEXT("盘根护身"),
				TEXT("获得 {Effect.0} 护盾，使一个敌方部位的当前意图延后 {Effect.1} 点先机。"),
				1, &Card_Rarity_Blue, &Card_Keyword_Tool,
				ECardTargetMode::SingleEnemyPart,
				{{ESeedEffect::Shield, 3, ESeedTarget::Player},
				 {ESeedEffect::Slow, 2, ESeedTarget::SingleEnemyPart}} },
			{ TEXT("Reward.SerpentWood.RootStalker.Destroy"), TEXT("毒根突袭"),
				TEXT("对一个敌方部位造成 {Effect.0} 点伤害并施加 {Effect.1} 层中毒。"),
				1, &Card_Rarity_Blue, &Card_Keyword_Weapon,
				ECardTargetMode::SingleEnemyPart,
				{{ESeedEffect::Damage, 5, ESeedTarget::SingleEnemyPart},
				 {ESeedEffect::Poison, 1, ESeedTarget::SingleEnemyPart}} },
			{ TEXT("Reward.SerpentWood.ShallowGuardian.Aid"), TEXT("冠鳞庇护"),
				TEXT("获得 {Effect.0} 护盾。"), 1, &Card_Rarity_Yellow,
				&Card_Keyword_Tool, ECardTargetMode::None,
				{{ESeedEffect::Shield, 10, ESeedTarget::Player}} },
			{ TEXT("Reward.SerpentWood.ShallowGuardian.Destroy"), TEXT("碎冠毒潮"),
				TEXT("对所有存活敌方部位造成 {Effect.0} 点伤害并施加 {Effect.1} 层中毒。"),
				2, &Card_Rarity_Yellow, &Card_Keyword_Weapon,
				ECardTargetMode::AllEnemyParts,
				{{ESeedEffect::Damage, 4, ESeedTarget::AllEnemyParts},
				 {ESeedEffect::Poison, 1, ESeedTarget::AllEnemyParts}} },
		};
		return Seeds;
	}

	const TArray<FIntentSeed>& IntentSeeds()
	{
		static const TArray<FIntentSeed> Seeds =
		{
			{TEXT("BrushSnake"), TEXT("Head"), TEXT("Bite"), TEXT("啃咬"), 3, 3, ESeedEffect::Damage, 3, ESeedTarget::Player},
			{TEXT("BrushSnake"), TEXT("Head"), TEXT("Venom"), TEXT("注毒"), 5, 0, ESeedEffect::Poison, 1, ESeedTarget::Player},
			{TEXT("BrushSnake"), TEXT("Body"), TEXT("Rush"), TEXT("突进"), 2, 2, ESeedEffect::Damage, 2, ESeedTarget::Player},
			{TEXT("BrushSnake"), TEXT("Body"), TEXT("Coil"), TEXT("缠绕"), 4, 0, ESeedEffect::Slow, 1, ESeedTarget::Player},
			{TEXT("BrushSnake"), TEXT("Body"), TEXT("Hide"), TEXT("藏身"), 2, 0, ESeedEffect::Shield, 2, ESeedTarget::Self},
			{TEXT("MoltGuard"), TEXT("Head"), TEXT("Snap"), TEXT("猛咬"), 3, 4, ESeedEffect::Damage, 4, ESeedTarget::Player},
			{TEXT("MoltGuard"), TEXT("Head"), TEXT("Spit"), TEXT("喷毒"), 5, 0, ESeedEffect::Poison, 1, ESeedTarget::Player},
			{TEXT("MoltGuard"), TEXT("Carapace"), TEXT("Harden"), TEXT("硬化"), 2, 0, ESeedEffect::Shield, 5, ESeedTarget::Self},
			{TEXT("MoltGuard"), TEXT("Carapace"), TEXT("Slam"), TEXT("重压"), 4, 5, ESeedEffect::Damage, 4, ESeedTarget::Player},
			{TEXT("MoltGuard"), TEXT("Tail"), TEXT("Sweep"), TEXT("横扫"), 2, 2, ESeedEffect::Damage, 2, ESeedTarget::Player},
			{TEXT("MoltGuard"), TEXT("Tail"), TEXT("Brace"), TEXT("支撑"), 2, 0, ESeedEffect::Shield, 2, ESeedTarget::Self},
			{TEXT("RootStalker"), TEXT("Head"), TEXT("Lunge"), TEXT("突刺"), 4, 5, ESeedEffect::Damage, 5, ESeedTarget::Player},
			{TEXT("RootStalker"), TEXT("Head"), TEXT("Sap"), TEXT("毒液"), 3, 0, ESeedEffect::Poison, 1, ESeedTarget::Player},
			{TEXT("RootStalker"), TEXT("Coil"), TEXT("Tangle"), TEXT("盘根"), 4, 0, ESeedEffect::Slow, 2, ESeedTarget::Player},
			{TEXT("RootStalker"), TEXT("Coil"), TEXT("Crush"), TEXT("绞杀"), 3, 4, ESeedEffect::Damage, 4, ESeedTarget::Player},
			{TEXT("RootStalker"), TEXT("Coil"), TEXT("RootGuard"), TEXT("根护"), 2, 0, ESeedEffect::Shield, 3, ESeedTarget::Self},
			{TEXT("ShallowGuardian"), TEXT("Head"), TEXT("Bite"), TEXT("噬咬"), 3, 6, ESeedEffect::Damage, 6, ESeedTarget::Player},
			{TEXT("ShallowGuardian"), TEXT("Head"), TEXT("Venom"), TEXT("剧毒"), 5, 0, ESeedEffect::Poison, 2, ESeedTarget::Player},
			{TEXT("ShallowGuardian"), TEXT("Body"), TEXT("Crush"), TEXT("碾压"), 4, 7, ESeedEffect::Damage, 6, ESeedTarget::Player},
			{TEXT("ShallowGuardian"), TEXT("Body"), TEXT("Harden"), TEXT("鳞甲硬化"), 2, 0, ESeedEffect::Shield, 6, ESeedTarget::Self},
			{TEXT("ShallowGuardian"), TEXT("Tail"), TEXT("Sweep"), TEXT("尾扫"), 2, 4, ESeedEffect::Damage, 4, ESeedTarget::Player},
			{TEXT("ShallowGuardian"), TEXT("Tail"), TEXT("Tangle"), TEXT("绞缠"), 3, 0, ESeedEffect::Slow, 1, ESeedTarget::Player},
			{TEXT("ShallowGuardian"), TEXT("Crest"), TEXT("Dread"), TEXT("暮气"), 5, 0, ESeedEffect::Twilight, 1, ESeedTarget::Player, EHandAfflictionSelection::AllCurrentHandCards},
			{TEXT("ShallowGuardian"), TEXT("Crest"), TEXT("CrownGuard"), TEXT("冠护"), 2, 0, ESeedEffect::Shield, 4, ESeedTarget::Self},
		};
		return Seeds;
	}

	const TArray<FPartSeed>& PartSeeds()
	{
		static const TArray<FPartSeed> Seeds =
		{
			{TEXT("BrushSnake"), TEXT("Head"), TEXT("头部"), 7, 1},
			{TEXT("BrushSnake"), TEXT("Body"), TEXT("躯体"), 9, 1},
			{TEXT("MoltGuard"), TEXT("Head"), TEXT("头部"), 8, 1},
			{TEXT("MoltGuard"), TEXT("Carapace"), TEXT("甲壳"), 14, 2},
			{TEXT("MoltGuard"), TEXT("Tail"), TEXT("尾部"), 6, 1},
			{TEXT("RootStalker"), TEXT("Head"), TEXT("头部"), 10, 2},
			{TEXT("RootStalker"), TEXT("Coil"), TEXT("盘身"), 16, 2},
			{TEXT("ShallowGuardian"), TEXT("Head"), TEXT("头部"), 14, 2},
			{TEXT("ShallowGuardian"), TEXT("Body"), TEXT("躯体"), 22, 4},
			{TEXT("ShallowGuardian"), TEXT("Tail"), TEXT("尾部"), 10, 2},
			{TEXT("ShallowGuardian"), TEXT("Crest"), TEXT("冠鳞"), 6, 1},
		};
		return Seeds;
	}

	const TArray<FEnemySeed>& EnemySeeds()
	{
		static const TArray<FEnemySeed> Seeds =
		{
			{TEXT("BrushSnake"), TEXT("Enemy.SerpentWood.BrushSnake"), TEXT("林地伏蛇"), {TEXT("Head"), TEXT("Body")}},
			{TEXT("MoltGuard"), TEXT("Enemy.SerpentWood.MoltGuard"), TEXT("蛇蜕守卫"), {TEXT("Head"), TEXT("Carapace"), TEXT("Tail")}},
			{TEXT("RootStalker"), TEXT("Enemy.SerpentWood.RootStalker"), TEXT("盘根伏蛇"), {TEXT("Head"), TEXT("Coil")}},
			{TEXT("ShallowGuardian"), TEXT("Enemy.SerpentWood.ShallowGuardian"), TEXT("浅巢守卫"), {TEXT("Head"), TEXT("Body"), TEXT("Tail"), TEXT("Crest")}},
		};
		return Seeds;
	}

	const FCardSeed* FindCardSeed(FName CardId)
	{
		return CardSeeds().FindByPredicate(
			[CardId](const FCardSeed& Seed) { return CardId == FName(Seed.CardId); });
	}

	const FPartSeed* FindPartSeed(FName PartId)
	{
		return PartSeeds().FindByPredicate([PartId](const FPartSeed& Seed)
		{
			return PartId == FName(*FString::Printf(
				TEXT("SerpentWood.%s.%s"), Seed.Archetype, Seed.PartSlot));
		});
	}

	const FEnemySeed* FindEnemySeed(FName EnemyId)
	{
		return EnemySeeds().FindByPredicate(
			[EnemyId](const FEnemySeed& Seed) { return EnemyId == FName(Seed.EnemyId); });
	}

	const FEnemySeed* FindBehaviorEnemySeed(FName BehaviorId)
	{
		return EnemySeeds().FindByPredicate([BehaviorId](const FEnemySeed& Seed)
		{
			return BehaviorId == FName(*FString::Printf(
				TEXT("SerpentWood.%s.Behavior"), Seed.Archetype));
		});
	}

	using FResolveObject = TFunction<UObject*(const FString&)>;

	bool ConfigureCard(
		UCardDefinition& Card,
		const FCardSeed& Seed)
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
				? WacomTags::CardLocation_Draw.GetTag()
				: FGameplayTag();
			Effect.Duration = 0;
			Effect.MagnitudeSource = FGameplayTag();
		}
		Card.PerfectReleaseEffects.Reset();
		Card.ZoneHooks.Reset();
		Card.Passives.Reset();
		return true;
	}

	bool ConfigureBehavior(
		UEnemyBehaviorDefinition& Behavior,
		const FEnemySeed& Seed)
	{
		Behavior.BehaviorId = *FString::Printf(
			TEXT("SerpentWood.%s.Behavior"), Seed.Archetype);
		Behavior.InitialPhaseId = TEXT("Default");
		Behavior.Phases.Reset();
		FWacomEnemyPhaseDefinition& Phase = Behavior.Phases.AddDefaulted_GetRef();
		Phase.PhaseId = TEXT("Default");
		for (const TCHAR* PartSlot : Seed.PartSlots)
		{
			FWacomEnemyIntentSetDefinition& Set =
				Phase.IntentSets.AddDefaulted_GetRef();
			Set.IntentSetId = *FString::Printf(
				TEXT("SerpentWood.%s.%s.Sequence"), Seed.Archetype, PartSlot);
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
				FWacomEnemyBehaviorIntent& BehaviorIntent =
					Set.Intents.AddDefaulted_GetRef();
				BehaviorIntent.CooldownGroup = NAME_None;
				BehaviorIntent.CooldownSelections = 0;
				FIntentDefinition& Intent = BehaviorIntent.Intent;
				Intent.IntentId = *FString::Printf(
					TEXT("SerpentWood.%s.%s.%s"), Seed.Archetype,
					PartSlot, IntentSeed.Suffix);
				Intent.DisplayName = FText::FromString(IntentSeed.DisplayName);
				Intent.Initiative = IntentSeed.Initiative;
				Intent.ResistanceValue = IntentSeed.Resistance;
				FIntentEffect& Effect = Intent.Effects.AddDefaulted_GetRef();
				Effect.EffectType = EffectTag(IntentSeed.Effect);
				Effect.Magnitude = IntentSeed.Magnitude;
				Effect.Target = TargetTag(IntentSeed.Target);
				Effect.Duration = 0;
				Effect.HandAffliction.Selection = IntentSeed.HandSelection;
				Effect.HandAffliction.TargetCardCount = IntentSeed.TargetCardCount;
			}
		}
		return true;
	}

	bool ConfigurePart(
		UEnemyPartDefinition& Part,
		const FPartSeed& Seed,
		const FResolveObject& Resolve,
		TArray<FString>& OutErrors)
	{
		Part.PartId = *FString::Printf(
			TEXT("SerpentWood.%s.%s"), Seed.Archetype, Seed.PartSlot);
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
			OutErrors.Add(FString::Printf(
				TEXT("Could not resolve branch rewards for %s"),
				*Part.PartId.ToString()));
			return false;
		}
		return true;
	}

	bool ConfigureEnemy(
		UEnemyDefinition& Enemy,
		const FEnemySeed& Seed,
		const FResolveObject& Resolve,
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
				TEXT("SerpentWood.%s.%s.Sequence"), Seed.Archetype, PartSlot);
			if (!Slot.PartDef)
			{
				OutErrors.Add(FString::Printf(
					TEXT("Could not resolve %s.%s Part"), Seed.Archetype, PartSlot));
			}
		}
		if (!Enemy.DefaultBehavior)
		{
			OutErrors.Add(FString::Printf(
				TEXT("Could not resolve %s Behavior"), Seed.Archetype));
		}
		return OutErrors.IsEmpty();
	}

	struct FEncounterSeed
	{
		const TCHAR* Id;
		const TCHAR* DisplayName;
		TArray<TPair<const TCHAR*, const TCHAR*>> Slots;
	};

	const TArray<FEncounterSeed>& EncounterSeeds()
	{
		static const TArray<FEncounterSeed> Seeds =
		{
			{TEXT("Encounter.SerpentWood.Scout"), TEXT("教学伏击"), {{TEXT("Scout"), TEXT("BrushSnake")}}},
			{TEXT("Encounter.SerpentWood.MoltGuard"), TEXT("蛇蜕守卫"), {{TEXT("Guard"), TEXT("MoltGuard")}}},
			{TEXT("Encounter.SerpentWood.Ambush"), TEXT("毒雾伏击"), {{TEXT("Left"), TEXT("BrushSnake")}, {TEXT("Right"), TEXT("BrushSnake")}}},
			{TEXT("Encounter.SerpentWood.RootStalker"), TEXT("盘根伏蛇"), {{TEXT("Stalker"), TEXT("RootStalker")}}},
			{TEXT("Encounter.SerpentWood.EliteSentinel"), TEXT("精英巡猎者"), {{TEXT("Guard"), TEXT("MoltGuard")}, {TEXT("Scout"), TEXT("BrushSnake")}}},
			{TEXT("Encounter.SerpentWood.ShallowGuardian"), TEXT("浅巢守卫"), {{TEXT("Guardian"), TEXT("ShallowGuardian")}}},
		};
		return Seeds;
	}

	const FEncounterSeed* FindEncounterSeed(FName Id)
	{
		return EncounterSeeds().FindByPredicate(
			[Id](const FEncounterSeed& Seed) { return Id == FName(Seed.Id); });
	}

	FWacomRunEventConditionDefinition MinGold(const int32 Value)
	{
		FWacomRunEventConditionDefinition Condition;
		Condition.Type = EWacomRunEventConditionType::MinGold;
		Condition.Value = Value;
		return Condition;
	}

	FWacomRunEventConditionDefinition RunFlagSet(const TCHAR* FlagId)
	{
		FWacomRunEventConditionDefinition Condition;
		Condition.Type = EWacomRunEventConditionType::RunFlagSet;
		Condition.FlagId = FlagId;
		return Condition;
	}

	FWacomRunEventEffectDefinition AddGold(const int32 Value)
	{
		FWacomRunEventEffectDefinition Effect;
		Effect.Type = EWacomRunEventEffectType::AddGold;
		Effect.Value = Value;
		return Effect;
	}

	FWacomRunEventEffectDefinition AddPressure(
		const TCHAR* PressureType,
		const int32 Value)
	{
		FWacomRunEventEffectDefinition Effect;
		Effect.Type = EWacomRunEventEffectType::AddPressure;
		Effect.PressureType = PressureType;
		Effect.Value = Value;
		return Effect;
	}

	FWacomRunEventEffectDefinition SetRunFlag(const TCHAR* FlagId)
	{
		FWacomRunEventEffectDefinition Effect;
		Effect.Type = EWacomRunEventEffectType::SetRunFlag;
		Effect.FlagId = FlagId;
		return Effect;
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

	bool ConfigureEncounter(
		UEncounterDefinition& Encounter,
		const FEncounterSeed& Seed,
		const FResolveObject& Resolve,
		TArray<FString>& OutErrors)
	{
		Encounter.EncounterDefinitionId = Seed.Id;
		Encounter.DisplayName = FText::FromString(Seed.DisplayName);
		Encounter.EnemySlots.Reset();
		for (const TPair<const TCHAR*, const TCHAR*>& SeedSlot : Seed.Slots)
		{
			FEncounterEnemySlot& Slot = Encounter.EnemySlots.AddDefaulted_GetRef();
			Slot.EnemySlotId = SeedSlot.Key;
			Slot.EnemyDefinition = Cast<UEnemyDefinition>(
				Resolve(EnemyPackage(SeedSlot.Value)));
			if (!Slot.EnemyDefinition)
			{
				OutErrors.Add(FString::Printf(
					TEXT("Could not resolve Encounter enemy %s"), SeedSlot.Value));
			}
		}
		return OutErrors.IsEmpty();
	}

	bool ConfigureEvent(UWacomRunEventDefinition& Event, FName EventId)
	{
		static const FName MoltTrailKnown(TEXT("SerpentWood.MoltTrailKnown"));
		static const FName MarshRouteKnown(TEXT("SerpentWood.MarshRouteKnown"));
		Event.EventId = EventId;
		Event.StartNodeId = TEXT("Start");
		Event.Nodes.Reset();
		FWacomRunEventNodeDefinition& Node = Event.Nodes.AddDefaulted_GetRef();
		Node.NodeId = TEXT("Start");

		if (EventId == TEXT("Event.SerpentWood.CastSkin"))
		{
			Event.DisplayName = FText::FromString(TEXT("蛇蜕事件"));
			Node.TitleText = FText::FromString(TEXT("林间蛇蜕"));
			Node.BodyText = FText::FromString(TEXT("林地里留着一张尚有余温的完整蛇蜕。"));
			Node.Choices =
			{
				Choice(TEXT("StudyPattern"), TEXT("研究纹路"), {}, {SetRunFlag(*MoltTrailKnown.ToString())}),
				Choice(TEXT("SellSkin"), TEXT("卖掉蛇蜕"), {}, {AddGold(2), AddPressure(TEXT("Misdeed"), 2)}),
				Choice(TEXT("LeaveUntouched"), TEXT("原样留下")),
			};
			return true;
		}
		if (EventId == TEXT("Event.SerpentWood.HunterTrace"))
		{
			Event.DisplayName = FText::FromString(TEXT("猎人痕迹"));
			Node.TitleText = FText::FromString(TEXT("泥地遗迹"));
			Node.BodyText = FText::FromString(TEXT("毒雾边缘散落着猎人的行囊与断裂足迹。"));
			Node.Choices =
			{
				Choice(TEXT("ReadTrail"), TEXT("辨认足迹"), {}, {SetRunFlag(*MarshRouteKnown.ToString())}),
				Choice(TEXT("LootPack"), TEXT("搜走行囊"), {}, {AddGold(3), AddPressure(TEXT("Misdeed"), 3)}),
				Choice(TEXT("BuryRemains"), TEXT("掩埋遗骸"), {}, {AddPressure(TEXT("Misdeed"), -2)}),
			};
			return true;
		}
		if (EventId == TEXT("Event.SerpentWood.MerchantRumor"))
		{
			Event.DisplayName = FText::FromString(TEXT("行商情报"));
			Node.TitleText = FText::FromString(TEXT("林下行商"));
			Node.BodyText = FText::FromString(TEXT("行商压低声音，等你拿情报、金币或风险交换路线。"));
			Node.Choices =
			{
				Choice(TEXT("TradeMoltClue"), TEXT("交换蛇蜕线索"),
					{RunFlagSet(*MoltTrailKnown.ToString())}, {SetRunFlag(*MarshRouteKnown.ToString())}),
				Choice(TEXT("BuyMap"), TEXT("购买地图"), {MinGold(1)},
					{AddGold(-1), SetRunFlag(*MarshRouteKnown.ToString())}),
				Choice(TEXT("Eavesdrop"), TEXT("偷听传闻"), {},
					{AddPressure(TEXT("Misdeed"), 2), SetRunFlag(*MarshRouteKnown.ToString())}),
				Choice(TEXT("Decline"), TEXT("婉拒")),
			};
			return true;
		}
		if (EventId == TEXT("Event.SerpentWood.PoisonMarsh"))
		{
			Event.DisplayName = FText::FromString(TEXT("毒沼抉择"));
			Node.TitleText = FText::FromString(TEXT("毒沼边缘"));
			Node.BodyText = FText::FromString(TEXT("沼气遮住前路，标记、供品或硬闯都能带你穿过。"));
			Node.Choices =
			{
				Choice(TEXT("FollowMarkedRoute"), TEXT("沿标记路线"),
					{RunFlagSet(*MarshRouteKnown.ToString())}, {AddPressure(TEXT("Fatigue"), -2)}),
				Choice(TEXT("BurnOffering"), TEXT("焚烧供品"), {MinGold(2)}, {AddGold(-2)}),
				Choice(TEXT("WadeThrough"), TEXT("涉水硬闯"), {}, {AddPressure(TEXT("Fatigue"), 5)}),
			};
			return true;
		}
		return false;
	}

	bool ConfigurePickup(
		UWacomRunPickupDefinition& Pickup,
		FName PickupId,
		const FResolveObject& Resolve,
		TArray<FString>& OutErrors)
	{
		Pickup.PickupId = PickupId;
		Pickup.RewardType = EWacomRunPickupRewardType::Card;
		Pickup.GoldAmount = 1;
		Pickup.GrantedCredentialIds.Reset();
		FString CardPackage;
		if (PickupId == TEXT("Pickup.SerpentWood.HerbCache"))
		{
			CardPackage = MakePackage(CardsRewardsRoot, TEXT("DA_Card_HerbalPoultice"));
		}
		else if (PickupId == TEXT("Pickup.SerpentWood.HunterCache"))
		{
			CardPackage = MakePackage(CardsRewardsRoot, TEXT("DA_Card_HunterSnare"));
		}
		else if (PickupId == TEXT("Pickup.SerpentWood.MoltCache"))
		{
			CardPackage = MakePackage(CardsRewardsRoot, TEXT("DA_Card_MoltWard"));
		}
		else if (PickupId == TEXT("Pickup.SerpentWood.SerpentSigil"))
		{
			CardPackage = MakePackage(CardsRunRoot, TEXT("DA_Card_SerpentSigil"));
			Pickup.GrantedCredentialIds.Add(TEXT("Credential.Run.SerpentSigil"));
		}
		Pickup.CardDefinition = Cast<UCardDefinition>(Resolve(CardPackage));
		if (!Pickup.CardDefinition)
		{
			OutErrors.Add(FString::Printf(TEXT("Could not resolve Pickup card for %s"),
				*PickupId.ToString()));
			return false;
		}
		return true;
	}

	bool ConfigureShop(
		UShopDefinition& Shop,
		const FResolveObject& Resolve,
		TArray<FString>& OutErrors)
	{
		Shop.ShopId = TEXT("Shop.SerpentWood.Wayfarer");
		Shop.DisplayName = FText::FromString(TEXT("林下行商"));
		Shop.Offers.Reset();
		const TArray<TPair<FString, int32>> OfferSeeds =
		{
			{ChitinWardPackage, 2},
			{AntennaSearchPackage, 2},
			{MoltCutPackage, 3},
			{PoisonFangPackage, 2},
			{MakePackage(CardsRewardsRoot, TEXT("DA_Card_HerbalPoultice")), 2},
		};
		for (const TPair<FString, int32>& OfferSeed : OfferSeeds)
		{
			FShopOfferDefinition& Offer = Shop.Offers.AddDefaulted_GetRef();
			Offer.CardDefinition = Cast<UCardDefinition>(Resolve(OfferSeed.Key));
			Offer.Price = OfferSeed.Value;
			if (!Offer.CardDefinition)
			{
				OutErrors.Add(FString::Printf(
					TEXT("Could not resolve Shop card %s"), *OfferSeed.Key));
			}
		}
		return OutErrors.IsEmpty();
	}

	bool ConfigureExpected(
		UObject& Object,
		const FFormalFloor1ContentManifestEntry& Entry,
		const FResolveObject& Resolve,
		TArray<FString>& OutErrors)
	{
		if (UCardDefinition* Card = Cast<UCardDefinition>(&Object))
		{
			const FCardSeed* Seed = FindCardSeed(Entry.StableId);
			return Seed && ConfigureCard(*Card, *Seed);
		}
		if (UEnemyBehaviorDefinition* Behavior = Cast<UEnemyBehaviorDefinition>(&Object))
		{
			const FEnemySeed* Seed = FindBehaviorEnemySeed(Entry.StableId);
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
		TArray<FFormalFloor1ContentManifestEntry>& Manifest,
		const FEnemySeed& Seed)
	{
		Manifest.Add({EFormalFloor1ContentGroup::EnemyGraph,
			BehaviorPackage(Seed.Archetype),
			*FString::Printf(TEXT("SerpentWood.%s.Behavior"), Seed.Archetype),
			UEnemyBehaviorDefinition::StaticClass()});
		for (const TCHAR* PartSlot : Seed.PartSlots)
		{
			Manifest.Add({EFormalFloor1ContentGroup::EnemyGraph,
				PartPackage(Seed.Archetype, PartSlot),
				*FString::Printf(TEXT("SerpentWood.%s.%s"), Seed.Archetype, PartSlot),
				UEnemyPartDefinition::StaticClass()});
		}
		Manifest.Add({EFormalFloor1ContentGroup::EnemyGraph,
			EnemyPackage(Seed.Archetype), Seed.EnemyId,
			UEnemyDefinition::StaticClass()});
	}

	TArray<FFormalFloor1ContentManifestEntry> BuildManifest()
	{
		TArray<FFormalFloor1ContentManifestEntry> Manifest;
		Manifest.Reserve(46);
		const TArray<TPair<FString, FName>> CardEntries =
		{
			{MakePackage(CardsRewardsRoot, TEXT("DA_Card_HerbalPoultice")), TEXT("Reward.SerpentWood.HerbalPoultice")},
			{MakePackage(CardsRewardsRoot, TEXT("DA_Card_HunterSnare")), TEXT("Reward.SerpentWood.HunterSnare")},
			{MakePackage(CardsRewardsRoot, TEXT("DA_Card_MoltWard")), TEXT("Reward.SerpentWood.MoltWard")},
			{MakePackage(CardsRunRoot, TEXT("DA_Card_SerpentSigil")), TEXT("Card.Run.SerpentSigil")},
			{BranchCardPackage(TEXT("BrushSnake"), TEXT("Aid")), TEXT("Reward.SerpentWood.BrushSnake.Aid")},
			{BranchCardPackage(TEXT("BrushSnake"), TEXT("Destroy")), TEXT("Reward.SerpentWood.BrushSnake.Destroy")},
			{BranchCardPackage(TEXT("MoltGuard"), TEXT("Aid")), TEXT("Reward.SerpentWood.MoltGuard.Aid")},
			{BranchCardPackage(TEXT("MoltGuard"), TEXT("Destroy")), TEXT("Reward.SerpentWood.MoltGuard.Destroy")},
			{BranchCardPackage(TEXT("RootStalker"), TEXT("Aid")), TEXT("Reward.SerpentWood.RootStalker.Aid")},
			{BranchCardPackage(TEXT("RootStalker"), TEXT("Destroy")), TEXT("Reward.SerpentWood.RootStalker.Destroy")},
			{BranchCardPackage(TEXT("ShallowGuardian"), TEXT("Aid")), TEXT("Reward.SerpentWood.ShallowGuardian.Aid")},
			{BranchCardPackage(TEXT("ShallowGuardian"), TEXT("Destroy")), TEXT("Reward.SerpentWood.ShallowGuardian.Destroy")},
		};
		for (const TPair<FString, FName>& CardEntry : CardEntries)
		{
			Manifest.Add({EFormalFloor1ContentGroup::Cards,
				CardEntry.Key, CardEntry.Value, UCardDefinition::StaticClass()});
		}
		for (const FEnemySeed& Seed : EnemySeeds())
		{
			AddEnemyGraphManifest(Manifest, Seed);
		}
		for (const FEncounterSeed& Seed : EncounterSeeds())
		{
			const FString Suffix = FString(Seed.Id).RightChop(
				FCString::Strlen(TEXT("Encounter.SerpentWood.")));
			Manifest.Add({EFormalFloor1ContentGroup::NodeDefinitions,
				MakePackage(EncountersRoot,
					*FString::Printf(TEXT("DA_Encounter_%s"), *Suffix)),
				Seed.Id, UEncounterDefinition::StaticClass()});
		}
		const TArray<TPair<const TCHAR*, const TCHAR*>> EventEntries =
		{
			{TEXT("Event.SerpentWood.CastSkin"), TEXT("DA_Event_CastSkin")},
			{TEXT("Event.SerpentWood.HunterTrace"), TEXT("DA_Event_HunterTrace")},
			{TEXT("Event.SerpentWood.MerchantRumor"), TEXT("DA_Event_MerchantRumor")},
			{TEXT("Event.SerpentWood.PoisonMarsh"), TEXT("DA_Event_PoisonMarsh")},
		};
		for (const auto& EventEntry : EventEntries)
		{
			Manifest.Add({EFormalFloor1ContentGroup::NodeDefinitions,
				MakePackage(EventsRoot, EventEntry.Value), EventEntry.Key,
				UWacomRunEventDefinition::StaticClass()});
		}
		const TArray<TPair<const TCHAR*, const TCHAR*>> PickupEntries =
		{
			{TEXT("Pickup.SerpentWood.HerbCache"), TEXT("DA_Pickup_HerbCache")},
			{TEXT("Pickup.SerpentWood.HunterCache"), TEXT("DA_Pickup_HunterCache")},
			{TEXT("Pickup.SerpentWood.MoltCache"), TEXT("DA_Pickup_MoltCache")},
			{TEXT("Pickup.SerpentWood.SerpentSigil"), TEXT("DA_Pickup_SerpentSigil")},
		};
		for (const auto& PickupEntry : PickupEntries)
		{
			Manifest.Add({EFormalFloor1ContentGroup::NodeDefinitions,
				MakePackage(PickupsRoot, PickupEntry.Value), PickupEntry.Key,
				UWacomRunPickupDefinition::StaticClass()});
		}
		Manifest.Add({EFormalFloor1ContentGroup::NodeDefinitions,
			MakePackage(ShopsRoot, TEXT("DA_Shop_Wayfarer")),
			TEXT("Shop.SerpentWood.Wayfarer"), UShopDefinition::StaticClass()});
		return Manifest;
	}

	FString GroupToString(const EFormalFloor1ContentGroup Group)
	{
		switch (Group)
		{
		case EFormalFloor1ContentGroup::Cards: return TEXT("Cards");
		case EFormalFloor1ContentGroup::EnemyGraph: return TEXT("EnemyGraph");
		case EFormalFloor1ContentGroup::NodeDefinitions: return TEXT("NodeDefinitions");
		case EFormalFloor1ContentGroup::All:
		default: return TEXT("All");
		}
	}

	FString StateToString(const EFormalFloor1ContentEntryState State)
	{
		switch (State)
		{
		case EFormalFloor1ContentEntryState::NotProcessed: return TEXT("NotProcessed");
		case EFormalFloor1ContentEntryState::Missing: return TEXT("Missing");
		case EFormalFloor1ContentEntryState::Existing: return TEXT("Existing");
		case EFormalFloor1ContentEntryState::Created: return TEXT("Created");
		case EFormalFloor1ContentEntryState::Failed: return TEXT("Failed");
		default: return TEXT("Unknown");
		}
	}

	bool IsSelected(
		const FFormalFloor1ContentManifestEntry& Entry,
		const EFormalFloor1ContentGroup Group)
	{
		return Group == EFormalFloor1ContentGroup::All || Entry.Group == Group;
	}

	void AppendTextErrors(
		const TArray<FText>& TextErrors,
		TArray<FString>& OutErrors)
	{
		for (const FText& Error : TextErrors)
		{
			OutErrors.Add(Error.ToString());
		}
	}

	bool ValidateWithSharedRules(UObject& Object, TArray<FString>& OutErrors)
	{
		TArray<FText> Errors;
		bool bValid = false;
		if (const UCardDefinition* Card = Cast<UCardDefinition>(&Object))
		{
			bValid = FWacomCardDefinitionValidation::Validate(Card, Errors);
		}
		else if (const UEnemyPartDefinition* Part = Cast<UEnemyPartDefinition>(&Object))
		{
			bValid = FWacomEnemyPartDefinitionValidation::Validate(
				Part, Errors, EWacomEnemyPartValidationProfile::FormalProduction);
		}
		else if (const UEnemyBehaviorDefinition* Behavior =
			Cast<UEnemyBehaviorDefinition>(&Object))
		{
			bValid = FWacomEnemyBehaviorDefinitionValidation::Validate(Behavior, Errors);
		}
		else if (const UEnemyDefinition* Enemy = Cast<UEnemyDefinition>(&Object))
		{
			bValid = FWacomEnemyDefinitionValidation::Validate(Enemy, Errors);
		}
		else if (const UEncounterDefinition* Encounter = Cast<UEncounterDefinition>(&Object))
		{
			bValid = FWacomEncounterDefinitionValidation::Validate(Encounter, Errors);
		}
		else if (const UWacomRunEventDefinition* Event =
			Cast<UWacomRunEventDefinition>(&Object))
		{
			bValid = FWacomRunEventDefinitionValidation::Validate(Event, Errors);
		}
		else if (const UWacomRunPickupDefinition* Pickup =
			Cast<UWacomRunPickupDefinition>(&Object))
		{
			bValid = FWacomRunPickupDefinitionValidation::Validate(Pickup, Errors);
		}
		else if (const UShopDefinition* Shop = Cast<UShopDefinition>(&Object))
		{
			bValid = FWacomShopDefinitionValidation::Validate(Shop, Errors);
		}
		else
		{
			OutErrors.Add(FString::Printf(TEXT("No validator for class %s"),
				*GetNameSafe(Object.GetClass())));
			return false;
		}
		AppendTextErrors(Errors, OutErrors);
		return bValid;
	}

	void NormalizeCardTunables(UCardDefinition& Actual, const UCardDefinition& Expected)
	{
		Actual.DisplayName = Expected.DisplayName;
		Actual.Description = Expected.Description;
		Actual.CardIllustration = Expected.CardIllustration;
		Actual.CardIllustrationDepthMap = Expected.CardIllustrationDepthMap;
		Actual.BaseCost = Expected.BaseCost;
		Actual.Rarity = Expected.Rarity;
		if (Actual.Effects.Num() == Expected.Effects.Num())
		{
			for (int32 Index = 0; Index < Actual.Effects.Num(); ++Index)
			{
				Actual.Effects[Index].Magnitude = Expected.Effects[Index].Magnitude;
			}
		}
	}

	void NormalizeBehaviorTunables(
		UEnemyBehaviorDefinition& Actual,
		const UEnemyBehaviorDefinition& Expected)
	{
		if (Actual.Phases.Num() != Expected.Phases.Num())
		{
			return;
		}
		for (int32 PhaseIndex = 0; PhaseIndex < Actual.Phases.Num(); ++PhaseIndex)
		{
			auto& ActualSets = Actual.Phases[PhaseIndex].IntentSets;
			const auto& ExpectedSets = Expected.Phases[PhaseIndex].IntentSets;
			if (ActualSets.Num() != ExpectedSets.Num())
			{
				continue;
			}
			for (int32 SetIndex = 0; SetIndex < ActualSets.Num(); ++SetIndex)
			{
				auto& ActualIntents = ActualSets[SetIndex].Intents;
				const auto& ExpectedIntents = ExpectedSets[SetIndex].Intents;
				if (ActualIntents.Num() != ExpectedIntents.Num())
				{
					continue;
				}
				for (int32 IntentIndex = 0; IntentIndex < ActualIntents.Num(); ++IntentIndex)
				{
					FIntentDefinition& ActualIntent = ActualIntents[IntentIndex].Intent;
					const FIntentDefinition& ExpectedIntent = ExpectedIntents[IntentIndex].Intent;
					ActualIntent.DisplayName = ExpectedIntent.DisplayName;
					ActualIntent.Initiative = ExpectedIntent.Initiative;
					ActualIntent.ResistanceValue = ExpectedIntent.ResistanceValue;
					if (ActualIntent.Effects.Num() == ExpectedIntent.Effects.Num())
					{
						for (int32 EffectIndex = 0;
							EffectIndex < ActualIntent.Effects.Num(); ++EffectIndex)
						{
							ActualIntent.Effects[EffectIndex].Magnitude =
								ExpectedIntent.Effects[EffectIndex].Magnitude;
							ActualIntent.Effects[EffectIndex].HandAffliction.TargetCardCount =
								ExpectedIntent.Effects[EffectIndex].HandAffliction.TargetCardCount;
						}
					}
				}
			}
		}
	}

	void NormalizeEventTunables(
		UWacomRunEventDefinition& Actual,
		const UWacomRunEventDefinition& Expected)
	{
		Actual.DisplayName = Expected.DisplayName;
		if (Actual.Nodes.Num() != Expected.Nodes.Num())
		{
			return;
		}
		for (int32 NodeIndex = 0; NodeIndex < Actual.Nodes.Num(); ++NodeIndex)
		{
			auto& ActualNode = Actual.Nodes[NodeIndex];
			const auto& ExpectedNode = Expected.Nodes[NodeIndex];
			ActualNode.TitleText = ExpectedNode.TitleText;
			ActualNode.BodyText = ExpectedNode.BodyText;
			if (ActualNode.Choices.Num() != ExpectedNode.Choices.Num())
			{
				continue;
			}
			for (int32 ChoiceIndex = 0; ChoiceIndex < ActualNode.Choices.Num(); ++ChoiceIndex)
			{
				auto& ActualChoice = ActualNode.Choices[ChoiceIndex];
				const auto& ExpectedChoice = ExpectedNode.Choices[ChoiceIndex];
				ActualChoice.LabelText = ExpectedChoice.LabelText;
				if (ActualChoice.Conditions.Num() == ExpectedChoice.Conditions.Num())
				{
					for (int32 ConditionIndex = 0;
						ConditionIndex < ActualChoice.Conditions.Num(); ++ConditionIndex)
					{
						ActualChoice.Conditions[ConditionIndex].Value =
							ExpectedChoice.Conditions[ConditionIndex].Value;
					}
				}
				if (ActualChoice.Effects.Num() == ExpectedChoice.Effects.Num())
				{
					for (int32 EffectIndex = 0;
						EffectIndex < ActualChoice.Effects.Num(); ++EffectIndex)
					{
						ActualChoice.Effects[EffectIndex].Value =
							ExpectedChoice.Effects[EffectIndex].Value;
					}
				}
			}
		}
	}

	void NormalizeTunables(UObject& Actual, const UObject& Expected)
	{
		if (UCardDefinition* ActualCard = Cast<UCardDefinition>(&Actual))
		{
			NormalizeCardTunables(*ActualCard, *CastChecked<UCardDefinition>(&Expected));
		}
		else if (UEnemyBehaviorDefinition* ActualBehavior =
			Cast<UEnemyBehaviorDefinition>(&Actual))
		{
			NormalizeBehaviorTunables(*ActualBehavior,
				*CastChecked<UEnemyBehaviorDefinition>(&Expected));
		}
		else if (UEnemyPartDefinition* ActualPart = Cast<UEnemyPartDefinition>(&Actual))
		{
			const auto* ExpectedPart = CastChecked<UEnemyPartDefinition>(&Expected);
			ActualPart->DisplayName = ExpectedPart->DisplayName;
			ActualPart->MaxHp = ExpectedPart->MaxHp;
			ActualPart->ExperienceReward = ExpectedPart->ExperienceReward;
		}
		else if (UEnemyDefinition* ActualEnemy = Cast<UEnemyDefinition>(&Actual))
		{
			ActualEnemy->DisplayName = CastChecked<UEnemyDefinition>(&Expected)->DisplayName;
		}
		else if (UEncounterDefinition* ActualEncounter = Cast<UEncounterDefinition>(&Actual))
		{
			ActualEncounter->DisplayName =
				CastChecked<UEncounterDefinition>(&Expected)->DisplayName;
		}
		else if (UWacomRunEventDefinition* ActualEvent =
			Cast<UWacomRunEventDefinition>(&Actual))
		{
			NormalizeEventTunables(*ActualEvent,
				*CastChecked<UWacomRunEventDefinition>(&Expected));
		}
		else if (UWacomRunPickupDefinition* ActualPickup =
			Cast<UWacomRunPickupDefinition>(&Actual))
		{
			ActualPickup->GoldAmount =
				CastChecked<UWacomRunPickupDefinition>(&Expected)->GoldAmount;
		}
		else if (UShopDefinition* ActualShop = Cast<UShopDefinition>(&Actual))
		{
			const auto* ExpectedShop = CastChecked<UShopDefinition>(&Expected);
			ActualShop->DisplayName = ExpectedShop->DisplayName;
			if (ActualShop->Offers.Num() == ExpectedShop->Offers.Num())
			{
				for (int32 Index = 0; Index < ActualShop->Offers.Num(); ++Index)
				{
					ActualShop->Offers[Index].Price = ExpectedShop->Offers[Index].Price;
				}
			}
		}
	}

	bool CompareEditableProperties(
		const UObject& Actual,
		const UObject& Expected,
		const bool bStrict,
		TArray<FString>& OutErrors)
	{
		if (Actual.GetClass() != Expected.GetClass())
		{
			OutErrors.Add(TEXT("Actual and expected classes differ"));
			return false;
		}
		TStrongObjectPtr<UObject> Comparable(
			DuplicateObject(&Actual, GetTransientPackage()));
		if (!Comparable.IsValid())
		{
			OutErrors.Add(TEXT("Could not duplicate object for read-only comparison"));
			return false;
		}
		if (!bStrict)
		{
			NormalizeTunables(*Comparable.Get(), Expected);
		}
		for (TFieldIterator<FProperty> It(Actual.GetClass()); It; ++It)
		{
			FProperty* Property = *It;
			if (!Property->HasAnyPropertyFlags(CPF_Edit)
				|| Property->HasAnyPropertyFlags(
					CPF_Transient | CPF_DuplicateTransient | CPF_NonPIEDuplicateTransient))
			{
				continue;
			}
			if (!Property->Identical_InContainer(
				Comparable.Get(), &Expected, PPF_None))
			{
				OutErrors.Add(FString::Printf(TEXT("%s mismatch: %s"),
					bStrict ? TEXT("Seed default") : TEXT("Stable structure"),
					*Property->GetName()));
			}
		}
		return OutErrors.IsEmpty();
	}

	UObject* ResolveObject(
		const FString& PackagePath,
		TMap<FString, UObject*>& ObjectsByPackage)
	{
		if (UObject** Found = ObjectsByPackage.Find(PackagePath))
		{
			return *Found;
		}
		UObject* Loaded = LoadObject<UObject>(nullptr, *ObjectPathForPackage(PackagePath));
		if (Loaded)
		{
			ObjectsByPackage.Add(PackagePath, Loaded);
		}
		return Loaded;
	}

	bool BuildExpectedObject(
		const FFormalFloor1ContentManifestEntry& Entry,
		TMap<FString, UObject*>& ObjectsByPackage,
		TStrongObjectPtr<UObject>& OutExpected,
		TArray<FString>& OutErrors)
	{
		OutExpected = TStrongObjectPtr<UObject>(NewObject<UObject>(
			GetTransientPackage(), Entry.AssetClass, NAME_None, RF_Transient));
		if (!OutExpected.IsValid())
		{
			OutErrors.Add(FString::Printf(TEXT("Could not allocate expected object for %s"),
				*Entry.PackagePath));
			return false;
		}
		const FResolveObject Resolver = [&ObjectsByPackage](const FString& PackagePath)
		{
			return ResolveObject(PackagePath, ObjectsByPackage);
		};
		if (!ConfigureExpected(*OutExpected.Get(), Entry, Resolver, OutErrors))
		{
			if (OutErrors.IsEmpty())
			{
				OutErrors.Add(FString::Printf(TEXT("No seed configuration for %s"),
					*Entry.StableId.ToString()));
			}
			return false;
		}
		return ValidateWithSharedRules(*OutExpected.Get(), OutErrors);
	}

	bool ValidateActualAgainstExpected(
		UObject& Actual,
		const FFormalFloor1ContentManifestEntry& Entry,
		TMap<FString, UObject*>& ObjectsByPackage,
		const bool bStrict,
		TArray<FString>& OutErrors)
	{
		if (!ValidateWithSharedRules(Actual, OutErrors))
		{
			return false;
		}
		TStrongObjectPtr<UObject> Expected;
		if (!BuildExpectedObject(Entry, ObjectsByPackage, Expected, OutErrors))
		{
			return false;
		}
		return CompareEditableProperties(Actual, *Expected.Get(), bStrict, OutErrors);
	}

	bool WriteReportJson(FFormalFloor1ContentBuildReport& Report)
	{
		if (Report.ReportPath.IsEmpty())
		{
			Report.ReportPath = FPaths::ProjectSavedDir()
				/ TEXT("FormalFloor1Content")
				/ FString::Printf(TEXT("%s-report.json"),
					*GroupToString(Report.Options.Group));
		}
		else if (FPaths::IsRelative(Report.ReportPath))
		{
			Report.ReportPath = FPaths::ConvertRelativePathToFull(
				FPaths::ProjectDir(), Report.ReportPath);
		}
		IFileManager::Get().MakeDirectory(
			*FPaths::GetPath(Report.ReportPath), true);

		TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
		Root->SetNumberField(TEXT("schemaVersion"), ReportSchemaVersion);
		Root->SetStringField(TEXT("timestampUtc"), FDateTime::UtcNow().ToIso8601());
		Root->SetStringField(TEXT("projectPath"), FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()));
		Root->SetStringField(TEXT("group"), GroupToString(Report.Options.Group));
		Root->SetBoolField(TEXT("seedMissing"), Report.Options.bSeedMissing);
		Root->SetBoolField(TEXT("compareSeedDefaults"), Report.Options.bCompareSeedDefaults);
		Root->SetNumberField(TEXT("manifestCount"), Report.ManifestCount);
		Root->SetNumberField(TEXT("selectedCount"), Report.SelectedCount);
		Root->SetNumberField(TEXT("createdCount"), Report.CreatedCount);
		Root->SetNumberField(TEXT("existingCount"), Report.ExistingCount);
		Root->SetNumberField(TEXT("missingCount"), Report.MissingCount);
		Root->SetNumberField(TEXT("failedCount"), Report.FailedCount);
		Root->SetNumberField(TEXT("savedCount"), Report.SavedCount);
		Root->SetNumberField(TEXT("exitCode"), Report.ExitCode);
		Root->SetStringField(TEXT("failureCategory"), Report.FailureCategory);
		TArray<TSharedPtr<FJsonValue>> JsonEntries;
		for (const FFormalFloor1ContentEntryReport& Entry : Report.Entries)
		{
			TSharedRef<FJsonObject> JsonEntry = MakeShared<FJsonObject>();
			JsonEntry->SetStringField(TEXT("package"), Entry.PackagePath);
			JsonEntry->SetStringField(TEXT("class"), Entry.ClassName);
			JsonEntry->SetStringField(TEXT("stableId"), Entry.StableId.ToString());
			JsonEntry->SetStringField(TEXT("state"), StateToString(Entry.State));
			JsonEntry->SetBoolField(TEXT("saved"), Entry.bSaved);
			TArray<TSharedPtr<FJsonValue>> Diagnostics;
			for (const FString& Diagnostic : Entry.Diagnostics)
			{
				Diagnostics.Add(MakeShared<FJsonValueString>(Diagnostic));
			}
			JsonEntry->SetArrayField(TEXT("diagnostics"), Diagnostics);
			JsonEntries.Add(MakeShared<FJsonValueObject>(JsonEntry));
		}
		Root->SetArrayField(TEXT("entries"), JsonEntries);
		FString Json;
		const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
		if (!FJsonSerializer::Serialize(Root, Writer))
		{
			return false;
		}
		return FFileHelper::SaveStringToFile(Json, *Report.ReportPath);
	}
}

namespace Wacom::ContentBuilder
{
	const TArray<FFormalFloor1ContentManifestEntry>& GetFormalFloor1ContentManifest()
	{
		static const TArray<FFormalFloor1ContentManifestEntry> Manifest = BuildManifest();
		return Manifest;
	}

	bool ParseFormalFloor1ContentOptions(
		const TArray<FString>& Arguments,
		FFormalFloor1ContentOptions& OutOptions,
		TArray<FString>& OutErrors)
	{
		OutOptions = FFormalFloor1ContentOptions();
		bool bSawGroup = false;
		bool bSawReport = false;
		for (FString Argument : Arguments)
		{
			while (Argument.RemoveFromStart(TEXT("-")))
			{
			}
			if (Argument.Equals(TEXT("SeedMissing"), ESearchCase::IgnoreCase))
			{
				if (OutOptions.bSeedMissing)
				{
					OutErrors.Add(TEXT("SeedMissing was specified more than once"));
				}
				OutOptions.bSeedMissing = true;
				continue;
			}
			if (Argument.Equals(TEXT("CompareSeedDefaults"), ESearchCase::IgnoreCase))
			{
				if (OutOptions.bCompareSeedDefaults)
				{
					OutErrors.Add(TEXT("CompareSeedDefaults was specified more than once"));
				}
				OutOptions.bCompareSeedDefaults = true;
				continue;
			}
			FString Value;
			if (Argument.StartsWith(TEXT("Group="), ESearchCase::IgnoreCase))
			{
				Value = Argument.Mid(FCString::Strlen(TEXT("Group=")));
				if (bSawGroup)
				{
					OutErrors.Add(TEXT("Group was specified more than once"));
				}
				bSawGroup = true;
				if (Value.Equals(TEXT("Cards"), ESearchCase::IgnoreCase))
				{
					OutOptions.Group = EFormalFloor1ContentGroup::Cards;
				}
				else if (Value.Equals(TEXT("EnemyGraph"), ESearchCase::IgnoreCase))
				{
					OutOptions.Group = EFormalFloor1ContentGroup::EnemyGraph;
				}
				else if (Value.Equals(TEXT("NodeDefinitions"), ESearchCase::IgnoreCase))
				{
					OutOptions.Group = EFormalFloor1ContentGroup::NodeDefinitions;
				}
				else if (Value.Equals(TEXT("All"), ESearchCase::IgnoreCase))
				{
					OutOptions.Group = EFormalFloor1ContentGroup::All;
				}
				else
				{
					OutErrors.Add(FString::Printf(TEXT("Invalid Group: %s"), *Value));
				}
				continue;
			}
			if (Argument.StartsWith(TEXT("Report="), ESearchCase::IgnoreCase))
			{
				Value = Argument.Mid(FCString::Strlen(TEXT("Report=")));
				if (bSawReport)
				{
					OutErrors.Add(TEXT("Report was specified more than once"));
				}
				bSawReport = true;
				OutOptions.ReportPath = Value.TrimQuotes();
				if (OutOptions.ReportPath.IsEmpty())
				{
					OutErrors.Add(TEXT("Report path cannot be empty"));
				}
				continue;
			}
			OutErrors.Add(FString::Printf(TEXT("Unknown argument: %s"), *Argument));
		}
		return OutErrors.IsEmpty();
	}

	bool ValidateFormalFloor1ContentManifest(TArray<FString>& OutErrors)
	{
		const TArray<FFormalFloor1ContentManifestEntry>& Manifest =
			GetFormalFloor1ContentManifest();
		if (Manifest.Num() != 46)
		{
			OutErrors.Add(FString::Printf(TEXT("Expected 46 manifest entries, got %d"),
				Manifest.Num()));
		}
		TSet<FString> Packages;
		TSet<FName> StableIds;
		TSet<FString> ObjectPaths;
		int32 CardsGroupCount = 0;
		int32 EnemyGraphGroupCount = 0;
		int32 NodeDefinitionsGroupCount = 0;
		TMap<UClass*, int32> ClassCounts;
		for (const FFormalFloor1ContentManifestEntry& Entry : Manifest)
		{
			if (!FPackageName::IsValidLongPackageName(Entry.PackagePath))
			{
				OutErrors.Add(FString::Printf(TEXT("Invalid package path: %s"),
					*Entry.PackagePath));
			}
			if (!Entry.PackagePath.StartsWith(TEXT("/Game/Wacom/Data/")))
			{
				OutErrors.Add(FString::Printf(TEXT("Package outside formal data root: %s"),
					*Entry.PackagePath));
			}
			if (Packages.Contains(Entry.PackagePath))
			{
				OutErrors.Add(FString::Printf(TEXT("Duplicate package: %s"),
					*Entry.PackagePath));
			}
			Packages.Add(Entry.PackagePath);
			const FString ObjectPath = ObjectPathForPackage(Entry.PackagePath);
			if (ObjectPaths.Contains(ObjectPath))
			{
				OutErrors.Add(FString::Printf(TEXT("Duplicate object path: %s"),
					*ObjectPath));
			}
			ObjectPaths.Add(ObjectPath);
			if (Entry.StableId.IsNone() || StableIds.Contains(Entry.StableId))
			{
				OutErrors.Add(FString::Printf(TEXT("Missing or duplicate stable id: %s"),
					*Entry.StableId.ToString()));
			}
			StableIds.Add(Entry.StableId);
			if (!Entry.AssetClass)
			{
				OutErrors.Add(FString::Printf(TEXT("Missing class: %s"), *Entry.PackagePath));
			}
			else
			{
				ClassCounts.FindOrAdd(Entry.AssetClass)++;
			}
			switch (Entry.Group)
			{
			case EFormalFloor1ContentGroup::Cards: ++CardsGroupCount; break;
			case EFormalFloor1ContentGroup::EnemyGraph: ++EnemyGraphGroupCount; break;
			case EFormalFloor1ContentGroup::NodeDefinitions: ++NodeDefinitionsGroupCount; break;
			case EFormalFloor1ContentGroup::All:
				OutErrors.Add(FString::Printf(TEXT("Manifest entry cannot use All group: %s"),
					*Entry.PackagePath));
				break;
			}
		}
		if (CardsGroupCount != 12 || EnemyGraphGroupCount != 19
			|| NodeDefinitionsGroupCount != 15)
		{
			OutErrors.Add(FString::Printf(TEXT("Group counts are %d/%d/%d, expected 12/19/15"),
				CardsGroupCount, EnemyGraphGroupCount, NodeDefinitionsGroupCount));
		}
		const TArray<TPair<UClass*, int32>> ExpectedClassCounts =
		{
			{UCardDefinition::StaticClass(), 12},
			{UEnemyBehaviorDefinition::StaticClass(), 4},
			{UEnemyPartDefinition::StaticClass(), 11},
			{UEnemyDefinition::StaticClass(), 4},
			{UEncounterDefinition::StaticClass(), 6},
			{UWacomRunEventDefinition::StaticClass(), 4},
			{UWacomRunPickupDefinition::StaticClass(), 4},
			{UShopDefinition::StaticClass(), 1},
		};
		for (const TPair<UClass*, int32>& Expected : ExpectedClassCounts)
		{
			const int32 Actual = ClassCounts.FindRef(Expected.Key);
			if (Actual != Expected.Value)
			{
				OutErrors.Add(FString::Printf(TEXT("Class %s count is %d, expected %d"),
					*GetNameSafe(Expected.Key), Actual, Expected.Value));
			}
		}
		if (CardSeeds().Num() != 12 || PartSeeds().Num() != 11
			|| EnemySeeds().Num() != 4 || IntentSeeds().Num() != 24
			|| EncounterSeeds().Num() != 6)
		{
			OutErrors.Add(TEXT("Seed table counts do not match the frozen content contract"));
		}
		for (const FString& ReadOnlyPackage :
			{ChitinWardPackage, AntennaSearchPackage, MoltCutPackage, PoisonFangPackage})
		{
			if (Packages.Contains(ReadOnlyPackage))
			{
				OutErrors.Add(FString::Printf(TEXT("Read-only dependency is writable: %s"),
					*ReadOnlyPackage));
			}
		}
		return OutErrors.IsEmpty();
	}

	bool ValidateFormalFloor1TransientDefaults(TArray<FString>& OutErrors)
	{
		if (!ValidateFormalFloor1ContentManifest(OutErrors))
		{
			return false;
		}
		TMap<FString, UObject*> ObjectsByPackage;
		TArray<TStrongObjectPtr<UObject>> KeepAlive;
		KeepAlive.Reserve(46);
		for (const FFormalFloor1ContentManifestEntry& Entry :
			GetFormalFloor1ContentManifest())
		{
			TStrongObjectPtr<UObject> Expected(nullptr);
			TArray<FString> EntryErrors;
			if (!BuildExpectedObject(Entry, ObjectsByPackage, Expected, EntryErrors))
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
		if (IntentCount != 24)
		{
			OutErrors.Add(FString::Printf(TEXT("Expected 24 configured Intents, got %d"),
				IntentCount));
		}
		if (ChoiceCount != 13)
		{
			OutErrors.Add(FString::Printf(TEXT("Expected 13 configured Choices, got %d"),
				ChoiceCount));
		}
		if (ExplicitPartRewardCount != 11)
		{
			OutErrors.Add(FString::Printf(
				TEXT("Expected 11 explicit branch-reward Parts, got %d"),
				ExplicitPartRewardCount));
		}
		const FString GuardianPackage = BranchCardPackage(
			TEXT("ShallowGuardian"), TEXT("Destroy"));
		const UCardDefinition* GuardianDestroy =
			Cast<UCardDefinition>(ObjectsByPackage.FindRef(GuardianPackage));
		if (!GuardianDestroy
			|| GuardianDestroy->TargetMode != ECardTargetMode::AllEnemyParts
			|| GuardianDestroy->Effects.Num() != 2
			|| GuardianDestroy->Effects[0].Target != WacomTags::Target_AllEnemyParts
			|| GuardianDestroy->Effects[1].Target != WacomTags::Target_AllEnemyParts)
		{
			OutErrors.Add(TEXT("Guardian Destroy must use AllEnemyParts for mode and both effects"));
		}
		return OutErrors.IsEmpty();
	}

	bool ValidateFormalFloor1ComparatorBoundaries(TArray<FString>& OutErrors)
	{
		const FFormalFloor1ContentManifestEntry* Entry =
			GetFormalFloor1ContentManifest().FindByPredicate([](const auto& Candidate)
			{
				return Candidate.StableId == TEXT("Reward.SerpentWood.HerbalPoultice");
			});
		if (!Entry)
		{
			OutErrors.Add(TEXT("HerbalPoultice manifest entry is missing"));
			return false;
		}
		TMap<FString, UObject*> ObjectsByPackage;
		TStrongObjectPtr<UObject> Expected(nullptr);
		if (!BuildExpectedObject(*Entry, ObjectsByPackage, Expected, OutErrors))
		{
			return false;
		}
		TStrongObjectPtr<UCardDefinition> Tuned(
			DuplicateObject(CastChecked<UCardDefinition>(Expected.Get()),
				GetTransientPackage()));
		Tuned->DisplayName = FText::FromString(TEXT("人工调优名称"));
		Tuned->Description = FText::FromString(TEXT("人工调优描述"));
		Tuned->BaseCost = 2;
		Tuned->Rarity = WacomTags::Card_Rarity_Blue;
		Tuned->Effects[0].Magnitude = 6;
		TArray<FString> StructuralErrors;
		if (!CompareEditableProperties(
			*Tuned.Get(), *Expected.Get(), false, StructuralErrors))
		{
			OutErrors.Add(TEXT("Structural comparison rejected approved tunable drift"));
			OutErrors.Append(StructuralErrors);
		}
		TArray<FString> StrictErrors;
		if (CompareEditableProperties(*Tuned.Get(), *Expected.Get(), true, StrictErrors))
		{
			OutErrors.Add(TEXT("Strict comparison accepted seed-default drift"));
		}
		Tuned->CardId = TEXT("Reward.SerpentWood.InvalidIdentity");
		StructuralErrors.Reset();
		if (CompareEditableProperties(*Tuned.Get(), *Expected.Get(), false, StructuralErrors))
		{
			OutErrors.Add(TEXT("Structural comparison accepted stable identity drift"));
		}
		return OutErrors.IsEmpty();
	}

	bool ValidateFormalFloor1LoadedAssets(
		const bool bCompareSeedDefaults,
		TArray<FString>& OutErrors)
	{
		if (!ValidateFormalFloor1ContentManifest(OutErrors))
		{
			return false;
		}

		TMap<FString, UObject*> ObjectsByPackage;
		for (const FFormalFloor1ContentManifestEntry& Entry :
			GetFormalFloor1ContentManifest())
		{
			UObject* Asset = LoadObject<UObject>(
				nullptr, *ObjectPathForPackage(Entry.PackagePath));
			if (!Asset)
			{
				OutErrors.Add(FString::Printf(TEXT("%s: asset failed to load"),
					*Entry.PackagePath));
				continue;
			}
			if (Asset->GetClass() != Entry.AssetClass)
			{
				OutErrors.Add(FString::Printf(
					TEXT("%s: class is %s, expected %s"),
					*Entry.PackagePath, *GetNameSafe(Asset->GetClass()),
					*GetNameSafe(Entry.AssetClass)));
				continue;
			}
			ObjectsByPackage.Add(Entry.PackagePath, Asset);
		}
		if (ObjectsByPackage.Num() != GetFormalFloor1ContentManifest().Num())
		{
			return false;
		}

		for (const FFormalFloor1ContentManifestEntry& Entry :
			GetFormalFloor1ContentManifest())
		{
			TArray<FString> EntryErrors;
			UObject* Asset = ObjectsByPackage.FindRef(Entry.PackagePath);
			if (!Asset || !ValidateActualAgainstExpected(
				*Asset, Entry, ObjectsByPackage,
				bCompareSeedDefaults, EntryErrors))
			{
				for (const FString& Error : EntryErrors)
				{
					OutErrors.Add(Entry.StableId.ToString() + TEXT(": ") + Error);
				}
			}
		}
		return OutErrors.IsEmpty();
	}

	int32 RunFormalFloor1ContentBuilder(
		const TArray<FString>& Arguments,
		FFormalFloor1ContentBuildReport* OutReport)
	{
		FFormalFloor1ContentBuildReport Report;
		TArray<FString> ParseErrors;
		if (!ParseFormalFloor1ContentOptions(Arguments, Report.Options, ParseErrors))
		{
			Report.ExitCode = 2;
			Report.FailureCategory = TEXT("Arguments");
			for (const FString& Error : ParseErrors)
			{
				UE_LOG(LogTemp, Error, TEXT("[FormalFloor1Content] %s"), *Error);
			}
			if (OutReport) { *OutReport = Report; }
			return Report.ExitCode;
		}
		Report.ReportPath = Report.Options.ReportPath;
		const TArray<FFormalFloor1ContentManifestEntry>& Manifest =
			GetFormalFloor1ContentManifest();
		Report.ManifestCount = Manifest.Num();
		TArray<FString> ManifestErrors;
		if (!ValidateFormalFloor1ContentManifest(ManifestErrors))
		{
			Report.ExitCode = 1;
			Report.FailureCategory = TEXT("Manifest");
			for (const FString& Error : ManifestErrors)
			{
				UE_LOG(LogTemp, Error, TEXT("[FormalFloor1Content] %s"), *Error);
			}
			if (!WriteReportJson(Report))
			{
				Report.ExitCode = 3;
				Report.FailureCategory = TEXT("ReportWrite");
			}
			if (OutReport) { *OutReport = Report; }
			return Report.ExitCode;
		}

		TArray<int32> SelectedManifestIndices;
		TMap<FString, UObject*> LoadedObjects;
		for (int32 ManifestIndex = 0; ManifestIndex < Manifest.Num(); ++ManifestIndex)
		{
			const FFormalFloor1ContentManifestEntry& Entry = Manifest[ManifestIndex];
			if (FPackageName::DoesPackageExist(Entry.PackagePath))
			{
				if (UObject* Existing = LoadObject<UObject>(
					nullptr, *ObjectPathForPackage(Entry.PackagePath)))
				{
					LoadedObjects.Add(Entry.PackagePath, Existing);
				}
			}
			if (!IsSelected(Entry, Report.Options.Group))
			{
				continue;
			}
			SelectedManifestIndices.Add(ManifestIndex);
			FFormalFloor1ContentEntryReport& EntryReport = Report.Entries.AddDefaulted_GetRef();
			EntryReport.PackagePath = Entry.PackagePath;
			EntryReport.ClassName = GetNameSafe(Entry.AssetClass);
			EntryReport.StableId = Entry.StableId;
		}
		Report.SelectedCount = SelectedManifestIndices.Num();

		bool bPreflightFailed = false;
		for (int32 SelectedIndex = 0; SelectedIndex < SelectedManifestIndices.Num(); ++SelectedIndex)
		{
			const auto& Entry = Manifest[SelectedManifestIndices[SelectedIndex]];
			auto& EntryReport = Report.Entries[SelectedIndex];
			if (!FPackageName::DoesPackageExist(Entry.PackagePath))
			{
				EntryReport.State = EFormalFloor1ContentEntryState::Missing;
				++Report.MissingCount;
				continue;
			}
			UObject* Existing = LoadedObjects.FindRef(Entry.PackagePath);
			if (!Existing)
			{
				EntryReport.State = EFormalFloor1ContentEntryState::Failed;
				EntryReport.Diagnostics.Add(TEXT("Package exists but the expected object failed to load"));
				++Report.FailedCount;
				bPreflightFailed = true;
				continue;
			}
			if (Existing->GetClass() != Entry.AssetClass)
			{
				EntryReport.State = EFormalFloor1ContentEntryState::Failed;
				EntryReport.Diagnostics.Add(FString::Printf(
					TEXT("Wrong class %s; expected %s"),
					*GetNameSafe(Existing->GetClass()), *GetNameSafe(Entry.AssetClass)));
				++Report.FailedCount;
				bPreflightFailed = true;
			}
			else
			{
				EntryReport.State = EFormalFloor1ContentEntryState::Existing;
			}
		}

		if (Report.Options.bSeedMissing && !bPreflightFailed)
		{
			TMap<FString, UObject*> PreflightObjects = LoadedObjects;
			TArray<TStrongObjectPtr<UObject>> PreflightKeepAlive;
			for (int32 SelectedIndex = 0; SelectedIndex < SelectedManifestIndices.Num(); ++SelectedIndex)
			{
				const auto& Entry = Manifest[SelectedManifestIndices[SelectedIndex]];
				TStrongObjectPtr<UObject> Expected(nullptr);
				TArray<FString> Errors;
				if (!BuildExpectedObject(Entry, PreflightObjects, Expected, Errors))
				{
					auto& EntryReport = Report.Entries[SelectedIndex];
					EntryReport.State = EFormalFloor1ContentEntryState::Failed;
					EntryReport.Diagnostics.Append(Errors);
					++Report.FailedCount;
					bPreflightFailed = true;
					break;
				}
				if (!PreflightObjects.Contains(Entry.PackagePath))
				{
					PreflightObjects.Add(Entry.PackagePath, Expected.Get());
					PreflightKeepAlive.Add(MoveTemp(Expected));
				}
			}
		}

		if (!Report.Options.bSeedMissing)
		{
			for (int32 SelectedIndex = 0; SelectedIndex < SelectedManifestIndices.Num(); ++SelectedIndex)
			{
				const auto& Entry = Manifest[SelectedManifestIndices[SelectedIndex]];
				auto& EntryReport = Report.Entries[SelectedIndex];
				if (EntryReport.State == EFormalFloor1ContentEntryState::Missing)
				{
					EntryReport.Diagnostics.Add(TEXT("Package is missing in inspect-only mode"));
					continue;
				}
				if (EntryReport.State == EFormalFloor1ContentEntryState::Failed)
				{
					continue;
				}
				TArray<FString> Errors;
				UObject* Existing = LoadedObjects.FindRef(Entry.PackagePath);
				if (!Existing || !ValidateActualAgainstExpected(
					*Existing, Entry, LoadedObjects,
					Report.Options.bCompareSeedDefaults, Errors))
				{
					EntryReport.State = EFormalFloor1ContentEntryState::Failed;
					EntryReport.Diagnostics.Append(Errors);
					++Report.FailedCount;
				}
				else
				{
					++Report.ExistingCount;
				}
			}
			Report.ExitCode = (Report.FailedCount == 0 && Report.MissingCount == 0) ? 0 : 1;
			Report.FailureCategory = Report.ExitCode == 0 ? TEXT("") : TEXT("Validation");
		}
		else if (bPreflightFailed)
		{
			Report.ExitCode = 1;
			Report.FailureCategory = TEXT("Preflight");
		}
		else
		{
			Report.MissingCount = 0;
			for (int32 SelectedIndex = 0; SelectedIndex < SelectedManifestIndices.Num(); ++SelectedIndex)
			{
				const auto& Entry = Manifest[SelectedManifestIndices[SelectedIndex]];
				auto& EntryReport = Report.Entries[SelectedIndex];
				if (EntryReport.State == EFormalFloor1ContentEntryState::Existing)
				{
					TArray<FString> Errors;
					UObject* Existing = LoadedObjects.FindRef(Entry.PackagePath);
					if (!Existing || !ValidateActualAgainstExpected(
						*Existing, Entry, LoadedObjects,
						Report.Options.bCompareSeedDefaults, Errors))
					{
						EntryReport.State = EFormalFloor1ContentEntryState::Failed;
						EntryReport.Diagnostics.Append(Errors);
						++Report.FailedCount;
						Report.ExitCode = 1;
						Report.FailureCategory = TEXT("Validation");
						break;
					}
					++Report.ExistingCount;
					continue;
				}

				UPackage* Package = CreatePackage(*Entry.PackagePath);
				const FName AssetName(*FPackageName::GetLongPackageAssetName(Entry.PackagePath));
				UObject* Asset = Package
					? NewObject<UObject>(Package, Entry.AssetClass, AssetName,
						RF_Public | RF_Standalone | RF_Transactional)
					: nullptr;
				TArray<FString> Errors;
				const FResolveObject Resolver = [&LoadedObjects](const FString& PackagePath)
				{
					return ResolveObject(PackagePath, LoadedObjects);
				};
				if (!Package || !Asset
					|| !ConfigureExpected(*Asset, Entry, Resolver, Errors)
					|| !ValidateWithSharedRules(*Asset, Errors))
				{
					EntryReport.State = EFormalFloor1ContentEntryState::Failed;
					EntryReport.Diagnostics.Append(Errors);
					if (EntryReport.Diagnostics.IsEmpty())
					{
						EntryReport.Diagnostics.Add(TEXT("Could not create/configure asset"));
					}
					++Report.FailedCount;
					Report.ExitCode = 3;
					Report.FailureCategory = TEXT("Create");
					break;
				}
				LoadedObjects.Add(Entry.PackagePath, Asset);
				TArray<FString> ComparisonErrors;
				if (!ValidateActualAgainstExpected(
					*Asset, Entry, LoadedObjects, true, ComparisonErrors))
				{
					EntryReport.State = EFormalFloor1ContentEntryState::Failed;
					EntryReport.Diagnostics.Append(ComparisonErrors);
					++Report.FailedCount;
					Report.ExitCode = 3;
					Report.FailureCategory = TEXT("CreateValidation");
					break;
				}
				if (!SaveAssetPackage(Package, Asset, Entry.PackagePath))
				{
					EntryReport.State = EFormalFloor1ContentEntryState::Failed;
					EntryReport.Diagnostics.Add(TEXT("SavePackage failed"));
					++Report.FailedCount;
					Report.ExitCode = 3;
					Report.FailureCategory = TEXT("Save");
					break;
				}
				UPackage::WaitForAsyncFileWrites();
				UObject* Reloaded = LoadObject<UObject>(
					nullptr, *ObjectPathForPackage(Entry.PackagePath));
				if (!Reloaded || Reloaded->GetClass() != Entry.AssetClass)
				{
					EntryReport.State = EFormalFloor1ContentEntryState::Failed;
					EntryReport.Diagnostics.Add(TEXT("Post-save load/class check failed"));
					++Report.FailedCount;
					Report.ExitCode = 3;
					Report.FailureCategory = TEXT("Reload");
					break;
				}
				EntryReport.State = EFormalFloor1ContentEntryState::Created;
				EntryReport.bSaved = true;
				++Report.CreatedCount;
				++Report.SavedCount;
			}
			if (Report.ExitCode == 0 && Report.FailedCount == 0)
			{
				Report.FailureCategory.Reset();
			}
		}

		if (!WriteReportJson(Report))
		{
			UE_LOG(LogTemp, Error, TEXT("[FormalFloor1Content] Failed to write report: %s"),
				*Report.ReportPath);
			Report.ExitCode = 3;
			Report.FailureCategory = TEXT("ReportWrite");
		}
		UE_LOG(LogTemp, Display,
			TEXT("[FormalFloor1Content] Group=%s Created=%d Existing=%d Missing=%d Failed=%d Saved=%d Report=%s Exit=%d"),
			*GroupToString(Report.Options.Group), Report.CreatedCount,
			Report.ExistingCount, Report.MissingCount, Report.FailedCount,
			Report.SavedCount, *Report.ReportPath, Report.ExitCode);
		if (OutReport)
		{
			*OutReport = Report;
		}
		return Report.ExitCode;
	}
}
