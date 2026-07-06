// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"

class APlayerController;
class UWacomCardDetailPanel;
class UWorld;
class UObject;
class FWacomFirstPersonCardDetailMotionController;
struct FWacomFirstPersonCardDetailMotionConfig;

struct FWacomFirstPersonCardDetailPanelHostContext
{
	UObject* Outer = nullptr;
	APlayerController* OwningPlayer = nullptr;
	UWorld* World = nullptr;
	TSubclassOf<UWacomCardDetailPanel> PanelClass;
	int32 ViewportZOrder = 9999;
	bool bCanAddToViewport = true;
};

class FWacomFirstPersonCardDetailPanelHost
{
public:
	static UWacomCardDetailPanel* EnsurePanel(
		TObjectPtr<UWacomCardDetailPanel>& PanelSlot,
		const FWacomFirstPersonCardDetailPanelHostContext& Context,
		FWacomFirstPersonCardDetailMotionController& MotionController,
		const FWacomFirstPersonCardDetailMotionConfig& MotionConfig);

	static void PrewarmPanel(
		TObjectPtr<UWacomCardDetailPanel>& PanelSlot,
		const FWacomFirstPersonCardDetailPanelHostContext& Context,
		FWacomFirstPersonCardDetailMotionController& MotionController,
		const FWacomFirstPersonCardDetailMotionConfig& MotionConfig);

	static void AddPanelToViewportIfNeeded(
		UWacomCardDetailPanel& Panel,
		const FWacomFirstPersonCardDetailPanelHostContext& Context);

	static void RemovePanelFromViewport(
		TObjectPtr<UWacomCardDetailPanel>& PanelSlot,
		FWacomFirstPersonCardDetailMotionController& MotionController);

	static FVector2D GetViewportSize(
		const FWacomFirstPersonCardDetailPanelHostContext& Context);
};
