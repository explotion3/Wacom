// Copyright Wacom. All Rights Reserved.

#include "Session/BattleSession.h"

#include "Core/BattleState.h"
#include "Core/BattleResolver.h"
#include "Core/BattleTurnFlow.h"
#include "Commands/KnockdownChoiceAvailability.h"
#include "Deck/DeckService.h"
#include "Events/BattleEventBus.h"
#include "Snapshots/BattleSnapshotBuilder.h"

#include "Characters/CharacterDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Cards/CardDefinition.h"
#include "Cards/CardPhysique.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Runtime/RuntimeEnemyPart.h"
#include "Tags/WacomGameplayTags.h"

UBattleSession::UBattleSession()
	: State(nullptr)
	, EventBus(nullptr)
{
	State    = new FBattleState();
	EventBus = new FBattleEventBus();
}

UBattleSession::~UBattleSession()
{
	delete State;
	State = nullptr;
	delete EventBus;
	EventBus = nullptr;
}

FWacomStatus UBattleSession::Initialize(const FBattleInitParams& Params)
{
	if (!Params.Character)
	{
		return FWacomStatus::Fail(EWacomError::InvalidArgument, TEXT("NoCharacter"));
	}
	if (!Params.Enemy)
	{
		return FWacomStatus::Fail(EWacomError::InvalidArgument, TEXT("NoEnemy"));
	}

	// 重置状态容器。
	delete State;
	State = new FBattleState();
	EventBus->Reset();
	ReferencedAssets.Reset();

	// ---- Rng ----
	const int32 Seed = (Params.RandomSeed != 0)
		? Params.RandomSeed
		: static_cast<int32>(FPlatformTime::Cycles());
	State->Rng.Initialize(Seed);

	// ---- 玩家 ----
	State->Player.CharacterDef    = Params.Character;
	State->Player.MaxHp     = Params.Character->GetBasePlayerMaxHp();
	State->Player.CurrentHp = State->Player.MaxHp;
	State->Player.Shield    = 0;
	ReferencedAssets.Add(Params.Character);

	// 阈值灌入。玩家 HP 变更路径负责维护跨阈值 flag。
	State->HighHpThreshold = Params.HighHpThreshold;
	State->LowHpThreshold  = Params.LowHpThreshold;

	// ---- 卡牌：左手 / 右手 / StarterDeck ----
	// 战内 HP 上限规则：
	//   战内 MaxHp = 本体上限 + Σ(备战卡组中带 Companion 关键词的卡的 MaxHpBonus)
	//
	// 只有带 Card.Keyword.Companion 的卡才计入累加。武器 / 工具 / 中立卡即便填了
	// MaxHpBonus 也不计入。烁光蝶（伙伴+武器+连击）因为带伙伴关键词，仍然计入。
	//
	// 当前在 Initialize 时全量累加；若以后定义"卡留在抽牌堆时不生效"，再改为懒触发。
	auto CreateCardInstance = [this](
		const UCardDefinition* Def,
		ECardLocation InitialLocation,
		const FGameplayTagContainer* CapacityEffectTags = nullptr) -> FGuid
	{
		FRuntimeCardInstance Card;
		Card.InstanceId = FGuid::NewGuid();
		Card.Definition = Def;
		Card.Location   = InitialLocation;
		if (CapacityEffectTags)
		{
			Card.CapacityEffectTags = *CapacityEffectTags;
		}

		const int32 NewIdx = State->Cards.AllCards.Add(Card);
		State->Cards.CardIndexById.Add(Card.InstanceId, NewIdx);

		if (Def)
		{
			ReferencedAssets.Add(Def);
			const bool bIsCompanion = Def->Keywords.HasTagExact(WacomTags::Card_Keyword_Companion);
			const int32 HpBonus = Def->Physique.MaxHpBonus;
			if (bIsCompanion && HpBonus > 0)
			{
				State->Player.MaxHp     += HpBonus;
				State->Player.CurrentHp += HpBonus;
			}
		}
		return Card.InstanceId;
	};

	if (Params.Character->LeftHandCard)
	{
		// 左右手先以 Unknown 位置登记，回合开始由 HandZoneService 放入 Hand。
		State->Cards.LeftHandInstanceId  = CreateCardInstance(Params.Character->LeftHandCard, ECardLocation::Unknown);
	}
	if (Params.Character->RightHandCard)
	{
		State->Cards.RightHandInstanceId = CreateCardInstance(Params.Character->RightHandCard, ECardLocation::Unknown);
	}

	// 战斗只读备战卡组。优先使用 BattleDeckEntries（来自 RunState.BattleDeck
	// 与 SpecialZone 入战卡），其次使用旧 BattleDeckOverride，空时回退 StarterDeck。
	if (Params.BattleDeckEntries.Num() > 0)
	{
		for (const FBattleDeckEntry& Entry : Params.BattleDeckEntries)
		{
			if (!Entry.Definition) { continue; }
			const FGuid CardId = CreateCardInstance(
				Entry.Definition.Get(),
				ECardLocation::Draw,
				&Entry.CapacityEffectTags);
			State->Cards.DrawPile.Add(CardId);
		}
	}
	else if (Params.BattleDeckOverride.Num() > 0)
	{
		for (const TObjectPtr<const UCardDefinition>& CardDef : Params.BattleDeckOverride)
		{
			if (!CardDef) { continue; }
			const FGuid CardId = CreateCardInstance(CardDef.Get(), ECardLocation::Draw);
			State->Cards.DrawPile.Add(CardId);
		}
	}
	else
	{
		for (const TObjectPtr<UCardDefinition>& CardDef : Params.Character->StarterDeck)
		{
			if (!CardDef) { continue; }
			const FGuid CardId = CreateCardInstance(CardDef.Get(), ECardLocation::Draw);
			State->Cards.DrawPile.Add(CardId);
		}
	}

	// 初始洗牌：消除"StarterDeck 数组顺序 = 首回合抽牌顺序"的隐式依赖。
	// 之后的 ReshuffleDiscardIntoDraw 会用同样的 Rng 做洗牌，保持一致。
	FDeckService::ShuffleDrawPile(*State);

	// ---- 敌人 ----
	State->Enemy.Definition = Params.Enemy;
	ReferencedAssets.Add(Params.Enemy);

	for (const FEnemyPartSlot& Slot : Params.Enemy->Parts)
	{
		if (!Slot.PartDef) { continue; }
		FRuntimeEnemyPart Part;
		Part.InstanceId         = FGuid::NewGuid();
		Part.Definition         = Slot.PartDef;
		Part.CurrentHp          = Slot.PartDef->MaxHp;
		Part.CurrentIntentIndex = Slot.PartDef->InitialIntentIndex;
		// 从初始意图读取先机值。若 IntentSequence 为空，保持 0。
		if (Slot.PartDef->IntentSequence.IsValidIndex(Part.CurrentIntentIndex))
		{
			Part.CurrentInitiative = Slot.PartDef->IntentSequence[Part.CurrentIntentIndex].Initiative;
		}
		else
		{
			Part.CurrentInitiative = 0;
		}

		const int32 NewIdx = State->Enemy.Parts.Add(Part);
		State->Enemy.PartIndexById.Add(Part.InstanceId, NewIdx);

		ReferencedAssets.Add(Slot.PartDef);
	}

	// ---- 应用预先破坏部位（撤离重入）----
	// 来自 RunSession.BattleProgress 的持久化破坏列表。
	// 不发 EnemyPartHpEmptied 事件、不入 PendingKnockdownEvents、不发 KnockdownExpGain
	// （已经在上一次撤离时处理过了，避免重复弹 dialog 和刷经验）。
	// 但加进 DestroyedPartIds，让本场撤离/胜利时仍能完整持久化。
	if (Params.PreDestroyedPartIds.Num() > 0)
	{
		for (FRuntimeEnemyPart& P : State->Enemy.Parts)
		{
			if (!P.Definition) { continue; }
			if (Params.PreDestroyedPartIds.Contains(P.Definition->PartId))
			{
				P.bDestroyed        = true;
				P.CurrentHp         = 0;
				P.CurrentInitiative = 0;
				State->DestroyedPartIds.AddUnique(P.Definition->PartId);

				UE_LOG(LogTemp, Display,
					TEXT("[BattleSession] Initialize: 应用预先破坏部位 %s（来自 RunState.BattleProgress）"),
					*P.Definition->PartId.ToString());
			}
		}
	}

	// ---- 阶段推进 ----
	// Setup -> TurnStart -> PlayerAction。
	// Setup 阶段完成敌人初始化；TurnStart 由 FBattleTurnFlow::BeginPlayerTurn 执行。
	State->Phase            = EBattlePhase::Setup;
	State->TurnNumber       = 1;
	State->CurrentWaitValue = 2;
	State->StateVersion     = 0;

	{
		FBattleEvent StartEvent;
		StartEvent.Type = EBattleEventType::BattleStarted;
		EventBus->Emit(StartEvent);
	}

	{
		FBattleEvent TurnEvent;
		TurnEvent.Type  = EBattleEventType::TurnStarted;
		TurnEvent.Count = State->TurnNumber;
		EventBus->Emit(TurnEvent);
	}

	// 起始阶段：抽牌、重置等待值、生成手牌队列。
	FBattleTurnFlow::BeginPlayerTurn(*State, *EventBus, /*bIsFirstTurn=*/true);

	return FWacomStatus::Ok();
}

