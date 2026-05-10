// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "PileCountView.generated.h"

class UTextBlock;
class UBorder;

/**
 * 通用"带标签的计数"小 Widget。用于抽牌堆/弃牌堆/消耗区等显示。
 *
 * C++ 默认外观：方块 + 顶部 Label + 下方大数字。
 *
 * WBP 约定（都可选）：
 * - LabelText : UTextBlock
 * - CountText : UTextBlock
 * - FrameBorder : UBorder
 */
UCLASS(Blueprintable)
class WACOMAPP_API UPileCountView : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Wacom|UI")
	void SetLabel(FText InLabel);

	UFUNCTION(BlueprintCallable, Category = "Wacom|UI")
	void SetCount(int32 InCount);

	UFUNCTION(BlueprintCallable, Category = "Wacom|UI")
	void SetFrameColor(FLinearColor Color);

	UFUNCTION(BlueprintPure, Category = "Wacom|UI")
	int32 GetCount() const { return Count; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativePreConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LabelText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CountText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> FrameBorder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI")
	FText DefaultLabel = FText::FromString(TEXT("Pile"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI")
	FLinearColor DefaultFrameColor = FLinearColor(0.15f, 0.15f, 0.2f, 0.9f);

private:
	FText CachedLabel;
	int32 Count = 0;

	void RefreshDisplay();
};
