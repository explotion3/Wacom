// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"
#include "HAL/FileManager.h"
#include "Materials/Material.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UI/Card/WacomCardPresentationBuilder.h"
#include "UI/CardViewSpecReceiver.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomCardSurfaceParallaxTransformSpec,
	"Wacom.UI.CardView.SurfaceParallax.AttachmentTransformAndReset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomCardSurfaceParallaxTransformSpec::RunTest(const FString& Parameters)
{
	UWacomCardSurfaceParallaxProbe* CardView = NewObject<UWacomCardSurfaceParallaxProbe>();
	TestNotNull(TEXT("Card surface parallax probe is created"), CardView);
	if (!CardView)
	{
		return false;
	}
	CardView->TakeWidget();
	UWidget* AttachmentHost = CardView->GetAttachmentParallaxHostForTest();
	TestNotNull(TEXT("Attachment host is available"), AttachmentHost);
	if (!AttachmentHost)
	{
		return false;
	}

	FWacomCardSurfacePerspectiveView Perspective;
	Perspective.bEnabled = true;
	Perspective.TiltDegrees = FVector2D(4.0f, -5.0f);
	Perspective.Strength = 1.0f;
	Perspective.AttachmentOffsetPixels = FVector2D(5.0f, -3.0f);
	CardView->SetCardSurfacePerspectiveView(Perspective);
	TestTrue(
		TEXT("Attachment offset is added to authored transform"),
		AttachmentHost->GetRenderTransform().Translation.Equals(FVector2D(8.0f, -5.0f), 0.001f));

	Perspective.bReducedMotion = true;
	CardView->SetCardSurfacePerspectiveView(Perspective);
	TestTrue(
		TEXT("Reduced motion restores authored transform"),
		AttachmentHost->GetRenderTransform().Translation.Equals(FVector2D(3.0f, -2.0f), 0.001f));

	Perspective.bReducedMotion = false;
	CardView->SetCardSurfacePerspectiveView(Perspective);
	CardView->ResetCardSurfacePerspectiveView();
	TestTrue(
		TEXT("Reset restores authored transform"),
		AttachmentHost->GetRenderTransform().Translation.Equals(FVector2D(3.0f, -2.0f), 0.001f));
	TestTrue(
		TEXT("Card hit size remains 296 by 420"),
		CardView->GetCardBodyHitSize().Equals(FVector2D(296.0f, 420.0f), 0.001f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomCardSurfaceParallaxProductionArtSpec,
	"Wacom.UI.CardView.SurfaceParallax.ProductionArtAndAuthoredFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomCardSurfaceParallaxProductionArtSpec::RunTest(const FString& Parameters)
{
	UCardDefinition* Definition = NewObject<UCardDefinition>();
	UTexture2D* DefinitionArt = NewObject<UTexture2D>(Definition);
	Definition->CardIllustration = DefinitionArt;
	const FWacomCardViewData BuiltData =
		UWacomCardPresentationBuilder::BuildCardViewData(Definition);
	TestEqual(
		TEXT("Production builder forwards CardDefinition illustration"),
		BuiltData.Art.Get(),
		DefinitionArt);

	UWacomCardSurfaceParallaxProbe* CardView = NewObject<UWacomCardSurfaceParallaxProbe>();
	const TSharedRef<SWidget> SlateWidget = CardView->TakeWidget();
	UTexture2D* AuthoredArt = NewObject<UTexture2D>(CardView);
	CardView->SetAuthoredCardArtTextureForTest(AuthoredArt);
	CardView->SetCardSurfaceMaterialForTest(UMaterial::GetDefaultMaterial(MD_UI));
	CardView->SetCardViewData(BuiltData);
	const FWacomCardViewAutomationTestView DefinitionView = CardView->GetAutomationTestViewForTest();
	TestTrue(TEXT("Definition illustration activates core surface composite"), DefinitionView.bSurfaceCompositeActive);
	TestEqual(TEXT("Definition illustration overrides authored fallback"), DefinitionView.ResolvedSurfaceArt, DefinitionArt);

	FWacomCardViewData LegacyData;
	LegacyData.Name = FText::FromString(TEXT("Legacy authored art fallback"));
	CardView->SetCardViewData(LegacyData);
	const FWacomCardViewAutomationTestView LegacyView = CardView->GetAutomationTestViewForTest();
	TestTrue(TEXT("Authored CardArt texture activates core surface composite"), LegacyView.bSurfaceCompositeActive);
	TestEqual(TEXT("Authored CardArt texture is the resolved surface art"), LegacyView.ResolvedSurfaceArt, AuthoredArt);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomCardSurfaceParallaxDreamShaderContractSpec,
	"Wacom.UI.CardView.SurfaceParallax.DreamShaderContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomCardSurfaceParallaxDreamShaderContractSpec::RunTest(const FString& Parameters)
{
	const FString MaterialPath = FPaths::ProjectDir()
		/ TEXT("DShader/Material/Card/M_WacomCardSurfaceComposite.dsm");
	const FString HelperPath = FPaths::ProjectDir()
		/ TEXT("DShader/Shared/WacomCardSurfaceParallax.dsh");
	FString MaterialSource;
	FString HelperSource;
	TestTrue(TEXT("Composite material source exists"), FFileHelper::LoadFileToString(MaterialSource, *MaterialPath));
	TestTrue(TEXT("Parallax helper source exists"), FFileHelper::LoadFileToString(HelperSource, *HelperPath));

	TestTrue(TEXT("Material remains UI domain"), MaterialSource.Contains(TEXT("Domain = \"UI\"")));
	TestTrue(TEXT("Material remains premultiplied alpha"), MaterialSource.Contains(TEXT("BlendMode = \"PremultipliedAlpha\"")));
	TestTrue(TEXT("Art texture contract is present"), MaterialSource.Contains(TEXT("ArtTexture")));
	TestTrue(TEXT("Frame texture contract is present"), MaterialSource.Contains(TEXT("FrameTexture")));
	TestTrue(TEXT("Rarity atlas contract is present"), MaterialSource.Contains(TEXT("RarityUVScaleBias")));
	const FString CardViewSourcePath = FPaths::ProjectDir()
		/ TEXT("Source/WacomApp/Private/UI/Card/WacomCardView.cpp");
	FString CardViewSource;
	TestTrue(TEXT("CardView source exists"), FFileHelper::LoadFileToString(CardViewSource, *CardViewSourcePath));
	TestTrue(
		TEXT("Rarity remains a PaperSprite atlas region instead of a whole texture"),
		CardViewSource.Contains(TEXT("GetBakedTexture"))
		&& CardViewSource.Contains(TEXT("GetSourceUV"))
		&& CardViewSource.Contains(TEXT("GetSourceSize")));
	TestTrue(TEXT("Runtime tilt contract is present"), MaterialSource.Contains(TEXT("ScalarParameter TiltX"))
		&& MaterialSource.Contains(TEXT("ScalarParameter TiltY")));
	TestTrue(TEXT("Helper clamps atlas sampling in local UV"), HelperSource.Contains(TEXT("localUVAndMask.z")));
	TestFalse(TEXT("Surface parallax has no time loop"), MaterialSource.Contains(TEXT("Time("))
		|| HelperSource.Contains(TEXT("Time(")));
	return true;
}
