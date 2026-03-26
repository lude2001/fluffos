param(
    [Parameter(Mandatory = $true)]
    [string]$InstallerScriptPath
)

$ErrorActionPreference = "Stop"
$scriptText = Get-Content -Raw -LiteralPath $InstallerScriptPath

$requiredPatterns = @(
    'UsePreviousAppDir=no',
    'UsePreviousPrivileges=no'
)

$missing = @()
foreach ($pattern in $requiredPatterns) {
    if ($scriptText -notmatch [regex]::Escape($pattern)) {
        $missing += $pattern
    }
}

if ($missing.Count -gt 0) {
    throw "Installer script is missing required settings: $($missing -join ', ')"
}

[pscustomobject]@{
    InstallerScriptPath = (Resolve-Path -LiteralPath $InstallerScriptPath).Path
    VerifiedSettings = $requiredPatterns
}
