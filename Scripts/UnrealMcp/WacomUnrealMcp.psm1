Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Get-WacomUnrealMcpDefaultStateRoot
{
    if ([string]::IsNullOrWhiteSpace($env:LOCALAPPDATA))
    {
        throw "LOCALAPPDATA is not available. Pass -LocalStateRoot explicitly."
    }

    return Join-Path $env:LOCALAPPDATA "Wacom\UnrealMcp"
}

function Get-WacomUnrealMcpDefaultEditorExecutable
{
    if (-not [string]::IsNullOrWhiteSpace($env:WACOM_UNREAL_EDITOR))
    {
        return $env:WACOM_UNREAL_EDITOR
    }

    return "E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
}

function Get-CanonicalPath
{
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,
        [switch]$MustExist
    )

    if ($MustExist)
    {
        return (Resolve-Path -LiteralPath $Path).Path.TrimEnd([char[]]@('\', '/'))
    }

    return [System.IO.Path]::GetFullPath($Path).TrimEnd([char[]]@('\', '/'))
}

function Test-PathEqual
{
    param(
        [Parameter(Mandatory = $true)][string]$Left,
        [Parameter(Mandatory = $true)][string]$Right
    )

    return [string]::Equals(
        (Get-CanonicalPath -Path $Left),
        (Get-CanonicalPath -Path $Right),
        [System.StringComparison]::OrdinalIgnoreCase)
}

function Get-ContainedProjectPath
{
    param(
        [Parameter(Mandatory = $true)][string]$ProjectRoot,
        [Parameter(Mandatory = $true)][string]$RelativePath
    )

    if ([System.IO.Path]::IsPathRooted($RelativePath))
    {
        throw "Expected a repository-relative path, received: $RelativePath"
    }

    $Root = Get-CanonicalPath -Path $ProjectRoot -MustExist
    $FullPath = Get-CanonicalPath -Path (Join-Path $Root $RelativePath)
    $RootPrefix = $Root.TrimEnd([char[]]@('\', '/')) + [System.IO.Path]::DirectorySeparatorChar
    if (-not $FullPath.StartsWith($RootPrefix, [System.StringComparison]::OrdinalIgnoreCase))
    {
        throw "Repository-relative path escapes ProjectRoot: $RelativePath"
    }
    return $FullPath
}

function Convert-ToNormalizedCommandLine
{
    param([AllowNull()][string]$CommandLine)

    if ($null -eq $CommandLine)
    {
        return ""
    }

    return $CommandLine.Replace('/', '\')
}

function Test-CommandLineContainsPath
{
    param(
        [AllowNull()][string]$CommandLine,
        [Parameter(Mandatory = $true)][string]$Path
    )

    $NormalizedCommandLine = Convert-ToNormalizedCommandLine $CommandLine
    $NormalizedPath = (Get-CanonicalPath -Path $Path).Replace('/', '\')
    return $NormalizedCommandLine.IndexOf(
        $NormalizedPath,
        [System.StringComparison]::OrdinalIgnoreCase) -ge 0
}

function Read-EndpointConfig
{
    param([Parameter(Mandatory = $true)][string]$Path)

    $ConfigPath = Get-CanonicalPath -Path $Path -MustExist
    $Config = Get-Content -LiteralPath $ConfigPath -Raw | ConvertFrom-Json
    if ($Config.schemaVersion -ne 1)
    {
        throw "Unsupported Unreal MCP endpoint schema version: $($Config.schemaVersion)"
    }

    $Roles = @($Config.roles)
    if ($Roles.Count -eq 0)
    {
        throw "Unreal MCP endpoint config has no roles: $ConfigPath"
    }

    $DuplicateNames = @(
        $Roles |
            Group-Object -Property name |
            Where-Object Count -gt 1 |
            ForEach-Object Name)
    $DuplicatePorts = @(
        $Roles |
            Group-Object -Property port |
            Where-Object Count -gt 1 |
            ForEach-Object Name)
    $DuplicateEndpoints = @(
        $Roles |
            Group-Object -Property codexEndpoint |
            Where-Object Count -gt 1 |
            ForEach-Object Name)
    if ($DuplicateNames.Count -gt 0 -or $DuplicatePorts.Count -gt 0 -or $DuplicateEndpoints.Count -gt 0)
    {
        throw "Unreal MCP endpoint names, ports, and Codex endpoint names must be unique."
    }

    foreach ($Entry in $Roles)
    {
        if (
            [string]::IsNullOrWhiteSpace([string]$Entry.name) -or
            [string]::IsNullOrWhiteSpace([string]$Entry.codexEndpoint) -or
            $Entry.port -lt 1 -or
            $Entry.port -gt 65535 -or
            @("read-only", "writer-eligible") -notcontains [string]$Entry.defaultAccess)
        {
            throw "Invalid Unreal MCP endpoint entry: $($Entry | ConvertTo-Json -Compress)"
        }
    }

    return $Config
}

function Get-RoleConfig
{
    param(
        [Parameter(Mandatory = $true)]$Config,
        [Parameter(Mandatory = $true)][string]$Role
    )

    $Matches = @($Config.roles | Where-Object { $_.name -eq $Role })
    if ($Matches.Count -ne 1)
    {
        $KnownRoles = (@($Config.roles | ForEach-Object name) -join ", ")
        throw "Unknown Unreal MCP role '$Role'. Known roles: $KnownRoles"
    }

    return $Matches[0]
}

function Assert-RoleProvided
{
    param([AllowNull()][string]$Role)

    if ([string]::IsNullOrWhiteSpace($Role))
    {
        throw "-Role is required for this action."
    }
}

function Initialize-StateDirectories
{
    param([Parameter(Mandatory = $true)][string]$StateRoot)

    foreach ($Name in @("Sessions", "Writers", "Audits", "Archive"))
    {
        $Path = Join-Path $StateRoot $Name
        if (-not (Test-Path -LiteralPath $Path -PathType Container))
        {
            New-Item -ItemType Directory -Path $Path -Force | Out-Null
        }
    }
}

function Get-StateFilePath
{
    param(
        [Parameter(Mandatory = $true)][string]$StateRoot,
        [Parameter(Mandatory = $true)][ValidateSet("Session", "Writer")][string]$Kind,
        [Parameter(Mandatory = $true)][string]$Role
    )

    $DirectoryName = if ($Kind -eq "Session") { "Sessions" } else { "Writers" }
    return Join-Path (Join-Path $StateRoot $DirectoryName) "$Role.json"
}

function Write-JsonFile
{
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)]$Value
    )

    $Json = $Value | ConvertTo-Json -Depth 20
    Set-Content -LiteralPath $Path -Value $Json -Encoding utf8NoBOM
}

function Write-NewJsonFile
{
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)]$Value
    )

    $Json = $Value | ConvertTo-Json -Depth 20
    $Bytes = [System.Text.UTF8Encoding]::new($false).GetBytes($Json)
    $Stream = [System.IO.File]::Open(
        $Path,
        [System.IO.FileMode]::CreateNew,
        [System.IO.FileAccess]::Write,
        [System.IO.FileShare]::None)
    try
    {
        $Stream.Write($Bytes, 0, $Bytes.Length)
    }
    finally
    {
        $Stream.Dispose()
    }
}

function Read-JsonFile
{
    param([Parameter(Mandatory = $true)][string]$Path)

    return Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json
}

function Move-StateFileToArchive
{
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$StateRoot,
        [Parameter(Mandatory = $true)][string]$Reason
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf))
    {
        return
    }

    $ArchiveRoot = Get-CanonicalPath -Path (Join-Path $StateRoot "Archive")
    $ResolvedPath = Get-CanonicalPath -Path $Path -MustExist
    $ExpectedParent = Get-CanonicalPath -Path (Split-Path -Parent $Path)
    if (-not (Test-PathEqual -Left (Split-Path -Parent $ResolvedPath) -Right $ExpectedParent))
    {
        throw "Refusing to archive a state file outside its expected directory: $ResolvedPath"
    }

    $Timestamp = [DateTime]::UtcNow.ToString("yyyyMMdd-HHmmss-fff")
    $Destination = Join-Path $ArchiveRoot "$Timestamp-$Reason-$(Split-Path -Leaf $Path)"
    Move-Item -LiteralPath $ResolvedPath -Destination $Destination
    return $Destination
}

