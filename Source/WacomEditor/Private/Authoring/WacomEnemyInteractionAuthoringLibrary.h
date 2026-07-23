// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WacomEnemyInteractionAuthoringLibrary.generated.h"

class UBlueprint;
class UPaperSprite;

/** Focused editor-only mutation seam used by the formal enemy interaction asset migration. */
UCLASS()
class UWacomEnemyInteractionAuthoringLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Returns -1 for invalid hierarchy, 0 for no change, and 1 after applying/saving-ready changes. */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Editor|Enemy Interaction")
	static int32 ApplyInteractionLayerContractToHostBlueprint(UBlueprint* Blueprint);

	/** Returns true when collision authoring values or generated BodySetup changed. */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Editor|Enemy Interaction")
	static bool ConfigureStableInteractionSprite(UPaperSprite* Sprite);
};
