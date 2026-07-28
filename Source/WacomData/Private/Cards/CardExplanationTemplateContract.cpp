// Copyright Wacom. All Rights Reserved.

#include "Cards/CardExplanationTemplateContract.h"

#include "Cards/CardEffect.h"
#include "Cards/CardPassive.h"
#include "Cards/CardUpgradeTypes.h"
#include "Tags/WacomGameplayTags.h"

namespace WacomCardExplanationTemplateContract
{
	namespace
	{
		void SetError(FString* OutError, const FString& Error)
		{
			if (OutError)
			{
				*OutError = Error;
			}
		}

		bool TryParsePassiveEffectPath(
			const FString& SlotType,
			const FString& SlotPath,
			FWacomCardExplanationTemplateSlot& OutSlot,
			FString* OutError)
		{
			constexpr TCHAR Prefix[] = TEXT("PassiveEffect[");
			if (!SlotPath.StartsWith(Prefix, ESearchCase::CaseSensitive))
			{
				SetError(
					OutError,
					FString::Printf(TEXT("未知被动模板参数：{%s:%s}。"), *SlotType, *SlotPath));
				return false;
			}

			const int32 IndexStart = UE_ARRAY_COUNT(Prefix) - 1;
			const int32 BracketIndex = SlotPath.Find(
				TEXT("]"),
				ESearchCase::CaseSensitive,
				ESearchDir::FromStart,
				IndexStart);
			if (BracketIndex == INDEX_NONE
				|| BracketIndex == IndexStart
				|| !SlotPath.Mid(BracketIndex).StartsWith(TEXT("]."), ESearchCase::CaseSensitive))
			{
				SetError(
					OutError,
					FString::Printf(TEXT("被动效果参数格式无效：{%s:%s}。"), *SlotType, *SlotPath));
				return false;
			}

			const FString IndexText = SlotPath.Mid(
				IndexStart,
				BracketIndex - IndexStart);
			bool bAllDigits = !IndexText.IsEmpty();
			for (const TCHAR Character : IndexText)
			{
				if (!FChar::IsDigit(Character))
				{
					bAllDigits = false;
					break;
				}
			}
			const int64 ParsedIndex = bAllDigits
				? FCString::Atoi64(*IndexText)
				: -1;
			if (ParsedIndex < 0 || ParsedIndex > MAX_int32)
			{
				SetError(
					OutError,
					FString::Printf(TEXT("被动效果索引必须是非负整数：{%s:%s}。"), *SlotType, *SlotPath));
				return false;
			}
			const int32 EffectIndex = static_cast<int32>(ParsedIndex);

			const FString Member = SlotPath.Mid(BracketIndex + 2);
			if (SlotType == TEXT("value") && Member == TEXT("Magnitude"))
			{
				OutSlot.Kind = EWacomCardExplanationTemplateSlotKind::PassiveEffectMagnitude;
			}
			else if (SlotType == TEXT("icon") && Member == TEXT("Icon"))
			{
				OutSlot.Kind = EWacomCardExplanationTemplateSlotKind::PassiveEffectIcon;
			}
			else if (SlotType == TEXT("status") && Member == TEXT("Status"))
			{
				OutSlot.Kind = EWacomCardExplanationTemplateSlotKind::PassiveEffectStatus;
			}
			else
			{
				SetError(
					OutError,
					FString::Printf(TEXT("被动效果参数类型与成员不匹配：{%s:%s}。"), *SlotType, *SlotPath));
				return false;
			}

			OutSlot.PassiveEffectIndex = EffectIndex;
			OutSlot.SlotName = FName(*SlotPath);
			return true;
		}