function Invoke-Git
{
    param(
        [Parameter(Mandatory = $true)][string]$ProjectRoot,
        [Parameter(Mandatory = $true)][string[]]$Arguments
    )

    $Output = @(& git -C $ProjectRoot @Arguments 2>&1)
    if ($LASTEXITCODE -ne 0)
    {
        throw "git $($Arguments -join ' ') failed in ${ProjectRoot}: $($Output -join [Environment]::NewLine)"
    }

    return $Output
}

function Get-GitDirtyPaths
{
    param([Parameter(Mandatory = $true)][string]$ProjectRoot)

    $Lines = @(Invoke-Git -ProjectRoot $ProjectRoot -Arguments @(
        "-c", "core.quotepath=false", "status", "--porcelain=v1", "--untracked-files=all"))
    $Paths = [System.Collections.Generic.List[string]]::new()
    foreach ($LineValue in $Lines)
    {
        $Line = [string]$LineValue
        if ($Line.Length -lt 4)
        {
            continue
        }

        $Path = $Line.Substring(3).Trim()
        $RenameSeparator = " -> "
        $RenameIndex = $Path.LastIndexOf($RenameSeparator, [System.StringComparison]::Ordinal)
        if ($RenameIndex -ge 0)
        {
            $Path = $Path.Substring($RenameIndex + $RenameSeparator.Length)
        }
        $Paths.Add($Path.Replace('\', '/'))
    }

    return @($Paths | Sort-Object -Unique)
}

function Get-GitStatusLines
{
    param([Parameter(Mandatory = $true)][string]$ProjectRoot)

    return @(
        Invoke-Git -ProjectRoot $ProjectRoot -Arguments @(
            "-c", "core.quotepath=false", "status", "--short", "--untracked-files=all") |
            ForEach-Object { [string]$_ })
}

function Get-LfsFilterMap
{
    param(
        [Parameter(Mandatory = $true)][string]$ProjectRoot,
        [Parameter(Mandatory = $true)][string[]]$RelativePaths
    )

    $Filters = [ordered]@{}
    foreach ($RelativePath in @($RelativePaths | Sort-Object -Unique))
    {
        $Output = @(
            Invoke-Git -ProjectRoot $ProjectRoot -Arguments @(
                "check-attr", "filter", "--", $RelativePath))
        $Line = [string]($Output | Select-Object -First 1)
        $Marker = ": filter: "
        $MarkerIndex = $Line.LastIndexOf($Marker, [System.StringComparison]::Ordinal)
        $Filters[$RelativePath] = if ($MarkerIndex -ge 0) {
            $Line.Substring($MarkerIndex + $Marker.Length).Trim()
        } else {
            "unspecified"
        }
    }
    return $Filters
}

function Get-GitContext
{
    param(
        [Parameter(Mandatory = $true)][string]$ProjectRoot,
        [AllowNull()][string]$ExpectedBranch
    )

    $Root = Get-CanonicalPath -Path $ProjectRoot -MustExist
    $ProjectFile = Join-Path $Root "Wacom.uproject"
    if (-not (Test-Path -LiteralPath $ProjectFile -PathType Leaf))
    {
        throw "ProjectRoot does not contain Wacom.uproject: $Root"
    }

    $GitRoot = [string](Invoke-Git -ProjectRoot $Root -Arguments @("rev-parse", "--show-toplevel") | Select-Object -First 1)
    if (-not (Test-PathEqual -Left $GitRoot -Right $Root))
    {
        throw "ProjectRoot must be the Git worktree root. Expected '$GitRoot', received '$Root'."
    }

    $Branch = [string](Invoke-Git -ProjectRoot $Root -Arguments @("rev-parse", "--abbrev-ref", "HEAD") | Select-Object -First 1)
    if (-not [string]::IsNullOrWhiteSpace($ExpectedBranch) -and $Branch -ne $ExpectedBranch)
    {
        throw "Worktree branch mismatch. Expected '$ExpectedBranch', actual '$Branch'."
    }

    $Head = [string](Invoke-Git -ProjectRoot $Root -Arguments @("rev-parse", "HEAD") | Select-Object -First 1)
    return [pscustomobject]@{
        ProjectRoot = $Root
        ProjectFile = Get-CanonicalPath -Path $ProjectFile -MustExist
        Branch = $Branch
        Head = $Head
        DirtyPaths = @(Get-GitDirtyPaths -ProjectRoot $Root)
    }
}

function Assert-EditorModuleBinariesPresent
{
    param([Parameter(Mandatory = $true)][string]$ProjectRoot)

    $Root = Get-CanonicalPath -Path $ProjectRoot -MustExist
    $Descriptors = [System.Collections.Generic.List[object]]::new()
    $Descriptors.Add([pscustomobject]@{
        Path = Join-Path $Root "Wacom.uproject"
        BinaryRoot = Join-Path $Root "Binaries\Win64"
    })

    $PluginsRoot = Join-Path $Root "Plugins"
    if (Test-Path -LiteralPath $PluginsRoot -PathType Container)
    {
        foreach ($PluginDescriptor in @(Get-ChildItem -LiteralPath $PluginsRoot -Recurse -Filter "*.uplugin" -File))
        {
            $Descriptors.Add([pscustomobject]@{
                Path = $PluginDescriptor.FullName
                BinaryRoot = Join-Path $PluginDescriptor.Directory.FullName "Binaries\Win64"
            })
        }
    }

    $Missing = [System.Collections.Generic.List[string]]::new()
    foreach ($Descriptor in $Descriptors)
    {
        $Json = Get-Content -LiteralPath $Descriptor.Path -Raw | ConvertFrom-Json
        $ModulesProperty = $Json.PSObject.Properties["Modules"]
        if ($null -eq $ModulesProperty)
        {
            continue
        }
        foreach ($Module in @($ModulesProperty.Value))
        {
            $ModuleName = [string]$Module.Name
            if ([string]::IsNullOrWhiteSpace($ModuleName))
            {
                continue
            }
            $BinaryPath = Join-Path $Descriptor.BinaryRoot "UnrealEditor-$ModuleName.dll"
            if (-not (Test-Path -LiteralPath $BinaryPath -PathType Leaf))
            {
                $Missing.Add($BinaryPath)
            }
        }
    }

    if ($Missing.Count -gt 0)
    {
        throw "Editor module binaries are missing for this worktree. Run AssertClosedForBuild, then build WacomEditor with Build.bat before Start. Missing: $($Missing -join ', ')"
    }
}

function Get-ProcessRecord
{
    param([Parameter(Mandatory = $true)][int]$ProcessId)

    $Process = Get-Process -Id $ProcessId -ErrorAction SilentlyContinue
    if (-not $Process)
    {
        return $null
    }

    $CimProcess = Get-CimInstance Win32_Process -Filter "ProcessId = $ProcessId" -ErrorAction SilentlyContinue
    if (-not $CimProcess)
    {
        return $null
    }

    return [pscustomobject]@{
        Id = $Process.Id
        Name = $Process.ProcessName
        StartTimeUtc = $Process.StartTime.ToUniversalTime().ToString("o")
        StartTimeUtcTicks = $Process.StartTime.ToUniversalTime().Ticks
        CommandLine = [string]$CimProcess.CommandLine
    }
}

function Get-RecordedProcessStartTimeUtcTicks
{
    param([Parameter(Mandatory = $true)]$Record)

    $TicksProperty = $Record.PSObject.Properties["editorProcessStartTimeUtcTicks"]
    if ($null -ne $TicksProperty)
    {
        return [long]$TicksProperty.Value
    }

    $TimeProperty = $Record.PSObject.Properties["editorProcessStartTimeUtc"]
    if ($null -eq $TimeProperty)
    {
        throw "State record has no Editor process start time."
    }
    if ($TimeProperty.Value -is [DateTime])
    {
        return ([DateTime]$TimeProperty.Value).ToUniversalTime().Ticks
    }
    if ($TimeProperty.Value -is [DateTimeOffset])
    {
        return ([DateTimeOffset]$TimeProperty.Value).UtcDateTime.Ticks
    }

    $Parsed = [DateTimeOffset]::Parse(
        [string]$TimeProperty.Value,
        [System.Globalization.CultureInfo]::InvariantCulture,
        [System.Globalization.DateTimeStyles]::RoundtripKind)
    return $Parsed.UtcDateTime.Ticks
}

function Get-PortOwnerIds
{
    param([Parameter(Mandatory = $true)][int]$Port)

    return @(
        Get-NetTCPConnection -State Listen -LocalPort $Port -ErrorAction SilentlyContinue |
            Select-Object -ExpandProperty OwningProcess -Unique)
}

function Get-UnrealEditorProcessesForProject
{
    param([Parameter(Mandatory = $true)][string]$ProjectFile)

    return @(
        Get-CimInstance Win32_Process |
            Where-Object {
                $_.Name -in @("UnrealEditor.exe", "UnrealEditor-Cmd.exe") -and
                (Test-CommandLineContainsPath -CommandLine $_.CommandLine -Path $ProjectFile)
            })
}

function Test-SessionProcessAlive
{
    param([Parameter(Mandatory = $true)]$Session)

    $Process = Get-ProcessRecord -ProcessId ([int]$Session.editorPid)
    if (-not $Process)
    {
        return $false
    }

    return $Process.StartTimeUtcTicks -eq (Get-RecordedProcessStartTimeUtcTicks -Record $Session)
}

function ConvertFrom-McpHttpContent
{
    param([Parameter(Mandatory = $true)][string]$Content)

    $Trimmed = $Content.Trim()
    if ($Trimmed.StartsWith("{", [System.StringComparison]::Ordinal))
    {
        return $Trimmed | ConvertFrom-Json
    }

    $DataLines = @(
        $Content -split "`r?`n" |
            Where-Object { $_ -like "data:*" } |
            ForEach-Object { $_.Substring(5).TrimStart() } |
            Where-Object { $_ -ne "[DONE]" })
    if ($DataLines.Count -eq 0)
    {
        throw "MCP HTTP response contained neither JSON nor SSE data."
    }
    return ($DataLines -join "`n") | ConvertFrom-Json
}

function Invoke-McpToolCallRequest
{
    param(
        [Parameter(Mandatory = $true)][string]$Uri,
        [Parameter(Mandatory = $true)][hashtable]$Headers,
        [Parameter(Mandatory = $true)][int]$RequestId,
        [Parameter(Mandatory = $true)][string]$ToolName,
        [Parameter(Mandatory = $true)][hashtable]$Arguments,
        [int]$TimeoutSeconds = 10
    )

    $Body = @{
        jsonrpc = "2.0"
        id = $RequestId
        method = "tools/call"
        params = @{
            name = $ToolName
            arguments = $Arguments
        }
    } | ConvertTo-Json -Depth 20 -Compress
    $Response = Invoke-WebRequest `
        -Uri $Uri `
        -Method Post `
        -Headers $Headers `
        -ContentType "application/json" `
        -Body $Body `
        -TimeoutSec $TimeoutSeconds `
        -UseBasicParsing
    if ($Response.StatusCode -ne 200)
    {
        throw "MCP tool '$ToolName' failed with HTTP $($Response.StatusCode)."
    }

    $Payload = ConvertFrom-McpHttpContent -Content ([string]$Response.Content)
    $ErrorProperty = $Payload.PSObject.Properties["error"]
    if ($null -ne $ErrorProperty)
    {
        throw "MCP tool '$ToolName' returned an error: $($ErrorProperty.Value | ConvertTo-Json -Depth 10 -Compress)"
    }
    $ResultProperty = $Payload.PSObject.Properties["result"]
    if ($null -eq $ResultProperty)
    {
        throw "MCP tool '$ToolName' returned no result."
    }
    $IsErrorProperty = $ResultProperty.Value.PSObject.Properties["isError"]
    if ($null -ne $IsErrorProperty -and [bool]$IsErrorProperty.Value)
    {
        throw "MCP tool '$ToolName' reported failure: $($ResultProperty.Value.content | ConvertTo-Json -Depth 10 -Compress)"
    }
    return $Payload
}

function Invoke-McpHealthCheck
{
    param(
        [Parameter(Mandatory = $true)][int]$Port,
        [int]$TimeoutSeconds = 30
    )

    $Uri = "http://127.0.0.1:$Port/mcp"
    $SessionId = $null
    try
    {
        $InitializeBody = @{
            jsonrpc = "2.0"
            id = 1
            method = "initialize"
            params = @{
                protocolVersion = "2025-11-25"
                clientInfo = @{
                    name = "WacomUnrealMcpHealth"
                    version = "1.0"
                }
                capabilities = @{}
            }
        } | ConvertTo-Json -Depth 10 -Compress
        $InitializeResponse = Invoke-WebRequest `
            -Uri $Uri `
            -Method Post `
            -ContentType "application/json" `
            -Body $InitializeBody `
            -TimeoutSec $TimeoutSeconds `
            -UseBasicParsing
        $SessionId = [string]$InitializeResponse.Headers["Mcp-Session-Id"]
        if ($InitializeResponse.StatusCode -ne 200 -or [string]::IsNullOrWhiteSpace($SessionId))
        {
            throw "MCP initialize did not return a usable session id."
        }

        $Headers = @{ "Mcp-Session-Id" = $SessionId }
        $InitializedBody = @{
            jsonrpc = "2.0"
            method = "notifications/initialized"
        } | ConvertTo-Json -Compress
        $InitializedResponse = Invoke-WebRequest `
            -Uri $Uri `
            -Method Post `
            -Headers $Headers `
            -ContentType "application/json" `
            -Body $InitializedBody `
            -TimeoutSec $TimeoutSeconds `
            -UseBasicParsing
        if ($InitializedResponse.StatusCode -notin @(200, 202))
        {
            throw "MCP initialized notification failed with HTTP $($InitializedResponse.StatusCode)."
        }

        $ToolNames = [System.Collections.Generic.List[string]]::new()
        $Cursor = $null
        $RequestId = 2
        do
        {
            $Params = @{}
            if (-not [string]::IsNullOrWhiteSpace([string]$Cursor))
            {
                $Params.cursor = [string]$Cursor
            }
            $ListBodyValue = @{
                jsonrpc = "2.0"
                id = $RequestId
                method = "tools/list"
            }
            if ($Params.Count -gt 0)
            {
                $ListBodyValue.params = $Params
            }
            $ListBody = $ListBodyValue | ConvertTo-Json -Depth 10 -Compress
            $ListResponse = Invoke-WebRequest `
                -Uri $Uri `
                -Method Post `
                -Headers $Headers `
                -ContentType "application/json" `
                -Body $ListBody `
                -TimeoutSec $TimeoutSeconds `
                -UseBasicParsing
            if ($ListResponse.StatusCode -ne 200)
            {
                throw "MCP tools/list failed with HTTP $($ListResponse.StatusCode)."
            }

            $ListPayload = $ListResponse.Content | ConvertFrom-Json
            foreach ($Tool in @($ListPayload.result.tools))
            {
                $ToolNames.Add([string]$Tool.name)
            }
            $NextCursorProperty = $ListPayload.result.PSObject.Properties["nextCursor"]
            $Cursor = if ($null -ne $NextCursorProperty) {
                [string]$NextCursorProperty.Value
            } else {
                $null
            }
            ++$RequestId
        }
        while (-not [string]::IsNullOrWhiteSpace([string]$Cursor))

        if ($ToolNames.Count -eq 0)
        {
            throw "MCP server returned no tools."
        }

        $RequiredGatewayTools = @("list_toolsets", "describe_toolset", "call_tool")
        $MissingGatewayTools = @($RequiredGatewayTools | Where-Object { $ToolNames -notcontains $_ })
        if ($MissingGatewayTools.Count -gt 0)
        {
            throw "MCP server is reachable but its toolset gateway is incomplete. Missing: $($MissingGatewayTools -join ', ')"
        }

        $EditorToolsetName = "EditorToolset.EditorAppToolset"
        try
        {
            $ProbePayload = Invoke-McpToolCallRequest `
                -Uri $Uri `
                -Headers $Headers `
                -RequestId $RequestId `
                -ToolName "call_tool" `
                -Arguments @{
                    toolset_name = $EditorToolsetName
                    tool_name = "IsPIERunning"
                    arguments = @{}
                } `
                -TimeoutSeconds $TimeoutSeconds
        }
        catch
        {
            throw "MCP server is reachable but $EditorToolsetName.IsPIERunning is unavailable. Run ModelContextProtocol.RefreshTools and retry. $($_.Exception.Message)"
        }
        $ProbeText = [string]$ProbePayload.result.content[0].text
        $ProbeResult = $ProbeText | ConvertFrom-Json
        if ($null -eq $ProbeResult.PSObject.Properties["returnValue"])
        {
            throw "$EditorToolsetName.IsPIERunning returned an unexpected payload."
        }

        return [pscustomobject]@{
            Uri = $Uri
            ToolCount = $ToolNames.Count
            HasEditorIdentityProbe = $true
            IsPIERunning = [bool]$ProbeResult.returnValue
        }
    }
    finally
    {
        if (-not [string]::IsNullOrWhiteSpace([string]$SessionId))
        {
            try
            {
                Invoke-WebRequest `
                    -Uri $Uri `
                    -Method Delete `
                    -Headers @{ "Mcp-Session-Id" = $SessionId } `
                    -TimeoutSec $TimeoutSeconds `
                    -UseBasicParsing | Out-Null
            }
            catch
            {
                Write-Warning "MCP health session cleanup failed: $($_.Exception.Message)"
            }
        }
    }
}

