// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/SnakeBuilder.h"
#include "ContentBuilders/ContentBuilderHelpers.h"

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

		SaveAssetPackage(Pkg, Part, PackagePath);
		return Part;
	}
}

namespace Wacom::ContentBuilder
{
	UEnemyDefinition* BuildSnakeContent()
	{
		// ---- 头 ----
		// 经验奖励：Head=3 / Body=2 / Tail=2（GDD §3.3，头是核心，多给 1 点）
		UEnemyPartDefinition* Head = BuildPart(
			TEXT("/Game/Wacom/Enemies/Snake/DA_Part_Snake_Head"),
			TEXT("DA_Part_Snake_Head"),
			TEXT("Snake.Head"),
			TEXT("Snake Head"),
			/*MaxHp*/ 16,
			/*Exp*/ 3,
			{
				MakeIntent(TEXT("Snake.Head.Bite"),   TEXT("Bite"),   /*Initiative*/ 3, /*Resist*/ 6,
				           { MakeDamage(6) }),
				MakeIntent(TEXT("Snake.Head.Venom"),  TEXT("Venom"),  5, 0,
				           { MakePoisonOnPlayer(2) }),
				MakeIntent(TEXT("Snake.Head.Strike"), TEXT("Strike"), 4, 8,
				           { MakeDamage(8) }),
			});
		if (!Head) { return nullptr; }

		// ---- 身体 ----
		UEnemyPartDefinition* Body = BuildPart(
			TEXT("/Game/Wacom/Enemies/Snake/DA_Part_Snake_Body"),
			TEXT("DA_Part_Snake_Body"),
			TEXT("Snake.Body"),
			TEXT("Snake Body"),
			/*MaxHp*/ 22,
			/*Exp*/ 2,
			{
				MakeIntent(TEXT("Snake.Body.Constrict"), TEXT("Constrict"), 4, 0,
				           { MakeSlowOnPlayer(1) }),
				MakeIntent(TEXT("Snake.Body.Harden"),    TEXT("Harden"),    2, 0,
				           { MakeShieldSelf(5) }),
				MakeIntent(TEXT("Snake.Body.Slam"),      TEXT("Slam"),      3, 5,
				           { MakeDamage(5) }),
			});
		if (!Body) { return nullptr; }

		// ---- 尾巴 ----
		UEnemyPartDefinition* Tail = BuildPart(
			TEXT("/Game/Wacom/Enemies/Snake/DA_Part_Snake_Tail"),
			TEXT("DA_Part_Snake_Tail"),
			TEXT("Snake.Tail"),
			TEXT("Snake Tail"),
			/*MaxHp*/ 10,
			/*Exp*/ 2,
			{
				MakeIntent(TEXT("Snake.Tail.Sweep"), TEXT("Sweep"), 1, 3,
				           { MakeDamage(3) }),
				MakeIntent(TEXT("Snake.Tail.Lash"),  TEXT("Lash"),  2, 5,
				           { MakeDamage(5) }),
				MakeIntent(TEXT("Snake.Tail.Whip"),  TEXT("Whip"),  3, 4,
				           { MakeDamage(4) }),
			});
		if (!Tail) { return nullptr; }

		// ---- 敌人 ----
		const FString EnemyPkgPath = TEXT("/Game/Wacom/Enemies/Snake/DA_Enemy_Snake");
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
