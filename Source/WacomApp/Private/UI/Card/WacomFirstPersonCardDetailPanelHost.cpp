// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardDetailPanelHost.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UI/Card/WacomCardDetailPanel.h"
#include "UI/Card/WacomFirstPersonCardDetailMotionController.h"

UWacomCardDetailPanel* FWacomFirstPersonCardDetailPanelHost::EnsurePanel(
	TObjectPtr<UWacomCardDetailPanel>& PanelSlot,
	const FWacomFirstPersonCardDetailPanelHostContext& Context,
	FWacomFirstPersonCardDetailMotionController& MotionController,
	const FWacomFirstPersonCardDetailMotionConfig& MotionConfig)
{
	const bool bHadPanel = PanelSlot != nullptr;
	if (!PanelSlot)
	{
		UClass* PanelClass = Context.PanelClass
			? Context.PanelClass.Get()
			: UWacomCardDetailPanel::StaticClass();
		if (Context.OwningPlayer
			&& Context.OwningPlayer->IsLocalController()
			&& Context.OwningPlayer->GetLocalPlayer())
		{
			PanelSlot = CreateWidget<UWacomCardDetailPanel>(
				Context.OwningPlayer,
				PanelClass);
		}
		if (!PanelSlot && Context.World)
		{
			PanelSlot = CreateWidget<UWacomCardDetailPanel>(
				Context.World,
				PanelClass);
		}
		if (!PanelSlot && Context.Outer)
		{
			PanelSlot = NewObject<UWacomCardDetailPanel>(
				Context.Outer,
				PanelClass);
		}
	}

	if (!PanelSlot)
	{
		return nullptr;
	}

	if (!bHadPanel)
	{
		MotionController.PrewarmPanel(*PanelSlot, MotionConfig);
	}
	AddPanelToViewportIfNeeded(*PanelSlot, Context);
	return PanelSlot;
}

void FWacomFirstPersonCardDetailPanelHost::PrewarmPanel(
	TObjectPtr<UWacomCardDetailPanel>& PanelSlot,
	const FWacomFirstPersonCardDetailPanelHostContext& Context,
	FWacomFirstPersonCardDetailMotionController& MotionController,
	const FWacomFirstPersonCardDetailMotionConfig& MotionConfig)
{
	if (UWacomCardDetailPanel* Panel =
		EnsurePanel(PanelSlot, Context, MotionController, MotionConfig))
	{
		MotionController.PrewarmPanel(*Panel, MotionConfig);
		AddPanelToViewportIfNeeded(*Panel, Context);
	}
}

void FWacomFirstPersonCardDetailPanelHost::AddPanelToViewportIfNeeded(
	UWacomCardDetailPanel& Panel,
	const FWacomFirstPersonCardDetailPanelHostContext& Context)
{
	if (Context.bCanAddToViewport && !Panel.IsInViewport())
	{
		Panel.AddToViewport(Context.ViewportZOrder);
	}
}

void FWacomFirstPersonCardDetailPanelHost::RemovePanelFromViewport(
	TObjectPtr<UWacomCardDetailPanel>& PanelSlot,
	FWacomFirstPersonCardDetailMotionController& MotionController)
{
	MotionController.ForceHideAll(PanelSlot);
	if (PanelSlot)
	{
		PanelSlot->RemoveFromParent();
		PanelSlot = nullptr;
	}
}

FVector2D FWacomFirstPersonCardDetailPanelHost::GetViewportSize(
	const FWacomFirstPersonCardDetailPanelHostContext& Context)
{
	FVector2D ViewportPixelSize = FVector2D::ZeroVector;
	if (const UWorld* World = Context.World)
	{
		if (const UGameViewportClient* GameViewport = World->GetGameViewport())
		{
			GameViewport->GetViewportSize(ViewportPixelSize);
		}
	}

	if ((ViewportPixelSize.X <= 0.0f || ViewportPixelSize.Y <= 0.0f)
		&& Context.OwningPlayer)
	{
		if (const ULocalPlayer* LocalPlayer = Context.OwningPlayer->GetLocalPlayer())
		{
			if (const UGameViewportClient* ViewportClient = LocalPlayer->ViewportClient)
			{
				ViewportClient->GetViewportSize(ViewportPixelSize);
			}
		}
	}

	if ((ViewportPixelSize.X <= 0.0f || ViewportPixelSize.Y <= 0.0f)
		&& Context.OwningPlayer)
	{
		int32 ViewportX = 0;
		int32 ViewportY = 0;
		Context.OwningPlayer->GetViewportSize(ViewportX, ViewportY);
		ViewportPixelSize = FVector2D(ViewportX, ViewportY);
	}

	if (ViewportPixelSize.X <= 0.0f || ViewportPixelSize.Y <= 0.0f)
	{
		ViewportPixelSize = FVector2D(1920.0f, 1080.0f);
	}

	float ViewportScale = 1.0f;
	if (Context.OwningPlayer)
	{
		ViewportScale = UWidgetLayoutLibrary::GetViewportScale(Context.OwningPlayer);
	}
	ViewportScale = FMath::Max(0.01f, ViewportScale);
	return ViewportPixelSize / ViewportScale;
}
