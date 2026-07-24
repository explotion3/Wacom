// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceSceneBuilder.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceStateSubsystem.h"
#include "Cards/CardDefinition.h"
#include "UI/Backpack/WacomBackpackWorkspaceStyle.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
FRunStorageCardView MakeBurdenCard(
	UCardDefinition* Definition,
	const FGuid& InstanceId)
{
	FRunStorageCardView Card;
	Card.Instance.InstanceId = InstanceId;
	Card.Instance.Definition = Definition;
	Card.PhysicalZone = EZoneKind::BurdenZone;
	return Card;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceBurdenPhysicalInteractionSpec,
	"Wacom.UI.Backpack.Workspace.BurdenInteraction.PhysicalCardsAndMovablePile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceBurdenPhysicalInteractionSpec::RunTest(
	const FString& Parameters)
{
	TStrongObjectPtr<UCardDefinition> Definition(NewObject<UCardDefinition>());
	Definition->CardId = TEXT("Backpack.Burden.Physical");
	const FGuid FirstId(101, 102, 103, 104);
	const FGuid SecondId(105, 106, 107, 108);

	FRunBackpackStorageSnapshot Snapshot;
	Snapshot.BurdenCards = {
		MakeBurdenCard(Definition.Get(), FirstId),
		MakeBurdenCard(Definition.Get(), SecondId)
	};
	FWacomBackpackWorkspaceStateStore Store;
	const FWacomBackpackZoneKey BurdenZone =
		FWacomBackpackZoneKey::Make(EZoneKind::BurdenZone);
	Store.SetExpandedPile(BurdenZone);

	const FWacomBackpackWorkspaceScene Scene =
		FWacomBackpackWorkspaceSceneBuilder::Build(
			Snapshot,
			Store,
			FWacomBackpackWorkspaceCarryProjection(),
			*GetDefault<UWacomBackpackWorkspaceStyle>(),
			FVector2D(1600.0f, 900.0f));

	const FWacomBackpackWorkspaceScenePileEntry* BurdenPile =
		Scene.Piles.FindByPredicate(
			[](const FWacomBackpackWorkspaceScenePileEntry& Pile)
			{
				return Pile.Zone.Zone == EZoneKind::BurdenZone;
			});
	if (!TestNotNull(TEXT("Burden has a real pile scene entry"), BurdenPile))
	{
		return false;
	}
	TestTrue(TEXT("Burden keeps warning presentation"), BurdenPile->View.bWarning);
	TestTrue(TEXT("Burden pile can use the normal title-drag flow"), BurdenPile->View.bMovable);
	TestFalse(TEXT("Burden is not a player deposit target"),
		BurdenPile->View.bAcceptsExternalCardDrop);

	TArray<FWacomBackpackWorkspaceCardHitRecord> HitRecords;
	for (int32 Index = 0; Index < Scene.Cards.Num(); ++Index)
	{
		const FWacomBackpackWorkspaceSceneCardEntry& Card = Scene.Cards[Index];
		TestEqual(TEXT("Burden entity has no read-only identity"),
			Card.ReadOnlyKind, EWacomBackpackWorkspaceCardReadOnlyKind::None);
		TestTrue(TEXT("Expanded Burden entity is interactive"), Card.bWorkspaceInteractive);
		HitRecords.Emplace(
			Card.CardView.Instance.InstanceId,
			BurdenZone,
			Scene.CardLayouts[Index].CardCenter,
			Scene.CardLayouts[Index].LayerRank,
			Card.bWorkspaceInteractive);
	}

	FWacomBackpackWorkspaceInteractionModel Interaction;
	Interaction.ReconcileCards(BurdenZone, HitRecords);
	Interaction.SelectAllMovable(BurdenZone);
	TestEqual(TEXT("Ctrl+A selects every movable Burden entity"),
		Interaction.GetSelection().OrderedSelectedInstanceIds,
		TArray<FGuid>({ FirstId, SecondId }));
	TestTrue(TEXT("Selecting a Burden entity can begin multi-card Carry"),
		Interaction.BeginCarry(FirstId, FVector2D(600.0f, 400.0f), 77));
	TestEqual(TEXT("Carry preserves the Burden source identity"),
		Interaction.GetCarry().SourceZone.Zone, EZoneKind::BurdenZone);
	TestEqual(TEXT("Carry contains every selected Burden entity"),
		Interaction.GetCarry().RemainingInstanceIds,
		TArray<FGuid>({ FirstId, SecondId }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceBurdenPileLayoutLifetimeSpec,
	"Wacom.UI.Backpack.Workspace.BurdenInteraction.PileLayoutLifetime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceBurdenPileLayoutLifetimeSpec::RunTest(
	const FString& Parameters)
{
	FWacomBackpackWorkspaceStateStore Store;
	const FWacomBackpackZoneKey BurdenZone =
		FWacomBackpackZoneKey::Make(EZoneKind::BurdenZone);
	FWacomBackpackWorkspacePileLayoutEntry Entry;
	Entry.NormalizedPosition = FVector2D(0.31f, 0.67f);
	Entry.LayerRank = 12;
	Entry.bHasManualPlacement = true;
	Store.SetPileLayout(BurdenZone, Entry);
	Store.SetExpandedPile(BurdenZone);

	Store.ReconcilePiles({});
	const FWacomBackpackWorkspacePileLayoutEntry* Preserved =
		Store.FindPileLayout(BurdenZone);
	if (!TestNotNull(TEXT("An empty Burden zone preserves its same-Run pile layout"), Preserved))
	{
		return false;
	}
	TestTrue(TEXT("Burden reappearance restores the exact manual position"),
		Preserved->NormalizedPosition.Equals(Entry.NormalizedPosition));
	TestEqual(TEXT("Burden reappearance restores its layer rank"),
		Preserved->LayerRank, Entry.LayerRank);
	TestFalse(TEXT("An absent Burden pile cannot remain expanded"),
		Store.GetExpandedPile().IsSet());

	Store.ResetPileLayouts();
	TestNull(TEXT("Explicit layout reset clears remembered Burden placement"),
		Store.FindPileLayout(BurdenZone));
	return true;
}

#endif
