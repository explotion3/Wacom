// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Events/BattleEvent.h"
#include "Types/WacomEnums.h"
#include "WacomBattleEventPresentationBuilder.generated.h"

class UCardDefinition;

UENUM(BlueprintType)
enum class EWacomBattleEventVisualTone : uint8
{
	Neutral  UMETA(DisplayName = "Neutral"),
	Positive UMETA(DisplayName = "Positive"),
	Warning  UMETA(DisplayName = "Warning"),
	Danger   UMETA(DisplayName = "Danger"),
	System   UMETA(DisplayName = "System"),
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FBattleEventPresentationView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Presentation")
	EBattleEventType EventType = EBattleEventType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Presentation")
	FText MessageText;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Presentation")
	bool bShouldDisplay = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Presentation")
	EWacomBattleEventVisualTone VisualTone = EWacomBattleEventVisualTone::Neutral;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Presentation")
	FName IconKey = NAME_None;
};

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
	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Presentation")
	static FBattleEventPresentationView BuildEventPresentationView(const FBattleEvent& Event);

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
