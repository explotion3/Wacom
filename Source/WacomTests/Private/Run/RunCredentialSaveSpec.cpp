// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Characters/CharacterDefinition.h"
#include "RunSession.h"
#include "RunState.h"
#include "WacomSaveGame.h"

#include "UObject/SoftObjectPath.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	const FName CredentialSaveSerpentCredential(TEXT("Credential.Run.SerpentSigil"));
	const TCHAR* CharacterPath =
		TEXT("/Game/Wacom/Data/Characters/DA_Character_BugGirl.DA_Character_BugGirl");

	UCharacterDefinition* LoadCredentialSaveCharacter()
	{
		return LoadObject<UCharacterDefinition>(nullptr, CharacterPath);
	}

	UWacomSaveGame* MakeCredentialSave(const TArray<FName>& CredentialIds)
	{
		UWacomSaveGame* Save = NewObject<UWacomSaveGame>();
		Save->SaveVersion = UWacomSaveGame::CurrentSaveVersion;
		Save->CharacterAssetPath = FSoftObjectPath(CharacterPath);
		Save->GrantedCredentialIds = CredentialIds;
		return Save;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCredentialSaveRoundtripSpec,
	"Wacom.Run.Save.Credential.RoundtripAndDeterministicOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunCredentialSaveRoundtripSpec::RunTest(const FString& /*Parameters*/)
{
	UCharacterDefinition* Character = LoadCredentialSaveCharacter();
	if (!TestNotNull(TEXT("Real character loads"), Character))
	{
		return false;
	}

	TStrongObjectPtr<URunSession> Source(NewObject<URunSession>());
	FRunState& SourceState = FWacomRunSessionTestAccess::GetMutableRunState(*Source);
	SourceState.Character = Character;
	SourceState.GrantedCredentialIds = {
		TEXT("Credential.Run.Zeta"),
		CredentialSaveSerpentCredential,
		TEXT("Credential.Run.Alpha")
	};

	TStrongObjectPtr<UWacomSaveGame> Save(Source->BuildSaveGameFromRunState());
	if (!TestNotNull(TEXT("Save builds"), Save.Get()))
	{
		return false;
	}

	TestEqual(TEXT("Save schema is v5"), Save->SaveVersion, 5);
	TestEqual(TEXT("All credentials serialized"), Save->GrantedCredentialIds.Num(), 3);
	if (Save->GrantedCredentialIds.Num() == 3)
	{
		TestEqual(TEXT("Credential order 0"), Save->GrantedCredentialIds[0],
			FName(TEXT("Credential.Run.Alpha")));
		TestEqual(TEXT("Credential order 1"), Save->GrantedCredentialIds[1],
			CredentialSaveSerpentCredential);
		TestEqual(TEXT("Credential order 2"), Save->GrantedCredentialIds[2],
			FName(TEXT("Credential.Run.Zeta")));
	}

	TStrongObjectPtr<URunSession> Loaded(NewObject<URunSession>());
	TestTrue(TEXT("Credential save applies"), Loaded->ApplySaveGameToRunState(Save.Get()));
	TestEqual(TEXT("Credential count roundtrips"),
		Loaded->GetRunState().GrantedCredentialIds.Num(), 3);
	TestTrue(TEXT("Serpent credential roundtrips"), Loaded->HasCredential(CredentialSaveSerpentCredential));
	TestTrue(TEXT("Alpha credential roundtrips"),
		Loaded->HasCredential(TEXT("Credential.Run.Alpha")));
	TestTrue(TEXT("Zeta credential roundtrips"),
		Loaded->HasCredential(TEXT("Credential.Run.Zeta")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCredentialSaveV3MigrationSpec,
	"Wacom.Run.Save.Credential.V3MigratesToEmptyWithoutCardInference",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunCredentialSaveV3MigrationSpec::RunTest(const FString& /*Parameters*/)
{
	if (!TestNotNull(TEXT("Real character loads"), LoadCredentialSaveCharacter()))
	{
		return false;
	}

	TStrongObjectPtr<UWacomSaveGame> Save(MakeCredentialSave({ CredentialSaveSerpentCredential }));
	Save->SaveVersion = 3;

	TStrongObjectPtr<URunSession> Loaded(NewObject<URunSession>());
	TestTrue(TEXT("v3 save migrates and applies"), Loaded->ApplySaveGameToRunState(Save.Get()));
	TestEqual(TEXT("Save migrated to v5"), Save->SaveVersion, 5);
	TestEqual(TEXT("v3 migration explicitly clears credentials"),
		Loaded->GetRunState().GrantedCredentialIds.Num(), 0);
	TestFalse(TEXT("No physical-card inference grants serpent credential"),
		Loaded->HasCredential(CredentialSaveSerpentCredential));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCredentialInvalidSaveAtomicSpec,
	"Wacom.Run.Save.Credential.InvalidIdsRejectAtomically",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunCredentialInvalidSaveAtomicSpec::RunTest(const FString& /*Parameters*/)
{
	if (!TestNotNull(TEXT("Real character loads"), LoadCredentialSaveCharacter()))
	{
		return false;
	}

	auto VerifyRejected = [this](const TArray<FName>& InvalidCredentialIds, const TCHAR* Label)
	{
		TStrongObjectPtr<UWacomSaveGame> Save(MakeCredentialSave(InvalidCredentialIds));
		Save->BattleSeed = 999;

		TStrongObjectPtr<URunSession> Target(NewObject<URunSession>());
		FRunState& Before = FWacomRunSessionTestAccess::GetMutableRunState(*Target);
		Before.BattleSeed = 123;
		Before.GrantedCredentialIds.Add(TEXT("Credential.Run.Existing"));
		int32 BroadcastCount = 0;
		Target->OnRunStateChangedNative.AddLambda([&BroadcastCount]() { ++BroadcastCount; });

		TestFalse(Label, Target->ApplySaveGameToRunState(Save.Get()));
		TestEqual(TEXT("Rejected save preserves BattleSeed"), Target->GetRunState().BattleSeed, 123);
		TestEqual(TEXT("Rejected save preserves credential set"),
			Target->GetRunState().GrantedCredentialIds.Num(), 1);
		TestTrue(TEXT("Rejected save preserves existing credential"),
			Target->HasCredential(TEXT("Credential.Run.Existing")));
		TestEqual(TEXT("Rejected save does not broadcast"), BroadcastCount, 0);
	};

	AddExpectedError(
		TEXT("GrantedCredentialIds 包含 None"),
		EAutomationExpectedErrorFlags::Contains,
		1);
	VerifyRejected({ NAME_None }, TEXT("None credential rejects save"));
	AddExpectedError(
		TEXT("GrantedCredentialIds 重复 ID"),
		EAutomationExpectedErrorFlags::Contains,
		1);
	VerifyRejected({ CredentialSaveSerpentCredential, CredentialSaveSerpentCredential },
		TEXT("Duplicate credential rejects save"));
	return true;
}
