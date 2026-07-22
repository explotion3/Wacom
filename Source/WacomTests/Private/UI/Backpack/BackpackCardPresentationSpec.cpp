// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceMotionCoordinator.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "Cards/CardDefinition.h"
#include "Components/CanvasPanel.h"
#include "Components/ScaleBox.h"
#include "UI/Backpack/WacomBackpackWorkspaceStyle.h"
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
	const UWacomBackpackWorkspaceStyle* Style = GetDefault<UWacomBackpackWorkspaceStyle>();
	TArray<TStrongObjectPtr<UWacomDeckCardWidget>> OwnedCards;
	TArray<TSharedPtr<SWidget>> OwnedCardSlates;
	TArray<TWeakObjectPtr<UWacomDeckCardWidget>> Cards;
	for (int32 Index = 0; Index < 2; ++Index)
	{
		TStrongObjectPtr<UWacomDeckCardWidget> Card(
			NewObject<UWacomDeckCardWidget>(GetTransientPackage(), DeckCardClass));
		OwnedCardSlates.Add(Card->TakeWidget());
		Card->SetBackpackCardDisplayScale(Style->GetSafeCardDisplayScale());
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
	UScaleBox* FirstScaleBox = Cast<UScaleBox>(
		OwnedCards[0]->GetWidgetFromName(TEXT("CardFaceScaleBox")));
	TestNotNull(TEXT("Formal DeckCard exposes its uniform face scale host"), FirstScaleBox);
	if (FirstScaleBox)
	{
		TestTrue(TEXT("DeckCard applies the fixed backpack display scale at runtime"),
			FMath::IsNearlyEqual(
				FirstScaleBox->GetUserSpecifiedScale(),
				Style->GetSafeCardDisplayScale(),
				0.001f));
	}
	if (!FirstFace || !SecondFace)
	{
		return false;
	}
	TestFalse(TEXT("Backpack cards start in static redraw mode"),
		FirstFace->GetAutomationTestViewForTest().bRealtimePresentationEnabled);
	const FWacomFirstPersonCardViewAutomationTestView StableFaceBaseline =
		FirstFace->GetAutomationTestViewForTest();
	FRunStorageCardView EquivalentView;
	EquivalentView.Instance.InstanceId = OwnedCards[0]->GetCardInstanceId();
	EquivalentView.Instance.Definition = Definition.Get();
	EquivalentView.PhysicalZone = EZoneKind::Backpack;
	OwnedCards[0]->SetStorageCardView(EquivalentView);
	OwnedCards[0]->SetMoveEnabled(true);
	OwnedCards[0]->SetBackpackCardFaceRetainedRenderingEnabled(true);
	const FWacomFirstPersonCardViewAutomationTestView StableFaceAfterEquivalentBind =
		FirstFace->GetAutomationTestViewForTest();
	TestEqual(TEXT("Equivalent scene binding does not resubmit card face data"),
		StableFaceAfterEquivalentBind.CardViewDataApplyCount,
		StableFaceBaseline.CardViewDataApplyCount);
	TestEqual(TEXT("Equivalent scene binding does not reapply Retainer capture mode"),
		StableFaceAfterEquivalentBind.RetainedRenderingApplyCount,
		StableFaceBaseline.RetainedRenderingApplyCount);

	FWacomBackpackWorkspaceMotionCoordinator Controller;
	Controller.Reconcile(
		Cards,
		OwnedCards[0].Get(),
		nullptr,
		nullptr,
		FGeometry(),
		FVector2D::ZeroVector,
		*Style,
		false);
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
		nullptr,
		&Carry,
		CarryLayer.Get(),
		FGeometry(),
		FVector2D::ZeroVector,
		*Style,
		false);
	const int32 CarryDepthApplyBaseline =
		FirstFace->GetAutomationTestViewForTest().CardDepthApplyCount;
	const int32 CarryRealtimeApplyBaseline =
		FirstFace->GetAutomationTestViewForTest().RealtimePresentationApplyCount;
	FirstFace->SetRealtimePresentationEnabled(true);
	TestEqual(
		TEXT("Equivalent realtime policy does not rewrite the Retainer phase"),
		FirstFace->GetAutomationTestViewForTest().RealtimePresentationApplyCount,
		CarryRealtimeApplyBaseline);
	for (int32 Step = 0; Step < 12; ++Step)
	{
		Controller.UpdatePointer(
			FGeometry(),
			FVector2D(100.0f + Step * 25.0f, 200.0f + Step * 11.0f),
			true);
		Controller.Tick(1.0f / 60.0f, FGeometry(), *Style, false);
	}
	TestTrue(
		TEXT("Carry pointer motion updates the active card velocity depth"),
		FirstFace->GetAutomationTestViewForTest().CardDepthApplyCount > CarryDepthApplyBaseline);
	TestEqual(TEXT("Carry keeps a single realtime Retainer budget"),
		Controller.GetRealtimeCardCount(), 1);
	CarryLayer->RemoveChild(OwnedCards[0].Get());

	OwnedCards[1]->SetWorkspaceInteractionEnabled(false);
	OwnedCards[1]->SetWorkspaceReadOnlyKind(
		EWacomBackpackWorkspaceCardReadOnlyKind::BattleProjection);
	Controller.Reconcile(
		Cards,
		OwnedCards[1].Get(),
		nullptr,
		nullptr,
		FGeometry(),
		FVector2D::ZeroVector,
		*Style,
		false);
	TestFalse(TEXT("Previous dynamic card returns to static redraw mode"),
		FirstFace->GetAutomationTestViewForTest().bRealtimePresentationEnabled);
	TestTrue(TEXT("Realtime budget transfers to the exact read-only browse focus"),
		SecondFace->GetAutomationTestViewForTest().bRealtimePresentationEnabled);
	Controller.Reset();
	TestFalse(TEXT("Controller reset leaves no realtime Backpack card"),
		SecondFace->GetAutomationTestViewForTest().bRealtimePresentationEnabled);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardLocalPoseMotionSpec,
	"Wacom.UI.Backpack.CardView.LocalPoseMotion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardLocalPoseMotionSpec::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UWacomDeckCardWidget> Card(NewObject<UWacomDeckCardWidget>());
	FWacomBackpackWorkspaceMotionCoordinator Controller;
	const UWacomBackpackWorkspaceStyle* Style = GetDefault<UWacomBackpackWorkspaceStyle>();
	const FVector2D InitialScale = Card->GetRenderTransform().Scale;
	const float InitialOpacity = Card->GetRenderOpacity();

	Controller.SetLocalPoseTarget(
		*Card,
		FVector2D(0.0f, -Style->ExpandedCardHoverLiftPixels),
		-12.0f,
		Style->HoverEnterSeconds,
		false);
	Controller.Tick(Style->HoverEnterSeconds * 0.5f, FGeometry(), *Style, false);
	TestTrue(TEXT("Hover local lift is in flight at half duration"),
		Card->GetBackpackLocalMotionTranslation().Y < 0.0f
			&& Card->GetBackpackLocalMotionTranslation().Y > -Style->ExpandedCardHoverLiftPixels);
	TestTrue(TEXT("Hover local rotation is in flight at half duration"),
		Card->GetBackpackLocalMotionAngle() < 0.0f
			&& Card->GetBackpackLocalMotionAngle() > -12.0f);
	Controller.Tick(Style->HoverEnterSeconds * 0.5f, FGeometry(), *Style, false);
	TestTrue(TEXT("Hover reaches the authored 48px lift"),
		Card->GetBackpackLocalMotionTranslation().Equals(
			FVector2D(0.0f, -Style->ExpandedCardHoverLiftPixels), 0.01f));
	TestTrue(TEXT("Hover reaches the angle compensation target"),
		FMath::IsNearlyEqual(Card->GetBackpackLocalMotionAngle(), -12.0f, 0.01f));
	TestTrue(TEXT("Local motion never changes card scale"),
		Card->GetRenderTransform().Scale.Equals(InitialScale));
	TestTrue(TEXT("Local motion never changes card opacity"),
		FMath::IsNearlyEqual(Card->GetRenderOpacity(), InitialOpacity));

	Controller.BeginSettlement(
		*Card,
		FVector2D(-140.0f, 35.0f),
		18.0f,
		Style->SettleSeconds,
		false);
	Controller.Tick(Style->SettleSeconds * 0.5f, FGeometry(), *Style, false);
	Controller.Tick(Style->SettleSeconds * 0.5f, FGeometry(), *Style, false);
	TArray<TWeakObjectPtr<UWacomDeckCardWidget>> Completed;
	Controller.ConsumeCompletedSettlements(Completed);
	TestEqual(TEXT("Settlement reports exactly one completed original Widget"),
		Completed.Num(), 1);
	TestTrue(TEXT("Settlement finishes at zero local translation"),
		Card->GetBackpackLocalMotionTranslation().IsNearlyZero(0.01f));
	TestTrue(TEXT("Settlement finishes at zero local angle"),
		FMath::IsNearlyZero(Card->GetBackpackLocalMotionAngle(), 0.01f));

	Controller.SetLocalPoseTarget(
		*Card,
		FVector2D(0.0f, -Style->CurrentCardLiftPixels),
		7.0f,
		Style->CarryCurrentTransitionSeconds,
		true);
	TestTrue(TEXT("Simplified motion snaps directly to its final pose"),
		Card->GetBackpackLocalMotionTranslation().Equals(
			FVector2D(0.0f, -Style->CurrentCardLiftPixels), 0.01f));
	Controller.Reset();
	return true;
}

#endif
