// Copyright Wacom. All Rights Reserved.

#include "UI/Map/WacomRunMapScreen.h"

#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "InputCoreTypes.h"
#include "UI/Foundation/WacomMenuButtonWidget.h"
#include "UI/Map/WacomRunMapEdgeLayerWidget.h"
#include "UI/Map/WacomRunMapNodeWidget.h"

#define LOCTEXT_NAMESPACE "WacomRunMapScreen"

namespace
{
	UWacomMenuButtonWidget* MakeActionButton(
		UWidgetTree& Tree,
		UVerticalBox& Parent,
		const FName Name,
		const FText& Label)
	{
		UWacomMenuButtonWidget* Button = Tree.ConstructWidget<UWacomMenuButtonWidget>(
			UWacomMenuButtonWidget::StaticClass(),
			Name);
		Button->SetButtonText(Label);
		if (UVerticalBoxSlot* Slot = Parent.AddChildToVerticalBox(Button))
		{
			Slot->SetPadding(FMargin(0.0f, 6.0f));
			Slot->SetHorizontalAlignment(HAlign_Fill);
		}
		return Button;
	}
}

UWacomRunMapScreen::UWacomRunMapScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeWidgetClass = UWacomRunMapNodeWidget::StaticClass();
}

TSharedRef<SWidget> UWacomRunMapScreen::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
	}
	if (!WidgetTree->RootWidget)
	{
		UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RootBorder"));
		Root->SetPadding(FMargin(28.0f));
		Root->SetBrushColor(FLinearColor(0.015f, 0.025f, 0.04f, 0.97f));
		WidgetTree->RootWidget = Root;

		UVerticalBox* RootColumn = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("RootColumn"));
		Root->AddChild(RootColumn);

		FloorTitleText = WidgetTree->ConstructWidget<UCommonTextBlock>(
			UCommonTextBlock::StaticClass(), TEXT("FloorTitleText"));
		FloorTitleText->SetText(LOCTEXT("MapTitleFallback", "当前区域"));
		RootColumn->AddChildToVerticalBox(FloorTitleText);

		UHorizontalBox* ContentRow = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), TEXT("ContentRow"));
		if (UVerticalBoxSlot* ContentSlot = RootColumn->AddChildToVerticalBox(ContentRow))
		{
			ContentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			ContentSlot->SetPadding(FMargin(0.0f, 16.0f));
		}

		MapViewportScaleBox = WidgetTree->ConstructWidget<UScaleBox>(
			UScaleBox::StaticClass(), TEXT("MapViewportScaleBox"));
		MapViewportScaleBox->SetStretch(EStretch::ScaleToFit);
		if (UHorizontalBoxSlot* MapSlot = ContentRow->AddChildToHorizontalBox(MapViewportScaleBox))
		{
			MapSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			MapSlot->SetPadding(FMargin(0.0f, 0.0f, 18.0f, 0.0f));
		}

		USizeBox* DesignCanvasSize = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("DesignCanvasSize"));
		DesignCanvasSize->SetWidthOverride(1920.0f);
		DesignCanvasSize->SetHeightOverride(1080.0f);
		MapViewportScaleBox->AddChild(DesignCanvasSize);

		UOverlay* MapOverlay = WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(), TEXT("MapOverlay"));
		DesignCanvasSize->AddChild(MapOverlay);

		EdgeLayer = WidgetTree->ConstructWidget<UWacomRunMapEdgeLayerWidget>(
			UWacomRunMapEdgeLayerWidget::StaticClass(), TEXT("EdgeLayer"));
		MapOverlay->AddChild(EdgeLayer);

		MapCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("MapCanvas"));
		MapOverlay->AddChild(MapCanvas);

		USizeBox* DetailWidth = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("DetailWidth"));
		DetailWidth->SetWidthOverride(360.0f);
		ContentRow->AddChildToHorizontalBox(DetailWidth);

		UVerticalBox* DetailColumn = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("DetailColumn"));
		DetailWidth->AddChild(DetailColumn);

		SelectedNodeTitleText = WidgetTree->ConstructWidget<UCommonTextBlock>(
			UCommonTextBlock::StaticClass(), TEXT("SelectedNodeTitleText"));
		DetailColumn->AddChildToVerticalBox(SelectedNodeTitleText);
		SelectedNodeDescriptionText = WidgetTree->ConstructWidget<UCommonTextBlock>(
			UCommonTextBlock::StaticClass(), TEXT("SelectedNodeDescriptionText"));
		if (UVerticalBoxSlot* DescriptionSlot =
			DetailColumn->AddChildToVerticalBox(SelectedNodeDescriptionText))
		{
			DescriptionSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			DescriptionSlot->SetPadding(FMargin(0.0f, 12.0f));
		}
		StatusText = WidgetTree->ConstructWidget<UCommonTextBlock>(
			UCommonTextBlock::StaticClass(), TEXT("StatusText"));
		DetailColumn->AddChildToVerticalBox(StatusText);
		TravelButton = MakeActionButton(
			*WidgetTree, *DetailColumn, TEXT("TravelButton"), LOCTEXT("Travel", "传送"));
		CloseButton = MakeActionButton(
			*WidgetTree, *DetailColumn, TEXT("CloseButton"), LOCTEXT("Close", "关闭"));
	}

	return Super::RebuildWidget();
}

