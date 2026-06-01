// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Events/BattleEvent.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Snapshots/BattleSnapshot.h"
#include "Types/WacomEnums.h"
#include "UI/Battle/WacomBattleEventPresentationBuilder.h"
#include "WacomBattleCombatLogBuilder.generated.h"

UENUM(BlueprintType)
enum class EWacomBattleCombatLogCommandKind : uint8
{
	Unknown UMETA(DisplayName = "Unknown"),
	System UMETA(DisplayName = "System"),
	PlayCard UMETA(DisplayName = "PlayCard"),
	Wait UMETA(DisplayName = "Wait"),
	EndTurn UMETA(DisplayName = "EndTurn"),
	KnockdownChoice UMETA(DisplayName = "KnockdownChoice"),
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattleCombatLogCommandContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log")
	EWacomBattleCombatLogCommandKind CommandKind = EWacomBattleCombatLogCommandKind::Unknown;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log")
	int32 TurnNumber = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log")
	FGuid CardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log")
	FGuid TargetPartInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log")
	FGuid TargetCardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log")
	FText CardName;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log")
	FText TargetName;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log")
	EKnockdownChoice KnockdownChoice = EKnockdownChoice::None;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattleCombatLogLineView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log")
	EBattleEventType SourceEventType = EBattleEventType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log")
	FText MessageText;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log")
	EWacomBattleEventVisualTone VisualTone = EWacomBattleEventVisualTone::Neutral;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log")
	FName IconKey = NAME_None;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattleCombatLogBlockView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log")
	EWacomBattleCombatLogCommandKind CommandKind = EWacomBattleCombatLogCommandKind::Unknown;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log")
	FText HeaderText;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log")
	TArray<FWacomBattleCombatLogLineView> DetailLines;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log")
	int32 FirstEventSequence = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log")
	int32 LastEventSequence = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log")
	bool bShouldDisplay = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log")
	EWacomBattleEventVisualTone VisualTone = EWacomBattleEventVisualTone::Neutral;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Combat Log")
	FName IconKey = NAME_None;
};

/**
 * Builds player-facing command-block battle log entries from rule events.
 *
 * WacomBattle remains the source of truth. This builder only groups the events
 * consumed after a successful HUD command into readable UI text.
 */
UCLASS()
class WACOMAPP_API UWacomBattleCombatLogBuilder : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Combat Log")
	static FWacomBattleCombatLogCommandContext BuildSystemCommandContext(const FBattleSnapshot& Snapshot);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Combat Log")
	static FWacomBattleCombatLogCommandContext BuildPlayCardCommandContext(
		const FBattleSnapshot& Snapshot,
		const FGuid& CardInstanceId,
		const FGuid& TargetPartInstanceId,
		const FGuid& TargetCardInstanceId);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Combat Log")
	static FWacomBattleCombatLogCommandContext BuildWaitCommandContext(const FBattleSnapshot& Snapshot);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Combat Log")
	static FWacomBattleCombatLogCommandContext BuildEndTurnCommandContext(const FBattleSnapshot& Snapshot);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Combat Log")
	static FWacomBattleCombatLogCommandContext BuildKnockdownChoiceCommandContext(
		const FBattleSnapshot& Snapshot,
		EKnockdownChoice Choice);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Combat Log")
	static FWacomBattleCombatLogBlockView BuildCombatLogBlock(
		const FWacomBattleCombatLogCommandContext& Context,
		const TArray<FBattleEvent>& Events,
		const FBattleSnapshot& PreCommandSnapshot,
		const FBattleSnapshot& PostCommandSnapshot);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Combat Log")
	static FWacomBattleCombatLogBlockView BuildLegacyEventBlock(const FBattleEventPresentationView& EventView);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Combat Log")
	static FString FormatCombatLogBlockForLog(const FWacomBattleCombatLogBlockView& Block);
};
