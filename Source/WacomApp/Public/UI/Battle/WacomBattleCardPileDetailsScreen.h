// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/TileView.h"
#include "UI/Battle/BattleCardPileEntryWidget.h"
#include "UI/Battle/WacomBattleCardPileDetailsTypes.h"
#include "UI/Battle/WacomBattleSecondaryPanelScreenBase.h"
#include "WacomBattleCardPileDetailsScreen.generated.h"

class UButton;
class UHorizontalBox;
class UImage;
class UMaterialInterface;
class USizeBox;
class UTextBlock;
class UWacomBattleCardPileDetailsStyle;
class UWacomCardDetailPanel;
class UWacomCardView;
class UWacomSettingsSubsystem;
enum class EWacomRuntimeSettingsChangeReason : uint8;
struct FWacomLocalSettingsSnapshot;
struct FWacomBattleCardPileDetailsTestAccess;

/** Transient list item retained by the screen for UTileView virtualization. */
UCLASS(Transient)
class WACOMAPP_API UWacomBattleCardPileItemViewModel : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient)
	FWacomBattlePileCardEntryView View;

	TObjectPtr<UMaterialInterface> SelectionOutlineMaterial;
	float HoverOutlineAmount = 0.72f;
	float LockedOutlineAmount = 1.0f;
	float SelectionOutlineExtentPixels = 4.0f;
	bool bReducedMotion = false;
	TSubclassOf<UWacomCardView> CardViewClass;
	FVector2D CardSize = FVector2D(296.0f, 420.0f);
	FVector2D EntrySize = FVector2D(320.0f, 448.0f);
	float EntryPaddingPixels = 4.0f;
};

/** Non-reflected state used by focused automation specs. */
struct FWacomBattleCardPileDetailsAutomationView
{
	EWacomBattlePileDetailsTab ActiveTab = EWacomBattlePileDetailsTab::Draw;
	EWacomBattlePileDiscardSection ActiveDiscardSection = EWacomBattlePileDiscardSection::Discard;
	bool bEmpty = true;
	bool bDetailVisible = false;
	FGuid DetailInstanceId;
	FGuid LockedInstanceId;
	TArray<FGuid> VisibleInstanceIds;
	TArray<int32> VisibleRuntimeCosts;
};

/** TileView with a narrow C++ authoring bridge for the fallback tree and commandlet. */
UCLASS()
class WACOMAPP_API UWacomBattleCardPileTileView : public UTileView
{
	GENERATED_BODY()

public:
	void SetRuntimeEntryWidgetClass(TSubclassOf<UUserWidget> InClass)
	{
		EntryWidgetClass = InClass;
	}
};

/** Battle pile details secondary panel. It only consumes a copied inspection snapshot. */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomBattleCardPileDetailsScreen : public UWacomBattleSecondaryPanelScreenBase
{
	GENERATED_BODY()

public:
	void SetPileDetailsContext(
		const FBattlePileInspectionSnapshot& InSnapshot,
		EWacomBattlePileDetailsTab InInitialTab);

	void SetAuthoringDefaults(
		UWacomBattleCardPileDetailsStyle* InStyle,
		TSubclassOf<UBattleCardPileEntryWidget> InEntryClass);

	EWacomBattlePileDetailsTab GetActiveTab() const { return ActiveTab; }
	EWacomBattlePileDiscardSection GetActiveDiscardSection() const { return ActiveDiscardSection; }
	int32 GetVisibleItemCount() const { return ItemViewModels.Num(); }
	UWacomBattleCardPileDetailsStyle* GetPileDetailsStyle() const { return PileDetailsStyle; }
	UClass* GetEntryWidgetClass() const { return EntryWidgetClass.Get(); }
	FWacomBattleCardPileDetailsAutomationView GetAutomationTestView() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<USizeBox> PanelSizeBox;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<USizeBox> NavigationRail;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> DrawTabButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> DiscardTabButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> ExhaustTabButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> DrawTabIcon;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> DiscardTabIcon;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> ExhaustTabIcon;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UHorizontalBox> DiscardSectionRoot;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> DiscardSectionButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> PlayedSectionButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWacomBattleCardPileTileView> VirtualizedCardTileView;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<USizeBox> CardGridSizeBox;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> EmptyText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<USizeBox> DetailPanelHost;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Battle|Pile Details",
		meta = (ToolTip = "牌堆详情页面的布局与颜色样式。"))
	TObjectPtr<UWacomBattleCardPileDetailsStyle> PileDetailsStyle;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Battle|Pile Details",
		meta = (ToolTip = "虚拟化 TileView 创建的卡牌条目类。"))
	TSubclassOf<UBattleCardPileEntryWidget> EntryWidgetClass;

