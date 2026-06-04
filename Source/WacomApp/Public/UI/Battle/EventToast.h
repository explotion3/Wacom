// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleWidgetBase.h"
#include "EventToast.generated.h"

class UVerticalBox;
class UTextBlock;
struct FBattleEventPresentationView;
struct FBattleEvent;

/**
 * Legacy 战斗事件 Toast。
 *
 * 该 Widget 只为旧 WBP / PIE 对照保留。当前正式 BattleHUD 主路径使用
 * CombatLogFeed + BattleCombatLogBlock，不再由 BattleHUD 创建旧 EventToast。
 *
 * C++ fallback 外观：半透明黑底竖向列表，每行一个 TextBlock。
 *
 * WBP 约定（可选）：
 * - Container : UVerticalBox   （BindWidgetOptional）
 */
UCLASS(Blueprintable, meta = (ToolTip = "Legacy 战斗事件 Toast，只为旧 WBP 或 PIE 对照保留。新的 BattleHUD 制作应使用 CombatLogFeed + BattleCombatLogBlock，不要重新绑定 EventToast。"))
class WACOMAPP_API UEventToast : public UWacomBattleWidgetBase
{
	GENERATED_BODY()

public:
	UEventToast();

	/** 把一批事件压入队列。 */
	void EnqueueEvents(const TArray<FBattleEvent>& Events);

	/** 把已经构建好的表现 View 压入显示队列。 */
	void EnqueuePresentationView(const FBattleEventPresentationView& View);

	/** 将战斗事件格式化为玩家可读中文提示。空字符串表示不显示该事件。 */
	static FString FormatEventForPlayer(const FBattleEvent& Event);

	/** 每条消息的显示时长（秒）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Legacy Event Log|Compatibility", meta = (ToolTip = "Legacy EventToast 单条消息的显示时长，单位秒。只影响旧 Toast 兼容组件，不影响当前正式 CombatLogFeed。"))
	float MessageLifetime = 3.0f;

	/** 最多同时显示多少条。超过时最旧的立即移除。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Legacy Event Log|Compatibility", meta = (ToolTip = "Legacy EventToast 最多同时显示的消息条数。只用于旧 WBP / PIE 对照，不是新的 BattleHUD 日志配置。"))
	int32 MaxVisibleMessages = 5;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& InGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> Container;

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

private:
	void PushMessage(const FString& Message);
	void RemoveAt(int32 Index);
};
