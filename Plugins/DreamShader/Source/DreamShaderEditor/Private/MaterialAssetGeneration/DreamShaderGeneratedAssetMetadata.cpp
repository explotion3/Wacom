#include "DreamShaderGeneratedAssetMetadata.h"

#include "DreamShaderModule.h"
#include "DreamShaderVersionCompat.h"

#include "FileHelpers.h"
#include "Misc/Crc.h"
#include "Misc/Paths.h"
#include "UObject/MetaData.h"
#include "UObject/Package.h"

namespace UE::DreamShader::Editor::Private
{
	namespace
	{
		FString NormalizeRelativePath(FString Path)
		{
			FPaths::NormalizeFilename(Path);
			FPaths::CollapseRelativeDirectories(Path);
			while (Path.RemoveFromStart(TEXT("./")))
			{
			}
			return Path;
		}

		FString AddTrailingSlash(FString Path)
		{
			if (!Path.EndsWith(TEXT("/")))
			{
				Path += TEXT("/");
			}
			return Path;
		}

		bool IsPathWithinDirectory(const FString& Path, const FString& Directory)
		{
			return Path.Equals(Directory, ESearchCase::IgnoreCase)
				|| Path.StartsWith(AddTrailingSlash(Directory), ESearchCase::IgnoreCase);
		}

		FString MakeRelativePath(const FString& Path, const FString& Directory)
		{
			FString RelativePath = Path;
			FPaths::MakePathRelativeTo(RelativePath, *AddTrailingSlash(Directory));
			return NormalizeRelativePath(MoveTemp(RelativePath));
		}

		FString GetProjectRelativeSourceDirectory()
		{
			const FString ProjectDirectory = UE::DreamShader::NormalizeSourceFilePath(FPaths::ProjectDir());
			const FString SourceDirectory = UE::DreamShader::NormalizeSourceFilePath(UE::DreamShader::GetSourceShaderDirectory());
			return IsPathWithinDirectory(SourceDirectory, ProjectDirectory)
				? MakeRelativePath(SourceDirectory, ProjectDirectory)
				: FString();
		}

		FString GetSourceMetadataValue(UObject* Asset, const TCHAR* Key)
		{
			if (!Asset)
			{
				return FString();
			}

			UPackage* Package = Asset->GetOutermost();
			if (!Package)
			{
				return FString();
			}

#if DREAMSHADER_UE_VERSION_AT_LEAST(5, 6)
			return Package->GetMetaData().GetValue(Asset, Key);
#else
			if (UMetaData* MetaData = Package->GetMetaData())
			{
				return MetaData->GetValue(Asset, Key);
			}
			return FString();
#endif
		}
	}

