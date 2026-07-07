// Copyright Wacom. All Rights Reserved.

#include "UI/Foundation/WacomAppToastSubsystem.h"

#include "Cards/CardDefinition.h"
#include "CommonActivatableWidget.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "UI/Foundation/WacomAppToastWidget.h"
#include "UI/Foundation/WacomGameUIManagerSubsystem.h"

#define LOCTEXT_NAMESPACE "WacomAppToastSubsystem"

namespace
{
	bool CanUseToastViewport(const APlayerController* PlayerController)
	{
		return PlayerController
			&& PlayerController->IsLocalController()
			&& PlayerController->GetLocalPlayer();
	}

	const APlayerController* ResolveToastWidgetOwnerNoLog(const UWacomAppToastWidget* Widget)
	{
		if (!Widget)
		{
			return nullptr;
		}

		if (const APlayerController* OwningPlayer = Widget->GetOwningPlayer())
		{
			return OwningPlayer;
		}

		return Widget->GetTypedOuter<APlayerController>();
	}

	const UWorld* ResolveToastWidgetWorldNoLog(const UWacomAppToastWidget* Widget)
	{
		if (!Widget)
		{
			return nullptr;
		}

		if (const APlayerController* Owner = ResolveToastWidgetOwnerNoLog(Widget))
		{
			return Owner->GetWorld();
		}

		return Widget->GetTypedOuter<UWorld>();
	}

	FText GetAppToastCardDisplayName(const UCardDefinition* Card)
	{
		if (!Card)
		{
			return LOCTEXT("UnknownCard", "未知卡牌");
		}
		return Card->DisplayName.IsEmpty()
			? FText::FromName(Card->CardId)
			: Card->DisplayName;
	}
}

void UWacomAppToastSubsystem::Deinitialize()
{
	ClearToastWidget();
	Super::Deinitialize();
}

UWacomAppToastWidget* UWacomAppToastSubsystem::EnsureAppToastReady()
{
	return EnsureToastWidget();
}

void UWacomAppToastSubsystem::ShowToast(const FWacomAppToastView& View)
{
	if (View.MessageText.IsEmpty())
	{
		return;
	}

	if (UWacomAppToastWidget* Widget = EnsureToastWidget())
	{
		Widget->EnqueueToast(View);
	}
}

void UWacomAppToastSubsystem::ShowTextToast(FText Message, EWacomAppToastTone Tone)
{
	FWacomAppToastView View;
	View.MessageText = Message;
	View.Tone = Tone;
	View.IconKey = TEXT("Text");
	ShowToast(View);
}

void UWacomAppToastSubsystem::ShowCardGained(UCardDefinition* Card)
{
	FWacomAppToastView View;
	View.MessageText = FText::Format(
		LOCTEXT("CardGained", "获得卡牌：{0}"),
		GetAppToastCardDisplayName(Card));
	View.Tone = EWacomAppToastTone::Positive;
	View.IconKey = TEXT("CardGained");
	ShowToast(View);
}

void UWacomAppToastSubsystem::ShowGoldChanged(int32 Amount)
{
	FWacomAppToastView View;
	View.MessageText = Amount >= 0
		? FText::Format(LOCTEXT("GoldGained", "获得 {0} 金币"), FText::AsNumber(Amount))
		: FText::Format(LOCTEXT("GoldSpent", "消耗 {0} 金币"), FText::AsNumber(FMath::Abs(Amount)));
	View.Tone = Amount >= 0 ? EWacomAppToastTone::Positive : EWacomAppToastTone::Warning;
	View.IconKey = TEXT("GoldChanged");
	ShowToast(View);
}

void UWacomAppToastSubsystem::ShowWarning(FText Message)
{
	FWacomAppToastView View;
	View.MessageText = Message;
	View.Tone = EWacomAppToastTone::Warning;
	View.IconKey = TEXT("Warning");
	ShowToast(View);
}

