// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WacomSettingsOptionRow.generated.h"

class UCommonTextBlock;
class USlider;
class UWacomMenuButtonWidget;

enum class EWacomSettingsOptionKind : uint8
{
	Discrete,
	Continuous
};

struct WACOMAPP_API FWacomSettingsOptionRowViewData
{
	FText Label;
	FText Value;
	EWacomSettingsOptionKind Kind = EWacomSettingsOptionKind::Discrete;
	float NormalizedValue = 0.0f;
	bool bEnabled = true;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FWacomSettingsOptionStepRequested, int32);
DECLARE_MULTICAST_DELEGATE_OneParam(FWacomSettingsOptionNormalizedValueRequested, float);

/** Passive, reusable settings row. It emits UI intent and never edits settings itself. */
UCLASS(Blueprintable, meta = (ToolTip = "设置页面可复用选项行。负责标签、值、左右步进和可选 Slider；只上报 UI 意图，不读取或保存设置。"))
class WACOMAPP_API UWacomSettingsOptionRow : public UUserWidget
{
	GENERATED_BODY()

public:
	UWacomSettingsOptionRow(const FObjectInitializer& ObjectInitializer);

	void ApplyViewData(const FWacomSettingsOptionRowViewData& InViewData);
	const FWacomSettingsOptionRowViewData& GetViewData() const { return ViewData; }

	FWacomSettingsOptionStepRequested OnStepRequestedNative;
	FWacomSettingsOptionNormalizedValueRequested OnNormalizedValueRequestedNative;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> LabelText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> ValueText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomMenuButtonWidget> DecreaseButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomMenuButtonWidget> IncreaseButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> ValueSlider;

private:
	void HandleDecreaseClicked();
	void HandleIncreaseClicked();

	UFUNCTION()
	void HandleSliderValueChanged(float Value);

	FWacomSettingsOptionRowViewData ViewData;
	bool bApplyingViewData = false;
};
