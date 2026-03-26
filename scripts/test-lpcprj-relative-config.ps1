param(
    [Parameter(Mandatory = $true)]
    [string]$LpcprjPath
)

$ErrorActionPreference = "Stop"
$PSNativeCommandUseErrorActionPreference = $false

$resolvedLpcprjPath = (Resolve-Path -LiteralPath $LpcprjPath -ErrorAction Stop).Path
$tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("fluffos-lpcprj-test-" + [guid]::NewGuid().ToString("N"))
$configDir = Join-Path $tempRoot "config"
$launchDir = Join-Path $tempRoot "launch"

New-Item -ItemType Directory -Path $configDir | Out-Null
New-Item -ItemType Directory -Path $launchDir | Out-Null

try {
    $configPath = Join-Path $configDir "config.hell"
    Set-Content -Path $configPath -Value "# lpcprj smoke test" -Encoding ASCII

    Push-Location $launchDir
    try {
        $output = & $resolvedLpcprjPath $configPath 2>&1
        $exitCode = $LASTEXITCODE
    } finally {
        Pop-Location
    }

    [pscustomobject]@{
        ExitCode = $exitCode
        Output = ($output -join [Environment]::NewLine)
        ConfigPath = $configPath
        LaunchDir = $launchDir
    }
} finally {
    Remove-Item -Recurse -Force $tempRoot -ErrorAction SilentlyContinue
}
