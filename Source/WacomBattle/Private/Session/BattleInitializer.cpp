// Copyright Wacom. All Rights Reserved.

#include "Session/BattleInitializer.h"

#include "Core/BattleState.h"
#include "Core/BattleTurnFlow.h"
#include "Deck/DeckService.h"
#include "Enemy/EnemyIntentSelector.h"
#include "Events/BattleEventBus.h"
#include "Session/BattleSession.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardPhysique.h"
#include "Characters/CharacterDefinition.h"
#include "Enemies/EnemyBehaviorDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Runtime/RuntimeEnemyPart.h"
#include "Tags/WacomGameplayTags.h"

#include "HAL/PlatformTime.h"

namespace
{
	FName GetEffectiveEncounterId(FName EncounterId)
	{
		return EncounterId.IsNone() ? FName(TEXT("Encounter")) : EncounterId;
	}

	FName GetEffectiveEnemySlotId(FName EnemySlotId)
	{
		return EnemySlotId.IsNone() ? FName(TEXT("Enemy")) : EnemySlotId;
	}

	FGuid CreateCardInstance(
		FBattleState& State,
		TArray<TObjectPtr<const UObject>>& ReferencedAssets,
		const UCardDefinition* Def,
		ECardLocation InitialLocation,
		const FGameplayTagContainer* CapacityEffectTags = nullptr)
	{
		FRuntimeCardInstance Card;
		Card.InstanceId = FGuid::NewGuid();
		Card.Definition = Def;
		Card.Location   = InitialLocation;
		if (CapacityEffectTags)
		{
			Card.CapacityEffectTags = *CapacityEffectTags;
		}

		const int32 NewIdx = State.Cards.AllCards.Add(Card);
		State.Cards.CardIndexById.Add(Card.InstanceId, NewIdx);

		if (Def)
		{
			ReferencedAssets.Add(Def);
			const bool bIsCompanion = Def->Keywords.HasTagExact(WacomTags::Card_Keyword_Companion);
			const int32 HpBonus = Def->Physique.MaxHpBonus;
			if (bIsCompanion && HpBonus > 0)
			{
				State.Player.MaxHp     += HpBonus;
				State.Player.CurrentHp += HpBonus;
			}
		}
		return Card.InstanceId;
	}

	bool IsPartPreDestroyed(
		const FBattlePartSlotIdentity& Identity,
		const TArray<FBattlePartSlotIdentity>& PreDestroyedParts)
	{
		for (const FBattlePartSlotIdentity& PreDestroyedIdentity : PreDestroyedParts)
		{
			if (Identity.MatchesRuntimeSlot(PreDestroyedIdentity))
			{
				return true;
			}
		}

		return false;
	}
}

