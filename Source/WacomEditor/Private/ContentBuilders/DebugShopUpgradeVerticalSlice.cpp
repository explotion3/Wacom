// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/DebugShopUpgradeVerticalSlice.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "ContentBuilders/ContentBuilderHelpers.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Shops/ShopDefinition.h"
#include "Tags/WacomGameplayTags.h"
#include "Testing/WacomDebugShopUpgradeVerticalSliceAutomationTestView.h"
#include "UI/Card/WacomCardView.h"
#include "UI/Shop/WacomShopScreen.h"
#include "WidgetBlueprint.h"

namespace
{
using namespace Wacom::ContentBuilder;

const FString WhitePackage(TEXT("/Game/Wacom/Data/Cards/Debug/ShopUpgrade/DA_Card_TestShopUpgrade_VenomProof_White"));
const FString BluePackage(TEXT("/Game/Wacom/Data/Cards/Debug/ShopUpgrade/DA_Card_TestShopUpgrade_VenomProof_Blue"));
const FString ShopPackage(TEXT("/Game/Wacom/Data/Shops/DA_Shop_DebugSnake"));
const FString ShopWidgetPackage(TEXT("/Game/Wacom/UI/Shop/WBP_ShopScreen"));
const FName UpgradeFamilyId(TEXT("Test.ShopUpgrade.VenomProof"));

FString ObjectPath(const FString& Package)
{
	return Package + TEXT(".") + FPackageName::GetLongPackageAssetName(Package);
}

FString DefaultReportPath()
{
	return FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("WacomReports"),
		TEXT("DebugShopUpgradeVerticalSlice.json"));
}

FCardEffect MakeEffect(const FGameplayTag& Type, int32 Magnitude)
{
	FCardEffect Effect;
	Effect.EffectType = Type;
	Effect.Magnitude = Magnitude;
	Effect.Target = WacomTags::Target_SingleEnemyPart;
	Effect.Duration = 0;
	return Effect;
}

void ConfigureCard(
	UCardDefinition& Card,
	FName CardId,
	const FText& DisplayName,
	const FGameplayTag& Rarity,
	int32 Damage,
	int32 Poison,
	UCardDefinition* Next)
{
	Card.CardId = CardId;
	Card.UpgradeFamilyId = UpgradeFamilyId;
	Card.NextUpgradeDefinition = Next;
	Card.DisplayName = DisplayName;
	Card.Description = FText::FromString(TEXT("Debug Shop 强化竖切专用测试卡；禁止进入 Production 内容闭包。"));
	Card.BaseCost = 1;
	Card.Rarity = Rarity;
	Card.Keywords.Reset();
	Card.Keywords.AddTag(WacomTags::Card_Keyword_Weapon);
	Card.TargetMode = ECardTargetMode::SingleEnemyPart;
	Card.HandCardTargetFilter = FWacomHandCardTargetFilter();
	Card.Physique = FCardPhysique();
	Card.Effects =
	{
		MakeEffect(WacomTags::Effect_Damage, Damage),
		MakeEffect(WacomTags::Effect_ApplyStatus_Poison, Poison),
	};
	Card.PerfectReleaseEffects.Reset();
	Card.ZoneHooks.Reset();
	Card.Passives.Reset();
}

enum class ECardInspectionResult : uint8
{
	Exact,
	MissingEffectTargets,
	LegacyDisplayName,
	LegacyDisplayNameAndMissingEffectTargets,
	Invalid,
};

ECardInspectionResult InspectCard(
	const UCardDefinition& Card,
	FName ExpectedId,
	const FText& ExpectedDisplayName,
	const FGameplayTag& ExpectedRarity,
	int32 ExpectedDamage,
	int32 ExpectedPoison,
	const UCardDefinition* ExpectedNext,
	TArray<FString>& OutErrors)
{
	const bool bEffectsStructurallyValid = Card.Effects.Num() == 2
		&& Card.Effects[0].EffectType == WacomTags::Effect_Damage
		&& Card.Effects[0].Magnitude == ExpectedDamage
		&& Card.Effects[0].Duration == 0
		&& Card.Effects[1].EffectType == WacomTags::Effect_ApplyStatus_Poison
		&& Card.Effects[1].Magnitude == ExpectedPoison
		&& Card.Effects[1].Duration == 0;
	const bool bValid = Card.CardId == ExpectedId
		&& Card.UpgradeFamilyId == UpgradeFamilyId
		&& Card.NextUpgradeDefinition == ExpectedNext
		&& Card.BaseCost == 1
		&& Card.Rarity == ExpectedRarity
		&& Card.Keywords.Num() == 1
		&& Card.Keywords.HasTagExact(WacomTags::Card_Keyword_Weapon)
		&& Card.TargetMode == ECardTargetMode::SingleEnemyPart
		&& Card.Physique.Capacity == 0
		&& bEffectsStructurallyValid
		&& Card.PerfectReleaseEffects.IsEmpty()
		&& Card.ZoneHooks.IsEmpty()
		&& Card.Passives.IsEmpty();
	if (!bValid)
	{
		OutErrors.Add(FString::Printf(TEXT("Card structure drift: %s"), *Card.GetPathName()));
		return ECardInspectionResult::Invalid;
	}
	const bool bDisplayNameExact = Card.DisplayName.EqualTo(ExpectedDisplayName);
	const bool bKnownLegacyBlueDisplayName = ExpectedId == TEXT("Test.ShopUpgrade.VenomProof.Blue")
		&& Card.DisplayName.EqualTo(FText::FromString(TEXT("强化试制毒牙")));
	if (!bDisplayNameExact && !bKnownLegacyBlueDisplayName)
	{
		OutErrors.Add(FString::Printf(TEXT("Card display name drift: %s"), *Card.GetPathName()));
		return ECardInspectionResult::Invalid;
	}
	const bool bTargetsExact = Card.Effects[0].Target == WacomTags::Target_SingleEnemyPart
		&& Card.Effects[1].Target == WacomTags::Target_SingleEnemyPart;
	if (bTargetsExact)
	{
		return bKnownLegacyBlueDisplayName
			? ECardInspectionResult::LegacyDisplayName
			: ECardInspectionResult::Exact;
	}
	const bool bKnownIncompleteSeed = !Card.Effects[0].Target.IsValid()
		&& !Card.Effects[1].Target.IsValid();
	if (bKnownIncompleteSeed)
	{
		return bKnownLegacyBlueDisplayName
			? ECardInspectionResult::LegacyDisplayNameAndMissingEffectTargets
			: ECardInspectionResult::MissingEffectTargets;
	}
	OutErrors.Add(FString::Printf(TEXT("Card effect target drift: %s"), *Card.GetPathName()));
	return ECardInspectionResult::Invalid;
}