	FString BuildSourceFileMetadataValue(const FString& SourceFilePath)
	{
		if (SourceFilePath.IsEmpty())
		{
			return FString();
		}

		const FString ProjectDirectory = UE::DreamShader::NormalizeSourceFilePath(FPaths::ProjectDir());
		const FString ProjectRelativeSourceDirectory = GetProjectRelativeSourceDirectory();
		FString NormalizedPath = SourceFilePath;
		FPaths::NormalizeFilename(NormalizedPath);

		if (FPaths::IsRelative(NormalizedPath))
		{
			NormalizedPath = NormalizeRelativePath(MoveTemp(NormalizedPath));
			if (!ProjectRelativeSourceDirectory.IsEmpty()
				&& (NormalizedPath.Equals(ProjectRelativeSourceDirectory, ESearchCase::IgnoreCase)
					|| NormalizedPath.StartsWith(AddTrailingSlash(ProjectRelativeSourceDirectory), ESearchCase::IgnoreCase)))
			{
				return NormalizedPath;
			}

			NormalizedPath = UE::DreamShader::NormalizeSourceFilePath(FPaths::Combine(ProjectDirectory, NormalizedPath));
		}
		else
		{
			NormalizedPath = UE::DreamShader::NormalizeSourceFilePath(NormalizedPath);
		}

		if (IsPathWithinDirectory(NormalizedPath, ProjectDirectory))
		{
			return MakeRelativePath(NormalizedPath, ProjectDirectory);
		}

		// Legacy generated packages stored the absolute path of the worktree that created them.
		// Recover the configured project-relative source suffix so those packages remain current
		// after they are checked out in another worktree.
		if (!ProjectRelativeSourceDirectory.IsEmpty())
		{
			const FString SourceMarker = TEXT("/") + ProjectRelativeSourceDirectory + TEXT("/");
			const int32 MarkerIndex = NormalizedPath.Find(SourceMarker, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
			if (MarkerIndex != INDEX_NONE)
			{
				return NormalizeRelativePath(NormalizedPath.Mid(MarkerIndex + 1));
			}
		}

		return NormalizedPath;
	}

	FString ResolveSourceFileMetadataValue(const FString& SourceFileMetadata)
	{
		const FString StablePath = BuildSourceFileMetadataValue(SourceFileMetadata);
		return StablePath.IsEmpty() || !FPaths::IsRelative(StablePath)
			? StablePath
			: UE::DreamShader::NormalizeSourceFilePath(FPaths::Combine(FPaths::ProjectDir(), StablePath));
	}

	FString BuildSourceHash(const FString& SourceText)
	{
		return FString::Printf(TEXT("%08x"), FCrc::StrCrc32(*SourceText));
	}

	bool IsGeneratedAssetSourceCurrent(UObject* Asset, const FString& SourceFilePath, const FString& SourceHash)
	{
		if (!Asset || SourceHash.IsEmpty())
		{
			return false;
		}

		const FString ExistingSourceFileRaw = GetSourceMetadataValue(Asset, TEXT("DreamShader.SourceFile"));
		if (ExistingSourceFileRaw.IsEmpty())
		{
			return false;
		}

		const FString ExistingSourceFile = BuildSourceFileMetadataValue(ExistingSourceFileRaw);
		const FString ExistingSourceHash = GetSourceMetadataValue(Asset, TEXT("DreamShader.SourceHash"));

		return ExistingSourceFile.Equals(BuildSourceFileMetadataValue(SourceFilePath), ESearchCase::IgnoreCase)
			&& ExistingSourceHash.Equals(SourceHash, ESearchCase::CaseSensitive);
	}

	void ApplySourceMetadata(UObject* Asset, const FString& SourceFilePath)
	{
		ApplySourceMetadata(Asset, SourceFilePath, FString());
	}

	void ApplySourceMetadata(UObject* Asset, const FString& SourceFilePath, const FString& SourceHash)
	{
		if (!Asset)
		{
			return;
		}

		UPackage* Package = Asset->GetOutermost();
		if (!Package)
		{
			return;
		}

#if DREAMSHADER_UE_VERSION_AT_LEAST(5, 6)
		FMetaData& MetaData = Package->GetMetaData();
		MetaData.SetValue(Asset, TEXT("DreamShader.SourceFile"), *BuildSourceFileMetadataValue(SourceFilePath));
		if (!SourceHash.IsEmpty())
		{
			MetaData.SetValue(Asset, TEXT("DreamShader.SourceHash"), *SourceHash);
			MetaData.SetValue(Asset, TEXT("DreamShader.GeneratedAtUtc"), *FDateTime::UtcNow().ToIso8601());
		}
#else
		UMetaData* MetaData = Package->GetMetaData();
		if (!MetaData)
		{
			return;
		}
		MetaData->SetValue(Asset, TEXT("DreamShader.SourceFile"), *BuildSourceFileMetadataValue(SourceFilePath));
		if (!SourceHash.IsEmpty())
		{
			MetaData->SetValue(Asset, TEXT("DreamShader.SourceHash"), *SourceHash);
			MetaData->SetValue(Asset, TEXT("DreamShader.GeneratedAtUtc"), *FDateTime::UtcNow().ToIso8601());
		}
#endif
	}

	bool SaveAssetPackage(UObject* Asset, FString& OutError)
	{
		check(Asset);

		TArray<UPackage*> PackagesToSave;
		PackagesToSave.Add(Asset->GetOutermost());
		if (!UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, true))
		{
			OutError = FString::Printf(TEXT("Generated DreamShader asset '%s' could not be saved."), *Asset->GetPathName());
			return false;
		}

		return true;
	}
}
