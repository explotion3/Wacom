// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Map/WacomMapTypes.h"
#include "WacomFloorMapDefinition.generated.h"

/** 一个 Floor 的手工 Logical Map Graph。运行态不写入本资产。 */
UCLASS(BlueprintType)
class WACOMDATA_API UWacomFloorMapDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Map",
		meta = (ToolTip = "Floor 的稳定 ID。在同一 Journey 中必须唯一，不能使用数组下标替代。"))
	FName FloorId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Map",
		meta = (ToolTip = "首次进入 Floor 时使用的 NodeId。必须存在于 Nodes。"))
	FName EntryNodeId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Map",
		meta = (ToolTip = "Floor 的节点定义。NodeId 必须非空且唯一。"))
	TArray<FWacomMapNodeDefinition> Nodes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Map",
		meta = (ToolTip = "Floor 的有向连接。EdgeId 必须唯一，端点必须引用本 Floor 节点。"))
	TArray<FWacomMapEdgeDefinition> Edges;

	const FWacomMapNodeDefinition* FindNode(FName NodeId) const;
	const FWacomMapEdgeDefinition* FindEdge(FName EdgeId) const;
	void FindOutgoingEdges(FName FromNodeId, TArray<const FWacomMapEdgeDefinition*>& OutEdges) const;
};
