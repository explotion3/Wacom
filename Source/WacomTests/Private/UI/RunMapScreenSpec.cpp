// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "UI/Map/WacomRunMapNodeWidget.h"
#include "UI/Map/WacomRunMapScreen.h"
#include "UI/RunMapScreenTestAccess.h"

namespace
{
	FWacomRunMapNodeViewData MakeNodeView(
		const FName FloorId,
		const FName NodeId,
		const bool bCanTravel,
		const bool bSelected,
		const FVector2D Position)
	{
		FWacomRunMapNodeViewData Node;
		Node.Handle = { FloorId, NodeId };
		Node.Title = FText::FromName(NodeId);
		Node.Description = FText::Format(
			NSLOCTEXT("WacomTests", "RunMapScreenNodeDescription", "{0} 说明"),
			Node.Title);
		Node.DesignPosition = Position;
		Node.VisualState = bSelected
			? EWacomRunMapNodeVisualState::Current
			: EWacomRunMapNodeVisualState::Resolved;
		Node.bCanSelect = true;
		Node.bCanTravel = bCanTravel;
		Node.bIsSelected = bSelected;
		Node.DisabledReason = bCanTravel
			? FText::GetEmpty()
			: NSLOCTEXT("WacomTests", "RunMapScreenDisabled", "不可传送");
		return Node;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunMapScreenPassiveContractTest,
	"Wacom.UI.RunMap.Screen.PassiveViewDataAndActions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunMapScreenPassiveContractTest::RunTest(const FString& Parameters)
{
	UWacomRunMapScreen* Screen = NewObject<UWacomRunMapScreen>(GetTransientPackage());
	FWacomRunMapScreenTestAccess::BuildAndConstruct(*Screen);

	const FName FloorId(TEXT("Test.Floor.Screen"));
	FWacomRunMapScreenViewData View;
	View.StateVersion = 7;
	View.FloorId = FloorId;
	View.FloorTitle = NSLOCTEXT("WacomTests", "RunMapScreenFloor", "屏幕测试区域");
	View.CurrentNode = { FloorId, TEXT("Node.Current") };
	View.SelectedNode = View.CurrentNode;
	View.DefaultFocusNode = View.CurrentNode;
	View.bIsAvailable = true;
	View.Nodes = {
		MakeNodeView(FloorId, TEXT("Node.Current"), false, true, FVector2D(600.0, 800.0)),
		MakeNodeView(FloorId, TEXT("Node.Travel"), true, false, FVector2D(900.0, 500.0)),
	};
	Screen->ApplyViewData(View);

	TestEqual(TEXT("Fallback creates one widget per visible node"),
		FWacomRunMapScreenTestAccess::GetNodeWidgetCount(*Screen),
		2);
	TestNotNull(TEXT("Default focus resolves the current node"),
		FWacomRunMapScreenTestAccess::GetDesiredFocusTarget(*Screen));
	TestFalse(TEXT("Current node cannot travel"),
		FWacomRunMapScreenTestAccess::IsTravelButtonEnabled(*Screen));

	int32 SelectCount = 0;
	int32 ConfirmCount = 0;
	int32 CloseCount = 0;
	Screen->OnRunMapActionNative.AddLambda(
		[&](const FWacomRunMapScreenActionRequest& Request)
		{
			switch (Request.Action)
			{
			case EWacomRunMapScreenAction::SelectNode:
				++SelectCount;
				break;
			case EWacomRunMapScreenAction::ConfirmTravel:
				++ConfirmCount;
				break;
			case EWacomRunMapScreenAction::Close:
				++CloseCount;
				break;
			}
		});

	TestFalse(TEXT("Disabled current node cannot confirm"), Screen->RequestConfirmTravel());
	TestTrue(TEXT("Visible travel node can be selected"),
		Screen->RequestSelectNode({ FloorId, TEXT("Node.Travel") }));
	TestTrue(TEXT("Travel node enables confirm"),
		FWacomRunMapScreenTestAccess::IsTravelButtonEnabled(*Screen));
	TestTrue(TEXT("Selected travel node can confirm"), Screen->RequestConfirmTravel());
	TestEqual(TEXT("Selection broadcasts once"), SelectCount, 1);
	TestEqual(TEXT("Confirmation broadcasts once"), ConfirmCount, 1);
	TestFalse(TEXT("Unknown programmatic node is rejected"),
		Screen->RequestSelectNode({ FloorId, TEXT("Node.Hidden") }));

	FWacomRunMapScreenViewData Refreshed = View;
	Refreshed.StateVersion = 8;
	Refreshed.SelectedNode = { FloorId, TEXT("Node.Travel") };
	Refreshed.DefaultFocusNode = Refreshed.SelectedNode;
	Refreshed.Nodes[0].bIsSelected = false;
	Refreshed.Nodes[1].bIsSelected = true;
	Refreshed.bCanConfirmTravel = true;
	Screen->ApplyViewData(Refreshed);
	Screen->ApplyViewData(Refreshed);
	TestEqual(TEXT("Repeated Apply does not duplicate node widgets"),
		FWacomRunMapScreenTestAccess::GetNodeWidgetCount(*Screen),
		2);
	UWacomRunMapNodeWidget* SelectedWidget = FWacomRunMapScreenTestAccess::FindNodeWidget(
		*Screen, FloorId, TEXT("Node.Travel"));
	if (TestNotNull(TEXT("Selected node widget remains present"), SelectedWidget))
	{
		TestTrue(TEXT("Focused/hover channel uses the same selected emphasis state"),
			SelectedWidget->GetSelected());
	}

	Screen->RequestClose();
	TestEqual(TEXT("Close intent broadcasts once"), CloseCount, 1);
	FWacomRunMapScreenTestAccess::Destruct(*Screen);
	return true;
}

#endif
