// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AWacomRunFloorSceneDescriptorActor;
class UWorld;
class UWacomFloorMapDefinition;

enum class EWacomRunFloorSceneDescriptorResolveStatus : uint8
{
	Resolved,
	WorldInvalid,
	DescriptorMissing,
	DescriptorDuplicate,
	FloorMissing,
	FloorIdMissing,
	FloorMismatch,
};

struct FWacomRunFloorSceneDescriptorResolveResult
{
	EWacomRunFloorSceneDescriptorResolveStatus Status =
		EWacomRunFloorSceneDescriptorResolveStatus::WorldInvalid;
	const AWacomRunFloorSceneDescriptorActor* Descriptor = nullptr;
	const UWacomFloorMapDefinition* FloorDefinition = nullptr;

	bool IsResolved() const
	{
		return Status == EWacomRunFloorSceneDescriptorResolveStatus::Resolved;
	}

	FName GetDetail() const;
};

/** App-private, read-only resolver for the unique loaded Run Floor Descriptor. */
class FWacomRunFloorSceneDescriptorResolver
{
public:
	static FWacomRunFloorSceneDescriptorResolveResult Resolve(
		UWorld* World,
		FName ExpectedFloorId = NAME_None);
};
