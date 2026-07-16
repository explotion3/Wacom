// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWorld;

enum class EWacomRunSceneBindingDiagnosticSeverity : uint8
{
	Info,
	Warning,
	Error
};

enum class EWacomRunSceneBindingDiagnosticCode : uint8
{
	WorldInvalid,
	WorldTypeUnsupported,
	DescriptorMissing,
	DescriptorDuplicate,
	DescriptorFloorMissing,
	DescriptorFloorIdentityMissing,
	NodeAnchorMissing,
	NodeAnchorDuplicate,
	NodeAnchorUnexpected,
	EdgePathMissing,
	EdgePathDuplicate,
	EdgePathUnexpected,
	BranchTargetMissing,
	BranchTargetDuplicate,
	BranchTargetUnexpected,
	ContentHostMissing,
	ContentHostDuplicate,
	ContentHostUnexpected,
	ContentHostTypeMismatch,
	ContentHostPayloadMismatch,
	SplinePointCountInvalid,
	SplineLengthTooShort,
	SplineTransformNonFinite,
	SplineDirectionReversed,
	SplineSourceEndpointWarning,
	SplineSourceEndpointError,
	SplineTargetEndpointWarning,
	SplineTargetEndpointError
};

struct WACOMEDITOR_API FWacomRunSceneBindingDiagnostic
{
	EWacomRunSceneBindingDiagnosticSeverity Severity =
		EWacomRunSceneBindingDiagnosticSeverity::Error;
	EWacomRunSceneBindingDiagnosticCode Code =
		EWacomRunSceneBindingDiagnosticCode::WorldInvalid;
	FString ObjectPath;
	FText Message;
};

/** Loaded exploration World 与当前 Floor 静态图之间的制作期绑定校验结果。 */
struct WACOMEDITOR_API FWacomRunSceneBindingValidationReport
{
	TArray<FWacomRunSceneBindingDiagnostic> Diagnostics;

	bool HasErrors() const;
	bool IsValid() const { return !HasErrors(); }
	bool HasCode(EWacomRunSceneBindingDiagnosticCode Code) const;
	bool HasDescriptorResolutionError() const;
	void Sort();
};

/** 只读取 loaded World；不会从 Actor 反向生成或修改 Floor 图。 */
struct WACOMEDITOR_API FWacomRunSceneBindingValidation
{
	static FWacomRunSceneBindingValidationReport ValidateLoadedWorld(const UWorld* World);
};

WACOMEDITOR_API const TCHAR* LexToString(
	EWacomRunSceneBindingDiagnosticSeverity Severity);
WACOMEDITOR_API const TCHAR* LexToString(EWacomRunSceneBindingDiagnosticCode Code);
