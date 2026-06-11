// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleEnemyPartEntryWidget.h"

#include "BattleEnemyPanelTestWidgets.generated.h"

UCLASS()
class UWacomBattleEnemyPanelSpecTrackingPartEntryWidget final : public UWacomBattleEnemyPartEntryWidget
{
	GENERATED_BODY()

public:
	virtual void SetPartEntryViewData(const FWacomBattleEnemyPartEntryViewData& InView) override
	{
		++ApplyCount;
		LastAppliedView = InView;
		Super::SetPartEntryViewData(InView);
	}

	int32 ApplyCount = 0;
	FWacomBattleEnemyPartEntryViewData LastAppliedView;
};
