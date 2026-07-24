// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Actors/WacomWorldShopActor.h"
#include "Components/WidgetComponent.h"
#include "Components/WacomWorldShopLayoutAnchorComponent.h"
#include "Engine/World.h"
#include "UI/Shop/WacomWorldShopCardGeometry.h"
#include "UI/Shop/WacomWorldShopPresentationHost.h"
#include "UObject/UnrealType.h"

namespace WacomWorldShopCardGeometrySpec
{
	struct FTransientWorldFixture
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);

		~FTransientWorldFixture()
		{
			if (World)
			{
				World->DestroyWorld(false);
			}
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomWorldShopCardGeometryAbsoluteScaleSpec,
	"Wacom.UI.WorldShop.CardGeometry.AbsoluteWorldScale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomWorldShopCardGeometryAbsoluteScaleSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomWorldShopCardGeometrySpec;

	FTransientWorldFixture Fixture;
	if (!TestNotNull(TEXT("Transient world exists"), Fixture.World))
	{
		return false;
	}

	AWacomWorldShopActor* Shop =
		Fixture.World->SpawnActor<AWacomWorldShopActor>();
	if (!TestNotNull(TEXT("Formal world shop spawns"), Shop))
	{
		return false;
	}
	Shop->SetActorScale3D(FVector(1.0f, 1.222f, 1.0f));
	Shop->RerunConstructionScripts();

	const FWacomWorldShopPresentationHost Host =
		Shop->BuildPresentationHost();
	const TArray<UWacomWorldShopLayoutAnchorComponent*> Anchors =
		Shop->GetOfferLayoutAnchorsSorted();
	if (!TestTrue(TEXT("Formal host and first anchor are valid"),
			Host.IsSet() && Anchors.IsValidIndex(0) && Anchors[0]))
	{
		return false;
	}

	UWidgetComponent* CardComponent = NewObject<UWidgetComponent>(
		Shop,
		TEXT("WorldShopCardGeometryProbe"));
	Shop->AddInstanceComponent(CardComponent);
	CardComponent->SetupAttachment(Anchors[0]);
	CardComponent->SetRelativeTransform(FTransform::Identity);
	Host.ApplyCardWidgetGeometry(*CardComponent);
	CardComponent->RegisterComponent();

	TestEqual(
		TEXT("registered runtime component updates its current draw width"),
		CardComponent->GetCurrentDrawSize().X,
		720.0);
	TestEqual(
		TEXT("registered runtime component updates its current draw height"),
		CardComponent->GetCurrentDrawSize().Y,
		976.0);
	TestTrue(
		TEXT("non-uniform owner scale does not change card world scale"),
		CardComponent->GetComponentTransform().GetScale3D().Equals(
			FVector(0.13f),
			0.001f));
	TestEqual(
		TEXT("runtime widget uses the formal render draw size"),
		CardComponent->GetDrawSize(),
		FVector2D(720.0f, 976.0f));
	TestTrue(
		TEXT("authoring preview reports the same absolute render plane"),
		Anchors[0]->GetCardPreviewSizeCm().Equals(
			FVector2D(93.6f, 126.88f),
			0.01f));
	TestTrue(
		TEXT("visible product preview excludes transparent overscan"),
		Anchors[0]->GetVisibleProductPreviewSizeCm().Equals(
			FVector2D(76.96f, 122.72f),
			0.01f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomWorldShopCardGeometryDetailsCategorySpec,
	"Wacom.UI.WorldShop.CardGeometry.DetailsCategory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomWorldShopCardGeometryDetailsCategorySpec::RunTest(
	const FString& /*Parameters*/)
{
	const FProperty* CardWorldScaleProperty =
		FindFProperty<FProperty>(
			AWacomWorldShopActor::StaticClass(),
			TEXT("CardWorldScale"));
	if (!TestNotNull(TEXT("CardWorldScale property exists"),
			CardWorldScaleProperty))
	{
		return false;
	}
	TestEqual(
		TEXT("CardWorldScale belongs to the single World Shop settings category"),
		CardWorldScaleProperty->GetMetaData(TEXT("Category")),
		FString(TEXT("Wacom|World Shop")));
	return true;
}

#endif
