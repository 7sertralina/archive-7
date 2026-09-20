#pragma once

/*
================================================================================
    AUTH HYPERX - Sistema de Autenticacao Completo
    Versao: 2.0
    
    Funcionalidades:
    - Login por Credenciais (login + senha)
    - Auto-Login por HWID
    - Heartbeat (manter sessao ativa)
    - Logout
    
    Dependencias: Nenhuma externa (apenas Windows APIs)
    Biblioteca HTTP: WinINet (nativa do Windows)
================================================================================
*/

#include <iostream>
#include <windows.h>
#include <wininet.h>
#include <string>
#include <thread>
#include <atomic>
#include <fstream>
#include <shlobj.h>
#include <functional>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "shell32.lib")

namespace AuthSystem {

    // ============================================================================
    //  CONFIGURACOES - ALTERE CONFORME SEU SERVIDOR
    // ============================================================================
    
    inline const char* API_HOST     = "authhyperx.discloud.app";
    inline const char* PRODUCT_NAME = "Spoofer Vanity";
    inline const int   API_PORT     = INTERNET_DEFAULT_HTTPS_PORT;
    
    inline const int HEARTBEAT_INTERVAL_MS = 60000;

    // ============================================================================
    //  ESTRUTURAS DE DADOS
    // ============================================================================
    
    struct UserData {
        std::string username;
        std::string login;
        std::string password;
        std::string expiry;
        std::string role;
        std::string product;
        std::string hwid;
        bool isAuthenticated = false;
    };
    
    struct AuthResponse {
        bool success = false;
        std::string message;
        std::string username;
        std::string expiry;
        std::string role;
        std::string product;
        std::string rawJson;
    };

    // ============================================================================
    //  VARIAVEIS GLOBAIS DO SISTEMA
    // ============================================================================
    
    inline UserData g_CurrentUser;
    inline std::atomic<bool> g_HeartbeatRunning(false);
    inline std::thread g_HeartbeatThread;
    
    inline std::function<void(const std::string&)> g_OnAuthSuccess = nullptr;
    inline std::function<void(const std::string&)> g_OnAuthError = nullptr;
    inline std::function<void()> g_OnLogout = nullptr;

    // ============================================================================
    //  UTILITARIOS - HWID
    // ============================================================================
    
    inline std::string GetHWID() {
        HW_PROFILE_INFO hwProfileInfo;
        if (GetCurrentHwProfile(&hwProfileInfo)) {
            // Convert WCHAR to std::string
            char buffer[80] = {};
            WideCharToMultiByte(CP_UTF8, 0, hwProfileInfo.szHwProfileGuid, -1,
                                buffer, sizeof(buffer), NULL, NULL);
            return std::string(buffer);
        }
        return "unknown-hwid";
    }

    // ============================================================================
    //  UTILITARIOS - CAMINHO PARA SALVAR CREDENCIAIS
    // ============================================================================
    
