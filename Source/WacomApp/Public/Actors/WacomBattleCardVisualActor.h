// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Snapshots/HandSnapshot.h"
#include "WacomBattleCardVisualActor.generated.h"

class UBoxComponent;
class UCardWidget;
class USceneComponent;
class UWidgetComponent;

DECLARE_MULTICAST_DELEGATE_TwoParams(FWacomBattleCardVisualActorInteractionNative, class AWacomBattleCardVisualActor*, FGuid);

/**
 * World-space card visual for the prototype 3D battle hand.
 *
 * The actor owns only presentation state. Battle rules still flow through
 * snapshots and HUD commands.
 */
UCLASS(Blueprintable, meta = (ToolTip = "3D 手牌 prototype 的世界空间单卡视觉 Actor。仅用于 PIE / 开发验证；正式主手牌方向是 first-person card layer。"))
class WACOMAPP_API AWacomBattleCardVisualActor : public AActor
{
	GENERATED_BODY()

public:
	AWacomBattleCardVisualActor();

	virtual void NotifyActorOnClicked(FKey ButtonPressed) override;
	virtual void NotifyActorBeginCursorOver() override;
	virtual void NotifyActorEndCursorOver() override;

	void ApplyCardSnapshot(const FHandCardSnapshot& InSnapshot);
	void SetTargetingHighlight(bool bHighlighted);
	void SetHovered(bool bInHovered);
	void SetBaseWorldTransform(const FTransform& InBaseWorldTransform);
	void SetHoverOffset(const FVector& InHoverOffset);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|3D Hand|Prototype")
	FGuid GetCardInstanceId() const { return CachedSnapshot.InstanceId; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|3D Hand|Prototype")
	UWidgetComponent* GetCardFaceWidget() const { return CardFaceWidget; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|3D Hand|Prototype")
	UBoxComponent* GetInteractionBounds() const { return InteractionBounds; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|3D Hand|Prototype")
	bool IsHovered() const { return bIsHovered; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|3D Hand|Prototype")
	bool IsTargetingHighlighted() const { return bIsTargetingHighlighted; }

	const FHandCardSnapshot& GetCardSnapshot() const { return CachedSnapshot; }
	const FTransform& GetBaseWorldTransform() const { return BaseWorldTransform; }
	FVector GetHoverOffset() const { return HoverOffset; }

	FWacomBattleCardVisualActorInteractionNative OnCardClickedNative;
	FWacomBattleCardVisualActorInteractionNative OnCardHoveredNative;
	FWacomBattleCardVisualActorInteractionNative OnCardUnhoveredNative;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|3D Hand|Prototype", meta = (AllowPrivateAccess = "true", ToolTip = "3D 手牌 prototype 的场景根组件。"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|3D Hand|Prototype", meta = (AllowPrivateAccess = "true", ToolTip = "3D 手牌 prototype 的世界空间点击 / hover 命中范围。"))
	TObjectPtr<UBoxComponent> InteractionBounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|3D Hand|Prototype", meta = (AllowPrivateAccess = "true", ToolTip = "3D 手牌 prototype 的卡面 WidgetComponent。正式主手牌不走该 WidgetComponent 路径。"))
	TObjectPtr<UWidgetComponent> CardFaceWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|3D Hand|Prototype", meta = (ToolTip = "3D 手牌 prototype：世界空间卡面使用的 Widget 类。可回退到 WBP_CardWidget 或 UCardWidget；正式 first-person 手牌不使用该路径。"))
	TSubclassOf<UCardWidget> CardWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|3D Hand|Prototype", meta = (ClampMin = "0.01", UIMin = "1.0", UIMax = "1.2", ToolTip = "3D 手牌 prototype：该卡是 pending target card 时使用的世界 Actor 缩放倍率。"))
	float TargetingHighlightScale = 1.04f;

private:
	UPROPERTY(Transient)
	FHandCardSnapshot CachedSnapshot;

	FTransform BaseWorldTransform;
	FVector HoverOffset = FVector(0.0f, 0.0f, 18.0f);
	bool bHasBaseWorldTransform = false;
	bool bIsHovered = false;
	bool bIsTargetingHighlighted = false;

	UCardWidget* EnsureCardWidget();
	UCardWidget* GetCardWidget() const;
	void RequestCardFaceRedraw() const;
	void CaptureCurrentTransformAsBaseIfNeeded();
	void ApplyVisualWorldTransform();

	UFUNCTION()
	void HandleCardWidgetClicked(FGuid CardInstanceId);
};
