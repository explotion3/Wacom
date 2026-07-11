// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Cards/CardPassive.h"
#include "Characters/CharacterDefinition.h"
#include "Commands/BattleCommand.h"
#include "Enemies/EnemyBehaviorDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Enemies/IntentDefinition.h"
#include "Enemies/IntentEffect.h"
#include "Runtime/BattleEnemyKeys.h"
#include "Events/BattleEvent.h"
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

	FWacomEnemyBehaviorIntent MakeTestBehaviorIntent(
		FName IntentId,
		int32 Initiative,
		int32 IntentResist,
		int32 Damage)
	{
		FWacomEnemyBehaviorIntent IntentEntry;
		IntentEntry.Intent.IntentId = IntentId;
		IntentEntry.Intent.DisplayName = FText::FromName(IntentId);
		IntentEntry.Intent.Initiative = Initiative;
		IntentEntry.Intent.ResistanceValue = IntentResist;

		FIntentEffect Eff;
		Eff.EffectType = WacomTags::Effect_Damage;
		Eff.Magnitude = Damage;
		Eff.Target = WacomTags::Target_Player;
		IntentEntry.Intent.Effects.Add(Eff);
		return IntentEntry;
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

UCardDefinition* FWacomBattleFixture::MakeHandCardCostModifierCard(int32 Cost, int32 Magnitude, bool bReduceCost)
{
	UCardDefinition* Card = NewTransient<UCardDefinition>();
	Card->CardId = FName(*FString::Printf(
		TEXT("TestHandCardCost_%s_C%d_M%d_%s"),
		bReduceCost ? TEXT("Reduce") : TEXT("Add"),
		Cost,
		Magnitude,
		*FGuid::NewGuid().ToString(EGuidFormats::Short)));
	Card->BaseCost = Cost;
	Card->TargetMode = ECardTargetMode::HandCard;
	Card->HandCardTargetFilter.bUseExplicitHandCardTargetFilter = true;
	Card->HandCardTargetFilter.bAllowNormalHandCards = true;
	Card->HandCardTargetFilter.bAllowHandAnchors = true;

	FCardEffect Effect;
	Effect.EffectType = bReduceCost
		? WacomTags::Effect_Card_ReduceCost
		: WacomTags::Effect_Card_AddCost;
	Effect.Magnitude = Magnitude;
	Effect.Target = WacomTags::Target_SelectedHandCard;
	Card->Effects.Add(Effect);

	Roots.Add(TStrongObjectPtr<UObject>(Card));
	return Card;
}

UCardDefinition* FWacomBattleFixture::MakeHandCardCostModifierCardWithTargetKeywordFilter(
	int32 Cost,
	int32 Magnitude,
	bool bReduceCost,
	const FGameplayTagContainer& RequiredTargetKeywords,
	const FGameplayTagContainer& BlockedTargetKeywords,
	bool bAllowNormalHandCards,
	bool bAllowHandAnchors)
{
	UCardDefinition* Card = MakeHandCardCostModifierCard(Cost, Magnitude, bReduceCost);
	Card->HandCardTargetFilter.bUseExplicitHandCardTargetFilter = true;
	Card->HandCardTargetFilter.bAllowNormalHandCards = bAllowNormalHandCards;
	Card->HandCardTargetFilter.bAllowHandAnchors = bAllowHandAnchors;
	Card->HandCardTargetFilter.RequiredTargetKeywords = RequiredTargetKeywords;
	Card->HandCardTargetFilter.BlockedTargetKeywords = BlockedTargetKeywords;
	return Card;
}

UCardDefinition* FWacomBattleFixture::MakeSelectedHandCardZoneMoveCard(int32 Cost, bool bExhaust)
{
	UCardDefinition* Card = NewTransient<UCardDefinition>();
	Card->CardId = FName(*FString::Printf(
		TEXT("TestSelectedHandCard_%s_C%d_%s"),
		bExhaust ? TEXT("Exhaust") : TEXT("Discard"),
		Cost,
		*FGuid::NewGuid().ToString(EGuidFormats::Short)));
	Card->BaseCost = Cost;
	Card->TargetMode = ECardTargetMode::HandCard;
	Card->HandCardTargetFilter.bUseExplicitHandCardTargetFilter = true;
	Card->HandCardTargetFilter.bAllowNormalHandCards = true;
	Card->HandCardTargetFilter.bAllowHandAnchors = false;

	FCardEffect Effect;
	Effect.EffectType = bExhaust
		? WacomTags::Effect_Card_ExhaustSelected
		: WacomTags::Effect_Card_DiscardSelected;
	Effect.Magnitude = 1;
	Effect.Target = WacomTags::Target_SelectedHandCard;
	Card->Effects.Add(Effect);

	Roots.Add(TStrongObjectPtr<UObject>(Card));
	return Card;
}

UCardDefinition* FWacomBattleFixture::MakeRandomDiscardCard(int32 Cost, int32 Count)
{
	UCardDefinition* Card = NewTransient<UCardDefinition>();
	Card->CardId = FName(*FString::Printf(
		TEXT("TestRandomDiscard_C%d_N%d_%s"),
		Cost,
		Count,
		*FGuid::NewGuid().ToString(EGuidFormats::Short)));
	Card->BaseCost = Cost;
	Card->TargetMode = ECardTargetMode::None;

	FCardEffect Effect;
	Effect.EffectType = WacomTags::Effect_Discard;
	Effect.Magnitude = Count;
	Effect.Target = WacomTags::Target_Player;
	Card->Effects.Add(Effect);

	Roots.Add(TStrongObjectPtr<UObject>(Card));
	return Card;
}

UCardDefinition* FWacomBattleFixture::MakeOnDiscardShieldCard(int32 Cost, int32 ShieldAmount)
{
	UCardDefinition* Card = MakeNoopCard(Cost);

	FCardPassive Passive;
	Passive.Trigger = WacomTags::Passive_Trigger_OnDiscard;

	FCardEffect Effect;
	Effect.EffectType = WacomTags::Status_Shield;
	Effect.Magnitude = ShieldAmount;
	Effect.Target = WacomTags::Target_Player;
	Passive.Effects.Add(Effect);
	Card->Passives.Add(Passive);
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
	return MakeSinglePartEnemyWithIntentDamage(Hp, Initiative, IntentResist, /*Damage*/1);
}

UEnemyDefinition* FWacomBattleFixture::MakeSinglePartEnemyWithIntentDamage(
	int32 Hp,
	int32 Initiative,
	int32 IntentResist,
	int32 Damage)
{
	UEnemyPartDefinition* Part = NewTransient<UEnemyPartDefinition>();
	Part->PartId = TEXT("Test.Part.Solo");
	Part->MaxHp  = Hp;

	Roots.Add(TStrongObjectPtr<UObject>(Part));

	UEnemyBehaviorDefinition* Behavior = NewTransient<UEnemyBehaviorDefinition>();
	Behavior->BehaviorId = TEXT("Test.Behavior.Solo");
	Behavior->InitialPhaseId = TEXT("Default");
	FWacomEnemyIntentSetDefinition IntentSet;
	IntentSet.IntentSetId = TEXT("Test.IntentSet.Solo");
	IntentSet.AppliesToPartSlotId = TEXT("Test.Part.Solo");
	IntentSet.SelectorMode = EWacomEnemyIntentSelectorMode::Sequence;
	IntentSet.Intents = {
		MakeTestBehaviorIntent(TEXT("Test.Intent.BasicAttack"), Initiative, IntentResist, Damage)
	};
	FWacomEnemyPhaseDefinition Phase;
	Phase.PhaseId = TEXT("Default");
	Phase.IntentSets = { IntentSet };
	Behavior->Phases = { Phase };
	Roots.Add(TStrongObjectPtr<UObject>(Behavior));

	UEnemyDefinition* Enemy = NewTransient<UEnemyDefinition>();
	Enemy->EnemyId = TEXT("Test.Enemy.Solo");
	Enemy->DefaultBehavior = Behavior;
	Enemy->DefaultPhaseId = TEXT("Default");
	FEnemyPartSlot Slot;
	Slot.PartSlotId = TEXT("Test.Part.Solo");
	Slot.PartDef = Part;
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

		Roots.Add(TStrongObjectPtr<UObject>(Part));
		return Part;
	};

	UEnemyDefinition* Enemy = NewTransient<UEnemyDefinition>();
	Enemy->EnemyId = TEXT("Test.Enemy.ThreeParts");

	UEnemyBehaviorDefinition* Behavior = NewTransient<UEnemyBehaviorDefinition>();
	Behavior->BehaviorId = TEXT("Test.Behavior.ThreeParts");
	Behavior->InitialPhaseId = TEXT("Default");
	FWacomEnemyPhaseDefinition Phase;
	Phase.PhaseId = TEXT("Default");

	FEnemyPartSlot S1;
	S1.PartSlotId = TEXT("Test.Part.Head");
	S1.PartDef = MakePart(TEXT("Test.Part.Head"), HH, IH);
	S1.InitialIntentSetId = TEXT("Test.IntentSet.Head");
	FEnemyPartSlot S2;
	S2.PartSlotId = TEXT("Test.Part.Body");
	S2.PartDef = MakePart(TEXT("Test.Part.Body"), HB, IB);
	S2.InitialIntentSetId = TEXT("Test.IntentSet.Body");
	FEnemyPartSlot S3;
	S3.PartSlotId = TEXT("Test.Part.Tail");
	S3.PartDef = MakePart(TEXT("Test.Part.Tail"), HT, IT);
	S3.InitialIntentSetId = TEXT("Test.IntentSet.Tail");
	Enemy->Parts = { S1, S2, S3 };

	auto AddIntentSet = [&Phase](FName IntentSetId, FName PartSlotId, int32 Initiative)
	{
		FWacomEnemyIntentSetDefinition IntentSet;
		IntentSet.IntentSetId = IntentSetId;
		IntentSet.AppliesToPartSlotId = PartSlotId;
		IntentSet.SelectorMode = EWacomEnemyIntentSelectorMode::Sequence;
		IntentSet.Intents = {
			MakeTestBehaviorIntent(PartSlotId, Initiative, /*IntentResist*/0, /*Damage*/1)
		};
		Phase.IntentSets.Add(IntentSet);
	};
	AddIntentSet(TEXT("Test.IntentSet.Head"), TEXT("Test.Part.Head"), IH);
	AddIntentSet(TEXT("Test.IntentSet.Body"), TEXT("Test.Part.Body"), IB);
	AddIntentSet(TEXT("Test.IntentSet.Tail"), TEXT("Test.Part.Tail"), IT);
	Behavior->Phases = { Phase };
	Enemy->DefaultBehavior = Behavior;
	Enemy->DefaultPhaseId = TEXT("Default");
	Roots.Add(TStrongObjectPtr<UObject>(Behavior));

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
	return CreateInitializedSession(Character, Enemy, Seed).Session;
}

