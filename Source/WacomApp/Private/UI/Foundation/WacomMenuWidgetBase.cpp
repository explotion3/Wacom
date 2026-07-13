// Copyright Wacom. All Rights Reserved.

#include "UI/Foundation/WacomMenuWidgetBase.h"

#include "Blueprint/WidgetTree.h"
#include "CommonButtonBase.h"
#include "Components/Button.h"
#include "GameFramework/WacomPlayerController.h"
#include "Input/Events.h"
#include "Input/CommonUIInputTypes.h"
#include "TimerManager.h"
#include "UI/Run/WacomRunMenuWidgetBase.h"

UWacomMenuWidgetBase::UWacomMenuWidgetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 菜单 Widget 都应当可获得焦点，这样键盘 / 手柄可用。
	// CommonUI 切换 UIInputConfig 时也需要 Widget 可聚焦。
	SetIsFocusable(true);
}

void UWacomMenuWidgetBase::NativeOnActivated()
{
	Super::NativeOnActivated();

	if (AWacomPlayerController* WacomPC = ResolveOwningWacomPlayerController())
	{
		WacomPC->RegisterActiveGameMenuWidget(this);
	}

	// 延迟一帧聚焦：CommonUI Router 在 Push 后需要一帧完成 leaf-most 切换。
	// 如果同帧 SetKeyboardFocus，下层 Widget 可能抢回焦点。
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				FocusFirstButton();
			}));
	}
	else
	{
		FocusFirstButton();
	}
}

bool UWacomMenuWidgetBase::SetOwnedRunFirstPersonCardLayerMenuLeaseFromRunCards(
	FWacomRunMenuCardLeaseRequest Request,
	FWacomRunMenuCardLeaseResult& OutResult)
{
	if (UWacomRunMenuWidgetBase* RunMenu = Cast<UWacomRunMenuWidgetBase>(this))
	{
		return RunMenu->SetOwnedRunMenuCardLeaseFromRunCards(Request, OutResult);
	}

	if (Request.LeaseId.IsNone())
	{
		Request.LeaseId = FName(*FString::Printf(
			TEXT("%s_MenuLease"),
			*GetName()));
	}
	if (Request.SourceId.IsNone())
	{
		Request.SourceId = FName(*FString::Printf(
			TEXT("%s_MenuLeaseSource"),
			*GetName()));
	}

	OutResult = FWacomRunMenuCardLeaseResult();
	OutResult.LeaseId = Request.LeaseId;
	OutResult.SourceId = Request.SourceId;
	OutResult.RejectReason = TEXT("UnsupportedMenuBase");
	OutResult.DebugSummary = FString::Printf(
		TEXT("RunMenuCardLeaseProvider{LeaseId=%s SourceId=%s LeaseSet=false Reject=UnsupportedMenuBase}"),
		*Request.LeaseId.ToString(),
		*Request.SourceId.ToString());
	return false;
}

FName UWacomMenuWidgetBase::GetOwnedRunFirstPersonCardLayerMenuLeaseId() const
{
	if (const UWacomRunMenuWidgetBase* RunMenu = Cast<UWacomRunMenuWidgetBase>(this))
	{
		return RunMenu->GetOwnedRunMenuCardLeaseId();
	}
	return NAME_None;
}

FWacomRunMenuCardDropResolveResult UWacomMenuWidgetBase::ResolveRunMenuFirstPersonCardDropIntent_Implementation(
	const FWacomRunMenuCardDropResolveResult& Candidate) const
{
	if (const UWacomRunMenuWidgetBase* RunMenu = Cast<UWacomRunMenuWidgetBase>(this))
	{
		return RunMenu->ResolveRunMenuCardDropIntent(Candidate);
	}

	FWacomRunMenuCardDropResolveResult Result = Candidate;
	Result.IntentKind = EWacomRunMenuCardDropIntentKind::ProbeZoneTarget;
	Result.RejectReason = EWacomRunMenuCardDropRejectReason::MenuDoesNotAccept;
	Result.SubmitPolicy = EWacomRunMenuCardDropSubmitPolicy::None;
	Result.bCanSubmit = false;
	Result.bSubmitted = false;
	return Result;
}

