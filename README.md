# FTP Client Tool v1.01

Cliente FTP recursivo en C++ con script PowerShell para automatización.

## ⚠️ SEGURIDAD IMPORTANTE

**NUNCA subas `config.json` a GitHub** - contiene credenciales reales.

1. Copia `config.example.json` a `config.json`
2. Edita `config.json` con tus credenciales
3. `config.json` está en `.gitignore` para protección

## Archivos

- `ftp_client.cpp` - Cliente FTP principal (C++)
- `ftp_client.exe` - Ejecutable compilado
- `ftp_client.ps1` - Script PowerShell para automatización
- `config.json` - Configuración (NO subir a GitHub)
- `config.example.json` - Ejemplo de configuración

## Uso

### Subir archivo específico:
```powershell
powershell -ExecutionPolicy Bypass -File .ftp\ftp_client.ps1 upload archivo.php
```

### Subir todo el proyecto:
```powershell
powershell -ExecutionPolicy Bypass -File .ftp\ftp_client.ps1 upload all
```

### Listar archivos remotos:
```powershell
powershell -ExecutionPolicy Bypass -File .ftp\ftp_client.ps1 list
```

## Compilación

```bash
g++ -std=c++17 -O2 -o ftp_client.exe ftp_client.cpp -lwininet
```

## Características

- ✅ Subida recursiva de carpetas
- ✅ Ignora archivos ocultos (.git, .vscode, etc.)
- ✅ Manejo de errores robusto
- ✅ Logging detallado
- ✅ Configuración flexible
- ✅ Compatible Windows/Linux

## Versión 1.01

- Limpieza de archivos innecesarios
- Protección de credenciales
- Documentación mejorada
- Scripts de automatización 