// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Exploration/RunExplorationTypes.h"
#include "Map/WacomMapTypes.h"
#include "WacomRunMapScreenTypes.generated.h"

/** 地图节点的纯表现状态；可传送性通过独立字段表达。 */
UENUM(BlueprintType)
enum class EWacomRunMapNodeVisualState : uint8
{
	Landmark,
	Revealed,
	Visited,
	Resolved,
	Current,
};

/** 被动节点 Widget 消费的一条只读显示数据。 */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomRunMapNodeViewData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run Map")
	FWacomMapNodeHandle Handle;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run Map")
	FText Title;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run Map")
	FText Description;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run Map")
	FText TypeLabel;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run Map")
	FVector2D DesignPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run Map")
	EWacomRunMapNodeVisualState VisualState = EWacomRunMapNodeVisualState::Landmark;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run Map")
	bool bCanSelect = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run Map")
	bool bCanTravel = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run Map")
	bool bIsSelected = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run Map")
	FText DisabledReason;
};

/** C++ edge paint layer 消费的一条有向连线数据。 */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomRunMapEdgeViewData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run Map")
	FWacomMapEdgeHandle Handle;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run Map")
	FWacomMapNodeHandle SourceNode;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run Map")
	FWacomMapNodeHandle TargetNode;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run Map")
	FVector2D SourceDesignPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run Map")
	FVector2D TargetDesignPosition = FVector2D::ZeroVector;
};

/** Run Map Screen 每次完整替换的只读 ViewData。 */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomRunMapScreenViewData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run Map")
	int32 StateVersion = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run Map")
	FName FloorId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run Map")
	FText FloorTitle;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run Map")
	FWacomMapNodeHandle CurrentNode;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run Map")
	TArray<FWacomRunMapNodeViewData> Nodes;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run Map")
	TArray<FWacomRunMapEdgeViewData> Edges;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run Map")
	FWacomMapNodeHandle SelectedNode;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run Map")
	FWacomMapNodeHandle DefaultFocusNode;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run Map")
	FText SelectedNodeTitle;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run Map")
	FText SelectedNodeDescription;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run Map")
	FText StatusText;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run Map")
	bool bCanConfirmTravel = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run Map")
	bool bIsAvailable = false;
};

enum class EWacomRunMapScreenAction : uint8
{
	SelectNode,
	ConfirmTravel,
	Close,
};

/** Screen 上报给 App-private Flow 的一次性玩家意图。 */
struct WACOMAPP_API FWacomRunMapScreenActionRequest
{
	EWacomRunMapScreenAction Action = EWacomRunMapScreenAction::Close;
	FWacomMapNodeHandle Node;
	int32 SourceStateVersion = 0;
};
