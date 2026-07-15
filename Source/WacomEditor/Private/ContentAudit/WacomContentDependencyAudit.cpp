// Copyright Wacom. All Rights Reserved.

#include "ContentAudit/WacomContentDependencyAudit.h"

#include "Algo/Reverse.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/AssetRegistryInterface.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace Wacom::ContentAudit
{
namespace
{
constexpr const TCHAR* ArtRoot = TEXT("/Game/Art");
constexpr const TCHAR* AssetRoot = TEXT("/Game/Asset");
constexpr const TCHAR* DreamMaterialsRoot = TEXT("/Game/DreamMaterials");
constexpr const TCHAR* LegacyTestBattlePackage = TEXT("/Game/L_TestBattle");

bool IsGamePackage(FName PackageName)
{
	return PackageName.ToString().StartsWith(TEXT("/Game/"), ESearchCase::CaseSensitive);
}

void SortNames(TArray<FName>& Names)
{
	Names.Sort(FNameLexicalLess());
}

TArray<FName> BuildShortestChain(
	FName PackageName,
	const TMap<FName, FName>& ParentByPackage)
{
	TArray<FName> ReverseChain;
	TSet<FName> Guard;
	FName Current = PackageName;
	while (!Current.IsNone() && !Guard.Contains(Current))
	{
		Guard.Add(Current);
		ReverseChain.Add(Current);
		const FName* Parent = ParentByPackage.Find(Current);
		if (!Parent || Parent->IsNone())
		{
			break;
		}
		Current = *Parent;
	}

	Algo::Reverse(ReverseChain);
	return ReverseChain;
}

TArray<TSharedPtr<FJsonValue>> NamesToJson(TConstArrayView<FName> Names)
{
	TArray<TSharedPtr<FJsonValue>> Values;
	Values.Reserve(Names.Num());
	for (FName Name : Names)
	{
		Values.Add(MakeShared<FJsonValueString>(Name.ToString()));
	}
	return Values;
}

TArray<TSharedPtr<FJsonValue>> StringsToJson(TConstArrayView<FString> Strings)
{
	TArray<TSharedPtr<FJsonValue>> Values;
	Values.Reserve(Strings.Num());
	for (const FString& String : Strings)
	{
		Values.Add(MakeShared<FJsonValueString>(String));
	}
	return Values;
}
}

bool IsPackageUnderRoot(FName PackagePath, FName Root)
{
	const FString PackageString = PackagePath.ToString();
	FString RootString = Root.ToString();
	RootString.RemoveFromEnd(TEXT("/"));
	return !RootString.IsEmpty()
		&& (PackageString == RootString
			|| PackageString.StartsWith(RootString + TEXT("/"), ESearchCase::CaseSensitive));
}

FString ClassifyExternalPackage(FName PackageName, FName ScanRoot)
{
	if (IsPackageUnderRoot(PackageName, ScanRoot))
	{
		return TEXT("Internal");
	}
	if (IsPackageUnderRoot(PackageName, FName(ArtRoot)))
	{
		return TEXT("Art");
	}
	if (IsPackageUnderRoot(PackageName, FName(AssetRoot)))
	{
		return TEXT("Asset");
	}
	if (IsPackageUnderRoot(PackageName, FName(DreamMaterialsRoot)))
	{
		return TEXT("DreamMaterials");
	}
	if (PackageName == FName(LegacyTestBattlePackage))
	{
		return TEXT("L_TestBattle");
	}
	return IsGamePackage(PackageName) ? TEXT("OtherGame") : TEXT("NonGame");
}

FWacomContentDependencyAuditReport BuildReport(
	IAssetRegistry& AssetRegistry,
	FName ScanRoot)
{
	FWacomContentDependencyAuditReport Report;
	Report.ScanRoot = ScanRoot;

	TArray<FAssetData> RootAssets;
	AssetRegistry.GetAssetsByPath(
		ScanRoot,
		RootAssets,
		/*bRecursive*/ true,
		/*bIncludeOnlyOnDiskAssets*/ true);

	TArray<FName> Queue;
	TSet<FName> VisitedPackages;
	TMap<FName, FName> ParentByPackage;
	for (const FAssetData& Asset : RootAssets)
	{
		if (!Asset.PackageName.IsNone() && !VisitedPackages.Contains(Asset.PackageName))
		{
			VisitedPackages.Add(Asset.PackageName);
			ParentByPackage.Add(Asset.PackageName, NAME_None);
			Queue.Add(Asset.PackageName);
		}
	}
	SortNames(Queue);
	Report.ScannedPackageCount = Queue.Num();

	struct FMutableFinding
	{
		TSet<FName> DirectWacomReferencers;
		TSet<FName> Referencers;
		bool bHasHardReference = false;
		bool bHasSoftReference = false;
		bool bHasGameReference = false;
		bool bHasEditorOnlyReference = false;
		bool bHasBuildReference = false;
	};
	TMap<FName, FMutableFinding> MutableFindings;

	for (int32 QueueIndex = 0; QueueIndex < Queue.Num(); ++QueueIndex)
	{
		const FName SourcePackage = Queue[QueueIndex];
		TArray<FAssetDependency> Dependencies;
		AssetRegistry.GetDependencies(
			FAssetIdentifier(SourcePackage),
			Dependencies,
			UE::AssetRegistry::EDependencyCategory::Package);
		Dependencies.Sort(
			[](const FAssetDependency& Left, const FAssetDependency& Right)
			{
				return Left.AssetId.PackageName.LexicalLess(Right.AssetId.PackageName);
			});

		for (const FAssetDependency& Dependency : Dependencies)
		{
			const FName TargetPackage = Dependency.AssetId.PackageName;
			if (TargetPackage.IsNone() || !IsGamePackage(TargetPackage))
			{
				continue;
			}

			if (!IsPackageUnderRoot(TargetPackage, ScanRoot))
			{
				FMutableFinding& Finding = MutableFindings.FindOrAdd(TargetPackage);
				Finding.Referencers.Add(SourcePackage);
				if (IsPackageUnderRoot(SourcePackage, ScanRoot))
				{
					Finding.DirectWacomReferencers.Add(SourcePackage);
				}
				const bool bHard = EnumHasAnyFlags(
					Dependency.Properties,
					UE::AssetRegistry::EDependencyProperty::Hard);
				const bool bGame = EnumHasAnyFlags(
					Dependency.Properties,
					UE::AssetRegistry::EDependencyProperty::Game);
				Finding.bHasHardReference |= bHard;
				Finding.bHasSoftReference |= !bHard;
				Finding.bHasGameReference |= bGame;
				Finding.bHasEditorOnlyReference |= !bGame;
				Finding.bHasBuildReference |= EnumHasAnyFlags(
					Dependency.Properties,
					UE::AssetRegistry::EDependencyProperty::Build);
			}

			if (!VisitedPackages.Contains(TargetPackage))
			{
				VisitedPackages.Add(TargetPackage);
				ParentByPackage.Add(TargetPackage, SourcePackage);
				Queue.Add(TargetPackage);
			}
		}
	}

	Report.TraversedGamePackageCount = VisitedPackages.Num();
	TArray<FName> FindingPackages;
	MutableFindings.GetKeys(FindingPackages);
	SortNames(FindingPackages);
	Report.ExternalFindings.Reserve(FindingPackages.Num());
	for (FName PackageName : FindingPackages)
	{
		const FMutableFinding& Mutable = MutableFindings.FindChecked(PackageName);
		FWacomExternalDependencyFinding& Finding = Report.ExternalFindings.AddDefaulted_GetRef();
		Finding.PackageName = PackageName;
		Finding.Classification = ClassifyExternalPackage(PackageName, ScanRoot);
		Finding.DirectWacomReferencers = Mutable.DirectWacomReferencers.Array();
		Finding.Referencers = Mutable.Referencers.Array();
		SortNames(Finding.DirectWacomReferencers);
		SortNames(Finding.Referencers);
		Finding.ShortestChain = BuildShortestChain(PackageName, ParentByPackage);
		Finding.bHasHardReference = Mutable.bHasHardReference;
		Finding.bHasSoftReference = Mutable.bHasSoftReference;
		Finding.bHasGameReference = Mutable.bHasGameReference;
		Finding.bHasEditorOnlyReference = Mutable.bHasEditorOnlyReference;
		Finding.bHasBuildReference = Mutable.bHasBuildReference;
		TArray<FAssetData> PackageAssets;
		AssetRegistry.GetAssetsByPackageName(
			PackageName,
			PackageAssets,
			/*bIncludeOnlyOnDiskAssets*/ true);
		Finding.AssetCount = PackageAssets.Num();
		Finding.bHasOnDiskAsset = !PackageAssets.IsEmpty();
		for (const FAssetData& Asset : PackageAssets)
		{
			Finding.AssetClasses.AddUnique(Asset.AssetClassPath.ToString());
		}
		Finding.AssetClasses.Sort();
	}
	return Report;
}

FString SerializeReportToJson(const FWacomContentDependencyAuditReport& Report)
{
	TSharedRef<FJsonObject> RootObject = MakeShared<FJsonObject>();
	RootObject->SetNumberField(TEXT("schemaVersion"), 1);
	RootObject->SetStringField(TEXT("scanRoot"), Report.ScanRoot.ToString());
	RootObject->SetNumberField(TEXT("scannedPackageCount"), Report.ScannedPackageCount);
	RootObject->SetNumberField(TEXT("traversedGamePackageCount"), Report.TraversedGamePackageCount);
	RootObject->SetNumberField(TEXT("externalPackageCount"), Report.ExternalFindings.Num());

	TMap<FString, int32> ClassificationCounts;
	TArray<FWacomExternalDependencyFinding> Findings = Report.ExternalFindings;
	Findings.Sort(
		[](const FWacomExternalDependencyFinding& Left, const FWacomExternalDependencyFinding& Right)
		{
			return Left.PackageName.LexicalLess(Right.PackageName);
		});
	TArray<TSharedPtr<FJsonValue>> FindingValues;
	FindingValues.Reserve(Findings.Num());
	for (FWacomExternalDependencyFinding& Finding : Findings)
	{
		++ClassificationCounts.FindOrAdd(Finding.Classification);
		SortNames(Finding.DirectWacomReferencers);
		SortNames(Finding.Referencers);
		TSharedRef<FJsonObject> FindingObject = MakeShared<FJsonObject>();
		FindingObject->SetStringField(TEXT("package"), Finding.PackageName.ToString());
		FindingObject->SetStringField(TEXT("classification"), Finding.Classification);
		FindingObject->SetNumberField(TEXT("assetCount"), Finding.AssetCount);
		FindingObject->SetBoolField(TEXT("hasOnDiskAsset"), Finding.bHasOnDiskAsset);
		FindingObject->SetArrayField(TEXT("assetClasses"), StringsToJson(Finding.AssetClasses));
		FindingObject->SetBoolField(TEXT("hasHardReference"), Finding.bHasHardReference);
		FindingObject->SetBoolField(TEXT("hasSoftReference"), Finding.bHasSoftReference);
		FindingObject->SetBoolField(TEXT("hasGameReference"), Finding.bHasGameReference);
		FindingObject->SetBoolField(TEXT("hasEditorOnlyReference"), Finding.bHasEditorOnlyReference);
		FindingObject->SetBoolField(TEXT("hasBuildReference"), Finding.bHasBuildReference);
		FindingObject->SetArrayField(
			TEXT("directWacomReferencers"),
			NamesToJson(Finding.DirectWacomReferencers));
		FindingObject->SetArrayField(TEXT("referencers"), NamesToJson(Finding.Referencers));
		FindingObject->SetArrayField(TEXT("shortestChain"), NamesToJson(Finding.ShortestChain));
		FindingValues.Add(MakeShared<FJsonValueObject>(FindingObject));
	}
	RootObject->SetArrayField(TEXT("externalPackages"), FindingValues);

	TSharedRef<FJsonObject> CountsObject = MakeShared<FJsonObject>();
	TArray<FString> Classifications;
	ClassificationCounts.GetKeys(Classifications);
	Classifications.Sort();
	for (const FString& Classification : Classifications)
	{
		CountsObject->SetNumberField(Classification, ClassificationCounts.FindChecked(Classification));
	}
	RootObject->SetObjectField(TEXT("classificationCounts"), CountsObject);

	FString Json;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
	FJsonSerializer::Serialize(RootObject, Writer);
	return Json;
}

bool WriteReport(
	const FWacomContentDependencyAuditReport& Report,
	const FString& OutputPath,
	FString& OutError)
{
	const FString Directory = FPaths::GetPath(OutputPath);
	if (!IFileManager::Get().MakeDirectory(*Directory, /*Tree*/ true))
	{
		OutError = FString::Printf(TEXT("无法创建报告目录：%s"), *Directory);
		return false;
	}
	if (!FFileHelper::SaveStringToFile(
		SerializeReportToJson(Report),
		*OutputPath,
		FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		OutError = FString::Printf(TEXT("无法写入报告：%s"), *OutputPath);
		return false;
	}
	OutError.Reset();
	return true;
}
}
