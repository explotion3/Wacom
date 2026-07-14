// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyPartActor.h"
#include "Actors/WacomBattleEnemyPartImpactStyle.h"
#include "Actors/WacomBattleEnemyPartTargetPreviewStyle.h"
#include "Components/WacomBattleEnemyPartPresentationComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"
#include "WacomBattleEnemyPartPresentationTestAccess.h"

#if WITH_AUTOMATION_TESTS

namespace WacomBattleSceneEnemyImpactSpec
{
	UWorld* FindAutomationWorld()
	{
		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (UWorld* World = Context.World())
				{
					return World;
				}
			}
		}
		return GWorld;
	}

	AWacomBattleEnemyPartActor* SpawnPart(UWorld& World)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.ObjectFlags |= RF_Transient;
		AWacomBattleEnemyPartActor* Part = World.SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
		if (Part)
		{
			Part->PartId = TEXT("Test.Part.Head");
			Part->PartSlotId = TEXT("Head");
			Part->SetEnemySlotId(TEXT("Enemy"));
		}
		return Part;
	}

	FWacomBattlePresentationTargetCue MakeCue(
		EWacomBattlePresentationTargetCueKind Kind,
		float Duration,
		int32 Amount,
		int32 Seed)
	{
		FWacomBattlePresentationTargetCue Cue;
		Cue.CueKind = Kind;
		Cue.Duration = Duration;
		Cue.Amount = Amount;
		Cue.Seed = Seed;
		return Cue;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyImpactStyleAndCueSpec,
	"Wacom.UI.Battle.BattleSceneEnemyImpact.StyleInheritanceIntensityPriorityAndAccessibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyImpactStyleAndCueSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyImpactSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleEnemyPartActor* Part = WacomBattleSceneEnemyImpactSpec::SpawnPart(*World);
	if (!TestNotNull(TEXT("Enemy part"), Part))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Part))
		{
			Part->Destroy();
		}
	};

	UWacomBattleEnemyPartImpactStyle* HostStyle =
		NewObject<UWacomBattleEnemyPartImpactStyle>(Part);
	UWacomBattleEnemyPartImpactStyle* OverrideStyle =
		NewObject<UWacomBattleEnemyPartImpactStyle>(Part);
	OverrideStyle->TargetConfirmedIntensity = 0.83f;
	Part->SetHostImpactStyle(HostStyle);
	TestEqual(TEXT("Host Style is used without an override"), Part->ResolveImpactStyle(), HostStyle);
	Part->ImpactStyleOverride = OverrideStyle;
	Part->RefreshAuthoringState();
	TestEqual(TEXT("Part Style overrides Host Style"), Part->ResolveImpactStyle(), OverrideStyle);
	UWacomBattleEnemyPartTargetPreviewStyle* HostPreviewStyle =
		NewObject<UWacomBattleEnemyPartTargetPreviewStyle>(Part);
	UWacomBattleEnemyPartTargetPreviewStyle* OverridePreviewStyle =
		NewObject<UWacomBattleEnemyPartTargetPreviewStyle>(Part);
	Part->SetHostTargetPreviewStyle(HostPreviewStyle);
	TestEqual(TEXT("Host target preview Style is inherited"),
		Part->ResolveTargetPreviewStyle(), HostPreviewStyle);
	Part->TargetPreviewStyleOverride = OverridePreviewStyle;
	Part->RefreshAuthoringState();
	TestEqual(TEXT("Part target preview Style overrides Host Style"),
		Part->ResolveTargetPreviewStyle(), OverridePreviewStyle);
	TestFalse(TEXT("Missing target preview System/MI is a safe visual fallback"),
		OverridePreviewStyle->HasValidVisualAssets());
	TestTrue(TEXT("Target preview default valid coverage is 1.10x"),
		FMath::IsNearlyEqual(OverridePreviewStyle->ValidCoverageMultiplier, 1.10f));

	TestTrue(TEXT("Missing System/MI is a safe visual fallback"), !OverrideStyle->HasValidVisualAssets());
	TestTrue(TEXT("Damage intensity uses the approved sqrt mapping"),
		FMath::IsNearlyEqual(OverrideStyle->ResolveDamageIntensity(100), 1.75f));
	TestTrue(TEXT("Damage intensity clamps to the lower bound"),
		FMath::IsNearlyEqual(OverrideStyle->ResolveDamageIntensity(0), 0.80f));
	TestTrue(TEXT("Damage intensity clamps to the upper bound"),
		FMath::IsNearlyEqual(OverrideStyle->ResolveDamageIntensity(10000), 1.80f));
	TestTrue(TEXT("TargetConfirmed covers the authored part bounds at 1.2x"),
		FMath::IsNearlyEqual(OverrideStyle->TargetConfirmedCoverageMultiplier, 1.20f));
	TestTrue(TEXT("Damage covers the authored part bounds at 1.2x"),
		FMath::IsNearlyEqual(OverrideStyle->DamageCoverageMultiplier, 1.20f));

	UWacomBattleEnemyPartPresentationComponent* Presentation = Part->GetPresentationComponent();
	if (!TestNotNull(TEXT("Presentation component"), Presentation))
	{
		return false;
	}

	Presentation->PlayBattlePresentationCue(
		WacomBattleSceneEnemyImpactSpec::MakeCue(
			EWacomBattlePresentationTargetCueKind::TargetConfirmed,
			0.10f,
			0,
			101));
	FWacomBattleEnemyPartPresentationDebugView View =
		Presentation->GetBattleEnemyPartPresentationDebugView();
	TestEqual(TEXT("TargetConfirmed request kind"), View.LastImpactEffectKind, FName(TEXT("TargetConfirmed")));
	TestTrue(TEXT("TargetConfirmed intensity comes from Style"),
		FMath::IsNearlyEqual(View.LastImpactIntensity, 0.83f));
	TestEqual(TEXT("TargetConfirmed keeps the stable seed"), View.LastImpactSeed, 101);
	TestTrue(TEXT("TargetConfirmed resolves a readable HitBounds-driven diameter"),
		View.LastImpactTargetDiameterCentimeters >= OverrideStyle->MinimumImpactDiameterCentimeters
		&& View.LastImpactTargetDiameterCentimeters <= OverrideStyle->MaximumImpactDiameterCentimeters);
	TestEqual(TEXT("Invalid visual assets do not create Niagara"), View.ImpactEffectPlayCount, 0);
	TestFalse(TEXT("Invalid visual assets are reported not ready"), View.bImpactNiagaraReady);

	Presentation->PlayBattlePresentationCue(
		WacomBattleSceneEnemyImpactSpec::MakeCue(
			EWacomBattlePresentationTargetCueKind::DamageDealt,
			0.18f,
			100,
			202));
	View = Presentation->GetBattleEnemyPartPresentationDebugView();
	TestEqual(TEXT("Damage replaces TargetConfirmed"), View.ActiveCueKind, FName(TEXT("Damage")));
	TestEqual(TEXT("Damage request kind"), View.LastImpactEffectKind, FName(TEXT("Damage")));
	TestTrue(TEXT("Damage request uses mapped intensity"),
		FMath::IsNearlyEqual(View.LastImpactIntensity, 1.75f));
	TestEqual(TEXT("Damage keeps the stable seed"), View.LastImpactSeed, 202);

	Presentation->PlayBattlePresentationCue(
		WacomBattleSceneEnemyImpactSpec::MakeCue(
			EWacomBattlePresentationTargetCueKind::TargetConfirmed,
			0.10f,
			0,
			303));
	View = Presentation->GetBattleEnemyPartPresentationDebugView();
	TestEqual(TEXT("Rejected lower-priority cue does not reach the controller"), View.LastImpactSeed, 202);

	Presentation->ResetBattlePresentationFeedback();
	FWacomBattleEnemyPartPresentationTestAccess::SetAccessibility(*Presentation, 0.35f, true);
	Presentation->PlayBattlePresentationCue(
		WacomBattleSceneEnemyImpactSpec::MakeCue(
			EWacomBattlePresentationTargetCueKind::DamageDealt,
			0.18f,
			4,
			404));
	View = Presentation->GetBattleEnemyPartPresentationDebugView();
	TestTrue(TEXT("Simplified motion is snapshotted per accepted cue"), View.bImpactReducedMotion);
	TestTrue(TEXT("Reduced flash uses the shared 35 percent policy"),
		FMath::IsNearlyEqual(View.ImpactDecorativeIntensity, 0.35f));

	Part->bEnableImpactFeedback = false;
	Part->RefreshAuthoringState();
	Presentation->ResetBattlePresentationFeedback();
	Presentation->PlayBattlePresentationCue(
		WacomBattleSceneEnemyImpactSpec::MakeCue(
			EWacomBattlePresentationTargetCueKind::TargetConfirmed,
			0.10f,
			0,
			505));
	View = Presentation->GetBattleEnemyPartPresentationDebugView();
	TestFalse(TEXT("Disabled feedback still consumes the Cue but exposes disabled state"),
		View.bImpactFeedbackEnabled);
	TestEqual(TEXT("Disabled feedback does not submit a new impact request"), View.LastImpactSeed, 404);
	TestTrue(TEXT("Disabled feedback does not block Cue playback"), View.bCuePlaybackActive);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyImpactDreamShaderContractSpec,
	"Wacom.UI.Battle.BattleSceneEnemyImpact.DreamShaderContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyImpactDreamShaderContractSpec::RunTest(
	const FString& /*Parameters*/)
{
	const FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	FString MaterialSource;
	FString HelperSource;
	FString NiagaraBuilderSource;
	const FString MaterialPath = FPaths::Combine(
		ProjectDir,
		TEXT("DShader/Material/World/M_WacomBattleEnemyPartImpactPixel.dsm"));
	const FString HelperPath = FPaths::Combine(
		ProjectDir,
		TEXT("DShader/Shared/WacomBattleEnemyPartImpactPixel.dsh"));
	const FString NiagaraBuilderPath = FPaths::Combine(
		ProjectDir,
		TEXT("Source/WacomEditor/Private/ContentBuilders/BattleEnemyPartImpactNiagaraBuilder.cpp"));
	TestTrue(TEXT("Enemy impact material source is readable"),
		FFileHelper::LoadFileToString(MaterialSource, *MaterialPath));
	TestTrue(TEXT("Enemy impact helper source is readable"),
		FFileHelper::LoadFileToString(HelperSource, *HelperPath));
	TestTrue(TEXT("Enemy impact Niagara builder source is readable"),
		FFileHelper::LoadFileToString(NiagaraBuilderSource, *NiagaraBuilderPath));
	TestTrue(TEXT("Material uses Surface domain"), MaterialSource.Contains(TEXT("Domain = \"Surface\"")));
	TestTrue(TEXT("Material is Unlit"), MaterialSource.Contains(TEXT("ShadingModel = \"Unlit\"")));
	TestTrue(TEXT("Material is Translucent"), MaterialSource.Contains(TEXT("BlendMode = \"Translucent\"")));
	TestTrue(TEXT("Material is TwoSided"), MaterialSource.Contains(TEXT("TwoSided = true")));
	TestTrue(TEXT("Material reads Niagara Dynamic Parameters"), MaterialSource.Contains(TEXT("DynamicParameter ParticleData")));
	TestTrue(TEXT("Dynamic W preserves semantic/decorative classification"), MaterialSource.Contains(TEXT("decorativeClass")));
	TestTrue(TEXT("Material exposes an invalid target color"),
		MaterialSource.Contains(TEXT("ImpactInvalidTargetColor")));
	TestTrue(TEXT("Material contains target preview shape branches"),
		HelperSource.Contains(TEXT("isPreviewFrame"))
		&& HelperSource.Contains(TEXT("isPreviewCore")));
	TestTrue(TEXT("Material reads alpha from Niagara Particle Color"),
		MaterialSource.Contains(TEXT("Class=\"ParticleColor\", OutputType=\"float1\", Output=\"A\"")));
	TestFalse(TEXT("Material does not mask alpha from the RGB-only Vertex Color default output"),
		MaterialSource.Contains(TEXT("UE.VertexColor()")));
	TestFalse(TEXT("Material has no texture dependency"), MaterialSource.Contains(TEXT("TextureSample")));
	TestFalse(TEXT("Material has no global Time dependency"), MaterialSource.Contains(TEXT("UE.Time")));
	TestFalse(TEXT("Helper has no random Noise dependency"), HelperSource.Contains(TEXT("Noise")));
	TestTrue(TEXT("Helper creates hard pixel shapes with step"), HelperSource.Contains(TEXT("step(")));
	TestTrue(TEXT("Niagara consumes an adaptive target diameter"),
		NiagaraBuilderSource.Contains(TEXT("User.TargetDiameter")));
	TestTrue(TEXT("Niagara builder owns target preview width/height and lifecycle parameters"),
		NiagaraBuilderSource.Contains(TEXT("User.TargetWidth"))
		&& NiagaraBuilderSource.Contains(TEXT("User.TargetHeight"))
		&& NiagaraBuilderSource.Contains(TEXT("User.PreviewAmount"))
		&& NiagaraBuilderSource.Contains(TEXT("TargetPreviewEmitter")));
	return true;
}

#endif
