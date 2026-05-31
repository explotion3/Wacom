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

	FText FormatValidationError(const TCHAR* Format, const FString& A, const FString& B, const FString& C)
	{
		return FText::FromString(FString::Format(Format, { A, B, C }));
	}

	FText FormatValidationError(
		const TCHAR* Format,
		const FString& A,
		const FString& B,
		const FString& C,
		const FString& D)
	{
		return FText::FromString(FString::Format(Format, { A, B, C, D }));
	}

	FName ResolvePaymentZoneId(const FWacomRunEventChoiceDefinition& Choice)
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

	bool HasValidPaymentFilter(const FWacomRunEventChoiceDefinition& Choice)
	{
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
		TMap<FName, FName> PaymentZoneIdsToChoiceIds;
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
					FormatValidationError(TEXT("Node {0} / Choice {1} 的 NextNodeId 无效：{2}。"),
						Node.NodeId.ToString(),
						Choice.ChoiceId.ToString(),
						Choice.NextNodeId.ToString()));
			}

			if (Choice.CardPayment.bRequiresOwnedCardPayment)
			{
				if (Choice.ChoiceId.IsNone())
				{
					AddValidationError(OutErrors,
						FormatValidationError(
							TEXT("Node {0} 中卡牌支付 ChoiceId 不能为空：无法生成默认 PaymentZoneId RunEvent.Pay.{{ChoiceId}}，也无法提交 RunEvent 选项。"),
							Node.NodeId.ToString()));
				}

				const FName PaymentZoneId = ResolvePaymentZoneId(Choice);
				if (PaymentZoneId.IsNone())
				{
					AddValidationError(OutErrors,
						FormatValidationError(
							TEXT("Node {0} / Choice {1} 的卡牌支付缺少有效 PaymentZoneId：PaymentZoneId 为空且 ChoiceId 无法用于生成 RunEvent.Pay.{{ChoiceId}}。"),
							Node.NodeId.ToString(),
							Choice.ChoiceId.ToString()));
				}
				else if (const FName* ExistingChoiceId = PaymentZoneIdsToChoiceIds.Find(PaymentZoneId))
				{
					AddValidationError(OutErrors,
						FormatValidationError(
							TEXT("Node {0} 中卡牌支付 PaymentZoneId 重复：{1}。Choice {2} 与 Choice {3} 使用同一 Zone，同一节点必须唯一。"),
							Node.NodeId.ToString(),
							PaymentZoneId.ToString(),
							ExistingChoiceId->ToString(),
							Choice.ChoiceId.ToString()));
				}
				else
				{
					PaymentZoneIdsToChoiceIds.Add(PaymentZoneId, Choice.ChoiceId);
				}

				if (!HasValidPaymentFilter(Choice))
				{
					AddValidationError(OutErrors,
						FormatValidationError(
							TEXT("Node {0} / Choice {1} / PaymentZoneId {2} 的卡牌支付筛选为空：请配置 AllowedCardDefinitions、AllowedCardIds、RequiredKeywords 或 BlockedKeywords。"),
							Node.NodeId.ToString(),
							Choice.ChoiceId.ToString(),
							PaymentZoneId.ToString()));
				}
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
				if (Choice.CardPayment.bRequiresOwnedCardPayment
					&& Effect.Type == EWacomRunEventEffectType::RemoveCard)
				{
					AddValidationError(OutErrors,
						FormatValidationError(
							TEXT("Node {0} / Choice {1} / PaymentZoneId {2} 是卡牌支付选项，不能同时配置 RemoveCard 效果；拖卡支付已经会移除精确实例。"),
							Node.NodeId.ToString(),
							Choice.ChoiceId.ToString(),
							ResolvePaymentZoneId(Choice).ToString()));
				}

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
