// Copyright Wacom. All Rights Reserved.

#include "GameFramework/WacomRunFloorPreviewGameMode.h"

#include "Actors/WacomRunFloorSceneDescriptorActor.h"
#include "Characters/CharacterDefinition.h"
#include "Engine/World.h"
#include "GameFramework/WacomRunFloorSceneDescriptorResolver.h"
#include "Map/WacomFloorMapDefinition.h"
#include "Map/WacomJourneyDefinition.h"

#define LOCTEXT_NAMESPACE "WacomRunFloorPreviewGameMode"

UWacomJourneyDefinition*
AWacomRunFloorPreviewGameMode::ResolveJourneyDefinitionForNewRun()
{
#if WITH_EDITOR
	UWorld* World = GetWorld();
	if (!IsValid(World) || World->WorldType != EWorldType::PIE)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomRunFloorPreviewGameMode] PreviewWorldNotPIE: World=%s Type=%d"),
			*GetNameSafe(World),
			World ? static_cast<int32>(World->WorldType) : INDEX_NONE);
		return nullptr;
	}
	if (!IsValid(DefaultCharacter))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomRunFloorPreviewGameMode] PreviewCharacterMissing"));
		return nullptr;
	}

	const FWacomRunFloorSceneDescriptorResolveResult DescriptorResult =
		FWacomRunFloorSceneDescriptorResolver::Resolve(World);
	if (!DescriptorResult.IsResolved())
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomRunFloorPreviewGameMode] %s"),
			*DescriptorResult.GetDetail().ToString());
		return nullptr;
	}

	AWacomRunFloorSceneDescriptorActor* Descriptor =
		const_cast<AWacomRunFloorSceneDescriptorActor*>(
			DescriptorResult.Descriptor);
	UWacomFloorMapDefinition* Floor =
		const_cast<UWacomFloorMapDefinition*>(
			DescriptorResult.FloorDefinition);
	if (PreviewJourney)
	{
		const bool bDescriptorStable =
			ResolvedPreviewDescriptor == Descriptor;
		const bool bFloorStable = ResolvedPreviewFloor == Floor
			&& ResolvedPreviewFloorId == Floor->FloorId
			&& PreviewJourney->Floors.Num() == 1
			&& PreviewJourney->Floors[0] == Floor;
		if (!bDescriptorStable || !bFloorStable)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[WacomRunFloorPreviewGameMode] %s: CachedDescriptor=%s ActualDescriptor=%s CachedFloor=%s ActualFloor=%s CachedFloorId=%s ActualFloorId=%s"),
				bDescriptorStable
					? TEXT("PreviewFloorDrift")
					: TEXT("PreviewDescriptorDrift"),
				*GetNameSafe(ResolvedPreviewDescriptor),
				*GetNameSafe(Descriptor),
				*GetNameSafe(ResolvedPreviewFloor),
				*GetNameSafe(Floor),
				*ResolvedPreviewFloorId.ToString(),
				*Floor->FloorId.ToString());
			return nullptr;
		}
		return PreviewJourney;
	}

	PreviewJourney = NewObject<UWacomJourneyDefinition>(
		this, NAME_None, RF_Transient);
	if (!PreviewJourney)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomRunFloorPreviewGameMode] PreviewJourneyAllocationFailed"));
		return nullptr;
	}

	ResolvedPreviewDescriptor = Descriptor;
	ResolvedPreviewFloor = Floor;
	ResolvedPreviewFloorId = Floor->FloorId;
	PreviewJourney->JourneyId = FName(*FString::Printf(
		TEXT("Journey.Preview.%s"), *Floor->FloorId.ToString()));
	const FText FloorTitle = Floor->DisplayName.IsEmpty()
		? FText::FromName(Floor->FloorId)
		: Floor->DisplayName;
	PreviewJourney->DisplayName = FText::Format(
		LOCTEXT("PreviewJourneyTitle", "[Preview] {0}"), FloorTitle);
	PreviewJourney->SupportedCharacters = {DefaultCharacter};
	PreviewJourney->Floors = {Floor};
	PreviewJourney->SuccessTerminalNode = FWacomMapNodeHandle();

	UE_LOG(LogTemp, Display,
		TEXT("[WacomRunFloorPreviewGameMode] Preview Journey ready: Journey=%s Floor=%s Descriptor=%s Character=%s"),
		*PreviewJourney->JourneyId.ToString(),
		*Floor->FloorId.ToString(),
		*GetNameSafe(Descriptor),
		*GetNameSafe(DefaultCharacter));
	return PreviewJourney;
#else
	UE_LOG(LogTemp, Error,
		TEXT("[WacomRunFloorPreviewGameMode] PreviewWorldNotPIE: Editor support is unavailable"));
	return nullptr;
#endif
}

#undef LOCTEXT_NAMESPACE
