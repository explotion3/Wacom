// Copyright Wacom. All Rights Reserved.

#include "UI/Foundation/WacomGameUIManagerSubsystem.h"
#include "UI/Foundation/WacomActivatableWidget.h"
#include "UI/Foundation/WacomAppToastWidget.h"
#include "UI/Foundation/WacomPrimaryGameLayout.h"
#include "UI/Foundation/WacomUIDeveloperSettings.h"
#include "UI/Foundation/WacomUITags.h"

#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Blueprint/UserWidget.h"
#include "CommonActivatableWidget.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

namespace
{
	const TCHAR* PrimaryLayoutFallbackPath =
		TEXT("/Game/Wacom/UI/Foundation/WBP_PrimaryGameLayout.WBP_PrimaryGameLayout_C");

	template<typename ExpectedT>
	UClass* LoadSoftClassChecked(
		const TSoftClassPtr<ExpectedT>& SoftClass,
		const TCHAR* LogContext,
		const TCHAR* FieldName)
	{
		UObject* LoadedObject = SoftClass.ToSoftObjectPath().TryLoad();
		UClass* LoadedClass = Cast<UClass>(LoadedObject);
		if (!LoadedClass)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[UIManager] %s: %s 加载失败或不是 UClass：%s"),
				LogContext, FieldName, *SoftClass.ToString());
			return nullptr;
		}

		if (!LoadedClass->IsChildOf(ExpectedT::StaticClass()))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[UIManager] %s: %s=%s 必须继承 %s"),
				LogContext,
				FieldName,
				*LoadedClass->GetName(),
				*ExpectedT::StaticClass()->GetName());
			return nullptr;
		}

		return LoadedClass;
	}

	UClass* LoadSoftWidgetClassChecked(
		const TSoftClassPtr<UWacomActivatableWidget>& SoftClass,
		UClass* ExpectedParentClass,
		const TCHAR* LogContext,
		const TCHAR* FieldName)
	{
		if (!ExpectedParentClass)
		{
			ExpectedParentClass = UWacomActivatableWidget::StaticClass();
		}

		UObject* LoadedObject = SoftClass.ToSoftObjectPath().TryLoad();
		UClass* LoadedClass = Cast<UClass>(LoadedObject);
		if (!LoadedClass)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[UIManager] %s: %s 加载失败或不是 UClass：%s"),
				LogContext, FieldName, *SoftClass.ToString());
			return nullptr;
		}

		if (!LoadedClass->IsChildOf(ExpectedParentClass))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[UIManager] %s: %s=%s 必须继承 %s"),
				LogContext,
				FieldName,
				*LoadedClass->GetName(),
				*ExpectedParentClass->GetName());
			return nullptr;
		}

		return LoadedClass;
	}

	UClass* GetLoadedSoftWidgetClassChecked(
		const TSoftClassPtr<UWacomActivatableWidget>& SoftClass,
		UClass* ExpectedParentClass,
		const TCHAR* LogContext,
		const TCHAR* FieldName)
	{
		if (!ExpectedParentClass)
		{
			ExpectedParentClass = UWacomActivatableWidget::StaticClass();
		}

		UObject* LoadedObject = SoftClass.Get();
		if (!LoadedObject)
		{
			LoadedObject = SoftClass.ToSoftObjectPath().ResolveObject();
		}

		UClass* LoadedClass = Cast<UClass>(LoadedObject);
		if (!LoadedClass)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[UIManager] %s: %s 加载失败或不是 UClass：%s"),
				LogContext, FieldName, *SoftClass.ToString());
			return nullptr;
		}

		if (!LoadedClass->IsChildOf(ExpectedParentClass))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[UIManager] %s: %s=%s 必须继承 %s"),
				LogContext,
				FieldName,
				*LoadedClass->GetName(),
				*ExpectedParentClass->GetName());
			return nullptr;
		}

		return LoadedClass;
	}

	const FWacomUIWidgetClassEntry* FindWidgetClassEntry(
		const UWacomUIDeveloperSettings* Settings,
		const FGameplayTag& WidgetTag)
	{
		return Settings
			? Settings->WidgetClasses.FindByPredicate(
				[WidgetTag](const FWacomUIWidgetClassEntry& Entry)
				{
					return Entry.WidgetTag == WidgetTag;
				})
			: nullptr;
	}

	UClass* LoadFallbackClassChecked(
		const TCHAR* ClassPath,
		UClass* ExpectedParentClass,
		const TCHAR* LogContext,
		const TCHAR* FieldName,
		bool bLoadFailureIsError)
	{
		UClass* LoadedClass = LoadObject<UClass>(nullptr, ClassPath);
		if (!LoadedClass)
		{
			if (bLoadFailureIsError)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[UIManager] %s: fallback %s 加载失败：%s"),
					LogContext, FieldName, ClassPath);
			}
			else
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[UIManager] %s: fallback %s 加载失败：%s"),
					LogContext, FieldName, ClassPath);
			}
			return nullptr;
		}

		if (!LoadedClass->IsChildOf(ExpectedParentClass))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[UIManager] %s: fallback %s=%s 必须继承 %s"),
				LogContext,
				FieldName,
				*LoadedClass->GetName(),
				*ExpectedParentClass->GetName());
			return nullptr;
		}

		return LoadedClass;
	}
}

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
		CancelAllPendingAsyncPushes();
		// 直接置空引用，不调 TearDown（World 正在销毁，Widget 可能已经无效）。
		PrimaryLayout = nullptr;
		ResetPrimaryLayoutViewportZOrderLeases();
	}
}

