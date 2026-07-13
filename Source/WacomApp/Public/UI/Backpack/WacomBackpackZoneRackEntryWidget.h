// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RunStateTypes.h"
#include "WacomBackpackZoneRackEntryWidget.generated.h"

class UBorder;
class UButton;
class UTextBlock;

/** 单个区域牌匣入口的只读表现数据。 */
struct WACOMAPP_API FWacomBackpackZoneRackEntryView
{
	EZoneKind Zone = EZoneKind::Backpack;
	FGuid OwnerInstanceId;
	FText Title;
	int32 CardCount = 0;
	int32 Capacity = 0;
	bool bHasCapacity = false;
	bool bActive = false;

	bool HasSameIdentity(EZoneKind OtherZone, FGuid OtherOwnerInstanceId) const
	{
		const FGuid NormalizedOwner = OtherZone == EZoneKind::SpecialZone
			? OtherOwnerInstanceId
			: FGuid();
		return Zone == OtherZone && OwnerInstanceId == NormalizedOwner;
	}
};

/** 被动区域牌匣入口；只显示 View 并转发激活意图。 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomBackpackZoneRackEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnZoneActivatedNative, EZoneKind, FGuid);
	FOnZoneActivatedNative OnZoneActivatedNative;

	void SetEntryView(const FWacomBackpackZoneRackEntryView& InView);
	/** 被动目标预览；校验结果由 Screen flow 提供，Entry 不读取 RunSession。 */
	void SetDropPreviewState(bool bVisible, bool bRejected);
	const FWacomBackpackZoneRackEntryView& GetEntryView() const { return EntryView; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ActivateButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CountText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> ActiveBorder;

private:
	FWacomBackpackZoneRackEntryView EntryView;
	bool bDropPreviewVisible = false;
	bool bDropPreviewRejected = false;

	UFUNCTION()
	void HandleActivated();

	void ApplyView();
};
