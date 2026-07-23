// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartTargetPreviewStyle.h"
#include "Actors/WacomBattleSceneEnemyAuthoringReport.h"
#include "Components/PrimitiveComponent.h"
#include "Data/EnemyHostComponentTestHelpers.h"
#include "Engine/Blueprint.h"
#include "Engine/CollisionProfile.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "PaperFlipbook.h"
#include "PaperSprite.h"
#include "PhysicsEngine/BodySetup.h"
#include "SpriteEditorOnlyTypes.h"
#include "Interaction/WacomInteractionCollisionChannels.h"
#include "UObject/UnrealType.h"

namespace WacomEnemyInteractionAssetContractSpec
{
	const TArray<FString>& HostPackages()
	{
		static const TArray<FString> Packages =
		{
			TEXT("/Game/Wacom/Core/Enemy/BP_EnemyHost_TrainingWarrior"),
			TEXT("/Game/Wacom/Core/Enemy/BP_EnemyHost_Snake"),
			TEXT("/Game/Wacom/Core/Enemy/BP_EnemyHost_SlimeTrio"),
			TEXT("/Game/Wacom/Core/Enemy/BP_SnakeHost_Debug"),
			TEXT("/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/BrushSnake/BP_EnemyHost_BrushSnake_Graybox"),
			TEXT("/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/MoltGuard/BP_EnemyHost_MoltGuard_Graybox"),
			TEXT("/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/RootStalker/BP_EnemyHost_RootStalker_Graybox"),
			TEXT("/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/ShallowGuardian/BP_EnemyHost_ShallowGuardian_Graybox"),
		};
		return Packages;
	}

	FString ObjectPath(const FString& PackagePath)
	{
		return PackagePath + TEXT(".") + FPackageName::GetLongPackageAssetName(PackagePath);
	}

