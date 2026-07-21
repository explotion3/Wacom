// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "RunSession.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/ShopRunEventTestAccess.h"
#include "UI/WacomShopRunEventTestProbes.h"

namespace WacomShopUpgradeScreenSpec
{
FRunShopSnapshot MakeSnapshot(bool bServiceEnabled, bool bCanUpgrade)
{
	FRunShopSnapshot Snapshot;
	Snapshot.ShopId = TEXT("Shop.TestUpgrade");
	Snapshot.bIsActive = true;
	Snapshot.CardUpgradeService.bEnabled = bServiceEnabled;
	if (!bServiceEnabled)
	{
		return Snapshot;
	}

	UCardDefinition* Current = NewObject<UCardDefinition>(GetTransientPackage());
	Current->CardId = TEXT("Test.Upgrade.Current");
	Current->DisplayName = FText::FromString(TEXT("当前卡"));
	Current->Rarity = WacomTags::Card_Rarity_White;
	UCardDefinition* Next = NewObject<UCardDefinition>(GetTransientPackage());
	Next->CardId = TEXT("Test.Upgrade.Next");
	Next->DisplayName = FText::FromString(TEXT("下一卡"));
	Next->Rarity = WacomTags::Card_Rarity_Blue;
	Current->NextUpgradeDefinition = Next;

	for (int32 Index = 0; Index < 2; ++Index)
	{
		FRunShopCardUpgradeQuote& Quote = Snapshot.CardUpgradeQuotes.AddDefaulted_GetRef();
		Quote.InstanceId = FGuid::NewGuid();
		Quote.CurrentDefinition = Current;
		Quote.NextDefinition = Next;
		Quote.Price = 2;
		Quote.bCanUpgrade = bCanUpgrade;
		Quote.DisabledReason = bCanUpgrade ? NAME_None : FName(TEXT("InsufficientGold"));
	}
	return Snapshot;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomShopUpgradeScreenBindingSpec,
	"Wacom.UI.Shop.UpgradeScreen.TabsSelectionAndEquivalentRefresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomShopUpgradeScreenBindingSpec::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UWacomShopScreenProbe> Screen(NewObject<UWacomShopScreenProbe>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Screen->SetRunSession(Run.Get());
	Screen->SetShopSnapshotOverride(WacomShopUpgradeScreenSpec::MakeSnapshot(true, false));
	Screen->TakeWidget();
	Screen->RefreshShop();

	FWacomShopScreenAutomationTestView State = FWacomShopRunEventTestAccess::View(*Screen);
	TestTrue(TEXT("Upgrade service is visible"), State.bUpgradeServiceVisible);
	TestEqual(TEXT("Two physical instances are listed"), State.CachedUpgradeCount, 2);
	TestEqual(TEXT("Initial page is purchase"), State.ActivePage, EWacomShopPage::Purchase);
	TestTrue(TEXT("First instance can be selected"), FWacomShopRunEventTestAccess::SelectUpgradeAt(*Screen, 0));
	State = FWacomShopRunEventTestAccess::View(*Screen);
	TestEqual(TEXT("Selection switches to upgrade page"), State.ActivePage, EWacomShopPage::Upgrade);
	TestTrue(TEXT("Selection keeps an instance id"), State.SelectedUpgradeInstanceId.IsValid());
	TestFalse(TEXT("Insufficient gold disables inline action"), State.bUpgradeActionEnabled);

	const int32 Applies = State.UpgradeRefreshApplyCount;
	Screen->RefreshShop();
	State = FWacomShopRunEventTestAccess::View(*Screen);
	TestEqual(TEXT("Equivalent refresh does not reapply rows"), State.UpgradeRefreshApplyCount, Applies);
	TestTrue(TEXT("Equivalent refresh records skip"), State.UpgradeRefreshSkipCount > 0);
	TestTrue(TEXT("Equivalent refresh preserves selection"), State.SelectedUpgradeInstanceId.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomShopUpgradeScreenServiceDisabledSpec,
	"Wacom.UI.Shop.UpgradeScreen.ServiceDisabledKeepsPurchaseOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomShopUpgradeScreenServiceDisabledSpec::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UWacomShopScreenProbe> Screen(NewObject<UWacomShopScreenProbe>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	Screen->SetRunSession(Run.Get());
	Screen->SetShopSnapshotOverride(WacomShopUpgradeScreenSpec::MakeSnapshot(false, false));
	Screen->TakeWidget();
	Screen->RefreshShop();
	const FWacomShopScreenAutomationTestView State = FWacomShopRunEventTestAccess::View(*Screen);
	TestFalse(TEXT("Upgrade tab is hidden for old shops"), State.bUpgradeServiceVisible);
	TestEqual(TEXT("Old shop remains on purchase page"), State.ActivePage, EWacomShopPage::Purchase);
	TestEqual(TEXT("No upgrade rows are cached"), State.CachedUpgradeCount, 0);
	return true;
}

#endif
