// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWacomSaveGame;
struct FRunState;

/**
 * FRunState <-> UWacomSaveGame 的私有字段拷贝 helper。
 *
 * 只做内存态与磁盘 schema 对应字段的转换、校验和原子还原准备；
 * 不广播、不访问 UI、不做 slot 磁盘 IO。
 */
struct FRunSaveGameSerializer
{
	static UWacomSaveGame* BuildSaveGameFromRunState(const FRunState& State);
	static bool TryApplySaveGameToRunState(UWacomSaveGame* SaveGame, FRunState& InOutState);
};
