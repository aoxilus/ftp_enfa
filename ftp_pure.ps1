# FTP Client PowerShell - Cliente FTP Puro
# Uso: .\ftp_pure.ps1 <comando> [opciones]

param(
    [Parameter(Mandatory=$true)]
    [string]$Command,
    
    [Parameter(Mandatory=$false)]
    [string]$File
)

# Función para logging
function Write-Log {
    param([string]$Level, [string]$Message)
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $logMessage = "[$timestamp] [$Level] $Message"
    Write-Host $logMessage
    Add-Content -Path "ftp.log" -Value $logMessage
}

# Función para cargar configuración JSON
function Load-Config {
    $configFiles = @("config.json", ".ftp/config.json", "ftp_config.json")
    
    foreach ($configFile in $configFiles) {
        if (Test-Path $configFile) {
            try {
                $config = Get-Content $configFile | ConvertFrom-Json
                Write-Log "INFO" "Configuración cargada desde: $configFile"
                return $config
            }
            catch {
                Write-Log "ERROR" "Error cargando configuración desde $configFile"
            }
        }
    }
    
    Write-Log "ERROR" "No se pudo cargar la configuración. Verifica que config.json existe."
    return $null
}

# Función para crear sesión FTP
function New-FtpSession {
    param($Config)
    
    try {
        $uri = "ftp://$($Config.host):$($Config.port)$($Config.remotePath)"
        $credential = New-Object System.Management.Automation.PSCredential($Config.username, (ConvertTo-SecureString $Config.password -AsPlainText -Force))
        
        Write-Log "INFO" "Conectando a $($Config.host)..."
        
        # Crear sesión FTP usando .NET
        $ftpRequest = [System.Net.FtpWebRequest]::Create($uri)
        $ftpRequest.Credentials = $credential
        $ftpRequest.UsePassive = $Config.passive
        $ftpRequest.Timeout = $Config.timeout * 1000
        
        return $ftpRequest
    }
    catch {
        Write-Log "ERROR" "Error creando sesión FTP: $($_.Exception.Message)"
        return $null
    }
}

# Función para listar archivos
function Get-FtpFiles {
    param($Config)
    
    try {
        $uri = "ftp://$($Config.host):$($Config.port)$($Config.remotePath)"
        $credential = New-Object System.Management.Automation.PSCredential($Config.username, (ConvertTo-SecureString $Config.password -AsPlainText -Force))
        
        $ftpRequest = [System.Net.FtpWebRequest]::Create($uri)
        $ftpRequest.Credentials = $credential
        $ftpRequest.Method = [System.Net.WebRequestMethods+Ftp]::ListDirectory
        $ftpRequest.UsePassive = $Config.passive
        $ftpRequest.Timeout = $Config.timeout * 1000
        
        Write-Log "INFO" "Listando archivos..."
        
        $response = $ftpRequest.GetResponse()
        $stream = $response.GetResponseStream()
        $reader = New-Object System.IO.StreamReader($stream)
        
        $files = $reader.ReadToEnd() -split "`r`n" | Where-Object { $_ -ne "" }
        
        Write-Log "INFO" "Contenido remoto de $($Config.remotePath):"
        foreach ($file in $files) {
            Write-Host "[FILE] $file"
            Write-Log "FILE" "[FILE] $file"
        }
        
        $reader.Close()
        $response.Close()
        
        return $true
    }
    catch {
        Write-Log "ERROR" "Error listando archivos: $($_.Exception.Message)"
        return $false
    }
}

# Función para descargar archivo
function Get-FtpFile {
    param($Config, $FileName)
    
    try {
        $remoteFile = "$($Config.remotePath)/$FileName"
        $localFile = "$($Config.localPath)\$FileName"
        
        $uri = "ftp://$($Config.host):$($Config.port)$remoteFile"
        $credential = New-Object System.Management.Automation.PSCredential($Config.username, (ConvertTo-SecureString $Config.password -AsPlainText -Force))
        
        $ftpRequest = [System.Net.FtpWebRequest]::Create($uri)
        $ftpRequest.Credentials = $credential
        $ftpRequest.Method = [System.Net.WebRequestMethods+Ftp]::DownloadFile
        $ftpRequest.UsePassive = $Config.passive
        $ftpRequest.Timeout = $Config.timeout * 1000
        
        Write-Log "INFO" "Descargando: $remoteFile -> $localFile"
        
        $response = $ftpRequest.GetResponse()
        $stream = $response.GetResponseStream()
        
        $fileStream = [System.IO.File]::Create($localFile)
        $stream.CopyTo($fileStream)
        
        $fileStream.Close()
        $response.Close()
        
        Write-Log "SUCCESS" "Archivo descargado exitosamente"
        return $true
    }
    catch {
        Write-Log "ERROR" "Error descargando archivo: $($_.Exception.Message)"
        return $false
    }
}

