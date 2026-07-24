// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Actors/WacomFirstPersonViewpointActor.h"
#include "Actors/WacomShopTriggerActor.h"
#include "Actors/WacomWorldShopActor.h"
#include "Actors/WacomWorldShopHostActor.h"
#include "Camera/WacomFirstPersonViewStageRequest.h"
#include "Components/BoxComponent.h"
#include "Components/ActorComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/DrawFrustumComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WacomRunMapNodeBindingComponent.h"
#include "Components/WacomWorldShopLayoutAnchorComponent.h"
#include "Components/WacomWorldShopOfferAnchorComponent.h"
#include "Engine/World.h"
#include "Editor/UnrealEdEngine.h"
#include "IDetailsView.h"
#include "Kismet2/ComponentEditorUtils.h"
#include "Map/WacomMapTypes.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorDelegates.h"
#include "PropertyEditorModule.h"
#include "PropertyPath.h"
#include "UI/Shop/WacomWorldShopPresentationHost.h"
#include "UObject/UnrealType.h"
#include "UnrealEdGlobals.h"

namespace WacomWorldShopFormalActorSpec
{
	bool IsWorldShopComponentReferencePath(
		const FPropertyPath& PropertyPath)
	{
		if (!PropertyPath.IsValid())
		{
			return false;
		}

		const FProperty* RootProperty =
			PropertyPath.GetRootProperty().Property.Get();
		const FObjectPropertyBase* ObjectProperty =
			CastField<FObjectPropertyBase>(RootProperty);
		const UClass* OwnerClass = RootProperty
			? Cast<UClass>(RootProperty->GetOwnerStruct())
			: nullptr;
		return ObjectProperty
			&& ObjectProperty->PropertyClass
			&& ObjectProperty->PropertyClass->IsChildOf(
				UActorComponent::StaticClass())
			&& OwnerClass
			&& OwnerClass->IsChildOf(
				AWacomShopTriggerActor::StaticClass());
	}

	bool HasDisplayedWorldShopComponentReference(
		const IDetailsView& DetailsView)
	{
		for (const FPropertyPath& PropertyPath :
			DetailsView.GetPropertiesInOrderDisplayed())
		{
			if (IsWorldShopComponentReferencePath(PropertyPath))
			{
				return true;
			}
		}
		return false;
	}

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