function Assert-SessionIdentity
{
    param(
        [Parameter(Mandatory = $true)]$RoleConfig,
        [Parameter(Mandatory = $true)][string]$StateRoot,
        [AllowNull()][string]$ProjectRoot,
        [AllowNull()][string]$ExpectedBranch,
        [switch]$SkipMcpHealth
    )

    $SessionPath = Get-StateFilePath -StateRoot $StateRoot -Kind Session -Role $RoleConfig.name
    if (-not (Test-Path -LiteralPath $SessionPath -PathType Leaf))
    {
        throw "No active Unreal MCP session is recorded for role '$($RoleConfig.name)'."
    }

    $Session = Read-JsonFile -Path $SessionPath
    if (
        [int]$Session.schemaVersion -ne 1 -or
        [string]$Session.role -ne [string]$RoleConfig.name -or
        [int]$Session.port -ne [int]$RoleConfig.port -or
        [string]$Session.codexEndpoint -ne [string]$RoleConfig.codexEndpoint)
    {
        throw "Session endpoint metadata does not match the repository endpoint config."
    }

    $Process = Get-ProcessRecord -ProcessId ([int]$Session.editorPid)
    if (
        -not $Process -or
        $Process.StartTimeUtcTicks -ne (Get-RecordedProcessStartTimeUtcTicks -Record $Session))
    {
        throw "Recorded Editor process is no longer alive for role '$($RoleConfig.name)'."
    }
    if ($Process.Name -notlike "UnrealEditor*")
    {
        throw "Recorded process is not an Unreal Editor process."
    }
    if (-not (Test-CommandLineContainsPath -CommandLine $Process.CommandLine -Path ([string]$Session.projectFile)))
    {
        throw "Recorded Editor PID no longer targets the recorded Wacom.uproject."
    }
    if (-not $Process.CommandLine.Contains("-WacomMcpSessionId=$($Session.sessionId)"))
    {
        throw "Editor command line does not contain the recorded Wacom MCP session id."
    }

    $PortOwners = @(Get-PortOwnerIds -Port ([int]$RoleConfig.port))
    if ($PortOwners.Count -eq 0 -or @($PortOwners | Where-Object { $_ -ne [int]$Session.editorPid }).Count -gt 0)
    {
        throw "Port $($RoleConfig.port) is not exclusively owned by Editor PID $($Session.editorPid)."
    }

    if (
        -not [string]::IsNullOrWhiteSpace($ProjectRoot) -and
        -not (Test-PathEqual -Left $ProjectRoot -Right ([string]$Session.projectRoot)))
    {
        throw "Requested ProjectRoot does not match the active session."
    }

    $GitContext = Get-GitContext `
        -ProjectRoot ([string]$Session.projectRoot) `
        -ExpectedBranch $ExpectedBranch
    if (
        -not (Test-PathEqual -Left $GitContext.ProjectFile -Right ([string]$Session.projectFile)) -or
        $GitContext.Branch -ne [string]$Session.branch -or
        $GitContext.Head -ne [string]$Session.head)
    {
        throw "Git branch or HEAD changed while the Editor MCP session remained active. Close and restart the session."
    }

    $Health = $null
    if (-not $SkipMcpHealth)
    {
        $Health = Invoke-McpHealthCheck -Port ([int]$RoleConfig.port)
    }

    return [pscustomobject]@{
        Session = $Session
        Process = $Process
        Git = $GitContext
        Health = $Health
    }
}

