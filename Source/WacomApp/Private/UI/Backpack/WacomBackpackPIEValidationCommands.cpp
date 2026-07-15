// Copyright Wacom. All Rights Reserved.

#if WITH_EDITOR

#include "Cards/CardDefinition.h"
#include "CommonActivatableWidget.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/WacomPlayerController.h"
#include "HAL/IConsoleManager.h"
#include "RunSession.h"
#include "UI/Backpack/WacomBackpackPIEValidationState.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Foundation/WacomGameUIManagerSubsystem.h"
#include "UI/Foundation/WacomUITags.h"

namespace
{
constexpr int32 BackpackValidationOwnedCardTarget = 24;
constexpr int32 BackpackValidationDenseOwnedCardTarget = 100;
constexpr int32 BackpackValidationSpecialZoneTarget = 2;

EWacomBackpackPIEValidationMode GBackpackPIEValidationMode =
	EWacomBackpackPIEValidationMode::None;

AWacomPlayerController* ResolveBackpackPIEPlayer(UWorld* World)
{
	return World
		? Cast<AWacomPlayerController>(World->GetFirstPlayerController())
		: nullptr;
}

int32 CountOwnedCards(const FRunState& State)
{
	int32 Count = State.Backpack.Num() + State.BattleDeck.Num() + State.BurdenZone.Num();
	for (const FSpecialZone& Zone : State.SpecialZones)
	{
		Count += Zone.Cards.Num();
	}
	return Count;
}

void SeedBackpackPIEValidationToTarget(UWorld* World, int32 OwnedCardTarget)
{
	AWacomPlayerController* PC = ResolveBackpackPIEPlayer(World);
	URunSession* Run = PC ? PC->GetRunSession() : nullptr;
	if (!Run)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BackpackPIE] 当前世界没有可用的 RunSession"));
		return;
	}

	UCardDefinition* MovableCard = LoadObject<UCardDefinition>(
		nullptr,
		TEXT("/Game/Wacom/Data/Cards/BugGirl/StarterPack/DA_Card_Starter_PoisonNeedle.DA_Card_Starter_PoisonNeedle"));
	UCardDefinition* SpecialZoneOwner = LoadObject<UCardDefinition>(
		nullptr,
		TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_ZhujianRongnang.DA_Card_ZhujianRongnang"));
	if (!MovableCard || !SpecialZoneOwner)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[BackpackPIE] 验收卡牌资产缺失：Movable=%s SpecialOwner=%s"),
			MovableCard ? TEXT("OK") : TEXT("Missing"),
			SpecialZoneOwner ? TEXT("OK") : TEXT("Missing"));
		return;
	}

	while (Run->GetRunState().SpecialZones.Num() < BackpackValidationSpecialZoneTarget)
	{
		const int32 Before = Run->GetRunState().SpecialZones.Num();
		Run->AcquireCardToRun(SpecialZoneOwner);
		if (Run->GetRunState().SpecialZones.Num() <= Before)
		{
			UE_LOG(LogTemp, Warning, TEXT("[BackpackPIE] B 类容器未创建新的 SpecialZone，停止补区"));
			break;
		}
	}

	while (CountOwnedCards(Run->GetRunState()) < OwnedCardTarget)
	{
		const int32 Before = CountOwnedCards(Run->GetRunState());
		Run->AcquireCardToRun(MovableCard);
		if (CountOwnedCards(Run->GetRunState()) <= Before)
		{
			UE_LOG(LogTemp, Warning, TEXT("[BackpackPIE] 实体牌数量未增长，停止补牌"));
			break;
		}
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("[BackpackPIE] 验收状态就绪：Owned=%d Backpack=%d Battle=%d Burden=%d SpecialZones=%d"),
		CountOwnedCards(Run->GetRunState()),
		Run->GetRunState().Backpack.Num(),
		Run->GetRunState().BattleDeck.Num(),
		Run->GetRunState().BurdenZone.Num(),
		Run->GetRunState().SpecialZones.Num());
}

void SeedBackpackPIEValidation(UWorld* World)
{
	SeedBackpackPIEValidationToTarget(World, BackpackValidationOwnedCardTarget);
}

void SeedDenseBackpackPIEValidation(UWorld* World)
{
	SeedBackpackPIEValidationToTarget(World, BackpackValidationDenseOwnedCardTarget);
}

