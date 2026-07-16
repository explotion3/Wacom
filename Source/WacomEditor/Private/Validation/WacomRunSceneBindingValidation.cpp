// Copyright Wacom. All Rights Reserved.

#include "Validation/WacomRunSceneBindingValidation.h"

#include "Actors/WacomRunFloorSceneDescriptorActor.h"
#include "Actors/WacomRunMapNodeAnchorActor.h"
#include "Actors/WacomRunPathBranchTargetActor.h"
#include "Actors/WacomRunPathSegmentActor.h"
#include "Cards/CardDefinition.h"
#include "Components/SplineComponent.h"
#include "Components/WacomRunMapNodeBindingComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Encounters/EncounterDefinition.h"
#include "Events/RunEventDefinition.h"
#include "Interactions/RunWorldCardInteractionDefinition.h"
#include "Map/WacomFloorMapDefinition.h"
#include "Pickups/RunPickupDefinition.h"
#include "Shops/ShopDefinition.h"
#include "UObject/UnrealType.h"

namespace
{
	constexpr double MinimumSplineLengthCm = 10.0;
	constexpr double EndpointPassDistanceCm = 100.0;
	constexpr double EndpointWarningDistanceCm = 300.0;
	constexpr double ReversedDirectionAdvantageCm = 1.0;

	void AddDiagnostic(
		FWacomRunSceneBindingValidationReport& Report,
		const EWacomRunSceneBindingDiagnosticSeverity Severity,
		const EWacomRunSceneBindingDiagnosticCode Code,
		FString ObjectPath,
		const FString& Message)
	{
		FWacomRunSceneBindingDiagnostic& Diagnostic =
			Report.Diagnostics.AddDefaulted_GetRef();
		Diagnostic.Severity = Severity;
		Diagnostic.Code = Code;
		Diagnostic.ObjectPath = MoveTemp(ObjectPath);
		Diagnostic.Message = FText::FromString(Message);
	}

	void AddError(
		FWacomRunSceneBindingValidationReport& Report,
		const EWacomRunSceneBindingDiagnosticCode Code,
		FString ObjectPath,
		const FString& Message)
	{
		AddDiagnostic(Report, EWacomRunSceneBindingDiagnosticSeverity::Error,
			Code, MoveTemp(ObjectPath), Message);
	}

	FString MakeIdentityPath(
		const UObject& Floor,
		const TCHAR* Kind,
		const FName Identity)
	{
		return FString::Printf(TEXT("%s#%s=%s"), *Floor.GetPathName(), Kind,
			*Identity.ToString());
	}

	bool IsSupportedWorldType(const EWorldType::Type WorldType)
	{
		switch (WorldType)
		{
		case EWorldType::None:
		case EWorldType::Game:
		case EWorldType::PIE:
		case EWorldType::Editor:
		case EWorldType::EditorPreview:
		case EWorldType::GamePreview:
		case EWorldType::Inactive:
			return true;
		default:
			return false;
		}
	}

	bool RequiresHost(const EWacomMapNodeType NodeType)
	{
		return NodeType != EWacomMapNodeType::Navigation;
	}

	void AddAllowedContentObjects(
		const FWacomMapNodeDefinition& Node,
		TSet<const UObject*>& OutAllowed)
	{
		switch (Node.NodeType)
		{
		case EWacomMapNodeType::Encounter:
			OutAllowed.Add(Node.Content.Encounter.EncounterDefinition.Get());
			break;
		case EWacomMapNodeType::RunEvent:
			OutAllowed.Add(Node.Content.RunEvent.RunEventDefinition.Get());
			break;
		case EWacomMapNodeType::Shop:
			OutAllowed.Add(Node.Content.Shop.ShopDefinition.Get());
			break;
		case EWacomMapNodeType::Treasure:
			OutAllowed.Add(Node.Content.Treasure.PickupDefinition.Get());
			OutAllowed.Add(Node.Content.Treasure.WorldCardInteractionDefinition.Get());
			if (Node.Content.Treasure.PickupDefinition)
			{
				OutAllowed.Add(
					Node.Content.Treasure.PickupDefinition->CardDefinition.Get());
			}
			if (Node.Content.Treasure.WorldCardInteractionDefinition)
			{
				for (const FWacomRunWorldCardInteractionReward& Reward :
					Node.Content.Treasure.WorldCardInteractionDefinition->Rewards)
				{
					OutAllowed.Add(Reward.CardDefinition.Get());
				}
			}
			break;
		default:
			break;
		}
		OutAllowed.Remove(nullptr);
	}