	const TArray<FString>& StableSpritePaths()
	{
		static const TArray<FString> Paths =
		{
			TEXT("/Game/Wacom/Art/Enemies/TrainingWarrior/Sprites/SPR_Enemy_TrainingWarrior_Idle_00.SPR_Enemy_TrainingWarrior_Idle_00"),
			TEXT("/Game/Wacom/Art/Placeholders/Enemies/Snake/Sprites/SPR_Enemy_SnakePlaceholder_Idle_00.SPR_Enemy_SnakePlaceholder_Idle_00"),
			TEXT("/Game/Wacom/Art/Placeholders/Enemies/SlimeTrio/Sprites/SPR_Enemy_SlimeTrioPlaceholder_Idle_00.SPR_Enemy_SlimeTrioPlaceholder_Idle_00"),
		};
		return Paths;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomEnemyInteractionAssetContractSpec,
	"Wacom.Editor.EnemyScene.InteractionVisual.FormalHostAndSpriteContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomEnemyInteractionAssetContractSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomEnemyInteractionAssetContractSpec;
	TestEqual(TEXT("Dedicated trace channel keeps its configured project name"),
		UCollisionProfile::Get()->ReturnChannelNameFromContainerIndex(
			Wacom::Interaction::BattleEnemyPartTraceChannel),
		FName(TEXT("WacomBattleInteraction")));
	UMaterial* ExpectedOutlineMaterial = LoadObject<UMaterial>(
		nullptr,
		TEXT("/Game/DreamMaterials/World/M_WacomBattleEnemyPartInteractionOutline.M_WacomBattleEnemyPartInteractionOutline"));
	TestNotNull(TEXT("Formal outline material loads"), ExpectedOutlineMaterial);
	if (ExpectedOutlineMaterial)
	{
		TestEqual(TEXT("Outline material is translucent"),
			ExpectedOutlineMaterial->GetBlendMode(), BLEND_Translucent);
		TestTrue(TEXT("Outline material is unlit"),
			ExpectedOutlineMaterial->GetShadingModels().HasOnlyShadingModel(MSM_Unlit));
		TestTrue(TEXT("Outline material is two-sided"),
			ExpectedOutlineMaterial->IsTwoSided());

		const TArray<FString> CompileErrors =
			UMaterialEditingLibrary::RecompileMaterial(ExpectedOutlineMaterial);
		if (!CompileErrors.IsEmpty())
		{
			AddError(FString::Printf(
				TEXT("Outline material has compile errors:\n%s"),
				*FString::Join(CompileErrors, TEXT("\n"))));
		}

		TArray<FMaterialParameterInfo> TextureParameters;
		TArray<FMaterialParameterInfo> ScalarParameters;
		TArray<FMaterialParameterInfo> VectorParameters;
		TArray<FGuid> ParameterIds;
		ExpectedOutlineMaterial->GetAllTextureParameterInfo(TextureParameters, ParameterIds);
		ParameterIds.Reset();
		ExpectedOutlineMaterial->GetAllScalarParameterInfo(ScalarParameters, ParameterIds);
		ParameterIds.Reset();
		ExpectedOutlineMaterial->GetAllVectorParameterInfo(VectorParameters, ParameterIds);
		auto HasParameter = [](const TArray<FMaterialParameterInfo>& Parameters, FName Name)
		{
			return Parameters.ContainsByPredicate([Name](const FMaterialParameterInfo& Parameter)
			{
				return Parameter.Name == Name;
			});
		};
		TestTrue(TEXT("Generated material exposes SpriteTexture"),
			HasParameter(TextureParameters, TEXT("SpriteTexture")));
		TestTrue(TEXT("Generated material exposes atlas-local origin"),
			HasParameter(VectorParameters, TEXT("OutlineAtlasUVOrigin")));
		TestTrue(TEXT("Generated material exposes atlas-local X axis"),
			HasParameter(VectorParameters, TEXT("OutlineAtlasUVAxisX")));
		TestTrue(TEXT("Generated material exposes atlas-local Y axis"),
			HasParameter(VectorParameters, TEXT("OutlineAtlasUVAxisY")));
		TestTrue(TEXT("Generated material exposes source-pixel size"),
			HasParameter(VectorParameters, TEXT("OutlineSourceInvPixelSize")));
		TestTrue(TEXT("Generated material counter-remaps the padded proxy canvas"),
			HasParameter(VectorParameters, TEXT("OutlineCanvasToSourceScale")));
		TestTrue(TEXT("Generated material exposes OutlineColor"),
			HasParameter(VectorParameters, TEXT("OutlineColor")));
		TestTrue(TEXT("Generated material exposes source-pixel thickness"),
			HasParameter(ScalarParameters, TEXT("OutlineThicknessPixels")));
		TestTrue(TEXT("Generated material exposes independent opacity"),
			HasParameter(ScalarParameters, TEXT("OutlineOpacity")));
		TestTrue(TEXT("Generated material exposes outer-ring color tuning"),
			HasParameter(ScalarParameters, TEXT("OutlineOuterColorMultiplier")));
		TestTrue(TEXT("Generated material exposes outer-ring alpha tuning"),
			HasParameter(ScalarParameters, TEXT("OutlineOuterAlphaMultiplier")));
	}

	FString MaterialSource;
	FString HelperSource;
	const FString MaterialSourcePath = FPaths::ProjectDir()
		/ TEXT("DShader/Material/World/M_WacomBattleEnemyPartInteractionOutline.dsm");
	const FString HelperSourcePath = FPaths::ProjectDir()
		/ TEXT("DShader/Shared/WacomBattleEnemyPartInteractionOutline.dsh");
	TestTrue(TEXT("DreamShader outline material source is readable"),
		FFileHelper::LoadFileToString(MaterialSource, *MaterialSourcePath));
	TestTrue(TEXT("DreamShader outline helper source is readable"),
		FFileHelper::LoadFileToString(HelperSource, *HelperSourcePath));
	TestTrue(TEXT("DreamShader source imports the formal outline helper"),
		MaterialSource.Contains(TEXT("Shared/WacomBattleEnemyPartInteractionOutline.dsh")));
	TestTrue(TEXT("DreamShader source targets the formal generated package"),
		MaterialSource.Contains(TEXT("DreamMaterials/World/M_WacomBattleEnemyPartInteractionOutline")));
	TestTrue(TEXT("DreamShader source explicitly masks atlas runtime vectors to RG"),
		MaterialSource.Contains(TEXT("Input=OutlineCanvasToSourceScale"))
		&& MaterialSource.Contains(TEXT("OutputType=\"float2\"")));
	TestTrue(TEXT("DreamShader source explicitly reads SpriteTexture alpha"),
		MaterialSource.Contains(TEXT("R=false, G=false, B=false, A=true")));
	TestTrue(TEXT("DreamShader helper isolates one-pixel and two-pixel rings"),
		HelperSource.Contains(TEXT("dilatedOnePixelAlpha - centerMask"))
		&& HelperSource.Contains(TEXT("dilatedTwoPixelAlpha - dilatedOnePixelAlpha")));
	TestTrue(TEXT("DreamShader helper clamps atlas samples and masks proxy padding"),
		HelperSource.Contains(TEXT("safeSourceUV = clamp"))
		&& HelperSource.Contains(TEXT("insideMask = aboveMinimum.x")));
	TestFalse(TEXT("Interaction outline stays deterministic and does not read Time"),
		MaterialSource.Contains(TEXT("UE.Time")) || HelperSource.Contains(TEXT("UE.Time")));
	UWacomBattleEnemyPartTargetPreviewStyle* ExpectedStyle =
		LoadObject<UWacomBattleEnemyPartTargetPreviewStyle>(
			nullptr,
			TEXT("/Game/Wacom/UI/Battle/WorldImpact/DA_BattleEnemyPartTargetPreviewStyle_PixelLock.DA_BattleEnemyPartTargetPreviewStyle_PixelLock"));
	if (TestNotNull(TEXT("Formal target preview style loads"), ExpectedStyle))
	{
		TestTrue(TEXT("Style references formal outline material"),
			ExpectedStyle->OutlineMaterial == ExpectedOutlineMaterial);
		TestTrue(TEXT("Selectable outline color"),
			ExpectedStyle->SelectableOutlineColor.Equals(
				FLinearColor(0.45f, 0.16f, 0.015f, 1.0f)));
		TestEqual(TEXT("Selectable outline source pixels"),
			ExpectedStyle->SelectableOutlineThicknessSourcePixels, 1.0f);
		TestEqual(TEXT("Selectable outline alpha"),
			ExpectedStyle->SelectableOutlineAlpha, 0.55f);
		TestTrue(TEXT("Hovered outline color"),
			ExpectedStyle->HoveredOutlineColor.Equals(
				FLinearColor(0.80f, 0.34f, 0.025f, 1.0f)));
		TestEqual(TEXT("Hovered outline source pixels"),
			ExpectedStyle->HoveredOutlineThicknessSourcePixels, 2.0f);
		TestEqual(TEXT("Hovered outline alpha"),
			ExpectedStyle->HoveredOutlineAlpha, 0.95f);
		TestEqual(TEXT("Hovered outer-ring color multiplier"),
			ExpectedStyle->OutlineOuterColorMultiplier, 0.45f);
		TestEqual(TEXT("Hovered outer-ring alpha multiplier"),
			ExpectedStyle->OutlineOuterAlphaMultiplier, 0.70f);
	}

	for (const FString& PackagePath : HostPackages())
	{
		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *ObjectPath(PackagePath));
		if (!TestNotNull(*FString::Printf(TEXT("Host loads: %s"), *PackagePath), Blueprint)
			|| !Blueprint->GeneratedClass)
		{
			continue;
		}
		TestFalse(*FString::Printf(
			TEXT("Host package is clean after read-only load: %s"),
			*PackagePath),
			Blueprint->GetOutermost()->IsDirty());
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		TestTrue(*FString::Printf(
			TEXT("Host compiles after Part parent-class migration: %s"),
			*PackagePath),
			Blueprint->Status != BS_Error);
		AWacomBattleEnemyActor* Host = Cast<AWacomBattleEnemyActor>(
			Blueprint->GeneratedClass->GetDefaultObject());
		if (!TestNotNull(*FString::Printf(TEXT("Host CDO: %s"), *PackagePath), Host))
		{
			continue;
		}
		const FWacomBattleSceneEnemyHostAuthoringReport Report =
			FWacomBattleSceneEnemyHostAuthoringEvaluator::Build(*Host);
		TestTrue(*FString::Printf(TEXT("Host authoring ready: %s"), *PackagePath),
			Report.bAuthoringReady);
		TestTrue(*FString::Printf(TEXT("Host has interaction layers: %s"), *PackagePath),
			Report.InteractionVisualLayerIds.Num() == Report.PartComponentCount);
		TestTrue(*FString::Printf(TEXT("Host collision ready: %s"), *PackagePath),
			Report.InteractionCollisionNotReadyPartSlotIds.IsEmpty());
		TestTrue(*FString::Printf(TEXT("Host has no authored visual collision: %s"), *PackagePath),
			Report.UnexpectedVisualCollisionPartSlotIds.IsEmpty());
		TestTrue(*FString::Printf(TEXT("Host outline style material: %s"), *PackagePath),
			Host->DefaultTargetPreviewStyle
			&& Host->DefaultTargetPreviewStyle->OutlineMaterial == ExpectedOutlineMaterial);

		for (const Wacom::Tests::EnemyHostComponents::FPartTemplates& Templates :
			Wacom::Tests::EnemyHostComponents::Collect(*Blueprint))
		{
			if (!Templates.Part || !Templates.Flipbook)
			{
				AddError(FString::Printf(
					TEXT("Incomplete interaction templates: %s"), *PackagePath));
				continue;
			}
			TestEqual(*FString::Printf(
				TEXT("Interaction layer ID matches typed visual: %s/%s"),
				*PackagePath,
				*Templates.Part->PartSlotId.ToString()),
				Templates.Part->InteractionVisualLayerId,
				Templates.Flipbook->LayerId);
			TestFalse(*FString::Printf(
				TEXT("Part is identity-only, not Primitive: %s/%s"),
				*PackagePath,
				*Templates.Part->PartSlotId.ToString()),
				Templates.Part->IsA<UPrimitiveComponent>());
			TestEqual(TEXT("Authored visual collision remains disabled"),
				Templates.Flipbook->GetCollisionEnabled(),
				ECollisionEnabled::NoCollision);
		}
	}

