// Copyright Wacom. All Rights Reserved.

#include "UI/Foundation/WacomGameUIManagerSubsystem.h"
#include "UI/Foundation/WacomPrimaryGameLayout.h"
#include "UI/Foundation/WacomUITags.h"

#include "Blueprint/UserWidget.h"
#include "CommonActivatableWidget.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

void UWacomGameUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 只在有渲染的环境（PIE / Standalone / Shipping）注册 World 清理监听。
	// -NullRHI（自动化测试 / Commandlet）不需要 UI 管理。
	if (!IsRunningCommandlet() && !FApp::IsUnattended())
	{
		WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(
			this, &UWacomGameUIManagerSubsystem::HandleWorldCleanup);
	}
}

void UWacomGameUIManagerSubsystem::Deinitialize()
{
	if (WorldCleanupHandle.IsValid())
	{
		FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
		WorldCleanupHandle.Reset();
	}
	TearDownPrimaryLayout();
	Super::Deinitialize();
}

void UWacomGameUIManagerSubsystem::HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	if (!PrimaryLayout) { return; }

	// 只清理属于被销毁 World 的 PrimaryLayout。
	if (PrimaryLayout->GetWorld() == World)
	{
		// 直接置空引用，不调 TearDown（World 正在销毁，Widget 可能已经无效）。
		PrimaryLayout = nullptr;
	}
}

void UWacomGameUIManagerSubsystem::TearDownPrimaryLayout()
{
	if (!PrimaryLayout) { return; }

	// 先清空所有 Stack 的 Widget，确保所有 Activatable 都 OnDeactivated，
	// 这样 CommonUI 的 ActionRouter 能释放 UIInputConfig。
	ClearAllLayers();

	if (PrimaryLayout->IsInViewport())
	{
		PrimaryLayout->RemoveFromParent();
	}
	PrimaryLayout = nullptr;
}

void UWacomGameUIManagerSubsystem::EnsurePrimaryLayout(APlayerController* PC)
{
	if (!PC || !PC->GetLocalPlayer())
	{
		UE_LOG(LogTemp, Warning, TEXT("[UIManager] EnsurePrimaryLayout: PC / LocalPlayer 无效"));
		return;
	}

	// 检查现有 PrimaryLayout 是否仍对应当前关卡的 PC。
	// 切关卡后旧 PC 失效，旧 PrimaryLayout 虽然还在 Viewport，但 Owner 已挂了，
	// 其 Widget Stack 里的旧 Widget（如 MainMenuScreen）还会覆盖 leaf-most input config，
	// 造成探索关卡游戏输入被 UI 层吃掉。
	const bool bStale = IsValid(PrimaryLayout)
		&& PrimaryLayout->GetOwningPlayer() != PC;
	if (bStale)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[UIManager] PrimaryLayout 对应旧 PC，重建"));
		TearDownPrimaryLayout();
	}

	// PrimaryLayout 被 GC 了（原 PC 销毁时把它也带走了）：清一下指针再重建。
	if (!IsValid(PrimaryLayout))
	{
		PrimaryLayout = nullptr;
	}

	// 已有有效实例且仍挂在 Viewport：直接返回。
	if (IsValid(PrimaryLayout) && PrimaryLayout->IsInViewport())
	{
		return;
	}

	// 懒加载默认 WBP 类。
	if (!PrimaryLayoutClass)
	{
		if (UClass* Loaded = LoadObject<UClass>(nullptr,
			TEXT("/Game/Wacom/UI/Foundation/WBP_PrimaryGameLayout.WBP_PrimaryGameLayout_C")))
		{
			PrimaryLayoutClass = Loaded;
		}
	}
	if (!PrimaryLayoutClass)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[UIManager] EnsurePrimaryLayout: PrimaryLayoutClass 加载失败"));
		return;
	}

	PrimaryLayout = CreateWidget<UWacomPrimaryGameLayout>(PC, PrimaryLayoutClass);
	if (!PrimaryLayout)
	{
		UE_LOG(LogTemp, Error, TEXT("[UIManager] CreateWidget<PrimaryLayout> 失败"));
		return;
	}
	PrimaryLayout->AddToViewport();

	UE_LOG(LogTemp, Display, TEXT("[UIManager] PrimaryLayout 已创建并挂载到 Viewport"));
}

UCommonActivatableWidget* UWacomGameUIManagerSubsystem::PushContentToLayer(
	FGameplayTag LayerTag,
	TSubclassOf<UCommonActivatableWidget> WidgetClass)
{
	if (!PrimaryLayout)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[UIManager] PushContentToLayer: PrimaryLayout 未就位（忘了 EnsurePrimaryLayout?）"));
		return nullptr;
	}
	if (!WidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UIManager] PushContentToLayer: WidgetClass 空"));
		return nullptr;
	}
	if (!LayerTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[UIManager] PushContentToLayer: LayerTag 无效"));
		return nullptr;
	}

	UCommonActivatableWidget* Pushed = PrimaryLayout->PushWidgetToLayer(LayerTag, WidgetClass);
	if (!Pushed)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[UIManager] PushContentToLayer: Layer=%s Class=%s 失败"),
			*LayerTag.ToString(), *WidgetClass->GetName());
	}
	return Pushed;
}

void UWacomGameUIManagerSubsystem::PopContentFromLayer(UCommonActivatableWidget* Widget)
{
	if (!Widget) { return; }

	// ActivatableWidgetStack 在 widget 被 DeactivateWidget 后会自动 Remove。
	Widget->DeactivateWidget();
}

void UWacomGameUIManagerSubsystem::ClearLayer(FGameplayTag LayerTag)
{
	if (!PrimaryLayout || !LayerTag.IsValid()) { return; }

	UCommonActivatableWidgetStack* Stack = PrimaryLayout->GetLayerStack(LayerTag);
	if (!Stack) { return; }

	// Snapshot 当前 Widget 列表（避免迭代时被 Deactivate / Remove 改动）。
	// 然后对每个 widget 先 Deactivate（让 CommonUI Router 释放 UIInputConfig），
	// 再显式 Remove。最后再 ClearWidgets 兜底。
	TArray<UCommonActivatableWidget*> Snapshot = Stack->GetWidgetList();
	for (UCommonActivatableWidget* W : Snapshot)
	{
		if (!W) { continue; }
		W->DeactivateWidget();
		Stack->RemoveWidget(*W);
	}

	Stack->ClearWidgets();
}

void UWacomGameUIManagerSubsystem::ClearAllLayers()
{
	if (!PrimaryLayout) { return; }

	ClearLayer(WacomUITags::UI_Layer_Game.GetTag());
	ClearLayer(WacomUITags::UI_Layer_GameMenu.GetTag());
	ClearLayer(WacomUITags::UI_Layer_Modal.GetTag());
	ClearLayer(WacomUITags::UI_Layer_Overlay.GetTag());
}
