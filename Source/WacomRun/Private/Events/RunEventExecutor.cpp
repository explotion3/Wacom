// Copyright Wacom. All Rights Reserved.

#include "Events/RunEventExecutor.h"

#include "Cards/CardDefinition.h"
#include "Deck/RunDeckRules.h"
#include "Events/RunEventDefinition.h"
#include "Time/RunTimeRules.h"

namespace
{
	namespace DeckReasons = WacomRunDeckOperationReasons;

	bool ChoiceHasRemoveCardEffect(const FWacomRunEventChoiceDefinition& Choice)
	{
		return Choice.Effects.ContainsByPredicate(
			[](const FWacomRunEventEffectDefinition& Effect)
			{
				return Effect.Type == EWacomRunEventEffectType::RemoveCard;
			});
	}

	FName ToRunEventRemoveCardDisabledReason(FName DeckRulesReason)
	{
		if (DeckRulesReason == DeckReasons::CardNotOwned())
		{
			return TEXT("MissingRequiredCard");
		}
		if (DeckRulesReason == DeckReasons::Intrinsic())
		{
			return TEXT("ProtectedCard");
		}
		if (DeckReasons::IsLastCapacityProvider(DeckRulesReason))
		{
			return TEXT("LastCapacityProvider");
		}
		if (DeckRulesReason == DeckReasons::MissingCard())
		{
			return TEXT("MissingCard");
		}
		return DeckRulesReason.IsNone() ? FName(TEXT("EffectFailed")) : DeckRulesReason;
	}
}

FName FRunEventExecutor::ResolvePaymentZoneId(const FWacomRunEventChoiceDefinition& Choice)
{
	if (!Choice.CardPayment.PaymentZoneId.IsNone())
	{
		return Choice.CardPayment.PaymentZoneId;
	}
	if (Choice.ChoiceId.IsNone())
	{
		return NAME_None;
	}
	return FName(*FString::Printf(TEXT("RunEvent.Pay.%s"), *Choice.ChoiceId.ToString()));
}

bool FRunEventExecutor::HasValidPaymentFilter(const FWacomRunEventChoiceDefinition& Choice)
{
	if (!Choice.CardPayment.bRequiresOwnedCardPayment)
	{
		return true;
	}

	for (const TObjectPtr<UCardDefinition>& CardDefinition : Choice.CardPayment.AllowedCardDefinitions)
	{
		if (CardDefinition)
		{
			return true;
		}
	}
	for (const FName& CardId : Choice.CardPayment.AllowedCardIds)
	{
		if (!CardId.IsNone())
		{
			return true;
		}
	}
	return !Choice.CardPayment.RequiredKeywords.IsEmpty()
		|| !Choice.CardPayment.BlockedKeywords.IsEmpty();
}

bool FRunEventExecutor::DoesCardMatchPaymentFilter(
	const FCardInstance& Instance,
	const FWacomRunEventChoiceDefinition& Choice,
	FName& OutDisabledReason)
{
	OutDisabledReason = NAME_None;
	const UCardDefinition* Card = Instance.Definition.Get();
	if (!Card)
	{
		OutDisabledReason = TEXT("MissingCard");
		return false;
	}
	if (!HasValidPaymentFilter(Choice))
	{
		OutDisabledReason = TEXT("MissingPaymentFilter");
		return false;
	}

	bool bHasIdentityFilter = false;
	bool bMatchesIdentityFilter = false;
	for (const TObjectPtr<UCardDefinition>& AllowedDefinition : Choice.CardPayment.AllowedCardDefinitions)
	{
		if (!AllowedDefinition)
		{
			continue;
		}
		bHasIdentityFilter = true;
		if (AllowedDefinition.Get() == Card)
		{
			bMatchesIdentityFilter = true;
			break;
		}
	}
	if (!bMatchesIdentityFilter)
	{
		for (const FName& AllowedCardId : Choice.CardPayment.AllowedCardIds)
		{
			if (AllowedCardId.IsNone())
			{
				continue;
			}
			bHasIdentityFilter = true;
			if (AllowedCardId == Card->CardId)
			{
				bMatchesIdentityFilter = true;
				break;
			}
		}
	}
	if (bHasIdentityFilter && !bMatchesIdentityFilter)
	{
		OutDisabledReason = TEXT("PaymentCardNotAllowed");
		return false;
	}

	if (!Choice.CardPayment.RequiredKeywords.IsEmpty()
		&& !Card->Keywords.HasAll(Choice.CardPayment.RequiredKeywords))
	{
		OutDisabledReason = TEXT("MissingRequiredPaymentKeyword");
		return false;
	}
	if (!Choice.CardPayment.BlockedKeywords.IsEmpty()
		&& Card->Keywords.HasAny(Choice.CardPayment.BlockedKeywords))
	{
		OutDisabledReason = TEXT("BlockedPaymentKeyword");
		return false;
	}
	return true;
}

