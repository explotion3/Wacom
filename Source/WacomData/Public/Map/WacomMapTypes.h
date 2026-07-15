// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "WacomMapTypes.generated.h"

class UCardDefinition;
class UEncounterDefinition;
class UWacomRunPickupDefinition;
class UWacomRunWorldCardInteractionDefinition;
class UShopDefinition;
class UWacomRunEventDefinition;

/** Logical Map Graph 的首版节点类型。Camp 不是节点类型。 */
UENUM(BlueprintType)
enum class EWacomMapNodeType : uint8
{
	Navigation,
	Encounter,
	RunEvent,
	Shop,
	Treasure,
	FloorEntrance,
};

/** 不推进节点生命周期的远景地标展示提示。 */
UENUM(BlueprintType)
enum class EWacomMapLandmarkVisibility : uint8
{
	None,
	FloorEntranceOutline,
	BossOutline,
};

/** 跨 Floor 唯一的 Map Node 身份。 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FWacomMapNodeHandle
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Map",
		meta = (ToolTip = "节点所属 Floor 的稳定 ID。与 NodeId 共同组成跨 Floor 唯一身份。"))
	FName FloorId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Map",
		meta = (ToolTip = "节点在所属 Floor 内的稳定 ID。不得依赖数组下标或世界坐标。"))
	FName NodeId = NAME_None;

	bool IsValid() const { return !FloorId.IsNone() && !NodeId.IsNone(); }

	friend bool operator==(const FWacomMapNodeHandle& A, const FWacomMapNodeHandle& B)
	{
		return A.FloorId == B.FloorId && A.NodeId == B.NodeId;
	}
};

FORCEINLINE uint32 GetTypeHash(const FWacomMapNodeHandle& Handle)
{
	return HashCombineFast(GetTypeHash(Handle.FloorId), GetTypeHash(Handle.NodeId));
}

/** 跨 Floor 唯一的 Map Edge 身份；单独 EdgeId 只在 Floor 内唯一。 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FWacomMapEdgeHandle
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Map",
		meta = (ToolTip = "边所属 Floor 的稳定 ID。与 EdgeId 共同组成跨 Floor 唯一身份。"))
	FName FloorId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Map",
		meta = (ToolTip = "边在所属 Floor 内的稳定 ID。场景 Path Actor 只在当前 Floor 范围内使用该值。"))
	FName EdgeId = NAME_None;

	bool IsValid() const { return !FloorId.IsNone() && !EdgeId.IsNone(); }

	friend bool operator==(const FWacomMapEdgeHandle& A, const FWacomMapEdgeHandle& B)
	{
		return A.FloorId == B.FloorId && A.EdgeId == B.EdgeId;
	}
};

FORCEINLINE uint32 GetTypeHash(const FWacomMapEdgeHandle& Handle)
{
	return HashCombineFast(GetTypeHash(Handle.FloorId), GetTypeHash(Handle.EdgeId));
}

/** Floor Entrance 使用的真实持有卡牌条件。多条 requirement 之间为 AND。 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FWacomOwnedCardRequirement
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Map|Requirement",
		meta = (ToolTip = "允许满足条件的卡牌定义。与 AllowedCardIds 为 OR；两者为空时由 RequiredKeywords 提供正向筛选。"))
	TArray<TObjectPtr<UCardDefinition>> AllowedCardDefinitions;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Map|Requirement",
		meta = (ToolTip = "允许满足条件的 CardId。与 AllowedCardDefinitions 为 OR；读取真实持有卡的定义 ID。"))
	TArray<FName> AllowedCardIds;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Map|Requirement",
		meta = (ToolTip = "目标卡必须全部拥有的关键词。读取卡牌定义上的 Card.Keyword。"))
	FGameplayTagContainer RequiredKeywords;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Map|Requirement",
		meta = (ToolTip = "目标卡不能拥有的关键词。命中任意一个即不满足该 requirement。"))
	FGameplayTagContainer BlockedKeywords;

	bool HasPositiveFilter() const
	{
		return !AllowedCardDefinitions.IsEmpty() || !AllowedCardIds.IsEmpty() || !RequiredKeywords.IsEmpty();
	}
};

USTRUCT(BlueprintType)
struct WACOMDATA_API FWacomMapEncounterPayload
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Map|Content",
		meta = (ToolTip = "Encounter 节点使用的战斗定义。节点规则只保存静态引用，不包含场景 Trigger。"))
	TObjectPtr<UEncounterDefinition> EncounterDefinition = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Map|Content",
		meta = (ToolTip = "是否为 Boss Encounter。Boss 可在 Floor 初始 Snapshot 中显示远景轮廓，但不会因此揭示路径或内容。"))
	bool bBoss = false;
};

USTRUCT(BlueprintType)
struct WACOMDATA_API FWacomMapRunEventPayload
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Map|Content",
		meta = (ToolTip = "RunEvent 节点使用的事件定义。运行时访问状态由 WacomRun 保存。"))
	TObjectPtr<UWacomRunEventDefinition> RunEventDefinition = nullptr;
};

USTRUCT(BlueprintType)
struct WACOMDATA_API FWacomMapShopPayload
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Map|Content",
		meta = (ToolTip = "Shop 节点使用的商店定义。运行时库存和访问状态由 WacomRun 保存。"))
	TObjectPtr<UShopDefinition> ShopDefinition = nullptr;
};

USTRUCT(BlueprintType)
struct WACOMDATA_API FWacomMapTreasurePayload
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Map|Content",
		meta = (ToolTip = "Treasure 节点使用的普通拾取定义。与 WorldCardInteractionDefinition 必须二选一。"))
	TObjectPtr<UWacomRunPickupDefinition> PickupDefinition = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Map|Content",
		meta = (ToolTip = "Treasure 节点使用的卡牌世界交互定义。与 PickupDefinition 必须二选一。"))
	TObjectPtr<UWacomRunWorldCardInteractionDefinition> WorldCardInteractionDefinition = nullptr;
};

USTRUCT(BlueprintType)
struct WACOMDATA_API FWacomMapFloorEntrancePayload
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Map|Content",
		meta = (ToolTip = "Floor Entrance 指向的后续 FloorId。只能指向当前 Journey 中更后的 Floor。"))
	FName TargetFloorId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Map|Content",
		meta = (ToolTip = "进入后续 Floor 必须全部满足的持有卡牌条件。只检查真实持有区且不消耗卡牌。"))
	TArray<FWacomOwnedCardRequirement> OwnedCardRequirements;
};

/** 固定 tagged payload；只有与 NodeType 匹配的字段允许非空。 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FWacomMapNodeContent
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Map|Content",
		meta = (ToolTip = "Encounter 节点的静态内容。仅当 NodeType=Encounter 时允许配置，其它节点必须保持为空。"))
	FWacomMapEncounterPayload Encounter;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Map|Content",
		meta = (ToolTip = "RunEvent 节点的静态内容。仅当 NodeType=RunEvent 时允许配置，其它节点必须保持为空。"))
	FWacomMapRunEventPayload RunEvent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Map|Content",
		meta = (ToolTip = "Shop 节点的静态内容。仅当 NodeType=Shop 时允许配置，其它节点必须保持为空。"))
	FWacomMapShopPayload Shop;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Map|Content",
		meta = (ToolTip = "Treasure 节点的静态内容。仅当 NodeType=Treasure 时允许配置，其它节点必须保持为空。"))
	FWacomMapTreasurePayload Treasure;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Map|Content",
		meta = (ToolTip = "FloorEntrance 节点的静态内容。仅当 NodeType=FloorEntrance 时允许配置，其它节点必须保持为空。"))
	FWacomMapFloorEntrancePayload FloorEntrance;
};

USTRUCT(BlueprintType)
struct WACOMDATA_API FWacomMapNodeDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Map",
		meta = (ToolTip = "节点在所属 Floor 内的稳定 ID。非空且同 Floor 唯一。"))
	FName NodeId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Map",
		meta = (ToolTip = "节点的玩家可读标题。用于地图界面和本地化，不替代 NodeId；正式内容必须非空。"))
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Map",
		meta = (ToolTip = "节点已揭示后显示的可选简述。留空表示不显示说明，不产生制作错误。"))
	FText ShortDescription;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Map",
		meta = (ToolTip = "节点的规则类型。决定 Content 中唯一允许使用的 typed payload。"))
	EWacomMapNodeType NodeType = EWacomMapNodeType::Navigation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Map",
		meta = (ToolTip = "地图页面上的设计坐标，单位为 1920x1080 设计画布像素；只用于排版，不参与规则距离、可达性或世界定位。"))
	FVector2D MapPosition = FVector2D::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Map",
		meta = (ToolTip = "该节点在 Resolved 后是否允许作为 Camp 落点。Camp 仍是 Night 事务，不会把节点改成营地类型。"))
	bool bAllowsCamp = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Map",
		meta = (ToolTip = "可选远景地标提示。只影响 Snapshot 展示事实，不会推进 Hidden/Revealed 生命周期。"))
	EWacomMapLandmarkVisibility LandmarkVisibility = EWacomMapLandmarkVisibility::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Map",
		meta = (ToolTip = "节点静态内容。只有与 NodeType 匹配的 typed payload 可以配置。"))
	FWacomMapNodeContent Content;
};

USTRUCT(BlueprintType)
struct WACOMDATA_API FWacomMapEdgeDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Map",
		meta = (ToolTip = "边在所属 Floor 内的稳定 ID。非空且同 Floor 唯一。"))
	FName EdgeId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Map",
		meta = (ToolTip = "有向边的起点 NodeId。必须属于同一 Floor。"))
	FName FromNodeId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Map",
		meta = (ToolTip = "有向边的终点 NodeId。必须属于同一 Floor，且不能与起点相同。"))
	FName ToNodeId = NAME_None;
};
