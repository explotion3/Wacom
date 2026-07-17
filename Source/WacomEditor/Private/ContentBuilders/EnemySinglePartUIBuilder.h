// Copyright Wacom. All Rights Reserved.

#pragma once

namespace Wacom::ContentBuilder
{
	/**
	 * 构建或只读检查单部位紧凑 Enemy Panel、Intent Style 和正式像素图标。
	 * 该入口只管理新路径，不读取或修改旧 BP_WacomBattleEnemy* 资产。
	 */
	bool ProcessEnemySinglePartUI(bool bBuild, bool bInspectOnly);
}
