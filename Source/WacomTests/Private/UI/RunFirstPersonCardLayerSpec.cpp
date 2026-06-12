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
#include "UI/FirstPersonCardLayerTestAccess.h"
#include "UI/PlayerControllerRunInteractionTestAccess.h"
#include "UI/RunMenuDropTargetWidgetTestAccess.h"
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

	UCharacterDefinition* MakePaymentTestCharacter(
		FWacomBattleFixture& Fx,
		UCardDefinition* PaidCard)
	{
		UCardDefinition* Pack = MakeTypeAContainerCard(Fx, 2);
		return Fx.MakeCharacter(nullptr, nullptr, { PaidCard, Pack });
	}

	void AttachFirstPersonPawnForTest(AWacomPlayerControllerProbe* PC)
	{
		if (PC)
		{
			PC->SetPawn(NewObject<AWacomPlayerCharacter>(PC));
		}
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

	FGuid FindOwnedInstanceIdByDefinition(const FRunState& State, const UCardDefinition* Definition)
	{
		if (!Definition)
		{
			return FGuid();
		}

		auto FindIn = [Definition](const TArray<FCardInstance>& Instances)
		{
			for (const FCardInstance& Instance : Instances)
			{
				if (Instance.Definition == Definition)
				{
					return Instance.InstanceId;
				}
			}
			return FGuid();
		};

		if (const FGuid FoundId = FindIn(State.Backpack); FoundId.IsValid())
		{
			return FoundId;
		}
		if (const FGuid FoundId = FindIn(State.BattleDeck); FoundId.IsValid())
		{
			return FoundId;
		}
		if (const FGuid FoundId = FindIn(State.BurdenZone); FoundId.IsValid())
		{
			return FoundId;
		}
		for (const FSpecialZone& SpecialZone : State.SpecialZones)
		{
			if (const FGuid FoundId = FindIn(SpecialZone.Cards); FoundId.IsValid())
			{
				return FoundId;
			}
		}
		return FGuid();
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
	FWacomUIRunFirstPersonRunStateUnchangedRevisionSkipsDefaultRewriteSpec,
	"Wacom.UI.RunFirstPersonCardLayer.RunStateChangedWithUnchangedStorageRevisionSkipsDefaultSourceRewrite",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonRunStateUnchangedRevisionSkipsDefaultRewriteSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Card = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.StateChangedSkip"), TEXT("State Changed Skip Card"), 1);
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
#if WITH_AUTOMATION_TESTS
	FWacomFirstPersonCardLayerTestAccess::ResetSourceCounters(*Source);
#endif

	Run->OnRunStateChangedNative.Broadcast();
	TestEqual(TEXT("Unchanged storage revision skips source rewrite"),
		Source->WriteCount,
		WritesAfterActivate);
	TestEqual(TEXT("Skip is reported"),
		Source->GetRunFirstPersonCardSourceDebugView().LastRefreshResult,
		FName(TEXT("SkippedUnchangedRevision")));
#if WITH_AUTOMATION_TESTS
	TestEqual(TEXT("Revision skip count increments"),
		FWacomFirstPersonCardLayerTestAccess::DefaultSourceCounters(*Source).RevisionSkipCount,
		1);
	TestEqual(TEXT("Skipped refresh does not rebuild snapshot"),
		FWacomFirstPersonCardLayerTestAccess::DefaultSourceCounters(*Source).DataBuildCount,
		0);
	TestEqual(TEXT("Skipped refresh does not apply runtime source"),
		FWacomFirstPersonCardLayerTestAccess::DefaultSourceCounters(*Source).RuntimeApplyCount,
		0);
#endif
	TestEqual(TEXT("Debug keeps previous entry count"),
		Source->GetRunFirstPersonCardSourceDebugView().EntryCount,
		1);

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
	FWacomUIRunFirstPersonMenuSuppressionBlocksDevelopmentPreviewSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuContext.GameMenuSuppressionBlocksDevelopmentPreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonMenuSuppressionBlocksDevelopmentPreviewSpec::RunTest(const FString& /*Parameters*/)
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
	Anchor->bDrawPreviewCardLayer = true;
	Anchor->PreviewCardCountFallback = 3;
	TestEqual(TEXT("Development preview is configured with placeholder cards"),
		Anchor->PreviewCardCountFallback,
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
	TestEqual(TEXT("Suppressed runtime entries are empty instead of exposing preview data"),
		Anchor->GetRuntimeCardLayerEntries().Num(),
		0);
	TestEqual(TEXT("Development preview is still configured, proving runtime ownership blocks preview data"),
		Anchor->PreviewCardCountFallback,
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
		Anchor->IsBattleHandInteractionEnabled());

	Source->ClearRunFirstPersonCardLayerMenuLease(TEXT("Lease"));
	TestFalse(TEXT("Suppressed default source disables interaction after lease clears"),
		Anchor->IsBattleHandInteractionEnabled());

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
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(PC.Get(), Run.Get());
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
	FWacomUIRunFirstPersonPrototypeTestMenuRequestsOwnedLeaseSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuLeaseProvider.PrototypeTestMenuRequestsOwnedLease",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonPrototypeTestMenuRequestsOwnedLeaseSpec::RunTest(const FString& /*Parameters*/)
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
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(PC.Get(), Run.Get());
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
	FWacomUIRunFirstPersonDefaultBattleDeckEnablesRunWorldDragSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuContext.DefaultBattleDeckSourceEnablesRunWorldDrag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonDefaultBattleDeckEnablesRunWorldDragSpec::RunTest(const FString& /*Parameters*/)
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
	TestTrue(TEXT("Default Run BattleDeck source enables run-world drag probe"),
		Anchor->IsBattleHandInteractionEnabled());

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
	FWacomRunMenuDropTargetWidgetTestAccess::SetProbeHit(Target.Get(), true);
	FWacomPlayerControllerRunInteractionTestAccess::RegisterRunMenuDropTarget(PC.Get(), Target.Get());

	FWacomInteractionTargetHandle Handle;
	TestTrue(TEXT("Menu zone is probed"),
		FWacomPlayerControllerRunInteractionTestAccess::ProbeRunMenuDropTargetAtWidgetPosition(PC.Get(), FVector2D(100.0f, 200.0f), Handle));
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
	FWacomRunMenuDropTargetWidgetTestAccess::SetProbeHit(Target.Get(), true);
	FWacomPlayerControllerRunInteractionTestAccess::RegisterRunMenuDropTarget(PC.Get(), Target.Get());

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
	FWacomPlayerControllerRunInteractionTestAccess::ApplyRunMenuDropProbeFeedback(PC.Get(), LeaseEntry.CardInstanceId, DragView, /*bReleased*/ true);

	const FString Debug = FWacomPlayerControllerRunInteractionTestAccess::RunMenuDropProbeDebugSummary(PC.Get());
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
	FWacomRunMenuDropTargetWidgetTestAccess::SetProbeHit(Target.Get(), true);
	FWacomPlayerControllerRunInteractionTestAccess::RegisterRunMenuDropTarget(PC.Get(), Target.Get());

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
	FWacomPlayerControllerRunInteractionTestAccess::ApplyRunMenuDropProbeFeedback(PC.Get(), LeaseEntry.CardInstanceId, DragView, /*bReleased*/ false);
	TestEqual(TEXT("Probe preview is active before clear"),
		Target->GetRunMenuDropPreviewState(),
		EWacomRunMenuDropTargetPreviewState::Probe);

	FWacomPlayerControllerRunInteractionTestAccess::ClearRunMenuDropTargetProbe(PC.Get());

	TestEqual(TEXT("Preview clears"),
		Target->GetRunMenuDropPreviewState(),
		EWacomRunMenuDropTargetPreviewState::Normal);
	TestTrue(TEXT("Debug reports clear"),
		FWacomPlayerControllerRunInteractionTestAccess::RunMenuDropProbeDebugSummary(PC.Get()).Contains(TEXT("Cleared")));

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
	FWacomRunMenuDropTargetWidgetTestAccess::SetProbeHit(Target.Get(), true);
	FWacomPlayerControllerRunInteractionTestAccess::RegisterRunMenuDropTarget(PC.Get(), Target.Get());
	PC->RegisterActiveGameMenuWidget(ActiveMenu.Get());

	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId = FGuid::NewGuid();
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.CurrentScreenPosition = FVector2D(100.0f, 200.0f);
	DragView.PointerViewportPosition = FVector2D(100.0f, 200.0f);
	DragView.bHasPointerViewportPosition = true;

	const FWacomRunMenuCardDropResolveResult Result =
		FWacomPlayerControllerRunInteractionTestAccess::ResolveRunMenuCardDropIntent(PC.Get(), DragView.CardInstanceId, DragView);
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
	"Wacom.UI.RunMenuCardDropIntent.ResolveProbeOnlyWhenMenuReturnsProbe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunMenuCardDropProbeOnlySpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("Poison Fang"), 0);
	UCharacterDefinition* Character = WacomRunFirstPersonCardLayerSpec::MakePaymentTestCharacter(Fx, Fang);
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(PC.Get(), Run.Get());
	WacomRunFirstPersonCardLayerSpec::AttachFirstPersonPawnForTest(PC.Get());
	PC->SetRunFirstPersonCardLayerActive(true);
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> ActiveMenu(NewObject<UWacomMenuWidgetBaseProbe>(PC.Get()));
	ActiveMenu->SetOwningWacomPlayerControllerForTest(PC.Get());
	TStrongObjectPtr<UWacomRunMenuDropTargetWidgetProbe> Target(
		NewObject<UWacomRunMenuDropTargetWidgetProbe>(PC.Get()));
	Target->ZoneId = TEXT("RunEvent.Pay.Fang");
	FWacomRunMenuDropTargetWidgetTestAccess::SetProbeHit(Target.Get(), true);
	FWacomPlayerControllerRunInteractionTestAccess::RegisterRunMenuDropTarget(PC.Get(), Target.Get());

	FWacomRunMenuCardLeaseRequest Request = WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("RunEventLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Lease is set"),
		ActiveMenu->SetOwnedRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, LeaseResult));
	PC->RegisterActiveGameMenuWidget(ActiveMenu.Get());

	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId =
		WacomRunFirstPersonCardLayerSpec::FindOwnedInstanceIdByDefinition(Run->GetRunState(), Fang);
	TestTrue(TEXT("Fang instance found"), DragView.CardInstanceId.IsValid());
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.CurrentScreenPosition = FVector2D(100.0f, 200.0f);
	DragView.PointerViewportPosition = FVector2D(100.0f, 200.0f);
	DragView.bHasPointerViewportPosition = true;

	const FWacomRunMenuCardDropResolveResult Result =
		FWacomPlayerControllerRunInteractionTestAccess::ResolveRunMenuCardDropIntent(PC.Get(), DragView.CardInstanceId, DragView);
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
	"Wacom.UI.RunMenuCardDropIntent.AcceptedZoneResolvesControllerDestroyOwnedCardPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunMenuCardDropAcceptedZoneSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("Poison Fang"), 0);
	UCharacterDefinition* Character = WacomRunFirstPersonCardLayerSpec::MakePaymentTestCharacter(Fx, Fang);
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(PC.Get(), Run.Get());
	WacomRunFirstPersonCardLayerSpec::AttachFirstPersonPawnForTest(PC.Get());
	PC->SetRunFirstPersonCardLayerActive(true);
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> ActiveMenu(NewObject<UWacomMenuWidgetBaseProbe>(PC.Get()));
	ActiveMenu->SetOwningWacomPlayerControllerForTest(PC.Get());
	ActiveMenu->bAcceptRunMenuFirstPersonCardDropForTest = true;
	ActiveMenu->SubmitPolicyForTest =
		EWacomRunMenuCardDropSubmitPolicy::ControllerDestroyOwnedCard;
	ActiveMenu->AcceptedZoneIdForTest = TEXT("RunEvent.Pay.Fang");
	TStrongObjectPtr<UWacomRunMenuDropTargetWidgetProbe> Target(
		NewObject<UWacomRunMenuDropTargetWidgetProbe>(PC.Get()));
	Target->ZoneId = TEXT("RunEvent.Pay.Fang");
	FWacomRunMenuDropTargetWidgetTestAccess::SetProbeHit(Target.Get(), true);
	FWacomPlayerControllerRunInteractionTestAccess::RegisterRunMenuDropTarget(PC.Get(), Target.Get());

	FWacomRunMenuCardLeaseRequest Request = WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("RunEventLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Lease is set"),
		ActiveMenu->SetOwnedRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, LeaseResult));
	PC->RegisterActiveGameMenuWidget(ActiveMenu.Get());

	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId =
		WacomRunFirstPersonCardLayerSpec::FindOwnedInstanceIdByDefinition(Run->GetRunState(), Fang);
	TestTrue(TEXT("Fang instance found"), DragView.CardInstanceId.IsValid());
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.CurrentScreenPosition = FVector2D(100.0f, 200.0f);
	DragView.PointerViewportPosition = FVector2D(100.0f, 200.0f);
	DragView.bHasPointerViewportPosition = true;

	const FWacomRunMenuCardDropResolveResult Result =
		FWacomPlayerControllerRunInteractionTestAccess::ResolveRunMenuCardDropIntent(PC.Get(), DragView.CardInstanceId, DragView);
	TestEqual(TEXT("Accepted zone resolves submit intent"),
		Result.IntentKind,
		EWacomRunMenuCardDropIntentKind::SubmitZoneTarget);
	TestEqual(TEXT("Submit policy is controller destroy"),
		Result.SubmitPolicy,
		EWacomRunMenuCardDropSubmitPolicy::ControllerDestroyOwnedCard);
	TestTrue(TEXT("Submit intent can submit"), Result.bCanSubmit);
	TestEqual(TEXT("Zone id preserved"),
		Result.ZoneId,
		FName(TEXT("RunEvent.Pay.Fang")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunMenuCardDropReleaseDestroysSpec,
	"Wacom.UI.RunMenuCardDropIntent.ReleaseWithControllerDestroyPolicyDestroysExactOwnedInstance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunMenuCardDropReleaseDestroysSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("Poison Fang"), 0);
	UCharacterDefinition* Character = WacomRunFirstPersonCardLayerSpec::MakePaymentTestCharacter(Fx, Fang);
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(PC.Get(), Run.Get());
	WacomRunFirstPersonCardLayerSpec::AttachFirstPersonPawnForTest(PC.Get());
	PC->SetRunFirstPersonCardLayerActive(true);
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> ActiveMenu(NewObject<UWacomMenuWidgetBaseProbe>(PC.Get()));
	ActiveMenu->SetOwningWacomPlayerControllerForTest(PC.Get());
	ActiveMenu->bAcceptRunMenuFirstPersonCardDropForTest = true;
	ActiveMenu->SubmitPolicyForTest =
		EWacomRunMenuCardDropSubmitPolicy::ControllerDestroyOwnedCard;
	ActiveMenu->AcceptedZoneIdForTest = TEXT("RunEvent.Pay.Fang");
	TStrongObjectPtr<UWacomRunMenuDropTargetWidgetProbe> Target(
		NewObject<UWacomRunMenuDropTargetWidgetProbe>(PC.Get()));
	Target->ZoneId = TEXT("RunEvent.Pay.Fang");
	FWacomRunMenuDropTargetWidgetTestAccess::SetProbeHit(Target.Get(), true);
	FWacomPlayerControllerRunInteractionTestAccess::RegisterRunMenuDropTarget(PC.Get(), Target.Get());

	FWacomRunMenuCardLeaseRequest Request = WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("RunEventLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Lease is set"),
		ActiveMenu->SetOwnedRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, LeaseResult));
	PC->RegisterActiveGameMenuWidget(ActiveMenu.Get());

	const FGuid PaidId =
		WacomRunFirstPersonCardLayerSpec::FindOwnedInstanceIdByDefinition(Run->GetRunState(), Fang);
	TestTrue(TEXT("Fang instance found"), PaidId.IsValid());
	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId = PaidId;
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.CurrentScreenPosition = FVector2D(100.0f, 200.0f);
	DragView.PointerViewportPosition = FVector2D(100.0f, 200.0f);
	DragView.bHasPointerViewportPosition = true;

	FWacomPlayerControllerRunInteractionTestAccess::ApplyRunMenuDropProbeFeedback(PC.Get(), PaidId, DragView, /*bReleased*/ true);

	FCardInstance Found;
	EZoneKind FoundZone = EZoneKind::Backpack;
	FGuid FoundOwner;
	TestFalse(TEXT("Paid instance is removed from owned zones"),
		Run->FindInstance(PaidId, Found, FoundZone, FoundOwner));
	TestEqual(TEXT("Drop target shows submitted preview"),
		Target->GetRunMenuDropPreviewState(),
		EWacomRunMenuDropTargetPreviewState::Submitted);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunMenuCardDropMenuHandledDoesNotDefaultDestroySpec,
	"Wacom.UI.RunMenuCardDropIntent.ReleaseWithMenuHandledPolicyUsesMenuSubmitResult",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunMenuCardDropMenuHandledDoesNotDefaultDestroySpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("Poison Fang"), 0);
	UCharacterDefinition* Character = WacomRunFirstPersonCardLayerSpec::MakePaymentTestCharacter(Fx, Fang);
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(PC.Get(), Run.Get());
	WacomRunFirstPersonCardLayerSpec::AttachFirstPersonPawnForTest(PC.Get());
	PC->SetRunFirstPersonCardLayerActive(true);
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> ActiveMenu(NewObject<UWacomMenuWidgetBaseProbe>(PC.Get()));
	ActiveMenu->SetOwningWacomPlayerControllerForTest(PC.Get());
	ActiveMenu->bAcceptRunMenuFirstPersonCardDropForTest = true;
	ActiveMenu->SubmitPolicyForTest =
		EWacomRunMenuCardDropSubmitPolicy::MenuHandled;
	ActiveMenu->AcceptedZoneIdForTest = TEXT("RunEvent.Pay.Fang");
	TStrongObjectPtr<UWacomRunMenuDropTargetWidgetProbe> Target(
		NewObject<UWacomRunMenuDropTargetWidgetProbe>(PC.Get()));
	Target->ZoneId = TEXT("RunEvent.Pay.Fang");
	FWacomRunMenuDropTargetWidgetTestAccess::SetProbeHit(Target.Get(), true);
	FWacomPlayerControllerRunInteractionTestAccess::RegisterRunMenuDropTarget(PC.Get(), Target.Get());

	FWacomRunMenuCardLeaseRequest Request = WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("RunEventLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Lease is set"),
		ActiveMenu->SetOwnedRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, LeaseResult));
	PC->RegisterActiveGameMenuWidget(ActiveMenu.Get());

	const FGuid PaidId =
		WacomRunFirstPersonCardLayerSpec::FindOwnedInstanceIdByDefinition(Run->GetRunState(), Fang);
	TestTrue(TEXT("Fang instance found"), PaidId.IsValid());
	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId = PaidId;
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.CurrentScreenPosition = FVector2D(100.0f, 200.0f);
	DragView.PointerViewportPosition = FVector2D(100.0f, 200.0f);
	DragView.bHasPointerViewportPosition = true;

	FWacomRunMenuCardDropResolveResult Result =
		FWacomPlayerControllerRunInteractionTestAccess::ResolveRunMenuCardDropIntent(PC.Get(), PaidId, DragView);
	TestEqual(TEXT("Menu-handled drop resolves submit intent"),
		Result.IntentKind,
		EWacomRunMenuCardDropIntentKind::SubmitZoneTarget);
	TestEqual(TEXT("Submit policy is menu handled"),
		Result.SubmitPolicy,
		EWacomRunMenuCardDropSubmitPolicy::MenuHandled);

	FWacomPlayerControllerRunInteractionTestAccess::ApplyRunMenuDropProbeFeedback(PC.Get(), PaidId, DragView, /*bReleased*/ true);

	FCardInstance Found;
	EZoneKind FoundZone = EZoneKind::Backpack;
	FGuid FoundOwner;
	TestTrue(TEXT("Default destroy path does not remove card"),
		Run->FindInstance(PaidId, Found, FoundZone, FoundOwner));
	TestTrue(TEXT("Menu submit result is recorded"),
		ActiveMenu->LastDropResultForTest.bSubmitted);
	TestEqual(TEXT("Menu receives menu-handled submit result"),
		ActiveMenu->LastDropResultForTest.SubmitPolicy,
		EWacomRunMenuCardDropSubmitPolicy::MenuHandled);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunMenuCardDropMenuHandledFailureSpec,
	"Wacom.UI.RunMenuCardDropIntent.MenuHandledSubmitFailureShowsRejectWithoutDefaultDestroy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunMenuCardDropMenuHandledFailureSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("Poison Fang"), 0);
	UCharacterDefinition* Character = WacomRunFirstPersonCardLayerSpec::MakePaymentTestCharacter(Fx, Fang);
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(PC.Get(), Run.Get());
	WacomRunFirstPersonCardLayerSpec::AttachFirstPersonPawnForTest(PC.Get());
	PC->SetRunFirstPersonCardLayerActive(true);
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> ActiveMenu(NewObject<UWacomMenuWidgetBaseProbe>(PC.Get()));
	ActiveMenu->SetOwningWacomPlayerControllerForTest(PC.Get());
	ActiveMenu->bAcceptRunMenuFirstPersonCardDropForTest = true;
	ActiveMenu->SubmitPolicyForTest =
		EWacomRunMenuCardDropSubmitPolicy::MenuHandled;
	ActiveMenu->bMenuSubmitSucceedsForTest = false;
	ActiveMenu->AcceptedZoneIdForTest = TEXT("RunEvent.Pay.Fang");
	TStrongObjectPtr<UWacomRunMenuDropTargetWidgetProbe> Target(
		NewObject<UWacomRunMenuDropTargetWidgetProbe>(PC.Get()));
	Target->ZoneId = TEXT("RunEvent.Pay.Fang");
	FWacomRunMenuDropTargetWidgetTestAccess::SetProbeHit(Target.Get(), true);
	FWacomPlayerControllerRunInteractionTestAccess::RegisterRunMenuDropTarget(PC.Get(), Target.Get());

	FWacomRunMenuCardLeaseRequest Request = WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("RunEventLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Lease is set"),
		ActiveMenu->SetOwnedRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, LeaseResult));
	PC->RegisterActiveGameMenuWidget(ActiveMenu.Get());

	const FGuid PaidId =
		WacomRunFirstPersonCardLayerSpec::FindOwnedInstanceIdByDefinition(Run->GetRunState(), Fang);
	TestTrue(TEXT("Fang instance found"), PaidId.IsValid());
	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId = PaidId;
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.CurrentScreenPosition = FVector2D(100.0f, 200.0f);
	DragView.PointerViewportPosition = FVector2D(100.0f, 200.0f);
	DragView.bHasPointerViewportPosition = true;

	FWacomPlayerControllerRunInteractionTestAccess::ApplyRunMenuDropProbeFeedback(PC.Get(), PaidId, DragView, /*bReleased*/ true);

	FCardInstance Found;
	EZoneKind FoundZone = EZoneKind::Backpack;
	FGuid FoundOwner;
	TestTrue(TEXT("Failed menu submit does not default destroy card"),
		Run->FindInstance(PaidId, Found, FoundZone, FoundOwner));
	TestFalse(TEXT("Menu submit result is failed"),
		ActiveMenu->LastDropResultForTest.bSubmitted);
	TestEqual(TEXT("Menu submit failure becomes reject"),
		ActiveMenu->LastDropResultForTest.RejectReason,
		EWacomRunMenuCardDropRejectReason::SubmitFailed);
	TestEqual(TEXT("Drop target shows invalid preview"),
		Target->GetRunMenuDropPreviewState(),
		EWacomRunMenuDropTargetPreviewState::Invalid);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonEconomyRevisionSkipsDefaultSnapshotSpec,
	"Wacom.UI.RunFirstPersonCardLayer.RunStateChangedWithEconomyOnlyRevisionSkipsDefaultSourceSnapshotBuild",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonEconomyRevisionSkipsDefaultSnapshotSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Card = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.EconomySkip"), TEXT("Economy Skip Card"), 1);
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
	const uint64 StorageRevisionAfterActivate = Run->GetBackpackStorageSnapshotRevision();