FWacomStatus UBattleSession::SubmitCommand(const FBattleCommand& Command)
{
	if (!State)
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("NotInitialized"));
	}
	if (State->Phase == EBattlePhase::BattleEnd)
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("BattleEnded"));
	}

	const int32 VersionBefore = State->StateVersion;
	const FWacomStatus Status = FBattleResolver::Resolve(*State, *EventBus, Command);
	if (Status.IsOk() && State->StateVersion == VersionBefore)
	{
		// 成功执行但未显式递增版本号，补一次。Resolver 应当自行管理，这里只是兜底。
		++State->StateVersion;
	}

	// 命令成功 + 战斗未结束 + 队列非空 -> 切到 PendingKnockdownChoice 阶段。
	if (Status.IsOk()
		&& State->Phase != EBattlePhase::BattleEnd
		&& State->Phase != EBattlePhase::PendingKnockdownChoice
		&& State->PendingKnockdownEvents.Num() > 0)
	{
		State->Phase = EBattlePhase::PendingKnockdownChoice;
		++State->StateVersion;

		const FBattleState::FPendingKnockdownEvent& Head = State->PendingKnockdownEvents[0];
		FBattleEvent Ev;
		Ev.Type            = EBattleEventType::KnockdownChoiceRequested;
		Ev.ActorInstanceId = Head.PartInstanceId;
		Ev.Count           = FKnockdownChoiceAvailability::BuildLegacyEventMask(
			FKnockdownChoiceAvailability::BuildView(*State));
		EventBus->Emit(Ev);
	}

	return Status;
}

