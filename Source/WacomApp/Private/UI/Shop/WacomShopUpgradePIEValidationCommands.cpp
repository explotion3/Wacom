// Copyright Wacom. All Rights Reserved.

#include "UI/Shop/WacomShopUpgradePIEValidationPolicy.h"

#if WITH_EDITOR

#include "Engine/World.h"
#include "GameFramework/WacomPlayerController.h"
#include "HAL/IConsoleManager.h"
#include "RunSession.h"

bool CanSeedShopUpgradePIEValidation(
	const FWacomShopUpgradePIEValidationFacts& Facts,
	FName& OutDisabledReason)
{
	OutDisabledReason = NAME_None;
	if (!Facts.bIsPIEWorld)
	{
		OutDisabledReason = TEXT("NotPIE");
	}
	else if (!Facts.bRunActive)
	{
		OutDisabledReason = TEXT("RunNotActive");
	}
	else if (Facts.JourneyId != TEXT("Journey.Debug"))
	{
		OutDisabledReason = TEXT("WrongJourney");
	}
	else if (Facts.CurrentNode.FloorId != TEXT("Floor.Debug.01"))
	{
		OutDisabledReason = TEXT("WrongFloor");
	}
	else if (Facts.CurrentNode.NodeId != TEXT("Node.Entry"))
	{
		OutDisabledReason = TEXT("WrongNode");
	}
	else if (Facts.ActiveActivityKind != ERunExplorationActivityKind::None)
	{
		OutDisabledReason = TEXT("NodeActivityActive");
	}
	return OutDisabledReason.IsNone();
}

namespace
{
void SeedShopUpgradePIEValidation(UWorld* World)
{
	AWacomPlayerController* PC = World
		? Cast<AWacomPlayerController>(World->GetFirstPlayerController())
		: nullptr;
	URunSession* Run = PC ? PC->GetRunSession() : nullptr;
	const FRunExplorationSnapshot Snapshot = Run
		? Run->BuildExplorationSnapshot()
		: FRunExplorationSnapshot();

	FWacomShopUpgradePIEValidationFacts Facts;
	Facts.bIsPIEWorld = World && World->WorldType == EWorldType::PIE;
	Facts.bRunActive = Run && Run->IsRunActive();
	Facts.JourneyId = Snapshot.JourneyId;
	Facts.CurrentNode = Snapshot.CurrentNode;
	Facts.ActiveActivityKind = Snapshot.ActiveActivityKind;

	FName DisabledReason;
	if (!Run || !CanSeedShopUpgradePIEValidation(Facts, DisabledReason))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[ShopUpgradePIE] 拒绝补金币：Reason=%s Journey=%s Floor=%s Node=%s Activity=%d"),
			*DisabledReason.ToString(),
			*Snapshot.JourneyId.ToString(),
			*Snapshot.CurrentNode.FloorId.ToString(),
			*Snapshot.CurrentNode.NodeId.ToString(),
			static_cast<int32>(Snapshot.ActiveActivityKind));
		return;
	}

	const int32 Before = Run->GetGold();
	if (Before < 3)
	{
		Run->AddGold(3 - Before);
	}
	UE_LOG(LogTemp, Display,
		TEXT("[ShopUpgradePIE] 验证金币已就绪：Before=%d After=%d Journey=%s Floor=%s Node=%s"),
		Before,
		Run->GetGold(),
		*Snapshot.JourneyId.ToString(),
		*Snapshot.CurrentNode.FloorId.ToString(),
		*Snapshot.CurrentNode.NodeId.ToString());
}

FAutoConsoleCommandWithWorld GSeedShopUpgradePIEValidationCommand(
	TEXT("Wacom.Shop.SeedUpgradePIEValidation"),
	TEXT("仅编辑器 PIE：只在 Debug Journey 的 Entry 且无活动节点交互时把金币补到 3，用于 Shop 强化竖切验收。"),
	FConsoleCommandWithWorldDelegate::CreateStatic(&SeedShopUpgradePIEValidation));
}

#else

bool CanSeedShopUpgradePIEValidation(
	const FWacomShopUpgradePIEValidationFacts&,
	FName& OutDisabledReason)
{
	OutDisabledReason = TEXT("EditorOnly");
	return false;
}

#endif
