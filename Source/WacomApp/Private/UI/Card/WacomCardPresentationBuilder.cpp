// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomCardPresentationBuilder.h"

#include "Cards/CardDefinition.h"
#include "Tags/WacomGameplayTags.h"

#define LOCTEXT_NAMESPACE "WacomCardPresentationBuilder"

namespace
{
	FText GetCardDisplayName(const UCardDefinition* Card)
	{
		if (!Card)
		{
			return LOCTEXT("UnknownCardName", "未知卡牌");
		}
		return Card->DisplayName.IsEmpty()
			? FText::FromName(Card->CardId)
			: Card->DisplayName;
	}

	FString ShortGameplayTagName(const FGameplayTag& Tag)
	{
		FString TagName = Tag.GetTagName().ToString();
		int32 LastDot = INDEX_NONE;
		TagName.FindLastChar(TEXT('.'), LastDot);
		return LastDot != INDEX_NONE ? TagName.Mid(LastDot + 1) : TagName;
	}

	FString GetKeywordDisplayName(const FGameplayTag& Tag)
	{
		if (Tag.MatchesTagExact(WacomTags::Card_Keyword_Swift))          { return TEXT("迅捷"); }
		if (Tag.MatchesTagExact(WacomTags::Card_Keyword_Retain))         { return TEXT("保留"); }
		if (Tag.MatchesTagExact(WacomTags::Card_Keyword_Combo))          { return TEXT("连击"); }
		if (Tag.MatchesTagExact(WacomTags::Card_Keyword_Companion))      { return TEXT("伙伴"); }
		if (Tag.MatchesTagExact(WacomTags::Card_Keyword_Weapon))         { return TEXT("武器"); }
		if (Tag.MatchesTagExact(WacomTags::Card_Keyword_Tool))           { return TEXT("工具"); }
		if (Tag.MatchesTagExact(WacomTags::Card_Keyword_Hand))           { return TEXT("手"); }
		if (Tag.MatchesTagExact(WacomTags::Card_Keyword_Exhaust))        { return TEXT("消耗"); }
		if (Tag.MatchesTagExact(WacomTags::Card_Keyword_BagProvider))    { return TEXT("容器"); }
		if (Tag.MatchesTagExact(WacomTags::Card_Keyword_DeleteProvider)) { return TEXT("删牌"); }
		return ShortGameplayTagName(Tag);
	}

	FText BuildTypeLine(const UCardDefinition* Card)
	{
		if (!Card)
		{
			return FText::GetEmpty();
		}

		if (Card->Physique.Capacity > 0)
		{
			return Card->Physique.CapacityEffect.IsValid()
				? LOCTEXT("TypeContainerB", "容器")
				: LOCTEXT("TypeContainerA", "背包");
		}

		TArray<FString> KeywordNames;
		for (const FGameplayTag& Tag : Card->Keywords)
		{
			KeywordNames.Add(GetKeywordDisplayName(Tag));
		}

		return KeywordNames.Num() > 0
			? FText::FromString(FString::Join(KeywordNames, TEXT(" / ")))
			: FText::GetEmpty();
	}

	FText BuildCompactDescriptionText(const UCardDefinition* Card)
	{
		if (!Card || Card->Description.IsEmpty())
		{
			return FText::GetEmpty();
		}

		FString Text = Card->Description.ToString();
		Text.ReplaceInline(TEXT("\r\n"), TEXT("\n"));
		Text.ReplaceInline(TEXT("\r"), TEXT("\n"));

		int32 FirstBreak = INDEX_NONE;
		if (Text.FindChar(TEXT('\n'), FirstBreak))
		{
			Text = Text.Left(FirstBreak);
		}

		constexpr int32 MaxChars = 28;
		if (Text.Len() > MaxChars)
		{
			Text = Text.Left(MaxChars).TrimEnd() + TEXT("...");
		}

		return FText::FromString(Text);
	}

	int32 GetDeleteValueFromRarity(const UCardDefinition* Card)
	{
		if (!Card)
		{
			return 0;
		}
		if (Card->Rarity.MatchesTagExact(WacomTags::Card_Rarity_White))
		{
			return 1;
		}
		if (Card->Rarity.MatchesTagExact(WacomTags::Card_Rarity_Blue))
		{
			return 2;
		}
		return 0;
	}

