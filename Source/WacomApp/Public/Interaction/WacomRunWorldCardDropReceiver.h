// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "RunStateTypes.h"
#include "UObject/Interface.h"
#include "WacomRunWorldCardDropReceiver.generated.h"

class AWacomPlayerController;
class UCardDefinition;
class URunSession;
class UWacomRunWorldCardInteractionDefinition;

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomRunWorldCardDropReceiverDebugView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Card Drop|Debug")
	FString ReceiverName;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Card Drop|Debug")
	FString OwnerName;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Card Drop|Debug")
	FName PersistentId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Card Drop|Debug")
	FName DefinitionName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Card Drop|Debug")
	FName InteractionId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Card Drop|Debug")
	FName DefinitionConfigWarningReason = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Card Drop|Debug")
	FName ConfigSource = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Card Drop|Debug")
	bool bHasRunSession = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Card Drop|Debug")
	bool bCompleted = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Card Drop|Debug")
	bool bCanSubmit = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Card Drop|Debug")
	FName RejectReason = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Card Drop|Debug")
	bool bConfigValid = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Card Drop|Debug")
	FName ConfigWarningReason = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Card Drop|Debug")
	int32 AllowedDefinitionCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Card Drop|Debug")
	int32 AllowedCardIdCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Card Drop|Debug")
	int32 RequiredKeywordCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Card Drop|Debug")
	int32 BlockedKeywordCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Card Drop|Debug")
	bool bHasPositiveCardFilter = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Card Drop|Debug")
	bool bConsumeCardOnSuccess = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Card Drop|Debug")
	int32 GoldReward = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Card Drop|Debug")
	FString PreviewPrompt;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Card Drop|Debug")
	FString SuccessPrompt;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Card Drop|Debug")
	FString CompletedPrompt;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Card Drop|Debug")
	FString RejectedCardPrompt;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Card Drop|Debug")
	FString ConfigWarningPrompt;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Card Drop|Debug")
	FString SourceCardUnavailablePrompt;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Card Drop|Debug")
	FString GenericFailurePrompt;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Card Drop|Debug")
	FString RunValidationSummary;
};

UINTERFACE(BlueprintType)
class WACOMAPP_API UWacomRunWorldCardDropReceiver : public UInterface
{
	GENERATED_BODY()
};

class WACOMAPP_API IWacomRunWorldCardDropReceiver
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Wacom|Run|World Card Drop")
	FRunWorldCardInteractionRequest BuildRunWorldCardDropRequest(
		FName PersistentId,
		const FGuid& SourceCardInstanceId) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Wacom|Run|World Card Drop")
	FRunWorldCardInteractionValidation ValidateRunWorldCardDrop(
		AWacomPlayerController* PC,
		FName PersistentId,
		const FGuid& SourceCardInstanceId) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Wacom|Run|World Card Drop")
	bool SubmitRunWorldCardDrop(
		AWacomPlayerController* PC,
		FName PersistentId,
		const FGuid& SourceCardInstanceId,
		FRunWorldCardInteractionValidation& OutValidation);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Wacom|Run|World Card Drop|Debug")
	FWacomRunWorldCardDropReceiverDebugView GetRunWorldCardDropReceiverDebugView(
		AWacomPlayerController* PC,
		FName PersistentId,
		const FGuid& SourceCardInstanceId) const;
};

