// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "PileCountView.generated.h"

class UTextBlock;
class UWidget;
struct FWacomPileFeedbackPlayback;
struct FWacomPileCountViewTestAccess;

DECLARE_MULTICAST_DELEGATE(FWacomPileDetailsRequestedNative);

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomPileReceiveFeedbackStyle
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Common UI|Pile Count|Receive Feedback", meta = (ToolTip = "是否允许牌堆在收到卡牌或牌印时播放接收反馈。关闭后仍正常更新数量，不修改布局或规则状态。"))
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Common UI|Pile Count|Receive Feedback", meta = (Units = "s", ToolTip = "接收反馈从开始到完全归位的总时长，单位为秒；默认 0.18，推荐 0.12 到 0.28。只修改 RenderTransform，不影响布局。"))
	float DurationSeconds = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Common UI|Pile Count|Receive Feedback", meta = (Units = "s", ToolTip = "接收反馈达到压缩峰值的时间，单位为秒；默认 0.04，推荐为总时长的 15% 到 30%。运行时会约束在总时长内。"))
	float CompressionPeakSeconds = 0.04f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Common UI|Pile Count|Receive Feedback", meta = (Units = "s", ToolTip = "接收反馈从开始到达到回弹峰值的时间，单位为秒；默认 0.09，推荐为总时长的 40% 到 60%。运行时不得早于压缩峰值。"))
	float ReboundPeakSeconds = 0.09f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Common UI|Pile Count|Receive Feedback", meta = (ToolTip = "牌印抵达时牌堆在压缩峰值的局部缩放；默认 1.03×0.94，推荐 X 1.00 到 1.08、Y 0.88 到 0.98。"))
	FVector2D CompressionScale = FVector2D(1.03f, 0.94f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Common UI|Pile Count|Receive Feedback", meta = (ToolTip = "牌堆在单次回弹峰值的局部缩放；默认 1.08×1.08，推荐 1.03 到 1.14。"))
	FVector2D ReboundScale = FVector2D(1.08f, 1.08f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Common UI|Pile Count|Receive Feedback", meta = (ToolTip = "压缩峰值向下移动的距离，单位为 UMG 逻辑像素；默认 2，推荐 0 到 5。只修改 RenderTransform。"))
	float CompressionTranslationPixels = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Common UI|Pile Count|Receive Feedback", meta = (ToolTip = "回弹峰值的纵向位移，单位为 UMG 逻辑像素；默认 -1 表示轻微向上，推荐 -3 到 0。只修改 RenderTransform。"))
	float ReboundTranslationPixels = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Common UI|Pile Count|Receive Feedback", meta = (ToolTip = "回弹峰值时数量文字的附加缩放；默认 1.12，推荐 1.04 到 1.20。该值叠加在牌堆整体缩放之上。"))
	float CountPulseScale = 1.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Common UI|Pile Count|Receive Feedback", meta = (ToolTip = "批次最后一枚牌印抵达时的强度倍率；默认 1.20，推荐 1.00 到 1.45。"))
	float FinalArrivalStrengthMultiplier = 1.20f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Common UI|Pile Count|Receive Feedback", meta = (ToolTip = "连续抵达脉冲叠加后的最大表现强度；默认 1.35，推荐 1.10 到 1.70，用于避免快速批量弃牌导致牌堆过度放大。"))
	float MaxCombinedStrength = 1.35f;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomPileSendFeedbackStyle
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Common UI|Pile Count|Send Feedback", meta = (ToolTip = "是否允许牌堆在发出卡牌时播放方向性后坐反馈。关闭后仍按真实发牌时刻逐张更新数量。"))
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Common UI|Pile Count|Send Feedback", meta = (ToolTip = "是否弱化发牌动效。开启后仍逐张更新数量并保留现有抽牌音，但不修改牌堆或数字的缩放与位移。"))
	bool bReduceMotion = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Common UI|Pile Count|Send Feedback", meta = (Units = "s", ToolTip = "发牌反馈从启动到完全归位的总时长，单位为秒；默认 0.16，推荐 0.10 到 0.24。只修改 RenderTransform，不影响布局。"))
	float DurationSeconds = 0.16f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Common UI|Pile Count|Send Feedback", meta = (Units = "s", ToolTip = "发牌反馈达到压缩峰值的时间，单位为秒；默认 0.025，推荐为总时长的 10% 到 25%。运行时会约束在总时长内。"))
	float CompressionPeakSeconds = 0.025f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Common UI|Pile Count|Send Feedback", meta = (Units = "s", ToolTip = "发牌反馈达到反向后坐峰值的时间，单位为秒；默认 0.075，推荐为总时长的 35% 到 60%。运行时不得早于压缩峰值。"))
	float RecoilPeakSeconds = 0.075f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Common UI|Pile Count|Send Feedback", meta = (ToolTip = "卡牌刚离堆时牌堆的局部压缩缩放；默认 1.03×0.95，推荐 X 1.00 到 1.08、Y 0.88 到 0.99。"))
	FVector2D CompressionScale = FVector2D(1.03f, 0.95f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Common UI|Pile Count|Send Feedback", meta = (ToolTip = "牌堆在反向后坐峰值的局部缩放；默认 1.06×1.06，推荐 1.02 到 1.12。"))
	FVector2D RecoilScale = FVector2D(1.06f, 1.06f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Common UI|Pile Count|Send Feedback", meta = (ToolTip = "压缩阶段沿卡牌发出方向移动的距离，单位为 UMG 逻辑像素；默认 1.5，推荐 0 到 4。"))
	float CompressionTranslationPixels = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Common UI|Pile Count|Send Feedback", meta = (ToolTip = "后坐阶段沿卡牌发出反方向移动的距离，单位为 UMG 逻辑像素；默认 3，推荐 1 到 7。"))
	float RecoilTranslationPixels = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Common UI|Pile Count|Send Feedback", meta = (ToolTip = "压缩峰值时数量文字的局部缩放；默认 0.94，推荐 0.88 到 0.98。"))
	float CountCompressionScale = 0.94f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Common UI|Pile Count|Send Feedback", meta = (ToolTip = "后坐峰值时数量文字的局部缩放；默认 1.08，推荐 1.03 到 1.16。"))
	float CountRecoilScale = 1.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Common UI|Pile Count|Send Feedback", meta = (ToolTip = "当前抽牌批次最后一张可见卡发出时的强度倍率；默认 1.12，推荐 1.00 到 1.35。"))
	float FinalDepartureStrengthMultiplier = 1.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Common UI|Pile Count|Send Feedback", meta = (ToolTip = "连续发牌脉冲叠加后的最大表现强度；默认 1.30，推荐 1.10 到 1.60。"))
	float MaxCombinedStrength = 1.30f;
};

/**
 * 通用计数小 Widget。用于抽牌堆/弃牌堆/消耗牌堆等显示。
 *
 * C++ 默认外观：居中的大数字。
 *
 * WBP 约定：
 * - CountText : UTextBlock
 *
 * 牌堆类型由 WBP 自行放置 Image 素材识别；C++ 只负责刷新数量文本。
 */
UCLASS(Blueprintable, meta = (ToolTip = "通用数量显示 Widget。用于 BattleHUD 抽牌堆、弃牌堆、消耗牌堆等计数展示；只刷新数量文本，不修改牌堆或规则状态。"))
class WACOMAPP_API UPileCountView : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UPileCountView(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual ~UPileCountView() override;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Common UI|Pile Count", meta = (ToolTip = "设置当前显示数量。只更新该 Widget 的显示缓存，不修改牌堆。"))
	void SetCount(int32 InCount);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Common UI|Pile Count", meta = (ToolTip = "设置当前计数显示文本。用于弃牌堆等需要展示复合数量的场景；不会修改缓存的纯数字数量。"))
	void SetCountDisplayText(FText InText);

	UFUNCTION(BlueprintPure, Category = "Wacom|Common UI|Pile Count", meta = (ToolTip = "返回当前缓存的显示数量。它是 UI 显示值，不直接读取牌堆规则状态。"))
	int32 GetCount() const { return Count; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Common UI|Pile Count", meta = (ToolTip = "返回当前计数显示文本。可能是纯数字，也可能是类似 2+3 的复合展示。"))
	FText GetCountDisplayText() const { return CountDisplayText.IsEmpty() ? FText::AsNumber(Count) : CountDisplayText; }

	UFUNCTION(BlueprintCallable, Category = "Wacom|Common UI|Pile Count|Receive Feedback", meta = (ToolTip = "播放一次牌堆接收反馈。ReceivedCount 是本帧新抵达的数量；最后一枚可启用更强脉冲；Reduced Motion 只更新数量，不修改 Transform。"))
	void PlayReceiveFeedback(int32 ReceivedCount = 1, bool bFinalArrival = false, bool bReducedMotion = false);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Common UI|Pile Count|Receive Feedback", meta = (ToolTip = "立即停止牌堆接收反馈并精确恢复播放前的 RenderTransform 与 Pivot。不会改变当前显示数量。"))
	void ResetReceiveFeedback();

	UFUNCTION(BlueprintCallable, Category = "Wacom|Common UI|Pile Count|Send Feedback", meta = (ToolTip = "播放一次牌堆发牌反馈。SentCount 是本帧实际启动的卡牌数量；LaunchDirection 使用逻辑视口方向；Reduced Motion 只保留逐张计数。"))
	void PlaySendFeedback(
		int32 SentCount,
		bool bFinalDeparture,
		FVector2D LaunchDirection,
		bool bReducedMotion = false);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Common UI|Pile Count|Send Feedback", meta = (ToolTip = "立即停止牌堆发牌反馈。若接收反馈仍在播放，会精确恢复并继续合成剩余接收反馈；不会改变当前显示数量。"))
	void ResetSendFeedback();

	/** Enables the generic details affordance. The owner maps this request to its own pile semantics. */
	void SetDetailsInteractionEnabled(bool bEnabled);
	bool IsDetailsInteractionEnabled() const { return bDetailsInteractionEnabled; }
	FWacomPileDetailsRequestedNative& OnPileDetailsRequestedNative() { return PileDetailsRequestedNative; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativePreConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnKeyDown(
		const FGeometry& InGeometry,
		const FKeyEvent& InKeyEvent) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CountText;

	UPROPERTY(meta = (BindWidgetOptional, ToolTip = "牌堆接收反馈优先修改的可选视觉根节点。推荐包住牌堆图标与 CountText，并使用中心 Render Pivot；缺失时回退到整个 PileCountView。"))
	TObjectPtr<UWidget> ReceiveFeedbackRoot;

	UPROPERTY(meta = (BindWidgetOptional, ToolTip = "牌堆接收与发出反馈共同使用的可选视觉根节点。推荐包住牌堆图标与 CountText，并使用中心 Render Pivot；缺失时兼容回退到 ReceiveFeedbackRoot，再回退到整个 PileCountView。"))
	TObjectPtr<UWidget> PileFeedbackRoot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Common UI|Pile Count|Receive Feedback", meta = (ShowOnlyInnerProperties))
	FWacomPileReceiveFeedbackStyle ReceiveFeedbackStyle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Common UI|Pile Count|Send Feedback", meta = (ShowOnlyInnerProperties))
	FWacomPileSendFeedbackStyle SendFeedbackStyle;

private:
	friend struct FWacomPileCountViewTestAccess;

	FText CountDisplayText;
	int32 Count = 0;
	FWacomPileFeedbackPlayback* PileFeedbackPlayback = nullptr;
	FWacomPileDetailsRequestedNative PileDetailsRequestedNative;
	bool bDetailsInteractionEnabled = false;

	void RefreshDisplay();
	void EnsurePileFeedbackPlayback();
	void EvaluateAndApplyPileFeedback(float DeltaTime, bool bAdvanceTime);
	void RestorePileFeedbackAuthoredState();
};