void FRunEventExecutor::CollectCardPaymentCandidateInstanceIds(
	const FRunState& State,
	const FWacomRunEventChoiceDefinition& Choice,
	TArray<FGuid>& OutInstanceIds,
	FName& OutDisabledReason)
{
	OutInstanceIds.Reset();
	OutDisabledReason = NAME_None;
	if (!Choice.CardPayment.bRequiresOwnedCardPayment)
	{
		return;
	}
	if (!HasValidPaymentFilter(Choice))
	{
		OutDisabledReason = TEXT("MissingPaymentFilter");
		return;
	}

	bool bSawFilterMatch = false;
	FName FirstValidationFailure = NAME_None;
	auto VisitInstance = [&State, &Choice, &OutInstanceIds, &OutDisabledReason, &bSawFilterMatch, &FirstValidationFailure](
		const FCardInstance& Instance)
	{
		FName MatchFailure = NAME_None;
		if (!FRunEventExecutor::DoesCardMatchPaymentFilter(Instance, Choice, MatchFailure))
		{
			if (OutDisabledReason.IsNone() && MatchFailure != TEXT("PaymentCardNotAllowed"))
			{
				OutDisabledReason = MatchFailure;
			}
			return;
		}

		bSawFilterMatch = true;
		const FRunDeckOperationValidation Validation =
			FRunDeckRules::ValidatePermanentRemoveInstance(State, Instance.InstanceId);
		if (Validation.bCanExecute)
		{
			OutInstanceIds.Add(Instance.InstanceId);
		}
		else if (FirstValidationFailure.IsNone())
		{
			FirstValidationFailure = ToRunEventRemoveCardDisabledReason(Validation.DisabledReason);
		}
	};

	for (const FCardInstance& Instance : State.Backpack)
	{
		VisitInstance(Instance);
	}
	for (const FCardInstance& Instance : State.BattleDeck)
	{
		VisitInstance(Instance);
	}
	for (const FCardInstance& Instance : State.BurdenZone)
	{
		VisitInstance(Instance);
	}
	for (const FSpecialZone& SpecialZone : State.SpecialZones)
	{
		for (const FCardInstance& Instance : SpecialZone.Cards)
		{
			VisitInstance(Instance);
		}
	}

	if (!OutInstanceIds.IsEmpty())
	{
		OutDisabledReason = NAME_None;
	}
	else if (bSawFilterMatch && !FirstValidationFailure.IsNone())
	{
		OutDisabledReason = FirstValidationFailure;
	}
	else if (OutDisabledReason.IsNone())
	{
		OutDisabledReason = TEXT("MissingRequiredCard");
	}
}

const FWacomRunEventNodeDefinition* FRunEventExecutor::FindNode(const UWacomRunEventDefinition* EventDefinition, FName NodeId)
{
	if (!EventDefinition || NodeId.IsNone())
	{
		return nullptr;
	}

	return EventDefinition->Nodes.FindByPredicate(
		[NodeId](const FWacomRunEventNodeDefinition& Node)
		{
			return Node.NodeId == NodeId;
		});
}

const FWacomRunEventChoiceDefinition* FRunEventExecutor::FindChoice(const FWacomRunEventNodeDefinition* Node, FName ChoiceId)
{
	if (!Node || ChoiceId.IsNone())
	{
		return nullptr;
	}

	return Node->Choices.FindByPredicate(
		[ChoiceId](const FWacomRunEventChoiceDefinition& Choice)
		{
			return Choice.ChoiceId == ChoiceId;
		});
}

bool FRunEventExecutor::BeginEvent(FRunState& State, FName PersistentId, UWacomRunEventDefinition* EventDefinition)
{
	if (PersistentId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunSession] BeginRunEvent: PersistentId 为 None，拒绝"));
		return false;
	}
	if (!EventDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunSession] BeginRunEvent: EventDefinition 为空，拒绝"));
		return false;
	}
	if (!FindNode(EventDefinition, EventDefinition->StartNodeId))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] BeginRunEvent: Event=%s StartNodeId=%s 无效"),
			*GetNameSafe(EventDefinition),
			*EventDefinition->StartNodeId.ToString());
		return false;
	}

	FRunEventState& EventState = State.RunEventStates.FindOrAdd(PersistentId);
	if (EventState.bCompleted)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[RunSession] BeginRunEvent: PersistentId=%s 已完成，拒绝重复打开"),
			*PersistentId.ToString());
		return false;
	}

	if (EventState.CurrentNodeId.IsNone() || !FindNode(EventDefinition, EventState.CurrentNodeId))
	{
		EventState.CurrentNodeId = EventDefinition->StartNodeId;
	}

	State.ActiveRunEventId = PersistentId;
	State.ActiveRunEventDefinition = EventDefinition;
	return true;
}

bool FRunEventExecutor::IsEventCompleted(const FRunState& State, FName PersistentId)
{
	if (const FRunEventState* EventState = State.RunEventStates.Find(PersistentId))
	{
		return EventState->bCompleted;
	}
	return false;
}

bool FRunEventExecutor::IsRunFlagSet(const FRunState& State, FName FlagId)
{
	return !FlagId.IsNone() && State.RunFlags.Contains(FlagId);
}

