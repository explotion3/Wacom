// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RunStateTypes.h"
#include "UI/Backpack/WacomBackpackZonePileTypes.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "WacomBackpackScreenPresenter.generated.h"

class UCardDefinition;
class UMaterialInterface;
class UWacomBackpackWorkspaceStyle;

struct WACOMAPP_API FWacomBackpackWorkspaceCardVisualState
{
	FLinearColor Tint = FLinearColor::White;
	float Opacity = 1.0f;
	float FeedbackOpacity = 0.0f;
	UMaterialInterface* FeedbackMaterial = nullptr;
};

/**
 * Pure presentation helpers for BackpackScreen.
 *
 * The screen owns widgets, events, and Run commands. This presenter owns only
 * deterministic display text/data and viewport-safe placement calculations.
 */
UCLASS()
class WACOMAPP_API UWacomBackpackScreenPresenter : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static FWacomBackpackWorkspaceCardVisualState BuildWorkspaceCardVisualState(
		const UWacomBackpackWorkspaceStyle* Style,
		bool bSelected,
		bool bCurrent,
		bool bReadOnly,
		bool bValidTarget = false,
		bool bRejectedTarget = false);
	static FText BuildBatchDeleteSummaryText(int32 CardCount, int32 TotalGoldReward);
	/** 从同一只读 Snapshot 构造工作台内嵌牌堆；通量卡不生成牌堆。 */
	static TArray<FWacomBackpackZonePileView> BuildWorkspacePileViews(
		const FRunBackpackStorageSnapshot& Snapshot,
		EZoneKind ExpandedZone,
		FGuid ExpandedOwnerInstanceId,
		bool bHasExpandedPile);

	UFUNCTION(BlueprintPure, Category = "Wacom|Backpack|Presentation")
	static FText BuildBattleDeckTitleText(int32 Count, int32 Capacity);

	UFUNCTION(BlueprintPure, Category = "Wacom|Backpack|Presentation")
	static FText BuildBackpackTitleText();

	UFUNCTION(BlueprintPure, Category = "Wacom|Backpack|Presentation")
	static FText BuildGoldText(int32 Gold);

	UFUNCTION(BlueprintPure, Category = "Wacom|Backpack|Presentation")
	static FText BuildFluxContentTitleText(int32 Count, int32 Capacity);

	UFUNCTION(BlueprintPure, Category = "Wacom|Backpack|Presentation")
	static FText BuildSpecialZoneTitleText(const FText& OwnerName, int32 CardCount, int32 Capacity);

	UFUNCTION(BlueprintPure, Category = "Wacom|Backpack|Presentation")
	static FText BuildBurdenZoneTitleText(int32 CardCount);

	UFUNCTION(BlueprintPure, Category = "Wacom|Backpack|Presentation")
	static ESlateVisibility GetBurdenZoneVisibility(int32 CardCount);

	UFUNCTION(BlueprintPure, Category = "Wacom|Backpack|Presentation")
	static ESlateVisibility GetSpecialZoneBattleReadyBadgeVisibility(EZoneKind OwnerZone);

	UFUNCTION(BlueprintPure, Category = "Wacom|Backpack|Presentation")
	static FText BuildProjectedFromBadgeText(const FText& OwnerName);

	UFUNCTION(BlueprintPure, Category = "Wacom|Backpack|Presentation")
	static FText BuildBattleDeckProjectedFromBadgeText(
		const FRunStorageCardView& ProjectedCard,
		const FRunBackpackStorageSnapshot& Snapshot);

	UFUNCTION(BlueprintPure, Category = "Wacom|Backpack|Presentation")
	static FWacomCardDetailViewData BuildCardDetailViewData(const UCardDefinition* Card);

	UFUNCTION(BlueprintPure, Category = "Wacom|Backpack|Presentation")
	static FVector2D ComputeCardDetailPanelPosition(
		FVector2D AnchorPosition,
		FVector2D AnchorSize,
		FVector2D LayerSize,
		FVector2D PanelSize,
		float Padding = 12.f);
};
