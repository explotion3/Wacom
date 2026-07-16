# Contract: Run Floor Scene Validation

## Shared entry point

The public editor validation surface accepts only a loaded World:

```cpp
static FWacomRunSceneBindingValidationReport ValidateLoadedWorld(
    const UWorld* World);
```

It resolves the unique Scene Descriptor internally. Callers cannot pass an unrelated Floor definition that bypasses the World declaration.

The report contains stable diagnostics:

```cpp
enum class EWacomRunSceneBindingDiagnosticSeverity { Info, Warning, Error };
enum class EWacomRunSceneBindingDiagnosticCode : uint8 { /* stable families */ };

struct FWacomRunSceneBindingDiagnostic
{
    EWacomRunSceneBindingDiagnosticSeverity Severity;
    EWacomRunSceneBindingDiagnosticCode Code;
    FString ObjectPath;
    FText Message;
};
```

The exact enum members are finalized beside focused tests. Text is explanatory; automation asserts code/severity/object identity rather than localized wording.

## Required validation

### Descriptor

- exactly one descriptor;
- non-null Floor definition;
- non-empty FloorId.

### Nodes and content hosts

- every Floor node has exactly one expected Anchor;
- no unexpected or duplicate Anchor identities;
- activity node payload/host exists and matches its declared type;
- navigation-only nodes do not acquire unrelated required-host semantics.

### Edges and branches

- every Floor Edge has exactly one Path segment;
- no unexpected or duplicate EdgeId;
- Path source/target identity matches Floor data;
- only statically multi-outgoing nodes require BranchTarget actors;
- each required Edge has exactly one BranchTarget; single-exit edges do not require one.

### Spline geometry

- at least 2 spline points;
- total spline length `> 10 cm`;
- all point location/rotation/scale values finite;
- source endpoint corresponds to source Anchor and target endpoint to target Anchor;
- direct endpoint distance thresholds:
  - `<= 100 cm`: pass;
  - `> 100 cm && <= 300 cm`: warning;
  - `> 300 cm`: error;
- if reversed total distance is at least `1 cm` smaller than direct total distance, emit reversed-direction error.

## Strict read-only guarantee

Validation code and every caller must not invoke:

- `Modify()`;
- `MarkPackageDirty()`;
- `SavePackage()`;
- Actor/DataAsset mutation or automatic repair;
- Blueprint compilation or construction-script rerun as a side effect.

Tests capture dirty-package state before and after valid and invalid validation.

## Editor menu

`Tools -> Wacom -> Validate Current Run Floor`:

- validates the current editor World;
- displays a concise success notification or opens/logs diagnostics;
- never saves or repairs;
- is registered/unregistered symmetrically by `FWacomEditorModule`.

## Commandlet

```text
-run=WacomValidateRunFloorScene -Map=/Game/Wacom/Maps/L_Exploration
```

Exit codes:

- `0`: map loaded, descriptor resolved, no Error diagnostics;
- `1`: map/descriptor resolved, scene contract has one or more Errors;
- `2`: invalid/missing arguments, package/world load failure, or descriptor cannot be resolved sufficiently to identify the Floor.

Warnings are printed but return `0`. Diagnostics are sorted and emitted with code + object path + message.
