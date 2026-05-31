// Copyright Wacom. All Rights Reserved.

#include "UI/Events/WacomRunEventPresentationBuilder.h"

#include "Cards/CardDefinition.h"

#define LOCTEXT_NAMESPACE "WacomRunEventPresentationBuilder"

namespace
{
	FText GetRunEventCardDisplayName(const UCardDefinition* Card)
	{
		if (!Card)
		{
			return LOCTEXT("UnknownCard", "未知卡牌");
		}
		return Card->DisplayName.IsEmpty()
			? FText::FromName(Card->CardId)
			: Card->DisplayName;
	}

	FWacomAppToastView MakeToast(FText Message, EWacomAppToastTone Tone, FName IconKey)
	{
		FWacomAppToastView View;
		View.MessageText = Message;
		View.Tone = Tone;
		View.IconKey = IconKey;
		return View;
	}

	FText FormatEventRequirementTarget(FName TargetPersistentId)
	{
		return TargetPersistentId.IsNone()
			? LOCTEXT("UnknownEventTarget", "事件目标")
			: FText::FromName(TargetPersistentId);
	}

	FText FormatRunFlagId(FName FlagId)
	{
		return FlagId.IsNone()
			? LOCTEXT("UnknownRunFlag", "未配置标记")
			: FText::FromName(FlagId);
	}
}

FText UWacomRunEventPresentationBuilder::FormatDisabledReason(FName DisabledReason)
{
	if (DisabledReason.IsNone())
	{
		return FText::GetEmpty();
	}
	if (DisabledReason == TEXT("InsufficientGold"))
	{
		return LOCTEXT("InsufficientGold", "金币不足");
	}
	if (DisabledReason == TEXT("InsufficientNode"))
	{
		return LOCTEXT("InsufficientNode", "行动点不足");
	}
	if (DisabledReason == TEXT("PressureTooHigh"))
	{
		return LOCTEXT("PressureTooHigh", "压力过高");
	}
	if (DisabledReason == TEXT("InvalidPressureType"))
	{
		return LOCTEXT("InvalidPressureType", "压力类型配置错误");
	}
	if (DisabledReason == TEXT("UnknownCondition"))
	{
		return LOCTEXT("UnknownCondition", "条件配置错误");
	}
	if (DisabledReason == TEXT("MissingRequiredCard"))
	{
		return LOCTEXT("MissingRequiredCard", "缺少所需卡牌");
	}
	if (DisabledReason == TEXT("AlreadyHasCard"))
	{
		return LOCTEXT("AlreadyHasCard", "已经拥有该卡牌");
	}
	if (DisabledReason == TEXT("RequiredEventNotCompleted"))
	{
		return LOCTEXT("RequiredEventNotCompleted", "前置事件未完成");
	}
	if (DisabledReason == TEXT("RequiredEventAlreadyCompleted"))
	{
		return LOCTEXT("RequiredEventAlreadyCompleted", "前置事件已完成");
	}
	if (DisabledReason == TEXT("MissingTargetPersistentId"))
	{
		return LOCTEXT("MissingTargetPersistentId", "事件目标配置错误");
	}
	if (DisabledReason == TEXT("ProtectedCard"))
	{
		return LOCTEXT("ProtectedCard", "该卡牌不能被移除");
	}
	if (DisabledReason == TEXT("LastCapacityProvider"))
	{
		return LOCTEXT("LastCapacityProvider", "这是最后一张背包容量卡");
	}
	if (DisabledReason == TEXT("MissingCard"))
	{
		return LOCTEXT("MissingCard", "卡牌配置错误");
	}
	if (DisabledReason == TEXT("NoActiveEvent"))
	{
		return LOCTEXT("NoActiveEvent", "当前没有事件");
	}
	if (DisabledReason == TEXT("InvalidEventState"))
	{
		return LOCTEXT("InvalidEventState", "事件状态无效");
	}
	if (DisabledReason == TEXT("ChoiceNotFound"))
	{
		return LOCTEXT("ChoiceNotFound", "找不到该选项");
	}
	if (DisabledReason == TEXT("EffectFailed"))
	{
		return LOCTEXT("EffectFailed", "事件效果执行失败");
	}
	if (DisabledReason == TEXT("RequiresCardPayment"))
	{
		return LOCTEXT("RequiresCardPayment", "需要拖入卡牌支付");
	}
	if (DisabledReason == TEXT("MissingPaidCard"))
	{
		return LOCTEXT("MissingPaidCard", "未选择支付卡牌");
	}
	if (DisabledReason == TEXT("PaymentCardNotAllowed"))
	{
		return LOCTEXT("PaymentCardNotAllowed", "这张卡不能用于支付");
	}
	if (DisabledReason == TEXT("MissingPaymentFilter"))
	{
		return LOCTEXT("MissingPaymentFilter", "卡牌支付配置错误");
	}
	if (DisabledReason == TEXT("MissingRequiredPaymentKeyword"))
	{
		return LOCTEXT("MissingRequiredPaymentKeyword", "支付卡牌缺少所需关键词");
	}
	if (DisabledReason == TEXT("BlockedPaymentKeyword"))
	{
		return LOCTEXT("BlockedPaymentKeyword", "支付卡牌带有禁止关键词");
	}
	if (DisabledReason == TEXT("PaymentChoiceHasRemoveCardEffect"))
	{
		return LOCTEXT("PaymentChoiceHasRemoveCardEffect", "卡牌支付选项配置了重复删牌效果");
	}
	if (DisabledReason == TEXT("PaymentNotRequired"))
	{
		return LOCTEXT("PaymentNotRequired", "该选项不需要卡牌支付");
	}
	if (DisabledReason == TEXT("CardNotOwned"))
	{
		return LOCTEXT("CardNotOwned", "未持有这张卡");
	}
	if (DisabledReason == TEXT("MissingRunFlagId"))
	{
		return LOCTEXT("MissingRunFlagId", "Run 标记配置错误");
	}
	if (DisabledReason == TEXT("RequiredRunFlagMissing"))
	{
		return LOCTEXT("RequiredRunFlagMissing", "缺少所需标记");
	}
	if (DisabledReason == TEXT("BlockedRunFlagSet"))
	{
		return LOCTEXT("BlockedRunFlagSet", "已有禁止标记");
	}
	return LOCTEXT("UnknownReason", "不可选择");
}

