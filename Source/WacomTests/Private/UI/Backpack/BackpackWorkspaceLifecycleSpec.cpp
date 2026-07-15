// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../BackpackScreenTestAccess.h"
#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "RunSession.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UI/Backpack/BackpackWorkspaceModelTestAccess.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceRunScopedLifecycleSpec,
	"Wacom.UI.Backpack.Workspace.RunScopedLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceRunScopedLifecycleSpec::RunTest(const FString& Parameters)
{
	const FWacomBackpackWorkspaceStateLifecycleTestView View =
		FWacomBackpackWorkspaceModelTestAccess::RunStateLifecycleScenario();
	TestTrue(TEXT("Rebinding the same Run preserves manual layout"), View.bSameRunPreservedLayout);
	TestTrue(TEXT("Rebinding the same Run preserves active zone"), View.bActiveZonePreservedForSameRun);
	TestTrue(TEXT("Snapshot reconcile prunes removed card layout"), View.bRemovedCardLayoutPruned);
	TestTrue(TEXT("Snapshot reconcile does not invent layout for new card"), View.bNewCardHasNoManualLayout);
	TestTrue(TEXT("Binding a new Run clears all old layouts"), View.bNewRunClearedLayouts);
	TestTrue(TEXT("Binding a new Run clears old active zone"), View.bActiveZoneResetForNewRun);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceScreenCompositionSpec,
	"Wacom.UI.Backpack.Workspace.ScreenComposition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceScreenCompositionSpec::RunTest(const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	UCardDefinition* CapacityCard = NewObject<UCardDefinition>(Outer);
	CapacityCard->CardId = TEXT("Backpack.Workspace.Capacity");
	CapacityCard->DisplayName = FText::FromString(TEXT("容量卡"));
	CapacityCard->Physique.Capacity = 4;
	UCardDefinition* BattleCard = NewObject<UCardDefinition>(Outer);
	BattleCard->CardId = TEXT("Backpack.Workspace.Battle");
	BattleCard->DisplayName = FText::FromString(TEXT("战斗卡"));

	UCharacterDefinition* Character = NewObject<UCharacterDefinition>(Outer);
	Character->CharacterId = TEXT("Backpack.Workspace.Character");
	Character->StarterDeck.Add(CapacityCard);
	for (int32 Index = 0; Index < 7; ++Index)
	{
		Character->StarterDeck.Add(BattleCard);
	}
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Workspace composition Run initializes"), Run->Initialize(Character));
	const FRunBackpackStorageSnapshot Snapshot = Run->BuildBackpackStorageSnapshot();

	TStrongObjectPtr<UWacomBackpackScreen> Screen(
		FWacomBackpackScreenTestAccess::Create(Outer, Run.Get()));
	TestEqual(TEXT("Fallback builds persistent flux and battle rack entries"),
		FWacomBackpackScreenTestAccess::ZoneRackEntryCount(*Screen), 2);
	TestEqual(TEXT("Flux workspace is the deterministic initial active zone"),
		FWacomBackpackScreenTestAccess::ActiveWorkspaceZone(*Screen), EZoneKind::Backpack);
	TestEqual(TEXT("Initial workspace contains only active flux cards"),
		FWacomBackpackScreenTestAccess::WorkspaceCardCount(*Screen), Snapshot.Flux.ContentCards.Num());
	TestTrue(TEXT("Fallback workspace child fills its runtime host"),
		FWacomBackpackScreenTestAccess::WorkspaceChildFillsHost(*Screen));
	TestTrue(TEXT("Fallback workspace owns blank-area pointer input"),
		FWacomBackpackScreenTestAccess::WorkspaceOwnsPointerInput(*Screen));
	TestTrue(TEXT("Marquee keeps capture when its route crosses a card"),
		FWacomBackpackScreenTestAccess::MarqueeCrossingCardPreservesMouseCapture(*Screen));
	TestTrue(TEXT("Marquee completes when left release occurs over a card"),
		FWacomBackpackScreenTestAccess::MarqueeCompletesWhenReleasedOverCard(*Screen));
	TestTrue(TEXT("Screen-level carry can begin in active workspace"),
		FWacomBackpackScreenTestAccess::BeginWorkspaceCarry(*Screen));
	TestTrue(TEXT("Screen-level carry owns capture"),
		FWacomBackpackScreenTestAccess::WorkspaceView(*Screen).bMouseCaptured);

	FWacomBackpackScreenTestAccess::ActivateZone(*Screen, EZoneKind::BattleDeck);
	TestTrue(TEXT("Zone switch clears screen-owned carry"),
		FWacomBackpackScreenTestAccess::WorkspaceView(*Screen).CarriedInstanceIds.IsEmpty());
	TestFalse(TEXT("Zone switch releases screen-owned capture"),
		FWacomBackpackScreenTestAccess::WorkspaceView(*Screen).bMouseCaptured);
	TestEqual(TEXT("Rack activation switches the sole active workspace"),
		FWacomBackpackScreenTestAccess::ActiveWorkspaceZone(*Screen), EZoneKind::BattleDeck);
	TestEqual(TEXT("Battle workspace includes physical and read-only projected cards"),
		FWacomBackpackScreenTestAccess::WorkspaceCardCount(*Screen),
		Snapshot.BattleDeckPhysicalCards.Num() + Snapshot.BattleDeckProjectedCards.Num());
	TestFalse(TEXT("Idle Workspace leaves Escape unhandled for CommonUI Back"),
		FWacomBackpackScreenTestAccess::PressWorkspaceEscape(*Screen));
	TestTrue(TEXT("Carry starts before layered Escape cancellation"),
		FWacomBackpackScreenTestAccess::BeginWorkspaceCarry(*Screen));
	TestTrue(TEXT("Workspace handles Escape while carrying"),
		FWacomBackpackScreenTestAccess::PressWorkspaceEscape(*Screen));
	TestTrue(TEXT("Carry Escape clears the carried fan"),
		FWacomBackpackScreenTestAccess::WorkspaceView(*Screen).CarriedInstanceIds.IsEmpty());
	TestFalse(TEXT("Carry Escape releases mouse capture"),
		FWacomBackpackScreenTestAccess::WorkspaceView(*Screen).bMouseCaptured);
	TestTrue(TEXT("Marquee starts before layered Escape cancellation"),
		FWacomBackpackScreenTestAccess::BeginWorkspaceMarquee(*Screen));
	TestTrue(TEXT("Workspace handles Escape while marquee is active"),
		FWacomBackpackScreenTestAccess::PressWorkspaceEscape(*Screen));
	TestFalse(TEXT("Marquee Escape clears the active marquee"),
		FWacomBackpackScreenTestAccess::IsWorkspaceMarqueeActive(*Screen));
	TestFalse(TEXT("Marquee Escape releases mouse capture"),
		FWacomBackpackScreenTestAccess::WorkspaceView(*Screen).bMouseCaptured);
	const TArray<FVector2D> BattleCardPositions =
		FWacomBackpackScreenTestAccess::WorkspaceCardPositions(*Screen);
	TSet<FIntPoint> DistinctBattleCardPositions;
	for (const FVector2D& Position : BattleCardPositions)
	{
		DistinctBattleCardPositions.Add(FIntPoint(
			FMath::RoundToInt(Position.X),
			FMath::RoundToInt(Position.Y)));
	}
	TestEqual(TEXT("Zone switch assigns a distinct default slot to every battle card"),
		DistinctBattleCardPositions.Num(), BattleCardPositions.Num());

	UClass* FormalScreenClass = LoadClass<UWacomBackpackScreen>(
		nullptr,
		TEXT("/Game/Wacom/UI/Backpack/WBP_BackpackScreen.WBP_BackpackScreen_C"));
	TestNotNull(TEXT("Formal Backpack screen class loads for zone-switch regression"), FormalScreenClass);
	TStrongObjectPtr<UWacomBackpackScreen> FormalScreen(
		FWacomBackpackScreenTestAccess::CreateWithClass(Outer, Run.Get(), FormalScreenClass));
	TestNotNull(TEXT("Formal Backpack screen instantiates for zone-switch regression"), FormalScreen.Get());
	if (FormalScreen)
	{
		TestTrue(TEXT("Formal WBP workspace child fills its overlay host"),
			FWacomBackpackScreenTestAccess::WorkspaceChildFillsHost(*FormalScreen));
		TestTrue(TEXT("Formal WBP workspace owns blank-area pointer input"),
			FWacomBackpackScreenTestAccess::WorkspaceOwnsPointerInput(*FormalScreen));
		FWacomBackpackScreenTestAccess::ActivateZone(*FormalScreen, EZoneKind::BattleDeck);
		const TArray<FVector2D> FormalBattlePositions =
			FWacomBackpackScreenTestAccess::WorkspaceCardPositions(*FormalScreen);
		TSet<FIntPoint> DistinctFormalBattlePositions;
		for (const FVector2D& Position : FormalBattlePositions)
		{
			DistinctFormalBattlePositions.Add(FIntPoint(
				FMath::RoundToInt(Position.X),
				FMath::RoundToInt(Position.Y)));
		}
		TestEqual(TEXT("Formal WBP zone switch assigns distinct battle-card slots"),
			DistinctFormalBattlePositions.Num(), FormalBattlePositions.Num());
		TestTrue(
			TEXT("First stable Workspace geometry triggers one authoritative layout refresh"),
			FWacomBackpackScreenTestAccess::ApplyStableWorkspaceGeometry(
				*FormalScreen,
				FVector2D(920.0f, 580.0f)));
		const FWacomBackpackWorkspaceAutomationTestView DeferredRenderView =
			FWacomBackpackScreenTestAccess::WorkspaceView(*FormalScreen);
		TestTrue(
			TEXT("Stable Workspace geometry defers the final retained-card redraw until the next Slate frame"),
			DeferredRenderView.bDeferredCardFaceRenderPending);
		FWacomBackpackScreenTestAccess::FlushDeferredWorkspaceCardFaceRender(*FormalScreen);
		const FWacomBackpackWorkspaceAutomationTestView FlushedRenderView =
			FWacomBackpackScreenTestAccess::WorkspaceView(*FormalScreen);
		TestFalse(
			TEXT("Deferred retained-card redraw is one-shot"),
			FlushedRenderView.bDeferredCardFaceRenderPending);
		TestEqual(
			TEXT("Deferred retained-card redraw executes exactly one pass"),
			FlushedRenderView.DeferredCardFaceRenderPassCount,
			DeferredRenderView.DeferredCardFaceRenderPassCount + 1);
		const TArray<FVector2D> StableFormalBattlePositions =
			FWacomBackpackScreenTestAccess::WorkspaceCardPositions(*FormalScreen);
		bool bAnyPositionChangedAfterStableGeometry = false;
		for (int32 Index = 0; Index < FormalBattlePositions.Num(); ++Index)
		{
			if (StableFormalBattlePositions.IsValidIndex(Index)
				&& !StableFormalBattlePositions[Index].Equals(FormalBattlePositions[Index], 0.5f))
			{
				bAnyPositionChangedAfterStableGeometry = true;
				break;
			}
		}
		TestTrue(
			TEXT("Stable Workspace geometry replaces the pre-layout fallback positions"),
			bAnyPositionChangedAfterStableGeometry);
		for (const float Opacity :
			FWacomBackpackScreenTestAccess::WorkspaceCardRenderOpacities(*FormalScreen))
		{
			TestTrue(
				TEXT("Stable Workspace refresh leaves every physical card fully opaque"),
				FMath::IsNearlyEqual(Opacity, 1.0f));
		}
		TestFalse(
			TEXT("An unchanged stable Workspace geometry does not rebuild again"),
			FWacomBackpackScreenTestAccess::ApplyStableWorkspaceGeometry(
				*FormalScreen,
				FVector2D(920.0f, 580.0f)));
		FWacomBackpackScreenTestAccess::ApplyWorkspaceLayerTransition(*FormalScreen, true);
		TestFalse(
			TEXT("CommonUI layer transition temporarily bypasses retained card-face rendering"),
			FWacomBackpackScreenTestAccess::WorkspaceView(*FormalScreen)
				.bCardFaceRetainedRenderingEnabled);
		FWacomBackpackScreenTestAccess::ApplyWorkspaceLayerTransition(*FormalScreen, false);
		TestTrue(
			TEXT("Retained card-face rendering resumes only after the CommonUI layer transition"),
			FWacomBackpackScreenTestAccess::WorkspaceView(*FormalScreen)
				.bCardFaceRetainedRenderingEnabled);
		TestTrue(TEXT("Formal WBP marquee keeps capture while crossing a card"),
			FWacomBackpackScreenTestAccess::MarqueeCrossingCardPreservesMouseCapture(*FormalScreen));
		TestTrue(TEXT("Formal WBP marquee completes when released over a card"),
			FWacomBackpackScreenTestAccess::MarqueeCompletesWhenReleasedOverCard(*FormalScreen));
	}
	TestTrue(TEXT("Carry restarts before deactivation"),
		FWacomBackpackScreenTestAccess::BeginWorkspaceCarry(*Screen));
	FWacomBackpackScreenTestAccess::DeactivateWorkspaceScreen(*Screen);
	TestTrue(TEXT("Deactivate clears carry"),
		FWacomBackpackScreenTestAccess::WorkspaceView(*Screen).CarriedInstanceIds.IsEmpty());
	TestFalse(TEXT("Deactivate releases capture"),
		FWacomBackpackScreenTestAccess::WorkspaceView(*Screen).bMouseCaptured);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceInteractionLifecycleSpec,
	"Wacom.UI.Backpack.Workspace.InteractionLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceInteractionLifecycleSpec::RunTest(const FString& Parameters)
{
	const FGuid First(1, 2, 3, 4);
	const FGuid Second(5, 6, 7, 8);
	const FWacomBackpackZoneKey Backpack = FWacomBackpackZoneKey::Make(EZoneKind::Backpack);
	TArray<FWacomBackpackWorkspaceCardHitRecord> Cards = {
		{ First, FVector2D(100.0f, 100.0f), 0, true },
		{ Second, FVector2D(200.0f, 100.0f), 1, true },
	};

	FWacomBackpackWorkspaceInteractionModel Model;
	Model.ReconcileCards(Backpack, Cards);
	Model.BeginMarquee(FVector2D::ZeroVector, false);
	TestTrue(TEXT("Marquee owns logical mouse capture"), Model.IsMouseCaptured());
	Model.CancelTransientState();
	TestFalse(TEXT("Deactivate-style cancel releases marquee capture"), Model.IsMouseCaptured());
	TestFalse(TEXT("Deactivate-style cancel clears marquee"), Model.IsMarqueeActive());

	Model.ReconcileCards(Backpack, Cards);
	Model.SelectAllMovable();
	TestTrue(TEXT("Carry starts for lifecycle test"), Model.BeginCarry(First, FVector2D(400.0f, 300.0f), 9));
	Model.ReconcileCards(FWacomBackpackZoneKey::Make(EZoneKind::BattleDeck), Cards);
	TestFalse(TEXT("Zone switch cancels carry"), Model.IsCarrying());
	TestFalse(TEXT("Zone switch releases capture"), Model.IsMouseCaptured());
	TestTrue(TEXT("Zone switch clears selection"), Model.GetSelection().OrderedSelectedInstanceIds.IsEmpty());

	Model.ReconcileCards(Backpack, Cards);
	Model.SelectAllMovable();
	TestTrue(TEXT("Carry restarts for invalidating refresh"), Model.BeginCarry(First, FVector2D(400.0f, 300.0f), 10));
	TArray<FWacomBackpackWorkspaceCardHitRecord> MissingCarriedCard = { Cards[0] };
	Model.ReconcileCards(Backpack, MissingCarriedCard);
	TestFalse(TEXT("Refresh removing any carried identity cancels carry"), Model.IsCarrying());
	TestFalse(TEXT("Invalidating refresh releases capture"), Model.IsMouseCaptured());
	return true;
}

#endif
