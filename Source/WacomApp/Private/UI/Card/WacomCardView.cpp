// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomCardView.h"

#include "Blueprint/WidgetTree.h"
#include "Cards/CardDefinition.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Engine/Texture2D.h"
#include "UI/Card/WacomCardEffectBadgeWidget.h"
#include "Tags/WacomGameplayTags.h"

#define LOCTEXT_NAMESPACE "WacomCardView"

namespace
{
	constexpr float DefaultCardWidth = 260.f;
	constexpr float DefaultCardHeight = 380.f;

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

	int32 GetDisplayMagnitude(const FCardEffect& Effect, const UCardDefinition* Card)
	{
		if (Effect.MagnitudeSource.MatchesTagExact(WacomTags::Magnitude_Source_RuntimeCost))
		{
			return Card ? Card->BaseCost : Effect.Magnitude;
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
		default:
			return FText::AsNumber(Value);
		}
	}

	bool TryBuildEffectBadge(const FCardEffect& Effect, const UCardDefinition* Card, FWacomCardViewEffectBadge& OutBadge)
	{
		const int32 Value = GetDisplayMagnitude(Effect, Card);
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
		else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Poison))
		{
			OutBadge.Kind = EWacomCardViewEffectBadgeKind::Poison;
		}
		else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Slow))
		{
			OutBadge.Kind = EWacomCardViewEffectBadgeKind::Slow;
		}
		else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Freeze))
		{
			OutBadge.Kind = EWacomCardViewEffectBadgeKind::Freeze;
		}
		else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Twilight))
		{
			OutBadge.Kind = EWacomCardViewEffectBadgeKind::Twilight;
		}
		else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Draw))
		{
			OutBadge.Kind = EWacomCardViewEffectBadgeKind::Draw;
		}
		else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Discard))
		{
			OutBadge.Kind = EWacomCardViewEffectBadgeKind::Discard;
		}
		else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_ModifyInitiative))
		{
			OutBadge.Kind = EWacomCardViewEffectBadgeKind::Initiative;
		}
		else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Card_AddCost)
			|| Effect.EffectType.MatchesTagExact(WacomTags::Effect_Card_ReduceCost))
		{
			OutBadge.Kind = EWacomCardViewEffectBadgeKind::Cost;
		}
		else
		{
			return false;
		}

		OutBadge.Value = Value;
		OutBadge.DisplayText = BuildEffectBadgeText(OutBadge.Kind, Value);
		return true;
	}

	TArray<FWacomCardViewEffectBadge> BuildEffectBadges(const UCardDefinition* Card)
	{
		TArray<FWacomCardViewEffectBadge> Badges;
		if (!Card)
		{
			return Badges;
		}

		for (const FCardEffect& Effect : Card->Effects)
		{
			FWacomCardViewEffectBadge Badge;
			if (TryBuildEffectBadge(Effect, Card, Badge))
			{
				Badges.Add(Badge);
			}
		}
		return Badges;
	}

	FText BuildPassiveTriggerText(const FCardPassive& Passive)
	{
		if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_AfterPlayed))
		{
			return LOCTEXT("PassiveTriggerAfterPlayed", "被动：打出后");
		}
		if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnCompanionCount))
		{
			return Passive.TriggerThreshold > 0
				? FText::Format(LOCTEXT("PassiveTriggerOnCompanionCountFmt", "被动：每打出{0}张伙伴"), FText::AsNumber(Passive.TriggerThreshold))
				: LOCTEXT("PassiveTriggerOnCompanionCount", "被动：打出伙伴计数");
		}
		if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnTwilightTriggered))
		{
			return LOCTEXT("PassiveTriggerOnTwilightTriggered", "被动：暮气触发");
		}
		if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnTurnStart))
		{
			return LOCTEXT("PassiveTriggerOnTurnStart", "被动：回合开始");
		}
		if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnTurnEnd))
		{
			return LOCTEXT("PassiveTriggerOnTurnEnd", "被动：回合结束");
		}
		if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnDraw))
		{
			return LOCTEXT("PassiveTriggerOnDraw", "被动：抽到时");
		}
		if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnDiscard))
		{
			return LOCTEXT("PassiveTriggerOnDiscard", "被动：弃掉时");
		}

		return Passive.Trigger.IsValid()
			? FText::FromString(FString::Printf(TEXT("被动：%s"), *ShortGameplayTagName(Passive.Trigger)))
			: LOCTEXT("PassiveTriggerUnknown", "被动");
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

	void SetOptionalText(UTextBlock* TextBlock, const FText& Text)
	{
		if (!TextBlock)
		{
			return;
		}

		TextBlock->SetText(Text);
		TextBlock->SetVisibility(Text.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	void SetOptionalNumberText(UTextBlock* TextBlock, int32 Value, bool bShow)
	{
		if (!TextBlock)
		{
			return;
		}

		TextBlock->SetText(FText::AsNumber(Value));
		TextBlock->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

UWacomCardView::UWacomCardView(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (UClass* Loaded = LoadObject<UClass>(
		nullptr,
		TEXT("/Game/Wacom/UI/Card/WBP_CardEffectBadge.WBP_CardEffectBadge_C")))
	{
		EffectBadgeWidgetClass = Loaded;
	}
	else
	{
		EffectBadgeWidgetClass = UWacomCardEffectBadgeWidget::StaticClass();
	}
}

TSharedRef<SWidget> UWacomCardView::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		USizeBox* Root = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CardViewRoot"));
		Root->SetWidthOverride(DefaultCardWidth);
		Root->SetHeightOverride(DefaultCardHeight);
		WidgetTree->RootWidget = Root;

		UOverlay* Stack = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("CardViewOverlay"));
		Root->AddChild(Stack);

		UBorder* Body = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CardBody"));
		Body->SetBrushColor(FLinearColor(0.06f, 0.055f, 0.045f, 0.95f));
		Body->SetPadding(FMargin(8.f));
		if (UOverlaySlot* BodySlot = Stack->AddChildToOverlay(Body))
		{
			BodySlot->SetHorizontalAlignment(HAlign_Fill);
			BodySlot->SetVerticalAlignment(VAlign_Fill);
		}

		UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CardContent"));
		Body->AddChild(Content);

		CostText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CostText"));
		CostText->SetJustification(ETextJustify::Left);
		CostText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.85f, 0.65f, 1.f)));
		if (UVerticalBoxSlot* CostSlot = Content->AddChildToVerticalBox(CostText))
		{
			CostSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));
		}

		UHorizontalBox* HeaderStats = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HeaderStats"));
		if (UVerticalBoxSlot* HeaderStatsSlot = Content->AddChildToVerticalBox(HeaderStats))
		{
			HeaderStatsSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
		}

		ValueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ValueText"));
		ValueText->SetColorAndOpacity(FSlateColor(FLinearColor(0.75f, 0.9f, 1.f, 1.f)));
		HeaderStats->AddChild(ValueText);

		PhysiqueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PhysiqueText"));
		PhysiqueText->SetAutoWrapText(true);
		PhysiqueText->SetColorAndOpacity(FSlateColor(FLinearColor(0.75f, 1.f, 0.75f, 1.f)));
		HeaderStats->AddChild(PhysiqueText);

		NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NameText"));
		NameText->SetJustification(ETextJustify::Center);
		NameText->SetAutoWrapText(true);
		NameText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		{
			FSlateFontInfo Font = NameText->GetFont();
			Font.Size = 14;
			NameText->SetFont(Font);
		}
		if (UVerticalBoxSlot* NameSlot = Content->AddChildToVerticalBox(NameText))
		{
			NameSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
		}

		CardArt = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CardArt"));
		CardArt->SetColorAndOpacity(FLinearColor(0.18f, 0.18f, 0.18f, 1.f));
		if (UVerticalBoxSlot* ArtSlot = Content->AddChildToVerticalBox(CardArt))
		{
			ArtSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			ArtSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
		}

		TypeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TypeText"));
		TypeText->SetJustification(ETextJustify::Center);
		TypeText->SetAutoWrapText(true);
		TypeText->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.75f, 0.65f, 1.f)));
		if (UVerticalBoxSlot* TypeSlot = Content->AddChildToVerticalBox(TypeText))
		{
			TypeSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
		}

		EffectStatsHost = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), TEXT("EffectStatsHost"));
		if (UVerticalBoxSlot* BadgeSlot = Content->AddChildToVerticalBox(EffectStatsHost))
		{
			BadgeSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
		}

		DescriptionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DescriptionText"));
		DescriptionText->SetJustification(ETextJustify::Center);
		DescriptionText->SetAutoWrapText(true);
		DescriptionText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.92f, 0.86f, 1.f)));
		{
			FSlateFontInfo Font = DescriptionText->GetFont();
			Font.Size = 11;
			DescriptionText->SetFont(Font);
		}
		if (UVerticalBoxSlot* DescSlot = Content->AddChildToVerticalBox(DescriptionText))
		{
			DescSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}

		DisabledOverlay = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DisabledOverlay"));
		DisabledOverlay->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.45f));
		DisabledOverlay->SetVisibility(ESlateVisibility::Collapsed);
		if (UOverlaySlot* DisabledSlot = Stack->AddChildToOverlay(DisabledOverlay))
		{
			DisabledSlot->SetHorizontalAlignment(HAlign_Fill);
			DisabledSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}

	return Super::RebuildWidget();
}