void UWacomRunMapScreen::NativeConstruct()
{
	Super::NativeConstruct();
	if (TravelButton)
	{
		TravelButton->OnClicked().RemoveAll(this);
		TravelClickedHandle = TravelButton->OnClicked().AddUObject(
			this, &UWacomRunMapScreen::HandleTravelClicked);
	}
	if (CloseButton)
	{
		CloseButton->OnClicked().RemoveAll(this);
		CloseClickedHandle = CloseButton->OnClicked().AddUObject(
			this, &UWacomRunMapScreen::HandleCloseClicked);
	}
	ApplyViewData(ViewData);
}

void UWacomRunMapScreen::NativeDestruct()
{
	// CommonUI normally deactivates before destruct. Direct world/viewport teardown may skip
	// that phase, so notify the Flow before clearing its native delegate.
	OnRunMapDeactivatedNative.Broadcast();
	if (TravelButton)
	{
		TravelButton->OnClicked().RemoveAll(this);
	}
	if (CloseButton)
	{
		CloseButton->OnClicked().RemoveAll(this);
	}
	TravelClickedHandle.Reset();
	CloseClickedHandle.Reset();
	for (UWacomRunMapNodeWidget* NodeWidget : NodeWidgets)
	{
		if (NodeWidget)
		{
			NodeWidget->OnNodeSelectedNative.Clear();
			NodeWidget->OnNodeConfirmRequestedNative.Clear();
		}
	}
	NodeWidgets.Reset();
	OnRunMapActionNative.Clear();
	OnRunMapDeactivatedNative.Clear();
	Super::NativeDestruct();
}

void UWacomRunMapScreen::NativeOnActivated()
{
	Super::NativeOnActivated();
	if (UWidget* FocusTarget = NativeGetDesiredFocusTarget())
	{
		FocusTarget->SetFocus();
	}
}

void UWacomRunMapScreen::NativeOnDeactivated()
{
	OnRunMapDeactivatedNative.Broadcast();
	Super::NativeOnDeactivated();
}

