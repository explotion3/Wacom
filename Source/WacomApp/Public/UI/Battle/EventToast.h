// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleWidgetBase.h"
#include "EventToast.generated.h"

class UVerticalBox;
class UTextBlock;
struct FBattleEvent;

/**
 * 战斗事件 Toast。
 *
 * 由 UBattleHUD 在每次命令完成后调 EnqueueEvents 塞入一批事件。
 * 本 Widget 把每条事件格式化成一行文字显示在一个 VerticalBox 里。
 * 每条消息默认 3 秒后淡出。超过最大条数时最旧的会被立即移除。
 *
 * 第一阶段用 NativeTick 做倒计时；P5 UI 动画时换成 UMG Animation。
 *
 * C++ 默认外观：半透明黑底竖向列表，每行一个 TextBlock。
 *
 * WBP 约定（可选）：
 * - Container : UVerticalBox   （BindWidgetOptional）
 */
UCLASS(Blueprintable)
class WACOMAPP_API UEventToast : public UWacomBattleWidgetBase
{
	GENERATED_BODY()

public:
	UEventToast();

	/** 把一批事件压入队列。 */
	void EnqueueEvents(const TArray<FBattleEvent>& Events);

	/** 将战斗事件格式化为玩家可读中文提示。空字符串表示不显示该事件。 */
	static FString FormatEventForPlayer(const FBattleEvent& Event);

	/** 每条消息的显示时长（秒）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI")
	float MessageLifetime = 3.0f;

	/** 最多同时显示多少条。超过时最旧的立即移除。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI")
	int32 MaxVisibleMessages = 5;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& InGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> Container;

private:
	struct FActiveMessage
	{
		TObjectPtr<UTextBlock> Text;
		float RemainingTime = 0.0f;
	};

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> ActiveTexts;

	// 并行数组：ActiveTexts[i] 的剩余时间 = ActiveRemaining[i]。
	// 用并行数组而非 FActiveMessage 是为了让 Transient UPROPERTY 更简单。
	TArray<float> ActiveRemaining;

	void PushMessage(const FString& Message);
	void RemoveAt(int32 Index);
};
