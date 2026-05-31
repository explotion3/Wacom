// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Commandlet 批量生成内容时用的轻量 helper。
 *
 * 仅 WacomEditor/Private 使用。统一 "create-or-replace package + NewObject + SavePackage"
 * 的流程，避免每个 builder 重复样板代码。
 */
namespace Wacom::ContentBuilder
{
	FString DataRoot();
	FString CardsRoot();
	FString BugGirlCardsRoot();
	FString RewardCardsRoot();
	FString CharactersRoot();
	FString SnakeEnemiesRoot();
	FString EventsRoot();
	FString ShopsRoot();
	FString PickupsRoot();
	FString MakePackagePath(const FString& FolderPath, const TCHAR* AssetName);
	FString MakeObjectPath(const FString& PackagePath);

	/** 创建或查找一个空 package，并 FullyLoad。 */
	UPackage* FindOrCreatePackage(const FString& PackagePath);

	/**
	 * 在指定 Package 中新建 T 类型 UObject。
	 * 若已存在同名对象，先 Rename 到 Transient 再 MarkAsGarbage，保证字段是全新的。
	 */
	template <typename T>
	T* CreateOrReplaceAsset(UPackage* Package, FName AssetName);

	/**
	 * 保存 package 到磁盘，并通知 AssetRegistry。
	 * PackagePath 形如 "/Game/Wacom/Data/Cards/BugGirl/DA_Card_LeftHand"。
	 */
	bool SaveAssetPackage(UPackage* Package, UObject* Asset, const FString& PackagePath);
}

// ---- 模板实现 ----

#include "Engine/Engine.h"
#include "UObject/Package.h"

namespace Wacom::ContentBuilder
{
	template <typename T>
	T* CreateOrReplaceAsset(UPackage* Package, FName AssetName)
	{
		if (UObject* Existing = StaticFindObject(nullptr, Package, *AssetName.ToString()))
		{
			// CDO 的 FObjectFinder 在资产不存在时可能留下 rooted 占位对象；
			// 跳过这类对象的 MarkAsGarbage，直接 Rename 到 Transient。
			Existing->Rename(nullptr, GetTransientPackage(),
				REN_DontCreateRedirectors | REN_DoNotDirty | REN_NonTransactional);
			if (!Existing->IsRooted())
			{
				Existing->MarkAsGarbage();
			}
		}
		T* NewAsset = NewObject<T>(Package, AssetName, RF_Public | RF_Standalone);
		return NewAsset;
	}
}
