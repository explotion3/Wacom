// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/WacomMenuWidgetBase.h"
#include "UI/Map/WacomRunMapScreenTypes.h"
#include "WacomRunMapScreen.generated.h"

class UCanvasPanel;
class UCommonTextBlock;
class UScaleBox;
class UWacomMenuButtonWidget;
class UWacomRunMapEdgeLayerWidget;
class UWacomRunMapNodeWidget;
struct FWacomRunMapScreenTestAccess;

/** 当前 Floor 的被动 CommonUI 地图 Screen。 */
UCLASS(Blueprintable, meta = (ToolTip = "当前 Floor 的 Run 地图 Screen。只接收完整 ViewData、维护瞬时选择并广播玩家意图；不读取 Session 或场景 Actor。"))
class WACOMAPP_API UWacomRunMapScreen : public UWacomMenuWidgetBase
{
	GENERATED_BODY()

public:
	UWacomRunMapScreen(const FObjectInitializer& ObjectInitializer);

	void ApplyViewData(const FWacomRunMapScreenViewData& InViewData);
	const FWacomRunMapScreenViewData& GetViewData() const { return ViewData; }

	bool RequestSelectNode(const FWacomMapNodeHandle& Node);
	bool RequestConfirmTravel();
	void RequestClose();

	DECLARE_MULTICAST_DELEGATE_OneParam(
		FOnRunMapActionNative,
		const FWacomRunMapScreenActionRequest&);
	FOnRunMapActionNative OnRunMapActionNative;

	DECLARE_MULTICAST_DELEGATE(FOnRunMapDeactivatedNative);
	FOnRunMapDeactivatedNative OnRunMapDeactivatedNative;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeHandleBackRequested() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScaleBox> MapViewportScaleBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> MapCanvas;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomRunMapEdgeLayerWidget> EdgeLayer;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> FloorTitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> SelectedNodeTitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> SelectedNodeDescriptionText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> StatusText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomMenuButtonWidget> TravelButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomMenuButtonWidget> CloseButton;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Run Map|Widget Classes",
		meta = (ToolTip = "地图节点 Widget 类。为空时使用 UWacomRunMapNodeWidget C++ fallback；只影响表现和绑定，不影响地图规则。"))
	TSubclassOf<UWacomRunMapNodeWidget> NodeWidgetClass;

	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|Run Map", meta = (DisplayName = "On Run Map View Data Applied"))
	void BP_OnViewDataApplied(const FWacomRunMapScreenViewData& InViewData);

private:
	void RebuildNodeWidgets();
	void RefreshSelectionPresentation();
	void HandleTravelClicked();
	void HandleCloseClicked();
	void HandleNodeSelected(const FWacomMapNodeHandle& Node);
	void HandleNodeConfirmRequested(const FWacomMapNodeHandle& Node);
	UWacomRunMapNodeWidget* FindNodeWidget(const FWacomMapNodeHandle& Node) const;
	const FWacomRunMapNodeViewData* FindNodeViewData(const FWacomMapNodeHandle& Node) const;

	UPROPERTY(Transient)
	FWacomRunMapScreenViewData ViewData;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWacomRunMapNodeWidget>> NodeWidgets;

	FDelegateHandle TravelClickedHandle;
	FDelegateHandle CloseClickedHandle;

#if WITH_AUTOMATION_TESTS
	friend struct FWacomRunMapScreenTestAccess;
#endif
};
