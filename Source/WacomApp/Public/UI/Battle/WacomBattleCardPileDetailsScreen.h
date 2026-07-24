// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/TileView.h"
#include "UI/Battle/BattleCardPileEntryWidget.h"
#include "UI/Battle/WacomBattleCardPileDetailsTypes.h"
#include "UI/Battle/WacomBattleSecondaryPanelScreenBase.h"
#include "UI/Card/WacomFirstPersonCardPresentationMetrics.h"
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
	FVector2D CardSize = FVector2D(178.0f, 252.0f);
	FVector2D EntrySize = FVector2D(198.0f, 274.0f);
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
	FGuid DetailCandidateInstanceId;
	FGuid PinnedInstanceId;
	FVector2D ResolvedCardSize = FVector2D::ZeroVector;
	FVector2D ResolvedEntrySize = FVector2D::ZeroVector;
	FVector2D ResolvedViewportPixels = FVector2D::ZeroVector;
	float ResolvedGlobalUIScale = 1.0f;
	float TargetPhysicalScale = 1.0f;
	float LocalPresentationScale = 1.0f;
	FString Title;
	FString EmptyMessage;
	int32 DrawNavigationCount = 0;
	int32 DiscardNavigationCount = 0;
	int32 ExhaustNavigationCount = 0;
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

	void SetRestingHandCardPresentationProfile(
		const FWacomFirstPersonCardRestingPresentationProfile& InProfile);

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
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DrawTabLabelText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DiscardTabLabelText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ExhaustTabLabelText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DrawTabCountText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DiscardTabCountText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ExhaustTabCountText;
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
	bool QueryViewportPresentationMetrics(
		FVector2D& OutViewportPixels,
		float& OutGlobalUIScale) const;
	void RefreshResponsiveCardLayout(bool bForce = false);
	void ResolveAndApplyResponsiveCardLayout(
		const FVector2D& ViewportPixels,
		float GlobalUIScale,
		bool bForce = false);
	void ApplyResolvedCardLayout();
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
	UWacomBattleCardPileItemViewModel* ResolvePreferredDetailItem() const;
	void RefreshDetailCandidate(bool bShowPinnedImmediately);
	void SetDetailCandidate(
		UWacomBattleCardPileItemViewModel* Item,
		bool bShowImmediately = false);
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
	UPROPERTY(Transient) TObjectPtr<UWacomBattleCardPileItemViewModel> PinnedItem;
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
	bool bHasResolvedCardLayout = false;
	FVector2D CachedViewportPixels = FVector2D::ZeroVector;
	float CachedGlobalUIScale = 0.0f;
	float ResolvedTargetPhysicalScale = 1.0f;
	float ResolvedLocalPresentationScale = 1.0f;
	FVector2D ResolvedCardSize = FVector2D(178.0f, 252.0f);
	FVector2D ResolvedEntrySize = FVector2D(198.0f, 274.0f);
	float ResolvedEntryPaddingPixels = 4.0f;
	float ResolvedSelectionOutlineExtentPixels = 4.0f;
	FWacomFirstPersonCardRestingPresentationProfile RestingHandCardPresentationProfile;

	EWacomBattlePileDetailsTab ActiveTab = EWacomBattlePileDetailsTab::Draw;
	EWacomBattlePileDiscardSection ActiveDiscardSection = EWacomBattlePileDiscardSection::Discard;
};
