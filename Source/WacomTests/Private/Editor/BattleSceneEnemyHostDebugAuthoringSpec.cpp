// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartActor.h"
#include "Authoring/WacomBattleSceneEnemyHostAuthoring.h"
#include "Components/ChildActorComponent.h"
#include "Engine/Blueprint.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/ScopeExit.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomBattleSceneEnemyHostDebugAuthoringSpec
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomEditorBattleSceneEnemyHostConfigureDebugSnakeSampleSpec,
	"Wacom.Editor.BattleSceneEnemyHostDebugAuthoring.ConfiguresLiveParts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomEditorBattleSceneEnemyHostConfigureDebugSnakeSampleSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleSceneEnemyHostDebugAuthoringSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyActor* Host = World->SpawnActor<AWacomBattleEnemyActor>(
		AWacomBattleEnemyActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Host"), Host))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Host))
		{
			Host->Destroy();
		}
	};

	UChildActorComponent* HeadComponent =
		NewObject<UChildActorComponent>(Host, TEXT("SnakeHeadPart"));
	UChildActorComponent* BodyComponent =
		NewObject<UChildActorComponent>(Host, TEXT("SnakeBodyPart"));
	UChildActorComponent* TailComponent =
		NewObject<UChildActorComponent>(Host, TEXT("SnakeTailPart"));
	for (UChildActorComponent* Component :
		{ HeadComponent, BodyComponent, TailComponent })
	{
		if (!TestNotNull(TEXT("Child component"), Component))
		{
			return false;
		}
		Component->CreationMethod = EComponentCreationMethod::Instance;
		Component->SetupAttachment(Host->GetRootComponent());
		Host->AddInstanceComponent(Component);
		Component->RegisterComponent();
		Component->SetChildActorClass(AWacomBattleEnemyPartActor::StaticClass());
	}

	AWacomBattleEnemyPartActor* Head =
		Cast<AWacomBattleEnemyPartActor>(HeadComponent->GetChildActor());
	AWacomBattleEnemyPartActor* Body =
		Cast<AWacomBattleEnemyPartActor>(BodyComponent->GetChildActor());
	AWacomBattleEnemyPartActor* Tail =
		Cast<AWacomBattleEnemyPartActor>(TailComponent->GetChildActor());
	if (!TestNotNull(TEXT("Head"), Head)
		|| !TestNotNull(TEXT("Body"), Body)
		|| !TestNotNull(TEXT("Tail"), Tail))
	{
		return false;
	}

	TestEqual(TEXT("Debug operation applied"),
		FWacomBattleSceneEnemyHostAuthoring::ConfigureDebugSnakeSample(*Host),
		FName(TEXT("Applied")));
	const FWacomBattleSceneEnemyDebugView View = Host->GetBattleSceneEnemyDebugView();
	TestEqual(TEXT("Snake sample part count"), View.AttachedPartActorCount, 3);
	TestTrue(TEXT("Snake sample stable targets"),
		View.StableSceneTargetIds.Contains(TEXT("Enemy.Head"))
		&& View.StableSceneTargetIds.Contains(TEXT("Enemy.Body"))
		&& View.StableSceneTargetIds.Contains(TEXT("Enemy.Tail")));
	TestEqual(TEXT("Head identity"), Head->PartId, FName(TEXT("Snake.Head")));
	TestEqual(TEXT("Body identity"), Body->PartId, FName(TEXT("Snake.Body")));
	TestEqual(TEXT("Tail identity"), Tail->PartId, FName(TEXT("Snake.Tail")));
	TestEqual(TEXT("Head position"),
		HeadComponent->GetRelativeLocation(), FVector(96.0f, -6.0f, 16.0f));
	TestEqual(TEXT("Body position"),
		BodyComponent->GetRelativeLocation(), FVector::ZeroVector);
	TestEqual(TEXT("Tail position"),
		TailComponent->GetRelativeLocation(), FVector(-92.0f, 16.0f, -8.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomEditorBattleSceneEnemyHostConfigureDebugSnakeTemplateSpec,
	"Wacom.Editor.BattleSceneEnemyHostDebugAuthoring.ConfiguresChildActorTemplates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomEditorBattleSceneEnemyHostConfigureDebugSnakeTemplateSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomBattleEnemyActor> Host(NewObject<AWacomBattleEnemyActor>(
		GetTransientPackage(),
		TEXT("BP_SnakeHost_Debug_CDO"),
		RF_ArchetypeObject | RF_Transactional));
	UChildActorComponent* HeadComponent = NewObject<UChildActorComponent>(
		Host.Get(), TEXT("SnakeHeadPart"), RF_ArchetypeObject | RF_Transactional);
	UChildActorComponent* BodyComponent = NewObject<UChildActorComponent>(
		Host.Get(), TEXT("SnakeBodyPart"), RF_ArchetypeObject | RF_Transactional);
	UChildActorComponent* TailComponent = NewObject<UChildActorComponent>(
		Host.Get(), TEXT("SnakeTailPart"), RF_ArchetypeObject | RF_Transactional);
	for (UChildActorComponent* Component :
		{ HeadComponent, BodyComponent, TailComponent })
	{
		if (!TestNotNull(TEXT("Template component"), Component))
		{
			return false;
		}
		Component->SetupAttachment(Host->GetRootComponent());
		Host->AddInstanceComponent(Component);
		Component->SetChildActorClass(AWacomBattleEnemyPartActor::StaticClass());
	}

	AWacomBattleEnemyPartActor* Head = Cast<AWacomBattleEnemyPartActor>(
		HeadComponent->GetChildActorTemplate());
	AWacomBattleEnemyPartActor* Body = Cast<AWacomBattleEnemyPartActor>(
		BodyComponent->GetChildActorTemplate());
	AWacomBattleEnemyPartActor* Tail = Cast<AWacomBattleEnemyPartActor>(
		TailComponent->GetChildActorTemplate());
	if (!TestNotNull(TEXT("Head template"), Head)
		|| !TestNotNull(TEXT("Body template"), Body)
		|| !TestNotNull(TEXT("Tail template"), Tail))
	{
		return false;
	}

	TestEqual(TEXT("Template debug operation applied"),
		FWacomBattleSceneEnemyHostAuthoring::ConfigureDebugSnakeSample(*Host),
		FName(TEXT("Applied")));
	TestEqual(TEXT("Head template identity"), Head->PartId, FName(TEXT("Snake.Head")));
	TestEqual(TEXT("Body template identity"), Body->PartId, FName(TEXT("Snake.Body")));
	TestEqual(TEXT("Tail template identity"), Tail->PartId, FName(TEXT("Snake.Tail")));
	TestEqual(TEXT("Head template position"),
		HeadComponent->GetRelativeLocation(), FVector(96.0f, -6.0f, 16.0f));
	TestEqual(TEXT("Body template position"),
		BodyComponent->GetRelativeLocation(), FVector::ZeroVector);
	TestEqual(TEXT("Tail template position"),
		TailComponent->GetRelativeLocation(), FVector(-92.0f, 16.0f, -8.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomEditorBattleSceneEnemyHostDebugBlueprintCompatibilitySpec,
	"Wacom.Editor.BattleSceneEnemyHostDebugAuthoring.ExistingBlueprintCompilesWithoutLegacyActorEntryPoints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomEditorBattleSceneEnemyHostDebugBlueprintCompatibilitySpec::RunTest(
	const FString& /*Parameters*/)
{
	UBlueprint* Blueprint = LoadObject<UBlueprint>(
		nullptr,
		TEXT("/Game/Wacom/Core/Enemy/BP_SnakeHost_Debug.BP_SnakeHost_Debug"));
	if (!TestNotNull(TEXT("Existing BP_SnakeHost_Debug loads"), Blueprint))
	{
		return false;
	}

	UPackage* Package = Blueprint->GetPackage();
	const bool bPackageDirtyBefore = Package && Package->IsDirty();
	ON_SCOPE_EXIT
	{
		if (Package)
		{
			Package->SetDirtyFlag(bPackageDirtyBefore);
		}
	};

	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	TestTrue(TEXT("Existing BP_SnakeHost_Debug compiles without error"),
		Blueprint->Status != BS_Error);
	TestTrue(TEXT("Generated class remains an Enemy Host"),
		Blueprint->GeneratedClass
		&& Blueprint->GeneratedClass->IsChildOf(
			AWacomBattleEnemyActor::StaticClass()));
	if (Blueprint->GeneratedClass)
	{
		TestNull(TEXT("Removed sync entry point is not required by the Blueprint"),
			Blueprint->GeneratedClass->FindFunctionByName(
				TEXT("SyncEnemyPartsFromDefinition")));
		TestNull(TEXT("Removed snake sample entry point is not required by the Blueprint"),
			Blueprint->GeneratedClass->FindFunctionByName(
				TEXT("ConfigureDebugSnakeHostSample")));
	}
	return true;
}