function Get-RelativePackageFileCandidates
{
    param([Parameter(Mandatory = $true)][string]$Package)

    if (
        $Package -notmatch '^/Game/[A-Za-z0-9_]+(?:/[A-Za-z0-9_]+)*$')
    {
        throw "Package allowlist entries must be full /Game/... package paths without extensions: $Package"
    }

    $RelativeStem = "Content/" + $Package.Substring(6).Replace('\', '/')
    return @("$RelativeStem.uasset", "$RelativeStem.umap")
}

function Get-FileHashes
{
    param(
        [Parameter(Mandatory = $true)][string]$ProjectRoot,
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [string[]]$RelativePaths
    )

    $Hashes = [ordered]@{}
    foreach ($RelativePath in @($RelativePaths | Sort-Object -Unique))
    {
        $FullPath = Get-ContainedProjectPath `
            -ProjectRoot $ProjectRoot `
            -RelativePath $RelativePath
        if (Test-Path -LiteralPath $FullPath -PathType Leaf)
        {
            $Hashes[$RelativePath] = (Get-FileHash -LiteralPath $FullPath -Algorithm SHA256).Hash
        }
        else
        {
            $Hashes[$RelativePath] = $null
        }
    }
    return $Hashes
}

function Compare-BaselineDirtyHashes
{
    param(
        [Parameter(Mandatory = $true)][string]$ProjectRoot,
        [Parameter(Mandatory = $true)]$BaselineHashes,
        [Parameter(Mandatory = $true)][string[]]$AllowedPaths
    )

    $Changed = [System.Collections.Generic.List[string]]::new()
    foreach ($Property in @($BaselineHashes.PSObject.Properties))
    {
        $Path = [string]$Property.Name
        if ($AllowedPaths -contains $Path)
        {
            continue
        }
        $Current = Get-FileHashes -ProjectRoot $ProjectRoot -RelativePaths @($Path)
        if ([string]$Property.Value -ne [string]$Current[$Path])
        {
            $Changed.Add($Path)
        }
    }

    return @($Changed)
}

function Start-WacomUnrealMcp
{
    param(
        [Parameter(Mandatory = $true)]$RoleConfig,
        [Parameter(Mandatory = $true)][string]$StateRoot,
        [Parameter(Mandatory = $true)][string]$ProjectRoot,
        [Parameter(Mandatory = $true)][string]$ExpectedBranch,
        [Parameter(Mandatory = $true)][string]$EditorExecutable,
        [int]$TimeoutSeconds,
        [switch]$AllowDirty
    )

    $GitContext = Get-GitContext -ProjectRoot $ProjectRoot -ExpectedBranch $ExpectedBranch
    if ($RoleConfig.name -eq "main" -and $GitContext.Branch -ne "main")
    {
        throw "The main endpoint may only target branch 'main'."
    }
    if (-not $AllowDirty -and $GitContext.DirtyPaths.Count -gt 0)
    {
        throw "Worktree is dirty. Handle the changes or rerun with -AllowDirty after explicitly assigning ownership."
    }
    Assert-EditorModuleBinariesPresent -ProjectRoot $GitContext.ProjectRoot

    $EditorPath = Get-CanonicalPath -Path $EditorExecutable -MustExist
    $SessionPath = Get-StateFilePath -StateRoot $StateRoot -Kind Session -Role $RoleConfig.name
    $WriterPath = Get-StateFilePath -StateRoot $StateRoot -Kind Writer -Role $RoleConfig.name
    if (Test-Path -LiteralPath $WriterPath -PathType Leaf)
    {
        throw "A writer lease still exists for role '$($RoleConfig.name)'. Release it before starting another Editor lifecycle."
    }
    if (Test-Path -LiteralPath $SessionPath -PathType Leaf)
    {
        $ExistingSession = Read-JsonFile -Path $SessionPath
        if (Test-SessionProcessAlive -Session $ExistingSession)
        {
            throw "A live Editor MCP session already exists for role '$($RoleConfig.name)'."
        }
        Move-StateFileToArchive `
            -Path $SessionPath `
            -StateRoot $StateRoot `
            -Reason "stale-session" | Out-Null
    }

    $PortOwners = @(Get-PortOwnerIds -Port ([int]$RoleConfig.port))
    if ($PortOwners.Count -gt 0)
    {
        throw "Port $($RoleConfig.port) is already occupied by PID(s): $($PortOwners -join ', ')"
    }
    $ExistingEditors = @(Get-UnrealEditorProcessesForProject -ProjectFile $GitContext.ProjectFile)
    if ($ExistingEditors.Count -gt 0)
    {
        throw "This worktree already has an Editor process. Close it and restart through the MCP launcher. PID(s): $($ExistingEditors.ProcessId -join ', ')"
    }

    $LfsOutput = @(& git -C $GitContext.ProjectRoot lfs fsck 2>&1)
    if ($LASTEXITCODE -ne 0)
    {
        throw "Git LFS fsck failed before Editor startup: $($LfsOutput -join [Environment]::NewLine)"
    }

    $SessionId = [Guid]::NewGuid().ToString("D")
    $Arguments = @(
        '"' + $GitContext.ProjectFile + '"',
        "-ModelContextProtocolStartServer",
        "-ModelContextProtocolPort=$($RoleConfig.port)",
        "-WacomMcpSessionId=$SessionId")
    $EditorProcess = Start-Process `
        -FilePath $EditorPath `
        -ArgumentList $Arguments `
        -PassThru
    $Deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    $ProcessRecord = $null
    $Health = $null
    do
    {
        if ($EditorProcess.HasExited)
        {
            throw "Editor exited before Unreal MCP became ready. Exit code: $($EditorProcess.ExitCode)"
        }
        $ProcessRecord = Get-ProcessRecord -ProcessId $EditorProcess.Id
        $PortOwners = @(Get-PortOwnerIds -Port ([int]$RoleConfig.port))
        if (
            $ProcessRecord -and
            $PortOwners.Count -gt 0 -and
            @($PortOwners | Where-Object { $_ -ne $EditorProcess.Id }).Count -eq 0)
        {
            try
            {
                $Health = Invoke-McpHealthCheck `
                    -Port ([int]$RoleConfig.port) `
                    -TimeoutSeconds ([Math]::Min(45, [Math]::Max(30, $TimeoutSeconds)))
                break
            }
            catch
            {
                $Health = $null
            }
        }
        Start-Sleep -Milliseconds 500
    }
    while ([DateTime]::UtcNow -lt $Deadline)

    if (-not $ProcessRecord -or -not $Health)
    {
        throw "Editor PID $($EditorProcess.Id) did not expose a healthy Unreal MCP server within $TimeoutSeconds seconds. The Editor was left running for inspection."
    }

    if (
        -not (Test-CommandLineContainsPath -CommandLine $ProcessRecord.CommandLine -Path $GitContext.ProjectFile) -or
        -not $ProcessRecord.CommandLine.Contains("-WacomMcpSessionId=$SessionId"))
    {
        throw "Started Editor process identity does not match the requested project/session."
    }

    $Session = [ordered]@{
        schemaVersion = 1
        role = [string]$RoleConfig.name
        codexEndpoint = [string]$RoleConfig.codexEndpoint
        port = [int]$RoleConfig.port
        sessionId = $SessionId
        editorPid = $EditorProcess.Id
        editorProcessStartTimeUtc = $ProcessRecord.StartTimeUtc
        editorProcessStartTimeUtcTicks = $ProcessRecord.StartTimeUtcTicks
        editorExecutable = $EditorPath
        projectRoot = $GitContext.ProjectRoot
        projectFile = $GitContext.ProjectFile
        branch = $GitContext.Branch
        head = $GitContext.Head
        initialDirtyPaths = @($GitContext.DirtyPaths)
        startedAtUtc = [DateTime]::UtcNow.ToString("o")
        toolCount = $Health.ToolCount
    }
    Write-NewJsonFile -Path $SessionPath -Value $Session

    return [pscustomobject]$Session
}

function Get-WacomUnrealMcpStatus
{
    param(
        [Parameter(Mandatory = $true)]$RoleConfig,
        [Parameter(Mandatory = $true)][string]$StateRoot
    )

    $SessionPath = Get-StateFilePath -StateRoot $StateRoot -Kind Session -Role $RoleConfig.name
    $WriterPath = Get-StateFilePath -StateRoot $StateRoot -Kind Writer -Role $RoleConfig.name
    $PortOwners = @(Get-PortOwnerIds -Port ([int]$RoleConfig.port))
    if (-not (Test-Path -LiteralPath $SessionPath -PathType Leaf))
    {
        return [pscustomobject]@{
            Role = $RoleConfig.name
            CodexEndpoint = $RoleConfig.codexEndpoint
            Port = $RoleConfig.port
            State = if ($PortOwners.Count -gt 0) { "UntrackedPortOwner" } else { "Offline" }
            PortOwners = $PortOwners
            WriterLease = Test-Path -LiteralPath $WriterPath -PathType Leaf
        }
    }

    $Session = Read-JsonFile -Path $SessionPath
    return [pscustomobject]@{
        Role = $RoleConfig.name
        CodexEndpoint = $RoleConfig.codexEndpoint
        Port = $RoleConfig.port
        State = if (Test-SessionProcessAlive -Session $Session) { "Recorded" } else { "Stale" }
        EditorPid = $Session.editorPid
        SessionId = $Session.sessionId
        ProjectRoot = $Session.projectRoot
        Branch = $Session.branch
        Head = $Session.head
        PortOwners = $PortOwners
        WriterLease = Test-Path -LiteralPath $WriterPath -PathType Leaf
    }
}

function Assert-WacomUnrealMcpClosedForBuild
{
    param(
        [Parameter(Mandatory = $true)]$RoleConfig,
        [Parameter(Mandatory = $true)][string]$StateRoot,
        [Parameter(Mandatory = $true)][string]$ProjectRoot,
        [Parameter(Mandatory = $true)][string]$ExpectedBranch
    )

    $GitContext = Get-GitContext -ProjectRoot $ProjectRoot -ExpectedBranch $ExpectedBranch
    $WriterPath = Get-StateFilePath -StateRoot $StateRoot -Kind Writer -Role $RoleConfig.name
    if (Test-Path -LiteralPath $WriterPath -PathType Leaf)
    {
        throw "Writer lease still exists for role '$($RoleConfig.name)'. Run ReleaseWriter before compiling."
    }

    $SessionPath = Get-StateFilePath -StateRoot $StateRoot -Kind Session -Role $RoleConfig.name
    if (Test-Path -LiteralPath $SessionPath -PathType Leaf)
    {
        $Session = Read-JsonFile -Path $SessionPath
        if (Test-SessionProcessAlive -Session $Session)
        {
            throw "Editor PID $($Session.editorPid) is still running for role '$($RoleConfig.name)'."
        }
        Move-StateFileToArchive `
            -Path $SessionPath `
            -StateRoot $StateRoot `
            -Reason "closed-session" | Out-Null
    }

    $PortOwners = @(Get-PortOwnerIds -Port ([int]$RoleConfig.port))
    if ($PortOwners.Count -gt 0)
    {
        throw "Role port $($RoleConfig.port) is still listening. Owner PID(s): $($PortOwners -join ', ')"
    }
    $Editors = @(Get-UnrealEditorProcessesForProject -ProjectFile $GitContext.ProjectFile)
    if ($Editors.Count -gt 0)
    {
        throw "An Editor still targets this worktree. PID(s): $($Editors.ProcessId -join ', ')"
    }

    return [pscustomobject]@{
        Role = $RoleConfig.name
        ProjectRoot = $GitContext.ProjectRoot
        Branch = $GitContext.Branch
        Head = $GitContext.Head
        ReadyForBuild = $true
    }
}

function Acquire-WacomUnrealMcpWriter
{
    param(
        [Parameter(Mandatory = $true)]$RoleConfig,
        [Parameter(Mandatory = $true)][string]$StateRoot,
        [Parameter(Mandatory = $true)][string]$ProjectRoot,
        [Parameter(Mandatory = $true)][string]$ExpectedBranch,
        [Parameter(Mandatory = $true)][string]$ThreadId,
        [Parameter(Mandatory = $true)][string[]]$Packages,
        [switch]$AllowProtectedRoleWrite
    )

    if ($RoleConfig.defaultAccess -eq "read-only" -and -not $AllowProtectedRoleWrite)
    {
        throw "Role '$($RoleConfig.name)' is read-only by default. Explicit user authorization and -AllowProtectedRoleWrite are required."
    }
    if ([string]::IsNullOrWhiteSpace($ThreadId))
    {
        throw "-ThreadId is required to acquire a writer lease."
    }
    $NormalizedPackages = @($Packages | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Sort-Object -Unique)
    if ($NormalizedPackages.Count -eq 0)
    {
        throw "At least one /Game/... package is required to acquire a writer lease."
    }

    $Identity = Assert-SessionIdentity `
        -RoleConfig $RoleConfig `
        -StateRoot $StateRoot `
        -ProjectRoot $ProjectRoot `
        -ExpectedBranch $ExpectedBranch
    $AllowedPaths = [System.Collections.Generic.List[string]]::new()
    foreach ($Package in $NormalizedPackages)
    {
        foreach ($Path in @(Get-RelativePackageFileCandidates -Package $Package))
        {
            $AllowedPaths.Add($Path)
        }
    }
    $AllowedPathsValue = @($AllowedPaths | Sort-Object -Unique)

    $LfsFilterMap = Get-LfsFilterMap `
        -ProjectRoot ([string]$Identity.Session.projectRoot) `
        -RelativePaths $AllowedPathsValue
    $NonLfsPaths = @(
        $LfsFilterMap.GetEnumerator() |
            Where-Object { [string]$_.Value -ne "lfs" } |
            ForEach-Object { [string]$_.Key })
    if ($NonLfsPaths.Count -gt 0)
    {
        throw "Writer allowlist contains Unreal packages that are not Git LFS tracked: $($NonLfsPaths -join ', ')"
    }

    $DirtyPaths = @($Identity.Git.DirtyPaths)
    $DirtyAllowedPaths = @($DirtyPaths | Where-Object { $AllowedPathsValue -contains $_ })
    if ($DirtyAllowedPaths.Count -gt 0)
    {
        throw "Allowed package files are already dirty and cannot be attributed to this MCP writer: $($DirtyAllowedPaths -join ', ')"
    }

    $WriterPath = Get-StateFilePath -StateRoot $StateRoot -Kind Writer -Role $RoleConfig.name
    if (Test-Path -LiteralPath $WriterPath -PathType Leaf)
    {
        $Existing = Read-JsonFile -Path $WriterPath
        throw "Writer role '$($RoleConfig.name)' is already leased by thread '$($Existing.threadId)' for session '$($Existing.sessionId)'."
    }

    $Lease = [ordered]@{
        schemaVersion = 1
        role = [string]$RoleConfig.name
        codexEndpoint = [string]$RoleConfig.codexEndpoint
        port = [int]$RoleConfig.port
        sessionId = [string]$Identity.Session.sessionId
        editorPid = [int]$Identity.Session.editorPid
        editorProcessStartTimeUtc = ([DateTime]$Identity.Session.editorProcessStartTimeUtc).ToUniversalTime().ToString("o")
        editorProcessStartTimeUtcTicks = Get-RecordedProcessStartTimeUtcTicks -Record $Identity.Session
        threadId = $ThreadId
        projectRoot = [string]$Identity.Session.projectRoot
        branch = [string]$Identity.Session.branch
        head = [string]$Identity.Session.head
        packages = $NormalizedPackages
        allowedRelativePaths = $AllowedPathsValue
        lfsFilters = $LfsFilterMap
        baselineDirtyPaths = $DirtyPaths
        baselineDirtyHashes = Get-FileHashes `
            -ProjectRoot ([string]$Identity.Session.projectRoot) `
            -RelativePaths $DirtyPaths
        baselineAllowedHashes = Get-FileHashes `
            -ProjectRoot ([string]$Identity.Session.projectRoot) `
            -RelativePaths $AllowedPathsValue
        acquiredAtUtc = [DateTime]::UtcNow.ToString("o")
    }
    Write-NewJsonFile -Path $WriterPath -Value $Lease
    return [pscustomobject]$Lease
}

function Release-WacomUnrealMcpWriter
{
    param(
        [Parameter(Mandatory = $true)]$RoleConfig,
        [Parameter(Mandatory = $true)][string]$StateRoot,
        [Parameter(Mandatory = $true)][string]$ThreadId
    )

    if ([string]::IsNullOrWhiteSpace($ThreadId))
    {
        throw "-ThreadId is required to release a writer lease."
    }
    $WriterPath = Get-StateFilePath -StateRoot $StateRoot -Kind Writer -Role $RoleConfig.name
    if (-not (Test-Path -LiteralPath $WriterPath -PathType Leaf))
    {
        throw "No writer lease exists for role '$($RoleConfig.name)'."
    }
    $Lease = Read-JsonFile -Path $WriterPath
    if (
        [int]$Lease.schemaVersion -ne 1 -or
        [string]$Lease.role -ne [string]$RoleConfig.name -or
        [string]$Lease.codexEndpoint -ne [string]$RoleConfig.codexEndpoint)
    {
        throw "Writer lease metadata does not match the repository endpoint config."
    }
    if ([string]$Lease.threadId -ne $ThreadId)
    {
        throw "Writer lease belongs to thread '$($Lease.threadId)', not '$ThreadId'."
    }

    $ProjectRoot = [string]$Lease.projectRoot
    $CurrentGit = Get-GitContext -ProjectRoot $ProjectRoot -ExpectedBranch ([string]$Lease.branch)
    if ($CurrentGit.Head -ne [string]$Lease.head)
    {
        throw "Git HEAD changed during the writer lease. Preserve the lease and audit the worktree before continuing."
    }

    $AllowedPaths = @($Lease.allowedRelativePaths | ForEach-Object { [string]$_ })
    $BaselineDirty = @($Lease.baselineDirtyPaths | ForEach-Object { [string]$_ })
    $NewDirtyPaths = @($CurrentGit.DirtyPaths | Where-Object { $BaselineDirty -notcontains $_ })
    $OutOfScopeNewPaths = @($NewDirtyPaths | Where-Object { $AllowedPaths -notcontains $_ })
    $ChangedBaselinePaths = @(
        Compare-BaselineDirtyHashes `
            -ProjectRoot $ProjectRoot `
            -BaselineHashes $Lease.baselineDirtyHashes `
            -AllowedPaths $AllowedPaths)
    if ($OutOfScopeNewPaths.Count -gt 0 -or $ChangedBaselinePaths.Count -gt 0)
    {
        $Details = @(
            if ($OutOfScopeNewPaths.Count -gt 0) {
                "new out-of-scope paths: $($OutOfScopeNewPaths -join ', ')"
            }
            if ($ChangedBaselinePaths.Count -gt 0) {
                "pre-existing dirty paths changed: $($ChangedBaselinePaths -join ', ')"
            }) -join "; "
        throw "Writer lease cannot be released because the worktree escaped its package allowlist ($Details). No files were cleaned and the lease was preserved."
    }

    $LfsFsck = @(& git -C $ProjectRoot lfs fsck 2>&1)
    if ($LASTEXITCODE -ne 0)
    {
        throw "Git LFS fsck failed while releasing the writer lease. The lease was preserved: $($LfsFsck -join [Environment]::NewLine)"
    }
    $LfsStatus = @(Invoke-Git -ProjectRoot $ProjectRoot -Arguments @("lfs", "status"))
    $GitStatus = @(Get-GitStatusLines -ProjectRoot $ProjectRoot)

    $Audit = [ordered]@{
        schemaVersion = 1
        role = [string]$Lease.role
        codexEndpoint = [string]$Lease.codexEndpoint
        sessionId = [string]$Lease.sessionId
        editorPid = [int]$Lease.editorPid
        threadId = [string]$Lease.threadId
        projectRoot = $ProjectRoot
        branch = [string]$Lease.branch
        head = [string]$Lease.head
        packages = @($Lease.packages)
        allowedRelativePaths = $AllowedPaths
        baselineDirtyPaths = $BaselineDirty
        finalDirtyPaths = @($CurrentGit.DirtyPaths)
        newDirtyPaths = $NewDirtyPaths
        gitStatus = $GitStatus
        gitLfsStatus = $LfsStatus
        gitLfsFsck = $LfsFsck
        lfsFilters = $Lease.lfsFilters
        baselineAllowedHashes = $Lease.baselineAllowedHashes
        finalAllowedHashes = Get-FileHashes `
            -ProjectRoot $ProjectRoot `
            -RelativePaths $AllowedPaths
        acquiredAtUtc = [string]$Lease.acquiredAtUtc
        releasedAtUtc = [DateTime]::UtcNow.ToString("o")
    }
    $Timestamp = [DateTime]::UtcNow.ToString("yyyyMMdd-HHmmss-fff")
    $AuditPath = Join-Path (Join-Path $StateRoot "Audits") "$Timestamp-$($RoleConfig.name)-$($Lease.sessionId).json"
    Write-NewJsonFile -Path $AuditPath -Value $Audit
    Remove-Item -LiteralPath $WriterPath

    return [pscustomobject]@{
        Role = $RoleConfig.name
        SessionId = $Lease.sessionId
        ThreadId = $Lease.threadId
        AuditPath = $AuditPath
        NewDirtyPaths = $NewDirtyPaths
        Released = $true
    }
}

function Archive-WacomUnrealMcpStaleWriter
{
    param(
        [Parameter(Mandatory = $true)]$RoleConfig,
        [Parameter(Mandatory = $true)][string]$StateRoot,
        [Parameter(Mandatory = $true)][string]$ThreadId,
        [Parameter(Mandatory = $true)][string]$Reason,
        [switch]$ConfirmStaleWriterArchive
    )

    if (-not $ConfirmStaleWriterArchive)
    {
        throw "ArchiveStaleWriter requires -ConfirmStaleWriterArchive after manual asset and Git audit."
    }
    if ([string]::IsNullOrWhiteSpace($Reason))
    {
        throw "ArchiveStaleWriter requires a non-empty -Reason."
    }

    $WriterPath = Get-StateFilePath -StateRoot $StateRoot -Kind Writer -Role $RoleConfig.name
    if (-not (Test-Path -LiteralPath $WriterPath -PathType Leaf))
    {
        throw "No writer lease exists for role '$($RoleConfig.name)'."
    }
    $Lease = Read-JsonFile -Path $WriterPath
    if (
        [int]$Lease.schemaVersion -ne 1 -or
        [string]$Lease.role -ne [string]$RoleConfig.name -or
        [string]$Lease.codexEndpoint -ne [string]$RoleConfig.codexEndpoint)
    {
        throw "Writer lease metadata does not match the repository endpoint config."
    }
    if ([string]$Lease.threadId -ne $ThreadId)
    {
        throw "Writer lease belongs to thread '$($Lease.threadId)', not '$ThreadId'."
    }

    $RecordedProcess = Get-ProcessRecord -ProcessId ([int]$Lease.editorPid)
    if (
        $RecordedProcess -and
        $RecordedProcess.StartTimeUtcTicks -eq (Get-RecordedProcessStartTimeUtcTicks -Record $Lease))
    {
        throw "Recorded Editor PID $($Lease.editorPid) is still running. Close the Editor before archiving a stale writer lease."
    }
    $PortOwners = @(Get-PortOwnerIds -Port ([int]$RoleConfig.port))
    if ($PortOwners.Count -gt 0)
    {
        throw "Role port $($RoleConfig.port) is still listening. Owner PID(s): $($PortOwners -join ', ')"
    }

    $ProjectRoot = [string]$Lease.projectRoot
    $GitContext = Get-GitContext -ProjectRoot $ProjectRoot
    $Editors = @(Get-UnrealEditorProcessesForProject -ProjectFile $GitContext.ProjectFile)
    if ($Editors.Count -gt 0)
    {
        throw "An Editor still targets the writer worktree. PID(s): $($Editors.ProcessId -join ', ')"
    }

    $Timestamp = [DateTime]::UtcNow.ToString("yyyyMMdd-HHmmss-fff")
    $AuditPath = Join-Path (Join-Path $StateRoot "Audits") "$Timestamp-$($RoleConfig.name)-stale-writer.json"
    $Audit = [ordered]@{
        schemaVersion = 1
        disposition = "stale-writer-archived"
        reason = $Reason.Trim()
        archivedAtUtc = [DateTime]::UtcNow.ToString("o")
        currentBranch = $GitContext.Branch
        currentHead = $GitContext.Head
        currentDirtyPaths = @($GitContext.DirtyPaths)
        gitStatus = @(Get-GitStatusLines -ProjectRoot $ProjectRoot)
        gitLfsStatus = @(Invoke-Git -ProjectRoot $ProjectRoot -Arguments @("lfs", "status"))
        lease = $Lease
    }
    Write-NewJsonFile -Path $AuditPath -Value $Audit
    $ArchivePath = Move-StateFileToArchive `
        -Path $WriterPath `
        -StateRoot $StateRoot `
        -Reason "stale-writer"

    return [pscustomobject]@{
        Role = $RoleConfig.name
        ThreadId = $ThreadId
        AuditPath = $AuditPath
        ArchivePath = $ArchivePath
        Archived = $true
    }
}

function Write-CodexConfigSnippet
{
    param([Parameter(Mandatory = $true)]$Config)

    foreach ($Role in $Config.roles)
    {
        Write-Output "[mcp_servers.$($Role.codexEndpoint)]"
        Write-Output "url = `"http://127.0.0.1:$($Role.port)/mcp`""
        Write-Output "required = false"
        Write-Output ""
    }
}

function Invoke-WacomUnrealMcp
{
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
        [AllowNull()][string]$Role,
        [AllowNull()][string]$ProjectRoot,
        [AllowNull()][string]$ExpectedBranch,
        [AllowNull()][string]$ThreadId,
        [AllowNull()][string]$Reason,
        [string[]]$Packages = @(),
        [AllowNull()][string]$EditorExecutable,
        [AllowNull()][string]$LocalStateRoot,
        [int]$TimeoutSeconds = 120,
        [switch]$AllowDirty,
        [switch]$AllowProtectedRoleWrite,
        [switch]$ConfirmStaleWriterArchive,
        [Parameter(Mandatory = $true)][string]$EndpointConfigPath
    )

    $Config = Read-EndpointConfig -Path $EndpointConfigPath
    if ($Action -eq "PrintCodexConfig")
    {
        Write-CodexConfigSnippet -Config $Config
        return
    }

    Assert-RoleProvided -Role $Role
    $RoleConfig = Get-RoleConfig -Config $Config -Role $Role
    $StateRoot = if ([string]::IsNullOrWhiteSpace($LocalStateRoot)) {
        Get-WacomUnrealMcpDefaultStateRoot
    } else {
        Get-CanonicalPath -Path $LocalStateRoot
    }
    Initialize-StateDirectories -StateRoot $StateRoot

    switch ($Action)
    {
        "Start"
        {
            if (
                [string]::IsNullOrWhiteSpace($ProjectRoot) -or
                [string]::IsNullOrWhiteSpace($ExpectedBranch))
            {
                throw "Start requires -ProjectRoot and -ExpectedBranch."
            }
            $EditorPath = if ([string]::IsNullOrWhiteSpace($EditorExecutable)) {
                Get-WacomUnrealMcpDefaultEditorExecutable
            } else {
                $EditorExecutable
            }
            return Start-WacomUnrealMcp `
                -RoleConfig $RoleConfig `
                -StateRoot $StateRoot `
                -ProjectRoot $ProjectRoot `
                -ExpectedBranch $ExpectedBranch `
                -EditorExecutable $EditorPath `
                -TimeoutSeconds $TimeoutSeconds `
                -AllowDirty:$AllowDirty
        }
        "Status"
        {
            return Get-WacomUnrealMcpStatus `
                -RoleConfig $RoleConfig `
                -StateRoot $StateRoot
        }
        "AssertReady"
        {
            return Assert-SessionIdentity `
                -RoleConfig $RoleConfig `
                -StateRoot $StateRoot `
                -ProjectRoot $ProjectRoot `
                -ExpectedBranch $ExpectedBranch
        }
        "AssertClosedForBuild"
        {
            if (
                [string]::IsNullOrWhiteSpace($ProjectRoot) -or
                [string]::IsNullOrWhiteSpace($ExpectedBranch))
            {
                throw "AssertClosedForBuild requires -ProjectRoot and -ExpectedBranch."
            }
            return Assert-WacomUnrealMcpClosedForBuild `
                -RoleConfig $RoleConfig `
                -StateRoot $StateRoot `
                -ProjectRoot $ProjectRoot `
                -ExpectedBranch $ExpectedBranch
        }
        "AcquireWriter"
        {
            if (
                [string]::IsNullOrWhiteSpace($ProjectRoot) -or
                [string]::IsNullOrWhiteSpace($ExpectedBranch) -or
                [string]::IsNullOrWhiteSpace($ThreadId))
            {
                throw "AcquireWriter requires -ProjectRoot, -ExpectedBranch, and -ThreadId."
            }
            return Acquire-WacomUnrealMcpWriter `
                -RoleConfig $RoleConfig `
                -StateRoot $StateRoot `
                -ProjectRoot $ProjectRoot `
                -ExpectedBranch $ExpectedBranch `
                -ThreadId $ThreadId `
                -Packages $Packages `
                -AllowProtectedRoleWrite:$AllowProtectedRoleWrite
        }
        "ReleaseWriter"
        {
            if ([string]::IsNullOrWhiteSpace($ThreadId))
            {
                throw "ReleaseWriter requires -ThreadId."
            }
            return Release-WacomUnrealMcpWriter `
                -RoleConfig $RoleConfig `
                -StateRoot $StateRoot `
                -ThreadId $ThreadId
        }
        "ArchiveStaleWriter"
        {
            if (
                [string]::IsNullOrWhiteSpace($ThreadId) -or
                [string]::IsNullOrWhiteSpace($Reason))
            {
                throw "ArchiveStaleWriter requires -ThreadId and -Reason."
            }
            return Archive-WacomUnrealMcpStaleWriter `
                -RoleConfig $RoleConfig `
                -StateRoot $StateRoot `
                -ThreadId $ThreadId `
                -Reason $Reason `
                -ConfirmStaleWriterArchive:$ConfirmStaleWriterArchive
        }
    }
}

Export-ModuleMember -Function Invoke-WacomUnrealMcp