	void ValidatePersistedContentDefinition(
		const AActor& Host,
		const FWacomMapNodeDefinition& Node,
		FWacomRunSceneBindingValidationReport& Report)
	{
		static const TArray<FName> LegacyDefinitionPropertyNames =
		{
			TEXT("EncounterDefinition"),
			TEXT("EventDefinition"),
			TEXT("ShopDefinition"),
			TEXT("PickupDefinition"),
			TEXT("InteractionDefinition"),
			TEXT("CardInteractionDefinition"),
			TEXT("CardDefinition"),
		};

		TSet<const UObject*> AllowedObjects;
		AddAllowedContentObjects(Node, AllowedObjects);
		for (const FName PropertyName : LegacyDefinitionPropertyNames)
		{
			const FObjectPropertyBase* Property =
				FindFProperty<FObjectPropertyBase>(Host.GetClass(), PropertyName);
			const UObject* PersistedObject = Property
				? Property->GetObjectPropertyValue_InContainer(&Host)
				: nullptr;
			if (PersistedObject && !AllowedObjects.Contains(PersistedObject))
			{
				AddError(Report,
					EWacomRunSceneBindingDiagnosticCode::ContentHostPayloadMismatch,
					Host.GetPathName(),
					FString::Printf(
						TEXT("Content Host %s 的 %s=%s 与 Floor 节点 %s 的 typed payload 不一致。"),
						*Host.GetName(), *PropertyName.ToString(),
						*GetNameSafe(PersistedObject), *Node.NodeId.ToString()));
			}
		}
	}

	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X)
			&& FMath::IsFinite(Value.Y)
			&& FMath::IsFinite(Value.Z);
	}

	bool IsFiniteQuat(const FQuat& Value)
	{
		return FMath::IsFinite(Value.X)
			&& FMath::IsFinite(Value.Y)
			&& FMath::IsFinite(Value.Z)
			&& FMath::IsFinite(Value.W);
	}

	bool IsFiniteTransform(const FTransform& Transform)
	{
		return IsFiniteVector(Transform.GetLocation())
			&& IsFiniteVector(Transform.GetScale3D())
			&& IsFiniteQuat(Transform.GetRotation());
	}

	void AddEndpointDiagnostic(
		FWacomRunSceneBindingValidationReport& Report,
		const AActor& Path,
		const double Distance,
		const bool bSource)
	{
		if (Distance <= EndpointPassDistanceCm)
		{
			return;
		}
		const TCHAR* Endpoint = bSource ? TEXT("Source") : TEXT("Target");
		if (Distance <= EndpointWarningDistanceCm)
		{
			AddDiagnostic(
				Report,
				EWacomRunSceneBindingDiagnosticSeverity::Warning,
				bSource
					? EWacomRunSceneBindingDiagnosticCode::SplineSourceEndpointWarning
					: EWacomRunSceneBindingDiagnosticCode::SplineTargetEndpointWarning,
				Path.GetPathName(),
				FString::Printf(TEXT("Spline %s 端点距离期望 Anchor %.2fcm，超过 100cm。"),
					Endpoint, Distance));
			return;
		}
		AddError(
			Report,
			bSource
				? EWacomRunSceneBindingDiagnosticCode::SplineSourceEndpointError
				: EWacomRunSceneBindingDiagnosticCode::SplineTargetEndpointError,
			Path.GetPathName(),
			FString::Printf(TEXT("Spline %s 端点距离期望 Anchor %.2fcm，超过 300cm。"),
				Endpoint, Distance));
	}

	void ValidatePathGeometry(
		const AWacomRunPathSegmentActor& Path,
		const FWacomMapEdgeDefinition& Edge,
		const TMap<FName, TArray<const AWacomRunMapNodeAnchorActor*>>& AnchorsById,
		FWacomRunSceneBindingValidationReport& Report)
	{
		const USplineComponent* Spline = Path.GetPathSpline();
		const int32 PointCount = Spline ? Spline->GetNumberOfSplinePoints() : 0;
		if (PointCount < 2)
		{
			AddError(Report,
				EWacomRunSceneBindingDiagnosticCode::SplinePointCountInvalid,
				Path.GetPathName(),
				FString::Printf(TEXT("Path %s 的 Spline 至少需要 2 个点，当前数量=%d。"),
					*Path.EdgeId.ToString(), PointCount));
			return;
		}

		if (Spline->GetSplineLength() <= MinimumSplineLengthCm)
		{
			AddError(Report,
				EWacomRunSceneBindingDiagnosticCode::SplineLengthTooShort,
				Path.GetPathName(),
				FString::Printf(TEXT("Path %s 的 Spline 长度必须大于 10cm，当前=%.2fcm。"),
					*Path.EdgeId.ToString(), Spline->GetSplineLength()));
		}

		bool bAllPointTransformsFinite = true;
		for (int32 Index = 0; Index < PointCount; ++Index)
		{
			const FTransform PointTransform = Spline->GetTransformAtSplinePoint(
				Index, ESplineCoordinateSpace::World, true);
			if (!IsFiniteTransform(PointTransform))
			{
				bAllPointTransformsFinite = false;
				AddError(Report,
					EWacomRunSceneBindingDiagnosticCode::SplineTransformNonFinite,
					FString::Printf(TEXT("%s#SplinePoint=%d"), *Path.GetPathName(), Index),
					FString::Printf(TEXT("Path %s 的 Spline 点 %d 包含非有限 Transform。"),
						*Path.EdgeId.ToString(), Index));
			}
		}
		if (!bAllPointTransformsFinite)
		{
			return;
		}

		const TArray<const AWacomRunMapNodeAnchorActor*>* SourceAnchors =
			AnchorsById.Find(Edge.FromNodeId);
		const TArray<const AWacomRunMapNodeAnchorActor*>* TargetAnchors =
			AnchorsById.Find(Edge.ToNodeId);
		if (!SourceAnchors || SourceAnchors->Num() != 1
			|| !TargetAnchors || TargetAnchors->Num() != 1)
		{
			return;
		}

		const FVector Start = Spline->GetLocationAtSplinePoint(
			0, ESplineCoordinateSpace::World);
		const FVector End = Spline->GetLocationAtSplinePoint(
			PointCount - 1, ESplineCoordinateSpace::World);
		const FVector Source = (*SourceAnchors)[0]->GetActorLocation();
		const FVector Target = (*TargetAnchors)[0]->GetActorLocation();
		if (!IsFiniteVector(Start) || !IsFiniteVector(End)
			|| !IsFiniteVector(Source) || !IsFiniteVector(Target))
		{
			AddError(Report,
				EWacomRunSceneBindingDiagnosticCode::SplineTransformNonFinite,
				Path.GetPathName(),
				FString::Printf(TEXT("Path %s 的端点或期望 Anchor 包含非有限位置。"),
					*Path.EdgeId.ToString()));
			return;
		}

		const double SourceDistance = FVector::Distance(Start, Source);
		const double TargetDistance = FVector::Distance(End, Target);
		const double DirectDistance = SourceDistance + TargetDistance;
		const double ReversedDistance =
			FVector::Distance(Start, Target) + FVector::Distance(End, Source);
		if (ReversedDistance + ReversedDirectionAdvantageCm <= DirectDistance)
		{
			AddError(Report,
				EWacomRunSceneBindingDiagnosticCode::SplineDirectionReversed,
				Path.GetPathName(),
				FString::Printf(TEXT("Path %s 的 Spline 方向与 Floor Edge 源/目标相反。"),
					*Path.EdgeId.ToString()));
		}
		AddEndpointDiagnostic(Report, Path, SourceDistance, true);
		AddEndpointDiagnostic(Report, Path, TargetDistance, false);
	}
}

