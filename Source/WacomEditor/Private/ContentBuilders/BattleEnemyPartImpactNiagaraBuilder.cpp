// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/BattleEnemyPartImpactNiagaraBuilder.h"

#include "ContentBuilders/ContentBuilderHelpers.h"
#include "Misc/PackageName.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraExternalSystemEditorUtilities.h"
#include "NiagaraEditorUtilities.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraScript.h"
#include "NiagaraSystem.h"
#include "NiagaraTypes.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace
{
	constexpr TCHAR ImpactSystemPackagePath[] =
		TEXT("/Game/Wacom/VFX/Battle/NS_WacomBattleEnemyPartImpact_Pixel");
	constexpr TCHAR ImpactSystemObjectPath[] =
		TEXT("/Game/Wacom/VFX/Battle/NS_WacomBattleEnemyPartImpact_Pixel.NS_WacomBattleEnemyPartImpact_Pixel");
	const FName ImpactContractVersionMetadataKey(TEXT("WacomEnemyImpactContractVersion"));
	constexpr TCHAR ImpactContractVersion[] = TEXT("2");

	constexpr TCHAR EmitterStatePath[] =
		TEXT("/Niagara/Modules/Emitter/EmitterState.EmitterState");
	constexpr TCHAR SpawnBurstPath[] =
		TEXT("/Niagara/Modules/Emitter/SpawnBurst_Instantaneous.SpawnBurst_Instantaneous");
	constexpr TCHAR InitializeParticlePath[] =
		TEXT("/Niagara/Modules/Spawn/Initialization/V2/InitializeParticle.InitializeParticle");
	constexpr TCHAR ParticleStatePath[] =
		TEXT("/Niagara/Modules/Update/Lifetime/ParticleState.ParticleState");

	const FName ConfirmStampEmitter(TEXT("ConfirmStamp"));
	const FName DamageCoreWaveEmitter(TEXT("DamageCoreWave"));
	const FName DamageFragmentsEmitter(TEXT("DamageFragments"));
	const FName DestroyedFractureEmitter(TEXT("DestroyedFracture"));
	const FName DestroyedFragmentsEmitter(TEXT("DestroyedFragments"));
	const FName TargetPreviewEmitter(TEXT("TargetPreview"));
	const TArray<FName> RequiredEmitterNames = {
		ConfirmStampEmitter,
		DamageCoreWaveEmitter,
		DamageFragmentsEmitter,
		DestroyedFractureEmitter,
		DestroyedFragmentsEmitter,
		TargetPreviewEmitter,
	};

	const FName EmitterUpdateScript(TEXT("EmitterUpdateScript"));
	const FName ParticleSpawnScript(TEXT("ParticleSpawnScript"));
	const FName ParticleUpdateScript(TEXT("ParticleUpdateScript"));

	struct FParameterExpression
	{
		FNiagaraExt_SetParameterEntry Entry;
		FString Expression;
	};

	template <typename TValue>
	FNiagaraExt_SetParameterEntry MakeParameterEntry(
		const FName Name,
		const FNiagaraTypeDefinition& Type,
		const TValue& DefaultValue)
	{
		FNiagaraExt_SetParameterEntry Entry;
		Entry.Variable.Name = Name;
		Entry.Variable.Type = Type;
		FNiagaraVariant Variant;
		Variant.SetBytesValue(Type, DefaultValue);
		Entry.DefaultValue.Set(Type, Variant);
		return Entry;
	}

	template <typename TValue>
	FParameterExpression MakeParameterExpression(
		const TCHAR* Name,
		const FNiagaraTypeDefinition& Type,
		const TValue& DefaultValue,
		const TCHAR* Expression)
	{
		FParameterExpression Result;
		Result.Entry = MakeParameterEntry(FName(Name), Type, DefaultValue);
		Result.Expression = Expression;
		return Result;
	}

	void LogScriptTopology(
		const FName EmitterName,
		const FNiagaraExt_ScriptStackTopology& Script)
	{
		UE_LOG(LogTemp, Display, TEXT("[EnemyImpactNiagara] Emitter=%s Script=%s Modules=%d"),
			*EmitterName.ToString(), *Script.ScriptName.ToString(), Script.Modules.Num());
		for (const FNiagaraExt_ModuleTopology& Module : Script.Modules)
		{
			UE_LOG(LogTemp, Display, TEXT("[EnemyImpactNiagara]   Module=%s Asset=%s SetParameters=%s"),
				*Module.ModuleName.ToString(),
				Module.ModuleScript ? *Module.ModuleScript->GetPathName() : TEXT("None"),
				Module.bIsSetParametersModule ? TEXT("true") : TEXT("false"));
			for (const FNiagaraExt_StackInputTopology& Input : Module.Inputs)
			{
				if (!Input.bIsVisible)
				{
					continue;
				}
				UE_LOG(LogTemp, Display,
					TEXT("[EnemyImpactNiagara]     Input=%s Type=%s Editable=%s Expression=%s"),
					*Input.Name.ToString(),
					*Input.Type.GetName(),
					Input.bIsEditable ? TEXT("true") : TEXT("false"),
					Input.bIsDynamic ? TEXT("dynamic") : TEXT("direct"));
			}
		}
	}

	bool LogContextErrors(const FNiagaraExternalEditContext& Context)
	{
		for (const FText& Error : Context.Errors)
		{
			UE_LOG(LogTemp, Error, TEXT("[EnemyImpactNiagara] %s"), *Error.ToString());
		}
		return !Context.HasErrors();
	}

	const FNiagaraExt_ScriptStackTopology* FindScriptTopology(
		const FNiagaraExt_EmitterTopology& Topology,
		const FName ScriptName)
	{
		if (Topology.EmitterSpawnScript.ScriptName == ScriptName)
		{
			return &Topology.EmitterSpawnScript;
		}
		if (Topology.EmitterUpdateScript.ScriptName == ScriptName)
		{
			return &Topology.EmitterUpdateScript;
		}
		if (Topology.ParticleSpawnScript.ScriptName == ScriptName)
		{
			return &Topology.ParticleSpawnScript;
		}
		if (Topology.ParticleUpdateScript.ScriptName == ScriptName)
		{
			return &Topology.ParticleUpdateScript;
		}
		return nullptr;
	}

	bool RemoveAllModules(
		UNiagaraSystem& System,
		const FName EmitterName,
		const FName ScriptName,
		FNiagaraExternalEditContext& Context)
	{
		FNiagaraExt_EmitterTopology Topology;
		UNiagaraExternalEditUtilities::GetEmitterTopology(
			FNiagaraExt_StackItemReference(&System, EmitterName),
			Topology,
			Context);
		const FNiagaraExt_ScriptStackTopology* Script = FindScriptTopology(Topology, ScriptName);
		if (!Script)
		{
			Context.Error(FText::Format(
				NSLOCTEXT("WacomNiagaraBuilder", "MissingScript", "Emitter '{0}' is missing script '{1}'."),
				FText::FromName(EmitterName),
				FText::FromName(ScriptName)));
			return false;
		}

		TArray<FName> ModuleNames;
		for (const FNiagaraExt_ModuleTopology& Module : Script->Modules)
		{
			ModuleNames.Add(Module.ModuleName);
		}
		for (const FName ModuleName : ModuleNames)
		{
			UNiagaraExternalEditUtilities::RemoveModule(
				FNiagaraExt_StackItemReference(&System, EmitterName, ScriptName, ModuleName),
				Context);
		}
		return !Context.HasErrors();
	}

	const FNiagaraExt_StackInputTopology* FindInput(
		const FNiagaraExt_ModuleTopology& Module,
		const FName DesiredName)
	{
		const FString Desired = DesiredName.ToString();
		for (const FNiagaraExt_StackInputTopology& Input : Module.Inputs)
		{
			const FString Candidate = Input.Name.ToString();
			if (Input.Name == DesiredName
				|| Candidate.EndsWith(Desired, ESearchCase::CaseSensitive))
			{
				return &Input;
			}
		}
		return nullptr;
	}

	bool SetInputExpression(
		UNiagaraSystem& System,
		const FName EmitterName,
		const FName ScriptName,
		const FNiagaraExt_ModuleTopology& Module,
		const FName InputName,
		const FString& Expression,
		FNiagaraExternalEditContext& Context)
	{
		const FNiagaraExt_StackInputTopology* Input = FindInput(Module, InputName);
		if (!Input)
		{
			Context.Error(FText::Format(
				NSLOCTEXT("WacomNiagaraBuilder", "MissingInput", "Module '{0}' is missing input '{1}'."),
				FText::FromName(Module.ModuleName),
				FText::FromName(InputName)));
			return false;
		}

		FNiagaraExt_StackItemReference InputRef(
			&System,
			EmitterName,
			ScriptName,
			Module.ModuleName);
		InputRef.InputNameStack.Add(Input->Name);
		FNiagaraExt_StackInputValue Value;
		FNiagaraExt_StackInputData_HlslExpression& Hlsl =
			Value.InitializeAs<FNiagaraExt_StackInputData_HlslExpression>();
		Hlsl.HlslExpression = Expression;
		UNiagaraExternalEditUtilities::SetStackInputData(InputRef, Value, Context);
		return !Context.HasErrors();
	}

	bool AddModule(
		UNiagaraSystem& System,
		const FName EmitterName,
		const FName ScriptName,
		const TCHAR* ModulePath,
		FNiagaraExt_ModuleTopology& OutTopology,
		FNiagaraExternalEditContext& Context)
	{
		const UNiagaraScript* Module = LoadObject<UNiagaraScript>(nullptr, ModulePath);
		if (!Module)
		{
			Context.Error(FText::Format(
				NSLOCTEXT("WacomNiagaraBuilder", "MissingModuleAsset", "Missing Niagara module asset '{0}'."),
				FText::FromString(ModulePath)));
			return false;
		}
		UNiagaraExternalEditUtilities::AddModule(
			FNiagaraExt_StackItemReference(&System, EmitterName, ScriptName),
			Module,
			OutTopology,
			Context);
		return !Context.HasErrors();
	}

	bool AddBurst(
		UNiagaraSystem& System,
		const FName EmitterName,
		const TCHAR* SpawnCountExpression,
		FNiagaraExternalEditContext& Context)
	{
		FNiagaraExt_ModuleTopology EmitterState;
		if (!AddModule(System, EmitterName, EmitterUpdateScript, EmitterStatePath, EmitterState, Context))
		{
			return false;
		}

		FNiagaraExt_ModuleTopology Burst;
		if (!AddModule(System, EmitterName, EmitterUpdateScript, SpawnBurstPath, Burst, Context))
		{
			return false;
		}
		return SetInputExpression(
			System,
			EmitterName,
			EmitterUpdateScript,
			Burst,
			TEXT("Spawn Count"),
			SpawnCountExpression,
			Context);
	}

	bool AddAssignmentModule(
		UNiagaraSystem& System,
		const FName EmitterName,
		const FName ScriptName,
		const TArray<FParameterExpression>& Parameters,
		FNiagaraExternalEditContext& Context)
	{
		TArray<FNiagaraExt_SetParameterEntry> Entries;
		Entries.Reserve(Parameters.Num());
		for (const FParameterExpression& Parameter : Parameters)
		{
			Entries.Add(Parameter.Entry);
		}

		FNiagaraExt_ModuleTopology Assignment;
		UNiagaraExternalEditUtilities::AddSetParametersModule(
			FNiagaraExt_StackItemReference(&System, EmitterName, ScriptName),
			Entries,
			Assignment,
			Context);
		if (Context.HasErrors())
		{
			return false;
		}

		for (const FParameterExpression& Parameter : Parameters)
		{
			if (!SetInputExpression(
				System,
				EmitterName,
				ScriptName,
				Assignment,
				Parameter.Entry.Variable.Name,
				Parameter.Expression,
				Context))
			{
				return false;
			}
		}
		return true;
	}

	TArray<FParameterExpression> MakeCommonSpawnParameters(
		const TCHAR* LifetimeExpression,
		const TCHAR* ShapeExpression,
		const TCHAR* InitialSizeExpression,
		const TCHAR* BaseAlphaExpression,
		const TCHAR* PaletteExpression,
		const TCHAR* DecorativeExpression)
	{
		const FNiagaraTypeDefinition& FloatType = FNiagaraTypeDefinition::GetFloatDef();
		const FNiagaraTypeDefinition& PositionType = FNiagaraTypeDefinition::GetPositionDef();
		const FNiagaraTypeDefinition& Vec2Type = FNiagaraTypeDefinition::GetVec2Def();
		const FNiagaraTypeDefinition& Vec4Type = FNiagaraTypeDefinition::GetVec4Def();
		const FNiagaraTypeDefinition& ColorType = FNiagaraTypeDefinition::GetColorDef();

		return {
			MakeParameterExpression(TEXT("Particles.Lifetime"), FloatType, 0.1f, LifetimeExpression),
			MakeParameterExpression(TEXT("Particles.Position"), PositionType, FNiagaraPosition(ForceInit), TEXT("Engine.Owner.Position")),
			MakeParameterExpression(TEXT("Particles.WacomShapeKind"), FloatType, 0.0f, ShapeExpression),
			MakeParameterExpression(TEXT("Particles.WacomInitialSpriteSize"), Vec2Type, FVector2f(16.0f), InitialSizeExpression),
			MakeParameterExpression(TEXT("Particles.SpriteSize"), Vec2Type, FVector2f(16.0f), InitialSizeExpression),
			MakeParameterExpression(TEXT("Particles.WacomBaseAlpha"), FloatType, 1.0f, BaseAlphaExpression),
			MakeParameterExpression(TEXT("Particles.Color"), ColorType, FLinearColor::White,
				*FString::Printf(TEXT("float4(1.0, 1.0, 1.0, %s)"), BaseAlphaExpression)),
			MakeParameterExpression(TEXT("Particles.WacomPaletteVariant"), FloatType, 0.0f, PaletteExpression),
			MakeParameterExpression(TEXT("Particles.WacomDecorativeClass"), FloatType, 0.0f, DecorativeExpression),
			MakeParameterExpression(TEXT("Particles.DynamicMaterialParameter"), Vec4Type, FVector4f(0.0f),
				*FString::Printf(
					TEXT("float4(%s, 0.0, %s, %s)"),
					ShapeExpression,
					PaletteExpression,
					DecorativeExpression)),
		};
	}

	TArray<FParameterExpression> MakeCommonUpdateParameters(
		const TCHAR* PositionExpression,
		const TCHAR* SizeExpression)
	{
		const FNiagaraTypeDefinition& FloatType = FNiagaraTypeDefinition::GetFloatDef();
		const FNiagaraTypeDefinition& PositionType = FNiagaraTypeDefinition::GetPositionDef();
		const FNiagaraTypeDefinition& Vec2Type = FNiagaraTypeDefinition::GetVec2Def();
		const FNiagaraTypeDefinition& Vec4Type = FNiagaraTypeDefinition::GetVec4Def();
		const FNiagaraTypeDefinition& ColorType = FNiagaraTypeDefinition::GetColorDef();

		return {
			MakeParameterExpression(TEXT("Particles.Position"), PositionType, FNiagaraPosition(ForceInit), PositionExpression),
			MakeParameterExpression(TEXT("Particles.SpriteSize"), Vec2Type, FVector2f(16.0f), SizeExpression),
			MakeParameterExpression(TEXT("Particles.Color"), ColorType, FLinearColor::White,
				TEXT("float4(1.0, 1.0, 1.0, Particles.WacomBaseAlpha * (1.0 - saturate(Particles.NormalizedAge)))")),
			MakeParameterExpression(TEXT("Particles.DynamicMaterialParameter"), Vec4Type, FVector4f(0.0f),
				TEXT("float4(Particles.WacomShapeKind, saturate(Particles.NormalizedAge), Particles.WacomPaletteVariant, Particles.WacomDecorativeClass)")),
		};
	}

	bool AddParticleScaffold(
		UNiagaraSystem& System,
		const FName EmitterName,
		const TArray<FParameterExpression>& SpawnParameters,
		const TArray<FParameterExpression>& UpdateParameters,
		FNiagaraExternalEditContext& Context)
	{
		FNiagaraExt_ModuleTopology InitializeParticle;
		if (!AddModule(System, EmitterName, ParticleSpawnScript, InitializeParticlePath, InitializeParticle, Context)
			|| !AddAssignmentModule(System, EmitterName, ParticleSpawnScript, SpawnParameters, Context))
		{
			return false;
		}

		FNiagaraExt_ModuleTopology ParticleState;
		return AddModule(System, EmitterName, ParticleUpdateScript, ParticleStatePath, ParticleState, Context)
			&& AddAssignmentModule(System, EmitterName, ParticleUpdateScript, UpdateParameters, Context);
	}

	bool BuildConfirmStamp(UNiagaraSystem& System, FNiagaraExternalEditContext& Context)
	{
		if (!AddBurst(System, ConfirmStampEmitter, TEXT("User.EffectKind == 0 ? 2 : 0"), Context))
		{
			return false;
		}

		const TCHAR* Shape = TEXT("Engine.ExecIndex == 0 ? 1.0 : 3.0");
		const TCHAR* Size = TEXT("float2(1.0, 1.0) * User.TargetDiameter * (Engine.ExecIndex == 0 ? 1.0 : 0.30) * lerp(0.94, 1.06, saturate(User.Intensity))");
		const TCHAR* Alpha = TEXT("Engine.ExecIndex == 0 ? User.DecorativeIntensity : 1.0");
		const TCHAR* Palette = TEXT("frac((float)(User.Seed + Engine.ExecIndex * 37) * 0.61803398875)");
		const TCHAR* Decorative = TEXT("Engine.ExecIndex == 0 ? 1.0 : 0.0");
		return AddParticleScaffold(
			System,
			ConfirmStampEmitter,
			MakeCommonSpawnParameters(TEXT("max(User.Duration, 0.01)"), Shape, Size, Alpha, Palette, Decorative),
			MakeCommonUpdateParameters(
				TEXT("Particles.Position"),
				TEXT("User.ReducedMotion ? Particles.WacomInitialSpriteSize : Particles.WacomInitialSpriteSize * lerp(1.0, Particles.WacomShapeKind < 2.0 ? 0.38 : 0.72, saturate(Particles.NormalizedAge))")),
			Context);
	}

	bool BuildDamageCoreWave(UNiagaraSystem& System, FNiagaraExternalEditContext& Context)
	{
		if (!AddBurst(System, DamageCoreWaveEmitter, TEXT("User.EffectKind == 1 ? 2 : 0"), Context))
		{
			return false;
		}

		const TCHAR* Lifetime = TEXT("max(User.Duration * (Engine.ExecIndex == 0 ? 0.20 : 0.60), 0.01)");
		const TCHAR* Shape = TEXT("Engine.ExecIndex == 0 ? 3.0 : 2.0");
		const TCHAR* Size = TEXT("float2(1.0, 1.0) * User.TargetDiameter * (Engine.ExecIndex == 0 ? 0.28 : 0.25) * lerp(0.96, 1.12, saturate((User.Intensity - 0.8) / 1.0))");
		const TCHAR* Alpha = TEXT("Engine.ExecIndex == 0 ? 1.0 : User.DecorativeIntensity");
		const TCHAR* Palette = TEXT("frac((float)(User.Seed + Engine.ExecIndex * 53) * 0.754877666)");
		const TCHAR* Decorative = TEXT("Engine.ExecIndex == 0 ? 0.0 : 1.0");
		return AddParticleScaffold(
			System,
			DamageCoreWaveEmitter,
			MakeCommonSpawnParameters(Lifetime, Shape, Size, Alpha, Palette, Decorative),
			MakeCommonUpdateParameters(
				TEXT("Particles.Position"),
				TEXT("User.ReducedMotion ? Particles.WacomInitialSpriteSize : Particles.WacomInitialSpriteSize * (Particles.WacomShapeKind > 1.5 && Particles.WacomShapeKind < 2.5 ? lerp(1.0, 4.0, 1.0 - pow(1.0 - saturate(Particles.NormalizedAge), 3.0)) : lerp(1.0, 0.88, saturate(Particles.NormalizedAge)))")),
			Context);
	}

	bool BuildDamageFragments(UNiagaraSystem& System, FNiagaraExternalEditContext& Context)
	{
		const TCHAR* Count =
			TEXT("(User.EffectKind == 1 && !User.ReducedMotion) ? clamp((int)(12.0 + 16.0 * saturate((User.Intensity - 0.8) / 1.0) + 0.5), 0, 32) : 0");
		if (!AddBurst(System, DamageFragmentsEmitter, Count, Context))
		{
			return false;
		}

		const TCHAR* Hash0 = TEXT("frac(sin((float)(User.Seed + Engine.ExecIndex * 17) * 12.9898) * 43758.5453)");
		const TCHAR* Hash1 = TEXT("frac(sin((float)(User.Seed + Engine.ExecIndex * 31) * 78.233) * 24634.6345)");
		const TCHAR* Hash2 = TEXT("frac(sin((float)(User.Seed + Engine.ExecIndex * 47) * 39.425) * 16543.2341)");

		TArray<FParameterExpression> Spawn = MakeCommonSpawnParameters(
			*FString::Printf(TEXT("max(User.Duration * lerp(0.72, 1.0, %s), 0.01)"), Hash0),
			TEXT("0.0"),
			*FString::Printf(TEXT("float2(1.0, 1.0) * clamp(User.TargetDiameter * lerp(0.04, 0.08, %s) * sqrt(max(User.Intensity, 0.01)), 4.0, 18.0)"), Hash2),
			TEXT("User.DecorativeIntensity"),
			Hash1,
			TEXT("1.0"));
		const FNiagaraTypeDefinition& PositionType = FNiagaraTypeDefinition::GetPositionDef();
		const FNiagaraTypeDefinition& Vec3Type = FNiagaraTypeDefinition::GetVec3Def();
		const FNiagaraTypeDefinition& FloatType = FNiagaraTypeDefinition::GetFloatDef();
		Spawn.Add(MakeParameterExpression(
			TEXT("Particles.WacomOrigin"),
			PositionType,
			FNiagaraPosition(ForceInit),
			TEXT("Engine.Owner.Position")));
		Spawn.Add(MakeParameterExpression(
			TEXT("Particles.WacomDirection"),
			Vec3Type,
			FVector3f::ZeroVector,
			*FString::Printf(
				TEXT("normalize(User.PlaneRight * cos(6.28318530718 * %s) + User.PlaneUp * sin(6.28318530718 * %s))"),
				Hash0,
				Hash0)));
		Spawn.Add(MakeParameterExpression(
			TEXT("Particles.WacomTravelDistance"),
			FloatType,
			20.0f,
			*FString::Printf(TEXT("User.TargetDiameter * lerp(0.28, 0.55, %s) * lerp(0.90, 1.15, saturate((User.Intensity - 0.8) / 1.0))"), Hash1)));

		TArray<FParameterExpression> Update = MakeCommonUpdateParameters(
			TEXT("Particles.WacomOrigin + Particles.WacomDirection * Particles.WacomTravelDistance * (1.0 - pow(1.0 - saturate(Particles.NormalizedAge), 3.0))"),
			TEXT("Particles.WacomInitialSpriteSize * lerp(1.0, 0.15, saturate(Particles.NormalizedAge))"));
		return AddParticleScaffold(System, DamageFragmentsEmitter, Spawn, Update, Context);
	}

	bool BuildDestroyedFracture(UNiagaraSystem& System, FNiagaraExternalEditContext& Context)
	{
		if (!AddBurst(System, DestroyedFractureEmitter, TEXT("User.EffectKind == 3 ? 5 : 0"), Context))
		{
			return false;
		}

		const TCHAR* Lifetime = TEXT("max(User.Duration * (Engine.ExecIndex == 0 ? 0.72 : 0.92), 0.01)");
		const TCHAR* Shape = TEXT("Engine.ExecIndex == 0 ? 3.0 : 1.0");
		const TCHAR* Size = TEXT("float2(1.0, 1.0) * User.TargetDiameter * (Engine.ExecIndex == 0 ? 0.42 : 0.64) * lerp(0.95, 1.10, saturate((User.Intensity - 1.0) / 0.6))");
		const TCHAR* Palette = TEXT("frac((float)(User.Seed + Engine.ExecIndex * 67) * 0.754877666)");
		return AddParticleScaffold(
			System,
			DestroyedFractureEmitter,
			MakeCommonSpawnParameters(Lifetime, Shape, Size, TEXT("1.0"), Palette, TEXT("0.0")),
			MakeCommonUpdateParameters(
				TEXT("Particles.Position"),
				TEXT("User.ReducedMotion ? Particles.WacomInitialSpriteSize : Particles.WacomInitialSpriteSize * (Particles.WacomShapeKind > 2.5 ? lerp(1.0, 0.62, saturate(Particles.NormalizedAge)) : lerp(0.72, 1.32, 1.0 - pow(1.0 - saturate(Particles.NormalizedAge), 3.0)))")),
			Context);
	}

	bool BuildDestroyedFragments(UNiagaraSystem& System, FNiagaraExternalEditContext& Context)
	{
		const TCHAR* Count =
			TEXT("(User.EffectKind == 3 && !User.ReducedMotion && User.DecorativeIntensity > 0.0) ? clamp((int)(24.0 + 20.0 * saturate((User.Intensity - 1.0) / 0.6) + 0.5), 0, 48) : 0");
		if (!AddBurst(System, DestroyedFragmentsEmitter, Count, Context))
		{
			return false;
		}

		const TCHAR* Hash0 = TEXT("frac(sin((float)(User.Seed + Engine.ExecIndex * 19) * 12.9898) * 43758.5453)");
		const TCHAR* Hash1 = TEXT("frac(sin((float)(User.Seed + Engine.ExecIndex * 43) * 78.233) * 24634.6345)");
		const TCHAR* Hash2 = TEXT("frac(sin((float)(User.Seed + Engine.ExecIndex * 71) * 39.425) * 16543.2341)");

		TArray<FParameterExpression> Spawn = MakeCommonSpawnParameters(
			*FString::Printf(TEXT("max(User.Duration * lerp(0.82, 1.18, %s), 0.01)"), Hash0),
			TEXT("0.0"),
			*FString::Printf(TEXT("float2(1.0, 1.0) * clamp(User.TargetDiameter * lerp(0.07, 0.13, %s) * sqrt(max(User.Intensity, 0.01)), 6.0, 28.0)"), Hash2),
			TEXT("User.DecorativeIntensity"),
			Hash1,
			TEXT("1.0"));
		const FNiagaraTypeDefinition& PositionType = FNiagaraTypeDefinition::GetPositionDef();
		const FNiagaraTypeDefinition& Vec3Type = FNiagaraTypeDefinition::GetVec3Def();
		const FNiagaraTypeDefinition& FloatType = FNiagaraTypeDefinition::GetFloatDef();
		Spawn.Add(MakeParameterExpression(
			TEXT("Particles.WacomOrigin"),
			PositionType,
			FNiagaraPosition(ForceInit),
			TEXT("Engine.Owner.Position")));
		Spawn.Add(MakeParameterExpression(
			TEXT("Particles.WacomDirection"),
			Vec3Type,
			FVector3f::ZeroVector,
			*FString::Printf(
				TEXT("normalize(User.PlaneRight * cos(6.28318530718 * %s) + User.PlaneUp * sin(6.28318530718 * %s))"),
				Hash0,
				Hash0)));
		Spawn.Add(MakeParameterExpression(
			TEXT("Particles.WacomTravelDistance"),
			FloatType,
			32.0f,
			*FString::Printf(TEXT("User.TargetDiameter * lerp(0.45, 0.90, %s) * lerp(0.95, 1.18, saturate((User.Intensity - 1.0) / 0.6))"), Hash1)));

		const TArray<FParameterExpression> Update = MakeCommonUpdateParameters(
			TEXT("Particles.WacomOrigin + Particles.WacomDirection * Particles.WacomTravelDistance * (1.0 - pow(1.0 - saturate(Particles.NormalizedAge), 3.0))"),
			TEXT("Particles.WacomInitialSpriteSize * lerp(1.0, 0.12, saturate(Particles.NormalizedAge))"));
		return AddParticleScaffold(System, DestroyedFragmentsEmitter, Spawn, Update, Context);
	}

	bool BuildTargetPreview(UNiagaraSystem& System, FNiagaraExternalEditContext& Context)
	{
		if (!AddBurst(System, TargetPreviewEmitter, TEXT("User.EffectKind == 2 ? 2 : 0"), Context))
		{
			return false;
		}

		const TCHAR* Shape = TEXT("Engine.ExecIndex == 0 ? 4.0 : 5.0");
		const TCHAR* Size =
			TEXT("Engine.ExecIndex == 0 ? float2(User.TargetWidth, User.TargetHeight) : float2(1.0, 1.0) * min(User.TargetWidth, User.TargetHeight) * 0.24");
		TArray<FParameterExpression> Spawn = MakeCommonSpawnParameters(
			TEXT("max(User.Duration, 0.01)"),
			Shape,
			Size,
			TEXT("1.0"),
			TEXT("0.0"),
			TEXT("0.0"));

		const FNiagaraTypeDefinition& PositionType = FNiagaraTypeDefinition::GetPositionDef();
		const FNiagaraTypeDefinition& Vec2Type = FNiagaraTypeDefinition::GetVec2Def();
		const FNiagaraTypeDefinition& Vec4Type = FNiagaraTypeDefinition::GetVec4Def();
		const FNiagaraTypeDefinition& ColorType = FNiagaraTypeDefinition::GetColorDef();
		TArray<FParameterExpression> Update = {
			MakeParameterExpression(
				TEXT("Particles.Position"),
				PositionType,
				FNiagaraPosition(ForceInit),
				TEXT("Particles.Position")),
			MakeParameterExpression(
				TEXT("Particles.SpriteSize"),
				Vec2Type,
				FVector2f(16.0f),
				TEXT("Particles.WacomInitialSpriteSize * (Particles.WacomShapeKind < 4.5 ? lerp(1.12, 1.0, saturate(User.PreviewAmount)) : 1.0)")),
			MakeParameterExpression(
				TEXT("Particles.Color"),
				ColorType,
				FLinearColor::White,
				TEXT("float4(1.0, 1.0, 1.0, saturate(User.PreviewAmount))")),
			MakeParameterExpression(
				TEXT("Particles.DynamicMaterialParameter"),
				Vec4Type,
				FVector4f(0.0f),
				TEXT("float4(Particles.WacomShapeKind, saturate(User.PreviewAmount), saturate(User.PreviewValidity), saturate(User.PreviewPulse * User.DecorativeIntensity))")),
		};
		return AddParticleScaffold(System, TargetPreviewEmitter, Spawn, Update, Context);
	}

	bool EnsureFloatUserParameter(UNiagaraSystem& System, const TCHAR* Name, float DefaultValue)
	{
		const FNiagaraVariable Variable(
			FNiagaraTypeDefinition::GetFloatDef(),
			Name);
		if (System.GetExposedParameters().IndexOf(Variable) == INDEX_NONE)
		{
			FNiagaraEditorUtilities::UserParameters::AddUserParameterToSystem(
				System,
				Variable);
		}
		return System.GetExposedParameters().SetParameterValue(DefaultValue, Variable, true);
	}

	bool EnsureGeneratedEmitterContract(UNiagaraSystem& System, FNiagaraExternalEditContext& Context)
	{
		UNiagaraEmitter* TemplateEmitter = nullptr;
		for (const FNiagaraEmitterHandle& Handle : System.GetEmitterHandles())
		{
			if (Handle.GetName() == ConfirmStampEmitter)
			{
				TemplateEmitter = Handle.GetInstance().Emitter;
				break;
			}
		}
		if (!TemplateEmitter)
		{
			Context.Error(NSLOCTEXT(
				"WacomNiagaraBuilder",
				"MissingGeneratedEmitterTemplate",
				"Cannot create generated emitters because ConfirmStamp is missing."));
			return false;
		}

		for (const FName Required : { TargetPreviewEmitter, DestroyedFractureEmitter, DestroyedFragmentsEmitter })
		{
			const bool bAlreadyExists = System.GetEmitterHandles().ContainsByPredicate(
				[Required](const FNiagaraEmitterHandle& Handle)
				{
					return Handle.GetName() == Required;
				});
			if (bAlreadyExists)
			{
				continue;
			}

			FNiagaraExt_EmitterTopology AddedEmitter;
			UNiagaraExternalEditUtilities::AddEmitter(
				TemplateEmitter,
				Required,
				AddedEmitter,
				Context);
			if (Context.HasErrors())
			{
				return false;
			}
		}

		return !Context.HasErrors()
			&& EnsureFloatUserParameter(System, TEXT("User.TargetDiameter"), 96.0f)
			&& EnsureFloatUserParameter(System, TEXT("User.TargetWidth"), 96.0f)
			&& EnsureFloatUserParameter(System, TEXT("User.TargetHeight"), 96.0f)
			&& EnsureFloatUserParameter(System, TEXT("User.PreviewAmount"), 0.0f)
			&& EnsureFloatUserParameter(System, TEXT("User.PreviewValidity"), 1.0f)
			&& EnsureFloatUserParameter(System, TEXT("User.PreviewPulse"), 0.0f);
	}

	bool ValidateRequiredEmitters(
		UNiagaraSystem& System,
		FNiagaraExternalEditContext& Context)
	{
		FNiagaraExt_SystemSummary Summary;
		UNiagaraExternalEditUtilities::GetSystemSummary(&System, Summary, Context);
		TSet<FName> Emitters;
		for (const FNiagaraExt_EmitterSummary& Emitter : Summary.Emitters)
		{
			Emitters.Add(Emitter.EmitterName);
		}
		for (const FName Required : RequiredEmitterNames)
		{
			if (!Emitters.Contains(Required))
			{
				Context.Error(FText::Format(
					NSLOCTEXT("WacomNiagaraBuilder", "MissingEmitter", "Niagara System is missing required emitter '{0}'."),
					FText::FromName(Required)));
			}
		}
		return !Context.HasErrors();
	}

	bool ValidateUserParameterContract(
		UNiagaraSystem& System,
		FNiagaraExternalEditContext& Context)
	{
		FNiagaraExt_SystemSummary Summary;
		UNiagaraExternalEditUtilities::GetSystemSummary(&System, Summary, Context);
		TMap<FName, FNiagaraTypeDefinition> ActualTypes;
		for (const FNiagaraExt_UserVariable& Variable : Summary.UserVariables)
		{
			ActualTypes.Add(Variable.Name, Variable.Type);
		}

		const TMap<FName, FNiagaraTypeDefinition> RequiredTypes = {
			{ TEXT("User.EffectKind"), FNiagaraTypeDefinition::GetIntDef() },
			{ TEXT("User.Duration"), FNiagaraTypeDefinition::GetFloatDef() },
			{ TEXT("User.Intensity"), FNiagaraTypeDefinition::GetFloatDef() },
			{ TEXT("User.Seed"), FNiagaraTypeDefinition::GetIntDef() },
			{ TEXT("User.DecorativeIntensity"), FNiagaraTypeDefinition::GetFloatDef() },
			{ TEXT("User.ReducedMotion"), FNiagaraTypeDefinition::GetBoolDef() },
			{ TEXT("User.ImpactMaterial"), FNiagaraTypeDefinition(UMaterialInterface::StaticClass()) },
			{ TEXT("User.PlaneRight"), FNiagaraTypeDefinition::GetVec3Def() },
			{ TEXT("User.PlaneUp"), FNiagaraTypeDefinition::GetVec3Def() },
			{ TEXT("User.TargetDiameter"), FNiagaraTypeDefinition::GetFloatDef() },
			{ TEXT("User.TargetWidth"), FNiagaraTypeDefinition::GetFloatDef() },
			{ TEXT("User.TargetHeight"), FNiagaraTypeDefinition::GetFloatDef() },
			{ TEXT("User.PreviewAmount"), FNiagaraTypeDefinition::GetFloatDef() },
			{ TEXT("User.PreviewValidity"), FNiagaraTypeDefinition::GetFloatDef() },
			{ TEXT("User.PreviewPulse"), FNiagaraTypeDefinition::GetFloatDef() },
		};

		for (const TPair<FName, FNiagaraTypeDefinition>& Required : RequiredTypes)
		{
			const FNiagaraTypeDefinition* Actual = ActualTypes.Find(Required.Key);
			if (!Actual || *Actual != Required.Value)
			{
				Context.Error(FText::Format(
					NSLOCTEXT("WacomNiagaraBuilder", "InvalidUserParameter", "User parameter '{0}' is missing or has the wrong type; expected '{1}'."),
					FText::FromName(Required.Key),
					FText::FromString(Required.Value.GetName())));
			}
		}
		return !Context.HasErrors();
	}

	bool ValidateRendererContract(
		UNiagaraSystem& System,
		FNiagaraExternalEditContext& Context)
	{
		for (const FName Emitter : RequiredEmitterNames)
		{
			FNiagaraExt_EmitterTopology Topology;
			const FNiagaraExt_StackItemReference EmitterRef(&System, Emitter);
			UNiagaraExternalEditUtilities::GetEmitterTopology(EmitterRef, Topology, Context);
			if (Topology.Renderers.Num() != 1)
			{
				Context.Error(FText::Format(
					NSLOCTEXT("WacomNiagaraBuilder", "InvalidRendererCount", "Emitter '{0}' must contain exactly one Sprite Renderer."),
					FText::FromName(Emitter)));
				continue;
			}

			FNiagaraExt_StackItemReference RendererRef = EmitterRef;
			RendererRef.RendererIndex = Topology.Renderers[0].RendererIndex;
			FNiagaraExt_RendererData RendererData;
			UNiagaraExternalEditUtilities::GetRendererData(RendererRef, RendererData, Context);
			if (!RendererData.PropertyValues.Contains(TEXT("User.ImpactMaterial"))
				|| !RendererData.PropertyValues.Contains(TEXT("Particles.DynamicMaterialParameter")))
			{
				Context.Error(FText::Format(
					NSLOCTEXT("WacomNiagaraBuilder", "InvalidRendererBindings", "Emitter '{0}' renderer must bind User.ImpactMaterial and Particles.DynamicMaterialParameter."),
					FText::FromName(Emitter)));
			}
		}
		return !Context.HasErrors();
	}

	bool ConfigureEmitterRuntimeData(
		UNiagaraSystem& System,
		FNiagaraExternalEditContext& Context)
	{
		for (const FName Emitter : RequiredEmitterNames)
		{
			const FNiagaraExt_StackItemReference EmitterRef(&System, Emitter);
			FNiagaraExt_EmitterData EmitterData;
			UNiagaraExternalEditUtilities::GetEmitterData(EmitterRef, EmitterData, Context);
			TSharedPtr<FJsonObject> Root;
			const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(EmitterData.PropertyValues);
			if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
			{
				Context.Error(FText::Format(
					NSLOCTEXT("WacomNiagaraBuilder", "InvalidEmitterJson", "Emitter '{0}' runtime data could not be parsed."),
					FText::FromName(Emitter)));
				continue;
			}

			Root->SetBoolField(TEXT("bLocalSpace"), false);
			Root->SetBoolField(TEXT("bDeterminism"), true);
			Root->SetStringField(TEXT("SimTarget"), TEXT("CPUSim"));
			Root->SetStringField(TEXT("CalculateBoundsMode"), TEXT("Fixed"));

			TSharedRef<FJsonObject> Min = MakeShared<FJsonObject>();
			Min->SetNumberField(TEXT("x"), -320.0);
			Min->SetNumberField(TEXT("y"), -320.0);
			Min->SetNumberField(TEXT("z"), -320.0);
			TSharedRef<FJsonObject> Max = MakeShared<FJsonObject>();
			Max->SetNumberField(TEXT("x"), 320.0);
			Max->SetNumberField(TEXT("y"), 320.0);
			Max->SetNumberField(TEXT("z"), 320.0);
			TSharedRef<FJsonObject> Bounds = MakeShared<FJsonObject>();
			Bounds->SetObjectField(TEXT("min"), Min);
			Bounds->SetObjectField(TEXT("max"), Max);
			Bounds->SetBoolField(TEXT("isValid"), true);
			Root->SetObjectField(TEXT("FixedBounds"), Bounds);

			FString UpdatedJson;
			const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&UpdatedJson);
			if (!FJsonSerializer::Serialize(Root.ToSharedRef(), Writer))
			{
				Context.Error(FText::Format(
					NSLOCTEXT("WacomNiagaraBuilder", "EmitterJsonSerializeFailed", "Emitter '{0}' runtime data could not be serialized."),
					FText::FromName(Emitter)));
				continue;
			}
			EmitterData.PropertyValues = MoveTemp(UpdatedJson);
			UNiagaraExternalEditUtilities::SetEmitterData(EmitterRef, EmitterData, Context);
		}
		return !Context.HasErrors();
	}

	bool ClearRequiredEmitterStacks(
		UNiagaraSystem& System,
		FNiagaraExternalEditContext& Context)
	{
		for (const FName Emitter : RequiredEmitterNames)
		{
			for (const FName Script : { EmitterUpdateScript, ParticleSpawnScript, ParticleUpdateScript })
			{
				if (!RemoveAllModules(System, Emitter, Script, Context))
				{
					return false;
				}
			}
		}
		return true;
	}

	bool ValidateCompile(UNiagaraSystem& System, FNiagaraExternalEditContext& Context)
	{
		System.RequestCompile(true);
		System.WaitForCompilationComplete(false, false);

		FNiagaraExt_SystemCompileState CompileState;
		UNiagaraExternalEditUtilities::GetSystemCompileState(&System, CompileState, Context);
		for (const FNiagaraExt_ScriptCompileInfo& Script : CompileState.Scripts)
		{
			for (const FNiagaraExt_CompileEvent& Event : Script.CompileEvents)
			{
				if (Event.Severity == ENiagaraExt_CompileEventSeverity::Error
					|| Event.Severity == ENiagaraExt_CompileEventSeverity::Warning)
				{
					if (Event.Severity == ENiagaraExt_CompileEventSeverity::Error)
					{
						UE_LOG(LogTemp, Error, TEXT("[EnemyImpactNiagara] Compile %s/%s: %s"),
							*Script.EmitterName.ToString(),
							*Script.ScriptName.ToString(),
							*Event.Message);
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("[EnemyImpactNiagara] Compile %s/%s: %s"),
							*Script.EmitterName.ToString(),
							*Script.ScriptName.ToString(),
							*Event.Message);
					}
				}
			}
		}
		return !Context.HasErrors() && !CompileState.bHasErrors;
	}

	bool SaveSystem(UNiagaraSystem& System)
	{
		UPackage* Package = System.GetOutermost();
		if (!Package)
		{
			return false;
		}
		Package->MarkPackageDirty();
		System.MarkPackageDirty();

		const FString Filename = FPackageName::LongPackageNameToFilename(
			ImpactSystemPackagePath,
			FPackageName::GetAssetPackageExtension());
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Package, &System, *Filename, Args);
	}

	bool HasCurrentImpactContractVersion(const UNiagaraSystem& System)
	{
		UPackage* Package = System.GetPackage();
		return Package
			&& Package->GetMetaData().GetValue(&System, ImpactContractVersionMetadataKey)
				== ImpactContractVersion;
	}

	void StampCurrentImpactContractVersion(UNiagaraSystem& System)
	{
		if (UPackage* Package = System.GetPackage())
		{
			Package->GetMetaData().SetValue(
				&System,
				ImpactContractVersionMetadataKey,
				ImpactContractVersion);
		}
	}

	void LogSystemTopology(UNiagaraSystem& System, FNiagaraExternalEditContext& Context)
	{
		FNiagaraExt_SystemSummary Summary;
		UNiagaraExternalEditUtilities::GetSystemSummary(&System, Summary, Context);
		UE_LOG(LogTemp, Display, TEXT("[EnemyImpactNiagara] System=%s UserVariables=%d Emitters=%d"),
			*Summary.SystemName.ToString(), Summary.UserVariables.Num(), Summary.Emitters.Num());
		for (const FNiagaraExt_UserVariable& Variable : Summary.UserVariables)
		{
			UE_LOG(LogTemp, Display, TEXT("[EnemyImpactNiagara] UserVariable=%s Type=%s"),
				*Variable.Name.ToString(), *Variable.Type.GetName());
		}
		for (const FNiagaraExt_EmitterSummary& EmitterSummary : Summary.Emitters)
		{
			FNiagaraExt_EmitterTopology Topology;
			UNiagaraExternalEditUtilities::GetEmitterTopology(
				FNiagaraExt_StackItemReference(&System, EmitterSummary.EmitterName),
				Topology,
				Context);
			LogScriptTopology(Topology.EmitterName, Topology.EmitterUpdateScript);
			LogScriptTopology(Topology.EmitterName, Topology.ParticleSpawnScript);
			LogScriptTopology(Topology.EmitterName, Topology.ParticleUpdateScript);
		}
	}
}

