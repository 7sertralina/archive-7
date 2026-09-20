#pragma once
#include <Windows.h>
#include <string>

namespace HWIDChecker {
    struct HWIDSnapshot {
        std::string cpu_serial;
        std::string mac_address;        // NEU: statt GPU
        std::string ram_serial;
        std::string disk_serial;
        std::string machine_guid;
        std::string baseboard_serial;
        bool is_valid = false;
    };

    extern HWIDSnapshot original_snapshot;
    extern HWIDSnapshot current_snapshot;

    void InitializeChecker();
    void RefreshCurrentSnapshot();
    HWIDSnapshot CaptureHWIDSnapshot();

    // Getter für aktuelle Serials
    std::string GetCurrentCPUSerial();
    std::string GetCurrentMACAddress();  // NEU: statt GPU
    std::string GetCurrentRAMSerial();
    std::string GetCurrentDiskSerial();
    std::string GetCurrentMachineGUID();
    std::string GetCurrentBaseboardSerial();
}