bool FWacomRunSceneBindingValidationReport::HasErrors() const
{
	return Diagnostics.ContainsByPredicate(
		[](const FWacomRunSceneBindingDiagnostic& Diagnostic)
		{
			return Diagnostic.Severity ==
				EWacomRunSceneBindingDiagnosticSeverity::Error;
		});
}

bool FWacomRunSceneBindingValidationReport::HasCode(
	const EWacomRunSceneBindingDiagnosticCode Code) const
{
	return Diagnostics.ContainsByPredicate(
		[Code](const FWacomRunSceneBindingDiagnostic& Diagnostic)
		{
			return Diagnostic.Code == Code;
		});
}

bool FWacomRunSceneBindingValidationReport::HasDescriptorResolutionError() const
{
	return HasCode(EWacomRunSceneBindingDiagnosticCode::WorldInvalid)
		|| HasCode(EWacomRunSceneBindingDiagnosticCode::WorldTypeUnsupported)
		|| HasCode(EWacomRunSceneBindingDiagnosticCode::DescriptorMissing)
		|| HasCode(EWacomRunSceneBindingDiagnosticCode::DescriptorDuplicate)
		|| HasCode(EWacomRunSceneBindingDiagnosticCode::DescriptorFloorMissing)
		|| HasCode(
			EWacomRunSceneBindingDiagnosticCode::DescriptorFloorIdentityMissing);
}

