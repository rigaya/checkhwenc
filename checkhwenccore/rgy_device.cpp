﻿// -----------------------------------------------------------------------------------------
// QSVEnc/NVEnc by rigaya
// -----------------------------------------------------------------------------------------
// The MIT License
//
// Copyright (c) 2011-2016 rigaya
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// -------------------------------------------------------------------------------------------

#include <set>
#include "rgy_tchar.h"
#include "rgy_device.h"
#include "rgy_log.h"
#include "rgy_util.h"

#if ENABLE_D3D9
#pragma comment(lib, "d3d9.lib")
#endif
#if ENABLE_D3D11
#pragma comment(lib, "d3d11.lib")
#endif

#define CHECK_HRESULT_ERROR_RETURN(hr, mes) { \
    if (hr != S_OK) { \
        AddMessage(RGY_LOG_ERROR, _T("%s: %x\n"), mes, hr); \
        return RGY_ERR_UNKNOWN; \
    } \
}

#if ENABLE_D3D9
DeviceDX9::DeviceDX9() :
    m_name(_T("devDX9")),
    m_pD3D(),
    m_pD3DDevice(),
    m_devLUID(),
    m_adaptersCount(0),
    m_adaptersIndexes(),
    m_displayDeviceName(),
    m_log() {
    memset(m_adaptersIndexes, 0, sizeof(m_adaptersIndexes));
}

DeviceDX9::~DeviceDX9() {
    Terminate();
}

bool DeviceDX9::isValid() const {
    return m_pD3DDevice != nullptr;
}

ATL::CComPtr<IDirect3DDevice9> DeviceDX9::GetDevice() {
    return m_pD3DDevice;
}

