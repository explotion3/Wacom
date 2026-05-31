// Copyright Wacom. All Rights Reserved.

#include "Validation/RunEventDefinitionValidation.h"

#include "Events/RunEventDefinition.h"

#define LOCTEXT_NAMESPACE "WacomRunEventDefinitionValidation"

namespace
{
	void AddValidationError(FWacomRunEventDefinitionValidationReport& Report, const FText& Message)
	{
		Report.Errors.Add(Message);
	}

	void AddValidationWarning(FWacomRunEventDefinitionValidationReport& Report, const FText& Message)
	{
		Report.Warnings.Add(Message);
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

FWacomRunEventDefinitionValidationReport FWacomRunEventDefinitionValidation::BuildReport(
	const UWacomRunEventDefinition* EventDefinition)
{
	FWacomRunEventDefinitionValidationReport Report;

	if (!EventDefinition)
	{
		AddValidationError(Report, LOCTEXT("MissingEventDefinition", "RunEventDefinition 为空。"));
		return Report;
	}

	if (EventDefinition->EventId.IsNone())
	{
		AddValidationError(Report, LOCTEXT("MissingEventId", "EventId 不能为空。"));
	}

	if (EventDefinition->StartNodeId.IsNone())
	{
		AddValidationError(Report, LOCTEXT("MissingStartNodeId", "StartNodeId 不能为空。"));
	}

	TSet<FName> NodeIds;
	for (const FWacomRunEventNodeDefinition& Node : EventDefinition->Nodes)
	{
		if (Node.NodeId.IsNone())
		{
			AddValidationError(Report, LOCTEXT("MissingNodeId", "NodeId 不能为空。"));
			continue;
		}

		if (NodeIds.Contains(Node.NodeId))
		{
			AddValidationError(Report,
				FormatValidationError(TEXT("NodeId 重复：{0}。"), Node.NodeId.ToString()));
			continue;
		}

		NodeIds.Add(Node.NodeId);
	}

	if (!EventDefinition->StartNodeId.IsNone() && !NodeIds.Contains(EventDefinition->StartNodeId))
	{
		AddValidationError(Report,
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
				AddValidationError(Report,
					FormatValidationError(TEXT("Node {0} 中 ChoiceId 不能为空。"), Node.NodeId.ToString()));
			}
			else if (ChoiceIds.Contains(Choice.ChoiceId))
			{
				AddValidationError(Report,
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
				AddValidationError(Report,
					FormatValidationError(TEXT("Node {0} / Choice {1} 的 NextNodeId 无效：{2}。"),
						Node.NodeId.ToString(),
						Choice.ChoiceId.ToString(),
						Choice.NextNodeId.ToString()));
			}
			else if (!Choice.NextNodeId.IsNone()
				&& (Choice.bCloseEventAfterResolve || Choice.bMarkEventCompleted))
			{
				AddValidationWarning(Report,
					FormatValidationError(
						TEXT("Node {0} / Choice {1} 同时配置 NextNodeId {2} 和关闭/完成事件：资产仍有效，但事件结束预览会优先，玩家不会看到进入该节点的预览。"),
						Node.NodeId.ToString(),
						Choice.ChoiceId.ToString(),
						Choice.NextNodeId.ToString()));
			}

			if (Choice.CardPayment.bRequiresOwnedCardPayment)
			{
				if (Choice.ChoiceId.IsNone())
				{
					AddValidationError(Report,
						FormatValidationError(
							TEXT("Node {0} 中卡牌支付 ChoiceId 不能为空：无法生成默认 PaymentZoneId RunEvent.Pay.{{ChoiceId}}，也无法提交 RunEvent 选项。"),
							Node.NodeId.ToString()));
				}

				const FName PaymentZoneId = ResolvePaymentZoneId(Choice);
				if (PaymentZoneId.IsNone())
				{
					AddValidationError(Report,
						FormatValidationError(
							TEXT("Node {0} / Choice {1} 的卡牌支付缺少有效 PaymentZoneId：PaymentZoneId 为空且 ChoiceId 无法用于生成 RunEvent.Pay.{{ChoiceId}}。"),
							Node.NodeId.ToString(),
							Choice.ChoiceId.ToString()));
				}
				else if (const FName* ExistingChoiceId = PaymentZoneIdsToChoiceIds.Find(PaymentZoneId))
				{
					AddValidationError(Report,
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
					AddValidationError(Report,
						FormatValidationError(
							TEXT("Node {0} / Choice {1} / PaymentZoneId {2} 的卡牌支付筛选为空：请配置 AllowedCardDefinitions、AllowedCardIds、RequiredKeywords 或 BlockedKeywords。"),
							Node.NodeId.ToString(),
							Choice.ChoiceId.ToString(),
							PaymentZoneId.ToString()));
				}
			}

			bool bHasMinGoldCondition = false;
			int32 MaxMinGoldRequirement = 0;
			for (int32 ConditionIndex = 0; ConditionIndex < Choice.Conditions.Num(); ++ConditionIndex)
			{
				const FWacomRunEventConditionDefinition& Condition = Choice.Conditions[ConditionIndex];
				switch (Condition.Type)
				{
				case EWacomRunEventConditionType::None:
				case EWacomRunEventConditionType::MinNodeCount:
					break;
				case EWacomRunEventConditionType::MinGold:
					bHasMinGoldCondition = true;
					MaxMinGoldRequirement = FMath::Max(MaxMinGoldRequirement, Condition.Value);
					break;
				case EWacomRunEventConditionType::MaxPressure:
					if (!IsValidPressureTypeId(Condition.PressureType))
					{
						AddValidationError(Report,
							FormatValidationError(TEXT("Node {0} / Choice {1} / ConditionIndex {2} 的 MaxPressure 缺少有效 PressureType。"),
								Node.NodeId.ToString(),
								Choice.ChoiceId.ToString(),
								FString::FromInt(ConditionIndex)));
					}
					break;
				case EWacomRunEventConditionType::HasCard:
				case EWacomRunEventConditionType::MissingCard:
					if (!Condition.CardDefinition)
					{
						AddValidationError(Report,
							FormatValidationError(TEXT("Node {0} / Choice {1} / ConditionIndex {2} 的卡牌条件缺少 CardDefinition。"),
								Node.NodeId.ToString(),
								Choice.ChoiceId.ToString(),
								FString::FromInt(ConditionIndex)));
					}
					break;
				case EWacomRunEventConditionType::EventCompleted:
				case EWacomRunEventConditionType::EventNotCompleted:
					if (Condition.TargetPersistentId.IsNone())
					{
						AddValidationError(Report,
							FormatValidationError(TEXT("Node {0} / Choice {1} / ConditionIndex {2} 的事件状态条件缺少 TargetPersistentId。"),
								Node.NodeId.ToString(),
								Choice.ChoiceId.ToString(),
								FString::FromInt(ConditionIndex)));
					}
					break;
				case EWacomRunEventConditionType::RunFlagSet:
				case EWacomRunEventConditionType::RunFlagNotSet:
					if (Condition.FlagId.IsNone())
					{
						AddValidationError(Report,
							FormatValidationError(TEXT("Node {0} / Choice {1} / ConditionIndex {2} 的 RunFlag 条件缺少 FlagId。"),
								Node.NodeId.ToString(),
								Choice.ChoiceId.ToString(),
								FString::FromInt(ConditionIndex)));
					}
					break;
				default:
					AddValidationError(Report,
						FormatValidationError(TEXT("Node {0} / Choice {1} / ConditionIndex {2} 包含未知条件类型。"),
							Node.NodeId.ToString(),
							Choice.ChoiceId.ToString(),
							FString::FromInt(ConditionIndex)));
					break;
				}
			}

			int32 TotalGoldCost = 0;
			for (int32 EffectIndex = 0; EffectIndex < Choice.Effects.Num(); ++EffectIndex)
			{
				const FWacomRunEventEffectDefinition& Effect = Choice.Effects[EffectIndex];
				if (Choice.CardPayment.bRequiresOwnedCardPayment
					&& Effect.Type == EWacomRunEventEffectType::RemoveCard)
				{
					AddValidationError(Report,
						FormatValidationError(
							TEXT("Node {0} / Choice {1} / EffectIndex {2} / PaymentZoneId {3} 是卡牌支付选项，不能同时配置 RemoveCard 效果；拖卡支付已经会移除精确实例。"),
							Node.NodeId.ToString(),
							Choice.ChoiceId.ToString(),
							FString::FromInt(EffectIndex),
							ResolvePaymentZoneId(Choice).ToString()));
				}

				switch (Effect.Type)
				{
				case EWacomRunEventEffectType::None:
					break;
				case EWacomRunEventEffectType::AddGold:
					if (Effect.Value < 0)
					{
						TotalGoldCost += FMath::Abs(Effect.Value);
					}
					if (Effect.Value == 0)
					{
						AddValidationWarning(Report,
							FormatValidationError(TEXT("Node {0} / Choice {1} / EffectIndex {2} 的 AddGold 数值为 0：资产仍有效，但不会产生可见后果预览或有意义的金币变化。"),
								Node.NodeId.ToString(),
								Choice.ChoiceId.ToString(),
								FString::FromInt(EffectIndex)));
					}
					break;
				case EWacomRunEventEffectType::GainCard:
				case EWacomRunEventEffectType::RemoveCard:
					if (!Effect.CardDefinition)
					{
						AddValidationError(Report,
							FormatValidationError(TEXT("Node {0} / Choice {1} / EffectIndex {2} 的卡牌效果缺少 CardDefinition。"),
								Node.NodeId.ToString(),
								Choice.ChoiceId.ToString(),
								FString::FromInt(EffectIndex)));
					}
					break;
				case EWacomRunEventEffectType::AddPressure:
					if (!IsValidPressureTypeId(Effect.PressureType))
					{
						AddValidationError(Report,
							FormatValidationError(TEXT("Node {0} / Choice {1} / EffectIndex {2} 的 AddPressure 缺少有效 PressureType。"),
								Node.NodeId.ToString(),
								Choice.ChoiceId.ToString(),
								FString::FromInt(EffectIndex)));
					}
					if (Effect.Value == 0)
					{
						AddValidationWarning(Report,
							FormatValidationError(TEXT("Node {0} / Choice {1} / EffectIndex {2} 的 AddPressure 数值为 0：资产仍有效，但不会产生可见后果预览或有意义的压力变化。"),
								Node.NodeId.ToString(),
								Choice.ChoiceId.ToString(),
								FString::FromInt(EffectIndex)));
					}
					break;
				case EWacomRunEventEffectType::ConsumeNode:
					if (Effect.Value < 0)
					{
						AddValidationError(Report,
							FormatValidationError(TEXT("Node {0} / Choice {1} / EffectIndex {2} 的 ConsumeNode 不能为负数。"),
								Node.NodeId.ToString(),
								Choice.ChoiceId.ToString(),
								FString::FromInt(EffectIndex)));
					}
					else if (Effect.Value == 0)
					{
						AddValidationWarning(Report,
							FormatValidationError(TEXT("Node {0} / Choice {1} / EffectIndex {2} 的 ConsumeNode 数值为 0：资产仍有效，但不会产生可见后果预览或有意义的行动点变化。"),
								Node.NodeId.ToString(),
								Choice.ChoiceId.ToString(),
								FString::FromInt(EffectIndex)));
					}
					break;
				case EWacomRunEventEffectType::MarkEventCompleted:
					if (Effect.TargetPersistentId.IsNone())
					{
						AddValidationError(Report,
							FormatValidationError(TEXT("Node {0} / Choice {1} / EffectIndex {2} 的 MarkEventCompleted 缺少 TargetPersistentId。"),
								Node.NodeId.ToString(),
								Choice.ChoiceId.ToString(),
								FString::FromInt(EffectIndex)));
					}
					break;
				case EWacomRunEventEffectType::SetRunFlag:
				case EWacomRunEventEffectType::ClearRunFlag:
					if (Effect.FlagId.IsNone())
					{
						AddValidationError(Report,
							FormatValidationError(TEXT("Node {0} / Choice {1} / EffectIndex {2} 的 RunFlag 效果缺少 FlagId。"),
								Node.NodeId.ToString(),
								Choice.ChoiceId.ToString(),
								FString::FromInt(EffectIndex)));
					}
					break;
				default:
					AddValidationError(Report,
						FormatValidationError(TEXT("Node {0} / Choice {1} / EffectIndex {2} 包含未知效果类型。"),
							Node.NodeId.ToString(),
							Choice.ChoiceId.ToString(),
							FString::FromInt(EffectIndex)));
					break;
				}
			}

			if (TotalGoldCost > 0 && !bHasMinGoldCondition)
			{
				AddValidationWarning(Report,
					FormatValidationError(
						TEXT("Node {0} / Choice {1} 配置了 AddGold 负数扣金币总额 {2}，但没有 MinGold 条件：资产仍有效，实际结算会 clamp 到 0，建议配置金币门槛以让支付和预览更清晰。"),
						Node.NodeId.ToString(),
						Choice.ChoiceId.ToString(),
						FString::FromInt(TotalGoldCost)));
			}
			else if (TotalGoldCost > 0 && TotalGoldCost != MaxMinGoldRequirement)
			{
				AddValidationWarning(Report,
					FormatValidationError(
						TEXT("Node {0} / Choice {1} 的金币门槛 MinGold 最大值 {2} 与 AddGold 负数扣金币总额 {3} 不一致：资产仍有效，但门槛和扣费可能不一致，影响 choice preview 清晰度。"),
						Node.NodeId.ToString(),
						Choice.ChoiceId.ToString(),
						FString::FromInt(MaxMinGoldRequirement),
						FString::FromInt(TotalGoldCost)));
			}
		}
	}

	return Report;
}

bool FWacomRunEventDefinitionValidation::Validate(
	const UWacomRunEventDefinition* EventDefinition,
	TArray<FText>& OutErrors)
{
	const FWacomRunEventDefinitionValidationReport Report = BuildReport(EventDefinition);
	OutErrors = Report.Errors;
	return Report.IsValid();
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
