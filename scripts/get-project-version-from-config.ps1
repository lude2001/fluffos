param(
    [Parameter(Mandatory = $true)]
    [string]$ConfigHeaderPath
)

$ErrorActionPreference = "Stop"
$resolvedConfigHeaderPath = (Resolve-Path -LiteralPath $ConfigHeaderPath -ErrorAction Stop).Path
$match = Select-String -Path $resolvedConfigHeaderPath -Pattern '^#define PROJECT_VERSION "fluffos (.+)"$' | Select-Object -First 1

if (-not $match) {
    throw "Unable to determine FluffOS version from $resolvedConfigHeaderPath"
}

$match.Matches[0].Groups[1].Value
