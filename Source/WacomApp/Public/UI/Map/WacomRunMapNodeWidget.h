// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/WacomMenuButtonWidget.h"
#include "UI/Map/WacomRunMapScreenTypes.h"
#include "WacomRunMapNodeWidget.generated.h"

class UBorder;
class UCommonTextBlock;

/** 被动地图节点按钮；只显示 ViewData 并广播自身稳定 Handle 的选择/确认意图。 */
UCLASS(Blueprintable, meta = (ToolTip = "Run 地图节点按钮。只消费 Screen 提供的只读 ViewData；Blueprint 只能制作视觉，不能决定传送合法性。"))
class WACOMAPP_API UWacomRunMapNodeWidget : public UWacomMenuButtonWidget
{
	GENERATED_BODY()

public:
	UWacomRunMapNodeWidget(const FObjectInitializer& ObjectInitializer);

	void ApplyViewData(const FWacomRunMapNodeViewData& InViewData);
	const FWacomRunMapNodeViewData& GetViewData() const { return ViewData; }

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnNodeIntentNative, const FWacomMapNodeHandle&);
	FOnNodeIntentNative OnNodeSelectedNative;
	FOnNodeIntentNative OnNodeConfirmRequestedNative;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnMouseButtonDoubleClick(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|Run Map", meta = (DisplayName = "On Run Map Node View Data Applied"))
	void BP_OnViewDataApplied(const FWacomRunMapNodeViewData& InViewData);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> NodeSemanticMarker;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> NodeTypeText;

private:
	void HandleClicked();

	UPROPERTY(Transient)
	FWacomRunMapNodeViewData ViewData;

	FDelegateHandle ClickedHandle;
};