	FText BuildPhysiqueText(const UCardDefinition* Card)
	{
		if (!Card)
		{
			return FText::GetEmpty();
		}

		TArray<FString> Parts;
		if (Card->Physique.Durability > 0)
		{
			Parts.Add(FString::Printf(TEXT("%d耐久"), Card->Physique.Durability));
		}
		if (Card->Physique.Capacity > 0)
		{
			Parts.Add(FString::Printf(TEXT("%d容量"), Card->Physique.Capacity));
		}
		if (Card->Physique.MaxHpBonus > 0)
		{
			Parts.Add(FString::Printf(TEXT("+%d生命"), Card->Physique.MaxHpBonus));
		}

		return Parts.Num() > 0
			? FText::FromString(FString::Join(Parts, TEXT("/")))
			: FText::GetEmpty();
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

	int32 GetPreviewDisplayMagnitude(
		const FCardEffect& Effect,
		const UCardDefinition* Card,
		const FWacomCardPresentationRuntimeContext& RuntimeContext,
		int32 EffectIndex)
	{
		if (const FWacomCardPresentationRuntimeContext::FEffectPreview* Preview =
			FindEffectPreview(RuntimeContext, EffectIndex))
		{
			if (Preview->bHasMagnitude)
			{
				return Preview->Magnitude;
			}
		}

		return GetBaseDisplayMagnitude(Effect, Card, RuntimeContext);
	}

	FName BuildEffectTokenStableId(int32 EffectIndex, const TCHAR* Suffix)
	{
		return FName(*FString::Printf(TEXT("Effect.%d.%s"), EffectIndex, Suffix));
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

	FName BuildDescriptionTokenStableId(int32 LineIndex, int32 TokenIndex, const TCHAR* Suffix)
	{
		return FName(*FString::Printf(TEXT("Description.%d.Token.%d.%s"), LineIndex, TokenIndex, Suffix));
	}

	void AppendDescriptionTextToken(
		const FString& Text,
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
			BuildDescriptionTokenStableId(LineIndex, TokenIndex, TEXT("Text"))));
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
		const FWacomCardPresentationRuntimeContext& RuntimeContext,
		int32 EffectIndex,
		int32 LineIndex,
		int32& TokenIndex,
		TArray<FWacomCardDetailToken>& OutTokens)
	{
		if (!Card || !Card->Effects.IsValidIndex(EffectIndex))
		{
			return false;
		}

		const FCardEffect& Effect = Card->Effects[EffectIndex];
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
			ActionText = LOCTEXT("DescriptionEffectPlaceholderDiscard", "弃置");
		}
		else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Card_ExhaustSelected))
		{
			Icon = EWacomCardDetailIcon::Exhaust;
			bHasNumber = false;
			ActionText = LOCTEXT("DescriptionEffectPlaceholderExhaust", "消耗");
		}
		else
		{
			return false;
		}

		const FString Prefix = FString::Printf(TEXT("Description.%d.Token.%d.Effect.%d"), LineIndex, TokenIndex, EffectIndex);
		const FWacomCardPresentationRuntimeContext::FEffectPreview* Preview =
			FindEffectPreview(RuntimeContext, EffectIndex);
		const bool bSkipped = Preview && Preview->bSkip;
		FWacomCardDetailToken IconToken = MakeIconToken(Icon, BuildTokenStableId(Prefix, TEXT("Icon")));
		IconToken.bSkipped = bSkipped;
		OutTokens.Add(MoveTemp(IconToken));

		if (bHasNumber)
		{
			FWacomCardDetailToken GapToken = MakeTextToken(
				FText::FromString(TEXT(" ")),
				BuildTokenStableId(Prefix, TEXT("Gap")));
			GapToken.bSkipped = bSkipped;
			OutTokens.Add(MoveTemp(GapToken));

			FWacomCardDetailToken NumberToken = MakeNumberToken(
				GetBaseDisplayMagnitude(Effect, Card, RuntimeContext),
				Preview,
				BuildTokenStableId(Prefix, TEXT("Magnitude")));
			NumberToken.bSkipped = bSkipped;
			OutTokens.Add(MoveTemp(NumberToken));
		}
		else if (!ActionText.IsEmpty())
		{
			FWacomCardDetailToken GapToken = MakeTextToken(
				FText::FromString(TEXT(" ")),
				BuildTokenStableId(Prefix, TEXT("Gap")));
			GapToken.bSkipped = bSkipped;
			OutTokens.Add(MoveTemp(GapToken));

			FWacomCardDetailToken ActionToken = MakeActionToken(
				ActionText,
				BuildTokenStableId(Prefix, TEXT("Action")));
			ActionToken.bSkipped = bSkipped;
			OutTokens.Add(MoveTemp(ActionToken));
		}

		++TokenIndex;
		return true;
	}

	FWacomCardDetailTokenLine MakeDescriptionTemplateTokenLine(
		const UCardDefinition* Card,
		const FWacomCardPresentationRuntimeContext& RuntimeContext,
		const FString& LineText,
		int32 LineIndex)
	{
		FWacomCardDetailTokenLine Line;
		Line.LineId = FName(*FString::Printf(TEXT("Description.%d.Line"), LineIndex));
		Line.Kind = EWacomCardDetailTokenLineKind::Description;

		int32 TokenIndex = 0;
		int32 Cursor = 0;
		while (Cursor < LineText.Len())
		{
			const int32 OpenIndex = LineText.Find(TEXT("{"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Cursor);
			if (OpenIndex == INDEX_NONE)
			{
				AppendDescriptionTextToken(LineText.Mid(Cursor), LineIndex, TokenIndex, Line.Tokens);
				break;
			}

			AppendDescriptionTextToken(LineText.Mid(Cursor, OpenIndex - Cursor), LineIndex, TokenIndex, Line.Tokens);

			const int32 CloseIndex = LineText.Find(TEXT("}"), ESearchCase::CaseSensitive, ESearchDir::FromStart, OpenIndex + 1);
			if (CloseIndex == INDEX_NONE)
			{
				AppendDescriptionTextToken(LineText.Mid(OpenIndex), LineIndex, TokenIndex, Line.Tokens);
				break;
			}

			const FString Placeholder = LineText.Mid(OpenIndex + 1, CloseIndex - OpenIndex - 1);
			int32 EffectIndex = INDEX_NONE;
			const bool bReplacedPlaceholder =
				TryParseEffectTemplatePlaceholder(Placeholder, EffectIndex)
				&& TryBuildEffectPlaceholderTokens(Card, RuntimeContext, EffectIndex, LineIndex, TokenIndex, Line.Tokens);
			if (!bReplacedPlaceholder)
			{
				AppendDescriptionTextToken(
					LineText.Mid(OpenIndex, CloseIndex - OpenIndex + 1),
					LineIndex,
					TokenIndex,
					Line.Tokens);
			}

			Cursor = CloseIndex + 1;
		}

		return Line;
	}

	FText BuildEffectBadgeText(EWacomCardViewEffectBadgeKind Kind, int32 Value)
	{
		switch (Kind)
		{
		case EWacomCardViewEffectBadgeKind::Damage:
			return FText::Format(LOCTEXT("DamageBadgeFmt", "伤{0}"), FText::AsNumber(Value));
		case EWacomCardViewEffectBadgeKind::Heal:
			return FText::Format(LOCTEXT("HealBadgeFmt", "疗{0}"), FText::AsNumber(Value));
		case EWacomCardViewEffectBadgeKind::Poison:
			return FText::Format(LOCTEXT("PoisonBadgeFmt", "毒{0}"), FText::AsNumber(Value));
		case EWacomCardViewEffectBadgeKind::Burn:
			return FText::Format(LOCTEXT("BurnBadgeFmt", "灼{0}"), FText::AsNumber(Value));
		case EWacomCardViewEffectBadgeKind::Slow:
			return FText::Format(LOCTEXT("SlowBadgeFmt", "缓{0}"), FText::AsNumber(Value));
		case EWacomCardViewEffectBadgeKind::Freeze:
			return FText::Format(LOCTEXT("FreezeBadgeFmt", "冻{0}"), FText::AsNumber(Value));
		case EWacomCardViewEffectBadgeKind::Twilight:
			return FText::Format(LOCTEXT("TwilightBadgeFmt", "暮{0}"), FText::AsNumber(Value));
		case EWacomCardViewEffectBadgeKind::Draw:
			return FText::Format(LOCTEXT("DrawBadgeFmt", "抽{0}"), FText::AsNumber(Value));
		case EWacomCardViewEffectBadgeKind::Discard:
			return FText::Format(LOCTEXT("DiscardBadgeFmt", "弃{0}"), FText::AsNumber(Value));
		case EWacomCardViewEffectBadgeKind::Initiative:
			return FText::Format(LOCTEXT("InitiativeBadgeFmt", "机{0}"), FText::AsNumber(Value));
		case EWacomCardViewEffectBadgeKind::Cost:
			return FText::Format(LOCTEXT("CostBadgeFmt", "费{0}"), FText::AsNumber(Value));
		case EWacomCardViewEffectBadgeKind::Shield:
			return FText::Format(LOCTEXT("ShieldBadgeFmt", "盾{0}"), FText::AsNumber(Value));
		default:
			return FText::AsNumber(Value);
		}
	}

	bool IsArtBackedCardFaceEffectBadgeKind(EWacomCardViewEffectBadgeKind Kind)
	{
		switch (Kind)
		{
		case EWacomCardViewEffectBadgeKind::Damage:
		case EWacomCardViewEffectBadgeKind::Poison:
		case EWacomCardViewEffectBadgeKind::Burn:
		case EWacomCardViewEffectBadgeKind::Heal:
		case EWacomCardViewEffectBadgeKind::Shield:
			return true;
		default:
			return false;
		}
	}

	bool TryBuildEffectBadge(
		const FCardEffect& Effect,
		const UCardDefinition* Card,
		const FWacomCardPresentationRuntimeContext& RuntimeContext,
		int32 EffectIndex,
		FWacomCardViewEffectBadge& OutBadge)
	{
		if (const FWacomCardPresentationRuntimeContext::FEffectPreview* Preview =
			FindEffectPreview(RuntimeContext, EffectIndex))
		{
			if (Preview->bSkip)
			{
				return false;
			}
		}

		const int32 Value = GetPreviewDisplayMagnitude(Effect, Card, RuntimeContext, EffectIndex);
		if (Value == 0)
		{
			return false;
		}

		if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Damage))
		{
			OutBadge.Kind = EWacomCardViewEffectBadgeKind::Damage;
		}
		else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Heal))
		{
			OutBadge.Kind = EWacomCardViewEffectBadgeKind::Heal;
		}
		else if (Effect.EffectType.MatchesTagExact(WacomTags::Status_Shield))
		{
			OutBadge.Kind = EWacomCardViewEffectBadgeKind::Shield;
		}
		else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Poison))
		{
			OutBadge.Kind = EWacomCardViewEffectBadgeKind::Poison;
		}
		else
		{
			return false;
		}

		if (!IsArtBackedCardFaceEffectBadgeKind(OutBadge.Kind))
		{
			return false;
		}

		OutBadge.Value = Value;
		OutBadge.DisplayText = BuildEffectBadgeText(OutBadge.Kind, Value);
		return true;
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

	FWacomCardDetailTokenLine BuildPassiveTextTokenLine(
		const FText& Text,
		int32 PassiveIndex,
		const TCHAR* StableIdSuffix)
	{
		FWacomCardDetailTokenLine Line;
		Line.LineId = FName(*FString::Printf(TEXT("Passive.%d.%s"), PassiveIndex, StableIdSuffix));
		Line.Kind = EWacomCardDetailTokenLineKind::Passive;
		Line.Tokens.Add(MakeTextToken(
			Text,
			FName(*FString::Printf(TEXT("Passive.%d.%s.Text"), PassiveIndex, StableIdSuffix))));
		return Line;
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

	bool TryBuildActiveEffectTokenLine(
		const FCardEffect& Effect,
		const UCardDefinition* Card,
		const FWacomCardPresentationRuntimeContext& RuntimeContext,
		int32 EffectIndex,
		FWacomCardDetailTokenLine& OutLine)
	{
		return TryBuildEffectTokenLine(
			Effect,
			Card,
			RuntimeContext,
			EffectIndex,
			FString::Printf(TEXT("Effect.%d"), EffectIndex),
			EWacomCardDetailTokenLineKind::Effect,
			OutLine);
	}

	bool TryBuildPassiveEffectTokenLine(
		const FCardEffect& Effect,
		const UCardDefinition* Card,
		const FWacomCardPresentationRuntimeContext& RuntimeContext,
		int32 PassiveIndex,
		int32 EffectIndex,
		FWacomCardDetailTokenLine& OutLine)
	{
		return TryBuildEffectTokenLine(
			Effect,
			Card,
			MakePassiveEffectRuntimeContext(RuntimeContext),
			EffectIndex,
			FString::Printf(TEXT("Passive.%d.Effect.%d"), PassiveIndex, EffectIndex),
			EWacomCardDetailTokenLineKind::Passive,
			OutLine);
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
				if (!PassiveBodyText.IsEmpty())
				{
					OutLines.Add(BuildPassiveTextTokenLine(PassiveBodyText, PassiveIndex, TEXT("Display")));
					bAnyPassiveTokenLines = true;
				}
				continue;
			}

			TArray<FWacomCardDetailTokenLine> EffectLines;
			for (int32 EffectIndex = 0; EffectIndex < Passive.Effects.Num(); ++EffectIndex)
			{
				FWacomCardDetailTokenLine EffectLine;
				if (TryBuildPassiveEffectTokenLine(
					Passive.Effects[EffectIndex],
					Card,
					RuntimeContext,
					PassiveIndex,
					EffectIndex,
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
				OutLines.Add(BuildPassiveTextTokenLine(PassiveBodyText, PassiveIndex, TEXT("Fallback")));
				bAnyPassiveTokenLines = true;
			}
		}
		return bAnyPassiveTokenLines;
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
			if (TryBuildActiveEffectTokenLine(Card->Effects[EffectIndex], Card, RuntimeContext, EffectIndex, Line))
			{
				Lines.Add(MoveTemp(Line));
			}
		}
		return Lines;
	}

	bool ShouldAppendGeneratedActiveEffectTokenLines(
		const UCardDefinition* Card)
	{
		if (!Card)
		{
			return false;
		}

		// 手写 Description 是卡牌详情的主阅读文本；自动 Effect 行只在没有描述时补规则。
		return Card->Description.IsEmpty();
	}

	void AppendDescriptionTokenLine(
		const UCardDefinition* Card,
		const FWacomCardPresentationRuntimeContext& RuntimeContext,
		TArray<FWacomCardDetailTokenLine>& OutLines)
	{
		if (!Card || Card->Description.IsEmpty())
		{
			return;
		}

		FString Description = Card->Description.ToString();
		Description.ReplaceInline(TEXT("\r\n"), TEXT("\n"));
		Description.ReplaceInline(TEXT("\r"), TEXT("\n"));

		TArray<FString> Lines;
		Description.ParseIntoArrayLines(Lines, false);
		for (int32 LineIndex = 0; LineIndex < Lines.Num(); ++LineIndex)
		{
			FWacomCardDetailTokenLine Line =
				MakeDescriptionTemplateTokenLine(Card, RuntimeContext, Lines[LineIndex], LineIndex);
			if (!Line.Tokens.IsEmpty())
			{
				OutLines.Add(MoveTemp(Line));
			}
		}
	}

	void AppendRuntimeDetailChangeLines(
		const UCardDefinition* Card,
		const FWacomCardPresentationRuntimeContext& RuntimeContext,
		TArray<FText>& OutLines)
	{
		if (!Card)
		{
			return;
		}

		if (RuntimeContext.bHasRuntimeCost && RuntimeContext.RuntimeCost != Card->BaseCost)
		{
			OutLines.Add(FText::Format(
				LOCTEXT("RuntimeCostChangeLineFmt", "当前费用：{0}（基础 {1}）"),
				FText::AsNumber(RuntimeContext.RuntimeCost),
				FText::AsNumber(Card->BaseCost)));
		}

		if (RuntimeContext.bHasPlayableState && !RuntimeContext.bIsPlayable)
		{
			OutLines.Add(LOCTEXT("RuntimeNotPlayableChangeLine", "先机不足，当前不可打出。"));
		}

		OutLines.Append(RuntimeContext.TargetPreviewChangeLines);
	}
}

