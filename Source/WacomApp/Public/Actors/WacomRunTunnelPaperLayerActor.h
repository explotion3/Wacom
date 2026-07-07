// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WacomRunTunnelPaperLayerActor.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMeshComponent;
class UTexture2D;

/**
 * Reusable visual-only paper layer for Run Tunnel scene authoring.
 *
 * Owns a plane mesh and assigns one texture from an authored list to a dynamic
 * material instance. It does not handle movement, interaction, or Run rules.
 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomRunTunnelPaperLayerActor : public AActor
{
	GENERATED_BODY()

public:
	AWacomRunTunnelPaperLayerActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "Wacom|Run Tunnel|Paper Layer")
	UStaticMeshComponent* GetPaperPlaneComponent() const { return PaperPlaneComponent; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Run Tunnel|Paper Layer")
	UMaterialInstanceDynamic* GetDynamicPaperMaterial() const { return DynamicPaperMaterial; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Run Tunnel|Paper Layer")
	int32 GetLastAppliedTextureIndex() const { return LastAppliedTextureIndex; }

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Wacom|Run Tunnel|Paper Layer",
		meta = (ToolTip = "重新应用纸片材质并按当前设置选择贴图。用于编辑器里调整材质、贴图数组或索引后手动刷新预览。"))
	void RefreshPaperLayerMaterial();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Wacom|Run Tunnel|Paper Layer",
		meta = (ToolTip = "增加随机种子并重新选择一张贴图。FixedTextureIndex >= 0 时不会改变固定选择。"))
	void RerollTextureSelection();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Paper Layer|Material",
		meta = (ToolTip = "纸片使用的材质模板。材质中应包含 Texture2D 参数，默认参数名为 PaperTexture；为空时使用 Plane 组件当前材质。"))
	TObjectPtr<UMaterialInterface> PaperMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Paper Layer|Material",
		meta = (ToolTip = "材质里的 Texture2D 参数名。默认 PaperTexture，需要和材质中的 TextureSampleParameter2D 参数名一致。"))
	FName TextureParameterName = TEXT("PaperTexture");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Paper Layer|Textures",
		meta = (ToolTip = "可随机选择的纸片贴图列表。例如 6 张黑底草地图片；空项会被跳过。"))
	TArray<TObjectPtr<UTexture2D>> PaperTextures;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Paper Layer|Textures",
		meta = (ClampMin = "-1", ToolTip = "固定贴图索引。-1 表示按随机种子自动选择；0 或更大时使用 PaperTextures 中对应下标。"))
	int32 FixedTextureIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Paper Layer|Textures",
		meta = (ToolTip = "随机选择贴图使用的基础种子。点击 RerollTextureSelection 会递增该值。"))
	int32 TextureSelectionSeed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Paper Layer|Textures",
		meta = (ToolTip = "开启后把 Actor 名字混入随机种子。复制多个纸片 Actor 时通常会得到不同贴图，同时保持编辑器预览稳定。"))
	bool bSaltRandomWithActorName = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run Tunnel|Paper Layer|Material",
		meta = (ToolTip = "开启后在 Construction Script / OnConstruction 阶段自动创建动态材质并应用贴图，方便编辑器摆放时立即预览。"))
	bool bApplyMaterialOnConstruction = true;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run Tunnel|Paper Layer",
		meta = (AllowPrivateAccess = "true", ToolTip = "纸片显示用 Plane 静态网格体组件。默认使用 Engine BasicShapes Plane，蓝图子类可替换。"))
	TObjectPtr<UStaticMeshComponent> PaperPlaneComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Run Tunnel|Paper Layer|Debug",
		meta = (AllowPrivateAccess = "true", ToolTip = "上一次成功应用的贴图索引；-1 表示没有可用贴图或没有设置贴图参数。"))
	int32 LastAppliedTextureIndex = INDEX_NONE;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicPaperMaterial = nullptr;

	int32 ResolveTextureIndex() const;
	int32 BuildStableRandomSeed() const;
};
