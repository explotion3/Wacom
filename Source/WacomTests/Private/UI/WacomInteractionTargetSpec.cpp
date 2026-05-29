// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Components/WacomInteractionTargetComponent.h"
#include "Types/WacomInteractionTargetTypes.h"

#include "GameFramework/Actor.h"
#include "Engine/World.h"

namespace
{
	UWorld* FindAutomationWorld()
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::Game
				|| Context.WorldType == EWorldType::PIE
				|| Context.WorldType == EWorldType::Editor)
			{
				return Context.World();
			}
		}
		return nullptr;
	}
}

// ============================================================================
// FWacomInteractionTargetHandle 单元测试
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomInteractionTargetHandleBasicSpec,
	"Wacom.Core.InteractionTarget.Handle.BasicValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomInteractionTargetHandleBasicSpec::RunTest(const FString& /*Parameters*/)
{
	{
		FWacomInteractionTargetHandle Handle;
		TestFalse(TEXT("Default-constructed handle is invalid"), Handle.IsValid());
		TestEqual(TEXT("Default-constructed handle kind is None"),
			Handle.TargetKind, EWacomInteractionTargetKind::None);
	}

	{
		const FGuid TestId = FGuid::NewGuid();
		FWacomInteractionTargetHandle Handle = FWacomInteractionTargetHandle::ForWorldTarget(
			TestId, nullptr);
		TestTrue(TEXT("ForWorldTarget handle is valid"), Handle.IsValid());
		TestEqual(TEXT("ForWorldTarget kind is World"),
			Handle.TargetKind, EWacomInteractionTargetKind::World);
		TestEqual(TEXT("ForWorldTarget preserves WorldTargetId"),
			Handle.WorldTargetId, TestId);
		TestFalse(TEXT("ForWorldTarget CardInstanceId is empty"),
			Handle.CardInstanceId.IsValid());
		TestEqual(TEXT("ForWorldTarget ZoneId is none"),
			Handle.ZoneId, NAME_None);
	}

	{
		const FGuid TestId = FGuid::NewGuid();
		FWacomInteractionTargetHandle Handle = FWacomInteractionTargetHandle::ForWorldTarget(
			FGuid(), nullptr);
		// WorldTargetId 为空时 handle 仍标记为 World，但 WorldTargetId 无效。
		// 这是允许的—— Provider 可能选择在 BuildWorldTargetHandle 里提前返回 invalid handle。
		TestTrue(TEXT("ForWorldTarget with empty GUID is still valid (kind=World)"),
			Handle.IsValid());
		TestFalse(TEXT("ForWorldTarget with empty GUID has empty WorldTargetId"),
			Handle.WorldTargetId.IsValid());
	}

	{
		FWacomInteractionTargetHandle Handle = FWacomInteractionTargetHandle::ForWorldTarget(
			FGuid::NewGuid(), nullptr, FVector(100.0f, 200.0f, 300.0f), FVector2D(400.0f, 500.0f));
		TestEqual(TEXT("ForWorldTarget preserves WorldLocation"),
			Handle.WorldLocation, FVector(100.0f, 200.0f, 300.0f));
		TestEqual(TEXT("ForWorldTarget preserves ScreenPosition"),
			Handle.ScreenPosition, FVector2D(400.0f, 500.0f));
	}

	return true;
}

// ============================================================================
// UWacomInteractionTargetComponent 测试
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIInteractionTargetComponentSpec,
	"Wacom.UI.InteractionTarget.Component.BuildHandle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIInteractionTargetComponentSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Owner actor spawned"), Owner))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Owner))
		{
			Owner->Destroy();
		}
	};

	UWacomInteractionTargetComponent* Component = NewObject<UWacomInteractionTargetComponent>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	if (!TestNotNull(TEXT("InteractionTargetComponent created"), Component))
	{
		return false;
	}

	{
		FWacomInteractionTargetHandle Handle = Component->BuildWorldTargetHandle();
		TestFalse(TEXT("Empty TargetId returns invalid handle"), Handle.IsValid());
	}

	{
		const FGuid TestId = FGuid::NewGuid();
		Component->SetTargetId(TestId);

		FWacomInteractionTargetHandle Handle = Component->BuildWorldTargetHandle();
		TestTrue(TEXT("Valid TargetId returns valid handle"), Handle.IsValid());
		TestEqual(TEXT("Handle kind is World"),
			Handle.TargetKind, EWacomInteractionTargetKind::World);
		TestEqual(TEXT("Handle preserves TargetId"),
			Handle.WorldTargetId, TestId);
		TestNotNull(TEXT("Handle references SourceObject"),
			Handle.SourceObject.Get());
	}

	{
		FGameplayTag TestTag = FGameplayTag::RequestGameplayTag(TEXT("Card.Keyword.Swift"), false);
		Component->SetInteractionTargetTag(TestTag);
		TestEqual(TEXT("GetInteractionTargetTag returns set tag"),
			Component->GetInteractionTargetTag(), TestTag);
	}

	return true;
}
