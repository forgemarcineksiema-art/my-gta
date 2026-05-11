param(
    [string]$Preset = "ci"
    ,[switch]$VerifyGoalLock = $false
)

$ErrorActionPreference = "Stop"

if ($VerifyGoalLock) {
    python (Join-Path $PSScriptRoot "verify_goal_guard.py") --model 5.3-Codex-Spark
}

$ProjectRoot = Split-Path $PSScriptRoot -Parent
python (Join-Path $PSScriptRoot "validate_assets.py") (Join-Path $ProjectRoot "data\assets")
python (Join-Path $PSScriptRoot "validate_world_contract.py") (Join-Path $ProjectRoot "data") --asset-root (Join-Path $ProjectRoot "data\assets")
cmake --preset $Preset
cmake --build --preset $Preset
ctest --preset $Preset
dotnet run --project (Join-Path $PSScriptRoot "BlokTools\BlokTools.Tests\BlokTools.Tests.csproj")
dotnet build (Join-Path $PSScriptRoot "BlokTools\BlokTools.App\BlokTools.App.csproj")
