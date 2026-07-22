// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace Wacom::ContentBuilder
{
struct FBackpackWorkspaceV4MigrationReport
{
	bool bAlreadyCurrent = false;
	TArray<FString> SavedPackages;
};

/**
 * 对 Backpack Workspace v4 的九个精确 package 执行一次性迁移或只读审计。
 * 不调用通用 Backpack Builder，也不会保存 manifest 外的 package。
 */
bool MigrateBackpackWorkspaceV4(
	bool bApply,
	FBackpackWorkspaceV4MigrationReport& OutReport);
}
