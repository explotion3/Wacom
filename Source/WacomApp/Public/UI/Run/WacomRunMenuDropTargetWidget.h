// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "WacomRunMenuDropTargetWidget.generated.h"

class UBorder;
class UWidget;

UENUM(BlueprintType)
enum class EWacomRunMenuDropTargetPreviewState : uint8
{
	Normal UMETA(DisplayName = "Normal"),
	Probe UMETA(DisplayName = "Probe"),
	Invalid UMETA(DisplayName = "Invalid"),
	ReleasedProbe UMETA(DisplayName = "Released Probe"),
	SubmitReady UMETA(DisplayName = "Submit Ready"),
	Submitted UMETA(DisplayName = "Submitted")
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomRunMenuDropTargetDebugView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Menu Drop Target|Debug", meta = (ToolTip = "菜单 drop target 的 ZoneId，只用于 PIE / 蓝图诊断。"))
	FName ZoneId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Menu Drop Target|Debug", meta = (ToolTip = "菜单 drop target 的稳定目标 ID。"))
	FName StableTargetId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Menu Drop Target|Debug", meta = (ToolTip = "当前是否允许 first-person 菜单卡牌 probe 命中。"))
	bool bProbeEnabled = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Menu Drop Target|Debug", meta = (ToolTip = "当前 Widget 是否可见且可交互。"))
	bool bVisibleAndEnabled = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Menu Drop Target|Debug", meta = (ToolTip = "当前菜单 drop target 的预览状态。"))
	EWacomRunMenuDropTargetPreviewState PreviewState =
		EWacomRunMenuDropTargetPreviewState::Normal;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Menu Drop Target|Debug", meta = (ToolTip = "当前是否存在 C++ fallback 预览反馈。"))
	bool bPreviewActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Menu Drop Target|Debug", meta = (ToolTip = "最近一次 probe 使用的 Widget 坐标。"))
	mutable FVector2D LastProbeWidgetPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Menu Drop Target|Debug", meta = (ToolTip = "最近一次 fallback 预览应用的缩放。"))
	FVector2D LastPreviewScale = FVector2D(1.0f, 1.0f);
};

/**
 * Menu-layer first-person card drop probe target.
 *
 * The widget only exposes a Zone interaction handle and lightweight preview.
 * It never mutates RunSession and does not participate in the backpack UMG
 * DragDrop operation path.
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomRunMenuDropTargetWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWacomRunMenuDropTargetWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Menu Drop Target", meta = (ToolTip = "菜单拖拽区域的 Zone Id。第一人称卡牌拖到这里时会生成 TargetKind=Zone 的交互目标。"))
	FName ZoneId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Menu Drop Target", meta = (ToolTip = "美术或关卡可读的稳定目标 ID；用于 debug 和未来规则层识别。为空时默认使用 ZoneId。"))
	FName StableTargetId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Menu Drop Target", meta = (ToolTip = "是否允许该菜单区域被 Run first-person menu lease 卡牌拖拽 probe 命中。关闭后不会返回 Zone target。"))
	bool bEnableRunMenuDropProbe = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Menu Drop Target|Preview", meta = (ToolTip = "是否使用 C++ fallback 轻量预览。WBP 可同时监听 BP_OnRunMenuDropPreviewStateChanged 做自定义表现。"))
	bool bEnableFallbackPreview = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Menu Drop Target|Preview", meta = (ClampMin = "0.01", UIMin = "0.9", UIMax = "1.2", ToolTip = "被 first-person 卡牌拖拽 probe 命中时的轻量缩放倍率。"))
	float ProbePreviewScale = 1.025f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Menu Drop Target|Preview", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "0.5", ToolTip = "C++ fallback 预览颜色叠加的不透明度，范围 0 到 1。"))
	float PreviewOpacity = 0.16f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Menu Drop Target|Preview", meta = (ToolTip = "拖拽指向该菜单 Zone target 时使用的探测颜色。"))
	FLinearColor ProbePreviewColor = FLinearColor(0.45f, 0.75f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Menu Drop Target|Preview", meta = (ToolTip = "拖拽指向不可用菜单 Zone target 时使用的拒绝颜色。"))
	FLinearColor InvalidPreviewColor = FLinearColor(1.0f, 0.12f, 0.08f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Menu Drop Target|Preview", meta = (ToolTip = "拖拽释放在该菜单 Zone target 上时使用的短暂确认颜色。也用于可提交和已提交状态。"))
	FLinearColor ReleasedProbePreviewColor = FLinearColor(0.75f, 1.0f, 0.55f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Menu Drop Target|Debug", meta = (ToolTip = "开启后，菜单 Zone drop target 的 preview 和 debug summary 会输出简短日志。默认关闭。"))
	bool bLogRunMenuDropTargetDebug = false;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Menu Drop Target")
	void SetDropContent(UWidget* InContent);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Menu Drop Target")
	FWacomInteractionTargetHandle BuildZoneTargetHandle(FVector2D ScreenPosition) const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Menu Drop Target")
	bool CanProbeRunMenuDropTarget() const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Menu Drop Target")
	virtual bool ContainsWidgetPosition(FVector2D WidgetPosition) const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Menu Drop Target")
	void SetRunMenuDropPreviewState(EWacomRunMenuDropTargetPreviewState NewState);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Menu Drop Target")
	void ClearRunMenuDropPreviewState();

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Menu Drop Target")
	EWacomRunMenuDropTargetPreviewState GetRunMenuDropPreviewState() const
	{
		return PreviewState;
	}

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Menu Drop Target|Debug", meta = (ToolTip = "获取菜单 drop target 的只读调试快照；用于排查 Zone target 和预览状态。"))
	FWacomRunMenuDropTargetDebugView GetRunMenuDropTargetDebugView() const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Menu Drop Target|Debug", meta = (ToolTip = "获取菜单 drop target 的单行调试摘要；不提交 Run 规则或拖拽结果。"))
	FString GetRunMenuDropTargetDebugSummary() const;

	UFUNCTION(CallInEditor, Category = "Wacom|Run|Menu Drop Target|Debug", meta = (ToolTip = "在编辑器或 PIE 中把菜单 drop target 调试摘要写入 Output Log；不改变 Widget 状态。"))
	void LogRunMenuDropTargetDebugSummary() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|Run|Menu Drop Target")
	void BP_OnRunMenuDropPreviewStateChanged(EWacomRunMenuDropTargetPreviewState NewState);

private:
	UPROPERTY(Transient)
	TObjectPtr<UBorder> RootBorder;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> DropContent;

	EWacomRunMenuDropTargetPreviewState PreviewState =
		EWacomRunMenuDropTargetPreviewState::Normal;
	mutable FVector2D LastProbeWidgetPosition = FVector2D::ZeroVector;
	FVector2D OriginalRenderScale = FVector2D(1.0f, 1.0f);
	bool bHasCapturedOriginalRenderScale = false;

	void ApplyFallbackPreview();
	bool IsVisibleForProbe() const;
	FName ResolveStableTargetId() const;
};