FText UWacomRunEventPresentationBuilder::FormatPressureName(EWacomPressureType PressureType)
{
	switch (PressureType)
	{
	case EWacomPressureType::Hunger: return LOCTEXT("PressureHunger", "饥饿");
	case EWacomPressureType::Wound: return LOCTEXT("PressureWound", "伤口");
	case EWacomPressureType::Fatigue: return LOCTEXT("PressureFatigue", "疲劳");
	case EWacomPressureType::Burden: return LOCTEXT("PressureBurden", "负重");
	case EWacomPressureType::Decay: return LOCTEXT("PressureDecay", "腐朽");
	case EWacomPressureType::Misdeed: return LOCTEXT("PressureMisdeed", "恶行");
	case EWacomPressureType::Bloodlust: return LOCTEXT("PressureBloodlust", "杀戮欲");
	case EWacomPressureType::Disability: return LOCTEXT("PressureDisability", "残疾");
	default: return LOCTEXT("PressureUnknown", "未知压力");
	}
}

FWacomRunEventChoiceRequirementView UWacomRunEventPresentationBuilder::BuildChoiceRequirementView(
	const FRunEventChoiceSnapshot& Choice)
{
	FWacomRunEventChoiceRequirementView View;
	View.ChoiceId = Choice.ChoiceId;
	View.bAvailable = Choice.bAvailable;
	View.bRequiresCardPayment = Choice.bRequiresOwnedCardPayment;
	View.PaymentCandidateCount = Choice.PaymentCandidateCount;

	for (const FRunEventChoiceRequirementSnapshot& Requirement : Choice.Requirements)
	{
		FWacomRunEventChoiceRequirementItemView Item;
		Item.Kind = Requirement.Kind;
		Item.bSatisfied = Requirement.bSatisfied;
		Item.DisabledReason = Requirement.DisabledReason;
		Item.Tone = Requirement.bSatisfied
			? EWacomRunEventChoiceAvailabilityTone::Ready
			: EWacomRunEventChoiceAvailabilityTone::Blocked;
		if (!Requirement.bSatisfied)
		{
			++View.UnsatisfiedRequirementCount;
		}

		switch (Requirement.Kind)
		{
		case ERunEventChoiceRequirementKind::MinGold:
			Item.Text = FText::Format(
				LOCTEXT("RequireGoldFmt", "需要金币：{0} / 当前 {1}"),
				FText::AsNumber(Requirement.RequiredValue),
				FText::AsNumber(Requirement.CurrentValue));
			break;
		case ERunEventChoiceRequirementKind::MinNodeCount:
			Item.Text = FText::Format(
				LOCTEXT("RequireNodeFmt", "需要行动点：{0} / 当前 {1}"),
				FText::AsNumber(Requirement.RequiredValue),
				FText::AsNumber(Requirement.CurrentValue));
			break;
		case ERunEventChoiceRequirementKind::MaxPressure:
			Item.Text = FText::Format(
				LOCTEXT("RequirePressureFmt", "压力不高于：{0} {1} / 当前 {2}"),
				FormatPressureName(Requirement.PressureType),
				FText::AsNumber(Requirement.RequiredValue),
				FText::AsNumber(Requirement.CurrentValue));
			break;
		case ERunEventChoiceRequirementKind::HasCard:
			Item.Text = FText::Format(
				LOCTEXT("RequireHasCardFmt", "需要持有：{0}"),
				GetRunEventCardDisplayName(Requirement.CardDefinition.Get()));
			break;
		case ERunEventChoiceRequirementKind::MissingCard:
			Item.Text = FText::Format(
				LOCTEXT("RequireMissingCardFmt", "不能持有：{0}"),
				GetRunEventCardDisplayName(Requirement.CardDefinition.Get()));
			break;
		case ERunEventChoiceRequirementKind::EventCompleted:
			Item.Text = FText::Format(
				LOCTEXT("RequireEventCompletedFmt", "需要事件已完成：{0}"),
				FormatEventRequirementTarget(Requirement.TargetPersistentId));
			break;
		case ERunEventChoiceRequirementKind::EventNotCompleted:
			Item.Text = FText::Format(
				LOCTEXT("RequireEventNotCompletedFmt", "需要事件未完成：{0}"),
				FormatEventRequirementTarget(Requirement.TargetPersistentId));
			break;
		case ERunEventChoiceRequirementKind::RunFlagSet:
			Item.Text = FText::Format(
				LOCTEXT("RequireRunFlagSetFmt", "需要标记：{0}"),
				FormatRunFlagId(Requirement.FlagId));
			break;
		case ERunEventChoiceRequirementKind::RunFlagNotSet:
			Item.Text = FText::Format(
				LOCTEXT("RequireRunFlagNotSetFmt", "不能有标记：{0}"),
				FormatRunFlagId(Requirement.FlagId));
			break;
		case ERunEventChoiceRequirementKind::CardPayment:
			Item.Text = Requirement.PaymentCandidateCount > 0
				? FText::Format(
					LOCTEXT("RequirePaymentCandidateFmt", "需要拖入卡牌支付：{0} 张可用"),
					FText::AsNumber(Requirement.PaymentCandidateCount))
				: FText::Format(
					LOCTEXT("RequirePaymentMissingFmt", "需要拖入卡牌支付：{0}"),
					FormatDisabledReason(Requirement.DisabledReason.IsNone()
						? FName(TEXT("MissingRequiredCard"))
						: Requirement.DisabledReason));
			Item.Tone = Requirement.PaymentCandidateCount > 0
				? EWacomRunEventChoiceAvailabilityTone::Requirement
				: EWacomRunEventChoiceAvailabilityTone::Blocked;
			break;
		case ERunEventChoiceRequirementKind::None:
		default:
			break;
		}

		if (!Item.Text.IsEmpty())
		{
			View.RequirementItems.Add(MoveTemp(Item));
		}
	}

	if (Choice.bRequiresOwnedCardPayment)
	{
		if (Choice.PaymentCandidateCount > 0)
		{
			View.RequirementText = FText::Format(
				LOCTEXT("PaymentCandidateCountFmt", "拖入卡牌支付：{0} 张可用"),
				FText::AsNumber(Choice.PaymentCandidateCount));
			View.Tone = EWacomRunEventChoiceAvailabilityTone::Requirement;
		}
		else
		{
			const FName Reason = Choice.PaymentDisabledReason.IsNone()
				? Choice.DisabledReason
				: Choice.PaymentDisabledReason;
			const FText ReasonText = FormatDisabledReason(Reason);
			View.PrimaryReason = Reason.IsNone() ? FName(TEXT("MissingRequiredCard")) : Reason;
			View.RequirementText = FText::Format(
				LOCTEXT("PaymentMissingReasonFmt", "缺少可支付卡牌：{0}"),
				ReasonText.IsEmpty() ? FormatDisabledReason(TEXT("MissingRequiredCard")) : ReasonText);
			View.Tone = EWacomRunEventChoiceAvailabilityTone::Blocked;
		}
	}

	if (!Choice.bAvailable)
	{
		View.PrimaryReason = !Choice.DisabledReason.IsNone()
			? Choice.DisabledReason
			: View.PrimaryReason;
		if (!Choice.DisabledReason.IsNone()
			&& (!Choice.bRequiresOwnedCardPayment || Choice.PaymentCandidateCount > 0))
		{
			View.BlockedReasonText = FText::Format(
				LOCTEXT("DisabledReasonFmt", "不可选：{0}"),
				FormatDisabledReason(Choice.DisabledReason));
		}
		View.Tone = EWacomRunEventChoiceAvailabilityTone::Blocked;
	}
	else if (View.Tone == EWacomRunEventChoiceAvailabilityTone::None)
	{
		View.Tone = EWacomRunEventChoiceAvailabilityTone::Ready;
	}

	return View;
}