void RepairCard(
	UCardDefinition& Card,
	bool bRepairDisplayName,
	bool bRepairEffectTargets)
{
	Card.Modify();
	if (bRepairDisplayName)
	{
		Card.DisplayName = FText::FromString(TEXT("试制毒牙"));
	}
	if (bRepairEffectTargets)
	{
		check(Card.Effects.Num() == 2);
		Card.Effects[0].Target = WacomTags::Target_SingleEnemyPart;
		Card.Effects[1].Target = WacomTags::Target_SingleEnemyPart;
	}
}

bool HasExactUpgradeService(const UShopDefinition& Shop)
{
	if (!Shop.CardUpgradeService.bEnabled || Shop.CardUpgradeService.Prices.Num() != 3)
	{
		return false;
	}
	return Shop.CardUpgradeService.Prices[0].FromRarity == WacomTags::Card_Rarity_White
		&& Shop.CardUpgradeService.Prices[0].Price == 2
		&& Shop.CardUpgradeService.Prices[1].FromRarity == WacomTags::Card_Rarity_Blue
		&& Shop.CardUpgradeService.Prices[1].Price == 3
		&& Shop.CardUpgradeService.Prices[2].FromRarity == WacomTags::Card_Rarity_Yellow
		&& Shop.CardUpgradeService.Prices[2].Price == 4;
}

bool ValidateShopCollisionPolicy(
	int32 OfferCount,
	bool bTargetOfferAtEnd,
	bool bOtherTargetOffer,
	bool bServicePristine,
	bool bServiceExact,
	TArray<FString>& OutErrors)
{
	const bool bPreSeed = OfferCount == 24 && !bTargetOfferAtEnd
		&& !bOtherTargetOffer && bServicePristine;
	const bool bSeeded = OfferCount == 25 && bTargetOfferAtEnd
		&& !bOtherTargetOffer && bServiceExact;
	if (!bPreSeed && !bSeeded)
	{
		OutErrors.Add(TEXT("Debug Shop is neither the authoritative 24-offer pre-seed state nor exact 25-offer seeded state"));
	}
	return bPreSeed || bSeeded;
}

bool InspectShop(
	UShopDefinition& Shop,
	const UCardDefinition* White,
	bool& bOutConfigured,
	TArray<FString>& OutErrors)
{
	if (Shop.ShopId != TEXT("Shop.DebugSnake"))
	{
		OutErrors.Add(TEXT("Debug Shop stable ShopId drift"));
	}
	bool bTargetAtEnd = false;
	bool bOtherTarget = false;
	for (int32 Index = 0; Index < Shop.Offers.Num(); ++Index)
	{
		const FShopOfferDefinition& Offer = Shop.Offers[Index];
		if (!Offer.CardDefinition || Offer.Price < 0)
		{
			OutErrors.Add(FString::Printf(TEXT("Debug Shop invalid existing offer at %d"), Index));
		}
		const bool bTarget = Offer.CardDefinition
			&& Offer.CardDefinition->CardId == TEXT("Test.ShopUpgrade.VenomProof.White");
		if (bTarget)
		{
			if (Index == Shop.Offers.Num() - 1 && Offer.CardDefinition == White && Offer.Price == 1)
			{
				bTargetAtEnd = true;
			}
			else
			{
				bOtherTarget = true;
			}
		}
	}
	const bool bServicePristine = !Shop.CardUpgradeService.bEnabled
		&& Shop.CardUpgradeService.Prices.IsEmpty();
	const bool bServiceExact = HasExactUpgradeService(Shop);
	const bool bAccepted = ValidateShopCollisionPolicy(
		Shop.Offers.Num(), bTargetAtEnd, bOtherTarget,
		bServicePristine, bServiceExact, OutErrors);
	bOutConfigured = bAccepted && Shop.Offers.Num() == 25
		&& bTargetAtEnd && bServiceExact;
	return bAccepted && OutErrors.IsEmpty();
}

