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

	return Views;
}

#undef LOCTEXT_NAMESPACE