FWacomRunEventChoiceConsequenceView UWacomRunEventPresentationBuilder::BuildChoiceConsequenceView(
	const FRunEventChoiceSnapshot& Choice)
{
	FWacomRunEventChoiceConsequenceView View;
	View.ChoiceId = Choice.ChoiceId;

	for (const FRunEventChoiceConsequenceSnapshot& Consequence : Choice.Consequences)
	{
		FWacomRunEventChoiceConsequenceItemView Item;
		Item.Kind = Consequence.Kind;
		Item.EffectType = Consequence.EffectType;
		Item.Tone = EWacomRunEventChoiceAvailabilityTone::None;

		switch (Consequence.Kind)
		{
		case ERunEventChoiceConsequenceKind::Effect:
			switch (Consequence.EffectType)
			{
			case EWacomRunEventEffectType::GainCard:
				Item.Text = FText::Format(
					LOCTEXT("PreviewGainCardFmt", "获得卡牌：{0}"),
					GetRunEventCardDisplayName(Consequence.CardDefinition.Get()));
				Item.Tone = EWacomRunEventChoiceAvailabilityTone::Ready;
				break;
			case EWacomRunEventEffectType::AddGold:
				if (Consequence.Amount > 0)
				{
					Item.Text = FText::Format(
						LOCTEXT("PreviewGoldGainFmt", "获得金币：{0}"),
						FText::AsNumber(Consequence.Amount));
					Item.Tone = EWacomRunEventChoiceAvailabilityTone::Ready;
				}
				else if (Consequence.Amount < 0)
				{
					Item.Text = FText::Format(
						LOCTEXT("PreviewGoldLossFmt", "失去金币：{0}"),
						FText::AsNumber(FMath::Abs(Consequence.Amount)));
					Item.Tone = EWacomRunEventChoiceAvailabilityTone::Requirement;
				}
				break;
			case EWacomRunEventEffectType::AddPressure:
				if (Consequence.Amount != 0)
				{
					Item.Text = Consequence.Amount > 0
						? FText::Format(
							LOCTEXT("PreviewPressureIncreaseFmt", "{0} +{1}"),
							FormatPressureName(Consequence.PressureType),
							FText::AsNumber(Consequence.Amount))
						: FText::Format(
							LOCTEXT("PreviewPressureReduceFmt", "{0} -{1}"),
							FormatPressureName(Consequence.PressureType),
							FText::AsNumber(FMath::Abs(Consequence.Amount)));
					Item.Tone = Consequence.Amount > 0
						? EWacomRunEventChoiceAvailabilityTone::Requirement
						: EWacomRunEventChoiceAvailabilityTone::Ready;
				}
				break;
			case EWacomRunEventEffectType::ConsumeNode:
				if (Consequence.Amount > 0)
				{
					Item.Text = FText::Format(
						LOCTEXT("PreviewConsumeNodeFmt", "消耗行动点：{0}"),
						FText::AsNumber(Consequence.Amount));
					Item.Tone = EWacomRunEventChoiceAvailabilityTone::Requirement;
				}
				break;
			case EWacomRunEventEffectType::RemoveCard:
				Item.Text = FText::Format(
					LOCTEXT("PreviewRemoveCardFmt", "交出卡牌：{0}"),
					GetRunEventCardDisplayName(Consequence.CardDefinition.Get()));
				Item.Tone = EWacomRunEventChoiceAvailabilityTone::Requirement;
				break;
			case EWacomRunEventEffectType::MarkEventCompleted:
				Item.Text = FText::Format(
					LOCTEXT("PreviewMarkEventCompletedFmt", "完成事件：{0}"),
					FormatEventRequirementTarget(Consequence.TargetPersistentId));
				Item.Tone = EWacomRunEventChoiceAvailabilityTone::Requirement;
				break;
			case EWacomRunEventEffectType::SetRunFlag:
				Item.Text = FText::Format(
					LOCTEXT("PreviewSetRunFlagFmt", "设置标记：{0}"),
					FormatRunFlagId(Consequence.FlagId));
				Item.Tone = EWacomRunEventChoiceAvailabilityTone::Requirement;
				break;
			case EWacomRunEventEffectType::ClearRunFlag:
				Item.Text = FText::Format(
					LOCTEXT("PreviewClearRunFlagFmt", "清除标记：{0}"),
					FormatRunFlagId(Consequence.FlagId));
				Item.Tone = EWacomRunEventChoiceAvailabilityTone::Requirement;
				break;
			case EWacomRunEventEffectType::None:
			default:
				break;
			}
			break;
		case ERunEventChoiceConsequenceKind::NodeTransition:
		{
			const FText NodeText = Consequence.ResolvedNodeTitleText.IsEmpty()
				? (Consequence.ResolvedNodeId.IsNone() ? FText::GetEmpty() : FText::FromName(Consequence.ResolvedNodeId))
				: Consequence.ResolvedNodeTitleText;
			if (!NodeText.IsEmpty())
			{
				Item.Text = FText::Format(LOCTEXT("PreviewNodeTransitionFmt", "进入：{0}"), NodeText);
				Item.Tone = EWacomRunEventChoiceAvailabilityTone::Requirement;
			}
			break;
		}
		case ERunEventChoiceConsequenceKind::EventEnds:
			Item.Text = LOCTEXT("PreviewEventEnds", "事件将结束");
			Item.Tone = EWacomRunEventChoiceAvailabilityTone::Requirement;
			break;
		case ERunEventChoiceConsequenceKind::None:
		default:
			break;
		}

		if (!Item.Text.IsEmpty())
		{
			View.ConsequenceItems.Add(MoveTemp(Item));
		}
	}

	return View;
}

