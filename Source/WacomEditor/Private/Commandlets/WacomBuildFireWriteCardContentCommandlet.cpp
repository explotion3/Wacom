// Copyright Wacom. All Rights Reserved.

#include "Commandlets/WacomBuildFireWriteCardContentCommandlet.h"

#include "ContentBuilders/FireWriteCardContentBuilder.h"
#include "Misc/Parse.h"

UWacomBuildFireWriteCardContentCommandlet::
	UWacomBuildFireWriteCardContentCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UWacomBuildFireWriteCardContentCommandlet::Main(const FString& Params)
{
	Wacom::ContentBuilder::FFireWriteCardContentOptions Options;
	Options.bSeedMissing = FParse::Param(*Params, TEXT("SeedMissing"));
	Options.bMigrateLegacyUpgrade =
		FParse::Param(*Params, TEXT("MigrateLegacyUpgrade"));
	Options.bWriteExplanationTemplates =
		FParse::Param(*Params, TEXT("WriteExplanationTemplates"));
	Options.bSyncSeedDefaults =
		FParse::Param(*Params, TEXT("SyncSeedDefaults"));
	Options.bSyncExplanationLexiconDefaults =
		FParse::Param(*Params, TEXT("SyncExplanationLexiconDefaults"));
	Options.bCompareSeedDefaults =
		FParse::Param(*Params, TEXT("CompareSeedDefaults"));
	return Wacom::ContentBuilder::RunFireWriteCardContentBuilder(Options);
}
