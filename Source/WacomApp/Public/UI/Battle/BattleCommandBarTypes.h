// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "BattleCommandBarTypes.generated.h"

UENUM(BlueprintType)
enum class EWacomBattleCommandId : uint8
{
	None UMETA(Hidden),
	Wait,
	EndTurn,
	CancelTargetSelect,
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattleCommandButtonView
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Command")
	EWacomBattleCommandId CommandId = EWacomBattleCommandId::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Command")
	FText DisplayText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Command")
	FText InputHintText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Command")
	FText ToolTipText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Command")
	FSlateBrush IconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Command")
	bool bHasIconBrush = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Command")
	bool bVisible = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Command")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Command")
	bool bPending = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Command")
	bool bPrimary = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Command")
	int32 SortOrder = 0;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattleCommandBarViewData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Command")
	FText WaitValueText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Command")
	FText PendingCommandText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Command")
	TArray<FWacomBattleCommandButtonView> Commands;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FWacomBattleCommandButtonClickedSignature,
	EWacomBattleCommandId,
	CommandId);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FWacomBattleCommandRequestedSignature,
	EWacomBattleCommandId,
	CommandId);
