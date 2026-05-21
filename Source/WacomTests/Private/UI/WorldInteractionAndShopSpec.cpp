// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/BattleTriggerActor.h"
#include "Actors/WacomShopTriggerActor.h"
#include "Cards/CardDefinition.h"
#include "Fixtures/BattleTestFixtures.h"
#include "GameFramework/WacomPlayerController.h"
#include "Interaction/WacomWorldInteractable.h"
#include "RunSession.h"
#include "RunState.h"
#include "Shops/ShopDefinition.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "UI/Foundation/WacomAppToastSubsystem.h"
#include "UI/Foundation/WacomAppToastWidget.h"
#include "UI/Shop/WacomShopOfferRowWidget.h"
#include "UI/Shop/WacomShopPresentationBuilder.h"
#include "UI/Shop/WacomShopScreen.h"

#include "Engine/GameInstance.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	void SetRunSessionForTest(AWacomPlayerController* PC, URunSession* Run)
	{
		FObjectProperty* RunSessionProperty = FindFProperty<FObjectProperty>(PC->GetClass(), TEXT("RunSession"));
		if (RunSessionProperty)
		{
			RunSessionProperty->SetObjectPropertyValue_InContainer(PC, Run);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIWorldInteractionClosestPromptSpec,
	"Wacom.UI.WorldInteraction.ClosestPrompt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIWorldInteractionClosestPromptSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerController> PC(NewObject<AWacomPlayerController>());
	TStrongObjectPtr<APawn> Pawn(NewObject<APawn>());
	TStrongObjectPtr<AWacomShopTriggerActor> Near(NewObject<AWacomShopTriggerActor>());
	TStrongObjectPtr<AWacomShopTriggerActor> Far(NewObject<AWacomShopTriggerActor>());
	TStrongObjectPtr<AWacomShopTriggerActor> Disabled(NewObject<AWacomShopTriggerActor>());

	Pawn->SetActorLocation(FVector::ZeroVector);
	PC->SetPawn(Pawn.Get());

	Near->SetActorLocation(FVector(100.f, 0.f, 0.f));
	Near->PersistentId = TEXT("Shop.Near");
	Near->InteractPromptText = FText::FromString(TEXT("按 E 交易"));
	Far->SetActorLocation(FVector(300.f, 0.f, 0.f));
	Far->PersistentId = TEXT("Shop.Far");
	Far->InteractPromptText = FText::FromString(TEXT("按 E 战斗"));
	Disabled->SetActorLocation(FVector(10.f, 0.f, 0.f));
	Disabled->InteractPromptText = FText::FromString(TEXT("不可用"));

	PC->RegisterCandidateInteractable(Far.Get());
	PC->RegisterCandidateInteractable(Near.Get());
	PC->RegisterCandidateInteractable(Disabled.Get());

	TestTrue(TEXT("Closest available interactable wins"), PC->PickClosestInteractableForTest() == Near.Get());
	TestEqual(TEXT("Prompt comes from closest available interactable"),
		PC->BuildCurrentInteractPromptForTest().ToString(),
		FString(TEXT("按 E 交易")));

	PC->UnregisterCandidateInteractable(Near.Get());
	TestTrue(TEXT("After unregister, far candidate wins"), PC->PickClosestInteractableForTest() == Far.Get());
	TestEqual(TEXT("Disabled candidate still ignored"),
		PC->BuildCurrentInteractPromptForTest().ToString(),
		FString(TEXT("按 E 战斗")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIWorldInteractionShopTriggerNativeContractSpec,
	"Wacom.UI.WorldInteraction.ShopTriggerNativeContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIWorldInteractionShopTriggerNativeContractSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerController> PC(NewObject<AWacomPlayerController>());
	TStrongObjectPtr<AWacomShopTriggerActor> Shop(NewObject<AWacomShopTriggerActor>());

	TestFalse(TEXT("Shop with None id is not interactable"),
		Shop->CanInteract_Implementation(PC.Get()));

	Shop->PersistentId = TEXT("Shop.Test");
	Shop->InteractPromptText = FText::FromString(TEXT("按 E 商店"));

	TestTrue(TEXT("Shop with id is interactable"), Shop->CanInteract_Implementation(PC.Get()));
	TestEqual(TEXT("Shop prompt uses configured text"),
		Shop->GetInteractPromptText_Implementation(PC.Get()).ToString(),
		FString(TEXT("按 E 商店")));
	TestEqual(TEXT("Shop interact location is actor location"),
		Shop->GetInteractLocation_Implementation(PC.Get()),
		Shop->GetActorLocation());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIWorldInteractionBattleTriggerCompatibilitySpec,
	"Wacom.UI.WorldInteraction.BattleTriggerCompatibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIWorldInteractionBattleTriggerCompatibilitySpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerController> PC(NewObject<AWacomPlayerController>());
	TStrongObjectPtr<APawn> Pawn(NewObject<APawn>());
	TStrongObjectPtr<ABattleTriggerActor> Trigger(NewObject<ABattleTriggerActor>());

	Pawn->SetActorLocation(FVector::ZeroVector);
	PC->SetPawn(Pawn.Get());
	Trigger->SetActorLocation(FVector(50.f, 0.f, 0.f));

	TestFalse(TEXT("Battle trigger without enemy is not interactable"),
		Trigger->CanInteract_Implementation(PC.Get()));

	PC->RegisterCandidateTrigger(Trigger.Get());
	TestNull(TEXT("Compatibility registration ignores unavailable trigger"), PC->PickClosestInteractableForTest());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIShopOfferPresentationBuilderSpec,
	"Wacom.UI.Shop.OfferPresentationBuilder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIShopOfferPresentationBuilderSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* DamageCard = Fx.MakeSimpleDamageCard(/*Cost*/2, /*Damage*/5);
	DamageCard->CardId = TEXT("Shop.Damage");
	DamageCard->DisplayName = FText::FromString(TEXT("伤害商品"));

	FRunShopOffer Offer;
	Offer.OfferId = FGuid::NewGuid();
	Offer.CardDefinition = DamageCard;
	Offer.Price = 3;

	FWacomShopOfferPresentationView View =
		UWacomShopPresentationBuilder::BuildOfferPresentationView(Offer, /*CurrentGold*/ 5);
	TestTrue(TEXT("Affordable offer can be purchased"), View.bCanPurchase);
	TestEqual(TEXT("Affordable view keeps card definition"), View.CardDefinition.Get(), DamageCard);
	TestEqual(TEXT("Affordable action text"), View.ActionText.ToString(), FString(TEXT("购买")));
	TestEqual(TEXT("Affordable disabled reason"), View.DisabledReason, NAME_None);
	TestEqual(TEXT("Price text uses gold"), View.PriceText.ToString(), FString(TEXT("3 金币")));
	TestEqual(TEXT("Card view data name from card builder"), View.CardViewData.Name.ToString(), FString(TEXT("伤害商品")));
	TestEqual(TEXT("Card view data cost from card builder"), View.CardViewData.Cost, 2);
	TestEqual(TEXT("Card view data has one badge"), View.CardViewData.EffectBadges.Num(), 1);
	if (View.CardViewData.EffectBadges.IsValidIndex(0))
	{
		TestTrue(TEXT("Badge is damage"),
			View.CardViewData.EffectBadges[0].Kind == EWacomCardViewEffectBadgeKind::Damage);
		TestEqual(TEXT("Damage badge value"), View.CardViewData.EffectBadges[0].Value, 5);
	}

	Offer.bPurchased = true;
	View = UWacomShopPresentationBuilder::BuildOfferPresentationView(Offer, /*CurrentGold*/ 5);
	TestFalse(TEXT("Purchased offer disabled"), View.bCanPurchase);
	TestEqual(TEXT("Purchased action text"), View.ActionText.ToString(), FString(TEXT("已购买")));
	TestEqual(TEXT("Purchased reason"), View.DisabledReason, FName(TEXT("Purchased")));

	Offer.bPurchased = false;
	View = UWacomShopPresentationBuilder::BuildOfferPresentationView(Offer, /*CurrentGold*/ 1);
	TestFalse(TEXT("Insufficient gold disabled"), View.bCanPurchase);
	TestEqual(TEXT("Insufficient action text"), View.ActionText.ToString(), FString(TEXT("金币不足")));
	TestEqual(TEXT("Insufficient reason"), View.DisabledReason, FName(TEXT("InsufficientGold")));

	Offer.Price = 0;
	View = UWacomShopPresentationBuilder::BuildOfferPresentationView(Offer, /*CurrentGold*/ 0);
	TestTrue(TEXT("Free offer can be purchased"), View.bCanPurchase);
	TestEqual(TEXT("Free price text"), View.PriceText.ToString(), FString(TEXT("免费")));

	Offer.CardDefinition = nullptr;
	View = UWacomShopPresentationBuilder::BuildOfferPresentationView(Offer, /*CurrentGold*/ 99);
	TestFalse(TEXT("Missing card disabled"), View.bCanPurchase);
	TestEqual(TEXT("Missing card name"), View.CardNameText.ToString(), FString(TEXT("未知卡牌")));
	TestEqual(TEXT("Missing card action text"), View.ActionText.ToString(), FString(TEXT("不可购买")));
	TestEqual(TEXT("Missing card reason"), View.DisabledReason, FName(TEXT("MissingCard")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIShopPurchaseFailureToastTextSpec,
	"Wacom.UI.Shop.PurchaseFailureToastText",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIShopPurchaseFailureToastTextSpec::RunTest(const FString& /*Parameters*/)
{
	TestEqual(TEXT("Insufficient gold toast"),
		UWacomShopScreen::BuildPurchaseFailureToastTextForTest(TEXT("InsufficientGold")).ToString(),
		FString(TEXT("金币不足")));
	TestEqual(TEXT("Purchased toast"),
		UWacomShopScreen::BuildPurchaseFailureToastTextForTest(TEXT("Purchased")).ToString(),
		FString(TEXT("该商品已购买")));
	TestEqual(TEXT("Missing card toast"),
		UWacomShopScreen::BuildPurchaseFailureToastTextForTest(TEXT("MissingCard")).ToString(),
		FString(TEXT("商品不可购买")));
	TestEqual(TEXT("Fallback purchase failure toast"),
		UWacomShopScreen::BuildPurchaseFailureToastTextForTest(NAME_None).ToString(),
		FString(TEXT("购买失败")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIShopOfferRowPresentationSpec,
	"Wacom.UI.Shop.OfferRowPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIShopOfferRowPresentationSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomShopOfferPresentationView View;
	View.OfferId = FGuid::NewGuid();
	View.CardNameText = FText::FromString(TEXT("行测试卡"));
	View.PriceText = FText::FromString(TEXT("免费"));
	View.ActionText = FText::FromString(TEXT("购买"));
	View.bCanPurchase = true;

	TStrongObjectPtr<UWacomShopOfferRowWidget> Row(NewObject<UWacomShopOfferRowWidget>());
	Row->TakeWidget();
	Row->SetOfferPresentationView(View);
	TestEqual(TEXT("Row stores presentation view"), Row->GetOfferPresentationView().OfferId, View.OfferId);
	TestTrue(TEXT("Row stored offer remains purchasable"), Row->GetOfferPresentationView().bCanPurchase);

	View.bCanPurchase = false;
	View.ActionText = FText::FromString(TEXT("金币不足"));
	View.DisabledReason = TEXT("InsufficientGold");
	Row->SetOfferPresentationView(View);
	TestFalse(TEXT("Row stores disabled view"), Row->GetOfferPresentationView().bCanPurchase);
	TestEqual(TEXT("Row disabled reason stored"),
		Row->GetOfferPresentationView().DisabledReason,
		FName(TEXT("InsufficientGold")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIShopScreenSnapshotAndPurchaseSpec,
	"Wacom.UI.Shop.ScreenSnapshotAndPurchase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIShopScreenSnapshotAndPurchaseSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* ShopCard = Fx.MakeNoopCard(0);
	ShopCard->CardId = TEXT("Shop.Test.Card");
	ShopCard->DisplayName = FText::FromString(TEXT("商店测试卡"));
	UCardDefinition* SecondCard = Fx.MakeNoopCard(0);
	SecondCard->CardId = TEXT("Shop.Test.Second");
	SecondCard->DisplayName = FText::FromString(TEXT("第二张商店卡"));

	TStrongObjectPtr<AWacomPlayerController> PC(NewObject<AWacomPlayerController>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PC.Get()));
	TStrongObjectPtr<UWacomShopScreen> Screen(NewObject<UWacomShopScreen>());
	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<UWacomAppToastSubsystem> ToastSubsystem(NewObject<UWacomAppToastSubsystem>(GameInstance.Get()));
	TStrongObjectPtr<UWacomAppToastWidget> ToastWidget(NewObject<UWacomAppToastWidget>());

	SetRunSessionForTest(PC.Get(), Run.Get());
	if (!TestNotNull(TEXT("Injected run session"), PC->GetRunSession()))
	{
		return false;
	}
	ToastWidget->TakeWidget();
	ToastSubsystem->SetToastWidgetOverrideForTest(ToastWidget.Get());

	Run->AddGold(5);
	TArray<FRunShopOfferInput> Offers;
	Offers.Add({ ShopCard, 3 });
	Offers.Add({ SecondCard, 3 });
	TestTrue(TEXT("Begin shop succeeds"), Run->BeginShopVisit(TEXT("Shop.Screen"), Offers));

	Screen->SetRunSessionOverrideForTest(Run.Get());
	Screen->SetToastSubsystemOverrideForTest(ToastSubsystem.Get());
	Screen->TakeWidget();
	Screen->RefreshShop();

	TestEqual(TEXT("Two offer rows"), Screen->GetOfferRowCount(), 2);
	TestTrue(TEXT("First offer starts purchasable"), Screen->GetOfferPresentationViewForTest(0).bCanPurchase);
	TestTrue(TEXT("Second offer starts purchasable"), Screen->GetOfferPresentationViewForTest(1).bCanPurchase);
	TestTrue(TEXT("Gold text reflects run gold"), Screen->GetGoldTextForTest().ToString().Contains(TEXT("5")));
	TestTrue(TEXT("Purchase first offer succeeds"), Screen->PurchaseOfferByIndexForTest(0));
	TestEqual(TEXT("Gold after purchase"), Run->GetGold(), 2);
	TestEqual(TEXT("Purchase success emits one app toast"), ToastWidget->GetVisibleToastCount(), 1);
	const TArray<FWacomAppToastView> PurchaseToasts = ToastWidget->GetCurrentToastsForTest();
	if (PurchaseToasts.IsValidIndex(0))
	{
		const FWacomAppToastView& Toast = PurchaseToasts[0];
		TestEqual(TEXT("Purchase success toast text"),
			Toast.MessageText.ToString(),
			FString(TEXT("获得卡牌：商店测试卡")));
		TestTrue(TEXT("Purchase success toast is positive"), Toast.Tone == EWacomAppToastTone::Positive);
		TestEqual(TEXT("Purchase success toast icon"), Toast.IconKey, FName(TEXT("CardGained")));
	}
	TestFalse(TEXT("Purchased offer disabled after refresh"), Screen->GetOfferPresentationViewForTest(0).bCanPurchase);
	TestEqual(TEXT("Purchased offer action after refresh"),
		Screen->GetOfferPresentationViewForTest(0).ActionText.ToString(),
		FString(TEXT("已购买")));
	TestFalse(TEXT("Second offer disabled after gold drops"), Screen->GetOfferPresentationViewForTest(1).bCanPurchase);
	TestEqual(TEXT("Second offer becomes insufficient"),
		Screen->GetOfferPresentationViewForTest(1).DisabledReason,
		FName(TEXT("InsufficientGold")));
	const FRunBackpackStorageSnapshot StorageSnapshot = Run->BuildBackpackStorageSnapshot();
	TestTrue(TEXT("Purchased card enters run storage"),
		StorageSnapshot.Flux.ContentCards.ContainsByPredicate([ShopCard](const FRunStorageCardView& CardView)
		{
			return CardView.Instance.Definition.Get() == ShopCard;
		})
		|| StorageSnapshot.BurdenCards.ContainsByPredicate([ShopCard](const FRunStorageCardView& CardView)
		{
			return CardView.Instance.Definition.Get() == ShopCard;
		}));
	TestTrue(TEXT("Shop visit has purchase"), Run->BuildCurrentShopSnapshot().bHasPurchaseThisVisit);

	const int32 NodesBeforeClose = Run->GetRemainingNodeCount();
	Run->EndShopVisit();
	TestFalse(TEXT("Shop visit closed"), Run->IsShopVisitActive());
	TestEqual(TEXT("Close after purchase consumes one node"), Run->GetRemainingNodeCount(), NodesBeforeClose - 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIShopTriggerDefinitionOffersSpec,
	"Wacom.UI.Shop.TriggerDefinitionOffers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIShopTriggerDefinitionOffersSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* DefinitionCard = Fx.MakeNoopCard(0);
	DefinitionCard->CardId = TEXT("Shop.Definition.Card");
	UCardDefinition* ManualCard = Fx.MakeNoopCard(0);
	ManualCard->CardId = TEXT("Shop.Manual.Card");

	TStrongObjectPtr<UShopDefinition> ShopDefinition(NewObject<UShopDefinition>());
	ShopDefinition->ShopId = TEXT("Shop.Definition");
	ShopDefinition->DisplayName = FText::FromString(TEXT("测试商店定义"));
	FShopOfferDefinition DefinitionOffer;
	DefinitionOffer.CardDefinition = DefinitionCard;
	DefinitionOffer.Price = 2;
	ShopDefinition->Offers.Add(DefinitionOffer);

	TStrongObjectPtr<AWacomShopTriggerActor> Shop(NewObject<AWacomShopTriggerActor>());
	Shop->PersistentId = TEXT("Shop.TriggerDefinition");
	Shop->ShopDefinition = ShopDefinition.Get();
	Shop->Offers.Add({ ManualCard, 9 });

	TArray<FRunShopOfferInput> ResolvedOffers = Shop->BuildResolvedOffers();
	TestEqual(TEXT("Definition offer wins count"), ResolvedOffers.Num(), 1);
	if (ResolvedOffers.IsValidIndex(0))
	{
		TestEqual(TEXT("Definition offer wins card"), ResolvedOffers[0].CardDefinition.Get(), DefinitionCard);
		TestEqual(TEXT("Definition offer wins price"), ResolvedOffers[0].Price, 2);
	}

	Shop->ShopDefinition = nullptr;
	ResolvedOffers = Shop->BuildResolvedOffers();
	TestEqual(TEXT("Manual offers fallback count"), ResolvedOffers.Num(), 1);
	if (ResolvedOffers.IsValidIndex(0))
	{
		TestEqual(TEXT("Manual fallback card"), ResolvedOffers[0].CardDefinition.Get(), ManualCard);
		TestEqual(TEXT("Manual fallback price"), ResolvedOffers[0].Price, 9);
	}

	return true;
}
