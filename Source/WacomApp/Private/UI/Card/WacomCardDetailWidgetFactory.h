// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"

namespace WacomCardDetailWidgetFactory
{
	template <typename WidgetT>
	WidgetT* CreateChildUserWidget(UUserWidget& Parent, UClass* WidgetClass)
	{
		UClass* SafeWidgetClass = WidgetClass;
		if (!SafeWidgetClass || !SafeWidgetClass->IsChildOf(WidgetT::StaticClass()))
		{
			SafeWidgetClass = WidgetT::StaticClass();
		}

		APlayerController* OwningPlayer = Parent.GetOwningPlayer();
		if (OwningPlayer && OwningPlayer->IsLocalController() && OwningPlayer->GetLocalPlayer())
		{
			return CreateWidget<WidgetT>(OwningPlayer, SafeWidgetClass);
		}

		return NewObject<WidgetT>(&Parent, SafeWidgetClass);
	}
}
