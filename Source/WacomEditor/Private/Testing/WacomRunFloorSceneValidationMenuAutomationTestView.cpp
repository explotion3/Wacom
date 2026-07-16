// Copyright Wacom. All Rights Reserved.

#include "Testing/WacomRunFloorSceneValidationMenuAutomationTestView.h"

#include "FileHelpers.h"
#include "Modules/ModuleManager.h"
#include "ToolMenu.h"
#include "ToolMenuEntry.h"
#include "ToolMenuSection.h"
#include "ToolMenus.h"
#include "WacomEditorModule.h"

namespace
{
	const FToolMenuEntry* FindValidationMenuEntry()
	{
		UToolMenu* ToolsMenu = UToolMenus::Get()->FindMenu(
			TEXT("LevelEditor.MainMenu.Tools"));
		if (!ToolsMenu)
		{
			return nullptr;
		}
		const FToolMenuSection* Section = ToolsMenu->FindSection(TEXT("Wacom"));
		return Section
			? Section->FindEntry(TEXT("Wacom.ValidateCurrentRunFloor"))
			: nullptr;
	}
}

bool FWacomRunFloorSceneValidationMenuAutomationTestView::IsMenuEntryRegistered()
{
	return FindValidationMenuEntry() != nullptr;
}

bool FWacomRunFloorSceneValidationMenuAutomationTestView::LoadMapAndExecuteMenuEntry(
	const FString& MapPackageName)
{
	if (!UEditorLoadingAndSavingUtils::LoadMap(MapPackageName))
	{
		return false;
	}
	if (!FindValidationMenuEntry())
	{
		return false;
	}
	FWacomEditorModule& Module =
		FModuleManager::GetModuleChecked<FWacomEditorModule>(TEXT("WacomEditor"));
	Module.ValidateCurrentRunFloor();
	return true;
}
