// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RunState.h"
#include "UI/Events/WacomRunEventPresentationBuilder.h"
#include "WacomRunEventChoiceButton.generated.h"

class UButton;
class UTextBlock;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnRunEventChoiceClickedNative, FName /*ChoiceId*/);

/** 最小事件选项按钮。WBP 可继承替换视觉，逻辑仍只广播 ChoiceId。 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomRunEventChoiceButton : public UUserWidget
{
	GENERATED_BODY()

public:
	FOnRunEventChoiceClickedNative OnChoiceClickedNative;

	UFUNCTION(BlueprintCallable, Category = "Wacom|RunEvent")
	void SetChoiceSnapshot(const FRunEventChoiceSnapshot& InChoice);

	UFUNCTION(BlueprintPure, Category = "Wacom|RunEvent")
	FRunEventChoiceSnapshot GetChoiceSnapshot() const { return ChoiceSnapshot; }

	UFUNCTION(BlueprintPure, Category = "Wacom|RunEvent")
	FWacomRunEventChoiceRequirementView GetChoiceRequirementView() const { return RequirementView; }

#if WITH_AUTOMATION_TESTS
	FText GetDisplayedPaymentStatusTextForTest() const;
	ESlateVisibility GetPaymentStatusVisibilityForTest() const;
	FText GetDisplayedDisabledReasonTextForTest() const;
	ESlateVisibility GetDisabledReasonVisibilityForTest() const;
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandleClicked();

	UFUNCTION(BlueprintNativeEvent, Category = "Wacom|RunEvent",
		meta = (ToolTip = "当 C++ 应用新的 RunEvent 选项快照并完成默认文本刷新后触发。WBP 可在这里刷新自定义视觉，不要直接提交 Run 规则。"))
	void BP_OnRunEventChoiceSnapshotApplied(const FRunEventChoiceSnapshot& AppliedChoiceSnapshot);
	virtual void BP_OnRunEventChoiceSnapshotApplied_Implementation(
		const FRunEventChoiceSnapshot& AppliedChoiceSnapshot);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ChoiceButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LabelText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PaymentStatusText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DisabledReasonText;

private:
	void RefreshVisuals();
	void NotifyChoiceSnapshotApplied();

	UPROPERTY(Transient)
	FRunEventChoiceSnapshot ChoiceSnapshot;

	UPROPERTY(Transient)
	FWacomRunEventChoiceRequirementView RequirementView;

	bool bHasAppliedChoiceSnapshot = false;
	bool bHasConstructed = false;
	bool bNeedsSnapshotAppliedNotifyAfterConstruct = false;
};
