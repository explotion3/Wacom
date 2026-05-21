// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Modules/ModuleInterface.h"
#include "UObject/StrongObjectPtr.h"

class UEditorValidatorBase;

class FWacomEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterEditorValidator(UEditorValidatorBase* Validator);
	void UnregisterEditorValidators();

	TArray<TStrongObjectPtr<UEditorValidatorBase>> RegisteredValidators;
};
