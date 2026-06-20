// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/SnakeBuilder.h"
#include "ContentBuilders/ContentBuilderHelpers.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Enemies/EnemyBehaviorDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Enemies/IntentDefinition.h"
#include "Enemies/IntentEffect.h"
#include "Tags/WacomGameplayTags.h"

namespace
{
	using namespace Wacom::ContentBuilder;

	FIntentEffect MakeDamage(int32 Amount)
	{
		FIntentEffect Eff;
		Eff.EffectType = WacomTags::Effect_Damage;
		Eff.Magnitude  = Amount;
		Eff.Target     = WacomTags::Target_Player;
		return Eff;
	}

	FIntentEffect MakePoisonOnPlayer(int32 Stacks)
	{
		FIntentEffect Eff;
		Eff.EffectType = WacomTags::Effect_ApplyStatus_Poison;
		Eff.Magnitude  = Stacks;
		Eff.Target     = WacomTags::Target_Player;
		return Eff;
	}

	FIntentEffect MakeSlowOnPlayer(int32 Stacks)
	{
		FIntentEffect Eff;
		Eff.EffectType = WacomTags::Effect_ApplyStatus_Slow;
		Eff.Magnitude  = Stacks;
		Eff.Target     = WacomTags::Target_Player;
		return Eff;
	}

	FIntentEffect MakeShieldSelf(int32 Amount)
	{
		FIntentEffect Eff;
		Eff.EffectType = WacomTags::Status_Shield;
		Eff.Magnitude  = Amount;
		Eff.Target     = WacomTags::Target_Self;
		return Eff;
	}

	FIntentDefinition MakeIntent(FName Id, const FString& DisplayName,
	                              int32 Initiative, int32 ResistanceValue,
	                              TArray<FIntentEffect> Effects)
	{
		FIntentDefinition Intent;
		Intent.IntentId        = Id;
		Intent.DisplayName     = FText::FromString(DisplayName);
		Intent.Initiative      = Initiative;
		Intent.ResistanceValue = ResistanceValue;
		Intent.Effects         = MoveTemp(Effects);
		return Intent;
	}

	FWacomEnemyBehaviorIntent MakeBehaviorIntent(
		FName Id,
		const FString& DisplayName,
		int32 Initiative,
		int32 ResistanceValue,
		TArray<FIntentEffect> Effects)
	{
		FWacomEnemyBehaviorIntent IntentEntry;
		IntentEntry.Intent = MakeIntent(Id, DisplayName, Initiative, ResistanceValue, MoveTemp(Effects));
		return IntentEntry;
	}

	FWacomEnemyIntentSetDefinition MakeSequenceIntentSet(
		FName IntentSetId,
		FName PartSlotId,
		TArray<FWacomEnemyBehaviorIntent> Intents)
	{
		FWacomEnemyIntentSetDefinition IntentSet;
		IntentSet.IntentSetId = IntentSetId;
		IntentSet.AppliesToPartSlotId = PartSlotId;
		IntentSet.SelectorMode = EWacomEnemyIntentSelectorMode::Sequence;
		IntentSet.Intents = MoveTemp(Intents);
		return IntentSet;
	}

