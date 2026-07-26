// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomCardDetailPanel.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "UI/Card/WacomCardSemanticTooltipWidget.h"
#include "UI/Card/WacomWorldCardInteractionTypes.h"
#include "UObject/StrongObjectPtr.h"

class AWacomPlayerController;
class UCardDefinition;
class UUserWidget;
class UWidgetComponent;
class UWacomCardView;

struct FWacomWorldCardInteractionItemView
{
	FGuid ItemId;
	TWeakObjectPtr<UCardDefinition> Definition;
	TWeakObjectPtr<UWidgetComponent> WidgetComponent;
	TWeakObjectPtr<UUserWidget> RootWidget;
	TWeakObjectPtr<UWacomCardView> CardView;
	FWacomCardDetailViewData DetailViewData;
};

/**
 * Presentation-only world-card interaction state.
 *
 * It never reads RunSession or submits commands. The activity coordinator
 * supplies immutable items and pointer samples, then keeps purchase input on
 * its existing path.
 */
class WACOMAPP_API FWacomWorldCardInteractionPresenter
{
public:
	void SyncItems(
		const TArray<FWacomWorldCardInteractionItemView>& InItems);

	void Tick(
		AWacomPlayerController& PlayerController,
		float DeltaTime,
		const FWacomWorldCardPointerSample& PointerSample,
		const FWacomWorldCardInteractionStyle& InStyle);

	/** Returns true whenever the world-card activity owns this right click. */
	bool RouteRightClick(
		AWacomPlayerController& PlayerController,
		const FWacomWorldCardPointerSample& PointerSample,
		const FWacomWorldCardInteractionStyle& InStyle);

	void Reset();

#if WITH_AUTOMATION_TESTS
	FGuid GetHoveredItemIdForTest() const { return HoveredItemId; }
	FGuid GetPinnedItemIdForTest() const { return PinnedItemId; }
	FName GetHoveredSemanticIdForTest() const
	{
		return HoveredSemanticId;
	}
	bool IsTooltipVisibleForTest() const;
	bool IsInspectVisibleForTest() const;
	static FVector2D ComputeTooltipPositionForTest(
		const FVector2D& MousePosition,
		const FVector2D& TooltipSize,
		const FVector2D& ViewportSize,
		const FWacomWorldCardInteractionStyle& Style)
	{
		return ComputeTooltipPosition(
			MousePosition,
			TooltipSize,
			ViewportSize,
			Style.Sanitized());
	}
	static FTransform ComputeHoverWorldTransformForTest(
		const FTransform& BaseWorld,
		const FVector& CameraLocation,
		float HoverAlpha,
		const FWacomWorldCardInteractionStyle& Style);
	static FVector2D ComputeInspectPositionForTest(
		const bool bTargetIsOnLeft,
		const FVector2D& ViewportSize,
		const FWacomWorldCardInteractionStyle& Style)
	{
		return ComputeInspectPosition(
			bTargetIsOnLeft,
			ViewportSize,
			Style.Sanitized());
	}
#endif

private:
	struct FItemRuntime
	{
		FWacomWorldCardInteractionItemView View;
		FTransform BaseRelativeTransform = FTransform::Identity;
		float HoverAlpha = 0.0f;
	};

	FItemRuntime* FindItem(const FGuid& ItemId);
	const FItemRuntime* FindItem(const FGuid& ItemId) const;
	FItemRuntime* FindItemForComponent(const UWidgetComponent* Component);
	void RestoreItemTransform(FItemRuntime& Item);
	void UpdateHoverTransforms(
		AWacomPlayerController& PlayerController,
		float DeltaTime,
		const FWacomWorldCardInteractionStyle& Style);
	static FTransform ComputeHoverWorldTransform(
		const FTransform& BaseWorld,
		const FVector& CameraLocation,
		float HoverAlpha,
		const FWacomWorldCardInteractionStyle& Style);
	bool ResolveSemanticUnderPointer(
		const FWacomWorldCardPointerSample& PointerSample,
		FItemRuntime*& OutItem,
		FWacomCardFaceSemanticTokenView& OutToken);
	void UpdateSemanticTooltip(
		AWacomPlayerController& PlayerController,
		float DeltaTime,
		const FWacomWorldCardPointerSample& PointerSample,
		const FWacomWorldCardInteractionStyle& Style);
	void EnsureTooltip(AWacomPlayerController& PlayerController);
	void HideTooltip();
	void PositionTooltip(
		AWacomPlayerController& PlayerController,
		const FWacomWorldCardInteractionStyle& Style);
	void OpenInspect(
		AWacomPlayerController& PlayerController,
		const FItemRuntime& Item,
		const FWacomWorldCardInteractionStyle& Style);
	void CloseInspect();

	static FVector2D GetViewportLogicalSize(
		AWacomPlayerController& PlayerController);
	static FVector2D ComputeTooltipPosition(
		const FVector2D& MousePosition,
		const FVector2D& TooltipSize,
		const FVector2D& ViewportSize,
		const FWacomWorldCardInteractionStyle& Style);
	static FVector2D ComputeInspectPosition(
		bool bTargetIsOnLeft,
		const FVector2D& ViewportSize,
		const FWacomWorldCardInteractionStyle& Style);

	TArray<FItemRuntime> Items;
	FGuid HoveredItemId;
	FGuid PinnedItemId;
	FGuid HoveredSemanticItemId;
	FName HoveredSemanticId = NAME_None;
	FGuid DisplayedSemanticItemId;
	FName DisplayedSemanticId = NAME_None;
	float SemanticHoverElapsedSeconds = 0.0f;
	bool bCurrentSemanticMissing = false;
	TStrongObjectPtr<UWacomCardSemanticTooltipWidget> TooltipWidget;
	TStrongObjectPtr<UWacomCardDetailPanel> InspectPanel;
};
