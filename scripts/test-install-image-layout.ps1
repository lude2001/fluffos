param(
    [Parameter(Mandatory = $true)]
    [string]$InstallImageDir
)

$ErrorActionPreference = "Stop"

$root = (Resolve-Path -LiteralPath $InstallImageDir -ErrorAction Stop).Path
$requiredPaths = @(
    "bin/lpcprj.exe",
    "bin/lpccp.exe",
    "libexec/fluffos/driver.exe",
    "share/fluffos/include",
    "share/fluffos/std",
    "share/fluffos/www"
)

$missing = @()
foreach ($relativePath in $requiredPaths) {
    $targetPath = Join-Path $root $relativePath
    if (-not (Test-Path -LiteralPath $targetPath)) {
        $missing += $relativePath
    }
}

if ($missing.Count -gt 0) {
    throw "Install image is missing required paths: $($missing -join ', ')"
}

[pscustomobject]@{
    InstallImageDir = $root
    VerifiedPaths = $requiredPaths
}