		template <typename ValidateSlotType>
		void ValidateTemplateSyntax(
			const FText& Template,
			EWacomCardExplanationTemplateContext Context,
			ValidateSlotType&& ValidateSlot,
			TArray<FString>& OutErrors)
		{
			const FString Source = Template.ToString();
			int32 Cursor = 0;
			while (Cursor < Source.Len())
			{
				const int32 OpenIndex = Source.Find(
					TEXT("{"),
					ESearchCase::CaseSensitive,
					ESearchDir::FromStart,
					Cursor);
				const int32 UnexpectedClose = Source.Find(
					TEXT("}"),
					ESearchCase::CaseSensitive,
					ESearchDir::FromStart,
					Cursor);
				if (UnexpectedClose != INDEX_NONE
					&& (OpenIndex == INDEX_NONE || UnexpectedClose < OpenIndex))
				{
					OutErrors.Add(FString::Printf(
						TEXT("模板在字符 %d 处存在没有起始括号的 '}'。"),
						UnexpectedClose));
					Cursor = UnexpectedClose + 1;
					continue;
				}
				if (OpenIndex == INDEX_NONE)
				{
					break;
				}

				const int32 CloseIndex = Source.Find(
					TEXT("}"),
					ESearchCase::CaseSensitive,
					ESearchDir::FromStart,
					OpenIndex + 1);
				if (CloseIndex == INDEX_NONE)
				{
					OutErrors.Add(FString::Printf(
						TEXT("模板在字符 %d 处的参数没有闭合 '}'。"),
						OpenIndex));
					break;
				}

				const FString SlotText = Source.Mid(
					OpenIndex + 1,
					CloseIndex - OpenIndex - 1);
				if (SlotText.Contains(TEXT("{")))
				{
					OutErrors.Add(FString::Printf(
						TEXT("模板参数不允许嵌套括号：{%s}。"),
						*SlotText));
					Cursor = CloseIndex + 1;
					continue;
				}

				FWacomCardExplanationTemplateSlot ParsedSlot;
				FString ParseError;
				if (!TryParseSlot(
					SlotText,
					Context,
					ParsedSlot,
					&ParseError))
				{
					OutErrors.Add(ParseError);
				}
				else
				{
					ValidateSlot(ParsedSlot, OutErrors);
				}
				Cursor = CloseIndex + 1;
			}
		}
	}

	bool TryParseSlot(
		const FString& Slot,
		const EWacomCardExplanationTemplateContext Context,
		FWacomCardExplanationTemplateSlot& OutSlot,
		FString* OutError)
	{
		OutSlot = {};
		if (Slot.IsEmpty())
		{
			SetError(OutError, TEXT("模板参数不能为空。"));
			return false;
		}

		FString SlotType;
		FString SlotPath;
		if (!Slot.Split(TEXT(":"), &SlotType, &SlotPath)
			|| SlotType.IsEmpty()
			|| SlotPath.IsEmpty())
		{
			SetError(
				OutError,
				FString::Printf(TEXT("模板参数缺少类型或名称：{%s}。"), *Slot));
			return false;
		}

		OutSlot.SlotName = FName(*SlotPath);
		if (Context == EWacomCardExplanationTemplateContext::Effect)
		{
			if (SlotType == TEXT("value") && SlotPath == TEXT("Magnitude"))
			{
				OutSlot.Kind = EWacomCardExplanationTemplateSlotKind::EffectMagnitude;
				return true;
			}
			if (SlotType == TEXT("icon") && SlotPath == TEXT("EffectIcon"))
			{
				OutSlot.Kind = EWacomCardExplanationTemplateSlotKind::EffectIcon;
				return true;
			}
			if (SlotType == TEXT("status") && SlotPath == TEXT("EffectStatus"))
			{
				OutSlot.Kind = EWacomCardExplanationTemplateSlotKind::EffectStatus;
				return true;
			}
			if (SlotType == TEXT("keyword") && SlotPath == TEXT("Tag"))
			{
				OutSlot.Kind = EWacomCardExplanationTemplateSlotKind::EffectTag;
				return true;
			}

			SetError(
				OutError,
				FString::Printf(TEXT("效果模板不支持参数：{%s}。"), *Slot));
			return false;
		}

		if (Context == EWacomCardExplanationTemplateContext::Passive)
		{
			if (SlotType == TEXT("value") && SlotPath == TEXT("TriggerThreshold"))
			{
				OutSlot.Kind = EWacomCardExplanationTemplateSlotKind::TriggerThreshold;
				return true;
			}
			return TryParsePassiveEffectPath(
				SlotType,
				SlotPath,
				OutSlot,
				OutError);
		}

		if (Context == EWacomCardExplanationTemplateContext::Keyword)
		{
			if (SlotType == TEXT("keyword") && SlotPath == TEXT("Keyword"))
			{
				OutSlot.Kind = EWacomCardExplanationTemplateSlotKind::Keyword;
				return true;
			}
			SetError(
				OutError,
				FString::Printf(TEXT("关键词模板不支持参数：{%s}。"), *Slot));
			return false;
		}

		if (SlotType == TEXT("status") && SlotPath == TEXT("CountedStatus"))
		{
			OutSlot.Kind = EWacomCardExplanationTemplateSlotKind::DynamicCostStatus;
			return true;
		}
		if (SlotType == TEXT("value")
			&& SlotPath == TEXT("ReductionPerMatchingCard"))
		{
			OutSlot.Kind =
				EWacomCardExplanationTemplateSlotKind::
					DynamicCostReductionPerMatchingCard;
			return true;
		}
		if (SlotType == TEXT("value") && SlotPath == TEXT("MinimumCost"))
		{
			OutSlot.Kind =
				EWacomCardExplanationTemplateSlotKind::DynamicCostMinimumCost;
			return true;
		}
		SetError(
			OutError,
			FString::Printf(TEXT("动态费用模板不支持参数：{%s}。"), *Slot));
		return false;
	}

