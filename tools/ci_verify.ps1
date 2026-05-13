param(
    [string]$Preset = "ci"
    ,[switch]$VerifyGoalLock = $false
)

$ErrorActionPreference = "Stop"

function Invoke-CheckedNative {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Command,
        [Parameter(ValueFromRemainingArguments = $true)]
        [string[]]$Arguments
    )

    & $Command @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code ${LASTEXITCODE}: $Command $($Arguments -join ' ')"
    }
}

if ($VerifyGoalLock) {
    Invoke-CheckedNative python (Join-Path $PSScriptRoot "verify_goal_guard.py") --model 5.3-Codex-Spark
}

$ProjectRoot = Split-Path $PSScriptRoot -Parent
Invoke-CheckedNative python (Join-Path $PSScriptRoot "validate_assets.py") (Join-Path $ProjectRoot "data\assets")
Invoke-CheckedNative python (Join-Path $PSScriptRoot "validate_world_contract.py") (Join-Path $ProjectRoot "data") --asset-root (Join-Path $ProjectRoot "data\assets")
Invoke-CheckedNative cmake --preset $Preset
Invoke-CheckedNative cmake --build --preset $Preset
Invoke-CheckedNative ctest --preset $Preset
Invoke-CheckedNative dotnet run --project (Join-Path $PSScriptRoot "BlokTools\BlokTools.Tests\BlokTools.Tests.csproj")
Invoke-CheckedNative dotnet build (Join-Path $PSScriptRoot "BlokTools\BlokTools.App\BlokTools.App.csproj")
