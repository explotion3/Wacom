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
	bool bHasRunSession = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Card Drop|Debug")
	bool bCompleted = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Card Drop|Debug")
	bool bCanSubmit = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Card Drop|Debug")
	FName RejectReason = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Card Drop|Debug")
	int32 AllowedDefinitionCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Card Drop|Debug")
	int32 AllowedCardIdCount = 0;

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

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|World Card Drop|Debug")
	FString GetRunWorldCardDropReceiverDebugSummary(
		AWacomPlayerController* PC,
		FName PersistentId,
		FGuid SourceCardInstanceId) const;

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
	FText GetDefaultPreviewPromptText() const;
	FText GetDefaultSuccessPromptText() const;
	FText GetDefaultCompletedPromptText() const;
};
