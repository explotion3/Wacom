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
UCLASS(Blueprintable)
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

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|3D Hand")
	FGuid GetCardInstanceId() const { return CachedSnapshot.InstanceId; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|3D Hand")
	UWidgetComponent* GetCardFaceWidget() const { return CardFaceWidget; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|3D Hand")
	UBoxComponent* GetInteractionBounds() const { return InteractionBounds; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|3D Hand")
	bool IsHovered() const { return bIsHovered; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|3D Hand")
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|3D Hand", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|3D Hand", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> InteractionBounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|3D Hand", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetComponent> CardFaceWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|3D Hand", meta = (ToolTip = "Widget class used by the world-space card face. Defaults to WBP_CardWidget when available, otherwise UCardWidget."))
	TSubclassOf<UCardWidget> CardWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|3D Hand", meta = (ClampMin = "0.01", UIMin = "1.0", UIMax = "1.2", ToolTip = "World actor scale multiplier while this card is the pending targeting card."))
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