	const FStructProperty* CollisionGeometryProperty =
		FindFProperty<FStructProperty>(UPaperSprite::StaticClass(), TEXT("CollisionGeometry"));
	if (!TestNotNull(TEXT("PaperSprite CollisionGeometry property"), CollisionGeometryProperty))
	{
		return false;
	}
	for (const FString& SpritePath : StableSpritePaths())
	{
		UPaperSprite* Sprite = LoadObject<UPaperSprite>(nullptr, *SpritePath);
		if (!TestNotNull(*FString::Printf(TEXT("Stable Sprite loads: %s"), *SpritePath), Sprite))
		{
			continue;
		}
		const FSpriteGeometryCollection* Geometry =
			CollisionGeometryProperty->ContainerPtrToValuePtr<FSpriteGeometryCollection>(Sprite);
		TestEqual(TEXT("Sprite collision domain is 3D"),
			Sprite->GetSpriteCollisionDomain(), ESpriteCollisionMode::Use3DPhysics);
		TestEqual(TEXT("Sprite collision thickness is 12cm"),
			Sprite->GetCollisionThickness(), 12.0f);
		TestTrue(TEXT("Sprite BodySetup has generated geometry"),
			Sprite->BodySetup && Sprite->BodySetup->AggGeom.GetElementCount() > 0);
		if (TestNotNull(TEXT("Sprite collision geometry settings"), Geometry))
		{
			TestEqual(TEXT("Collision geometry is ShrinkWrapped"),
				Geometry->GeometryType.GetValue(), ESpritePolygonMode::ShrinkWrapped);
			TestEqual(TEXT("Alpha threshold"), Geometry->AlphaThreshold, 0.30f);
			TestEqual(TEXT("Detail amount"), Geometry->DetailAmount, 0.65f);
			TestEqual(TEXT("Simplify epsilon"), Geometry->SimplifyEpsilon, 1.5f);
		}
	}
	return true;
}
