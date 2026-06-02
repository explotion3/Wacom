// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/WacomInteractionTargetTypes.h"

class AWacomBattleEnemyActor;
class UBattleHUD;
class UWacomBattleEnemyPartWorldTargetBridgeComponent;
struct FBattleSnapshot;
struct FWacomBattleEnemyPartDragPredictionDebugInput;

class FWacomBattleHUDSceneEnemyTargetCoordinator
{
public:
	explicit FWacomBattleHUDSceneEnemyTargetCoordinator(UBattleHUD& InHUD);

	void SetSceneEnemyHost(AWacomBattleEnemyActor* InHost);
	AWacomBattleEnemyActor* GetSceneEnemyHost() const { return SceneEnemyHost.Get(); }
	bool HasSceneEnemyHost() const { return SceneEnemyHost.IsValid(); }

	bool IsWorldTargetInCurrentRegistry(const FWacomInteractionTargetHandle& TargetHandle) const;
	UWacomBattleEnemyPartWorldTargetBridgeComponent* ResolveWorldTargetBridge(
		const FWacomInteractionTargetHandle& TargetHandle) const;
	bool IsBridgeInCurrentRegistry(const UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge) const;

	void RebuildRegistry();
	void SyncWorldTargets(const FBattleSnapshot& Snapshot);
	void ClearWorldTargets();

	bool CanUpdateHoverProbe() const;
	void TickHoverProbe(float DeltaTime);
	void UpdateHoverProbe();
	void ClearHoverProbe(FName Reason);
	FWacomBattleEnemyPartDragPredictionDebugInput BuildHoverPredictionInput(
		const FWacomInteractionTargetHandle& TargetHandle) const;

	int32 GetRegisteredBridgeCount() const { return SceneEnemyPartWorldTargetBridges.Num(); }

private:
	UBattleHUD& HUD;
	TWeakObjectPtr<AWacomBattleEnemyActor> SceneEnemyHost;
	TArray<TWeakObjectPtr<UWacomBattleEnemyPartWorldTargetBridgeComponent>> SceneEnemyPartWorldTargetBridges;
	TWeakObjectPtr<UWacomBattleEnemyPartWorldTargetBridgeComponent> HoveredBridge;
	FWacomInteractionTargetHandle HoveredHandle;
	float HoverProbeElapsedSeconds = 0.0f;
};