#if WITH_AUTOMATION_TESTS
	FWacomFirstPersonCardLayerTestAccess::ResetSourceCounters(*Source);
#endif

	Run->AddGold(1);
	TestEqual(TEXT("Gold-only change does not bump storage revision"),
		Run->GetBackpackStorageSnapshotRevision(),
		StorageRevisionAfterActivate);
	TestEqual(TEXT("Economy-only notification skips default source write"),
		Source->WriteCount,
		WritesAfterActivate);
	TestEqual(TEXT("Economy-only notification reports revision skip"),
		Source->GetRunFirstPersonCardSourceDebugView().LastRefreshResult,
		FName(TEXT("SkippedUnchangedRevision")));
#if WITH_AUTOMATION_TESTS
	TestEqual(TEXT("Economy-only skip increments skip count"),
		FWacomFirstPersonCardLayerTestAccess::DefaultSourceCounters(*Source).RevisionSkipCount,
		1);
	TestEqual(TEXT("Economy-only skip does not rebuild backpack snapshot"),
		FWacomFirstPersonCardLayerTestAccess::DefaultSourceCounters(*Source).DataBuildCount,
		0);
#endif

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonDefaultSourceRefreshKeyBreaksSkipSpec,
	"Wacom.UI.RunFirstPersonCardLayer.RunFirstPersonDefaultSourceRefreshKeyBreaksSkipWhenSourceIdOrProjectedFlagChanges",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonDefaultSourceRefreshKeyBreaksSkipSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Card = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.DefaultKey"), TEXT("Default Key Card"), 1);
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
#if WITH_AUTOMATION_TESTS
	FWacomFirstPersonCardLayerTestAccess::ResetSourceCounters(*Source);
