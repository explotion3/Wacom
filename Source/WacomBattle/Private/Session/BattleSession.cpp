// Copyright Wacom. All Rights Reserved.

#include "Session/BattleSession.h"

#include "Core/BattleState.h"
#include "Core/BattleResolver.h"
#include "Core/BattleTurnFlow.h"
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
	State->Player.MaxHp     = Params.Character->BaseMaxHp;
	State->Player.CurrentHp = State->Player.MaxHp;
	State->Player.Shield    = 0;
	ReferencedAssets.Add(Params.Character);

	// ---- 卡牌：左手 / 右手 / StarterDeck ----
	// 第一阶段简化：在 Initialize 时把所有卡的身材 MaxHpBonus 累加到玩家 HP。
	// 对齐 Data_Schema_Draft §5.2、FirstPerson_HD2D_Card_Variant "入战属性立即生效"。
	// 严格讲身材应在"卡进入手牌时"触发；第一阶段 StarterDeck 首回合全部能进入手牌，
	// 所以提前在 Initialize 累加与"进入手牌触发"行为等价。
	// 未来若引入"卡留在抽牌堆时不生效"语义，再改为懒触发。
	auto CreateCardInstance = [this](const UCardDefinition* Def, ECardLocation InitialLocation) -> FGuid
	{
		FRuntimeCardInstance Card;
		Card.InstanceId = FGuid::NewGuid();
		Card.Definition = Def;
		Card.Location   = InitialLocation;

		const int32 NewIdx = State->Cards.AllCards.Add(Card);
		State->Cards.CardIndexById.Add(Card.InstanceId, NewIdx);

		if (Def)
		{
			ReferencedAssets.Add(Def);
			const int32 HpBonus = Def->Physique.MaxHpBonus;
			if (HpBonus > 0)
			{
				State->Player.MaxHp     += HpBonus;
				State->Player.CurrentHp += HpBonus;
			}
		}
		return Card.InstanceId;
	};

	if (Params.Character->LeftHandCard)
	{
		// 左右手第一阶段放入手牌锚点容器：S3 起始阶段会按规则生成最终手牌队列。
		// 这里先以 Unknown 位置登记，交给 S3/S4 的 HandZoneService 放入 Hand。
		State->Cards.LeftHandInstanceId  = CreateCardInstance(Params.Character->LeftHandCard, ECardLocation::Unknown);
	}
	if (Params.Character->RightHandCard)
	{
		State->Cards.RightHandInstanceId = CreateCardInstance(Params.Character->RightHandCard, ECardLocation::Unknown);
	}

	for (const TObjectPtr<UCardDefinition>& CardDef : Params.Character->StarterDeck)
	{
		if (!CardDef) { continue; }
		const FGuid CardId = CreateCardInstance(CardDef.Get(), ECardLocation::Draw);
		State->Cards.DrawPile.Add(CardId);
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

	// 起始阶段：抽牌、重置等待值、生成手牌队列（S3 初版，S4 重写）。
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
