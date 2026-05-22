// Copyright Wacom. All Rights Reserved.

#include "Events/RunEventExecutor.h"

#include "Cards/CardDefinition.h"
#include "Deck/RunDeckRules.h"
#include "Events/RunEventDefinition.h"

namespace
{
	FName ToRunEventRemoveCardDisabledReason(FName DeckRulesReason)
	{
		if (DeckRulesReason == TEXT("CardNotOwned"))
		{
			return TEXT("MissingRequiredCard");
		}
		if (DeckRulesReason == TEXT("Intrinsic"))
		{
			return TEXT("ProtectedCard");
		}
		if (DeckRulesReason == TEXT("LastCapacityProvider"))
		{
			return TEXT("LastCapacityProvider");
		}
		if (DeckRulesReason == TEXT("MissingCard"))
		{
			return TEXT("MissingCard");
		}
		return DeckRulesReason.IsNone() ? FName(TEXT("EffectFailed")) : DeckRulesReason;
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
		ChoiceSnapshot.bAvailable = IsChoiceAvailable(State, Choice, ChoiceSnapshot.DisabledReason);
		Snapshot.Choices.Add(MoveTemp(ChoiceSnapshot));
	}

	return Snapshot;
}

FRunEventChoiceResult FRunEventExecutor::ChooseOption(FRunState& State, FName ChoiceId)
{
	FRunEventChoiceResult Result;
	Result.ChoiceId = ChoiceId;

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

	if (!ApplyChoiceEffects(State, *Choice, &Result.EffectResults, &Result.DisabledReason))
	{
		if (Result.DisabledReason.IsNone())
		{
			Result.DisabledReason = TEXT("EffectFailed");
		}
		return Result;
	}

	if (Choice->bMarkEventCompleted)
	{
		EventState->bCompleted = true;
	}

	if (!Choice->NextNodeId.IsNone())
	{
		if (!FindNode(State.ActiveRunEventDefinition, Choice->NextNodeId))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[RunSession] ChooseRunEventOption: NextNodeId=%s 无效，保持当前节点"),
				*Choice->NextNodeId.ToString());
		}
		else
		{
			EventState->CurrentNodeId = Choice->NextNodeId;
		}
	}

	if (Choice->bCloseEventAfterResolve || EventState->bCompleted)
	{
		State.ActiveRunEventId = NAME_None;
		State.ActiveRunEventDefinition = nullptr;
	}

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
			const int32 NodesBefore = State.RemainingNodeCount;
			EffectResult.ActualDelta = -FMath::Min(Count, NodesBefore);
			if (Count > 0)
			{
				const bool bHadEnoughNode = State.RemainingNodeCount >= Count;
				State.RemainingNodeCount = FMath::Max(0, State.RemainingNodeCount - Count);
				if (State.RemainingNodeCount <= 0)
				{
					AdvanceToNextPhase(State);
				}
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

void FRunEventExecutor::AdvanceToNextPhase(FRunState& State)
{
	const ETimePhase PrevPhase = State.CurrentTimePhase;

	switch (State.CurrentTimePhase)
	{
	case ETimePhase::Morning: State.CurrentTimePhase = ETimePhase::Day;     break;
	case ETimePhase::Day:     State.CurrentTimePhase = ETimePhase::Dusk;    break;
	case ETimePhase::Dusk:    State.CurrentTimePhase = ETimePhase::Night;   break;
	case ETimePhase::Night:   State.CurrentTimePhase = ETimePhase::Sunrise; break;
	case ETimePhase::Sunrise:
		State.CurrentTimePhase = ETimePhase::Morning;
		++State.CurrentDayNumber;
		break;
	default:
		ensureMsgf(false, TEXT("[RunSession] AdvanceToNextPhase 收到未知时段 %d"),
			(int32)State.CurrentTimePhase);
		State.CurrentTimePhase = ETimePhase::Morning;
		break;
	}

	ResetRemainingNodeForPhase(State);

	UE_LOG(LogTemp, Display,
		TEXT("[RunSession] Phase advanced: Day=%d Phase=%d RemainingNodes=%d"),
		State.CurrentDayNumber, (int32)State.CurrentTimePhase, State.RemainingNodeCount);

	OnPhaseEntered(State, State.CurrentTimePhase, PrevPhase);
}

void FRunEventExecutor::OnPhaseEntered(FRunState& State, ETimePhase NewPhase, ETimePhase PrevPhase)
{
	switch (NewPhase)
	{
	case ETimePhase::Morning:
		State.Pressure.Add(EWacomPressureType::Hunger, 5);
		if (PrevPhase == ETimePhase::Sunrise)
		{
			State.Pressure.Add(EWacomPressureType::Decay, 5);
		}
		break;
	case ETimePhase::Dusk:
		State.Pressure.Add(EWacomPressureType::Hunger, 5);
		break;
	case ETimePhase::Sunrise:
		State.Pressure.Add(EWacomPressureType::Fatigue, 10);
		break;
	case ETimePhase::Day:
	case ETimePhase::Night:
	default:
		break;
	}
}

void FRunEventExecutor::ResetRemainingNodeForPhase(FRunState& State)
{
	switch (State.CurrentTimePhase)
	{
	case ETimePhase::Morning: State.RemainingNodeCount = State.InitialNodeCount_Morning; break;
	case ETimePhase::Day:     State.RemainingNodeCount = State.InitialNodeCount_Day;     break;
	case ETimePhase::Dusk:    State.RemainingNodeCount = State.InitialNodeCount_Dusk;    break;
	case ETimePhase::Night:   State.RemainingNodeCount = State.InitialNodeCount_Night;   break;
	case ETimePhase::Sunrise: State.RemainingNodeCount = State.InitialNodeCount_Sunrise; break;
	default:
		State.RemainingNodeCount = 0;
		break;
	}
}
