// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WacomRunFirstPersonCardSourceComponent.h"
#include "RunFirstPersonCardLayerSpecReceiver.generated.h"

class UWacomFirstPersonCardAnchorComponent;

UCLASS()
class UWacomRunFirstPersonCardSourceSpecProbeComponent
	: public UWacomRunFirstPersonCardSourceComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient)
	TObjectPtr<UWacomFirstPersonCardAnchorComponent> AnchorForTest = nullptr;

	TArray<FWacomFirstPersonCardLayerEntry> LastWrittenEntries;
	int32 WriteCount = 0;
	int32 ClearCount = 0;

protected:
	virtual UWacomFirstPersonCardAnchorComponent* ResolveFirstPersonCardAnchor() const override
	{
		return AnchorForTest;
	}

	virtual void WriteRuntimeCardLayerEntries(
		UWacomFirstPersonCardAnchorComponent& Anchor,
		const TArray<FWacomFirstPersonCardLayerEntry>& Entries) override
	{
		LastWrittenEntries = Entries;
		++WriteCount;
	}

	virtual void ClearRuntimeCardLayerEntries(UWacomFirstPersonCardAnchorComponent& Anchor) override
	{
		LastWrittenEntries.Reset();
		++ClearCount;
	}
};
