// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Layout/Geometry.h"
#include "../Card/WacomFirstPersonCardDepthMotion.h"

class UCanvasPanel;
class UMaterialInterface;
class UWacomBackpackWorkspaceStyle;
class UWacomDeckCardWidget;
struct FWacomBackpackWorkspaceCarryState;

struct FWacomBackpackWorkspaceCardVisualState
{
	FLinearColor Tint = FLinearColor::White;
	float Opacity = 1.0f;
	float FeedbackOpacity = 0.0f;
	UMaterialInterface* FeedbackMaterial = nullptr;
};

/**
 * Backpack-private card presentation owner.
 *
 * Workspace owns input and base Canvas layout. This controller exclusively owns local card pose,
 * active-card DepthMotion, pickup feedback and settlement completion. It never reads Run rules and
 * never enters the Battle slot / transition-hint state machine.
 */
class WACOMAPP_API FWacomBackpackWorkspaceMotionCoordinator
{
public:
	static FWacomBackpackWorkspaceCardVisualState BuildVisualState(
		const UWacomBackpackWorkspaceStyle& Style,
		bool bSelected,
		bool bCurrent,
		bool bReadOnly,
		bool bValidTarget = false,
		bool bRejectedTarget = false);
	void Reconcile(
		TConstArrayView<TWeakObjectPtr<UWacomDeckCardWidget>> Cards,
		UWacomDeckCardWidget* FocusedCard,
		const FWacomBackpackWorkspaceCarryState* Carry,
		const UCanvasPanel* CarryLayer,
		const FGeometry& WorkspaceGeometry,
		FVector2D PointerLocal,
		const UWacomBackpackWorkspaceStyle& Style,
		bool bSimplifiedMotion);
	void UpdatePointer(const FGeometry& WorkspaceGeometry, FVector2D PointerLocal, bool bCarrying);
	void SetLocalPoseTarget(
		UWacomDeckCardWidget& Card,
		FVector2D Translation,
		float AngleDegrees,
		float DurationSeconds,
		bool bSimplifiedMotion);
	void SnapLocalPose(UWacomDeckCardWidget& Card, FVector2D Translation, float AngleDegrees);
	/** 停止局部运动但保留 Widget 当前姿态，供选择视觉快照接管。 */
	void StopLocalPoseMotionPreservingCurrent(UWacomDeckCardWidget& Card);
	void BeginCarryPickup(
		TConstArrayView<TWeakObjectPtr<UWacomDeckCardWidget>> Cards,
		float LiftPixels,
		float DurationSeconds,
		bool bSimplifiedMotion);
	void BeginSettlement(
		UWacomDeckCardWidget& Card,
		FVector2D StartLocalTranslation,
		float StartLocalAngleDegrees,
		float DurationSeconds,
		bool bSimplifiedMotion);
	void Tick(
		float DeltaTime,
		const FGeometry& WorkspaceGeometry,
		const UWacomBackpackWorkspaceStyle& Style,
		bool bSimplifiedMotion);
	bool WantsTick() const;
	void ConsumeCompletedSettlements(TArray<TWeakObjectPtr<UWacomDeckCardWidget>>& OutCards);
	void Reset();

	UWacomDeckCardWidget* GetActiveCard() const { return ActiveCard.Get(); }
	int32 GetMovingCardCount() const { return LocalPoseMotions.Num(); }
	int32 GetRealtimeCardCount() const { return ActiveCard.IsValid() ? 1 : 0; }
	bool IsCardMoving(const UWacomDeckCardWidget& Card) const;

private:
	struct FLocalPoseMotion
	{
		FVector2D StartTranslation = FVector2D::ZeroVector;
		FVector2D TargetTranslation = FVector2D::ZeroVector;
		float StartAngleDegrees = 0.0f;
		float TargetAngleDegrees = 0.0f;
		float ElapsedSeconds = 0.0f;
		float DurationSeconds = 0.0f;
		bool bSettlement = false;
	};

	TWeakObjectPtr<UWacomDeckCardWidget> ActiveCard;
	TMap<TWeakObjectPtr<UWacomDeckCardWidget>, FLocalPoseMotion> LocalPoseMotions;
	TArray<TWeakObjectPtr<UWacomDeckCardWidget>> PickupCards;
	TArray<TWeakObjectPtr<UWacomDeckCardWidget>> CompletedSettlements;
	FWacomFirstPersonCardDepthMotion ActiveDepthMotion;
	FVector2D PointerLocal = FVector2D::ZeroVector;
	FVector2D LastDepthPointerLocal = FVector2D::ZeroVector;
	float PickupElapsedSeconds = 0.0f;
	float PickupDurationSeconds = 0.0f;
	float PickupLiftPixels = 0.0f;
	float LastPickupOffsetPixels = 0.0f;
	bool bHasPointer = false;
	bool bDepthPointerChanged = false;
	bool bActiveCardCarrying = false;
	bool bSimplified = false;

	void DisableActiveCard();
	void UpdateActiveDepth(
		float DeltaTime,
		const FGeometry& WorkspaceGeometry,
		const UWacomBackpackWorkspaceStyle& Style);
};
