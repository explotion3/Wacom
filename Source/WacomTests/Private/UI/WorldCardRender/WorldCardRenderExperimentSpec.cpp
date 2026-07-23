// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "../../../../WacomApp/Private/UI/Card/WacomWorldCardRenderExperimentPolicy.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionEyeAdaptationInverse.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "UI/Card/WacomCardView.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomWorldCardRenderExperimentModePolicySpec,
	"Wacom.UI.WorldCardRenderExperiment.ModePolicy.ExactFourModes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomWorldCardRenderExperimentModePolicySpec::RunTest(
	const FString& Parameters)
{
	const TConstArrayView<FWacomWorldCardRenderExperimentModeConfig> Modes =
		FWacomWorldCardRenderExperimentPolicy::GetModes();
	if (!TestEqual(TEXT("exactly four modes"), Modes.Num(), 4))
	{
		return false;
	}

	TestEqual(
		TEXT("mode 0 identity"),
		Modes[0].Mode,
		EWacomWorldCardRenderExperimentMode::EngineTransparent);
	TestEqual(
		TEXT("mode 0 label"),
		FString(Modes[0].Label),
		FString(TEXT("Engine Transparent")));
	TestEqual(
		TEXT("mode 0 blend"),
		Modes[0].BlendMode,
		EWidgetBlendMode::Transparent);
	TestFalse(TEXT("mode 0 uses no custom material"), Modes[0].bUseWacomMaterial);

	TestEqual(
		TEXT("mode 1 identity"),
		Modes[1].Mode,
		EWacomWorldCardRenderExperimentMode::EngineMasked);
	TestEqual(
		TEXT("mode 1 blend"),
		Modes[1].BlendMode,
		EWidgetBlendMode::Masked);
	TestFalse(TEXT("mode 1 uses no custom material"), Modes[1].bUseWacomMaterial);

	TestEqual(
		TEXT("mode 2 identity"),
		Modes[2].Mode,
		EWacomWorldCardRenderExperimentMode::WacomMaskedRaw);
	TestEqual(
		TEXT("mode 2 blend"),
		Modes[2].BlendMode,
		EWidgetBlendMode::Masked);
	TestTrue(TEXT("mode 2 uses custom material"), Modes[2].bUseWacomMaterial);
	TestEqual(
		TEXT("mode 2 disables exposure compensation"),
		Modes[2].ExposureCompensationStrength,
		0.0f);

	TestEqual(
		TEXT("mode 3 identity"),
		Modes[3].Mode,
		EWacomWorldCardRenderExperimentMode::WacomMaskedExposure);
	TestEqual(
		TEXT("mode 3 blend"),
		Modes[3].BlendMode,
		EWidgetBlendMode::Masked);
	TestTrue(TEXT("mode 3 uses custom material"), Modes[3].bUseWacomMaterial);
	TestEqual(
		TEXT("mode 3 defaults to full exposure compensation"),
		Modes[3].ExposureCompensationStrength,
		1.0f);

	TestFalse(
		TEXT("missing world fails closed"),
		FWacomWorldCardRenderExperimentPolicy::IsSupportedPIEWorld(nullptr));
	UWorld* NonPIEWorld = NewObject<UWorld>();
	if (TestNotNull(TEXT("transient policy world can be created"), NonPIEWorld))
	{
		NonPIEWorld->WorldType = EWorldType::Game;
		TestFalse(
			TEXT("game world fails closed"),
			FWacomWorldCardRenderExperimentPolicy::IsSupportedPIEWorld(
				NonPIEWorld));
		NonPIEWorld->WorldType = EWorldType::PIE;
		TestTrue(
			TEXT("PIE world is accepted"),
			FWacomWorldCardRenderExperimentPolicy::IsSupportedPIEWorld(
				NonPIEWorld));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomWorldCardRenderExperimentCommandContractSpec,
	"Wacom.UI.WorldCardRenderExperiment.Commands.Registered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomWorldCardRenderExperimentCommandContractSpec::RunTest(
	const FString& Parameters)
{
	const TCHAR* Commands[] =
	{
		TEXT("Wacom.WorldCardRender.OpenPIEValidation"),
		TEXT("Wacom.WorldCardRender.SetExposureStrength"),
		TEXT("Wacom.WorldCardRender.DumpPIEValidation"),
		TEXT("Wacom.WorldCardRender.ClearPIEValidation")
	};
	for (const TCHAR* Command : Commands)
	{
		TestNotNull(
			*FString::Printf(TEXT("%s is registered"), Command),
			IConsoleManager::Get().FindConsoleObject(Command));
	}

	UClass* CardViewClass = LoadClass<UWacomCardView>(
		nullptr,
		FWacomWorldCardRenderExperimentPolicy::GetCardViewClassPath());
	if (TestNotNull(TEXT("approved card view class loads"), CardViewClass))
	{
		TestTrue(
			TEXT("approved card view derives from UWacomCardView"),
			CardViewClass->IsChildOf(UWacomCardView::StaticClass()));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomWorldCardRenderExperimentMaterialContractSpec,
	"Wacom.UI.WorldCardRenderExperiment.Material.Contract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomWorldCardRenderExperimentMaterialContractSpec::RunTest(
	const FString& Parameters)
{
	UMaterial* Material = LoadObject<UMaterial>(
		nullptr,
		FWacomWorldCardRenderExperimentPolicy::GetWorldMaterialPath());
	if (!TestNotNull(TEXT("world card surface material loads"), Material))
	{
		return false;
	}

	TestEqual(TEXT("material domain is Surface"), Material->MaterialDomain, MD_Surface);
	TestEqual(TEXT("material is Masked"), Material->BlendMode, BLEND_Masked);
	TestTrue(TEXT("material is Unlit"), Material->GetShadingModels().IsUnlit());
	TestTrue(TEXT("material is Two Sided"), Material->IsTwoSided());
	TestEqual(
		TEXT("opacity mask clip value preserves card edges"),
		Material->GetOpacityMaskClipValue(),
		0.1f);

	bool bHasSlateUI = false;
	bool bHasBackColor = false;
	bool bHasTintColorAndOpacity = false;
	bool bHasOpacityFromTexture = false;
	UMaterialExpressionScalarParameter* ExposureStrength = nullptr;
	UMaterialExpressionEyeAdaptationInverse* EyeAdaptationInverse = nullptr;
	for (const TObjectPtr<UMaterialExpression>& Expression :
		Material->GetExpressions())
	{
		if (const UMaterialExpressionTextureSampleParameter2D* TextureParameter =
			Cast<UMaterialExpressionTextureSampleParameter2D>(Expression.Get()))
		{
			bHasSlateUI |= TextureParameter->ParameterName == TEXT("SlateUI");
		}
		if (const UMaterialExpressionVectorParameter* VectorParameter =
			Cast<UMaterialExpressionVectorParameter>(Expression.Get()))
		{
			bHasBackColor |= VectorParameter->ParameterName == TEXT("BackColor");
			bHasTintColorAndOpacity |=
				VectorParameter->ParameterName == TEXT("TintColorAndOpacity");
		}
		if (UMaterialExpressionScalarParameter* ScalarParameter =
			Cast<UMaterialExpressionScalarParameter>(Expression.Get()))
		{
			bHasOpacityFromTexture |=
				ScalarParameter->ParameterName == TEXT("OpacityFromTexture");
			if (ScalarParameter->ParameterName ==
				TEXT("ExposureCompensationStrength"))
			{
				ExposureStrength = ScalarParameter;
			}
		}
		if (UMaterialExpressionEyeAdaptationInverse* EyeExpression =
			Cast<UMaterialExpressionEyeAdaptationInverse>(Expression.Get()))
		{
			EyeAdaptationInverse = EyeExpression;
		}
	}

	TestTrue(TEXT("SlateUI texture parameter exists"), bHasSlateUI);
	TestTrue(TEXT("BackColor vector parameter exists"), bHasBackColor);
	TestTrue(
		TEXT("TintColorAndOpacity vector parameter exists"),
		bHasTintColorAndOpacity);
	TestTrue(
		TEXT("OpacityFromTexture scalar parameter exists"),
		bHasOpacityFromTexture);
	if (TestNotNull(TEXT("ExposureCompensationStrength exists"), ExposureStrength))
	{
		TestEqual(
			TEXT("exposure strength defaults to full conversion"),
			ExposureStrength->DefaultValue,
			1.0f);
	}
	if (TestNotNull(TEXT("EyeAdaptationInverse exists"), EyeAdaptationInverse))
	{
		TestNotNull(
			TEXT("EyeAdaptationInverse receives composed widget color"),
			EyeAdaptationInverse->LightValueInput.Expression);
		TestEqual(
			TEXT("EyeAdaptationInverse alpha is driven directly by exposure strength"),
			EyeAdaptationInverse->AlphaInput.Expression,
			static_cast<UMaterialExpression*>(ExposureStrength));
	}

	const FExpressionInput* EmissiveInput =
		Material->GetExpressionInputForProperty(MP_EmissiveColor);
	const FExpressionInput* OpacityMaskInput =
		Material->GetExpressionInputForProperty(MP_OpacityMask);
	if (TestNotNull(TEXT("Emissive input exists"), EmissiveInput))
	{
		TestEqual(
			TEXT("EyeAdaptationInverse feeds Emissive"),
			EmissiveInput->Expression,
			static_cast<UMaterialExpression*>(EyeAdaptationInverse));
	}
	if (TestNotNull(TEXT("Opacity Mask input exists"), OpacityMaskInput))
	{
		TestNotNull(
			TEXT("Opacity Mask has an authored alpha graph"),
			OpacityMaskInput->Expression);
	}
	return true;
}