RGY_ERR DeviceDX9::Init(bool dx9ex, int adapterID, uint32_t vendorID, bool bFullScreen, int width, int height, shared_ptr<RGYLog> log) {
    m_log = (log) ? log : std::make_shared<RGYLog>(nullptr, RGY_LOG_ERROR);
    HRESULT hr = S_OK;
    ATL::CComPtr<IDirect3D9Ex> pD3DEx;
    if (dx9ex) {
        hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &pD3DEx);
        CHECK_HRESULT_ERROR_RETURN(hr, L"Direct3DCreate9Ex Failed");
        m_pD3D = pD3DEx;
    } else {
        m_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
        m_pD3D.p->Release(); // fixed leak
    }

    EnumerateAdapters(vendorID);

    if (m_adaptersCount <= adapterID) {
        AddMessage(RGY_LOG_ERROR, _T("Invalid Adapter ID: %d"), adapterID);
        return RGY_ERR_NOT_FOUND;
    }

    //convert logical id to real index
    adapterID = m_adaptersIndexes[adapterID];
    D3DADAPTER_IDENTIFIER9 adapterIdentifier = { 0 };
    hr = m_pD3D->GetAdapterIdentifier(adapterID, 0, &adapterIdentifier);
    CHECK_HRESULT_ERROR_RETURN(hr, L"m_pD3D->GetAdapterIdentifier Failed");

    //Get LUID
    pD3DEx->GetAdapterLUID((UINT)adapterID, &m_devLUID);

    std::wstringstream wstrDeviceName; wstrDeviceName << adapterIdentifier.DeviceName;
    m_displayDeviceName = wstrDeviceName.str();

    char strDevice[100];
    _snprintf_s(strDevice, 100, "%X", adapterIdentifier.DeviceId);

    AddMessage(RGY_LOG_DEBUG, _T("DX9 : Chosen Device %d: Device ID: %s [ %s ]"), adapterID, char_to_tstring(strDevice).c_str(), char_to_tstring(adapterIdentifier.Description).c_str());

    D3DDISPLAYMODE d3ddm;
    hr = m_pD3D->GetAdapterDisplayMode((UINT)adapterID, &d3ddm);
    CHECK_HRESULT_ERROR_RETURN(hr, L"m_pD3D->GetAdapterDisplayMode Failed");

    D3DPRESENT_PARAMETERS               presentParameters;
    ZeroMemory(&presentParameters, sizeof(presentParameters));

    if (bFullScreen) {
        width = d3ddm.Width;
        height = d3ddm.Height;

        presentParameters.BackBufferWidth = width;
        presentParameters.BackBufferHeight = height;
        presentParameters.Windowed = FALSE;
        presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
        presentParameters.FullScreen_RefreshRateInHz = d3ddm.RefreshRate;
    } else {
        presentParameters.BackBufferWidth = 1;
        presentParameters.BackBufferHeight = 1;
        presentParameters.Windowed = TRUE;
        presentParameters.SwapEffect = D3DSWAPEFFECT_COPY;
    }
    presentParameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    presentParameters.hDeviceWindow = GetDesktopWindow();
    presentParameters.Flags = D3DPRESENTFLAG_VIDEO;
    presentParameters.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

    D3DDISPLAYMODEEX dismodeEx;
    dismodeEx.Size = sizeof(dismodeEx);
    dismodeEx.Format = presentParameters.BackBufferFormat;
    dismodeEx.Width = width;
    dismodeEx.Height = height;
    dismodeEx.RefreshRate = d3ddm.RefreshRate;
    dismodeEx.ScanLineOrdering = D3DSCANLINEORDERING_PROGRESSIVE;

    D3DCAPS9 ddCaps;
    ZeroMemory(&ddCaps, sizeof(ddCaps));
    hr = m_pD3D->GetDeviceCaps((UINT)adapterID, D3DDEVTYPE_HAL, &ddCaps);

    CHECK_HRESULT_ERROR_RETURN(hr, L"m_pD3D->GetDeviceCaps Failed");

    DWORD vp = 0;
    if (ddCaps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) {
        vp = D3DCREATE_HARDWARE_VERTEXPROCESSING;
    } else {
        vp = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    }
    if (dx9ex) {
        ATL::CComPtr<IDirect3DDevice9Ex> pD3DDeviceEx;

        hr = pD3DEx->CreateDeviceEx(
            adapterID,
            D3DDEVTYPE_HAL,
            presentParameters.hDeviceWindow,
            vp | D3DCREATE_NOWINDOWCHANGES | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE,
            &presentParameters,
            bFullScreen ? &dismodeEx : NULL,
            &pD3DDeviceEx
        );
        CHECK_HRESULT_ERROR_RETURN(hr, L"m_pD3D->CreateDeviceEx() failed");
        m_pD3DDevice = pD3DDeviceEx;
    } else {
        hr = m_pD3D->CreateDevice(
            adapterID,
            D3DDEVTYPE_HAL,
            presentParameters.hDeviceWindow,
            vp | D3DCREATE_NOWINDOWCHANGES | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE,
            &presentParameters,
            &m_pD3DDevice
        );
        CHECK_HRESULT_ERROR_RETURN(hr, L"m_pD3D->CreateDevice() failed");
    }
    return RGY_ERR_NONE;
}
RGY_ERR DeviceDX9::Terminate() {
    AddMessage(RGY_LOG_DEBUG, _T("Closing DX9 device...\n"));
    m_pD3DDevice.Release();
    m_pD3D.Release();
    return RGY_ERR_NONE;
}

RGY_ERR DeviceDX9::EnumerateAdapters(const uint32_t vendorID) {
    UINT count=0;
    m_adaptersCount = 0;
    ATL::CComPtr<IDirect3D9Ex> pD3DEx;
    {
        HRESULT hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &pD3DEx);
        CHECK_HRESULT_ERROR_RETURN(hr, L"Direct3DCreate9Ex Failed");
    }
    std::vector<LUID> enumeratedAdapterLUIDs;
    while (true) {
        D3DDISPLAYMODE displayMode;
        HRESULT hr = pD3DEx->EnumAdapterModes(count, D3DFMT_X8R8G8B8, 0, &displayMode);

        if (hr != D3D_OK && hr != D3DERR_NOTAVAILABLE) {
            break;
        }
        D3DADAPTER_IDENTIFIER9 adapterIdentifier ={ 0 };
        pD3DEx->GetAdapterIdentifier(count, 0, &adapterIdentifier);

        if (adapterIdentifier.VendorId != vendorID) {
            count++;
            continue;
        }

        LUID adapterLuid;
        pD3DEx->GetAdapterLUID(count, &adapterLuid);
        bool enumerated = false;
        for (auto it = enumeratedAdapterLUIDs.begin(); it != enumeratedAdapterLUIDs.end(); it++) {
            if (adapterLuid.HighPart == it->HighPart && adapterLuid.LowPart == it->LowPart) {
                enumerated = true;
                break;
            }
        }
        if (enumerated) {
            count++;
            continue;
        }

        enumeratedAdapterLUIDs.push_back(adapterLuid);

        char strDevice[100];
        _snprintf_s(strDevice, 100, "%X", adapterIdentifier.DeviceId);

        m_adaptersIndexes[m_adaptersCount] = count;
        m_adaptersCount++;
        count++;
    }
    return RGY_ERR_NONE;
}