void OpenBackpackPIEValidation(
	UWorld* World,
	EWacomBackpackPIEValidationMode Mode,
	bool bUseFormalWidgetClass)
{
	AWacomPlayerController* PC = ResolveBackpackPIEPlayer(World);
	URunSession* Run = PC ? PC->GetRunSession() : nullptr;
	UGameInstance* GameInstance = PC ? PC->GetGameInstance() : nullptr;
	UWacomGameUIManagerSubsystem* UIManager = GameInstance
		? GameInstance->GetSubsystem<UWacomGameUIManagerSubsystem>()
		: nullptr;
	if (!PC || !Run || !UIManager)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[BackpackPIE] 无法打开验证界面：PC=%s Run=%s UIManager=%s"),
			PC ? TEXT("OK") : TEXT("Missing"),
			Run ? TEXT("OK") : TEXT("Missing"),
			UIManager ? TEXT("OK") : TEXT("Missing"));
		return;
	}

	TSubclassOf<UWacomBackpackScreen> ScreenClass = UWacomBackpackScreen::StaticClass();
	if (bUseFormalWidgetClass)
	{
		if (UClass* FormalClass = LoadObject<UClass>(
			nullptr,
			TEXT("/Game/Wacom/UI/Backpack/WBP_BackpackScreen.WBP_BackpackScreen_C")))
		{
			if (FormalClass->IsChildOf(UWacomBackpackScreen::StaticClass()))
			{
				ScreenClass = FormalClass;
			}
		}
	}

	UIManager->EnsurePrimaryLayout(PC);
	UIManager->CancelPendingAsyncPushToLayer(WacomUITags::UI_Layer_GameMenu);
	UIManager->ClearLayer(WacomUITags::UI_Layer_GameMenu);

	FScopedWacomBackpackPIEValidationMode ScopedMode(Mode);
	UCommonActivatableWidget* Pushed = UIManager->PushContentToLayer(
		WacomUITags::UI_Layer_GameMenu,
		ScreenClass);
	if (Pushed)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("[BackpackPIE] 验证界面已打开：Mode=%d Class=%s"),
			static_cast<int32>(Mode),
			*GetNameSafe(ScreenClass.Get()));
	}
	else
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[BackpackPIE] 验证界面打开失败：Mode=%d Class=%s"),
			static_cast<int32>(Mode),
			*GetNameSafe(ScreenClass.Get()));
	}
}

void OpenEmptyBackpackPIEValidation(UWorld* World)
{
	OpenBackpackPIEValidation(
		World,
		EWacomBackpackPIEValidationMode::EmptySnapshot,
		/*bUseFormalWidgetClass*/ true);
}

void OpenNativeFallbackBackpackPIEValidation(UWorld* World)
{
	OpenBackpackPIEValidation(
		World,
		EWacomBackpackPIEValidationMode::NativeFallback,
		/*bUseFormalWidgetClass*/ false);
}

FAutoConsoleCommandWithWorld GSeedBackpackPIEValidationCommand(
	TEXT("Wacom.Backpack.SeedPIEValidation"),
	TEXT("仅编辑器：把当前 Run 补到至少 24 张实体牌和 2 个 SpecialZone，用于 Backpack Workspace PIE 验收。"),
	FConsoleCommandWithWorldDelegate::CreateStatic(&SeedBackpackPIEValidation));

FAutoConsoleCommandWithWorld GSeedDenseBackpackPIEValidationCommand(
	TEXT("Wacom.Backpack.SeedPIEValidation100"),
	TEXT("仅编辑器：把当前 Run 补到至少 100 张实体牌和 2 个 SpecialZone，用于密集 Backpack Workspace PIE 验收。"),
	FConsoleCommandWithWorldDelegate::CreateStatic(&SeedDenseBackpackPIEValidation));

FAutoConsoleCommandWithWorld GOpenEmptyBackpackPIEValidationCommand(
	TEXT("Wacom.Backpack.OpenEmptyPIEValidation"),
	TEXT("仅编辑器：使用正式 WBP 打开一个只读 0 张牌背包视图，不修改 Run 状态。"),
	FConsoleCommandWithWorldDelegate::CreateStatic(&OpenEmptyBackpackPIEValidation));

FAutoConsoleCommandWithWorld GOpenNativeFallbackBackpackPIEValidationCommand(
	TEXT("Wacom.Backpack.OpenNativeFallbackPIEValidation"),
	TEXT("仅编辑器：使用纯 C++ fallback Screen 和子控件打开当前背包，不修改 Run 状态。"),
	FConsoleCommandWithWorldDelegate::CreateStatic(&OpenNativeFallbackBackpackPIEValidation));
}

EWacomBackpackPIEValidationMode GetWacomBackpackPIEValidationMode()
{
	return GBackpackPIEValidationMode;
}

FScopedWacomBackpackPIEValidationMode::FScopedWacomBackpackPIEValidationMode(
	EWacomBackpackPIEValidationMode InMode)
	: PreviousMode(GBackpackPIEValidationMode)
{
	GBackpackPIEValidationMode = InMode;
}

FScopedWacomBackpackPIEValidationMode::~FScopedWacomBackpackPIEValidationMode()
{
	GBackpackPIEValidationMode = PreviousMode;
}

#endif
