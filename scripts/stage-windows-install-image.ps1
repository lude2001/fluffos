param(
    [Parameter(Mandatory = $true)]
    [string]$RootDir,
    [Parameter(Mandatory = $true)]
    [string]$DistDir,
    [Parameter(Mandatory = $true)]
    [string]$InstallImageDir
)

$ErrorActionPreference = "Stop"

function Sync-Directory {
    param(
        [Parameter(Mandatory = $true)]
        [string]$SourceDir,
        [Parameter(Mandatory = $true)]
        [string]$TargetDir
    )

    if (Test-Path -LiteralPath $TargetDir) {
        Remove-Item -Recurse -Force $TargetDir
    }

    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $TargetDir) | Out-Null
    Copy-Item -Recurse -Force $SourceDir $TargetDir
}

$resolvedRootDir = (Resolve-Path -LiteralPath $RootDir).Path
$resolvedDistDir = (Resolve-Path -LiteralPath $DistDir).Path

$binDir = Join-Path $InstallImageDir "bin"
$libexecDir = Join-Path $InstallImageDir "libexec\fluffos"
$shareDir = Join-Path $InstallImageDir "share\fluffos"
$runtimeDir = Join-Path $InstallImageDir "runtime"

if (Test-Path -LiteralPath $InstallImageDir) {
    Remove-Item -Recurse -Force $InstallImageDir
}

New-Item -ItemType Directory -Force -Path $binDir | Out-Null
New-Item -ItemType Directory -Force -Path $libexecDir | Out-Null
New-Item -ItemType Directory -Force -Path $shareDir | Out-Null
New-Item -ItemType Directory -Force -Path $runtimeDir | Out-Null

$driverPath = Join-Path $resolvedDistDir "driver.exe"
$lpcprjPath = Join-Path $resolvedDistDir "lpcprj.exe"
$lpccpPath = Join-Path $resolvedDistDir "lpccp.exe"

if (-not (Test-Path -LiteralPath $driverPath)) {
    throw "Missing driver.exe at $driverPath"
}
if (-not (Test-Path -LiteralPath $lpccpPath)) {
    throw "Missing lpccp.exe at $lpccpPath"
}

Copy-Item -Force $driverPath (Join-Path $libexecDir "driver.exe")
if (Test-Path -LiteralPath $lpcprjPath) {
    Copy-Item -Force $lpcprjPath (Join-Path $binDir "lpcprj.exe")
} else {
    Copy-Item -Force $driverPath (Join-Path $binDir "lpcprj.exe")
}
Copy-Item -Force $lpccpPath (Join-Path $binDir "lpccp.exe")

$runtimeDlls = Get-ChildItem -LiteralPath $resolvedDistDir -Filter *.dll -File
foreach ($dll in $runtimeDlls) {
    Copy-Item -Force $dll.FullName (Join-Path $runtimeDir $dll.Name)
    Copy-Item -Force $dll.FullName (Join-Path $binDir $dll.Name)
    Copy-Item -Force $dll.FullName (Join-Path $libexecDir $dll.Name)
}

Sync-Directory -SourceDir (Join-Path $resolvedDistDir "include") -TargetDir (Join-Path $shareDir "include")
Sync-Directory -SourceDir (Join-Path $resolvedDistDir "std") -TargetDir (Join-Path $shareDir "std")
Sync-Directory -SourceDir (Join-Path $resolvedDistDir "www") -TargetDir (Join-Path $shareDir "www")

$licenseCandidates = @(
    (Join-Path $resolvedRootDir "Copyright"),
    (Join-Path $resolvedRootDir "README.md")
)
if (Test-Path -LiteralPath $licenseCandidates[0]) {
    Copy-Item -Force $licenseCandidates[0] (Join-Path $InstallImageDir "LICENSE.txt")
}
if (Test-Path -LiteralPath $licenseCandidates[1]) {
    Copy-Item -Force $licenseCandidates[1] (Join-Path $InstallImageDir "README.txt")
}
