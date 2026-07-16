// Copyright Wacom. All Rights Reserved.

#pragma once

namespace Wacom::ContentBuilder
{
	/**
	 * 检查或迁移 Scene Enemy Panel 的两个固定 Widget Blueprint。
	 *
	 * MigrateLegacy 只会填充已存在且为空的兼容空壳；未知人工布局不会被修改。
	 * InspectOnly 永远只读。
	 */
	bool ProcessEnemyUIWidgetBlueprints(bool bMigrateLegacy, bool bInspectOnly);
}
