// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "WacomFirstPersonCardAnchorComponent.generated.h"

class APlayerController;
class AWacomPlayerCharacter;
class UCardDefinition;
class UWacomCardView;
class UWacomFirstPersonCardAnchorDebugWidget;
class UWacomFirstPersonCardLayerWidget;

UENUM(BlueprintType)
enum class EWacomFirstPersonCardAnchorMode : uint8
{
	Invalid,
	BattleCamera,
	RunTunnel,
	CameraFallback
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardProjectedPoint
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	int32 Index = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D ScreenPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bProjected = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bClamped = false;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardAnchorDebugView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bHasValidAnchor = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	EWacomFirstPersonCardAnchorMode Mode = EWacomFirstPersonCardAnchorMode::Invalid;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FTransform AnchorTransform = FTransform::Identity;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FRotator LookOffsetUsed = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	TArray<FWacomFirstPersonCardProjectedPoint> ProjectedPoints;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FName LastFallbackReason = NAME_None;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonStaticCardSlotView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	int32 Index = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FWacomCardViewData CardViewData;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D ScreenPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float RenderAngleDegrees = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float RenderScale = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bProjected = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bClamped = false;
};

/**
 * Computes the first-person virtual card hand anchor used by future HUD-rendered
 * cards. V0-B can draw a non-interactive static card layer for PIE validation.
 */
