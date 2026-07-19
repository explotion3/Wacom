// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceVisualRegistry.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceVisualState.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceVisualStateSpec,
	"Wacom.UI.Backpack.Workspace.Runtime.VisualState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceVisualStateSpec::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UWacomDeckCardWidget> CardA(NewObject<UWacomDeckCardWidget>());
	TStrongObjectPtr<UWacomDeckCardWidget> CardB(NewObject<UWacomDeckCardWidget>());
	FWacomBackpackWorkspaceVisualState State;

	FWacomBackpackWorkspaceCardLayout InitialA;
	InitialA.Center = FVector2D(100.0f, 200.0f);
	InitialA.Size = FVector2D(296.0f, 420.0f);
	InitialA.ZOrder = 10;
	FWacomBackpackWorkspaceCardLayout InitialB = InitialA;
	InitialB.Center.X = 140.0f;
	TestTrue(TEXT("Runtime primes the first base layout"),
		State.PrimeBaseLayout(*CardA, InitialA));
	TestTrue(TEXT("Runtime primes an independent second base layout"),
		State.PrimeBaseLayout(*CardB, InitialB));
	TestFalse(TEXT("Runtime does not overwrite an already primed layout"),
		State.PrimeBaseLayout(*CardA, InitialB));

	FWacomBackpackWorkspaceCardLayout FirstTarget = InitialA;
	FirstTarget.Center.X = 300.0f;
	TestTrue(TEXT("A changed target starts one transition"),
		State.RetargetBaseLayout(*CardA, FirstTarget, true, 1.0f));
	int32 AppliedCount = 0;
	TestTrue(TEXT("Half-ticked transition remains active"),
		State.TickBaseTransitions(
			0.5f,
			false,
			[](UWacomDeckCardWidget&) {},
			&AppliedCount));
	TestEqual(TEXT("Only the moving card is applied"), AppliedCount, 1);
	const FWacomBackpackWorkspaceCardLayoutTransition* HalfTransition =
		State.FindBaseTransition(CardA.Get());
	TestNotNull(TEXT("The active transition is queryable"), HalfTransition);
	const float HalfCenterX = HalfTransition ? HalfTransition->Current.Center.X : 0.0f;

	FWacomBackpackWorkspaceCardLayout RedirectedTarget = FirstTarget;
	RedirectedTarget.Center.X = 500.0f;
	TestTrue(TEXT("Mid-flight retarget creates a continuous replacement transition"),
		State.RetargetBaseLayout(*CardA, RedirectedTarget, true, 1.0f));
	const FWacomBackpackWorkspaceCardLayoutTransition* Redirected =
		State.FindBaseTransition(CardA.Get());
	TestTrue(TEXT("Retarget starts from the current visual center"),
		Redirected && FMath::IsNearlyEqual(Redirected->Start.Center.X, HalfCenterX, 0.01f));

	const FGuid ReleasedId(1, 2, 3, 4);
	FWacomBackpackWorkspaceCardVisualPose ReleasedPose;
	ReleasedPose.Center = FVector2D(640.0f, 360.0f);
	State.RecordReleasedVisualPose(ReleasedId, ReleasedPose);
	State.MarkReleasedHandoff(ReleasedId);
	State.SelectionFrozenLayouts().Add(CardB.Get(), InitialB);
	TSet<UWacomDeckCardWidget*> VisibleCards { CardA.Get() };
	TSet<FGuid> VisibleIds;
	TestTrue(TEXT("Reconcile reports that no frozen selection remains"),
		State.ReconcileVisibleCards(VisibleCards, VisibleIds));
	TestFalse(TEXT("Invisible card base state is pruned"), State.HasBaseLayout(CardB.Get()));
	TestFalse(TEXT("Invisible released handoff is pruned"),
		State.IsReleasedHandoffPending(ReleasedId));

	State.Reset();
	TestFalse(TEXT("Runtime reset clears base layout ownership"),
		State.HasBaseLayout(CardA.Get()));
	TestFalse(TEXT("Runtime reset clears active transitions"),
		State.FindBaseTransition(CardA.Get()) != nullptr);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceVisualRegistryRosterSpec,
	"Wacom.UI.Backpack.Workspace.Runtime.VisualRegistryRoster",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceVisualRegistryRosterSpec::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UWacomDeckCardWidget> CardA(NewObject<UWacomDeckCardWidget>());
	TStrongObjectPtr<UWacomDeckCardWidget> CardB(NewObject<UWacomDeckCardWidget>());
	TArray<TObjectPtr<UWacomDeckCardWidget>> Cards { CardA.Get(), CardB.Get() };
	FWacomBackpackWorkspaceVisualRegistry Registry;
	Registry.ReplaceOrderedCards(Cards);
	const TConstArrayView<TWeakObjectPtr<UWacomDeckCardWidget>> Ordered =
		Registry.GetCardWidgets();
	TestEqual(TEXT("Registry owns the canonical ordered roster"), Ordered.Num(), 2);
	TestEqual(TEXT("Registry preserves scene order"), Ordered[0].Get(), CardA.Get());
	TestEqual(TEXT("Registry preserves the second scene entry"), Ordered[1].Get(), CardB.Get());
	Registry.ResetIndexes();
	TestEqual(TEXT("Registry lifecycle reset clears the ordered roster"),
		Registry.GetCardWidgets().Num(), 0);
	return true;
}

#endif
