// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "BattleCardPileEntryWidget.generated.h"

class UImage;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UScaleBox;
class USizeBox;
class UWacomCardView;
class UWacomBattleCardPileItemViewModel;
class UBattleCardPileEntryWidget;
struct FWacomBattleCardPileDetailsTestAccess;

DECLARE_MULTICAST_DELEGATE_TwoParams(
	FWacomBattlePileEntryFocusChangedNative,
	UBattleCardPileEntryWidget&,
	bool);
DECLARE_MULTICAST_DELEGATE_TwoParams(
	FWacomBattlePileEntryHoverChangedNative,
	UBattleCardPileEntryWidget&,
	bool);

/** Virtualized, display-only pile browser entry. */
UCLASS(Blueprintable)
class WACOMAPP_API UBattleCardPileEntryWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	void SetSelectionPresentation(
		UMaterialInterface* InOutlineMaterial,
		float InHoverAmount,
		float InLockedAmount,
		float InOutlineExtentPixels,
		bool bInReducedMotion);
	void SetLockedSelected(bool bInLockedSelected);
	void SetOwnerReportedPointerHovered(bool bInPointerHovered);
	void RefreshResolvedLayout();
	UWacomBattleCardPileItemViewModel* GetItemViewModel() const { return ItemViewModel.Get(); }
	FWacomBattlePileEntryFocusChangedNative& OnFocusChangedNative() { return FocusChangedNative; }
	FWacomBattlePileEntryHoverChangedNative& OnHoverChangedNative() { return HoverChangedNative; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	virtual void NativeOnItemSelectionChanged(bool bIsSelected) override;
	virtual void NativeOnEntryReleased() override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> EntrySizeBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> SelectionOutlineImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> CardHost;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScaleBox> CardScaleBox;

	UPROPERTY(Transient)
	TObjectPtr<UWacomCardView> RuntimeCardView;

private:
	friend struct FWacomBattleCardPileDetailsTestAccess;

	void EnsureCardView(TSubclassOf<UWacomCardView> RequestedClass);
	void ApplySelectionState();
	void EnsureSelectionMID();
	void ReleaseSelectionMID();

	TWeakObjectPtr<UWacomBattleCardPileItemViewModel> ItemViewModel;
	TObjectPtr<UMaterialInterface> SelectionOutlineMaterial;
	TObjectPtr<UMaterialInstanceDynamic> SelectionOutlineMID;
	float HoverOutlineAmount = 0.72f;
	float LockedOutlineAmount = 1.0f;
	float SelectionOutlineExtentPixels = 4.0f;
	bool bPointerHovered = false;
	bool bKeyboardFocused = false;
	bool bLockedSelected = false;
	bool bReducedMotion = false;
	FWacomBattlePileEntryFocusChangedNative FocusChangedNative;
	FWacomBattlePileEntryHoverChangedNative HoverChangedNative;
};