#endif

	Source->RunFirstPersonCardLayerSourceId = TEXT("RunFirstPersonChangedSource");
	Run->OnRunStateChangedNative.Broadcast();
	TestEqual(TEXT("Source id change refreshes despite unchanged storage revision"),
		Source->WriteCount,
		WritesAfterActivate + 1);
	TestEqual(TEXT("Changed source id is written"),
		Source->LastWrittenSourceId,
		FName(TEXT("RunFirstPersonChangedSource")));
#if WITH_AUTOMATION_TESTS
	TestEqual(TEXT("Source id change rebuilds default data"),
		FWacomFirstPersonCardLayerTestAccess::DefaultSourceCounters(*Source).DataBuildCount,
		1);
	TestEqual(TEXT("Source id change applies default source"),
		FWacomFirstPersonCardLayerTestAccess::DefaultSourceCounters(*Source).RuntimeApplyCount,
		1);
	TestEqual(TEXT("Source id change does not count as skip"),
		FWacomFirstPersonCardLayerTestAccess::DefaultSourceCounters(*Source).RevisionSkipCount,
		0);

	FWacomFirstPersonCardLayerTestAccess::ResetSourceCounters(*Source);
#endif

	const int32 WritesAfterSourceChange = Source->WriteCount;
	Source->bIncludeProjectedRunBattleDeckCards =
		!Source->bIncludeProjectedRunBattleDeckCards;
	Run->OnRunStateChangedNative.Broadcast();
	TestEqual(TEXT("Projected include flag change refreshes despite unchanged storage revision"),
		Source->WriteCount,
		WritesAfterSourceChange + 1);
