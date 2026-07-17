// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class IAssetRegistry;

namespace Wacom::ContentAudit
{
struct FWacomExternalDependencyFinding
{
	FName PackageName;
	FString Classification;
	TArray<FName> DirectWacomReferencers;
	TArray<FName> Referencers;
	TArray<FName> ShortestChain;
	TArray<FString> AssetClasses;
	int32 AssetCount = 0;
	bool bHasOnDiskAsset = false;
	bool bHasHardReference = false;
	bool bHasSoftReference = false;
	bool bHasGameReference = false;
	bool bHasEditorOnlyReference = false;
	bool bHasBuildReference = false;
};

struct FWacomContentDependencyAuditReport
{
	FName ScanRoot;
	int32 ScannedPackageCount = 0;
	int32 TraversedGamePackageCount = 0;
	TArray<FWacomExternalDependencyFinding> ExternalFindings;
	TArray<FName> PlaceholderPackages;
};

/** PackagePath 必须等于 Root 或位于 Root 的真实子目录，避免把 /Game/Wacomish 误判为 /Game/Wacom。 */
WACOMEDITOR_API bool IsPackageUnderRoot(FName PackagePath, FName Root);

/** 返回已知本地依赖分类；其它 /Game 路径统一归入 OtherGame，便于发现未登记目录。 */
WACOMEDITOR_API FString ClassifyExternalPackage(FName PackageName, FName ScanRoot);

/** 从 ScanRoot 下全部磁盘资产出发，遍历 /Game package dependency closure。 */
WACOMEDITOR_API FWacomContentDependencyAuditReport BuildReport(
	IAssetRegistry& AssetRegistry,
	FName ScanRoot);

/** 稳定排序并序列化报告；不写时间戳，便于连续运行比较内容。 */
WACOMEDITOR_API FString SerializeReportToJson(
	const FWacomContentDependencyAuditReport& Report);

/** 发布门槛：任何被遍历到的 /Game/Wacom/Art/Placeholders package 都必须阻断。 */
WACOMEDITOR_API bool HasPlaceholderPackages(
	const FWacomContentDependencyAuditReport& Report);

WACOMEDITOR_API bool WriteReport(
	const FWacomContentDependencyAuditReport& Report,
	const FString& OutputPath,
	FString& OutError);
}
