// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "WacomProgressBar.generated.h"

class UProgressBar;
class UTextBlock;

/**
 * 通用进度条 Widget。
 *
 * 两种使用方式：
 * 1. 直接用 C++ 类 UWacomProgressBar 实例化，获得内置硬编码外观（绿色填充条 + 居中 "Current/Max" 文字）
 * 2. 创建 WBP_WacomProgressBar（或子类）在 Designer 里自定义，C++ 的 RebuildWidget 在检测到 WBP 提供了
 *    WidgetTree 时自动让步
 *
 * WBP 约定：
 * - Fill      : UProgressBar (BindWidget)
 * - ValueText : UTextBlock   (BindWidgetOptional)
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomProgressBar : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Wacom|UI")
	void SetValue(int32 InCurrent, int32 InMax);

	UFUNCTION(BlueprintCallable, Category = "Wacom|UI")
	void SetFillColor(FLinearColor Color);

	UFUNCTION(BlueprintCallable, Category = "Wacom|UI")
	void SetShowText(bool bShow);

	UFUNCTION(BlueprintCallable, Category = "Wacom|UI")
	void SetTextFormat(FText InFormat);

	UFUNCTION(BlueprintPure, Category = "Wacom|UI")
	int32 GetCurrent() const { return Current; }

	UFUNCTION(BlueprintPure, Category = "Wacom|UI")
	int32 GetMax() const { return MaxValue; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativePreConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> Fill;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ValueText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|UI")
	FText TextFormat = FText::FromString(TEXT("{0}/{1}"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|UI")
	bool bShowText = true;

private:
	UPROPERTY(Transient)
	int32 Current = 0;

	UPROPERTY(Transient)
	int32 MaxValue = 0;

	void RefreshDisplay();
};
