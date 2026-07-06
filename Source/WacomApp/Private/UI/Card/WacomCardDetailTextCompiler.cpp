// Copyright Wacom. All Rights Reserved.

#include "WacomCardDetailTextCompiler.h"

#include "Cards/CardDefinition.h"
#include "Tags/WacomGameplayTags.h"

#define LOCTEXT_NAMESPACE "WacomCardDetailTextCompiler"

namespace WacomCardDetailTextCompiler
{
	namespace
	{
		FString ShortGameplayTagName(const FGameplayTag& Tag)
		{
			FString TagName = Tag.GetTagName().ToString();
			int32 LastDot = INDEX_NONE;
			TagName.FindLastChar(TEXT('.'), LastDot);
			return LastDot != INDEX_NONE ? TagName.Mid(LastDot + 1) : TagName;
		}

		bool UsesRuntimeCostMagnitude(const FCardEffect& Effect)
		{
			return Effect.MagnitudeSource.MatchesTagExact(WacomTags::Magnitude_Source_RuntimeCost)
				|| (!Effect.MagnitudeSource.IsValid() && Effect.bMagnitudeFromRuntimeCost);
		}

		const FWacomCardPresentationRuntimeContext::FEffectPreview* FindEffectPreview(
			const FWacomCardPresentationRuntimeContext& RuntimeContext,
			int32 EffectIndex)
		{
			for (const FWacomCardPresentationRuntimeContext::FEffectPreview& Preview : RuntimeContext.EffectPreviews)
			{
				if (Preview.EffectIndex == EffectIndex)
				{
					return &Preview;
				}
			}
			return nullptr;
		}

		FWacomCardPresentationRuntimeContext MakePassiveEffectRuntimeContext(
			const FWacomCardPresentationRuntimeContext& RuntimeContext)
		{
			FWacomCardPresentationRuntimeContext PassiveContext;
			PassiveContext.bHasRuntimeCost = RuntimeContext.bHasRuntimeCost;
			PassiveContext.RuntimeCost = RuntimeContext.RuntimeCost;
			PassiveContext.bHasPlayableState = RuntimeContext.bHasPlayableState;
			PassiveContext.bIsPlayable = RuntimeContext.bIsPlayable;
			return PassiveContext;
		}

		int32 GetBaseDisplayMagnitude(
			const FCardEffect& Effect,
			const UCardDefinition* Card,
			const FWacomCardPresentationRuntimeContext& RuntimeContext)
		{
			if (UsesRuntimeCostMagnitude(Effect))
			{
				if (RuntimeContext.bHasRuntimeCost)
				{
					return RuntimeContext.RuntimeCost;
				}
				return Card ? Card->BaseCost : Effect.Magnitude;
			}
			return Effect.Magnitude;
		}

		FName BuildTokenStableId(const FString& Prefix, const TCHAR* Suffix)
		{
			return FName(*FString::Printf(TEXT("%s.%s"), *Prefix, Suffix));
		}

		FWacomCardDetailToken MakeTextToken(const FText& Text, FName StableId = NAME_None)
		{
			FWacomCardDetailToken Token;
			Token.StableId = StableId;
			Token.Kind = EWacomCardDetailTokenKind::Text;
			Token.Text = Text;
			return Token;
		}

		FWacomCardDetailToken MakeIconToken(EWacomCardDetailIcon Icon, FName StableId)
		{
			FWacomCardDetailToken Token;
			Token.StableId = StableId;
			Token.Kind = EWacomCardDetailTokenKind::Icon;
			Token.Icon = Icon;
			return Token;
		}

		FWacomCardDetailToken MakeNumberToken(
			int32 Value,
			const FWacomCardPresentationRuntimeContext::FEffectPreview* Preview,
			FName StableId)
		{
			FWacomCardDetailToken Token;
			Token.StableId = StableId;
			Token.Kind = EWacomCardDetailTokenKind::Number;
			Token.Value = Value;
			Token.bHasValue = true;
			if (Preview && !Preview->bSkip && Preview->bHasMagnitude && Preview->Magnitude != Value)
			{
				Token.PreviewValue = Preview->Magnitude;
				Token.bHasPreviewValue = true;
				Token.bEmphasized = true;
			}
			return Token;
		}

