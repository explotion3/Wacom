// Copyright Wacom. All Rights Reserved.

#include "GameFramework/WacomRunFloorSceneDescriptorResolver.h"

#include "Actors/WacomRunFloorSceneDescriptorActor.h"
#include "EngineUtils.h"
#include "Map/WacomFloorMapDefinition.h"

FName FWacomRunFloorSceneDescriptorResolveResult::GetDetail() const
{
	switch (Status)
	{
	case EWacomRunFloorSceneDescriptorResolveStatus::Resolved:
		return TEXT("DescriptorResolved");
	case EWacomRunFloorSceneDescriptorResolveStatus::WorldInvalid:
		return TEXT("DescriptorWorldInvalid");
	case EWacomRunFloorSceneDescriptorResolveStatus::DescriptorMissing:
		return TEXT("DescriptorMissing");
	case EWacomRunFloorSceneDescriptorResolveStatus::DescriptorDuplicate:
		return TEXT("DescriptorDuplicate");
	case EWacomRunFloorSceneDescriptorResolveStatus::FloorMissing:
		return TEXT("DescriptorFloorMissing");
	case EWacomRunFloorSceneDescriptorResolveStatus::FloorIdMissing:
		return TEXT("DescriptorFloorIdMissing");
	case EWacomRunFloorSceneDescriptorResolveStatus::FloorMismatch:
		return TEXT("DescriptorFloorMismatch");
	default:
		return TEXT("DescriptorResolveUnknown");
	}
}

FWacomRunFloorSceneDescriptorResolveResult
FWacomRunFloorSceneDescriptorResolver::Resolve(
	UWorld* World,
	const FName ExpectedFloorId)
{
	FWacomRunFloorSceneDescriptorResolveResult Result;
	if (!IsValid(World))
	{
		Result.Status = EWacomRunFloorSceneDescriptorResolveStatus::WorldInvalid;
		return Result;
	}

	const AWacomRunFloorSceneDescriptorActor* FoundDescriptor = nullptr;
	for (TActorIterator<AWacomRunFloorSceneDescriptorActor> It(World); It; ++It)
	{
		if (FoundDescriptor)
		{
			Result.Status =
				EWacomRunFloorSceneDescriptorResolveStatus::DescriptorDuplicate;
			return Result;
		}
		FoundDescriptor = *It;
	}
	if (!FoundDescriptor)
	{
		Result.Status = EWacomRunFloorSceneDescriptorResolveStatus::DescriptorMissing;
		return Result;
	}

	const UWacomFloorMapDefinition* FloorDefinition =
		FoundDescriptor->GetFloorDefinition();
	if (!IsValid(FloorDefinition))
	{
		Result.Status = EWacomRunFloorSceneDescriptorResolveStatus::FloorMissing;
		return Result;
	}
	if (FloorDefinition->FloorId.IsNone())
	{
		Result.Status = EWacomRunFloorSceneDescriptorResolveStatus::FloorIdMissing;
		return Result;
	}
	if (!ExpectedFloorId.IsNone() && FloorDefinition->FloorId != ExpectedFloorId)
	{
		Result.Status = EWacomRunFloorSceneDescriptorResolveStatus::FloorMismatch;
		return Result;
	}

	Result.Status = EWacomRunFloorSceneDescriptorResolveStatus::Resolved;
	Result.Descriptor = FoundDescriptor;
	Result.FloorDefinition = FloorDefinition;
	return Result;
}
