// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RunStateTypes.h"
#include "WacomBackpackPilePreviewWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;
class UCardDefinition;

/** 折叠牌堆中的固定尺寸简化卡面；不复用完整 Retainer 卡面，也不拥有输入。 */
struct WACOMAPP_API FWacomBackpackPilePreviewCardView
{
	TWeakObjectPtr<UCardDefinition> Definition;
	bool bOwnerIdentity = false;
	bool bProjected = false;
};

/** 折叠牌堆的只读 Scene ViewData。 */
struct WACOMAPP_API FWacomBackpackZonePileView
{
	EZoneKind Zone = EZoneKind::BattleDeck;
	FGuid OwnerInstanceId;
	FText Title;
	int32 CardCount = 0;
	int32 Capacity = 0;
	int32 ProjectedCount = 0;
	bool bHasCapacity = false;
	bool bMovable = true;
	bool bWarning = false;
	bool bExpanded = false;
	TArray<FWacomBackpackPilePreviewCardView> PreviewCards;

	bool HasSameIdentity(EZoneKind OtherZone, FGuid OtherOwnerInstanceId) const
	{
		const FGuid NormalizedOwner = OtherZone == EZoneKind::SpecialZone
			? OtherOwnerInstanceId
			: FGuid();
		return Zone == OtherZone && OwnerInstanceId == NormalizedOwner;
	}
};

/** 固定 110×160 设计尺寸的简化缩略卡，只显示插画、费用、名称和身份角标。 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomBackpackPilePreviewWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetPreviewView(const FWacomBackpackPilePreviewCardView& InView);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CardArt;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CostText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RoleBadgeText;

private:
	FWacomBackpackPilePreviewCardView PreviewView;
	void EnsureFallbackTree();
	void ApplyView();
};

