@echo off
setlocal EnableDelayedExpansion

echo ====================================================
echo [RapidDesk] Iniciando Compilacao Automatizada
echo ====================================================

:: Define a pasta de destino final solicitada
set "TARGET_DIR=C:\Carlo\AcessoRemoto\rapiddesk\Release"

:: Cria a pasta Release se ela nao existir
if not exist "%TARGET_DIR%" mkdir "%TARGET_DIR%"

:: --- CONTROLE DE VERSÃO (Chama o script externo alterando o destino do JSON) ---
:: Passamos a gerar/atualizar o version.json direto dentro da pasta Release
set "JSON_FILE=%TARGET_DIR%\version.json"

:: Cria um arquivo temporario em PowerShell para rodar a atualizacao apontando para o caminho correto
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
echo [Info] Versao Atualizada do Build: !NEW_VERSION!
:: ------------------------------------------------------------------

:: 1. TENTA ENCONTRAR O CAMINHO DO CMAKE DO VISUAL STUDIO AUTOMATICAMENTE
set "FOUND_CMAKE="
for %%V in (18 2022 2019) do (
    for %%E in (Community Professional Enterprise) do (
        set "TEST_PATH=C:\Program Files\Microsoft Visual Studio\%%V\%%E\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
        if exist "!TEST_PATH!\cmake.exe" (
            set "FOUND_CMAKE=!TEST_PATH!"
            goto CMAKE_FOUND
        )
    )
)

:CMAKE_FOUND
if "%FOUND_CMAKE%" NEQ "" (
    set "PATH=%FOUND_CMAKE%;%PATH%"
    echo [Info] CMake encontrado com sucesso no Visual Studio.
) else (
    where cmake >nul 2>nul
    if !ERRORLEVEL! NEQ 0 (
        echo [Erro] O CMake nao foi encontrado no seu computador.
        goto FIM
    )
    echo [Info] CMake global detectado no sistema.
)

:: 2. LIMPA E RECRIA A PASTA DE COMPILACAO
echo [RapidDesk] Limpando arquivos temporarios antigos...
if exist build rmdir /s /q build
mkdir build
cd build

:: 3. GERA AS CONFIGURACOES DO PROJETO
echo [RapidDesk] Configurando o ambiente do projeto via CMake...
cmake -DCMAKE_BUILD_TYPE=Release ..
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [Erro] Falha ao gerar os arquivos de configuracao do CMake.
    cd ..
    goto FIM
)

:: 4. COMPILA O CODIGO FONTE EM MODO RELEASE
echo [RapidDesk] Compilando os codigos fontes (Modo Ultra-Performance)...
cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [Erro] Falha na compilacao dos arquivos C++. Verifique os erros acima.
    cd ..
    goto FIM
)

:: 5. DETECTA E MOVE O EXECUTAVEL PARA A PASTA RELEASE
echo.
echo [Sucesso] Compilacao concluida com êxito!
echo [RapidDesk] Enviando os arquivos para a pasta Release...

set "MOVED=0"
if exist Release\RapidDesk.exe (
    move /y Release\RapidDesk.exe "%TARGET_DIR%\" >nul
    set "MOVED=1"
) else if exist RapidDesk.exe (
    move /y RapidDesk.exe "%TARGET_DIR%\" >nul
    set "MOVED=1"
)

cd ..

if "!MOVED!"=="1" (
    echo ====================================================
    echo [Pronto] Arquivos gerados com sucesso em:
    echo %TARGET_DIR%
    echo.
    echo  -^> RapidDesk.exe (Pronto para rodar)
    echo  -^> version.json  (Versao: !NEW_VERSION!)
    echo ====================================================
) else (
    echo [Aviso] O executavel foi compilado, mas nao foi encontrado na pasta padrao para ser movido.
)

:FIM
pause