// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "BackpackScreenTestAccess.h"

#include "Blueprint/WidgetTree.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomBackpackScreenPresenter.h"
#include "UI/Backpack/WacomBackpackZoneSectionWidget.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "UI/Backpack/WacomSpecialZoneWidget.h"
#include "UI/Card/WacomCardEffectBadgeWidget.h"
#include "UI/Card/WacomCardDetailPanel.h"
#include "UI/Card/WacomCardDetailPlainTextRenderer.h"
#include "UI/Card/WacomCardDetailSectionWidget.h"
#include "UI/Card/WacomCardPresentationBuilder.h"
#include "UI/Card/WacomCardView.h"
#include "UI/CardViewTestAccess.h"
#include "UI/CardViewSpecReceiver.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Components/Image.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "PaperSprite.h"
#include "RunSession.h"
#include "Tags/WacomGameplayTags.h"

#include "UObject/StrongObjectPtr.h"

namespace
{

	FWacomCardViewEffectBadge MakeCardViewEffectBadgeForTest(EWacomCardViewEffectBadgeKind Kind, int32 Value)
	{
		FWacomCardViewEffectBadge Badge;
		Badge.Kind = Kind;
		Badge.Value = Value;
		return Badge;
	}

	FWacomCardDetailBlock MakeCardDetailTextBlockForTest(
		FName BlockId,
		EWacomCardDetailBlockKind Kind,
		const FString& Text)
	{
		FWacomCardDetailRun Run;
		Run.StableId = FName(*FString::Printf(TEXT("%s.Text"), *BlockId.ToString()));
		Run.Kind = EWacomCardDetailRunKind::Text;
		Run.Text = FText::FromString(Text);

		FWacomCardDetailBlock Block;
		Block.BlockId = BlockId;
		Block.Kind = Kind;
		Block.Runs.Add(Run);
		return Block;
	}

	FString JoinCardDetailSectionTextForTest(
		const FWacomCardDetailViewData& Data,
		EWacomCardDetailSectionKind SectionKind)
	{
		FString Text;
		for (const FWacomCardDetailSection& Section : Data.Sections)
		{
			if (Section.Kind != SectionKind)
			{
				continue;
			}

			const FString SectionText =
				UWacomCardDetailPlainTextRenderer::RenderSectionPlainText(Section).ToString();
			if (!SectionText.IsEmpty())
			{
				if (!Text.IsEmpty())
				{
					Text += TEXT("\n");
				}
				Text += SectionText;
			}
		}
		return Text;
	}

	const UWacomCardEffectBadgeWidget* GetSingleSlotBadgeForTest(const UPanelWidget* Slot)
	{
		if (!Slot || Slot->GetChildrenCount() != 1)
		{
			return nullptr;
		}

		return Cast<UWacomCardEffectBadgeWidget>(Slot->GetChildAt(0));
	}

	UCardDefinition* MakeBackpackUiCardForTest(UObject* Outer, FName CardId, int32 Capacity = 0, bool bTypeB = false)
	{
		UCardDefinition* Card = NewObject<UCardDefinition>(Outer);
		Card->CardId = CardId;
		Card->DisplayName = FText::FromName(CardId);
		Card->BaseCost = 1;
		Card->Physique.Capacity = Capacity;
		if (bTypeB)
		{
			Card->Physique.CapacityEffect = WacomTags::Card_CapacityEffect_WeaponDamagePlus3;
		}
		return Card;
	}

	UCharacterDefinition* MakeBackpackUiCharacterForTest(UObject* Outer, const TArray<UCardDefinition*>& StarterDeck)
	{
		UCharacterDefinition* Character = NewObject<UCharacterDefinition>(Outer);
		Character->CharacterId = TEXT("Backpack.UI.Character");
		Character->DisplayName = FText::FromString(TEXT("背包 UI 测试角色"));
		for (UCardDefinition* Card : StarterDeck)
		{
			Character->StarterDeck.Add(Card);
		}
		return Character;
	}

	UWacomBackpackScreen* MakeBackpackUiScreenForTest(UObject* Outer, URunSession* Run)
	{
		return FWacomBackpackScreenTestAccess::Create(Outer, Run);
	}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardViewUnboundFallbackSpec,
	"Wacom.UI.Backpack.CardViewUnboundFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardViewUnboundFallbackSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardView> CardView(NewObject<UWacomCardView>());

	FWacomCardViewData Data;
	Data.Name = FText::FromString(TEXT("测试卡"));
	Data.TypeText = FText::FromString(TEXT("Companion"));
	Data.Description = FText::FromString(TEXT("测试描述"));
	Data.Cost = 2;
	Data.bShowCost = true;
	Data.bDisabled = true;

	CardView->SetCardViewData(Data);

	TestEqual(TEXT("Unbound CardView preserves data name"),
		CardView->GetCardViewData().Name.ToString(),
		Data.Name.ToString());
	TestEqual(TEXT("Unbound CardView preserves data cost"),
		CardView->GetCardViewData().Cost,
		Data.Cost);
	TestTrue(TEXT("Unbound CardView preserves disabled flag"),
		CardView->GetCardViewData().bDisabled);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardViewSpriteIconFallbackSpec,
	"Wacom.UI.Backpack.CardViewSpriteIconFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardViewSpriteIconFallbackSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomCardViewData CostData;
	CostData.Name = FText::FromString(TEXT("费用回退测试"));
	CostData.Cost = 10;
	CostData.bShowCost = true;
	TStrongObjectPtr<UWacomCardViewSpecProbe> CostCardView(NewObject<UWacomCardViewSpecProbe>());
	CostCardView->SetCostDigitIconForTest(1, NewObject<UPaperSprite>(CostCardView.Get()));
	CostCardView->TakeWidget();
	CostCardView->SetCardViewData(CostData);

	UImage* CostDigitImage = CostCardView->GetCostDigitImageForTest();
	TestNotNull(TEXT("Fallback CardView creates CostDigitImage"), CostDigitImage);
	if (CostDigitImage)
	{
		TestEqual(TEXT("Multi-digit cost hides compact single cost icon"),
			CostDigitImage->GetVisibility(),
			ESlateVisibility::Collapsed);
	}

	FWacomCardViewData DurabilityData = CostData;
	DurabilityData.Durability = 10;
	DurabilityData.bShowDurability = true;
	TStrongObjectPtr<UWacomCardViewSpecProbe> DurabilityCardView(NewObject<UWacomCardViewSpecProbe>());
	DurabilityCardView->SetDurabilityDigitIconForTest(1, NewObject<UPaperSprite>(DurabilityCardView.Get()));
	DurabilityCardView->TakeWidget();
	DurabilityCardView->SetCardViewData(DurabilityData);

	UWidget* DurabilityHost = DurabilityCardView->GetDurabilityHostForTest();
	UPanelWidget* DurabilityDigitsHost = DurabilityCardView->GetDurabilityDigitsHostForTest();
	TestNotNull(TEXT("Fallback CardView creates DurabilityHost"), DurabilityHost);
	TestNotNull(TEXT("Fallback CardView creates DurabilityDigitsHost"), DurabilityDigitsHost);
	if (DurabilityHost && DurabilityDigitsHost)
	{
		TestEqual(TEXT("Missing durability digit sprite hides durability host"),
			DurabilityHost->GetVisibility(),
			ESlateVisibility::Collapsed);
		TestEqual(TEXT("Missing durability digit sprite leaves no empty digits"),
			DurabilityDigitsHost->GetChildrenCount(),
			0);
	}

