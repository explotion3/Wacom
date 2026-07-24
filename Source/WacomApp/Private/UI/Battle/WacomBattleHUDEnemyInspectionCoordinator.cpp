// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleHUDEnemyInspectionCoordinator.h"

#include "Blueprint/UserWidget.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "GameFramework/PlayerController.h"
#include "UI/Battle/WacomBattleEnemyInspectionWidget.h"
#include "UI/Battle/WacomBattleHUDRuntime.h"
#include "UI/Battle/WacomBattleViewportLayerPolicy.h"
#include "UI/Foundation/WacomUIDeveloperSettings.h"

namespace
{
	bool IsConstructibleInspectionClass(const UClass* WidgetClass)
	{
		return WidgetClass
			&& WidgetClass->IsChildOf(UWacomBattleEnemyInspectionWidget::StaticClass())
			&& !WidgetClass->HasAnyClassFlags(
				CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists);
	}
}

FWacomBattleHUDEnemyInspectionCoordinator::FWacomBattleHUDEnemyInspectionCoordinator(
	FWacomBattleHUDRuntime& InRuntime)
	: Runtime(InRuntime)
{
}

FWacomBattleHUDEnemyInspectionCoordinator::~FWacomBattleHUDEnemyInspectionCoordinator()
{
	Shutdown();
}

bool FWacomBattleHUDEnemyInspectionCoordinator::ToggleInspection(
	const FWacomBattleEnemyPanelViewData& EnemyView,
	const FBattlePartSlotIdentity& PartIdentity)
{
	if (!Runtime.CanOpenEnemyInspection()
		|| !PartIdentity.IsValidSlot()
		|| EnemyView.Parts.IsEmpty())
	{
		return false;
	}

	if (IsInspectionOpen()
		&& InspectedEnemySlotId == EnemyView.EnemySlotId
		&& SelectedPartIdentity == PartIdentity)
	{
		CloseInspection(false);
		return true;
	}

	UWacomBattleEnemyInspectionWidget* Widget = EnsureWidget();
	if (!Widget)
	{
		return false;
	}

	FWacomBattleEnemyInspectionViewData View;
	View.Enemy = EnemyView;
	View.SelectedPartIdentity = PartIdentity;
	if (!Widget->SetInspectionViewData(View))
	{
		return false;
	}

	InspectedEnemySlotId = EnemyView.EnemySlotId;
	SelectedPartIdentity = Widget->GetInspectionViewData().SelectedPartIdentity;
	Widget->OpenInspection();
	return true;
}

void FWacomBattleHUDEnemyInspectionCoordinator::PrewarmInspection()
{
	const APlayerController* OwningPlayer = Runtime.GetOwningPlayer();
	if (!OwningPlayer || !OwningPlayer->GetLocalPlayer())
	{
		return;
	}
	EnsureWidget();
}

void FWacomBattleHUDEnemyInspectionCoordinator::UpdateEnemyView(
	const FWacomBattleEnemyPanelViewData& EnemyView)
{
	if (!IsInspectionOpen() || EnemyView.EnemySlotId != InspectedEnemySlotId)
	{
		return;
	}

	UWacomBattleEnemyInspectionWidget* Widget = InspectionWidget.Get();
	if (!Widget || EnemyView.Parts.IsEmpty())
	{
		CloseInspection(true);
		return;
	}
	const bool bSelectedPartStillPresent = EnemyView.Parts.ContainsByPredicate(
		[this](const FWacomBattleEnemyPartEntryViewData& Part)
		{
			return Part.Identity == SelectedPartIdentity;
		});
	if (!bSelectedPartStillPresent)
	{
		CloseInspection(true);
		return;
	}

	FWacomBattleEnemyInspectionViewData View;
	View.Enemy = EnemyView;
	View.SelectedPartIdentity = SelectedPartIdentity;
	if (!Widget->SetInspectionViewData(View))
	{
		CloseInspection(true);
		return;
	}
	SelectedPartIdentity = Widget->GetInspectionViewData().SelectedPartIdentity;
}

bool FWacomBattleHUDEnemyInspectionCoordinator::TryCloseInspection()
{
	if (!IsInspectionOpen())
	{
		return false;
	}
	CloseInspection(false);
	return true;
}

