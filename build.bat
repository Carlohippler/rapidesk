@echo off
setlocal EnableDelayedExpansion

echo ====================================================
echo [RapidDesk] Build com Conan + CMake
echo ====================================================

set "PROJECT_DIR=C:\Carlo\AcessoRemoto\rapiddesk"
set "TARGET_DIR=%PROJECT_DIR%\Release"
cd /d "%PROJECT_DIR%"

if not exist "%TARGET_DIR%" mkdir "%TARGET_DIR%"

:: --- VERSAO ---
set "JSON_FILE=%TARGET_DIR%\version.json"
echo $jsonFile = "%JSON_FILE:\=\\%" > temp_update.ps1
echo if (-not (Test-Path $jsonFile)) { >> temp_update.ps1
echo     $data = @{ version = 1.0 } >> temp_update.ps1
echo     $data ^| ConvertTo-Json ^| Set-Content $jsonFile >> temp_update.ps1
echo     Write-Output "1.0" >> temp_update.ps1
echo } else { >> temp_update.ps1
echo     $data = Get-Content $jsonFile ^| ConvertFrom-Json >> temp_update.ps1
echo     $data.version = [math]::Round($data.version + 0.1, 1) >> temp_update.ps1
echo     $data ^| ConvertTo-Json ^| Set-Content $jsonFile >> temp_update.ps1
echo     Write-Output $data.version >> temp_update.ps1
echo } >> temp_update.ps1

for /f "delims=" %%V in ('powershell -NoProfile -ExecutionPolicy Bypass -File temp_update.ps1') do (
    set "NEW_VERSION=%%V"
)
del temp_update.ps1 >nul 2>nul
echo [Info] Versao: !NEW_VERSION!

:: ============================================
:: ETAPA 1: ENCONTRAR CMAKE (caminho curto)
:: ============================================
echo.
echo [1/6] Procurando CMake...

set "FOUND_CMAKE="

for %%V in (18 2022 2019) do (
    for %%E in (Community Professional Enterprise) do (
        set "TEST_PATH=C:\Program Files\Microsoft Visual Studio\%%V\%%E\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
        if exist "!TEST_PATH!" (
            set "FOUND_CMAKE=!TEST_PATH!"
            goto CMAKE_FOUND
        )
    )
)

:CMAKE_FOUND
if "%FOUND_CMAKE%"=="" (
    where cmake >nul 2>nul
    if !ERRORLEVEL! EQU 0 (
        for /f "delims=" %%i in ('where cmake') do set "FOUND_CMAKE=%%i"
    )
)

if "%FOUND_CMAKE%"=="" (
    echo [ERRO] CMake nao encontrado! Instale em https://cmake.org/download/
    goto FIM
)

:: Converter para caminho curto (8.3) - ESSENCIAL!
for %%F in ("%FOUND_CMAKE%") do set "SHORT_CMAKE=%%~sF"
echo [OK] CMake: %SHORT_CMAKE%

:: ============================================
:: ETAPA 2: CONFIGURAR CONAN
:: ============================================
echo.
echo [2/6] Configurando Conan...

set "PROFILE_DIR=C:\Users\Carlo\.conan2\profiles"
if not exist "%PROFILE_DIR%" mkdir "%PROFILE_DIR%"

(
echo [settings]
echo arch=x86_64
echo build_type=Release
echo compiler=msvc
echo compiler.cppstd=20
echo compiler.runtime=dynamic
echo compiler.runtime_type=Release
echo compiler.version=195
echo os=Windows
) > "%PROFILE_DIR%\default"

:: Criar global.conf com caminho CURTO (sem espaços!)
(
echo tools.cmake:cmake_program=%SHORT_CMAKE%
) > "C:\Users\Carlo\.conan2\global.conf"

echo [OK] global.conf criado

:: ============================================
:: ETAPA 3: CONAN INSTALL
:: ============================================
echo.
echo [3/6] Conan install...

conan install "%PROJECT_DIR%" --build=missing -s build_type=Release

if %ERRORLEVEL% NEQ 0 (
    echo [ERRO] Conan install falhou!
    goto FIM
)

:: ============================================
:: ETAPA 4: VERIFICAR TOOLCHAIN
:: ============================================
echo.
echo [4/6] Verificando toolchain...

set "TOOLCHAIN=%PROJECT_DIR%\build\generators\conan_toolchain.cmake"
if not exist "%TOOLCHAIN%" (
    echo [ERRO] Toolchain nao encontrado!
    goto FIM
)
echo [OK] Toolchain encontrado

:: ============================================
:: ETAPA 5: CMAKE CONFIGURE
:: ============================================
echo.
echo [5/6] Configurando CMake...

if exist build\CMakeCache.txt del /f build\CMakeCache.txt

cmake -B build -S "%PROJECT_DIR%" -G "Visual Studio 18 2026" -A x64 ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_TOOLCHAIN_FILE="%TOOLCHAIN%" ^
  -DVCPKG_TOOLCHAIN="" ^
  -DVCPKG_TARGET_TRIPLET=""

if %ERRORLEVEL% NEQ 0 (
    echo [ERRO] CMake configure falhou!
    goto FIM
)

:: ============================================
:: ETAPA 6: BUILD
:: ============================================
echo.
echo [6/6] Compilando...

cmake --build build --config Release --parallel

if %ERRORLEVEL% NEQ 0 (
    echo [ERRO] Build falhou!
    goto FIM
)

:: ============================================
:: MOVER PARA RELEASE
:: ============================================
echo.
echo [Sucesso] Movendo executavel...

set "MOVED=0"
if exist "build\src\Release\rapiddesk.exe" (
    copy /y "build\src\Release\rapiddesk.exe" "%TARGET_DIR%\" >nul
    set "MOVED=1"
)

cd /d "%PROJECT_DIR%"

if "!MOVED!"=="1" (
    echo ====================================================
    echo [SUCESSO] Build v!NEW_VERSION! completo!
    echo Pasta: %TARGET_DIR%
    echo ====================================================
) else (
    echo [AVISO] Executavel nao encontrado na pasta esperada.
    dir /s /b build\rapiddesk.exe 2>nul
)

:FIM
echo.
pause