int DeviceDX9::adapterCount(const uint32_t vendorID) {
    int count = 0;
    ATL::CComPtr<IDirect3D9Ex> pD3DEx;
    {
        HRESULT hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &pD3DEx);
        if (hr != S_OK) return 0;
    }
    std::vector<LUID> enumeratedAdapterLUIDs;
    while (true) {
        D3DDISPLAYMODE displayMode;
        HRESULT hr = pD3DEx->EnumAdapterModes(count, D3DFMT_X8R8G8B8, 0, &displayMode);

        if (hr != D3D_OK && hr != D3DERR_NOTAVAILABLE) {
            break;
        }
        D3DADAPTER_IDENTIFIER9 adapterIdentifier = { 0 };
        pD3DEx->GetAdapterIdentifier(count, 0, &adapterIdentifier);

        if (adapterIdentifier.VendorId != vendorID) {
            count++;
            continue;
        }

        LUID adapterLuid;
        pD3DEx->GetAdapterLUID(count, &adapterLuid);
        bool enumerated = false;
        for (auto it = enumeratedAdapterLUIDs.begin(); it != enumeratedAdapterLUIDs.end(); it++) {
            if (adapterLuid.HighPart == it->HighPart && adapterLuid.LowPart == it->LowPart) {
                enumerated = true;
                break;
            }
        }
        if (enumerated) {
            count++;
            continue;
        }

        enumeratedAdapterLUIDs.push_back(adapterLuid);
        count++;
    }
    return (int)enumeratedAdapterLUIDs.size();
}

void DeviceDX9::AddMessage(RGYLogLevel log_level, const tstring &str) {
    if (m_log == nullptr || log_level < m_log->getLogLevel(RGY_LOGT_DEV)) {
        return;
    }
    auto lines = split(str, _T("\n"));
    for (const auto &line : lines) {
        if (line[0] != _T('\0')) {
            m_log->write(log_level, RGY_LOGT_DEV, (m_name + _T(": ") + line + _T("\n")).c_str());
        }
    }
}
void DeviceDX9::AddMessage(RGYLogLevel log_level, const TCHAR *format, ...) {
    if (m_log == nullptr || log_level < m_log->getLogLevel(RGY_LOGT_DEV)) {
        return;
    }

    va_list args;
    va_start(args, format);
    int len = _vsctprintf(format, args) + 1; // _vscprintf doesn't count terminating '\0'
    tstring buffer;
    buffer.resize(len, _T('\0'));
    _vstprintf_s(&buffer[0], len, format, args);
    va_end(args);
    AddMessage(log_level, buffer);
}
#endif

#if ENABLE_D3D11
DeviceDX11::DeviceDX11() :
    m_name(_T("devDX11")),
    m_pD3DDevice(),
    m_devLUID(),
    m_adaptersCount(0),
    m_adaptersIndexes(),
    m_displayDeviceName(),
    m_log() {
    memset(m_adaptersIndexes, 0, sizeof(m_adaptersIndexes));
}

DeviceDX11::~DeviceDX11() {
    Terminate();
}

bool DeviceDX11::isValid() const {
    return m_pD3DDevice != nullptr;
}

ATL::CComPtr<ID3D11Device> DeviceDX11::GetDevice() {
    return m_pD3DDevice;
}

