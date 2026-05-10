// Copyright Wacom. All Rights Reserved.

#include "UI/Foundation/WacomPrimaryGameLayout.h"
#include "UI/Foundation/WacomUITags.h"
#include "CommonActivatableWidget.h"
#include "GameplayTagContainer.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

void UWacomPrimaryGameLayout::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	LayerEntries.Reset();
	FLayerEntry E;
	E.Tag = WacomUITags::UI_Layer_Game.GetTag();     E.Stack = GameLayerStack;     LayerEntries.Add(E);
	E.Tag = WacomUITags::UI_Layer_GameMenu.GetTag(); E.Stack = GameMenuLayerStack; LayerEntries.Add(E);
	E.Tag = WacomUITags::UI_Layer_Modal.GetTag();    E.Stack = ModalLayerStack;    LayerEntries.Add(E);
	E.Tag = WacomUITags::UI_Layer_Overlay.GetTag();  E.Stack = OverlayLayerStack;  LayerEntries.Add(E);
}

UCommonActivatableWidgetStack* UWacomPrimaryGameLayout::GetLayerStack(const FGameplayTag& LayerTag) const
{
	for (const FLayerEntry& Entry : LayerEntries)
	{
		if (Entry.Tag.MatchesTagExact(LayerTag))
		{
			return Entry.Stack;
		}
	}
	return nullptr;
}

UCommonActivatableWidget* UWacomPrimaryGameLayout::PushWidgetToLayer(const FGameplayTag& LayerTag, TSubclassOf<UCommonActivatableWidget> WidgetClass)
{
	UCommonActivatableWidgetStack* Stack = GetLayerStack(LayerTag);
	if (!Stack || !WidgetClass)
	{
		return nullptr;
	}
	return Cast<UCommonActivatableWidget>(Stack->AddWidget(*WidgetClass));
}

UWacomPrimaryGameLayout* UWacomPrimaryGameLayout::GetPrimaryLayout(APlayerController* PC)
{
	if (!PC || !PC->GetLocalPlayer())
	{
		return nullptr;
	}
	// 从 LocalPlayer 的 ViewportClient 的 Widget 列表里找。
	// 第一阶段只有一个 Layout 实例，由 GameMode 或 TestActor 在 BeginPlay 时 AddToViewport。
	// 这里用 TObjectIterator 做简单查找。
	for (TObjectIterator<UWacomPrimaryGameLayout> It; It; ++It)
	{
		if (It->IsInViewport())
		{
			return *It;
		}
	}
	return nullptr;
}
