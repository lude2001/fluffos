param(
    [Parameter(Mandatory = $true)]
    [string]$InstallerScriptPath
)

$ErrorActionPreference = "Stop"
$scriptText = Get-Content -Raw -LiteralPath $InstallerScriptPath
$scriptDir = Split-Path -Parent (Resolve-Path -LiteralPath $InstallerScriptPath)

$requiredPatterns = @(
    'LanguageDetectionMethod=uilanguage',
    'ShowLanguageDialog=auto',
    'Name: "english"; MessagesFile: "compiler:Default.isl"',
    'Name: "chinesesimplified"; MessagesFile: "ChineseSimplified.isl"',
    'english.AddToPathCaption=Add FluffOS commands to your user PATH',
    'chinesesimplified.AddToPathCaption=',
    "ExpandConstant('{cm:AddToPathCaption}')",
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

$chineseLanguageFile = Join-Path $scriptDir 'ChineseSimplified.isl'
if (-not (Test-Path -LiteralPath $chineseLanguageFile)) {
    throw "Installer script references ChineseSimplified.isl, but the file was not found next to the script: $chineseLanguageFile"
}

[pscustomobject]@{
    InstallerScriptPath = (Resolve-Path -LiteralPath $InstallerScriptPath).Path
    VerifiedSettings = $requiredPatterns
    ChineseLanguageFile = (Resolve-Path -LiteralPath $chineseLanguageFile).Path
}
