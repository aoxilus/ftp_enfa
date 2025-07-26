#include <windows.h>
#include <wininet.h>
#include <iostream>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <regex>

namespace fs = std::filesystem;

#pragma comment(lib, "wininet.lib")

// Configuración global
struct Config {
    std::string host;
    std::string username;
    std::string password;
    std::string remotePath;
    std::string localPath;
    int port;
    int timeout;
    bool passive;
};

Config config;
std::ofstream logFile("ftp.log", std::ios::app);

void log(const std::string& level, const std::string& msg) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::string timestamp = std::ctime(&time_t);
    timestamp.pop_back(); // Remove newline
    
    std::cout << "[" << level << "] " << msg << std::endl;
    logFile << "[" << timestamp << "] [" << level << "] " << msg << std::endl;
    logFile.flush();
}

void print_last_error() {
    DWORD err = GetLastError();
    char* msg = NULL;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&msg, 0, NULL);
    if (msg) {
        log("ERROR", "WinINet " + std::to_string(err) + " - " + msg);
        LocalFree(msg);
        return;
    }
    log("ERROR", "WinINet " + std::to_string(err));
}

std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::string get_json_value(const std::string& json, const std::string& key) {
    std::regex pattern("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch match;
    if (std::regex_search(json, match, pattern)) {
        return match[1].str();
    }
    
    // Try numeric value
    pattern = "\"" + key + "\"\\s*:\\s*([0-9]+)";
    if (std::regex_search(json, match, pattern)) {
        return match[1].str();
    }
    
    // Try boolean value
    pattern = "\"" + key + "\"\\s*:\\s*(true|false)";
    if (std::regex_search(json, match, pattern)) {
        return match[1].str();
    }
    
    return "";
}

bool load_config(const std::string& configFile) {
    std::ifstream f(configFile);
    if (!f) {
        log("ERROR", "No se pudo abrir " + configFile);
        return false;
    }
    
    std::string json((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    
    config.host = get_json_value(json, "host");
    config.username = get_json_value(json, "username");
    config.password = get_json_value(json, "password");
    config.remotePath = get_json_value(json, "remotePath");
    config.localPath = get_json_value(json, "localPath");
    config.port = std::stoi(get_json_value(json, "port").empty() ? "21" : get_json_value(json, "port"));
    config.timeout = std::stoi(get_json_value(json, "timeout").empty() ? "30" : get_json_value(json, "timeout"));
    config.passive = get_json_value(json, "passive") == "true";
    
    if (config.host.empty() || config.username.empty() || config.password.empty()) {
        log("ERROR", "Configuración incompleta en " + configFile);
        return false;
    }
    
    if (config.localPath.empty()) config.localPath = ".";
    if (config.remotePath.empty()) config.remotePath = "/";
    
    return true;
}

bool excluded(const std::string& name) {
    if (name.empty()) return true;
    if (name[0] == '.') return true;
    if (name == "node_modules" || name == "vendor" || name == ".git" || name == ".vscode") return true;
    if (name == "ftp.log" || name == "ftp_client.exe" || name == "ftp_client_v2.exe") return true;
    return false;
}

void ensure_remote_path(HINTERNET hFtp, const std::string& fullPath) {
    std::string acc;
    std::istringstream ss(fullPath);
    std::string segment;
    while (std::getline(ss, segment, '/')) {
        if (segment.empty()) continue;
        acc += (acc.empty() ? "" : "/") + segment;
        if (!FtpCreateDirectoryA(hFtp, acc.c_str())) {
            DWORD err = GetLastError();
            if (err == ERROR_ALREADY_EXISTS || err == ERROR_FILE_EXISTS) continue;
            log("ERROR", "Error creando directorio remoto: " + acc);
            print_last_error();
        }
    }
}

bool upload_recursive(HINTERNET hFtp, const std::string& local, const std::string& remote) {
    for (const auto& entry : fs::directory_iterator(local)) {
        std::string name = entry.path().filename().string();
        if (excluded(name)) continue;
        
        std::string localPath = entry.path().string();
        std::string remotePath = remote + "/" + name;
        
        if (entry.is_directory()) {
            log("INFO", "[DIR] " + localPath + " -> " + remotePath);
            ensure_remote_path(hFtp, remotePath);
            if (!upload_recursive(hFtp, localPath, remotePath)) return false;
        } else {
            log("INFO", "[FILE] " + localPath + " -> " + remotePath);
            if (!FtpPutFileA(hFtp, localPath.c_str(), remotePath.c_str(), FTP_TRANSFER_TYPE_BINARY, 0)) {
                log("ERROR", "Error subiendo: " + localPath + " -> " + remotePath);
                print_last_error();
                return false;
            }
        }
    }
    return true;
}

void list_files(HINTERNET hFtp, const std::string& path) {
    WIN32_FIND_DATAA findData;
    std::string searchPath = path.empty() ? "*.*" : path + "/*";
    
    DWORD timeout = config.timeout * 1000;
    InternetSetOptionA(hFtp, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionA(hFtp, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
    
    HINTERNET hFind = FtpFindFirstFileA(hFtp, searchPath.c_str(), &findData, INTERNET_FLAG_RELOAD, 0);
    if (!hFind) {
        DWORD err = GetLastError();
        if (err == ERROR_NO_MORE_FILES) {
            log("INFO", "Directorio vacío: " + (path.empty() ? "/" : path));
            return;
        }
        log("ERROR", "Error listando: " + (path.empty() ? "/" : path));
        print_last_error();
        return;
    }
    
    log("INFO", "Contenido remoto de " + (path.empty() ? "/" : path) + ":");
    do {
        std::string type = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? "[DIR] " : "[FILE]";
        std::cout << type << findData.cFileName << std::endl;
        log("FILE", type + findData.cFileName);
    } while (InternetFindNextFileA(hFind, &findData));
    InternetCloseHandle(hFind);
}

bool download_recursive(HINTERNET hFtp, const std::string& remote, const std::string& local) {
    WIN32_FIND_DATAA findData;
    std::string searchPath = remote.empty() ? "*" : remote + "/*";
    
    DWORD timeout = config.timeout * 1000;
    InternetSetOptionA(hFtp, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionA(hFtp, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
    
    HINTERNET hFind = FtpFindFirstFileA(hFtp, searchPath.c_str(), &findData, INTERNET_FLAG_RELOAD, 0);
    if (!hFind) {
        DWORD err = GetLastError();
        if (err == ERROR_NO_MORE_FILES) {
            log("INFO", "Directorio vacío: " + (remote.empty() ? "/" : remote));
            return true;
        }
        log("ERROR", "Error listando: " + (remote.empty() ? "/" : remote) + " - continuando...");
        print_last_error();
        return true;
    }
    
    bool hasFiles = false;
    do {
        std::string name = findData.cFileName;
        if (name == "." || name == "..") continue;
        hasFiles = true;
        
        std::string remotePath = remote.empty() ? name : remote + "/" + name;
        std::string localPath = local + "\\" + name;
        
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            log("INFO", "[DIR] " + remotePath + " -> " + localPath);
            CreateDirectoryA(localPath.c_str(), NULL);
            if (!download_recursive(hFtp, remotePath, localPath)) {
                log("ERROR", "Error en directorio: " + remotePath + " - continuando...");
            }
        } else {
            log("INFO", "[FILE] " + remotePath + " -> " + localPath);
            
            DWORD downloadTimeout = config.timeout * 1000;
            InternetSetOptionA(hFtp, INTERNET_OPTION_CONNECT_TIMEOUT, &downloadTimeout, sizeof(downloadTimeout));
            InternetSetOptionA(hFtp, INTERNET_OPTION_RECEIVE_TIMEOUT, &downloadTimeout, sizeof(downloadTimeout));
            
            if (!FtpGetFileA(hFtp, remotePath.c_str(), localPath.c_str(), FALSE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_BINARY, 0)) {
                log("ERROR", "Error descargando: " + remotePath + " - continuando...");
                print_last_error();
            }
        }
    } while (InternetFindNextFileA(hFind, &findData));
    
    InternetCloseHandle(hFind);
    if (!hasFiles) {
        log("INFO", "Directorio vacío: " + (remote.empty() ? "/" : remote));
    }
    return true;
}

bool delete_recursive(HINTERNET hFtp, const std::string& remote) {
    WIN32_FIND_DATAA findData;
    std::string searchPath = remote.empty() ? "*" : remote + "/*";
    
    DWORD timeout = config.timeout * 1000;
    InternetSetOptionA(hFtp, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionA(hFtp, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
    
    HINTERNET hFind = FtpFindFirstFileA(hFtp, searchPath.c_str(), &findData, INTERNET_FLAG_RELOAD, 0);
    if (!hFind) {
        DWORD err = GetLastError();
        if (err == ERROR_NO_MORE_FILES) {
            log("INFO", "Directorio vacío: " + (remote.empty() ? "/" : remote));
            return true;
        }
        log("ERROR", "Error listando: " + (remote.empty() ? "/" : remote));
        print_last_error();
        return false;
    }
    
    std::vector<std::string> files, dirs;
    do {
        std::string name = findData.cFileName;
        if (name == "." || name == "..") continue;
        
        std::string remotePath = remote.empty() ? name : remote + "/" + name;
        
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            dirs.push_back(remotePath);
        } else {
            files.push_back(remotePath);
        }
    } while (InternetFindNextFileA(hFind, &findData));
    InternetCloseHandle(hFind);
    
    // Delete files first
    for (const auto& file : files) {
        log("INFO", "[DELETE] " + file);
        if (!FtpDeleteFileA(hFtp, file.c_str())) {
            log("ERROR", "Error borrando archivo: " + file);
            print_last_error();
        }
    }
    
    // Delete directories recursively
    for (const auto& dir : dirs) {
        if (!delete_recursive(hFtp, dir)) {
            log("ERROR", "Error borrando directorio: " + dir);
        }
    }
    
    // Delete current directory if not root
    if (!remote.empty()) {
        log("INFO", "[DELETE DIR] " + remote);
        if (!FtpRemoveDirectoryA(hFtp, remote.c_str())) {
            log("ERROR", "Error borrando directorio: " + remote);
            print_last_error();
        }
    }
    
    return true;
}

void show_usage(const char* program) {
    std::cout << "FTP Client Tool v2.0 - Cliente FTP Robusto\n";
    std::cout << "Uso:\n";
    std::cout << "  " << program << " <comando> [opciones]\n\n";
    std::cout << "Comandos:\n";
    std::cout << "  list                    - Listar archivos del servidor\n";
    std::cout << "  download <archivo>      - Descargar archivo específico\n";
    std::cout << "  download-all            - Descargar todos los archivos\n";
    std::cout << "  upload <archivo>        - Subir archivo específico\n";
    std::cout << "  upload-all              - Subir todos los archivos\n";
    std::cout << "  delete <archivo>        - Borrar archivo específico\n";
    std::cout << "  delete-all              - Borrar todos los archivos\n";
    std::cout << "  mkdir <directorio>      - Crear directorio remoto\n";
    std::cout << "  rmdir <directorio>      - Borrar directorio remoto\n";
    std::cout << "  test                    - Probar conexión\n\n";
    std::cout << "Ejemplos:\n";
    std::cout << "  " << program << " list\n";
    std::cout << "  " << program << " download-all\n";
    std::cout << "  " << program << " upload-all\n";
    std::cout << "  " << program << " delete-all\n";
    std::cout << "  " << program << " download debug.php\n";
    std::cout << "  " << program << " upload api.php\n";
    std::cout << "  " << program << " delete old_file.php\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        show_usage(argv[0]);
        return 1;
    }
    
    std::string command = argv[1];
    
    // Try multiple config locations
    std::vector<std::string> configFiles = {
        "config.json",
        ".ftp/config.json",
        "ftp_config.json"
    };
    
    bool configLoaded = false;
    for (const auto& configFile : configFiles) {
        if (load_config(configFile)) {
            log("INFO", "Configuración cargada desde: " + configFile);
            configLoaded = true;
            break;
        }
    }
    
    if (!configLoaded) {
        log("ERROR", "No se pudo cargar la configuración. Verifica que config.json existe.");
        return 1;
    }
    
    log("INFO", "Configuración:");
    log("INFO", "  Host: " + config.host);
    log("INFO", "  User: " + config.username);
    log("INFO", "  Remote: " + config.remotePath);
    log("INFO", "  Local: " + config.localPath);
    log("INFO", "  Port: " + std::to_string(config.port));
    log("INFO", "  Timeout: " + std::to_string(config.timeout) + "s");
    log("INFO", "  Passive: " + std::string(config.passive ? "Sí" : "No"));
    
    log("INFO", "Conectando a " + config.host + "...");
    
    HINTERNET hInternet = InternetOpenA("ftp_client_v2", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) { 
        log("ERROR", "Error inicializando WinINet");
        print_last_error(); 
        return 2; 
    }
    
    log("INFO", "WinINet inicializado correctamente");
    
    // Set global timeouts
    DWORD globalTimeout = config.timeout * 1000;
    InternetSetOptionA(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &globalTimeout, sizeof(globalTimeout));
    InternetSetOptionA(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &globalTimeout, sizeof(globalTimeout));
    InternetSetOptionA(hInternet, INTERNET_OPTION_SEND_TIMEOUT, &globalTimeout, sizeof(globalTimeout));
    
    log("INFO", "Intentando conectar FTP...");
    
    DWORD flags = INTERNET_FLAG_PASSIVE;
    if (!config.passive) flags = 0;
    
    HINTERNET hFtp = InternetConnectA(hInternet, config.host.c_str(), config.port,
        config.username.c_str(), config.password.c_str(), INTERNET_SERVICE_FTP, flags, 0);
    if (!hFtp) { 
        log("ERROR", "Error conectando a " + config.host);
        print_last_error(); 
        InternetCloseHandle(hInternet); 
        return 3; 
    }
    
    log("INFO", "Conexión FTP establecida");
    
    bool ok = false;
    
    if (command == "test") {
        log("INFO", "Probando conexión...");
        ok = true;
    } else if (command == "list") {
        log("INFO", "Listando archivos...");
        list_files(hFtp, config.remotePath);
        ok = true;
    } else if (command == "download-all") {
        log("INFO", "Descargando todos los archivos...");
        ok = download_recursive(hFtp, config.remotePath, config.localPath);
    } else if (command == "upload-all") {
        log("INFO", "Subiendo todos los archivos...");
        ok = upload_recursive(hFtp, config.localPath, config.remotePath);
    } else if (command == "delete-all") {
        log("INFO", "Borrando todos los archivos...");
        ok = delete_recursive(hFtp, config.remotePath);
    } else if (command == "download" && argc > 2) {
        std::string file = argv[2];
        std::string remoteFile = config.remotePath + "/" + file;
        std::string localFile = config.localPath + "\\" + file;
        log("INFO", "Descargando: " + remoteFile + " -> " + localFile);
        ok = FtpGetFileA(hFtp, remoteFile.c_str(), localFile.c_str(), FALSE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_BINARY, 0);
        if (!ok) {
            log("ERROR", "Error descargando archivo");
            print_last_error();
        }
    } else if (command == "upload" && argc > 2) {
        std::string file = argv[2];
        std::string localFile = config.localPath + "\\" + file;
        std::string remoteFile = config.remotePath + "/" + file;
        log("INFO", "Subiendo: " + localFile + " -> " + remoteFile);
        ok = FtpPutFileA(hFtp, localFile.c_str(), remoteFile.c_str(), FTP_TRANSFER_TYPE_BINARY, 0);
        if (!ok) {
            log("ERROR", "Error subiendo archivo");
            print_last_error();
        }
    } else if (command == "delete" && argc > 2) {
        std::string file = argv[2];
        std::string remoteFile = config.remotePath + "/" + file;
        log("INFO", "Borrando: " + remoteFile);
        ok = FtpDeleteFileA(hFtp, remoteFile.c_str());
        if (!ok) {
            log("ERROR", "Error borrando archivo");
            print_last_error();
        }
    } else if (command == "mkdir" && argc > 2) {
        std::string dir = argv[2];
        std::string remoteDir = config.remotePath + "/" + dir;
        log("INFO", "Creando directorio: " + remoteDir);
        ok = FtpCreateDirectoryA(hFtp, remoteDir.c_str());
        if (!ok) {
            DWORD err = GetLastError();
            if (err == ERROR_ALREADY_EXISTS || err == ERROR_FILE_EXISTS) {
                log("INFO", "Directorio ya existe");
                ok = true;
            } else {
                log("ERROR", "Error creando directorio");
                print_last_error();
            }
        }
    } else if (command == "rmdir" && argc > 2) {
        std::string dir = argv[2];
        std::string remoteDir = config.remotePath + "/" + dir;
        log("INFO", "Borrando directorio: " + remoteDir);
        ok = delete_recursive(hFtp, remoteDir);
    } else {
        log("ERROR", "Comando no válido: " + command);
        show_usage(argv[0]);
        ok = false;
    }
    
    log("INFO", "Cerrando conexiones...");
    InternetCloseHandle(hFtp);
    InternetCloseHandle(hInternet);
    
    if (ok) {
        log("SUCCESS", "Operación completada exitosamente");
        return 0;
    } else {
        log("FAILURE", "Operación falló");
        return 4;
    }
} 