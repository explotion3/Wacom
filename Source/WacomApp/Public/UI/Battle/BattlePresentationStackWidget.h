// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleWidgetBase.h"
#include "UI/Battle/BattlePresentationStackEntryWidget.h"
#include "BattlePresentationStackWidget.generated.h"

class UCanvasPanel;
class UWacomCardView;

/**
 * Read-only stack of recently submitted cards whose presentation is still catching up.
 *
 * Entry array order is oldest -> newest. Visual z-order keeps the oldest card on top,
 * because it is the next card whose presentation will complete.
 */
UCLASS(Blueprintable)
class WACOMAPP_API UBattlePresentationStackWidget : public UWacomBattleWidgetBase
{
	GENERATED_BODY()

public:
	UBattlePresentationStackWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI|Presentation Stack", meta = (ClampMin = "1", UIMin = "1", UIMax = "10", ToolTip = "战斗表现栈最多显示几张小卡。内部队列仍会保留全部待播放卡牌。"))
	int32 MaxVisibleEntries = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI|Presentation Stack", meta = (ToolTip = "表现栈小卡使用的只读卡面 Widget。为空时优先加载 WBP_FirstPersonCardView，再回退 WBP_CardView，最后回退 UWacomCardView。"))
	TSubclassOf<UWacomCardView> MiniCardViewClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI|Presentation Stack", meta = (ClampMin = "1.0", UIMin = "48.0", UIMax = "240.0", ToolTip = "表现栈小卡宽高，单位为 Slate 像素。"))
	FVector2D MiniCardSize = FVector2D(104.0f, 150.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI|Presentation Stack", meta = (UIMin = "-40.0", UIMax = "40.0", ToolTip = "表现栈每张小卡之间的错位距离，单位为 Slate 像素。"))
	FVector2D EntryOffset = FVector2D(10.0f, 12.0f);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Presentation Stack")
	void SetPresentationStackEntries(const TArray<FWacomBattlePresentationStackEntryView>& InEntries);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Presentation Stack")
	void ClearPresentationStack();

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Presentation Stack")
	const TArray<FWacomBattlePresentationStackEntryView>& GetCurrentEntries() const { return CurrentEntries; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Presentation Stack")
	int32 GetVisibleEntryCount() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> StackCanvas;

private:
	UPROPERTY(Transient)
	TArray<FWacomBattlePresentationStackEntryView> CurrentEntries;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBattlePresentationStackEntryWidget>> EntryWidgets;

	void RebuildEntryWidgets();
	TSubclassOf<UWacomCardView> ResolveMiniCardViewClass() const;
};