FRunEventSnapshot FRunEventExecutor::BuildSnapshot(const FRunState& State)
{
	FRunEventSnapshot Snapshot;
	Snapshot.PersistentId = State.ActiveRunEventId;
	Snapshot.bIsActive = !State.ActiveRunEventId.IsNone() && State.ActiveRunEventDefinition;

	const UWacomRunEventDefinition* EventDefinition = State.ActiveRunEventDefinition;
	if (!Snapshot.bIsActive || !EventDefinition)
	{
		return Snapshot;
	}

	Snapshot.EventId = EventDefinition->EventId;
	const FRunEventState* EventState = State.RunEventStates.Find(State.ActiveRunEventId);
	Snapshot.bCompleted = EventState ? EventState->bCompleted : false;
	Snapshot.CurrentNodeId = EventState ? EventState->CurrentNodeId : EventDefinition->StartNodeId;

	const FWacomRunEventNodeDefinition* Node = FindNode(EventDefinition, Snapshot.CurrentNodeId);
	if (!Node)
	{
		return Snapshot;
	}

	Snapshot.TitleText = Node->TitleText.IsEmpty() ? EventDefinition->DisplayName : Node->TitleText;
	Snapshot.BodyText = Node->BodyText;
	Snapshot.Choices.Reserve(Node->Choices.Num());
	for (const FWacomRunEventChoiceDefinition& Choice : Node->Choices)
	{
		FRunEventChoiceSnapshot ChoiceSnapshot;
		ChoiceSnapshot.ChoiceId = Choice.ChoiceId;
		ChoiceSnapshot.LabelText = Choice.LabelText;
		BuildConsequenceSnapshotsForChoice(EventDefinition, Choice, ChoiceSnapshot.Consequences);
		for (const FWacomRunEventConditionDefinition& Condition : Choice.Conditions)
		{
			FRunEventChoiceRequirementSnapshot Requirement =
				BuildRequirementSnapshotForCondition(State, Condition);
			if (Requirement.Kind != ERunEventChoiceRequirementKind::None)
			{
				ChoiceSnapshot.Requirements.Add(MoveTemp(Requirement));
			}
		}
		ChoiceSnapshot.bAvailable = IsChoiceAvailable(State, Choice, ChoiceSnapshot.DisabledReason);
		ChoiceSnapshot.bRequiresOwnedCardPayment = Choice.CardPayment.bRequiresOwnedCardPayment;
		if (Choice.CardPayment.bRequiresOwnedCardPayment)
		{
			ChoiceSnapshot.PaymentZoneId = ResolvePaymentZoneId(Choice);
			CollectCardPaymentCandidateInstanceIds(
				State,
				Choice,
				ChoiceSnapshot.PaymentCandidateInstanceIds,
				ChoiceSnapshot.PaymentDisabledReason);
			ChoiceSnapshot.PaymentCandidateCount = ChoiceSnapshot.PaymentCandidateInstanceIds.Num();
			if (ChoiceSnapshot.PaymentCandidateCount <= 0)
			{
				ChoiceSnapshot.bAvailable = false;
				if (ChoiceSnapshot.DisabledReason.IsNone())
				{
					ChoiceSnapshot.DisabledReason = ChoiceSnapshot.PaymentDisabledReason.IsNone()
						? FName(TEXT("MissingRequiredCard"))
						: ChoiceSnapshot.PaymentDisabledReason;
				}
			}

			FRunEventChoiceRequirementSnapshot PaymentRequirement;
			PaymentRequirement.Kind = ERunEventChoiceRequirementKind::CardPayment;
			PaymentRequirement.bSatisfied = ChoiceSnapshot.PaymentCandidateCount > 0;
			PaymentRequirement.DisabledReason = ChoiceSnapshot.PaymentDisabledReason;
			if (!PaymentRequirement.bSatisfied && PaymentRequirement.DisabledReason.IsNone())
			{
				PaymentRequirement.DisabledReason = TEXT("MissingRequiredCard");
			}
			PaymentRequirement.PaymentCandidateCount = ChoiceSnapshot.PaymentCandidateCount;
			ChoiceSnapshot.Requirements.Add(MoveTemp(PaymentRequirement));
		}
		Snapshot.Choices.Add(MoveTemp(ChoiceSnapshot));
	}

	return Snapshot;
}

FRunEventChoiceResult FRunEventExecutor::ChooseOption(FRunState& State, FName ChoiceId)
{
	return ChooseOptionInternal(State, ChoiceId, TOptional<FGuid>());
}

FRunEventChoiceResult FRunEventExecutor::ChooseOptionWithPaidCard(
	FRunState& State,
	FName ChoiceId,
	FGuid PaidCardInstanceId)
{
	return ChooseOptionInternal(State, ChoiceId, PaidCardInstanceId);
}

FRunDeckOperationValidation FRunEventExecutor::ValidateChoiceCardPayment(
	const FRunState& State,
	FName ChoiceId,
	FGuid PaidCardInstanceId)
{
	FRunDeckOperationValidation Result;
	if (!PaidCardInstanceId.IsValid())
	{
		Result.DisabledReason = TEXT("MissingPaidCard");
		return Result;
	}
	if (State.ActiveRunEventId.IsNone() || !State.ActiveRunEventDefinition)
	{
		Result.DisabledReason = TEXT("NoActiveEvent");
		return Result;
	}

	const FRunEventState* EventState = State.RunEventStates.Find(State.ActiveRunEventId);
	if (!EventState || EventState->bCompleted)
	{
		Result.DisabledReason = TEXT("InvalidEventState");
		return Result;
	}

	const FWacomRunEventNodeDefinition* Node = FindNode(State.ActiveRunEventDefinition, EventState->CurrentNodeId);
	const FWacomRunEventChoiceDefinition* Choice = FindChoice(Node, ChoiceId);
	if (!Choice)
	{
		Result.DisabledReason = TEXT("ChoiceNotFound");
		return Result;
	}
	if (!Choice->CardPayment.bRequiresOwnedCardPayment)
	{
		Result.DisabledReason = TEXT("PaymentNotRequired");
		return Result;
	}
	if (ChoiceHasRemoveCardEffect(*Choice))
	{
		Result.DisabledReason = TEXT("PaymentChoiceHasRemoveCardEffect");
		return Result;
	}

	FName DisabledReason = NAME_None;
	if (!IsChoiceAvailable(State, *Choice, DisabledReason))
	{
		Result.DisabledReason = DisabledReason.IsNone() ? FName(TEXT("ChoiceUnavailable")) : DisabledReason;
		return Result;
	}

	FRunOwnedCardLocation Location;
	if (!FRunDeckRules::FindOwnedCardInstance(State, PaidCardInstanceId, Location))
	{
		Result.DisabledReason = DeckReasons::CardNotOwned();
		return Result;
	}

	FName MatchDisabledReason = NAME_None;
	if (!DoesCardMatchPaymentFilter(Location.Instance, *Choice, MatchDisabledReason))
	{
		Result.DisabledReason = MatchDisabledReason.IsNone()
			? FName(TEXT("PaymentCardNotAllowed"))
			: MatchDisabledReason;
		return Result;
	}

	Result = FRunDeckRules::ValidatePermanentRemoveInstance(State, PaidCardInstanceId);
	if (!Result.bCanExecute)
	{
		Result.DisabledReason = ToRunEventRemoveCardDisabledReason(Result.DisabledReason);
	}
	return Result;
}

