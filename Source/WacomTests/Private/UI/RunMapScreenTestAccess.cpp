// Copyright Wacom. All Rights Reserved.

#include "UI/RunMapScreenTestAccess.h"

#include "UI/Foundation/WacomMenuButtonWidget.h"
#include "UI/Map/WacomRunMapNodeWidget.h"
#include "UI/Map/WacomRunMapScreen.h"
#include "InputCoreTypes.h"

void FWacomRunMapScreenTestAccess::BuildAndConstruct(UWacomRunMapScreen& Screen)
{
	Screen.TakeWidget();
	Screen.NativeConstruct();
}

void FWacomRunMapScreenTestAccess::Destruct(UWacomRunMapScreen& Screen)
{
	Screen.NativeDestruct();
}

int32 FWacomRunMapScreenTestAccess::GetNodeWidgetCount(
	const UWacomRunMapScreen& Screen)
{
	return Screen.NodeWidgets.Num();
}

UWacomRunMapNodeWidget* FWacomRunMapScreenTestAccess::FindNodeWidget(
	const UWacomRunMapScreen& Screen,
	const FName FloorId,
	const FName NodeId)
{
	return Screen.FindNodeWidget({ FloorId, NodeId });
}

UWidget* FWacomRunMapScreenTestAccess::GetDesiredFocusTarget(
	const UWacomRunMapScreen& Screen)
{
	return Screen.NativeGetDesiredFocusTarget();
}

bool FWacomRunMapScreenTestAccess::IsTravelButtonEnabled(
	const UWacomRunMapScreen& Screen)
{
	return Screen.TravelButton && Screen.TravelButton->IsInteractionEnabled();
}

FReply FWacomRunMapScreenTestAccess::PressKey(
	UWacomRunMapScreen& Screen,
	const FKey& Key)
{
	const FKeyEvent Event(Key, FModifierKeysState(), 0, false, 0, 0);
	return Screen.NativeOnKeyDown(FGeometry(), Event);
}

bool FWacomRunMapScreenTestAccess::HasRequiredBindings(
	const UWacomRunMapScreen& Screen)
{
	return Screen.MapViewportScaleBox
		&& Screen.MapCanvas
		&& Screen.EdgeLayer
		&& Screen.FloorTitleText
		&& Screen.SelectedNodeTitleText
		&& Screen.SelectedNodeDescriptionText
		&& Screen.StatusText
		&& Screen.TravelButton
		&& Screen.CloseButton;
}

UClass* FWacomRunMapScreenTestAccess::GetNodeWidgetClass(
	const UWacomRunMapScreen& Screen)
{
	return Screen.NodeWidgetClass.Get();
}
