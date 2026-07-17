// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FRunState;

/** Session-private credential validation, idempotent grant and requirement rules. */
class FRunCredentialModule
{
public:
	/** 输入必须全部非空且唯一。空列表合法。 */
	static bool AreCredentialIdsValid(TConstArrayView<FName> CredentialIds);

	/** 校验后幂等授予全部凭证；非法输入不修改 State。 */
	static bool GrantAll(FRunState& State, TConstArrayView<FName> CredentialIds);

	/** 空列表返回 true；非法或缺失任一凭证返回 false。 */
	static bool HasAll(const FRunState& State, TConstArrayView<FName> CredentialIds);
};
