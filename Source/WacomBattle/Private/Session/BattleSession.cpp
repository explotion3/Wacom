// Copyright Wacom. All Rights Reserved.

#include "Session/BattleSession.h"

#include "Cards/CardZoneAggregate.h"
#include "Core/BattleState.h"
#include "Commands/KnockdownChoiceAvailability.h"
#include "Commands/PlayCardEvaluation.h"
#include "Events/BattleEventBus.h"
#include "Passives/PassiveDispatcher.h"
#include "Session/BattleCommandPipeline.h"
#include "Session/BattleInitializer.h"
#include "Session/BattleResultPacketBuilder.h"
#include "Resolution/BattleCardActionPreviewBuilder.h"
#include "Resolution/BattleCardTargetPreviewBuilder.h"
#include "Snapshots/BattleSnapshotBuilder.h"
#include "Snapshots/BattlePileInspectionSnapshotBuilder.h"

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

FBattleInitializationResult UBattleSession::Initialize(const FBattleInitParams& Params)
{
	FBattleInitializationResult Result;
	Result.PostSnapshot = BuildSnapshot();

	if (!Params.Character)
	{
		Result.Status = FWacomStatus::Fail(EWacomError::InvalidArgument, TEXT("NoCharacter"));
		return Result;
	}
	if (Params.EnemySlots.IsEmpty())
	{
		Result.Status = FWacomStatus::Fail(EWacomError::InvalidArgument, TEXT("NoEnemy"));
		return Result;
	}

	FBattleState WorkingState;
	FBattleEventBus WorkingEventBus;
	TArray<TObjectPtr<const UObject>> WorkingReferencedAssets;
	Result.Status = FBattleInitializer::Initialize(
		WorkingState,
		WorkingEventBus,
		Params,
		WorkingReferencedAssets);
	if (!Result.Status.IsOk())
	{
		return Result;
	}

	Result.Events = WorkingEventBus.Consume();
	Result.PostSnapshot = FBattleSnapshotBuilder::Build(WorkingState);
	*State = MoveTemp(WorkingState);
	*EventBus = MoveTemp(WorkingEventBus);
	ReferencedAssets = MoveTemp(WorkingReferencedAssets);
	return Result;
}

FBattleResolution UBattleSession::ResolveCommand(const FBattleCommand& Command)
{
	FBattleResolution Resolution;
	if (!State)
	{
		Resolution.Status = FWacomStatus::Fail(EWacomError::InvalidState, TEXT("NotInitialized"));
		return Resolution;
	}

	Resolution.VersionBefore = State->StateVersion;
	FBattleState WorkingState = *State;
	FBattleEventBus TransactionEvents = EventBus->BeginTransaction();
	FBattlePresentationJournal TransactionJournal;
	Resolution.Status = FBattleCommandPipeline::Submit(
		WorkingState,
		TransactionEvents,
		TransactionJournal,
		Command);
	if (!Resolution.Status.IsOk())
	{
		Resolution.VersionAfter = Resolution.VersionBefore;
		Resolution.PostSnapshot = FBattleSnapshotBuilder::Build(*State);
		return Resolution;
	}

	WorkingState.StateVersion = Resolution.VersionBefore + 1;
	Resolution.VersionAfter = WorkingState.StateVersion;
	Resolution.Events = TransactionEvents.Consume();
	TransactionJournal.AppendDeckStepsFromEvents(Resolution.Events);
	Resolution.PresentationJournal = MoveTemp(TransactionJournal);
	Resolution.PostSnapshot = FBattleSnapshotBuilder::Build(WorkingState);

	*State = MoveTemp(WorkingState);
	EventBus->CommitTransactionSequence(TransactionEvents);
	return Resolution;
}

FBattleSnapshot UBattleSession::BuildSnapshot() const
{
	if (!State)
	{
		return FBattleSnapshot{};
	}
	return FBattleSnapshotBuilder::Build(*State);
}

