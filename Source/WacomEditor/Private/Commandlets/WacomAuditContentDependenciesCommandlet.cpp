// Copyright Wacom. All Rights Reserved.

#include "Commandlets/WacomAuditContentDependenciesCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "ContentAudit/WacomContentDependencyAudit.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Modules/ModuleManager.h"

UWacomAuditContentDependenciesCommandlet::UWacomAuditContentDependenciesCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UWacomAuditContentDependenciesCommandlet::Main(const FString& Params)
{
	FString RootString = TEXT("/Game/Wacom");
	FParse::Value(*Params, TEXT("Root="), RootString);
	RootString.RemoveFromEnd(TEXT("/"));
	if (!RootString.StartsWith(TEXT("/Game/"), ESearchCase::CaseSensitive))
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomContentAudit] Root 必须是 /Game 下的 package path：%s"), *RootString);
		return 1;
	}

	FString OutputPath;
	if (!FParse::Value(*Params, TEXT("Output="), OutputPath))
	{
		OutputPath = FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Reports"),
			TEXT("WacomContentDependencyAudit.json"));
	}
	if (FPaths::IsRelative(OutputPath))
	{
		OutputPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), OutputPath);
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
		TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	AssetRegistry.SearchAllAssets(/*bSynchronousSearch*/ true);
	AssetRegistry.WaitForCompletion();

	const Wacom::ContentAudit::FWacomContentDependencyAuditReport Report =
		Wacom::ContentAudit::BuildReport(AssetRegistry, FName(*RootString));
	FString WriteError;
	if (!Wacom::ContentAudit::WriteReport(Report, OutputPath, WriteError))
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomContentAudit] %s"), *WriteError);
		return 1;
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("[WacomContentAudit] Done Root=%s Scanned=%d TraversedGame=%d External=%d Report=%s"),
		*RootString,
		Report.ScannedPackageCount,
		Report.TraversedGamePackageCount,
		Report.ExternalFindings.Num(),
		*OutputPath);
	if (FParse::Param(*Params, TEXT("FailOnExternal")) && !Report.ExternalFindings.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomContentAudit] FailOnExternal: 检测到 %d 个外部 package"), Report.ExternalFindings.Num());
		return 2;
	}
	return 0;
}
