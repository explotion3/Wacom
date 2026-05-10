// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Modules/ModuleInterface.h"

class FWacomEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
