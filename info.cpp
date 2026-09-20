#include "info.h"
#include <cmath>

std::string ui::hardware::get_os_info() {
    HKEY hKey;
    DWORD dwType = REG_SZ;
    char buffer[256] = { 0 };
    DWORD bufferSize = sizeof(buffer);

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        std::string productName = "Windows";
        std::string displayVersion = "";
        std::string currentBuild = "";
        std::string releaseId = "";

        bufferSize = sizeof(buffer);
        if (RegQueryValueExA(hKey, "ProductName", NULL, &dwType, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
            productName = std::string(buffer);
        }

        bufferSize = sizeof(buffer);
        if (RegQueryValueExA(hKey, "DisplayVersion", NULL, &dwType, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
            displayVersion = std::string(buffer);
        }

        bufferSize = sizeof(buffer);
        if (RegQueryValueExA(hKey, "CurrentBuild", NULL, &dwType, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
            currentBuild = std::string(buffer);
        }

        bufferSize = sizeof(buffer);
        if (RegQueryValueExA(hKey, "UBR", NULL, &dwType, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
            std::string ubr = std::string(buffer);
            if (!ubr.empty() && ubr != "0") {
                currentBuild += "." + ubr;
            }
        }

        RegCloseKey(hKey);

        std::string windowsVersion;
        int buildNumber = currentBuild.empty() ? 0 : atoi(currentBuild.c_str());

        if (productName.find("Windows 11") != std::string::npos) {
            windowsVersion = "Windows 11";
        }
        else if (productName.find("Windows 10") != std::string::npos) {
            if (buildNumber >= 22000) {
                windowsVersion = "Windows 11";
            }
            else {
                windowsVersion = "Windows 10";
            }
        }
        else if (productName.find("Windows 8") != std::string::npos) {
            windowsVersion = "Windows 8";
        }
        else if (productName.find("Windows 7") != std::string::npos) {
            windowsVersion = "Windows 7";
        }
        else {
            windowsVersion = productName;
        }
        std::string hVersion = "";

        if (!displayVersion.empty()) {
            hVersion = displayVersion;
        }
        else if (!currentBuild.empty()) {
            if (windowsVersion == "Windows 11") {
                if (buildNumber >= 26000) hVersion = "24H2";
                else if (buildNumber >= 22631) hVersion = "23H2";
                else if (buildNumber >= 22621) hVersion = "22H2";
                else if (buildNumber >= 22000) hVersion = "21H2";
            }
            else if (windowsVersion == "Windows 10") {
                if (buildNumber >= 19045) hVersion = "22H2";
                else if (buildNumber >= 19044) hVersion = "21H2";
                else if (buildNumber >= 19043) hVersion = "21H1";
                else if (buildNumber >= 19042) hVersion = "20H2";
                else if (buildNumber >= 19041) hVersion = "2004";
                else if (buildNumber >= 18363) hVersion = "1909";
                else if (buildNumber >= 18362) hVersion = "1903";
                else if (buildNumber >= 17763) hVersion = "1809";
                else if (buildNumber >= 17134) hVersion = "1803";
                else if (buildNumber >= 16299) hVersion = "1709";
                else if (buildNumber >= 15063) hVersion = "1703";
                else if (buildNumber >= 14393) hVersion = "1607";
                else if (buildNumber >= 10586) hVersion = "1511";
                else if (buildNumber >= 10240) hVersion = "1507";
            }
        }
        std::string result = windowsVersion;
        if (!hVersion.empty()) {
            result += " " + hVersion;
        }

        return result;
    }
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (hNtdll) {
        typedef LONG(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
        RtlGetVersionPtr RtlGetVersion = (RtlGetVersionPtr)GetProcAddress(hNtdll, "RtlGetVersion");

        if (RtlGetVersion) {
            RTL_OSVERSIONINFOW versionInfo = { 0 };
            versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);

            if (RtlGetVersion(&versionInfo) == 0) {
                if (versionInfo.dwMajorVersion == 10 && versionInfo.dwMinorVersion == 0) {
                    if (versionInfo.dwBuildNumber >= 22000) {
                        return "Windows 11";
                    }
                    else {
                        return "Windows 10";
                    }
                }
            }
        }
    }

    return "Windows";
}

std::string ui::hardware::get_cpu_info() {
    char cpu_brand[0x40] = { 0 };
    int cpu_info[4] = { -1 };

    __cpuid(cpu_info, 0x80000002);
    memcpy(cpu_brand, cpu_info, sizeof(cpu_info));

    __cpuid(cpu_info, 0x80000003);
    memcpy(cpu_brand + 16, cpu_info, sizeof(cpu_info));

    __cpuid(cpu_info, 0x80000004);
    memcpy(cpu_brand + 32, cpu_info, sizeof(cpu_info));

    std::string result(cpu_brand);
    result.erase(result.find_last_not_of(" \t\n\r\f\v") + 1);
    return result.empty() ? "Unknown CPU" : result;
}

std::string ui::hardware::get_ram_info() {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);

    if (GlobalMemoryStatusEx(&memInfo)) {
        DWORDLONG totalPhysMem = memInfo.ullTotalPhys;
        double totalGB = totalPhysMem / (1024.0 * 1024.0 * 1024.0);

        char buffer[64];
        if (totalGB < 1.0) {
            double totalMB = totalGB * 1024.0;
            snprintf(buffer, sizeof(buffer), "%.0fMB", totalMB);
        }
        else if (totalGB >= 1000.0) {
            double totalTB = totalGB / 1024.0;
            snprintf(buffer, sizeof(buffer), "%.1fTB", totalTB);
        }
        else {
            snprintf(buffer, sizeof(buffer), "%.1fGB", totalGB);
        }
        return std::string(buffer);
    }
    return "Unknown RAM";
}

std::string ui::hardware::get_gpu_info() {
    DISPLAY_DEVICE displayDevice;
    ZeroMemory(&displayDevice, sizeof(displayDevice));
    displayDevice.cb = sizeof(DISPLAY_DEVICE);

    if (EnumDisplayDevices(NULL, 0, &displayDevice, EDD_GET_DEVICE_INTERFACE_NAME)) {
        char gpuName[128] = { 0 };
        WideCharToMultiByte(CP_UTF8, 0, displayDevice.DeviceString, -1, gpuName, sizeof(gpuName), NULL, NULL);

        std::string result = gpuName;
        return result.empty() ? "Unknown GPU" : result;
    }

    return "Unknown GPU";
}

std::string ui::hardware::get_storage_info() {
    ULARGE_INTEGER totalBytes = { 0 };
    ULARGE_INTEGER freeBytes = { 0 };

    wchar_t drive[] = L"C:\\";

    if (GetDiskFreeSpaceExW(drive, NULL, &totalBytes, &freeBytes)) {
        double totalGB = static_cast<double>(totalBytes.QuadPart) / (1024.0 * 1024.0 * 1024.0);

        char buffer[64];
        if (totalGB >= 1000.0) {
            double totalTB = totalGB / 1024.0;
            snprintf(buffer, sizeof(buffer), "%.1fTB", totalTB);
        }
        else if (totalGB < 10.0) {
            snprintf(buffer, sizeof(buffer), "%.2fGB", totalGB);
        }
        else if (totalGB < 100.0) {
            snprintf(buffer, sizeof(buffer), "%.1fGB", totalGB);
        }
        else {
            snprintf(buffer, sizeof(buffer), "%.0fGB", totalGB);
        }
        return std::string(buffer);
    }

    return "Unknown Storage";
}
static std::string truncate(const std::string& str, size_t max_len) {
    if (str.size() <= max_len) return str;
    return str.substr(0, max_len - 3) + "...";
}
std::string ui::hardware::get_motherboard_info() {
    HKEY hKey;
    DWORD dwType = REG_SZ;
    char buffer[256] = { 0 };
    DWORD bufferSize = sizeof(buffer);

    std::string manufacturer = "Unknown";
    std::string product = "Motherboard";
    bool foundInfo = false;

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\BIOS", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        // Hersteller
        bufferSize = sizeof(buffer);
        if (RegQueryValueExA(hKey, "BaseBoardManufacturer", NULL, &dwType, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
            manufacturer = std::string(buffer);
            foundInfo = true;
        }

        // Produkt
        bufferSize = sizeof(buffer);
        if (RegQueryValueExA(hKey, "BaseBoardProduct", NULL, &dwType, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
            product = std::string(buffer);
            foundInfo = true;
        }
        RegCloseKey(hKey);
    }

    if (foundInfo) {
        if (manufacturer == "Unknown" && product != "Motherboard") {
            return truncate(product, 24);
        }
        else if (manufacturer != "Unknown" && product == "Motherboard") {
            return truncate(manufacturer, 24);
        }
        else {
            return truncate(manufacturer + " " + product, 24);
        }
    }

    return "Unknown Motherboard";
}