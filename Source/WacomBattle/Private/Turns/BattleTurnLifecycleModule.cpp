// Copyright Wacom. All Rights Reserved.

#include "Turns/BattleTurnLifecycleModule.h"

#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Deck/DeckService.h"
#include "Enemy/EnemyPartActionResolver.h"
#include "Events/BattleEventBus.h"
#include "Events/BattleEventHelpers.h"
#include "Hand/BattleCardZoneTransition.h"
#include "Hand/HandZoneService.h"
#include "Presentation/BattlePresentationJournal.h"
#include "Snapshots/BattleSnapshotBuilder.h"
#include "Statuses/BattleStatusSemanticsModule.h"
#include "Types/WacomEnums.h"

namespace
{
	constexpr int32 InitialDrawCount = 5;
	constexpr int32 DefaultCurrentWaitValue = 2;

	void RecordCardCheckpoint(
		FBattlePresentationJournal& PresentationJournal,
		EBattlePresentationCheckpointType Type,
		const FBattleState& State,
		const TArray<FGuid>& CardInstanceIds,
		int32 FirstEventSequence,
		int32 LastEventSequence)
	{
		if (CardInstanceIds.IsEmpty())
		{
			return;
		}

		PresentationJournal.AddCheckpoint(
			Type,
			FBattleSnapshotBuilder::Build(State),
			CardInstanceIds,
			FirstEventSequence,
			LastEventSequence);
	}

	TArray<FGuid> StartPlayerTurn(
		FBattleState& State,
		FBattleEventBus& Events)
	{
		State.Phase = EBattlePhase::TurnStart;

		// Reserved boundary: future start-of-turn expiry and OnTurnStart resolve here,
		// before wait reset and draw. This slice deliberately executes neither.
		State.CurrentWaitValue = DefaultCurrentWaitValue;

		TArray<FGuid> DrawnCardIds;
		const int32 AvailableSlots = FHandZoneService::GetAvailableNormalCardSlots(State);
		FDeckService::DrawCards(
			State,
			FMath::Min(InitialDrawCount, AvailableSlots),
			DrawnCardIds);

		// DrawCards only updates Location; hand queue ownership stays in HandZoneService.
		FHandZoneService::GenerateHandQueueOnTurnStart(State, DrawnCardIds);

		if (!DrawnCardIds.IsEmpty())
		{
			WacomBattleEvents::EmitCardsDrawn(Events, DrawnCardIds);
		}

		FBattleStatusSemanticsModule::MaterializePendingHandAfflictions(State, Events);

		FBattleEvent HandZoneEvent;
		HandZoneEvent.Type = EBattleEventType::HandZoneChanged;
		Events.Emit(HandZoneEvent);

		State.Phase = EBattlePhase::PlayerAction;
		++State.StateVersion;
		return DrawnCardIds;
	}
}

void FBattleTurnLifecycleModule::StartInitialPlayerTurn(
	FBattleState& State,
	FBattleEventBus& Events)
{
	// 兼容现有事件合同：TurnStarted 只在初始化后的首回合发布，且位于初始敌方
	// phase / intent facts 之后、TurnStart 状态修改之前。
	FBattleEvent TurnEvent;
	TurnEvent.Type = EBattleEventType::TurnStarted;
	TurnEvent.Count = State.TurnNumber;
	Events.Emit(TurnEvent);

	StartPlayerTurn(State, Events);
}

void FBattleTurnLifecycleModule::CompleteCurrentTurn(
	FBattleState& State,
	FBattleEventBus& Events,
	FBattlePresentationJournal& PresentationJournal)
{
	State.Phase = EBattlePhase::TurnEnd;

	FBattleEvent TurnEndedEvent;
	TurnEndedEvent.Type = EBattleEventType::TurnEnded;
	TurnEndedEvent.Count = State.TurnNumber;
	Events.Emit(TurnEndedEvent);

	// Reserved boundary: future OnTurnEnd resolves after TurnEnded and before card cleanup.
	FBattleStatusSemanticsModule::ExpireTurnEndCardStatuses(State, Events);
	// PlayedPile natural cleanup is intentionally not a discard event and never runs OnDiscard.
	FDeckService::MovePlayedPileToDiscard(State);

	const int32 DiscardFirstEventSequence = Events.GetNextSequence();
	const FBattleTurnEndHandTransitionResult HandTransition =
		FBattleCardZoneTransition::ResolveTurnEndHand(State, Events);
	RecordCardCheckpoint(
		PresentationJournal,
		EBattlePresentationCheckpointType::TurnEndDiscardResolved,
		State,
		HandTransition.DiscardedCardInstanceIds,
		DiscardFirstEventSequence,
		Events.GetNextSequence() - 1);

	if (!HandTransition.RetainedCardInstanceIds.IsEmpty())
	{
		const int32 RetainFirstEventSequence = Events.GetNextSequence();
		WacomBattleEvents::EmitCardsRetained(
			Events,
			HandTransition.RetainedCardInstanceIds);
		RecordCardCheckpoint(
			PresentationJournal,
			EBattlePresentationCheckpointType::TurnEndRetainResolved,
			State,
			HandTransition.RetainedCardInstanceIds,
			RetainFirstEventSequence,
			Events.GetNextSequence() - 1);
	}

	// Reserved boundary: future until-turn-end expiry resolves after retain facts and before
	// the first BattleEnd gate. This slice deliberately performs no expiry work.
	if (FBattleRules::CheckAndApplyBattleEnd(State, Events))
	{
		return;
	}

	FEnemyPartActionResolver::ResolveEndTurnActions(State, Events);

	if (FBattleRules::CheckAndApplyBattleEnd(State, Events))
	{
		return;
	}

	++State.TurnNumber;
	const int32 DrawFirstEventSequence = Events.GetNextSequence();
	const TArray<FGuid> DrawnCardIds = StartPlayerTurn(State, Events);
	RecordCardCheckpoint(
		PresentationJournal,
		EBattlePresentationCheckpointType::TurnStartDrawResolved,
		State,
		DrawnCardIds,
		DrawFirstEventSequence,
		Events.GetNextSequence() - 1);
}
