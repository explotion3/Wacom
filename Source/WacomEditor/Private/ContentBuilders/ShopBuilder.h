// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UShopDefinition;

namespace Wacom::ContentBuilder
{
	/**
	 * 生成第一版调试商店内容。
	 *
	 * 在 /Game/Wacom/Shops/ 下生成 DA_Shop_DebugSnake。
	 */
	UShopDefinition* BuildShopContent();
}
