#include <algorithm>
#include <string>
#include <cstdio>
#include "MozaFFB.h"

#ifdef _WIN32
#include <windows.h>
#include <hidsdi.h>
#include <setupapi.h>
static const GUID GUID_DEVCLASS_HID =
    {0x745a17a0, 0x74d3, 0x11d0, {0xb6, 0xfe, 0x00, 0xa0, 0xc9, 0x0f, 0x57, 0xda}};
#pragma comment(lib, "hid.lib")
#pragma comment(lib, "setupapi.lib")
#endif

namespace ks::device {

const std::vector<MozaFFB::DeviceInfo>& MozaFFB::knownDevices() {
    static const std::vector<DeviceInfo> devices = {
        {0x0001, WheelModel::R5,      "MOZA R5",       5.0f},
        {0x0002, WheelModel::R9,      "MOZA R9",       9.0f},
        {0x0003, WheelModel::R12,     "MOZA R12",     12.0f},
        {0x0004, WheelModel::R16,     "MOZA R16",     16.0f},
        {0x0005, WheelModel::R21,     "MOZA R21",     21.0f},
        {0x0006, WheelModel::R21F,    "MOZA R21F",    21.0f},
        {0x0010, WheelModel::MBoat,   "MOZA M Boat",   9.0f},
        {0x0011, WheelModel::MBoatPro,"MOZA M Boat Pro", 9.0f},
    };
    return devices;
}

float MozaFFB::maxTorqueForModel(WheelModel model) {
    for (const auto& dev : knownDevices()) {
        if (dev.model == model) return dev.maxTorque;
    }
    return 10.0f;
}

MozaFFB::MozaFFB() = default;

MozaFFB::~MozaFFB() {
    shutdown();
}

bool MozaFFB::initialize() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::fprintf(stderr, "MozaFFB: Initializing (USB HID)\n");

    if (!enumerateDevice()) {
        std::fprintf(stderr, "MozaFFB: No MOZA wheel found\n");
        return false;
    }

    m_connected = true;
    std::fprintf(stderr, "MozaFFB: Connected to %s (max %.1f Nm)\n",
                 modelName().c_str(), maxTorqueNm());
    return true;
}

bool MozaFFB::enumerateDevice() {
#ifdef _WIN32
    HDEVINFO devInfo = SetupDiGetClassDevs(
        &GUID_DEVCLASS_HID, nullptr, nullptr, DIGCF_PRESENT);
    if (devInfo == INVALID_HANDLE_VALUE) {
        std::fprintf(stderr, "MozaFFB: SetupDiGetClassDevs failed\n");
        return false;
    }

    SP_DEVICE_INTERFACE_DATA interfaceData;
    interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(devInfo, nullptr,
            &GUID_DEVCLASS_HID, i, &interfaceData); ++i) {

        DWORD requiredSize = 0;
        SetupDiGetDeviceInterfaceDetail(devInfo, &interfaceData, nullptr, 0, &requiredSize, nullptr);

        auto* detailData = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA*>(
            new char[requiredSize]);
        detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        if (!SetupDiGetDeviceInterfaceDetail(devInfo, &interfaceData,
                detailData, requiredSize, nullptr, nullptr)) {
            delete[] reinterpret_cast<char*>(detailData);
            continue;
        }

        HANDLE hidHandle = CreateFile(
            detailData->DevicePath,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr, OPEN_EXISTING, 0, nullptr);
        delete[] reinterpret_cast<char*>(detailData);

        if (hidHandle == INVALID_HANDLE_VALUE) continue;

        HIDD_ATTRIBUTES attributes;
        attributes.Size = sizeof(HIDD_ATTRIBUTES);
        if (HidD_GetAttributes(hidHandle, &attributes)) {
            if (attributes.VendorID == MOZA_VID) {
                for (const auto& dev : knownDevices()) {
                    if (attributes.ProductID == dev.pid) {
                        m_productId = dev.pid;
                        m_model = dev.model;
                        m_hidDevice = hidHandle;
                        std::fprintf(stderr, "MozaFFB: Found %s (PID: 0x%04x, %.1f Nm)\n",
                                     dev.name, dev.pid, dev.maxTorque);
                        SetupDiDestroyDeviceInfoList(devInfo);
                        return true;
                    }
                }
            }
        }
        CloseHandle(hidHandle);
    }

    SetupDiDestroyDeviceInfoList(devInfo);
    return false;
