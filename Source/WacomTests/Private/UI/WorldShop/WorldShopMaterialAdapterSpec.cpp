// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "../../../../WacomApp/Private/UI/Card/WacomWorldCardSurfaceMaterialAdapter.h"
#include "Components/WidgetComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomWorldShopMaterialAdapterProductionSpec,
	"Wacom.UI.WorldShop.MaterialAdapter.ProductionContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomWorldShopMaterialAdapterProductionSpec::RunTest(
	const FString& Parameters)
{
	const FString ExpectedMaterialPath =
		TEXT("/Game/Wacom/UI/Card/World/M_WorldCardSurface.M_WorldCardSurface");
	TestEqual(
		TEXT("adapter owns exact formal material path"),
		FString(FWacomWorldCardSurfaceMaterialAdapter::GetMaterialPath()),
		ExpectedMaterialPath);
	TestEqual(
		TEXT("formal shop strength is fixed"),
		FWacomWorldCardSurfaceMaterialAdapter::GetProductionExposureStrength(),
		1.0f);

	UMaterialInterface* Material =
		FWacomWorldCardSurfaceMaterialAdapter::ResolveMaterial();
	if (!TestNotNull(TEXT("formal world card material loads"), Material))
	{
		return false;
	}
	TestEqual(TEXT("resolved material is exact asset"), Material->GetPathName(), ExpectedMaterialPath);

	UWidgetComponent* Component = NewObject<UWidgetComponent>();
	TestTrue(
		TEXT("formal adapter applies"),
		FWacomWorldCardSurfaceMaterialAdapter::Apply(
			*Component,
			FWacomWorldCardSurfaceMaterialAdapter::GetProductionExposureStrength()));
	TestEqual(TEXT("formal blend is Masked"), Component->GetBlendMode(), EWidgetBlendMode::Masked);

	UMaterialInstanceDynamic* MaterialInstance = Component->GetMaterialInstance();
	if (TestNotNull(TEXT("widget component owns a MID"), MaterialInstance))
	{
		TestEqual(
			TEXT("MID parent is exact formal material"),
			MaterialInstance->Parent.Get(),
			Material);
		TestEqual(
			TEXT("MID exposure strength is full compensation"),
			MaterialInstance->K2_GetScalarParameterValue(
				FWacomWorldCardSurfaceMaterialAdapter::GetExposureStrengthParameterName()),
			1.0f);
	}

	UWidgetComponent* MissingMaterialComponent = NewObject<UWidgetComponent>();
	TestFalse(
		TEXT("missing material fails closed"),
		FWacomWorldCardSurfaceMaterialAdapter::ApplyResolvedMaterial(
			*MissingMaterialComponent,
			nullptr,
			1.0f));
	TestFalse(
		TEXT("out-of-contract strength fails closed"),
		FWacomWorldCardSurfaceMaterialAdapter::ApplyResolvedMaterial(
			*MissingMaterialComponent,
			Material,
			1.01f));
	return true;
}