FRunEventChoiceResult FRunEventExecutor::ChooseOptionInternal(
	FRunState& State,
	FName ChoiceId,
	TOptional<FGuid> PaidCardInstanceId)
{
	FRunEventChoiceResult Result;
	Result.ChoiceId = ChoiceId;
	if (PaidCardInstanceId.IsSet())
	{
		Result.PaidCardInstanceId = PaidCardInstanceId.GetValue();
	}

	if (State.ActiveRunEventId.IsNone() || !State.ActiveRunEventDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunSession] ChooseRunEventOption: 当前没有 active event，拒绝"));
		Result.DisabledReason = TEXT("NoActiveEvent");
		return Result;
	}

	FRunEventState* EventState = State.RunEventStates.Find(State.ActiveRunEventId);
	if (!EventState || EventState->bCompleted)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunSession] ChooseRunEventOption: 事件状态无效或已完成，拒绝"));
		Result.DisabledReason = TEXT("InvalidEventState");
		return Result;
	}

	const FWacomRunEventNodeDefinition* Node = FindNode(State.ActiveRunEventDefinition, EventState->CurrentNodeId);
	const FWacomRunEventChoiceDefinition* Choice = FindChoice(Node, ChoiceId);
	if (!Choice)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] ChooseRunEventOption: 找不到 ChoiceId=%s"),
			*ChoiceId.ToString());
		Result.DisabledReason = TEXT("ChoiceNotFound");
		return Result;
	}

	if (Choice->CardPayment.bRequiresOwnedCardPayment && !PaidCardInstanceId.IsSet())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] ChooseRunEventOption: ChoiceId=%s 需要卡牌支付，普通点击拒绝"),
			*ChoiceId.ToString());
		Result.DisabledReason = TEXT("RequiresCardPayment");
		return Result;
	}
	if (!Choice->CardPayment.bRequiresOwnedCardPayment && PaidCardInstanceId.IsSet())
	{
		Result.DisabledReason = TEXT("PaymentNotRequired");
		return Result;
	}
	if (Choice->CardPayment.bRequiresOwnedCardPayment && ChoiceHasRemoveCardEffect(*Choice))
	{
		Result.DisabledReason = TEXT("PaymentChoiceHasRemoveCardEffect");
		return Result;
	}

	FName DisabledReason = NAME_None;
	if (!IsChoiceAvailable(State, *Choice, DisabledReason))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] ChooseRunEventOption: ChoiceId=%s 不可用 Reason=%s"),
			*ChoiceId.ToString(),
			*DisabledReason.ToString());
		Result.DisabledReason = DisabledReason;
		return Result;
	}

	const FName ActiveRunEventId = State.ActiveRunEventId;
	const UWacomRunEventDefinition* ActiveRunEventDefinition = State.ActiveRunEventDefinition;
	const FName PreviousNodeId = EventState->CurrentNodeId;
	FRunState WorkingState = State;
	TArray<FRunEventChoiceEffectResult> PendingEffectResults;
	UCardDefinition* PaidCardDefinitionForResult = nullptr;

	if (PaidCardInstanceId.IsSet())
	{
		const FRunDeckOperationValidation PaymentValidation =
			ValidateChoiceCardPayment(WorkingState, ChoiceId, PaidCardInstanceId.GetValue());
		if (!PaymentValidation.bCanExecute)
		{
			Result.DisabledReason = PaymentValidation.DisabledReason.IsNone()
				? FName(TEXT("PaymentRejected"))
				: PaymentValidation.DisabledReason;
			return Result;
		}

		FRunOwnedCardLocation PaidCardLocation;
		if (FRunDeckRules::FindOwnedCardInstance(WorkingState, PaidCardInstanceId.GetValue(), PaidCardLocation))
		{
			PaidCardDefinitionForResult = PaidCardLocation.Instance.Definition;
		}

		FName PaymentRemoveDisabledReason = NAME_None;
		if (!FRunDeckRules::PermanentRemoveOwnedInstance(
			WorkingState,
			PaidCardInstanceId.GetValue(),
			&PaymentRemoveDisabledReason))
		{
			Result.DisabledReason = ToRunEventRemoveCardDisabledReason(PaymentRemoveDisabledReason);
			return Result;
		}
	}

	if (!ApplyChoiceEffects(WorkingState, *Choice, &PendingEffectResults, &Result.DisabledReason))
	{
		if (Result.DisabledReason.IsNone())
		{
			Result.DisabledReason = TEXT("EffectFailed");
		}
		return Result;
	}

	FRunEventState* WorkingEventState = WorkingState.RunEventStates.Find(ActiveRunEventId);
	if (!WorkingEventState)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunSession] ChooseRunEventOption: 工作事件状态无效，拒绝提交"));
		Result.DisabledReason = TEXT("InvalidEventState");
		return Result;
	}

	if (Choice->bMarkEventCompleted)
	{
		WorkingEventState->bCompleted = true;
	}

	if (!Choice->NextNodeId.IsNone())
	{
		if (!FindNode(WorkingState.ActiveRunEventDefinition, Choice->NextNodeId))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[RunSession] ChooseRunEventOption: NextNodeId=%s 无效，保持当前节点"),
				*Choice->NextNodeId.ToString());
		}
		else
		{
			WorkingEventState->CurrentNodeId = Choice->NextNodeId;
		}
	}

	if (Choice->bCloseEventAfterResolve || WorkingEventState->bCompleted)
	{
		WorkingState.ActiveRunEventId = NAME_None;
		WorkingState.ActiveRunEventDefinition = nullptr;
	}

	const FName ResolvedNodeId = WorkingEventState->CurrentNodeId;
	const FWacomRunEventNodeDefinition* ResolvedNode =
		FindNode(ActiveRunEventDefinition, ResolvedNodeId);
	const bool bEventClosedAfterResolve = WorkingState.ActiveRunEventId.IsNone();
	const bool bEventCompletedAfterResolve = WorkingEventState->bCompleted;

	State = MoveTemp(WorkingState);
	Result.PaidCardDefinition = PaidCardDefinitionForResult;
	Result.PreviousNodeId = PreviousNodeId;
	Result.ResolvedNodeId = ResolvedNodeId;
	Result.ResolvedNodeTitleText = ResolvedNode ? ResolvedNode->TitleText : FText::GetEmpty();
	Result.bNodeChanged = PreviousNodeId != ResolvedNodeId;
	Result.bEventClosedAfterResolve = bEventClosedAfterResolve;
	Result.bEventCompletedAfterResolve = bEventCompletedAfterResolve;
	Result.EffectResults = MoveTemp(PendingEffectResults);
	Result.bSucceeded = true;
	return Result;
}