#if WITH_AUTOMATION_TESTS
	TestEqual(TEXT("Projected include flag change rebuilds default data"),
		FWacomFirstPersonCardLayerTestAccess::DefaultSourceCounters(*Source).DataBuildCount,
		1);
	TestEqual(TEXT("Projected include flag change applies default source"),
		FWacomFirstPersonCardLayerTestAccess::DefaultSourceCounters(*Source).RuntimeApplyCount,
		1);
	TestEqual(TEXT("Projected include flag change does not count as skip"),
		FWacomFirstPersonCardLayerTestAccess::DefaultSourceCounters(*Source).RevisionSkipCount,
		0);
#endif

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonProviderLeaseUnchangedRevisionSkipsSpec,
	"Wacom.UI.RunFirstPersonCardLayer.ProviderBackedLeaseRunStateChangedWithUnchangedStorageRevisionSkipsRebuildAndRewrite",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonProviderLeaseUnchangedRevisionSkipsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("Poison Fang"), 0);
	UCharacterDefinition* Character = WacomRunFirstPersonCardLayerSpec::MakePaymentTestCharacter(Fx, Fang);

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

	FWacomRunMenuCardLeaseRequest Request =
		WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("ProviderSkipLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Provider lease is set"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, LeaseResult));
	const int32 WritesAfterLease = Source->WriteCount;