		FWacomCardDetailToken MakeActionToken(const FText& Text, FName StableId)
		{
			FWacomCardDetailToken Token;
			Token.StableId = StableId;
			Token.Kind = EWacomCardDetailTokenKind::Text;
			Token.Text = Text;
			Token.bEmphasized = true;
			return Token;
		}

		void AppendAuthoredTextToken(
			const FString& Text,
			const FString& StableIdPrefix,
			int32 LineIndex,
			int32& TokenIndex,
			TArray<FWacomCardDetailToken>& OutTokens)
		{
			if (Text.IsEmpty())
			{
				return;
			}

			OutTokens.Add(MakeTextToken(
				FText::FromString(Text),
				FName(*FString::Printf(TEXT("%s.%d.Token.%d.Text"), *StableIdPrefix, LineIndex, TokenIndex))));
			++TokenIndex;
		}

		bool TryParseEffectTemplatePlaceholder(const FString& Placeholder, int32& OutEffectIndex)
		{
			static const FString Prefix(TEXT("Effect."));
			if (!Placeholder.StartsWith(Prefix))
			{
				return false;
			}

			const FString IndexText = Placeholder.Mid(Prefix.Len());
			if (IndexText.IsEmpty())
			{
				return false;
			}

			for (const TCHAR Character : IndexText)
			{
				if (!FChar::IsDigit(Character))
				{
					return false;
				}
			}

			OutEffectIndex = FCString::Atoi(*IndexText);
			return true;
		}