TArray<FWacomAppToastView> UWacomRunEventPresentationBuilder::BuildToastViewsFromChoiceResult(
	const FRunEventChoiceResult& Result)
{
	TArray<FWacomAppToastView> Views;
	if (!Result.bSucceeded)
	{
		const FText ReasonText = FormatDisabledReason(Result.DisabledReason);
		if (!ReasonText.IsEmpty())
		{
			Views.Add(MakeToast(ReasonText, EWacomAppToastTone::Warning, TEXT("RunEventBlocked")));
		}
		return Views;
	}

	for (const FRunEventChoiceEffectResult& EffectResult : Result.EffectResults)
	{
		if (!EffectResult.bApplied)
		{
			continue;
		}

		switch (EffectResult.EffectType)
		{
		case EWacomRunEventEffectType::GainCard:
			Views.Add(MakeToast(
				FText::Format(LOCTEXT("CardGainedFmt", "获得卡牌：{0}"),
					GetRunEventCardDisplayName(EffectResult.CardDefinition.Get())),
				EWacomAppToastTone::Positive,
				TEXT("CardGained")));
			break;
		case EWacomRunEventEffectType::AddGold:
			if (EffectResult.ActualDelta > 0)
			{
				Views.Add(MakeToast(
					FText::Format(LOCTEXT("GoldGainedFmt", "获得 {0} 金币"), FText::AsNumber(EffectResult.ActualDelta)),
					EWacomAppToastTone::Positive,
					TEXT("GoldChanged")));
			}
			else if (EffectResult.ActualDelta < 0)
			{
				Views.Add(MakeToast(
					FText::Format(LOCTEXT("GoldLostFmt", "失去 {0} 金币"), FText::AsNumber(FMath::Abs(EffectResult.ActualDelta))),
					EWacomAppToastTone::Warning,
					TEXT("GoldChanged")));
			}
			break;
		case EWacomRunEventEffectType::AddPressure:
			if (EffectResult.ActualDelta != 0)
			{
				const FText PressureName = FormatPressureName(EffectResult.PressureType);
				Views.Add(MakeToast(
					EffectResult.ActualDelta > 0
						? FText::Format(LOCTEXT("PressureIncreasedFmt", "{0} +{1}"),
							PressureName,
							FText::AsNumber(EffectResult.ActualDelta))
						: FText::Format(LOCTEXT("PressureReducedFmt", "{0} -{1}"),
							PressureName,
							FText::AsNumber(FMath::Abs(EffectResult.ActualDelta))),
					EffectResult.ActualDelta > 0 ? EWacomAppToastTone::Warning : EWacomAppToastTone::Positive,
					TEXT("PressureChanged")));
			}
			break;
		case EWacomRunEventEffectType::ConsumeNode:
			if (EffectResult.ActualDelta < 0)
			{
				Views.Add(MakeToast(
					FText::Format(LOCTEXT("NodeConsumedFmt", "消耗 {0} 行动点"), FText::AsNumber(FMath::Abs(EffectResult.ActualDelta))),
					EWacomAppToastTone::System,
					TEXT("NodeConsumed")));
			}
			break;
		case EWacomRunEventEffectType::RemoveCard:
			Views.Add(MakeToast(
				FText::Format(LOCTEXT("CardRemovedFmt", "交出卡牌：{0}"),
					GetRunEventCardDisplayName(EffectResult.CardDefinition.Get())),
				EWacomAppToastTone::Warning,
				TEXT("CardRemoved")));
			break;
		case EWacomRunEventEffectType::MarkEventCompleted:
			break;
		default:
			break;
		}
	}

	if (Result.PaidCardDefinition)
	{
		Views.Add(MakeToast(
			FText::Format(LOCTEXT("PaidCardFmt", "交出卡牌：{0}"),
				GetRunEventCardDisplayName(Result.PaidCardDefinition.Get())),
			EWacomAppToastTone::Warning,
			TEXT("CardPaid")));
	}

	if (Result.bEventClosedAfterResolve || Result.bEventCompletedAfterResolve)
	{
		Views.Add(MakeToast(
			LOCTEXT("RunEventEnded", "事件已结束"),
			EWacomAppToastTone::System,
			TEXT("RunEventEnded")));
	}
	else if (Result.bNodeChanged)
	{
		const FText NodeText = Result.ResolvedNodeTitleText.IsEmpty()
			? (Result.ResolvedNodeId.IsNone() ? FText::GetEmpty() : FText::FromName(Result.ResolvedNodeId))
			: Result.ResolvedNodeTitleText;
		if (!NodeText.IsEmpty())
		{
			Views.Add(MakeToast(
				FText::Format(LOCTEXT("RunEventEnteredNodeFmt", "进入：{0}"), NodeText),
				EWacomAppToastTone::System,
				TEXT("RunEventNodeChanged")));
		}
	}

	return Views;
}

#undef LOCTEXT_NAMESPACE
