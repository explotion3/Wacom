// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceSceneBuilder.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceStateSubsystem.h"
#include "Cards/CardDefinition.h"
#include "UI/Backpack/WacomBackpackWorkspaceStyle.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
FRunStorageCardView MakeWorkspaceSceneCard(
	UCardDefinition* Definition,
	FGuid InstanceId,
	EZoneKind PhysicalZone,
	FGuid OwnerInstanceId = FGuid())
{
	FRunStorageCardView Card;
	Card.Instance.InstanceId = InstanceId;
	Card.Instance.Definition = Definition;
	Card.PhysicalZone = PhysicalZone;
	Card.ZoneOwnerInstanceId = OwnerInstanceId;
	return Card;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceSceneBuilderCountSpec,
	"Wacom.UI.Backpack.Workspace.SceneBuilder.CountAndAlignment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceSceneBuilderCountSpec::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UCardDefinition> Definition(NewObject<UCardDefinition>());
	Definition->CardId = TEXT("Backpack.SceneBuilder.Count");
	const UWacomBackpackWorkspaceStyle* Style = GetDefault<UWacomBackpackWorkspaceStyle>();
	for (const int32 CardCount : TArray<int32>{ 0, 1, 2, 3, 5, 7, 15, 21 })
	{
		FRunBackpackStorageSnapshot Snapshot;
		for (int32 Index = 0; Index < CardCount; ++Index)
		{
			Snapshot.Flux.ContentCards.Add(MakeWorkspaceSceneCard(
				Definition.Get(), FGuid(Index + 1, 101, 102, 103), EZoneKind::Backpack));
		}
		FWacomBackpackWorkspaceStateStore State;
		const FWacomBackpackWorkspaceScene Scene =
			FWacomBackpackWorkspaceSceneBuilder::Build(
				Snapshot,
				State,
				FWacomBackpackWorkspaceCarryProjection(),
				*Style,
				FVector2D(1600.0f, 900.0f));
		TestEqual(
			*FString::Printf(TEXT("%d cards produce aligned scene entries"), CardCount),
			Scene.Cards.Num(), CardCount);
		TestEqual(
			*FString::Printf(TEXT("%d cards produce aligned layouts"), CardCount),
			Scene.CardLayouts.Num(), Scene.Cards.Num());
		TestEqual(
			*FString::Printf(TEXT("%d cards preserve the flux count"), CardCount),
			Scene.FluxCardCount, CardCount);
		TestEqual(
			*FString::Printf(TEXT("%d cards keep one BattleDeck pile"), CardCount),
			Scene.Piles.Num(), 1);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceSceneBuilderIdentitySpec,
	"Wacom.UI.Backpack.Workspace.SceneBuilder.IdentityAndReadOnlyOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceSceneBuilderIdentitySpec::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UCardDefinition> Definition(NewObject<UCardDefinition>());
	Definition->CardId = TEXT("Backpack.SceneBuilder.Identity");
	Definition->DisplayName = FText::FromString(TEXT("场景身份卡"));
	const FGuid OwnerId(41, 42, 43, 44);
	FRunBackpackStorageSnapshot Snapshot;
	Snapshot.BattleDeckPhysicalCards.Add(MakeWorkspaceSceneCard(
		Definition.Get(), FGuid(1, 2, 3, 4), EZoneKind::BattleDeck));
	Snapshot.BattleDeckProjectedCards.Add(MakeWorkspaceSceneCard(
		Definition.Get(), FGuid(5, 6, 7, 8), EZoneKind::SpecialZone, OwnerId));
	FRunSpecialStorageView Special;
	Special.OwnerCard = MakeWorkspaceSceneCard(
		Definition.Get(), OwnerId, EZoneKind::Backpack);
	Special.ContentCards.Add(MakeWorkspaceSceneCard(
		Definition.Get(), FGuid(9, 10, 11, 12), EZoneKind::SpecialZone, OwnerId));
	Snapshot.SpecialZones.Add(Special);
	Snapshot.BurdenCards.Add(MakeWorkspaceSceneCard(
		Definition.Get(), FGuid(13, 14, 15, 16), EZoneKind::BurdenZone));

	FWacomBackpackWorkspaceStateStore State;
	State.SetExpandedPile(FWacomBackpackZoneKey::Make(EZoneKind::BurdenZone));
	const FWacomBackpackWorkspaceScene Scene = FWacomBackpackWorkspaceSceneBuilder::Build(
		Snapshot,
		State,
		FWacomBackpackWorkspaceCarryProjection(),
		*GetDefault<UWacomBackpackWorkspaceStyle>(),
		FVector2D(1920.0f, 1080.0f));
	if (!TestEqual(TEXT("Scene contains physical, projection, owner, content and burden"),
		Scene.Cards.Num(), 5))
	{
		return false;
	}
	TestEqual(TEXT("Battle physical card is first"),
		Scene.Cards[0].Role, EWacomBackpackDeckCardListReuseRole::PhysicalList);
	TestEqual(TEXT("Projection follows its physical Battle card"),
		Scene.Cards[1].ReadOnlyKind,
		EWacomBackpackWorkspaceCardReadOnlyKind::BattleProjection);
	TestFalse(TEXT("Projection is never workspace interactive"),
		Scene.Cards[1].bWorkspaceInteractive);
	TestEqual(TEXT("Special owner precedes Special content"),
		Scene.Cards[2].Role, EWacomBackpackDeckCardListReuseRole::SpecialOwner);
	TestEqual(TEXT("Special content preserves the physical role"),
		Scene.Cards[3].Role, EWacomBackpackDeckCardListReuseRole::SpecialContent);
	TestEqual(TEXT("Burden is a normal physical workspace card"),
		Scene.Cards[4].ReadOnlyKind,
		EWacomBackpackWorkspaceCardReadOnlyKind::None);
	TestTrue(TEXT("Expanded Burden card is workspace interactive"),
		Scene.Cards[4].bWorkspaceInteractive);
	const FWacomBackpackWorkspaceScenePileEntry* BurdenPile = Scene.Piles.FindByPredicate(
		[](const FWacomBackpackWorkspaceScenePileEntry& Pile)
		{
			return Pile.View.Zone == EZoneKind::BurdenZone;
		});
	TestNotNull(TEXT("Scene contains the Burden warning pile"), BurdenPile);
	if (BurdenPile)
	{
		TestTrue(TEXT("Burden warning pile is movable"), BurdenPile->View.bMovable);
		TestTrue(TEXT("Burden warning styling remains enabled"), BurdenPile->View.bWarning);
		TestFalse(TEXT("Burden rejects external player drops"),
			BurdenPile->View.bAcceptsExternalCardDrop);
	}
	TestTrue(TEXT("Indexed projection source produces a badge"),
		!Scene.Cards[1].ProjectedBadgeText.IsEmpty());
	return true;
}

#endif