FWacomStatus FBattleInitializer::Initialize(
	FBattleState& State,
	FBattleEventBus& EventBus,
	const FBattleInitParams& Params,
	TArray<TObjectPtr<const UObject>>& ReferencedAssets)
{
	// ---- Rng ----
	const int32 Seed = (Params.RandomSeed != 0)
		? Params.RandomSeed
		: static_cast<int32>(FPlatformTime::Cycles());
	State.Rng.Initialize(Seed);

	// ---- 玩家 ----
	State.Player.CharacterDef = Params.Character;
	State.Player.MaxHp        = Params.Character->GetBasePlayerMaxHp();
	State.Player.CurrentHp    = State.Player.MaxHp;
	State.Player.Shield       = 0;
	ReferencedAssets.Add(Params.Character);

	// 阈值灌入。玩家 HP 变更路径负责维护跨阈值 flag。
	State.HighHpThreshold = Params.HighHpThreshold;
	State.LowHpThreshold  = Params.LowHpThreshold;

	// ---- 卡牌：左手 / 右手 / StarterDeck ----
	// 战内 HP 上限规则：
	//   战内 MaxHp = 本体上限 + Σ(备战卡组中带 Companion 关键词的卡的 MaxHpBonus)
	//
	// 只有带 Card.Keyword.Companion 的卡才计入累加。武器 / 工具 / 中立卡即便填了
	// MaxHpBonus 也不计入。烁光蝶（伙伴+武器+连击）因为带伙伴关键词，仍然计入。
	//
	// 当前在 Initialize 时全量累加；若以后定义"卡留在抽牌堆时不生效"，再改为懒触发。
	if (Params.Character->LeftHandCard)
	{
		// 左右手先以 Unknown 位置登记，回合开始由 HandZoneService 放入 Hand。
		State.Cards.LeftHandInstanceId = CreateCardInstance(
			State,
			ReferencedAssets,
			Params.Character->LeftHandCard,
			ECardLocation::Unknown);
	}
	if (Params.Character->RightHandCard)
	{
		State.Cards.RightHandInstanceId = CreateCardInstance(
			State,
			ReferencedAssets,
			Params.Character->RightHandCard,
			ECardLocation::Unknown);
	}

	// 战斗只读备战卡组。优先使用 BattleDeckEntries（来自 RunState.BattleDeck
	// 与 SpecialZone 入战卡），其次使用旧 BattleDeckOverride，空时回退 StarterDeck。
	if (Params.BattleDeckEntries.Num() > 0)
	{
		for (const FBattleDeckEntry& Entry : Params.BattleDeckEntries)
		{
			if (!Entry.Definition) { continue; }
			const FGuid CardId = CreateCardInstance(
				State,
				ReferencedAssets,
				Entry.Definition.Get(),
				ECardLocation::Draw,
				&Entry.CapacityEffectTags);
			State.Cards.DrawPile.Add(CardId);
		}
	}
	else if (Params.BattleDeckOverride.Num() > 0)
	{
		for (const TObjectPtr<const UCardDefinition>& CardDef : Params.BattleDeckOverride)
		{
			if (!CardDef) { continue; }
			const FGuid CardId = CreateCardInstance(
				State,
				ReferencedAssets,
				CardDef.Get(),
				ECardLocation::Draw);
			State.Cards.DrawPile.Add(CardId);
		}
	}
	else
	{
		for (const TObjectPtr<UCardDefinition>& CardDef : Params.Character->StarterDeck)
		{
			if (!CardDef) { continue; }
			const FGuid CardId = CreateCardInstance(
				State,
				ReferencedAssets,
				CardDef.Get(),
				ECardLocation::Draw);
			State.Cards.DrawPile.Add(CardId);
		}
	}

	// 初始洗牌：消除"StarterDeck 数组顺序 = 首回合抽牌顺序"的隐式依赖。
	// 之后的 ReshuffleDiscardIntoDraw 会用同样的 Rng 做洗牌，保持一致。
	FDeckService::ShuffleDrawPile(State);

	// ---- 敌人 ----
	const FName EncounterId = GetEffectiveEncounterId(Params.EncounterId);
	State.Enemy.EncounterId = EncounterId;

	TSet<FName> UsedEnemySlotIds;
	for (const FBattleEnemySlotInit& EnemySlotInput : Params.EnemySlots)
	{
		if (!EnemySlotInput.Enemy)
		{
			return FWacomStatus::Fail(EWacomError::InvalidArgument, TEXT("MissingEnemySlotDefinition"));
		}

		const FName EnemySlotId = GetEffectiveEnemySlotId(EnemySlotInput.EnemySlotId);
		if (UsedEnemySlotIds.Contains(EnemySlotId))
		{
			return FWacomStatus::Fail(EWacomError::InvalidArgument, TEXT("DuplicateEnemySlotId"));
		}
		UsedEnemySlotIds.Add(EnemySlotId);

		FEnemySlotState RuntimeEnemySlot;
		RuntimeEnemySlot.EncounterId = EncounterId;
		RuntimeEnemySlot.EnemySlotId = EnemySlotId;
		RuntimeEnemySlot.Definition = EnemySlotInput.Enemy;
		ReferencedAssets.Add(EnemySlotInput.Enemy);
		if (EnemySlotInput.Enemy->DefaultBehavior)
		{
			ReferencedAssets.Add(EnemySlotInput.Enemy->DefaultBehavior);
		}

		TSet<FName> UsedPartSlotIds;
		for (const FEnemyPartSlot& Slot : EnemySlotInput.Enemy->Parts)
		{
			if (!Slot.PartDef) { continue; }

			const FName PartSlotId = Slot.PartSlotId;
			if (PartSlotId.IsNone())
			{
				return FWacomStatus::Fail(EWacomError::InvalidArgument, TEXT("MissingPartSlotId"));
			}
			if (UsedPartSlotIds.Contains(PartSlotId))
			{
				return FWacomStatus::Fail(EWacomError::InvalidArgument, TEXT("DuplicatePartSlotId"));
			}
			UsedPartSlotIds.Add(PartSlotId);

			FRuntimeEnemyPart Part;
			Part.InstanceId         = FGuid::NewGuid();
			Part.Definition         = Slot.PartDef;
			Part.BehaviorDefinition = Slot.BehaviorOverride
				? Slot.BehaviorOverride.Get()
				: EnemySlotInput.Enemy->DefaultBehavior.Get();
			Part.CurrentPhaseId = !EnemySlotInput.Enemy->DefaultPhaseId.IsNone()
				? EnemySlotInput.Enemy->DefaultPhaseId
				: (Part.BehaviorDefinition ? Part.BehaviorDefinition->InitialPhaseId : NAME_None);
			Part.PreferredIntentSetId = Slot.InitialIntentSetId;
		Part.Identity           = FBattlePartSlotIdentity::Make(
			EncounterId,
			EnemySlotId,
			PartSlotId);
			Part.CurrentHp          = Slot.PartDef->MaxHp;

			const int32 NewIdx = State.Enemy.Parts.Add(Part);
			State.Enemy.PartIndexById.Add(Part.InstanceId, NewIdx);
			State.Enemy.PartIndexByKey.Add(Part.Identity.ToEnemyPartKey(), NewIdx);
			RuntimeEnemySlot.PartInstanceIds.Add(Part.InstanceId);

			ReferencedAssets.Add(Slot.PartDef);
			if (Slot.BehaviorOverride)
			{
				ReferencedAssets.Add(Slot.BehaviorOverride);
			}

		}

		State.Enemy.EnemySlots.Add(MoveTemp(RuntimeEnemySlot));
	}

	if (State.Enemy.EnemySlots.IsEmpty())
	{
		return FWacomStatus::Fail(EWacomError::InvalidArgument, TEXT("NoEnemy"));
	}

	for (FRuntimeEnemyPart& Part : State.Enemy.Parts)
	{
		FEnemyIntentSelector::RefreshIntentForPart(State, Part, /*bAdvanceSequence*/false);
	}

	// ---- 应用预先破坏部位（撤离重入）----
	// 来自 RunSession.BattleProgress 的持久化破坏列表。
	// 不发 EnemyPartHpEmptied 事件、不入 PendingKnockdownEvents、不发 KnockdownExpGain
	// （已经在上一次撤离时处理过了，避免重复弹 dialog 和刷经验）。
	// 但加进 DestroyedParts，让本场撤离/胜利时仍能完整持久化。
	if (Params.PreDestroyedParts.Num() > 0)
	{
		for (FRuntimeEnemyPart& P : State.Enemy.Parts)
		{
			if (!P.Definition) { continue; }
			if (IsPartPreDestroyed(P.Identity, Params.PreDestroyedParts))
			{
				P.bDestroyed        = true;
				P.CurrentHp         = 0;
				P.CurrentInitiative = 0;
				State.DestroyedParts.AddUnique(P.Identity);

				UE_LOG(LogTemp, Display,
					TEXT("[BattleSession] Initialize: 应用预先破坏部位 %s（来自 RunState.BattleProgress）"),
					*P.Identity.ToDebugString());
			}
		}
	}

	// ---- 阶段推进 ----
	// Setup -> TurnStart -> PlayerAction。
	// Setup 阶段完成敌人初始化；TurnStart 由 FBattleTurnFlow::BeginPlayerTurn 执行。
	State.Phase            = EBattlePhase::Setup;
	State.TurnNumber       = 1;
	State.CurrentWaitValue = 2;
	State.StateVersion     = 0;

	{
		FBattleEvent StartEvent;
		StartEvent.Type = EBattleEventType::BattleStarted;
		EventBus.Emit(StartEvent);
	}

	for (const FRuntimeEnemyPart& Part : State.Enemy.Parts)
	{
		if (Part.bDestroyed)
		{
			continue;
		}

		if (!Part.CurrentPhaseId.IsNone())
		{
			FBattleEvent PhaseEvent;
			PhaseEvent.Type = EBattleEventType::EnemyPhaseChanged;
			PhaseEvent.ActorInstanceId = Part.InstanceId;
			PhaseEvent.ActorEnemyPartKey = Part.Identity.ToEnemyPartKey();
			PhaseEvent.EnemyPhaseId = Part.CurrentPhaseId;
			EventBus.Emit(PhaseEvent);
		}

		if (!Part.CurrentIntentId.IsNone())
		{
			FBattleEvent IntentEvent;
			IntentEvent.Type = EBattleEventType::EnemyIntentSelected;
			IntentEvent.ActorInstanceId = Part.InstanceId;
			IntentEvent.ActorEnemyPartKey = Part.Identity.ToEnemyPartKey();
			IntentEvent.IntentId = Part.CurrentIntentId;
			IntentEvent.IntentSetId = Part.CurrentIntentSetId;
			IntentEvent.EnemyPhaseId = Part.CurrentPhaseId;
			IntentEvent.Amount = Part.CurrentInitiative;
			EventBus.Emit(IntentEvent);
		}
	}

	{
		FBattleEvent TurnEvent;
		TurnEvent.Type  = EBattleEventType::TurnStarted;
		TurnEvent.Count = State.TurnNumber;
		EventBus.Emit(TurnEvent);
	}

	// 起始阶段：抽牌、重置等待值、生成手牌队列。
	FBattleTurnFlow::BeginPlayerTurn(State, EventBus, /*bIsFirstTurn=*/true);

	return FWacomStatus::Ok();
}
