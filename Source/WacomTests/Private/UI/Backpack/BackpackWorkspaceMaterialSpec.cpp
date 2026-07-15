// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionNamedReroute.h"
#include "Materials/MaterialExpressionVertexColor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceFeedbackMaterialCompilesSpec,
	"Wacom.UI.Backpack.Material.WorkspaceFeedbackCompiles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceFeedbackMaterialCompilesSpec::RunTest(const FString& Parameters)
{
	UMaterial* FeedbackMaterial = LoadObject<UMaterial>(
		nullptr,
		TEXT("/Game/Wacom/UI/Backpack/Materials/M_BackpackWorkspaceCardFeedback.M_BackpackWorkspaceCardFeedback"));
	TestNotNull(TEXT("Workspace feedback material loads"), FeedbackMaterial);
	if (!FeedbackMaterial)
	{
		return false;
	}

	const TArray<FString> CompileErrors = UMaterialEditingLibrary::RecompileMaterial(FeedbackMaterial);
	if (!CompileErrors.IsEmpty())
	{
		AddError(FString::Printf(
			TEXT("Workspace feedback material has compile errors:\n%s"),
			*FString::Join(CompileErrors, TEXT("\n"))));
	}

	auto ResolveNamedRerouteSource = [](const FExpressionInput& Input, int32& OutOutputIndex)
	{
		UMaterialExpression* SourceExpression = Input.Expression;
		OutOutputIndex = Input.OutputIndex;
		TSet<UMaterialExpression*> Visited;
		while (UMaterialExpressionNamedRerouteUsage* Usage = Cast<UMaterialExpressionNamedRerouteUsage>(SourceExpression))
		{
			if (!Usage->Declaration || Visited.Contains(Usage))
			{
				break;
			}

			Visited.Add(Usage);
			const FExpressionInput& DeclarationInput = Usage->Declaration->Input;
			SourceExpression = DeclarationInput.Expression;
			OutOutputIndex = DeclarationInput.OutputIndex;
		}
		return SourceExpression;
	};

	bool bHasVertexColorRGBOutput = false;
	bool bHasVertexColorAlphaOutput = false;
	bool bHasInvalidVertexColorAlphaMask = false;
	for (const TObjectPtr<UMaterialExpression>& Expression : FeedbackMaterial->GetExpressions())
	{
		if (!Expression)
		{
			continue;
		}

		for (int32 InputIndex = 0; InputIndex < 32; ++InputIndex)
		{
			const FExpressionInput* Input = Expression->GetInput(InputIndex);
			if (!Input)
			{
				break;
			}

			int32 SourceOutputIndex = INDEX_NONE;
			UMaterialExpression* Source = ResolveNamedRerouteSource(*Input, SourceOutputIndex);
			if (!Source || !Source->IsA<UMaterialExpressionVertexColor>())
			{
				continue;
			}

			bHasVertexColorRGBOutput |= SourceOutputIndex == 0;
			bHasVertexColorAlphaOutput |= SourceOutputIndex == 4;
			if (const UMaterialExpressionComponentMask* Mask = Cast<UMaterialExpressionComponentMask>(Expression))
			{
				bHasInvalidVertexColorAlphaMask |= SourceOutputIndex == 0
					&& !Mask->R && !Mask->G && !Mask->B && Mask->A;
			}
		}
	}

	TestTrue(TEXT("Workspace feedback material consumes VertexColor RGB output 0"),
		bHasVertexColorRGBOutput);
	TestTrue(TEXT("Workspace feedback material consumes VertexColor alpha output 4"),
		bHasVertexColorAlphaOutput);
	TestFalse(TEXT("Workspace feedback material never applies an alpha mask to VertexColor RGB output 0"),
		bHasInvalidVertexColorAlphaMask);

	return true;
}

#endif
