// Copyright Wacom. All Rights Reserved.

#include "Commandlets/WacomMigrateBackpackWorkspaceV4Commandlet.h"

#include "ContentBuilders/BackpackWorkspaceV4Migration.h"
#include "Misc/Parse.h"

UWacomMigrateBackpackWorkspaceV4Commandlet::
	UWacomMigrateBackpackWorkspaceV4Commandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UWacomMigrateBackpackWorkspaceV4Commandlet::Main(const FString& Params)
{
	const bool bApply = FParse::Param(*Params, TEXT("Apply"));
	const bool bInspectOnly = FParse::Param(*Params, TEXT("InspectOnly"));
	if (bApply == bInspectOnly)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomMigrateBackpackWorkspaceV4] Specify exactly one of -Apply or -InspectOnly"));
		return 1;
	}

	UE_LOG(LogTemp, Display,
		TEXT("[WacomMigrateBackpackWorkspaceV4] Start mode=%s"),
		bApply ? TEXT("Apply") : TEXT("InspectOnly"));
	Wacom::ContentBuilder::FBackpackWorkspaceV4MigrationReport Report;
	if (!Wacom::ContentBuilder::MigrateBackpackWorkspaceV4(bApply, Report))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomMigrateBackpackWorkspaceV4] Failed before completing the exact manifest"));
		return 1;
	}
	if (!bApply && !Report.bAlreadyCurrent)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[WacomMigrateBackpackWorkspaceV4] Audit valid but migration is still required"));
		return 2;
	}

	UE_LOG(LogTemp, Display,
		TEXT("[WacomMigrateBackpackWorkspaceV4] Done current=%s saved=%d"),
		Report.bAlreadyCurrent ? TEXT("true") : TEXT("false"),
		Report.SavedPackages.Num());
	return 0;
}
