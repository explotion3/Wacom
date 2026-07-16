// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackCardPresentationController.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "Cards/CardDefinition.h"
#include "Components/CanvasPanel.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "UI/Card/WacomFirstPersonCardViewWidget.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardPresentationBudgetSpec,
	"Wacom.UI.Backpack.CardView.RealtimeBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardPresentationBudgetSpec::RunTest(const FString& Parameters)
{
	UClass* DeckCardClass = LoadClass<UWacomDeckCardWidget>(
		nullptr,
		TEXT("/Game/Wacom/UI/Card/WBP_WacomDeckCardWidget.WBP_WacomDeckCardWidget_C"));
	TestNotNull(TEXT("Formal DeckCard class loads"), DeckCardClass);
	if (!DeckCardClass)
	{
		return false;
	}

	TStrongObjectPtr<UCardDefinition> Definition(NewObject<UCardDefinition>());
	Definition->CardId = TEXT("Backpack.Presentation.Budget");
	TArray<TStrongObjectPtr<UWacomDeckCardWidget>> OwnedCards;
	TArray<TWeakObjectPtr<UWacomDeckCardWidget>> Cards;
	for (int32 Index = 0; Index < 2; ++Index)
	{
		TStrongObjectPtr<UWacomDeckCardWidget> Card(
			NewObject<UWacomDeckCardWidget>(GetTransientPackage(), DeckCardClass));
		Card->TakeWidget();
		FCardInstance Instance;
		Instance.InstanceId = FGuid(Index + 1, 22, 33, 44);
		Instance.Definition = Definition.Get();
		Card->SetCard(Instance, EZoneKind::Backpack, FGuid());
		Card->PrepareForBackpackListReuse();
		Cards.Add(Card.Get());
		OwnedCards.Add(MoveTemp(Card));
	}

	auto CardFace = [](UWacomDeckCardWidget& Card)
	{
		return Cast<UWacomFirstPersonCardViewWidget>(
			Card.GetWidgetFromName(TEXT("BackpackCardView")));
	};
	UWacomFirstPersonCardViewWidget* FirstFace = CardFace(*OwnedCards[0]);
	UWacomFirstPersonCardViewWidget* SecondFace = CardFace(*OwnedCards[1]);
	TestNotNull(TEXT("First DeckCard hosts WBP_FPCardView"), FirstFace);
	TestNotNull(TEXT("Second DeckCard hosts WBP_FPCardView"), SecondFace);
	if (!FirstFace || !SecondFace)
	{
		return false;
	}
	TestFalse(TEXT("Backpack cards start in static redraw mode"),
		FirstFace->GetAutomationTestViewForTest().bRealtimePresentationEnabled);

	FWacomBackpackCardPresentationController Controller;
	Controller.Reconcile(
		Cards,
		OwnedCards[0]->GetCardInstanceId(),
		nullptr,
		nullptr,
		FGeometry(),
		FVector2D::ZeroVector);
	TestTrue(TEXT("Hovered card enables shared Fake3D realtime presentation"),
		FirstFace->GetAutomationTestViewForTest().bRealtimePresentationEnabled);
	TestFalse(TEXT("Non-hovered card stays static"),
		SecondFace->GetAutomationTestViewForTest().bRealtimePresentationEnabled);
	TestTrue(TEXT("Hovered card receives Fake3D depth"),
		FirstFace->GetAutomationTestViewForTest().CardDepthView.bFake3DEnabled);

	TStrongObjectPtr<UCanvasPanel> CarryLayer(NewObject<UCanvasPanel>());
	CarryLayer->AddChild(OwnedCards[0].Get());
	FWacomBackpackWorkspaceCarryState Carry;
	Carry.RemainingInstanceIds.Add(OwnedCards[0]->GetCardInstanceId());
	Carry.CurrentIndex = 0;
	Carry.DefaultIndex = 0;
	Controller.Reconcile(
		Cards,
		FGuid(),
		&Carry,
		CarryLayer.Get(),
		FGeometry(),
		FVector2D::ZeroVector);
	const int32 CarryDepthApplyBaseline =
		FirstFace->GetAutomationTestViewForTest().CardDepthApplyCount;
	const int32 CarryRealtimeApplyBaseline =
		FirstFace->GetAutomationTestViewForTest().RealtimePresentationApplyCount;
	FirstFace->SetRealtimePresentationEnabled(true);
	TestEqual(
		TEXT("Equivalent realtime policy does not rewrite the Retainer phase"),
		FirstFace->GetAutomationTestViewForTest().RealtimePresentationApplyCount,
		CarryRealtimeApplyBaseline);
	OwnedCards[0]->SetBackpackRealtimePresentation(
		true,
		FVector2D::ZeroVector,
		true);
	TestEqual(
		TEXT("Equivalent Backpack carry presentation is idempotent"),
		FirstFace->GetAutomationTestViewForTest().CardDepthApplyCount,
		CarryDepthApplyBaseline);
	for (int32 Step = 0; Step < 12; ++Step)
	{
		Controller.UpdatePointer(
			FGeometry(),
			FVector2D(100.0f + Step * 25.0f, 200.0f + Step * 11.0f),
			true);
	}
	TestEqual(
		TEXT("Carry pointer motion does not rewrite the pointer-independent retained card depth"),
		FirstFace->GetAutomationTestViewForTest().CardDepthApplyCount,
		CarryDepthApplyBaseline);
	CarryLayer->RemoveChild(OwnedCards[0].Get());

	Controller.Reconcile(
		Cards,
		OwnedCards[1]->GetCardInstanceId(),
		nullptr,
		nullptr,
		FGeometry(),
		FVector2D::ZeroVector);
	TestFalse(TEXT("Previous dynamic card returns to static redraw mode"),
		FirstFace->GetAutomationTestViewForTest().bRealtimePresentationEnabled);
	TestTrue(TEXT("Realtime budget transfers to the new hover card"),
		SecondFace->GetAutomationTestViewForTest().bRealtimePresentationEnabled);
	Controller.Reset();
	TestFalse(TEXT("Controller reset leaves no realtime Backpack card"),
		SecondFace->GetAutomationTestViewForTest().bRealtimePresentationEnabled);
	return true;
}

#endif
