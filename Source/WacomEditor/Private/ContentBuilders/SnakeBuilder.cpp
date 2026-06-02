// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/SnakeBuilder.h"
#include "ContentBuilders/ContentBuilderHelpers.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
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
		Card->Description = FText::FromString(TEXT("对一个敌方部位施加 1 中毒。"));
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
		UCardDefinition* KnockdownRewardCard,
		TArray<FIntentDefinition> IntentSequence)
	{
		UPackage* Pkg = FindOrCreatePackage(PackagePath);
		if (!Pkg) { return nullptr; }

		UEnemyPartDefinition* Part = CreateOrReplaceAsset<UEnemyPartDefinition>(Pkg, AssetName);
		if (!Part) { return nullptr; }

		Part->PartId              = PartId;
		Part->DisplayName         = FText::FromString(DisplayName);
		Part->MaxHp               = MaxHp;
		Part->InitialIntentIndex  = 0;
		Part->IntentSequence      = MoveTemp(IntentSequence);
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

		// ---- 头 ----
		// 经验奖励：Head=3 / Body=2 / Tail=2，头是核心，多给 1 点。
		UEnemyPartDefinition* Head = BuildPart(
			MakePackagePath(SnakeEnemiesRoot(), TEXT("DA_Part_Snake_Head")),
			TEXT("DA_Part_Snake_Head"),
			TEXT("Snake.Head"),
			TEXT("Snake Head"),
			/*MaxHp*/ 16,
			/*Exp*/ 3,
			PoisonFang,
			{
				MakeIntent(TEXT("Snake.Head.Bite"),   TEXT("Bite"),   /*Initiative*/ 3, /*Resist*/ 6,
				           { MakeDamage(6) }),
				MakeIntent(TEXT("Snake.Head.Venom"),  TEXT("Venom"),  5, 0,
				           { MakePoisonOnPlayer(2) }),
				MakeIntent(TEXT("Snake.Head.Strike"), TEXT("Strike"), 4, 8,
				           { MakeDamage(8) }),
				MakeIntent(TEXT("Snake.Head.CoiledGuard"), TEXT("Coiled Guard"), 2, 0,
				           { MakeShieldSelf(4) }),
			});
		if (!Head) { return nullptr; }

		// ---- 身体 ----
		UEnemyPartDefinition* Body = BuildPart(
			MakePackagePath(SnakeEnemiesRoot(), TEXT("DA_Part_Snake_Body")),
			TEXT("DA_Part_Snake_Body"),
			TEXT("Snake.Body"),
			TEXT("Snake Body"),
			/*MaxHp*/ 22,
			/*Exp*/ 2,
			PoisonFang,
			{
				MakeIntent(TEXT("Snake.Body.Constrict"), TEXT("Constrict"), 4, 0,
				           { MakeSlowOnPlayer(1) }),
				MakeIntent(TEXT("Snake.Body.Harden"),    TEXT("Harden"),    2, 0,
				           { MakeShieldSelf(5) }),
				MakeIntent(TEXT("Snake.Body.Slam"),      TEXT("Slam"),      3, 5,
				           { MakeDamage(5) }),
				MakeIntent(TEXT("Snake.Body.VenomMist"), TEXT("Venom Mist"), 5, 0,
				           { MakePoisonOnPlayer(1) }),
			});
		if (!Body) { return nullptr; }

		// ---- 尾巴 ----
		UEnemyPartDefinition* Tail = BuildPart(
			MakePackagePath(SnakeEnemiesRoot(), TEXT("DA_Part_Snake_Tail")),
			TEXT("DA_Part_Snake_Tail"),
			TEXT("Snake.Tail"),
			TEXT("Snake Tail"),
			/*MaxHp*/ 10,
			/*Exp*/ 2,
			PoisonFang,
			{
				MakeIntent(TEXT("Snake.Tail.Sweep"), TEXT("Sweep"), 1, 3,
				           { MakeDamage(3) }),
				MakeIntent(TEXT("Snake.Tail.Lash"),  TEXT("Lash"),  2, 5,
				           { MakeDamage(5) }),
				MakeIntent(TEXT("Snake.Tail.Whip"),  TEXT("Whip"),  3, 4,
				           { MakeDamage(4) }),
				MakeIntent(TEXT("Snake.Tail.Brace"), TEXT("Brace"), 2, 0,
				           { MakeShieldSelf(3) }),
				MakeIntent(TEXT("Snake.Tail.Tangle"), TEXT("Tangle"), 4, 0,
				           { MakeSlowOnPlayer(1) }),
			});
		if (!Tail) { return nullptr; }

		// ---- 敌人 ----
		const FString EnemyPkgPath = MakePackagePath(SnakeEnemiesRoot(), TEXT("DA_Enemy_Snake"));
		UPackage* EnemyPkg = FindOrCreatePackage(EnemyPkgPath);
		if (!EnemyPkg) { return nullptr; }

		UEnemyDefinition* Enemy = CreateOrReplaceAsset<UEnemyDefinition>(EnemyPkg, TEXT("DA_Enemy_Snake"));
		if (!Enemy) { return nullptr; }

		Enemy->EnemyId      = TEXT("Snake");
		Enemy->DisplayName  = FText::FromString(TEXT("Snake"));

		FEnemyPartSlot SlotHead; SlotHead.PartDef = Head;
		FEnemyPartSlot SlotBody; SlotBody.PartDef = Body;
		FEnemyPartSlot SlotTail; SlotTail.PartDef = Tail;
		Enemy->Parts = { SlotHead, SlotBody, SlotTail };

		SaveAssetPackage(EnemyPkg, Enemy, EnemyPkgPath);
		return Enemy;
	}
}