void ConfigureShop(UShopDefinition& Shop, UCardDefinition& White)
{
	Shop.Modify();
	FShopOfferDefinition& Offer = Shop.Offers.AddDefaulted_GetRef();
	Offer.CardDefinition = &White;
	Offer.Price = 1;
	Shop.CardUpgradeService.bEnabled = true;
	Shop.CardUpgradeService.Prices.Reset();
	auto AddPrice = [&Shop](const FGameplayTag& Rarity, int32 Price)
	{
		FShopCardUpgradePriceDefinition& Row = Shop.CardUpgradeService.Prices.AddDefaulted_GetRef();
		Row.FromRarity = Rarity;
		Row.Price = Price;
	};
	AddPrice(WacomTags::Card_Rarity_White, 2);
	AddPrice(WacomTags::Card_Rarity_Blue, 3);
	AddPrice(WacomTags::Card_Rarity_Yellow, 4);
}

void MarkVariable(UWidgetBlueprint& Blueprint, UWidget& Widget)
{
	Widget.bIsVariable = true;
}

void PopulateWidgetGuids(UWidgetBlueprint& Blueprint)
{
	Blueprint.WidgetVariableNameToGuidMap.Reset();
	Blueprint.ForEachSourceWidget([&Blueprint](UWidget* Widget)
	{
		if (!Widget)
		{
			return;
		}
		const FString StablePath = FString::Printf(
			TEXT("%s:%s"), *Blueprint.GetPathName(), *Widget->GetName());
		Blueprint.WidgetVariableNameToGuidMap.Add(
			Widget->GetFName(), FGuid::NewDeterministicGuid(StablePath));
	});
}

UTextBlock* MakeWidgetText(
	UWidgetBlueprint& Blueprint,
	FName Name,
	const FText& Text,
	int32 FontSize,
	bool bVariable = false)
{
	UTextBlock* Block = Blueprint.WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	Block->SetText(Text);
	FSlateFontInfo Font = Block->GetFont();
	Font.Size = FontSize;
	Block->SetFont(Font);
	Block->SetColorAndOpacity(FSlateColor(FLinearColor(0.93f, 0.94f, 0.90f, 1.f)));
	if (bVariable)
	{
		MarkVariable(Blueprint, *Block);
	}
	return Block;
}

UButton* MakeButton(
	UWidgetBlueprint& Blueprint,
	FName Name,
	const FText& Label,
	FName TextName,
	bool bMarkText = false)
{
	UButton* Button = Blueprint.WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	MarkVariable(Blueprint, *Button);
	UTextBlock* Text = MakeWidgetText(Blueprint, TextName, Label, 17, bMarkText);
	Text->SetJustification(ETextJustify::Center);
	Button->AddChild(Text);
	return Button;
}

void ResetWidgetBlueprint(UWidgetBlueprint& Blueprint)
{
	Blueprint.Modify();
	if (Blueprint.WidgetTree)
	{
		Blueprint.WidgetTree->Rename(
			*MakeUniqueObjectName(GetTransientPackage(), UWidgetTree::StaticClass(), TEXT("PreviousShopTree")).ToString(),
			GetTransientPackage(),
			REN_DontCreateRedirectors | REN_NonTransactional);
	}
	Blueprint.WidgetTree = NewObject<UWidgetTree>(&Blueprint, TEXT("WidgetTree"), RF_Transactional);
	Blueprint.Bindings.Reset();
	Blueprint.Animations.Reset();
	Blueprint.WidgetVariableNameToGuidMap.Reset();
	Blueprint.BlueprintDescription = TEXT("正式 Shop Screen：购买/强化双页签；规则由 UWacomShopScreen 和 RunSession 持有。");
	Blueprint.bCanCallInitializedWithoutPlayerContext = true;
}

