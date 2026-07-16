// Copyright Wacom. All Rights Reserved.

#include "WacomEditorModule.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Details/WacomBattleEnemyActorDetails.h"
#include "Editor.h"
#include "EditorValidatorSubsystem.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "ToolMenus.h"
#include "Validation/WacomCardDefinitionValidator.h"
#include "Validation/WacomCharacterDefinitionValidator.h"
#include "Validation/WacomEncounterDefinitionValidator.h"
#include "Validation/WacomEnemyBehaviorDefinitionValidator.h"
#include "Validation/WacomEnemyDefinitionValidator.h"
#include "Validation/WacomEnemyPartDefinitionValidator.h"
#include "Validation/WacomRunEventDefinitionValidator.h"
#include "Validation/WacomRunPickupDefinitionValidator.h"
#include "Validation/WacomRunWorldCardInteractionDefinitionValidator.h"
#include "Validation/WacomShopDefinitionValidator.h"
#include "Validation/WacomRunSceneBindingValidation.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "WacomEditorModule"

namespace
{
	const FName WacomEditorToolMenuOwner(TEXT("WacomEditor"));

	void ShowRunFloorValidationNotification(
		const FText& Message,
		const SNotificationItem::ECompletionState State)
	{
		FNotificationInfo Info(Message);
		Info.ExpireDuration = 5.0f;
		Info.bUseLargeFont = false;
		if (const TSharedPtr<SNotificationItem> Notification =
			FSlateNotificationManager::Get().AddNotification(Info))
		{
			Notification->SetCompletionState(State);
		}
	}

	void LogRunFloorDiagnostic(const FWacomRunSceneBindingDiagnostic& Diagnostic)
	{
		if (Diagnostic.Severity == EWacomRunSceneBindingDiagnosticSeverity::Error)
		{
			UE_LOG(LogTemp, Error, TEXT("[RunFloorValidation][%s][%s] %s: %s"), LexToString(Diagnostic.Severity),
				LexToString(Diagnostic.Code), *Diagnostic.ObjectPath,
				*Diagnostic.Message.ToString());
		}
		else if (Diagnostic.Severity ==
			EWacomRunSceneBindingDiagnosticSeverity::Warning)
		{
			UE_LOG(LogTemp, Warning, TEXT("[RunFloorValidation][%s][%s] %s: %s"), LexToString(Diagnostic.Severity),
				LexToString(Diagnostic.Code), *Diagnostic.ObjectPath,
				*Diagnostic.Message.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Display, TEXT("[RunFloorValidation][%s][%s] %s: %s"), LexToString(Diagnostic.Severity),
				LexToString(Diagnostic.Code), *Diagnostic.ObjectPath,
				*Diagnostic.Message.ToString());
		}
	}
}

void FWacomEditorModule::StartupModule()
{
	RegisterDetailsCustomizations();
	RegisterEditorValidator(NewObject<UWacomCardDefinitionValidator>(GetTransientPackage()));
	RegisterEditorValidator(NewObject<UWacomEncounterDefinitionValidator>(GetTransientPackage()));
	RegisterEditorValidator(NewObject<UWacomEnemyBehaviorDefinitionValidator>(GetTransientPackage()));
	RegisterEditorValidator(NewObject<UWacomEnemyDefinitionValidator>(GetTransientPackage()));
	RegisterEditorValidator(NewObject<UWacomEnemyPartDefinitionValidator>(GetTransientPackage()));
	RegisterEditorValidator(NewObject<UWacomCharacterDefinitionValidator>(GetTransientPackage()));
	RegisterEditorValidator(NewObject<UWacomRunEventDefinitionValidator>(GetTransientPackage()));
	RegisterEditorValidator(NewObject<UWacomRunPickupDefinitionValidator>(GetTransientPackage()));
	RegisterEditorValidator(NewObject<UWacomRunWorldCardInteractionDefinitionValidator>(GetTransientPackage()));
	RegisterEditorValidator(NewObject<UWacomShopDefinitionValidator>(GetTransientPackage()));
	ToolMenusStartupCallbackHandle = UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(
			this, &FWacomEditorModule::RegisterMenus));
}

void FWacomEditorModule::ShutdownModule()
{
	if (ToolMenusStartupCallbackHandle.IsValid())
	{
		UToolMenus::UnRegisterStartupCallback(ToolMenusStartupCallbackHandle);
		ToolMenusStartupCallbackHandle.Reset();
	}
	if (!IsEngineExitRequested() && !GExitPurge)
	{
		UToolMenus::UnregisterOwner(WacomEditorToolMenuOwner);
	}
	UnregisterDetailsCustomizations();
	UnregisterEditorValidators();
}

void FWacomEditorModule::RegisterDetailsCustomizations()
{
	FPropertyEditorModule& PropertyEditorModule =
		FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
	PropertyEditorModule.RegisterCustomClassLayout(
		AWacomBattleEnemyActor::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(
			&FWacomBattleEnemyActorDetails::MakeInstance));
	PropertyEditorModule.NotifyCustomizationModuleChanged();
}

void FWacomEditorModule::UnregisterDetailsCustomizations()
{
	if (!FModuleManager::Get().IsModuleLoaded(TEXT("PropertyEditor")))
	{
		return;
	}

	FPropertyEditorModule& PropertyEditorModule =
		FModuleManager::GetModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
	PropertyEditorModule.UnregisterCustomClassLayout(
		AWacomBattleEnemyActor::StaticClass()->GetFName());
	if (!IsEngineExitRequested() && !GExitPurge)
	{
		PropertyEditorModule.NotifyCustomizationModuleChanged();
	}
}

void FWacomEditorModule::RegisterMenus()
{
	FToolMenuOwnerScoped MenuOwner(WacomEditorToolMenuOwner);
	if (UToolMenu* ToolsMenu =
		UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Tools")))
	{
		FToolMenuSection& Section = ToolsMenu->FindOrAddSection(TEXT("Wacom"));
		Section.AddMenuEntry(
			TEXT("Wacom.ValidateCurrentRunFloor"),
			LOCTEXT("ValidateCurrentRunFloorLabel", "Validate Current Run Floor"),
			LOCTEXT("ValidateCurrentRunFloorTooltip",
				"Read-only validation of the current World Descriptor, Run scene identities and Spline geometry."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(
				this, &FWacomEditorModule::ValidateCurrentRunFloor)));
	}
}

void FWacomEditorModule::ValidateCurrentRunFloor()
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	const FWacomRunSceneBindingValidationReport Report =
		FWacomRunSceneBindingValidation::ValidateLoadedWorld(World);
	for (const FWacomRunSceneBindingDiagnostic& Diagnostic : Report.Diagnostics)
	{
		LogRunFloorDiagnostic(Diagnostic);
	}
	if (Report.IsValid())
	{
		ShowRunFloorValidationNotification(
			FText::Format(
				LOCTEXT("RunFloorValidationSucceeded",
					"Run Floor validation passed ({0} diagnostics)."),
				Report.Diagnostics.Num()),
			SNotificationItem::CS_Success);
	}
	else
	{
		ShowRunFloorValidationNotification(
			FText::Format(
				LOCTEXT("RunFloorValidationFailed",
					"Run Floor validation failed ({0} diagnostics). See Output Log."),
				Report.Diagnostics.Num()),
			SNotificationItem::CS_Fail);
	}
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

#undef LOCTEXT_NAMESPACE