bool FRunEventExecutor::TryResolvePressureType(FName PressureTypeId, EWacomPressureType& OutType)
{
	if (PressureTypeId == TEXT("Hunger"))     { OutType = EWacomPressureType::Hunger; return true; }
	if (PressureTypeId == TEXT("Wound"))      { OutType = EWacomPressureType::Wound; return true; }
	if (PressureTypeId == TEXT("Fatigue"))    { OutType = EWacomPressureType::Fatigue; return true; }
	if (PressureTypeId == TEXT("Burden"))     { OutType = EWacomPressureType::Burden; return true; }
	if (PressureTypeId == TEXT("Decay"))      { OutType = EWacomPressureType::Decay; return true; }
	if (PressureTypeId == TEXT("Misdeed"))    { OutType = EWacomPressureType::Misdeed; return true; }
	if (PressureTypeId == TEXT("Bloodlust"))  { OutType = EWacomPressureType::Bloodlust; return true; }
	if (PressureTypeId == TEXT("Disability")) { OutType = EWacomPressureType::Disability; return true; }
	return false;
}

FRunEventChoiceRequirementSnapshot FRunEventExecutor::BuildRequirementSnapshotForCondition(
	const FRunState& State,
	const FWacomRunEventConditionDefinition& Condition)
{
	FRunEventChoiceRequirementSnapshot Requirement;
	Requirement.RequiredValue = Condition.Value;
	Requirement.CardDefinition = Condition.CardDefinition;
	Requirement.TargetPersistentId = Condition.TargetPersistentId;
	Requirement.FlagId = Condition.FlagId;

	switch (Condition.Type)
	{
	case EWacomRunEventConditionType::None:
		Requirement.Kind = ERunEventChoiceRequirementKind::None;
		return Requirement;
	case EWacomRunEventConditionType::MinGold:
		Requirement.Kind = ERunEventChoiceRequirementKind::MinGold;
		Requirement.CurrentValue = State.Gold;
		Requirement.bSatisfied = State.Gold >= Condition.Value;
		Requirement.DisabledReason = Requirement.bSatisfied ? NAME_None : FName(TEXT("InsufficientGold"));
		return Requirement;
	case EWacomRunEventConditionType::MinNodeCount:
		Requirement.Kind = ERunEventChoiceRequirementKind::MinNodeCount;
		Requirement.CurrentValue = State.RemainingNodeCount;
		Requirement.bSatisfied = State.RemainingNodeCount >= Condition.Value;
		Requirement.DisabledReason = Requirement.bSatisfied ? NAME_None : FName(TEXT("InsufficientNode"));
		return Requirement;
	case EWacomRunEventConditionType::MaxPressure:
	{
		Requirement.Kind = ERunEventChoiceRequirementKind::MaxPressure;
		EWacomPressureType PressureType = EWacomPressureType::Count;
		if (!TryResolvePressureType(Condition.PressureType, PressureType))
		{
			Requirement.bSatisfied = false;
			Requirement.DisabledReason = TEXT("InvalidPressureType");
			return Requirement;
		}
		Requirement.PressureType = PressureType;
		Requirement.CurrentValue = State.Pressure.Get(PressureType);
		Requirement.bSatisfied = Requirement.CurrentValue <= Condition.Value;
		Requirement.DisabledReason = Requirement.bSatisfied ? NAME_None : FName(TEXT("PressureTooHigh"));
		return Requirement;
	}
	case EWacomRunEventConditionType::HasCard:
		Requirement.Kind = ERunEventChoiceRequirementKind::HasCard;
		if (!Condition.CardDefinition)
		{
			Requirement.bSatisfied = false;
			Requirement.DisabledReason = TEXT("MissingCard");
			return Requirement;
		}
		Requirement.bSatisfied = FRunDeckRules::DoesRunOwnCardDefinition(State, Condition.CardDefinition.Get());
		Requirement.DisabledReason = Requirement.bSatisfied ? NAME_None : FName(TEXT("MissingRequiredCard"));
		return Requirement;
	case EWacomRunEventConditionType::MissingCard:
		Requirement.Kind = ERunEventChoiceRequirementKind::MissingCard;
		if (!Condition.CardDefinition)
		{
			Requirement.bSatisfied = false;
			Requirement.DisabledReason = TEXT("MissingCard");
			return Requirement;
		}
		Requirement.bSatisfied = !FRunDeckRules::DoesRunOwnCardDefinition(State, Condition.CardDefinition.Get());
		Requirement.DisabledReason = Requirement.bSatisfied ? NAME_None : FName(TEXT("AlreadyHasCard"));
		return Requirement;
	case EWacomRunEventConditionType::EventCompleted:
		Requirement.Kind = ERunEventChoiceRequirementKind::EventCompleted;
		if (Condition.TargetPersistentId.IsNone())
		{
			Requirement.bSatisfied = false;
			Requirement.DisabledReason = TEXT("MissingTargetPersistentId");
			return Requirement;
		}
		Requirement.bSatisfied = IsEventCompleted(State, Condition.TargetPersistentId);
		Requirement.DisabledReason = Requirement.bSatisfied ? NAME_None : FName(TEXT("RequiredEventNotCompleted"));
		return Requirement;
	case EWacomRunEventConditionType::EventNotCompleted:
		Requirement.Kind = ERunEventChoiceRequirementKind::EventNotCompleted;
		if (Condition.TargetPersistentId.IsNone())
		{
			Requirement.bSatisfied = false;
			Requirement.DisabledReason = TEXT("MissingTargetPersistentId");
			return Requirement;
		}
		Requirement.bSatisfied = !IsEventCompleted(State, Condition.TargetPersistentId);
		Requirement.DisabledReason = Requirement.bSatisfied ? NAME_None : FName(TEXT("RequiredEventAlreadyCompleted"));
		return Requirement;
	case EWacomRunEventConditionType::RunFlagSet:
		Requirement.Kind = ERunEventChoiceRequirementKind::RunFlagSet;
		if (Condition.FlagId.IsNone())
		{
			Requirement.bSatisfied = false;
			Requirement.DisabledReason = TEXT("MissingRunFlagId");
			return Requirement;
		}
		Requirement.bSatisfied = IsRunFlagSet(State, Condition.FlagId);
		Requirement.DisabledReason = Requirement.bSatisfied ? NAME_None : FName(TEXT("RequiredRunFlagMissing"));
		return Requirement;
	case EWacomRunEventConditionType::RunFlagNotSet:
		Requirement.Kind = ERunEventChoiceRequirementKind::RunFlagNotSet;
		if (Condition.FlagId.IsNone())
		{
			Requirement.bSatisfied = false;
			Requirement.DisabledReason = TEXT("MissingRunFlagId");
			return Requirement;
		}
		Requirement.bSatisfied = !IsRunFlagSet(State, Condition.FlagId);
		Requirement.DisabledReason = Requirement.bSatisfied ? NAME_None : FName(TEXT("BlockedRunFlagSet"));
		return Requirement;
	default:
		Requirement.Kind = ERunEventChoiceRequirementKind::None;
		Requirement.bSatisfied = false;
		Requirement.DisabledReason = TEXT("UnknownCondition");
		return Requirement;
	}
}

