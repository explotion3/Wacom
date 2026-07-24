// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceReconciler.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceStateSubsystem.h"
#include "Cards/CardDefinition.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "../BackpackScreenTestAccess.h"
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
	FWacomUIBackpackSettlementSceneIdentitySpec,
	"Wacom.UI.Backpack.Workspace.SettlementSceneIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackSettlementSceneIdentitySpec::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UCardDefinition> Definition(NewObject<UCardDefinition>());
	Definition->CardId = TEXT("Backpack.Settlement.SceneIdentity");
	const FGuid FluxId(91, 92, 93, 94);
	FRunBackpackStorageSnapshot Snapshot;
	Snapshot.Flux.ContentCards.Add(MakeSceneCard(
		Definition.Get(), FluxId, EZoneKind::Backpack));

	TStrongObjectPtr<UWacomBackpackWorkspaceWidget> Workspace(
		NewObject<UWacomBackpackWorkspaceWidget>());
	const TSharedRef<SWidget> RetainedWorkspaceSlate = Workspace->TakeWidget();
	TSharedPtr<FWacomBackpackWorkspaceInteractionModel> Interaction =
		MakeShared<FWacomBackpackWorkspaceInteractionModel>();
	Workspace->SetInteractionModel(Interaction, nullptr);
	FWacomBackpackWorkspaceStateStore State;
	int32 CreatedWidgetCount = 0;
	auto CreateCard = [&Workspace, &CreatedWidgetCount](const FRunStorageCardView&)
	{
		++CreatedWidgetCount;
		return NewObject<UWacomDeckCardWidget>(Workspace.Get());
	};
	auto RemoveCard = [](UWacomDeckCardWidget*) {};
	TArray<TObjectPtr<UWacomDeckCardWidget>> FirstScene;
	FWacomBackpackWorkspaceReconciler::Reconcile(
		*Workspace, Snapshot, State, Interaction.Get(), nullptr,
		CreateCard, RemoveCard, &FirstScene);
	if (!TestEqual(TEXT("Initial scene creates one physical card"), FirstScene.Num(), 1))
	{
		return false;
	}
	UWacomDeckCardWidget* OriginalCard = FirstScene[0];
	UCanvasPanel* Settlement = Workspace->GetSettlementCanvas();
	if (!TestNotNull(TEXT("Workspace provides the dedicated settlement layer"), Settlement))
	{
		return false;
	}
	OriginalCard->RemoveFromParent();
	Settlement->AddChildToCanvas(OriginalCard);

	TArray<TObjectPtr<UWacomDeckCardWidget>> ReconciledScene;
	FWacomBackpackWorkspaceReconciler::Reconcile(
		*Workspace, Snapshot, State, Interaction.Get(), nullptr,
		CreateCard, RemoveCard, &ReconciledScene);
	TestEqual(TEXT("A scene refresh during settlement does not create a duplicate card"),
		CreatedWidgetCount, 1);
	TestEqual(TEXT("The authoritative scene reuses the settling widget identity"),
		ReconciledScene.Num() == 1 ? ReconciledScene[0].Get() : nullptr,
		OriginalCard);
	TestEqual(TEXT("The settling widget retains its visual parent until motion completes"),
		OriginalCard->GetParent(), static_cast<UPanelWidget*>(Settlement));
	TestEqual(TEXT("Settlement contains exactly one visual for the physical card"),
		Settlement->GetChildrenCount(), 1);

	UWacomDeckCardWidget* StaleDuplicate = NewObject<UWacomDeckCardWidget>(Workspace.Get());
	StaleDuplicate->SetStorageCardView(Snapshot.Flux.ContentCards[0]);
	Workspace->GetStaticCardLayer()->AddChildToCanvas(StaleDuplicate);
	TArray<TObjectPtr<UWacomDeckCardWidget>> DeduplicatedScene;
	FWacomBackpackWorkspaceReconciler::Reconcile(
		*Workspace, Snapshot, State, Interaction.Get(), nullptr,
		CreateCard, RemoveCard, &DeduplicatedScene);
	TestEqual(TEXT("An existing duplicate is retired without creating another widget"),
		CreatedWidgetCount, 1);
	TestEqual(TEXT("Deduplication preserves the authoritative settling identity"),
		DeduplicatedScene.Num() == 1 ? DeduplicatedScene[0].Get() : nullptr,
		OriginalCard);
	TestNull(TEXT("The stale static duplicate no longer has a visual parent"),
		StaleDuplicate->GetParent());
	return true;
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
	const TSharedRef<SWidget> RetainedWorkspaceSlate = Workspace->TakeWidget();
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
	TestTrue(TEXT("Collapsed Battle pile content drag starts and completes a marquee"),
		FWacomBackpackScreenTestAccess::MarqueeWorkspacePileContents(
			*Workspace, EZoneKind::BattleDeck));
	TestEqual(TEXT("Collapsed pile marquee selects its movable physical card only"),
		Interaction->GetSelection().OrderedSelectedInstanceIds,
		TArray<FGuid>{ BattleId });
	FWacomBackpackScreenTestAccess::ClearWorkspaceSelection(*Workspace);
	TestEqual(TEXT("Collapsed Burden card keeps a normal physical identity"),
		CollapsedCards[5]->GetWorkspaceReadOnlyKind(),
		EWacomBackpackWorkspaceCardReadOnlyKind::None);
	TestFalse(TEXT("Collapsed Burden card waits for pile expansion"),
		CollapsedCards[5]->IsWorkspaceInteractionEnabled());
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
	// the in-flight animation and snap every card to its final strip position.
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
	const FWacomBackpackWorkspaceAutomationTestView BeforeTransitionTick =
		Workspace->GetAutomationTestView();
	TestTrue(TEXT("The shared frame scheduler owns active base-layout transitions"),
		BeforeTransitionTick.bFrameSchedulerActive);
	FWacomBackpackScreenTestAccess::TickWorkspaceBaseCardLayoutTransitions(
		*Workspace,
		1.0f / 120.0f);
	const FWacomBackpackWorkspaceAutomationTestView AfterTransitionTick =
		Workspace->GetAutomationTestView();
	TestEqual(TEXT("A base-layout animation frame never invokes a full presentation refresh"),
		AfterTransitionTick.FullPresentationRefreshCount,
		BeforeTransitionTick.FullPresentationRefreshCount);
	TestEqual(TEXT("The targeted base-layout transition records one animation frame"),
		AfterTransitionTick.BaseCardLayoutTransitionTickCount,
		BeforeTransitionTick.BaseCardLayoutTransitionTickCount + 1);
	TestEqual(TEXT("The transition frame updates only its active card set"),
		AfterTransitionTick.BaseCardLayoutTransitionApplyCount
			- BeforeTransitionTick.BaseCardLayoutTransitionApplyCount,
		BeforeTransitionTick.ActiveBaseCardLayoutTransitionCount);
	TestEqual(TEXT("Targeted base-layout updates and static presentation updates stay one-to-one"),
		AfterTransitionTick.StaticCardPresentationUpdateCount
			- BeforeTransitionTick.StaticCardPresentationUpdateCount,
		BeforeTransitionTick.ActiveBaseCardLayoutTransitionCount);
	TestTrue(TEXT("A 120 FPS transition step does not prematurely finish the expansion"),
		AfterTransitionTick.ActiveBaseCardLayoutTransitionCount > 0);
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
	const auto CaptureVisualCenter = [](const UWacomDeckCardWidget& Card)
	{
		const UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Card.Slot);
		return Slot
			? Slot->GetPosition() + Slot->GetSize() * 0.5f
				+ Card.GetBackpackLocalMotionTranslation()
			: FVector2D::ZeroVector;
	};
	const FVector2D OwnerCenterBeforeSelection = CaptureVisualCenter(*ExpandedCards[3]);
	const FVector2D ContentCenterBeforeSelection = CaptureVisualCenter(*ExpandedCards[4]);
	TestTrue(TEXT("Expanded Special pile content drag starts and completes a marquee"),
		FWacomBackpackScreenTestAccess::MarqueeWorkspacePileContents(
			*Workspace, EZoneKind::SpecialZone, OwnerId));
	TestTrue(TEXT("Marquee freezes the owner preview at its displayed animation frame"),
		CaptureVisualCenter(*ExpandedCards[3]).Equals(OwnerCenterBeforeSelection, 0.1f));
	TestTrue(TEXT("Marquee freezes selectable content at its displayed animation frame"),
		CaptureVisualCenter(*ExpandedCards[4]).Equals(ContentCenterBeforeSelection, 0.1f));
	TestEqual(TEXT("Expanded selection owns a visual snapshot for both browse identities"),
		Workspace->GetAutomationTestView().SelectionFrozenCardCount, 2);
	TestEqual(TEXT("Expanded pile marquee selects content but excludes its owner preview"),
		Interaction->GetSelection().OrderedSelectedInstanceIds,
		TArray<FGuid>{ ContentId });
	FWacomBackpackScreenTestAccess::ClearWorkspaceSelection(*Workspace);
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
	// The horizontal strip keeps its first card anchored, so only cards whose final
	// collapsed position actually changes should allocate a transition.
	TestTrue(TEXT("Expanded pile writes a collapse target for every card that must move"),
		!CollapseTargets.IsEmpty());
	bool bHasDuplicateCollapseTarget = false;
	for (int32 LeftIndex = 0; LeftIndex < CollapseTargets.Num(); ++LeftIndex)
	{
		for (int32 RightIndex = LeftIndex + 1; RightIndex < CollapseTargets.Num(); ++RightIndex)
		{
			bHasDuplicateCollapseTarget |= CollapseTargets[LeftIndex].Equals(
				CollapseTargets[RightIndex], 0.5f);
		}
	}
	TestFalse(TEXT("Moving cards never collapse through one shared intermediate point"),
		bHasDuplicateCollapseTarget);
	return true;
}

#endif
