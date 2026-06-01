// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "RunWorldCardInteractionDefinition.generated.h"

class UCardDefinition;

UENUM(BlueprintType)
enum class EWacomRunWorldCardInteractionRewardType : uint8
{
	None UMETA(DisplayName = "None"),
	Gold UMETA(DisplayName = "Gold"),
	Card UMETA(DisplayName = "Card"),
};

USTRUCT(BlueprintType)
struct WACOMDATA_API FWacomRunWorldCardInteractionReward
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|World Card Interaction|Reward",
		meta = (ToolTip = "奖励类型。V1 只支持 Gold 和 Card；None 会被视为无效配置。"))
	EWacomRunWorldCardInteractionRewardType Type =
		EWacomRunWorldCardInteractionRewardType::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|World Card Interaction|Reward",
		meta = (ToolTip = "金币奖励数量。仅 Type=Gold 使用；单位：金币。",
			ClampMin = "0", UIMin = "0"))
	int32 GoldAmount = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|World Card Interaction|Reward",
		meta = (ToolTip = "卡牌奖励定义。仅 Type=Card 使用；成功后获得一张该卡牌。"))
	TObjectPtr<UCardDefinition> CardDefinition = nullptr;
};

/**
 * Run 世界拖卡交互的通用静态定义。
 *
 * 本资产只描述交互接受哪些卡、奖励和反馈文案；运行时完成状态 key 仍来自场景 Actor 的
 * PersistentId。
 */
UCLASS(BlueprintType)
class WACOMDATA_API UWacomRunWorldCardInteractionDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 交互内容 ID，用于内容识别、调试和校验；不作为运行时完成状态 key。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|World Card Interaction",
		meta = (ToolTip = "交互内容 ID，用于内容识别、调试和校验；不作为运行时完成状态 key，运行时 key 仍来自场景 Actor 的 PersistentId。"))
	FName InteractionId = NAME_None;

	/** 允许提交的卡牌定义。与 AllowedCardIds 是 OR 关系；至少需要一个正向筛选。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|World Card Interaction|Card Filter",
		meta = (ToolTip = "允许提交的卡牌定义。与 AllowedCardIds 是 OR 关系；AllowedCardDefinitions / AllowedCardIds / RequiredKeywords 至少需要一个非空。"))
	TArray<TObjectPtr<UCardDefinition>> AllowedCardDefinitions;

	/** 允许提交的 CardId。与 AllowedCardDefinitions 是 OR 关系；至少需要一个正向筛选。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|World Card Interaction|Card Filter",
		meta = (ToolTip = "允许提交的 CardId 列表。与 AllowedCardDefinitions 是 OR 关系；AllowedCardDefinitions / AllowedCardIds / RequiredKeywords 至少需要一个非空。"))
	TArray<FName> AllowedCardIds;

	/** 提交卡必须全部拥有的关键词。非空时也算正向筛选。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|World Card Interaction|Card Filter",
		meta = (ToolTip = "提交卡必须全部拥有的关键词。非空时算正向筛选；读取卡牌定义上的 Card.Keyword。"))
	FGameplayTagContainer RequiredKeywords;

	/** 提交卡不能拥有的关键词。只填黑名单不算有效配置。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|World Card Interaction|Card Filter",
		meta = (ToolTip = "提交卡不能拥有的关键词。只作为附加限制；只填黑名单不算有效配置。"))
	FGameplayTagContainer BlockedKeywords;

	/** 拖卡提交成功时按顺序发放的奖励。V1 只支持金币和固定卡牌奖励。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|World Card Interaction|Reward",
		meta = (ToolTip = "拖卡提交成功时按顺序发放的奖励。V1 只支持金币和固定卡牌奖励；至少需要一个有效奖励。"))
	TArray<FWacomRunWorldCardInteractionReward> Rewards;

	/** 拖卡提交成功时是否永久消耗拖拽的精确卡牌实例。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|World Card Interaction|Reward",
		meta = (ToolTip = "拖卡提交成功时是否永久消耗拖拽的精确卡牌实例。"))
	bool bConsumeCardOnSuccess = true;

	/** 拖拽卡牌指向目标但尚未释放时的调试/预览文案；为空则使用 receiver 默认文案。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|World Card Interaction|Text",
		meta = (ToolTip = "拖拽卡牌指向目标但尚未释放时的调试/预览文案；为空则使用 receiver 默认文案。"))
	FText PreviewPromptText;

	/** 提交成功后的调试文案；为空则使用 receiver 默认文案。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|World Card Interaction|Text",
		meta = (ToolTip = "提交成功后的调试文案；为空则使用 receiver 默认文案。"))
	FText SuccessPromptText;

	/** 目标已完成后拖拽指向或释放时的提示文案；为空则使用 receiver 默认文案。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|World Card Interaction|Text",
		meta = (ToolTip = "目标已完成后拖拽指向或释放时的提示文案；为空则使用 receiver 默认文案。"))
	FText CompletedPromptText;

	/** 拖拽释放时卡牌不被目标接受的失败提示。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|World Card Interaction|Text",
		meta = (ToolTip = "拖拽释放时卡牌不被目标接受的失败提示。用于错卡、缺少关键词、命中黑名单或源卡缺定义等情况；为空则使用 receiver 默认文案。"))
	FText RejectedCardPromptText;

	/** 拖拽释放时目标配置异常的失败提示前缀。最终 Toast 会追加具体 Reason。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|World Card Interaction|Text",
		meta = (ToolTip = "拖拽释放时目标配置异常的失败提示前缀。最终 Toast 会追加具体 Reason，便于 PIE 排查；为空则使用 receiver 默认文案。"))
	FText ConfigWarningPromptText;

	/** 拖拽释放时源卡不可用的失败提示。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|World Card Interaction|Text",
		meta = (ToolTip = "拖拽释放时源卡不可用的失败提示，例如卡不存在、未持有、固有卡或最后容量来源卡；为空则使用 receiver 默认文案。"))
	FText SourceCardUnavailablePromptText;

	/** 拖拽释放失败但没有更具体分类时的通用提示前缀。最终 Toast 会追加具体 Reason。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run|World Card Interaction|Text",
		meta = (ToolTip = "拖拽释放失败但没有更具体分类时的通用提示前缀。最终 Toast 会追加具体 Reason；为空则使用 receiver 默认文案。"))
	FText GenericFailurePromptText;

	/** 返回当前通用拖卡交互定义配置的阻断原因；None 表示配置有效。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|World Card Interaction|Validation",
		meta = (ToolTip = "返回当前通用拖卡交互定义配置的阻断原因；None 表示配置有效。"))
	FName GetConfigWarningReason() const;

	/** 当前定义是否可用于制作期配置同步和运行时拖卡交互。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|World Card Interaction|Validation",
		meta = (ToolTip = "当前定义是否可用于制作期配置同步和运行时拖卡交互。"))
	bool IsConfigValid() const { return GetConfigWarningReason().IsNone(); }
};