#if WITH_AUTOMATION_TESTS
	FWacomFirstPersonCardLayerTestAccess::ResetSourceCounters(*Source);
#endif

	Run->OnRunStateChangedNative.Broadcast();
	TestEqual(TEXT("Unchanged storage revision skips provider source rewrite"),
		Source->WriteCount,
		WritesAfterLease);
	TestEqual(TEXT("Provider skip is reported"),
		Source->GetRunFirstPersonCardSourceDebugView().LastRefreshResult,
		FName(TEXT("MenuLeaseProviderSkippedUnchangedRevision")));
#if WITH_AUTOMATION_TESTS
	TestEqual(TEXT("Provider skip count increments"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).RevisionSkipCount,
		1);
	TestEqual(TEXT("Provider candidate rebuild is skipped"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).DataBuildCount,
		0);
	TestEqual(TEXT("Provider runtime apply is skipped"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).RuntimeApplyCount,
		0);
#endif
	TestEqual(TEXT("Candidate count is preserved"),
		Source->GetRunFirstPersonCardSourceDebugView().ActiveMenuLeaseEntryCount,
		1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonProviderLeaseRefreshKeyBreaksSkipSpec,
	"Wacom.UI.RunFirstPersonCardLayer.ProviderBackedLeaseRefreshKeyBreaksSkipWhenProviderRequestChanges",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonProviderLeaseRefreshKeyBreaksSkipSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("Poison Fang"), 0);
	UCardDefinition* Other = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Other"), TEXT("Other"), 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Fang, Other });

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

	FWacomRunMenuCardLeaseRequest Request =
		WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("ProviderRequestKeyLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Provider lease is set"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, LeaseResult));
	const int32 WritesAfterLease = Source->WriteCount;
#if WITH_AUTOMATION_TESTS
	FWacomFirstPersonCardLayerTestAccess::ResetSourceCounters(*Source);

	FWacomRunMenuCardLeaseRequest ChangedRequest = Request;
	ChangedRequest.AllowedCardIds.Reset();
	ChangedRequest.AllowedCardIds.Add(TEXT("Other"));
	FWacomFirstPersonCardLayerTestAccess::SetActiveProviderLeaseRequest(*Source, ChangedRequest);
#endif

	Run->OnRunStateChangedNative.Broadcast();
	TestEqual(TEXT("Provider request change refreshes despite unchanged storage revision"),
		Source->WriteCount,
		WritesAfterLease + 1);
	TestEqual(TEXT("Changed provider request reports other card"),
		Source->LastWrittenEntries[0].CardViewData.Name.ToString(),
		FString(TEXT("Other")));
#if WITH_AUTOMATION_TESTS
	TestEqual(TEXT("Provider request change rebuilds candidates"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).DataBuildCount,
		1);
	TestEqual(TEXT("Provider request change applies runtime source"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).RuntimeApplyCount,
		1);
	TestEqual(TEXT("Provider request change does not count as skip"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).RevisionSkipCount,
		0);
#endif

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonProviderLeaseEconomyRevisionSkipsSpec,
	"Wacom.UI.RunFirstPersonCardLayer.ProviderBackedLeaseEconomyOnlyRevisionSkipsCandidateRebuild",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonProviderLeaseEconomyRevisionSkipsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("Poison Fang"), 0);
	UCharacterDefinition* Character = WacomRunFirstPersonCardLayerSpec::MakePaymentTestCharacter(Fx, Fang);

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

	FWacomRunMenuCardLeaseRequest Request =
		WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("ProviderEconomySkipLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Provider lease is set"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, LeaseResult));
	const int32 WritesAfterLease = Source->WriteCount;
	const uint64 StorageRevisionAfterLease = Run->GetBackpackStorageSnapshotRevision();
#if WITH_AUTOMATION_TESTS
	FWacomFirstPersonCardLayerTestAccess::ResetSourceCounters(*Source);
#endif

	Run->AddGold(1);
	TestEqual(TEXT("Economy-only mutation does not bump storage revision"),
		Run->GetBackpackStorageSnapshotRevision(),
		StorageRevisionAfterLease);
	TestEqual(TEXT("Economy-only notification skips provider rewrite"),
		Source->WriteCount,
		WritesAfterLease);
#if WITH_AUTOMATION_TESTS
	TestEqual(TEXT("Economy-only provider skip increments skip count"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).RevisionSkipCount,
		1);
	TestEqual(TEXT("Economy-only provider skip avoids rebuild"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).DataBuildCount,
		0);
#endif

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonProviderLeaseStorageRevisionRefreshesSpec,
	"Wacom.UI.RunFirstPersonCardLayer.ProviderBackedLeaseStorageRevisionRefreshesCandidates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonProviderLeaseStorageRevisionRefreshesSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("Poison Fang"), 0);
	UCharacterDefinition* Character = WacomRunFirstPersonCardLayerSpec::MakePaymentTestCharacter(Fx, Fang);

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

	FWacomRunMenuCardLeaseRequest Request =
		WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("ProviderStorageRefreshLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Provider lease is set"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, LeaseResult));
	const int32 WritesAfterLease = Source->WriteCount;
	const uint64 StorageRevisionAfterLease = Run->GetBackpackStorageSnapshotRevision();
