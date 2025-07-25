# FTP Client Tool v1.03

Herramienta FTP robusta para descargar y subir archivos al servidor con manejo de errores y timeouts.

**Hecha por aoxilus que se cansó de las herramientas estúpidas y voluminosas y de los mainstreamers que usan una librería para literalmente un loop.**

## Archivos

- `ftp_client.cpp` - Cliente FTP en C++ con logging
- `ftp_client.exe` - Ejecutable compilado con MinGW-W64 GCC 15.1.0
- `config.json` - Configuración de conexión (NO incluir en git)
- `config.example.json` - Ejemplo de configuración
- `.gitignore` - Excluye archivos sensibles

## Configuración

1. Copia `config.example.json` a `config.json`
2. Edita `config.json` con tus credenciales:

```json
{
    "host": "tu-servidor.com",
    "username": "tu-usuario",
    "password": "tu-password",
    "remotePath": "/",
    "localPath": "."
}
```

## Compilación

```bash
g++ -std=c++17 -O2 -o ftp_client.exe ftp_client.cpp -lwininet
```

## Uso

```bash
# Listar archivos del servidor
ftp_client.exe list

# Descargar todos los archivos
ftp_client.exe download-all

# Subir todos los archivos
ftp_client.exe upload-all

# Descargar archivo específico
ftp_client.exe download archivo.php

# Subir archivo específico
ftp_client.exe upload archivo.php

# Crear directorio remoto
ftp_client.exe mkdir nuevo_directorio
```

## Características

- ✅ Descarga recursiva de directorios
- ✅ Manejo robusto de errores (continúa en errores)
- ✅ Timeouts configurables (20s listado, 30s transferencia)
- ✅ Logging detallado en `ftp.log`
- ✅ Argumentos directos (sin batch files)
- ✅ Compilado con MinGW-W64 GCC 15.1.0

## Seguridad

- `config.json` está excluido del control de versiones
- Las credenciales nunca se suben a GitHub
- Logs de errores para debugging

## Versión 1.03

- Herramienta limpia y optimizada
- Solo archivos esenciales
- README actualizado
- Configuración de seguridad mejorada 