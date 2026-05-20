// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/CardWidget.h"
#include "Components/TextBlock.h"
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

UCLASS()
class UWacomBattleCardWidgetNoCardViewTest : public UCardWidget
{
	GENERATED_BODY()

public:
	void DisableCardViewForTest()
	{
		CardView = nullptr;
	}

	FString GetFallbackNameText() const
	{
		return NameText ? NameText->GetText().ToString() : FString();
	}

	FString GetFallbackCostText() const
	{
		return CostText ? CostText->GetText().ToString() : FString();
	}

	FString GetFallbackZoneText() const
	{
		return ZoneText ? ZoneText->GetText().ToString() : FString();
	}
};
