// Copyright Wacom. All Rights Reserved.

#if WITH_EDITOR

#include "Cards/CardDefinition.h"
#include "Engine/World.h"
#include "GameFramework/WacomPlayerController.h"
#include "HAL/IConsoleManager.h"
#include "RunSession.h"

namespace
{
constexpr int32 BackpackValidationOwnedCardTarget = 24;
constexpr int32 BackpackValidationSpecialZoneTarget = 2;

int32 CountOwnedCards(const FRunState& State)
{
	int32 Count = State.Backpack.Num() + State.BattleDeck.Num() + State.BurdenZone.Num();
	for (const FSpecialZone& Zone : State.SpecialZones)
	{
		Count += Zone.Cards.Num();
	}
	return Count;
}

void SeedBackpackPIEValidation(UWorld* World)
{
	AWacomPlayerController* PC = World
		? Cast<AWacomPlayerController>(World->GetFirstPlayerController())
		: nullptr;
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

	while (CountOwnedCards(Run->GetRunState()) < BackpackValidationOwnedCardTarget)
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

FAutoConsoleCommandWithWorld GSeedBackpackPIEValidationCommand(
	TEXT("Wacom.Backpack.SeedPIEValidation"),
	TEXT("仅编辑器：把当前 Run 补到至少 24 张实体牌和 2 个 SpecialZone，用于 Backpack Workspace PIE 验收。"),
	FConsoleCommandWithWorldDelegate::CreateStatic(&SeedBackpackPIEValidation));
}

#endif