FBattleSnapshot UBattleSession::BuildSnapshot() const
{
	if (!State)
	{
		return FBattleSnapshot{};
	}
	return FBattleSnapshotBuilder::Build(*State);
}

TArray<FBattleEvent> UBattleSession::ConsumeEvents()
{
	if (!EventBus)
	{
		return {};
	}
	return EventBus->Consume();
}

bool UBattleSession::IsBattleEnded() const
{
	return State != nullptr && State->Phase == EBattlePhase::BattleEnd;
}

EBattlePhase UBattleSession::GetPhase() const
{
	return State != nullptr ? State->Phase : EBattlePhase::None;
}

FKnockdownChoiceView UBattleSession::BuildPendingKnockdownChoiceView() const
{
	return State ? FKnockdownChoiceAvailability::BuildView(*State) : FKnockdownChoiceView{};
}

FBattleResultPacket UBattleSession::BuildResultPacket() const
{
	FBattleResultPacket Packet;
	if (!State)
	{
		return Packet;
	}

	Packet.Outcome                  = State->Outcome;
	Packet.bCrossedHighHpThreshold  = State->bCrossedHighHpThreshold;
	Packet.bCrossedLowHpThreshold   = State->bCrossedLowHpThreshold;
	Packet.bMutualDestruction       = State->bMutualDestruction;
	Packet.KnockdownExpGains        = State->PendingKnockdownExpGains;
	Packet.KnockdownChoices         = State->PendingKnockdownChoices;
	Packet.GainedCards              = State->PendingGainedCards;
	Packet.DestroyedPartIds         = State->DestroyedPartIds;
	// bWithdrawn：通过 KnockdownChoices 末尾是否有 Withdraw 项判定（撤离结束后队列清空、最后一项必是 Withdraw）
	for (const FKnockdownChoice& C : State->PendingKnockdownChoices)
	{
		if (C.Choice == EKnockdownChoice::Withdraw)
		{
			Packet.bWithdrawn = true;
			break;
		}
	}
	return Packet;
}