bool BuildShopWidgetTree(UWidgetBlueprint& Blueprint, UClass& CardViewClass, TArray<FString>& OutErrors)
{
	ResetWidgetBlueprint(Blueprint);
	UWidgetTree& Tree = *Blueprint.WidgetTree;
	UCanvasPanel* Root = Tree.ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	Tree.RootWidget = Root;
	UBorder* Panel = Tree.ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ShopPanel"));
	Panel->SetBrushColor(FLinearColor(0.018f, 0.028f, 0.042f, 0.97f));
	Panel->SetPadding(FMargin(28.f));
	if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(Panel))
	{
		Slot->SetAnchors(FAnchors(0.5f, 0.5f));
		Slot->SetAlignment(FVector2D(0.5f));
		Slot->SetOffsets(FMargin(-600.f, -370.f, 1200.f, 740.f));
	}
	UVerticalBox* Column = Tree.ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	Panel->AddChild(Column);
	UTextBlock* Title = MakeWidgetText(Blueprint, TEXT("TitleText"), FText::FromString(TEXT("商店")), 30, true);
	Title->SetJustification(ETextJustify::Center);
	Column->AddChildToVerticalBox(Title);
	UTextBlock* Gold = MakeWidgetText(Blueprint, TEXT("GoldText"), FText::FromString(TEXT("金币：0")), 18, true);
	Gold->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* Slot = Column->AddChildToVerticalBox(Gold))
	{
		Slot->SetPadding(FMargin(0.f, 5.f, 0.f, 10.f));
	}

	UHorizontalBox* Tabs = Tree.ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Tabs"));
	if (UVerticalBoxSlot* Slot = Column->AddChildToVerticalBox(Tabs))
	{
		Slot->SetHorizontalAlignment(HAlign_Center);
		Slot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
	}
	Tabs->AddChildToHorizontalBox(MakeButton(Blueprint, TEXT("PurchaseTabButton"), FText::FromString(TEXT("购买")), TEXT("PurchaseTabText")));
	Tabs->AddChildToHorizontalBox(MakeButton(Blueprint, TEXT("UpgradeTabButton"), FText::FromString(TEXT("强化")), TEXT("UpgradeTabText")));

	UWidgetSwitcher* Switcher = Tree.ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("PageSwitcher"));
	MarkVariable(Blueprint, *Switcher);
	if (UVerticalBoxSlot* Slot = Column->AddChildToVerticalBox(Switcher))
	{
		Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	UVerticalBox* PurchasePage = Tree.ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PurchasePage"));
	Switcher->AddChild(PurchasePage);
	UTextBlock* Empty = MakeWidgetText(Blueprint, TEXT("EmptyText"), FText::FromString(TEXT("暂无商品")), 17, true);
	Empty->SetJustification(ETextJustify::Center);
	PurchasePage->AddChildToVerticalBox(Empty);
	UScrollBox* OfferScroll = Tree.ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("OfferScroll"));
	if (UVerticalBoxSlot* Slot = PurchasePage->AddChildToVerticalBox(OfferScroll))
	{
		Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	UVerticalBox* Offers = Tree.ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("OfferList"));
	MarkVariable(Blueprint, *Offers);
	OfferScroll->AddChild(Offers);

	UHorizontalBox* UpgradePage = Tree.ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("UpgradePage"));
	Switcher->AddChild(UpgradePage);
	UVerticalBox* UpgradeColumn = Tree.ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("UpgradeColumn"));
	if (UHorizontalBoxSlot* Slot = UpgradePage->AddChildToHorizontalBox(UpgradeColumn))
	{
		Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		Slot->SetPadding(FMargin(0.f, 0.f, 18.f, 0.f));
	}
	UTextBlock* UpgradeEmpty = MakeWidgetText(Blueprint, TEXT("UpgradeEmptyText"), FText::FromString(TEXT("没有可强化的卡牌")), 17, true);
	UpgradeEmpty->SetJustification(ETextJustify::Center);
	UpgradeColumn->AddChildToVerticalBox(UpgradeEmpty);
	UScrollBox* UpgradeScroll = Tree.ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("UpgradeScroll"));
	if (UVerticalBoxSlot* Slot = UpgradeColumn->AddChildToVerticalBox(UpgradeScroll))
	{
		Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	UVerticalBox* Upgrades = Tree.ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("UpgradeList"));
	MarkVariable(Blueprint, *Upgrades);
	UpgradeScroll->AddChild(Upgrades);

	UVerticalBox* Details = Tree.ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Details"));
	if (UHorizontalBoxSlot* Slot = UpgradePage->AddChildToHorizontalBox(Details))
	{
		Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	UHorizontalBox* Compare = Tree.ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CardCompare"));
	if (UVerticalBoxSlot* Slot = Details->AddChildToVerticalBox(Compare))
	{
		Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	auto AddCard = [&Blueprint, &Tree, &CardViewClass, Compare](FName Name) -> bool
	{
		USizeBox* Box = Tree.ConstructWidget<USizeBox>(USizeBox::StaticClass(), *FString(Name.ToString() + TEXT("Box")));
		Box->SetWidthOverride(230.f);
		Box->SetHeightOverride(330.f);
		UWacomCardView* View = Tree.ConstructWidget<UWacomCardView>(&CardViewClass, Name);
		if (!View)
		{
			return false;
		}
		MarkVariable(Blueprint, *View);
		Box->AddChild(View);
		Compare->AddChildToHorizontalBox(Box);
		return true;
	};
	if (!AddCard(TEXT("CurrentCardView")) || !AddCard(TEXT("NextCardView")))
	{
		OutErrors.Add(TEXT("Could not create WBP_CardView comparison widgets"));
		return false;
	}
	UTextBlock* Summary = MakeWidgetText(Blueprint, TEXT("UpgradeDetailsText"), FText::FromString(TEXT("选择一张卡牌查看强化差异")), 16, true);
	Summary->SetAutoWrapText(true);
	if (UVerticalBoxSlot* Slot = Details->AddChildToVerticalBox(Summary))
	{
		Slot->SetPadding(FMargin(4.f, 8.f));
	}
	UButton* UpgradeAction = MakeButton(
		Blueprint, TEXT("UpgradeActionButton"), FText::FromString(TEXT("请选择卡牌")), TEXT("UpgradeActionText"), true);
	Details->AddChildToVerticalBox(UpgradeAction);

	UButton* Close = MakeButton(Blueprint, TEXT("CloseButton"), FText::FromString(TEXT("关闭")), TEXT("CloseText"));
	if (UVerticalBoxSlot* Slot = Column->AddChildToVerticalBox(Close))
	{
		Slot->SetHorizontalAlignment(HAlign_Center);
		Slot->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));
	}
	Switcher->SetActiveWidgetIndex(0);
	PopulateWidgetGuids(Blueprint);
	return true;
}

