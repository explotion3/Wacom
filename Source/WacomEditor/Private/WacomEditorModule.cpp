// Copyright Wacom. All Rights Reserved.

#include "WacomEditorModule.h"

#include "Editor.h"
#include "EditorValidatorSubsystem.h"
#include "Modules/ModuleManager.h"
#include "Validation/WacomCardDefinitionValidator.h"
#include "Validation/WacomCharacterDefinitionValidator.h"
#include "Validation/WacomEncounterDefinitionValidator.h"
#include "Validation/WacomEnemyBehaviorDefinitionValidator.h"
#include "Validation/WacomEnemyDefinitionValidator.h"
#include "Validation/WacomEnemyPartDefinitionValidator.h"
#include "Validation/WacomFirstPersonCardLayoutPresetValidator.h"
#include "Validation/WacomRunEventDefinitionValidator.h"
#include "Validation/WacomRunPickupDefinitionValidator.h"
#include "Validation/WacomRunWorldCardInteractionDefinitionValidator.h"
#include "Validation/WacomShopDefinitionValidator.h"

void FWacomEditorModule::StartupModule()
{
	RegisterEditorValidator(NewObject<UWacomCardDefinitionValidator>(GetTransientPackage()));
	RegisterEditorValidator(NewObject<UWacomEncounterDefinitionValidator>(GetTransientPackage()));
	RegisterEditorValidator(NewObject<UWacomEnemyBehaviorDefinitionValidator>(GetTransientPackage()));
	RegisterEditorValidator(NewObject<UWacomEnemyDefinitionValidator>(GetTransientPackage()));
	RegisterEditorValidator(NewObject<UWacomEnemyPartDefinitionValidator>(GetTransientPackage()));
	RegisterEditorValidator(NewObject<UWacomFirstPersonCardLayoutPresetValidator>(GetTransientPackage()));
	RegisterEditorValidator(NewObject<UWacomCharacterDefinitionValidator>(GetTransientPackage()));
	RegisterEditorValidator(NewObject<UWacomRunEventDefinitionValidator>(GetTransientPackage()));
	RegisterEditorValidator(NewObject<UWacomRunPickupDefinitionValidator>(GetTransientPackage()));
	RegisterEditorValidator(NewObject<UWacomRunWorldCardInteractionDefinitionValidator>(GetTransientPackage()));
	RegisterEditorValidator(NewObject<UWacomShopDefinitionValidator>(GetTransientPackage()));
}

void FWacomEditorModule::ShutdownModule()
{
	UnregisterEditorValidators();
}

void FWacomEditorModule::RegisterEditorValidator(UEditorValidatorBase* Validator)
{
	if (!Validator)
	{
		return;
	}

	RegisteredValidators.Emplace(Validator);

	if (GEditor)
	{
		if (UEditorValidatorSubsystem* ValidatorSubsystem = GEditor->GetEditorSubsystem<UEditorValidatorSubsystem>())
		{
			ValidatorSubsystem->AddValidator(Validator);
		}
	}
}

void FWacomEditorModule::UnregisterEditorValidators()
{
	if (GEditor)
	{
		if (UEditorValidatorSubsystem* ValidatorSubsystem = GEditor->GetEditorSubsystem<UEditorValidatorSubsystem>())
		{
			for (const TStrongObjectPtr<UEditorValidatorBase>& Validator : RegisteredValidators)
			{
				ValidatorSubsystem->RemoveValidator(Validator.Get());
			}
		}
	}

	RegisteredValidators.Reset();
}

IMPLEMENT_MODULE(FWacomEditorModule, WacomEditor);
