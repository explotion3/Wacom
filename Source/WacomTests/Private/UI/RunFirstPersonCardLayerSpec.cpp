// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Components/WacomRunFirstPersonCardSourceComponent.h"
#include "Fixtures/BattleTestFixtures.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "RunSession.h"
#include "RunState.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/RunFirstPersonCardLayerSpecReceiver.h"
#include "UI/WacomShopRunEventTestProbes.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomRunFirstPersonCardLayerSpec
{
	UCardDefinition* MakeTypeAContainerCard(FWacomBattleFixture& Fx, int32 Capacity)
	{
		UCardDefinition* Card = Fx.MakeNoopCard(0);
		Card->CardId = TEXT("Test.TypeAContainer");
		Card->DisplayName = FText::FromString(TEXT("TypeA Container"));
		Card->Physique.Capacity = Capacity;
		return Card;
	}

	UCardDefinition* MakeTypeBContainerCard(FWacomBattleFixture& Fx, int32 Capacity)
	{
		UCardDefinition* Card = Fx.MakeNoopCard(0);
		Card->CardId = TEXT("Test.TypeBContainer");
		Card->DisplayName = FText::FromString(TEXT("TypeB Container"));
		Card->Physique.Capacity = Capacity;
		Card->Physique.CapacityEffect = WacomTags::Card_CapacityEffect_Placeholder;
		return Card;
	}

	UCardDefinition* MakeNamedNoopCard(
		FWacomBattleFixture& Fx,
		FName CardId,
		const FString& DisplayName,
		int32 Cost)
	{
		UCardDefinition* Card = Fx.MakeNoopCard(Cost);
		Card->CardId = CardId;
		Card->DisplayName = FText::FromString(DisplayName);
		return Card;
	}

	FWacomRunMenuCardLeaseRequest MakeLeaseRequest(FName LeaseId = TEXT("ProviderLease"))
	{
		FWacomRunMenuCardLeaseRequest Request;
		Request.LeaseId = LeaseId;
		Request.SourceId = FName(*FString::Printf(TEXT("%sSource"), *LeaseId.ToString()));
		return Request;
	}

	FCardInstance MakeRunCardInstance(UCardDefinition* Definition)
	{
		FCardInstance Instance;
		Instance.InstanceId = FGuid::NewGuid();
		Instance.Definition = Definition;
		return Instance;
	}

	void ResetRunOwnedZones(FRunState& State)
	{
		State.Backpack.Reset();
		State.BattleDeck.Reset();
		State.BurdenZone.Reset();
		State.SpecialZones.Reset();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonBuildsEntriesFromBattleDeckSpec,
	"Wacom.UI.RunFirstPersonCardLayer.BuildsEntriesFromBattleDeckPhysicalCards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonBuildsEntriesFromBattleDeckSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* First = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.RunCard.A"), TEXT("Run Card A"), 1);
	UCardDefinition* Second = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.RunCard.B"), TEXT("Run Card B"), 2);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 3);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { First, Second, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());

	TArray<FWacomFirstPersonCardLayerEntry> Entries;
	TestTrue(TEXT("Build returns entries"), Source->BuildRunFirstPersonCardEntries(*Run, Entries));
	TestEqual(TEXT("BattleDeck cards become first-person entries"), Entries.Num(), 2);
	TestEqual(TEXT("Entry preserves first instance id"),
		Entries[0].CardInstanceId,
		Run->GetBattleDeck()[0].InstanceId);
	TestEqual(TEXT("Entry preserves second instance id"),
		Entries[1].CardInstanceId,
		Run->GetBattleDeck()[1].InstanceId);
	TestEqual(TEXT("Card view data uses presentation display name"),
		Entries[0].CardViewData.Name.ToString(),
		FString(TEXT("Run Card A")));
	TestFalse(TEXT("Run first-person cards are not visually disabled"),
		Entries[0].CardViewData.bDisabled);
	TestTrue(TEXT("Run first-person entries stay visually playable"), Entries[0].bIsPlayable);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonIncludesProjectedBattleDeckCardsSpec,
	"Wacom.UI.RunFirstPersonCardLayer.IncludesProjectedBattleDeckCardsWhenEnabled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonIncludesProjectedBattleDeckCardsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* TypeB = WacomRunFirstPersonCardLayerSpec::MakeTypeBContainerCard(Fx, 3);
	UCardDefinition* Stored = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.Stored"), TEXT("Projected Stored Card"), 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { TypeB, Stored });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run);
	TestEqual(TEXT("Fixture starts with one TypeB special zone"), State.SpecialZones.Num(), 1);
	TestEqual(TEXT("TypeB starts in Backpack"), State.Backpack.Num(), 1);
	TestEqual(TEXT("Stored card starts in BattleDeck"), State.BattleDeck.Num(), 1);

	const FCardInstance TypeBInstance = State.Backpack[0];
	FCardInstance StoredInstance = State.BattleDeck[0];
	State.Backpack.Reset();
	State.BattleDeck.Reset();
	State.BattleDeck.Add(TypeBInstance);
	State.SpecialZones[0].Cards.Add(StoredInstance);
	State.SpecialZones[0].Cards.Last().bBattleEnabledInSpecialZone = true;

	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());

	TArray<FWacomFirstPersonCardLayerEntry> Entries;
	Source->bIncludeProjectedRunBattleDeckCards = true;
	Source->BuildRunFirstPersonCardEntries(*Run, Entries);
	TestEqual(TEXT("Physical + projected card are included"), Entries.Num(), 2);
	TestEqual(TEXT("Projected card keeps its instance id"),
		Entries[1].CardInstanceId,
		StoredInstance.InstanceId);

	Source->bIncludeProjectedRunBattleDeckCards = false;
	Source->BuildRunFirstPersonCardEntries(*Run, Entries);
	TestEqual(TEXT("Projected cards can be excluded"), Entries.Num(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonRefreshWritesAnchorRuntimeSourceSpec,
	"Wacom.UI.RunFirstPersonCardLayer.ExplorationFlowFeedsAnchorRuntimeEntries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonRefreshWritesAnchorRuntimeSourceSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Card = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.Refresh"), TEXT("Refresh Card"), 3);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Card, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());

	Source->SetRunFirstPersonCardLayerActive(true);
	TestEqual(TEXT("Refresh writes once when activated"), Source->WriteCount, 1);
	TestEqual(TEXT("Refresh writes BattleDeck entry"), Source->LastWrittenEntries.Num(), 1);
	TestEqual(TEXT("Debug tracks written entry count"),
		Source->GetRunFirstPersonCardSourceDebugView().EntryCount,
		1);

	Source->ClearRunFirstPersonCardLayer();
	TestEqual(TEXT("Clear goes through runtime source cleanup"), Source->ClearCount, 1);
	TestEqual(TEXT("Clear drops cached test entries"), Source->LastWrittenEntries.Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonRunStateChangedRefreshesSpec,
	"Wacom.UI.RunFirstPersonCardLayer.RunStateChangedRefreshesRunCardLayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonRunStateChangedRefreshesSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Card = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.StateChanged"), TEXT("State Changed Card"), 1);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Card, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	const int32 WritesAfterActivate = Source->WriteCount;

	Run->OnRunStateChangedNative.Broadcast();
	TestEqual(TEXT("Active source refreshes on RunState change"),
		Source->WriteCount,
		WritesAfterActivate + 1);

	Source->SetRunFirstPersonCardLayerActive(false);
	const int32 WritesAfterDeactivate = Source->WriteCount;
	Run->OnRunStateChangedNative.Broadcast();
	TestEqual(TEXT("Inactive source ignores RunState change"),
		Source->WriteCount,
		WritesAfterDeactivate);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonMissingSessionOrAnchorClearsSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MissingSessionOrAnchorClearsRuntimeSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonMissingSessionOrAnchorClearsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();

	Source->SetRunFirstPersonCardLayerActive(true);
	TestEqual(TEXT("Missing session clears runtime source"), Source->ClearCount, 1);
	TestEqual(TEXT("Missing session is reported"),
		Source->GetRunFirstPersonCardSourceDebugView().LastRefreshResult,
		FName(TEXT("MissingRunSession")));

	Source->ClearCount = 0;
	Source->AnchorForTest = nullptr;
	TestFalse(TEXT("Missing anchor refresh fails"), Source->RefreshRunFirstPersonCardLayer());
	TestEqual(TEXT("Missing anchor does not call anchor clear"), Source->ClearCount, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonGameMenuSuppressionClearsDefaultSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuContext.GameMenuSuppressionClearsDefaultBattleDeckSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonGameMenuSuppressionClearsDefaultSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Card = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.MenuSuppress"), TEXT("Menu Suppress Card"), 1);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Card, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	TestEqual(TEXT("Default source writes once"), Source->WriteCount, 1);

	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(true);
	TestTrue(TEXT("Suppression keeps runtime ownership so static preview cannot reappear"),
		Anchor->HasRuntimeCardLayerData());
	TestEqual(TEXT("Suppression writes an empty runtime layer"),
		Anchor->GetRuntimeCardLayerCardCount(),
		0);
	TestEqual(TEXT("Suppression uses its own runtime source"),
		Anchor->GetRuntimeCardLayerSourceId(),
		FName(TEXT("RunFirstPersonMenuSuppressed")));
	TestEqual(TEXT("Suppression reports state"),
		Source->GetRunFirstPersonCardSourceDebugView().LastRefreshResult,
		FName(TEXT("SuppressedByGameMenu")));
	TestTrue(TEXT("Debug marks GameMenu suppression"),
		Source->GetRunFirstPersonCardSourceDebugView().bSuppressedByGameMenu);
	TestEqual(TEXT("Suppression does not keep stale entries"),
		Source->GetRunFirstPersonCardSourceDebugView().EntryCount,
		0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonMenuSuppressionBlocksStaticFallbackSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuContext.GameMenuSuppressionBlocksStaticAnchorFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonMenuSuppressionBlocksStaticFallbackSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Card = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.MenuSuppressStatic"), TEXT("Menu Suppress Static Card"), 1);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Card, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	Anchor->bDrawStaticCardLayer = true;
	Anchor->StaticCardCountFallback = 3;
	TestEqual(TEXT("Static preview is configured with fallback cards"),
		Anchor->StaticCardCountFallback,
		3);

	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	TestEqual(TEXT("Default runtime source has one run card"),
		Anchor->GetRuntimeCardLayerCardCount(),
		1);

	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(true);
	TestTrue(TEXT("Suppressed layer still has runtime data ownership"),
		Anchor->HasRuntimeCardLayerData());
	TestEqual(TEXT("Suppressed runtime entries are empty instead of exposing fallback data"),
		Anchor->GetRuntimeCardLayerEntries().Num(),
		0);
	TestEqual(TEXT("Static preview is still configured, proving runtime ownership blocks fallback"),
		Anchor->StaticCardCountFallback,
		3);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonGameMenuSuppressionReleaseRestoresSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuContext.ReleasingSuppressionRestoresBattleDeckSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonGameMenuSuppressionReleaseRestoresSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Card = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.MenuRestore"), TEXT("Menu Restore Card"), 1);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Card, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(true);
	const int32 WritesWhileSuppressed = Source->WriteCount;

	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(false);
	TestEqual(TEXT("Release suppression writes default source"),
		Source->WriteCount,
		WritesWhileSuppressed + 1);
	TestEqual(TEXT("Restored entry count"),
		Source->LastWrittenEntries.Num(),
		1);
	TestEqual(TEXT("Restore reports refreshed"),
		Source->GetRunFirstPersonCardSourceDebugView().LastRefreshResult,
		FName(TEXT("Refreshed")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonRunStateSuppressedDoesNotRewriteSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuContext.RunStateChangedDoesNotRewriteBattleDeckWhileSuppressed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonRunStateSuppressedDoesNotRewriteSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Card = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.MenuState"), TEXT("Menu State Card"), 1);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Card, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(true);
	const int32 WritesWhileSuppressed = Source->WriteCount;

	Run->OnRunStateChangedNative.Broadcast();
	TestEqual(TEXT("Suppressed source does not rewrite entries"),
		Source->WriteCount,
		WritesWhileSuppressed);
	TestEqual(TEXT("Suppressed state remains reported"),
		Source->GetRunFirstPersonCardSourceDebugView().LastRefreshResult,
		FName(TEXT("SuppressedByGameMenu")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonMenuLeaseOverridesDefaultSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuContext.MenuLeaseOverridesDefaultBattleDeckSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonMenuLeaseOverridesDefaultSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* DefaultCard = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.Default"), TEXT("Default Card"), 1);
	UCardDefinition* LeaseCard = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.Lease"), TEXT("Lease Card"), 2);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { DefaultCard, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);

	FWacomFirstPersonCardLayerEntry LeaseEntry;
	LeaseEntry.CardInstanceId = FGuid::NewGuid();
	LeaseEntry.CardViewData.Name = FText::FromString(TEXT("Lease Card"));
	LeaseEntry.CardViewData.Cost = 2;
	LeaseEntry.CardViewData.bShowCost = true;
	LeaseEntry.CardViewData.bDisabled = false;
	LeaseEntry.bIsPlayable = true;
	LeaseEntry.TargetMode = LeaseCard->TargetMode;

	TestTrue(TEXT("Lease can be set"),
		Source->SetRunFirstPersonCardLayerMenuLease(
			TEXT("RunEventChoice"),
			TEXT("RunEventChoiceSource"),
			{ LeaseEntry }));
	TestEqual(TEXT("Lease writes its own source id"),
		Source->LastWrittenSourceId,
		FName(TEXT("RunEventChoiceSource")));
	TestEqual(TEXT("Lease writes candidate entries"),
		Source->LastWrittenEntries.Num(),
		1);
	TestTrue(TEXT("Debug marks active lease"),
		Source->GetRunFirstPersonCardSourceDebugView().bHasActiveMenuLease);
	TestEqual(TEXT("Debug reports lease entry count"),
		Source->GetRunFirstPersonCardSourceDebugView().ActiveMenuLeaseEntryCount,
		1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonSuppressionDoesNotDisableLeaseSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuContext.GameMenuSuppressionDoesNotDisableActiveMenuLease",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonSuppressionDoesNotDisableLeaseSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Anchor->bEnableClickToPlayCard = true;
	Source->SetRunFirstPersonCardLayerActive(true);
	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(true);

	FWacomFirstPersonCardLayerEntry LeaseEntry;
	LeaseEntry.CardInstanceId = FGuid::NewGuid();
	LeaseEntry.CardViewData.Name = FText::FromString(TEXT("Lease While Suppressed"));
	LeaseEntry.bIsPlayable = true;

	TestTrue(TEXT("Lease writes while GameMenu suppression is active"),
		Source->SetRunFirstPersonCardLayerMenuLease(
			TEXT("MenuLease"),
			TEXT("MenuLeaseSource"),
			{ LeaseEntry }));
	TestEqual(TEXT("Lease source still writes"),
		Source->LastWrittenSourceId,
		FName(TEXT("MenuLeaseSource")));
	TestEqual(TEXT("Lease entry remains visible"),
		Source->LastWrittenEntries.Num(),
		1);
	TestTrue(TEXT("Suppression remains tracked"),
		Source->GetRunFirstPersonCardSourceDebugView().bSuppressedByGameMenu);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonLeaseReleaseRestoresStateSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuContext.ReleasingMenuLeaseRestoresSuppressedOrDefaultState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonLeaseReleaseRestoresStateSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Card = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.LeaseRestore"), TEXT("Lease Restore Card"), 1);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Card, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(true);

	FWacomFirstPersonCardLayerEntry LeaseEntry;
	LeaseEntry.CardInstanceId = FGuid::NewGuid();
	LeaseEntry.CardViewData.Name = FText::FromString(TEXT("Lease"));
	LeaseEntry.bIsPlayable = true;
	Source->SetRunFirstPersonCardLayerMenuLease(TEXT("Lease"), TEXT("LeaseSource"), { LeaseEntry });
	const int32 WritesWithLease = Source->WriteCount;

	TestTrue(TEXT("Lease can be cleared"),
		Source->ClearRunFirstPersonCardLayerMenuLease(TEXT("Lease")));
	TestEqual(TEXT("Suppressed state writes empty runtime source after lease clears"),
		Source->WriteCount,
		WritesWithLease + 1);
	TestEqual(TEXT("Lease release restores suppression source"),
		Source->LastWrittenSourceId,
		FName(TEXT("RunFirstPersonMenuSuppressed")));
	TestEqual(TEXT("Lease release does not expose stale lease entries"),
		Source->LastWrittenEntries.Num(),
		0);
	TestEqual(TEXT("Suppression is restored"),
		Source->GetRunFirstPersonCardSourceDebugView().LastRefreshResult,
		FName(TEXT("SuppressedByGameMenu")));

	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(false);
	TestEqual(TEXT("Default returns after suppression ends"),
		Source->LastWrittenSourceId,
		Source->RunFirstPersonCardLayerSourceId);
	TestEqual(TEXT("Default has entries"),
		Source->LastWrittenEntries.Num(),
		1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonDifferentLeaseCannotStealSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuContext.DifferentLeaseIdCannotStealActiveLease",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonDifferentLeaseCannotStealSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->SetRunFirstPersonCardLayerActive(true);

	FWacomFirstPersonCardLayerEntry LeaseEntry;
	LeaseEntry.CardInstanceId = FGuid::NewGuid();
	LeaseEntry.CardViewData.Name = FText::FromString(TEXT("Lease"));
	LeaseEntry.bIsPlayable = true;

	TestTrue(TEXT("First lease succeeds"),
		Source->SetRunFirstPersonCardLayerMenuLease(TEXT("LeaseA"), TEXT("LeaseASource"), { LeaseEntry }));
	TestFalse(TEXT("Different lease cannot steal active lease"),
		Source->SetRunFirstPersonCardLayerMenuLease(TEXT("LeaseB"), TEXT("LeaseBSource"), { LeaseEntry }));
	TestEqual(TEXT("Conflict is reported"),
		Source->GetRunFirstPersonCardSourceDebugView().LastRefreshResult,
		FName(TEXT("MenuLeaseConflict")));
	TestEqual(TEXT("Original lease source remains active"),
		Source->GetRunFirstPersonCardSourceDebugView().ActiveMenuLeaseSourceId,
		FName(TEXT("LeaseASource")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonClearLayerClearsMenuContextSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuContext.ClearRunFirstPersonCardLayerClearsLeaseAndSuppressionOutput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonClearLayerClearsMenuContextSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->SetRunFirstPersonCardLayerActive(true);
	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(true);

	FWacomFirstPersonCardLayerEntry LeaseEntry;
	LeaseEntry.CardInstanceId = FGuid::NewGuid();
	LeaseEntry.CardViewData.Name = FText::FromString(TEXT("Lease"));
	LeaseEntry.bIsPlayable = true;
	Source->SetRunFirstPersonCardLayerMenuLease(TEXT("Lease"), TEXT("LeaseSource"), { LeaseEntry });

	Source->ClearRunFirstPersonCardLayer();
	const FWacomRunFirstPersonCardSourceDebugView Debug = Source->GetRunFirstPersonCardSourceDebugView();
	TestFalse(TEXT("Suppression is cleared"), Debug.bSuppressedByGameMenu);
	TestFalse(TEXT("Lease is cleared"), Debug.bHasActiveMenuLease);
	TestEqual(TEXT("Visible entry output is cleared"), Debug.EntryCount, 0);
	TestEqual(TEXT("Clear reports result"), Debug.LastRefreshResult, FName(TEXT("Cleared")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonDebugSummaryReportsMenuContextSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuContext.DebugSummaryReportsMenuContext",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonDebugSummaryReportsMenuContextSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->SetRunFirstPersonCardLayerActive(true);
	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(true);

	FWacomFirstPersonCardLayerEntry LeaseEntry;
	LeaseEntry.CardInstanceId = FGuid::NewGuid();
	LeaseEntry.CardViewData.Name = FText::FromString(TEXT("Lease"));
	LeaseEntry.bIsPlayable = true;
	Source->SetRunFirstPersonCardLayerMenuLease(TEXT("Lease"), TEXT("LeaseSource"), { LeaseEntry });

	const FString Summary = Source->GetRunFirstPersonCardSourceDebugSummary();
	TestTrue(TEXT("Summary includes suppression"),
		Summary.Contains(TEXT("SuppressedByGameMenu=true")));
	TestTrue(TEXT("Summary includes lease id"),
		Summary.Contains(TEXT("LeaseId=Lease")));
	TestTrue(TEXT("Summary includes lease source"),
		Summary.Contains(TEXT("LeaseSource=LeaseSource")));
	TestTrue(TEXT("Summary includes lease entry count"),
		Summary.Contains(TEXT("LeaseEntries=1")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonMenuLeaseCanEnableDragProbeSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuContext.MenuLeaseCanEnableFirstPersonDragProbe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonMenuLeaseCanEnableDragProbeSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->SetRunFirstPersonCardLayerActive(true);
	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(true);

	FWacomFirstPersonCardLayerEntry LeaseEntry;
	LeaseEntry.CardInstanceId = FGuid::NewGuid();
	LeaseEntry.CardViewData.Name = FText::FromString(TEXT("Lease"));
	LeaseEntry.bIsPlayable = true;
	LeaseEntry.TargetMode = ECardTargetMode::SingleEnemyPart;

	TestTrue(TEXT("Menu lease can be written"),
		Source->SetRunFirstPersonCardLayerMenuLease(TEXT("Lease"), TEXT("LeaseSource"), { LeaseEntry }));
	TestTrue(TEXT("Menu lease enables first-person interaction for probe"),
		Anchor->IsBattleHandInteractionPrototypeEnabled());
	TestFalse(TEXT("Menu lease disables quick click-to-play while probe drag is active"),
		Anchor->bEnableClickToPlayCard);

	Source->ClearRunFirstPersonCardLayerMenuLease(TEXT("Lease"));
	TestFalse(TEXT("Suppressed default source disables interaction after lease clears"),
		Anchor->IsBattleHandInteractionPrototypeEnabled());
	TestTrue(TEXT("Menu lease restores the anchor click-to-play setting after clear"),
		Anchor->bEnableClickToPlayCard);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonRequestBuildsLeaseEntriesFromDefinitionsSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuLeaseProvider.RequestBuildsLeaseEntriesFromAllowedDefinitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonRequestBuildsLeaseEntriesFromDefinitionsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("毒牙"), 0);
	UCardDefinition* Other = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Other"), TEXT("Other"), 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Fang });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run);
	WacomRunFirstPersonCardLayerSpec::ResetRunOwnedZones(State);
	State.Backpack = {
		WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Fang),
		WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Other)
	};

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(true);

	FWacomRunMenuCardLeaseRequest Request =
		WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest();
	Request.AllowedCardDefinitions.Add(Fang);
	FWacomRunMenuCardLeaseResult Result;

	TestTrue(TEXT("Provider sets lease from allowed definition"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, Result));
	TestTrue(TEXT("Result reports lease set"), Result.bLeaseSet);
	TestEqual(TEXT("Only matching definition becomes a candidate"), Result.CandidateCount, 1);
	TestEqual(TEXT("Written lease entry preserves Fang instance"),
		Source->LastWrittenEntries[0].CardInstanceId,
		State.Backpack[0].InstanceId);
	TestEqual(TEXT("Written lease uses provider source id"),
		Source->LastWrittenSourceId,
		Request.SourceId);
	TestEqual(TEXT("Card face uses presentation data"),
		Source->LastWrittenEntries[0].CardViewData.Name.ToString(),
		FString(TEXT("毒牙")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonRequestMatchesAllowedCardIdsSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuLeaseProvider.RequestMatchesAllowedCardIds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonRequestMatchesAllowedCardIdsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("毒牙"), 0);
	UCardDefinition* Other = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Other"), TEXT("Other"), 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Fang });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run);
	WacomRunFirstPersonCardLayerSpec::ResetRunOwnedZones(State);
	State.BattleDeck = {
		WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Other),
		WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Fang)
	};

	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);

	FWacomRunMenuCardLeaseRequest Request =
		WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("CardIdLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	FWacomRunMenuCardLeaseResult Result;

	TestTrue(TEXT("Provider matches by CardId"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, Result));
	TestEqual(TEXT("One CardId match is shown"), Result.CandidateCount, 1);
	TestEqual(TEXT("Matched entry is Fang"),
		Source->LastWrittenEntries[0].CardInstanceId,
		State.BattleDeck[1].InstanceId);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonRequestMatchesExplicitInstanceIdsSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuLeaseProvider.RequestMatchesExplicitInstanceIds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonRequestMatchesExplicitInstanceIdsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Shared = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Shared"), TEXT("Shared"), 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Shared });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run);
	WacomRunFirstPersonCardLayerSpec::ResetRunOwnedZones(State);
	State.Backpack = {
		WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Shared),
		WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Shared)
	};

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);

	FWacomRunMenuCardLeaseRequest Request =
		WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("InstanceLease"));
	Request.AllowedCardIds.Add(TEXT("Shared"));
	Request.ExplicitCardInstanceIds.Add(State.Backpack[1].InstanceId);
	FWacomRunMenuCardLeaseResult Result;

	TestTrue(TEXT("Provider matches explicit instance whitelist"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, Result));
	TestEqual(TEXT("Only whitelisted instance is shown"), Result.CandidateCount, 1);
	TestEqual(TEXT("Matched entry is second shared instance"),
		Source->LastWrittenEntries[0].CardInstanceId,
		State.Backpack[1].InstanceId);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonRequestUsesKeywordsSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuLeaseProvider.RequestUsesRequiredAndBlockedKeywords",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonRequestUsesKeywordsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Companion = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Companion"), TEXT("Companion"), 1);
	Companion->Keywords.AddTag(WacomTags::Card_Keyword_Companion);
	UCardDefinition* WeaponCompanion = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("WeaponCompanion"), TEXT("Weapon Companion"), 1);
	WeaponCompanion->Keywords.AddTag(WacomTags::Card_Keyword_Companion);
	WeaponCompanion->Keywords.AddTag(WacomTags::Card_Keyword_Weapon);
	UCardDefinition* Plain = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Plain"), TEXT("Plain"), 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Companion });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run);
	WacomRunFirstPersonCardLayerSpec::ResetRunOwnedZones(State);
	State.Backpack = {
		WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Companion),
		WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(WeaponCompanion),
		WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Plain)
	};

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);

	FWacomRunMenuCardLeaseRequest Request =
		WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("KeywordLease"));
	Request.RequiredKeywords.AddTag(WacomTags::Card_Keyword_Companion);
	Request.BlockedKeywords.AddTag(WacomTags::Card_Keyword_Weapon);
	FWacomRunMenuCardLeaseResult Result;

	TestTrue(TEXT("Provider applies required and blocked keywords"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, Result));
	TestEqual(TEXT("Only non-weapon companion is shown"), Result.CandidateCount, 1);
	TestEqual(TEXT("Matched entry is companion"),
		Source->LastWrittenEntries[0].CardInstanceId,
		State.Backpack[0].InstanceId);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonAllHeldZonesNoProjectedDuplicatesSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuLeaseProvider.AllHeldZonesAreIncludedWithoutProjectedDuplicates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonAllHeldZonesNoProjectedDuplicatesSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Match = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Match"), TEXT("Match"), 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Match });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run);
	WacomRunFirstPersonCardLayerSpec::ResetRunOwnedZones(State);
	State.Backpack = { WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Match) };
	State.BattleDeck = { WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Match) };
	State.BurdenZone = { WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Match) };
	FSpecialZone SpecialZone;
	SpecialZone.OwnerInstanceId = FGuid::NewGuid();
	SpecialZone.Cards.Add(WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Match));
	SpecialZone.Cards[0].bBattleEnabledInSpecialZone = true;
	State.SpecialZones = { SpecialZone };

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);

	FWacomRunMenuCardLeaseRequest Request =
		WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("AllZonesLease"));
	Request.AllowedCardDefinitions.Add(Match);
	FWacomRunMenuCardLeaseResult Result;

	TestTrue(TEXT("Provider builds candidates from all physical owned zones"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, Result));
	TestEqual(TEXT("Backpack, BattleDeck, Burden and SpecialZone are included once each"),
		Result.CandidateCount,
		4);
	TestEqual(TEXT("Projected duplicates are not added"),
		Source->LastWrittenEntries.Num(),
		4);
	TestEqual(TEXT("SpecialZone physical card is the last candidate"),
		Source->LastWrittenEntries[3].CardInstanceId,
		State.SpecialZones[0].Cards[0].InstanceId);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonNoMatchingClearsSameLeaseSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuLeaseProvider.NoMatchingCardsClearsExistingSameLease",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonNoMatchingClearsSameLeaseSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Match = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Match"), TEXT("Match"), 1);
	UCardDefinition* Other = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Other"), TEXT("Other"), 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Match });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run);
	WacomRunFirstPersonCardLayerSpec::ResetRunOwnedZones(State);
	State.Backpack = { WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Match) };

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(true);

	FWacomRunMenuCardLeaseRequest Request =
		WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("SameLease"));
	Request.AllowedCardDefinitions.Add(Match);
	FWacomRunMenuCardLeaseResult Result;
	TestTrue(TEXT("Initial matching lease succeeds"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, Result));
	TestTrue(TEXT("Lease is active"), Source->HasActiveMenuLease());

	Request.AllowedCardDefinitions.Reset();
	Request.AllowedCardDefinitions.Add(Other);
	TestFalse(TEXT("No matching candidates rejects"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, Result));
	TestEqual(TEXT("No match reason is reported"),
		Result.RejectReason,
		FName(TEXT("NoMatchingCandidates")));
	TestFalse(TEXT("Same lease is cleared to avoid stale candidates"),
		Source->HasActiveMenuLease());
	TestEqual(TEXT("Suppression output is restored after clearing stale lease"),
		Source->LastWrittenEntries.Num(),
		0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonEmptyFilterRejectsUnlessAllowAllSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuLeaseProvider.EmptyFilterRejectsUnlessAllowAllIsEnabled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonEmptyFilterRejectsUnlessAllowAllSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* First = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("First"), TEXT("First"), 1);
	UCardDefinition* Second = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Second"), TEXT("Second"), 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { First });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run);
	WacomRunFirstPersonCardLayerSpec::ResetRunOwnedZones(State);
	State.Backpack = {
		WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(First),
		WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Second)
	};

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);

	FWacomRunMenuCardLeaseRequest Request =
		WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("EmptyLease"));
	FWacomRunMenuCardLeaseResult Result;
	TestFalse(TEXT("Empty filter rejects by default"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, Result));
	TestEqual(TEXT("Empty filter reason is reported"),
		Result.RejectReason,
		FName(TEXT("EmptyFilter")));

	Request.bAllowAllOwnedCardsWhenNoFilter = true;
	TestTrue(TEXT("Allow all enables empty filter request"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, Result));
	TestEqual(TEXT("All owned physical cards are shown"),
		Result.CandidateCount,
		2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonMenuWidgetOwnedLeaseClearsSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuLeaseProvider.MenuWidgetOwnedLeaseAutoClearsOnDeactivate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonMenuWidgetOwnedLeaseClearsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("毒牙"), 0);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Fang });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run);
	WacomRunFirstPersonCardLayerSpec::ResetRunOwnedZones(State);
	State.Backpack = { WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Fang) };

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	PC->SetRunSessionForTest(Run.Get());
	TStrongObjectPtr<AWacomPlayerCharacter> CharacterPawn(NewObject<AWacomPlayerCharacter>());
	PC->SetPawn(CharacterPawn.Get());
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> Menu(NewObject<UWacomMenuWidgetBaseProbe>(PC.Get()));
	Menu->SetOwningWacomPlayerControllerForTest(PC.Get());

	FWacomRunMenuCardLeaseRequest Request;
	Request.AllowedCardDefinitions.Add(Fang);
	FWacomRunMenuCardLeaseResult Result;
	TestTrue(TEXT("Menu widget can set owned lease with generated ids"),
		Menu->SetOwnedRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, Result));

	UWacomRunFirstPersonCardSourceComponent* Source = PC->GetRunFirstPersonCardSourceComponent();
	TestTrue(TEXT("PC source has active menu lease"),
		Source && Source->HasActiveMenuLease());
	TestFalse(TEXT("Generated lease id is non-empty"),
		Source->GetActiveMenuLeaseId().IsNone());

	Menu->DeactivateForTest();
	TestFalse(TEXT("Owned lease is cleared on deactivate"),
		Source->HasActiveMenuLease());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonCppTestMenuRequestsOwnedLeaseSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuLeaseProvider.CppTestMenuRequestsOwnedLease",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonCppTestMenuRequestsOwnedLeaseSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("毒牙"), 0);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Fang });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run);
	WacomRunFirstPersonCardLayerSpec::ResetRunOwnedZones(State);
	State.Backpack = { WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Fang) };

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	PC->SetRunSessionForTest(Run.Get());
	TStrongObjectPtr<AWacomPlayerCharacter> CharacterPawn(NewObject<AWacomPlayerCharacter>());
	PC->SetPawn(CharacterPawn.Get());

	TStrongObjectPtr<UWacomRunMenuCardLeaseTestMenuProbe> Menu(
		NewObject<UWacomRunMenuCardLeaseTestMenuProbe>(PC.Get()));
	Menu->SetOwningWacomPlayerControllerForTest(PC.Get());
	Menu->LeaseRequest.AllowedCardIds = { TEXT("PoisonFang") };

	TestTrue(TEXT("C++ test menu requests owned lease"),
		Menu->RequestOwnedLeaseNow());
	TestTrue(TEXT("Result reports one candidate"),
		Menu->GetLastLeaseResult().CandidateCount == 1);

	UWacomRunFirstPersonCardSourceComponent* Source = PC->GetRunFirstPersonCardSourceComponent();
	TestTrue(TEXT("PC source has active lease after C++ test menu request"),
		Source && Source->HasActiveMenuLease());

	Menu->DeactivateForTest();
	TestFalse(TEXT("C++ test menu owned lease clears on deactivate"),
		Source->HasActiveMenuLease());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonLeaseProviderRejectsMissingAnchorSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuLeaseProvider.MissingAnchorRejectsWithoutActiveLease",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonLeaseProviderRejectsMissingAnchorSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("毒牙"), 0);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Fang });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run);
	WacomRunFirstPersonCardLayerSpec::ResetRunOwnedZones(State);
	State.Backpack = { WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Fang) };

	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);

	FWacomRunMenuCardLeaseRequest Request =
		WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("MissingAnchorLease"));
	Request.AllowedCardDefinitions.Add(Fang);
	FWacomRunMenuCardLeaseResult Result;

	TestFalse(TEXT("Provider rejects when no anchor can display the lease"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, Result));
	TestEqual(TEXT("Missing anchor reason is reported"),
		Result.RejectReason,
		FName(TEXT("MissingAnchor")));
	TestFalse(TEXT("Rejected provider request does not leave an active lease"),
		Source->HasActiveMenuLease());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonLeaseProviderDebugReportsSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuLeaseProvider.LeaseProviderDebugReportsCandidateResult",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonLeaseProviderDebugReportsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("毒牙"), 0);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Fang });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run);
	WacomRunFirstPersonCardLayerSpec::ResetRunOwnedZones(State);
	State.Backpack = { WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Fang) };

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);

	FWacomRunMenuCardLeaseRequest Request =
		WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("DebugLease"));
	Request.AllowedCardDefinitions.Add(Fang);
	FWacomRunMenuCardLeaseResult Result;
	TestTrue(TEXT("Provider sets lease"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, Result));

	const FWacomRunFirstPersonCardSourceDebugView Debug =
		Source->GetRunFirstPersonCardSourceDebugView();
	TestEqual(TEXT("Debug stores provider lease id"),
		Debug.LastMenuLeaseProviderLeaseId,
		Request.LeaseId);
	TestEqual(TEXT("Debug stores provider source id"),
		Debug.LastMenuLeaseProviderSourceId,
		Request.SourceId);
	TestEqual(TEXT("Debug stores candidate count"),
		Debug.LastMenuLeaseProviderCandidateCount,
		1);
	TestTrue(TEXT("Debug summary includes provider result"),
		Source->GetRunFirstPersonCardSourceDebugSummary().Contains(TEXT("Provider{")));
	TestTrue(TEXT("Result summary includes candidate count"),
		Result.DebugSummary.Contains(TEXT("Candidates=1")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonDefaultBattleDeckDisablesInteractionSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuContext.DefaultBattleDeckSourceStillDisablesInteraction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonDefaultBattleDeckDisablesInteractionSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Card = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.DefaultInteraction"), TEXT("Default Interaction Card"), 1);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Card, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());

	Source->SetRunFirstPersonCardLayerActive(true);
	TestFalse(TEXT("Default Run BattleDeck source remains non-interactive"),
		Anchor->IsBattleHandInteractionPrototypeEnabled());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonDragUpdateOverMenuZoneReportsTargetSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuContext.DragUpdateOverMenuZoneReportsZoneTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonDragUpdateOverMenuZoneReportsTargetSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<UWacomRunMenuDropTargetWidgetProbe> Target(
		NewObject<UWacomRunMenuDropTargetWidgetProbe>(PC.Get()));
	Target->ZoneId = TEXT("RunEvent.Pay.Fang");
	Target->bProbeHitForTest = true;
	PC->RegisterRunMenuDropTargetForTest(Target.Get());

	FWacomInteractionTargetHandle Handle;
	TestTrue(TEXT("Menu zone is probed"),
		PC->ProbeRunMenuDropTargetAtWidgetPositionForTest(FVector2D(100.0f, 200.0f), Handle));
	TestEqual(TEXT("Zone target id is reported"),
		Handle.ZoneId,
		FName(TEXT("RunEvent.Pay.Fang")));
	TestEqual(TEXT("Target receives probe preview"),
		Target->GetRunMenuDropPreviewState(),
		EWacomRunMenuDropTargetPreviewState::Normal);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonDragReleaseOnMenuZoneProbeOnlySpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuContext.DragReleaseOnMenuZoneProbeOnlyWhenMenuDoesNotAccept",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonDragReleaseOnMenuZoneProbeOnlySpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	UWacomRunFirstPersonCardSourceComponent* Source = PC->GetRunFirstPersonCardSourceComponent();
	TStrongObjectPtr<UWacomRunMenuDropTargetWidgetProbe> Target(
		NewObject<UWacomRunMenuDropTargetWidgetProbe>(PC.Get()));
	TStrongObjectPtr<UWacomShopScreenProbe> ActiveMenu(NewObject<UWacomShopScreenProbe>(PC.Get()));
	Target->ZoneId = TEXT("RunEvent.Pay.Fang");
	Target->bProbeHitForTest = true;
	PC->RegisterRunMenuDropTargetForTest(Target.Get());

	FWacomFirstPersonCardLayerEntry LeaseEntry;
	LeaseEntry.CardInstanceId = FGuid::NewGuid();
	LeaseEntry.CardViewData.Name = FText::FromString(TEXT("Lease"));
	LeaseEntry.bIsPlayable = true;
	LeaseEntry.TargetMode = ECardTargetMode::SingleEnemyPart;
	TestTrue(TEXT("Lease is accepted"),
		PC->SetRunFirstPersonCardLayerMenuLease(TEXT("RunEventLease"), TEXT("RunEventLeaseSource"), { LeaseEntry }));

	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId = LeaseEntry.CardInstanceId;
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.CurrentScreenPosition = FVector2D(100.0f, 200.0f);
	DragView.PointerViewportPosition = FVector2D(100.0f, 200.0f);
	DragView.bHasPointerViewportPosition = true;

	PC->RegisterActiveGameMenuWidget(ActiveMenu.Get());
	PC->ApplyRunMenuDropProbeFeedbackForTest(LeaseEntry.CardInstanceId, DragView, /*bReleased*/ true);

	const FString Debug = PC->ReadRunMenuDropProbeDebugSummaryForTest();
	TestTrue(TEXT("Release is probe-only"),
		Debug.Contains(TEXT("Intent=ProbeZoneTarget")));
	TestTrue(TEXT("Debug includes zone id"),
		Debug.Contains(TEXT("ZoneId=RunEvent.Pay.Fang")));

	if (Source)
	{
		TestTrue(TEXT("Run source lease still exists after probe release"),
			Source->HasActiveMenuLease());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonMenuDropProbeClearSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuContext.DragCancelLeaseClearMenuCloseClearsZonePreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonMenuDropProbeClearSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<UWacomRunMenuDropTargetWidgetProbe> Target(
		NewObject<UWacomRunMenuDropTargetWidgetProbe>(PC.Get()));
	TStrongObjectPtr<UWacomShopScreenProbe> ActiveMenu(NewObject<UWacomShopScreenProbe>(PC.Get()));
	Target->ZoneId = TEXT("RunEvent.Clear.Zone");
	Target->bProbeHitForTest = true;
	PC->RegisterRunMenuDropTargetForTest(Target.Get());

	FWacomFirstPersonCardLayerEntry LeaseEntry;
	LeaseEntry.CardInstanceId = FGuid::NewGuid();
	LeaseEntry.CardViewData.Name = FText::FromString(TEXT("Lease"));
	LeaseEntry.bIsPlayable = true;
	LeaseEntry.TargetMode = ECardTargetMode::SingleEnemyPart;
	TestTrue(TEXT("Lease is accepted"),
		PC->SetRunFirstPersonCardLayerMenuLease(TEXT("RunEventLease"), TEXT("RunEventLeaseSource"), { LeaseEntry }));
	PC->RegisterActiveGameMenuWidget(ActiveMenu.Get());

	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId = LeaseEntry.CardInstanceId;
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.CurrentScreenPosition = FVector2D(100.0f, 200.0f);
	DragView.PointerViewportPosition = FVector2D(100.0f, 200.0f);
	DragView.bHasPointerViewportPosition = true;
	PC->ApplyRunMenuDropProbeFeedbackForTest(LeaseEntry.CardInstanceId, DragView, /*bReleased*/ false);
	TestEqual(TEXT("Probe preview is active before clear"),
		Target->GetRunMenuDropPreviewState(),
		EWacomRunMenuDropTargetPreviewState::Probe);

	PC->ClearRunMenuDropTargetProbeForTest();

	TestEqual(TEXT("Preview clears"),
		Target->GetRunMenuDropPreviewState(),
		EWacomRunMenuDropTargetPreviewState::Normal);
	TestTrue(TEXT("Debug reports clear"),
		PC->ReadRunMenuDropProbeDebugSummaryForTest().Contains(TEXT("Cleared")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunMenuCardDropRejectsWithoutLeaseSpec,
	"Wacom.UI.RunMenuCardDropIntent.ResolveRejectsWithoutActiveMenuLease",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunMenuCardDropRejectsWithoutLeaseSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<UWacomRunMenuDropTargetWidgetProbe> Target(
		NewObject<UWacomRunMenuDropTargetWidgetProbe>(PC.Get()));
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> ActiveMenu(NewObject<UWacomMenuWidgetBaseProbe>(PC.Get()));
	ActiveMenu->SetOwningWacomPlayerControllerForTest(PC.Get());
	Target->ZoneId = TEXT("RunEvent.Pay.Fang");
	Target->bProbeHitForTest = true;
	PC->RegisterRunMenuDropTargetForTest(Target.Get());
	PC->RegisterActiveGameMenuWidget(ActiveMenu.Get());

	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId = FGuid::NewGuid();
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.CurrentScreenPosition = FVector2D(100.0f, 200.0f);
	DragView.PointerViewportPosition = FVector2D(100.0f, 200.0f);
	DragView.bHasPointerViewportPosition = true;

	const FWacomRunMenuCardDropResolveResult Result =
		PC->ResolveRunMenuCardDropIntentForTest(DragView.CardInstanceId, DragView);
	TestEqual(TEXT("Intent rejects without lease"),
		Result.IntentKind,
		EWacomRunMenuCardDropIntentKind::Reject);
	TestEqual(TEXT("Reject reason reports missing lease"),
		Result.RejectReason,
		EWacomRunMenuCardDropRejectReason::MissingMenuLease);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunMenuCardDropProbeOnlySpec,
	"Wacom.UI.RunMenuCardDropIntent.ResolveProbeOnlyWhenMenuDoesNotAcceptPayment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunMenuCardDropProbeOnlySpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("Poison Fang"), 0);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Fang });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	PC->SetRunSessionForTest(Run.Get());
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> ActiveMenu(NewObject<UWacomMenuWidgetBaseProbe>(PC.Get()));
	ActiveMenu->SetOwningWacomPlayerControllerForTest(PC.Get());
	TStrongObjectPtr<UWacomRunMenuDropTargetWidgetProbe> Target(
		NewObject<UWacomRunMenuDropTargetWidgetProbe>(PC.Get()));
	Target->ZoneId = TEXT("RunEvent.Pay.Fang");
	Target->bProbeHitForTest = true;
	PC->RegisterRunMenuDropTargetForTest(Target.Get());

	FWacomRunMenuCardLeaseRequest Request = WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("RunEventLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Lease is set"),
		ActiveMenu->SetOwnedRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, LeaseResult));
	PC->RegisterActiveGameMenuWidget(ActiveMenu.Get());

	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId = Run->GetRunState().Backpack[0].InstanceId;
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.CurrentScreenPosition = FVector2D(100.0f, 200.0f);
	DragView.PointerViewportPosition = FVector2D(100.0f, 200.0f);
	DragView.bHasPointerViewportPosition = true;

	const FWacomRunMenuCardDropResolveResult Result =
		PC->ResolveRunMenuCardDropIntentForTest(DragView.CardInstanceId, DragView);
	TestEqual(TEXT("Intent remains probe-only"),
		Result.IntentKind,
		EWacomRunMenuCardDropIntentKind::ProbeZoneTarget);
	TestEqual(TEXT("Menu does not accept by default"),
		Result.RejectReason,
		EWacomRunMenuCardDropRejectReason::MenuDoesNotAccept);
	TestFalse(TEXT("Probe-only intent cannot submit"), Result.bCanSubmit);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunMenuCardDropAcceptedZoneSpec,
	"Wacom.UI.RunMenuCardDropIntent.AcceptedZoneResolvesPayOwnedCardToZone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunMenuCardDropAcceptedZoneSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("Poison Fang"), 0);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Fang });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	PC->SetRunSessionForTest(Run.Get());
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> ActiveMenu(NewObject<UWacomMenuWidgetBaseProbe>(PC.Get()));
	ActiveMenu->SetOwningWacomPlayerControllerForTest(PC.Get());
	ActiveMenu->bAcceptOwnedRunFirstPersonCardPaymentForTest = true;
	ActiveMenu->AcceptedZoneIdForTest = TEXT("RunEvent.Pay.Fang");
	TStrongObjectPtr<UWacomRunMenuDropTargetWidgetProbe> Target(
		NewObject<UWacomRunMenuDropTargetWidgetProbe>(PC.Get()));
	Target->ZoneId = TEXT("RunEvent.Pay.Fang");
	Target->bProbeHitForTest = true;
	PC->RegisterRunMenuDropTargetForTest(Target.Get());

	FWacomRunMenuCardLeaseRequest Request = WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("RunEventLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Lease is set"),
		ActiveMenu->SetOwnedRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, LeaseResult));
	PC->RegisterActiveGameMenuWidget(ActiveMenu.Get());

	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId = Run->GetRunState().Backpack[0].InstanceId;
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.CurrentScreenPosition = FVector2D(100.0f, 200.0f);
	DragView.PointerViewportPosition = FVector2D(100.0f, 200.0f);
	DragView.bHasPointerViewportPosition = true;

	const FWacomRunMenuCardDropResolveResult Result =
		PC->ResolveRunMenuCardDropIntentForTest(DragView.CardInstanceId, DragView);
	TestEqual(TEXT("Accepted zone resolves payment"),
		Result.IntentKind,
		EWacomRunMenuCardDropIntentKind::PayOwnedCardToZone);
	TestTrue(TEXT("Payment intent can submit"), Result.bCanSubmit);
	TestEqual(TEXT("Zone id preserved"),
		Result.ZoneId,
		FName(TEXT("RunEvent.Pay.Fang")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunMenuCardDropReleaseDestroysSpec,
	"Wacom.UI.RunMenuCardDropIntent.ReleaseOnAcceptedZoneDestroysExactOwnedInstance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunMenuCardDropReleaseDestroysSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("Poison Fang"), 0);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Fang });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	PC->SetRunSessionForTest(Run.Get());
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> ActiveMenu(NewObject<UWacomMenuWidgetBaseProbe>(PC.Get()));
	ActiveMenu->SetOwningWacomPlayerControllerForTest(PC.Get());
	ActiveMenu->bAcceptOwnedRunFirstPersonCardPaymentForTest = true;
	ActiveMenu->AcceptedZoneIdForTest = TEXT("RunEvent.Pay.Fang");
	TStrongObjectPtr<UWacomRunMenuDropTargetWidgetProbe> Target(
		NewObject<UWacomRunMenuDropTargetWidgetProbe>(PC.Get()));
	Target->ZoneId = TEXT("RunEvent.Pay.Fang");
	Target->bProbeHitForTest = true;
	PC->RegisterRunMenuDropTargetForTest(Target.Get());

	FWacomRunMenuCardLeaseRequest Request = WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("RunEventLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Lease is set"),
		ActiveMenu->SetOwnedRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, LeaseResult));
	PC->RegisterActiveGameMenuWidget(ActiveMenu.Get());

	const FGuid PaidId = Run->GetRunState().Backpack[0].InstanceId;
	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId = PaidId;
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.CurrentScreenPosition = FVector2D(100.0f, 200.0f);
	DragView.PointerViewportPosition = FVector2D(100.0f, 200.0f);
	DragView.bHasPointerViewportPosition = true;

	PC->ApplyRunMenuDropProbeFeedbackForTest(PaidId, DragView, /*bReleased*/ true);

	FCardInstance Found;
	EZoneKind FoundZone = EZoneKind::Backpack;
	FGuid FoundOwner;
	TestFalse(TEXT("Paid instance is removed from owned zones"),
		Run->FindInstance(PaidId, Found, FoundZone, FoundOwner));
	TestTrue(TEXT("Menu receives submitted payment result"),
		ActiveMenu->LastPaymentResultForTest.bSubmitted);
	TestEqual(TEXT("Drop target shows submitted preview"),
		Target->GetRunMenuDropPreviewState(),
		EWacomRunMenuDropTargetPreviewState::PaymentSubmitted);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunMenuCardDropValidationFailsSpec,
	"Wacom.UI.RunMenuCardDropIntent.ReleaseRejectsWhenRunSessionValidationFails",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunMenuCardDropValidationFailsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("Poison Fang"), 0);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Fang });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	PC->SetRunSessionForTest(Run.Get());
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> ActiveMenu(NewObject<UWacomMenuWidgetBaseProbe>(PC.Get()));
	ActiveMenu->SetOwningWacomPlayerControllerForTest(PC.Get());
	ActiveMenu->bAcceptOwnedRunFirstPersonCardPaymentForTest = true;
	ActiveMenu->AcceptedZoneIdForTest = TEXT("RunEvent.Pay.Fang");
	TStrongObjectPtr<UWacomRunMenuDropTargetWidgetProbe> Target(
		NewObject<UWacomRunMenuDropTargetWidgetProbe>(PC.Get()));
	Target->ZoneId = TEXT("RunEvent.Pay.Fang");
	Target->bProbeHitForTest = true;
	PC->RegisterRunMenuDropTargetForTest(Target.Get());

	FWacomFirstPersonCardLayerEntry LeaseEntry;
	LeaseEntry.CardInstanceId = FGuid::NewGuid();
	LeaseEntry.CardViewData.Name = FText::FromString(TEXT("Phantom Fang"));
	LeaseEntry.bIsPlayable = true;
	LeaseEntry.TargetMode = ECardTargetMode::SingleEnemyPart;
	TestTrue(TEXT("Manual lease is accepted"),
		PC->SetRunFirstPersonCardLayerMenuLease(TEXT("RunEventLease"), TEXT("RunEventLeaseSource"), { LeaseEntry }));
	PC->RegisterActiveGameMenuWidget(ActiveMenu.Get());

	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId = LeaseEntry.CardInstanceId;
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.CurrentScreenPosition = FVector2D(100.0f, 200.0f);
	DragView.PointerViewportPosition = FVector2D(100.0f, 200.0f);
	DragView.bHasPointerViewportPosition = true;

	const FWacomRunMenuCardDropResolveResult Result =
		PC->ResolveRunMenuCardDropIntentForTest(LeaseEntry.CardInstanceId, DragView);
	TestEqual(TEXT("Missing owned card rejects"),
		Result.IntentKind,
		EWacomRunMenuCardDropIntentKind::Reject);
	TestEqual(TEXT("Reject reason is card not owned"),
		Result.RejectReason,
		EWacomRunMenuCardDropRejectReason::CardNotOwned);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunMenuCardDropRefreshesLeaseSpec,
	"Wacom.UI.RunMenuCardDropIntent.PaidCardRefreshesProviderBackedLeaseEntries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunMenuCardDropRefreshesLeaseSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("Poison Fang"), 0);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Fang, Fang, Pack });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run);
	WacomRunFirstPersonCardLayerSpec::ResetRunOwnedZones(State);
	State.Backpack = {
		WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Fang),
		WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Fang)
	};

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	PC->SetRunSessionForTest(Run.Get());
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> ActiveMenu(NewObject<UWacomMenuWidgetBaseProbe>(PC.Get()));
	ActiveMenu->SetOwningWacomPlayerControllerForTest(PC.Get());
	ActiveMenu->bAcceptOwnedRunFirstPersonCardPaymentForTest = true;
	ActiveMenu->AcceptedZoneIdForTest = TEXT("RunEvent.Pay.Fang");
	TStrongObjectPtr<UWacomRunMenuDropTargetWidgetProbe> Target(
		NewObject<UWacomRunMenuDropTargetWidgetProbe>(PC.Get()));
	Target->ZoneId = TEXT("RunEvent.Pay.Fang");
	Target->bProbeHitForTest = true;
	PC->RegisterRunMenuDropTargetForTest(Target.Get());

	FWacomRunMenuCardLeaseRequest Request = WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("RunEventLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Lease is set"),
		ActiveMenu->SetOwnedRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, LeaseResult));
	PC->RegisterActiveGameMenuWidget(ActiveMenu.Get());
	UWacomRunFirstPersonCardSourceComponent* Source = PC->GetRunFirstPersonCardSourceComponent();
	TestTrue(TEXT("Lease has two candidates"),
		Source && Source->GetRunFirstPersonCardSourceDebugView().ActiveMenuLeaseEntryCount == 2);

	const FGuid PaidId = State.Backpack[0].InstanceId;
	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId = PaidId;
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.CurrentScreenPosition = FVector2D(100.0f, 200.0f);
	DragView.PointerViewportPosition = FVector2D(100.0f, 200.0f);
	DragView.bHasPointerViewportPosition = true;
	PC->ApplyRunMenuDropProbeFeedbackForTest(PaidId, DragView, /*bReleased*/ true);

	TestTrue(TEXT("Provider-backed lease remains active"),
		Source && Source->HasActiveMenuLease());
	TestEqual(TEXT("Lease refreshes to remaining candidate"),
		Source->GetRunFirstPersonCardSourceDebugView().ActiveMenuLeaseEntryCount,
		1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunMenuCardDropNoCandidatesClearsLeaseSpec,
	"Wacom.UI.RunMenuCardDropIntent.NoRemainingCandidatesClearsProviderBackedLease",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunMenuCardDropNoCandidatesClearsLeaseSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("Poison Fang"), 0);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Fang });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	PC->SetRunSessionForTest(Run.Get());
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> ActiveMenu(NewObject<UWacomMenuWidgetBaseProbe>(PC.Get()));
	ActiveMenu->SetOwningWacomPlayerControllerForTest(PC.Get());
	ActiveMenu->bAcceptOwnedRunFirstPersonCardPaymentForTest = true;
	ActiveMenu->AcceptedZoneIdForTest = TEXT("RunEvent.Pay.Fang");
	TStrongObjectPtr<UWacomRunMenuDropTargetWidgetProbe> Target(
		NewObject<UWacomRunMenuDropTargetWidgetProbe>(PC.Get()));
	Target->ZoneId = TEXT("RunEvent.Pay.Fang");
	Target->bProbeHitForTest = true;
	PC->RegisterRunMenuDropTargetForTest(Target.Get());

	FWacomRunMenuCardLeaseRequest Request = WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("RunEventLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Lease is set"),
		ActiveMenu->SetOwnedRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, LeaseResult));
	PC->RegisterActiveGameMenuWidget(ActiveMenu.Get());

	const FGuid PaidId = Run->GetRunState().Backpack[0].InstanceId;
	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId = PaidId;
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.CurrentScreenPosition = FVector2D(100.0f, 200.0f);
	DragView.PointerViewportPosition = FVector2D(100.0f, 200.0f);
	DragView.bHasPointerViewportPosition = true;
	PC->ApplyRunMenuDropProbeFeedbackForTest(PaidId, DragView, /*bReleased*/ true);

	UWacomRunFirstPersonCardSourceComponent* Source = PC->GetRunFirstPersonCardSourceComponent();
	TestTrue(TEXT("Source exists"), Source != nullptr);
	TestFalse(TEXT("Provider-backed lease clears after last candidate"),
		Source->HasActiveMenuLease());
	TestEqual(TEXT("Provider reports no candidates"),
		Source->GetRunFirstPersonCardSourceDebugView().LastMenuLeaseProviderResult,
		FName(TEXT("NoMatchingCandidates")));

	return true;
}
