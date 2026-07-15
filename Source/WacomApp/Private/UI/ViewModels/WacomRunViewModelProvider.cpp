// Copyright Wacom. All Rights Reserved.

#include "UI/ViewModels/WacomRunViewModelProvider.h"

#include "MVVMGameSubsystem.h"
#include "Types/MVVMViewModelContext.h"
#include "Types/MVVMViewModelCollection.h"

#include "GameFramework/PlayerController.h"
#include "RunSession.h"
#include "RunState.h"
#include "RunStateTypes.h"

#include "UI/ViewModels/WacomRunViewModel.h"
#include "GameFramework/WacomPlayerController.h"

const FName UWacomRunViewModelProvider::GlobalContextName = FName(TEXT("WacomRunViewModel"));

namespace
{
	FText PhaseToText(ETimePhase Phase)
	{
		switch (Phase)
		{
		case ETimePhase::Morning: return NSLOCTEXT("Wacom", "Phase.Morning", "清晨");
		case ETimePhase::Day:     return NSLOCTEXT("Wacom", "Phase.Day",     "日间");
		case ETimePhase::Dusk:    return NSLOCTEXT("Wacom", "Phase.Dusk",    "黄昏");
		case ETimePhase::Night:   return NSLOCTEXT("Wacom", "Phase.Night",   "夜间");
		case ETimePhase::Sunrise: return NSLOCTEXT("Wacom", "Phase.Sunrise", "日出");
		default:                  return NSLOCTEXT("Wacom", "Phase.Unknown", "未知");
		}
	}
}

void UWacomRunViewModelProvider::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 显式声明对 MVVMGameSubsystem 的依赖：保证 MVVM 先 Initialize、晚于本 Subsystem 被 Deinitialize。
	// 没有这一行 Subsystem 销毁顺序不保证，PIE 结束时可能崩在 RemoveInstance（见 ViewModelCollection 已 null）。
	Collection.InitializeDependency(UMVVMGameSubsystem::StaticClass());

	// 创建 ViewModel 实例（持久于 GameInstance 生命周期）
	RunViewModel = NewObject<UWacomRunViewModel>(this, TEXT("RunViewModel"));

	// 注册到 MVVM Global Viewmodel Collection。
	// WBP 中 Viewmodel 创建模式选 "Global Viewmodel Collection"，
	// Global Viewmodel Identifier 填 "WacomRunViewModel"（必须等于类名）。
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UMVVMGameSubsystem* MVVMSub = GI->GetSubsystem<UMVVMGameSubsystem>())
		{
			if (UMVVMViewModelCollectionObject* CollectionObj = MVVMSub->GetViewModelCollection())
			{
				FMVVMViewModelContext Ctx;
				Ctx.ContextClass = UWacomRunViewModel::StaticClass();
				Ctx.ContextName  = GlobalContextName;
				CollectionObj->AddViewModelInstance(Ctx, RunViewModel);
			}
		}
	}

	UE_LOG(LogTemp, Display, TEXT("[WacomRunViewModelProvider] Initialized, ViewModel registered to Global Collection"));
}

void UWacomRunViewModelProvider::Deinitialize()
{
	UnbindFromCurrentRunSession();

	// 从 Global Collection 移除。注意：UMVVMGameSubsystem 可能已先于本 Subsystem
	// Deinitialize（销毁顺序不保证），此时 ViewModelCollection 是 nullptr，必须判空。
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UMVVMGameSubsystem* MVVMSub = GI->GetSubsystem<UMVVMGameSubsystem>())
		{
			if (UMVVMViewModelCollectionObject* Collection = MVVMSub->GetViewModelCollection())
			{
				FMVVMViewModelContext Ctx;
				Ctx.ContextClass = UWacomRunViewModel::StaticClass();
				Ctx.ContextName  = GlobalContextName;
				Collection->RemoveViewModel(Ctx);
			}
		}
	}

	RunViewModel = nullptr;
	Super::Deinitialize();
}

void UWacomRunViewModelProvider::BindToPlayerController(APlayerController* PC)
{
	UnbindFromCurrentRunSession();

	if (!PC) { return; }

	AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(PC);
	if (!WacomPC) { return; }

	URunSession* Run = WacomPC->GetRunSession();
	if (!Run)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[WacomRunViewModelProvider] BindToPlayerController: PC 无 RunSession，跳过订阅"));
		return;
	}

	Run->OnRunStateChangedNative.AddUObject(this, &UWacomRunViewModelProvider::HandleRunStateChanged);
	SubscribedRunSession = Run;

	// 立即同步一次当前状态
	RefreshAllFields(Run);

	UE_LOG(LogTemp, Display,
		TEXT("[WacomRunViewModelProvider] Bound to RunSession of PC %s"),
		*GetNameSafe(PC));
}

void UWacomRunViewModelProvider::UnbindFromCurrentRunSession()
{
	if (URunSession* Run = SubscribedRunSession.Get())
	{
		Run->OnRunStateChangedNative.RemoveAll(this);
	}
	SubscribedRunSession.Reset();
}

void UWacomRunViewModelProvider::HandleRunStateChanged()
{
	if (URunSession* Run = SubscribedRunSession.Get())
	{
		RefreshAllFields(Run);
	}
}

void UWacomRunViewModelProvider::RefreshAllFields(URunSession* Run)
{
	if (!Run || !RunViewModel) { return; }

	const FRunState& State = Run->GetRunState();
	const FRunExplorationSnapshot Exploration = Run->BuildExplorationSnapshot();

	RunViewModel->SetPhaseDisplay(PhaseToText(Exploration.Time.CurrentTimePhase));
	RunViewModel->SetRemainingActionPoints(Exploration.Time.RemainingActionPoints);
	RunViewModel->SetCurrentDayNumber(Exploration.Time.CurrentDayNumber);

	RunViewModel->SetFingerCount(State.FingerCount);
	RunViewModel->SetExperienceCurrent(State.ExperienceCurrent);
	RunViewModel->SetExperienceCapacity(State.ExperienceCapacity);
	RunViewModel->SetAcquiredSkillCount(State.AcquiredSkills.Num());

	RunViewModel->SetGold(Run->GetGold());

	const FRunBackpackStorageSnapshot StorageSnapshot = Run->BuildBackpackStorageSnapshot();
	RunViewModel->SetFluxCapacity(StorageSnapshot.FluxCapacity);
	RunViewModel->SetBattleDeckCapacity(StorageSnapshot.BattleDeckCapacity);
	RunViewModel->SetBackpackCount(StorageSnapshot.FluxContentCount);
	RunViewModel->SetBattleDeckCount(StorageSnapshot.BattleDeckPhysicalCount);

	RunViewModel->SetPressureHunger    (State.Pressure.Hunger);
	RunViewModel->SetPressureWound     (State.Pressure.Wound);
	RunViewModel->SetPressureFatigue   (State.Pressure.Fatigue);
	RunViewModel->SetPressureBurden    (State.Pressure.Burden);
	RunViewModel->SetPressureDecay     (State.Pressure.Decay);
	RunViewModel->SetPressureMisdeed   (State.Pressure.Misdeed);
	RunViewModel->SetPressureBloodlust (State.Pressure.Bloodlust);
	RunViewModel->SetPressureDisability(State.Pressure.Disability);
	RunViewModel->SetPressureTotal     (State.Pressure.GetTotal());

	OnRunViewModelRefreshedNative.Broadcast();
}
