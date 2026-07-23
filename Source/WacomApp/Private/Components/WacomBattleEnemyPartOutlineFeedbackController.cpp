// Copyright Wacom. All Rights Reserved.

#include "Components/WacomBattleEnemyPartOutlineFeedbackController.h"

#include "Actors/WacomBattleEnemyPartTargetPreviewStyle.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/ActorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "Slate/SlateTextureAtlasInterface.h"

namespace
{
	const FName SpriteTextureParameter(TEXT("SpriteTexture"));
	const FName OutlineAtlasUVOriginParameter(TEXT("OutlineAtlasUVOrigin"));
	const FName OutlineAtlasUVAxisXParameter(TEXT("OutlineAtlasUVAxisX"));
	const FName OutlineAtlasUVAxisYParameter(TEXT("OutlineAtlasUVAxisY"));
	const FName OutlineSourceInvPixelSizeParameter(TEXT("OutlineSourceInvPixelSize"));
	const FName OutlineCanvasToSourceScaleParameter(TEXT("OutlineCanvasToSourceScale"));
	const FName OutlineColorParameter(TEXT("OutlineColor"));
	const FName OutlineOpacityParameter(TEXT("OutlineOpacity"));
	const FName OutlineThicknessPixelsParameter(TEXT("OutlineThicknessPixels"));
	const FName OutlineOuterColorMultiplierParameter(TEXT("OutlineOuterColorMultiplier"));
	const FName OutlineOuterAlphaMultiplierParameter(TEXT("OutlineOuterAlphaMultiplier"));
	constexpr float OutlineDepthBiasCentimeters = 0.25f;
	constexpr float OutlineMaximumSourcePixels = 2.0f;
	constexpr float BasicPlaneSizeUnrealUnits = 100.0f;

	void ConfigureProxyPrimitive(UStaticMeshComponent& Component)
	{
		Component.SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component.SetGenerateOverlapEvents(false);
		Component.SetCastShadow(false);
		Component.SetReceivesDecals(false);
		Component.SetRenderCustomDepth(false);
		Component.SetCanEverAffectNavigation(false);
		Component.SetVisibility(false, true);
	}

	FVector ResolveOutlineDepthBiasInSourceSpace(const UPrimitiveComponent& Source)
	{
		const UWorld* World = Source.GetWorld();
		const APlayerController* PlayerController = World
			? World->GetFirstPlayerController()
			: nullptr;
		const APlayerCameraManager* CameraManager = PlayerController
			? PlayerController->PlayerCameraManager
			: nullptr;
		if (!CameraManager)
		{
			return FVector::ZeroVector;
		}

		const FVector PlaneNormal = Source.GetComponentTransform()
			.TransformVectorNoScale(FVector::YAxisVector)
			.GetSafeNormal();
		const FVector ToCamera = CameraManager->GetCameraLocation()
			- Source.GetComponentLocation();
		const FVector AwayFromCamera = FVector::DotProduct(PlaneNormal, ToCamera) >= 0.0f
			? -PlaneNormal
			: PlaneNormal;
		return Source.GetComponentTransform().InverseTransformVectorNoScale(
			AwayFromCamera * OutlineDepthBiasCentimeters);
	}

	FLinearColor PackVector2(const FVector2D& Value)
	{
		return FLinearColor(Value.X, Value.Y, 0.0f, 0.0f);
	}
}

FName FWacomBattleEnemyPartOutlineFeedbackController::StateToName(
	EWacomBattleEnemyPartOutlineState State)
{
	switch (State)
	{
	case EWacomBattleEnemyPartOutlineState::Selectable: return TEXT("Selectable");
	case EWacomBattleEnemyPartOutlineState::Hovered: return TEXT("Hovered");
	default: return TEXT("None");
	}
}

void FWacomBattleEnemyPartOutlineFeedbackController::BeginOrUpdate(
	UActorComponent& InLifetimeOwner,
	UPrimitiveComponent* InSourceVisual,
	const UWacomBattleEnemyPartTargetPreviewStyle* Style,
	EWacomBattleEnemyPartOutlineState State)
{
	LifetimeOwner = &InLifetimeOwner;
	SourceVisual = InSourceVisual;
	ActiveStyle = Style;
	ActiveState = State;
	DebugView.State = StateToName(State);
	if (State == EWacomBattleEnemyPartOutlineState::None
		|| !IsValid(InSourceVisual)
		|| !IsValid(Style)
		|| !Style->HasValidOutlineAsset())
	{
		ResetImmediate(false);
		return;
	}

	if (UStaticMeshComponent* OutlineProxy = ResolveOrCreateProxy(InLifetimeOwner))
	{
		if (!DynamicMaterial.IsValid()
			|| DynamicMaterial->Parent != Style->OutlineMaterial)
		{
			UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(
				Style->OutlineMaterial,
				OutlineProxy);
			DynamicMaterial = Material;
			OutlineProxy->SetMaterial(0, Material);
		}
		SyncProxy();
	}
}

