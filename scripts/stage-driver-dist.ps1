param(
    [Parameter(Mandatory = $true)]
    [string]$RootDir,
    [Parameter(Mandatory = $true)]
    [string]$BuildDir,
    [Parameter(Mandatory = $true)]
    [string]$DistDir,
    [Parameter(Mandatory = $true)]
    [string]$Msys2Root
)

$ErrorActionPreference = "Stop"

function Get-DllDependencies {
    param(
        [Parameter(Mandatory = $true)]
        [string]$BinaryPath,
        [Parameter(Mandatory = $true)]
        [string]$ObjdumpPath
    )

    $lines = & $ObjdumpPath -p $BinaryPath 2>$null
    foreach ($line in $lines) {
        if ($line -match 'DLL Name:\s+(.+)$') {
            $Matches[1].Trim()
        }
    }
}

function Sync-Directory {
    param(
        [Parameter(Mandatory = $true)]
        [string]$SourceDir,
        [Parameter(Mandatory = $true)]
        [string]$TargetDir
    )

    if (Test-Path $TargetDir) {
        Remove-Item -Recurse -Force $TargetDir
    }
    Copy-Item -Recurse -Force $SourceDir $TargetDir
}

$driverPath = Join-Path $BuildDir "src\driver.exe"
$msysBinDir = Join-Path $Msys2Root "mingw64\bin"
$objdumpPath = Join-Path $msysBinDir "objdump.exe"

if (-not (Test-Path $driverPath)) {
    throw "Missing driver.exe at $driverPath"
}
if (-not (Test-Path $objdumpPath)) {
    throw "Missing objdump.exe at $objdumpPath"
}

New-Item -ItemType Directory -Force -Path $DistDir | Out-Null
Copy-Item -Force $driverPath (Join-Path $DistDir "driver.exe")

$systemDlls = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
@(
    "ADVAPI32.dll",
    "bcrypt.dll",
    "CRYPT32.dll",
    "IPHLPAPI.DLL",
    "KERNEL32.dll",
    "msvcrt.dll",
    "Secur32.dll",
    "SHELL32.dll",
    "SHLWAPI.dll",
    "USER32.dll",
    "WLDAP32.dll",
    "WS2_32.dll"
) | ForEach-Object { [void]$systemDlls.Add($_) }

$copiedDlls = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
$queued = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
$queue = [System.Collections.Generic.Queue[string]]::new()
$queue.Enqueue((Join-Path $DistDir "driver.exe"))

while ($queue.Count -gt 0) {
    $current = $queue.Dequeue()
    foreach ($dll in Get-DllDependencies -BinaryPath $current -ObjdumpPath $objdumpPath) {
        if ($dll -match '^(api-ms-win-|ext-ms-win-)') {
            continue
        }
        if ($systemDlls.Contains($dll)) {
            continue
        }

        $sourceDll = Join-Path $msysBinDir $dll
        if (-not (Test-Path $sourceDll)) {
            continue
        }

        $targetDll = Join-Path $DistDir $dll
        Copy-Item -Force $sourceDll $targetDll

        if ($copiedDlls.Add($dll) -and $queued.Add($targetDll)) {
            $queue.Enqueue($targetDll)
        }
    }
}

Sync-Directory -SourceDir (Join-Path $RootDir "testsuite\std") -TargetDir (Join-Path $DistDir "std")
Sync-Directory -SourceDir (Join-Path $RootDir "src\include") -TargetDir (Join-Path $DistDir "include")
Sync-Directory -SourceDir (Join-Path $RootDir "src\www") -TargetDir (Join-Path $DistDir "www")

$runDriverPath = Join-Path $DistDir "run-driver.cmd"
$runDriverContent = @'
@echo off
setlocal EnableExtensions EnableDelayedExpansion

if "%~1"=="" (
  echo Usage: run-driver.cmd ^<config-file^> [extra-driver-args...]
  exit /b 1
)

set "SCRIPT_DIR=%~dp0"
set "CONFIG_FILE=%~f1"
set "RUN_DIR=%~dp1"
shift
set "DRIVER_ARGS="
:collect_args
if "%~1"=="" goto run_driver
set "DRIVER_ARGS=!DRIVER_ARGS! "%~1""
shift
goto collect_args
:run_driver
set "PATH=%SCRIPT_DIR%;%PATH%"
pushd "%RUN_DIR%" >nul
"%SCRIPT_DIR%driver.exe" "%CONFIG_FILE%"!DRIVER_ARGS!
set "EXIT_CODE=%ERRORLEVEL%"
popd >nul
exit /b %EXIT_CODE%
'@
[System.IO.File]::WriteAllText($runDriverPath, $runDriverContent, [System.Text.Encoding]::ASCII)
