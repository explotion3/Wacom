// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WacomBattleEnemyPartVisualLayerTypes.generated.h"

class UPaperFlipbook;
class UPaperSprite;
class UMaterialInterface;

UENUM(BlueprintType)
enum class EWacomBattleEnemyPartVisualLayerMode : uint8
{
	StaticSprite UMETA(DisplayName = "Static Sprite"),
	Flipbook UMETA(DisplayName = "Flipbook"),
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattleEnemyPartVisualLayer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual Layers",
		meta = (ToolTip = "部位视觉层稳定 ID，例如 Head.Main、Body.Shadow。用于 debug / validation，不影响战斗规则。"))
	FName LayerId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual Layers",
		meta = (ToolTip = "视觉层类型。StaticSprite 使用单张 PaperSprite；Flipbook 使用 PaperFlipbook 播放序列帧。只影响表现，不影响命中或战斗规则。"))
	EWacomBattleEnemyPartVisualLayerMode LayerMode = EWacomBattleEnemyPartVisualLayerMode::StaticSprite;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual Layers",
		meta = (ToolTip = "StaticSprite 层使用的 PaperSprite。LayerMode=StaticSprite 且留空时不生成组件，但会进入 debug / validation。"))
	TObjectPtr<UPaperSprite> Sprite = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual Layers",
		meta = (ToolTip = "Flipbook 层使用的 PaperFlipbook。LayerMode=Flipbook 且留空时不生成组件，但会进入 debug / validation。"))
	TObjectPtr<UPaperFlipbook> Flipbook = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Destroyed",
		meta = (ToolTip = "StaticSprite 层进入部位破坏终态时原地替换的 PaperSprite。可留空；留空时该层继续保持原资源。"))
	TObjectPtr<UPaperSprite> DestroyedSprite = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Destroyed",
		meta = (ToolTip = "Flipbook 层进入部位破坏终态时原地替换的 PaperFlipbook。可留空；留空时该层继续保持原资源。"))
	TObjectPtr<UPaperFlipbook> DestroyedFlipbook = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Destroyed",
		meta = (ToolTip = "破损 Flipbook 的播放倍率。必须为有限正数；非循环播放结束后保持末帧。", ClampMin = "0.001"))
	float DestroyedFlipbookPlayRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Flipbook",
		meta = (ToolTip = "Flipbook 层播放倍率。只影响视觉播放速度；1 表示原速。", ClampMin = "0.0", ClampMax = "8.0", UIMin = "0.0", UIMax = "3.0"))
	float FlipbookPlayRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Flipbook",
		meta = (ToolTip = "Flipbook 层是否循环播放。只影响视觉表现。"))
	bool bLoopFlipbook = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Flipbook",
		meta = (ToolTip = "Flipbook 层初始播放时间，单位秒。用于让多层动画错帧。", ClampMin = "0.0", UIMin = "0.0"))
	float FlipbookStartTimeSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Flipbook",
		meta = (ToolTip = "Flipbook 层是否在生成后立即播放。关闭时停在初始播放时间。"))
	bool bAutoPlayFlipbook = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Transform",
		meta = (ToolTip = "视觉层相对 VisualLayersRoot 的位置。单位：厘米；只影响显示，不影响 HitBounds。"))
	FVector RelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Transform",
		meta = (ToolTip = "视觉层相对 VisualLayersRoot 的旋转。只影响显示，不影响 HitBounds。"))
	FRotator RelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Transform",
		meta = (ToolTip = "视觉层相对缩放。任一轴不能为 0；只影响显示，不影响 HitBounds。"))
	FVector RelativeScale3D = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Rendering",
		meta = (ToolTip = "视觉层半透明排序优先级。数值越大越靠前。", UIMin = "-100", UIMax = "100"))
	int32 SortOrder = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Rendering",
		meta = (ToolTip = "视觉层颜色和透明度。Alpha 会作为 sprite 透明度。"))
	FLinearColor Tint = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Rendering",
		meta = (ToolTip = "该视觉层 PaperSprite / PaperFlipbook 的材质覆盖。需要投射阴影时可指定 Paper2D 的 MaskedLitSpriteMaterial 或等效 lit masked 材质。"))
	TObjectPtr<UMaterialInterface> MaterialOverride = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Rendering",
		meta = (ToolTip = "该视觉层是否投射阴影。需要材质支持光照/Masked，并确保场景光源开启阴影。"))
	bool bCastShadow = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Rendering",
		meta = (ToolTip = "是否显示该视觉层。关闭时仍保留配置，但生成组件默认隐藏。"))
	bool bVisible = true;
};
