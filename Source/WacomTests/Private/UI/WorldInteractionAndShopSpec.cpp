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
#include "UI/Shop/WacomShopScreen.h"

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
	FWacomUIShopScreenSnapshotAndPurchaseSpec,
	"Wacom.UI.Shop.ScreenSnapshotAndPurchase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIShopScreenSnapshotAndPurchaseSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* ShopCard = Fx.MakeNoopCard(0);
	ShopCard->CardId = TEXT("Shop.Test.Card");
	ShopCard->DisplayName = FText::FromString(TEXT("商店测试卡"));

	TStrongObjectPtr<AWacomPlayerController> PC(NewObject<AWacomPlayerController>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PC.Get()));
	TStrongObjectPtr<UWacomShopScreen> Screen(NewObject<UWacomShopScreen>());

	SetRunSessionForTest(PC.Get(), Run.Get());
	if (!TestNotNull(TEXT("Injected run session"), PC->GetRunSession()))
	{
		return false;
	}

	Run->AddGold(5);
	TArray<FRunShopOfferInput> Offers;
	Offers.Add({ ShopCard, 3 });
	TestTrue(TEXT("Begin shop succeeds"), Run->BeginShopVisit(TEXT("Shop.Screen"), Offers));

	Screen->SetRunSessionOverrideForTest(Run.Get());
	Screen->TakeWidget();
	Screen->RefreshShop();

	TestEqual(TEXT("One offer row"), Screen->GetOfferRowCount(), 1);
	TestTrue(TEXT("Gold text reflects run gold"), Screen->GetGoldTextForTest().ToString().Contains(TEXT("5")));
	TestTrue(TEXT("Purchase first offer succeeds"), Screen->PurchaseOfferByIndexForTest(0));
	TestEqual(TEXT("Gold after purchase"), Run->GetGold(), 2);
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
