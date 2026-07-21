// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Snapshots/BattlePileInspectionSnapshot.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "WacomBattleCardPileDetailsTypes.generated.h"

UENUM(BlueprintType)
enum class EWacomBattlePileDetailsTab : uint8
{
	Draw,
	Discard,
	Exhaust
};

UENUM(BlueprintType)
enum class EWacomBattlePileDiscardSection : uint8
{
	Discard,
	Played
};

/** UI-only projection for one runtime card instance in the pile browser. */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattlePileCardEntryView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Pile Details")
	FGuid InstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Pile Details")
	ECardLocation SourceLocation = ECardLocation::Unknown;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Pile Details")
	int32 RuntimeCost = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Pile Details")
	FWacomCardViewData CardViewData;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Pile Details")
	FWacomCardDetailViewData CardDetailData;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Pile Details")
	TMap<FGameplayTag, int32> StatusStacks;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Pile Details")
	FGameplayTagContainer TemporaryKeywords;

};
