// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"

#include "RunSession.h"
#include "RunState.h"
#include "RunStateTypes.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Tags/WacomGameplayTags.h"

#include "UObject/StrongObjectPtr.h"

namespace WacomRunCardWorkspaceSpec
{
	FCardInstance MakeRunCardInstance(UCardDefinition* Definition)
	{
		FCardInstance Instance;
		Instance.InstanceId = FGuid::NewGuid();
		Instance.Definition = Definition;
		return Instance;
	}

	UCardDefinition* MakeNamedNoopCard(
		FWacomBattleFixture& Fx,
		FName CardId)
	{
		UCardDefinition* Card = Fx.MakeNoopCard(0);
		Card->CardId = CardId;
		return Card;
	}

	UCardDefinition* MakeTypeBContainerCard(
		FWacomBattleFixture& Fx,
		FName CardId,
		int32 Capacity)
	{
		UCardDefinition* Card = MakeNamedNoopCard(Fx, CardId);
		Card->Physique.Capacity = Capacity;
		Card->Physique.CapacityEffect =
			WacomTags::Card_CapacityEffect_Placeholder;
		return Card;
	}

	TStrongObjectPtr<URunSession> MakeInitializedRun(
		FWacomBattleFixture& Fx)
	{
		TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
		UCharacterDefinition* Character = Fx.MakeCharacter(
			Fx.MakeNoopCard(1),
			Fx.MakeNoopCard(1),
			{});
		Run->Initialize(Character);
		return Run;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCardWorkspaceDefaultExplorationSpec,
	"Wacom.Run.CardWorkspace.DefaultExplorationIncludesProjectedMetadata",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunCardWorkspaceDefaultExplorationSpec::RunTest(
	const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	TStrongObjectPtr<URunSession> Run =
		WacomRunCardWorkspaceSpec::MakeInitializedRun(Fx);

	UCardDefinition* PhysicalCard =
		WacomRunCardWorkspaceSpec::MakeNamedNoopCard(Fx, TEXT("Workspace.Physical"));
	UCardDefinition* OwnerCard =
		WacomRunCardWorkspaceSpec::MakeTypeBContainerCard(
			Fx,
			TEXT("Workspace.Owner"),
			3);
	UCardDefinition* ProjectedCard =
		WacomRunCardWorkspaceSpec::MakeNamedNoopCard(Fx, TEXT("Workspace.Projected"));

	FCardInstance PhysicalInstance =
		WacomRunCardWorkspaceSpec::MakeRunCardInstance(PhysicalCard);
	FCardInstance OwnerInstance =
		WacomRunCardWorkspaceSpec::MakeRunCardInstance(OwnerCard);
	FCardInstance ProjectedInstance =
		WacomRunCardWorkspaceSpec::MakeRunCardInstance(ProjectedCard);
	ProjectedInstance.bBattleEnabledInSpecialZone = true;

	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run.Get());
	State.Backpack.Reset();
	State.BattleDeck = { PhysicalInstance, OwnerInstance };
	State.BurdenZone.Reset();
	State.SpecialZones.Reset();
	FSpecialZone SpecialZone;
	SpecialZone.OwnerInstanceId = OwnerInstance.InstanceId;
	SpecialZone.Cards.Add(ProjectedInstance);
	State.SpecialZones.Add(MoveTemp(SpecialZone));

	FRunCardWorkspaceRequest Request;
	Request.WorkspaceId = TEXT("ExplorationDefault");
	Request.Kind = ERunCardWorkspaceKind::DefaultExploration;
	Request.bIncludeProjectedBattleDeckCards = true;

	const FRunCardWorkspaceSnapshot Snapshot =
		Run->BuildRunCardWorkspaceSnapshot(Request);
	TestTrue(TEXT("Default exploration workspace succeeds"), Snapshot.bSucceeded);
	TestEqual(TEXT("Physical BattleDeck count"), Snapshot.PhysicalBattleDeckCount, 2);
	TestEqual(TEXT("Projected BattleDeck count"), Snapshot.ProjectedBattleDeckCount, 1);
	TestEqual(TEXT("Entry count"), Snapshot.Entries.Num(), 3);
	if (Snapshot.Entries.Num() != 3)
	{
		return false;
	}

	TestEqual(TEXT("Physical entry keeps BattleDeck zone"),
		Snapshot.Entries[0].PhysicalZone,
		EZoneKind::BattleDeck);
	TestFalse(TEXT("Physical card is not projected"),
		Snapshot.Entries[0].bIsProjectedBattleDeckCard);
	TestEqual(TEXT("Projected entry keeps SpecialZone source"),
		Snapshot.Entries[2].PhysicalZone,
		EZoneKind::SpecialZone);
	TestEqual(TEXT("Projected entry keeps owner id"),
		Snapshot.Entries[2].ZoneOwnerInstanceId,
		OwnerInstance.InstanceId);
	TestTrue(TEXT("Projected entry is marked projected"),
		Snapshot.Entries[2].bIsProjectedBattleDeckCard);
	TestEqual(TEXT("Projected entry id"),
		Snapshot.Entries[2].Instance.InstanceId,
		ProjectedInstance.InstanceId);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCardWorkspaceOwnedCardsFilterSpec,
	"Wacom.Run.CardWorkspace.OwnedCardsFilterScansOwnedZonesWithMetadata",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunCardWorkspaceOwnedCardsFilterSpec::RunTest(
	const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	TStrongObjectPtr<URunSession> Run =
		WacomRunCardWorkspaceSpec::MakeInitializedRun(Fx);

	UCardDefinition* MatchCard =
		WacomRunCardWorkspaceSpec::MakeNamedNoopCard(Fx, TEXT("Workspace.Match"));
	UCardDefinition* OwnerCard =
		WacomRunCardWorkspaceSpec::MakeTypeBContainerCard(
			Fx,
			TEXT("Workspace.FilterOwner"),
			3);

	const FCardInstance BackpackInstance =
		WacomRunCardWorkspaceSpec::MakeRunCardInstance(MatchCard);
	const FCardInstance BattleDeckInstance =
		WacomRunCardWorkspaceSpec::MakeRunCardInstance(MatchCard);
	const FCardInstance BurdenInstance =
		WacomRunCardWorkspaceSpec::MakeRunCardInstance(MatchCard);
	const FCardInstance OwnerInstance =
		WacomRunCardWorkspaceSpec::MakeRunCardInstance(OwnerCard);
	const FCardInstance SpecialInstance =
		WacomRunCardWorkspaceSpec::MakeRunCardInstance(MatchCard);

	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run.Get());
	State.Backpack = { BackpackInstance };
	State.BattleDeck = { BattleDeckInstance, OwnerInstance };
	State.BurdenZone = { BurdenInstance };
	State.SpecialZones.Reset();
	FSpecialZone SpecialZone;
	SpecialZone.OwnerInstanceId = OwnerInstance.InstanceId;
	SpecialZone.Cards.Add(SpecialInstance);
	State.SpecialZones.Add(MoveTemp(SpecialZone));

	FRunCardWorkspaceRequest Request;
	Request.WorkspaceId = TEXT("OwnedFilter");
	Request.Kind = ERunCardWorkspaceKind::OwnedCardsFilter;
	Request.AllowedCardDefinitions.Add(MatchCard);

	const FRunCardWorkspaceSnapshot Snapshot =
		Run->BuildRunCardWorkspaceSnapshot(Request);
	TestTrue(TEXT("Owned filter workspace succeeds"), Snapshot.bSucceeded);
	TestEqual(TEXT("Considered cards include all valid owned zone cards"),
		Snapshot.ConsideredCount,
		5);
	TestEqual(TEXT("Matched entries"), Snapshot.Entries.Num(), 4);
	if (Snapshot.Entries.Num() != 4)
	{
		return false;
	}

	TestEqual(TEXT("Backpack first"), Snapshot.Entries[0].PhysicalZone, EZoneKind::Backpack);
	TestEqual(TEXT("BattleDeck second"), Snapshot.Entries[1].PhysicalZone, EZoneKind::BattleDeck);
	TestEqual(TEXT("Burden third"), Snapshot.Entries[2].PhysicalZone, EZoneKind::BurdenZone);
	TestEqual(TEXT("Special fourth"), Snapshot.Entries[3].PhysicalZone, EZoneKind::SpecialZone);
	TestEqual(TEXT("Special owner metadata"),
		Snapshot.Entries[3].ZoneOwnerInstanceId,
		OwnerInstance.InstanceId);
	TestFalse(TEXT("Owned filter does not project entries"),
		Snapshot.Entries[3].bIsProjectedBattleDeckCard);

	return true;
}