void UWacomGameUIManagerSubsystem::TearDownPrimaryLayout()
{
	CancelAllPendingAsyncPushes();
	// Invalidate leases before deactivating layer contents. A closing screen may
	// attempt to release its already-invalid handle; it must not remount a root
	// that is in the middle of teardown.
	ResetPrimaryLayoutViewportZOrderLeases();

	if (!PrimaryLayout)
	{
		return;
	}

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
		ApplyPrimaryLayoutViewportZOrder();
		return;
	}

	const TSubclassOf<UWacomPrimaryGameLayout> PrimaryLayoutClass = ResolvePrimaryLayoutClass();
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
	AppliedPrimaryLayoutViewportZOrder = GetEffectivePrimaryLayoutViewportZOrder();
	PrimaryLayout->AddToViewport(AppliedPrimaryLayoutViewportZOrder);

	UE_LOG(LogTemp, Display,
		TEXT("[UIManager] PrimaryLayout 已创建并挂载到 Viewport（ZOrder=%d）"),
		AppliedPrimaryLayoutViewportZOrder);
}

uint64 UWacomGameUIManagerSubsystem::AcquirePrimaryLayoutViewportZOrderLease(
	const int32 RequestedZOrder)
{
	uint64 LeaseId = NextPrimaryLayoutViewportZOrderLeaseId++;
	if (LeaseId == 0)
	{
		LeaseId = NextPrimaryLayoutViewportZOrderLeaseId++;
	}

	PrimaryLayoutViewportZOrderLeases.Add(LeaseId, FMath::Max(0, RequestedZOrder));
	ApplyPrimaryLayoutViewportZOrder();
	return LeaseId;
}

void UWacomGameUIManagerSubsystem::ReleasePrimaryLayoutViewportZOrderLease(
	const uint64 LeaseId)
{
	if (LeaseId == 0 || PrimaryLayoutViewportZOrderLeases.Remove(LeaseId) == 0)
	{
		return;
	}
	ApplyPrimaryLayoutViewportZOrder();
}

int32 UWacomGameUIManagerSubsystem::GetEffectivePrimaryLayoutViewportZOrder() const
{
	int32 EffectiveZOrder = 0;
	for (const TPair<uint64, int32>& Lease : PrimaryLayoutViewportZOrderLeases)
	{
		EffectiveZOrder = FMath::Max(EffectiveZOrder, Lease.Value);
	}
	return EffectiveZOrder;
}

void UWacomGameUIManagerSubsystem::ApplyPrimaryLayoutViewportZOrder()
{
	const int32 RequestedZOrder = GetEffectivePrimaryLayoutViewportZOrder();
	if (!IsValid(PrimaryLayout)
		|| !PrimaryLayout->IsInViewport()
		|| AppliedPrimaryLayoutViewportZOrder == RequestedZOrder)
	{
		return;
	}

	// UE 5.8 does not expose an outer viewport-slot Z-order setter for UUserWidget.
	// Reattaching the already-built root is synchronous and preserves the CommonUI
	// layer stacks and their active widgets while replacing only the viewport slot.
	PrimaryLayout->RemoveFromParent();
	if (!IsValid(PrimaryLayout))
	{
		return;
	}
	PrimaryLayout->AddToViewport(RequestedZOrder);
	AppliedPrimaryLayoutViewportZOrder = RequestedZOrder;
}

