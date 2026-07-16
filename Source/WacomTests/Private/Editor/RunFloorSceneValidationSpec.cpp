// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Editor/RunFloorSceneValidationTestFixture.h"

using namespace WacomRunFloorSceneValidationTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunFloorSceneDescriptorDiagnosticSpec,
	"Wacom.Editor.RunSceneValidation.Identity.DescriptorMatrix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunFloorSceneDescriptorDiagnosticSpec::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Null World is a stable error"),
		HasCode(FWacomRunSceneBindingValidation::ValidateLoadedWorld(nullptr),
			EWacomRunSceneBindingDiagnosticCode::WorldInvalid));

	{
		FFixture Fixture;
		Fixture.World->DestroyActor(Fixture.Descriptor);
		TestTrue(TEXT("Missing Descriptor"),
			HasCode(FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World),
				EWacomRunSceneBindingDiagnosticCode::DescriptorMissing));
	}
	{
		FFixture Fixture;
		AWacomRunFloorSceneDescriptorActor* Duplicate =
			Fixture.World->SpawnActor<AWacomRunFloorSceneDescriptorActor>();
		Duplicate->FloorDefinition = Fixture.Floor;
		TestTrue(TEXT("Duplicate Descriptor"),
			HasCode(FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World),
				EWacomRunSceneBindingDiagnosticCode::DescriptorDuplicate));
	}
	{
		FFixture Fixture;
		Fixture.Descriptor->FloorDefinition = nullptr;
		TestTrue(TEXT("Descriptor null Floor"),
			HasCode(FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World),
				EWacomRunSceneBindingDiagnosticCode::DescriptorFloorMissing));
	}
	{
		FFixture Fixture;
		Fixture.Floor->FloorId = NAME_None;
		TestTrue(TEXT("Descriptor Floor identity missing"),
			HasCode(FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World),
				EWacomRunSceneBindingDiagnosticCode::DescriptorFloorIdentityMissing));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunFloorSceneIdentityDiagnosticSpec,
	"Wacom.Editor.RunSceneValidation.Identity.SceneBindingMatrix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunFloorSceneIdentityDiagnosticSpec::RunTest(const FString& Parameters)
{
	{
		FFixture Fixture;
		Fixture.World->DestroyActor(Fixture.EventAnchor);
		TestTrue(TEXT("Missing Anchor"), HasCode(
			FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World),
			EWacomRunSceneBindingDiagnosticCode::NodeAnchorMissing));
	}
	{
		FFixture Fixture;
		Fixture.SpawnAnchor(TEXT("Event"), FVector(1200.0, 0.0, 0.0));
		TestTrue(TEXT("Duplicate Anchor"), HasCode(
			FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World),
			EWacomRunSceneBindingDiagnosticCode::NodeAnchorDuplicate));
	}
	{
		FFixture Fixture;
		Fixture.SpawnAnchor(TEXT("Unexpected"), FVector::ZeroVector);
		TestTrue(TEXT("Unexpected Anchor"), HasCode(
			FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World),
			EWacomRunSceneBindingDiagnosticCode::NodeAnchorUnexpected));
	}
	{
		FFixture Fixture;
		Fixture.World->DestroyActor(Fixture.Path);
		TestTrue(TEXT("Missing Path"), HasCode(
			FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World),
			EWacomRunSceneBindingDiagnosticCode::EdgePathMissing));
	}
	{
		FFixture Fixture;
		Fixture.SpawnPath(TEXT("EntryToEvent"), FVector::ZeroVector,
			FVector(1000.0, 0.0, 0.0));
		TestTrue(TEXT("Duplicate Path"), HasCode(
			FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World),
			EWacomRunSceneBindingDiagnosticCode::EdgePathDuplicate));
	}
	{
		FFixture Fixture;
		Fixture.SpawnPath(TEXT("UnexpectedEdge"), FVector::ZeroVector,
			FVector(1000.0, 0.0, 0.0));
		TestTrue(TEXT("Unexpected Path"), HasCode(
			FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World),
			EWacomRunSceneBindingDiagnosticCode::EdgePathUnexpected));
	}
	{
		FFixture Fixture;
		AWacomRunPathBranchTargetActor* Branch =
			Fixture.World->SpawnActor<AWacomRunPathBranchTargetActor>();
		Branch->EdgeId = TEXT("EntryToEvent");
		TestTrue(TEXT("Single-exit Branch is unexpected"), HasCode(
			FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World),
			EWacomRunSceneBindingDiagnosticCode::BranchTargetUnexpected));
	}
	{
		FFixture Fixture;
		Fixture.AddSecondExit(false);
		TestEqual(TEXT("Both multi-exit Branch targets are required"), CountCode(
			FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World),
			EWacomRunSceneBindingDiagnosticCode::BranchTargetMissing), 2);
	}
	{
		FFixture Fixture;
		Fixture.AddSecondExit(true);
		AWacomRunPathBranchTargetActor* Duplicate =
			Fixture.World->SpawnActor<AWacomRunPathBranchTargetActor>();
		Duplicate->EdgeId = TEXT("EntryToEvent");
		TestTrue(TEXT("Duplicate Branch"), HasCode(
			FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World),
			EWacomRunSceneBindingDiagnosticCode::BranchTargetDuplicate));
	}
	{
		FFixture Fixture;
		Fixture.World->DestroyActor(Fixture.Host);
		TestTrue(TEXT("Missing Host"), HasCode(
			FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World),
			EWacomRunSceneBindingDiagnosticCode::ContentHostMissing));
	}
	{
		FFixture Fixture;
		UWacomRunMapNodeBindingComponent* DuplicateBinding = nullptr;
		Fixture.SpawnHost(TEXT("Event"), EWacomMapNodeType::RunEvent, DuplicateBinding);
		TestTrue(TEXT("Duplicate Host"), HasCode(
			FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World),
			EWacomRunSceneBindingDiagnosticCode::ContentHostDuplicate));
	}
	{
		FFixture Fixture;
		UWacomRunMapNodeBindingComponent* UnexpectedBinding = nullptr;
		Fixture.SpawnHost(TEXT("Unexpected"), EWacomMapNodeType::RunEvent,
			UnexpectedBinding);
		TestTrue(TEXT("Unexpected Host"), HasCode(
			FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World),
			EWacomRunSceneBindingDiagnosticCode::ContentHostUnexpected));
		Fixture.HostBinding->NodeType = EWacomMapNodeType::Shop;
		TestTrue(TEXT("Host type mismatch"), HasCode(
			FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World),
			EWacomRunSceneBindingDiagnosticCode::ContentHostTypeMismatch));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunFloorSceneDiagnosticSortSpec,
	"Wacom.Editor.RunSceneValidation.Identity.StableDiagnosticSort",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunFloorSceneDiagnosticSortSpec::RunTest(const FString& Parameters)
{
	FFixture Fixture;
	Fixture.SpawnAnchor(TEXT("Z.Unexpected"), FVector::ZeroVector);
	Fixture.SpawnAnchor(TEXT("A.Unexpected"), FVector::ZeroVector);
	Fixture.World->DestroyActor(Fixture.EventAnchor);
	Fixture.World->DestroyActor(Fixture.Host);
	const FWacomRunSceneBindingValidationReport First =
		FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World);
	const FWacomRunSceneBindingValidationReport Second =
		FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World);
	TestEqual(TEXT("Repeated validation has the same diagnostic count"),
		First.Diagnostics.Num(), Second.Diagnostics.Num());
	for (int32 Index = 0; Index < First.Diagnostics.Num(); ++Index)
	{
		const FWacomRunSceneBindingDiagnostic& A = First.Diagnostics[Index];
		const FWacomRunSceneBindingDiagnostic& B = Second.Diagnostics[Index];
		TestEqual(TEXT("Stable severity"), A.Severity, B.Severity);
		TestEqual(TEXT("Stable code"), A.Code, B.Code);
		TestEqual(TEXT("Stable object path"), A.ObjectPath, B.ObjectPath);
		TestEqual(TEXT("Stable message"), A.Message.ToString(), B.Message.ToString());
		if (Index == 0)
		{
			continue;
		}
		const FWacomRunSceneBindingDiagnostic& Previous = First.Diagnostics[Index - 1];
		const bool bSorted = Previous.Severity < A.Severity
			|| (Previous.Severity == A.Severity && Previous.Code < A.Code)
			|| (Previous.Severity == A.Severity && Previous.Code == A.Code
				&& Previous.ObjectPath <= A.ObjectPath);
		TestTrue(TEXT("Diagnostics use Severity/Code/ObjectPath ordering"), bSorted);
	}
	return true;
}

#endif
