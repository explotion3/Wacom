// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Backpack/WacomBackpackZoneRackEntryWidget.h"
#include "WacomBackpackZoneRackWidget.generated.h"

class UVerticalBox;

/** 右侧常驻区域牌匣；按 Zone identity 稳定 reconcile 被动 Entry。 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomBackpackZoneRackWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnZoneActivatedNative, EZoneKind, FGuid);
	FOnZoneActivatedNative OnZoneActivatedNative;

	void SetZoneEntries(TConstArrayView<FWacomBackpackZoneRackEntryView> DesiredEntries);
	int32 GetZoneEntryCount() const { return EntryWidgets.Num(); }
	const FWacomBackpackZoneRackEntryView* GetZoneEntryView(int32 Index) const;
	bool FindZoneAtAbsolutePosition(FVector2D AbsolutePosition, EZoneKind& OutZone, FGuid& OutOwnerInstanceId) const;
	void SetDropPreviewForZone(EZoneKind Zone, FGuid OwnerInstanceId, bool bVisible, bool bRejected);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> EntriesHost;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Backpack|Zone Rack",
		meta = (ToolTip = "单个区域牌匣入口的 Widget 类。只负责区域标题、数量、激活和目标预览表现，不直接访问 RunSession。"))
	TSubclassOf<UWacomBackpackZoneRackEntryWidget> EntryWidgetClass;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UWacomBackpackZoneRackEntryWidget>> EntryWidgets;

	UVerticalBox* EnsureEntriesHost();
	void HandleEntryActivated(EZoneKind Zone, FGuid OwnerInstanceId);
};
