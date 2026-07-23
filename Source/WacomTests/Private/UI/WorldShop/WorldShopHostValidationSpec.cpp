// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomWorldShopHostActor.h"
#include "Components/WacomWorldShopOfferAnchorComponent.h"

#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomWorldShopHostValidationSpec,
	"Wacom.UI.WorldShop.HostValidation.DefaultCapacityAndStableOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomWorldShopHostValidationSpec::RunTest(const FString& Parameters)
{
	AWacomWorldShopHostActor* Host = NewObject<AWacomWorldShopHostActor>();
	TestEqual(TEXT("world card uses 2x supersampled draw size"),
		Host->CardDrawSize, FIntPoint(720, 976));
	TestEqual(TEXT("world scale preserves the intended physical shelf size"),
		Host->CardWorldScale, 0.10f);
	const TArray<UWacomWorldShopOfferAnchorComponent*> Anchors = Host->GetEnabledOfferAnchorsSorted();
	TestEqual(TEXT("default host has 2x4 anchors"), Anchors.Num(), 8);
	for (int32 Index = 0; Index < Anchors.Num(); ++Index)
	{
		TestEqual(TEXT("stable order"), Anchors[Index]->SlotOrder, Index);
		TestFalse(TEXT("stable slot id"), Anchors[Index]->SlotId.IsNone());
	}
	TestTrue(TEXT("eight offers fit"), Host->ValidateForOfferCount(8).bValid);
	TestEqual(
		TEXT("nine offers fail closed"),
		Host->ValidateForOfferCount(9).FailureReason,
		FName(TEXT("InsufficientAnchorCapacity")));

	Host->InteractionDistance = 0.0f;
	TestEqual(
		TEXT("invalid distance rejected"),
		Host->ValidateForOfferCount(1).FailureReason,
		FName(TEXT("InvalidWidgetProfile")));

	Host = NewObject<AWacomWorldShopHostActor>();
	TArray<UWacomWorldShopOfferAnchorComponent*> MutableAnchors = Host->GetEnabledOfferAnchorsSorted();
	MutableAnchors[1]->SlotId = MutableAnchors[0]->SlotId;
	TestEqual(
		TEXT("duplicate slot id rejected"),
		Host->ValidateForOfferCount(1).FailureReason,
		FName(TEXT("DuplicateSlotIdentity")));

	Host = NewObject<AWacomWorldShopHostActor>();
	MutableAnchors = Host->GetEnabledOfferAnchorsSorted();
	MutableAnchors[1]->SlotOrder = MutableAnchors[0]->SlotOrder;
	TestEqual(
		TEXT("duplicate slot order rejected"),
		Host->ValidateForOfferCount(1).FailureReason,
		FName(TEXT("DuplicateSlotIdentity")));

	Host = NewObject<AWacomWorldShopHostActor>();
	MutableAnchors = Host->GetEnabledOfferAnchorsSorted();
	MutableAnchors[0]->SlotId = NAME_None;
	TestEqual(
		TEXT("missing slot id rejected"),
		Host->ValidateForOfferCount(1).FailureReason,
		FName(TEXT("MissingSlotId")));

	Host = NewObject<AWacomWorldShopHostActor>();
	MutableAnchors = Host->GetEnabledOfferAnchorsSorted();
	for (UWacomWorldShopOfferAnchorComponent* Anchor : MutableAnchors)
	{
		Anchor->bEnabledForOffers = false;
	}
	TestEqual(
		TEXT("host without enabled anchors rejected"),
		Host->ValidateForOfferCount(0).FailureReason,
		FName(TEXT("MissingOfferAnchors")));

	Host = NewObject<AWacomWorldShopHostActor>();
	MutableAnchors = Host->GetEnabledOfferAnchorsSorted();
	MutableAnchors[0]->SetRelativeScale3D(FVector::ZeroVector);
	TestEqual(
		TEXT("zero-scale anchor transform rejected"),
		Host->ValidateForOfferCount(1).FailureReason,
		FName(TEXT("InvalidAnchorTransform")));

	Host = NewObject<AWacomWorldShopHostActor>();
	Host->CardDrawSize = FIntPoint::ZeroValue;
	TestEqual(
		TEXT("invalid draw size rejected"),
		Host->ValidateForOfferCount(1).FailureReason,
		FName(TEXT("InvalidWidgetProfile")));

	Host = NewObject<AWacomWorldShopHostActor>();
	Host->bOverrideCursorLookProfile = true;
	Host->CursorLookProfileOverride.YawClampDegrees = std::numeric_limits<float>::quiet_NaN();
	TestEqual(
		TEXT("non-finite look override rejected"),
		Host->ValidateForOfferCount(1).FailureReason,
		FName(TEXT("InvalidLookProfile")));
	return true;
}