		template <typename TActor>
		TActor* Spawn() const
		{
			FActorSpawnParameters Params;
			Params.ObjectFlags |= RF_Transient;
			return World
				? World->SpawnActor<TActor>(
					TActor::StaticClass(),
					FTransform::Identity,
					Params)
				: nullptr;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomWorldShopLayoutAnchorVisualizerRegistrationSpec,
	"Wacom.UI.WorldShop.FormalActor.LayoutAnchorVisualizerRegistration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomWorldShopLayoutAnchorVisualizerRegistrationSpec::RunTest(
	const FString& Parameters)
{
	FModuleManager::LoadModuleChecked<IModuleInterface>(TEXT("WacomEditor"));
	if (!TestNotNull(TEXT("GUnrealEd is initialized"), GUnrealEd))
	{
		return false;
	}
	TestTrue(
		TEXT("the layout anchor component visualizer is registered"),
		GUnrealEd->FindComponentVisualizer(
			UWacomWorldShopLayoutAnchorComponent::StaticClass()).IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomWorldShopFormalActorCompositeContractSpec,
	"Wacom.UI.WorldShop.FormalActor.CompositeContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomWorldShopFormalActorCompositeContractSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomWorldShopFormalActorSpec;

	FTransientWorldFixture Fixture;
	AWacomWorldShopActor* Shop = Fixture.Spawn<AWacomWorldShopActor>();
	if (!TestNotNull(TEXT("Formal shop spawns"), Shop))
	{
		return false;
	}

	TestTrue(TEXT("Formal shop remains a legacy-compatible trigger"),
		Shop->IsA<AWacomShopTriggerActor>());
	TestFalse(
		TEXT("Layout anchors must not carry Primitive/BodyInstance state"),
		UWacomWorldShopLayoutAnchorComponent::StaticClass()->IsChildOf(
			UPrimitiveComponent::StaticClass()));
	TestNull(
		TEXT("Layout anchor collection must not be exposed as an inline reflected property"),
		FindFProperty<FProperty>(
			AWacomWorldShopActor::StaticClass(),
			TEXT("OfferLayoutAnchors")));
	for (TFieldIterator<FObjectPropertyBase> PropertyIt(
			AWacomWorldShopActor::StaticClass(),
			EFieldIteratorFlags::IncludeSuper);
		PropertyIt;
		++PropertyIt)
	{
		const FObjectPropertyBase* ObjectProperty = *PropertyIt;
		const UClass* OwnerClass = ObjectProperty
			? Cast<UClass>(ObjectProperty->GetOwnerStruct())
			: nullptr;
		if (!ObjectProperty
			|| !ObjectProperty->PropertyClass
			|| !ObjectProperty->PropertyClass->IsChildOf(
				UActorComponent::StaticClass())
			|| !OwnerClass
			|| !OwnerClass->IsChildOf(
				AWacomShopTriggerActor::StaticClass()))
		{
			continue;
		}

		TestTrue(
			*FString::Printf(
				TEXT("%s is excluded from Level Actor Details"),
				*ObjectProperty->GetName()),
			ObjectProperty->HasAnyPropertyFlags(
				CPF_DisableEditOnInstance));
	}
	TestEqual(TEXT("Default trigger radius"), Shop->TriggerRadius, 350.0f);
	TestEqual(TEXT("Trigger sphere follows authored radius"),
		Shop->GetTriggerSphere()->GetUnscaledSphereRadius(), 350.0f);
	TestNotNull(
		TEXT("Trigger sphere remains selectable through the Components tree"),
		FComponentEditorUtils::GetPropertyForEditableNativeComponent(
			Shop->GetTriggerSphere()));
	TestNotNull(
		TEXT("Click bounds remains selectable through the Components tree"),
		FComponentEditorUtils::GetPropertyForEditableNativeComponent(
			Shop->GetClickBounds()));

	USceneComponent* PresentationRoot = Shop->GetPresentationRootComponent();
	USceneComponent* CardLayoutRoot = Shop->GetCardLayoutRootComponent();
	UChildActorComponent* ViewpointComponent =
		Shop->GetShopEntryViewpointComponent();
	USceneComponent* FocusComponent =
		Shop->GetShopFocusAnchorComponent();
	TestNotNull(TEXT("Presentation root exists"), PresentationRoot);
	TestNotNull(TEXT("Card layout root exists"), CardLayoutRoot);
	TestNotNull(TEXT("Internal viewpoint component exists"), ViewpointComponent);
	TestNotNull(TEXT("Composition focus exists"), FocusComponent);
	TestTrue(TEXT("Card layout is directly parented to presentation root"),
		CardLayoutRoot
			&& CardLayoutRoot->GetAttachParent() == PresentationRoot);
	TestTrue(TEXT("Composition focus is parented to card layout"),
		FocusComponent
			&& FocusComponent->GetAttachParent() == CardLayoutRoot);
	TestTrue(TEXT("Composition focus defaults to second-column center"),
		FocusComponent && FocusComponent->GetRelativeLocation().Equals(
			FVector(0.0f, -50.8f, 0.0f),
			0.01f));
	TestEqual(TEXT("Formal card scale defaults to 0.13"),
		Shop->CardWorldScale, 0.13f);

	TArray<UWacomWorldShopLayoutAnchorComponent*> LayoutAnchors =
		Shop->GetOfferLayoutAnchorsSorted();
	TestEqual(TEXT("Formal actor exposes eight actual layout anchors"),
		LayoutAnchors.Num(), 8);
	const TArray<FName> ExpectedAnchorPropertyNames =
	{
		TEXT("OfferLayoutAnchor01"),
		TEXT("OfferLayoutAnchor02"),
		TEXT("OfferLayoutAnchor03"),
		TEXT("OfferLayoutAnchor04"),
		TEXT("OfferLayoutAnchor05"),
		TEXT("OfferLayoutAnchor06"),
		TEXT("OfferLayoutAnchor07"),
		TEXT("OfferLayoutAnchor08"),
	};
	const FVector2D ExpectedPreviewSize(93.6f, 126.88f);
	const TArray<float> ExpectedColumns =
		{ -152.4f, -50.8f, 50.8f, 152.4f };
	const TArray<float> ExpectedRows = { 67.44f, -67.44f };
	for (int32 Index = 0; Index < LayoutAnchors.Num(); ++Index)
	{
		UWacomWorldShopLayoutAnchorComponent* Anchor =
			LayoutAnchors[Index];
		const int32 Row = Index / 4;
		const int32 Column = Index % 4;
		if (!TestNotNull(
				*FString::Printf(TEXT("Layout anchor %d exists"), Index + 1),
				Anchor))
		{
			continue;
		}
		TestTrue(
			*FString::Printf(TEXT("Layout anchor %d is a runtime offer anchor"), Index + 1),
			Anchor->IsA<UWacomWorldShopOfferAnchorComponent>());
		TestEqual(
			*FString::Printf(TEXT("Layout anchor %d stable id"), Index + 1),
			Anchor->GetSlotId(),
			FName(*FString::Printf(TEXT("Offer.%02d"), Index + 1)));
		TestEqual(
			*FString::Printf(TEXT("Layout anchor %d stable order"), Index + 1),
			Anchor->GetSlotOrder(),
			Index);
		TestTrue(
			*FString::Printf(TEXT("Layout anchor %d editable when inherited"), Index + 1),
			Anchor->bEditableWhenInherited);
		const FProperty* EditableNativeProperty =
			FComponentEditorUtils::GetPropertyForEditableNativeComponent(
				Anchor);
		if (TestNotNull(
			*FString::Printf(
				TEXT("Layout anchor %d has an editable native component property"),
				Index + 1),
			EditableNativeProperty))
		{
			TestEqual(
				*FString::Printf(
					TEXT("Layout anchor %d property identity"),
					Index + 1),
				EditableNativeProperty->GetFName(),
				ExpectedAnchorPropertyNames[Index]);
		}
		TestTrue(
			*FString::Printf(TEXT("Layout anchor %d preview matches world card size"), Index + 1),
			Anchor->GetCardPreviewSizeCm().Equals(
				ExpectedPreviewSize,
				0.01f));
		TestTrue(
			*FString::Printf(TEXT("Layout anchor %d default position"), Index + 1),
			Anchor->GetRelativeLocation().Equals(
				FVector(
					0.0f,
					ExpectedColumns[Column],
					ExpectedRows[Row]),
				0.01f));
	}

	UWacomWorldShopLayoutAnchorComponent* ExtraDerivedAnchor =
		NewObject<UWacomWorldShopLayoutAnchorComponent>(
			Shop,
			TEXT("OfferLayoutAnchor_DerivedExtra"));
	Shop->AddInstanceComponent(ExtraDerivedAnchor);
	TestEqual(
		TEXT("Formal getter ignores extra derived or instance layout anchors"),
		Shop->GetOfferLayoutAnchorsSorted().Num(),
		8);

	FWacomWorldShopPresentationHost Host =
		Shop->BuildPresentationHost();
	TestTrue(TEXT("Formal actor is its own presentation host owner"),
		Host.IsOwnedBy(Shop));
	TestTrue(TEXT("Formal host validates for eight offers"),
		Host.ValidateForOfferCount(8).bValid);
	TestEqual(TEXT("Formal host uses exact draw size"),
		Host.CardDrawSize, FIntPoint(720, 976));
	TestEqual(TEXT("Formal host receives card scale"),
		Host.CardWorldScale, 0.13f);
	const TArray<UWacomWorldShopOfferAnchorComponent*> RuntimeAnchors =
		Host.GetEnabledOfferAnchorsSorted();
	TestEqual(TEXT("Formal host exposes eight actual anchors"),
		RuntimeAnchors.Num(), 8);
	for (int32 Index = 0; Index < RuntimeAnchors.Num(); ++Index)
	{
		TestTrue(
			*FString::Printf(TEXT("Runtime anchor %d is the authored component"), Index + 1),
			LayoutAnchors.IsValidIndex(Index)
				&& RuntimeAnchors[Index] == LayoutAnchors[Index]);
	}

	if (LayoutAnchors.Num() == 8)
	{
		const FVector AuthoredLocation(12.0f, -165.0f, 72.0f);
		const FRotator AuthoredRotation(0.0f, 4.0f, -2.0f);
		LayoutAnchors[0]->SetRelativeLocation(AuthoredLocation);
		LayoutAnchors[0]->SetRelativeRotation(AuthoredRotation);
		LayoutAnchors[0]->SetRelativeScale3D(FVector(2.0f));
		Shop->RerunConstructionScripts();
		LayoutAnchors = Shop->GetOfferLayoutAnchorsSorted();
		TestTrue(TEXT("Construction preserves hand-authored anchor location"),
			LayoutAnchors.Num() == 8
				&& LayoutAnchors[0]->GetRelativeLocation().Equals(
					AuthoredLocation,
					0.001f));
		TestTrue(TEXT("Construction preserves hand-authored anchor rotation"),
			LayoutAnchors.Num() == 8
				&& LayoutAnchors[0]->GetRelativeRotation().Equals(
					AuthoredRotation,
					0.001f));
		TestTrue(TEXT("Construction normalizes per-slot scale"),
			LayoutAnchors.Num() == 8
				&& LayoutAnchors[0]->GetRelativeScale3D().Equals(
					FVector::OneVector,
					0.001f));

		Shop->CardWorldScale = 0.12f;
		Shop->RerunConstructionScripts();
		LayoutAnchors = Shop->GetOfferLayoutAnchorsSorted();
		Host = Shop->BuildPresentationHost();
		TestTrue(TEXT("Scale change updates editor preview"),
			LayoutAnchors.Num() == 8
				&& LayoutAnchors[0]->GetCardPreviewSizeCm().Equals(
					FVector2D(86.4f, 117.12f),
					0.01f));
		TestTrue(TEXT("Scale change updates direct runtime host"),
			FMath::IsNearlyEqual(Host.CardWorldScale, 0.12f));
		Shop->CardWorldScale = 0.13f;
		Shop->RerunConstructionScripts();
	}

	TestNotNull(TEXT("Editor viewpoint frustum exists"),
		Shop->GetShopViewFrustumComponent());
	TestNotNull(TEXT("Editor focus direction exists"),
		Shop->GetShopFocusDirectionComponent());
	TestTrue(TEXT("Viewpoint frustum is editor-only"),
		Shop->GetShopViewFrustumComponent()
			&& Shop->GetShopViewFrustumComponent()->IsEditorOnly());
	TestEqual(TEXT("Close browse preset defaults to 220 cm"),
		Shop->CloseBrowsePresetDistanceCm, 220.0f);
	TestTrue(TEXT("Click bounds follow presentation root"),
		Shop->GetClickBounds()->GetAttachParent() == PresentationRoot);
	TestTrue(TEXT("Viewpoint child class is exact native viewpoint"),
		ViewpointComponent
			&& ViewpointComponent->GetChildActorClass()
				== AWacomFirstPersonViewpointActor::StaticClass());

	AWacomFirstPersonViewpointActor* InternalViewpoint =
		Shop->GetInternalShopEntryViewpoint();
	if (!TestNotNull(TEXT("Internal viewpoint is created"), InternalViewpoint))
	{
		return false;
	}
	TestEqual(TEXT("Viewpoint default blend time"),
		InternalViewpoint->StageBlendTimeSeconds, 0.25f);
	TestEqual(TEXT("Viewpoint default blend curve"),
		InternalViewpoint->StageBlendCurve,
		EWacomFirstPersonViewStageBlendCurve::SmoothStep);
	TestFalse(TEXT("Formal actor defaults to Run live look profile"),
		Shop->bOverrideCursorLookProfile);

	Shop->ApplyCloseBrowsePreset();
	const FTransform FirstPresetTransform =
		ViewpointComponent->GetComponentTransform();
	TestTrue(TEXT("Close browse preset places viewpoint 220 cm from focus"),
		FMath::IsNearlyEqual(
			FVector::Distance(
				ViewpointComponent->GetComponentLocation(),
				FocusComponent->GetComponentLocation()),
			220.0f,
			0.01f));
	TestTrue(TEXT("Close browse preset aims viewpoint at focus"),
		FVector::DotProduct(
			ViewpointComponent->GetForwardVector(),
			(FocusComponent->GetComponentLocation()
				- ViewpointComponent->GetComponentLocation()).GetSafeNormal())
			> 0.999f);
	Shop->ApplyCloseBrowsePreset();
	TestTrue(TEXT("Close browse preset is idempotent"),
		ViewpointComponent->GetComponentTransform().Equals(
			FirstPresetTransform,
			0.001f));

	Shop->bOverrideCursorLookProfile = true;
	Shop->CursorLookProfileOverride.YawClampDegrees = 30.0f;
	Shop->CursorLookProfileOverride.PitchClampDegrees = 12.0f;
	Shop->CursorLookProfileOverride.CursorDeadZoneNormalized =
		FVector2D(0.12f, 0.18f);
	Shop->CursorLookProfileOverride.CursorResponseExponent = 1.35f;
	Shop->RerunConstructionScripts();
	Host = Shop->BuildPresentationHost();
	TestTrue(TEXT("Formal host receives browse override"),
		Host.bOverrideCursorLookProfile);
	TestTrue(TEXT("Formal host receives formal yaw clamp"),
		FMath::IsNearlyEqual(
			Host.CursorLookProfileOverride.YawClampDegrees,
			30.0f));

	UWacomRunMapNodeBindingComponent* Binding =
		Shop->GetRunMapNodeBindingComponent();
	TestNotNull(TEXT("Run map binding exists"), Binding);
	TestTrue(TEXT("Run map binding defaults to Shop"),
		Binding && Binding->NodeType == EWacomMapNodeType::Shop);
	TestTrue(TEXT("Run map binding leaves per-instance NodeId unset"),
		Binding && Binding->NodeId.IsNone());

	AWacomWorldShopHostActor* ExternalHost =
		Fixture.Spawn<AWacomWorldShopHostActor>();
	AWacomFirstPersonViewpointActor* ExternalViewpoint =
		Fixture.Spawn<AWacomFirstPersonViewpointActor>();
	Shop->WorldShopHost = ExternalHost;
	Shop->ShopEntryViewpoint = ExternalViewpoint;
	Shop->PersistentId = TEXT("Shop.Formal.CompositeTest");

	FWacomFirstPersonViewStageRequest FormalStageRequest;
	TestTrue(TEXT("Formal shop resolves its internal viewpoint"),
		Shop->TryBuildShopEntryViewStageRequest(FormalStageRequest));
	TestTrue(TEXT("Formal stage ignores legacy external viewpoint"),
		FormalStageRequest.ViewTransform.Equals(
			InternalViewpoint->GetActorTransform()));
	TestEqual(TEXT("Formal debug resolves the formal actor as host"),
		Shop->GetShopTriggerDebugView(nullptr).WorldShopHostName,
		Shop->GetName());

	AWacomShopTriggerActor* LegacyShop =
		Fixture.Spawn<AWacomShopTriggerActor>();
	LegacyShop->PersistentId = TEXT("Shop.Legacy.CompositeTest");
	LegacyShop->WorldShopHost = ExternalHost;
	LegacyShop->ShopEntryViewpoint = ExternalViewpoint;
	FWacomFirstPersonViewStageRequest LegacyStageRequest;
	TestTrue(TEXT("Legacy trigger still resolves external viewpoint"),
		LegacyShop->TryBuildShopEntryViewStageRequest(LegacyStageRequest));
	TestTrue(TEXT("Legacy stage transform remains external"),
		LegacyStageRequest.ViewTransform.Equals(
			ExternalViewpoint->GetActorTransform()));
	TestEqual(TEXT("Legacy debug still resolves external host"),
		LegacyShop->GetShopTriggerDebugView(nullptr).WorldShopHostName,
		ExternalHost->GetName());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomWorldShopFormalActorDetailsPanelSafetySpec,
	"Wacom.UI.WorldShop.FormalActor.DetailsPanelSelectionSafety",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomWorldShopFormalActorDetailsPanelSafetySpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomWorldShopFormalActorSpec;

	FTransientWorldFixture Fixture;
	AWacomWorldShopActor* Shop = Fixture.Spawn<AWacomWorldShopActor>();
	if (!TestNotNull(TEXT("Formal shop spawns for details safety"), Shop))
	{
		return false;
	}

	FPropertyEditorModule& PropertyEditor =
		FModuleManager::LoadModuleChecked<FPropertyEditorModule>(
			TEXT("PropertyEditor"));
	FDetailsViewArgs DetailsArgs;
	DetailsArgs.bUpdatesFromSelection = false;
	DetailsArgs.bLockable = false;
	DetailsArgs.bAllowSearch = true;
	DetailsArgs.DefaultsOnlyVisibility =
		EEditDefaultsOnlyNodeVisibility::Hide;
	const TSharedRef<IDetailsView> DetailsView =
		PropertyEditor.CreateDetailView(DetailsArgs);
	DetailsView->SetIsPropertyVisibleDelegate(
		FIsPropertyVisible::CreateLambda(
			[](const FPropertyAndParent& PropertyAndParent)
			{
				return !PropertyAndParent.Property.HasAnyPropertyFlags(
					CPF_DisableEditOnInstance);
			}));
	DetailsView->SetObject(Shop);

	TestTrue(TEXT("Details panel can inspect formal shop without recursion"),
		IsValid(Shop));
	TestEqual(TEXT("Details panel keeps the selected formal shop"),
		DetailsView->GetSelectedObjects().Num(), 1);
	TestFalse(
		TEXT("Actor Details excludes inline World Shop component object graphs"),
		HasDisplayedWorldShopComponentReference(*DetailsView));

	const FProperty* RelativeLocationProperty =
		FindFProperty<FProperty>(
			USceneComponent::StaticClass(),
			USceneComponent::GetRelativeLocationPropertyName());
	const FProperty* RelativeRotationProperty =
		FindFProperty<FProperty>(
			USceneComponent::StaticClass(),
			USceneComponent::GetRelativeRotationPropertyName());
	const FProperty* RelativeScaleProperty =
		FindFProperty<FProperty>(
			USceneComponent::StaticClass(),
			USceneComponent::GetRelativeScale3DPropertyName());
	const FProperty* SlotIdProperty =
		FindFProperty<FProperty>(
			UWacomWorldShopOfferAnchorComponent::StaticClass(),
			GET_MEMBER_NAME_CHECKED(
				UWacomWorldShopOfferAnchorComponent,
				SlotId));
	const FProperty* SlotOrderProperty =
		FindFProperty<FProperty>(
			UWacomWorldShopOfferAnchorComponent::StaticClass(),
			GET_MEMBER_NAME_CHECKED(
				UWacomWorldShopOfferAnchorComponent,
				SlotOrder));
	const FProperty* EnabledProperty =
		FindFProperty<FProperty>(
			UWacomWorldShopOfferAnchorComponent::StaticClass(),
			GET_MEMBER_NAME_CHECKED(
				UWacomWorldShopOfferAnchorComponent,
				bEnabledForOffers));

	TestNotNull(TEXT("Relative location property resolves"), RelativeLocationProperty);
	TestNotNull(TEXT("Relative rotation property resolves"), RelativeRotationProperty);
	TestNotNull(TEXT("Relative scale property resolves"), RelativeScaleProperty);
	TestNotNull(TEXT("Slot id property resolves"), SlotIdProperty);
	TestNotNull(TEXT("Slot order property resolves"), SlotOrderProperty);
	TestNotNull(TEXT("Enabled property resolves"), EnabledProperty);

	const TArray<UWacomWorldShopLayoutAnchorComponent*> LayoutAnchors =
		Shop->GetOfferLayoutAnchorsSorted();
	TestEqual(TEXT("Details safety fixture exposes eight anchors"),
		LayoutAnchors.Num(), 8);
	for (int32 Index = 0; Index < LayoutAnchors.Num(); ++Index)
	{
		UWacomWorldShopLayoutAnchorComponent* Anchor =
			LayoutAnchors[Index];
		if (!TestNotNull(
			*FString::Printf(
				TEXT("Details anchor %d exists"),
				Index + 1),
			Anchor))
		{
			continue;
		}

		const FProperty* AnchorProperty =
			FComponentEditorUtils::GetPropertyForEditableNativeComponent(
				Anchor);
		TestNotNull(
			*FString::Printf(
				TEXT("Details anchor %d is editor-resolvable"),
				Index + 1),
			AnchorProperty);

		DetailsView->SetObject(Anchor, true);
		TestEqual(
			*FString::Printf(
				TEXT("Details panel selects anchor %d"),
				Index + 1),
			DetailsView->GetSelectedObjects().Num(),
			1);
		TestTrue(
			*FString::Printf(
				TEXT("Anchor %d location remains editable"),
				Index + 1),
			RelativeLocationProperty
				&& Anchor->CanEditChange(RelativeLocationProperty));
		TestTrue(
			*FString::Printf(
				TEXT("Anchor %d rotation remains editable"),
				Index + 1),
			RelativeRotationProperty
				&& Anchor->CanEditChange(RelativeRotationProperty));
		TestFalse(
			*FString::Printf(
				TEXT("Anchor %d per-slot scale is locked"),
				Index + 1),
			RelativeScaleProperty
				&& Anchor->CanEditChange(RelativeScaleProperty));
		TestFalse(
			*FString::Printf(
				TEXT("Anchor %d slot id is locked"),
				Index + 1),
			SlotIdProperty
				&& Anchor->CanEditChange(SlotIdProperty));
		TestFalse(
			*FString::Printf(
				TEXT("Anchor %d slot order is locked"),
				Index + 1),
			SlotOrderProperty
				&& Anchor->CanEditChange(SlotOrderProperty));
		TestFalse(
			*FString::Printf(
				TEXT("Anchor %d enabled flag is locked"),
				Index + 1),
			EnabledProperty
				&& Anchor->CanEditChange(EnabledProperty));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomWorldShopFormalActorLevelInstanceDetailsPanelSafetySpec,
	"Wacom.UI.WorldShop.FormalActor.LevelInstanceDetailsPanelSafety",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomWorldShopFormalActorLevelInstanceDetailsPanelSafetySpec::RunTest(
	const FString& /*Parameters*/)
{
	UWorld* ExplorationWorld = LoadObject<UWorld>(
		nullptr,
		TEXT("/Game/Wacom/Maps/L_Exploration.L_Exploration"));
	if (!TestNotNull(TEXT("L_Exploration loads for level details safety"),
		ExplorationWorld)
		|| !TestNotNull(TEXT("L_Exploration has a persistent level"),
			ExplorationWorld
				? ExplorationWorld->PersistentLevel.Get()
				: nullptr))
	{
		return false;
	}

	AWacomWorldShopActor* FormalShop = nullptr;
	for (AActor* Actor : ExplorationWorld->PersistentLevel->Actors)
	{
		if (AWacomWorldShopActor* Candidate =
			Cast<AWacomWorldShopActor>(Actor))
		{
			FormalShop = Candidate;
			break;
		}
	}
	if (!TestNotNull(TEXT("L_Exploration formal shop exists"), FormalShop))
	{
		return false;
	}

	FPropertyEditorModule& PropertyEditor =
		FModuleManager::LoadModuleChecked<FPropertyEditorModule>(
			TEXT("PropertyEditor"));
	FDetailsViewArgs DetailsArgs;
	DetailsArgs.bUpdatesFromSelection = false;
	DetailsArgs.bLockable = false;
	DetailsArgs.bAllowSearch = true;
	DetailsArgs.DefaultsOnlyVisibility =
		EEditDefaultsOnlyNodeVisibility::Hide;
	const TSharedRef<IDetailsView> DetailsView =
		PropertyEditor.CreateDetailView(DetailsArgs);
	DetailsView->SetIsPropertyVisibleDelegate(
		FIsPropertyVisible::CreateLambda(
			[](const FPropertyAndParent& PropertyAndParent)
			{
				return !PropertyAndParent.Property.HasAnyPropertyFlags(
					CPF_DisableEditOnInstance);
			}));
	DetailsView->SetObject(FormalShop);

	TestEqual(TEXT("Level details panel keeps the selected formal shop"),
		DetailsView->GetSelectedObjects().Num(), 1);
	TestFalse(
		TEXT("Level actor Details excludes inline World Shop component object graphs"),
		WacomWorldShopFormalActorSpec::
			HasDisplayedWorldShopComponentReference(*DetailsView));

	TInlineComponentArray<UActorComponent*> Components(FormalShop);
	int32 LayoutAnchorCount = 0;
	for (const UActorComponent* Component : Components)
	{
		if (!Component
			|| !Component->GetName().StartsWith(
				TEXT("OfferLayoutAnchor_")))
		{
			continue;
		}

		++LayoutAnchorCount;
		TestTrue(
			*FString::Printf(
				TEXT("%s resolves to the non-primitive layout anchor class"),
				*Component->GetName()),
			Component->IsA<UWacomWorldShopLayoutAnchorComponent>());
		TestFalse(
			*FString::Printf(
				TEXT("%s carries no BodyInstance"),
				*Component->GetName()),
			Component->IsA<UPrimitiveComponent>());
	}
	TestEqual(
		TEXT("Level instance has exactly eight resolved layout anchors"),
		LayoutAnchorCount,
		8);
	return true;
}

#endif
