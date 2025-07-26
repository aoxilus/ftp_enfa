@echo off
echo Compilando FTP Client v2.0...

REM Verificar si MinGW está instalado
where g++ >nul 2>nul
if %errorlevel% neq 0 (
    echo Error: g++ no encontrado. Instala MinGW-W64 o MSYS2
    echo Descarga desde: https://www.mingw-w64.org/downloads/
    pause
    exit /b 1
)

REM Compilar con optimizaciones
g++ -std=c++17 -O2 -Wall -o ftp_client_v2.exe ftp_client_v2.cpp -lwininet

if %errorlevel% equ 0 (
    echo Compilación exitosa: ftp_client_v2.exe
    echo.
    echo Uso:
    echo   ftp_client_v2.exe list
    echo   ftp_client_v2.exe download-all
    echo   ftp_client_v2.exe upload-all
    echo   ftp_client_v2.exe delete-all
) else (
    echo Error en la compilación
)

pause 