bool ValidateShopWidgetBlueprint(UWidgetBlueprint& Blueprint, UClass& CardViewClass, TArray<FString>& OutErrors)
{
	if (Blueprint.ParentClass != UWacomShopScreen::StaticClass()
		|| !Blueprint.GeneratedClass
		|| !Blueprint.GeneratedClass->IsChildOf(UWacomShopScreen::StaticClass())
		|| Blueprint.Status == BS_Error
		|| !Blueprint.WidgetTree)
	{
		OutErrors.Add(TEXT("WBP_ShopScreen parent/generated class/compile status is invalid"));
		return false;
	}
	const TArray<FName> RequiredNames =
	{
		TEXT("TitleText"), TEXT("GoldText"), TEXT("EmptyText"),
		TEXT("PurchaseTabButton"), TEXT("UpgradeTabButton"), TEXT("PageSwitcher"),
		TEXT("OfferList"), TEXT("UpgradeList"), TEXT("UpgradeEmptyText"),
		TEXT("CurrentCardView"), TEXT("NextCardView"), TEXT("UpgradeDetailsText"),
		TEXT("UpgradeActionButton"), TEXT("UpgradeActionText"), TEXT("CloseButton"),
	};
	for (const FName Name : RequiredNames)
	{
		UWidget* Widget = Blueprint.WidgetTree->FindWidget(Name);
		if (!Widget || !Widget->bIsVariable)
		{
			OutErrors.Add(TEXT("WBP_ShopScreen missing bound variable: ") + Name.ToString());
		}
	}
	for (const FName Name : { FName(TEXT("CurrentCardView")), FName(TEXT("NextCardView")) })
	{
		UWidget* Widget = Blueprint.WidgetTree->FindWidget(Name);
		if (Widget && !Widget->IsA(&CardViewClass))
		{
			OutErrors.Add(TEXT("WBP_ShopScreen comparison card is not WBP_CardView: ") + Name.ToString());
		}
	}
	Blueprint.ForEachSourceWidget([&Blueprint, &OutErrors](UWidget* Widget)
	{
		if (!Widget)
		{
			return;
		}
		const FGuid* Guid = Blueprint.WidgetVariableNameToGuidMap.Find(Widget->GetFName());
		if (!Guid || !Guid->IsValid())
		{
			OutErrors.Add(TEXT("WBP_ShopScreen missing WidgetTree GUID: ") + Widget->GetName());
		}
	});
	return OutErrors.IsEmpty();
}

struct FPreflight
{
	UCardDefinition* White = nullptr;
	UCardDefinition* Blue = nullptr;
	UShopDefinition* Shop = nullptr;
	UWidgetBlueprint* Widget = nullptr;
	UClass* CardViewClass = nullptr;
	bool bWhiteExists = false;
	bool bBlueExists = false;
	bool bWidgetExists = false;
	bool bShopConfigured = false;
	bool bWhiteNeedsEffectTargetRepair = false;
	bool bBlueNeedsEffectTargetRepair = false;
	bool bBlueNeedsDisplayNameRepair = false;
};

FPreflight Preflight(TArray<FString>& OutErrors)
{
	FPreflight Facts;
	Facts.CardViewClass = LoadObject<UClass>(nullptr, TEXT("/Game/Wacom/UI/Card/WBP_CardView.WBP_CardView_C"));
	if (!Facts.CardViewClass || !Facts.CardViewClass->IsChildOf(UWacomCardView::StaticClass()))
	{
		OutErrors.Add(TEXT("Required WBP_CardView class is missing or incompatible"));
		return Facts;
	}
	Facts.bBlueExists = FPackageName::DoesPackageExist(BluePackage);
	Facts.bWhiteExists = FPackageName::DoesPackageExist(WhitePackage);
	if (Facts.bBlueExists)
	{
		Facts.Blue = LoadObject<UCardDefinition>(nullptr, *ObjectPath(BluePackage));
		if (!Facts.Blue)
		{
			OutErrors.Add(TEXT("Existing Blue package has wrong class or failed load"));
		}
	}
	if (Facts.bWhiteExists)
	{
		Facts.White = LoadObject<UCardDefinition>(nullptr, *ObjectPath(WhitePackage));
		if (!Facts.White)
		{
			OutErrors.Add(TEXT("Existing White package has wrong class or failed load"));
		}
	}
	if (Facts.Blue)
	{
		const ECardInspectionResult BlueInspection = InspectCard(
			*Facts.Blue, TEXT("Test.ShopUpgrade.VenomProof.Blue"),
			FText::FromString(TEXT("试制毒牙")),
			WacomTags::Card_Rarity_Blue, 5, 2, nullptr, OutErrors);
		Facts.bBlueNeedsEffectTargetRepair =
			BlueInspection == ECardInspectionResult::MissingEffectTargets
			|| BlueInspection == ECardInspectionResult::LegacyDisplayNameAndMissingEffectTargets;
		Facts.bBlueNeedsDisplayNameRepair =
			BlueInspection == ECardInspectionResult::LegacyDisplayName
			|| BlueInspection == ECardInspectionResult::LegacyDisplayNameAndMissingEffectTargets;
	}
	if (Facts.White)
	{
		if (!Facts.Blue)
		{
			OutErrors.Add(TEXT("Existing White card requires an existing authoritative Blue card"));
		}
		else
		{
			Facts.bWhiteNeedsEffectTargetRepair = InspectCard(
				*Facts.White, TEXT("Test.ShopUpgrade.VenomProof.White"),
				FText::FromString(TEXT("试制毒牙")),
				WacomTags::Card_Rarity_White, 3, 1, Facts.Blue, OutErrors)
				== ECardInspectionResult::MissingEffectTargets;
		}
	}

	Facts.Shop = LoadObject<UShopDefinition>(nullptr, *ObjectPath(ShopPackage));
	if (!Facts.Shop)
	{
		OutErrors.Add(TEXT("DA_Shop_DebugSnake missing or wrong class"));
	}
	else
	{
		InspectShop(*Facts.Shop, Facts.White, Facts.bShopConfigured, OutErrors);
	}

	Facts.bWidgetExists = FPackageName::DoesPackageExist(ShopWidgetPackage);
	if (Facts.bWidgetExists)
	{
		Facts.Widget = LoadObject<UWidgetBlueprint>(nullptr, *ObjectPath(ShopWidgetPackage));
		if (!Facts.Widget)
		{
			OutErrors.Add(TEXT("Existing WBP_ShopScreen has wrong class or failed load"));
		}
		else
		{
			ValidateShopWidgetBlueprint(*Facts.Widget, *Facts.CardViewClass, OutErrors);
		}
	}
	return Facts;
}