namespace Wacom::ContentBuilder
{
	bool BuildBattleEnemyPartImpactNiagara(const bool bInspectOnly)
	{
		UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr, ImpactSystemObjectPath);
		if (!System)
		{
			UE_LOG(LogTemp, Error, TEXT("[EnemyImpactNiagara] Missing system: %s"), ImpactSystemObjectPath);
			return false;
		}

		if (bInspectOnly)
		{
			FNiagaraExternalEditContext Context(System);
			LogSystemTopology(*System, Context);
			if (!HasCurrentImpactContractVersion(*System))
			{
				Context.Error(NSLOCTEXT(
					"WacomNiagaraBuilder",
					"StaleImpactContractVersion",
					"Niagara System does not carry the current generated impact contract version."));
			}
			return ValidateRequiredEmitters(*System, Context)
				&& ValidateUserParameterContract(*System, Context)
				&& ValidateRendererContract(*System, Context)
				&& ValidateCompile(*System, Context)
				&& LogContextErrors(Context);
		}

		if (HasCurrentImpactContractVersion(*System))
		{
			FNiagaraExternalEditContext CurrentContractContext(System);
			if (ValidateRequiredEmitters(*System, CurrentContractContext)
				&& ValidateUserParameterContract(*System, CurrentContractContext)
				&& ValidateRendererContract(*System, CurrentContractContext)
				&& ValidateCompile(*System, CurrentContractContext)
				&& LogContextErrors(CurrentContractContext))
			{
				UE_LOG(LogTemp, Display,
					TEXT("[EnemyImpactNiagara] Current six-emitter contract is already valid; no asset write required."));
				return true;
			}
			UE_LOG(LogTemp, Warning,
				TEXT("[EnemyImpactNiagara] Current contract marker is stale relative to asset contents; rebuilding."));
		}

