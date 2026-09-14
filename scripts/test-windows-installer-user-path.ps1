param(
    [Parameter(Mandatory = $true)]
    [string]$InstallerPath
)

$ErrorActionPreference = "Stop"

$resolvedInstallerPath = (Resolve-Path -LiteralPath $InstallerPath -ErrorAction Stop).Path
$tempDir = Join-Path $env:TEMP ('fluffos-installer-path-test-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $tempDir | Out-Null

$originalPath = ''
$hasOriginalPath = $false
try {
    $envKey = Get-Item 'HKCU:\Environment' -ErrorAction Stop
    $originalPath = $envKey.GetValue('Path', '', 'DoNotExpandEnvironmentNames')
    $hasOriginalPath = $null -ne $envKey.GetValue('Path', $null, 'DoNotExpandEnvironmentNames')
} catch {
    $originalPath = ''
    $hasOriginalPath = $false
}

try {
    $process = Start-Process -FilePath $resolvedInstallerPath `
        -ArgumentList @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/CURRENTUSER', "/DIR=$tempDir", '/ADDTOPATH=1') `
        -Wait -PassThru

    if ($process.ExitCode -ne 0) {
        throw "Installer exited with code $($process.ExitCode)"
    }

    $updatedPath = (Get-ItemProperty 'HKCU:\Environment' -ErrorAction SilentlyContinue).Path
    $expectedBin = Join-Path $tempDir 'bin'
    if (-not $updatedPath -or $updatedPath -notmatch [regex]::Escape($expectedBin)) {
        throw "User PATH does not contain expected bin directory: $expectedBin"
    }

    if (-not (Test-Path (Join-Path $tempDir 'bin\lpcprj.exe'))) {
        throw "Installed lpcprj.exe not found under custom install directory."
    }
    if (-not (Test-Path (Join-Path $tempDir 'libexec\fluffos\driver.exe'))) {
        throw "Installed driver.exe not found under custom install directory."
    }

    [pscustomobject]@{
        InstallDir = $tempDir
        AddedPath = $expectedBin
        ExitCode = $process.ExitCode
    }
} finally {
    if ($hasOriginalPath) {
        Set-ItemProperty -Path 'HKCU:\Environment' -Name Path -Value $originalPath
    } else {
        Remove-ItemProperty -Path 'HKCU:\Environment' -Name Path -ErrorAction SilentlyContinue
    }

    $uninstaller = Join-Path $tempDir 'unins000.exe'
    if (Test-Path $uninstaller) {
        Start-Process -FilePath $uninstaller -ArgumentList @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART') -Wait | Out-Null
    }
    Remove-Item -Recurse -Force $tempDir -ErrorAction SilentlyContinue
}