FBattlePileInspectionSnapshot UBattleSession::BuildPileInspectionSnapshot() const
{
	if (!State)
	{
		return FBattlePileInspectionSnapshot{};
	}
	return FBattlePileInspectionSnapshotBuilder::Build(*State);
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

	return FPlayCardEvaluator::EvaluateTargetProbe(*State, CardInstanceId, Target).Validation;
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

FBattleCardActionPreview UBattleSession::BuildCardActionPreview(
	const FGuid& CardInstanceId,
	const FWacomInteractionTargetHandle& Target) const
{
	if (!State)
	{
		FBattleCardActionPreview Preview;
		Preview.TargetPreview.Validation.bCanTarget = false;
		Preview.TargetPreview.Validation.RejectReason = EWacomBattleTargetRejectReason::SourceCardInvalid;
		Preview.TargetPreview.Validation.DebugSummary = TEXT("CardActionPreview{MissingBattleState}");
		return Preview;
	}

	return FBattleCardActionPreviewBuilder::Build(*State, CardInstanceId, Target);
}

#if WITH_AUTOMATION_TESTS
bool UBattleSession::ValidateCardZoneInvariantsForAutomationTest(FString& OutError) const
{
	return State && FCardZoneAggregate::ValidateInvariants(*State, &OutError);
}

int32 UBattleSession::GetReferencedAssetCountForAutomationTest() const
{
	return ReferencedAssets.Num();
}

bool UBattleSession::ContainsReferencedAssetForAutomationTest(const UObject* Asset) const
{
	return ReferencedAssets.Contains(Asset);
}

int32 UBattleSession::GetNextEventSequenceForAutomationTest() const
{
	return EventBus ? EventBus->GetNextSequence() : INDEX_NONE;
}

int32 UBattleSession::GetRandomCurrentSeedForAutomationTest() const
{
	return State ? State->Rng.GetCurrentSeed() : INDEX_NONE;
}

bool UBattleSession::SetPlayerStatusStacksForAutomationTest(
	const FGameplayTag& Status,
	const int32 Stacks)
{
	if (!State || !Status.IsValid())
	{
		return false;
	}
	if (Stacks > 0)
	{
		State->Player.StatusStacks.FindOrAdd(Status) = Stacks;
	}
	else
	{
		State->Player.StatusStacks.Remove(Status);
	}
	++State->StateVersion;
	return true;
}

bool UBattleSession::SetEnemyPartStatusStacksForAutomationTest(
	const FBattleEnemyPartKey& PartKey,
	const FGameplayTag& Status,
	const int32 Stacks)
{
	if (!State || !PartKey.IsValidKey() || !Status.IsValid())
	{
		return false;
	}
	const int32* PartIndex = State->Enemy.PartIndexByKey.Find(PartKey);
	if (!PartIndex || !State->Enemy.Parts.IsValidIndex(*PartIndex))
	{
		return false;
	}
	TMap<FGameplayTag, int32>& StatusStacks =
		State->Enemy.Parts[*PartIndex].StatusStacks;
	if (Stacks > 0)
	{
		StatusStacks.FindOrAdd(Status) = Stacks;
	}
	else
	{
		StatusStacks.Remove(Status);
	}
	++State->StateVersion;
	return true;
}

bool UBattleSession::SetEnemyPartShieldForAutomationTest(
	const FBattleEnemyPartKey& PartKey,
	const int32 Shield)
{
	if (!State || !PartKey.IsValidKey())
	{
		return false;
	}
	const int32* PartIndex = State->Enemy.PartIndexByKey.Find(PartKey);
	if (!PartIndex || !State->Enemy.Parts.IsValidIndex(*PartIndex))
	{
		return false;
	}
	State->Enemy.Parts[*PartIndex].Shield = FMath::Max(0, Shield);
	++State->StateVersion;
	return true;
}

bool UBattleSession::SetCardStatusStacksForAutomationTest(
	const FGuid& CardInstanceId,
	const FGameplayTag& Status,
	const int32 Stacks)
{
	if (!State || !CardInstanceId.IsValid() || !Status.IsValid())
	{
		return false;
	}
	const int32* CardIndex = State->Cards.CardIndexById.Find(CardInstanceId);
	if (!CardIndex || !State->Cards.AllCards.IsValidIndex(*CardIndex))
	{
		return false;
	}
	TMap<FGameplayTag, int32>& StatusStacks =
		State->Cards.AllCards[*CardIndex].StatusStacks;
	if (Stacks > 0)
	{
		StatusStacks.FindOrAdd(Status) = Stacks;
	}
	else
	{
		StatusStacks.Remove(Status);
	}
	++State->StateVersion;
	return true;
}

bool UBattleSession::SetCardRuntimeCostModifierForAutomationTest(
	const FGuid& CardInstanceId,
	const int32 Modifier)
{
	if (!State || !CardInstanceId.IsValid())
	{
		return false;
	}
	const int32* CardIndex = State->Cards.CardIndexById.Find(CardInstanceId);
	if (!CardIndex || !State->Cards.AllCards.IsValidIndex(*CardIndex))
	{
		return false;
	}
	State->Cards.AllCards[*CardIndex].RuntimeCostModifier = Modifier;
	++State->StateVersion;
	return true;
}

bool UBattleSession::SetCardCriticalChanceBonusForAutomationTest(
	const FGuid& CardInstanceId,
	const int32 BonusPercent)
{
	if (!State || !CardInstanceId.IsValid())
	{
		return false;
	}
	const int32* CardIndex = State->Cards.CardIndexById.Find(CardInstanceId);
	if (!CardIndex || !State->Cards.AllCards.IsValidIndex(*CardIndex))
	{
		return false;
	}
	State->Cards.AllCards[*CardIndex].CriticalChanceBonusPercent =
		FMath::Clamp(BonusPercent, 0, 100);
	++State->StateVersion;
	return true;
}

bool UBattleSession::ResolveSettlementPassivesForAutomationTest()
{
	if (!State || !EventBus)
	{
		return false;
	}
	FPassiveDispatcher::RunOnBattleSettlement(*State, *EventBus);
	++State->StateVersion;
	return true;
}

bool UBattleSession::GetCardRuntimeStateForAutomationTest(
	const FGuid& CardInstanceId,
	FRuntimeCardInstance& OutCard) const
{
	if (!State || !CardInstanceId.IsValid())
	{
		return false;
	}
	const int32* CardIndex = State->Cards.CardIndexById.Find(CardInstanceId);
	if (!CardIndex || !State->Cards.AllCards.IsValidIndex(*CardIndex))
	{
		return false;
	}
	OutCard = State->Cards.AllCards[*CardIndex];
	return true;
}
#endif
