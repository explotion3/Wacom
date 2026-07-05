// Copyright Wacom. All Rights Reserved.

#include "Session/BattleSession.h"

#include "Core/BattleState.h"
#include "Commands/KnockdownChoiceAvailability.h"
#include "Events/BattleEventBus.h"
#include "Session/BattleCommandPipeline.h"
#include "Session/BattleInitializer.h"
#include "Session/BattleResultPacketBuilder.h"
#include "Resolution/BattleCardTargetPreviewBuilder.h"
#include "Resolution/BattleTargetResolver.h"
#include "Snapshots/BattleSnapshotBuilder.h"

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
	if (Params.EnemySlots.IsEmpty())
	{
		return FWacomStatus::Fail(EWacomError::InvalidArgument, TEXT("NoEnemy"));
	}

	// 重置状态容器。
	delete State;
	State = new FBattleState();
	EventBus->Reset();
	PresentationJournal.Reset();
	ReferencedAssets.Reset();

	return FBattleInitializer::Initialize(*State, *EventBus, Params, ReferencedAssets);
}

FWacomStatus UBattleSession::SubmitCommand(const FBattleCommand& Command)
{
	if (!State)
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("NotInitialized"));
	}
	PresentationJournal.Reset();
	const FWacomStatus Status =
		FBattleCommandPipeline::Submit(*State, *EventBus, PresentationJournal, Command);
	if (!Status.IsOk())
	{
		PresentationJournal.Reset();
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

FBattlePresentationJournal UBattleSession::ConsumePresentationJournal()
{
	FBattlePresentationJournal Out = MoveTemp(PresentationJournal);
	PresentationJournal.Reset();
	return Out;
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
	if (!State)
	{
		return FBattleResultPacket{};
	}

	return FBattleResultPacketBuilder::Build(*State);
}

FWacomBattleTargetValidationResult UBattleSession::ValidateTargetWithCard(
	const FGuid& CardInstanceId,
	const FWacomInteractionTargetHandle& Target) const
{
	if (!State)
	{
		FWacomBattleTargetValidationResult Result;
		Result.bCanTarget = false;
		Result.RejectReason = EWacomBattleTargetRejectReason::SourceCardInvalid;
		Result.DebugSummary = TEXT("TargetValidation{MissingBattleState}");
		return Result;
	}

	return FBattleTargetResolver::ValidateTargetWithCard(*State, CardInstanceId, Target);
}

FBattleCardTargetPreview UBattleSession::BuildCardTargetPreview(
	const FGuid& CardInstanceId,
	const FWacomInteractionTargetHandle& Target) const
{
	if (!State)
	{
		FBattleCardTargetPreview Preview;
		Preview.Validation.bCanTarget = false;
		Preview.Validation.RejectReason = EWacomBattleTargetRejectReason::SourceCardInvalid;
		Preview.Validation.DebugSummary = TEXT("CardTargetPreview{MissingBattleState}");
		return Preview;
	}

	return FBattleCardTargetPreviewBuilder::Build(*State, CardInstanceId, Target);
}