#else
    std::fprintf(stderr, "MozaFFB: HID enumeration not available on this platform\n");
    return false;
#endif
}

bool MozaFFB::sendFFBCommand(const uint8_t* data, size_t len) {
#ifdef _WIN32
    if (!m_hidDevice) return false;
    DWORD bytesWritten = 0;
    BOOL result = WriteFile(
        m_hidDevice, data, static_cast<DWORD>(len), &bytesWritten, nullptr);
    return result != FALSE;
#else
    (void)data; (void)len;
    return false;
#endif
}

void MozaFFB::updateFFB(float torqueNm) {
    if (!m_connected) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lastTorque = torqueNm;

    float maxNm = maxTorqueNm();
    float clampedTorque = std::clamp(torqueNm, -maxNm, maxNm);

    int32_t forceUnits = static_cast<int32_t>(clampedTorque * 100.0f);

    uint8_t report[64] = {};
    report[0] = 0x01;
    report[1] = 0x01;
    report[2] = static_cast<uint8_t>(forceUnits & 0xFF);
    report[3] = static_cast<uint8_t>((forceUnits >> 8) & 0xFF);
    report[4] = static_cast<uint8_t>((forceUnits >> 16) & 0xFF);
    report[5] = static_cast<uint8_t>((forceUnits >> 24) & 0xFF);

    sendFFBCommand(report, sizeof(report));
}

void MozaFFB::setConstantForce(float magnitude) {
    if (!m_connected) return;
    updateFFB(magnitude * maxTorqueNm());
}

void MozaFFB::setSpringForce(float center, float stiffness, float damping) {
    if (!m_connected) return;
    (void)center; (void)stiffness; (void)damping;

    uint8_t report[64] = {};
    report[0] = 0x01;
    report[1] = 0x02;
    report[2] = static_cast<uint8_t>(std::clamp(center, 0.0f, 1.0f) * 255);
    report[3] = static_cast<uint8_t>(std::clamp(stiffness, 0.0f, 1.0f) * 255);
    report[4] = static_cast<uint8_t>(std::clamp(damping, 0.0f, 1.0f) * 255);
    sendFFBCommand(report, sizeof(report));
}

void MozaFFB::setDamperForce(float velocity, float coefficient) {
    if (!m_connected) return;
    (void)velocity;

    uint8_t report[64] = {};
    report[0] = 0x01;
    report[1] = 0x03;
    report[2] = static_cast<uint8_t>(std::clamp(coefficient, 0.0f, 1.0f) * 255);
    sendFFBCommand(report, sizeof(report));
}

void MozaFFB::setFrictionForce(float coefficient) {
    if (!m_connected) return;

    uint8_t report[64] = {};
    report[0] = 0x01;
    report[1] = 0x04;
    report[2] = static_cast<uint8_t>(std::clamp(coefficient, 0.0f, 1.0f) * 255);
    sendFFBCommand(report, sizeof(report));
}

void MozaFFB::setRumble(float strongMotor, float weakMotor) {
    if (!m_connected) return;
    (void)strongMotor; (void)weakMotor;
}

void MozaFFB::processHIDInput() {
#ifdef _WIN32
    if (!m_hidDevice) return;
    uint8_t inputReport[64] = {};
    DWORD bytesRead = 0;
    ReadFile(m_hidDevice, inputReport, sizeof(inputReport), &bytesRead, nullptr);
#else
#endif
}

void MozaFFB::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);

#ifdef _WIN32
    if (m_connected) {
        // zero force without re-locking: write directly
        uint8_t report[64] = {};
        report[0] = 0x01;
        report[1] = 0x01;
        sendFFBCommand(report, sizeof(report));
    }
    if (m_hidDevice) {
        CloseHandle(static_cast<HANDLE>(m_hidDevice));
        m_hidDevice = nullptr;
    }
#endif

    m_connected = false;
    m_model = WheelModel::Unknown;
    m_productId = 0;
}

std::string MozaFFB::modelName() const {
    for (const auto& dev : knownDevices()) {
        if (dev.pid == m_productId) return std::string(dev.name);
    }
    return "MOZA (Unknown)";
}

float MozaFFB::maxTorqueNm() const {
    return maxTorqueForModel(m_model);
}

} // namespace ks::device