void FWacomRunSceneBindingValidationReport::Sort()
{
	Diagnostics.Sort(
		[](const FWacomRunSceneBindingDiagnostic& A,
			const FWacomRunSceneBindingDiagnostic& B)
		{
			if (A.Severity != B.Severity)
				return A.Severity < B.Severity;
			if (A.Code != B.Code)
				return A.Code < B.Code;
			if (A.ObjectPath != B.ObjectPath)
				return A.ObjectPath < B.ObjectPath;
			return A.Message.ToString() < B.Message.ToString();
		});
}

FWacomRunSceneBindingValidationReport FWacomRunSceneBindingValidation::ValidateLoadedWorld(
	const UWorld* World)
{
	FWacomRunSceneBindingValidationReport Report;
	if (!World)
	{
		AddError(Report, EWacomRunSceneBindingDiagnosticCode::WorldInvalid,
			TEXT("None"), TEXT("Loaded World 为空。"));
		Report.Sort();
		return Report;
	}
	if (!IsSupportedWorldType(World->WorldType))
	{
		AddError(Report,
			EWacomRunSceneBindingDiagnosticCode::WorldTypeUnsupported,
			World->GetPathName(),
			FString::Printf(TEXT("World 类型 %d 不支持 Run Floor 场景验证。"),
				static_cast<int32>(World->WorldType)));
		Report.Sort();
		return Report;
	}

	TArray<const AWacomRunFloorSceneDescriptorActor*> Descriptors;
	for (TActorIterator<AWacomRunFloorSceneDescriptorActor> It(
		const_cast<UWorld*>(World)); It; ++It)
	{
		Descriptors.Add(*It);
	}
	if (Descriptors.IsEmpty())
	{
		AddError(Report,
			EWacomRunSceneBindingDiagnosticCode::DescriptorMissing,
			World->GetPathName(), TEXT("World 缺少 Run Floor Scene Descriptor。"));
		Report.Sort();
		return Report;
	}
	if (Descriptors.Num() != 1)
	{
		AddError(Report,
			EWacomRunSceneBindingDiagnosticCode::DescriptorDuplicate,
			World->GetPathName(),
			FString::Printf(TEXT("World 必须且只能有一个 Run Floor Scene Descriptor，当前数量=%d。"),
				Descriptors.Num()));
		Report.Sort();
		return Report;
	}

	const AWacomRunFloorSceneDescriptorActor* Descriptor = Descriptors[0];
	const UWacomFloorMapDefinition* FloorDefinition =
		Descriptor->GetFloorDefinition();
	if (!FloorDefinition)
	{
		AddError(Report,
			EWacomRunSceneBindingDiagnosticCode::DescriptorFloorMissing,
			Descriptor->GetPathName(), TEXT("Run Floor Scene Descriptor 的 FloorDefinition 为空。"));
		Report.Sort();
		return Report;
	}
	if (FloorDefinition->FloorId.IsNone())
	{
		AddError(Report,
			EWacomRunSceneBindingDiagnosticCode::DescriptorFloorIdentityMissing,
			FloorDefinition->GetPathName(), TEXT("Descriptor 引用的 FloorDefinition 缺少 FloorId。"));
		Report.Sort();
		return Report;
	}

	TMap<FName, TArray<const AWacomRunMapNodeAnchorActor*>> AnchorsById;
	for (TActorIterator<AWacomRunMapNodeAnchorActor> It(
		const_cast<UWorld*>(World)); It; ++It)
	{
		if (It->NodeId.IsNone() || !FloorDefinition->FindNode(It->NodeId))
		{
			AddError(Report,
				EWacomRunSceneBindingDiagnosticCode::NodeAnchorUnexpected,
				It->GetPathName(),
				FString::Printf(TEXT("NodeAnchor %s 使用未声明 NodeId：%s。"),
					*It->GetName(), *It->NodeId.ToString()));
			continue;
		}
		AnchorsById.FindOrAdd(It->NodeId).Add(*It);
	}
	for (const FWacomMapNodeDefinition& Node : FloorDefinition->Nodes)
	{
		const int32 Count = AnchorsById.FindRef(Node.NodeId).Num();
		if (Count == 0)
		{
			AddError(Report, EWacomRunSceneBindingDiagnosticCode::NodeAnchorMissing,
				MakeIdentityPath(*FloorDefinition, TEXT("Node"), Node.NodeId),
				FString::Printf(TEXT("Floor 节点 %s 缺少 NodeAnchor。"),
					*Node.NodeId.ToString()));
		}
		else if (Count > 1)
		{
			AddError(Report, EWacomRunSceneBindingDiagnosticCode::NodeAnchorDuplicate,
				MakeIdentityPath(*FloorDefinition, TEXT("Node"), Node.NodeId),
				FString::Printf(TEXT("Floor 节点 %s 的 NodeAnchor 重复，当前数量=%d。"),
					*Node.NodeId.ToString(), Count));
		}
	}

	TMap<FName, TArray<const AWacomRunPathSegmentActor*>> PathsById;
	for (TActorIterator<AWacomRunPathSegmentActor> It(
		const_cast<UWorld*>(World)); It; ++It)
	{
		if (It->EdgeId.IsNone() || !FloorDefinition->FindEdge(It->EdgeId))
		{
			AddError(Report,
				EWacomRunSceneBindingDiagnosticCode::EdgePathUnexpected,
				It->GetPathName(),
				FString::Printf(TEXT("PathSegment %s 使用未声明 EdgeId：%s。"),
					*It->GetName(), *It->EdgeId.ToString()));
			continue;
		}
		PathsById.FindOrAdd(It->EdgeId).Add(*It);
	}
	for (const FWacomMapEdgeDefinition& Edge : FloorDefinition->Edges)
	{
		const TArray<const AWacomRunPathSegmentActor*> Paths =
			PathsById.FindRef(Edge.EdgeId);
		if (Paths.IsEmpty())
		{
			AddError(Report, EWacomRunSceneBindingDiagnosticCode::EdgePathMissing,
				MakeIdentityPath(*FloorDefinition, TEXT("Edge"), Edge.EdgeId),
				FString::Printf(TEXT("Floor Edge %s 缺少 PathSegment。"),
					*Edge.EdgeId.ToString()));
		}
		else if (Paths.Num() > 1)
		{
			AddError(Report, EWacomRunSceneBindingDiagnosticCode::EdgePathDuplicate,
				MakeIdentityPath(*FloorDefinition, TEXT("Edge"), Edge.EdgeId),
				FString::Printf(TEXT("Floor Edge %s 的 PathSegment 重复，当前数量=%d。"),
					*Edge.EdgeId.ToString(), Paths.Num()));
		}
		for (const AWacomRunPathSegmentActor* Path : Paths)
		{
			ValidatePathGeometry(*Path, Edge, AnchorsById, Report);
		}
	}

	TMap<FName, TArray<const AWacomRunPathBranchTargetActor*>> BranchesById;
	for (TActorIterator<AWacomRunPathBranchTargetActor> It(
		const_cast<UWorld*>(World)); It; ++It)
	{
		if (It->EdgeId.IsNone() || !FloorDefinition->FindEdge(It->EdgeId))
		{
			AddError(Report,
				EWacomRunSceneBindingDiagnosticCode::BranchTargetUnexpected,
				It->GetPathName(),
				FString::Printf(TEXT("BranchTarget %s 使用未声明 EdgeId：%s。"),
					*It->GetName(), *It->EdgeId.ToString()));
			continue;
		}
		BranchesById.FindOrAdd(It->EdgeId).Add(*It);
	}
	TMap<FName, int32> DeclaredOutgoingEdgeCounts;
	for (const FWacomMapEdgeDefinition& Edge : FloorDefinition->Edges)
	{
		++DeclaredOutgoingEdgeCounts.FindOrAdd(Edge.FromNodeId);
	}
	for (const FWacomMapEdgeDefinition& Edge : FloorDefinition->Edges)
	{
		const bool bRequired =
			DeclaredOutgoingEdgeCounts.FindRef(Edge.FromNodeId) >= 2;
		const TArray<const AWacomRunPathBranchTargetActor*> Branches =
			BranchesById.FindRef(Edge.EdgeId);
		if (bRequired && Branches.IsEmpty())
		{
			AddError(Report,
				EWacomRunSceneBindingDiagnosticCode::BranchTargetMissing,
				MakeIdentityPath(*FloorDefinition, TEXT("Edge"), Edge.EdgeId),
				FString::Printf(TEXT("多出口 Floor Edge %s 缺少 BranchTarget。"),
					*Edge.EdgeId.ToString()));
		}
		else if (bRequired && Branches.Num() > 1)
		{
			AddError(Report,
				EWacomRunSceneBindingDiagnosticCode::BranchTargetDuplicate,
				MakeIdentityPath(*FloorDefinition, TEXT("Edge"), Edge.EdgeId),
				FString::Printf(TEXT("多出口 Floor Edge %s 的 BranchTarget 重复，当前数量=%d。"),
					*Edge.EdgeId.ToString(), Branches.Num()));
		}
		else if (!bRequired)
		{
			for (const AWacomRunPathBranchTargetActor* Branch : Branches)
			{
				AddError(Report,
					EWacomRunSceneBindingDiagnosticCode::BranchTargetUnexpected,
					Branch->GetPathName(),
					FString::Printf(TEXT("单出口 Floor Edge %s 不应放置 BranchTarget。"),
						*Edge.EdgeId.ToString()));
			}
		}
	}

	TMap<FName, TArray<const UWacomRunMapNodeBindingComponent*>> HostsByNodeId;
	for (TActorIterator<AActor> It(const_cast<UWorld*>(World)); It; ++It)
	{
		TArray<UWacomRunMapNodeBindingComponent*> Bindings;
		It->GetComponents<UWacomRunMapNodeBindingComponent>(Bindings);
		for (const UWacomRunMapNodeBindingComponent* Binding : Bindings)
		{
			const FWacomMapNodeDefinition* Node = Binding
				? FloorDefinition->FindNode(Binding->NodeId)
				: nullptr;
			if (!Node || !RequiresHost(Node->NodeType))
			{
				AddError(Report,
					EWacomRunSceneBindingDiagnosticCode::ContentHostUnexpected,
					It->GetPathName(),
					FString::Printf(
						TEXT("Content Host %s 使用未声明或不需要 Host 的 NodeId：%s。"),
						*It->GetName(), Binding ? *Binding->NodeId.ToString() : TEXT("None")));
				continue;
			}
			HostsByNodeId.FindOrAdd(Node->NodeId).Add(Binding);
			if (Binding->NodeType != Node->NodeType)
			{
				AddError(Report,
					EWacomRunSceneBindingDiagnosticCode::ContentHostTypeMismatch,
					It->GetPathName(),
					FString::Printf(TEXT("Content Host %s 的 NodeType 与 Floor 节点 %s 不一致。"),
						*It->GetName(), *Node->NodeId.ToString()));
			}
			ValidatePersistedContentDefinition(**It, *Node, Report);
		}
	}
	for (const FWacomMapNodeDefinition& Node : FloorDefinition->Nodes)
	{
		if (!RequiresHost(Node.NodeType))
		{
			continue;
		}
		const int32 Count = HostsByNodeId.FindRef(Node.NodeId).Num();
		if (Count == 0)
		{
			AddError(Report, EWacomRunSceneBindingDiagnosticCode::ContentHostMissing,
				MakeIdentityPath(*FloorDefinition, TEXT("Node"), Node.NodeId),
				FString::Printf(TEXT("Content 节点 %s 缺少权威 Host。"),
					*Node.NodeId.ToString()));
		}
		else if (Count > 1)
		{
			AddError(Report, EWacomRunSceneBindingDiagnosticCode::ContentHostDuplicate,
				MakeIdentityPath(*FloorDefinition, TEXT("Node"), Node.NodeId),
				FString::Printf(TEXT("Content 节点 %s 的权威 Host 重复，当前数量=%d。"),
					*Node.NodeId.ToString(), Count));
		}
	}

	Report.Sort();
	return Report;
}

