// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FSlateBrush;
class FSlateWindowElementList;
struct FGeometry;

/** Native post-child marquee paint facts, expressed in Workspace-local coordinates. */
struct FWacomBackpackWorkspaceMarqueePaintView
{
	bool bVisible = false;
	FVector2D Start = FVector2D::ZeroVector;
	FVector2D End = FVector2D::ZeroVector;
	FLinearColor Color = FLinearColor::Transparent;
	float FillOpacity = 0.0f;
	float BorderThickness = 2.0f;
};

/** Native post-child card marker facts. Brushes remain owned by the card widget. */
struct FWacomBackpackCardOverlayPaintView
{
	const FSlateBrush* FocusBrush = nullptr;
	const FSlateBrush* SemanticBrush = nullptr;
	FVector2D LocalMotionTranslation = FVector2D::ZeroVector;
	float LocalMotionAngleDegrees = 0.0f;
	FVector2D IconSize = FVector2D(32.0f, 32.0f);
	float Padding = 10.0f;
};

/** Comparable visual order for card bodies that may live in different Workspace panels. */
struct FWacomBackpackOverlayVisualOrder
{
	int32 LayerPriority = 0;
	int32 ZOrder = 0;
	int32 ChildIndex = 0;
	int32 StableIndex = 0;
};

/** Workspace-local oriented card body used to hide markers behind higher cards. */
struct FWacomBackpackCardMarkerOccluder
{
	FWacomBackpackOverlayVisualOrder Order;
	FVector2D Center = FVector2D::ZeroVector;
	FVector2D Size = FVector2D::ZeroVector;
	float AngleDegrees = 0.0f;
};

/** Stateless App-private Slate painter for overlays that must outlive child Retainer composition. */
class WACOMAPP_API FWacomBackpackWorkspaceOverlayPainter
{
public:
	static int32 PaintMarquee(
		const FGeometry& AllottedGeometry,
		FSlateWindowElementList& OutDrawElements,
		int32 ChildMaxLayerId,
		const FWacomBackpackWorkspaceMarqueePaintView& View,
		float WidgetOpacity,
		bool bParentEnabled);

	static int32 PaintCardMarkers(
		const FGeometry& AllottedGeometry,
		FSlateWindowElementList& OutDrawElements,
		int32 ChildMaxLayerId,
		const FWacomBackpackCardOverlayPaintView& View,
		const FLinearColor& WidgetTint,
		bool bParentEnabled);

	/** Pure layer contract used by production paint and automation coverage. */
	static int32 ResolveMarqueeMaxLayer(int32 ChildMaxLayerId, bool bVisible, bool bHasArea);
	static int32 ResolveCardMarkerMaxLayer(
		int32 ChildMaxLayerId,
		bool bHasFocusBrush,
		bool bHasSemanticBrush);

	/** Resolve one marker center after the card's complete base/local visual pose. */
	static FVector2D ResolveCardMarkerCenter(
		FVector2D CardCenter,
		FVector2D CardSize,
		float CardAngleDegrees,
		bool bSemanticMarker,
		FVector2D IconSize = FVector2D(32.0f, 32.0f),
		float Padding = 10.0f);

	/** A centralized marker is suppressed when any visually higher card body overlaps it. */
	static bool IsMarkerOccludedByHigherCard(
		FVector2D MarkerCenter,
		FVector2D MarkerSize,
		float MarkerAngleDegrees,
		const FWacomBackpackOverlayVisualOrder& MarkerCardOrder,
		TConstArrayView<FWacomBackpackCardMarkerOccluder> CardBodies);
};
