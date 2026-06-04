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

/** 3D 手牌 prototype 的确定性布局参数；仅用于 PIE / 开发验证，不是正式战斗手牌制作入口。 */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattle3DHandLayoutParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|3D Hand|Prototype", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "900.0", ToolTip = "3D 手牌 prototype：手牌中心距离锚点的距离，单位为 Unreal 厘米。仅用于 PIE / 开发验证。"))
	float Distance = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|3D Hand|Prototype", meta = (UIMin = "-240.0", UIMax = "240.0", ToolTip = "3D 手牌 prototype：手牌中心相对锚点的垂直偏移，单位为 Unreal 厘米；负数会放到锚点下方。"))
	float VerticalOffset = -60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|3D Hand|Prototype", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "120.0", ToolTip = "3D 手牌 prototype：相邻卡牌的水平间距，单位为 Unreal 厘米。"))
	float CardSpacing = 34.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|3D Hand|Prototype", meta = (UIMin = "-30.0", UIMax = "30.0", ToolTip = "3D 手牌 prototype：每张卡牌的扇形 yaw 角度差，单位为度；卡牌会围绕手牌中心对称排布。"))
	float FanYawDegrees = 4.0f;
};

/**
 * Prototype presenter for a world-space battle hand made of card actors.
 *
 * Consumes battle snapshots, owns spawned card visuals, and forwards player
 * card clicks to the BattleHUD command entry point.
 */
UCLASS(Blueprintable, meta = (ToolTip = "CardActor + WidgetComponent 的 3D 手牌 prototype Presenter。仅用于 PIE / 开发验证空间手牌可行性；正式主手牌方向是 first-person card layer。"))
class WACOMAPP_API AWacomBattle3DHandPresenter : public AActor
{
	GENERATED_BODY()

public:
	AWacomBattle3DHandPresenter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void Destroyed() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|3D Hand|Prototype", meta = (ToolTip = "3D 手牌 prototype：每张世界空间卡牌使用的 Actor 类。仅用于 PIE / 开发验证，不是正式手牌制作入口。"))
	TSubclassOf<AWacomBattleCardVisualActor> CardActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|3D Hand|Prototype", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "900.0", ToolTip = "3D 手牌 prototype：手牌中心距离相机或 Presenter 锚点的距离，单位为 Unreal 厘米。"))
	float Distance = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|3D Hand|Prototype", meta = (UIMin = "-240.0", UIMax = "240.0", ToolTip = "3D 手牌 prototype：手牌中心相对相机或 Presenter 锚点的垂直偏移，单位为 Unreal 厘米；负数会放到锚点下方。"))
	float VerticalOffset = -60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|3D Hand|Prototype", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "120.0", ToolTip = "3D 手牌 prototype：相邻卡牌的水平间距，单位为 Unreal 厘米。"))
	float CardSpacing = 34.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|3D Hand|Prototype", meta = (UIMin = "-30.0", UIMax = "30.0", ToolTip = "3D 手牌 prototype：每张卡牌的扇形 yaw 角度差，单位为度；卡牌会围绕手牌中心对称排布。"))
	float FanYawDegrees = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|3D Hand|Prototype", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "80.0", ToolTip = "3D 手牌 prototype：卡牌 hover 时上浮的距离，单位为 Unreal 厘米。"))
	float HoverLift = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|3D Hand|Prototype", meta = (ToolTip = "3D 手牌 prototype：开启时使用本地玩家相机作为锚点；关闭或找不到相机时使用 Presenter Actor transform，便于自动化测试确定布局。"))
	bool bFollowLocalPlayerCamera = true;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|3D Hand|Prototype")
	void RefreshFromSnapshot(const FBattleSnapshot& Snapshot);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|3D Hand|Prototype")
	void SetOwningBattleHUD(UBattleHUD* InHUD);

	void SetTargetSelectionView(const FBattleTargetSelectionView& TargetSelectionView);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|3D Hand|Prototype")
	void SetPendingTargetingCard(const FGuid& CardInstanceId);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|3D Hand|Prototype")
	FGuid GetPendingTargetingCardId() const { return PendingTargetingCardId; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|3D Hand|Prototype")
	int32 GetSpawnedCardActorCount() const { return CardActors.Num(); }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|3D Hand|Prototype")
	AWacomBattleCardVisualActor* GetCardActor(const FGuid& CardInstanceId) const;

	const TArray<FGuid>& GetOrderedCardIds() const { return OrderedCardIds; }
	const TMap<FGuid, TObjectPtr<AWacomBattleCardVisualActor>>& GetCardActors() const { return CardActors; }

	FWacomBattle3DHandLayoutParams GetLayoutParams() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|3D Hand|Prototype")
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
