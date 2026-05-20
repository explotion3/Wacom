// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BattleWidgetSpecReceiver.generated.h"

UCLASS()
class UWacomBattleCardWidgetClickReceiver : public UObject
{
	GENERATED_BODY()

public:
	int32 ClickCount = 0;
	FGuid LastClickedId;

	UFUNCTION()
	void HandleClicked(FGuid CardInstanceId)
	{
		++ClickCount;
		LastClickedId = CardInstanceId;
	}
};