void FRunEventExecutor::BuildConsequenceSnapshotsForChoice(
	const UWacomRunEventDefinition* EventDefinition,
	const FWacomRunEventChoiceDefinition& Choice,
	TArray<FRunEventChoiceConsequenceSnapshot>& OutConsequences)
{
	for (const FWacomRunEventEffectDefinition& Effect : Choice.Effects)
	{
		if (Effect.Type == EWacomRunEventEffectType::None)
		{
			continue;
		}

		FRunEventChoiceConsequenceSnapshot Consequence;
		Consequence.Kind = ERunEventChoiceConsequenceKind::Effect;
		Consequence.EffectType = Effect.Type;
		Consequence.CardDefinition = Effect.CardDefinition;
		Consequence.Amount = Effect.Value;
		Consequence.TargetPersistentId = Effect.TargetPersistentId;
		Consequence.FlagId = Effect.FlagId;
		if (Effect.Type == EWacomRunEventEffectType::AddPressure)
		{
			EWacomPressureType PressureType = EWacomPressureType::Count;
			if (TryResolvePressureType(Effect.PressureType, PressureType))
			{
				Consequence.PressureType = PressureType;
			}
		}
		OutConsequences.Add(MoveTemp(Consequence));
	}

	if (Choice.bCloseEventAfterResolve || Choice.bMarkEventCompleted)
	{
		FRunEventChoiceConsequenceSnapshot Consequence;
		Consequence.Kind = ERunEventChoiceConsequenceKind::EventEnds;
		OutConsequences.Add(MoveTemp(Consequence));
		return;
	}

	if (!Choice.NextNodeId.IsNone())
	{
		const FWacomRunEventNodeDefinition* NextNode = FindNode(EventDefinition, Choice.NextNodeId);
		if (!NextNode)
		{
			return;
		}

		FRunEventChoiceConsequenceSnapshot Consequence;
		Consequence.Kind = ERunEventChoiceConsequenceKind::NodeTransition;
		Consequence.ResolvedNodeId = Choice.NextNodeId;
		Consequence.ResolvedNodeTitleText = NextNode->TitleText;
		OutConsequences.Add(MoveTemp(Consequence));
	}
}