	void ValidateEffectTemplate(
		const FText& Template,
		const FCardEffect& Effect,
		TArray<FString>& OutErrors)
	{
		ValidateTemplateSyntax(
			Template,
			EWacomCardExplanationTemplateContext::Effect,
			[&Effect](
				const FWacomCardExplanationTemplateSlot& Slot,
				TArray<FString>& Errors)
			{
				if (Slot.Kind == EWacomCardExplanationTemplateSlotKind::EffectStatus
					&& !ResolveEffectStatusTag(Effect).IsValid())
				{
					Errors.Add(FString::Printf(
						TEXT("{status:EffectStatus} 不适用于效果 %s。"),
						*Effect.EffectType.ToString()));
				}
				else if (Slot.Kind == EWacomCardExplanationTemplateSlotKind::EffectTag
					&& !Effect.Target.IsValid())
				{
					Errors.Add(FString::Printf(
						TEXT("{keyword:Tag} 需要效果 %s 配置有效 Target。"),
						*Effect.EffectType.ToString()));
				}
			},
			OutErrors);
	}

	void ValidatePassiveTemplate(
		const FText& Template,
		const FCardPassive& Passive,
		TArray<FString>& OutErrors)
	{
		ValidateTemplateSyntax(
			Template,
			EWacomCardExplanationTemplateContext::Passive,
			[&Passive](
				const FWacomCardExplanationTemplateSlot& Slot,
				TArray<FString>& Errors)
			{
				if (Slot.PassiveEffectIndex == INDEX_NONE)
				{
					return;
				}
				if (!Passive.Effects.IsValidIndex(Slot.PassiveEffectIndex))
				{
					Errors.Add(FString::Printf(
						TEXT("PassiveEffect[%d] 超出当前被动的 Effects 范围（数量 %d）。"),
						Slot.PassiveEffectIndex,
						Passive.Effects.Num()));
					return;
				}
				if (Slot.Kind == EWacomCardExplanationTemplateSlotKind::PassiveEffectStatus
					&& !ResolveEffectStatusTag(
						Passive.Effects[Slot.PassiveEffectIndex]).IsValid())
				{
					Errors.Add(FString::Printf(
						TEXT("{status:PassiveEffect[%d].Status} 指向的效果 %s 不是状态效果。"),
						Slot.PassiveEffectIndex,
						*Passive.Effects[Slot.PassiveEffectIndex].EffectType.ToString()));
				}
			},
			OutErrors);
	}

	void ValidateKeywordTemplate(
		const FText& Template,
		const FGameplayTag Keyword,
		TArray<FString>& OutErrors)
	{
		ValidateTemplateSyntax(
			Template,
			EWacomCardExplanationTemplateContext::Keyword,
			[Keyword](
				const FWacomCardExplanationTemplateSlot& Slot,
				TArray<FString>& Errors)
			{
				if (Slot.Kind == EWacomCardExplanationTemplateSlotKind::Keyword
					&& !Keyword.IsValid())
				{
					Errors.Add(TEXT("{keyword:Keyword} 需要有效的卡牌关键词。"));
				}
			},
			OutErrors);
	}

	void ValidateDynamicCostTemplate(
		const FText& Template,
		const FWacomCardDynamicCostRule& DynamicCostRule,
		TArray<FString>& OutErrors)
	{
		ValidateTemplateSyntax(
			Template,
			EWacomCardExplanationTemplateContext::DynamicCost,
			[&DynamicCostRule](
				const FWacomCardExplanationTemplateSlot& Slot,
				TArray<FString>& Errors)
			{
				if (Slot.Kind
						== EWacomCardExplanationTemplateSlotKind::DynamicCostStatus
					&& !DynamicCostRule.CountHandCardsWithStatus.IsValid())
				{
					Errors.Add(
						TEXT("{status:CountedStatus} 需要动态费用规则配置有效状态。"));
				}
			},
			OutErrors);
	}

	FGameplayTag ResolveEffectStatusTag(const FCardEffect& Effect)
	{
		if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Poison))
		{
			return WacomTags::Status_Poison;
		}
		if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Slow))
		{
			return WacomTags::Status_Slow;
		}
		if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Freeze))
		{
			return WacomTags::Status_Freeze;
		}
		if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Twilight))
		{
			return WacomTags::Status_Twilight;
		}
		if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Burn))
		{
			return WacomTags::Status_Burn;
		}
		if (Effect.EffectType.MatchesTagExact(WacomTags::Status_Shield))
		{
			return WacomTags::Status_Shield;
		}
		return FGameplayTag();
	}
}
