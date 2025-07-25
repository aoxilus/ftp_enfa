#include <windows.h>
#include <wininet.h>
#include <iostream>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>

namespace fs = std::filesystem;

#pragma comment(lib, "wininet.lib")

std::ofstream logFile("ftp.log", std::ios::app);

void print_last_error() {
    DWORD err = GetLastError();
    char* msg = NULL;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&msg, 0, NULL);
    if (msg) {
        std::cerr << "WinINet error: " << err << " - " << msg;
        logFile << "[ERROR] WinINet " << err << " - " << msg;
        LocalFree(msg);
        return;
    }
    std::cerr << "WinINet error: " << err << std::endl;
    logFile << "[ERROR] WinINet " << err << std::endl;
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
			std::cerr << "Error creando directorio remoto: " << acc << std::endl;
			logFile << "[ERROR] Creando directorio remoto: " << acc << std::endl;
			print_last_error();
		}
	}
}

bool excluded(const std::string& name) {
    if (name.empty()) return true;
    if (name[0] == '.') return true;
    if (name == "node_modules" || name == "vendor" || name == ".git" || name == ".vscode") return true;
    return false;
}

bool upload_recursive(HINTERNET hFtp, const std::string& local, const std::string& remote) {
    for (const auto& entry : fs::directory_iterator(local)) {
        std::string name = entry.path().filename().string();
        if (excluded(name)) continue;
        std::string localPath = entry.path().string();
        std::string remotePath = remote + "/" + name;
        if (entry.is_directory()) {
            std::cout << "[DIR] " << localPath << " -> " << remotePath << std::endl;
            logFile << "[DIR] " << localPath << " -> " << remotePath << std::endl;
            ensure_remote_path(hFtp, remotePath);
            if (!upload_recursive(hFtp, localPath, remotePath)) return false;
        } else {
            std::cout << "[FILE] " << localPath << " -> " << remotePath << std::endl;
            logFile << "[FILE] " << localPath << " -> " << remotePath << std::endl;
            if (!FtpPutFileA(hFtp, localPath.c_str(), remotePath.c_str(), FTP_TRANSFER_TYPE_BINARY, 0)) {
                std::cerr << "Error subiendo: " << localPath << " -> " << remotePath << std::endl;
                logFile << "[ERROR] Subiendo: " << localPath << " -> " << remotePath << std::endl;
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
    
    // Set timeout for list operation
    DWORD timeout = 20000; // 20 seconds
    InternetSetOptionA(hFtp, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionA(hFtp, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
    
    HINTERNET hFind = FtpFindFirstFileA(hFtp, searchPath.c_str(), &findData, INTERNET_FLAG_RELOAD, 0);
    if (!hFind) {
        DWORD err = GetLastError();
        if (err == ERROR_NO_MORE_FILES) {
            std::cout << "Directorio vacio: " << (path.empty() ? "/" : path) << std::endl;
            logFile << "[INFO] Directorio vacio: " << (path.empty() ? "/" : path) << std::endl;
            return;
        }
        std::cerr << "Error listando: " << (path.empty() ? "/" : path) << std::endl;
        logFile << "[ERROR] Listando: " << (path.empty() ? "/" : path) << std::endl;
        print_last_error();
        return;
    }
    
    std::cout << "Contenido remoto de " << (path.empty() ? "/" : path) << ":\n";
    logFile << "[INFO] Contenido remoto de " << (path.empty() ? "/" : path) << ":" << std::endl;
    do {
        std::cout << findData.cFileName << std::endl;
        logFile << "[FILE] " << findData.cFileName << std::endl;
    } while (InternetFindNextFileA(hFind, &findData));
    InternetCloseHandle(hFind);
}

bool download_recursive(HINTERNET hFtp, const std::string& remote, const std::string& local) {
    WIN32_FIND_DATAA findData;
    std::string searchPath = remote.empty() ? "*" : remote + "/*";
    
    // Set timeout for list operation
    DWORD timeout = 20000; // 20 seconds
    InternetSetOptionA(hFtp, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionA(hFtp, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
    
    HINTERNET hFind = FtpFindFirstFileA(hFtp, searchPath.c_str(), &findData, INTERNET_FLAG_RELOAD, 0);
    if (!hFind) {
        DWORD err = GetLastError();
        if (err == ERROR_NO_MORE_FILES) {
            std::cout << "Directorio vacio: " << (remote.empty() ? "/" : remote) << std::endl;
            logFile << "[INFO] Directorio vacio: " << (remote.empty() ? "/" : remote) << std::endl;
            return true;
        }
        std::cerr << "Error listando: " << (remote.empty() ? "/" : remote) << " - continuando..." << std::endl;
        logFile << "[ERROR] Listando: " << (remote.empty() ? "/" : remote) << " - continuando..." << std::endl;
        print_last_error();
        return true; // Continue instead of failing
    }
    
    bool hasFiles = false;
    do {
        std::string name = findData.cFileName;
        if (name == "." || name == "..") continue;
        hasFiles = true;
        
        std::string remotePath = remote.empty() ? name : remote + "/" + name;
        std::string localPath = local + "\\" + name;
        
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            std::cout << "[DIR] " << remotePath << " -> " << localPath << std::endl;
            logFile << "[DIR] " << remotePath << " -> " << localPath << std::endl;
            CreateDirectoryA(localPath.c_str(), NULL);
            if (!download_recursive(hFtp, remotePath, localPath)) {
                std::cerr << "Error en directorio: " << remotePath << " - continuando..." << std::endl;
                logFile << "[ERROR] En directorio: " << remotePath << " - continuando..." << std::endl;
                // Continue instead of failing
            }
        } else {
            std::cout << "[FILE] " << remotePath << " -> " << localPath << std::endl;
            logFile << "[FILE] " << remotePath << " -> " << localPath << std::endl;
            
            // Set timeout for download
            DWORD downloadTimeout = 30000; // 30 seconds
            InternetSetOptionA(hFtp, INTERNET_OPTION_CONNECT_TIMEOUT, &downloadTimeout, sizeof(downloadTimeout));
            InternetSetOptionA(hFtp, INTERNET_OPTION_RECEIVE_TIMEOUT, &downloadTimeout, sizeof(downloadTimeout));
            
            if (!FtpGetFileA(hFtp, remotePath.c_str(), localPath.c_str(), FALSE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_BINARY, 0)) {
                std::cerr << "Error descargando: " << remotePath << " - continuando..." << std::endl;
                logFile << "[ERROR] Descargando: " << remotePath << " - continuando..." << std::endl;
                print_last_error();
                // Continue instead of failing
            }
        }
    } while (InternetFindNextFileA(hFind, &findData));
    
    InternetCloseHandle(hFind);
    if (!hasFiles) {
        std::cout << "Directorio vacio: " << (remote.empty() ? "/" : remote) << std::endl;
        logFile << "[INFO] Directorio vacio: " << (remote.empty() ? "/" : remote) << std::endl;
    }
    return true;
}

std::string get_json_value(const std::string& file, const std::string& key) {
    std::ifstream f(file);
    if (!f) return "";
    std::string j((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    auto pos = j.find("\"" + key + "\"");
    if (pos == std::string::npos) return "";
    pos = j.find(":", pos);
    if (pos == std::string::npos) return "";
    pos = j.find_first_of("\"0123456789", pos + 1);
    if (pos == std::string::npos) return "";
    if (j[pos] == '"') pos++;
    auto end = j.find_first_of(j[pos - 1] == '"' ? "\"" : ",}", pos);
    return j.substr(pos, end - pos);
}

void show_usage(const char* program) {
    std::cout << "FTP Client Tool v1.02\n";
    std::cout << "Uso:\n";
    std::cout << "  " << program << " <comando> [opciones]\n\n";
    std::cout << "Comandos:\n";
    std::cout << "  list                    - Listar archivos del servidor\n";
    std::cout << "  download <archivo>      - Descargar archivo específico\n";
    std::cout << "  download-all            - Descargar todos los archivos\n";
    std::cout << "  upload <archivo>        - Subir archivo específico\n";
    std::cout << "  upload-all              - Subir todos los archivos\n";
    std::cout << "  mkdir <directorio>      - Crear directorio remoto\n\n";
    std::cout << "Ejemplos:\n";
    std::cout << "  " << program << " list\n";
    std::cout << "  " << program << " download-all\n";
    std::cout << "  " << program << " upload-all\n";
    std::cout << "  " << program << " download debug.php\n";
    std::cout << "  " << program << " upload api.php\n";
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		show_usage(argv[0]);
		return 1;
	}
	
	std::string command = argv[1];
	
	// Load config from JSON
	std::string host = get_json_value(".ftp/config.json", "host");
	std::string user = get_json_value(".ftp/config.json", "username");
	std::string pass = get_json_value(".ftp/config.json", "password");
	std::string remotePath = get_json_value(".ftp/config.json", "remotePath");
	std::string localPath = get_json_value(".ftp/config.json", "localPath");
	
	std::cout << "Configuracion cargada:" << std::endl;
	logFile << "[INFO] Configuracion cargada:" << std::endl;
	std::cout << "  Host: " << host << std::endl;
	logFile << "[INFO] Host: " << host << std::endl;
	std::cout << "  User: " << user << std::endl;
	logFile << "[INFO] User: " << user << std::endl;
	std::cout << "  Remote: " << remotePath << std::endl;
	logFile << "[INFO] Remote: " << remotePath << std::endl;
	std::cout << "  Local: " << localPath << std::endl;
	logFile << "[INFO] Local: " << localPath << std::endl;
	
	if (host.empty() || user.empty() || pass.empty()) {
		std::cerr << "Error: Configuracion incompleta en .ftp/config.json\n";
		logFile << "[ERROR] Configuracion incompleta en .ftp/config.json" << std::endl;
		std::cerr << "Necesitas: host, username, password\n";
		return 1;
	}
	
	if (localPath.empty()) localPath = ".";
	if (remotePath.empty()) remotePath = "/";
	
	std::cout << "Conectando a " << host << "..." << std::endl;
	logFile << "[INFO] Conectando a " << host << "..." << std::endl;
	
	HINTERNET hInternet = InternetOpenA("ftp_client", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (!hInternet) { 
		std::cerr << "Error inicializando WinINet" << std::endl;
		logFile << "[ERROR] Inicializando WinINet" << std::endl;
		print_last_error(); 
		return 2; 
	}
	
	std::cout << "WinINet inicializado correctamente" << std::endl;
	logFile << "[INFO] WinINet inicializado correctamente" << std::endl;
	
	// Set global timeouts
	DWORD globalTimeout = 30000; // 30 seconds
	InternetSetOptionA(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &globalTimeout, sizeof(globalTimeout));
	InternetSetOptionA(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &globalTimeout, sizeof(globalTimeout));
	InternetSetOptionA(hInternet, INTERNET_OPTION_SEND_TIMEOUT, &globalTimeout, sizeof(globalTimeout));
	
	std::cout << "Intentando conectar FTP..." << std::endl;
	logFile << "[INFO] Intentando conectar FTP..." << std::endl;
	
	HINTERNET hFtp = InternetConnectA(hInternet, host.c_str(), INTERNET_DEFAULT_FTP_PORT,
		user.c_str(), pass.c_str(), INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);
	if (!hFtp) { 
		std::cerr << "Error conectando a " << host << std::endl;
		logFile << "[ERROR] Conectando a " << host << std::endl;
		print_last_error(); 
		InternetCloseHandle(hInternet); 
		return 3; 
	}
	
	std::cout << "Conexion FTP establecida" << std::endl;
	logFile << "[INFO] Conexion FTP establecida" << std::endl;
	
	bool ok = false;
	
	if (command == "list") {
		std::cout << "Listando archivos..." << std::endl;
		logFile << "[INFO] Listando archivos..." << std::endl;
		list_files(hFtp, remotePath);
		ok = true;
	} else if (command == "download-all") {
		std::cout << "Descargando todos los archivos..." << std::endl;
		logFile << "[INFO] Descargando todos los archivos..." << std::endl;
		ok = download_recursive(hFtp, remotePath, localPath);
	} else if (command == "upload-all") {
		std::cout << "Subiendo todos los archivos..." << std::endl;
		logFile << "[INFO] Subiendo todos los archivos..." << std::endl;
		ok = upload_recursive(hFtp, localPath, remotePath);
	} else if (command == "download" && argc > 2) {
		std::string file = argv[2];
		std::string remoteFile = remotePath + "/" + file;
		std::string localFile = localPath + "\\" + file;
		std::cout << "Descargando: " << remoteFile << " -> " << localFile << std::endl;
		logFile << "[INFO] Descargando: " << remoteFile << " -> " << localFile << std::endl;
		ok = FtpGetFileA(hFtp, remoteFile.c_str(), localFile.c_str(), FALSE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_BINARY, 0);
		if (!ok) {
			std::cerr << "Error descargando archivo" << std::endl;
			logFile << "[ERROR] Descargando archivo" << std::endl;
			print_last_error();
		}
	} else if (command == "upload" && argc > 2) {
		std::string file = argv[2];
		std::string localFile = localPath + "\\" + file;
		std::string remoteFile = remotePath + "/" + file;
		std::cout << "Subiendo: " << localFile << " -> " << remoteFile << std::endl;
		logFile << "[INFO] Subiendo: " << localFile << " -> " << remoteFile << std::endl;
		ok = FtpPutFileA(hFtp, localFile.c_str(), remoteFile.c_str(), FTP_TRANSFER_TYPE_BINARY, 0);
		if (!ok) {
			std::cerr << "Error subiendo archivo" << std::endl;
			logFile << "[ERROR] Subiendo archivo" << std::endl;
			print_last_error();
		}
	} else if (command == "mkdir" && argc > 2) {
		std::string dir = argv[2];
		std::string remoteDir = remotePath + "/" + dir;
		std::cout << "Creando directorio: " << remoteDir << std::endl;
		logFile << "[INFO] Creando directorio: " << remoteDir << std::endl;
		ok = FtpCreateDirectoryA(hFtp, remoteDir.c_str());
		if (!ok) {
			DWORD err = GetLastError();
			if (err == ERROR_ALREADY_EXISTS || err == ERROR_FILE_EXISTS) {
				std::cout << "Directorio ya existe" << std::endl;
				logFile << "[INFO] Directorio ya existe" << std::endl;
				ok = true;
			} else {
				std::cerr << "Error creando directorio" << std::endl;
				logFile << "[ERROR] Creando directorio" << std::endl;
				print_last_error();
			}
		}
	} else {
		std::cerr << "Comando no valido: " << command << std::endl;
		logFile << "[ERROR] Comando no valido: " << command << std::endl;
		show_usage(argv[0]);
		ok = false;
	}
	
	std::cout << "Cerrando conexiones..." << std::endl;
	logFile << "[INFO] Cerrando conexiones..." << std::endl;
	InternetCloseHandle(hFtp);
	InternetCloseHandle(hInternet);
	
	if (ok) {
		std::cout << "Operacion completada exitosamente" << std::endl;
		logFile << "[SUCCESS] Operacion completada exitosamente" << std::endl;
		return 0;
	} else {
		std::cout << "Operacion fallo" << std::endl;
		logFile << "[FAILURE] Operacion fallo" << std::endl;
		return 4;
	}
}