		bool TryBuildEffectPlaceholderTokens(
			const UCardDefinition* Card,
			const TArray<FCardEffect>& Effects,
			const FWacomCardPresentationRuntimeContext& RuntimeContext,
			int32 EffectIndex,
			const FString& StableIdPrefix,
			int32 LineIndex,
			int32& TokenIndex,
			TArray<FWacomCardDetailToken>& OutTokens)
		{
			if (!Effects.IsValidIndex(EffectIndex))
			{
				return false;
			}

			const FCardEffect& Effect = Effects[EffectIndex];
			EWacomCardDetailIcon Icon = EWacomCardDetailIcon::None;
			bool bHasNumber = true;
			FText ActionText;
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Damage))
			{
				Icon = EWacomCardDetailIcon::Damage;
			}
			else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Heal))
			{
				Icon = EWacomCardDetailIcon::Heal;
			}
			else if (Effect.EffectType.MatchesTagExact(WacomTags::Status_Shield))
			{
				Icon = EWacomCardDetailIcon::Shield;
			}
			else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Poison))
			{
				Icon = EWacomCardDetailIcon::Poison;
			}
			else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Card_AddCost)
				|| Effect.EffectType.MatchesTagExact(WacomTags::Effect_Card_ReduceCost))
			{
				Icon = EWacomCardDetailIcon::Cost;
			}
			else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Card_DiscardSelected))
			{
				Icon = EWacomCardDetailIcon::Discard;
				bHasNumber = false;
				ActionText = LOCTEXT("EffectPlaceholderDiscard", "弃置");
			}
			else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Card_ExhaustSelected))
			{
				Icon = EWacomCardDetailIcon::Exhaust;
				bHasNumber = false;
				ActionText = LOCTEXT("EffectPlaceholderExhaust", "消耗");
			}
			else
			{
				return false;
			}

			const FString Prefix = FString::Printf(
				TEXT("%s.%d.Token.%d.Effect.%d"),
				*StableIdPrefix,
				LineIndex,
				TokenIndex,
				EffectIndex);
			const FWacomCardPresentationRuntimeContext::FEffectPreview* Preview =
				FindEffectPreview(RuntimeContext, EffectIndex);
			const bool bSkipped = Preview && Preview->bSkip;
			FWacomCardDetailToken IconToken = MakeIconToken(Icon, BuildTokenStableId(Prefix, TEXT("Icon")));
			IconToken.bSkipped = bSkipped;
			OutTokens.Add(MoveTemp(IconToken));

			FWacomCardDetailToken GapToken = MakeTextToken(
				FText::FromString(TEXT(" ")),
				BuildTokenStableId(Prefix, TEXT("Gap")));
			GapToken.bSkipped = bSkipped;
			OutTokens.Add(MoveTemp(GapToken));

			if (bHasNumber)
			{
				FWacomCardDetailToken NumberToken = MakeNumberToken(
					GetBaseDisplayMagnitude(Effect, Card, RuntimeContext),
					Preview,
					BuildTokenStableId(Prefix, TEXT("Magnitude")));
				NumberToken.bSkipped = bSkipped;
				OutTokens.Add(MoveTemp(NumberToken));
			}
			else if (!ActionText.IsEmpty())
			{
				FWacomCardDetailToken ActionToken = MakeActionToken(
					ActionText,
					BuildTokenStableId(Prefix, TEXT("Action")));
				ActionToken.bSkipped = bSkipped;
				OutTokens.Add(MoveTemp(ActionToken));
			}

			++TokenIndex;
			return true;
		}

		FWacomCardDetailTokenLine MakeAuthoredTextTokenLine(
			const UCardDefinition* Card,
			const TArray<FCardEffect>& Effects,
			const FWacomCardPresentationRuntimeContext& RuntimeContext,
			const FString& LineText,
			EWacomCardDetailTokenLineKind LineKind,
			const FString& StableIdPrefix,
			int32 LineIndex)
		{
			FWacomCardDetailTokenLine Line;
			Line.LineId = FName(*FString::Printf(TEXT("%s.%d.Line"), *StableIdPrefix, LineIndex));
			Line.Kind = LineKind;

			int32 TokenIndex = 0;
			int32 Cursor = 0;
			while (Cursor < LineText.Len())
			{
				const int32 OpenIndex = LineText.Find(TEXT("{"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Cursor);
				if (OpenIndex == INDEX_NONE)
				{
					AppendAuthoredTextToken(LineText.Mid(Cursor), StableIdPrefix, LineIndex, TokenIndex, Line.Tokens);
					break;
				}

				AppendAuthoredTextToken(
					LineText.Mid(Cursor, OpenIndex - Cursor),
					StableIdPrefix,
					LineIndex,
					TokenIndex,
					Line.Tokens);

				const int32 CloseIndex = LineText.Find(TEXT("}"), ESearchCase::CaseSensitive, ESearchDir::FromStart, OpenIndex + 1);
				if (CloseIndex == INDEX_NONE)
				{
					AppendAuthoredTextToken(LineText.Mid(OpenIndex), StableIdPrefix, LineIndex, TokenIndex, Line.Tokens);
					break;
				}

				const FString Placeholder = LineText.Mid(OpenIndex + 1, CloseIndex - OpenIndex - 1);
				int32 EffectIndex = INDEX_NONE;
				const bool bReplacedPlaceholder =
					TryParseEffectTemplatePlaceholder(Placeholder, EffectIndex)
					&& TryBuildEffectPlaceholderTokens(
						Card,
						Effects,
						RuntimeContext,
						EffectIndex,
						StableIdPrefix,
						LineIndex,
						TokenIndex,
						Line.Tokens);
				if (!bReplacedPlaceholder)
				{
					AppendAuthoredTextToken(
						LineText.Mid(OpenIndex, CloseIndex - OpenIndex + 1),
						StableIdPrefix,
						LineIndex,
						TokenIndex,
						Line.Tokens);
				}

				Cursor = CloseIndex + 1;
			}

			return Line;
		}

		FText BuildPassiveTriggerText(const FCardPassive& Passive)
		{
			if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_AfterPlayed))
			{
				return LOCTEXT("PassiveTriggerAfterPlayed", "打出后");
			}
			if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnCompanionCount))
			{
				return Passive.TriggerThreshold > 0
					? FText::Format(LOCTEXT("PassiveTriggerOnCompanionCountFmt", "每打出{0}张伙伴"), FText::AsNumber(Passive.TriggerThreshold))
					: LOCTEXT("PassiveTriggerOnCompanionCount", "打出伙伴计数");
			}
			if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnTwilightTriggered))
			{
				return LOCTEXT("PassiveTriggerOnTwilightTriggered", "暮气触发时");
			}
			if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnTurnStart))
			{
				return LOCTEXT("PassiveTriggerOnTurnStart", "回合开始");
			}
			if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnTurnEnd))
			{
				return LOCTEXT("PassiveTriggerOnTurnEnd", "回合结束");
			}
			if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnDraw))
			{
				return LOCTEXT("PassiveTriggerOnDraw", "抽到时");
			}
			if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnDiscard))
			{
				return LOCTEXT("PassiveTriggerOnDiscard", "弃掉时");
			}

			return Passive.Trigger.IsValid()
				? FText::FromString(ShortGameplayTagName(Passive.Trigger))
				: LOCTEXT("PassiveTriggerUnknown", "触发时机未知");
		}

		FText BuildPassiveTriggerDisplayText(const FCardPassive& Passive)
		{
			if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_AfterPlayed))
			{
				return LOCTEXT("PassiveTokenTriggerAfterPlayed", "打出后");
			}
			if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnCompanionCount))
			{
				return Passive.TriggerThreshold > 0
					? FText::Format(LOCTEXT("PassiveTokenTriggerOnCompanionCountFmt", "每打出 {0} 张伙伴"), FText::AsNumber(Passive.TriggerThreshold))
					: LOCTEXT("PassiveTokenTriggerOnCompanionCount", "打出伙伴计数");
			}
			if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnTwilightTriggered))
			{
				return LOCTEXT("PassiveTokenTriggerOnTwilightTriggered", "暮气触发时");
			}
			if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnTurnStart))
			{
				return LOCTEXT("PassiveTokenTriggerOnTurnStart", "回合开始");
			}
			if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnTurnEnd))
			{
				return LOCTEXT("PassiveTokenTriggerOnTurnEnd", "回合结束");
			}
			if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnDraw))
			{
				return LOCTEXT("PassiveTokenTriggerOnDraw", "抽到时");
			}
			if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnDiscard))
			{
				return LOCTEXT("PassiveTokenTriggerOnDiscard", "弃掉时");
			}

			return Passive.Trigger.IsValid()
				? FText::FromString(ShortGameplayTagName(Passive.Trigger))
				: LOCTEXT("PassiveTokenTriggerUnknown", "被动");
		}

		FText BuildPassiveLine(const FCardPassive& Passive)
		{
			FText TriggerText = BuildPassiveTriggerText(Passive);
			if (Passive.Effects.Num() <= 0)
			{
				return TriggerText;
			}

			return FText::Format(
				LOCTEXT("PassiveLineWithEffectCountFmt", "{0}（效果 {1}）"),
				TriggerText,
				FText::AsNumber(Passive.Effects.Num()));
		}

		FText NormalizePassiveBodyText(const FText& Text)
		{
			FString Value = Text.ToString().TrimStartAndEnd();
			const FString FullWidthLabel(TEXT("被动："));
			const FString HalfWidthLabel(TEXT("被动:"));
			if (Value.StartsWith(FullWidthLabel))
			{
				Value.RightChopInline(FullWidthLabel.Len(), EAllowShrinking::No);
				Value.TrimStartInline();
			}
			else if (Value.StartsWith(HalfWidthLabel))
			{
				Value.RightChopInline(HalfWidthLabel.Len(), EAllowShrinking::No);
				Value.TrimStartInline();
			}
			return Value.IsEmpty() ? FText::GetEmpty() : FText::FromString(Value);
		}

		bool TryBuildEffectTokenLine(
			const FCardEffect& Effect,
			const UCardDefinition* Card,
			const FWacomCardPresentationRuntimeContext& RuntimeContext,
			int32 EffectIndex,
			const FString& StableIdPrefix,
			EWacomCardDetailTokenLineKind LineKind,
			FWacomCardDetailTokenLine& OutLine)
		{
			const FWacomCardPresentationRuntimeContext::FEffectPreview* Preview =
				FindEffectPreview(RuntimeContext, EffectIndex);
			const bool bSkipped = Preview && Preview->bSkip;
			const int32 Value = GetBaseDisplayMagnitude(Effect, Card, RuntimeContext);

			OutLine = FWacomCardDetailTokenLine();
			OutLine.LineId = BuildTokenStableId(StableIdPrefix, TEXT("Line"));
			OutLine.Kind = LineKind;

			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Damage))
			{
				OutLine.Tokens.Add(MakeTextToken(LOCTEXT("DetailTokenDamagePrefix", "造成 "), BuildTokenStableId(StableIdPrefix, TEXT("Prefix"))));
				OutLine.Tokens.Add(MakeIconToken(EWacomCardDetailIcon::Damage, BuildTokenStableId(StableIdPrefix, TEXT("Icon"))));
				OutLine.Tokens.Add(MakeTextToken(FText::FromString(TEXT(" ")), BuildTokenStableId(StableIdPrefix, TEXT("IconGap"))));
				OutLine.Tokens.Add(MakeNumberToken(Value, Preview, BuildTokenStableId(StableIdPrefix, TEXT("Magnitude"))));
				OutLine.Tokens.Add(MakeTextToken(LOCTEXT("DetailTokenDamageSuffix", " 点伤害。"), BuildTokenStableId(StableIdPrefix, TEXT("Suffix"))));
			}
			else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Heal))
			{
				OutLine.Tokens.Add(MakeTextToken(LOCTEXT("DetailTokenHealPrefix", "恢复 "), BuildTokenStableId(StableIdPrefix, TEXT("Prefix"))));
				OutLine.Tokens.Add(MakeIconToken(EWacomCardDetailIcon::Heal, BuildTokenStableId(StableIdPrefix, TEXT("Icon"))));
				OutLine.Tokens.Add(MakeTextToken(FText::FromString(TEXT(" ")), BuildTokenStableId(StableIdPrefix, TEXT("IconGap"))));
				OutLine.Tokens.Add(MakeNumberToken(Value, Preview, BuildTokenStableId(StableIdPrefix, TEXT("Magnitude"))));
				OutLine.Tokens.Add(MakeTextToken(LOCTEXT("DetailTokenHealSuffix", " 点生命。"), BuildTokenStableId(StableIdPrefix, TEXT("Suffix"))));
			}
			else if (Effect.EffectType.MatchesTagExact(WacomTags::Status_Shield))
			{
				OutLine.Tokens.Add(MakeTextToken(LOCTEXT("DetailTokenShieldPrefix", "获得 "), BuildTokenStableId(StableIdPrefix, TEXT("Prefix"))));
				OutLine.Tokens.Add(MakeIconToken(EWacomCardDetailIcon::Shield, BuildTokenStableId(StableIdPrefix, TEXT("Icon"))));
				OutLine.Tokens.Add(MakeTextToken(FText::FromString(TEXT(" ")), BuildTokenStableId(StableIdPrefix, TEXT("IconGap"))));
				OutLine.Tokens.Add(MakeNumberToken(Value, Preview, BuildTokenStableId(StableIdPrefix, TEXT("Magnitude"))));
				OutLine.Tokens.Add(MakeTextToken(LOCTEXT("DetailTokenShieldSuffix", " 点护盾。"), BuildTokenStableId(StableIdPrefix, TEXT("Suffix"))));
			}
			else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Poison))
			{
				OutLine.Tokens.Add(MakeTextToken(LOCTEXT("DetailTokenPoisonPrefix", "施加 "), BuildTokenStableId(StableIdPrefix, TEXT("Prefix"))));
				OutLine.Tokens.Add(MakeIconToken(EWacomCardDetailIcon::Poison, BuildTokenStableId(StableIdPrefix, TEXT("Icon"))));
				OutLine.Tokens.Add(MakeTextToken(FText::FromString(TEXT(" ")), BuildTokenStableId(StableIdPrefix, TEXT("IconGap"))));
				OutLine.Tokens.Add(MakeNumberToken(Value, Preview, BuildTokenStableId(StableIdPrefix, TEXT("Magnitude"))));
				OutLine.Tokens.Add(MakeTextToken(LOCTEXT("DetailTokenPoisonSuffix", " 层中毒。"), BuildTokenStableId(StableIdPrefix, TEXT("Suffix"))));
			}
			else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Card_AddCost))
			{
				OutLine.Tokens.Add(MakeTextToken(LOCTEXT("DetailTokenAddCostPrefix", "目标手牌费用增加 "), BuildTokenStableId(StableIdPrefix, TEXT("Prefix"))));
				OutLine.Tokens.Add(MakeIconToken(EWacomCardDetailIcon::Cost, BuildTokenStableId(StableIdPrefix, TEXT("Icon"))));
				OutLine.Tokens.Add(MakeTextToken(FText::FromString(TEXT(" ")), BuildTokenStableId(StableIdPrefix, TEXT("IconGap"))));
				OutLine.Tokens.Add(MakeNumberToken(Value, Preview, BuildTokenStableId(StableIdPrefix, TEXT("Magnitude"))));
				OutLine.Tokens.Add(MakeTextToken(FText::FromString(TEXT("。")), BuildTokenStableId(StableIdPrefix, TEXT("Suffix"))));
			}
			else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Card_ReduceCost))
			{
				OutLine.Tokens.Add(MakeTextToken(LOCTEXT("DetailTokenReduceCostPrefix", "目标手牌费用降低 "), BuildTokenStableId(StableIdPrefix, TEXT("Prefix"))));
				OutLine.Tokens.Add(MakeIconToken(EWacomCardDetailIcon::Cost, BuildTokenStableId(StableIdPrefix, TEXT("Icon"))));
				OutLine.Tokens.Add(MakeTextToken(FText::FromString(TEXT(" ")), BuildTokenStableId(StableIdPrefix, TEXT("IconGap"))));
				OutLine.Tokens.Add(MakeNumberToken(Value, Preview, BuildTokenStableId(StableIdPrefix, TEXT("Magnitude"))));
				OutLine.Tokens.Add(MakeTextToken(FText::FromString(TEXT("。")), BuildTokenStableId(StableIdPrefix, TEXT("Suffix"))));
			}
			else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Card_DiscardSelected))
			{
				OutLine.Tokens.Add(MakeIconToken(EWacomCardDetailIcon::Discard, BuildTokenStableId(StableIdPrefix, TEXT("Icon"))));
				OutLine.Tokens.Add(MakeTextToken(FText::FromString(TEXT(" ")), BuildTokenStableId(StableIdPrefix, TEXT("IconGap"))));
				OutLine.Tokens.Add(MakeActionToken(LOCTEXT("DetailTokenDiscardSelected", "弃置目标手牌。"), BuildTokenStableId(StableIdPrefix, TEXT("Action"))));
			}
			else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Card_ExhaustSelected))
			{
				OutLine.Tokens.Add(MakeIconToken(EWacomCardDetailIcon::Exhaust, BuildTokenStableId(StableIdPrefix, TEXT("Icon"))));
				OutLine.Tokens.Add(MakeTextToken(FText::FromString(TEXT(" ")), BuildTokenStableId(StableIdPrefix, TEXT("IconGap"))));
				OutLine.Tokens.Add(MakeActionToken(LOCTEXT("DetailTokenExhaustSelected", "消耗目标手牌。"), BuildTokenStableId(StableIdPrefix, TEXT("Action"))));
			}
			else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Shuffle_Random))
			{
				OutLine.Tokens.Add(MakeActionToken(LOCTEXT("DetailTokenShuffleRandom", "随机腾挪 1 张手牌。"), BuildTokenStableId(StableIdPrefix, TEXT("Action"))));
			}
			else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Shuffle_FromBothToOther))
			{
				OutLine.Tokens.Add(MakeActionToken(LOCTEXT("DetailTokenShuffleFromBothToOther", "将双手区随机 1 张卡牌腾挪至其他区域。"), BuildTokenStableId(StableIdPrefix, TEXT("Action"))));
			}
			else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Shuffle_ToRandomZone))
			{
				OutLine.Tokens.Add(MakeActionToken(LOCTEXT("DetailTokenShuffleToRandomZone", "该牌腾挪至随机区域。"), BuildTokenStableId(StableIdPrefix, TEXT("Action"))));
			}
			else
			{
				return false;
			}

			if (bSkipped)
			{
				for (FWacomCardDetailToken& Token : OutLine.Tokens)
				{
					Token.bSkipped = true;
				}
			}
			return true;
		}

		bool TryBuildPassiveTriggerTokenLine(
			const FCardPassive& Passive,
			int32 PassiveIndex,
			FWacomCardDetailTokenLine& OutLine)
		{
			OutLine = FWacomCardDetailTokenLine();
			OutLine.LineId = FName(*FString::Printf(TEXT("Passive.%d.Trigger"), PassiveIndex));
			OutLine.Kind = EWacomCardDetailTokenLineKind::Passive;
			OutLine.Tokens.Add(MakeActionToken(
				BuildPassiveTriggerDisplayText(Passive),
				FName(*FString::Printf(TEXT("Passive.%d.Trigger.Text"), PassiveIndex))));
			if (Passive.Condition.IsSet())
			{
				OutLine.Tokens.Add(MakeTextToken(
					LOCTEXT("PassiveTokenConditionHint", "（有条件）"),
					FName(*FString::Printf(TEXT("Passive.%d.Trigger.ConditionHint"), PassiveIndex))));
			}
			OutLine.Tokens.Add(MakeTextToken(
				LOCTEXT("PassiveTokenTriggerSuffix", "："),
				FName(*FString::Printf(TEXT("Passive.%d.Trigger.Suffix"), PassiveIndex))));
			return true;
		}

		FWacomCardDetailTokenLine BuildPlainTextTokenLine(
			const FText& Text,
			EWacomCardDetailTokenLineKind Kind,
			const FString& StableIdPrefix,
			int32 LineIndex)
		{
			FWacomCardDetailTokenLine Line;
			Line.LineId = FName(*FString::Printf(TEXT("%s.%d.Line"), *StableIdPrefix, LineIndex));
			Line.Kind = Kind;
			Line.Tokens.Add(MakeTextToken(
				Text,
				FName(*FString::Printf(TEXT("%s.%d.Text"), *StableIdPrefix, LineIndex))));
			return Line;
		}
	}

	TArray<FWacomCardDetailTokenLine> BuildAuthoredTextTokenLines(
		const UCardDefinition* Card,
		const TArray<FCardEffect>& Effects,
		const FWacomCardPresentationRuntimeContext& RuntimeContext,
		const FText& Text,
		EWacomCardDetailTokenLineKind LineKind,
		const FString& StableIdPrefix)
	{
		TArray<FWacomCardDetailTokenLine> TokenLines;
		if (Text.IsEmpty())
		{
			return TokenLines;
		}

		FString SourceText = Text.ToString();
		SourceText.ReplaceInline(TEXT("\r\n"), TEXT("\n"));
		SourceText.ReplaceInline(TEXT("\r"), TEXT("\n"));

		TArray<FString> Lines;
		SourceText.ParseIntoArrayLines(Lines, false);
		for (int32 LineIndex = 0; LineIndex < Lines.Num(); ++LineIndex)
		{
			FWacomCardDetailTokenLine Line = MakeAuthoredTextTokenLine(
				Card,
				Effects,
				RuntimeContext,
				Lines[LineIndex],
				LineKind,
				StableIdPrefix,
				LineIndex);
			if (!Line.Tokens.IsEmpty())
			{
				TokenLines.Add(MoveTemp(Line));
			}
		}

		return TokenLines;
	}

	TArray<FWacomCardDetailTokenLine> BuildEffectTokenLines(
		const UCardDefinition* Card,
		const FWacomCardPresentationRuntimeContext& RuntimeContext)
	{
		TArray<FWacomCardDetailTokenLine> Lines;
		if (!Card)
		{
			return Lines;
		}

		for (int32 EffectIndex = 0; EffectIndex < Card->Effects.Num(); ++EffectIndex)
		{
			FWacomCardDetailTokenLine Line;
			if (TryBuildEffectTokenLine(
				Card->Effects[EffectIndex],
				Card,
				RuntimeContext,
				EffectIndex,
				FString::Printf(TEXT("Effect.%d"), EffectIndex),
				EWacomCardDetailTokenLineKind::Effect,
				Line))
			{
				Lines.Add(MoveTemp(Line));
			}
		}
		return Lines;
	}

	bool BuildPassiveTokenLines(
		const UCardDefinition* Card,
		const FWacomCardPresentationRuntimeContext& RuntimeContext,
		TArray<FWacomCardDetailTokenLine>& OutLines,
		TArray<FText>& OutFallbackPassiveLines)
	{
		if (!Card)
		{
			return false;
		}

		bool bAnyPassiveTokenLines = false;
		const FWacomCardPresentationRuntimeContext PassiveRuntimeContext =
			MakePassiveEffectRuntimeContext(RuntimeContext);
		for (int32 PassiveIndex = 0; PassiveIndex < Card->Passives.Num(); ++PassiveIndex)
		{
			const FCardPassive& Passive = Card->Passives[PassiveIndex];
			const FText PassiveBodyText = NormalizePassiveBodyText(
				Passive.DisplayText.IsEmpty()
					? BuildPassiveLine(Passive)
					: Passive.DisplayText);
			if (!PassiveBodyText.IsEmpty())
			{
				OutFallbackPassiveLines.Add(PassiveBodyText);
			}

			if (!Passive.DisplayText.IsEmpty())
			{
				TArray<FWacomCardDetailTokenLine> DisplayLines = BuildAuthoredTextTokenLines(
					Card,
					Passive.Effects,
					PassiveRuntimeContext,
					PassiveBodyText,
					EWacomCardDetailTokenLineKind::Passive,
					FString::Printf(TEXT("Passive.%d.Display"), PassiveIndex));
				if (!DisplayLines.IsEmpty())
				{
					OutLines.Append(MoveTemp(DisplayLines));
					bAnyPassiveTokenLines = true;
				}
				continue;
			}

			TArray<FWacomCardDetailTokenLine> EffectLines;
			for (int32 EffectIndex = 0; EffectIndex < Passive.Effects.Num(); ++EffectIndex)
			{
				FWacomCardDetailTokenLine EffectLine;
				if (TryBuildEffectTokenLine(
					Passive.Effects[EffectIndex],
					Card,
					PassiveRuntimeContext,
					EffectIndex,
					FString::Printf(TEXT("Passive.%d.Effect.%d"), PassiveIndex, EffectIndex),
					EWacomCardDetailTokenLineKind::Passive,
					EffectLine))
				{
					EffectLines.Add(MoveTemp(EffectLine));
				}
			}

			if (Passive.Effects.Num() > 0 && EffectLines.Num() == Passive.Effects.Num())
			{
				FWacomCardDetailTokenLine TriggerLine;
				if (TryBuildPassiveTriggerTokenLine(Passive, PassiveIndex, TriggerLine))
				{
					OutLines.Add(MoveTemp(TriggerLine));
				}
				for (FWacomCardDetailTokenLine& EffectLine : EffectLines)
				{
					OutLines.Add(MoveTemp(EffectLine));
				}
				bAnyPassiveTokenLines = true;
			}
			else if (!PassiveBodyText.IsEmpty())
			{
				TArray<FWacomCardDetailTokenLine> FallbackLines = BuildAuthoredTextTokenLines(
					Card,
					Passive.Effects,
					PassiveRuntimeContext,
					PassiveBodyText,
					EWacomCardDetailTokenLineKind::Passive,
					FString::Printf(TEXT("Passive.%d.Fallback"), PassiveIndex));
				if (!FallbackLines.IsEmpty())
				{
					OutLines.Append(MoveTemp(FallbackLines));
					bAnyPassiveTokenLines = true;
				}
			}
		}
		return bAnyPassiveTokenLines;
	}

	TArray<FWacomCardDetailTokenLine> BuildPlainTextTokenLines(
		const TArray<FText>& Lines,
		EWacomCardDetailTokenLineKind Kind,
		const FString& StableIdPrefix)
	{
		TArray<FWacomCardDetailTokenLine> TokenLines;
		int32 LineIndex = 0;
		for (const FText& LineText : Lines)
		{
			if (!LineText.IsEmpty())
			{
				TokenLines.Add(BuildPlainTextTokenLine(LineText, Kind, StableIdPrefix, LineIndex));
				++LineIndex;
			}
		}
		return TokenLines;
	}

	void AddCardDetailSection(
		FWacomCardDetailViewData& Data,
		FName SectionId,
		EWacomCardDetailSectionKind Kind,
		const FText& Title,
		TArray<FWacomCardDetailTokenLine>&& TokenLines)
	{
		TArray<FWacomCardDetailTokenLine> NonEmptyLines;
		for (FWacomCardDetailTokenLine& Line : TokenLines)
		{
			if (!Line.Tokens.IsEmpty())
			{
				NonEmptyLines.Add(MoveTemp(Line));
			}
		}
		if (NonEmptyLines.IsEmpty())
		{
			return;
		}

		FWacomCardDetailSection Section;
		Section.SectionId = SectionId;
		Section.Kind = Kind;
		Section.Title = Title;
		Section.TokenLines = MoveTemp(NonEmptyLines);
		Data.Sections.Add(MoveTemp(Section));
	}
}

#undef LOCTEXT_NAMESPACE