void UWacomCardView::NativeConstruct()
{
	Super::NativeConstruct();
	ApplyCurrentDataToWidgets();
}

void UWacomCardView::SetCardViewData(const FWacomCardViewData& InData)
{
	CurrentData = InData;
	ApplyCurrentDataToWidgets();
}

FWacomCardViewData UWacomCardView::BuildFromCardDefinition(const UCardDefinition* Card)
{
	FWacomCardViewData Data;
	Data.Name = GetCardDisplayName(Card);
	Data.TypeText = BuildTypeLine(Card);
	Data.Description = BuildCompactDescriptionText(Card);
	Data.Cost = Card ? Card->BaseCost : 0;
	Data.bShowCost = Card != nullptr;
	Data.Value = GetDeleteValueFromRarity(Card);
	Data.bShowValue = Data.Value > 0;
	Data.PhysiqueText = BuildPhysiqueText(Card);
	Data.bShowPhysique = !Data.PhysiqueText.IsEmpty();
	Data.EffectBadges = BuildEffectBadges(Card);
	return Data;
}

FWacomCardDetailViewData UWacomCardView::BuildDetailFromCardDefinition(const UCardDefinition* Card)
{
	FWacomCardDetailViewData Data;
	Data.Name = GetCardDisplayName(Card);
	if (!Card)
	{
		return Data;
	}

	Data.Description = Card->Description;
	for (const FCardPassive& Passive : Card->Passives)
	{
		Data.PassiveLines.Add(Passive.DisplayText.IsEmpty() ? BuildPassiveLine(Passive) : Passive.DisplayText);
	}
	return Data;
}