template<typename TObjectType>
TObjectType* CreateDataAsset(const FString& PackagePath)
{
	UPackage* Package = FindOrCreatePackage(PackagePath);
	if (!Package)
	{
		return nullptr;
	}
	const FName AssetName(*FPackageName::GetLongPackageAssetName(PackagePath));
	TObjectType* Asset = NewObject<TObjectType>(
		Package, AssetName, RF_Public | RF_Standalone | RF_Transactional);
	if (Asset)
	{
		FAssetRegistryModule::AssetCreated(Asset);
	}
	return Asset;
}

UWidgetBlueprint* CreateShopWidget(UClass& CardViewClass, TArray<FString>& OutErrors)
{
	UPackage* Package = FindOrCreatePackage(ShopWidgetPackage);
	if (!Package)
	{
		OutErrors.Add(TEXT("Could not create WBP_ShopScreen package"));
		return nullptr;
	}
	UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(
		UWacomShopScreen::StaticClass(),
		Package,
		FName(TEXT("WBP_ShopScreen")),
		BPTYPE_Normal,
		UWidgetBlueprint::StaticClass(),
		UWidgetBlueprintGeneratedClass::StaticClass()));
	if (!Blueprint || !BuildShopWidgetTree(*Blueprint, CardViewClass, OutErrors))
	{
		return nullptr;
	}
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	if (Blueprint->Status == BS_Error)
	{
		OutErrors.Add(TEXT("WBP_ShopScreen compile failed"));
		return nullptr;
	}
	FAssetRegistryModule::AssetCreated(Blueprint);
	return Blueprint;
}

bool SaveObject(UObject& Object, const FString& PackagePath)
{
	return SaveAssetPackage(Object.GetOutermost(), &Object, PackagePath);
}

FDebugShopUpgradeVerticalSlicePassReport ApplyPass(bool bAllowMutation)
{
	FDebugShopUpgradeVerticalSlicePassReport Report;
	TArray<FString> Errors;
	if (!ValidateDebugShopUpgradeVerticalSliceManifest(Errors))
	{
		Report.Diagnostics = MoveTemp(Errors);
		Report.FailedCount = 1;
		return Report;
	}
	FPreflight Facts = Preflight(Errors);
	if (!Errors.IsEmpty())
	{
		Report.Diagnostics = MoveTemp(Errors);
		Report.FailedCount = 1;
		return Report;
	}
	if (!bAllowMutation)
	{
		Report.ExistingCount = (Facts.bWhiteExists ? 1 : 0)
			+ (Facts.bBlueExists ? 1 : 0) + (Facts.Shop ? 1 : 0)
			+ (Facts.bWidgetExists ? 1 : 0);
		return Report;
	}

	if (!Facts.bBlueExists)
	{
		Facts.Blue = CreateDataAsset<UCardDefinition>(BluePackage);
		if (!Facts.Blue)
		{
			Report.Diagnostics.Add(TEXT("Could not create Blue card"));
			Report.FailedCount = 1;
			return Report;
		}
		ConfigureCard(*Facts.Blue, TEXT("Test.ShopUpgrade.VenomProof.Blue"),
			FText::FromString(TEXT("试制毒牙")), WacomTags::Card_Rarity_Blue, 5, 2, nullptr);
		++Report.CreatedCount;
	}
	else
	{
		if (Facts.bBlueNeedsDisplayNameRepair || Facts.bBlueNeedsEffectTargetRepair)
		{
			RepairCard(
				*Facts.Blue,
				Facts.bBlueNeedsDisplayNameRepair,
				Facts.bBlueNeedsEffectTargetRepair);
			++Report.ModifiedCount;
		}
		else
		{
			++Report.ExistingCount;
		}
	}
	if (!Facts.bWhiteExists)
	{
		Facts.White = CreateDataAsset<UCardDefinition>(WhitePackage);
		if (!Facts.White)
		{
			Report.Diagnostics.Add(TEXT("Could not create White card"));
			Report.FailedCount = 1;
			return Report;
		}
		ConfigureCard(*Facts.White, TEXT("Test.ShopUpgrade.VenomProof.White"),
			FText::FromString(TEXT("试制毒牙")), WacomTags::Card_Rarity_White, 3, 1, Facts.Blue);
		++Report.CreatedCount;
	}
	else
	{
		if (Facts.bWhiteNeedsEffectTargetRepair)
		{
			RepairCard(*Facts.White, false, true);
			++Report.ModifiedCount;
		}
		else
		{
			++Report.ExistingCount;
		}
	}

	if (!Facts.bBlueExists || Facts.bBlueNeedsDisplayNameRepair || Facts.bBlueNeedsEffectTargetRepair)
	{
		if (!SaveObject(*Facts.Blue, BluePackage))
		{
			Report.Diagnostics.Add(TEXT("Blue card save failed"));
			Report.FailedCount = 1;
			return Report;
		}
		++Report.SavedCount;
		Report.SavedPackages.Add(BluePackage);
	}
	if (!Facts.bWhiteExists || Facts.bWhiteNeedsEffectTargetRepair)
	{
		if (!SaveObject(*Facts.White, WhitePackage))
		{
			Report.Diagnostics.Add(TEXT("White card save failed"));
			Report.FailedCount = 1;
			return Report;
		}
		++Report.SavedCount;
		Report.SavedPackages.Add(WhitePackage);
	}

	// Re-check the Shop after the authoritative White asset exists.
	Errors.Reset();
	bool bShopConfigured = false;
	if (!InspectShop(*Facts.Shop, Facts.White, bShopConfigured, Errors))
	{
		Report.Diagnostics.Append(Errors);
		Report.FailedCount = 1;
		return Report;
	}
	if (!bShopConfigured)
	{
		ConfigureShop(*Facts.Shop, *Facts.White);
		if (!SaveObject(*Facts.Shop, ShopPackage))
		{
			Report.Diagnostics.Add(TEXT("Debug Shop save failed"));
			Report.FailedCount = 1;
			return Report;
		}
		++Report.ModifiedCount;
		++Report.SavedCount;
		Report.SavedPackages.Add(ShopPackage);
	}
	else
	{
		++Report.ExistingCount;
	}

	if (!Facts.bWidgetExists)
	{
		Facts.Widget = CreateShopWidget(*Facts.CardViewClass, Report.Diagnostics);
		if (!Facts.Widget || !SaveObject(*Facts.Widget, ShopWidgetPackage))
		{
			if (Report.Diagnostics.IsEmpty())
			{
				Report.Diagnostics.Add(TEXT("WBP_ShopScreen save failed"));
			}
			Report.FailedCount = 1;
			return Report;
		}
		++Report.CreatedCount;
		++Report.SavedCount;
		Report.SavedPackages.Add(ShopWidgetPackage);
	}
	else
	{
		++Report.ExistingCount;
	}

	UPackage::WaitForAsyncFileWrites();
	Errors.Reset();
	FPreflight Reloaded = Preflight(Errors);
	if (!Errors.IsEmpty() || !Reloaded.White || !Reloaded.Blue
		|| !Reloaded.bShopConfigured || !Reloaded.Widget)
	{
		Report.Diagnostics.Append(Errors);
		Report.Diagnostics.Add(TEXT("Post-save strict inspection failed"));
		Report.FailedCount = 1;
	}
	return Report;
}

