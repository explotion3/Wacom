// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomRunTunnelPaperLayerActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UI/WacomRunTunnelPaperLayerActorTestAccess.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunTunnelPaperLayerPersistenceSpec,
	"Wacom.App.RunTunnel.PaperLayerPersistence.TransientPreviewMaterial",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunTunnelPaperLayerPersistenceSpec::RunTest(
	const FString& /*Parameters*/)
{
	AWacomRunTunnelPaperLayerActor* Actor =
		NewObject<AWacomRunTunnelPaperLayerActor>(GetTransientPackage());
	if (!TestNotNull(TEXT("Paper layer actor"), Actor))
	{
		return false;
	}

	UMaterialInterface* SourceMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
	UTexture2D* Texture = UTexture2D::CreateTransient(2, 2, PF_B8G8R8A8);
	Actor->PaperMaterial = SourceMaterial;
	Actor->PaperTextures = { Texture };
	Actor->FixedTextureIndex = 0;
	Actor->RefreshPaperLayerMaterial();

	UMaterialInstanceDynamic* FirstPreview = Actor->GetDynamicPaperMaterial();
	TestNotNull(TEXT("Transient preview MID is created"), FirstPreview);
	TestTrue(TEXT("Preview material is transient"),
		FirstPreview && FirstPreview->HasAnyFlags(RF_Transient));
	TestTrue(TEXT("Plane uses preview MID"),
		Actor->GetPaperPlaneComponent()->GetMaterial(0) == FirstPreview);

	Actor->RefreshPaperLayerMaterial();
	TestTrue(TEXT("Repeated refresh reuses preview MID"),
		Actor->GetDynamicPaperMaterial() == FirstPreview);

	FWacomRunTunnelPaperLayerActorTestAccess::
		RestoreAuthoredMaterialForSerialization(*Actor);
	TestNull(TEXT("Serialization boundary clears preview MID"),
		Actor->GetDynamicPaperMaterial());
	TestTrue(TEXT("Serialization boundary restores authored material"),
		Actor->GetPaperPlaneComponent()->GetMaterial(0) == SourceMaterial);

	Actor->RefreshPaperLayerMaterial();
	TestNotNull(TEXT("Editor preview can be rebuilt after save"),
		Actor->GetDynamicPaperMaterial());
	TestTrue(TEXT("Rebuilt preview remains transient"),
		Actor->GetDynamicPaperMaterial()->HasAnyFlags(RF_Transient));
	return true;
}
