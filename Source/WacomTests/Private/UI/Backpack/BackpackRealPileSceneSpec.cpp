// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceReconciler.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceStateSubsystem.h"
#include "Cards/CardDefinition.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
FRunStorageCardView MakeSceneCard(
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
	FWacomUIBackpackRealPileSceneIdentitySpec,
	"Wacom.UI.Backpack.Workspace.RealPileSceneIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackRealPileSceneIdentitySpec::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UCardDefinition> Definition(NewObject<UCardDefinition>());
	Definition->CardId = TEXT("Backpack.RealPile.Scene");
	Definition->DisplayName = FText::FromString(TEXT("场景卡"));
	const FGuid FluxId(1, 1, 1, 1);
	const FGuid BattleId(2, 2, 2, 2);
	const FGuid ProjectedId(3, 3, 3, 3);
	const FGuid OwnerId(4, 4, 4, 4);
	const FGuid ContentId(5, 5, 5, 5);
	const FGuid BurdenId(6, 6, 6, 6);

	FRunBackpackStorageSnapshot Snapshot;
	Snapshot.BattleDeckCapacity = 21;
	Snapshot.Flux.ContentCards.Add(MakeSceneCard(
		Definition.Get(), FluxId, EZoneKind::Backpack));
	Snapshot.BattleDeckPhysicalCards.Add(MakeSceneCard(
		Definition.Get(), BattleId, EZoneKind::BattleDeck));
	Snapshot.BattleDeckProjectedCards.Add(MakeSceneCard(
		Definition.Get(), ProjectedId, EZoneKind::SpecialZone, OwnerId));
	FRunSpecialStorageView Special;
	Special.Capacity = 2;
	Special.OwnerCard = MakeSceneCard(
		Definition.Get(), OwnerId, EZoneKind::Backpack);
	Special.ContentCards.Add(MakeSceneCard(
		Definition.Get(), ContentId, EZoneKind::SpecialZone, OwnerId));
	Snapshot.SpecialZones.Add(Special);
	Snapshot.BurdenCards.Add(MakeSceneCard(
		Definition.Get(), BurdenId, EZoneKind::BurdenZone));

	TStrongObjectPtr<UWacomBackpackWorkspaceWidget> Workspace(
		NewObject<UWacomBackpackWorkspaceWidget>());
	Workspace->TakeWidget();
	TSharedPtr<FWacomBackpackWorkspaceInteractionModel> Interaction =
		MakeShared<FWacomBackpackWorkspaceInteractionModel>();
	Workspace->SetInteractionModel(Interaction, nullptr);
	FWacomBackpackWorkspaceStateStore State;
	TArray<TObjectPtr<UWacomDeckCardWidget>> CollapsedCards;
	auto CreateCard = [&Workspace](const FRunStorageCardView&)
	{
		return NewObject<UWacomDeckCardWidget>(Workspace.Get());
	};
	auto RemoveCard = [](UWacomDeckCardWidget*) {};
	FWacomBackpackWorkspaceReconciler::Reconcile(
		*Workspace,
		Snapshot,
		State,
		Interaction.Get(),
		nullptr,
		CreateCard,
		RemoveCard,
		&CollapsedCards);
	Workspace->BindWorkspaceCards(CollapsedCards, 1);

	TestEqual(TEXT("Collapsed scene materializes every real and semantic card"),
		CollapsedCards.Num(), 6);
	if (CollapsedCards.Num() != 6)
	{
		return false;
	}
	TestTrue(TEXT("Flux card remains directly interactive"),
		CollapsedCards[0]->IsWorkspaceInteractionEnabled());
	TestFalse(TEXT("Collapsed Battle physical card is visible but non-interactive"),
		CollapsedCards[1]->IsWorkspaceInteractionEnabled());
	TestEqual(TEXT("Battle projection keeps its read-only role"),
		CollapsedCards[2]->GetWorkspaceReadOnlyKind(),
		EWacomBackpackWorkspaceCardReadOnlyKind::BattleProjection);
	TestEqual(TEXT("Special owner is the first special identity card"),
		CollapsedCards[3]->GetWorkspaceReadOnlyKind(),
		EWacomBackpackWorkspaceCardReadOnlyKind::SpecialOwner);
	TestEqual(TEXT("Special owner displays in its pile without changing physical identity"),
		CollapsedCards[3]->GetWorkspaceDisplayZone(), EZoneKind::SpecialZone);
	TestEqual(TEXT("Special owner retains its physical Backpack identity"),
		CollapsedCards[3]->GetFromZone(), EZoneKind::Backpack);
	TestFalse(TEXT("Collapsed Special content is non-interactive"),
		CollapsedCards[4]->IsWorkspaceInteractionEnabled());
	TestEqual(TEXT("Burden card remains normally opaque but locked"),
		CollapsedCards[5]->GetWorkspaceReadOnlyKind(),
		EWacomBackpackWorkspaceCardReadOnlyKind::BurdenLocked);
	TestFalse(TEXT("Only Battle projections use read-only opacity"),
		CollapsedCards[5]->UsesReadOnlyOpacity());

	State.SetExpandedPile(FWacomBackpackZoneKey::Make(EZoneKind::SpecialZone, OwnerId));
	TArray<TObjectPtr<UWacomDeckCardWidget>> ExpandedCards;
	FWacomBackpackWorkspaceReconciler::Reconcile(
		*Workspace,
		Snapshot,
		State,
		Interaction.Get(),
		nullptr,
		CreateCard,
		RemoveCard,
		&ExpandedCards);
	const int32 ExpansionTransitionCount =
		Workspace->GetAutomationTestView().ActiveBaseCardLayoutTransitionCount;
	TestTrue(TEXT("Expanding a populated pile starts card layout transitions"),
		ExpansionTransitionCount > 0);

	// A stable-geometry refresh can reconcile the same expanded scene before the
	// first transition finishes. Reapplying an identical target must not cancel
	// the in-flight animation and snap every card to its final fan position.
	TArray<TObjectPtr<UWacomDeckCardWidget>> ReconciledExpandedCards;
	FWacomBackpackWorkspaceReconciler::Reconcile(
		*Workspace,
		Snapshot,
		State,
		Interaction.Get(),
		nullptr,
		CreateCard,
		RemoveCard,
		&ReconciledExpandedCards);
	TestEqual(TEXT("An identical expanded reconcile preserves in-flight transitions"),
		Workspace->GetAutomationTestView().ActiveBaseCardLayoutTransitionCount,
		ExpansionTransitionCount);
	ExpandedCards = MoveTemp(ReconciledExpandedCards);
	Workspace->BindWorkspaceCards(ExpandedCards, 1);
	TestEqual(TEXT("Expand reuses the same complete card set"), ExpandedCards.Num(), 6);
	for (int32 Index = 0; Index < CollapsedCards.Num() && ExpandedCards.IsValidIndex(Index); ++Index)
	{
		TestEqual(TEXT("Collapse-to-expand preserves each card widget identity"),
			ExpandedCards[Index].Get(), CollapsedCards[Index].Get());
	}
	TestFalse(TEXT("Expanded Special owner stays read-only"),
		ExpandedCards[3]->IsWorkspaceInteractionEnabled());
	TestTrue(TEXT("Expanded Special content becomes operable"),
		ExpandedCards[4]->IsWorkspaceInteractionEnabled());
	const FWacomBackpackWorkspaceAutomationTestView PreCollapse =
		Workspace->GetAutomationTestView();
	TestTrue(TEXT("Expanded scene publishes content bounds before collapse"),
		PreCollapse.bHasExpandedContentBounds);
	TestEqual(TEXT("Expanded content bounds retain the Special zone"),
		PreCollapse.ExpandedContentZone, EZoneKind::SpecialZone);
	TestEqual(TEXT("Expanded content bounds retain the Special owner"),
		PreCollapse.ExpandedContentOwnerInstanceId, OwnerId);
	TestFalse(TEXT("No collapse is already pending"),
		PreCollapse.bPileCollapseAnimationPending);
	Workspace->BeginPileCollapseAnimation(EZoneKind::SpecialZone, OwnerId);
	const TArray<FVector2D>& CollapseTargets =
		Workspace->GetAutomationTestView().ActiveBaseCardLayoutTransitionTargetCenters;
	TestTrue(TEXT("Expanded pile writes collapse targets for every visible pile card"),
		CollapseTargets.Num() >= 2);
	bool bHasSeparatedCollapseTargets = false;
	for (int32 LeftIndex = 0; LeftIndex < CollapseTargets.Num(); ++LeftIndex)
	{
		for (int32 RightIndex = LeftIndex + 1; RightIndex < CollapseTargets.Num(); ++RightIndex)
		{
			bHasSeparatedCollapseTargets |= !CollapseTargets[LeftIndex].Equals(
				CollapseTargets[RightIndex], 0.5f);
		}
	}
	TestTrue(TEXT("Collapse targets the final spaced pile instead of one header point"),
		bHasSeparatedCollapseTargets);
	return true;
}

#endif
