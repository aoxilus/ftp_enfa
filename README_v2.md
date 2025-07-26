# FTP Client Tool v2.0 - Cliente FTP Robusto

Herramienta FTP mejorada para Windows con manejo robusto de errores, timeouts configurables y comandos adicionales.

## 🚀 Características Nuevas

### ✅ Mejoras de Estabilidad
- **Manejo de errores mejorado** - No falla silenciosamente
- **Timeouts configurables** - Se adapta a diferentes conexiones
- **Parsing JSON robusto** - Usa regex para mayor confiabilidad
- **Logging con timestamps** - Mejor debugging

### ✅ Nuevos Comandos
- `delete <archivo>` - Borrar archivo específico
- `delete-all` - Borrar todos los archivos recursivamente
- `rmdir <directorio>` - Borrar directorio remoto
- `test` - Probar conexión sin hacer nada

### ✅ Configuración Flexible
- Múltiples ubicaciones de config: `config.json`, `.ftp/config.json`, `ftp_config.json`
- Configuración de puerto personalizable
- Modo pasivo/activo configurable
- Timeouts personalizables

## 📁 Archivos

- `ftp_client_v2.cpp` - Código fuente mejorado
- `ftp_client_v2.exe` - Ejecutable compilado
- `config_v2.json` - Configuración de ejemplo
- `build.bat` - Script de compilación
- `ftp.ps1` - Wrapper de PowerShell
- `ftp.log` - Log de operaciones

## 🔧 Instalación

### 1. Compilar
```cmd
build.bat
```

### 2. Configurar
Copia `config_v2.json` a `config.json` y edita:
```json
{
    "host": "tu-servidor.com",
    "username": "tu-usuario", 
    "password": "tu-password",
    "remotePath": "/public_html",
    "localPath": ".",
    "port": 21,
    "timeout": 30,
    "passive": true
}
```

## 🎯 Uso

### Comandos Básicos
```cmd
# Listar archivos
ftp_client_v2.exe list

# Descargar todo
ftp_client_v2.exe download-all

# Subir todo  
ftp_client_v2.exe upload-all

# Borrar todo
ftp_client_v2.exe delete-all
```

### Comandos Específicos
```cmd
# Archivo específico
ftp_client_v2.exe download archivo.php
ftp_client_v2.exe upload archivo.php
ftp_client_v2.exe delete archivo.php

# Directorios
ftp_client_v2.exe mkdir nuevo_directorio
ftp_client_v2.exe rmdir directorio_viejo

# Probar conexión
ftp_client_v2.exe test
```

### PowerShell (Recomendado)
```powershell
# Habilitar ejecución de scripts
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser

# Usar wrapper
.\ftp.ps1 list
.\ftp.ps1 download-all
.\ftp.ps1 upload-all
.\ftp.ps1 delete-all
.\ftp.ps1 download archivo.php
```

## 🔍 Logging

Todas las operaciones se registran en `ftp.log` con timestamps:
```
[2025-01-27 10:30:15] [INFO] Conectando a servidor.com...
[2025-01-27 10:30:16] [INFO] Conexión FTP establecida
[2025-01-27 10:30:17] [INFO] [FILE] index.php -> /public_html/index.php
[2025-01-27 10:30:18] [SUCCESS] Operación completada exitosamente
```

## ⚙️ Configuración Avanzada

### Timeouts
```json
{
    "timeout": 60  // 60 segundos para conexiones lentas
}
```

### Modo Activo/Pasivo
```json
{
    "passive": false  // Modo activo para algunos servidores
}
```

### Puerto Personalizado
```json
{
    "port": 2121  // Puerto FTP alternativo
}
```

## 🛠️ Compilación Manual

Si tienes MinGW-W64 instalado:
```cmd
g++ -std=c++17 -O2 -Wall -o ftp_client_v2.exe ftp_client_v2.cpp -lwininet
```

## 🔒 Seguridad

- `config.json` está excluido del control de versiones
- Las credenciales nunca se suben a GitHub
- Logs detallados para debugging

## 🐛 Solución de Problemas

### Error de Conexión
1. Verifica credenciales en `config.json`
2. Prueba con `ftp_client_v2.exe test`
3. Revisa `ftp.log` para errores específicos

### Timeouts
1. Aumenta `timeout` en la configuración
2. Verifica conexión a internet
3. Prueba modo pasivo/activo

### Archivos No Encontrados
1. Verifica `remotePath` y `localPath`
2. Usa `list` para ver contenido del servidor
3. Revisa permisos de archivos

## 📊 Comparación v1 vs v2

| Característica | v1.03 | v2.0 |
|---|---|---|
| Manejo de errores | Básico | Robusto |
| Timeouts | Fijos | Configurables |
| Comandos | 6 | 10 |
| Logging | Simple | Con timestamps |
| Configuración | Rígida | Flexible |
| Borrado recursivo | ❌ | ✅ |
| Prueba de conexión | ❌ | ✅ |

## 🎯 Próximas Mejoras

- [ ] Sincronización inteligente (solo archivos modificados)
- [ ] Compresión automática
- [ ] Interfaz gráfica simple
- [ ] Soporte para SFTP
- [ ] Backup automático antes de borrar

---

**Hecho por aoxilus** - Cliente FTP simple, robusto y sin dependencias innecesarias. 