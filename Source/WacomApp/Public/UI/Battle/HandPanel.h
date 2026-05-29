// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleWidgetBase.h"
#include "Snapshots/HandSnapshot.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "HandPanel.generated.h"

class UCardWidget;
class UPanelWidget;

DECLARE_MULTICAST_DELEGATE_OneParam(FWacomHandPanelCardHoverStateChangedNative, UCardWidget*);

/**
 * UI-only hand card visual entry.
 *
 * This is the bridge between battle hand snapshot semantics and the current
 * hand renderer. Future unified/curved renderers should consume this instead
 * of reinterpreting FHandQueueSnapshot directly.
 */
USTRUCT(BlueprintType)
struct WACOMAPP_API FHandCardVisualEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|UI")
	FHandCardSnapshot Snapshot;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|UI")
	int32 VisualIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|UI")
	EHandZone LogicalZone = EHandZone::None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|UI")
	bool bIsAnchor = false;
};

/**
 * 手牌容器。当前默认 renderer 是统一水平手牌带。
 *
 * C++ 内置默认外观：Border + HorizontalBox，支持未配 WBP 时的快速预览。
 *
 * WBP 约定：
 * - UnifiedHandSlot : UPanelWidget，当前默认视觉入口
 */
UCLASS(Blueprintable)
class WACOMAPP_API UHandPanel : public UWacomBattleWidgetBase
{
	GENERATED_BODY()

public:
	UHandPanel();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI", meta = (ToolTip = "普通战斗手牌使用的卡牌 Widget 类。通常设置为 WBP_CardWidget；为空时使用 C++ UCardWidget fallback。"))
	TSubclassOf<UCardWidget> CardWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI", meta = (ToolTip = "左右手锚点牌使用的卡牌 Widget 类。为空时会使用 CardWidgetClass。"))
	TSubclassOf<UCardWidget> AnchorCardWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI|Layout", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "80.0", ToolTip = "卡牌之间的水平间距，单位为 Slate 像素。不会改变 WBP_CardWidget 自身尺寸。"))
	float CardSpacing = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI|Layout", meta = (ToolTip = "整条手牌内容的外边距。Left/Right 会叠加到首尾卡牌，Top/Bottom 会应用到每张卡牌的 slot padding。"))
	FMargin HandContentPadding = FMargin(0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI|Layout", meta = (ToolTip = "为 true 时，C++ fallback 会把整条手牌在 HandPanel 内水平居中。若 WBP 使用非 HorizontalBox 容器，可能不会生效。"))
	bool bCenterCardsWhenNotOverflow = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI|Layout", meta = (ToolTip = "卡牌在手牌带中的垂直对齐方式。只影响卡牌所在 slot，不改变卡牌自身大小。"))
	TEnumAsByte<EVerticalAlignment> CardVerticalAlignment = VAlign_Center;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|UI")
	const TArray<FHandCardVisualEntry>& GetCurrentVisualEntries() const { return CurrentVisualEntries; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|UI")
	int32 GetSpawnedCardCount() const { return SpawnedCards.Num(); }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|UI")
	int32 GetUnifiedHandSlotCardCount() const;

	FWacomHandPanelCardHoverStateChangedNative OnCardHoveredNative;
	FWacomHandPanelCardHoverStateChangedNative OnCardUnhoveredNative;

	/** 从当前悬停的手牌卡牌构建统一交互目标 handle。无悬停时返回无效 handle。 */
	FWacomInteractionTargetHandle BuildCardTargetHandle() const;

	static TArray<FHandCardVisualEntry> BuildVisualEntries(const FHandQueueSnapshot& HandSnapshot);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeRefreshFromSnapshot(const struct FBattleSnapshot& Snap) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> UnifiedHandSlot;

	void ApplyUnifiedHandSlotAlignment() const;
	void ApplyCardSlotLayout(UCardWidget* Card, int32 CardIndex, int32 CardCount) const;
	FMargin BuildCardSlotPadding(int32 CardIndex, int32 CardCount) const;
	UCardWidget* GetSpawnedCardAt(int32 Index) const;

private:
	UFUNCTION()
	void HandleCardClicked(FGuid CardInstanceId);

	void HandleCardHovered(UCardWidget* SourceWidget);
	void HandleCardUnhovered(UCardWidget* SourceWidget);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCardWidget>> SpawnedCards;

	UPROPERTY(Transient)
	TArray<FHandCardVisualEntry> CurrentVisualEntries;

	FGuid CachedHoveredCardInstanceId;

	void RebuildUnifiedHorizontalRenderer(const TArray<FHandCardVisualEntry>& Entries);
	UCardWidget* CreateAndPlaceCard(const FHandCardVisualEntry& Entry, UPanelWidget* TargetSlot, int32 CardIndex, int32 CardCount);
	void ClearAllSlots();
	void ApplyTargetingHighlight();
};
