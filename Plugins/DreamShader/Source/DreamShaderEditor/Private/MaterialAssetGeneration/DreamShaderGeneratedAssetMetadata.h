#pragma once

#include "CoreMinimal.h"

class UObject;

namespace UE::DreamShader::Editor::Private
{
	FString BuildSourceFileMetadataValue(const FString& SourceFilePath);
	FString ResolveSourceFileMetadataValue(const FString& SourceFileMetadata);
	FString BuildSourceHash(const FString& SourceText);
	bool IsGeneratedAssetSourceCurrent(UObject* Asset, const FString& SourceFilePath, const FString& SourceHash);
	void ApplySourceMetadata(UObject* Asset, const FString& SourceFilePath);
	void ApplySourceMetadata(UObject* Asset, const FString& SourceFilePath, const FString& SourceHash);
	bool SaveAssetPackage(UObject* Asset, FString& OutError);
}
