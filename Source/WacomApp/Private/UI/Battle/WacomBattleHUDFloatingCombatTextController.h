// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleFloatingCombatTextSynchronizer.h"

class FWacomBattleHUDRuntime;
struct FBattleEvent;
struct FWacomBattleFloatingCombatTextEmission;
struct FWacomBattleFloatingCombatTextRow;
struct FWacomBattleFloatingCombatTextSpawnRequest;
struct FWacomBattlePresentationProgress;

/** BattleHUD 私有的飘字事务、坐标捕获和可选世界装饰协调器。 */
class FWacomBattleHUDFloatingCombatTextController
{
public:
	explicit FWacomBattleHUDFloatingCombatTextController(
		FWacomBattleHUDRuntime& InRuntime);

	void StageResolvedCommand(
		uint64 PresentationTransactionId,
		const TArray<FBattleEvent>& Events);
	void ApplyPresentationProgress(const FWacomBattlePresentationProgress& Progress);
	void FlushTransaction(uint64 PresentationTransactionId);
	void DiscardTransaction(uint64 PresentationTransactionId);
	void Tick(float DeltaTime);
	void Clear();

private:
	FWacomBattleHUDRuntime& Runtime;
	FWacomBattleFloatingCombatTextSynchronizer Synchronizer;

	void PresentEmissions(
		const TArray<FWacomBattleFloatingCombatTextEmission>& Emissions);
	bool BuildSpawnRequest(
		const FWacomBattleFloatingCombatTextRow& Row,
		FWacomBattleFloatingCombatTextSpawnRequest& OutRequest) const;
	bool ResolvePlayerAnchor(FVector2D& OutWidgetPosition) const;
	bool ResolveEnemyAnchor(
		const FBattleEnemyPartKey& PartKey,
		FVector2D& OutWidgetPosition,
		FVector& OutWorldLocation) const;
	FVector2D ResolveViewportFallback(bool bPlayer) const;
	void PlayOptionalWorldAccent(
		const FWacomBattleFloatingCombatTextSpawnRequest& Request) const;
};
