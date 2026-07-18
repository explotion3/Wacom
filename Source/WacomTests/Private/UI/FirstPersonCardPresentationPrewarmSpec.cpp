// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Sound/SoundBase.h"
#include "UI/Card/WacomFirstPersonCardViewWidget.h"
#include "../../../WacomApp/Private/UI/Card/WacomFirstPersonCardPresentationAssetCollector.h"
#include "../../../WacomApp/Private/UI/Card/WacomFirstPersonCardPresentationPrewarmController.h"

#if WITH_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardPresentationPrewarmControllerTest,
	"Wacom.UI.FirstPersonCardLayer.PresentationPrewarm.Controller",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardPresentationPrewarmControllerTest::RunTest(
	const FString& /*Parameters*/)
{
	FWacomFirstPersonCardPresentationPrewarmController Controller;
	FWacomFirstPersonCardPresentationPrewarmRequest EmptyRequest;
	EmptyRequest.ScopeId = TEXT("Empty");
	const uint32 EmptyGeneration = Controller.Begin(EmptyRequest);
	FWacomFirstPersonCardPresentationPrewarmDebugView View = Controller.BuildDebugView();
	TestEqual(TEXT("An empty required set is immediately ready"), View.State,
		EWacomFirstPersonCardPresentationPrewarmState::Ready);
	TestTrue(TEXT("The empty request resolves its gate"), View.bGateResolved);
	uint32 ResolvedGeneration = 0;
	EWacomFirstPersonCardPresentationPrewarmState ResolvedState =
		EWacomFirstPersonCardPresentationPrewarmState::Inactive;
	TestTrue(TEXT("The ready edge is consumed once"),
		Controller.ConsumeGateResolution(ResolvedGeneration, ResolvedState));
	TestEqual(TEXT("The ready edge keeps its generation"), ResolvedGeneration, EmptyGeneration);
	TestFalse(TEXT("The ready edge is not replayed"),
		Controller.ConsumeGateResolution(ResolvedGeneration, ResolvedState));

	const FSoftObjectPath RequiredPath(
		TEXT("/Engine/EngineResources/DefaultTexture.DefaultTexture"));
	const FSoftObjectPath OptionalPath(
		TEXT("/Game/Wacom/Tests/MissingOptionalSound.MissingOptionalSound"));
	FWacomFirstPersonCardPresentationPrewarmRequest DeferredRequest;
	DeferredRequest.ScopeId = TEXT("Deferred");
	DeferredRequest.RequiredVisualAssets = {RequiredPath, RequiredPath};
	DeferredRequest.OptionalAudioAssets = {OptionalPath, OptionalPath};
	DeferredRequest.TimeoutSeconds = 1.5f;
	DeferredRequest.bDeferRequiredCompletionForTest = true;
	const uint32 DeferredGeneration = Controller.Begin(DeferredRequest);
	View = Controller.BuildDebugView();
	TestEqual(TEXT("Required paths are stable-deduplicated"), View.RequiredAssetCount, 1);
	TestEqual(TEXT("Optional paths are stable-deduplicated"), View.OptionalAssetCount, 1);
	Controller.Tick(1.49f);
	TestEqual(TEXT("The request remains loading before the limit"),
		Controller.BuildDebugView().State,
		EWacomFirstPersonCardPresentationPrewarmState::Loading);
	Controller.Tick(0.01f);
	View = Controller.BuildDebugView();
	TestEqual(TEXT("The fixed limit resolves as timed out"), View.State,
		EWacomFirstPersonCardPresentationPrewarmState::TimedOut);
	TestTrue(TEXT("The timeout is recorded as the gate reason"), View.bGateResolvedByTimeout);
	TestTrue(TEXT("The timeout publishes one release edge"),
		Controller.ConsumeGateResolution(ResolvedGeneration, ResolvedState));
	TestEqual(TEXT("The timeout edge keeps its generation"), ResolvedGeneration, DeferredGeneration);

	const uint32 ReplacementGeneration = Controller.Begin(EmptyRequest);
	TestTrue(TEXT("A replacement request advances the generation"),
		ReplacementGeneration > DeferredGeneration);
	TestEqual(TEXT("A replacement request is independently ready"),
		Controller.BuildDebugView().State,
		EWacomFirstPersonCardPresentationPrewarmState::Ready);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardPresentationAssetCollectorTest,
	"Wacom.UI.FirstPersonCardLayer.PresentationPrewarm.AssetCollector",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardPresentationAssetCollectorTest::RunTest(
	const FString& /*Parameters*/)
{
	UClass* FirstPersonCardViewClass = LoadClass<UWacomFirstPersonCardViewWidget>(
		nullptr,
		TEXT("/Game/Wacom/UI/Card/WBP_FPCardView.WBP_FPCardView_C"));
	TestNotNull(TEXT("The production first-person card class loads"), FirstPersonCardViewClass);
	if (!FirstPersonCardViewClass)
	{
		return false;
	}

	UWacomFirstPersonCardAnchorComponent* Anchor =
		NewObject<UWacomFirstPersonCardAnchorComponent>();
	Anchor->FirstPersonCardViewClass = FirstPersonCardViewClass;
	Anchor->DrawnCardEnterSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(
		TEXT("/Game/Wacom/Tests/PresentationPrewarmSound.PresentationPrewarmSound")));

	const FWacomFirstPersonCardPresentationAssetCollection Collection =
		FWacomFirstPersonCardPresentationAssetCollector::Collect(*Anchor);
	TestTrue(TEXT("The collector visits the wrapper widget class"),
		Collection.VisitedWidgetClassCount > 0);
	TestTrue(TEXT("The collector finds the nested authored CardView"),
		Collection.VisitedCardViewTemplateCount > 0);
	TestTrue(TEXT("The collector finds authored visual soft resources"),
		Collection.RequiredVisualAssets.Num() > 0);
	TestEqual(TEXT("The authored optional sound path is collected once"),
		Collection.OptionalAudioAssets.Num(), 1);
	return true;
}

#endif