void UWacomGameUIManagerSubsystem::ResetPrimaryLayoutViewportZOrderLeases()
{
	PrimaryLayoutViewportZOrderLeases.Reset();
	NextPrimaryLayoutViewportZOrderLeaseId = 1;
	AppliedPrimaryLayoutViewportZOrder = 0;
}

TSubclassOf<UWacomPrimaryGameLayout> UWacomGameUIManagerSubsystem::ResolvePrimaryLayoutClass() const
{
	const UWacomUIDeveloperSettings* Settings = GetDefault<UWacomUIDeveloperSettings>();
	if (Settings && !Settings->PrimaryLayoutClass.IsNull())
	{
		if (UClass* Loaded = LoadSoftClassChecked(
			Settings->PrimaryLayoutClass,
			TEXT("ResolvePrimaryLayoutClass"),
			TEXT("Settings PrimaryLayoutClass")))
		{
			return Loaded;
		}
	}

	if (UClass* Loaded = LoadFallbackClassChecked(
		PrimaryLayoutFallbackPath,
		UWacomPrimaryGameLayout::StaticClass(),
		TEXT("ResolvePrimaryLayoutClass"),
		TEXT("PrimaryLayoutClass"),
		/*bLoadFailureIsError*/ true))
	{
		return Loaded;
	}
	return nullptr;
}

TSubclassOf<UWacomActivatableWidget> UWacomGameUIManagerSubsystem::ResolveWidgetClass(
	FGameplayTag WidgetTag,
	TSubclassOf<UWacomActivatableWidget> FallbackClass,
	bool bLogMissingEntry) const
{
	if (!WidgetTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[UIManager] ResolveWidgetClass: WidgetTag 无效，使用 fallback"));
		return FallbackClass;
	}

	const UWacomUIDeveloperSettings* Settings = GetDefault<UWacomUIDeveloperSettings>();
	const FWacomUIWidgetClassEntry* MatchingEntry = FindWidgetClassEntry(Settings, WidgetTag);

	if (!MatchingEntry)
	{
		if (bLogMissingEntry)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[UIManager] ResolveWidgetClass: 未找到 WidgetTag=%s 的 settings 注册，使用 fallback"),
				*WidgetTag.ToString());
		}
		return FallbackClass;
	}

	if (MatchingEntry->WidgetClass.IsNull())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[UIManager] ResolveWidgetClass: WidgetTag=%s 的 WidgetClass 为空，使用 fallback"),
			*WidgetTag.ToString());
		return FallbackClass;
	}

	UClass* ExpectedParentClass = FallbackClass
		? FallbackClass.Get()
		: UWacomActivatableWidget::StaticClass();

	if (UClass* Loaded = LoadSoftWidgetClassChecked(
		MatchingEntry->WidgetClass,
		ExpectedParentClass,
		TEXT("ResolveWidgetClass"),
		TEXT("Settings WidgetClass")))
	{
		return Loaded;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[UIManager] ResolveWidgetClass: WidgetTag=%s Class=%s 无法使用，使用 fallback"),
		*WidgetTag.ToString(), *MatchingEntry->WidgetClass.ToString());
	return FallbackClass;
}

TSubclassOf<UWacomAppToastWidget> UWacomGameUIManagerSubsystem::ResolveToastWidgetClass() const
{
	const UWacomUIDeveloperSettings* Settings = GetDefault<UWacomUIDeveloperSettings>();
	if (Settings && !Settings->AppToastWidgetClass.IsNull())
	{
		if (UClass* Loaded = LoadSoftClassChecked(
			Settings->AppToastWidgetClass,
			TEXT("ResolveToastWidgetClass"),
			TEXT("Settings AppToastWidgetClass")))
		{
			return Loaded;
		}

		UE_LOG(LogTemp, Warning,
			TEXT("[UIManager] ResolveToastWidgetClass: Settings AppToastWidgetClass 无法使用：%s，使用 C++ fallback"),
			*Settings->AppToastWidgetClass.ToString());
	}

	return UWacomAppToastWidget::StaticClass();
}