UWacomAppToastWidget* UWacomAppToastSubsystem::EnsureToastWidget()
{
	if (IsValid(ToastWidget))
	{
		const APlayerController* CurrentPC = FindLocalPlayerController();
		if (!IsToastWidgetUsableForCurrentOwner(ToastWidget, CurrentPC))
		{
			UE_LOG(LogTemp, Display, TEXT("[AppToast] 缓存 ToastWidget 属于旧 World/PC，重建"));
			ClearToastWidget();
		}
	}

	if (IsValid(ToastWidget))
	{
		const APlayerController* CurrentPC = FindLocalPlayerController();
		if (!ToastWidget->IsInViewport() && CanUseToastViewport(CurrentPC))
		{
			ToastWidget->AddToViewport(/*ZOrder*/ 10000);
		}
		if (ToastWidget->GetVisibleToastCount() == 0)
		{
			ToastWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
		return ToastWidget;
	}
	ToastWidget = nullptr;

	APlayerController* PC = FindLocalPlayerController();
	if (!CanUseToastViewport(PC))
	{
		UE_LOG(LogTemp, Warning, TEXT("[AppToast] 本地 PlayerController 未就位，忽略 Toast"));
		return nullptr;
	}

	if (!ToastWidgetClass)
	{
		const UGameInstance* GameInstance = GetGameInstance();
		const UWacomGameUIManagerSubsystem* UIManager = GameInstance
			? GameInstance->GetSubsystem<UWacomGameUIManagerSubsystem>()
			: nullptr;

		if (UIManager)
		{
			ToastWidgetClass = UIManager->ResolveToastWidgetClass();
		}
	}
	if (!ToastWidgetClass)
	{
		ToastWidgetClass = UWacomAppToastWidget::StaticClass();
	}

	ToastWidget = CreateWidget<UWacomAppToastWidget>(PC, ToastWidgetClass);
	if (!ToastWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AppToast] Create ToastWidget 失败"));
		return nullptr;
	}

	ToastWidget->SetVisibility(ESlateVisibility::Collapsed);
	ToastWidget->AddToViewport(/*ZOrder*/ 10000);
	return ToastWidget;
}

APlayerController* UWacomAppToastSubsystem::FindLocalPlayerController() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (CanUseToastViewport(PC))
		{
			return PC;
		}
	}
	return nullptr;
}

bool UWacomAppToastSubsystem::IsToastWidgetUsableForCurrentOwner(
	const UWacomAppToastWidget* Widget,
	const APlayerController* CurrentPC) const
{
	if (!IsValid(Widget))
	{
		return false;
	}

	const UWorld* CurrentWorld = GetWorld();
	const UWorld* WidgetWorld = ResolveToastWidgetWorldNoLog(Widget);
	const APlayerController* WidgetOwner = ResolveToastWidgetOwnerNoLog(Widget);
	// Test/prewarm overrides can surface a transient world after TakeWidget() in UE 5.8.
	if (!WidgetOwner
		&& !Widget->IsInViewport()
		&& !Widget->GetTypedOuter<UWorld>()
		&& !Widget->GetTypedOuter<APlayerController>())
	{
		return true;
	}

	return IsToastOwnerPairUsable(WidgetWorld, WidgetOwner, CurrentWorld, CurrentPC);
}

bool UWacomAppToastSubsystem::IsToastOwnerPairUsable(
	const UWorld* WidgetWorld,
	const APlayerController* WidgetOwner,
	const UWorld* CurrentWorld,
	const APlayerController* CurrentPC) const
{
	// Transient widgets can have no real world or player owner.
	// Keep that cache reusable instead of classifying "unknown" as stale.
	const bool bHasNoRuntimeOwner = WidgetWorld == nullptr && WidgetOwner == nullptr;
	if (bHasNoRuntimeOwner)
	{
		return true;
	}

	if (!CurrentWorld || !CurrentPC)
	{
		return false;
	}

	if (!WidgetWorld || WidgetWorld != CurrentWorld)
	{
		return false;
	}

	if (!WidgetOwner || WidgetOwner != CurrentPC)
	{
		return false;
	}

	return true;
}

void UWacomAppToastSubsystem::ClearToastWidget()
{
	if (ToastWidget)
	{
		ToastWidget->RemoveFromParent();
		ToastWidget = nullptr;
	}
}

#undef LOCTEXT_NAMESPACE
