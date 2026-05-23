// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Snapshots/BattleSnapshot.h"
#include "WacomBattle3DHandPresenter.generated.h"

class AWacomBattleCardVisualActor;
class APlayerController;
class UBattleHUD;
struct FBattleTargetSelectionView;

DECLARE_MULTICAST_DELEGATE_OneParam(FWacomBattle3DHandPresenterCardInteractionNative, FGuid);

/** Parameters consumed by the deterministic 3D hand layout helper. */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattle3DHandLayoutParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|3D Hand", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "900.0", ToolTip = "Distance from the anchor to the hand center, in Unreal centimeters."))
	float Distance = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|3D Hand", meta = (UIMin = "-240.0", UIMax = "240.0", ToolTip = "Vertical offset from the anchor to the hand center, in Unreal centimeters. Negative values place the hand below the anchor."))
	float VerticalOffset = -60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|3D Hand", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "120.0", ToolTip = "Horizontal spacing between neighboring cards, in Unreal centimeters."))
	float CardSpacing = 34.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|3D Hand", meta = (UIMin = "-30.0", UIMax = "30.0", ToolTip = "Per-card fan yaw in degrees. Cards are distributed symmetrically around the hand center."))
	float FanYawDegrees = 4.0f;
};

/**
 * Prototype presenter for a world-space battle hand made of card actors.
 *
 * Consumes battle snapshots, owns spawned card visuals, and forwards player
 * card clicks to the BattleHUD command entry point.
 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomBattle3DHandPresenter : public AActor
{
	GENERATED_BODY()

public:
	AWacomBattle3DHandPresenter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void Destroyed() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|3D Hand", meta = (ToolTip = "Actor class used for each world-space battle card."))
	TSubclassOf<AWacomBattleCardVisualActor> CardActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|3D Hand", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "900.0", ToolTip = "Distance from the camera or presenter anchor to the hand center, in Unreal centimeters."))
	float Distance = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|3D Hand", meta = (UIMin = "-240.0", UIMax = "240.0", ToolTip = "Vertical offset from the camera or presenter anchor to the hand center, in Unreal centimeters. Negative values place the hand below the anchor."))
	float VerticalOffset = -60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|3D Hand", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "120.0", ToolTip = "Horizontal spacing between neighboring cards, in Unreal centimeters."))
	float CardSpacing = 34.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|3D Hand", meta = (UIMin = "-30.0", UIMax = "30.0", ToolTip = "Per-card fan yaw in degrees. Cards are distributed symmetrically around the hand center."))
	float FanYawDegrees = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|3D Hand", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "80.0", ToolTip = "Amount each card lifts while hovered, in Unreal centimeters."))
	float HoverLift = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|3D Hand", meta = (ToolTip = "When true, the hand uses the local player camera as its anchor. When false, or when no local player camera exists, the presenter actor transform is used for deterministic layout."))
	bool bFollowLocalPlayerCamera = true;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|3D Hand")
	void RefreshFromSnapshot(const FBattleSnapshot& Snapshot);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|3D Hand")
	void SetOwningBattleHUD(UBattleHUD* InHUD);

	void SetTargetSelectionView(const FBattleTargetSelectionView& TargetSelectionView);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|3D Hand")
	void SetPendingTargetingCard(const FGuid& CardInstanceId);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|3D Hand")
	FGuid GetPendingTargetingCardId() const { return PendingTargetingCardId; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|3D Hand")
	int32 GetSpawnedCardActorCount() const { return CardActors.Num(); }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|3D Hand")
	AWacomBattleCardVisualActor* GetCardActor(const FGuid& CardInstanceId) const;

	const TArray<FGuid>& GetOrderedCardIds() const { return OrderedCardIds; }
	const TMap<FGuid, TObjectPtr<AWacomBattleCardVisualActor>>& GetCardActors() const { return CardActors; }

	FWacomBattle3DHandLayoutParams GetLayoutParams() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|3D Hand")
	static FTransform ComputeCardTransform(
		int32 NumCards,
		int32 CardIndex,
		const FTransform& AnchorTransform,
		const FWacomBattle3DHandLayoutParams& LayoutParams);

	FWacomBattle3DHandPresenterCardInteractionNative OnCardClickedNative;
	FWacomBattle3DHandPresenterCardInteractionNative OnCardHoveredNative;
	FWacomBattle3DHandPresenterCardInteractionNative OnCardUnhoveredNative;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	FTransform ResolveAnchorTransform() const;
	void ApplyCurrentLayout();
	void ApplyTargetingHighlights();
	void DestroySpawnedCards();

private:
	UPROPERTY(Transient)
	TMap<FGuid, TObjectPtr<AWacomBattleCardVisualActor>> CardActors;

	UPROPERTY(Transient)
	TArray<FGuid> OrderedCardIds;

	TWeakObjectPtr<UBattleHUD> OwningBattleHUD;
	FGuid PendingTargetingCardId;

	APlayerController* ResolveOwningPlayerController() const;
	AWacomBattleCardVisualActor* SpawnCardActor(const FGuid& CardInstanceId, int32 CardIndex, int32 CardCount);
	void DestroyCardActor(AWacomBattleCardVisualActor* CardActor);

	void HandleCardClicked(AWacomBattleCardVisualActor* CardActor, FGuid CardInstanceId);
	void HandleCardHovered(AWacomBattleCardVisualActor* CardActor, FGuid CardInstanceId);
	void HandleCardUnhovered(AWacomBattleCardVisualActor* CardActor, FGuid CardInstanceId);
};
