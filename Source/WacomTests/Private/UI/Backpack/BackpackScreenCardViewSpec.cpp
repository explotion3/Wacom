// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Fixtures/WacomRunExplorationFixture.h"

#include "../BackpackScreenTestAccess.h"

#include "Blueprint/WidgetTree.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
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

#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackCardDetailController.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceSceneBuilder.h"

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
