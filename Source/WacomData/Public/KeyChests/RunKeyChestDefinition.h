// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "RunKeyChestDefinition.generated.h"

class UCardDefinition;

/**
 * Run 世界钥匙宝箱的静态交互定义。
 *
 * 本资产只描述宝箱接受哪些卡、奖励和提示文案；运行时完成状态 key 仍来自场景
 * AWacomRunKeyChestActor.PersistentId。
 */
UCLASS(BlueprintType)
class WACOMDATA_API UWacomRunKeyChestDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 宝箱内容 ID，用于内容识别、调试和校验；不作为运行时完成状态 key。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|Key Chest",
		meta = (ToolTip = "宝箱内容 ID，用于内容识别、调试和校验；不作为运行时完成状态 key，运行时 key 仍来自场景 KeyChest Actor 的 PersistentId。"))
	FName ChestId = NAME_None;

	/** 允许开箱的卡牌定义。与 AllowedCardIds / RequiredKeywords 是 OR/筛选组合；至少需要一个正向筛选。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Card Filter",
		meta = (ToolTip = "允许开箱的卡牌定义。与 AllowedCardIds 是 OR 关系；AllowedCardDefinitions / AllowedCardIds / RequiredKeywords 至少需要一个非空。"))
	TArray<TObjectPtr<UCardDefinition>> AllowedCardDefinitions;

	/** 允许开箱的 CardId。与 AllowedCardDefinitions 是 OR 关系；至少需要一个正向筛选。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Card Filter",
		meta = (ToolTip = "允许开箱的 CardId 列表。与 AllowedCardDefinitions 是 OR 关系；AllowedCardDefinitions / AllowedCardIds / RequiredKeywords 至少需要一个非空。"))
	TArray<FName> AllowedCardIds;

	/** 开箱卡必须全部拥有的关键词。非空时也算正向筛选。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Card Filter",
		meta = (ToolTip = "开箱卡必须全部拥有的关键词。非空时算正向筛选；读取卡牌定义上的 Card.Keyword。"))
	FGameplayTagContainer RequiredKeywords;

	/** 开箱卡不能拥有的关键词。只填黑名单不算有效配置。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Card Filter",
		meta = (ToolTip = "开箱卡不能拥有的关键词。只作为附加限制；只填黑名单不算有效配置。"))
	FGameplayTagContainer BlockedKeywords;

	/** 拖卡开箱成功时获得的金币数量。V1 只支持正数金币奖励。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Reward",
		meta = (ToolTip = "拖卡开箱成功时获得的金币数量。V1 只支持正数金币奖励；单位：金币。",
			ClampMin = "1", UIMin = "1"))
	int32 GoldReward = 3;

	/** 拖卡开箱成功时是否永久消耗拖拽的精确卡牌实例。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Reward",
		meta = (ToolTip = "拖卡开箱成功时是否永久消耗拖拽的精确卡牌实例。"))
	bool bConsumeCardOnSuccess = true;

	/** 未打开时，玩家在 E 键范围内看到的提示文案；为空则回退 Actor 字段。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Text",
		meta = (ToolTip = "未打开时，玩家在 E 键范围内看到的提示文案；为空则回退 Actor 字段，再回退默认文案。"))
	FText InteractPromptText;

	/** 鼠标 hover 到宝箱 ClickBounds 时看到的提示文案；为空则回退 Actor 字段。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Text",
		meta = (ToolTip = "鼠标 hover 到宝箱 ClickBounds 时看到的提示文案；为空则回退 Actor 字段，再回退默认文案。"))
	FText HoverPromptText;

	/** 宝箱已经打开后显示的弱提示文案；为空则回退 Actor 字段。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Text",
		meta = (ToolTip = "宝箱已经打开后显示的弱提示文案；为空则回退 Actor 字段，再回退默认文案。"))
	FText CompletedPromptText;

	/** 拖拽卡牌指向目标但尚未释放时的调试/预览文案；为空则使用 receiver 默认文案。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Receiver Text",
		meta = (ToolTip = "拖拽卡牌指向目标但尚未释放时的调试/预览文案；为空则使用 receiver 默认文案。"))
	FText PreviewPromptText;

	/** 提交成功后的调试文案；为空则使用 receiver 默认文案。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Receiver Text",
		meta = (ToolTip = "提交成功后的调试文案；为空则使用 receiver 默认文案。"))
	FText SuccessPromptText;

	/** 目标已完成后拖拽指向时的调试文案；为空则回退 CompletedPromptText 或 receiver 默认文案。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Receiver Text",
		meta = (ToolTip = "目标已完成后拖拽指向时的调试文案；为空则回退 CompletedPromptText 或 receiver 默认文案。"))
	FText ReceiverCompletedPromptText;

	/** 返回当前宝箱定义配置的阻断原因；None 表示配置有效。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Key Chest|Validation",
		meta = (ToolTip = "返回当前宝箱定义配置的阻断原因；None 表示配置有效。"))
	FName GetConfigWarningReason() const;

	/** 当前 KeyChestDefinition 是否可用于制作期配置同步和运行时拖卡开箱。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Key Chest|Validation",
		meta = (ToolTip = "当前 KeyChestDefinition 是否可用于制作期配置同步和运行时拖卡开箱。"))
	bool IsConfigValid() const { return GetConfigWarningReason().IsNone(); }
};
