// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Types/WacomEnums.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "WacomFirstPersonCardAnchorComponent.generated.h"

class APlayerController;
class AWacomPlayerCharacter;
class UCardDefinition;
class UWacomCardView;
class UWacomFirstPersonCardAnchorDebugWidget;
class UWacomFirstPersonCardLayerWidget;
struct FWacomFirstPersonCardLayerSlotView;

DECLARE_MULTICAST_DELEGATE_TwoParams(FWacomFirstPersonCardLayerAnchorInteractionNative, const FGuid&, const FWacomFirstPersonCardLayerSlotView&);

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
	FVector2D RawScreenPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D WidgetPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D SnappedWidgetPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D ScreenPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float ViewportScale = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bProjected = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bClamped = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bPixelSnapped = false;
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
struct WACOMAPP_API FWacomFirstPersonCardLayerEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FGuid CardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FWacomCardViewData CardViewData;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	EHandZone Zone = EHandZone::None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bIsHandAnchor = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bIsPlayable = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bIsPendingTargeting = false;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardLayerSlotView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	int32 Index = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FWacomFirstPersonCardLayerEntry Entry;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D ScreenPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D RawScreenPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D WidgetPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D SnappedWidgetPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float RenderAngleDegrees = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float RenderScale = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float RenderOpacity = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	int32 ZOrder = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float ViewportScale = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bProjected = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bClamped = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bPixelSnapped = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bIsHovered = false;
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Layout", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "600.0", Units = "cm", ToolTip = "第一人称锚点到虚拟手牌平面的距离，单位为 Unreal 厘米。"))
	float DistanceFromView = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Layout", meta = (UIMin = "-240.0", UIMax = "120.0", Units = "cm", ToolTip = "虚拟手牌平面相对第一人称锚点的垂直偏移，单位为 Unreal 厘米；负值会让卡牌在画面中更低。"))
	float VerticalOffset = -70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Layout", meta = (UIMin = "-240.0", UIMax = "240.0", Units = "cm", ToolTip = "虚拟手牌平面相对第一人称锚点的水平偏移，单位为 Unreal 厘米。"))
	float HorizontalOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Layout", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "120.0", Units = "cm", ToolTip = "投影前相邻虚拟卡牌槽之间的距离，单位为 Unreal 厘米。"))
	float CardSpacing = 36.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Layout", meta = (UIMin = "-30.0", UIMax = "30.0", Units = "deg", ToolTip = "每张卡牌相对第一人称手牌锚点增加的扇形偏航角，单位为度；角度越大，旋转锯齿风险越高。"))
	float FanYawDegrees = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Look", meta = (UIMin = "0.0", UIMax = "1.0", ToolTip = "共享鼠标镜头偏航偏移对卡牌锚点的影响比例；数值越低，卡牌越像跟随角色身体而不是镜头。"))
	float LookInfluenceYaw = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Look", meta = (UIMin = "0.0", UIMax = "1.0", ToolTip = "共享鼠标镜头俯仰偏移对卡牌锚点的影响比例；数值越低，卡牌越像跟随角色身体而不是镜头。"))
	float LookInfluencePitch = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Look", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "30.0", ToolTip = "第一人称卡牌锚点跟随目标位置和朝向的插值速度；设为 0 时立即贴合目标锚点。"))
	float FollowInterpSpeed = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Projection", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "200.0", ToolTip = "投影点被限制在视口内时保留的屏幕安全边距，单位为 UMG 布局像素。"))
	float ProjectionPadding = 24.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Projection", meta = (ToolTip = "是否在投影、边缘下坠、悬停上浮和等待选目标上浮后，把最终卡牌位置吸附到稳定网格；用于减少 UMG 旋转时的位置闪动。"))
	bool bEnableCardLayerPixelSnapping = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Projection", meta = (ClampMin = "0.01", UIMin = "0.25", UIMax = "8.0", ToolTip = "开启像素对齐时使用的 UMG 布局网格大小；1.0 表示吸附到整数 UMG 布局单位。"))
	float CardLayerPixelSnapGrid = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Projection", meta = (ToolTip = "是否限制第一人称卡牌层的 UMG 渲染旋转角；高对比卡面被整体旋转时容易出现锯齿。"))
	bool bClampCardLayerRenderAngle = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Projection", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "20.0", Units = "deg", ToolTip = "开启旋转限制时，每张第一人称卡牌允许的最大 UMG 渲染旋转角，单位为度。"))
	float MaxCardLayerRenderAngleDegrees = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Debug", meta = (ToolTip = "是否绘制 5 个非交互 HUD 调试点，用于验证第一人称卡牌锚点投影位置；仅开发调试使用，默认关闭。"))
	bool bDrawDebugProjection = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Debug", meta = (ClampMin = "0", UIMin = "0", UIMax = "20000", ToolTip = "第一人称卡牌锚点调试 Widget 的视口层级。"))
	int32 DebugWidgetZOrder = 9998;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Static Layer", meta = (ToolTip = "是否从第一人称卡牌锚点绘制非交互 HUD/UMG 静态预览卡牌层；仅原型验证使用，默认关闭。"))
	bool bDrawStaticCardLayer = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Static Layer", meta = (ToolTip = "非交互第一人称静态卡牌层使用的 Widget 类；为空时使用 C++ 默认层 Widget。"))
	TSubclassOf<UWacomFirstPersonCardLayerWidget> StaticCardLayerWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|View", meta = (ToolTip = "第一人称卡牌层使用的卡面 Widget 类；正式验证建议设置为 /Game/Wacom/UI/Card/WBP_FirstPersonCardView。为空时仅使用 UWacomCardView 作为测试兜底。"))
	TSubclassOf<UWacomCardView> FirstPersonCardViewClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Static Layer", meta = (ToolTip = "静态预览卡牌层使用的可选卡牌定义；为空时生成占位卡牌数据，便于 PIE 直接验证。"))
	TArray<TSoftObjectPtr<UCardDefinition>> StaticPreviewCardDefinitions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Static Layer", meta = (ClampMin = "0.01", UIMin = "0.1", UIMax = "2.0", ToolTip = "第一人称卡牌层中每张卡牌使用的 UMG 渲染缩放。"))
	float StaticCardRenderScale = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Static Layer", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "260.0", ToolTip = "最外侧卡牌额外下坠的屏幕距离，单位为 UMG 布局像素；越靠近中心的卡牌下坠越少，用于形成手牌弧线。"))
	float StaticCardEdgeDropPixels = 72.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Static Layer", meta = (ClampMin = "0", UIMin = "0", UIMax = "20000", ToolTip = "第一人称静态卡牌层 Widget 的视口层级。"))
	int32 StaticCardLayerZOrder = 9996;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Static Layer", meta = (ClampMin = "0", UIMin = "0", UIMax = "12", ToolTip = "静态预览卡牌定义为空时绘制的占位卡牌数量。"))
	int32 StaticCardCountFallback = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Visual States", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "160.0", ToolTip = "正在等待目标选择的卡牌额外上浮距离，单位为 UMG 布局像素。"))
	float PendingTargetingLiftPixels = 36.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Visual States", meta = (ClampMin = "0.01", UIMin = "0.5", UIMax = "1.5", ToolTip = "正在等待目标选择的卡牌额外渲染缩放倍率。"))
	float PendingTargetingScale = 1.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Visual States", meta = (ClampMin = "0.01", UIMin = "0.5", UIMax = "1.5", ToolTip = "手牌锚点卡牌使用的渲染缩放倍率。"))
	float HandAnchorScale = 0.96f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Visual States", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "不可用卡牌在第一人称卡牌层上的整体透明度；卡面自身的 disabled overlay 仍由 FWacomCardViewData::bDisabled 控制。"))
	float DisabledRenderOpacity = 0.78f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Interaction Prototype", meta = (ToolTip = "是否启用第一人称战斗手牌层的 hover/click 处理；原型开关，战斗中由 BattleHUD 控制，默认关闭。"))
	bool bEnableBattleHandInteractionPrototype = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Interaction Prototype", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "160.0", ToolTip = "鼠标悬停的第一人称卡牌槽额外上浮距离，单位为 UMG 布局像素。"))
	float HoverLiftPixels = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Interaction Prototype", meta = (ClampMin = "0.01", UIMin = "0.5", UIMax = "1.5", ToolTip = "鼠标悬停的第一人称卡牌槽额外渲染缩放倍率。"))
	float HoverScale = 1.06f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Interaction Prototype", meta = (ClampMin = "0", UIMin = "0", UIMax = "5000", ToolTip = "鼠标悬停的第一人称卡牌槽额外增加的 ZOrder 层级。"))
	int32 HoverZOrderBoost = 500;

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
	TArray<FWacomFirstPersonCardLayerSlotView> BuildStaticCardSlotViews() const;

	TArray<FWacomFirstPersonCardLayerSlotView> BuildActiveCardLayerSlotViews() const;

	void SetRuntimeCardLayerEntries(FName SourceId, const TArray<FWacomFirstPersonCardLayerEntry>& Entries);
	void SetRuntimeCardLayerData(FName SourceId, const TArray<FWacomCardViewData>& Cards);
	void ClearRuntimeCardLayerData(FName SourceId);

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	bool HasRuntimeCardLayerData() const { return bHasRuntimeCardLayerData; }

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	FName GetRuntimeCardLayerSourceId() const { return RuntimeCardLayerSourceId; }

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	int32 GetRuntimeCardLayerCardCount() const { return RuntimeCardLayerEntries.Num(); }

	const TArray<FWacomCardViewData>& GetRuntimeCardLayerData() const { return RuntimeCardLayerData; }
	const TArray<FWacomFirstPersonCardLayerEntry>& GetRuntimeCardLayerEntries() const { return RuntimeCardLayerEntries; }

	void SetBattleHandInteractionPrototypeEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	bool IsStaticCardLayerWidgetActive() const { return StaticCardLayerWidget != nullptr; }

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	bool IsBattleHandInteractionPrototypeEnabled() const { return bEnableBattleHandInteractionPrototype; }

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	FGuid GetHoveredCardInstanceId() const { return HoveredCardInstanceId; }

	FWacomFirstPersonCardLayerAnchorInteractionNative OnFirstPersonCardLayerCardClicked;
	FWacomFirstPersonCardLayerAnchorInteractionNative OnFirstPersonCardLayerCardHovered;
	FWacomFirstPersonCardLayerAnchorInteractionNative OnFirstPersonCardLayerCardUnhovered;
	FWacomFirstPersonCardLayerAnchorInteractionNative OnFirstPersonCardLayerHoveredCardLayoutUpdated;

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	FString GetDebugSummary() const;

