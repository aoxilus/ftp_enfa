# FTP Client PowerShell Wrapper
# Uso: .\ftp.ps1 <comando> [opciones]

param(
    [Parameter(Mandatory=$true)]
    [string]$Command,
    
    [Parameter(Mandatory=$false)]
    [string]$File
)

# Verificar si el ejecutable existe
if (-not (Test-Path "ftp_client_v2.exe")) {
    Write-Host "Error: ftp_client_v2.exe no encontrado. Ejecuta build.bat primero." -ForegroundColor Red
    exit 1
}

# Construir argumentos
$args = @($Command)
if ($File) {
    $args += $File
}

# Ejecutar cliente FTP
Write-Host "Ejecutando: ftp_client_v2.exe $($args -join ' ')" -ForegroundColor Green
& .\ftp_client_v2.exe @args

# Mostrar resultado
if ($LASTEXITCODE -eq 0) {
    Write-Host "Operación completada exitosamente" -ForegroundColor Green
} else {
    Write-Host "Operación falló con código: $LASTEXITCODE" -ForegroundColor Red
} 