#include <windows.h>
#include <wininet.h>
#include <iostream>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <string>
#ifdef USE_NLOHMANN_JSON
#include <nlohmann/json.hpp>
#endif

namespace fs = std::filesystem;

#pragma comment(lib, "wininet.lib")

void print_last_error() {
    DWORD err = GetLastError();
    char* msg = NULL;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&msg, 0, NULL);
    if (msg) {
        std::cout << "WinINet error: " << err << " - " << msg;
        LocalFree(msg);
        return;
    }
    std::cout << "WinINet error: " << err << std::endl;
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
			print_last_error();
			// NO return; solo loguea y sigue
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
            ensure_remote_path(hFtp, remotePath);
            if (!upload_recursive(hFtp, localPath, remotePath)) return false;
        } else {
            std::cout << "[FILE] " << localPath << " -> " << remotePath << std::endl;
            if (!FtpPutFileA(hFtp, localPath.c_str(), remotePath.c_str(), FTP_TRANSFER_TYPE_BINARY, 0)) {
                std::cerr << "Error subiendo: " << localPath << " -> " << remotePath << std::endl;
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
    HINTERNET hFind = FtpFindFirstFileA(hFtp, searchPath.c_str(), &findData, INTERNET_FLAG_RELOAD, 0);
    if (!hFind) {
        print_last_error();
        return;
    }
    std::cout << "Contenido remoto de " << (path.empty() ? "/" : path) << ":\n";
    do {
        std::cout << findData.cFileName << std::endl;
    } while (InternetFindNextFileA(hFind, &findData));
    InternetCloseHandle(hFind);
}

bool download_recursive(HINTERNET hFtp, const std::string& remote, const std::string& local) {
    WIN32_FIND_DATAA findData;
    std::string searchPath = remote + "/*";
    HINTERNET hFind = FtpFindFirstFileA(hFtp, searchPath.c_str(), &findData, INTERNET_FLAG_RELOAD, 0);
    if (!hFind) {
        print_last_error();
        return false;
    }
    do {
        std::string name = findData.cFileName;
        if (name == "." || name == "..") continue;
        std::string remotePath = remote + "/" + name;
        std::string localPath = local + "\\" + name;
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            CreateDirectoryA(localPath.c_str(), NULL);
            if (!download_recursive(hFtp, remotePath, localPath)) return false;
        } else {
            if (!FtpGetFileA(hFtp, remotePath.c_str(), localPath.c_str(), FALSE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_BINARY, 0)) {
                std::cerr << "Error descargando: " << remotePath << std::endl;
                print_last_error();
                return false;
            }
        }
    } while (InternetFindNextFileA(hFind, &findData));
    InternetCloseHandle(hFind);
    return true;
}

std::string get_json_value(const std::string& file, const std::string& key) {
	std::ifstream f(file);
	if (!f) return "";
#ifdef USE_NLOHMANN_JSON
	try {
		nlohmann::json j; f >> j;
		if (j.contains(key)) return j[key].get<std::string>();
	} catch (...) {}
#else
	std::string line, j;
	while (getline(f, line)) j += line;
	auto p = j.find("\"" + key + "\"");
	if (p == std::string::npos) return "";
	p = j.find(":", p); p = j.find_first_of("\"0123456789", p + 1);
	if (j[p] == '"') p++;
	auto end = j.find_first_of(j[p - 1] == '"' ? "\"" : ",}", p);
	return j.substr(p, end - p);
#endif
	return "";
}

int main(int argc, char* argv[]) {
	std::string localPath, remotePath;
	localPath = get_json_value(".ftp/config.json", "localPath");
	remotePath = get_json_value(".ftp/config.json", "remotePath");
	if (localPath.empty()) localPath = ".";
	if (remotePath.empty()) remotePath = "/public_html/jenni";

	if (argc < 5) {
		std::cerr << "Uso: " << argv[0] << " <host> <user> <pass> <op> [<local>] <remote>\n";
		std::cerr << "O usa config.json con 'localPath' y 'remotePath'\n";
		return 1;
	}
	std::string host = argv[1], user = argv[2], pass = argv[3], op = argv[4];
	std::string local = argc > 5 ? argv[5] : localPath;
	std::string remote = argc > 6 ? argv[6] : remotePath;
	HINTERNET hInternet = InternetOpenA("ftp_client", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (!hInternet) { print_last_error(); return 2; }
	HINTERNET hFtp = InternetConnectA(hInternet, host.c_str(), INTERNET_DEFAULT_FTP_PORT,
		user.c_str(), pass.c_str(), INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);
	if (!hFtp) { print_last_error(); InternetCloseHandle(hInternet); return 3; }
	bool ok = false;
	if (op == "upload") {
		if (fs::is_directory(local))
			ok = upload_recursive(hFtp, local, remote);
		else
			ok = FtpPutFileA(hFtp, local.c_str(), remote.c_str(), FTP_TRANSFER_TYPE_BINARY, 0);
	} else if (op == "download") {
		ok = FtpGetFileA(hFtp, remote.c_str(), local.c_str(), FALSE, FILE_ATTRIBUTE_NORMAL,
						 FTP_TRANSFER_TYPE_BINARY, 0);
	} else if (op == "list") {
		list_files(hFtp, remote); ok = true;
	} else if (op == "download-all") {
		ok = download_recursive(hFtp, remote, local);
	} else if (op == "mkdir") {
		ok = FtpCreateDirectoryA(hFtp, remote.c_str());
		if (!ok) {
			DWORD err = GetLastError();
			if (err == ERROR_ALREADY_EXISTS || err == ERROR_FILE_EXISTS) ok = true;
			else print_last_error();
		}
	} else {
		std::cerr << "Operación no válida: " << op << std::endl;
	}
	InternetCloseHandle(hFtp);
	InternetCloseHandle(hInternet);
	return ok ? 0 : 4;
}
