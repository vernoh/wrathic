#pragma once
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <intrin.h>
#include <iphlpapi.h>
#include <string>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "iphlpapi.lib")

// Simple hash function
static unsigned long long hashString(const std::wstring& str) {
    unsigned long long hash = 14695981039346656037ULL;
    for (wchar_t c : str) {
        hash ^= (unsigned long long)c;
        hash *= 1099511628211ULL;
    }
    return hash;
}

// Get CPU ID
static std::wstring getCPUID() {
    int cpuInfo[4] = {};
    __cpuid(cpuInfo, 1);
    std::wostringstream ss;
    ss << std::hex << cpuInfo[0] << cpuInfo[1] << cpuInfo[2] << cpuInfo[3];
    return ss.str();
}

// Get MAC address of first adapter
static std::wstring getMACAddress() {
    IP_ADAPTER_INFO adapterInfo[16];
    DWORD bufLen = sizeof(adapterInfo);
    if (GetAdaptersInfo(adapterInfo, &bufLen) != ERROR_SUCCESS)
        return L"NOMAC";
    std::wostringstream ss;
    for (int i = 0; i < 6; i++)
        ss << std::hex << std::setw(2) << std::setfill(L'0') << adapterInfo[0].Address[i];
    return ss.str();
}

// Get volume serial number of C drive
static std::wstring getVolumeSerial() {
    DWORD serial = 0;
    GetVolumeInformation(L"C:\\", NULL, 0, &serial, NULL, NULL, NULL, 0);
    std::wostringstream ss;
    ss << std::hex << serial;
    return ss.str();
}

// Generate final HWID — stable across reboots, unique per machine
static std::wstring generateHWID() {
    std::wstring raw = getCPUID() + L"-" + getMACAddress() + L"-" + getVolumeSerial();
    unsigned long long hash = hashString(raw);
    std::wostringstream ss;
    ss << std::uppercase << std::hex << std::setw(16) << std::setfill(L'0') << hash;
    std::wstring h = ss.str();
    // Format as XXXX-XXXX-XXXX-XXXX
    return h.substr(0,4) + L"-" + h.substr(4,4) + L"-" + h.substr(8,4) + L"-" + h.substr(12,4);
}
