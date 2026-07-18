// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceLayoutSolver.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceSceneBuilder.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceVisualRegistry.h"
#include "Blueprint/WidgetTree.h"
#include "Cards/CardDefinition.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UI/Backpack/WacomBackpackWorkspaceStyle.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "../BackpackScreenTestAccess.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCarryLayerAnchorSpec,
	"Wacom.UI.Backpack.Workspace.CarryLayerAnchor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCarryLayerAnchorSpec::RunTest(const FString& Parameters)
{
	for (const int32 CardCount : { 1, 7, 15, 21 })
	{
		TStrongObjectPtr<UWacomBackpackWorkspaceWidget> Workspace(
			NewObject<UWacomBackpackWorkspaceWidget>());
		const TSharedRef<SWidget> WorkspaceSlate = Workspace->TakeWidget();
		TSharedPtr<FWacomBackpackWorkspaceInteractionModel> Model =
			MakeShared<FWacomBackpackWorkspaceInteractionModel>();
		Workspace->SetInteractionModel(Model, nullptr);

		TStrongObjectPtr<UCardDefinition> Definition(NewObject<UCardDefinition>());
		Definition->CardId = *FString::Printf(TEXT("Backpack.CarryLayer.%d"), CardCount);
		TArray<TStrongObjectPtr<UWacomDeckCardWidget>> OwnedCards;
		TArray<TObjectPtr<UWacomDeckCardWidget>> Cards;
		for (int32 Index = 0; Index < CardCount; ++Index)
		{
			TStrongObjectPtr<UWacomDeckCardWidget> Card(NewObject<UWacomDeckCardWidget>());
			FCardInstance Instance;
			Instance.InstanceId = FGuid(Index + 1, CardCount, 3, 4);
			Instance.Definition = Definition.Get();
			Card->SetCard(Instance, EZoneKind::Backpack, FGuid());
			Workspace->GetStaticCardLayer()->AddChildToCanvas(Card.Get());
			Workspace->PrimeCardBaseLayout(
				*Card,
				FVector2D(180.0f + Index * 24.0f, 240.0f),
				FVector2D(220.0f, 320.0f),
				0.0f,
				Index);
			Cards.Add(Card.Get());
			OwnedCards.Add(MoveTemp(Card));
		}
		Workspace->BindWorkspaceCards(Cards, 17);
		Model->SelectAllMovable();
		TestTrue(*FString::Printf(TEXT("%d cards begin carry"), CardCount),
			Model->BeginCarry(Cards[0]->GetCardInstanceId(), FVector2D(300.0f, 220.0f), 17));
		Workspace->RefreshInteractionPresentation();

		const FWacomBackpackWorkspaceAutomationTestView Started =
			Workspace->GetAutomationTestView();
		TestEqual(TEXT("Carry strip is calculated once at pickup"),
			Started.CarryStripLayoutRebuildCount, 1);
		TestEqual(TEXT("Exactly one current card lives outside the cached strip"),
			Started.ActiveCarryCardCount, 1);
		TestEqual(TEXT("Only non-current cards live in the cached strip"),
			Started.CachedCarryCardCount, FMath::Max(0, CardCount - 1));
		const FWacomBackpackWorkspaceCarryState& StartedCarry = Model->GetCarry();
		const UWacomBackpackWorkspaceStyle* Style = GetDefault<UWacomBackpackWorkspaceStyle>();
		UWacomDeckCardWidget* CurrentCard = nullptr;
		if (StartedCarry.RemainingInstanceIds.IsValidIndex(StartedCarry.CurrentIndex))
		{
			const FGuid CurrentId = StartedCarry.RemainingInstanceIds[StartedCarry.CurrentIndex];
			for (UWacomDeckCardWidget* Card : Cards)
			{
				if (Card && Card->GetCardInstanceId() == CurrentId)
				{
					CurrentCard = Card;
					break;
				}
			}
		}
		const UCanvasPanelSlot* CurrentSlot = CurrentCard
			? Cast<UCanvasPanelSlot>(CurrentCard->Slot)
			: nullptr;
		TestNotNull(TEXT("Current carry card has a canvas slot"), CurrentSlot);
		if (CurrentSlot)
		{
			TestTrue(TEXT("Carry focus strip keeps the formal fixed card size"),
				CurrentSlot->GetSize().Equals(Style->CardRenderSize, 0.1f));
			int32 HighestZOrder = TNumericLimits<int32>::Lowest();
			for (UWacomDeckCardWidget* Card : Cards)
			{
				if (const UCanvasPanelSlot* Slot = Card
					? Cast<UCanvasPanelSlot>(Card->Slot)
					: nullptr)
				{
					HighestZOrder = FMath::Max(HighestZOrder, Slot->GetZOrder());
				}
			}
			TestEqual(TEXT("Default release card owns the highest carry ZOrder"),
				CurrentSlot->GetZOrder(), HighestZOrder);
		}
		if (CardCount >= 3)
		{
			TestTrue(TEXT("First wheel step is handled by Workspace"),
				FWacomBackpackScreenTestAccess::StepWorkspaceCarryCurrentByWheel(
					*Workspace, 1.0f));
			UWacomDeckCardWidget* PreviousWheelCurrent = nullptr;
			const FGuid PreviousWheelCurrentId =
				Model->GetCarry().RemainingInstanceIds[Model->GetCarry().CurrentIndex];
			for (UWacomDeckCardWidget* Card : Cards)
			{
				if (Card && Card->GetCardInstanceId() == PreviousWheelCurrentId)
				{
					PreviousWheelCurrent = Card;
					break;
				}
			}

			TestTrue(TEXT("Second wheel step is handled by Workspace"),
				FWacomBackpackScreenTestAccess::StepWorkspaceCarryCurrentByWheel(
					*Workspace, 1.0f));
			UWacomDeckCardWidget* NewWheelCurrent = nullptr;
			const FGuid NewWheelCurrentId =
				Model->GetCarry().RemainingInstanceIds[Model->GetCarry().CurrentIndex];
			for (UWacomDeckCardWidget* Card : Cards)
			{
				if (Card && Card->GetCardInstanceId() == NewWheelCurrentId)
				{
					NewWheelCurrent = Card;
					break;
				}
			}
			const UCanvasPanelSlot* PreviousWheelSlot = PreviousWheelCurrent
				? Cast<UCanvasPanelSlot>(PreviousWheelCurrent->Slot)
				: nullptr;
			const UCanvasPanelSlot* NewWheelSlot = NewWheelCurrent
				? Cast<UCanvasPanelSlot>(NewWheelCurrent->Slot)
				: nullptr;
			TestNotNull(TEXT("Previous wheel card remains available during pose handoff"),
				PreviousWheelSlot);
			TestNotNull(TEXT("New wheel current card has an active canvas slot"), NewWheelSlot);
			if (PreviousWheelSlot && NewWheelSlot)
			{
				TestEqual(TEXT("Wheel handoff keeps both moving cards in the active layer"),
					NewWheelCurrent->GetParent(), PreviousWheelCurrent->GetParent());
				TestTrue(TEXT("New wheel current card renders above the returning previous card"),
					NewWheelSlot->GetZOrder() > PreviousWheelSlot->GetZOrder());
			}

			FWacomBackpackScreenTestAccess::StepWorkspaceCarryCurrentByWheel(
				*Workspace, -1.0f);
			FWacomBackpackScreenTestAccess::StepWorkspaceCarryCurrentByWheel(
				*Workspace, -1.0f);
			const FWacomBackpackWorkspaceCarryState& ReturnedCarry = Model->GetCarry();
			TestEqual(TEXT("Wheel can return to the default rightmost card"),
				ReturnedCarry.CurrentIndex, ReturnedCarry.DefaultIndex);
			FWacomBackpackScreenTestAccess::TickWorkspaceCardMotion(
				*Workspace, Style->CarryCurrentTransitionSeconds + 0.01f);
			UWacomDeckCardWidget* ReturnedDefaultCard = nullptr;
			if (ReturnedCarry.RemainingInstanceIds.IsValidIndex(ReturnedCarry.CurrentIndex))
			{
				const FGuid ReturnedId = ReturnedCarry.RemainingInstanceIds[ReturnedCarry.CurrentIndex];
				for (UWacomDeckCardWidget* Card : Cards)
				{
					if (Card && Card->GetCardInstanceId() == ReturnedId)
					{
						ReturnedDefaultCard = Card;
						break;
					}
				}
			}
			TestNotNull(TEXT("Returned default card remains available"), ReturnedDefaultCard);
			if (ReturnedDefaultCard)
			{
				TestTrue(TEXT("Wheel-selected rightmost card lifts like every other current card"),
					ReturnedDefaultCard->GetBackpackLocalMotionTranslation().Equals(
						FVector2D(0.0f, -Style->CurrentCardLiftPixels), 0.1f));
			}
		}
		const FWacomBackpackWorkspaceAutomationTestView AfterWheelHandoff =
			Workspace->GetAutomationTestView();
		const int32 StaticUpdatesBeforeRepeatedPresentation =
			AfterWheelHandoff.StaticCardPresentationUpdateCount;
		Workspace->RefreshInteractionPresentation();
		TestEqual(TEXT("Repeated presentation does not rewrite a carried current card as static"),
			Workspace->GetAutomationTestView().StaticCardPresentationUpdateCount,
			StaticUpdatesBeforeRepeatedPresentation);
		const FWacomBackpackWorkspaceAutomationTestView PointerBaseline =
			Workspace->GetAutomationTestView();

		const FVector2D LatestPointer(845.25f, 417.75f);
		TArray<FVector2D> PointerBurst;
		for (int32 Step = 0; Step < 12; ++Step)
		{
			PointerBurst.Add(FMath::Lerp(
				FVector2D(300.0f, 220.0f),
				LatestPointer,
				(Step + 1) / 12.0f));
		}
		if (CurrentCard)
		{
			FWacomBackpackScreenTestAccess::SendWorkspaceCarryPointerEvents(
				*Workspace, *CurrentCard, PointerBurst);
		}
		const FWacomBackpackWorkspaceAutomationTestView BeforeFrameFlush =
			Workspace->GetAutomationTestView();
		TestTrue(TEXT("Pointer events update exact logical target immediately"),
			BeforeFrameFlush.CarryAnchorLocal.Equals(LatestPointer, 1.0f));
		TestTrue(TEXT("High-frequency pointer events wait for the single Slate-frame apply"),
			BeforeFrameFlush.CarryRootTranslation.Equals(PointerBaseline.CarryRootTranslation, 0.1f));
		TestEqual(TEXT("Pointer burst does not issue visual anchor writes before the frame flush"),
			BeforeFrameFlush.CarryVisualAnchorApplyCount,
			PointerBaseline.CarryVisualAnchorApplyCount);
		FWacomBackpackScreenTestAccess::FlushWorkspaceCarryPointer(*Workspace);
		const FWacomBackpackWorkspaceAutomationTestView Moved =
			Workspace->GetAutomationTestView();
		TestTrue(TEXT("Carry logical anchor remains at the latest pointer with <=1px error"),
			Moved.CarryAnchorLocal.Equals(LatestPointer, 1.0f));
		TestTrue(TEXT("Visual CarryRoot remains within the authored maximum lag"),
			FVector2D::Distance(Moved.CarryRootTranslation, LatestPointer)
				<= Style->CarryMaximumVisualLagPixels + 0.1f);
		TestTrue(TEXT("Automation view reports the independent visual anchor"),
			Moved.CarryRootTranslation.Equals(Moved.CarryVisualAnchorLocal, 0.1f));
		TestTrue(TEXT("Invalidation cache remains at a stable local transform"),
			Moved.CarryCacheTranslation.IsNearlyZero(0.1f));
		TestEqual(TEXT("One Slate-frame flush applies the latest pointer exactly once"),
			Moved.CarryVisualAnchorApplyCount,
			PointerBaseline.CarryVisualAnchorApplyCount + 1);
		TestEqual(TEXT("Pointer movement does not rebuild the local strip"),
			Moved.CarryStripLayoutRebuildCount, PointerBaseline.CarryStripLayoutRebuildCount);
		TestEqual(TEXT("Pointer movement does not refresh static card presentation"),
			Moved.StaticCardPresentationUpdateCount, PointerBaseline.StaticCardPresentationUpdateCount);
		if (CardCount == 1)
		{
			TestTrue(TEXT("Release commit is observed before the target scene reconcile"),
				FWacomBackpackScreenTestAccess::CommitWorkspaceReleaseBeforeTargetReconcile(
					*Workspace));
			TestTrue(TEXT("Released card keeps carry visual ownership until target layout handoff"),
				Workspace->ShouldPreserveCardParent(Cards[0]));
			const FVector2D TargetCenter(920.0f, 510.0f);
			Workspace->ApplyCardBaseLayout(
				*Cards[0], TargetCenter, FVector2D(220.0f, 320.0f), 0.0f, 4000);
			Workspace->RefreshInteractionPresentation();
			TestEqual(TEXT("Committed release does not start a cached source-to-target transition"),
				Workspace->GetAutomationTestView().ActiveBaseCardLayoutTransitionCount, 0);
			const UCanvasPanelSlot* ReleasedSlot = Cast<UCanvasPanelSlot>(Cards[0]->Slot);
			TestEqual(TEXT("Released card enters the dedicated SettlementLayer"),
				Workspace->GetAutomationTestView().SettlementCardCount, 1);
			TestNotNull(TEXT("Released card keeps a target canvas slot during settlement"), ReleasedSlot);
			if (ReleasedSlot)
			{
				TestTrue(TEXT("Released card lands directly at the target layout"),
					ReleasedSlot->GetPosition().Equals(
						TargetCenter - FVector2D(110.0f, 160.0f), 1.0f));
			}
		}

		Workspace->CancelInteraction();
		for (UWacomDeckCardWidget* Card : Cards)
		{
			TestEqual(TEXT("Cancel restores the original widget to StaticCardLayer"),
				Card->GetParent(), static_cast<UPanelWidget*>(Workspace->GetStaticCardLayer()));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCarryCrossZoneIdentitySpec,
	"Wacom.UI.Backpack.Workspace.CarryCrossZoneIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCarryCrossZoneIdentitySpec::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UCanvasPanel> StaticCanvas(NewObject<UCanvasPanel>());
	TStrongObjectPtr<UCanvasPanel> CarryCanvas(NewObject<UCanvasPanel>());
	TStrongObjectPtr<UCardDefinition> Definition(NewObject<UCardDefinition>());
	Definition->CardId = TEXT("Backpack.Carry.CrossZoneIdentity");
	FCardInstance Instance;
	Instance.InstanceId = FGuid(61, 62, 63, 64);
	Instance.Definition = Definition.Get();
	TStrongObjectPtr<UWacomDeckCardWidget> CarriedWidget(NewObject<UWacomDeckCardWidget>());
	CarriedWidget->SetCard(Instance, EZoneKind::Backpack, FGuid());
	CarriedWidget->ApplyBackpackLocalMotionPose(FVector2D(13.0f, -9.0f), 7.0f);
	CarryCanvas->AddChildToCanvas(CarriedWidget.Get());

	FWacomBackpackWorkspaceSceneCardEntry Desired;
	Desired.CardView.Instance = Instance;
	Desired.CardView.PhysicalZone = EZoneKind::BattleDeck;
	Desired.DisplayZone = FWacomBackpackZoneKey::Make(EZoneKind::BattleDeck);
	Desired.Role = EWacomBackpackDeckCardListReuseRole::PhysicalList;
	TArray<UPanelWidget*> SearchPanels { StaticCanvas.Get(), CarryCanvas.Get() };
	TArray<FWacomBackpackWorkspaceSceneCardEntry> DesiredCards { Desired };
	TArray<TObjectPtr<UWacomDeckCardWidget>> Ordered;
	int32 CreatedWidgetCount = 0;
	FWacomBackpackWorkspaceVisualRegistry Registry;
	Registry.ReconcileCards(
		SearchPanels,
		*StaticCanvas,
		DesiredCards,
		[Carried = CarriedWidget.Get()](const UWacomDeckCardWidget* Widget)
		{
			return Widget == Carried;
		},
		[&CreatedWidgetCount](const FRunStorageCardView&)
		{
			++CreatedWidgetCount;
			return NewObject<UWacomDeckCardWidget>();
		},
		[](UWacomDeckCardWidget*) {},
		Ordered);

	TestEqual(TEXT("Cross-zone handoff does not create a replacement widget"),
		CreatedWidgetCount, 0);
	TestEqual(TEXT("The ordered target scene keeps the carried widget identity"),
		Ordered.Num() == 1 ? Ordered[0].Get() : nullptr, CarriedWidget.Get());
	TestEqual(TEXT("The reused widget consumes the target physical zone"),
		CarriedWidget->GetFromZone(), EZoneKind::BattleDeck);
	TestEqual(TEXT("The reused widget remains in carry ownership until target layout"),
		CarriedWidget->GetParent(), static_cast<UPanelWidget*>(CarryCanvas.Get()));
	TestTrue(TEXT("Cross-zone handoff preserves the captured local pose"),
		CarriedWidget->GetBackpackLocalMotionTranslation().Equals(
			FVector2D(13.0f, -9.0f), 0.01f));
	TestTrue(TEXT("Cross-zone handoff preserves the captured local angle"),
		FMath::IsNearlyEqual(CarriedWidget->GetBackpackLocalMotionAngle(), 7.0f, 0.01f));
	return true;
}

#endif
