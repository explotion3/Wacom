[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet(
        "Start",
        "Status",
        "AssertReady",
        "AssertClosedForBuild",
        "AcquireWriter",
        "ReleaseWriter",
        "ArchiveStaleWriter",
        "PrintCodexConfig")]
    [string]$Action,

    [string]$Role,
    [string]$ProjectRoot,
    [string]$ExpectedBranch,
    [string]$ThreadId,
    [string]$Reason,
    [string[]]$Packages = @(),
    [string]$EditorExecutable,
    [string]$LocalStateRoot,
    [int]$TimeoutSeconds = 120,
    [switch]$AllowDirty,
    [switch]$AllowExistingDirtyPackages,
    [switch]$AllowProtectedRoleWrite,
    [switch]$ConfirmStaleWriterArchive,
    [string]$EndpointConfigPath = (Join-Path $PSScriptRoot "UnrealMcp\Endpoints.json")
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$ModulePath = Join-Path $PSScriptRoot "UnrealMcp\WacomUnrealMcp.psm1"
Import-Module $ModulePath -Force

Invoke-WacomUnrealMcp `
    -Action $Action `
    -Role $Role `
    -ProjectRoot $ProjectRoot `
    -ExpectedBranch $ExpectedBranch `
    -ThreadId $ThreadId `
    -Reason $Reason `
    -Packages $Packages `
    -EditorExecutable $EditorExecutable `
    -LocalStateRoot $LocalStateRoot `
    -TimeoutSeconds $TimeoutSeconds `
    -AllowDirty:$AllowDirty `
    -AllowExistingDirtyPackages:$AllowExistingDirtyPackages `
    -AllowProtectedRoleWrite:$AllowProtectedRoleWrite `
    -ConfirmStaleWriterArchive:$ConfirmStaleWriterArchive `
    -EndpointConfigPath $EndpointConfigPath
