#pragma once
#include <string>
#include <windows.h>
#include <intrin.h>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

namespace ui {
    namespace hardware {
        std::string get_cpu_info();
        std::string get_ram_info();
        std::string get_os_info();
        std::string get_gpu_info();
        std::string get_storage_info();
        std::string get_motherboard_info();
    }
}