UCLASS(ClassGroup = (Wacom), meta = (BlueprintSpawnableComponent, ToolTip = "Run 场景物体接收第一人称卡牌拖拽的默认配置组件。只构建/提交 RunSession 事务，不直接修改 RunState。"))
class WACOMAPP_API UWacomRunWorldCardDropReceiverComponent
	: public UActorComponent
	, public IWacomRunWorldCardDropReceiver
{
	GENERATED_BODY()

public:
	UWacomRunWorldCardDropReceiverComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|World Card Drop|Definition",
		meta = (ToolTip = "通用 Run 世界拖卡交互定义。填入后优先使用 Definition 的筛选、金币、消耗和反馈文案；运行时完成状态 key 仍由目标 Actor 的 PersistentId 提供。"))
	TObjectPtr<UWacomRunWorldCardInteractionDefinition> InteractionDefinition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|World Card Drop",
		meta = (ToolTip = "允许提交的卡牌定义资产。与 AllowedCardIds 是 OR 关系；为空表示不按定义筛选。"))
	TArray<TObjectPtr<UCardDefinition>> AllowedCardDefinitions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|World Card Drop",
		meta = (ToolTip = "允许提交的 CardId 列表。与 AllowedCardDefinitions 是 OR 关系；为空表示不按 CardId 筛选。"))
	TArray<FName> AllowedCardIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|World Card Drop",
		meta = (ToolTip = "提交卡必须全部拥有的关键词。读取玩家持有卡实例对应定义上的 Card.Keyword。"))
	FGameplayTagContainer RequiredKeywords;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|World Card Drop",
		meta = (ToolTip = "提交卡不能拥有的关键词。命中任意一个即被拒绝。"))
	FGameplayTagContainer BlockedKeywords;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|World Card Drop",
		meta = (ToolTip = "提交成功时是否永久消耗拖拽的精确卡牌实例。"))
	bool bConsumeCardOnSuccess = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|World Card Drop",
		meta = (ToolTip = "提交成功时获得的金币数量。V1 只支持正数金币奖励。",
			ClampMin = "1", UIMin = "1"))
	int32 GoldReward = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|World Card Drop|Text",
		meta = (ToolTip = "拖拽卡牌指向目标但尚未释放时的调试/预览文案。"))
	FText PreviewPromptText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|World Card Drop|Text",
		meta = (ToolTip = "提交成功后的调试文案。"))
	FText SuccessPromptText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|World Card Drop|Text",
		meta = (ToolTip = "目标已完成后拖拽指向时的调试文案。"))
	FText CompletedPromptText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|World Card Drop|Text",
		meta = (ToolTip = "拖拽释放时卡牌不被目标接受的失败提示。用于错卡、缺少关键词、命中黑名单或源卡缺定义等情况。"))
	FText RejectedCardPromptText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|World Card Drop|Text",
		meta = (ToolTip = "拖拽释放时目标配置异常的失败提示前缀。最终 Toast 会追加具体 Reason，便于 PIE 排查。"))
	FText ConfigWarningPromptText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|World Card Drop|Text",
		meta = (ToolTip = "拖拽释放时源卡不可用的失败提示，例如卡不存在、未持有、固有卡或最后容量来源卡。"))
	FText SourceCardUnavailablePromptText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|World Card Drop|Text",
		meta = (ToolTip = "拖拽释放失败但没有更具体分类时的通用提示前缀。最终 Toast 会追加具体 Reason。"))
	FText GenericFailurePromptText;

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|World Card Drop|Debug")
	FString GetRunWorldCardDropReceiverDebugSummary(
		AWacomPlayerController* PC,
		FName PersistentId,
		FGuid SourceCardInstanceId) const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|World Card Drop|Debug")
	bool HasPositiveCardFilter() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|World Card Drop|Debug")
	FName GetRunWorldCardDropReceiverConfigWarningReason() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|World Card Drop|Feedback")
	virtual FText BuildRunWorldCardDropFailureToastText(
		AWacomPlayerController* PC,
		FName PersistentId,
		FGuid SourceCardInstanceId,
		FName FailureReason) const;

	virtual FRunWorldCardInteractionRequest BuildRunWorldCardDropRequest_Implementation(
		FName PersistentId,
		const FGuid& SourceCardInstanceId) const override;

	virtual FRunWorldCardInteractionValidation ValidateRunWorldCardDrop_Implementation(
		AWacomPlayerController* PC,
		FName PersistentId,
		const FGuid& SourceCardInstanceId) const override;

	virtual bool SubmitRunWorldCardDrop_Implementation(
		AWacomPlayerController* PC,
		FName PersistentId,
		const FGuid& SourceCardInstanceId,
		FRunWorldCardInteractionValidation& OutValidation) override;

	virtual FWacomRunWorldCardDropReceiverDebugView GetRunWorldCardDropReceiverDebugView_Implementation(
		AWacomPlayerController* PC,
		FName PersistentId,
		const FGuid& SourceCardInstanceId) const override;

protected:
	const TArray<TObjectPtr<UCardDefinition>>& ResolveAllowedCardDefinitions() const;
	const TArray<FName>& ResolveAllowedCardIds() const;
	const FGameplayTagContainer& ResolveRequiredKeywords() const;
	const FGameplayTagContainer& ResolveBlockedKeywords() const;
	bool ResolveConsumeCardOnSuccess() const;
	int32 ResolveGoldReward() const;
	FText ResolvePreviewPromptText() const;
	FText ResolveSuccessPromptText() const;
	FText ResolveCompletedPromptText() const;
	FText ResolveRejectedCardPromptText() const;
	FText ResolveConfigWarningPromptText() const;
	FText ResolveSourceCardUnavailablePromptText() const;
	FText ResolveGenericFailurePromptText() const;
	FText GetDefaultPreviewPromptText() const;
	FText GetDefaultSuccessPromptText() const;
	FText GetDefaultCompletedPromptText() const;
	FText GetDefaultRejectedCardPromptText() const;
	FText GetDefaultConfigWarningPromptText() const;
	FText GetDefaultSourceCardUnavailablePromptText() const;
	FText GetDefaultGenericFailurePromptText() const;
};