FWacomInitializedBattleSession FWacomBattleFixture::CreateInitializedSession(
	UCharacterDefinition* Character,
	UEnemyDefinition* Enemy,
	int32 Seed)
{
	UBattleSession* Session = NewTransient<UBattleSession>();
	SessionPtr = TStrongObjectPtr<UBattleSession>(Session);

	FBattleInitParams P;
	P.Character  = Character;
	P.RandomSeed = Seed;
	FBattleEnemySlotInit EnemySlot;
	EnemySlot.EnemySlotId = TEXT("Enemy");
	EnemySlot.Enemy = Enemy;
	P.EnemySlots.Add(EnemySlot);

	FBattleInitializationResult Initialization = Session->Initialize(P);
	checkf(
		Initialization.IsOk(),
		TEXT("BattleFixture::CreateInitializedSession Initialize failed: %d"),
		(int32)Initialization.Status.Code);
	return { Session, MoveTemp(Initialization) };
}

// ================ Snapshot 查询 ================

FGuid FWacomBattleFixture::FindHandInstanceByCardId(const FBattleSnapshot& Snap, FName CardId)
{
	if (const FHandCardSnapshot* Card = FindHandCardByCardId(Snap, CardId))
	{
		return Card->InstanceId;
	}
	return FGuid();
}

