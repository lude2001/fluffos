@echo off
setlocal EnableExtensions

set "ROOT_DIR=%~dp0"
for %%I in ("%ROOT_DIR%.") do set "ROOT_DIR=%%~fI"
set "BUILD_ROOT=%ROOT_DIR%\build"
set "BUILD_DIR=%BUILD_ROOT%\work"
set "DIST_DIR=%BUILD_ROOT%\dist"
set "INSTALL_IMAGE_DIR=%BUILD_ROOT%\install-image"

set "MSYS2_ROOT="
if defined MSYS2_ROOT if exist "%MSYS2_ROOT%\usr\bin\bash.exe" set "MSYS2_ROOT=%MSYS2_ROOT%"
if not defined MSYS2_ROOT if exist "D:\software\msys2\usr\bin\bash.exe" set "MSYS2_ROOT=D:\software\msys2"
if not defined MSYS2_ROOT if exist "C:\msys64\usr\bin\bash.exe" set "MSYS2_ROOT=C:\msys64"

if not defined MSYS2_ROOT (
  echo Could not find MSYS2. Set MSYS2_ROOT to your MSYS2 install directory.
  exit /b 1
)

set "BASH_EXE=%MSYS2_ROOT%\usr\bin\bash.exe"
set "POWERSHELL_EXE=powershell.exe"
set "JOBS=%NUMBER_OF_PROCESSORS%"
if not defined JOBS set "JOBS=8"

echo Using MSYS2 from "%MSYS2_ROOT%"
echo Configuring and building driver artifacts into "%BUILD_DIR%"

"%BASH_EXE%" -lc "set -euo pipefail; export MSYSTEM=MINGW64; export PATH=/mingw64/bin:/usr/bin:$PATH; root=$(cygpath -m \"$1\"); build=$(cygpath -m \"$2\"); msys=$(cygpath -m \"$3\"); jobs=\"$4\"; cmake -S \"$root\" -B \"$build\" -G 'MSYS Makefiles' -DCMAKE_BUILD_TYPE=RelWithDebInfo -DMARCH_NATIVE=OFF -DPACKAGE_CRYPTO=OFF -DPACKAGE_DB=ON -DPACKAGE_DB_MYSQL=1 -DPACKAGE_DB_DEFAULT_DB=1 -DMYSQL_INCLUDE_DIR=\"$msys/mingw64/include/mysql\" -DMYSQL_LIB_DIR=\"$msys/mingw64/lib\" \"-DMYSQL_EXTRA_LIBRARIES=$msys/mingw64/lib/libcurl.dll.a;$msys/mingw64/lib/libzstd.dll.a\"; cmake --build \"$build\" --target driver lpccp -j \"$jobs\"" -- "%ROOT_DIR%" "%BUILD_DIR%" "%MSYS2_ROOT%" "%JOBS%"
if errorlevel 1 exit /b %errorlevel%

"%POWERSHELL_EXE%" -NoProfile -ExecutionPolicy Bypass -File "%ROOT_DIR%\scripts\export-vscode-compile-commands.ps1" -SourcePath "%BUILD_DIR%\compile_commands.json" -OutputPath "%BUILD_ROOT%\compile_commands.json"
if errorlevel 1 exit /b %errorlevel%

"%POWERSHELL_EXE%" -NoProfile -ExecutionPolicy Bypass -File "%ROOT_DIR%\scripts\stage-driver-dist.ps1" -RootDir "%ROOT_DIR%" -BuildDir "%BUILD_DIR%" -DistDir "%DIST_DIR%" -Msys2Root "%MSYS2_ROOT%"
if errorlevel 1 exit /b %errorlevel%

"%POWERSHELL_EXE%" -NoProfile -ExecutionPolicy Bypass -File "%ROOT_DIR%\scripts\stage-windows-install-image.ps1" -RootDir "%ROOT_DIR%" -DistDir "%DIST_DIR%" -InstallImageDir "%INSTALL_IMAGE_DIR%"
if errorlevel 1 exit /b %errorlevel%

echo Dist staged at "%DIST_DIR%"
echo Install image staged at "%INSTALL_IMAGE_DIR%"
exit /b 0