bool UWacomGameUIManagerSubsystem::HasPendingAsyncPushToLayer(FGameplayTag LayerTag) const
{
	return LayerTag.IsValid() && PendingAsyncWidgetPushes.Contains(LayerTag);
}

void UWacomGameUIManagerSubsystem::CancelPendingAsyncPushToLayer(FGameplayTag LayerTag)
{
	if (!LayerTag.IsValid())
	{
		return;
	}

	FPendingAsyncWidgetPush Pending;
	if (!PendingAsyncWidgetPushes.RemoveAndCopyValue(LayerTag, Pending))
	{
		return;
	}

	if (Pending.Handle.IsValid())
	{
		Pending.Handle->CancelHandle();
		Pending.Handle.Reset();
	}
}

void UWacomGameUIManagerSubsystem::PushRegisteredWidgetToLayerAsync(FWacomAsyncWidgetPushRequest Request)
{
	if (!Request.LayerTag.IsValid())
	{
		CompleteAsyncWidgetPushResult(MoveTemp(Request), TEXT("InvalidLayerTag"));
		return;
	}
	if (!Request.WidgetTag.IsValid())
	{
		CompleteAsyncWidgetPushResult(MoveTemp(Request), TEXT("InvalidWidgetTag"));
		return;
	}
	if (!Request.FallbackClass)
	{
		CompleteAsyncWidgetPushResult(MoveTemp(Request), TEXT("MissingFallbackClass"));
		return;
	}
	if (!PrimaryLayout)
	{
		CompleteAsyncWidgetPushResult(MoveTemp(Request), TEXT("MissingPrimaryLayout"));
		return;
	}
	if (PendingAsyncWidgetPushes.Contains(Request.LayerTag))
	{
		CompleteAsyncWidgetPushResult(MoveTemp(Request), TEXT("LayerPending"));
		return;
	}

	const FGameplayTag LayerTag = Request.LayerTag;
	const int32 RequestId = NextAsyncPushRequestId++;
	FPendingAsyncWidgetPush Pending;
	Pending.RequestId = RequestId;
	Pending.ExpectedPrimaryLayout = PrimaryLayout;
	Pending.Request = MoveTemp(Request);
	PendingAsyncWidgetPushes.Add(LayerTag, MoveTemp(Pending));

	FPendingAsyncWidgetPush* StoredPendingForRequest = PendingAsyncWidgetPushes.Find(LayerTag);
	if (!StoredPendingForRequest || StoredPendingForRequest->RequestId != RequestId)
	{
		return;
	}
	FWacomAsyncWidgetPushRequest& PendingRequest = StoredPendingForRequest->Request;

	const UWacomUIDeveloperSettings* Settings = GetDefault<UWacomUIDeveloperSettings>();
	const FWacomUIWidgetClassEntry* MatchingEntry = FindWidgetClassEntry(Settings, PendingRequest.WidgetTag);
	if (!MatchingEntry)
	{
		if (PendingRequest.bLogMissingEntry)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[UIManager] PushRegisteredWidgetToLayerAsync: 未找到 WidgetTag=%s 的 settings 注册，使用 fallback"),
				*PendingRequest.WidgetTag.ToString());
		}
		CompleteAsyncWidgetPush(LayerTag, RequestId, PendingRequest.FallbackClass.Get());
		return;
	}

	if (MatchingEntry->WidgetClass.IsNull())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[UIManager] PushRegisteredWidgetToLayerAsync: WidgetTag=%s 的 WidgetClass 为空，使用 fallback"),
			*PendingRequest.WidgetTag.ToString());
		CompleteAsyncWidgetPush(LayerTag, RequestId, PendingRequest.FallbackClass.Get());
		return;
	}

	UClass* ExpectedParentClass = PendingRequest.FallbackClass
		? PendingRequest.FallbackClass.Get()
		: UWacomActivatableWidget::StaticClass();

	if (MatchingEntry->WidgetClass.IsValid())
	{
		if (UClass* Loaded = GetLoadedSoftWidgetClassChecked(
			MatchingEntry->WidgetClass,
			ExpectedParentClass,
			TEXT("PushRegisteredWidgetToLayerAsync"),
			TEXT("Settings WidgetClass")))
		{
			CompleteAsyncWidgetPush(LayerTag, RequestId, Loaded);
			return;
		}

		UE_LOG(LogTemp, Warning,
			TEXT("[UIManager] PushRegisteredWidgetToLayerAsync: WidgetTag=%s Class=%s 无法使用，使用 fallback"),
			*PendingRequest.WidgetTag.ToString(), *MatchingEntry->WidgetClass.ToString());
		CompleteAsyncWidgetPush(LayerTag, RequestId, PendingRequest.FallbackClass.Get());
		return;
	}

	const FSoftObjectPath WidgetClassPath = MatchingEntry->WidgetClass.ToSoftObjectPath();
	TWeakObjectPtr<UWacomGameUIManagerSubsystem> WeakThis(this);
	TSharedPtr<FStreamableHandle> Handle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		WidgetClassPath,
		[WeakThis, LayerTag, RequestId]()
		{
			if (UWacomGameUIManagerSubsystem* StrongThis = WeakThis.Get())
			{
				StrongThis->HandleAsyncWidgetClassLoaded(LayerTag, RequestId);
			}
		},
		FStreamableManager::AsyncLoadHighPriority,
		/*bManageActiveHandle*/ false,
		/*bStartStalled*/ false,
		FString::Printf(TEXT("WacomAsyncPush:%s"), *PendingRequest.WidgetTag.ToString()));

	if (FPendingAsyncWidgetPush* StoredPendingForHandle = PendingAsyncWidgetPushes.Find(LayerTag))
	{
		if (StoredPendingForHandle->RequestId == RequestId)
		{
			StoredPendingForHandle->Handle = MoveTemp(Handle);
		}
	}
}

