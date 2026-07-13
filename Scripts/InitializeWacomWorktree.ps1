[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$WorktreePath,

    [Parameter(Mandatory = $true)]
    [string]$LocalDataPath,

    [Parameter(Mandatory = $true)]
    [string]$SeedProjectPath,

    [ValidateSet("Seed", "Verify")]
    [string]$Mode = "Seed",

    [string]$ManifestPath = (Join-Path $PSScriptRoot "WorktreeLocalDependencies.json")
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Get-ExistingFullPath
{
    param([Parameter(Mandatory = $true)][string]$Path)

    return (Resolve-Path -LiteralPath $Path).Path.TrimEnd([char[]]@('\', '/'))
}

function Get-FullPath
{
    param([Parameter(Mandatory = $true)][string]$Path)

    return [System.IO.Path]::GetFullPath($Path).TrimEnd([char[]]@('\', '/'))
}

function Test-PathWithin
{
    param(
        [Parameter(Mandatory = $true)][string]$Candidate,
        [Parameter(Mandatory = $true)][string]$Parent
    )

    $CandidatePath = Get-FullPath $Candidate
    $ParentPath = Get-FullPath $Parent
    $Prefix = $ParentPath + [System.IO.Path]::DirectorySeparatorChar
    return $CandidatePath.StartsWith($Prefix, [System.StringComparison]::OrdinalIgnoreCase)
}

function Assert-ProjectRoot
{
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if (-not (Test-Path -LiteralPath (Join-Path $Path "Wacom.uproject") -PathType Leaf))
    {
        throw "$Label does not contain Wacom.uproject: $Path"
    }
}

function Assert-CleanTrackedWorktree
{
    param([Parameter(Mandatory = $true)][string]$Path)

    $Status = @(& git -C $Path status --porcelain --untracked-files=no)
    if ($LASTEXITCODE -ne 0)
    {
        throw "Unable to inspect git status for worktree: $Path"
    }

    if ($Status.Count -gt 0)
    {
        throw "Tracked worktree changes must be committed or handled before hydration: $Path"
    }
}

function Test-LfsPointerFile
{
    param([Parameter(Mandatory = $true)][string]$Path)

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf))
    {
        return $false
    }

    $ExpectedHeader = "version https://git-lfs.github.com/spec/v1"
    $Buffer = New-Object byte[] $ExpectedHeader.Length
    $Stream = [System.IO.File]::OpenRead($Path)
    try
    {
        if ($Stream.Length -lt $ExpectedHeader.Length)
        {
            return $false
        }

        $ReadCount = $Stream.Read($Buffer, 0, $Buffer.Length)
        if ($ReadCount -ne $Buffer.Length)
        {
            return $false
        }
    }
    finally
    {
        $Stream.Dispose()
    }

    return [System.Text.Encoding]::ASCII.GetString($Buffer) -eq $ExpectedHeader
}

function Assert-LfsObjectsCheckedOut
{
    param([Parameter(Mandatory = $true)][string]$WorktreeRoot)

    $LfsFiles = @(& git -C $WorktreeRoot lfs ls-files -n)
    if ($LASTEXITCODE -ne 0)
    {
        throw "Unable to enumerate Git LFS files: $WorktreeRoot"
    }

    $PointerFiles = [System.Collections.Generic.List[string]]::new()
    foreach ($RelativePath in $LfsFiles)
    {
        $FullPath = Join-Path $WorktreeRoot $RelativePath
        if (Test-LfsPointerFile $FullPath)
        {
            $PointerFiles.Add($RelativePath)
        }
    }

    if ($PointerFiles.Count -gt 0)
    {
        $Sample = ($PointerFiles | Select-Object -First 10) -join ", "
        throw "Git LFS checkout is incomplete ($($PointerFiles.Count) pointer file(s) remain): $Sample"
    }

    return $LfsFiles.Count
}

function Get-JunctionTargetPath
{
    param([Parameter(Mandatory = $true)]$Item)

    $Targets = @($Item.Target)
    if ($Targets.Count -eq 0 -or [string]::IsNullOrWhiteSpace([string]$Targets[0]))
    {
        return $null
    }

    $Target = [string]$Targets[0]
    if (-not [System.IO.Path]::IsPathRooted($Target))
    {
        $Target = Join-Path $Item.Parent.FullName $Target
    }

    return Get-FullPath $Target
}

function Ensure-Directory
{
    param([Parameter(Mandatory = $true)][string]$Path)

    if (-not (Test-Path -LiteralPath $Path))
    {
        New-Item -ItemType Directory -Path $Path -Force | Out-Null
    }
}

function Ensure-Junction
{
    param(
        [Parameter(Mandatory = $true)][string]$VisiblePath,
        [Parameter(Mandatory = $true)][string]$BackingPath,
        [switch]$AllowExistingRealDirectory
    )

    Ensure-Directory $BackingPath

    if (Test-Path -LiteralPath $VisiblePath)
    {
        $Item = Get-Item -LiteralPath $VisiblePath -Force
        if (($Item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0)
        {
            $ActualTarget = Get-JunctionTargetPath $Item
            $ExpectedTarget = Get-FullPath $BackingPath
            if (-not [string]::Equals($ActualTarget, $ExpectedTarget, [System.StringComparison]::OrdinalIgnoreCase))
            {
                throw "Existing junction points somewhere else: $VisiblePath -> $ActualTarget"
            }

            return "Junction"
        }

        if ($AllowExistingRealDirectory)
        {
            Write-Warning "Keeping existing generated directory on its current drive: $VisiblePath"
            return "ExistingDirectory"
        }

        throw "Refusing to replace an existing real directory: $VisiblePath"
    }

    Ensure-Directory (Split-Path -Parent $VisiblePath)
    New-Item -ItemType Junction -Path $VisiblePath -Target $BackingPath | Out-Null
    return "Junction"
}

function Copy-MissingDirectoryContent
{
    param(
        [Parameter(Mandatory = $true)][string]$SourcePath,
        [Parameter(Mandatory = $true)][string]$DestinationPath
    )

    if (-not (Test-Path -LiteralPath $SourcePath -PathType Container))
    {
        throw "Seed directory is missing: $SourcePath"
    }

    Ensure-Directory $DestinationPath

    $Arguments = @(
        $SourcePath,
        $DestinationPath,
        "/E",
        "/COPY:DAT",
        "/DCOPY:DAT",
        "/R:2",
        "/W:1",
        "/XJ",
        "/XC",
        "/XN",
        "/XO",
        "/NFL",
        "/NDL",
        "/NP",
        "/NJH",
        "/NJS"
    )

    & robocopy @Arguments | Out-Null
    $ExitCode = $LASTEXITCODE
    if ($ExitCode -gt 7)
    {
        throw "Robocopy failed with exit code ${ExitCode}: $SourcePath -> $DestinationPath"
    }
}

function Assert-DirectoryComplete
{
    param(
        [Parameter(Mandatory = $true)][string]$SourcePath,
        [Parameter(Mandatory = $true)][string]$DestinationPath,
        [switch]$CompareMetadata
    )

    if (-not (Test-Path -LiteralPath $DestinationPath -PathType Container))
    {
        throw "Hydrated directory is missing: $DestinationPath"
    }

    $Missing = [System.Collections.Generic.List[string]]::new()
    $Changed = [System.Collections.Generic.List[string]]::new()
    $SourceFiles = @(Get-ChildItem -LiteralPath $SourcePath -Recurse -File -Force)
    foreach ($SourceFile in $SourceFiles)
    {
        $RelativePath = $SourceFile.FullName.Substring($SourcePath.Length).TrimStart([char[]]@('\', '/'))
        $DestinationFilePath = Join-Path $DestinationPath $RelativePath
        if (-not (Test-Path -LiteralPath $DestinationFilePath -PathType Leaf))
        {
            $Missing.Add($RelativePath)
            continue
        }

        if ($CompareMetadata)
        {
            $DestinationFile = Get-Item -LiteralPath $DestinationFilePath -Force
            if ($DestinationFile.Length -ne $SourceFile.Length)
            {
                $Changed.Add($RelativePath)
            }
        }
    }

    if ($Missing.Count -gt 0)
    {
        $Sample = ($Missing | Select-Object -First 10) -join ", "
        throw "Hydrated directory is missing $($Missing.Count) file(s): $Sample"
    }

    if ($Changed.Count -gt 0)
    {
        $Sample = ($Changed | Select-Object -First 10) -join ", "
        throw "Read-only local dependencies differ from the seed project ($($Changed.Count) file(s)): $Sample"
    }

    return $SourceFiles.Count
}

function Seed-StandaloneFile
{
    param(
        [Parameter(Mandatory = $true)][string]$RelativePath,
        [Parameter(Mandatory = $true)][string]$SeedRoot,
        [Parameter(Mandatory = $true)][string]$WorktreeRoot,
        [Parameter(Mandatory = $true)][string]$LocalRoot
    )

    $SourcePath = Join-Path $SeedRoot $RelativePath
    $BackingPath = Join-Path (Join-Path $LocalRoot "LocalDependencies") $RelativePath
    $VisiblePath = Join-Path $WorktreeRoot $RelativePath

    if (-not (Test-Path -LiteralPath $SourcePath -PathType Leaf))
    {
        throw "Seed file is missing: $SourcePath"
    }

    Ensure-Directory (Split-Path -Parent $BackingPath)
    if (-not (Test-Path -LiteralPath $BackingPath -PathType Leaf))
    {
        Copy-Item -LiteralPath $SourcePath -Destination $BackingPath
    }

    Ensure-Directory (Split-Path -Parent $VisiblePath)
    if (-not (Test-Path -LiteralPath $VisiblePath -PathType Leaf))
    {
        Copy-Item -LiteralPath $BackingPath -Destination $VisiblePath
    }

    if ((Get-Item -LiteralPath $SourcePath).Length -ne (Get-Item -LiteralPath $VisiblePath).Length)
    {
        throw "Hydrated file differs from the seed project: $VisiblePath"
    }
}

$WorktreeRoot = Get-ExistingFullPath $WorktreePath
$SeedRoot = Get-ExistingFullPath $SeedProjectPath
$ManifestFullPath = Get-ExistingFullPath $ManifestPath
$LocalRoot = Get-FullPath $LocalDataPath

Assert-ProjectRoot -Path $WorktreeRoot -Label "Worktree"
Assert-ProjectRoot -Path $SeedRoot -Label "Seed project"
Assert-CleanTrackedWorktree $WorktreeRoot

if ([string]::Equals($WorktreeRoot, $SeedRoot, [System.StringComparison]::OrdinalIgnoreCase))
{
    throw "The seed project and target worktree must be different directories."
}

if ([string]::Equals($LocalRoot, $WorktreeRoot, [System.StringComparison]::OrdinalIgnoreCase) -or
    [string]::Equals($LocalRoot, $SeedRoot, [System.StringComparison]::OrdinalIgnoreCase) -or
    (Test-PathWithin -Candidate $LocalRoot -Parent $WorktreeRoot) -or
    (Test-PathWithin -Candidate $LocalRoot -Parent $SeedRoot) -or
    (Test-PathWithin -Candidate $WorktreeRoot -Parent $LocalRoot) -or
    (Test-PathWithin -Candidate $SeedRoot -Parent $LocalRoot))
{
    throw "LocalDataPath must be outside both project trees: $LocalRoot"
}

$Manifest = Get-Content -LiteralPath $ManifestFullPath -Raw | ConvertFrom-Json
if ($Manifest.schemaVersion -ne 1)
{
    throw "Unsupported worktree dependency manifest version: $($Manifest.schemaVersion)"
}

if ($Mode -eq "Seed")
{
    Ensure-Directory $LocalRoot

    & git -C $WorktreeRoot lfs checkout
    if ($LASTEXITCODE -ne 0)
    {
        throw "Git LFS checkout failed for worktree: $WorktreeRoot"
    }
}
elseif (-not (Test-Path -LiteralPath $LocalRoot -PathType Container))
{
    throw "LocalDataPath has not been initialized: $LocalRoot"
}

$LfsFileCount = Assert-LfsObjectsCheckedOut $WorktreeRoot
Write-Host "[GitLFS] $LfsFileCount file(s) materialized"

$DirectoryPolicies = @(
    @{
        Paths = @($Manifest.readOnlySeedDirectories)
        CompareMetadata = $true
        Policy = "ReadOnlySeed"
    },
    @{
        Paths = @($Manifest.editableSeedDirectories)
        CompareMetadata = $false
        Policy = "EditableSeed"
    }
)

foreach ($Policy in $DirectoryPolicies)
{
    foreach ($RelativePath in $Policy.Paths)
    {
        $SourcePath = Join-Path $SeedRoot $RelativePath
        $BackingPath = Join-Path (Join-Path $LocalRoot "LocalDependencies") $RelativePath
        $VisiblePath = Join-Path $WorktreeRoot $RelativePath

        if ($Mode -eq "Seed")
        {
            Copy-MissingDirectoryContent -SourcePath $SourcePath -DestinationPath $BackingPath
            $null = Ensure-Junction -VisiblePath $VisiblePath -BackingPath $BackingPath
        }

        $Count = Assert-DirectoryComplete `
            -SourcePath $SourcePath `
            -DestinationPath $BackingPath `
            -CompareMetadata:$Policy.CompareMetadata

        $VisibleItem = Get-Item -LiteralPath $VisiblePath -Force
        $VisibleTarget = Get-JunctionTargetPath $VisibleItem
        if (-not [string]::Equals($VisibleTarget, (Get-FullPath $BackingPath), [System.StringComparison]::OrdinalIgnoreCase))
        {
            throw "Local dependency is not exposed through the expected junction: $VisiblePath"
        }

        Write-Host "[$($Policy.Policy)] $RelativePath : $Count file(s)"
    }
}

foreach ($RelativePath in @($Manifest.seedFiles))
{
    if ($Mode -eq "Seed")
    {
        Seed-StandaloneFile `
            -RelativePath $RelativePath `
            -SeedRoot $SeedRoot `
            -WorktreeRoot $WorktreeRoot `
            -LocalRoot $LocalRoot
    }
    else
    {
        $SourcePath = Join-Path $SeedRoot $RelativePath
        $VisiblePath = Join-Path $WorktreeRoot $RelativePath
        if (-not (Test-Path -LiteralPath $VisiblePath -PathType Leaf))
        {
            throw "Hydrated file is missing: $VisiblePath"
        }

        if ((Get-Item -LiteralPath $SourcePath).Length -ne (Get-Item -LiteralPath $VisiblePath).Length)
        {
            throw "Hydrated file differs from the seed project: $VisiblePath"
        }
    }

    Write-Host "[SeedFile] $RelativePath"
}

foreach ($RelativePath in @($Manifest.generatedDirectories))
{
    $BackingPath = Join-Path (Join-Path $LocalRoot "Generated") $RelativePath
    $VisiblePath = Join-Path $WorktreeRoot $RelativePath
    if ($Mode -eq "Seed")
    {
        $Result = Ensure-Junction `
            -VisiblePath $VisiblePath `
            -BackingPath $BackingPath `
            -AllowExistingRealDirectory
        Write-Host "[Generated:$Result] $RelativePath"
        continue
    }

    if (-not (Test-Path -LiteralPath $VisiblePath -PathType Container))
    {
        throw "Generated directory is missing: $VisiblePath"
    }

    $VisibleItem = Get-Item -LiteralPath $VisiblePath -Force
    if (($VisibleItem.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0)
    {
        $VisibleTarget = Get-JunctionTargetPath $VisibleItem
        if (-not [string]::Equals($VisibleTarget, (Get-FullPath $BackingPath), [System.StringComparison]::OrdinalIgnoreCase))
        {
            throw "Generated directory junction points somewhere else: $VisiblePath -> $VisibleTarget"
        }
    }
    else
    {
        Write-Warning "Generated directory remains on its current drive: $VisiblePath"
    }

    Write-Host "[Generated] $RelativePath"
}

Assert-CleanTrackedWorktree $WorktreeRoot
Write-Host "Worktree local dependencies are ready: $WorktreeRoot"