const TCHAR* LexToString(const EWacomRunSceneBindingDiagnosticSeverity Severity)
{
	switch (Severity)
	{
	case EWacomRunSceneBindingDiagnosticSeverity::Info: return TEXT("Info");
	case EWacomRunSceneBindingDiagnosticSeverity::Warning: return TEXT("Warning");
	case EWacomRunSceneBindingDiagnosticSeverity::Error: return TEXT("Error");
	default: return TEXT("UnknownSeverity");
	}
}

const TCHAR* LexToString(const EWacomRunSceneBindingDiagnosticCode Code)
{
#define WACOM_DIAGNOSTIC_CASE(Name) case EWacomRunSceneBindingDiagnosticCode::Name: return TEXT(#Name)
	switch (Code)
	{
	WACOM_DIAGNOSTIC_CASE(WorldInvalid);
	WACOM_DIAGNOSTIC_CASE(WorldTypeUnsupported);
	WACOM_DIAGNOSTIC_CASE(DescriptorMissing);
	WACOM_DIAGNOSTIC_CASE(DescriptorDuplicate);
	WACOM_DIAGNOSTIC_CASE(DescriptorFloorMissing);
	WACOM_DIAGNOSTIC_CASE(DescriptorFloorIdentityMissing);
	WACOM_DIAGNOSTIC_CASE(NodeAnchorMissing);
	WACOM_DIAGNOSTIC_CASE(NodeAnchorDuplicate);
	WACOM_DIAGNOSTIC_CASE(NodeAnchorUnexpected);
	WACOM_DIAGNOSTIC_CASE(EdgePathMissing);
	WACOM_DIAGNOSTIC_CASE(EdgePathDuplicate);
	WACOM_DIAGNOSTIC_CASE(EdgePathUnexpected);
	WACOM_DIAGNOSTIC_CASE(BranchTargetMissing);
	WACOM_DIAGNOSTIC_CASE(BranchTargetDuplicate);
	WACOM_DIAGNOSTIC_CASE(BranchTargetUnexpected);
	WACOM_DIAGNOSTIC_CASE(ContentHostMissing);
	WACOM_DIAGNOSTIC_CASE(ContentHostDuplicate);
	WACOM_DIAGNOSTIC_CASE(ContentHostUnexpected);
	WACOM_DIAGNOSTIC_CASE(ContentHostTypeMismatch);
	WACOM_DIAGNOSTIC_CASE(ContentHostPayloadMismatch);
	WACOM_DIAGNOSTIC_CASE(SplinePointCountInvalid);
	WACOM_DIAGNOSTIC_CASE(SplineLengthTooShort);
	WACOM_DIAGNOSTIC_CASE(SplineTransformNonFinite);
	WACOM_DIAGNOSTIC_CASE(SplineDirectionReversed);
	WACOM_DIAGNOSTIC_CASE(SplineSourceEndpointWarning);
	WACOM_DIAGNOSTIC_CASE(SplineSourceEndpointError);
	WACOM_DIAGNOSTIC_CASE(SplineTargetEndpointWarning);
	WACOM_DIAGNOSTIC_CASE(SplineTargetEndpointError);
	default: return TEXT("UnknownCode");
	}
#undef WACOM_DIAGNOSTIC_CASE
}
