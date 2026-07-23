// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WacomBackpackWorkspaceTypes.h"
#include "UI/Backpack/WacomBackpackZonePileTypes.h"
#include "UI/Backpack/WacomDeckCardWidget.h"

class UCanvasPanel;
class UPanelWidget;
class UUserWidget;
class UWacomBackpackZonePileWidget;
class UWacomDeckCardWidget;
struct FWacomBackpackWorkspaceSceneCardEntry;
struct FWacomBackpackWorkspaceScenePileEntry;

struct FWacomBackpackWorkspaceCardViewKey
{
	FGuid InstanceId;
	FGuid OwnerInstanceId;
	EZoneKind PhysicalZone = EZoneKind::Backpack;
	EWacomBackpackDeckCardListReuseRole Role =
		EWacomBackpackDeckCardListReuseRole::PhysicalList;

	friend bool operator==(
		const FWacomBackpackWorkspaceCardViewKey& A,
		const FWacomBackpackWorkspaceCardViewKey& B)
	{
		return A.InstanceId == B.InstanceId
			&& A.OwnerInstanceId == B.OwnerInstanceId
			&& A.PhysicalZone == B.PhysicalZone
			&& A.Role == B.Role;
	}
};

uint32 GetTypeHash(const FWacomBackpackWorkspaceCardViewKey& Key);

/**
 * Workspace 的唯一视觉实例注册表。
 *
 * 每次 Scene reconcile 都从实际 Canvas 子控件线性重建索引；缓存只用于本次/下一次
 * 身份查询，不作为 UObject 所有权真相。
 */
class WACOMAPP_API FWacomBackpackWorkspaceVisualRegistry
{
public:
	void RebuildCardIndexes(
		TConstArrayView<UPanelWidget*> SearchPanels,
		TFunctionRef<bool(const UWacomDeckCardWidget*)> PreserveCurrentParent,
		TFunctionRef<bool(const UWacomDeckCardWidget*)> ExcludeFromRegistry);

	void ReconcileCards(
		TConstArrayView<UPanelWidget*> SearchPanels,
		UPanelWidget& DestinationPanel,
		TConstArrayView<FWacomBackpackWorkspaceSceneCardEntry> DesiredCards,
		TFunctionRef<bool(const UWacomDeckCardWidget*)> PreserveCurrentParent,
		TFunctionRef<bool(const UWacomDeckCardWidget*)> ExcludeFromRegistry,
		TFunctionRef<UWacomDeckCardWidget*(const FRunStorageCardView&)> CreateWidget,
		TFunctionRef<void(UWacomDeckCardWidget*)> OnRemovedWidget);

	/**
	 * 为不经过 Scene reconcile 的原生测试/兜底入口接管有序卡牌名册。
	 * 正式运行时名册由 ReconcileCards 生成；调用方不得另存第二份长期数组。
	 */
	void ReplaceOrderedCards(TConstArrayView<TObjectPtr<UWacomDeckCardWidget>> Cards);

	void ReconcilePiles(
		UUserWidget& Owner,
		UCanvasPanel& Canvas,
		UClass* PileWidgetClass,
		TConstArrayView<FWacomBackpackWorkspaceScenePileEntry> Piles,
		TFunctionRef<void(UWacomBackpackZonePileWidget&)> BindWidget);

	UWacomDeckCardWidget* FindPhysicalCard(FGuid InstanceId) const;
	UWacomBackpackZonePileWidget* FindPile(const FWacomBackpackZoneKey& Zone) const;
	TConstArrayView<TWeakObjectPtr<UWacomDeckCardWidget>> GetCardWidgets() const
	{
		return OrderedCards;
	}
	const TArray<TWeakObjectPtr<UWacomBackpackZonePileWidget>>& GetPileWidgets() const
	{
		return OrderedPiles;
	}

	void ResetPiles(bool bRemoveFromParent);
	void ResetIndexes();

private:
	TMap<FWacomBackpackWorkspaceCardViewKey, TWeakObjectPtr<UWacomDeckCardWidget>> CardsByViewKey;
	TMap<FGuid, TWeakObjectPtr<UWacomDeckCardWidget>> PhysicalCardsByInstanceId;
	/** Scene 顺序的唯一卡牌视觉名册；不构成 UObject 所有权。 */
	TArray<TWeakObjectPtr<UWacomDeckCardWidget>> OrderedCards;
	TMap<FWacomBackpackZoneKey, TWeakObjectPtr<UWacomBackpackZonePileWidget>> PilesByZone;
	TArray<TWeakObjectPtr<UWacomBackpackZonePileWidget>> OrderedPiles;
};
