// Copyright Wacom. All Rights Reserved.

#include "WacomCardFaceViewDataBuilder.h"

#include "Cards/CardDefinition.h"
#include "RunSession.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Card/WacomCardExplanationLexicon.h"
#include "WacomCardExplanationLexiconProvider.h"

#define LOCTEXT_NAMESPACE "WacomCardFaceViewDataBuilder"

namespace WacomCardFaceViewDataBuilder
{
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

		FText GetRunCardDisplayName(const UCardDefinition* Card)
		{
			if (!Card)
			{
				return LOCTEXT("UnknownRunCardName", "未知卡牌");
			}
			return Card->RunFace.DisplayNameOverride.IsEmpty()
				? GetCardDisplayName(Card)
				: Card->RunFace.DisplayNameOverride;
		}

		FString ShortGameplayTagName(const FGameplayTag& Tag)
		{
			FString TagName = Tag.GetTagName().ToString();
			int32 LastDot = INDEX_NONE;
			TagName.FindLastChar(TEXT('.'), LastDot);
			return LastDot != INDEX_NONE ? TagName.Mid(LastDot + 1) : TagName;
		}

		FText GetKeywordFallbackDisplayName(const FGameplayTag& Tag)
		{
			if (Tag.MatchesTagExact(WacomTags::Card_Keyword_Swift))          { return LOCTEXT("KeywordSwift", "迅捷"); }
			if (Tag.MatchesTagExact(WacomTags::Card_Keyword_Retain))         { return LOCTEXT("KeywordRetain", "保留"); }
			if (Tag.MatchesTagExact(WacomTags::Card_Keyword_Combo))          { return LOCTEXT("KeywordCombo", "连击"); }
			if (Tag.MatchesTagExact(WacomTags::Card_Keyword_Companion))      { return LOCTEXT("KeywordCompanion", "伙伴"); }
			if (Tag.MatchesTagExact(WacomTags::Card_Keyword_Weapon))         { return LOCTEXT("KeywordWeapon", "武器"); }
			if (Tag.MatchesTagExact(WacomTags::Card_Keyword_Tool))           { return LOCTEXT("KeywordTool", "工具"); }
			if (Tag.MatchesTagExact(WacomTags::Card_Keyword_Hand))           { return LOCTEXT("KeywordHand", "手"); }
			if (Tag.MatchesTagExact(WacomTags::Card_Keyword_Exhaust))        { return LOCTEXT("KeywordExhaust", "消耗"); }
			if (Tag.MatchesTagExact(WacomTags::Card_Keyword_BagProvider))    { return LOCTEXT("KeywordBagProvider", "容器兼容标记"); }
			if (Tag.MatchesTagExact(WacomTags::Card_Keyword_DeleteProvider)) { return LOCTEXT("KeywordDeleteProvider", "删牌"); }
			if (Tag.MatchesTagExact(WacomTags::Card_Keyword_Food))           { return LOCTEXT("KeywordFood", "食物"); }
			if (Tag.MatchesTagExact(WacomTags::Card_Keyword_Container))      { return LOCTEXT("KeywordContainer", "容器"); }
			return FText::FromString(ShortGameplayTagName(Tag));
		}

		struct FTypeLineView
		{
			FText Text;
			TArray<FWacomCardFaceSemanticTokenView> Tokens;
		};

		FWacomCardFaceSemanticTokenView MakeSemanticToken(
			const FName SemanticId,
			const FGameplayTag SourceTag,
			const FText& FallbackDisplayName)
		{
			FWacomCardFaceSemanticTokenView Token;
			Token.SemanticId = SemanticId;
			Token.SourceTag = SourceTag;
			FWacomCardFaceSemanticLexiconEntry Entry;
			Token.DisplayText =
				WacomCardExplanationLexiconProvider::FindCardFaceSemantic(
					SemanticId,
					SourceTag,
					Entry)
				? Entry.DisplayName
				: FallbackDisplayName;
			return Token;
		}

		EWacomCardUpgradeTier ResolveTier(const FWacomCardPresentationRuntimeContext& RuntimeContext)
		{
			return RuntimeContext.bHasUpgradeTier
				? RuntimeContext.UpgradeTier
				: EWacomCardUpgradeTier::White;
		}