	FWacomCardViewData RarityData = CostData;
	RarityData.Rarity = WacomTags::Card_Rarity_Blue;
	TStrongObjectPtr<UWacomCardViewSpecProbe> RarityCardView(NewObject<UWacomCardViewSpecProbe>());
	RarityCardView->TakeWidget();
	RarityCardView->SetCardViewData(RarityData);
	UImage* RarityBorder = RarityCardView->GetRarityBorderForTest();
	TestNotNull(TEXT("Fallback CardView creates RarityBorder"), RarityBorder);
	if (RarityBorder)
	{
		TestEqual(TEXT("Missing rarity sprite keeps border hidden"),
			RarityBorder->GetVisibility(),
			ESlateVisibility::Collapsed);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardViewSpriteIconCacheSpec,
	"Wacom.UI.Backpack.CardViewSpriteIconCacheAvoidsPerRefreshResolve",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardViewSpriteIconCacheSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardViewSpecProbe> CardView(NewObject<UWacomCardViewSpecProbe>());
	CardView->SetCostDigitIconForTest(1, NewObject<UPaperSprite>(CardView.Get()));
	CardView->SetCostDigitSizeForTest(FVector2D(31.0f, 37.0f));
	CardView->TakeWidget();

	FWacomCardViewData Data;
	Data.Name = FText::FromString(TEXT("缓存测试"));
	Data.Cost = 1;
	Data.bShowCost = true;
	CardView->SetCardViewData(Data);

	UImage* CostDigitImage = CardView->GetCostDigitImageForTest();
	TestNotNull(TEXT("Fallback CardView creates CostDigitImage"), CostDigitImage);
	if (!CostDigitImage)
	{
		return false;
	}

	TestEqual(TEXT("Resolved digit icon shows bound image"),
		CostDigitImage->GetVisibility(),
		ESlateVisibility::HitTestInvisible);
	const FVector2f BrushImageSize = CostDigitImage->GetBrush().GetImageSize();
	TestEqual(TEXT("Resolved cost digit uses stable brush width"),
		BrushImageSize.X,
		31.0f);
	TestEqual(TEXT("Resolved cost digit uses stable brush height"),
		BrushImageSize.Y,
		37.0f);

	CardView->ClearCostDigitIconsForTest();
	Data.Name = FText::FromString(TEXT("缓存测试二次刷新"));
	CardView->SetCardViewData(Data);

	TestEqual(TEXT("Second refresh still uses resolved sprite cache after source map changes"),
		CostDigitImage->GetVisibility(),
		ESlateVisibility::HitTestInvisible);
	TestEqual(TEXT("View data still refreshes while sprite cache stays stable"),
		CardView->GetCardViewData().Name.ToString(),
		FString(TEXT("缓存测试二次刷新")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardViewSingleCostDigitImageSpec,
	"Wacom.UI.Backpack.CardViewSingleCostDigitImageUsesStableBoundWidget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardViewSingleCostDigitImageSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardViewSingleCostDigitProbe> CardView(
		NewObject<UWacomCardViewSingleCostDigitProbe>());
	CardView->SetCostDigitIconForTest(1, NewObject<UPaperSprite>(CardView.Get()));
	CardView->SetCostDigitSizeForTest(FVector2D(29.0f, 41.0f));
	CardView->TakeWidget();

	FWacomCardViewData Data;
	Data.Name = FText::FromString(TEXT("单图费用测试"));
	Data.Cost = 1;
	Data.bShowCost = true;
	CardView->SetCardViewData(Data);

	UImage* CostDigitImage = CardView->GetCostDigitImageForTest();
	TestNotNull(TEXT("Single digit CardView binds CostDigitImage"), CostDigitImage);
	if (!CostDigitImage)
	{
		return false;
	}

	TestEqual(TEXT("Single digit icon shows bound image"),
		CostDigitImage->GetVisibility(),
		ESlateVisibility::HitTestInvisible);
	const FVector2f SingleDigitBrushImageSize = CostDigitImage->GetBrush().GetImageSize();
	TestEqual(TEXT("Single digit bound image uses stable brush width"),
		SingleDigitBrushImageSize.X,
		29.0f);
	TestEqual(TEXT("Single digit bound image uses stable brush height"),
		SingleDigitBrushImageSize.Y,
		41.0f);

	Data.Cost = 10;
	CardView->SetCardViewData(Data);
	TestEqual(TEXT("Multi-digit cost hides bound single digit image on compact card face"),
		CostDigitImage->GetVisibility(),
		ESlateVisibility::Collapsed);

	Data.Cost = 2;
	CardView->SetCardViewData(Data);
	TestEqual(TEXT("Missing single digit sprite keeps compact cost hidden"),
		CostDigitImage->GetVisibility(),
		ESlateVisibility::Collapsed);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardViewDurabilityDigitsReuseSpec,
	"Wacom.UI.Backpack.CardViewDurabilityDigitsReuseImagesAcrossRefresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardViewDurabilityDigitsReuseSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardViewSpecProbe> CardView(NewObject<UWacomCardViewSpecProbe>());
	UPaperSprite* OneSprite = NewObject<UPaperSprite>(CardView.Get());
	UPaperSprite* TwoSprite = NewObject<UPaperSprite>(CardView.Get());
	UPaperSprite* ThreeSprite = NewObject<UPaperSprite>(CardView.Get());
	CardView->SetDurabilityDigitIconForTest(1, OneSprite);
	CardView->SetDurabilityDigitIconForTest(2, TwoSprite);
	CardView->SetDurabilityDigitIconForTest(3, ThreeSprite);
	CardView->TakeWidget();

	FWacomCardViewData Data;
	Data.Name = FText::FromString(TEXT("耐久复用测试"));
	Data.Durability = 12;
	Data.bShowDurability = true;
	CardView->SetCardViewData(Data);

	UPanelWidget* DigitsHost = CardView->GetDurabilityDigitsHostForTest();
	TestNotNull(TEXT("DurabilityDigitsHost exists"), DigitsHost);
	if (!DigitsHost || DigitsHost->GetChildrenCount() < 2)
	{
		return false;
	}

	UWidget* FirstDigit = DigitsHost->GetChildAt(0);
	UWidget* SecondDigit = DigitsHost->GetChildAt(1);
	Data.Durability = 13;
	CardView->SetCardViewData(Data);

	TestEqual(TEXT("Durability digit count remains two"), DigitsHost->GetChildrenCount(), 2);
	TestTrue(TEXT("First durability digit image is reused"), DigitsHost->GetChildAt(0) == FirstDigit);
	TestTrue(TEXT("Second durability digit image is reused"), DigitsHost->GetChildAt(1) == SecondDigit);
	const UImage* ReusedSecondDigit = Cast<UImage>(DigitsHost->GetChildAt(1));
	TestTrue(TEXT("Second reused digit gets new sprite"),
		ReusedSecondDigit && ReusedSecondDigit->GetBrush().GetResourceObject() == ThreeSprite);

	Data.Durability = 1;
	CardView->SetCardViewData(Data);
	TestEqual(TEXT("Durability host trims extra digit image"), DigitsHost->GetChildrenCount(), 1);
	TestTrue(TEXT("First durability digit survives trim"), DigitsHost->GetChildAt(0) == FirstDigit);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardViewRetainerRefreshSpec,
	"Wacom.UI.Backpack.CardViewRefreshInvalidatesRetainerCache",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardViewRetainerRefreshSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardViewRetainerRefreshProbe> CardView(
		NewObject<UWacomCardViewRetainerRefreshProbe>());
	CardView->TakeWidget();

	const FWacomCardViewAutomationTestView InitialView = FWacomCardViewTestAccess::View(*CardView);

	FWacomCardViewData Data;
	Data.Name = FText::FromString(TEXT("Retainer 刷新测试"));
	Data.TypeText = FText::FromString(TEXT("技能"));
	Data.Cost = 1;
	Data.bShowCost = true;
	CardView->SetCardViewData(Data);

	TestEqual(TEXT("SetCardViewData invalidates card render cache once"),
		FWacomCardViewTestAccess::View(*CardView).RenderCacheInvalidationCount,
		InitialView.RenderCacheInvalidationCount + 1);
	TestEqual(TEXT("CardView requests render on its retainer host"),
		FWacomCardViewTestAccess::View(*CardView).LastRetainerRenderRequestCount,
		1);
	TestEqual(TEXT("View data type text is preserved for retainer-backed views"),
		CardView->GetCardViewData().TypeText.ToString(),
		FString(TEXT("技能")));

	CardView->SetCardViewData(Data);
	TestEqual(TEXT("Identical card data refresh does not invalidate retainer every frame"),
		FWacomCardViewTestAccess::View(*CardView).RenderCacheInvalidationCount,
		InitialView.RenderCacheInvalidationCount + 1);
	TestEqual(TEXT("Identical refresh does not request retainer render"),
		FWacomCardViewTestAccess::View(*CardView).LastRetainerRenderRequestCount,
		1);

	Data.TypeText = FText::FromString(TEXT("伙伴"));
	CardView->SetCardViewData(Data);
	TestEqual(TEXT("Second SetCardViewData also invalidates card render cache"),
		FWacomCardViewTestAccess::View(*CardView).RenderCacheInvalidationCount,
		InitialView.RenderCacheInvalidationCount + 2);
	TestEqual(TEXT("Second refresh requests retainer render again"),
		FWacomCardViewTestAccess::View(*CardView).LastRetainerRenderRequestCount,
		1);
	TestEqual(TEXT("Second type text refresh is preserved"),
		CardView->GetCardViewData().TypeText.ToString(),
		FString(TEXT("伙伴")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardViewEquivalentDataDirtyGateSpec,
	"Wacom.UI.Backpack.CardViewSetCardViewDataSkipsEquivalentApplyAndRenderInvalidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardViewEquivalentDataDirtyGateSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardViewSpecProbe> CardView(NewObject<UWacomCardViewSpecProbe>());
	CardView->SetCostDigitIconForTest(1, NewObject<UPaperSprite>(CardView.Get()));
	CardView->TakeWidget();

	FWacomCardViewData Data;
	Data.Name = FText::FromString(TEXT("等价刷新测试"));
	Data.TypeText = FText::FromString(TEXT("技能"));
	Data.Cost = 1;
	Data.bShowCost = true;
	Data.EffectBadges.Add(MakeCardViewEffectBadgeForTest(EWacomCardViewEffectBadgeKind::Damage, 7));

	CardView->SetCardViewData(Data);
	const FWacomCardViewAutomationTestView InitialView = FWacomCardViewTestAccess::View(*CardView);

	CardView->SetCardViewData(Data);

	TestEqual(TEXT("Equivalent card data skips render invalidation"),
		FWacomCardViewTestAccess::View(*CardView).RenderCacheInvalidationCount,
		InitialView.RenderCacheInvalidationCount);
	TestEqual(TEXT("Equivalent card data skips text update"),
		FWacomCardViewTestAccess::View(*CardView).TextDisplayUpdateCount,
		InitialView.TextDisplayUpdateCount);
	TestEqual(TEXT("Equivalent card data skips cost update"),
		FWacomCardViewTestAccess::View(*CardView).CostDisplayUpdateCount,
		InitialView.CostDisplayUpdateCount);
	TestEqual(TEXT("Equivalent card data skips badge update"),
		FWacomCardViewTestAccess::View(*CardView).EffectBadgeDisplayUpdateCount,
		InitialView.EffectBadgeDisplayUpdateCount);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardViewSectionDirtyGateSpec,
	"Wacom.UI.Backpack.CardViewDataDirtyGateRefreshesOnlyChangedSections",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardViewSectionDirtyGateSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardViewSpecProbe> CardView(NewObject<UWacomCardViewSpecProbe>());
	CardView->SetCostDigitIconForTest(1, NewObject<UPaperSprite>(CardView.Get()));
	CardView->SetCostDigitIconForTest(2, NewObject<UPaperSprite>(CardView.Get()));
	CardView->SetDurabilityDigitIconForTest(3, NewObject<UPaperSprite>(CardView.Get()));
	CardView->SetRarityBorderSpriteForTest(WacomTags::Card_Rarity_Blue, NewObject<UPaperSprite>(CardView.Get()));
	CardView->TakeWidget();

	FWacomCardViewData Data;
	Data.Name = FText::FromString(TEXT("分区刷新测试"));
	Data.TypeText = FText::FromString(TEXT("技能"));
	Data.Cost = 1;
	Data.bShowCost = true;
	Data.Rarity = WacomTags::Card_Rarity_Blue;
	Data.Durability = 3;
	Data.bShowDurability = true;
	Data.bDisabled = false;
	Data.Art = NewObject<UTexture2D>(CardView.Get());
	Data.EffectBadges.Add(MakeCardViewEffectBadgeForTest(EWacomCardViewEffectBadgeKind::Damage, 7));
	CardView->SetCardViewData(Data);

	const FWacomCardViewAutomationTestView BaseView = FWacomCardViewTestAccess::View(*CardView);

	FWacomCardViewData CostChanged = Data;
	CostChanged.Cost = 2;
	CardView->SetCardViewData(CostChanged);

	const FWacomCardViewAutomationTestView CostChangedView = FWacomCardViewTestAccess::View(*CardView);
	TestEqual(TEXT("Cost change invalidates once"), CostChangedView.RenderCacheInvalidationCount, BaseView.RenderCacheInvalidationCount + 1);
	TestEqual(TEXT("Cost section refreshed"), CostChangedView.CostDisplayUpdateCount, BaseView.CostDisplayUpdateCount + 1);
	TestEqual(TEXT("Text section not refreshed by cost only"), CostChangedView.TextDisplayUpdateCount, BaseView.TextDisplayUpdateCount);
	TestEqual(TEXT("Durability section not refreshed by cost only"), CostChangedView.DurabilityDisplayUpdateCount, BaseView.DurabilityDisplayUpdateCount);
	TestEqual(TEXT("Rarity section not refreshed by cost only"), CostChangedView.RarityDisplayUpdateCount, BaseView.RarityDisplayUpdateCount);
	TestEqual(TEXT("Art section not refreshed by cost only"), CostChangedView.ArtDisplayUpdateCount, BaseView.ArtDisplayUpdateCount);
	TestEqual(TEXT("Disabled section not refreshed by cost only"), CostChangedView.DisabledDisplayUpdateCount, BaseView.DisabledDisplayUpdateCount);
	TestEqual(TEXT("Badge section not refreshed by cost only"), CostChangedView.EffectBadgeDisplayUpdateCount, BaseView.EffectBadgeDisplayUpdateCount);

	FWacomCardViewData BadgeChanged = CostChanged;
	BadgeChanged.EffectBadges[0].Value = 8;
	CardView->SetCardViewData(BadgeChanged);
	const FWacomCardViewAutomationTestView BadgeChangedView = FWacomCardViewTestAccess::View(*CardView);
	TestEqual(TEXT("Badge change invalidates once more"), BadgeChangedView.RenderCacheInvalidationCount, BaseView.RenderCacheInvalidationCount + 2);
	TestEqual(TEXT("Badge section refreshed"), BadgeChangedView.EffectBadgeDisplayUpdateCount, BaseView.EffectBadgeDisplayUpdateCount + 1);
	TestEqual(TEXT("Cost section not refreshed by badge only"), BadgeChangedView.CostDisplayUpdateCount, BaseView.CostDisplayUpdateCount + 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardViewBuildSummarySpec,
	"Wacom.UI.Backpack.CardViewBuildSummary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardViewBuildSummarySpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	Card->CardId = TEXT("Shuoguangdie");
	Card->DisplayName = FText::FromString(TEXT("烁光蝶"));
	Card->BaseCost = 1;
	Card->Rarity = WacomTags::Card_Rarity_White;
	Card->Keywords.AddTag(WacomTags::Card_Keyword_Companion);
	Card->Keywords.AddTag(WacomTags::Card_Keyword_Weapon);
	Card->Physique.MaxHpBonus = 6;

	FCardEffect Shield;
	Shield.EffectType = WacomTags::Status_Shield;
	Shield.Magnitude = 5;
	Card->Effects.Add(Shield);

	FCardEffect Damage;
	Damage.EffectType = WacomTags::Effect_Damage;
	Damage.Magnitude = 7;
	Card->Effects.Add(Damage);

	FCardEffect Freeze;
	Freeze.EffectType = WacomTags::Effect_ApplyStatus_Freeze;
	Freeze.Magnitude = 1;
	Card->Effects.Add(Freeze);

	const FWacomCardViewData Data = UWacomCardPresentationBuilder::BuildCardViewData(Card.Get());

	TestEqual(TEXT("Summary name"), Data.Name.ToString(), TEXT("烁光蝶"));
	TestEqual(TEXT("Summary cost"), Data.Cost, 1);
	TestTrue(TEXT("White rarity exposes value"), Data.bShowValue);
	TestEqual(TEXT("White rarity value"), Data.Value, 1);
	TestTrue(TEXT("Physique summary visible"), Data.bShowPhysique);
	TestTrue(TEXT("Physique summary contains max hp"), Data.PhysiqueText.ToString().Contains(TEXT("6")));
	TestTrue(TEXT("Keyword line contains localized companion"), Data.TypeText.ToString().Contains(TEXT("伙伴")));
	TestTrue(TEXT("Keyword line contains localized weapon"), Data.TypeText.ToString().Contains(TEXT("武器")));
	TestEqual(TEXT("Only art-backed effect badges appear on card face data"), Data.EffectBadges.Num(), 2);
	if (Data.EffectBadges.Num() >= 2)
	{
		TestTrue(TEXT("First badge is shield"), Data.EffectBadges[0].Kind == EWacomCardViewEffectBadgeKind::Shield);
		TestEqual(TEXT("Shield badge value"), Data.EffectBadges[0].Value, 5);
		TestTrue(TEXT("Second badge is damage"), Data.EffectBadges[1].Kind == EWacomCardViewEffectBadgeKind::Damage);
		TestEqual(TEXT("Damage badge value"), Data.EffectBadges[1].Value, 7);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardViewEffectStatsHostFiltersUnsupportedKindsSpec,
	"Wacom.UI.Backpack.CardViewEffectStatsHostFiltersUnsupportedKinds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardViewEffectStatsHostFiltersUnsupportedKindsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardViewSpecProbe> CardView(NewObject<UWacomCardViewSpecProbe>());
	CardView->TakeWidget();

	FWacomCardViewData Data;
	Data.Name = FText::FromString(TEXT("徽章过滤测试"));

	FWacomCardViewEffectBadge DamageBadge;
	DamageBadge.Kind = EWacomCardViewEffectBadgeKind::Damage;
	DamageBadge.Value = 7;
	Data.EffectBadges.Add(DamageBadge);

	FWacomCardViewEffectBadge FreezeBadge;
	FreezeBadge.Kind = EWacomCardViewEffectBadgeKind::Freeze;
	FreezeBadge.Value = 1;
	Data.EffectBadges.Add(FreezeBadge);

	FWacomCardViewEffectBadge ShieldBadge;
	ShieldBadge.Kind = EWacomCardViewEffectBadgeKind::Shield;
	ShieldBadge.Value = 5;
	Data.EffectBadges.Add(ShieldBadge);

	CardView->SetCardViewData(Data);

	UPanelWidget* EffectStatsHost = CardView->GetEffectStatsHostForTest();
	TestNotNull(TEXT("Fallback CardView creates EffectStatsHost"), EffectStatsHost);
	if (!EffectStatsHost)
	{
		return false;
	}

	TestEqual(TEXT("EffectStatsHost renders only art-backed badge kinds"), EffectStatsHost->GetChildrenCount(), 2);
	TestEqual(TEXT("EffectStatsHost remains visible when supported badges exist"),
		EffectStatsHost->GetVisibility(),
		ESlateVisibility::HitTestInvisible);

	Data.EffectBadges.Reset();
	Data.EffectBadges.Add(FreezeBadge);
	CardView->SetCardViewData(Data);

	TestEqual(TEXT("EffectStatsHost renders no unsupported-only badges"), EffectStatsHost->GetChildrenCount(), 0);
	TestEqual(TEXT("EffectStatsHost collapses when only unsupported badges exist"),
		EffectStatsHost->GetVisibility(),
		ESlateVisibility::Collapsed);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardViewEffectBadgeSlotsFillSequentiallySpec,
	"Wacom.UI.Backpack.CardViewEffectBadgeSlotsFillSequentially",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardViewEffectBadgeSlotsFillSequentiallySpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardViewEffectBadgeSlotProbe> CardView(NewObject<UWacomCardViewEffectBadgeSlotProbe>());
	CardView->TakeWidget();

	FWacomCardViewData Data;
	Data.EffectBadges.Add(MakeCardViewEffectBadgeForTest(EWacomCardViewEffectBadgeKind::Damage, 7));
	Data.EffectBadges.Add(MakeCardViewEffectBadgeForTest(EWacomCardViewEffectBadgeKind::Freeze, 1));
	Data.EffectBadges.Add(MakeCardViewEffectBadgeForTest(EWacomCardViewEffectBadgeKind::Shield, 5));
	Data.EffectBadges.Add(MakeCardViewEffectBadgeForTest(EWacomCardViewEffectBadgeKind::Poison, 2));
	CardView->SetCardViewData(Data);

	UPanelWidget* LegacyHost = CardView->GetEffectStatsHostForTest();
	TestNotNull(TEXT("Slot probe has legacy EffectStatsHost"), LegacyHost);
	if (LegacyHost)
	{
		TestEqual(TEXT("Slot mode clears legacy host"), LegacyHost->GetChildrenCount(), 0);
		TestEqual(TEXT("Slot mode collapses legacy host"), LegacyHost->GetVisibility(), ESlateVisibility::Collapsed);
	}

	const UWacomCardEffectBadgeWidget* Slot1Badge = GetSingleSlotBadgeForTest(CardView->GetEffectBadgeSlotForTest(0));
	const UWacomCardEffectBadgeWidget* Slot2Badge = GetSingleSlotBadgeForTest(CardView->GetEffectBadgeSlotForTest(1));
	const UWacomCardEffectBadgeWidget* Slot3Badge = GetSingleSlotBadgeForTest(CardView->GetEffectBadgeSlotForTest(2));
	TestNotNull(TEXT("Slot1 has badge"), Slot1Badge);
	TestNotNull(TEXT("Slot2 has badge"), Slot2Badge);
	TestNotNull(TEXT("Slot3 has badge"), Slot3Badge);
	if (Slot1Badge && Slot2Badge && Slot3Badge)
	{
		TestTrue(TEXT("Slot1 receives first supported badge"),
			Slot1Badge->GetEffectBadgeData().Kind == EWacomCardViewEffectBadgeKind::Damage);
		TestTrue(TEXT("Slot2 skips unsupported Freeze and receives Shield"),
			Slot2Badge->GetEffectBadgeData().Kind == EWacomCardViewEffectBadgeKind::Shield);
		TestTrue(TEXT("Slot3 receives next supported badge"),
			Slot3Badge->GetEffectBadgeData().Kind == EWacomCardViewEffectBadgeKind::Poison);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardViewEffectBadgeSlotsHideEmptyAndOverflowSpec,
	"Wacom.UI.Backpack.CardViewEffectBadgeSlotsHideEmptyAndOverflow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardViewEffectBadgeSlotsHideEmptyAndOverflowSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardViewEffectBadgeSlotProbe> CardView(NewObject<UWacomCardViewEffectBadgeSlotProbe>());
	CardView->TakeWidget();

	FWacomCardViewData Data;
	for (int32 Index = 0; Index < 5; ++Index)
	{
		Data.EffectBadges.Add(MakeCardViewEffectBadgeForTest(EWacomCardViewEffectBadgeKind::Damage, Index + 1));
	}
	CardView->SetCardViewData(Data);

	for (int32 SlotIndex = 0; SlotIndex < 4; ++SlotIndex)
	{
		UPanelWidget* Slot = CardView->GetEffectBadgeSlotForTest(SlotIndex);
		TestNotNull(FString::Printf(TEXT("Slot %d exists"), SlotIndex + 1), Slot);
		if (Slot)
		{
			TestEqual(FString::Printf(TEXT("Slot %d renders one badge"), SlotIndex + 1), Slot->GetChildrenCount(), 1);
			TestEqual(FString::Printf(TEXT("Slot %d is visible"), SlotIndex + 1),
				Slot->GetVisibility(),
				ESlateVisibility::HitTestInvisible);
		}
	}

	Data.EffectBadges.SetNum(2);
	CardView->SetCardViewData(Data);

	for (int32 SlotIndex = 0; SlotIndex < 4; ++SlotIndex)
	{
		UPanelWidget* Slot = CardView->GetEffectBadgeSlotForTest(SlotIndex);
		if (!Slot)
		{
			continue;
		}

		const bool bExpectedOccupied = SlotIndex < 2;
		TestTrue(FString::Printf(TEXT("Slot %d keeps at most one reusable badge child after short refresh"), SlotIndex + 1),
			Slot->GetChildrenCount() <= 1);
		TestEqual(FString::Printf(TEXT("Slot %d visibility after short refresh"), SlotIndex + 1),
			Slot->GetVisibility(),
			bExpectedOccupied ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardViewEffectBadgeSlotsIgnoreUnsupportedKindsSpec,
	"Wacom.UI.Backpack.CardViewEffectBadgeSlotsIgnoreUnsupportedKinds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardViewEffectBadgeSlotsIgnoreUnsupportedKindsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardViewEffectBadgeSlotProbe> CardView(NewObject<UWacomCardViewEffectBadgeSlotProbe>());
	CardView->TakeWidget();

	FWacomCardViewData Data;
	Data.EffectBadges.Add(MakeCardViewEffectBadgeForTest(EWacomCardViewEffectBadgeKind::Freeze, 1));
	Data.EffectBadges.Add(MakeCardViewEffectBadgeForTest(EWacomCardViewEffectBadgeKind::Slow, 2));
	Data.EffectBadges.Add(MakeCardViewEffectBadgeForTest(EWacomCardViewEffectBadgeKind::Draw, 3));
	CardView->SetCardViewData(Data);

	for (int32 SlotIndex = 0; SlotIndex < 4; ++SlotIndex)
	{
		UPanelWidget* Slot = CardView->GetEffectBadgeSlotForTest(SlotIndex);
		if (!Slot)
		{
			continue;
		}

		TestEqual(FString::Printf(TEXT("Unsupported-only Slot %d has no badge"), SlotIndex + 1),
			Slot->GetChildrenCount(),
			0);
		TestEqual(FString::Printf(TEXT("Unsupported-only Slot %d collapsed"), SlotIndex + 1),
			Slot->GetVisibility(),
			ESlateVisibility::Collapsed);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardViewEffectBadgeSlotsReuseSpec,
	"Wacom.UI.Backpack.CardViewEffectBadgeSlotsReuseWidgetsAcrossRefresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardViewEffectBadgeSlotsReuseSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardViewEffectBadgeSlotProbe> CardView(NewObject<UWacomCardViewEffectBadgeSlotProbe>());
	CardView->TakeWidget();

	FWacomCardViewData Data;
	Data.EffectBadges.Add(MakeCardViewEffectBadgeForTest(EWacomCardViewEffectBadgeKind::Damage, 7));
	Data.EffectBadges.Add(MakeCardViewEffectBadgeForTest(EWacomCardViewEffectBadgeKind::Shield, 5));
	CardView->SetCardViewData(Data);

	const UWacomCardEffectBadgeWidget* FirstSlotInitial =
		GetSingleSlotBadgeForTest(CardView->GetEffectBadgeSlotForTest(0));
	const UWacomCardEffectBadgeWidget* SecondSlotInitial =
		GetSingleSlotBadgeForTest(CardView->GetEffectBadgeSlotForTest(1));
	TestNotNull(TEXT("Initial first slot badge"), FirstSlotInitial);
	TestNotNull(TEXT("Initial second slot badge"), SecondSlotInitial);
	if (!FirstSlotInitial || !SecondSlotInitial)
	{
		return false;
	}

	Data.EffectBadges[0].Value = 8;
	Data.EffectBadges[1].Value = 6;
	CardView->SetCardViewData(Data);

	TestTrue(TEXT("First slot reuses badge widget"),
		GetSingleSlotBadgeForTest(CardView->GetEffectBadgeSlotForTest(0)) == FirstSlotInitial);
	TestTrue(TEXT("Second slot reuses badge widget"),
		GetSingleSlotBadgeForTest(CardView->GetEffectBadgeSlotForTest(1)) == SecondSlotInitial);
	TestEqual(TEXT("Reused first slot badge gets updated value"),
		FirstSlotInitial->GetEffectBadgeData().Value,
		8);

	Data.EffectBadges.SetNum(1);
	CardView->SetCardViewData(Data);
	UPanelWidget* SecondSlot = CardView->GetEffectBadgeSlotForTest(1);
	TestNotNull(TEXT("Second slot still exists"), SecondSlot);
	if (SecondSlot)
	{
		TestEqual(TEXT("Second slot collapses when no badge is assigned"),
			SecondSlot->GetVisibility(),
			ESlateVisibility::Collapsed);
		TestTrue(TEXT("Second slot may retain one reusable hidden child"),
			SecondSlot->GetChildrenCount() <= 1);
	}

	Data.EffectBadges.Add(MakeCardViewEffectBadgeForTest(EWacomCardViewEffectBadgeKind::Shield, 9));
	CardView->SetCardViewData(Data);
	TestTrue(TEXT("Second slot reuses hidden badge when occupied again"),
		GetSingleSlotBadgeForTest(CardView->GetEffectBadgeSlotForTest(1)) == SecondSlotInitial);
	TestEqual(TEXT("Reused hidden badge receives new value"),
		SecondSlotInitial->GetEffectBadgeData().Value,
		9);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardViewEffectStatsHostReuseSpec,
	"Wacom.UI.Backpack.CardViewEffectStatsHostFallbackReusesWidgetsAcrossRefresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardViewEffectStatsHostReuseSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardViewSpecProbe> CardView(NewObject<UWacomCardViewSpecProbe>());
	CardView->TakeWidget();

	FWacomCardViewData Data;
	Data.EffectBadges.Add(MakeCardViewEffectBadgeForTest(EWacomCardViewEffectBadgeKind::Damage, 7));
	Data.EffectBadges.Add(MakeCardViewEffectBadgeForTest(EWacomCardViewEffectBadgeKind::Shield, 5));
	CardView->SetCardViewData(Data);

	UPanelWidget* Host = CardView->GetEffectStatsHostForTest();
	TestNotNull(TEXT("Fallback EffectStatsHost exists"), Host);
	if (!Host || Host->GetChildrenCount() < 2)
	{
		return false;
	}

	UWacomCardEffectBadgeWidget* FirstBadge = Cast<UWacomCardEffectBadgeWidget>(Host->GetChildAt(0));
	UWacomCardEffectBadgeWidget* SecondBadge = Cast<UWacomCardEffectBadgeWidget>(Host->GetChildAt(1));
	TestNotNull(TEXT("First fallback badge"), FirstBadge);
	TestNotNull(TEXT("Second fallback badge"), SecondBadge);
	if (!FirstBadge || !SecondBadge)
	{
		return false;
	}

	Data.EffectBadges[0].Value = 8;
	Data.EffectBadges[1].Value = 6;
	CardView->SetCardViewData(Data);
	TestTrue(TEXT("First fallback badge is reused"), Host->GetChildAt(0) == FirstBadge);
	TestTrue(TEXT("Second fallback badge is reused"), Host->GetChildAt(1) == SecondBadge);
	TestEqual(TEXT("Fallback reused badge gets updated data"), FirstBadge->GetEffectBadgeData().Value, 8);

	Data.EffectBadges.SetNum(1);
	CardView->SetCardViewData(Data);
	TestEqual(TEXT("Fallback host trims extra badge widgets"), Host->GetChildrenCount(), 1);
	TestTrue(TEXT("Fallback first badge remains reused after trim"), Host->GetChildAt(0) == FirstBadge);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardEffectBadgeWidgetDataSpec,
	"Wacom.UI.Backpack.CardEffectBadgeWidgetData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardEffectBadgeWidgetDataSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardEffectBadgeWidget> BadgeWidget(NewObject<UWacomCardEffectBadgeWidget>());

	FWacomCardViewEffectBadge Badge;
	Badge.Kind = EWacomCardViewEffectBadgeKind::Damage;
	Badge.Value = 7;
	Badge.DisplayText = FText::FromString(TEXT("伤7"));

	BadgeWidget->SetEffectBadgeData(Badge);
	BadgeWidget->TakeWidget();
	BadgeWidget->SetEffectBadgeData(Badge);

	TestTrue(TEXT("Badge kind preserved"),
		BadgeWidget->GetEffectBadgeData().Kind == EWacomCardViewEffectBadgeKind::Damage);
	TestEqual(TEXT("Badge value preserved"), BadgeWidget->GetEffectBadgeData().Value, 7);
	TestEqual(TEXT("Value text getter reports current value"), BadgeWidget->GetValueText().ToString(), TEXT("7"));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardEffectBadgeWidgetThreeDigitSpec,
	"Wacom.UI.Backpack.CardEffectBadgeWidgetUsesThreeDigitImages",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardEffectBadgeWidgetThreeDigitSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardEffectBadgeSpecProbe> BadgeWidget(NewObject<UWacomCardEffectBadgeSpecProbe>());
	UPaperSprite* ZeroSprite = NewObject<UPaperSprite>(BadgeWidget.Get());
	UPaperSprite* SevenSprite = NewObject<UPaperSprite>(BadgeWidget.Get());
	BadgeWidget->SetDigitSpriteForTest(0, ZeroSprite);
	BadgeWidget->SetDigitSpriteForTest(7, SevenSprite);
	BadgeWidget->SetMinimumDigitCountForTest(3);
	BadgeWidget->SetInteriorDigitPaddingForTest(FMargin(1.0f, 0.0f, 1.0f, 0.0f));
	BadgeWidget->TakeWidget();

	FWacomCardViewEffectBadge Badge;
	Badge.Kind = EWacomCardViewEffectBadgeKind::Damage;
	Badge.Value = 7;
	BadgeWidget->SetEffectBadgeData(Badge);

	UPanelWidget* DigitHost = BadgeWidget->GetDigitHostForTest();
	TestNotNull(TEXT("Effect badge fallback creates DigitHost"), DigitHost);
	if (!DigitHost)
	{
		return false;
	}

	TestEqual(TEXT("Single digit value renders as three image digits"), DigitHost->GetChildrenCount(), 3);
	if (DigitHost->GetChildrenCount() >= 3)
	{
		const UImage* FirstDigit = Cast<UImage>(DigitHost->GetChildAt(0));
		const UImage* SecondDigit = Cast<UImage>(DigitHost->GetChildAt(1));
		const UImage* ThirdDigit = Cast<UImage>(DigitHost->GetChildAt(2));
		TestTrue(TEXT("First digit is zero sprite"),
			FirstDigit && FirstDigit->GetBrush().GetResourceObject() == ZeroSprite);
		TestTrue(TEXT("Second digit is zero sprite"),
			SecondDigit && SecondDigit->GetBrush().GetResourceObject() == ZeroSprite);
		TestTrue(TEXT("Third digit is seven sprite"),
			ThirdDigit && ThirdDigit->GetBrush().GetResourceObject() == SevenSprite);

		const UHorizontalBoxSlot* MiddleSlot = SecondDigit ? Cast<UHorizontalBoxSlot>(SecondDigit->Slot) : nullptr;
		TestNotNull(TEXT("Middle digit has horizontal slot"), MiddleSlot);
		if (MiddleSlot)
		{
			const FMargin Padding = MiddleSlot->GetPadding();
			TestEqual(TEXT("Middle digit left padding"), Padding.Left, 1.0f);
			TestEqual(TEXT("Middle digit right padding"), Padding.Right, 1.0f);
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardEffectBadgeDigitReuseSpec,
	"Wacom.UI.Backpack.CardEffectBadgeDigitImagesReuseAcrossValueRefresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardEffectBadgeDigitReuseSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardEffectBadgeSpecProbe> BadgeWidget(NewObject<UWacomCardEffectBadgeSpecProbe>());
	UPaperSprite* ZeroSprite = NewObject<UPaperSprite>(BadgeWidget.Get());
	UPaperSprite* OneSprite = NewObject<UPaperSprite>(BadgeWidget.Get());
	UPaperSprite* TwoSprite = NewObject<UPaperSprite>(BadgeWidget.Get());
	UPaperSprite* ThreeSprite = NewObject<UPaperSprite>(BadgeWidget.Get());
	BadgeWidget->SetDigitSpriteForTest(0, ZeroSprite);
	BadgeWidget->SetDigitSpriteForTest(1, OneSprite);
	BadgeWidget->SetDigitSpriteForTest(2, TwoSprite);
	BadgeWidget->SetDigitSpriteForTest(3, ThreeSprite);
	BadgeWidget->SetMinimumDigitCountForTest(3);
	BadgeWidget->TakeWidget();

	FWacomCardViewEffectBadge Badge;
	Badge.Kind = EWacomCardViewEffectBadgeKind::Damage;
	Badge.Value = 12;
	BadgeWidget->SetEffectBadgeData(Badge);

	UPanelWidget* DigitHost = BadgeWidget->GetDigitHostForTest();
	TestNotNull(TEXT("Effect badge DigitHost exists"), DigitHost);
	if (!DigitHost || DigitHost->GetChildrenCount() < 3)
	{
		return false;
	}

	UWidget* FirstDigit = DigitHost->GetChildAt(0);
	UWidget* SecondDigit = DigitHost->GetChildAt(1);
	UWidget* ThirdDigit = DigitHost->GetChildAt(2);
	const FWacomCardEffectBadgeAutomationTestView InitialView = FWacomCardViewTestAccess::View(*BadgeWidget);

	BadgeWidget->SetEffectBadgeData(Badge);
	TestEqual(TEXT("Equivalent badge data skips apply"),
		FWacomCardViewTestAccess::View(*BadgeWidget).ApplyCount,
		InitialView.ApplyCount);
	TestEqual(TEXT("Equivalent badge data skips digit update"),
		FWacomCardViewTestAccess::View(*BadgeWidget).DigitImageUpdateCount,
		InitialView.DigitImageUpdateCount);

	Badge.Value = 13;
	BadgeWidget->SetEffectBadgeData(Badge);
	TestEqual(TEXT("Effect badge keeps three digit images"), DigitHost->GetChildrenCount(), 3);
	TestTrue(TEXT("First effect digit image reused"), DigitHost->GetChildAt(0) == FirstDigit);
	TestTrue(TEXT("Second effect digit image reused"), DigitHost->GetChildAt(1) == SecondDigit);
	TestTrue(TEXT("Third effect digit image reused"), DigitHost->GetChildAt(2) == ThirdDigit);
	const UImage* ReusedThirdDigit = Cast<UImage>(DigitHost->GetChildAt(2));
	TestTrue(TEXT("Third reused digit gets updated sprite"),
		ReusedThirdDigit && ReusedThirdDigit->GetBrush().GetResourceObject() == ThreeSprite);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackZoneSectionWidgetDataSpec,
	"Wacom.UI.Backpack.ZoneSectionWidgetData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackZoneSectionWidgetDataSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBackpackZoneSectionWidget> Section(NewObject<UWacomBackpackZoneSectionWidget>());

	Section->SetZoneTitleText(FText::FromString(TEXT("[ 备战区 ] 5 / 15")));
	TestNotNull(TEXT("Zone section EnsureContentHost builds fallback"), Section->EnsureContentHost());
	Section->SetZoneTitleText(FText::FromString(TEXT("[ 备战区 ] 6 / 15")));

	TestEqual(TEXT("Zone section title preserved"), Section->GetZoneTitleText().ToString(), TEXT("[ 备战区 ] 6 / 15"));
	TestNotNull(TEXT("Zone section fallback creates content host"), Section->GetContentHost());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardDetailBuildDataSpec,
	"Wacom.UI.Backpack.CardDetailBuildData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardDetailBuildDataSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	Card->CardId = TEXT("TwilightLantern");
	Card->DisplayName = FText::FromString(TEXT("暮色引虫灯"));
	FCardEffect Poison;
	Poison.EffectType = WacomTags::Effect_ApplyStatus_Poison;
	Poison.Magnitude = 1;
	Card->Effects.Add(Poison);

	FCardPassive Passive;
	Passive.Trigger = WacomTags::Passive_Trigger_OnCompanionCount;
	Passive.TriggerThreshold = 3;
	Card->Passives.Add(Passive);

	const FWacomCardDetailViewData Data = UWacomCardPresentationBuilder::BuildCardDetailViewData(Card.Get());

	TestEqual(TEXT("Detail name"), Data.Name.ToString(), TEXT("暮色引虫灯"));
	TestEqual(TEXT("Detail description section uses explanation template"),
		JoinCardDetailSectionTextForTest(Data, EWacomCardDetailSectionKind::Description),
		TEXT("施加 1 层 中毒。"));
	TestFalse(TEXT("Description section does not contain passive copy"),
		JoinCardDetailSectionTextForTest(Data, EWacomCardDetailSectionKind::Description).Contains(TEXT("被动")));
	TestTrue(TEXT("Detail document has no task section before schema support"),
		JoinCardDetailSectionTextForTest(Data, EWacomCardDetailSectionKind::Task).IsEmpty());
	TestTrue(TEXT("Passive section uses trigger template"),
		JoinCardDetailSectionTextForTest(Data, EWacomCardDetailSectionKind::Passive)
			.Contains(TEXT("每打出 3 张伙伴：")));
	TestTrue(TEXT("Passive section uses trigger outcome template"),
		JoinCardDetailSectionTextForTest(Data, EWacomCardDetailSectionKind::Passive)
			.Contains(TEXT("使此牌回到手中。")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardDetailPassiveFallbackSpec,
	"Wacom.UI.Backpack.CardDetailPassiveFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardDetailPassiveFallbackSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	Card->CardId = TEXT("PassiveFallbackCard");
	Card->DisplayName = FText::FromString(TEXT("被动回退卡"));
	FCardEffect Damage;
	Damage.EffectType = WacomTags::Effect_Damage;
	Damage.Magnitude = 2;
	Card->Effects.Add(Damage);

	FCardPassive Passive;
	Passive.Trigger = WacomTags::Passive_Trigger_OnCompanionCount;
	Passive.TriggerThreshold = 3;
	Card->Passives.Add(Passive);

	const FWacomCardDetailViewData Data = UWacomCardPresentationBuilder::BuildCardDetailViewData(Card.Get());

	const FString PassiveSectionText =
		JoinCardDetailSectionTextForTest(Data, EWacomCardDetailSectionKind::Passive);
	TestTrue(TEXT("Fallback passive section contains threshold"), PassiveSectionText.Contains(TEXT("3")));
	TestTrue(TEXT("Fallback passive section contains companion"), PassiveSectionText.Contains(TEXT("伙伴")));
	TestTrue(TEXT("Fallback passive section contains outcome"), PassiveSectionText.Contains(TEXT("使此牌回到手中。")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardDetailPanelSectionsSpec,
	"Wacom.UI.Backpack.CardDetailPanelSections",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardDetailPanelSectionsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardDetailPanel> Panel(NewObject<UWacomCardDetailPanel>());

	FWacomCardDetailViewData Data;
	Data.Name = FText::FromString(TEXT("详情测试卡"));

	FWacomCardDetailSection DescriptionSection;
	DescriptionSection.SectionId = FName(TEXT("Description"));
	DescriptionSection.Kind = EWacomCardDetailSectionKind::Description;
	DescriptionSection.Title = FText::FromString(TEXT("描述"));
	DescriptionSection.Blocks.Add(MakeCardDetailTextBlockForTest(
		FName(TEXT("Description.0.Block")),
		EWacomCardDetailBlockKind::Paragraph,
		TEXT("完整描述文本")));
	Data.Sections.Add(DescriptionSection);

	FWacomCardDetailSection PassiveSection;
	PassiveSection.SectionId = FName(TEXT("Passive"));
	PassiveSection.Kind = EWacomCardDetailSectionKind::Passive;
	PassiveSection.Title = FText::FromString(TEXT("被动"));
	PassiveSection.Blocks.Add(MakeCardDetailTextBlockForTest(
		FName(TEXT("Passive.0.Block")),
		EWacomCardDetailBlockKind::Paragraph,
		TEXT("回合结束")));
	Data.Sections.Add(PassiveSection);

	Panel->SetCardDetailData(Data);
	Panel->TakeWidget();
	Panel->SetCardDetailData(Data);

	TestEqual(TEXT("Detail panel preserves name"), Panel->GetCardDetailData().Name.ToString(), TEXT("详情测试卡"));
	TestEqual(TEXT("Detail panel name getter"), Panel->GetNameText().ToString(), TEXT("详情测试卡"));
	TestEqual(TEXT("Detail panel creates description and passive sections"), Panel->GetSectionCount(), 2);
	TestEqual(TEXT("First section is description"), Panel->GetSectionTitleText(0).ToString(), TEXT("描述"));
	TestEqual(TEXT("Second section is passive"), Panel->GetSectionTitleText(1).ToString(), TEXT("被动"));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardDetailSectionWidgetDataSpec,
	"Wacom.UI.Backpack.CardDetailSectionWidgetData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardDetailSectionWidgetDataSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardDetailSectionWidget> SectionWidget(NewObject<UWacomCardDetailSectionWidget>());

	FWacomCardDetailSectionData Data;
	Data.Title = FText::FromString(TEXT("描述"));
	Data.RichText = FText::FromString(TEXT("第一行\n第二行"));

	SectionWidget->SetSectionData(Data);
	SectionWidget->TakeWidget();
	SectionWidget->SetSectionData(Data);

	TestEqual(TEXT("Section title preserved"), SectionWidget->GetTitleText().ToString(), TEXT("描述"));
	TestEqual(TEXT("Section rich text preserved"), SectionWidget->GetBodyRichText().ToString(), TEXT("第一行\n第二行"));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackToastTextSpec,
	"Wacom.UI.Backpack.ToastText",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackToastTextSpec::RunTest(const FString& /*Parameters*/)
{
	// Command flow owns these player-facing texts; passive widgets never format rule failures themselves.
	TestEqual(TEXT("Move success target name"),
		FWacomBackpackScreenTestAccess::BuildMoveZoneNameText(EZoneKind::BattleDeck).ToString(),
		FString(TEXT("备战区")));
	TestEqual(TEXT("Move flux full reason"),
		FWacomBackpackScreenTestAccess::BuildMoveFailureToastText(TEXT("FluxFull")).ToString(),
		FString(TEXT("无法移动：通量区已满。")));
	TestEqual(TEXT("Move battle deck full reason"),
		FWacomBackpackScreenTestAccess::BuildMoveFailureToastText(TEXT("BattleDeckFull")).ToString(),
		FString(TEXT("无法移动：备战区已满。")));
	TestEqual(TEXT("Move unknown helper reason falls back"),
		FWacomBackpackScreenTestAccess::BuildMoveFailureToastText(TEXT("RunSessionMissing")).ToString(),
		FString(TEXT("无法移动：当前规则不允许。")));
	TestEqual(TEXT("Delete missing card reason"),
		FWacomBackpackScreenTestAccess::BuildDeleteFailureToastText(TEXT("MissingCard")).ToString(),
		FString(TEXT("无法销毁：没有卡牌数据。")));
	TestEqual(TEXT("Delete intrinsic reason"),
		FWacomBackpackScreenTestAccess::BuildDeleteFailureToastText(TEXT("Intrinsic")).ToString(),
		FString(TEXT("无法销毁：固有卡不能被销毁。")));
	TestEqual(TEXT("Delete not owned reason"),
		FWacomBackpackScreenTestAccess::BuildDeleteFailureToastText(TEXT("CardNotOwned")).ToString(),
		FString(TEXT("无法销毁：这张卡不在当前背包中。")));
	TestEqual(TEXT("Delete last bag reason"),
		FWacomBackpackScreenTestAccess::BuildDeleteFailureToastText(TEXT("LastBagProvider")).ToString(),
		FString(TEXT("无法销毁：这是最后一张背包容量卡。")));
	TestEqual(TEXT("Delete last capacity provider reason"),
		FWacomBackpackScreenTestAccess::BuildDeleteFailureToastText(TEXT("LastCapacityProvider")).ToString(),
		FString(TEXT("无法销毁：这是最后一张背包容量卡。")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackDeckCardHoverEventsSpec,
	"Wacom.UI.Backpack.DeckCardHoverEvents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackDeckCardHoverEventsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());

	FCardInstance Inst;
	Inst.InstanceId = FGuid::NewGuid();
	Inst.Definition = Card.Get();
	Widget->SetCard(Inst, EZoneKind::Backpack, FGuid());

	int32 HoverCount = 0;
	int32 UnhoverCount = 0;
	UWacomDeckCardWidget* LastHoverSource = nullptr;
	UWacomDeckCardWidget* LastUnhoverSource = nullptr;
	Widget->OnCardHoveredNative.AddLambda(
		[&HoverCount, &LastHoverSource](UWacomDeckCardWidget* Source)
		{
			++HoverCount;
			LastHoverSource = Source;
		});
	Widget->OnCardUnhoveredNative.AddLambda(
		[&UnhoverCount, &LastUnhoverSource](UWacomDeckCardWidget* Source)
		{
			++UnhoverCount;
			LastUnhoverSource = Source;
		});

	TestTrue(TEXT("Hover request accepted"), Widget->RequestCardHover());
	TestEqual(TEXT("Hover emitted once"), HoverCount, 1);
	TestEqual(TEXT("Hover carries source widget"), LastHoverSource, Widget.Get());

	TestTrue(TEXT("Unhover request accepted"), Widget->RequestCardUnhover());
	TestEqual(TEXT("Unhover emitted once"), UnhoverCount, 1);
	TestEqual(TEXT("Unhover carries source widget"), LastUnhoverSource, Widget.Get());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackDisabledDeckCardBlocksToggleButKeepsHoverSpec,
	"Wacom.UI.Backpack.DisabledDeckCardBlocksToggleButKeepsHover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackDisabledDeckCardBlocksToggleButKeepsHoverSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());

	FCardInstance Inst;
	Inst.InstanceId = FGuid::NewGuid();
	Inst.Definition = Card.Get();

	FRunStorageCardView View;
	View.Instance = Inst;
	View.PhysicalZone = EZoneKind::SpecialZone;
	View.ZoneOwnerInstanceId = FGuid::NewGuid();
	View.bCanToggleBattleEnabledInSpecialZone = true;
	Widget->SetStorageCardView(View);
	Widget->SetMoveEnabled(false);

	int32 HoverCount = 0;
	int32 ToggleCount = 0;
	Widget->OnCardHoveredNative.AddLambda(
		[&HoverCount](UWacomDeckCardWidget*)
		{
			++HoverCount;
		});
	Widget->OnBattleEnabledToggleRequestedNative.AddLambda(
		[&ToggleCount](FGuid)
		{
			++ToggleCount;
		});

	TestFalse(TEXT("Disabled card cannot request right-click toggle"), Widget->RequestBattleEnabledToggle());
	TestEqual(TEXT("Disabled card emits no toggle request"), ToggleCount, 0);
	TestTrue(TEXT("Disabled card still emits hover"), Widget->RequestCardHover());
	TestEqual(TEXT("Hover still broadcasts once"), HoverCount, 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardDetailPanelPositionSpec,
	"Wacom.UI.Backpack.CardDetailPanelPosition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardDetailPanelPositionSpec::RunTest(const FString& /*Parameters*/)
{
	const FVector2D LayerSize(1000.f, 700.f);
	const FVector2D PanelSize(360.f, 420.f);
	const FVector2D AnchorSize(260.f, 380.f);

	const FVector2D Right = UWacomBackpackScreenPresenter::ComputeCardDetailPanelPosition(
		FVector2D(100.f, 80.f),
		AnchorSize,
		LayerSize,
		PanelSize,
		12.f);
	TestEqual(TEXT("Detail panel prefers right side"), Right.X, 372.0);
	TestEqual(TEXT("Detail panel keeps top alignment"), Right.Y, 80.0);

	const FVector2D Left = UWacomBackpackScreenPresenter::ComputeCardDetailPanelPosition(
		FVector2D(700.f, 80.f),
		AnchorSize,
		LayerSize,
		PanelSize,
		12.f);
	TestEqual(TEXT("Detail panel flips to left when right side overflows"), Left.X, 328.0);

	const FVector2D Clamped = UWacomBackpackScreenPresenter::ComputeCardDetailPanelPosition(
		FVector2D(900.f, 650.f),
		AnchorSize,
		LayerSize,
		PanelSize,
		12.f);
	TestEqual(TEXT("Detail panel clamps x within layer"), Clamped.X, 528.0);
	TestEqual(TEXT("Detail panel clamps y within layer"), Clamped.Y, 280.0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardDetailHoverShowsPanelSpec,
	"Wacom.UI.Backpack.CardDetailHoverShowsPanel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardDetailHoverShowsPanelSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBackpackScreen> Screen(NewObject<UWacomBackpackScreen>());
	TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());

	Card->CardId = TEXT("DetailHoverCard");
	Card->DisplayName = FText::FromString(TEXT("悬停详情卡"));
	Card->Description = FText::FromString(TEXT("悬停时显示完整描述"));

	FCardInstance Inst;
	Inst.InstanceId = FGuid::NewGuid();
	Inst.Definition = Card.Get();
	Widget->SetCard(Inst, EZoneKind::Backpack, FGuid());

	Screen->TakeWidget();
	TestTrue(TEXT("BackpackScreen can show detail panel for hovered card"), FWacomBackpackScreenTestAccess::ShowDetailForCardWidget(*Screen, Widget.Get()));
	TestTrue(TEXT("Detail panel visible after hover"), FWacomBackpackScreenTestAccess::IsDetailVisible(*Screen));
	TestEqual(TEXT("Detail panel receives card name"), FWacomBackpackScreenTestAccess::DetailNameText(*Screen).ToString(), TEXT("悬停详情卡"));

	FWacomBackpackScreenTestAccess::HideDetail(*Screen);
	TestFalse(TEXT("Detail panel hidden on request"), FWacomBackpackScreenTestAccess::IsDetailVisible(*Screen));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackDeckCardMoveClickUnboundSpec,
	"Wacom.UI.Backpack.DeckCardMoveClickUnbound",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackDeckCardMoveClickUnboundSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, R6.6: Main card button stays passive until the Workspace binds pointer forwarding.
	TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
	Widget->TakeWidget();

	TestFalse(TEXT("MoveButton has no click bindings"), Widget->HasMoveButtonClickBindings());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackStorageCardViewPayloadSpec,
	"Wacom.UI.Backpack.StorageCardViewPayload",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackStorageCardViewPayloadSpec::RunTest(const FString& /*Parameters*/)
{
	// Workspace 卡牌保持 Snapshot 身份和物理归属，只转发指针事件，不再构造第二套拖拽 payload。
	TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());

	FRunStorageCardView View;
	View.Instance.InstanceId = FGuid::NewGuid();
	View.Instance.Definition = Card.Get();
	View.PhysicalZone = EZoneKind::SpecialZone;
	View.ZoneOwnerInstanceId = FGuid::NewGuid();

	Widget->SetCard(View.Instance, View.PhysicalZone, View.ZoneOwnerInstanceId);

	TestEqual(TEXT("StorageCardView instance id copied"), Widget->GetCardInstanceId(), View.Instance.InstanceId);
	TestTrue(TEXT("StorageCardView physical zone copied"), Widget->GetFromZone() == View.PhysicalZone);
	TestEqual(TEXT("StorageCardView owner id copied"), Widget->GetFromZoneOwnerInstanceId(), View.ZoneOwnerInstanceId);
	TestEqual(TEXT("StorageCardView definition copied"), Widget->GetCard(), Card.Get());

	Widget->SetProjectedFromBadgeText(FText::FromString(TEXT("来自 蛛茧绒囊")));
	TestTrue(TEXT("Projected badge still available for snapshot projection cards"), Widget->IsProjectedFromBadgeVisible());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackScreenRefreshReusesCardWidgetsSpec,
	"Wacom.UI.Backpack.BackpackScreenRefreshReusesCardWidgetsForEquivalentSnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackScreenRefreshReusesCardWidgetsSpec::RunTest(const FString& /*Parameters*/)
{
	UObject* Outer = GetTransientPackage();
	UCardDefinition* Capacity = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Capacity"), 4);
	UCardDefinition* BattleCard = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Battle"));
	UCardDefinition* FluxCard = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Flux"));
	UCharacterDefinition* Character = MakeBackpackUiCharacterForTest(Outer, { Capacity, BattleCard });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	Run->AcquireCardToRun(FluxCard);

	TStrongObjectPtr<UWacomBackpackScreen> Screen(MakeBackpackUiScreenForTest(GetTransientPackage(), Run.Get()));
	UWacomDeckCardWidget* InitialBattle = FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 0);
	UWacomDeckCardWidget* InitialFlux = FWacomBackpackScreenTestAccess::FluxContentCard(*Screen, 0);
	TestNotNull(TEXT("Initial battle widget"), InitialBattle);
	TestNotNull(TEXT("Initial flux widget"), InitialFlux);

	FWacomBackpackScreenTestAccess::Refresh(*Screen);
	TestEqual(TEXT("Equivalent refresh reuses battle widget"), FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 0), InitialBattle);
	TestEqual(TEXT("Equivalent refresh reuses flux widget"), FWacomBackpackScreenTestAccess::FluxContentCard(*Screen, 0), InitialFlux);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackScreenRefreshDirtyGateSkipsEquivalentListReconcileSpec,
	"Wacom.UI.Backpack.BackpackScreenRefreshDirtyGateSkipsEquivalentListReconcile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackScreenRefreshDirtyGateSkipsEquivalentListReconcileSpec::RunTest(const FString& /*Parameters*/)
{
	UObject* Outer = GetTransientPackage();
	UCardDefinition* Capacity = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Dirty.Capacity"), 4);
	UCardDefinition* BattleCard = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Dirty.Battle"));
	UCardDefinition* FluxCard = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Dirty.Flux"));
	UCharacterDefinition* Character = MakeBackpackUiCharacterForTest(Outer, { Capacity, BattleCard });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	Run->AcquireCardToRun(FluxCard);

	TStrongObjectPtr<UWacomBackpackScreen> Screen(MakeBackpackUiScreenForTest(GetTransientPackage(), Run.Get()));
	UWacomDeckCardWidget* InitialBattle = FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 0);
	UWacomDeckCardWidget* InitialFlux = FWacomBackpackScreenTestAccess::FluxContentCard(*Screen, 0);
	TestNotNull(TEXT("Initial battle widget"), InitialBattle);
	TestNotNull(TEXT("Initial flux widget"), InitialFlux);
	const int32 BaselineApplyCount = FWacomBackpackScreenTestAccess::RefreshApplyCount(*Screen);
	const int32 BaselineSkipCount = FWacomBackpackScreenTestAccess::RefreshSkipCount(*Screen);
	const int32 BaselineSnapshotBuildCount = FWacomBackpackScreenTestAccess::SnapshotBuildCount(*Screen);
	const int32 BaselineSnapshotSkipCount = FWacomBackpackScreenTestAccess::SnapshotRevisionSkipCount(*Screen);
	TestTrue(TEXT("Initial refresh applies list reconcile"), BaselineApplyCount >= 1);
	TestTrue(TEXT("Initial refresh builds storage snapshot"), BaselineSnapshotBuildCount >= 1);

	FWacomBackpackScreenTestAccess::Refresh(*Screen);
	TestEqual(TEXT("Equivalent revision refresh skips snapshot"), FWacomBackpackScreenTestAccess::SnapshotRevisionSkipCount(*Screen), BaselineSnapshotSkipCount + 1);
	TestEqual(TEXT("Equivalent revision refresh does not build snapshot"), FWacomBackpackScreenTestAccess::SnapshotBuildCount(*Screen), BaselineSnapshotBuildCount);
	TestEqual(TEXT("Equivalent revision refresh does not reach signature skip"), FWacomBackpackScreenTestAccess::RefreshSkipCount(*Screen), BaselineSkipCount);
	TestEqual(TEXT("Equivalent refresh does not apply again"), FWacomBackpackScreenTestAccess::RefreshApplyCount(*Screen), BaselineApplyCount);
	TestEqual(TEXT("Skipped refresh keeps battle widget"), FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 0), InitialBattle);
	TestEqual(TEXT("Skipped refresh keeps flux widget"), FWacomBackpackScreenTestAccess::FluxContentCard(*Screen, 0), InitialFlux);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackScreenRefreshCreatesAndRemovesOnlyChangedCardWidgetsSpec,
	"Wacom.UI.Backpack.BackpackScreenRefreshCreatesAndRemovesOnlyChangedCardWidgets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackScreenRefreshCreatesAndRemovesOnlyChangedCardWidgetsSpec::RunTest(const FString& /*Parameters*/)
{
	UObject* Outer = GetTransientPackage();
	UCardDefinition* Capacity = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Capacity"), 4);
	UCardDefinition* BattleCard = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Battle"));
	UCardDefinition* NewFluxCard = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.NewFlux"));
	UCharacterDefinition* Character = MakeBackpackUiCharacterForTest(Outer, { Capacity, BattleCard });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomBackpackScreen> Screen(MakeBackpackUiScreenForTest(GetTransientPackage(), Run.Get()));
	UWacomDeckCardWidget* InitialBattle = FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 0);
	UWacomDeckCardWidget* InitialFlux = FWacomBackpackScreenTestAccess::FluxContentCard(*Screen, 0);
	TestNotNull(TEXT("Initial battle widget"), InitialBattle);
	TestNotNull(TEXT("Initial capacity card is visible as flux content"), InitialFlux);

	Run->AcquireCardToRun(NewFluxCard);
	FWacomBackpackScreenTestAccess::Refresh(*Screen);
	TestEqual(TEXT("Existing battle widget stays reused"), FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 0), InitialBattle);
	TestEqual(TEXT("Existing flux widget stays reused"), FWacomBackpackScreenTestAccess::FluxContentCard(*Screen, 0), InitialFlux);
	TestNotNull(TEXT("New flux widget created"), FWacomBackpackScreenTestAccess::FluxContentCard(*Screen, 1));

	const FGuid BattleInstanceId = Run->GetBattleDeck().IsValidIndex(0)
		? Run->GetBattleDeck()[0].InstanceId
		: FGuid();
	TestTrue(TEXT("Battle instance valid"), BattleInstanceId.IsValid());
	TestTrue(TEXT("Move battle card to backpack"), Run->MoveInstance(BattleInstanceId, EZoneKind::Backpack, FGuid()));
	FWacomBackpackScreenTestAccess::Refresh(*Screen);
	TestTrue(TEXT("Moved battle widget no longer remains in battle list"), FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 0) != InitialBattle);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackScreenRefreshDirtyGateRefreshesMovedCardsSpec,
	"Wacom.UI.Backpack.BackpackScreenRefreshDirtyGateRefreshesMovedCards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackScreenRefreshDirtyGateRefreshesMovedCardsSpec::RunTest(const FString& /*Parameters*/)
{
	UObject* Outer = GetTransientPackage();
	UCardDefinition* Capacity = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Dirty.Move.Capacity"), 4);
	UCardDefinition* BattleCard = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Dirty.Move.Battle"));
	UCharacterDefinition* Character = MakeBackpackUiCharacterForTest(Outer, { Capacity, BattleCard });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomBackpackScreen> Screen(MakeBackpackUiScreenForTest(GetTransientPackage(), Run.Get()));
	UWacomDeckCardWidget* InitialBattle = FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 0);
	TestNotNull(TEXT("Initial battle widget"), InitialBattle);
	const int32 BaselineApplyCount = FWacomBackpackScreenTestAccess::RefreshApplyCount(*Screen);
	const int32 BaselineSkipCount = FWacomBackpackScreenTestAccess::RefreshSkipCount(*Screen);
	const int32 BaselineSnapshotBuildCount = FWacomBackpackScreenTestAccess::SnapshotBuildCount(*Screen);
	TestTrue(TEXT("Initial refresh applies list reconcile"), BaselineApplyCount >= 1);
	TestTrue(TEXT("Initial refresh builds storage snapshot"), BaselineSnapshotBuildCount >= 1);

	const FGuid BattleInstanceId = Run->GetBattleDeck().IsValidIndex(0)
		? Run->GetBattleDeck()[0].InstanceId
		: FGuid();
	TestTrue(TEXT("Move battle card to backpack"), Run->MoveInstance(BattleInstanceId, EZoneKind::Backpack, FGuid()));
	FWacomBackpackScreenTestAccess::Refresh(*Screen);

	TestEqual(TEXT("Moved card refresh builds new storage snapshot"), FWacomBackpackScreenTestAccess::SnapshotBuildCount(*Screen), BaselineSnapshotBuildCount + 1);
	TestEqual(TEXT("Moved card refresh applies list reconcile"), FWacomBackpackScreenTestAccess::RefreshApplyCount(*Screen), BaselineApplyCount + 1);
	TestEqual(TEXT("Moved card refresh does not skip"), FWacomBackpackScreenTestAccess::RefreshSkipCount(*Screen), BaselineSkipCount);
	TestTrue(TEXT("Moved battle widget no longer remains in battle list"), FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 0) != InitialBattle);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackScreenProjectedDuplicateCardsDoNotShareWidgetSpec,
	"Wacom.UI.Backpack.BackpackScreenProjectedAndPhysicalDuplicateCardsDoNotShareWidget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackScreenProjectedDuplicateCardsDoNotShareWidgetSpec::RunTest(const FString& /*Parameters*/)
{
	UObject* Outer = GetTransientPackage();
	UCardDefinition* TypeB = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.TypeB"), 3, true);
	UCardDefinition* Content = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Content"));
	UCharacterDefinition* Character = MakeBackpackUiCharacterForTest(Outer, { TypeB, Content });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	const FGuid OwnerId = Run->GetBackpack().IsValidIndex(0) ? Run->GetBackpack()[0].InstanceId : FGuid();
	const FGuid ContentId = Run->GetBattleDeck().IsValidIndex(0) ? Run->GetBattleDeck()[0].InstanceId : FGuid();
	TestTrue(TEXT("Owner id valid"), OwnerId.IsValid());
	TestTrue(TEXT("Content id valid"), ContentId.IsValid());
	TestTrue(TEXT("Move content to special"), Run->MoveInstance(ContentId, EZoneKind::SpecialZone, OwnerId));
	TestTrue(TEXT("Move owner to battle"), Run->MoveInstance(OwnerId, EZoneKind::BattleDeck, FGuid()));
	TestTrue(TEXT("Enable content projection"), Run->SetSpecialZoneCardBattleEnabled(ContentId, true));

	TStrongObjectPtr<UWacomBackpackScreen> Screen(MakeBackpackUiScreenForTest(GetTransientPackage(), Run.Get()));
	UWacomDeckCardWidget* PhysicalOwnerWidget = FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 0);
	UWacomDeckCardWidget* ProjectedContentWidget = FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 1);
	UWacomSpecialZoneWidget* SpecialZoneWidget = FWacomBackpackScreenTestAccess::SpecialZone(*Screen, 0);
	TestNotNull(TEXT("Physical owner widget"), PhysicalOwnerWidget);
	TestNotNull(TEXT("Projected content widget"), ProjectedContentWidget);
	TestNotNull(TEXT("Special zone widget"), SpecialZoneWidget);
	if (!SpecialZoneWidget)
	{
		return false;
	}
	UWacomDeckCardWidget* SpecialContentWidget = FWacomBackpackScreenTestAccess::ContentCard(*SpecialZoneWidget, 0);
	TestNotNull(TEXT("Special content widget"), SpecialContentWidget);
	TestNotEqual(TEXT("Projected card does not share widget with special content"), ProjectedContentWidget, SpecialContentWidget);
	TestEqual(TEXT("Projected widget is marked projected"),
		ProjectedContentWidget ? ProjectedContentWidget->GetBackpackListReuseRole() : EWacomBackpackDeckCardListReuseRole::PhysicalList,
		EWacomBackpackDeckCardListReuseRole::BattleDeckProjected);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackScreenRefreshDirtyGateRefreshesProjectedAndSpecialZoneStateSpec,
	"Wacom.UI.Backpack.BackpackScreenRefreshDirtyGateRefreshesProjectedAndSpecialZoneState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackScreenRefreshDirtyGateRefreshesProjectedAndSpecialZoneStateSpec::RunTest(const FString& /*Parameters*/)
{
	UObject* Outer = GetTransientPackage();
	UCardDefinition* TypeB = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Dirty.TypeB"), 3, true);
	UCardDefinition* Content = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Dirty.Content"));
	UCharacterDefinition* Character = MakeBackpackUiCharacterForTest(Outer, { TypeB, Content });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	const FGuid OwnerId = Run->GetBackpack().IsValidIndex(0) ? Run->GetBackpack()[0].InstanceId : FGuid();
	const FGuid ContentId = Run->GetBattleDeck().IsValidIndex(0) ? Run->GetBattleDeck()[0].InstanceId : FGuid();
	TestTrue(TEXT("Move content to special"), Run->MoveInstance(ContentId, EZoneKind::SpecialZone, OwnerId));
	TestTrue(TEXT("Move owner to battle"), Run->MoveInstance(OwnerId, EZoneKind::BattleDeck, FGuid()));

	TStrongObjectPtr<UWacomBackpackScreen> Screen(MakeBackpackUiScreenForTest(GetTransientPackage(), Run.Get()));
	const int32 BaselineApplyCount = FWacomBackpackScreenTestAccess::RefreshApplyCount(*Screen);
	const int32 BaselineSkipCount = FWacomBackpackScreenTestAccess::RefreshSkipCount(*Screen);
	const int32 BaselineSnapshotBuildCount = FWacomBackpackScreenTestAccess::SnapshotBuildCount(*Screen);
	TestTrue(TEXT("Initial refresh applies list reconcile"), BaselineApplyCount >= 1);
	TestNull(TEXT("No projected content before enable"), FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 1));
	UWacomSpecialZoneWidget* InitialZone = FWacomBackpackScreenTestAccess::SpecialZone(*Screen, 0);
	TestNotNull(TEXT("Initial special zone"), InitialZone);

	TestTrue(TEXT("Enable content projection"), Run->SetSpecialZoneCardBattleEnabled(ContentId, true));
	FWacomBackpackScreenTestAccess::Refresh(*Screen);

	TestEqual(TEXT("Projection change builds new storage snapshot"), FWacomBackpackScreenTestAccess::SnapshotBuildCount(*Screen), BaselineSnapshotBuildCount + 1);
	TestEqual(TEXT("Projection change applies list reconcile"), FWacomBackpackScreenTestAccess::RefreshApplyCount(*Screen), BaselineApplyCount + 1);
	TestEqual(TEXT("Projection change does not skip"), FWacomBackpackScreenTestAccess::RefreshSkipCount(*Screen), BaselineSkipCount);
	TestNotNull(TEXT("Projected content appears"), FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 1));
	TestEqual(TEXT("Special zone widget reused while state refreshes"), FWacomBackpackScreenTestAccess::SpecialZone(*Screen, 0), InitialZone);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackScreenReusableCardWidgetsResetStateSpec,
	"Wacom.UI.Backpack.BackpackScreenReusableCardWidgetsResetProjectedBadgeAndToggleState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackScreenReusableCardWidgetsResetStateSpec::RunTest(const FString& /*Parameters*/)
{
	UObject* Outer = GetTransientPackage();
	UCardDefinition* TypeB = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.TypeB"), 3, true);
	UCardDefinition* Content = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Content"));
	UCharacterDefinition* Character = MakeBackpackUiCharacterForTest(Outer, { TypeB, Content });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	const FGuid OwnerId = Run->GetBackpack().IsValidIndex(0) ? Run->GetBackpack()[0].InstanceId : FGuid();
	const FGuid ContentId = Run->GetBattleDeck().IsValidIndex(0) ? Run->GetBattleDeck()[0].InstanceId : FGuid();
	TestTrue(TEXT("Move content to special"), Run->MoveInstance(ContentId, EZoneKind::SpecialZone, OwnerId));
	TestTrue(TEXT("Move owner to battle"), Run->MoveInstance(OwnerId, EZoneKind::BattleDeck, FGuid()));
	TestTrue(TEXT("Enable content projection"), Run->SetSpecialZoneCardBattleEnabled(ContentId, true));

	TStrongObjectPtr<UWacomBackpackScreen> Screen(MakeBackpackUiScreenForTest(GetTransientPackage(), Run.Get()));
	UWacomDeckCardWidget* ProjectedWidget = FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 1);
	TestNotNull(TEXT("Projected widget exists"), ProjectedWidget);
	if (!ProjectedWidget)
	{
		return false;
	}
	TestTrue(TEXT("Projected badge visible before disabling"), ProjectedWidget->IsProjectedFromBadgeVisible());

	TestTrue(TEXT("Disable content projection"), Run->SetSpecialZoneCardBattleEnabled(ContentId, false));
	FWacomBackpackScreenTestAccess::Refresh(*Screen);
	TestNull(TEXT("Projected widget removed from battle list"), FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 1));

	UWacomSpecialZoneWidget* SpecialZoneWidget = FWacomBackpackScreenTestAccess::SpecialZone(*Screen, 0);
	UWacomDeckCardWidget* SpecialContentWidget = SpecialZoneWidget ? FWacomBackpackScreenTestAccess::ContentCard(*SpecialZoneWidget, 0) : nullptr;
	TestNotNull(TEXT("Special content widget remains"), SpecialContentWidget);
	if (SpecialContentWidget)
	{
		TestFalse(TEXT("Special content has no projected badge after refresh"), SpecialContentWidget->IsProjectedFromBadgeVisible());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackSpecialZoneRefreshReusesWidgetsSpec,
	"Wacom.UI.Backpack.BackpackSpecialZoneRefreshReusesZoneOwnerAndContentWidgets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackSpecialZoneRefreshReusesWidgetsSpec::RunTest(const FString& /*Parameters*/)
{
	UObject* Outer = GetTransientPackage();
	UCardDefinition* TypeB = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.TypeB"), 3, true);
	UCardDefinition* Content = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Content"));
	UCharacterDefinition* Character = MakeBackpackUiCharacterForTest(Outer, { TypeB, Content });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	const FGuid OwnerId = Run->GetBackpack().IsValidIndex(0) ? Run->GetBackpack()[0].InstanceId : FGuid();
	const FGuid ContentId = Run->GetBattleDeck().IsValidIndex(0) ? Run->GetBattleDeck()[0].InstanceId : FGuid();
	TestTrue(TEXT("Move content to special"), Run->MoveInstance(ContentId, EZoneKind::SpecialZone, OwnerId));

	TStrongObjectPtr<UWacomBackpackScreen> Screen(MakeBackpackUiScreenForTest(GetTransientPackage(), Run.Get()));
	UWacomSpecialZoneWidget* InitialZone = FWacomBackpackScreenTestAccess::SpecialZone(*Screen, 0);
	UWacomDeckCardWidget* InitialOwner = InitialZone ? FWacomBackpackScreenTestAccess::OwnerCard(*InitialZone) : nullptr;
	UWacomDeckCardWidget* InitialContent = InitialZone ? FWacomBackpackScreenTestAccess::ContentCard(*InitialZone, 0) : nullptr;
	TestNotNull(TEXT("Initial zone"), InitialZone);
	TestNotNull(TEXT("Initial owner"), InitialOwner);
	TestNotNull(TEXT("Initial content"), InitialContent);

	FWacomBackpackScreenTestAccess::Refresh(*Screen);
	UWacomSpecialZoneWidget* RefreshedZone = FWacomBackpackScreenTestAccess::SpecialZone(*Screen, 0);
	TestEqual(TEXT("Special zone widget reused"), RefreshedZone, InitialZone);
	TestEqual(TEXT("Owner card widget reused"), RefreshedZone ? FWacomBackpackScreenTestAccess::OwnerCard(*RefreshedZone) : nullptr, InitialOwner);
	TestEqual(TEXT("Content card widget reused"), RefreshedZone ? FWacomBackpackScreenTestAccess::ContentCard(*RefreshedZone, 0) : nullptr, InitialContent);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackScreenRemovesHoveredSourceAndHidesDetailSpec,
	"Wacom.UI.Backpack.BackpackScreenRemovesHoveredSourceAndHidesDetail",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackScreenRemovesHoveredSourceAndHidesDetailSpec::RunTest(const FString& /*Parameters*/)
{
	UObject* Outer = GetTransientPackage();
	UCardDefinition* Capacity = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Capacity"), 4);
	UCardDefinition* BattleCard = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Battle"));
	UCharacterDefinition* Character = MakeBackpackUiCharacterForTest(Outer, { Capacity, BattleCard });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomBackpackScreen> Screen(MakeBackpackUiScreenForTest(GetTransientPackage(), Run.Get()));
	UWacomDeckCardWidget* BattleWidget = FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 0);
	TestNotNull(TEXT("Battle widget"), BattleWidget);
	if (!BattleWidget)
	{
		return false;
	}
	TestTrue(TEXT("Show detail for battle widget"), FWacomBackpackScreenTestAccess::ShowDetailForCardWidget(*Screen, BattleWidget));
	TestTrue(TEXT("Detail visible before remove"), FWacomBackpackScreenTestAccess::IsDetailVisible(*Screen));

	const FGuid BattleId = Run->GetBattleDeck().IsValidIndex(0) ? Run->GetBattleDeck()[0].InstanceId : FGuid();
	TestTrue(TEXT("Move battle card to backpack"), Run->MoveInstance(BattleId, EZoneKind::Backpack, FGuid()));
	FWacomBackpackScreenTestAccess::Refresh(*Screen);
	TestFalse(TEXT("Detail hidden after hovered source removed"), FWacomBackpackScreenTestAccess::IsDetailVisible(*Screen));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackScreenRefreshDirtyGateResetsAfterMissingRunOrWidgetRebuildSpec,
	"Wacom.UI.Backpack.BackpackScreenRefreshDirtyGateResetsAfterMissingRunOrWidgetRebuild",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackScreenRefreshDirtyGateResetsAfterMissingRunOrWidgetRebuildSpec::RunTest(const FString& /*Parameters*/)
{
	UObject* Outer = GetTransientPackage();
	UCardDefinition* Capacity = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Dirty.Reset.Capacity"), 4);
	UCardDefinition* BattleCard = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Dirty.Reset.Battle"));
	UCharacterDefinition* Character = MakeBackpackUiCharacterForTest(Outer, { Capacity, BattleCard });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomBackpackScreen> Screen(MakeBackpackUiScreenForTest(GetTransientPackage(), Run.Get()));
	const int32 BaselineApplyCount = FWacomBackpackScreenTestAccess::RefreshApplyCount(*Screen);
	const int32 BaselineSkipCount = FWacomBackpackScreenTestAccess::RefreshSkipCount(*Screen);
	const int32 BaselineSnapshotBuildCount = FWacomBackpackScreenTestAccess::SnapshotBuildCount(*Screen);
	const int32 BaselineSnapshotSkipCount = FWacomBackpackScreenTestAccess::SnapshotRevisionSkipCount(*Screen);
	TestTrue(TEXT("Initial refresh applies list reconcile"), BaselineApplyCount >= 1);
	UWacomDeckCardWidget* InitialBattle = FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 0);
	TestNotNull(TEXT("Initial battle widget"), InitialBattle);

	FWacomBackpackScreenTestAccess::SetRunSession(*Screen, nullptr);
	FWacomBackpackScreenTestAccess::Refresh(*Screen);
	TestNull(TEXT("Missing run clears battle list"), FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 0));

	FWacomBackpackScreenTestAccess::SetRunSession(*Screen, Run.Get());
	FWacomBackpackScreenTestAccess::Refresh(*Screen);
	TestEqual(TEXT("Restored run builds snapshot after dirty gate reset"), FWacomBackpackScreenTestAccess::SnapshotBuildCount(*Screen), BaselineSnapshotBuildCount + 1);
	TestEqual(TEXT("Restored run applies after dirty gate reset"), FWacomBackpackScreenTestAccess::RefreshApplyCount(*Screen), BaselineApplyCount + 1);
	TestNotNull(TEXT("Restored run rebuilds battle list"), FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 0));

	FWacomBackpackScreenTestAccess::Refresh(*Screen);
	TestEqual(TEXT("Equivalent restored refresh skips snapshot"), FWacomBackpackScreenTestAccess::SnapshotRevisionSkipCount(*Screen), BaselineSnapshotSkipCount + 1);
	TestEqual(TEXT("Equivalent restored refresh does not reach signature skip"), FWacomBackpackScreenTestAccess::RefreshSkipCount(*Screen), BaselineSkipCount);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackSpecialZoneWidgetSnapshotSpec,
	"Wacom.UI.Backpack.SpecialZoneWidgetSnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackSpecialZoneWidgetSnapshotSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomSpecialZoneWidget> Widget(NewObject<UWacomSpecialZoneWidget>());
	TStrongObjectPtr<UCardDefinition> OwnerCard(NewObject<UCardDefinition>());
	TStrongObjectPtr<UCardDefinition> ContentCard(NewObject<UCardDefinition>());

	OwnerCard->DisplayName = FText::FromString(TEXT("蛛茧绒囊"));
	ContentCard->DisplayName = FText::FromString(TEXT("内容卡"));

	const FGuid OwnerId = FGuid::NewGuid();
	const FGuid ContentId = FGuid::NewGuid();

	FRunSpecialStorageView View;
	View.Capacity = 2;
	View.bOwnerInBattleDeck = true;
	View.OwnerCard.Instance.InstanceId = OwnerId;
	View.OwnerCard.Instance.Definition = OwnerCard.Get();
	View.OwnerCard.PhysicalZone = EZoneKind::BattleDeck;
	View.OwnerCard.bIsPhysicalInBattleDeck = true;
	View.OwnerCard.bIsContainer = true;
	View.OwnerCard.bIsTypeBContainer = true;

	FRunStorageCardView ContentView;
	ContentView.Instance.InstanceId = ContentId;
	ContentView.Instance.Definition = ContentCard.Get();
	ContentView.Instance.bBattleEnabledInSpecialZone = true;
	ContentView.PhysicalZone = EZoneKind::SpecialZone;
	ContentView.ZoneOwnerInstanceId = OwnerId;
	ContentView.bCanToggleBattleEnabledInSpecialZone = true;
	ContentView.bShowBattleEnabledInSpecialZoneBadge = true;
	View.ContentCards.Add(ContentView);

	int32 ToggleCount = 0;
	FGuid LastToggledId;
	Widget->OnBattleEnabledToggleRequestedNative.AddLambda(
		[&ToggleCount, &LastToggledId](FGuid InstanceId)
		{
			++ToggleCount;
			LastToggledId = InstanceId;
		});

	Widget->SetSpecialZoneView(View, nullptr, UWacomDeckCardWidget::StaticClass());

	const FString Title = FWacomBackpackScreenTestAccess::ZoneTitleText(*Widget).ToString();
	TestTrue(TEXT("SpecialZoneWidget title includes owner"), Title.Contains(TEXT("蛛茧绒囊")));
	TestTrue(TEXT("SpecialZoneWidget title includes count/capacity"), Title.Contains(TEXT("1 / 2")));
	TestTrue(TEXT("Battle ready badge visible when owner is in BattleDeck"), FWacomBackpackScreenTestAccess::IsBattleReadyBadgeVisible(*Widget));

	View.bOwnerInBattleDeck = false;
	View.OwnerCard.PhysicalZone = EZoneKind::Backpack;
	View.OwnerCard.bIsPhysicalInBattleDeck = false;
	Widget->SetSpecialZoneView(View, nullptr, UWacomDeckCardWidget::StaticClass());
	TestFalse(TEXT("Battle ready badge hidden when owner is in Backpack"), FWacomBackpackScreenTestAccess::IsBattleReadyBadgeVisible(*Widget));

	View.bOwnerInBattleDeck = true;
	View.OwnerCard.PhysicalZone = EZoneKind::BattleDeck;
	View.OwnerCard.bIsPhysicalInBattleDeck = true;
	Widget->SetSpecialZoneView(View, nullptr, UWacomDeckCardWidget::StaticClass());

	UWacomDeckCardWidget* OwnerWidget = FWacomBackpackScreenTestAccess::OwnerCard(*Widget);
	UWacomDeckCardWidget* ContentWidget = FWacomBackpackScreenTestAccess::ContentCard(*Widget, 0);
	TestNotNull(TEXT("Owner card is rendered"), OwnerWidget);
	TestNotNull(TEXT("Content card is rendered"), ContentWidget);
	if (OwnerWidget)
	{
		TestEqual(TEXT("Owner card keeps instance id"), OwnerWidget->GetCardInstanceId(), OwnerId);
		TestTrue(TEXT("Owner card keeps physical zone"), OwnerWidget->GetFromZone() == EZoneKind::BattleDeck);
	}
	if (ContentWidget)
	{
		TestEqual(TEXT("Content card keeps instance id"), ContentWidget->GetCardInstanceId(), ContentId);
		TestTrue(TEXT("Content card keeps physical zone"), ContentWidget->GetFromZone() == EZoneKind::SpecialZone);
		TestEqual(TEXT("Content card keeps owner id"), ContentWidget->GetFromZoneOwnerInstanceId(), OwnerId);
	}

	TestTrue(TEXT("Content toggle request accepted"), FWacomBackpackScreenTestAccess::RequestContentCardBattleEnabledToggle(*Widget, 0));
	TestEqual(TEXT("Content toggle emits once"), ToggleCount, 1);
	TestEqual(TEXT("Content toggle carries content id"), LastToggledId, ContentId);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackSpecialZoneBattleEnabledBadgeSpec,
	"Wacom.UI.Backpack.SpecialZoneBattleEnabledBadge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackSpecialZoneBattleEnabledBadgeSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, R6.7/R6.10: SpecialZone cards expose an identifiable battle-enabled badge.
	TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());

	FCardInstance Inst;
	Inst.InstanceId = FGuid::NewGuid();
	Inst.Definition = Card.Get();
	Inst.bBattleEnabledInSpecialZone = true;

	Widget->SetCard(Inst, EZoneKind::SpecialZone, FGuid::NewGuid());
	Widget->TakeWidget();

	TestTrue(TEXT("BattleEnabledBadge visible for selected SpecialZone card"), Widget->IsBattleEnabledBadgeVisible());

	Inst.bBattleEnabledInSpecialZone = false;
	Widget->SetCard(Inst, EZoneKind::SpecialZone, FGuid::NewGuid());
	TestFalse(TEXT("BattleEnabledBadge collapsed for unselected SpecialZone card"), Widget->IsBattleEnabledBadgeVisible());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackSpecialZoneTitleAndReadyBadgeSpec,
	"Wacom.UI.Backpack.SpecialZoneTitleAndReadyBadge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackSpecialZoneTitleAndReadyBadgeSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, R6.7/R6.13: SpecialZone section title and battle-ready badge are deterministic.
	const FString Title = UWacomBackpackScreenPresenter::BuildSpecialZoneTitleText(
		FText::FromString(TEXT("蛛茧绒囊")),
		1,
		2).ToString();

	TestTrue(TEXT("SpecialZone title includes owner name"), Title.Contains(TEXT("蛛茧绒囊")));
	TestTrue(TEXT("SpecialZone title includes count/capacity"), Title.Contains(TEXT("1 / 2")));
	TestTrue(
		TEXT("BattleReady badge visible when owner is in BattleDeck"),
		UWacomBackpackScreenPresenter::GetSpecialZoneBattleReadyBadgeVisibility(EZoneKind::BattleDeck) != ESlateVisibility::Collapsed);
	TestEqual(
		TEXT("BattleReady badge collapsed when owner is in Backpack"),
		UWacomBackpackScreenPresenter::GetSpecialZoneBattleReadyBadgeVisibility(EZoneKind::Backpack),
		ESlateVisibility::Collapsed);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackProjectedFromBadgeSpec,
	"Wacom.UI.Backpack.ProjectedFromBadge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackProjectedFromBadgeSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, R6.10: BattleDeck projection keeps a visible source-owner badge.
	TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
	Widget->TakeWidget();

	TestFalse(TEXT("ProjectedFromBadge hidden by default"), Widget->IsProjectedFromBadgeVisible());

	const FText SourceText = FText::FromString(TEXT("来自 蛛茧绒囊"));
	Widget->SetProjectedFromBadgeText(SourceText);

	TestTrue(TEXT("ProjectedFromBadge visible when text is set"), Widget->IsProjectedFromBadgeVisible());
	TestEqual(TEXT("ProjectedFromBadge text preserved"), Widget->GetProjectedFromBadgeText().ToString(), SourceText.ToString());

	Widget->SetProjectedFromBadgeText(FText::GetEmpty());
	TestFalse(TEXT("ProjectedFromBadge collapsed when text is cleared"), Widget->IsProjectedFromBadgeVisible());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackProjectedFromBadgePresenterSpec,
	"Wacom.UI.Backpack.ProjectedFromBadgePresenter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackProjectedFromBadgePresenterSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> OwnerCard(NewObject<UCardDefinition>());
	OwnerCard->DisplayName = FText::FromString(TEXT("蛛茧绒囊"));

	FRunBackpackStorageSnapshot Snapshot;
	FRunSpecialStorageView SpecialView;
	SpecialView.OwnerCard.Instance.InstanceId = FGuid::NewGuid();
	SpecialView.OwnerCard.Instance.Definition = OwnerCard.Get();
	Snapshot.SpecialZones.Add(SpecialView);

	FRunStorageCardView ProjectedCard;
	ProjectedCard.ZoneOwnerInstanceId = SpecialView.OwnerCard.Instance.InstanceId;

	const FText BadgeText = UWacomBackpackScreenPresenter::BuildBattleDeckProjectedFromBadgeText(ProjectedCard, Snapshot);
	TestEqual(TEXT("Projected badge text uses special zone owner name"), BadgeText.ToString(), TEXT("来自 蛛茧绒囊"));

	ProjectedCard.ZoneOwnerInstanceId = FGuid::NewGuid();
	TestTrue(
		TEXT("Projected badge text is empty when owner cannot be found"),
		UWacomBackpackScreenPresenter::BuildBattleDeckProjectedFromBadgeText(ProjectedCard, Snapshot).IsEmpty());

	TestEqual(
		TEXT("Direct projected badge text helper formats owner name"),
		UWacomBackpackScreenPresenter::BuildProjectedFromBadgeText(FText::FromString(TEXT("引虫灯"))).ToString(),
		TEXT("来自 引虫灯"));
	TestTrue(
		TEXT("Direct projected badge text helper keeps empty owner empty"),
		UWacomBackpackScreenPresenter::BuildProjectedFromBadgeText(FText::GetEmpty()).IsEmpty());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackPresenterCompatibilitySpec,
	"Wacom.UI.Backpack.PresenterCompatibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackPresenterCompatibilitySpec::RunTest(const FString& /*Parameters*/)
{
	const FText OwnerName = FText::FromString(TEXT("兼容主卡"));
	TestEqual(
		TEXT("Legacy special title helper forwards to presenter"),
		UWacomBackpackScreen::BuildSpecialZoneTitleText(OwnerName, 2, 5).ToString(),
		UWacomBackpackScreenPresenter::BuildSpecialZoneTitleText(OwnerName, 2, 5).ToString());

	TestEqual(
		TEXT("Legacy burden title helper forwards to presenter"),
		UWacomBackpackScreen::BuildBurdenZoneTitleText(4).ToString(),
		UWacomBackpackScreenPresenter::BuildBurdenZoneTitleText(4).ToString());

	TestEqual(
		TEXT("Legacy battle-ready visibility helper forwards to presenter"),
		UWacomBackpackScreen::GetSpecialZoneBattleReadyBadgeVisibility(EZoneKind::BattleDeck),
		UWacomBackpackScreenPresenter::GetSpecialZoneBattleReadyBadgeVisibility(EZoneKind::BattleDeck));

	const FVector2D LegacyPosition = UWacomBackpackScreen::ComputeCardDetailPanelPosition(
		FVector2D(100.f, 80.f),
		FVector2D(260.f, 380.f),
		FVector2D(1000.f, 700.f),
		FVector2D(360.f, 420.f),
		12.f);
	const FVector2D PresenterPosition = UWacomBackpackScreenPresenter::ComputeCardDetailPanelPosition(
		FVector2D(100.f, 80.f),
		FVector2D(260.f, 380.f),
		FVector2D(1000.f, 700.f),
		FVector2D(360.f, 420.f),
		12.f);
	TestEqual(TEXT("Legacy detail position helper forwards to presenter X"), LegacyPosition.X, PresenterPosition.X);
	TestEqual(TEXT("Legacy detail position helper forwards to presenter Y"), LegacyPosition.Y, PresenterPosition.Y);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackBattleEnabledToggleRequestSpec,
	"Wacom.UI.Backpack.BattleEnabledToggleRequest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackBattleEnabledToggleRequestSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, R6.11: right-click toggle path emits one request for the card instance.
	TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());

	FCardInstance Inst;
	Inst.InstanceId = FGuid::NewGuid();
	Inst.Definition = Card.Get();

	FRunStorageCardView View;
	View.Instance = Inst;
	View.PhysicalZone = EZoneKind::SpecialZone;
	View.ZoneOwnerInstanceId = FGuid::NewGuid();
	Widget->SetStorageCardView(View);

	int32 ToggleCount = 0;
	FGuid LastToggledId;
	Widget->OnBattleEnabledToggleRequestedNative.AddLambda(
		[&ToggleCount, &LastToggledId](FGuid InstanceId)
		{
			++ToggleCount;
			LastToggledId = InstanceId;
		});

	TestFalse(TEXT("Toggle disabled by default"), Widget->RequestBattleEnabledToggle());
	TestEqual(TEXT("No request emitted while disabled"), ToggleCount, 0);

	View.bCanToggleBattleEnabledInSpecialZone = true;
	Widget->SetStorageCardView(View);
	TestTrue(TEXT("Toggle request accepted when enabled"), Widget->RequestBattleEnabledToggle());
	TestEqual(TEXT("One toggle request emitted"), ToggleCount, 1);
	TestEqual(TEXT("Toggle request carries instance id"), LastToggledId, Inst.InstanceId);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackBurdenZoneTitleAndCardOrderSpec,
	"Wacom.UI.Backpack.BurdenZoneTitleAndCardOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackBurdenZoneTitleAndCardOrderSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, R6.8: BurdenZone title count and rendered card widgets preserve instance order.
	TestTrue(
		TEXT("BurdenZone title includes card count"),
		UWacomBackpackScreenPresenter::BuildBurdenZoneTitleText(3).ToString().Contains(TEXT("3")));
	TestEqual(
		TEXT("BurdenZone hidden when empty"),
		UWacomBackpackScreenPresenter::GetBurdenZoneVisibility(0),
		ESlateVisibility::Collapsed);
	TestEqual(
		TEXT("BurdenZone visible when cards overflow"),
		UWacomBackpackScreenPresenter::GetBurdenZoneVisibility(1),
		ESlateVisibility::SelfHitTestInvisible);

	TArray<FCardInstance> BurdenCards;
	TArray<TStrongObjectPtr<UCardDefinition>> CardDefinitions;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
		FCardInstance Inst;
		Inst.InstanceId = FGuid::NewGuid();
		Inst.Definition = Card.Get();

		CardDefinitions.Add(MoveTemp(Card));
		BurdenCards.Add(Inst);
	}

	for (int32 Index = 0; Index < BurdenCards.Num(); ++Index)
	{
		TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
		Widget->SetCard(BurdenCards[Index], EZoneKind::BurdenZone, FGuid::NewGuid());

		TestEqual(TEXT("Burden card order preserves instance id"), Widget->GetCardInstanceId(), BurdenCards[Index].InstanceId);
		TestEqual(TEXT("Burden card order preserves definition"), Widget->GetCard(), BurdenCards[Index].Definition.Get());
		TestTrue(TEXT("Burden card source zone is BurdenZone"), Widget->GetFromZone() == EZoneKind::BurdenZone);
		TestFalse(TEXT("Burden card owner id normalized to invalid"), Widget->GetFromZoneOwnerInstanceId().IsValid());
	}

	return true;
}