bool FRunEventExecutor::IsChoiceAvailable(const FRunState& State, const FWacomRunEventChoiceDefinition& Choice, FName& OutDisabledReason)
{
	OutDisabledReason = NAME_None;
	for (const FWacomRunEventConditionDefinition& Condition : Choice.Conditions)
	{
		switch (Condition.Type)
		{
		case EWacomRunEventConditionType::None:
			break;
		case EWacomRunEventConditionType::MinGold:
			if (State.Gold < Condition.Value)
			{
				OutDisabledReason = TEXT("InsufficientGold");
				return false;
			}
			break;
		case EWacomRunEventConditionType::MinNodeCount:
			if (State.RemainingNodeCount < Condition.Value)
			{
				OutDisabledReason = TEXT("InsufficientNode");
				return false;
			}
			break;
		case EWacomRunEventConditionType::MaxPressure:
		{
			EWacomPressureType PressureType = EWacomPressureType::Count;
			if (!TryResolvePressureType(Condition.PressureType, PressureType))
			{
				OutDisabledReason = TEXT("InvalidPressureType");
				return false;
			}
			if (State.Pressure.Get(PressureType) > Condition.Value)
			{
				OutDisabledReason = TEXT("PressureTooHigh");
				return false;
			}
			break;
		}
		case EWacomRunEventConditionType::HasCard:
			if (!Condition.CardDefinition)
			{
				OutDisabledReason = TEXT("MissingCard");
				return false;
			}
			if (!FRunDeckRules::DoesRunOwnCardDefinition(State, Condition.CardDefinition.Get()))
			{
				OutDisabledReason = TEXT("MissingRequiredCard");
				return false;
			}
			break;
		case EWacomRunEventConditionType::MissingCard:
			if (!Condition.CardDefinition)
			{
				OutDisabledReason = TEXT("MissingCard");
				return false;
			}
			if (FRunDeckRules::DoesRunOwnCardDefinition(State, Condition.CardDefinition.Get()))
			{
				OutDisabledReason = TEXT("AlreadyHasCard");
				return false;
			}
			break;
		case EWacomRunEventConditionType::EventCompleted:
			if (Condition.TargetPersistentId.IsNone())
			{
				OutDisabledReason = TEXT("MissingTargetPersistentId");
				return false;
			}
			if (!IsEventCompleted(State, Condition.TargetPersistentId))
			{
				OutDisabledReason = TEXT("RequiredEventNotCompleted");
				return false;
			}
			break;
		case EWacomRunEventConditionType::EventNotCompleted:
			if (Condition.TargetPersistentId.IsNone())
			{
				OutDisabledReason = TEXT("MissingTargetPersistentId");
				return false;
			}
			if (IsEventCompleted(State, Condition.TargetPersistentId))
			{
				OutDisabledReason = TEXT("RequiredEventAlreadyCompleted");
				return false;
			}
			break;
		case EWacomRunEventConditionType::RunFlagSet:
			if (Condition.FlagId.IsNone())
			{
				OutDisabledReason = TEXT("MissingRunFlagId");
				return false;
			}
			if (!IsRunFlagSet(State, Condition.FlagId))
			{
				OutDisabledReason = TEXT("RequiredRunFlagMissing");
				return false;
			}
			break;
		case EWacomRunEventConditionType::RunFlagNotSet:
			if (Condition.FlagId.IsNone())
			{
				OutDisabledReason = TEXT("MissingRunFlagId");
				return false;
			}
			if (IsRunFlagSet(State, Condition.FlagId))
			{
				OutDisabledReason = TEXT("BlockedRunFlagSet");
				return false;
			}
			break;
		default:
			OutDisabledReason = TEXT("UnknownCondition");
			return false;
		}
	}
	return true;
}

