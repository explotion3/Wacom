// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Components/ChildActorComponent.h"

#include "WacomBattleEnemyPartChildActorComponent.generated.h"

/**
 * Runtime-safe identity adapter for Editor-created level-instance enemy parts.
 *
 * Unreal does not persist per-instance ChildActor properties by default. The
 * Editor authoring service stores the derived identity on this component so a
 * recreated ChildActor receives the same stable slot identity after load or
 * Undo/Redo. The component never derives identity from object names.
 */
UCLASS(ClassGroup = Utility, NotBlueprintable)
class WACOMAPP_API UWacomBattleEnemyPartChildActorComponent final
	: public UChildActorComponent
{
	GENERATED_BODY()

public:
	void SetStoredPartIdentity(FName InPartSlotId, FName InPartId);

protected:
	virtual void OnRegister() override;

#if WITH_EDITOR
	virtual void PostEditUndo() override;
#endif

private:
	void ApplyStoredPartIdentity() const;

	UPROPERTY()
	FName StoredPartSlotId = NAME_None;

	UPROPERTY()
	FName StoredPartId = NAME_None;
};