void UWacomGameUIManagerSubsystem::HandleAsyncWidgetClassLoaded(FGameplayTag LayerTag, int32 RequestId)
{
	FPendingAsyncWidgetPush* Pending = PendingAsyncWidgetPushes.Find(LayerTag);
	if (!Pending || Pending->RequestId != RequestId)
	{
		return;
	}

	const UWacomUIDeveloperSettings* Settings = GetDefault<UWacomUIDeveloperSettings>();
	const FWacomUIWidgetClassEntry* MatchingEntry = FindWidgetClassEntry(Settings, Pending->Request.WidgetTag);
	UClass* ExpectedParentClass = Pending->Request.FallbackClass
		? Pending->Request.FallbackClass.Get()
		: UWacomActivatableWidget::StaticClass();

	UClass* ResolvedClass = nullptr;
	if (MatchingEntry && !MatchingEntry->WidgetClass.IsNull())
	{
		ResolvedClass = GetLoadedSoftWidgetClassChecked(
			MatchingEntry->WidgetClass,
			ExpectedParentClass,
			TEXT("HandleAsyncWidgetClassLoaded"),
			TEXT("Settings WidgetClass"));
	}

	if (!ResolvedClass)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[UIManager] HandleAsyncWidgetClassLoaded: WidgetTag=%s 加载后不可用，使用 fallback"),
			*Pending->Request.WidgetTag.ToString());
		ResolvedClass = Pending->Request.FallbackClass.Get();
	}

	CompleteAsyncWidgetPush(LayerTag, RequestId, ResolvedClass);
}

void UWacomGameUIManagerSubsystem::CancelAllPendingAsyncPushes()
{
	TArray<FGameplayTag> PendingLayerTags;
	PendingAsyncWidgetPushes.GetKeys(PendingLayerTags);
	for (const FGameplayTag& LayerTag : PendingLayerTags)
	{
		CancelPendingAsyncPushToLayer(LayerTag);
	}
	PendingAsyncWidgetPushes.Reset();
}

