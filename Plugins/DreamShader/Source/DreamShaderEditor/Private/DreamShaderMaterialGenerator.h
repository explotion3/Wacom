#pragma once

#include "CoreMinimal.h"

namespace UE::DreamShader::Editor
{
	class FMaterialGenerator
	{
	public:
		static bool GenerateAssetsFromFile(const FString& SourceFilePath, FString& OutMessage, bool bForce = false);
		static bool GenerateMaterialFromFile(const FString& SourceFilePath, FString& OutMessage, bool bForce = false);
	};
}
