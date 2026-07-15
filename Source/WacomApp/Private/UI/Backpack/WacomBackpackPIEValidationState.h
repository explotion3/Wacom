// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_EDITOR

/** 仅供编辑器 PIE 验收使用；不会成为正式背包或 Run 状态的一部分。 */
enum class EWacomBackpackPIEValidationMode : uint8
{
	None,
	EmptySnapshot,
	NativeFallback,
};

WACOMAPP_API EWacomBackpackPIEValidationMode GetWacomBackpackPIEValidationMode();

/**
 * 只在同步构造测试 Screen 的期间切换验证模式，析构时恢复上一层模式。
 * 允许自动化测试嵌套验证，且不会把状态泄漏到随后正常打开的背包。
 */
class WACOMAPP_API FScopedWacomBackpackPIEValidationMode
{
public:
	explicit FScopedWacomBackpackPIEValidationMode(EWacomBackpackPIEValidationMode InMode);
	~FScopedWacomBackpackPIEValidationMode();

	FScopedWacomBackpackPIEValidationMode(const FScopedWacomBackpackPIEValidationMode&) = delete;
	FScopedWacomBackpackPIEValidationMode& operator=(const FScopedWacomBackpackPIEValidationMode&) = delete;

private:
	EWacomBackpackPIEValidationMode PreviousMode;
};

#endif
