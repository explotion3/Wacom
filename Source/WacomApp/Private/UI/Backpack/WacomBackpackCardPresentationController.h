// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Layout/Geometry.h"

class UCanvasPanel;
class UWacomDeckCardWidget;
struct FWacomBackpackWorkspaceCarryState;

/**
 * 背包私有卡面表现控制器。
 *
 * 只把 Workspace 的 hover/carry 状态映射到 FirstPersonCardView 的视觉参数；
 * 不接入 Battle slot、transition hint 或规则状态机，并保证最多一张卡实时重绘。
 */
class WACOMAPP_API FWacomBackpackCardPresentationController
{
public:
	void Reconcile(
		TConstArrayView<TWeakObjectPtr<UWacomDeckCardWidget>> Cards,
		FGuid HoveredInstanceId,
		const FWacomBackpackWorkspaceCarryState* Carry,
		const UCanvasPanel* CarryLayer,
		const FGeometry& WorkspaceGeometry,
		FVector2D PointerLocal);
	void UpdatePointer(const FGeometry& WorkspaceGeometry, FVector2D PointerLocal, bool bCarrying);
	void Reset();

	UWacomDeckCardWidget* GetActiveCard() const { return ActiveCard.Get(); }

private:
	TWeakObjectPtr<UWacomDeckCardWidget> ActiveCard;
	bool bActiveCardCarrying = false;

	void ApplyActivePointer(const FGeometry& WorkspaceGeometry, FVector2D PointerLocal);
};
