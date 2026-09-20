#include "checker.h"
#include <comdef.h>
#include <Wbemidl.h>
#include <iphlpapi.h>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "advapi32.lib")

namespace HWIDChecker {
    HWIDSnapshot original_snapshot;
    HWIDSnapshot current_snapshot;

    std::string GetWMIProperty(const wchar_t* wmiClass, const wchar_t* property) {
        HRESULT hres;

        hres = CoInitializeEx(0, COINIT_MULTITHREADED);
        if (FAILED(hres)) return "N/A";

        hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
            RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);

        IWbemLocator* pLoc = NULL;
        hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
            IID_IWbemLocator, (LPVOID*)&pLoc);
        if (FAILED(hres)) {
            CoUninitialize();
            return "N/A";
        }

        IWbemServices* pSvc = NULL;
        hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
        if (FAILED(hres)) {
            pLoc->Release();
            CoUninitialize();
            return "N/A";
        }

        hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
            RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);

        std::wstring query = L"SELECT * FROM ";
        query += wmiClass;

        IEnumWbemClassObject* pEnumerator = NULL;
        hres = pSvc->ExecQuery(bstr_t("WQL"), bstr_t(query.c_str()),
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);

        std::string result = "N/A";
        if (SUCCEEDED(hres)) {
            IWbemClassObject* pclsObj = NULL;
            ULONG uReturn = 0;

            while (pEnumerator) {
                HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
                if (0 == uReturn) break;

                VARIANT vtProp;
                hr = pclsObj->Get(property, 0, &vtProp, 0, 0);
                if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR) {
                    _bstr_t bstrValue(vtProp.bstrVal);
                    result = (char*)bstrValue;
                }
                VariantClear(&vtProp);
                pclsObj->Release();
                break;
            }
        }

        pSvc->Release();
        pLoc->Release();
        pEnumerator->Release();
        CoUninitialize();

        return result;
    }

    std::string GetMACAddress() {
        IP_ADAPTER_INFO AdapterInfo[16];
        DWORD dwBufLen = sizeof(AdapterInfo);
        DWORD dwStatus = GetAdaptersInfo(AdapterInfo, &dwBufLen);

        if (dwStatus != ERROR_SUCCESS) return "N/A";

        PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;
        std::stringstream ss;

        ss << std::hex << std::uppercase << std::setfill('0')
            << std::setw(2) << (int)pAdapterInfo->Address[0] << ":"
            << std::setw(2) << (int)pAdapterInfo->Address[1] << ":"
            << std::setw(2) << (int)pAdapterInfo->Address[2] << ":"
            << std::setw(2) << (int)pAdapterInfo->Address[3] << ":"
            << std::setw(2) << (int)pAdapterInfo->Address[4] << ":"
            << std::setw(2) << (int)pAdapterInfo->Address[5];

        return ss.str();
    }

    std::string ShortenSerial(const std::string& serial, size_t maxLen = 12) {
        if (serial == "N/A" || serial.empty()) return "N/A";

        std::string cleaned;
        for (char c : serial) {
            if (isalnum(c)) cleaned += c;
        }

        if (cleaned.length() <= maxLen) return cleaned;

        return cleaned.substr(0, maxLen);
    }

    HWIDSnapshot CaptureHWIDSnapshot() {
        HWIDSnapshot snapshot;

        std::string cpu_raw = GetWMIProperty(L"Win32_Processor", L"ProcessorId");
        snapshot.cpu_serial = ShortenSerial(cpu_raw, 12);

        snapshot.mac_address = GetMACAddress();

        std::string ram_raw = GetWMIProperty(L"Win32_PhysicalMemory", L"SerialNumber");
        snapshot.ram_serial = ShortenSerial(ram_raw, 12);

        std::string disk_raw = GetWMIProperty(L"Win32_DiskDrive", L"SerialNumber");
        snapshot.disk_serial = ShortenSerial(disk_raw, 12);

        std::string uuid_raw = GetWMIProperty(L"Win32_ComputerSystemProduct", L"UUID");
        snapshot.machine_guid = ShortenSerial(uuid_raw, 12);

        std::string board_raw = GetWMIProperty(L"Win32_BaseBoard", L"SerialNumber");
        snapshot.baseboard_serial = ShortenSerial(board_raw, 12);

        snapshot.is_valid = true;

        return snapshot;
    }

    void InitializeChecker() {
        original_snapshot = CaptureHWIDSnapshot();
        current_snapshot = original_snapshot;
    }

    void RefreshCurrentSnapshot() {
        current_snapshot = CaptureHWIDSnapshot();
    }

    std::string GetCurrentCPUSerial() {
        return current_snapshot.cpu_serial;
    }

    std::string GetCurrentMACAddress() {
        return current_snapshot.mac_address;
    }

    std::string GetCurrentRAMSerial() {
        return current_snapshot.ram_serial;
    }

    std::string GetCurrentDiskSerial() {
        return current_snapshot.disk_serial;
    }

    std::string GetCurrentMachineGUID() {
        return current_snapshot.machine_guid;
    }

    std::string GetCurrentBaseboardSerial() {
        return current_snapshot.baseboard_serial;
    }
}