RGY_ERR DeviceDX11::Init(int adapterID, uint32_t vendorID, bool onlyWithOutputs, shared_ptr<RGYLog> log) {
    HRESULT hr = S_OK;
    m_log = (log) ? log : std::make_shared<RGYLog>(nullptr, RGY_LOG_ERROR);
    // find adapter
    ATL::CComPtr<IDXGIAdapter> pAdapter;

    EnumerateAdapters(vendorID, onlyWithOutputs);
    if (m_adaptersCount <= adapterID) {
        AddMessage(RGY_LOG_ERROR, _T("Invalid Adapter ID: %d\n"), adapterID);
        return RGY_ERR_NOT_FOUND;
    }

    //convert logical id to real index
    adapterID = m_adaptersIndexes[adapterID];

    ATL::CComPtr<IDXGIFactory> pFactory;
    hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void **)&pFactory);
    if (FAILED(hr)) {
        AddMessage(RGY_LOG_ERROR, _T("CreateDXGIFactory failed. Error: %x\n"), hr);
        return RGY_ERR_UNKNOWN;
    }

    AddMessage(RGY_LOG_DEBUG, _T("EnumAdapters %d...\n"), adapterID);
    if (pFactory->EnumAdapters(adapterID, &pAdapter) == DXGI_ERROR_NOT_FOUND) {
        AddMessage(RGY_LOG_DEBUG, _T("Adaptor %d not found.\n"), adapterID);
        return RGY_ERR_UNKNOWN;
    }

    DXGI_ADAPTER_DESC desc;
    pAdapter->GetDesc(&desc);
    m_displayDeviceName = desc.Description;
    m_devLUID = desc.AdapterLuid;

    ATL::CComPtr<IDXGIOutput> pOutput;
    if (SUCCEEDED(pAdapter->EnumOutputs(0, &pOutput))) {
        DXGI_OUTPUT_DESC outputDesc;
        pOutput->GetDesc(&outputDesc);

    }
    ATL::CComPtr<ID3D11Device> pD3D11Device;
    ATL::CComPtr<ID3D11DeviceContext>  pD3D11Context;
    UINT createDeviceFlags = 0;

#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
    };
    D3D_FEATURE_LEVEL featureLevel;

    D3D_DRIVER_TYPE eDriverType = pAdapter != NULL ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE;
    hr = D3D11CreateDevice(pAdapter, eDriverType, NULL, createDeviceFlags, featureLevels, _countof(featureLevels),
        D3D11_SDK_VERSION, &pD3D11Device, &featureLevel, &pD3D11Context);
#ifdef _DEBUG
    if (FAILED(hr)) {
        createDeviceFlags &= (~D3D11_CREATE_DEVICE_DEBUG);
        hr = D3D11CreateDevice(pAdapter, eDriverType, NULL, createDeviceFlags, featureLevels, _countof(featureLevels),
            D3D11_SDK_VERSION, &pD3D11Device, &featureLevel, &pD3D11Context);
    }
#endif
    if (FAILED(hr)) {
        AddMessage(RGY_LOG_ERROR, L"InitDX11() failed to create HW DX11.1 device\n");
        hr = D3D11CreateDevice(pAdapter, eDriverType, NULL, createDeviceFlags, featureLevels + 1, _countof(featureLevels) - 1,
            D3D11_SDK_VERSION, &pD3D11Device, &featureLevel, &pD3D11Context);
    }
    if (FAILED(hr)) {
        AddMessage(RGY_LOG_ERROR, L"InitDX11() failed to create HW DX11 device\n");
        hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_SOFTWARE, NULL, createDeviceFlags, featureLevels, _countof(featureLevels),
            D3D11_SDK_VERSION, &pD3D11Device, &featureLevel, &pD3D11Context);
    }

    if (FAILED(hr)) {
        AddMessage(RGY_LOG_ERROR, L"InitDX11() failed to create SW DX11.1 device\n");
        hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_SOFTWARE, NULL, createDeviceFlags, featureLevels + 1, _countof(featureLevels) - 1,
            D3D11_SDK_VERSION, &pD3D11Device, &featureLevel, &pD3D11Context);
    }
    if (FAILED(hr)) {
        AddMessage(RGY_LOG_ERROR, L"InitDX11() failed to create SW DX11 device\n");
    }
    AddMessage(RGY_LOG_DEBUG, _T("InitDX11() success.\n"));

    ATL::CComPtr<ID3D10Multithread> pMultithread = NULL;
    hr = pD3D11Device->QueryInterface(__uuidof(ID3D10Multithread), reinterpret_cast<void **>(&pMultithread));
    if (FAILED(hr)) {
        AddMessage(RGY_LOG_ERROR, L"QueryInterface() failed\n");
    }
    if (pMultithread) {
        //        amf_bool isSafe = pMultithread->GetMultithreadProtected() ? true : false;
        pMultithread->SetMultithreadProtected(true);
    }

    m_pD3DDevice = pD3D11Device;

    return RGY_ERR_NONE;
}

RGY_ERR DeviceDX11::Terminate() {
    AddMessage(RGY_LOG_DEBUG, _T("Closing DX11 device...\n"));
    m_pD3DDevice.Release();
    return RGY_ERR_NONE;
}