FWacomCardViewData UWacomCardPresentationBuilder::BuildCardViewData(const UCardDefinition* Card)
{
	return BuildCardViewData(Card, FWacomCardPresentationRuntimeContext());
}

FWacomCardViewData UWacomCardPresentationBuilder::BuildCardViewData(
	const UCardDefinition* Card,
	const FWacomCardPresentationRuntimeContext& RuntimeContext)
{
	FWacomCardViewData Data;
	Data.Name = GetCardDisplayName(Card);
	Data.TypeText = BuildTypeLine(Card);
	Data.Description = BuildCompactDescriptionText(Card);
	Data.Cost = RuntimeContext.bHasRuntimeCost ? RuntimeContext.RuntimeCost : (Card ? Card->BaseCost : 0);
	Data.bShowCost = Card != nullptr;
	Data.Rarity = Card ? Card->Rarity : FGameplayTag();
	Data.Value = GetDeleteValueFromRarity(Card);
	Data.bShowValue = Data.Value > 0;
	Data.PhysiqueText = BuildPhysiqueText(Card);
	Data.bShowPhysique = !Data.PhysiqueText.IsEmpty();
	if (Card)
	{
		int32 Effective = Card->Physique.Durability;
		if (Effective == 0) Effective = Card->Physique.MaxHpBonus;
		Data.Durability = Effective;
		Data.bShowDurability = Effective > 0;
	}
	else
	{
		Data.Durability = 0;
		Data.bShowDurability = false;
	}
	Data.EffectBadges = BuildEffectBadges(Card, RuntimeContext);
	if (RuntimeContext.bHasPlayableState)
	{
		Data.bDisabled = !RuntimeContext.bIsPlayable;
	}
	return Data;
}

