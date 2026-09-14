param(
    [Parameter(Mandatory = $true)]
    [string]$RunDriverPath
)

$ErrorActionPreference = "Stop"
$PSNativeCommandUseErrorActionPreference = $false

$runDriverPath = (Resolve-Path $RunDriverPath).Path
$tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("fluffos-run-driver-test-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $tempRoot | Out-Null

try {
    $configPath = Join-Path $tempRoot "config.hell"
    Set-Content -Path $configPath -Value "# test config" -Encoding ASCII

    Push-Location $tempRoot
    try {
        $output = & cmd /c """$runDriverPath"" config.hell" 2>&1
        $exitCode = $LASTEXITCODE
    } finally {
        Pop-Location
    }

    [pscustomobject]@{
        ExitCode = $exitCode
        Output = ($output -join [Environment]::NewLine)
    }
} finally {
    Remove-Item -Recurse -Force $tempRoot -ErrorAction SilentlyContinue
}