private:
	friend struct FWacomBattleCardPileDetailsTestAccess;

	UFUNCTION() void HandleDrawTabClicked();
	UFUNCTION() void HandleDiscardTabClicked();
	UFUNCTION() void HandleExhaustTabClicked();
	UFUNCTION() void HandleDiscardSectionClicked();
	UFUNCTION() void HandlePlayedSectionClicked();

	void ResolveRuntimeBindings();
	void BindControls();
	void UnbindControls();
	void RebuildItems();
	void SetActiveTab(EWacomBattlePileDetailsTab NewTab);
	void SetActiveDiscardSection(EWacomBattlePileDiscardSection NewSection);
	const FBattlePileInspectionSectionSnapshot* ResolveActiveSection() const;
	void UpdateCountLabels();
	void UpdateNavigationVisuals();
	void HandleItemClicked(UObject* Item);
	void HandleItemHoveredChanged(UObject* Item, bool bIsHovered);
	void HandleListViewScrolled(float ItemOffset, float DistanceRemaining);
	void HandleEntryWidgetGenerated(UUserWidget& EntryWidget);
	void HandleEntryWidgetReleased(UUserWidget& EntryWidget);
	void HandleEntryHoverChanged(UBattleCardPileEntryWidget& EntryWidget, bool bIsHovered);
	void HandleEntryFocusChanged(UBattleCardPileEntryWidget& EntryWidget, bool bIsFocused);
	void ApplyEntrySelectionStates();
	void SetDetailCandidate(UWacomBattleCardPileItemViewModel* Item);
	void ClearDetailCandidate();
	void EnsureDetailPanel();
	void ShowDetailPanel(UWacomBattleCardPileItemViewModel& Item);
	void HideDetailPanel(bool bImmediate);
	void UpdateDetailPanelPosition();
	void BindRuntimeSettings();
	void UnbindRuntimeSettings();
	void HandleRuntimeSettingsChanged(
		const FWacomLocalSettingsSnapshot& Snapshot,
		EWacomRuntimeSettingsChangeReason Reason);
	void ClearItems();
	void ApplyFullscreenLayout();

	UPROPERTY(Transient) FBattlePileInspectionSnapshot InspectionSnapshot;
	UPROPERTY(Transient) TArray<TObjectPtr<UWacomBattleCardPileItemViewModel>> ItemViewModels;
	UPROPERTY(Transient) TObjectPtr<UWacomBattleCardPileItemViewModel> LockedItem;
	UPROPERTY(Transient) TObjectPtr<UWacomBattleCardPileItemViewModel> HoveredItem;
	UPROPERTY(Transient) TObjectPtr<UWacomBattleCardPileItemViewModel> FocusedItem;
	UPROPERTY(Transient) TObjectPtr<UWacomBattleCardPileItemViewModel> DetailCandidateItem;
	UPROPERTY(Transient) TObjectPtr<UWacomBattleCardPileItemViewModel> DetailVisibleItem;
	UPROPERTY(Transient) TObjectPtr<UWacomCardDetailPanel> RuntimeDetailPanel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DiscardSectionLabel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PlayedSectionLabel;
	TWeakObjectPtr<UWacomSettingsSubsystem> BoundSettingsSubsystem;
	FDelegateHandle RuntimeSettingsChangedHandle;
	float DetailCandidateElapsedSeconds = 0.0f;
	float DetailOpacity = 0.0f;
	bool bDetailWantsVisible = false;
	bool bRuntimeSimplifiedMotion = false;

	EWacomBattlePileDetailsTab ActiveTab = EWacomBattlePileDetailsTab::Draw;
	EWacomBattlePileDiscardSection ActiveDiscardSection = EWacomBattlePileDiscardSection::Discard;
};
