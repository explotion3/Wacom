// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleWidgetBase.h"
#include "HandPanel.generated.h"

class UCardWidget;
class UPanelWidget;

/**
 * 手牌容器。5 段 Slot：左手区/左手锚点/双手区/右手锚点/右手区。
 *
 * C++ 内置默认外观：水平铺开 5 个 HorizontalBox 作为 Slot，支持未配 WBP 时的快速预览。
 *
 * WBP 约定（BindWidget）：
 * - LeftZoneSlot / LeftAnchorSlot / BothZoneSlot / RightAnchorSlot / RightZoneSlot : UPanelWidget
 */
UCLASS(Blueprintable)
class WACOMAPP_API UHandPanel : public UWacomBattleWidgetBase
{
	GENERATED_BODY()

public:
	UHandPanel();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI")
	TSubclassOf<UCardWidget> CardWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI")
	TSubclassOf<UCardWidget> AnchorCardWidgetClass;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeRefreshFromSnapshot(const struct FBattleSnapshot& Snap) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> LeftZoneSlot;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> LeftAnchorSlot;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> BothZoneSlot;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> RightAnchorSlot;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> RightZoneSlot;

private:
	UFUNCTION()
	void HandleCardClicked(FGuid CardInstanceId);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCardWidget>> SpawnedCards;

	UCardWidget* CreateAndPlaceCard(const struct FHandCardSnapshot& CardSnap, UPanelWidget* TargetSlot);
	void ClearAllSlots();
	void ApplyTargetingHighlight();
};
