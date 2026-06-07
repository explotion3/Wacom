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

	void SetSceneEnemyHosts(const TArray<AWacomBattleEnemyActor*>& InHosts);
	bool HasSceneEnemyHost() const;
	bool IsSceneEnemyHostInCurrentRegistry(const AWacomBattleEnemyActor* Host) const;

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
	TArray<TWeakObjectPtr<AWacomBattleEnemyActor>> SceneEnemyHosts;
	TArray<TWeakObjectPtr<UWacomBattleEnemyPartWorldTargetBridgeComponent>> SceneEnemyPartWorldTargetBridges;
	TWeakObjectPtr<UWacomBattleEnemyPartWorldTargetBridgeComponent> HoveredBridge;
	FWacomInteractionTargetHandle HoveredHandle;
	float HoverProbeElapsedSeconds = 0.0f;
};
