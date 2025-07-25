param (
    [string]$command = "",
    [string]$target = ""
)

$ftpFolder = $PSScriptRoot
$projectRoot = Split-Path $ftpFolder -Parent
$configPath = Join-Path $ftpFolder "config.json"

if (!(Test-Path $configPath)) {
    Write-Host "❌ No se encontró $configPath"
    exit 1
}

try {
    $config = Get-Content $configPath -Raw | ConvertFrom-Json
} catch {
    Write-Host "❌ Error al leer o parsear $configPath"
    exit 1
}

$ftpHost   = $config.host
$user      = $config.username
$pass      = $config.password
$remote    = $config.remotePath.TrimEnd('/')
$exe       = Join-Path $ftpFolder "ftp_client.exe"

if (!(Test-Path $exe)) {
    Write-Host "❌ No se encontró el ejecutable $exe"
    exit 2
}

function IsDotPath($path) {
    return (($path -split '[\\/]').Where({ $_ -like '.*' }).Count -gt 0)
}

function Ensure-RemoteDir($remoteDir) {
	$parts = $remoteDir -split '/'
	$path = ''
	foreach ($p in $parts) {
		if ($p -eq '' -or $p -eq '.') { continue }
		$path = $path + '/' + $p
		& $exe $ftpHost $user $pass mkdir "" "$path"
	}
}

function Upload-All {
	$dirs = Get-ChildItem -Path $projectRoot -Recurse -Directory | Where-Object {
		$_.FullName -notmatch "\\.ftp(\$|\\)" -and -not (IsDotPath $_.FullName)
	}
	foreach ($d in $dirs) {
		$rel = $d.FullName.Substring($projectRoot.Length + 1) -replace "\\", "/"
		$remotePath = "$remote/$rel"
		$rc = & $exe $ftpHost $user $pass mkdir "" "$remotePath"
		if ($LASTEXITCODE -ne 0) {
			Write-Host "❌ Error creando carpeta remota: $remotePath"
			exit 10
		}
	}
	$files = Get-ChildItem -Path $projectRoot -Recurse -File | Where-Object {
		$_.FullName -notmatch "\\.ftp(\$|\\)" -and -not (IsDotPath $_.FullName)
	}
	foreach ($f in $files) {
		$rel = $f.FullName.Substring($projectRoot.Length + 1) -replace "\\", "/"
		$remotePath = "$remote/$rel"
		Write-Host "⬆️ Subiendo $rel"
		$rc = & $exe $ftpHost $user $pass upload "`"$($f.FullName)`"" "`"$remotePath`""
		if ($LASTEXITCODE -ne 0) {
			Write-Host "❌ Error subiendo archivo: $rel"
			exit 11
		}
	}
}

function Upload-File($filename) {
    $localPath = Join-Path $projectRoot $filename
    if (!(Test-Path $localPath)) {
        Write-Host "❌ Archivo no encontrado: $filename"
        exit 3
    }
    if (IsDotPath $filename) {
        Write-Host "⚠️ Ignorado archivo oculto: $filename"
        exit 0
    }
    $remotePath = "$remote/$filename" -replace "\\", "/"
    Write-Host "⬆️ Subiendo $filename"
    & $exe $ftpHost $user $pass upload "`"$localPath`"" "`"$remotePath`""
}

function Download-File($filename) {
    if (IsDotPath $filename) {
        Write-Host "⚠️ Ignorado archivo oculto: $filename"
        exit 0
    }
    $remotePath = "$remote/$filename" -replace "\\", "/"
    $localPath = Join-Path $projectRoot $filename
    Write-Host "⬇️ Descargando $remotePath → $localPath"
    & $exe $ftpHost $user $pass download "`"$remotePath`"" "`"$localPath`""
}

function Download-All {
    Write-Host "⬇️ Descargando TODO el contenido de $remote → $projectRoot"
    & $exe $ftpHost $user $pass download-all "`"$projectRoot`"" "`"$remote`""
}

switch ($command) {
	"list" {
		& $exe $ftpHost $user $pass list "" "$remote"
	}
	"upload" {
		if ($target -eq "all" -or [string]::IsNullOrEmpty($target)) {
			Upload-All
		} else {
			Upload-File $target
		}
	}
	"download" {
		if ($target -eq "all" -or [string]::IsNullOrEmpty($target)) {
			Download-All
		} else {
			Download-File $target
		}
	}
	"delete" {
		$remotePath = "$remote/$target" -replace "\\", "/"
		& $exe $ftpHost $user $pass delete "`"$remotePath`""
	}
	default {
		Write-Host "Comando inválido."
		exit 1
	}
}
