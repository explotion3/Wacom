// Copyright Wacom. All Rights Reserved.

#include "Session/BattleSession.h"

#include "Core/BattleState.h"
#include "Core/BattleResolver.h"
#include "Events/BattleEventBus.h"
#include "Snapshots/BattleSnapshotBuilder.h"

#include "Characters/CharacterDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Cards/CardDefinition.h"
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
	State->CharacterDef    = Params.Character;
	State->PlayerMaxHp     = Params.Character->BaseMaxHp;
	State->PlayerCurrentHp = State->PlayerMaxHp;
	State->PlayerShield    = 0;
	ReferencedAssets.Add(Params.Character);

	// ---- 卡牌：左手 / 右手 / StarterDeck ----
	auto CreateCardInstance = [this](const UCardDefinition* Def, ECardLocation InitialLocation) -> FGuid
	{
		FRuntimeCardInstance Card;
		Card.InstanceId = FGuid::NewGuid();
		Card.Definition = Def;
		Card.Location   = InitialLocation;
		State->AllCards.Add(Card);
		if (Def)
		{
			ReferencedAssets.Add(Def);
		}
		return Card.InstanceId;
	};

	if (Params.Character->LeftHandCard)
	{
		// 左右手第一阶段放入手牌锚点容器：S3 起始阶段会按规则生成最终手牌队列。
		// 这里先以 Unknown 位置登记，交给 S3/S4 的 HandZoneService 放入 Hand。
		State->LeftHandInstanceId  = CreateCardInstance(Params.Character->LeftHandCard, ECardLocation::Unknown);
	}
	if (Params.Character->RightHandCard)
	{
		State->RightHandInstanceId = CreateCardInstance(Params.Character->RightHandCard, ECardLocation::Unknown);
	}

	for (const TObjectPtr<UCardDefinition>& CardDef : Params.Character->StarterDeck)
	{
		if (!CardDef) { continue; }
		const FGuid CardId = CreateCardInstance(CardDef.Get(), ECardLocation::Draw);
		State->DrawPile.Add(CardId);
	}

	// ---- 敌人 ----
	State->EnemyDef = Params.Enemy;
	ReferencedAssets.Add(Params.Enemy);

	for (const FEnemyPartSlot& Slot : Params.Enemy->Parts)
	{
		if (!Slot.PartDef) { continue; }
		FRuntimeEnemyPart Part;
		Part.InstanceId         = FGuid::NewGuid();
		Part.Definition         = Slot.PartDef;
		Part.CurrentHp          = Slot.PartDef->MaxHp;
		Part.CurrentIntentIndex = Slot.PartDef->InitialIntentIndex;
		// CurrentInitiative 在 S6/S9 随 IntentSequence 就位后从首个意图填入。
		// 第一阶段 IntentSequence 还未定义，这里保持 0。
		Part.CurrentInitiative  = 0;
		State->EnemyParts.Add(Part);
		ReferencedAssets.Add(Slot.PartDef);
	}

	// ---- 阶段推进 ----
	// Setup -> PlayerAction。
	// S3 会在这里插入 TurnStart 抽牌流程；S2 只把阶段切到 PlayerAction 以便命令可以进入。
	State->Phase        = EBattlePhase::Setup;
	State->TurnNumber   = 1;
	State->CurrentWaitValue = 2;
	State->StateVersion = 0;

	FBattleEvent StartEvent;
	StartEvent.Type = EBattleEventType::BattleStarted;
	EventBus->Emit(StartEvent);

	State->Phase = EBattlePhase::PlayerAction;
	++State->StateVersion;

	FBattleEvent TurnEvent;
	TurnEvent.Type  = EBattleEventType::TurnStarted;
	TurnEvent.Count = State->TurnNumber;
	EventBus->Emit(TurnEvent);

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