const FHandCardSnapshot* FWacomBattleFixture::FindHandCardByInstanceId(const FBattleSnapshot& Snap, const FGuid& InstanceId)
{
	for (const FHandCardSnapshot& Card : Snap.Hand.Cards)
	{
		if (Card.InstanceId == InstanceId)
		{
			return &Card;
		}
	}
	return nullptr;
}

const FHandCardSnapshot* FWacomBattleFixture::FindHandCardByCardId(const FBattleSnapshot& Snap, FName CardId)
{
	for (const FHandCardSnapshot& Card : Snap.Hand.Cards)
	{
		if (Card.Definition && Card.Definition->CardId == CardId)
		{
			return &Card;
		}
	}
	return nullptr;
}

FGuid FWacomBattleFixture::FindHandInstanceByCardIdInZone(const FBattleSnapshot& Snap, FName CardId, EHandZone Zone)
{
	for (const FHandCardSnapshot& Card : Snap.Hand.Cards)
	{
		if (Card.bIsHandAnchor)
		{
			continue;
		}
		if (Card.Zone != Zone)
		{
			continue;
		}
		if (Card.Definition && Card.Definition->CardId == CardId)
		{
			return Card.InstanceId;
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

const FEnemySnapshot* FWacomBattleFixture::GetEnemySnapshot(const FBattleSnapshot& Snap, int32 EnemyIndex)
{
	return Snap.Enemies.IsValidIndex(EnemyIndex) ? &Snap.Enemies[EnemyIndex] : nullptr;
}

const FEnemyPartSnapshot* FWacomBattleFixture::GetEnemyPartSnapshot(
	const FBattleSnapshot& Snap,
	int32 EnemyIndex,
	int32 PartIndex)
{
	const FEnemySnapshot* Enemy = GetEnemySnapshot(Snap, EnemyIndex);
	return Enemy && Enemy->Parts.IsValidIndex(PartIndex) ? &Enemy->Parts[PartIndex] : nullptr;
}

const FEnemyPartSnapshot* FWacomBattleFixture::GetEnemyPartSnapshot(const FBattleSnapshot& Snap, int32 PartIndex)
{
	return GetEnemyPartSnapshot(Snap, 0, PartIndex);
}

int32 FWacomBattleFixture::FindPartInitiative(const FBattleSnapshot& Snap, int32 PartIndex)
{
	const FEnemyPartSnapshot* Part = GetEnemyPartSnapshot(Snap, PartIndex);
	return Part ? Part->CurrentInitiative : INT32_MIN;
}

int32 FWacomBattleFixture::FindPartHp(const FBattleSnapshot& Snap, int32 PartIndex)
{
	const FEnemyPartSnapshot* Part = GetEnemyPartSnapshot(Snap, PartIndex);
	return Part ? Part->CurrentHp : INT32_MIN;
}

FGuid FWacomBattleFixture::FindPartInstanceId(const FBattleSnapshot& Snap, int32 PartIndex)
{
	const FEnemyPartSnapshot* Part = GetEnemyPartSnapshot(Snap, PartIndex);
	return Part ? Part->InstanceId : FGuid();
}

FBattleEnemyPartKey FWacomBattleFixture::FindPartKey(const FBattleSnapshot& Snap, int32 PartIndex)
{
	const FEnemyPartSnapshot* Part = GetEnemyPartSnapshot(Snap, PartIndex);
	return Part ? Part->PartKey : FBattleEnemyPartKey();
}

FBattleEnemyPartKey FWacomBattleFixture::FindPartKeyByInstanceId(
	const FBattleSnapshot& Snap,
	const FGuid& PartInstanceId)
{
	for (const FEnemySnapshot& Enemy : Snap.Enemies)
	{
		for (const FEnemyPartSnapshot& Part : Enemy.Parts)
		{
			if (Part.InstanceId == PartInstanceId)
			{
				return Part.PartKey;
			}
		}
	}
	return FBattleEnemyPartKey();
}

FBattleCommand FWacomBattleFixture::MakePlayCardOnPart(
	const FBattleSnapshot& Snap,
	const FGuid& CardInstanceId,
	int32 PartIndex)
{
	return FBattleCommand::MakePlayCardOnEnemyPartKey(CardInstanceId, FindPartKey(Snap, PartIndex));
}

FBattleCommand FWacomBattleFixture::MakePlayCardOnPartInstance(
	const FBattleSnapshot& Snap,
	const FGuid& CardInstanceId,
	const FGuid& PartInstanceId)
{
	return FBattleCommand::MakePlayCardOnEnemyPartKey(
		CardInstanceId,
		FindPartKeyByInstanceId(Snap, PartInstanceId));
}

const FEnemyPartSnapshot* FWacomBattleFixture::FindPartByPartId(const FBattleSnapshot& Snap, FName PartId)
{
	for (const FEnemySnapshot& Enemy : Snap.Enemies)
	{
		for (const FEnemyPartSnapshot& Part : Enemy.Parts)
		{
			if (Part.Definition && Part.Definition->PartId == PartId)
			{
				return &Part;
			}
		}
	}
	return nullptr;
}

FGuid FWacomBattleFixture::FindPartInstanceByPartId(const FBattleSnapshot& Snap, FName PartId)
{
	if (const FEnemyPartSnapshot* Part = FindPartByPartId(Snap, PartId))
	{
		return Part->InstanceId;
	}
	return FGuid();
}

int32 FWacomBattleFixture::GetStatusStacks(const TMap<FGameplayTag, int32>& StatusStacks, const FGameplayTag& StatusTag)
{
	if (const int32* Stacks = StatusStacks.Find(StatusTag))
	{
		return *Stacks;
	}
	return 0;
}

int32 FWacomBattleFixture::CountEvents(const TArray<FBattleEvent>& Events, EBattleEventType Type)
{
	int32 Count = 0;
	for (const FBattleEvent& Event : Events)
	{
		if (Event.Type == Type)
		{
			++Count;
		}
	}
	return Count;
}

int32 FWacomBattleFixture::CountEventsWithTag(
	const TArray<FBattleEvent>& Events,
	EBattleEventType Type,
	const FGameplayTag& Tag)
{
	int32 Count = 0;
	for (const FBattleEvent& Event : Events)
	{
		if (Event.Type == Type && Event.Tag == Tag)
		{
			++Count;
		}
	}
	return Count;
}

bool FWacomBattleFixture::HasEvent(
	const TArray<FBattleEvent>& Events,
	EBattleEventType Type,
	const FGameplayTag& Tag)
{
	for (const FBattleEvent& Event : Events)
	{
		if (Event.Type != Type)
		{
			continue;
		}
		if (Tag.IsValid() && Event.Tag != Tag)
		{
			continue;
		}
		return true;
	}
	return false;
}

bool FWacomBattleFixture::HasEventForActor(
	const TArray<FBattleEvent>& Events,
	EBattleEventType Type,
	const FGuid& ActorInstanceId,
	const FGameplayTag& Tag)
{
	for (const FBattleEvent& Event : Events)
	{
		if (Event.Type != Type)
		{
			continue;
		}
		if (Event.ActorInstanceId != ActorInstanceId)
		{
			continue;
		}
		if (Tag.IsValid() && Event.Tag != Tag)
		{
			continue;
		}
		return true;
	}
	return false;
}

int32 FWacomBattleFixture::SumDamageEventsForCard(const TArray<FBattleEvent>& Events, const FGuid& CardInstanceId)
{
	int32 Total = 0;
	for (const FBattleEvent& Event : Events)
	{
		if (Event.Type == EBattleEventType::DamageDealt && Event.CardInstanceId == CardInstanceId)
		{
			Total += Event.Amount;
		}
	}
	return Total;
}