#if WITH_AUTOMATION_TESTS
	UWacomFirstPersonCardLayerWidget* GetStaticCardLayerWidgetForTest() const { return StaticCardLayerWidget; }
	void SetHoveredCardInstanceIdForTest(const FGuid& CardInstanceId) { HoveredCardInstanceId = CardInstanceId; }
#endif

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual bool ResolveCameraTransformForAnchor(FTransform& OutCameraTransform) const;
	virtual bool ProjectWorldLocationForAnchor(const FVector& WorldLocation, FVector2D& OutScreenPosition) const;
	virtual bool ProjectWorldLocationToWidgetPositionForAnchor(
		const FVector& WorldLocation,
		FVector2D& OutWidgetPosition,
		FVector2D& OutRawScreenPosition) const;
	virtual bool GetViewportSizeForAnchor(FVector2D& OutViewportSize) const;
	virtual float GetViewportScaleForAnchor() const;
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

	UPROPERTY(Transient)
	TArray<FWacomCardViewData> RuntimeCardLayerData;

	UPROPERTY(Transient)
	TArray<FWacomFirstPersonCardLayerEntry> RuntimeCardLayerEntries;

	bool bHasRuntimeCardLayerData = false;
	FName RuntimeCardLayerSourceId = NAME_None;
	FGuid HoveredCardInstanceId;

	AWacomPlayerCharacter* GetOwnerCharacter() const;
	APlayerController* GetOwnerPlayerController() const;
	bool ResolveBaseAnchor(FTransform& OutBaseTransform, EWacomFirstPersonCardAnchorMode& OutMode, FName& OutFallbackReason) const;
	FWacomCardViewData BuildStaticCardViewData(int32 CardIndex) const;
	TArray<FWacomFirstPersonCardLayerEntry> BuildStaticCardLayerEntries() const;
	FVector2D SnapCardLayerPosition(FVector2D Position, bool& bOutPixelSnapped) const;
	float ClampCardLayerRenderAngle(float AngleDegrees) const;
	static TArray<FWacomFirstPersonCardLayerEntry> BuildCardLayerEntriesFromData(
		const TArray<FWacomCardViewData>& CardData);
	TArray<FWacomFirstPersonCardLayerSlotView> BuildCardSlotViewsFromEntries(
		const TArray<FWacomFirstPersonCardLayerEntry>& CardEntries) const;
	void UpdateDebugWidget();
	void RemoveDebugWidget();
	void RemoveStaticCardLayer();
	void BindStaticCardLayerWidget(UWacomFirstPersonCardLayerWidget* LayerWidget);
	void UnbindStaticCardLayerWidget(UWacomFirstPersonCardLayerWidget* LayerWidget);
	void HandleLayerCardClicked(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleLayerCardHovered(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleLayerCardUnhovered(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleLayerHoveredCardSlotUpdated(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	static FString AnchorModeToString(EWacomFirstPersonCardAnchorMode Mode);
};
