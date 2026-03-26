param(
    [Parameter(Mandatory = $true)]
    [string]$InstallImageDir,
    [Parameter(Mandatory = $true)]
    [string]$OutputDir,
    [Parameter(Mandatory = $true)]
    [string]$Version,
    [string]$InnoSetupCompilerPath
)

$ErrorActionPreference = "Stop"
$resolvedInstallImageDir = (Resolve-Path -LiteralPath $InstallImageDir -ErrorAction Stop).Path

if (-not (Test-Path -LiteralPath $OutputDir)) {
    New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
}
$resolvedOutputDir = (Resolve-Path -LiteralPath $OutputDir).Path
$scriptPath = Join-Path $PSScriptRoot "..\packaging\windows\fluffos.iss"
$resolvedScriptPath = (Resolve-Path -LiteralPath $scriptPath -ErrorAction Stop).Path

$compilerCandidates = @()
if ($InnoSetupCompilerPath) {
    $compilerCandidates += $InnoSetupCompilerPath
}

$discoveredCompiler = (Get-Command ISCC.exe -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty Source)
if ($discoveredCompiler) {
    $compilerCandidates += $discoveredCompiler
}

$compilerCandidates += @(
    "C:\Program Files (x86)\Inno Setup 6\ISCC.exe",
    "C:\Program Files\Inno Setup 6\ISCC.exe"
)

$resolvedCompilerPath = $null
foreach ($candidate in $compilerCandidates) {
    if ($candidate -and (Test-Path -LiteralPath $candidate)) {
        $resolvedCompilerPath = (Resolve-Path -LiteralPath $candidate).Path
        break
    }
}

if (-not $resolvedCompilerPath) {
    throw "Unable to find ISCC.exe. Install Inno Setup 6 or pass -InnoSetupCompilerPath explicitly."
}

$arguments = @(
    "/DAppVersion=$Version",
    "/DSourceDir=$resolvedInstallImageDir",
    "/DOutputDir=$resolvedOutputDir",
    $resolvedScriptPath
)

& $resolvedCompilerPath @arguments
if ($LASTEXITCODE -ne 0) {
    throw "Inno Setup compiler failed with exit code $LASTEXITCODE"
}

[pscustomobject]@{
    Compiler = $resolvedCompilerPath
    Installer = Join-Path $resolvedOutputDir "fluffos-$Version-windows-x86_64-installer.exe"
}
