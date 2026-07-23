// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WacomBattleEnemyPartOutlineProxyGeometry.h"

class UMaterialInstanceDynamic;
class UPrimitiveComponent;
class UPaperSprite;
class UStaticMeshComponent;
class UWacomBattleEnemyPartTargetPreviewStyle;

enum class EWacomBattleEnemyPartOutlineState : uint8
{
	None,
	Selectable,
	Hovered,
};

struct FWacomBattleEnemyPartOutlineFeedbackDebugView
{
	FName State = TEXT("None");
	bool bComponentCreated = false;
	bool bVisible = false;
	int32 ComponentCreateCount = 0;
};

/**
 * App-private enemy interaction outline presentation.
 *
 * It maps one typed visual onto a padded transient quad. The quad gains room for
 * alpha dilation while the source silhouette keeps its authored world size.
 * The proxy never becomes an authored visual layer and never owns target identity.
 */
class FWacomBattleEnemyPartOutlineFeedbackController
{
public:
	void BeginOrUpdate(
		UActorComponent& LifetimeOwner,
		UPrimitiveComponent* SourceVisual,
		const UWacomBattleEnemyPartTargetPreviewStyle* Style,
		EWacomBattleEnemyPartOutlineState State);
	bool Tick();
	void ResetImmediate(bool bDestroyComponent);

	const FWacomBattleEnemyPartOutlineFeedbackDebugView& GetDebugView() const
	{
		return DebugView;
	}

	static FName StateToName(EWacomBattleEnemyPartOutlineState State);

private:
	UStaticMeshComponent* ResolveOrCreateProxy(UActorComponent& LifetimeOwner);
	void DestroyProxy();
	void SyncProxy();
	bool SyncProxyFrame(UPaperSprite& Sprite);
	UPaperSprite* ResolveCurrentSprite() const;

	TWeakObjectPtr<UPrimitiveComponent> SourceVisual;
	TWeakObjectPtr<UStaticMeshComponent> Proxy;
	TWeakObjectPtr<UPaperSprite> CachedSprite;
	TWeakObjectPtr<const UWacomBattleEnemyPartTargetPreviewStyle> ActiveStyle;
	TWeakObjectPtr<UActorComponent> LifetimeOwner;
	TWeakObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;
	FWacomBattleEnemyPartOutlineProxyFrame CachedFrame;
	EWacomBattleEnemyPartOutlineState ActiveState = EWacomBattleEnemyPartOutlineState::None;
	FWacomBattleEnemyPartOutlineFeedbackDebugView DebugView;
};