UCLASS(ClassGroup = (Wacom), meta = (BlueprintSpawnableComponent))
class WACOMAPP_API UWacomFirstPersonCardAnchorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWacomFirstPersonCardAnchorComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Layout", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "600.0", Units = "cm", ToolTip = "Distance from the selected first-person anchor to the virtual card hand plane, in Unreal centimeters."))
	float DistanceFromView = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Layout", meta = (UIMin = "-240.0", UIMax = "120.0", Units = "cm", ToolTip = "Vertical offset from the selected first-person anchor to the virtual card hand plane, in Unreal centimeters. Negative values place the cards lower in view."))
	float VerticalOffset = -70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Layout", meta = (UIMin = "-240.0", UIMax = "240.0", Units = "cm", ToolTip = "Horizontal offset from the selected first-person anchor to the virtual card hand plane, in Unreal centimeters."))
	float HorizontalOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Layout", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "120.0", Units = "cm", ToolTip = "Distance between adjacent virtual card slots before projection, in Unreal centimeters."))
	float CardSpacing = 36.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Layout", meta = (UIMin = "-30.0", UIMax = "30.0", Units = "deg", ToolTip = "Per-card fan yaw around the first-person hand anchor."))
	float FanYawDegrees = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Look", meta = (UIMin = "0.0", UIMax = "1.0", ToolTip = "Fraction of shared cursor yaw offset applied to the card anchor. Lower values make cards follow the character/body more than the camera."))
	float LookInfluenceYaw = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Look", meta = (UIMin = "0.0", UIMax = "1.0", ToolTip = "Fraction of shared cursor pitch offset applied to the card anchor. Lower values make cards follow the character/body more than the camera."))
	float LookInfluencePitch = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Look", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "30.0", ToolTip = "Interpolation speed in inverse seconds used by the first-person card anchor. Set to 0 to snap to the resolved anchor."))
	float FollowInterpSpeed = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Projection", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "200.0", ToolTip = "Screen-space padding, in pixels, used when clamping projected debug card points to the viewport."))
	float ProjectionPadding = 24.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Debug", meta = (ToolTip = "Draws five non-interactive HUD debug points for the first-person card anchor projection. Development-only visual aid; off by default."))
	bool bDrawDebugProjection = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Debug", meta = (ClampMin = "0", UIMin = "0", UIMax = "20000", ToolTip = "Viewport z-order used for the first-person card anchor debug widget."))
	int32 DebugWidgetZOrder = 9998;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Static Layer", meta = (ToolTip = "Draws a non-interactive HUD/UMG static card layer from the first-person card anchor. Prototype validation only; off by default."))
	bool bDrawStaticCardLayer = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Static Layer", meta = (ToolTip = "Widget class used for the non-interactive first-person static card layer. Empty uses the C++ default layer widget."))
	TSubclassOf<UWacomFirstPersonCardLayerWidget> StaticCardLayerWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Static Layer", meta = (ToolTip = "Card view class used by the non-interactive static card layer. Empty uses UWacomCardView."))
	TSubclassOf<UWacomCardView> StaticCardViewClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Static Layer", meta = (ToolTip = "Optional card definitions used by the static card layer. If empty, placeholder card view data is generated so PIE can validate the layer without data asset setup."))
	TArray<TSoftObjectPtr<UCardDefinition>> StaticPreviewCardDefinitions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Static Layer", meta = (ClampMin = "0.01", UIMin = "0.1", UIMax = "2.0", ToolTip = "UMG render scale applied to every static card in the first-person card layer."))
	float StaticCardRenderScale = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Static Layer", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "260.0", ToolTip = "Screen-space vertical drop, in pixels, applied to the outermost static cards. Cards near the center stay higher, producing a fan arc."))
	float StaticCardEdgeDropPixels = 72.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Static Layer", meta = (ClampMin = "0", UIMin = "0", UIMax = "20000", ToolTip = "Viewport z-order used for the first-person static card layer widget."))
	int32 StaticCardLayerZOrder = 9996;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Static Layer", meta = (ClampMin = "0", UIMin = "0", UIMax = "12", ToolTip = "Number of placeholder cards drawn when StaticPreviewCardDefinitions is empty."))
	int32 StaticCardCountFallback = 5;

	UFUNCTION(BlueprintCallable, Category = "Wacom|First Person Card Layer")
	void RefreshAnchor(float DeltaTime);

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	FTransform GetCurrentAnchorTransform() const { return CurrentAnchorTransform; }

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	FTransform ComputeCardTransform(int32 NumCards, int32 CardIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|First Person Card Layer")
	bool ProjectCardTransformToScreen(
		const FTransform& CardTransform,
		FWacomFirstPersonCardProjectedPoint& OutProjectedPoint,
		int32 PointIndex = -1) const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|First Person Card Layer")
	FWacomFirstPersonCardAnchorDebugView GetFirstPersonCardAnchorDebugView(int32 NumDebugCards = 5) const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|First Person Card Layer")
	TArray<FWacomFirstPersonStaticCardSlotView> BuildStaticCardSlotViews() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	bool IsStaticCardLayerWidgetActive() const { return StaticCardLayerWidget != nullptr; }

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	FString GetDebugSummary() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual bool ResolveCameraTransformForAnchor(FTransform& OutCameraTransform) const;
	virtual bool ProjectWorldLocationForAnchor(const FVector& WorldLocation, FVector2D& OutScreenPosition) const;
	virtual bool GetViewportSizeForAnchor(FVector2D& OutViewportSize) const;
	virtual bool CanCreateStaticCardLayerForAnchor(APlayerController* PlayerController) const;
	virtual UWacomFirstPersonCardLayerWidget* CreateStaticCardLayerWidgetForAnchor(
		APlayerController* PlayerController,
		TSubclassOf<UWacomFirstPersonCardLayerWidget> LayerClass) const;
	virtual void AddStaticCardLayerWidgetToViewportForAnchor(
		UWacomFirstPersonCardLayerWidget* LayerWidget,
		int32 ZOrder) const;
	void UpdateStaticCardLayer();

private:
	UPROPERTY(Transient)
	TObjectPtr<UWacomFirstPersonCardAnchorDebugWidget> DebugWidget;

	UPROPERTY(Transient)
	TObjectPtr<UWacomFirstPersonCardLayerWidget> StaticCardLayerWidget;

	FTransform CurrentAnchorTransform = FTransform::Identity;
	EWacomFirstPersonCardAnchorMode CurrentMode = EWacomFirstPersonCardAnchorMode::Invalid;
	FRotator CurrentLookOffsetUsed = FRotator::ZeroRotator;
	FName LastFallbackReason = NAME_None;
	bool bHasValidAnchor = false;
	bool bHasInitializedAnchor = false;

	AWacomPlayerCharacter* GetOwnerCharacter() const;
	APlayerController* GetOwnerPlayerController() const;
	bool ResolveBaseAnchor(FTransform& OutBaseTransform, EWacomFirstPersonCardAnchorMode& OutMode, FName& OutFallbackReason) const;
	FWacomCardViewData BuildStaticCardViewData(int32 CardIndex) const;
	void UpdateDebugWidget();
	void RemoveDebugWidget();
	void RemoveStaticCardLayer();
	static FString AnchorModeToString(EWacomFirstPersonCardAnchorMode Mode);
};
