// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Wacom 项目的 ID 类型别名。
 *
 * 第一阶段所有静态资产 ID 直接使用 FName 语义。不再做强类型 wrapper：
 * - FName 已反射友好，可直接作为 USTRUCT 字段。
 * - 强类型 wrapper 在 DataAsset / DataTable 里编辑体验较差。
 *
 * 若后续需要编译期隔离（例如防止 FCardId 被赋给 FPartId），再引入
 * phantom-typed wrapper。
 *
 * 运行时实例使用 FGuid，由 BattleSession 在初始化时生成。
 */
namespace Wacom
{
	using FCardDefId = FName;     // UCardDefinition::CardId
	using FEnemyDefId = FName;    // UEnemyDefinition::EnemyId
	using FPartDefId = FName;     // UEnemyPartDefinition::PartId
	using FIntentDefId = FName;   // FIntentDefinition::IntentId
	using FCharacterDefId = FName;// UCharacterDefinition::CharacterId
}
