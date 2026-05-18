// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Characters/CharacterDefinition.h"
#include "Commands/BattleCommand.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Enemies/IntentDefinition.h"
#include "Enemies/IntentEffect.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomEnums.h"

namespace
{
	template <typename T>
	T* NewTransient()
	{
		return NewObject<T>(GetTransientPackage(), NAME_None, RF_Transient);
	}
}

FWacomBattleFixture::FWacomBattleFixture() = default;
FWacomBattleFixture::~FWacomBattleFixture() = default;

// ================ DataAsset 工厂 ================

UCardDefinition* FWacomBattleFixture::MakeSimpleDamageCard(int32 Cost, int32 Damage)
{
	UCardDefinition* Card = NewTransient<UCardDefinition>();
	Card->CardId    = FName(*FString::Printf(TEXT("TestDmgCard_C%d_D%d_%s"), Cost, Damage, *FGuid::NewGuid().ToString(EGuidFormats::Short)));
	Card->BaseCost  = Cost;
	Card->TargetMode = ECardTargetMode::SingleEnemyPart;

	FCardEffect Eff;
	Eff.EffectType = WacomTags::Effect_Damage;
	Eff.Magnitude  = Damage;
	Eff.Target     = WacomTags::Target_SingleEnemyPart;
	Card->Effects.Add(Eff);

	Roots.Add(TStrongObjectPtr<UObject>(Card));
	return Card;
}

UCardDefinition* FWacomBattleFixture::MakeComboDamageCard(int32 Cost, int32 Damage)
{
	UCardDefinition* Card = MakeSimpleDamageCard(Cost, Damage);
	Card->Keywords.AddTag(WacomTags::Card_Keyword_Combo);
	return Card;
}

UCardDefinition* FWacomBattleFixture::MakeNoopCard(int32 Cost)
{
	UCardDefinition* Card = NewTransient<UCardDefinition>();
	Card->CardId     = FName(*FString::Printf(TEXT("TestNoopCard_C%d_%s"), Cost, *FGuid::NewGuid().ToString(EGuidFormats::Short)));
	Card->BaseCost   = Cost;
	Card->TargetMode = ECardTargetMode::None;
	Roots.Add(TStrongObjectPtr<UObject>(Card));
	return Card;
}

UCardDefinition* FWacomBattleFixture::MakeDamageCardWithKeywords(int32 Cost, int32 Damage, const TArray<FGameplayTag>& Keywords)
{
	UCardDefinition* Card = MakeSimpleDamageCard(Cost, Damage);
	for (const FGameplayTag& K : Keywords)
	{
		Card->Keywords.AddTag(K);
	}
	return Card;
}

UEnemyDefinition* FWacomBattleFixture::MakeSinglePartEnemy(int32 Hp, int32 Initiative, int32 IntentResist)
{
	UEnemyPartDefinition* Part = NewTransient<UEnemyPartDefinition>();
	Part->PartId = TEXT("Test.Part.Solo");
	Part->MaxHp  = Hp;
	Part->InitialIntentIndex = 0;

	FIntentDefinition Intent;
	Intent.IntentId        = TEXT("Test.Intent.BasicAttack");
	Intent.Initiative      = Initiative;
	Intent.ResistanceValue = IntentResist;
	FIntentEffect Eff;
	Eff.EffectType = WacomTags::Effect_Damage;
	Eff.Magnitude  = 1;
	Eff.Target     = WacomTags::Target_Player;
	Intent.Effects.Add(Eff);
	Part->IntentSequence.Add(Intent);

	Roots.Add(TStrongObjectPtr<UObject>(Part));

	UEnemyDefinition* Enemy = NewTransient<UEnemyDefinition>();
	Enemy->EnemyId = TEXT("Test.Enemy.Solo");
	FEnemyPartSlot Slot; Slot.PartDef = Part;
	Enemy->Parts.Add(Slot);

	Roots.Add(TStrongObjectPtr<UObject>(Enemy));
	return Enemy;
}

