// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ShopDefinition.generated.h"

class UCardDefinition;

/** 静态商店商品定义。运行时库存状态由 WacomRun 保存，不写在这里。 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FShopOfferDefinition
{
	GENERATED_BODY()

	/** 商品对应的卡牌定义。为空的条目会在打开商店时被跳过。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Shop",
		meta = (ToolTip = "商品对应的卡牌定义。为空的条目会在打开商店时被跳过。"))
	TObjectPtr<UCardDefinition> CardDefinition = nullptr;

	/** 商品价格，单位为金币。0 表示免费商品；负数不会出现在编辑器输入中。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Shop",
		meta = (ToolTip = "商品价格，单位为金币。0 表示免费商品；本字段只定义静态价格，不保存购买状态。",
			ClampMin = "0", UIMin = "0"))
	int32 Price = 0;
};

/**
 * 商店静态内容定义。
 *
 * 本资产只描述商品内容；当前 Run 内的库存、已购买状态和关闭扣节点仍由 URunSession 使用场景
 * AWacomShopTriggerActor.PersistentId 保存。
 */
UCLASS(BlueprintType)
class WACOMDATA_API UShopDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 商店内容 ID，用于内容识别和显示调试；不作为运行时库存持久化 key。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Shop",
		meta = (ToolTip = "商店内容 ID，用于内容识别和显示调试；不作为运行时库存持久化 key，库存 key 仍来自场景商店 Actor 的 PersistentId。"))
	FName ShopId = NAME_None;

	/** 商店显示名。第一版 ShopScreen 暂未美术化，后续 UI 可用它替代调试 ID。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Shop",
		meta = (ToolTip = "商店显示名。第一版 ShopScreen 暂未美术化，后续 UI 可用它替代调试 ID。"))
	FText DisplayName;

	/** 固定商品列表。第一版不做随机池、权重和价格公式。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Shop",
		meta = (ToolTip = "固定商品列表。第一版不做随机池、权重和价格公式；运行时购买状态由 RunSession 保存。"))
	TArray<FShopOfferDefinition> Offers;
};