bool FWacomBattleEnemyPartOutlineFeedbackController::Tick()
{
	if (ActiveState == EWacomBattleEnemyPartOutlineState::None)
	{
		return false;
	}
	SyncProxy();
	return DebugView.bVisible;
}

void FWacomBattleEnemyPartOutlineFeedbackController::ResetImmediate(bool bDestroyComponent)
{
	ActiveState = EWacomBattleEnemyPartOutlineState::None;
	ActiveStyle.Reset();
	SourceVisual.Reset();
	LifetimeOwner.Reset();
	DebugView.State = TEXT("None");
	DebugView.bVisible = false;
	if (UStaticMeshComponent* OutlineProxy = Proxy.Get())
	{
		OutlineProxy->SetVisibility(false, true);
	}
	if (bDestroyComponent)
	{
		DestroyProxy();
	}
}

UStaticMeshComponent* FWacomBattleEnemyPartOutlineFeedbackController::ResolveOrCreateProxy(
	UActorComponent& InLifetimeOwner)
{
	UPrimitiveComponent* Source = SourceVisual.Get();
	AActor* Host = InLifetimeOwner.GetOwner();
	if (!Source || !Host
		|| (!Cast<UPaperSpriteComponent>(Source)
			&& !Cast<UPaperFlipbookComponent>(Source)))
	{
		return nullptr;
	}

	if (!Proxy.IsValid())
	{
		UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(
			nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
		if (!PlaneMesh)
		{
			return nullptr;
		}
		UStaticMeshComponent* NewProxy = NewObject<UStaticMeshComponent>(
			Host,
			MakeUniqueObjectName(
				Host, UStaticMeshComponent::StaticClass(), TEXT("EnemyPartOutlineProxy")),
			RF_Transient);
		Host->AddInstanceComponent(NewProxy);
		NewProxy->CreationMethod = EComponentCreationMethod::Instance;
		NewProxy->SetupAttachment(Source);
		NewProxy->SetStaticMesh(PlaneMesh);
		ConfigureProxyPrimitive(*NewProxy);
		NewProxy->RegisterComponent();
		Proxy = NewProxy;
		++DebugView.ComponentCreateCount;
	}

	if (Proxy->GetAttachParent() != Source)
	{
		Proxy->AttachToComponent(Source, FAttachmentTransformRules::KeepRelativeTransform);
		CachedSprite.Reset();
		CachedFrame = FWacomBattleEnemyPartOutlineProxyFrame();
	}
	DebugView.bComponentCreated = true;
	return Proxy.Get();
}

void FWacomBattleEnemyPartOutlineFeedbackController::DestroyProxy()
{
	if (UStaticMeshComponent* OutlineProxy = Proxy.Get())
	{
		OutlineProxy->DestroyComponent();
	}
	Proxy.Reset();
	CachedSprite.Reset();
	CachedFrame = FWacomBattleEnemyPartOutlineProxyFrame();
	DynamicMaterial.Reset();
	DebugView.bComponentCreated = false;
	DebugView.bVisible = false;
}

UPaperSprite* FWacomBattleEnemyPartOutlineFeedbackController::ResolveCurrentSprite() const
{
	if (UPaperSpriteComponent* SpriteSource = Cast<UPaperSpriteComponent>(SourceVisual.Get()))
	{
		return SpriteSource->GetSprite();
	}
	if (UPaperFlipbookComponent* FlipbookSource = Cast<UPaperFlipbookComponent>(SourceVisual.Get()))
	{
		const UPaperFlipbook* Flipbook = FlipbookSource->GetFlipbook();
		return Flipbook
			? Flipbook->GetSpriteAtTime(FlipbookSource->GetPlaybackPosition(), true)
			: nullptr;
	}
	return nullptr;
}

bool FWacomBattleEnemyPartOutlineFeedbackController::SyncProxyFrame(UPaperSprite& Sprite)
{
	UStaticMeshComponent* OutlineProxy = Proxy.Get();
	UMaterialInstanceDynamic* Material = DynamicMaterial.Get();
	if (!OutlineProxy || !Material)
	{
		return false;
	}
	if (CachedSprite.Get() == &Sprite && CachedFrame.IsValid())
	{
		return true;
	}

	const FSlateAtlasData AtlasData = Sprite.GetSlateAtlasData();
	UTexture* AtlasTexture = AtlasData.AtlasTexture;
	const float PixelsPerUnrealUnit = Sprite.GetPixelsPerUnrealUnit();
	const UWacomBattleEnemyPartTargetPreviewStyle* Style = ActiveStyle.Get();
	const float PaddingSourcePixels = Style
		? FMath::Clamp(FMath::Max(
			Style->SelectableOutlineThicknessSourcePixels,
			Style->HoveredOutlineThicknessSourcePixels),
			0.0f,
			OutlineMaximumSourcePixels)
		: 2.0f;
	FWacomBattleEnemyPartOutlineProxyFrame Frame;
	if (!AtlasTexture
		|| PixelsPerUnrealUnit <= UE_SMALL_NUMBER
		|| !WacomBattleEnemyPartOutlineProxyGeometry::BuildFrame(
			Sprite.BakedRenderData,
			AtlasData.StartUV,
			AtlasData.SizeUV,
			FVector2D(
				FMath::Max(1, AtlasTexture->GetSurfaceWidth()),
				FMath::Max(1, AtlasTexture->GetSurfaceHeight())),
			1.0f / PixelsPerUnrealUnit,
			FMath::Max(0.0f, PaddingSourcePixels),
			Frame))
	{
		CachedSprite.Reset();
		CachedFrame = FWacomBattleEnemyPartOutlineProxyFrame();
		return false;
	}

	CachedSprite = &Sprite;
	CachedFrame = Frame;
	Material->SetTextureParameterValue(SpriteTextureParameter, AtlasTexture);
	Material->SetVectorParameterValue(
		OutlineAtlasUVOriginParameter, PackVector2(Frame.AtlasUVOrigin));
	Material->SetVectorParameterValue(
		OutlineAtlasUVAxisXParameter, PackVector2(Frame.AtlasUVAxisX));
	Material->SetVectorParameterValue(
		OutlineAtlasUVAxisYParameter, PackVector2(Frame.AtlasUVAxisY));
	Material->SetVectorParameterValue(
		OutlineSourceInvPixelSizeParameter,
		PackVector2(FVector2D(
			1.0 / Frame.SourcePixelSize.X,
			1.0 / Frame.SourcePixelSize.Y)));
	Material->SetVectorParameterValue(
		OutlineCanvasToSourceScaleParameter,
		PackVector2(Frame.CanvasToSourceScale));
	return true;
}

void FWacomBattleEnemyPartOutlineFeedbackController::SyncProxy()
{
	UPrimitiveComponent* Source = SourceVisual.Get();
	UStaticMeshComponent* OutlineProxy = Proxy.Get();
	const UWacomBattleEnemyPartTargetPreviewStyle* Style = ActiveStyle.Get();
	UPaperSprite* CurrentSprite = ResolveCurrentSprite();
	if (!Source || !OutlineProxy || !Style || !Style->HasValidOutlineAsset()
		|| !CurrentSprite || !SyncProxyFrame(*CurrentSprite))
	{
		if (OutlineProxy)
		{
			OutlineProxy->SetVisibility(false, true);
		}
		DebugView.bVisible = false;
		return;
	}

	const bool bHovered = ActiveState == EWacomBattleEnemyPartOutlineState::Hovered;
	const FLinearColor Color = bHovered
		? Style->HoveredOutlineColor
		: Style->SelectableOutlineColor;
	const float Alpha = FMath::Clamp(
		bHovered ? Style->HoveredOutlineAlpha : Style->SelectableOutlineAlpha,
		0.0f,
		1.0f);
	const float ThicknessPixels = FMath::Clamp(
		bHovered
			? Style->HoveredOutlineThicknessSourcePixels
			: Style->SelectableOutlineThicknessSourcePixels,
		0.0f,
		OutlineMaximumSourcePixels);
	if (UMaterialInstanceDynamic* Material = DynamicMaterial.Get())
	{
		Material->SetVectorParameterValue(OutlineColorParameter, Color);
		Material->SetScalarParameterValue(OutlineOpacityParameter, Alpha);
		Material->SetScalarParameterValue(
			OutlineThicknessPixelsParameter, ThicknessPixels);
		Material->SetScalarParameterValue(
			OutlineOuterColorMultiplierParameter,
			FMath::Max(0.0f, Style->OutlineOuterColorMultiplier));
		Material->SetScalarParameterValue(
			OutlineOuterAlphaMultiplierParameter,
			FMath::Clamp(Style->OutlineOuterAlphaMultiplier, 0.0f, 1.0f));
	}

	const FVector DepthBias = ResolveOutlineDepthBiasInSourceSpace(*Source);
	OutlineProxy->SetRelativeLocation(
		FVector(CachedFrame.LocalCenter.X, 0.0, CachedFrame.LocalCenter.Y)
		+ DepthBias);
	OutlineProxy->SetRelativeRotation(
		WacomBattleEnemyPartOutlineProxyGeometry::ResolvePlaneToSpriteRotation());
	OutlineProxy->SetRelativeScale3D(FVector(
		CachedFrame.PaddedSizeUnrealUnits.X / BasicPlaneSizeUnrealUnits,
		CachedFrame.PaddedSizeUnrealUnits.Y / BasicPlaneSizeUnrealUnits,
		1.0));
	OutlineProxy->SetTranslucentSortPriority(Source->TranslucencySortPriority - 1);
	const bool bVisible = Source->IsVisible() && Alpha > 0.0f && ThicknessPixels > 0.0f;
	OutlineProxy->SetVisibility(bVisible, true);
	DebugView.bVisible = bVisible;
}
