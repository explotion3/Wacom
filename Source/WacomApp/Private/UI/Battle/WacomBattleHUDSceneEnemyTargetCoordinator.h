// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/WacomInteractionTargetTypes.h"

class AWacomBattleEnemyActor;
class UBattleHUD;
class UWacomBattleEnemyPartPresentationComponent;
class UWacomBattleEnemyPartWorldTargetBridgeComponent;
struct FBattleSnapshot;
struct FWacomBattlePresentationTargetCue;
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
	UWacomBattleEnemyPartPresentationComponent* ResolveWorldTargetPresentation(
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

	int32 GetRegisteredBridgeCount() const { return SceneEnemyPartWorldTargets.Num(); }

private:
	struct FSceneEnemyPartWorldTargetEntry
	{
		TWeakObjectPtr<UWacomBattleEnemyPartWorldTargetBridgeComponent> Bridge;
		TWeakObjectPtr<UWacomBattleEnemyPartPresentationComponent> Presentation;
	};

	UBattleHUD& HUD;
	TArray<TWeakObjectPtr<AWacomBattleEnemyActor>> SceneEnemyHosts;
	TArray<FSceneEnemyPartWorldTargetEntry> SceneEnemyPartWorldTargets;
	TWeakObjectPtr<UWacomBattleEnemyPartPresentationComponent> HoveredPresentation;
	FWacomInteractionTargetHandle HoveredHandle;
	float HoverProbeElapsedSeconds = 0.0f;
};