TSharedRef<FJsonObject> PassToJson(const FDebugShopUpgradeVerticalSlicePassReport& Pass)
{
	TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
	Json->SetNumberField(TEXT("created"), Pass.CreatedCount);
	Json->SetNumberField(TEXT("modified"), Pass.ModifiedCount);
	Json->SetNumberField(TEXT("existing"), Pass.ExistingCount);
	Json->SetNumberField(TEXT("saved"), Pass.SavedCount);
	Json->SetNumberField(TEXT("failed"), Pass.FailedCount);
	TArray<TSharedPtr<FJsonValue>> Saved;
	for (const FString& Package : Pass.SavedPackages)
	{
		Saved.Add(MakeShared<FJsonValueString>(Package));
	}
	Json->SetArrayField(TEXT("savedPackages"), Saved);
	TArray<TSharedPtr<FJsonValue>> Diagnostics;
	for (const FString& Diagnostic : Pass.Diagnostics)
	{
		Diagnostics.Add(MakeShared<FJsonValueString>(Diagnostic));
	}
	Json->SetArrayField(TEXT("diagnostics"), Diagnostics);
	return Json;
}

bool WriteReport(const FDebugShopUpgradeVerticalSliceReport& Report)
{
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("schemaVersion"), 1);
	Root->SetStringField(TEXT("timestampUtc"), FDateTime::UtcNow().ToIso8601());
	Root->SetNumberField(TEXT("exitCode"), Report.ExitCode);
	Root->SetStringField(TEXT("failureCategory"), Report.FailureCategory);
	Root->SetObjectField(TEXT("firstPass"), PassToJson(Report.FirstPass));
	Root->SetObjectField(TEXT("secondPass"), PassToJson(Report.SecondPass));
	TArray<TSharedPtr<FJsonValue>> Manifest;
	for (const FString& Package : GetDebugShopUpgradeVerticalSlicePackageManifest())
	{
		Manifest.Add(MakeShared<FJsonValueString>(Package));
	}
	Root->SetArrayField(TEXT("manifest"), Manifest);
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Report.ReportPath), true);
	FString Output;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
	return FJsonSerializer::Serialize(Root, Writer)
		&& FFileHelper::SaveStringToFile(Output, *Report.ReportPath);
}
}