#if WITH_AUTOMATION_TESTS
	FWacomFirstPersonCardLayerTestAccess::ResetSourceCounters(*Source);
#endif

	Run->AcquireCardToRun(Fang);
	TestTrue(TEXT("Storage mutation bumps storage revision"),
		Run->GetBackpackStorageSnapshotRevision() > StorageRevisionAfterLease);
	TestEqual(TEXT("Provider source rewrites after storage revision change"),
		Source->WriteCount,
		WritesAfterLease + 1);
	TestEqual(TEXT("Provider candidate count refreshes"),
		Source->GetRunFirstPersonCardSourceDebugView().ActiveMenuLeaseEntryCount,
		2);
#if WITH_AUTOMATION_TESTS
	TestEqual(TEXT("Provider rebuild happens once"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).DataBuildCount,
		1);
	TestEqual(TEXT("Provider apply happens once"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).RuntimeApplyCount,
		1);
	TestEqual(TEXT("Storage revision refresh does not count as skip"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).RevisionSkipCount,
		0);
#endif

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonProviderLeaseStorageRevisionCanClearSpec,
	"Wacom.UI.RunFirstPersonCardLayer.ProviderBackedLeaseStorageRevisionCanClearWhenNoCandidatesRemain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonProviderLeaseStorageRevisionCanClearSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("Poison Fang"), 0);
	UCharacterDefinition* Character = WacomRunFirstPersonCardLayerSpec::MakePaymentTestCharacter(Fx, Fang);

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

	FWacomRunMenuCardLeaseRequest Request =
		WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("ProviderClearLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	const FGuid PaidId =
		WacomRunFirstPersonCardLayerSpec::FindOwnedInstanceIdByDefinition(Run->GetRunState(), Fang);
	TestTrue(TEXT("Fang instance exists"), PaidId.IsValid());
	Request.ExplicitCardInstanceIds.Add(PaidId);
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Provider lease is set"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, LeaseResult));
#if WITH_AUTOMATION_TESTS
	FWacomFirstPersonCardLayerTestAccess::ResetSourceCounters(*Source);
#endif

	TestTrue(TEXT("Destroying the only candidate succeeds"),
		Run->DestroyCardByInstance(PaidId));
	TestFalse(TEXT("Provider lease clears when no candidates remain"),
		Source->HasActiveMenuLease());
	TestEqual(TEXT("Provider reports no candidates"),
		Source->GetRunFirstPersonCardSourceDebugView().LastMenuLeaseProviderResult,
		FName(TEXT("NoMatchingCandidates")));
#if WITH_AUTOMATION_TESTS
	TestEqual(TEXT("Clear path still rebuilds candidates"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).DataBuildCount,
		1);
	TestEqual(TEXT("Clear path does not skip"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).RevisionSkipCount,
		0);
#endif

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonProviderLeaseRequestOrSessionResetsGateSpec,
	"Wacom.UI.RunFirstPersonCardLayer.ProviderBackedLeaseRequestOrRunSessionSwitchResetsRevisionGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonProviderLeaseRequestOrSessionResetsGateSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("Poison Fang"), 0);
	UCardDefinition* Other = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Other"), TEXT("Other"), 1);
	UCharacterDefinition* FirstCharacter = Fx.MakeCharacter(nullptr, nullptr, { Fang, Other });
	UCharacterDefinition* SecondCharacter = Fx.MakeCharacter(nullptr, nullptr, { Other });

	TStrongObjectPtr<URunSession> FirstRun(NewObject<URunSession>());
	TStrongObjectPtr<URunSession> SecondRun(NewObject<URunSession>());
	TestTrue(TEXT("First run initializes"), FirstRun->Initialize(FirstCharacter));
	TestTrue(TEXT("Second run initializes"), SecondRun->Initialize(SecondCharacter));
	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(FirstRun.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(true);

	FWacomRunMenuCardLeaseRequest FirstRequest =
		WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("ProviderResetLease"));
	FirstRequest.AllowedCardIds.Add(TEXT("PoisonFang"));
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("First provider lease is set"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(FirstRequest, LeaseResult));
	const int32 WritesAfterFirstLease = Source->WriteCount;
#if WITH_AUTOMATION_TESTS
	FWacomFirstPersonCardLayerTestAccess::ResetSourceCounters(*Source);
#endif

	FWacomRunMenuCardLeaseRequest ChangedRequest = FirstRequest;
	ChangedRequest.AllowedCardIds.Reset();
	ChangedRequest.AllowedCardIds.Add(TEXT("Other"));
	TestTrue(TEXT("Changed provider request refreshes same lease"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(ChangedRequest, LeaseResult));
	TestEqual(TEXT("Changed provider request rewrites active lease"),
		Source->WriteCount,
		WritesAfterFirstLease + 1);
	TestEqual(TEXT("Changed provider request reports other card"),
		Source->LastWrittenEntries[0].CardViewData.Name.ToString(),
		FString(TEXT("Other")));

	Source->BindRunSession(SecondRun.Get());
	TestEqual(TEXT("RunSession switch keeps active provider lease"),
		Source->GetRunFirstPersonCardSourceDebugView().ActiveMenuLeaseId,
		FirstRequest.LeaseId);
	TestEqual(TEXT("RunSession switch refreshes provider entries from new run"),
		Source->LastWrittenEntries[0].CardViewData.Name.ToString(),
		FString(TEXT("Other")));
#if WITH_AUTOMATION_TESTS
	FWacomFirstPersonCardLayerTestAccess::ResetSourceCounters(*Source);
#endif

	SecondRun->OnRunStateChangedNative.Broadcast();
	TestEqual(TEXT("New session stored provider key allows later unchanged skip"),
		Source->GetRunFirstPersonCardSourceDebugView().LastRefreshResult,
		FName(TEXT("MenuLeaseProviderSkippedUnchangedRevision")));
#if WITH_AUTOMATION_TESTS
	TestEqual(TEXT("New session skip count increments after key is restablished"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).RevisionSkipCount,
		1);
#endif

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonProviderLeaseManualAndSuppressionBypassSpec,
	"Wacom.UI.RunFirstPersonCardLayer.ManualProviderLeaseSetAndSuppressionReleaseBypassRevisionGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonProviderLeaseManualAndSuppressionBypassSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("Poison Fang"), 0);
	UCharacterDefinition* Character = WacomRunFirstPersonCardLayerSpec::MakePaymentTestCharacter(Fx, Fang);

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

	FWacomRunMenuCardLeaseRequest Request =
		WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("ProviderManualBypassLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Provider lease is set"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, LeaseResult));
	const int32 WritesAfterLease = Source->WriteCount;
#if WITH_AUTOMATION_TESTS
	FWacomFirstPersonCardLayerTestAccess::ResetSourceCounters(*Source);
#endif

	TestTrue(TEXT("Manual refresh succeeds"),
		Source->RefreshRunFirstPersonCardLayer());
	TestEqual(TEXT("Manual refresh rewrites provider source"),
		Source->WriteCount,
		WritesAfterLease + 1);
#if WITH_AUTOMATION_TESTS
	TestEqual(TEXT("Manual refresh rebuilds provider lease"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).DataBuildCount,
		1);
	TestEqual(TEXT("Manual refresh applies provider source"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).RuntimeApplyCount,
		1);
	TestEqual(TEXT("Manual refresh does not count as provider skip"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).RevisionSkipCount,
		0);
#endif

	TestTrue(TEXT("Clearing provider lease succeeds"),
		Source->ClearRunFirstPersonCardLayerMenuLease(Request.LeaseId));
	TestFalse(TEXT("Suppressed state remains active after lease clear"),
		Source->HasActiveMenuLease());
	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(false);
	TestEqual(TEXT("Suppression release restores default source"),
		Source->LastWrittenSourceId,
		Source->RunFirstPersonCardLayerSourceId);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonStorageRevisionRefreshesDefaultSourceSpec,
	"Wacom.UI.RunFirstPersonCardLayer.RunStateChangedWithStorageRevisionRefreshesDefaultSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonStorageRevisionRefreshesDefaultSourceSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Card = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.StorageRefresh"), TEXT("Storage Refresh Card"), 1);
	UCardDefinition* NewCard = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.StorageRefresh.New"), TEXT("Storage Refresh New Card"), 0);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 3);
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
	const uint64 StorageRevisionAfterActivate = Run->GetBackpackStorageSnapshotRevision();