		FTypeLineView BuildTypeLine(
			const UCardDefinition* Card,
			const FWacomCardPresentationRuntimeContext& RuntimeContext)
		{
			FTypeLineView Result;
			if (!Card)
			{
				return Result;
			}

			const FCardPhysique& Physique = Card->ResolvePhysique(ResolveTier(RuntimeContext));
			if (Physique.Capacity > 0)
			{
				const bool bDedicatedContainer =
					Physique.CapacityEffect.IsValid();
				Result.Tokens.Add(MakeSemanticToken(
					bDedicatedContainer
						? WacomCardFaceSemanticIds::Container
						: WacomCardFaceSemanticIds::Backpack,
					FGameplayTag(),
					bDedicatedContainer
						? LOCTEXT("TypeContainerB", "容器")
						: LOCTEXT("TypeContainerA", "背包")));
			}
			else
			{
				for (const FGameplayTag& Tag : Card->Keywords)
				{
					Result.Tokens.Add(MakeSemanticToken(
						Tag.GetTagName(),
						Tag,
						GetKeywordFallbackDisplayName(Tag)));
				}
			}

			FString TypeString;
			for (FWacomCardFaceSemanticTokenView& Token : Result.Tokens)
			{
				if (!TypeString.IsEmpty())
				{
					TypeString += TEXT(" / ");
				}
				Token.StartIndex = TypeString.Len();
				const FString DisplayString = Token.DisplayText.ToString();
				Token.Length = DisplayString.Len();
				TypeString += DisplayString;
			}
			Result.Text = FText::FromString(TypeString);
			return Result;
		}