		FNiagaraExternalEditContext Context(System);
		if (!EnsureGeneratedEmitterContract(*System, Context)
			|| !ValidateRequiredEmitters(*System, Context)
			|| !ValidateUserParameterContract(*System, Context)
			|| !ValidateRendererContract(*System, Context)
			|| !ClearRequiredEmitterStacks(*System, Context)
			|| !BuildConfirmStamp(*System, Context)
			|| !BuildDamageCoreWave(*System, Context)
			|| !BuildDamageFragments(*System, Context)
			|| !BuildDestroyedFracture(*System, Context)
			|| !BuildDestroyedFragments(*System, Context)
			|| !BuildTargetPreview(*System, Context)
			|| !ConfigureEmitterRuntimeData(*System, Context))
		{
			LogContextErrors(Context);
			return false;
		}

		LogSystemTopology(*System, Context);
		if (!ValidateCompile(*System, Context))
		{
			LogContextErrors(Context);
			return false;
		}

		StampCurrentImpactContractVersion(*System);
		if (!SaveSystem(*System))
		{
			UE_LOG(LogTemp, Error, TEXT("[EnemyImpactNiagara] Failed to save %s"), ImpactSystemPackagePath);
			return false;
		}

		UE_LOG(LogTemp, Display,
			TEXT("[EnemyImpactNiagara] Built and saved six emitters including DestroyedFracture and DestroyedFragments."));
		return LogContextErrors(Context);
	}
}
