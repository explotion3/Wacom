// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Editor/RunFloorSceneValidationTestFixture.h"

using namespace WacomRunFloorSceneValidationTests;

namespace
{
	struct FReadOnlyState
	{
		bool bWorldPackageDirty = false;
		bool bFloorPackageDirty = false;
		FName FloorId = NAME_None;
		FName EntryNodeId = NAME_None;
		TArray<FName> NodeIds;
		TArray<FName> EdgeIds;
		const UWacomFloorMapDefinition* DescriptorFloor = nullptr;
		FName EntryAnchorId = NAME_None;
		FTransform EntryTransform;
		FName EventAnchorId = NAME_None;
		FTransform EventTransform;
		FName PathEdgeId = NAME_None;
		TArray<FTransform> SplinePointTransforms;
		FName HostNodeId = NAME_None;
		EWacomMapNodeType HostNodeType = EWacomMapNodeType::Navigation;

		static FReadOnlyState Capture(const FFixture& Fixture)
		{
			FReadOnlyState State;
			State.bWorldPackageDirty = Fixture.World->GetPackage()->IsDirty();
			State.bFloorPackageDirty = Fixture.Floor->GetPackage()->IsDirty();
			State.FloorId = Fixture.Floor->FloorId;
			State.EntryNodeId = Fixture.Floor->EntryNodeId;
			for (const FWacomMapNodeDefinition& Node : Fixture.Floor->Nodes)
				State.NodeIds.Add(Node.NodeId);
			for (const FWacomMapEdgeDefinition& Edge : Fixture.Floor->Edges)
				State.EdgeIds.Add(Edge.EdgeId);
			State.DescriptorFloor = Fixture.Descriptor->GetFloorDefinition();
			State.EntryAnchorId = Fixture.EntryAnchor->NodeId;
			State.EntryTransform = Fixture.EntryAnchor->GetActorTransform();
			State.EventAnchorId = Fixture.EventAnchor->NodeId;
			State.EventTransform = Fixture.EventAnchor->GetActorTransform();
			State.PathEdgeId = Fixture.Path->EdgeId;
			USplineComponent* Spline = Fixture.Path->GetPathSpline();
			for (int32 Index = 0; Index < Spline->GetNumberOfSplinePoints(); ++Index)
			{
				State.SplinePointTransforms.Add(Spline->GetTransformAtSplinePoint(
					Index, ESplineCoordinateSpace::World, true));
			}
			State.HostNodeId = Fixture.HostBinding->NodeId;
			State.HostNodeType = Fixture.HostBinding->NodeType;
			return State;
		}

		bool Equals(const FReadOnlyState& Other) const
		{
			if (bWorldPackageDirty != Other.bWorldPackageDirty
				|| bFloorPackageDirty != Other.bFloorPackageDirty
				|| FloorId != Other.FloorId
				|| EntryNodeId != Other.EntryNodeId
				|| NodeIds != Other.NodeIds
				|| EdgeIds != Other.EdgeIds
				|| DescriptorFloor != Other.DescriptorFloor
				|| EntryAnchorId != Other.EntryAnchorId
				|| !EntryTransform.Equals(Other.EntryTransform)
				|| EventAnchorId != Other.EventAnchorId
				|| !EventTransform.Equals(Other.EventTransform)
				|| PathEdgeId != Other.PathEdgeId
				|| SplinePointTransforms.Num() != Other.SplinePointTransforms.Num()
				|| HostNodeId != Other.HostNodeId
				|| HostNodeType != Other.HostNodeType)
			{
				return false;
			}
			for (int32 Index = 0; Index < SplinePointTransforms.Num(); ++Index)
			{
				if (!SplinePointTransforms[Index].Equals(Other.SplinePointTransforms[Index]))
					return false;
			}
			return true;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunFloorSceneValidationReadOnlySpec,
	"Wacom.Editor.RunSceneValidation.ReadOnly.ValidAndInvalidWorlds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunFloorSceneValidationReadOnlySpec::RunTest(const FString& Parameters)
{
	{
		FFixture Fixture;
		const FReadOnlyState Before = FReadOnlyState::Capture(Fixture);
		const FWacomRunSceneBindingValidationReport Report =
			FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World);
		TestTrue(TEXT("Valid fixture validates"), Report.IsValid());
		TestTrue(TEXT("Valid validation is strictly read-only"),
			Before.Equals(FReadOnlyState::Capture(Fixture)));
	}
	{
		FFixture Fixture;
		Fixture.Path->EdgeId = TEXT("UnexpectedEdge");
		const FReadOnlyState Before = FReadOnlyState::Capture(Fixture);
		const FWacomRunSceneBindingValidationReport Report =
			FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World);
		TestFalse(TEXT("Invalid fixture fails"), Report.IsValid());
		TestTrue(TEXT("Invalid validation is strictly read-only"),
			Before.Equals(FReadOnlyState::Capture(Fixture)));
	}
	return true;
}

#endif