FWacomCardDetailViewData UWacomCardPresentationBuilder::BuildCardDetailViewData(const UCardDefinition* Card)
{
	return BuildCardDetailViewData(Card, FWacomCardPresentationRuntimeContext());
}

FWacomCardDetailViewData UWacomCardPresentationBuilder::BuildCardDetailViewData(
	const UCardDefinition* Card,
	const FWacomCardPresentationRuntimeContext& RuntimeContext)
{
	FWacomCardDetailViewData Data;
	Data.Name = GetCardDisplayName(Card);
	if (!Card)
	{
		return Data;
	}

	Data.Description = Card->Description;
	AppendDescriptionTokenLine(Card, RuntimeContext, Data.TokenLines);
	if (ShouldAppendGeneratedActiveEffectTokenLines(Card))
	{
		Data.TokenLines.Append(BuildEffectTokenLines(Card, RuntimeContext));
	}
	AppendRuntimeDetailChangeLines(Card, RuntimeContext, Data.ChangeLines);
	BuildPassiveTokenLines(Card, RuntimeContext, Data.TokenLines, Data.PassiveLines);
	return Data;
}

TArray<FWacomCardViewEffectBadge> UWacomCardPresentationBuilder::BuildEffectBadges(const UCardDefinition* Card)
{
	return BuildEffectBadges(Card, FWacomCardPresentationRuntimeContext());
}

TArray<FWacomCardViewEffectBadge> UWacomCardPresentationBuilder::BuildEffectBadges(
	const UCardDefinition* Card,
	const FWacomCardPresentationRuntimeContext& RuntimeContext)
{
	TArray<FWacomCardViewEffectBadge> Badges;
	if (!Card)
	{
		return Badges;
	}

	for (int32 EffectIndex = 0; EffectIndex < Card->Effects.Num(); ++EffectIndex)
	{
		const FCardEffect& Effect = Card->Effects[EffectIndex];
		FWacomCardViewEffectBadge Badge;
		if (TryBuildEffectBadge(Effect, Card, RuntimeContext, EffectIndex, Badge))
		{
			Badges.Add(Badge);
		}
	}
	return Badges;
}

#undef LOCTEXT_NAMESPACE
