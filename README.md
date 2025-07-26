# FTP Client v2.0 - Cliente FTP Robusto
Ar
Cliente FTP simple y eficiente para Windows. Sin dependencias externas, solo PowerShell nativo o C++ compilado.

## 🚀 Características

- ✅ **Conexión estable** - Manejo robusto de errores
- ✅ **Timeouts configurables** - Se adapta a conexiones lentas
- ✅ **Logging detallado** - Timestamps y debugging completo
- ✅ **Dos versiones** - PowerShell puro y C++ compilado
- ✅ **Configuración flexible** - JSON simple y claro

## 📁 Archivos

- `ftp_pure.ps1` - Cliente PowerShell (recomendado)
- `ftp_client_v2.cpp` - Código fuente C++
- `ftp_client_v2.exe` - Ejecutable compilado
- `config.json` - Configuración de conexión
- `build.bat` - Script de compilación

## 🔧 Uso Rápido

### PowerShell (Recomendado)
```powershell
# Habilitar scripts
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser

# Probar conexión
.\ftp_pure.ps1 test

# Listar archivos
.\ftp_pure.ps1 list

# Subir archivo
.\ftp_pure.ps1 upload archivo.txt

# Descargar archivo
.\ftp_pure.ps1 download archivo.txt

# Borrar archivo
.\ftp_pure.ps1 delete archivo.txt
```

### C++ Compilado
```cmd
# Compilar
build.bat

# Usar
.\ftp_client_v2.exe test
.\ftp_client_v2.exe list
.\ftp_client_v2.exe upload archivo.txt
.\ftp_client_v2.exe download archivo.txt
```

## ⚙️ Configuración

Edita `config.json`:
```json
{
    "host": "tu-servidor.com",
    "username": "tu-usuario",
    "password": "tu-password",
    "remotePath": "",
    "localPath": ".",
    "port": 21,
    "timeout": 30,
    "passive": false
}
```

## 📊 Comandos Disponibles

| Comando | Descripción |
|---------|-------------|
| `test` | Probar conexión |
| `list` | Listar archivos |
| `upload <archivo>` | Subir archivo |
| `download <archivo>` | Descargar archivo |
| `delete <archivo>` | Borrar archivo |

## 🔍 Logs

Todas las operaciones se registran en `ftp.log`:
```
[2025-01-27 10:30:15] [INFO] Conectando a servidor.com...
[2025-01-27 10:30:16] [SUCCESS] Conexión exitosa
```

## 🐛 Solución de Problemas

1. **Error de conexión**: Verifica credenciales en `config.json`
2. **Timeouts**: Aumenta `timeout` en la configuración
3. **Modo pasivo**: Cambia `passive` a `true` o `false`

---

**Hecho por aoxilus** - Simple, rápido, eficiente. 