# FTP Client v2.0 - Terminal FTP Solution for AI Developers 🥑

> **Made by Aoxilus** - Because sometimes you just need to upload files without the GUI drama! 🎯

A simple, efficient FTP client for Windows. No external dependencies, just native PowerShell or compiled C++. Built for a terminal workflow when you need to upload files without a GUI.

## 🚀 Features

- ✅ **Stable connection** - Robust error handling
- ✅ **Configurable timeouts** - Adapts to slow connections
- ✅ **Detailed logging** - Timestamps and complete debugging
- ✅ **Two versions** - Pure PowerShell and compiled C++
- ✅ **Flexible configuration** - Simple and clear JSON
- ✅ **Directory upload** - Upload entire folders with one command

## 📁 Files

- `ftp_pure.ps1` - PowerShell client (recommended)
- `ftp_client_v2.cpp` - C++ source code
- `ftp_client_v2.exe` - Compiled executable
- `config.json` - Connection configuration
- `build.bat` - Compilation script

## 🔧 Quick Start

### PowerShell (Recommended)
```powershell
# Enable scripts
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser

# Test connection
.\ftp_pure.ps1 test

# List files
.\ftp_pure.ps1 list

# Upload single file
.\ftp_pure.ps1 upload file.txt

# Upload entire directory
.\ftp_pure.ps1 upload-dir my_project

# Download file
.\ftp_pure.ps1 download file.txt

# Delete file
.\ftp_pure.ps1 delete file.txt
```

### C++ Compiled
```cmd
# Compile
build.bat

# Use
.\ftp_client_v2.exe test
.\ftp_client_v2.exe list
.\ftp_client_v2.exe upload file.txt
.\ftp_client_v2.exe download file.txt
```

## ⚙️ Configuration

Edit `config.json`:
```json
{
    "host": "your-server.com",
    "username": "your-username",
    "password": "your-password",
    "remotePath": "",
    "localPath": ".",
    "port": 21,
    "timeout": 30,
    "passive": false
}
```

## 📊 Available Commands

| Command | Description |
|---------|-------------|
| `test` | Test connection |
| `list` | List files |
| `upload <file>` | Upload single file |
| `upload-dir <directory>` | Upload entire directory |
| `download <file>` | Download file |
| `delete <file>` | Delete file |

## 🔍 Logs

All operations are logged in `ftp.log`:
```
[2025-01-27 10:30:15] [INFO] Connecting to server.com...
[2025-01-27 10:30:16] [SUCCESS] Connection successful
```

## 🐛 Troubleshooting

1. **Connection error**: Check credentials in `config.json`
2. **Timeouts**: Increase `timeout` in configuration
3. **Passive mode**: Change `passive` to `true` or `false`

## 🎭 Why PowerShell is More Portable?

> **Joke**: Why did the C++ developer go broke? Because he spent all his money on dependencies! 😂

### PowerShell Advantages:
- ✅ **No compilation needed** - Works directly on any Windows
- ✅ **Native dependencies** - Uses .NET Framework (included)
- ✅ **Easy distribution** - Just copy the .ps1 file
- ✅ **Quick modifications** - Edit → Test immediately
- ✅ **Universal compatibility** - Windows 7+ to Windows 11

### When to use each:

**PowerShell (.ps1) - Recommended for:**
- ✅ Fast development
- ✅ Testing and debugging
- ✅ Simple distribution
- ✅ Frequent modifications
- ✅ Various Windows environments

**C++ (.exe) - Better for:**
- ✅ Maximum performance
- ✅ Final distribution
- ✅ Non-technical users
- ✅ Controlled environments

## 🥑 The Avocado Story

> **Fun fact**: This FTP client is like an avocado - simple on the outside, powerful on the inside, and perfect for developers who want to "upload their guacamole" to the server! 🥑

## 🎯 Terminal workflow

- **No GUI**: stay in the terminal
- **Script automation**: easy to drop into your own scripts
- **Quick file management**: fast upload/download cycles

## 🚀 Directory Upload Example

```powershell
# Upload your entire project
.\ftp_pure.ps1 upload-dir my_awesome_project

# Upload specific folder
.\ftp_pure.ps1 upload-dir src
```

## 💡 Pro Tips

1. **Use PowerShell for development** - Faster iteration
2. **Use C++ for production** - Better performance
3. **Check logs** - Always check `ftp.log` for debugging
4. **Test connection first** - Always run `test` before uploading

---

**Made with 🥑 by [aoxilus](https://github.com/aoxilus)** — simple, fast, efficient. Like an avocado, but for FTP! 🥑⚡

## License / Licencia

[CC BY-NC-SA 4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/) — Attribution-NonCommercial-ShareAlike. See [LICENSE](LICENSE).
