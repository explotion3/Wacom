// Copyright Wacom. All Rights Reserved.

#pragma once

#if WITH_EDITOR

class UWorld;

namespace UE::Wacom::Backpack::PIEValidation
{
bool SeedToTarget(UWorld* World, int32 OwnedCardTarget);
bool OpenFormalWorkspace(UWorld* World);
bool CloseWorkspace(UWorld* World);
}

#endif