#if WITH_AUTOMATION_TESTS
	FWacomFirstPersonCardLayerTestAccess::ResetSourceCounters(*Source);
#endif

	Run->AcquireCardToRun(NewCard);
	TestTrue(TEXT("Storage mutation bumps storage revision"),
		Run->GetBackpackStorageSnapshotRevision() > StorageRevisionAfterActivate);
	TestEqual(TEXT("Storage revision change refreshes default source"),
		Source->WriteCount,
		WritesAfterActivate + 1);
	TestEqual(TEXT("Storage refresh reports refreshed"),
		Source->GetRunFirstPersonCardSourceDebugView().LastRefreshResult,
		FName(TEXT("Refreshed")));
#if WITH_AUTOMATION_TESTS
	TestEqual(TEXT("Storage refresh rebuilds snapshot once"),
		FWacomFirstPersonCardLayerTestAccess::DefaultSourceCounters(*Source).DataBuildCount,
		1);
	TestEqual(TEXT("Storage refresh applies runtime source once"),
		FWacomFirstPersonCardLayerTestAccess::DefaultSourceCounters(*Source).RuntimeApplyCount,
		1);
	TestEqual(TEXT("Storage refresh does not count as skip"),
		FWacomFirstPersonCardLayerTestAccess::DefaultSourceCounters(*Source).RevisionSkipCount,
		0);
