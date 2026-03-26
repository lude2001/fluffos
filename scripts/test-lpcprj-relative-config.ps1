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

    $outputText = ($output -join [Environment]::NewLine)
    if ($outputText -notmatch [regex]::Escape("Full Command Line:")) {
        throw "lpcprj did not emit driver startup output."
    }
    if ($outputText -notmatch [regex]::Escape("libexec\fluffos\driver.exe")) {
        throw "lpcprj did not launch the staged driver executable."
    }
    if ($outputText -notmatch [regex]::Escape("Processing config file: $configPath")) {
        throw "lpcprj did not forward the requested config file path."
    }

    [pscustomobject]@{
        ExitCode = $exitCode
        Output = $outputText
        ConfigPath = $configPath
        LaunchDir = $launchDir
    }
} finally {
    Remove-Item -Recurse -Force $tempRoot -ErrorAction SilentlyContinue
}