namespace Wacom::ContentBuilder
{
const TArray<FString>& GetDebugShopUpgradeVerticalSlicePackageManifest()
{
	static const TArray<FString> Manifest =
	{
		WhitePackage,
		BluePackage,
		ShopPackage,
		ShopWidgetPackage,
	};
	return Manifest;
}

bool ValidateDebugShopUpgradeVerticalSliceManifest(TArray<FString>& OutErrors)
{
	const TArray<FString>& Manifest = GetDebugShopUpgradeVerticalSlicePackageManifest();
	if (Manifest.Num() != 4)
	{
		OutErrors.Add(TEXT("Debug Shop upgrade manifest must contain exactly four packages"));
	}
	TSet<FString> Unique;
	for (const FString& Package : Manifest)
	{
		if (!FPackageName::IsValidLongPackageName(Package)
			|| !Package.StartsWith(TEXT("/Game/Wacom/")))
		{
			OutErrors.Add(TEXT("Invalid package: ") + Package);
		}
		if (Unique.Contains(Package))
		{
			OutErrors.Add(TEXT("Duplicate package: ") + Package);
		}
		Unique.Add(Package);
	}
	return OutErrors.IsEmpty();
}

FDebugShopUpgradeVerticalSlicePassReport InspectDebugShopUpgradeVerticalSliceAssets()
{
	return ApplyPass(false);
}

int32 RunDebugShopUpgradeVerticalSliceSeed(FDebugShopUpgradeVerticalSliceReport* OutReport)
{
	FDebugShopUpgradeVerticalSliceReport Report;
	Report.ReportPath = DefaultReportPath();
	Report.FirstPass = ApplyPass(true);
	if (!Report.FirstPass.IsOk())
	{
		Report.ExitCode = 1;
		Report.FailureCategory = TEXT("FirstPass");
	}
	else
	{
		Report.SecondPass = ApplyPass(true);
		if (!Report.SecondPass.IsIdempotent())
		{
			Report.ExitCode = 1;
			Report.FailureCategory = TEXT("Idempotence");
		}
	}
	if (!WriteReport(Report))
	{
		Report.ExitCode = 2;
		Report.FailureCategory = TEXT("ReportWrite");
	}
	UE_LOG(LogTemp, Display,
		TEXT("[DebugShopUpgradeVerticalSlice] First(Created=%d Modified=%d Existing=%d Saved=%d Failed=%d) Second(Created=%d Modified=%d Existing=%d Saved=%d Failed=%d) Report=%s Exit=%d"),
		Report.FirstPass.CreatedCount, Report.FirstPass.ModifiedCount,
		Report.FirstPass.ExistingCount, Report.FirstPass.SavedCount, Report.FirstPass.FailedCount,
		Report.SecondPass.CreatedCount, Report.SecondPass.ModifiedCount,
		Report.SecondPass.ExistingCount, Report.SecondPass.SavedCount, Report.SecondPass.FailedCount,
		*Report.ReportPath, Report.ExitCode);
	for (const FString& Diagnostic : Report.FirstPass.Diagnostics)
	{
		UE_LOG(LogTemp, Error, TEXT("[DebugShopUpgradeVerticalSlice] %s"), *Diagnostic);
	}
	for (const FString& Diagnostic : Report.SecondPass.Diagnostics)
	{
		UE_LOG(LogTemp, Error, TEXT("[DebugShopUpgradeVerticalSlice] %s"), *Diagnostic);
	}
	if (OutReport)
	{
		*OutReport = MoveTemp(Report);
		return OutReport->ExitCode;
	}
	return Report.ExitCode;
}
}

#if WITH_AUTOMATION_TESTS
FWacomDebugShopUpgradeVerticalSliceAutomationSummary
FWacomDebugShopUpgradeVerticalSliceAutomationTestView::InspectRealAssets()
{
	FWacomDebugShopUpgradeVerticalSliceAutomationSummary Summary;
	Summary.PackagePaths = GetDebugShopUpgradeVerticalSlicePackageManifest();
	Summary.ManifestCount = Summary.PackagePaths.Num();
	for (const FString& Package : Summary.PackagePaths)
	{
		Summary.ExistingCount += FPackageName::DoesPackageExist(Package) ? 1 : 0;
	}
	Summary.MissingCount = Summary.ManifestCount - Summary.ExistingCount;
	TArray<FString> Errors;
	const FPreflight Facts = Preflight(Errors);
	Summary.RepairRequiredCount = (Facts.bWhiteNeedsEffectTargetRepair ? 1 : 0)
		+ ((Facts.bBlueNeedsEffectTargetRepair || Facts.bBlueNeedsDisplayNameRepair) ? 1 : 0);
	Summary.FailedCount = Errors.IsEmpty() ? 0 : 1;
	Summary.Diagnostics = MoveTemp(Errors);
	return Summary;
}

bool FWacomDebugShopUpgradeVerticalSliceAutomationTestView::ValidateManifest(TArray<FString>& OutErrors)
{
	return ValidateDebugShopUpgradeVerticalSliceManifest(OutErrors);
}

bool FWacomDebugShopUpgradeVerticalSliceAutomationTestView::ValidateShopCollisionPolicyMatrix(
	TArray<FString>& OutErrors)
{
	auto ExpectRejected = [&OutErrors](int32 Count, bool End, bool Other, bool Pristine, bool Exact)
	{
		TArray<FString> Errors;
		if (ValidateShopCollisionPolicy(Count, End, Other, Pristine, Exact, Errors)
			|| Errors.IsEmpty())
		{
			OutErrors.Add(TEXT("Collision policy accepted an invalid Shop state"));
		}
	};
	TArray<FString> ValidErrors;
	if (!ValidateShopCollisionPolicy(24, false, false, true, false, ValidErrors)
		|| !ValidateShopCollisionPolicy(25, true, false, false, true, ValidErrors))
	{
		OutErrors.Add(TEXT("Collision policy rejected valid pre-seed/seeded states"));
	}
	ExpectRejected(23, false, false, true, false);
	ExpectRejected(25, false, true, true, false);
	ExpectRejected(25, true, false, false, false);
	ExpectRejected(24, false, false, false, true);
	return OutErrors.IsEmpty();
}
#endif
