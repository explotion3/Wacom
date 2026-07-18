// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWacomFirstPersonCardAnchorComponent;

struct FWacomFirstPersonCardPresentationAssetCollection
{
	TArray<FSoftObjectPath> RequiredVisualAssets;
	TArray<FSoftObjectPath> OptionalAudioAssets;
	int32 VisitedWidgetClassCount = 0;
	int32 VisitedCardViewTemplateCount = 0;
	int32 VisitedBadgeTemplateCount = 0;
};

class WACOMAPP_API FWacomFirstPersonCardPresentationAssetCollector
{
public:
	static FWacomFirstPersonCardPresentationAssetCollection Collect(
		const UWacomFirstPersonCardAnchorComponent& Anchor);
};