		FText BuildCompactDescriptionText(
			const UCardDefinition* Card,
			const FWacomCardPresentationRuntimeContext& RuntimeContext)
		{
			if (!Card)
			{
				return FText::GetEmpty();
			}

			const FText& Description = Card->ResolveDescription(ResolveTier(RuntimeContext));
			if (Description.IsEmpty())
			{
				return FText::GetEmpty();
			}
			FString Text = Description.ToString();
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

		FText BuildPhysiqueText(
			const UCardDefinition* Card,
			const FWacomCardPresentationRuntimeContext& RuntimeContext)
		{
			if (!Card)
			{
				return FText::GetEmpty();
			}

			const FCardPhysique& Physique = Card->ResolvePhysique(ResolveTier(RuntimeContext));
			TArray<FString> Parts;
			if (Physique.Durability > 0)
			{
				Parts.Add(FString::Printf(TEXT("%d耐久"), Physique.Durability));
			}
			if (Physique.Capacity > 0)
			{
				Parts.Add(FString::Printf(TEXT("%d容量"), Physique.Capacity));
			}
			if (Physique.MaxHpBonus > 0)
			{
				Parts.Add(FString::Printf(TEXT("+%d生命"), Physique.MaxHpBonus));
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

		const FWacomCardPresentationRuntimeContext::FCurrentEffectMagnitude*
			FindCurrentEffectMagnitude(
				const FWacomCardPresentationRuntimeContext& RuntimeContext,
				int32 EffectIndex)
		{
			for (const FWacomCardPresentationRuntimeContext::FCurrentEffectMagnitude&
				Magnitude : RuntimeContext.CurrentEffectMagnitudes)
			{
				if (Magnitude.EffectIndex == EffectIndex)
				{
					return &Magnitude;
				}
			}
			return nullptr;
		}

		int32 GetBaseDisplayMagnitude(
			const FCardEffect& Effect,
			const UCardDefinition* Card,
			const FWacomCardPresentationRuntimeContext& RuntimeContext,
			int32 EffectIndex)
		{
			if (const FWacomCardPresentationRuntimeContext::FCurrentEffectMagnitude*
				CurrentMagnitude =
					FindCurrentEffectMagnitude(RuntimeContext, EffectIndex))
			{
				return CurrentMagnitude->Magnitude;
			}
			if (UsesRuntimeCostMagnitude(Effect))
			{
				if (RuntimeContext.bHasRuntimeCost)
				{
					return RuntimeContext.RuntimeCost;
				}
				return Card
					? Card->ResolveBaseCost(ResolveTier(RuntimeContext))
					: Effect.Magnitude;
			}
			return Effect.Magnitude;
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

		bool TryResolveEffectBadgeKind(
			const FCardEffect& Effect,
			EWacomCardViewEffectBadgeKind& OutKind)
		{
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Damage))
			{
				OutKind = EWacomCardViewEffectBadgeKind::Damage;
			}
			else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Heal))
			{
				OutKind = EWacomCardViewEffectBadgeKind::Heal;
			}
			else if (Effect.EffectType.MatchesTagExact(WacomTags::Status_Shield))
			{
				OutKind = EWacomCardViewEffectBadgeKind::Shield;
			}
			else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Poison))
			{
				OutKind = EWacomCardViewEffectBadgeKind::Poison;
			}
			else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Burn))
			{
				OutKind = EWacomCardViewEffectBadgeKind::Burn;
			}
			else
			{
				return false;
			}
			return IsArtBackedCardFaceEffectBadgeKind(OutKind);
		}

		FName GetEffectBadgePresentationKey(EWacomCardViewEffectBadgeKind Kind)
		{
			switch (Kind)
			{
			case EWacomCardViewEffectBadgeKind::Damage: return TEXT("Badge.Damage");
			case EWacomCardViewEffectBadgeKind::Heal: return TEXT("Badge.Heal");
			case EWacomCardViewEffectBadgeKind::Poison: return TEXT("Badge.Poison");
			case EWacomCardViewEffectBadgeKind::Burn: return TEXT("Badge.Burn");
			case EWacomCardViewEffectBadgeKind::Shield: return TEXT("Badge.Shield");
			default:
				return FName(*FString::Printf(TEXT("Badge.%d"), static_cast<int32>(Kind)));
			}
		}

		struct FEffectBadgeAggregate
		{
			EWacomCardViewEffectBadgeKind Kind = EWacomCardViewEffectBadgeKind::Generic;
			int32 AuthoritativeValue = 0;
			int32 PreviewValue = 0;
			bool bHasPreviewFacts = false;
			bool bHasApplicablePreviewContribution = false;
		};
	}

	FWacomCardViewData BuildCardViewData(
		const UCardDefinition* Card,
		EWacomCardFaceContext FaceContext,
		const FWacomCardPresentationRuntimeContext& RuntimeContext)
	{
		if (FaceContext == EWacomCardFaceContext::Run)
		{
			FWacomCardViewData Data;
			Data.Name = GetRunCardDisplayName(Card);
			Data.TypeText = LOCTEXT("RunCardType", "探索");
			Data.Description = Card ? Card->RunFace.Description : FText::GetEmpty();
			Data.bShowCost = false;
			Data.Art = Card && Card->RunFace.IllustrationOverride
				? Card->RunFace.IllustrationOverride
				: (Card ? Card->CardIllustration : nullptr);
			Data.ArtDepthMap = Card && Card->RunFace.IllustrationDepthMapOverride
				? Card->RunFace.IllustrationDepthMapOverride
				: (Card ? Card->CardIllustrationDepthMap : nullptr);
			Data.Rarity = Card ? Card->ResolveRarity(ResolveTier(RuntimeContext)) : FGameplayTag();
			Data.bShowValue = false;
			Data.bShowPhysique = false;
			Data.bShowDurability = false;
			Data.bDisabled = !Card
				|| !Card->HasEnabledRunFace()
				|| (RuntimeContext.bHasPlayableState && !RuntimeContext.bIsPlayable);
			return Data;
		}

		FWacomCardViewData Data;
		Data.Name = GetCardDisplayName(Card);
		const FTypeLineView TypeLine = BuildTypeLine(Card, RuntimeContext);
		Data.TypeText = TypeLine.Text;
		Data.TypeSemanticTokens = TypeLine.Tokens;
		Data.Description = BuildCompactDescriptionText(Card, RuntimeContext);
		Data.Cost = RuntimeContext.bHasRuntimeCost
			? RuntimeContext.RuntimeCost
			: (Card ? Card->ResolveBaseCost(ResolveTier(RuntimeContext)) : 0);
		Data.bShowCost = Card != nullptr;
		Data.bHasCostPreview = RuntimeContext.bHasRuntimeCostPreview;
		Data.PreviewCost = RuntimeContext.RuntimeCostPreview;
		Data.Art = Card ? Card->CardIllustration : nullptr;
		Data.ArtDepthMap = Card ? Card->CardIllustrationDepthMap : nullptr;
		Data.Rarity = Card ? Card->ResolveRarity(ResolveTier(RuntimeContext)) : FGameplayTag();
		Data.Value = URunSession::GetDeleteGoldRewardForCard(Card);
		Data.bShowValue = Data.Value > 0;
		Data.PhysiqueText = BuildPhysiqueText(Card, RuntimeContext);
		Data.bShowPhysique = !Data.PhysiqueText.IsEmpty();
		if (Card)
		{
			const FCardPhysique& Physique = Card->ResolvePhysique(ResolveTier(RuntimeContext));
			int32 Effective = RuntimeContext.bHasCurrentDurability
				? RuntimeContext.CurrentDurability
				: Physique.Durability;
			if (Effective == 0) Effective = Physique.MaxHpBonus;
			Data.Durability = Effective;
			Data.bShowDurability = Effective > 0;
		}
		else
		{
			Data.Durability = 0;
			Data.bShowDurability = false;
		}
		Data.EffectBadges = BuildEffectBadges(
			Card,
			EWacomCardFaceContext::Battle,
			RuntimeContext);
		if (RuntimeContext.bHasPlayableState)
		{
			Data.bDisabled = !RuntimeContext.bIsPlayable;
		}
		return Data;
	}

	TArray<FWacomCardViewEffectBadge> BuildEffectBadges(
		const UCardDefinition* Card,
		EWacomCardFaceContext FaceContext,
		const FWacomCardPresentationRuntimeContext& RuntimeContext)
	{
		TArray<FWacomCardViewEffectBadge> Badges;
		if (!Card || FaceContext == EWacomCardFaceContext::Run)
		{
			return Badges;
		}
		TArray<FEffectBadgeAggregate> Aggregates;
		const TArray<FCardEffect>& Effects = Card->ResolveEffects(ResolveTier(RuntimeContext));
		Aggregates.Reserve(Effects.Num());

		for (int32 EffectIndex = 0; EffectIndex < Effects.Num(); ++EffectIndex)
		{
			const FCardEffect& Effect = Effects[EffectIndex];
			EWacomCardViewEffectBadgeKind Kind = EWacomCardViewEffectBadgeKind::Generic;
			if (!TryResolveEffectBadgeKind(Effect, Kind))
			{
				continue;
			}

			FEffectBadgeAggregate* Aggregate = Aggregates.FindByPredicate([Kind](
				const FEffectBadgeAggregate& Candidate)
			{
				return Candidate.Kind == Kind;
			});
			if (!Aggregate)
			{
				Aggregate = &Aggregates.AddDefaulted_GetRef();
				Aggregate->Kind = Kind;
			}

			const int32 BaseMagnitude = GetBaseDisplayMagnitude(
				Effect,
				Card,
				RuntimeContext,
				EffectIndex);
			const int32 AuthoritativeContribution = Effect.Condition.IsSet() ? 0 : BaseMagnitude;
			Aggregate->AuthoritativeValue += AuthoritativeContribution;

			const FWacomCardPresentationRuntimeContext::FEffectPreview* Preview =
				FindEffectPreview(RuntimeContext, EffectIndex);
			if (!Preview)
			{
				Aggregate->PreviewValue += AuthoritativeContribution;
				Aggregate->bHasApplicablePreviewContribution |= AuthoritativeContribution != 0;
				continue;
			}

			Aggregate->bHasPreviewFacts = true;
			if (!Preview->bSkip)
			{
				const int32 PreviewContribution = Preview->bHasMagnitude
					? Preview->Magnitude
					: BaseMagnitude;
				Aggregate->PreviewValue += PreviewContribution;
				Aggregate->bHasApplicablePreviewContribution |= PreviewContribution != 0;
			}
		}

		for (const FEffectBadgeAggregate& Aggregate : Aggregates)
		{
			if (Aggregate.AuthoritativeValue == 0 && Aggregate.PreviewValue == 0)
			{
				continue;
			}

			FWacomCardViewEffectBadge& Badge = Badges.AddDefaulted_GetRef();
			Badge.Kind = Aggregate.Kind;
			Badge.PresentationKey = GetEffectBadgePresentationKey(Aggregate.Kind);
			Badge.Value = Aggregate.AuthoritativeValue;
			Badge.PreviewValue = Aggregate.PreviewValue;
			Badge.bPreviewSkipped = Aggregate.bHasPreviewFacts
				&& !Aggregate.bHasApplicablePreviewContribution;
			Badge.bHasPreviewValue = Aggregate.bHasPreviewFacts
				&& !Badge.bPreviewSkipped
				&& Aggregate.PreviewValue != Aggregate.AuthoritativeValue;
			Badge.DisplayText = BuildEffectBadgeText(
				Badge.Kind,
				Badge.Value != 0 ? Badge.Value : Badge.PreviewValue);
		}
		return Badges;
	}
}

#undef LOCTEXT_NAMESPACE
