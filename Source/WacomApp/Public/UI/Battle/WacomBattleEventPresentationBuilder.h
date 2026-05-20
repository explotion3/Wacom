// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Events/BattleEvent.h"
#include "Types/WacomEnums.h"
#include "WacomBattleEventPresentationBuilder.generated.h"

class UCardDefinition;

/**
 * Builds player-facing presentation text for battle events.
 *
 * Battle rules emit FBattleEvent as a record stream. This builder owns the
 * UI-only Chinese text mapping so widgets, logs, and future battle history
 * views can share one display vocabulary.
 */
UCLASS()
class WACOMAPP_API UWacomBattleEventPresentationBuilder : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 将战斗事件格式化为玩家可读中文提示。空字符串表示不显示该事件。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Presentation")
	static FString FormatEventForPlayer(const FBattleEvent& Event);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Presentation")
	static FString FormatCardName(const UCardDefinition* Card);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Presentation")
	static FString FormatStatusName(FGameplayTag Tag);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Presentation")
	static FString FormatKnockdownChoice(EKnockdownChoice Choice);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Presentation")
	static FString FormatHandLimitDiscardSource(EHandLimitDiscardSource Source);
};