bool FRunEventExecutor::ApplyChoiceEffects(FRunState& State, const FWacomRunEventChoiceDefinition& Choice, TArray<FRunEventChoiceEffectResult>* OutEffectResults, FName* OutDisabledReason)
{
	for (const FWacomRunEventEffectDefinition& Effect : Choice.Effects)
	{
		FRunEventChoiceEffectResult EffectResult;
		EffectResult.EffectType = Effect.Type;
		EffectResult.CardDefinition = Effect.CardDefinition;
		EffectResult.Amount = Effect.Value;
		EffectResult.FlagId = Effect.FlagId;

		switch (Effect.Type)
		{
		case EWacomRunEventEffectType::None:
			EffectResult.bApplied = true;
			break;
		case EWacomRunEventEffectType::GainCard:
			if (!Effect.CardDefinition)
			{
				UE_LOG(LogTemp, Warning, TEXT("[RunSession] ApplyRunEventChoiceEffects: GainCard 缺少 CardDefinition"));
				if (OutDisabledReason)
				{
					*OutDisabledReason = TEXT("MissingCard");
				}
				return false;
			}
			if (!AcquireCard(State, Effect.CardDefinition.Get()))
			{
				if (OutDisabledReason)
				{
					*OutDisabledReason = TEXT("EffectFailed");
				}
				return false;
			}
			EffectResult.bApplied = true;
			break;
		case EWacomRunEventEffectType::AddGold:
		{
			const int32 GoldBefore = State.Gold;
			State.Gold = FMath::Max(0, State.Gold + Effect.Value);
			EffectResult.ActualDelta = State.Gold - GoldBefore;
			EffectResult.bApplied = true;
			break;
		}
		case EWacomRunEventEffectType::AddPressure:
		{
			EWacomPressureType PressureType = EWacomPressureType::Count;
			if (!TryResolvePressureType(Effect.PressureType, PressureType))
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[RunSession] ApplyRunEventChoiceEffects: 无效 PressureType=%s"),
					*Effect.PressureType.ToString());
				if (OutDisabledReason)
				{
					*OutDisabledReason = TEXT("InvalidPressureType");
				}
				return false;
			}
			const int32 PressureBefore = State.Pressure.Get(PressureType);
			State.Pressure.Add(PressureType, Effect.Value);
			EffectResult.PressureType = PressureType;
			EffectResult.ActualDelta = State.Pressure.Get(PressureType) - PressureBefore;
			EffectResult.bApplied = true;
			break;
		}
		case EWacomRunEventEffectType::ConsumeNode:
		{
			const int32 Count = FMath::Max(0, Effect.Value);
			int32 ConsumedNodeCount = 0;
			const bool bHadEnoughNode = FRunTimeRules::ConsumeNode(State, Count, &ConsumedNodeCount);
			EffectResult.ActualDelta = -ConsumedNodeCount;
			if (Count > 0)
			{
				if (!bHadEnoughNode)
				{
					UE_LOG(LogTemp, Warning,
						TEXT("[RunSession] ApplyRunEventChoiceEffects: 节点不足但已推进时段 Count=%d"),
						Count);
				}
			}
			EffectResult.bApplied = true;
			break;
		}
		case EWacomRunEventEffectType::RemoveCard:
		{
			FRunDeckOperationValidation Validation = FRunDeckRules::ValidatePermanentRemoveCard(State, Effect.CardDefinition.Get());
			if (!Validation.bCanExecute)
			{
				if (OutDisabledReason)
				{
					*OutDisabledReason = ToRunEventRemoveCardDisabledReason(Validation.DisabledReason);
				}
				UE_LOG(LogTemp, Warning,
					TEXT("[RunEventExecutor] RemoveCard: 拒绝 Card=%s Reason=%s"),
					*GetNameSafe(Effect.CardDefinition.Get()),
					*Validation.DisabledReason.ToString());
				return false;
			}
			FName RemoveDisabledReason = NAME_None;
			if (!FRunDeckRules::PermanentRemoveOwnedCard(State, Effect.CardDefinition.Get(), &RemoveDisabledReason))
			{
				if (OutDisabledReason)
				{
					*OutDisabledReason = ToRunEventRemoveCardDisabledReason(RemoveDisabledReason);
				}
				return false;
			}
			EffectResult.ActualDelta = -1;
			EffectResult.bApplied = true;
			break;
		}
		case EWacomRunEventEffectType::MarkEventCompleted:
			if (Effect.TargetPersistentId.IsNone())
			{
				UE_LOG(LogTemp, Warning, TEXT("[RunSession] ApplyRunEventChoiceEffects: MarkEventCompleted 缺少 TargetPersistentId"));
				if (OutDisabledReason)
				{
					*OutDisabledReason = TEXT("MissingTargetPersistentId");
				}
				return false;
			}
			State.RunEventStates.FindOrAdd(Effect.TargetPersistentId).bCompleted = true;
			EffectResult.ActualDelta = 1;
			EffectResult.bApplied = true;
			break;
		case EWacomRunEventEffectType::SetRunFlag:
			if (Effect.FlagId.IsNone())
			{
				UE_LOG(LogTemp, Warning, TEXT("[RunSession] ApplyRunEventChoiceEffects: SetRunFlag 缺少 FlagId"));
				if (OutDisabledReason)
				{
					*OutDisabledReason = TEXT("MissingRunFlagId");
				}
				return false;
			}
			State.RunFlags.Add(Effect.FlagId);
			EffectResult.ActualDelta = 1;
			EffectResult.bApplied = true;
			break;
		case EWacomRunEventEffectType::ClearRunFlag:
			if (Effect.FlagId.IsNone())
			{
				UE_LOG(LogTemp, Warning, TEXT("[RunSession] ApplyRunEventChoiceEffects: ClearRunFlag 缺少 FlagId"));
				if (OutDisabledReason)
				{
					*OutDisabledReason = TEXT("MissingRunFlagId");
				}
				return false;
			}
			State.RunFlags.Remove(Effect.FlagId);
			EffectResult.ActualDelta = -1;
			EffectResult.bApplied = true;
			break;
		default:
			UE_LOG(LogTemp, Warning, TEXT("[RunSession] ApplyRunEventChoiceEffects: 未知效果类型"));
			if (OutDisabledReason)
			{
				*OutDisabledReason = TEXT("EffectFailed");
			}
			return false;
		}

		if (OutEffectResults)
		{
			OutEffectResults->Add(MoveTemp(EffectResult));
		}
	}
	return true;
}

bool FRunEventExecutor::AcquireCard(FRunState& State, UCardDefinition* Card)
{
	if (!Card)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] AcquireCardToRun: Card 为空，拒绝"));
		return false;
	}

	FCardInstance Inst;
	Inst.Definition = Card;
	Inst.InstanceId = FGuid::NewGuid();
	ensureMsgf(Inst.InstanceId.IsValid(),
		TEXT("[RunSession] AcquireCardToRun: FGuid::NewGuid() 生成了 zero GUID"));
	State.Backpack.Add(Inst);
	FRunDeckRules::EnsureSpecialZoneEntryFor(State, Inst);
	FRunDeckRules::RecomputeBurden(State, /*bAllowBurdenRefill=*/true);
	return true;
}
