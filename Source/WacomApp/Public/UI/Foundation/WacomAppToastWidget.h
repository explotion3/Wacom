// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/WacomActivatableWidget.h"
#include "UI/Foundation/WacomAppToastTypes.h"
#include "WacomAppToastWidget.generated.h"

class UTextBlock;
class UVerticalBox;
struct FUIInputConfig;

/**
 * App-level non-blocking toast widget for Run/exploration/menu feedback.
 *
 * It owns only display queue state. Callers pass FWacomAppToastView through
 * UWacomAppToastSubsystem; gameplay truth stays in Run/Battle modules.
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomAppToastWidget : public UWacomActivatableWidget
{
	GENERATED_BODY()

public:
	UWacomAppToastWidget();

	UFUNCTION(BlueprintCallable, Category = "Wacom|Toast")
	void EnqueueToast(const FWacomAppToastView& View);

	UFUNCTION(BlueprintPure, Category = "Wacom|Toast")
	int32 GetVisibleToastCount() const { return ActiveViews.Num(); }

	/** 每条 Toast 的默认显示时长，单位为秒。单条 View 的 LifetimeOverride > 0 时会覆盖它。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Toast", meta = (ToolTip = "每条 Toast 的默认显示时长，单位为秒。单条 Toast 的 LifetimeOverride 大于 0 时会覆盖该值；建议范围 0.5-10 秒。", ClampMin = "0.1", UIMin = "0.5", UIMax = "10.0"))
	float DefaultMessageLifetime = 3.0f;

	/** Toast 过期前淡出的持续时间，单位为秒。0 表示不淡出。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Toast", meta = (ToolTip = "Toast 过期前淡出的持续时间，单位为秒。0 表示不淡出；该值不会延长总显示时长。建议范围 0-3 秒。", ClampMin = "0.0", UIMin = "0.0", UIMax = "3.0"))
	float FadeDuration = 0.8f;

	/** 最多同时显示的 Toast 条数；超过时移除最旧消息。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Toast", meta = (ToolTip = "最多同时显示的 Toast 条数。超过该数量时会移除最旧消息，避免 Overlay 堆满；建议范围 1-10 条。", ClampMin = "1", ClampMax = "20", UIMin = "1", UIMax = "10"))
	int32 MaxVisibleMessages = 5;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& InGeometry, float InDeltaTime) override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> Container;

private:
#if WITH_AUTOMATION_TESTS
	friend class FWacomUITestAccess;
#endif

	UPROPERTY(Transient)
	TArray<FWacomAppToastView> ActiveViews;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> ActiveTexts;

	TArray<float> ActiveRemaining;

	void PushToast(const FWacomAppToastView& View);
	void RemoveAt(int32 Index);
	void TickToasts(float DeltaTime);
	void HandleQueueEmpty();
	int32 GetEffectiveMaxVisibleMessages() const;
};