# Función para subir archivo
function Set-FtpFile {
    param($Config, $FileName)
    
    try {
        $localFile = "$($Config.localPath)\$FileName"
        $remoteFile = "$($Config.remotePath)/$FileName"
        
        if (-not (Test-Path $localFile)) {
            Write-Log "ERROR" "Archivo local no encontrado: $localFile"
            return $false
        }
        
        $uri = "ftp://$($Config.host):$($Config.port)$remoteFile"
        $credential = New-Object System.Management.Automation.PSCredential($Config.username, (ConvertTo-SecureString $Config.password -AsPlainText -Force))
        
        $ftpRequest = [System.Net.FtpWebRequest]::Create($uri)
        $ftpRequest.Credentials = $credential
        $ftpRequest.Method = [System.Net.WebRequestMethods+Ftp]::UploadFile
        $ftpRequest.UsePassive = $Config.passive
        $ftpRequest.Timeout = $Config.timeout * 1000
        
        Write-Log "INFO" "Subiendo: $localFile -> $remoteFile"
        
        $fileStream = [System.IO.File]::OpenRead($localFile)
        $requestStream = $ftpRequest.GetRequestStream()
        
        $fileStream.CopyTo($requestStream)
        
        $requestStream.Close()
        $fileStream.Close()
        
        $response = $ftpRequest.GetResponse()
        $response.Close()
        
        Write-Log "SUCCESS" "Archivo subido exitosamente"
        return $true
    }
    catch {
        Write-Log "ERROR" "Error subiendo archivo: $($_.Exception.Message)"
        return $false
    }
}

# Función para probar conexión
function Test-FtpConnection {
    param($Config)
    
    try {
        $uri = "ftp://$($Config.host):$($Config.port)"
        $credential = New-Object System.Management.Automation.PSCredential($Config.username, (ConvertTo-SecureString $Config.password -AsPlainText -Force))
        
        $ftpRequest = [System.Net.FtpWebRequest]::Create($uri)
        $ftpRequest.Credentials = $credential
        $ftpRequest.Method = [System.Net.WebRequestMethods+Ftp]::PrintWorkingDirectory
        $ftpRequest.UsePassive = $Config.passive
        $ftpRequest.Timeout = $Config.timeout * 1000
        
        Write-Log "INFO" "Probando conexión..."
        
        $response = $ftpRequest.GetResponse()
        $response.Close()
        
        Write-Log "SUCCESS" "Conexión exitosa"
        return $true
    }
    catch {
        Write-Log "ERROR" "Error de conexión: $($_.Exception.Message)"
        return $false
    }
}

# Función para mostrar ayuda
function Show-Help {
    Write-Host "FTP Client PowerShell - Cliente FTP Puro"
    Write-Host "Uso: .\ftp_pure.ps1 <comando> [opciones]`n"
    Write-Host "Comandos:"
    Write-Host "  list                    - Listar archivos del servidor"
    Write-Host "  download <archivo>      - Descargar archivo específico"
    Write-Host "  upload <archivo>        - Subir archivo específico"
    Write-Host "  test                    - Probar conexión`n"
    Write-Host "Ejemplos:"
    Write-Host "  .\ftp_pure.ps1 list"
    Write-Host "  .\ftp_pure.ps1 download archivo.php"
    Write-Host "  .\ftp_pure.ps1 upload archivo.php"
    Write-Host "  .\ftp_pure.ps1 test"
}

# Función principal
function Main {
    # Cargar configuración
    $config = Load-Config
    if (-not $config) {
        exit 1
    }
    
    Write-Log "INFO" "Configuración:"
    Write-Log "INFO" "  Host: $($config.host)"
    Write-Log "INFO" "  User: $($config.username)"
    Write-Log "INFO" "  Remote: $($config.remotePath)"
    Write-Log "INFO" "  Local: $($config.localPath)"
    Write-Log "INFO" "  Port: $($config.port)"
    Write-Log "INFO" "  Timeout: $($config.timeout)s"
    Write-Log "INFO" "  Passive: $($config.passive)"
    
    # Ejecutar comando
    $success = $false
    
    switch ($Command.ToLower()) {
        "list" {
            $success = Get-FtpFiles -Config $config
        }
        "download" {
            if ($File) {
                $success = Get-FtpFile -Config $config -FileName $File
            } else {
                Write-Log "ERROR" "Comando download requiere nombre de archivo"
                Show-Help
            }
        }
        "upload" {
            if ($File) {
                $success = Set-FtpFile -Config $config -FileName $File
            } else {
                Write-Log "ERROR" "Comando upload requiere nombre de archivo"
                Show-Help
            }
        }
        "test" {
            $success = Test-FtpConnection -Config $config
        }
        default {
            Write-Log "ERROR" "Comando no válido: $Command"
            Show-Help
        }
    }
    
    if ($success) {
        Write-Log "SUCCESS" "Operación completada exitosamente"
        exit 0
    } else {
        Write-Log "FAILURE" "Operación falló"
        exit 1
    }
}

# Ejecutar función principal
Main 