bool UWacomMenuWidgetBase::SubmitRunMenuFirstPersonCardDropIntent_Implementation(
	const FWacomRunMenuCardDropResolveResult& Resolved,
	FWacomRunMenuCardDropResolveResult& OutSubmitted)
{
	if (UWacomRunMenuWidgetBase* RunMenu = Cast<UWacomRunMenuWidgetBase>(this))
	{
		return RunMenu->SubmitRunMenuCardDropIntent(Resolved, OutSubmitted);
	}

	OutSubmitted = Resolved;
	OutSubmitted.IntentKind = EWacomRunMenuCardDropIntentKind::Reject;
	OutSubmitted.RejectReason = EWacomRunMenuCardDropRejectReason::SubmitFailed;
	OutSubmitted.SubmitPolicy = EWacomRunMenuCardDropSubmitPolicy::None;
	OutSubmitted.bCanSubmit = false;
	OutSubmitted.bSubmitted = false;
	return false;
}

bool UWacomMenuWidgetBase::HasOwnedRunFirstPersonCardLayerMenuLease(FName LeaseId) const
{
	if (const UWacomRunMenuWidgetBase* RunMenu = Cast<UWacomRunMenuWidgetBase>(this))
	{
		return RunMenu->HasOwnedRunMenuCardLease(LeaseId);
	}
	return false;
}

void UWacomMenuWidgetBase::NativeOnDeactivated()
{
	if (AWacomPlayerController* WacomPC = ResolveOwningWacomPlayerController())
	{
		WacomPC->UnregisterActiveGameMenuWidget(this);
	}

	Super::NativeOnDeactivated();
}

void UWacomMenuWidgetBase::FocusFirstButton()
{
	if (UWidget* DesiredTarget = NativeGetDesiredFocusTarget())
	{
		if (DesiredTarget->GetIsEnabled()
			&& DesiredTarget->GetVisibility() != ESlateVisibility::Collapsed
			&& DesiredTarget->GetVisibility() != ESlateVisibility::Hidden)
		{
			DesiredTarget->SetKeyboardFocus();
			return;
		}
	}

	if (WidgetTree)
	{
		UCommonButtonBase* FirstCommonButton = nullptr;
		UButton* FirstButton = nullptr;
		WidgetTree->ForEachWidget([&FirstCommonButton, &FirstButton](UWidget* W)
		{
			if (FirstCommonButton || FirstButton) { return; }
			if (UCommonButtonBase* CommonButton = Cast<UCommonButtonBase>(W))
			{
				if (CommonButton->GetIsEnabled()
					&& CommonButton->IsInteractionEnabled()
					&& CommonButton->GetVisibility() != ESlateVisibility::Collapsed
					&& CommonButton->GetVisibility() != ESlateVisibility::Hidden)
				{
					FirstCommonButton = CommonButton;
				}
				return;
			}
			if (UButton* Btn = Cast<UButton>(W))
			{
				if (Btn->GetIsEnabled())
				{
					FirstButton = Btn;
				}
			}
		});

		if (FirstCommonButton)
		{
			FirstCommonButton->SetKeyboardFocus();
			return;
		}
		if (FirstButton)
		{
			FirstButton->SetKeyboardFocus();
			return;
		}
	}
	SetFocus();
}

void UWacomMenuWidgetBase::ClearOwnedRunFirstPersonCardLayerMenuLease()
{
	if (UWacomRunMenuWidgetBase* RunMenu = Cast<UWacomRunMenuWidgetBase>(this))
	{
		RunMenu->ClearOwnedRunMenuCardLease();
	}
}

AWacomPlayerController* UWacomMenuWidgetBase::ResolveOwningWacomPlayerController() const
{
	return Cast<AWacomPlayerController>(GetOwningPlayer());
}

TOptional<FUIInputConfig> UWacomMenuWidgetBase::GetDesiredInputConfig() const
{
	// Menu 模式：鼠标可见、不锁 viewport、UIOnly 阻断游戏输入
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

FReply UWacomMenuWidgetBase::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// ECommonInputMode::Menu 会屏蔽 EnhancedInput 的 IA_OpenMenu，
	// 因此 ESC 关闭菜单的逻辑必须在 widget 层处理。
	// 子类如果需要不同行为（例如 ConfirmDialog 把 ESC 当 Cancel），可以 override。
	if (InKeyEvent.GetKey() == EKeys::Escape
		|| InKeyEvent.GetKey() == EKeys::Gamepad_FaceButton_Right)
	{
		return NativeHandleBackRequested();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UWacomMenuWidgetBase::NativeHandleBackRequested()
{
	OnBackRequestedNative.Broadcast();
	DeactivateWidget();
	return FReply::Handled();
}
