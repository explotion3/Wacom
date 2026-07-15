// Copyright Wacom. All Rights Reserved.

#pragma once

namespace Wacom::ContentBuilder
{
	/**
	 * 读取或幂等重建敌人部位像素命中 Niagara System。
	 *
	 * NiagaraEditor 依赖严格留在 WacomEditor；运行时 WacomApp 只消费生成后的
	 * UNiagaraSystem 资产和固定 User Parameter contract。实现依赖 UE 5.8 的
	 * experimental external-edit API，升级引擎后必须重跑生成与编译验证。
	 */
	bool BuildBattleEnemyPartImpactNiagara(bool bInspectOnly);
}