void UWacomCardView::ApplyCurrentDataToWidgets()
{
	if (CostText)
	{
		CostText->SetText(FText::AsNumber(CurrentData.Cost));
		CostText->SetVisibility(CurrentData.bShowCost ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	SetOptionalNumberText(ValueText, CurrentData.Value, CurrentData.bShowValue);
	SetOptionalText(PhysiqueText, CurrentData.bShowPhysique ? CurrentData.PhysiqueText : FText::GetEmpty());
	SetOptionalText(NameText, CurrentData.Name);
	SetOptionalText(TypeText, CurrentData.TypeText);
	SetOptionalText(DescriptionText, CurrentData.Description);
	if (EffectStatsHost)
	{
		EffectStatsHost->ClearChildren();
		UClass* BadgeClass = EffectBadgeWidgetClass
			? EffectBadgeWidgetClass.Get()
			: UWacomCardEffectBadgeWidget::StaticClass();
		for (const FWacomCardViewEffectBadge& Badge : CurrentData.EffectBadges)
		{
			UWacomCardEffectBadgeWidget* BadgeWidget = CreateWidget<UWacomCardEffectBadgeWidget>(this, BadgeClass);
			if (!BadgeWidget)
			{
				continue;
			}
			BadgeWidget->SetEffectBadgeData(Badge);
			EffectStatsHost->AddChild(BadgeWidget);
		}
		EffectStatsHost->SetVisibility(CurrentData.EffectBadges.Num() > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (CardArt)
	{
		if (CurrentData.Art)
		{
			CardArt->SetBrushFromTexture(CurrentData.Art);
			CardArt->SetColorAndOpacity(FLinearColor::White);
		}
	}

	if (DisabledOverlay)
	{
		DisabledOverlay->SetVisibility(CurrentData.bDisabled ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

#undef LOCTEXT_NAMESPACE