    inline std::string GetCredentialsPath() {
        char appDataPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appDataPath))) {
            std::string path = std::string(appDataPath) + "\\AuthHyperX";
            CreateDirectoryA(path.c_str(), NULL);
            return path + "\\credentials.dat";
        }
        return "credentials.dat";
    }

    // ============================================================================
    //  UTILITARIOS - PARSING JSON SIMPLES
    // ============================================================================
    
    inline std::string ExtractJsonValue(const std::string& json, const std::string& key) {
        std::string searchKey = "\"" + key + "\":";
        size_t pos = json.find(searchKey);
        if (pos == std::string::npos) return "";
        
        pos += searchKey.length();
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
        if (pos >= json.length()) return "";
        
        if (json[pos] == '"') {
            pos++;
            size_t end = json.find('"', pos);
            if (end != std::string::npos) {
                return json.substr(pos, end - pos);
            }
        } else {
            size_t end = pos;
            while (end < json.length() && json[end] != ',' && json[end] != '}' && json[end] != ' ') {
                end++;
            }
            return json.substr(pos, end - pos);
        }
        return "";
    }
    
    inline bool JsonHasSuccess(const std::string& json) {
        return json.find("\"success\":true") != std::string::npos;
    }

    // ============================================================================
    //  UTILITARIOS - XOR PARA OFUSCAR CREDENCIAIS SALVAS
    // ============================================================================
    
    inline std::string XorEncrypt(const std::string& data, const std::string& key) {
        std::string result = data;
        for (size_t i = 0; i < data.size(); i++) {
            result[i] = data[i] ^ key[i % key.size()];
        }
        return result;
    }

    // ============================================================================
    //  HTTP REQUEST - USANDO WININET
    // ============================================================================
    
    inline std::string HttpPost(const std::string& endpoint, const std::string& jsonBody, bool& success) {
        success = false;
        std::string response;
        
        HINTERNET hInternet = InternetOpenA(
            "AuthHyperX-Loader/2.0",
            INTERNET_OPEN_TYPE_DIRECT,
            NULL, NULL, 0
        );
        
        if (!hInternet) {
            return "{\"success\":false,\"error\":\"InternetOpen failed\"}";
        }
        
        HINTERNET hConnect = InternetConnectA(
            hInternet,
            API_HOST,
            API_PORT,
            NULL, NULL,
            INTERNET_SERVICE_HTTP,
            0, 0
        );
        
        if (!hConnect) {
            InternetCloseHandle(hInternet);
            return "{\"success\":false,\"error\":\"InternetConnect failed\"}";
        }
        
        HINTERNET hRequest = HttpOpenRequestA(
            hConnect,
            "POST",
            endpoint.c_str(),
            NULL, NULL, NULL,
            INTERNET_FLAG_SECURE | INTERNET_FLAG_NO_CACHE_WRITE,
            0
        );
        
        if (!hRequest) {
            InternetCloseHandle(hConnect);
            InternetCloseHandle(hInternet);
            return "{\"success\":false,\"error\":\"HttpOpenRequest failed\"}";
        }
        
        std::string headers = "Content-Type: application/json\r\n";
        
        BOOL sent = HttpSendRequestA(
            hRequest,
            headers.c_str(),
            (DWORD)headers.length(),
            (LPVOID)jsonBody.c_str(),
            (DWORD)jsonBody.length()
        );
        
        if (sent) {
            char buffer[4096];
            DWORD bytesRead;
            response = "";
            
            while (InternetReadFile(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                response += buffer;
            }
            success = true;
        } else {
            response = "{\"success\":false,\"error\":\"HttpSendRequest failed: " + std::to_string(GetLastError()) + "\"}";
        }
        
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        
        return response;
    }

    // ============================================================================
    //  SALVAR/CARREGAR CREDENCIAIS
    // ============================================================================
    
    inline bool SaveCredentials(const std::string& login, const std::string& password) {
        std::string path = GetCredentialsPath();
        std::string key = GetHWID();
        std::string data = login + "\n" + password;
        std::string encrypted = XorEncrypt(data, key);
        
        std::ofstream file(path, std::ios::binary);
        if (file.is_open()) {
            file.write(encrypted.c_str(), encrypted.size());
            file.close();
            return true;
        }
        return false;
    }
    
    inline bool LoadCredentials(std::string& login, std::string& password) {
        std::string path = GetCredentialsPath();
        std::string key = GetHWID();
        
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) return false;
        
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        std::string encrypted(size, '\0');
        if (!file.read(&encrypted[0], size)) {
            file.close();
            return false;
        }
        file.close();
        
        std::string decrypted = XorEncrypt(encrypted, key);
        size_t sep = decrypted.find('\n');
        if (sep == std::string::npos) return false;
        
        login = decrypted.substr(0, sep);
        password = decrypted.substr(sep + 1);
        return !login.empty();
    }
    
    inline void ClearCredentials() {
        std::string path = GetCredentialsPath();
        DeleteFileA(path.c_str());
    }

    // ============================================================================
    //  HEARTBEAT
    // ============================================================================
    
    inline void HeartbeatLoop() {
        while (g_HeartbeatRunning.load()) {
            if (g_CurrentUser.isAuthenticated && !g_CurrentUser.login.empty()) {
                std::string json = "{";
                json += "\"login\":\"" + g_CurrentUser.login + "\",";
                json += "\"product\":\"" + std::string(PRODUCT_NAME) + "\",";
                json += "\"hwid\":\"" + GetHWID() + "\"";
                json += "}";
                
                bool httpSuccess;
                HttpPost("/api/heartbeat", json, httpSuccess);
            }
            
            for (int i = 0; i < HEARTBEAT_INTERVAL_MS / 1000 && g_HeartbeatRunning.load(); i++) {
                Sleep(1000);
            }
        }
    }
    
    inline void StartHeartbeat() {
        if (!g_HeartbeatRunning.load()) {
            g_HeartbeatRunning.store(true);
            g_HeartbeatThread = std::thread(HeartbeatLoop);
            g_HeartbeatThread.detach();
        }
    }
    
    inline void StopHeartbeat() {
        g_HeartbeatRunning.store(false);
    }

    // ============================================================================
    //  LOGOUT
    // ============================================================================
    
    inline void Logout() {
        StopHeartbeat();
        
        if (g_CurrentUser.isAuthenticated && !g_CurrentUser.login.empty()) {
            std::string json = "{";
            json += "\"login\":\"" + g_CurrentUser.login + "\",";
            json += "\"product\":\"" + std::string(PRODUCT_NAME) + "\",";
            json += "\"hwid\":\"" + GetHWID() + "\"";
            json += "}";
            
            bool httpSuccess;
            HttpPost("/api/logout", json, httpSuccess);
        }
        
        g_CurrentUser = UserData();
        ClearCredentials();
        
        if (g_OnLogout) g_OnLogout();
    }

    // ============================================================================
    //  AUTENTICACAO POR LOGIN E SENHA
    // ============================================================================
    
    inline AuthResponse AuthenticateLogin(const std::string& login, const std::string& password, bool rememberMe = true) {
        AuthResponse result;
        
        std::string hwid = GetHWID();
        
        std::string json = "{";
        json += "\"login\":\"" + login + "\",";
        json += "\"password\":\"" + password + "\",";
        json += "\"hwid\":\"" + hwid + "\",";
        json += "\"product\":\"" + std::string(PRODUCT_NAME) + "\"";
        json += "}";
        
        bool httpSuccess;
        std::string response = HttpPost("/api/auth", json, httpSuccess);
        result.rawJson = response;
        
        if (!httpSuccess) {
            result.success = false;
            result.message = "Connection error.";
            if (g_OnAuthError) g_OnAuthError(result.message);
            return result;
        }
        
        if (response.find("<!DOCTYPE") != std::string::npos || response.find("<html") != std::string::npos) {
            result.success = false;
            result.message = "Server returned invalid page (404).";
            if (g_OnAuthError) g_OnAuthError(result.message);
            return result;
        }
        
        result.success = JsonHasSuccess(response);
        
        if (result.success) {
            result.username = ExtractJsonValue(response, "username");
            result.expiry = ExtractJsonValue(response, "expiry");
            result.role = ExtractJsonValue(response, "role");
            result.product = ExtractJsonValue(response, "product");
            result.message = "Login successful!";
            
            g_CurrentUser.isAuthenticated = true;
            g_CurrentUser.login = login;
            g_CurrentUser.password = password;
            g_CurrentUser.username = result.username.empty() ? login : result.username;
            g_CurrentUser.expiry = result.expiry;
            g_CurrentUser.role = result.role;
            g_CurrentUser.product = result.product;
            g_CurrentUser.hwid = hwid;
            
            if (rememberMe) SaveCredentials(login, password);
            StartHeartbeat();
            
            if (g_OnAuthSuccess) g_OnAuthSuccess(result.message);
        } else {
            result.message = ExtractJsonValue(response, "error");
            if (result.message.empty()) result.message = "Authentication failed.";
            if (g_OnAuthError) g_OnAuthError(result.message);
        }
        
        return result;
    }

    // ============================================================================
    //  AUTENTICACAO POR KEY
    // ============================================================================
    
    inline AuthResponse AuthenticateKey(const std::string& key) {
        AuthResponse result;
        
        std::string hwid = GetHWID();
        
        std::string json = "{";
        json += "\"key\":\"" + key + "\",";
        json += "\"product\":\"" + std::string(PRODUCT_NAME) + "\",";
        json += "\"hwid\":\"" + hwid + "\"";
        json += "}";
        
        bool httpSuccess;
        std::string response = HttpPost("/api/auth-key", json, httpSuccess);
        result.rawJson = response;
        
        if (!httpSuccess) {
            result.success = false;
            result.message = "Connection error.";
            if (g_OnAuthError) g_OnAuthError(result.message);
            return result;
        }
        
        if (response.find("<!DOCTYPE") != std::string::npos || response.find("<html") != std::string::npos) {
            result.success = false;
            result.message = "Server returned invalid page (404).";
            if (g_OnAuthError) g_OnAuthError(result.message);
            return result;
        }
        
        result.success = JsonHasSuccess(response);
        
        if (result.success) {
            result.username = ExtractJsonValue(response, "username");
            result.expiry = ExtractJsonValue(response, "expiry");
            result.role = ExtractJsonValue(response, "role");
            result.product = ExtractJsonValue(response, "product");
            result.message = "Key valid!";
            
            g_CurrentUser.isAuthenticated = true;
            g_CurrentUser.login = key;
            g_CurrentUser.username = result.username.empty() ? key : result.username;
            g_CurrentUser.expiry = result.expiry;
            g_CurrentUser.role = result.role;
            g_CurrentUser.product = result.product;
            g_CurrentUser.hwid = hwid;
            
            StartHeartbeat();
            if (g_OnAuthSuccess) g_OnAuthSuccess(result.message);
        } else {
            result.message = ExtractJsonValue(response, "error");
            if (result.message.empty()) result.message = "Invalid or expired key.";
            if (g_OnAuthError) g_OnAuthError(result.message);
        }
        
        return result;
    }

    // ============================================================================
    //  AUTO-LOGIN
    // ============================================================================
    
    inline AuthResponse TryAutoLogin() {
        AuthResponse result;
        std::string login, password;
        if (!LoadCredentials(login, password)) {
            result.success = false;
            result.message = "No saved credentials.";
            return result;
        }
        return AuthenticateLogin(login, password, false);
    }

    // ============================================================================
    //  GETTERS
    // ============================================================================
    
    inline bool IsAuthenticated() { return g_CurrentUser.isAuthenticated; }
    inline std::string GetUsername() { return g_CurrentUser.username; }
    inline std::string GetExpiry() { return g_CurrentUser.expiry; }
    inline std::string GetRole() { return g_CurrentUser.role; }
    inline std::string GetProduct() { return g_CurrentUser.product; }
    inline const UserData& GetUserData() { return g_CurrentUser; }

} // namespace AuthSystem
