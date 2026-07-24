// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Modules/ModuleInterface.h"
#include "UObject/StrongObjectPtr.h"

class UEditorValidatorBase;
class FWacomRunFloorSceneValidationMenuAutomationTestView;

class FWacomEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	friend class FWacomRunFloorSceneValidationMenuAutomationTestView;

	void RegisterEditorValidator(UEditorValidatorBase* Validator);
	void UnregisterEditorValidators();
	void RegisterDetailsCustomizations();
	void UnregisterDetailsCustomizations();
	void RegisterComponentVisualizers();
	void UnregisterComponentVisualizers();
	void RegisterMenus();
	void ValidateCurrentRunFloor();

	TArray<TStrongObjectPtr<UEditorValidatorBase>> RegisteredValidators;
	FDelegateHandle ToolMenusStartupCallbackHandle;
	FDelegateHandle PostEngineInitHandle;
	bool bComponentVisualizersRegistered = false;
};
