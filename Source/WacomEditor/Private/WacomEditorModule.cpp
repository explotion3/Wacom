// Copyright Wacom. All Rights Reserved.

#include "WacomEditorModule.h"

#include "Editor.h"
#include "EditorValidatorSubsystem.h"
#include "Modules/ModuleManager.h"
#include "Validation/WacomRunEventDefinitionValidator.h"
#include "Validation/WacomShopDefinitionValidator.h"

void FWacomEditorModule::StartupModule()
{
	RegisterEditorValidator(NewObject<UWacomRunEventDefinitionValidator>(GetTransientPackage()));
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