FReply UWacomRunMapScreen::NativeOnKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::M || Key == EKeys::B || Key == EKeys::Gamepad_Special_Left)
	{
		RequestClose();
		return FReply::Handled();
	}
	if (Key == EKeys::E || Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		RequestConfirmTravel();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UWacomRunMapScreen::NativeHandleBackRequested()
{
	RequestClose();
	return FReply::Handled();
}

UWidget* UWacomRunMapScreen::NativeGetDesiredFocusTarget() const
{
	if (UWacomRunMapNodeWidget* NodeWidget = FindNodeWidget(ViewData.DefaultFocusNode))
	{
		return NodeWidget;
	}
	return CloseButton;
}

void UWacomRunMapScreen::ApplyViewData(const FWacomRunMapScreenViewData& InViewData)
{
	ViewData = InViewData;
	if (FloorTitleText)
	{
		FloorTitleText->SetText(ViewData.FloorTitle);
	}
	if (EdgeLayer)
	{
		EdgeLayer->ApplyEdges(ViewData.Edges);
	}
	RebuildNodeWidgets();
	RefreshSelectionPresentation();
	BP_OnViewDataApplied(ViewData);
	if (IsActivated())
	{
		if (UWidget* FocusTarget = NativeGetDesiredFocusTarget())
		{
			FocusTarget->SetKeyboardFocus();
		}
	}
}

bool UWacomRunMapScreen::RequestSelectNode(const FWacomMapNodeHandle& Node)
{
	const FWacomRunMapNodeViewData* Candidate = FindNodeViewData(Node);
	if (!Candidate || !Candidate->bCanSelect || ViewData.StateVersion <= 0)
	{
		return false;
	}

	ViewData.SelectedNode = Node;
	ViewData.DefaultFocusNode = Node;
	ViewData.StatusText = FText::GetEmpty();
	RefreshSelectionPresentation();
	FWacomRunMapScreenActionRequest Request;
	Request.Action = EWacomRunMapScreenAction::SelectNode;
	Request.Node = Node;
	Request.SourceStateVersion = ViewData.StateVersion;
	OnRunMapActionNative.Broadcast(Request);
	return true;
}

bool UWacomRunMapScreen::RequestConfirmTravel()
{
	const FWacomRunMapNodeViewData* Selected = FindNodeViewData(ViewData.SelectedNode);
	if (!Selected || !Selected->bCanTravel || !ViewData.bCanConfirmTravel)
	{
		return false;
	}

	FWacomRunMapScreenActionRequest Request;
	Request.Action = EWacomRunMapScreenAction::ConfirmTravel;
	Request.Node = Selected->Handle;
	Request.SourceStateVersion = ViewData.StateVersion;
	OnRunMapActionNative.Broadcast(Request);
	return true;
}

void UWacomRunMapScreen::RequestClose()
{
	FWacomRunMapScreenActionRequest Request;
	Request.Action = EWacomRunMapScreenAction::Close;
	Request.SourceStateVersion = ViewData.StateVersion;
	if (OnRunMapActionNative.IsBound())
	{
		OnRunMapActionNative.Broadcast(Request);
	}
	else
	{
		DeactivateWidget();
	}
}

void UWacomRunMapScreen::RebuildNodeWidgets()
{
	if (!MapCanvas || !WidgetTree)
	{
		return;
	}
	MapCanvas->ClearChildren();
	NodeWidgets.Reset();
	UClass* ResolvedNodeClass = NodeWidgetClass
		? NodeWidgetClass.Get()
		: UWacomRunMapNodeWidget::StaticClass();
	for (const FWacomRunMapNodeViewData& Node : ViewData.Nodes)
	{
		UWacomRunMapNodeWidget* NodeWidget = WidgetTree->ConstructWidget<UWacomRunMapNodeWidget>(
			ResolvedNodeClass,
			MakeUniqueObjectName(this, ResolvedNodeClass, TEXT("RunMapNode")));
		if (!NodeWidget)
		{
			continue;
		}
		NodeWidget->ApplyViewData(Node);
		NodeWidget->OnNodeSelectedNative.AddUObject(this, &UWacomRunMapScreen::HandleNodeSelected);
		NodeWidget->OnNodeConfirmRequestedNative.AddUObject(
			this, &UWacomRunMapScreen::HandleNodeConfirmRequested);
		if (UCanvasPanelSlot* CanvasSlot = MapCanvas->AddChildToCanvas(NodeWidget))
		{
			CanvasSlot->SetAutoSize(false);
			CanvasSlot->SetSize(FVector2D(180.0f, 72.0f));
			CanvasSlot->SetPosition(Node.DesignPosition - FVector2D(90.0f, 36.0f));
		}
		NodeWidgets.Add(NodeWidget);
	}
}

void UWacomRunMapScreen::RefreshSelectionPresentation()
{
	const FText ExplicitStatus = ViewData.StatusText;
	const FWacomRunMapNodeViewData* Selected = nullptr;
	for (FWacomRunMapNodeViewData& Node : ViewData.Nodes)
	{
		Node.bIsSelected = Node.Handle == ViewData.SelectedNode;
		if (Node.bIsSelected)
		{
			Selected = &Node;
		}
	}

	ViewData.bCanConfirmTravel = ViewData.bIsAvailable && Selected && Selected->bCanTravel;
	ViewData.SelectedNodeTitle = Selected ? Selected->Title : FText::GetEmpty();
	ViewData.SelectedNodeDescription = Selected ? Selected->Description : FText::GetEmpty();
	ViewData.StatusText = !ExplicitStatus.IsEmpty()
		? ExplicitStatus
		: (Selected ? Selected->DisabledReason : FText::GetEmpty());

	for (UWacomRunMapNodeWidget* NodeWidget : NodeWidgets)
	{
		if (NodeWidget)
		{
			if (const FWacomRunMapNodeViewData* Node = FindNodeViewData(NodeWidget->GetViewData().Handle))
			{
				NodeWidget->ApplyViewData(*Node);
			}
		}
	}
	if (SelectedNodeTitleText)
	{
		SelectedNodeTitleText->SetText(ViewData.SelectedNodeTitle);
	}
	if (SelectedNodeDescriptionText)
	{
		SelectedNodeDescriptionText->SetText(ViewData.SelectedNodeDescription);
	}
	if (StatusText)
	{
		StatusText->SetText(ViewData.StatusText);
	}
	if (TravelButton)
	{
		TravelButton->SetIsInteractionEnabled(ViewData.bCanConfirmTravel);
		TravelButton->RefreshPresentationState();
	}
}

void UWacomRunMapScreen::HandleTravelClicked()
{
	RequestConfirmTravel();
}

void UWacomRunMapScreen::HandleCloseClicked()
{
	RequestClose();
}

void UWacomRunMapScreen::HandleNodeSelected(const FWacomMapNodeHandle& Node)
{
	RequestSelectNode(Node);
}

void UWacomRunMapScreen::HandleNodeConfirmRequested(const FWacomMapNodeHandle& Node)
{
	if (ViewData.SelectedNode != Node && !RequestSelectNode(Node))
	{
		return;
	}
	RequestConfirmTravel();
}

UWacomRunMapNodeWidget* UWacomRunMapScreen::FindNodeWidget(
	const FWacomMapNodeHandle& Node) const
{
	const TObjectPtr<UWacomRunMapNodeWidget>* Found = NodeWidgets.FindByPredicate(
		[&Node](const UWacomRunMapNodeWidget* Widget)
		{
			return Widget && Widget->GetViewData().Handle == Node;
		});
	return Found ? Found->Get() : nullptr;
}

const FWacomRunMapNodeViewData* UWacomRunMapScreen::FindNodeViewData(
	const FWacomMapNodeHandle& Node) const
{
	return ViewData.Nodes.FindByPredicate(
		[&Node](const FWacomRunMapNodeViewData& Candidate)
		{
			return Candidate.Handle == Node;
		});
}

#undef LOCTEXT_NAMESPACE