void UWacomGameUIManagerSubsystem::CompleteAsyncWidgetPush(
	FGameplayTag LayerTag,
	int32 RequestId,
	TSubclassOf<UCommonActivatableWidget> WidgetClass)
{
	FWacomAsyncWidgetPushRequest Request;
	TWeakObjectPtr<UWacomPrimaryGameLayout> ExpectedPrimaryLayout;
	FPendingAsyncWidgetPush Pending;
	if (FPendingAsyncWidgetPush* Found = PendingAsyncWidgetPushes.Find(LayerTag))
	{
		if (Found->RequestId != RequestId)
		{
			return;
		}
		Pending = MoveTemp(*Found);
		PendingAsyncWidgetPushes.Remove(LayerTag);
	}
	else
	{
		return;
	}

	Request = MoveTemp(Pending.Request);
	ExpectedPrimaryLayout = Pending.ExpectedPrimaryLayout;

	if (!WidgetClass)
	{
		CompleteAsyncWidgetPushResult(MoveTemp(Request), TEXT("MissingResolvedClass"));
		return;
	}
	if (!PrimaryLayout || ExpectedPrimaryLayout.Get() != PrimaryLayout)
	{
		CompleteAsyncWidgetPushResult(MoveTemp(Request), TEXT("StalePrimaryLayout"), WidgetClass);
		return;
	}
	if (!Request.OwningPlayer.IsExplicitlyNull()
		&& (!Request.OwningPlayer.IsValid() || PrimaryLayout->GetOwningPlayer() != Request.OwningPlayer.Get()))
	{
		CompleteAsyncWidgetPushResult(MoveTemp(Request), TEXT("StaleOwningPlayer"), WidgetClass);
		return;
	}
	if (Request.CanPush && !Request.CanPush())
	{
		CompleteAsyncWidgetPushResult(MoveTemp(Request), TEXT("PrePushGuardRejected"), WidgetClass);
		return;
	}

	bool bBeforePushSucceeded = false;
	if (Request.BeforePush)
	{
		FName FailureReason = NAME_None;
		bBeforePushSucceeded = Request.BeforePush(FailureReason);
		if (!bBeforePushSucceeded)
		{
			CompleteAsyncWidgetPushResult(
				MoveTemp(Request),
				FailureReason.IsNone() ? FName(TEXT("BeforePushFailed")) : FailureReason,
				WidgetClass);
			return;
		}
	}

	UCommonActivatableWidget* Pushed = PushResolvedWidgetToLayer(Request.LayerTag, WidgetClass);
	if (!Pushed)
	{
		if (bBeforePushSucceeded && Request.Rollback)
		{
			Request.Rollback(TEXT("PushFailed"));
		}
		CompleteAsyncWidgetPushResult(MoveTemp(Request), TEXT("PushFailed"), WidgetClass);
		return;
	}

	if (Request.AfterPush)
	{
		FName FailureReason = NAME_None;
		if (!Request.AfterPush(*Pushed, FailureReason))
		{
			const FName ResolvedFailureReason = FailureReason.IsNone()
				? FName(TEXT("AfterPushFailed"))
				: FailureReason;
			if (Request.PrepareFailedPushedWidget)
			{
				Request.PrepareFailedPushedWidget(*Pushed, ResolvedFailureReason);
			}
			if (bBeforePushSucceeded && Request.Rollback)
			{
				Request.Rollback(ResolvedFailureReason);
			}
			Pushed->DeactivateWidget();
			CompleteAsyncWidgetPushResult(MoveTemp(Request), ResolvedFailureReason, WidgetClass, Pushed);
			return;
		}
	}
	CompleteAsyncWidgetPushResult(MoveTemp(Request), NAME_None, WidgetClass, Pushed);
}

void UWacomGameUIManagerSubsystem::CompleteAsyncWidgetPushResult(
	FWacomAsyncWidgetPushRequest&& Request,
	FName FailureReason,
	TSubclassOf<UCommonActivatableWidget> ResolvedClass,
	UCommonActivatableWidget* PushedWidget)
{
	FWacomAsyncWidgetPushResult Result;
	Result.bSucceeded = FailureReason.IsNone() && PushedWidget != nullptr;
	Result.LayerTag = Request.LayerTag;
	Result.WidgetTag = Request.WidgetTag;
	Result.ResolvedClass = ResolvedClass;
	Result.PushedWidget = PushedWidget;
	Result.FailureReason = FailureReason;

	if (Request.OnComplete)
	{
		Request.OnComplete(Result);
	}
}

UCommonActivatableWidget* UWacomGameUIManagerSubsystem::PushContentToLayer(
	FGameplayTag LayerTag,
	TSubclassOf<UCommonActivatableWidget> WidgetClass)
{
	return PushResolvedWidgetToLayer(LayerTag, WidgetClass);
}

UCommonActivatableWidget* UWacomGameUIManagerSubsystem::PushResolvedWidgetToLayer(
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
