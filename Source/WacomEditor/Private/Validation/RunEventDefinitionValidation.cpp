// Copyright Wacom. All Rights Reserved.

#include "Validation/RunEventDefinitionValidation.h"

#include "Events/RunEventDefinition.h"

#define LOCTEXT_NAMESPACE "WacomRunEventDefinitionValidation"

namespace
{
	void AddValidationError(TArray<FText>& OutErrors, const FText& Message)
	{
		OutErrors.Add(Message);
	}

	FText FormatValidationError(const TCHAR* Format, const FString& A)
	{
		return FText::FromString(FString::Format(Format, { A }));
	}

	FText FormatValidationError(const TCHAR* Format, const FString& A, const FString& B)
	{
		return FText::FromString(FString::Format(Format, { A, B }));
	}
}

bool FWacomRunEventDefinitionValidation::Validate(
	const UWacomRunEventDefinition* EventDefinition,
	TArray<FText>& OutErrors)
{
	OutErrors.Reset();

	if (!EventDefinition)
	{
		AddValidationError(OutErrors, LOCTEXT("MissingEventDefinition", "RunEventDefinition 为空。"));
		return false;
	}

	if (EventDefinition->EventId.IsNone())
	{
		AddValidationError(OutErrors, LOCTEXT("MissingEventId", "EventId 不能为空。"));
	}

	if (EventDefinition->StartNodeId.IsNone())
	{
		AddValidationError(OutErrors, LOCTEXT("MissingStartNodeId", "StartNodeId 不能为空。"));
	}

	TSet<FName> NodeIds;
	for (const FWacomRunEventNodeDefinition& Node : EventDefinition->Nodes)
	{
		if (Node.NodeId.IsNone())
		{
			AddValidationError(OutErrors, LOCTEXT("MissingNodeId", "NodeId 不能为空。"));
			continue;
		}

		if (NodeIds.Contains(Node.NodeId))
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("NodeId 重复：{0}。"), Node.NodeId.ToString()));
			continue;
		}

		NodeIds.Add(Node.NodeId);
	}

	if (!EventDefinition->StartNodeId.IsNone() && !NodeIds.Contains(EventDefinition->StartNodeId))
	{
		AddValidationError(OutErrors,
			FormatValidationError(TEXT("StartNodeId 无法找到对应 Node：{0}。"), EventDefinition->StartNodeId.ToString()));
	}

	for (const FWacomRunEventNodeDefinition& Node : EventDefinition->Nodes)
	{
		TSet<FName> ChoiceIds;
		for (const FWacomRunEventChoiceDefinition& Choice : Node.Choices)
		{
			if (Choice.ChoiceId.IsNone())
			{
				AddValidationError(OutErrors,
					FormatValidationError(TEXT("Node {0} 中 ChoiceId 不能为空。"), Node.NodeId.ToString()));
			}
			else if (ChoiceIds.Contains(Choice.ChoiceId))
			{
				AddValidationError(OutErrors,
					FormatValidationError(TEXT("Node {0} 中 ChoiceId 重复：{1}。"),
						Node.NodeId.ToString(),
						Choice.ChoiceId.ToString()));
			}
			else
			{
				ChoiceIds.Add(Choice.ChoiceId);
			}

			if (!Choice.NextNodeId.IsNone() && !NodeIds.Contains(Choice.NextNodeId))
			{
				AddValidationError(OutErrors,
					FormatValidationError(TEXT("Choice {0} 的 NextNodeId 无效：{1}。"),
						Choice.ChoiceId.ToString(),
						Choice.NextNodeId.ToString()));
			}

			for (const FWacomRunEventConditionDefinition& Condition : Choice.Conditions)
			{
				switch (Condition.Type)
				{
				case EWacomRunEventConditionType::None:
				case EWacomRunEventConditionType::MinGold:
				case EWacomRunEventConditionType::MinNodeCount:
					break;
				case EWacomRunEventConditionType::MaxPressure:
					if (!IsValidPressureTypeId(Condition.PressureType))
					{
						AddValidationError(OutErrors,
							FormatValidationError(TEXT("Choice {0} 的 MaxPressure 缺少有效 PressureType。"),
								Choice.ChoiceId.ToString()));
					}
					break;
				case EWacomRunEventConditionType::HasCard:
				case EWacomRunEventConditionType::MissingCard:
					if (!Condition.CardDefinition)
					{
						AddValidationError(OutErrors,
							FormatValidationError(TEXT("Choice {0} 的卡牌条件缺少 CardDefinition。"),
								Choice.ChoiceId.ToString()));
					}
					break;
				case EWacomRunEventConditionType::EventCompleted:
				case EWacomRunEventConditionType::EventNotCompleted:
					if (Condition.TargetPersistentId.IsNone())
					{
						AddValidationError(OutErrors,
							FormatValidationError(TEXT("Choice {0} 的事件状态条件缺少 TargetPersistentId。"),
								Choice.ChoiceId.ToString()));
					}
					break;
				default:
					AddValidationError(OutErrors,
						FormatValidationError(TEXT("Choice {0} 包含未知条件类型。"), Choice.ChoiceId.ToString()));
					break;
				}
			}

			for (const FWacomRunEventEffectDefinition& Effect : Choice.Effects)
			{
				switch (Effect.Type)
				{
				case EWacomRunEventEffectType::None:
				case EWacomRunEventEffectType::AddGold:
					break;
				case EWacomRunEventEffectType::GainCard:
				case EWacomRunEventEffectType::RemoveCard:
					if (!Effect.CardDefinition)
					{
						AddValidationError(OutErrors,
							FormatValidationError(TEXT("Choice {0} 的卡牌效果缺少 CardDefinition。"),
								Choice.ChoiceId.ToString()));
					}
					break;
				case EWacomRunEventEffectType::AddPressure:
					if (!IsValidPressureTypeId(Effect.PressureType))
					{
						AddValidationError(OutErrors,
							FormatValidationError(TEXT("Choice {0} 的 AddPressure 缺少有效 PressureType。"),
								Choice.ChoiceId.ToString()));
					}
					break;
				case EWacomRunEventEffectType::ConsumeNode:
					if (Effect.Value < 0)
					{
						AddValidationError(OutErrors,
							FormatValidationError(TEXT("Choice {0} 的 ConsumeNode 不能为负数。"),
								Choice.ChoiceId.ToString()));
					}
					break;
				case EWacomRunEventEffectType::MarkEventCompleted:
					if (Effect.TargetPersistentId.IsNone())
					{
						AddValidationError(OutErrors,
							FormatValidationError(TEXT("Choice {0} 的 MarkEventCompleted 缺少 TargetPersistentId。"),
								Choice.ChoiceId.ToString()));
					}
					break;
				default:
					AddValidationError(OutErrors,
						FormatValidationError(TEXT("Choice {0} 包含未知效果类型。"), Choice.ChoiceId.ToString()));
					break;
				}
			}
		}
	}

	return OutErrors.IsEmpty();
}

bool FWacomRunEventDefinitionValidation::IsValidPressureTypeId(FName PressureTypeId)
{
	static const TSet<FName> ValidPressureTypes =
	{
		TEXT("Hunger"),
		TEXT("Wound"),
		TEXT("Fatigue"),
		TEXT("Burden"),
		TEXT("Decay"),
		TEXT("Misdeed"),
		TEXT("Bloodlust"),
		TEXT("Disability"),
	};

	return ValidPressureTypes.Contains(PressureTypeId);
}

#undef LOCTEXT_NAMESPACE
