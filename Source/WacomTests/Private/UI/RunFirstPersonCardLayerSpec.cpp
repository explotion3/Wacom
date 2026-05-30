// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Components/WacomRunFirstPersonCardSourceComponent.h"
#include "Fixtures/BattleTestFixtures.h"
#include "RunSession.h"
#include "RunState.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/RunFirstPersonCardLayerSpecReceiver.h"
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