	UEnemyBehaviorDefinition* BuildSnakeBehavior()
	{
		const FString BehaviorPkgPath = MakePackagePath(SnakeEnemiesRoot(), TEXT("DA_Behavior_Snake"));
		UPackage* BehaviorPkg = FindOrCreatePackage(BehaviorPkgPath);
		if (!BehaviorPkg) { return nullptr; }

		UEnemyBehaviorDefinition* Behavior =
			CreateOrReplaceAsset<UEnemyBehaviorDefinition>(BehaviorPkg, TEXT("DA_Behavior_Snake"));
		if (!Behavior) { return nullptr; }

		Behavior->BehaviorId = TEXT("Snake.Behavior");
		Behavior->InitialPhaseId = TEXT("Default");

		FWacomEnemyPhaseDefinition DefaultPhase;
		DefaultPhase.PhaseId = TEXT("Default");
		DefaultPhase.IntentSets = {
			MakeSequenceIntentSet(
				TEXT("Snake.Head.Sequence"),
				TEXT("Head"),
				{
					MakeBehaviorIntent(TEXT("Snake.Head.Bite"), TEXT("Bite"), /*Initiative*/ 3, /*Resist*/ 6,
						{ MakeDamage(6) }),
					MakeBehaviorIntent(TEXT("Snake.Head.Venom"), TEXT("Venom"), 5, 0,
						{ MakePoisonOnPlayer(2) }),
					MakeBehaviorIntent(TEXT("Snake.Head.Strike"), TEXT("Strike"), 4, 8,
						{ MakeDamage(8) }),
					MakeBehaviorIntent(TEXT("Snake.Head.CoiledGuard"), TEXT("Coiled Guard"), 2, 0,
						{ MakeShieldSelf(4) }),
				}),
			MakeSequenceIntentSet(
				TEXT("Snake.Body.Sequence"),
				TEXT("Body"),
				{
					MakeBehaviorIntent(TEXT("Snake.Body.Constrict"), TEXT("Constrict"), 4, 0,
						{ MakeSlowOnPlayer(1) }),
					MakeBehaviorIntent(TEXT("Snake.Body.Harden"), TEXT("Harden"), 2, 0,
						{ MakeShieldSelf(5) }),
					MakeBehaviorIntent(TEXT("Snake.Body.Slam"), TEXT("Slam"), 3, 5,
						{ MakeDamage(5) }),
					MakeBehaviorIntent(TEXT("Snake.Body.VenomMist"), TEXT("Venom Mist"), 5, 0,
						{ MakePoisonOnPlayer(1) }),
				}),
			MakeSequenceIntentSet(
				TEXT("Snake.Tail.Sequence"),
				TEXT("Tail"),
				{
					MakeBehaviorIntent(TEXT("Snake.Tail.Sweep"), TEXT("Sweep"), 1, 3,
						{ MakeDamage(3) }),
					MakeBehaviorIntent(TEXT("Snake.Tail.Lash"), TEXT("Lash"), 2, 5,
						{ MakeDamage(5) }),
					MakeBehaviorIntent(TEXT("Snake.Tail.Whip"), TEXT("Whip"), 3, 4,
						{ MakeDamage(4) }),
					MakeBehaviorIntent(TEXT("Snake.Tail.Brace"), TEXT("Brace"), 2, 0,
						{ MakeShieldSelf(3) }),
					MakeBehaviorIntent(TEXT("Snake.Tail.Tangle"), TEXT("Tangle"), 4, 0,
						{ MakeSlowOnPlayer(1) }),
				}),
		};
		Behavior->Phases = { DefaultPhase };

		SaveAssetPackage(BehaviorPkg, Behavior, BehaviorPkgPath);
		return Behavior;
	}

	FCardEffect MakePoisonOnEnemyPart(int32 Stacks)
	{
		FCardEffect Eff;
		Eff.EffectType = WacomTags::Effect_ApplyStatus_Poison;
		Eff.Magnitude  = Stacks;
		Eff.Target     = WacomTags::Target_SingleEnemyPart;
		return Eff;
	}

	UCardDefinition* BuildPoisonFangCard()
	{
		const FString PackagePath = MakePackagePath(RewardCardsRoot(), TEXT("DA_Card_PoisonFang"));
		UPackage* Pkg = FindOrCreatePackage(PackagePath);
		if (!Pkg) { return nullptr; }

		UCardDefinition* Card = CreateOrReplaceAsset<UCardDefinition>(Pkg, TEXT("DA_Card_PoisonFang"));
		if (!Card) { return nullptr; }

		Card->CardId      = TEXT("PoisonFang");
		Card->DisplayName = FText::FromString(TEXT("毒牙"));
		Card->Description = FText::FromString(TEXT("对一个敌方部位施加 {Effect.0} 中毒。"));
		Card->BaseCost    = 0;
		Card->Rarity      = WacomTags::Card_Rarity_White;
		Card->Keywords.Reset();
		Card->TargetMode = ECardTargetMode::SingleEnemyPart;
		Card->Physique = FCardPhysique{};
		Card->Effects = { MakePoisonOnEnemyPart(1) };
		Card->PerfectReleaseEffects.Reset();
		Card->ZoneHooks.Reset();
		Card->Passives.Reset();

		SaveAssetPackage(Pkg, Card, PackagePath);
		return Card;
	}

