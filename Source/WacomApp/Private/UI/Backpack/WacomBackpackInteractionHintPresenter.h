// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

enum class ECommonInputType : uint8;
enum class EWacomBackpackWorkspaceInteractionMode : uint8;

/** Screen 消费的只读背包操作提示；不包含 Run 状态或命令。 */
struct FWacomBackpackInteractionHintView
{
	FText ContextHint;
	FText HelpText;
};

/** 根据输入来源和 Workspace 模式构造稳定文案，避免 WBP 自行推断交互语义。 */
class WACOMAPP_API FWacomBackpackInteractionHintPresenter
{
public:
	static FWacomBackpackInteractionHintView Build(
		ECommonInputType InputType,
		EWacomBackpackWorkspaceInteractionMode InteractionMode,
		bool bExpandedPile);
};