#endif

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonRunSessionSwitchResetsRevisionGateSpec,
	"Wacom.UI.RunFirstPersonCardLayer.RunSessionSwitchResetsRunFirstPersonSourceRevisionGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonRunSessionSwitchResetsRevisionGateSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* FirstCard = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.SessionA"), TEXT("Session A Card"), 1);
	UCardDefinition* SecondCard = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.SessionB"), TEXT("Session B Card"), 2);
	UCardDefinition* FirstPack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCardDefinition* SecondPack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* FirstCharacter = Fx.MakeCharacter(nullptr, nullptr, { FirstCard, FirstPack });
	UCharacterDefinition* SecondCharacter = Fx.MakeCharacter(nullptr, nullptr, { SecondCard, SecondPack });

	TStrongObjectPtr<URunSession> FirstRun(NewObject<URunSession>());
	TStrongObjectPtr<URunSession> SecondRun(NewObject<URunSession>());
	TestTrue(TEXT("First run initializes"), FirstRun->Initialize(FirstCharacter));
	TestTrue(TEXT("Second run initializes"), SecondRun->Initialize(SecondCharacter));

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(FirstRun.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	const int32 WritesAfterFirstRun = Source->WriteCount;

	Source->BindRunSession(SecondRun.Get());
	TestEqual(TEXT("RunSession switch forces a new default source write"),
		Source->WriteCount,
		WritesAfterFirstRun + 1);
	TestEqual(TEXT("Switched run writes one card"),
		Source->LastWrittenEntries.Num(),
		1);
	TestEqual(TEXT("Switched run writes its own card"),
		Source->LastWrittenEntries[0].CardViewData.Name.ToString(),
		FString(TEXT("Session B Card")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonManualAndSuppressionBypassRevisionGateSpec,
	"Wacom.UI.RunFirstPersonCardLayer.ManualRefreshAndSuppressionReleaseBypassRevisionGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonManualAndSuppressionBypassRevisionGateSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Card = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.ManualBypass"), TEXT("Manual Bypass Card"), 1);
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
#if WITH_AUTOMATION_TESTS
	FWacomFirstPersonCardLayerTestAccess::ResetSourceCounters(*Source);
#endif

	TestTrue(TEXT("Manual refresh still succeeds"),
		Source->RefreshRunFirstPersonCardLayer());
	TestEqual(TEXT("Manual refresh bypasses revision skip"),
		Source->WriteCount,
		WritesAfterActivate + 1);
#if WITH_AUTOMATION_TESTS
	TestEqual(TEXT("Manual refresh rebuilds snapshot"),
		FWacomFirstPersonCardLayerTestAccess::DefaultSourceCounters(*Source).DataBuildCount,
		1);
	TestEqual(TEXT("Manual refresh does not count as revision skip"),
		FWacomFirstPersonCardLayerTestAccess::DefaultSourceCounters(*Source).RevisionSkipCount,
		0);
#endif

	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(true);
	const int32 WritesWhileSuppressed = Source->WriteCount;
	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(false);
	TestEqual(TEXT("Suppression release forces default source write"),
		Source->WriteCount,
		WritesWhileSuppressed + 1);
	TestEqual(TEXT("Suppression release reports refreshed"),
		Source->GetRunFirstPersonCardSourceDebugView().LastRefreshResult,
		FName(TEXT("Refreshed")));

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
	UCharacterDefinition* Character = WacomRunFirstPersonCardLayerSpec::MakePaymentTestCharacter(Fx, Fang);
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(PC.Get(), Run.Get());
	WacomRunFirstPersonCardLayerSpec::AttachFirstPersonPawnForTest(PC.Get());
	PC->SetRunFirstPersonCardLayerActive(true);
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> ActiveMenu(NewObject<UWacomMenuWidgetBaseProbe>(PC.Get()));
	ActiveMenu->SetOwningWacomPlayerControllerForTest(PC.Get());
	ActiveMenu->bAcceptRunMenuFirstPersonCardDropForTest = true;
	ActiveMenu->SubmitPolicyForTest =
		EWacomRunMenuCardDropSubmitPolicy::ControllerDestroyOwnedCard;
	ActiveMenu->AcceptedZoneIdForTest = TEXT("RunEvent.Pay.Fang");
	TStrongObjectPtr<UWacomRunMenuDropTargetWidgetProbe> Target(
		NewObject<UWacomRunMenuDropTargetWidgetProbe>(PC.Get()));
	Target->ZoneId = TEXT("RunEvent.Pay.Fang");
	FWacomRunMenuDropTargetWidgetTestAccess::SetProbeHit(Target.Get(), true);
	FWacomPlayerControllerRunInteractionTestAccess::RegisterRunMenuDropTarget(PC.Get(), Target.Get());

	const FGuid PaidId =
		WacomRunFirstPersonCardLayerSpec::FindOwnedInstanceIdByDefinition(Run->GetRunState(), Fang);
	TestTrue(TEXT("Fang instance found"), PaidId.IsValid());
	FWacomRunMenuCardLeaseRequest Request = WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("RunEventLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	Request.ExplicitCardInstanceIds.Add(PaidId);
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Lease is set"),
		ActiveMenu->SetOwnedRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, LeaseResult));
	PC->RegisterActiveGameMenuWidget(ActiveMenu.Get());

	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run);
	WacomRunFirstPersonCardLayerSpec::ResetRunOwnedZones(State);

	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId = PaidId;
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.CurrentScreenPosition = FVector2D(100.0f, 200.0f);
	DragView.PointerViewportPosition = FVector2D(100.0f, 200.0f);
	DragView.bHasPointerViewportPosition = true;

	const FWacomRunMenuCardDropResolveResult Result =
		FWacomPlayerControllerRunInteractionTestAccess::ResolveRunMenuCardDropIntent(PC.Get(), PaidId, DragView);
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
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(PC.Get(), Run.Get());
	WacomRunFirstPersonCardLayerSpec::AttachFirstPersonPawnForTest(PC.Get());
	PC->SetRunFirstPersonCardLayerActive(true);
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> ActiveMenu(NewObject<UWacomMenuWidgetBaseProbe>(PC.Get()));
	ActiveMenu->SetOwningWacomPlayerControllerForTest(PC.Get());
	ActiveMenu->bAcceptRunMenuFirstPersonCardDropForTest = true;
	ActiveMenu->SubmitPolicyForTest =
		EWacomRunMenuCardDropSubmitPolicy::ControllerDestroyOwnedCard;
	ActiveMenu->AcceptedZoneIdForTest = TEXT("RunEvent.Pay.Fang");
	TStrongObjectPtr<UWacomRunMenuDropTargetWidgetProbe> Target(
		NewObject<UWacomRunMenuDropTargetWidgetProbe>(PC.Get()));
	Target->ZoneId = TEXT("RunEvent.Pay.Fang");
	FWacomRunMenuDropTargetWidgetTestAccess::SetProbeHit(Target.Get(), true);
	FWacomPlayerControllerRunInteractionTestAccess::RegisterRunMenuDropTarget(PC.Get(), Target.Get());

	FWacomRunMenuCardLeaseRequest Request = WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("RunEventLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	Request.ExplicitCardInstanceIds.Add(State.Backpack[0].InstanceId);
	Request.ExplicitCardInstanceIds.Add(State.Backpack[1].InstanceId);
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
	FWacomPlayerControllerRunInteractionTestAccess::ApplyRunMenuDropProbeFeedback(PC.Get(), PaidId, DragView, /*bReleased*/ true);

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
	UCharacterDefinition* Character = WacomRunFirstPersonCardLayerSpec::MakePaymentTestCharacter(Fx, Fang);
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(PC.Get(), Run.Get());
	WacomRunFirstPersonCardLayerSpec::AttachFirstPersonPawnForTest(PC.Get());
	PC->SetRunFirstPersonCardLayerActive(true);
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> ActiveMenu(NewObject<UWacomMenuWidgetBaseProbe>(PC.Get()));
	ActiveMenu->SetOwningWacomPlayerControllerForTest(PC.Get());
	ActiveMenu->bAcceptRunMenuFirstPersonCardDropForTest = true;
	ActiveMenu->SubmitPolicyForTest =
		EWacomRunMenuCardDropSubmitPolicy::ControllerDestroyOwnedCard;
	ActiveMenu->AcceptedZoneIdForTest = TEXT("RunEvent.Pay.Fang");
	TStrongObjectPtr<UWacomRunMenuDropTargetWidgetProbe> Target(
		NewObject<UWacomRunMenuDropTargetWidgetProbe>(PC.Get()));
	Target->ZoneId = TEXT("RunEvent.Pay.Fang");
	FWacomRunMenuDropTargetWidgetTestAccess::SetProbeHit(Target.Get(), true);
	FWacomPlayerControllerRunInteractionTestAccess::RegisterRunMenuDropTarget(PC.Get(), Target.Get());

	FWacomRunMenuCardLeaseRequest Request = WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("RunEventLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	const FGuid PaidId =
		WacomRunFirstPersonCardLayerSpec::FindOwnedInstanceIdByDefinition(Run->GetRunState(), Fang);
	TestTrue(TEXT("Fang instance found"), PaidId.IsValid());
	Request.ExplicitCardInstanceIds.Add(PaidId);
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Lease is set"),
		ActiveMenu->SetOwnedRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, LeaseResult));
	PC->RegisterActiveGameMenuWidget(ActiveMenu.Get());

	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId = PaidId;
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.CurrentScreenPosition = FVector2D(100.0f, 200.0f);
	DragView.PointerViewportPosition = FVector2D(100.0f, 200.0f);
	DragView.bHasPointerViewportPosition = true;
	FWacomPlayerControllerRunInteractionTestAccess::ApplyRunMenuDropProbeFeedback(PC.Get(), PaidId, DragView, /*bReleased*/ true);

	UWacomRunFirstPersonCardSourceComponent* Source = PC->GetRunFirstPersonCardSourceComponent();
	TestTrue(TEXT("Source exists"), Source != nullptr);
	TestFalse(TEXT("Provider-backed lease clears after last candidate"),
		Source->HasActiveMenuLease());
	TestEqual(TEXT("Provider reports no candidates"),
		Source->GetRunFirstPersonCardSourceDebugView().LastMenuLeaseProviderResult,
		FName(TEXT("NoMatchingCandidates")));

	return true;
}