	/**
	 * 通用：在给定 package 里建一个 UEnemyPartDefinition。
	 */
	UEnemyPartDefinition* BuildPart(
		const FString& PackagePath,
		FName AssetName,
		FName PartId,
		const FString& DisplayName,
		int32 MaxHp,
		int32 ExperienceReward,
		UCardDefinition* KnockdownRewardCard)
	{
		UPackage* Pkg = FindOrCreatePackage(PackagePath);
		if (!Pkg) { return nullptr; }

		UEnemyPartDefinition* Part = CreateOrReplaceAsset<UEnemyPartDefinition>(Pkg, AssetName);
		if (!Part) { return nullptr; }

		Part->PartId              = PartId;
		Part->DisplayName         = FText::FromString(DisplayName);
		Part->MaxHp               = MaxHp;
		Part->ExperienceReward    = ExperienceReward;
		Part->KnockdownRewardCard = KnockdownRewardCard;

		SaveAssetPackage(Pkg, Part, PackagePath);
		return Part;
	}
}

namespace Wacom::ContentBuilder
{
	UEnemyDefinition* BuildSnakeContent()
	{
		UCardDefinition* PoisonFang = BuildPoisonFangCard();
		if (!PoisonFang) { return nullptr; }
		UEnemyBehaviorDefinition* SnakeBehavior = BuildSnakeBehavior();
		if (!SnakeBehavior) { return nullptr; }

		// ---- 头 ----
		// 经验奖励：Head=3 / Body=2 / Tail=2，头是核心，多给 1 点。
		UEnemyPartDefinition* Head = BuildPart(
			MakePackagePath(SnakeEnemiesRoot(), TEXT("DA_Part_Snake_Head")),
			TEXT("DA_Part_Snake_Head"),
			TEXT("Snake.Head"),
			TEXT("Snake Head"),
			/*MaxHp*/ 16,
			/*Exp*/ 3,
			PoisonFang);
		if (!Head) { return nullptr; }

		// ---- 身体 ----
		UEnemyPartDefinition* Body = BuildPart(
			MakePackagePath(SnakeEnemiesRoot(), TEXT("DA_Part_Snake_Body")),
			TEXT("DA_Part_Snake_Body"),
			TEXT("Snake.Body"),
			TEXT("Snake Body"),
			/*MaxHp*/ 22,
			/*Exp*/ 2,
			PoisonFang);
		if (!Body) { return nullptr; }

		// ---- 尾巴 ----
		UEnemyPartDefinition* Tail = BuildPart(
			MakePackagePath(SnakeEnemiesRoot(), TEXT("DA_Part_Snake_Tail")),
			TEXT("DA_Part_Snake_Tail"),
			TEXT("Snake.Tail"),
			TEXT("Snake Tail"),
			/*MaxHp*/ 10,
			/*Exp*/ 2,
			PoisonFang);
		if (!Tail) { return nullptr; }

		// ---- 敌人 ----
		const FString EnemyPkgPath = MakePackagePath(SnakeEnemiesRoot(), TEXT("DA_Enemy_Snake"));
		UPackage* EnemyPkg = FindOrCreatePackage(EnemyPkgPath);
		if (!EnemyPkg) { return nullptr; }

		UEnemyDefinition* Enemy = CreateOrReplaceAsset<UEnemyDefinition>(EnemyPkg, TEXT("DA_Enemy_Snake"));
		if (!Enemy) { return nullptr; }

		Enemy->EnemyId      = TEXT("Snake");
		Enemy->DisplayName  = FText::FromString(TEXT("Snake"));
		Enemy->DefaultBehavior = SnakeBehavior;
		Enemy->DefaultPhaseId = TEXT("Default");

		FEnemyPartSlot SlotHead;
		SlotHead.PartSlotId = TEXT("Head");
		SlotHead.PartDef = Head;
		SlotHead.InitialIntentSetId = TEXT("Snake.Head.Sequence");
		FEnemyPartSlot SlotBody;
		SlotBody.PartSlotId = TEXT("Body");
		SlotBody.PartDef = Body;
		SlotBody.InitialIntentSetId = TEXT("Snake.Body.Sequence");
		FEnemyPartSlot SlotTail;
		SlotTail.PartSlotId = TEXT("Tail");
		SlotTail.PartDef = Tail;
		SlotTail.InitialIntentSetId = TEXT("Snake.Tail.Sequence");
		Enemy->Parts = { SlotHead, SlotBody, SlotTail };

		SaveAssetPackage(EnemyPkg, Enemy, EnemyPkgPath);
		return Enemy;
	}
}
