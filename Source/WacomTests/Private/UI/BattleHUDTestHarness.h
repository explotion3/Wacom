// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/StrongObjectPtr.h"

class AWacomBattleEnemyActor;
class AWacomBattleEnemyPartActor;
class AWacomBattleHUDLocalPlayerControllerTest;
class AWacomPlayerCharacter;
class UBattleCombatLogFeedWidget;
class UBattleCommandBarWidget;
class UBattlePresentationStackWidget;
class UBattleSession;
class UEnemyDefinition;
class UWacomBattleCommandBarTestProbe;
class UWacomBattleCameraLookComponent;
class UWacomBattleHUDDetailTest;
class UWacomFirstPersonCardAnchorComponent;

struct FWacomBattlePresentationStackEntryView;

struct FWacomBattleHUDTestSceneEnemyHost
{
	AWacomBattleEnemyActor* Host = nullptr;
	TArray<AWacomBattleEnemyPartActor*> Parts;
};

class FWacomBattleHUDTestHarness
{
public:
	~FWacomBattleHUDTestHarness();

	static TUniquePtr<FWacomBattleHUDTestHarness> CreateHUDOnly(UWorld* InWorld);
	static TUniquePtr<FWacomBattleHUDTestHarness> CreateHUDWithPlayer(UWorld* InWorld);

	UWorld* GetWorld() const { return World.Get(); }
	AWacomBattleHUDLocalPlayerControllerTest* PlayerController() const { return PC.Get(); }
	UWacomBattleHUDDetailTest* HUD() const { return HUDPtr.Get(); }
	UBattleCombatLogFeedWidget* CombatLogFeed() const { return CombatLogFeedPtr.Get(); }
	UBattlePresentationStackWidget* PresentationStack() const { return PresentationStackPtr.Get(); }
	UWacomBattleCommandBarTestProbe* CommandBar() const { return CommandBarPtr.Get(); }
	AWacomPlayerCharacter* FirstPersonCharacter() const { return FirstPersonCharacterActor.Get(); }
	UWacomFirstPersonCardAnchorComponent* FirstPersonAnchor() const { return FirstPersonAnchorPtr; }
	UWacomBattleCameraLookComponent* BattleCameraLook() const { return BattleCameraLookPtr; }
	const FWacomBattleHUDTestSceneEnemyHost& SceneEnemyHost() const { return CurrentSceneEnemyHost; }

	UBattleCombatLogFeedWidget* AttachCombatLogFeed();
	UBattlePresentationStackWidget* AttachPresentationStack();
	UWacomBattleCommandBarTestProbe* AttachCommandBar();
	AWacomPlayerCharacter* AttachFirstPersonCharacter();
	FWacomBattleHUDTestSceneEnemyHost& AttachSceneEnemyHost(
		UEnemyDefinition* EnemyDefinition,
		const TArray<FName>& PartIds);

	void SetSession(UBattleSession* Session, bool bSettleInitialPresentation = true);
	void SettlePresentationQueue(int32 MaxSteps = 32);
	void SettlePresentationQueueAndExitStack(int32 MaxSteps = 64);

private:
	explicit FWacomBattleHUDTestHarness(UWorld* InWorld);

	static void DestroySceneEnemyHost(FWacomBattleHUDTestSceneEnemyHost& Actors);
	void DestroySpawnedActors();

	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<AWacomBattleHUDLocalPlayerControllerTest> PC;
	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUDPtr;
	TStrongObjectPtr<UBattleCombatLogFeedWidget> CombatLogFeedPtr;
	TStrongObjectPtr<UBattlePresentationStackWidget> PresentationStackPtr;
	TStrongObjectPtr<UWacomBattleCommandBarTestProbe> CommandBarPtr;
	TWeakObjectPtr<AWacomPlayerCharacter> FirstPersonCharacterActor;
	UWacomFirstPersonCardAnchorComponent* FirstPersonAnchorPtr = nullptr;
	UWacomBattleCameraLookComponent* BattleCameraLookPtr = nullptr;
	FWacomBattleHUDTestSceneEnemyHost CurrentSceneEnemyHost;
};