UEnemyDefinition* FWacomBattleFixture::MakeThreePartEnemy(int32 HH, int32 HB, int32 HT,
                                                          int32 IH, int32 IB, int32 IT)
{
	auto MakePart = [this](FName Id, int32 Hp, int32 Initiative) -> UEnemyPartDefinition*
	{
		UEnemyPartDefinition* Part = NewTransient<UEnemyPartDefinition>();
		Part->PartId = Id;
		Part->MaxHp  = Hp;
		Part->InitialIntentIndex = 0;

		FIntentDefinition Intent;
		Intent.IntentId        = Id;
		Intent.Initiative      = Initiative;
		Intent.ResistanceValue = 0;
		FIntentEffect Eff;
		Eff.EffectType = WacomTags::Effect_Damage;
		Eff.Magnitude  = 1;
		Eff.Target     = WacomTags::Target_Player;
		Intent.Effects.Add(Eff);
		Part->IntentSequence.Add(Intent);

		Roots.Add(TStrongObjectPtr<UObject>(Part));
		return Part;
	};

	UEnemyDefinition* Enemy = NewTransient<UEnemyDefinition>();
	Enemy->EnemyId = TEXT("Test.Enemy.ThreeParts");

	FEnemyPartSlot S1; S1.PartDef = MakePart(TEXT("Test.Part.Head"), HH, IH);
	FEnemyPartSlot S2; S2.PartDef = MakePart(TEXT("Test.Part.Body"), HB, IB);
	FEnemyPartSlot S3; S3.PartDef = MakePart(TEXT("Test.Part.Tail"), HT, IT);
	Enemy->Parts = { S1, S2, S3 };

	Roots.Add(TStrongObjectPtr<UObject>(Enemy));
	return Enemy;
}

UCharacterDefinition* FWacomBattleFixture::MakeCharacter(UCardDefinition* LeftHand, UCardDefinition* RightHand,
                                                        const TArray<UCardDefinition*>& StarterDeck)
{
	UCharacterDefinition* Char = NewTransient<UCharacterDefinition>();
	Char->CharacterId   = TEXT("Test.Character");
	// 测试角色 100 本体 HP（高血量避免被中毒/敌方意外打死）。
	// 用 FingerCount × HpPerFinger 计算（HP 重构后字段）。
	Char->FingerCount   = 50;
	Char->HpPerFinger   = 2;
	Char->LeftHandCard  = LeftHand;
	Char->RightHandCard = RightHand;
	for (UCardDefinition* C : StarterDeck) { Char->StarterDeck.Add(C); }

	Roots.Add(TStrongObjectPtr<UObject>(Char));
	return Char;
}

UBattleSession* FWacomBattleFixture::CreateSession(UCharacterDefinition* Character, UEnemyDefinition* Enemy, int32 Seed)
{
	UBattleSession* Session = NewTransient<UBattleSession>();
	SessionPtr = TStrongObjectPtr<UBattleSession>(Session);

	FBattleInitParams P;
	P.Character  = Character;
	P.Enemy      = Enemy;
	P.RandomSeed = Seed;

	const FWacomStatus Status = Session->Initialize(P);
	checkf(Status.IsOk(), TEXT("BattleFixture::CreateSession Initialize failed: %d"), (int32)Status.Code);
	return Session;
}

// ================ Snapshot 查询 ================

FGuid FWacomBattleFixture::FindHandInstanceByCardId(const FBattleSnapshot& Snap, FName CardId)
{
	for (const FHandCardSnapshot& C : Snap.Hand.Cards)
	{
		if (C.Definition && C.Definition->CardId == CardId)
		{
			return C.InstanceId;
		}
	}
	return FGuid();
}

int32 FWacomBattleFixture::FindHandIndex(const FBattleSnapshot& Snap, const FGuid& InstanceId)
{
	for (int32 i = 0; i < Snap.Hand.Cards.Num(); ++i)
	{
		if (Snap.Hand.Cards[i].InstanceId == InstanceId) { return i; }
	}
	return INDEX_NONE;
}

int32 FWacomBattleFixture::FindPartInitiative(const FBattleSnapshot& Snap, int32 PartIndex)
{
	return Snap.Enemy.Parts.IsValidIndex(PartIndex) ? Snap.Enemy.Parts[PartIndex].CurrentInitiative : INT32_MIN;
}

int32 FWacomBattleFixture::FindPartHp(const FBattleSnapshot& Snap, int32 PartIndex)
{
	return Snap.Enemy.Parts.IsValidIndex(PartIndex) ? Snap.Enemy.Parts[PartIndex].CurrentHp : INT32_MIN;
}

FGuid FWacomBattleFixture::FindPartInstanceId(const FBattleSnapshot& Snap, int32 PartIndex)
{
	return Snap.Enemy.Parts.IsValidIndex(PartIndex) ? Snap.Enemy.Parts[PartIndex].InstanceId : FGuid();
}
