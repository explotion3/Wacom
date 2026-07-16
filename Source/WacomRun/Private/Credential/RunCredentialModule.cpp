// Copyright Wacom. All Rights Reserved.

#include "Credential/RunCredentialModule.h"

#include "RunState.h"

bool FRunCredentialModule::AreCredentialIdsValid(const TConstArrayView<FName> CredentialIds)
{
	TSet<FName> UniqueIds;
	for (const FName CredentialId : CredentialIds)
	{
		if (CredentialId.IsNone() || UniqueIds.Contains(CredentialId))
		{
			return false;
		}
		UniqueIds.Add(CredentialId);
	}
	return true;
}

bool FRunCredentialModule::GrantAll(
	FRunState& State,
	const TConstArrayView<FName> CredentialIds)
{
	if (!AreCredentialIdsValid(CredentialIds))
	{
		return false;
	}

	for (const FName CredentialId : CredentialIds)
	{
		State.GrantedCredentialIds.Add(CredentialId);
	}
	return true;
}

bool FRunCredentialModule::HasAll(
	const FRunState& State,
	const TConstArrayView<FName> CredentialIds)
{
	if (!AreCredentialIdsValid(CredentialIds))
	{
		return false;
	}

	for (const FName CredentialId : CredentialIds)
	{
		if (!State.GrantedCredentialIds.Contains(CredentialId))
		{
			return false;
		}
	}
	return true;
}
