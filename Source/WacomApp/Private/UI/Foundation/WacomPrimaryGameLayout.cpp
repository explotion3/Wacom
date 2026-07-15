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

void UWacomPrimaryGameLayout::NativeConstruct()
{
	Super::NativeConstruct();

	for (FLayerEntry& Entry : LayerEntries)
	{
		Entry.bIsTransitioning = false;
		if (Entry.Stack)
		{
			Entry.Stack->OnTransitioningChanged.RemoveAll(this);
			Entry.Stack->OnTransitioningChanged.AddUObject(
				this,
				&UWacomPrimaryGameLayout::HandleLayerTransitioningChanged);
		}
	}
}

void UWacomPrimaryGameLayout::NativeDestruct()
{
	for (FLayerEntry& Entry : LayerEntries)
	{
		if (Entry.Stack)
		{
			Entry.Stack->OnTransitioningChanged.RemoveAll(this);
		}
		Entry.bIsTransitioning = false;
	}
	OnLayerTransitioningChangedNative.Clear();
	Super::NativeDestruct();
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

bool UWacomPrimaryGameLayout::IsLayerTransitioning(const FGameplayTag& LayerTag) const
{
	for (const FLayerEntry& Entry : LayerEntries)
	{
		if (Entry.Tag.MatchesTagExact(LayerTag))
		{
			return Entry.bIsTransitioning;
		}
	}
	return false;
}

void UWacomPrimaryGameLayout::HandleLayerTransitioningChanged(
	UCommonActivatableWidgetContainerBase* Container,
	bool bIsTransitioning)
{
	for (FLayerEntry& Entry : LayerEntries)
	{
		if (Entry.Stack == Container)
		{
			Entry.bIsTransitioning = bIsTransitioning;
			OnLayerTransitioningChangedNative.Broadcast(Entry.Tag, bIsTransitioning);
			return;
		}
	}
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
	// 兼容/诊断入口：通常应通过 UWacomGameUIManagerSubsystem 获取当前 PrimaryLayout。
	// 这里用 TObjectIterator 做兜底查找。
	for (TObjectIterator<UWacomPrimaryGameLayout> It; It; ++It)
	{
		if (It->IsInViewport())
		{
			return *It;
		}
	}
	return nullptr;
}