void FWacomBattleHUDEnemyInspectionCoordinator::CloseInspection(const bool bImmediate)
{
	if (UWacomBattleEnemyInspectionWidget* Widget = InspectionWidget.Get())
	{
		Widget->CloseInspection(bImmediate);
	}
	InspectedEnemySlotId = NAME_None;
	SelectedPartIdentity = FBattlePartSlotIdentity();
}

void FWacomBattleHUDEnemyInspectionCoordinator::Shutdown()
{
	if (UWacomBattleEnemyInspectionWidget* Widget = InspectionWidget.Get())
	{
		Widget->OnCloseRequestedNative.RemoveAll(this);
		Widget->OnSelectionRequestedNative.RemoveAll(this);
		Widget->CloseInspection(true);
		Widget->ClearInspectionViewData();
		Widget->RemoveFromParent();
	}
	InspectionWidget.Reset();
	InspectedEnemySlotId = NAME_None;
	SelectedPartIdentity = FBattlePartSlotIdentity();
}

bool FWacomBattleHUDEnemyInspectionCoordinator::IsInspectionOpen() const
{
	const UWacomBattleEnemyInspectionWidget* Widget = InspectionWidget.Get();
	return Widget && Widget->IsInspectionOpen();
}

bool FWacomBattleHUDEnemyInspectionCoordinator::IsInspectingEnemySlot(
	const FName EnemySlotId) const
{
	return IsInspectionOpen() && InspectedEnemySlotId == EnemySlotId;
}

UWacomBattleEnemyInspectionWidget*
FWacomBattleHUDEnemyInspectionCoordinator::EnsureWidget()
{
	if (UWacomBattleEnemyInspectionWidget* Existing = InspectionWidget.Get())
	{
		BindWidgetDelegates(*Existing);
		return Existing;
	}

	const UWacomUIDeveloperSettings* Settings = GetDefault<UWacomUIDeveloperSettings>();
	UClass* WidgetClass = Settings
		? Settings->DefaultBattleEnemyInspectionWidgetClass.LoadSynchronous()
		: nullptr;
	if (!IsConstructibleInspectionClass(WidgetClass))
	{
		if (!bLoggedMissingWidgetClass)
		{
			bLoggedMissingWidgetClass = true;
			UE_LOG(LogTemp, Error,
				TEXT("[BattleHUD] DefaultBattleEnemyInspectionWidgetClass 无效；敌人详情入口已禁用。"));
		}
		return nullptr;
	}

	APlayerController* OwningPlayer = Runtime.GetOwningPlayer();
	UWacomBattleEnemyInspectionWidget* Created = OwningPlayer
		? CreateWidget<UWacomBattleEnemyInspectionWidget>(OwningPlayer, WidgetClass)
		: nullptr;
	if (!Created)
	{
		return nullptr;
	}

	const UWacomFirstPersonCardAnchorComponent* ActiveAnchor =
		Runtime.ResolveActiveFirstPersonCardAnchor();
	const int32 InspectionViewportZOrder = ActiveAnchor
		? WacomBattleViewportLayerPolicy::ResolveInspectionPanelZOrder(
			ActiveAnchor->CardLayerZOrder)
		: WacomBattleViewportLayerPolicy::InspectionPanelZOrder;
	Created->AddToViewport(InspectionViewportZOrder);
	Created->SetVisibility(ESlateVisibility::Collapsed);
	InspectionWidget = Created;
	BindWidgetDelegates(*Created);
	return Created;
}

void FWacomBattleHUDEnemyInspectionCoordinator::BindWidgetDelegates(
	UWacomBattleEnemyInspectionWidget& Widget)
{
	Widget.OnCloseRequestedNative.RemoveAll(this);
	Widget.OnCloseRequestedNative.AddRaw(
		this, &FWacomBattleHUDEnemyInspectionCoordinator::HandleCloseRequested);
	Widget.OnSelectionRequestedNative.RemoveAll(this);
	Widget.OnSelectionRequestedNative.AddRaw(
		this, &FWacomBattleHUDEnemyInspectionCoordinator::HandleSelectionRequested);
}

void FWacomBattleHUDEnemyInspectionCoordinator::HandleCloseRequested()
{
	CloseInspection(false);
}

void FWacomBattleHUDEnemyInspectionCoordinator::HandleSelectionRequested(
	const FBattlePartSlotIdentity& PartIdentity)
{
	if (PartIdentity.IsValidSlot())
	{
		SelectedPartIdentity = PartIdentity;
	}
}