void DeviceDX11::EnumerateAdapters(const uint32_t vendorID, bool onlyWithOutputs) {
    ATL::CComPtr<IDXGIFactory> pFactory;
    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void **)&pFactory);
    if (FAILED(hr)) {
        AddMessage(RGY_LOG_ERROR, _T("CreateDXGIFactory failed. Error: 0x%08x"), hr);
        return;
    }

    UINT count = 0;
    m_adaptersCount = 0;
    while (true) {
        AddMessage(RGY_LOG_DEBUG, _T("EnumAdapters %d...\n"), count);
        ATL::CComPtr<IDXGIAdapter> pAdapter;
        if (pFactory->EnumAdapters(count, &pAdapter) == DXGI_ERROR_NOT_FOUND) {
            break;
        }

        DXGI_ADAPTER_DESC desc;
        pAdapter->GetDesc(&desc);

        if (desc.VendorId != vendorID) {
            AddMessage(RGY_LOG_DEBUG, _T("Non Target Adaptor %d: VendorId: 0x%08x.\n"), count, desc.VendorId);
            count++;
            continue;
        }
        ATL::CComPtr<IDXGIOutput> pOutput;
        if (onlyWithOutputs && pAdapter->EnumOutputs(0, &pOutput) == DXGI_ERROR_NOT_FOUND) {
            count++;
            continue;
        }
        char strDevice[100];
        _snprintf_s(strDevice, 100, "%X", desc.DeviceId);
        AddMessage(RGY_LOG_DEBUG, _T("Found Adaptor %d [%d]: %s\n"), m_adaptersCount, count, char_to_tstring(strDevice).c_str());

        m_adaptersIndexes[m_adaptersCount] = count;
        m_adaptersCount++;
        count++;
    }
}

int DeviceDX11::adapterCount(const uint32_t vendorID, RGYLog *log) {
    ATL::CComPtr<IDXGIFactory> pFactory;
    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void **)&pFactory);
    if (FAILED(hr)) {
        if (log) log->write(RGY_LOG_ERROR, RGY_LOGT_DEV, _T("Failed to CreateDXGIFactory: Error: 0x%08x\n"), hr);
        return 0;
    }

    UINT count = 0;
    int adaptersCount = 0;
    while (true) {
        if (log) log->write(RGY_LOG_DEBUG, RGY_LOGT_DEV, _T("EnumAdapters %d...\n"), count);
        ATL::CComPtr<IDXGIAdapter> pAdapter;
        if (pFactory->EnumAdapters(count, &pAdapter) == DXGI_ERROR_NOT_FOUND) {
            break;
        }

        DXGI_ADAPTER_DESC desc;
        pAdapter->GetDesc(&desc);

        if (desc.VendorId != vendorID) {
            if (log) log->write(RGY_LOG_DEBUG, RGY_LOGT_DEV, _T("Non Target Adaptor %d: VendorId: 0x%08x.\n"), count, desc.VendorId);
            count++;
            continue;
        }
        if (log) log->write(RGY_LOG_DEBUG, RGY_LOGT_DEV, _T("Found Adaptor %d [%d]...\n"), adaptersCount, count);
        adaptersCount++;
        count++;
    }
    if (log) log->write(RGY_LOG_DEBUG, RGY_LOGT_DEV, _T("Found adapters: %d\n"), adaptersCount);
    return adaptersCount;
}

void DeviceDX11::AddMessage(RGYLogLevel log_level, const tstring &str) {
    if (m_log == nullptr || log_level < m_log->getLogLevel(RGY_LOGT_DEV)) {
        return;
    }
    auto lines = split(str, _T("\n"));
    for (const auto &line : lines) {
        if (line[0] != _T('\0')) {
            m_log->write(log_level, RGY_LOGT_DEV, (m_name + _T(": ") + line + _T("\n")).c_str());
        }
    }
}
void DeviceDX11::AddMessage(RGYLogLevel log_level, const TCHAR *format, ...) {
    if (m_log == nullptr || log_level < m_log->getLogLevel(RGY_LOGT_DEV)) {
        return;
    }

    va_list args;
    va_start(args, format);
    int len = _vsctprintf(format, args) + 1; // _vscprintf doesn't count terminating '\0'
    tstring buffer;
    buffer.resize(len, _T('\0'));
    _vstprintf_s(&buffer[0], len, format, args);
    va_end(args);
    AddMessage(log_level, buffer);
}
#endif
