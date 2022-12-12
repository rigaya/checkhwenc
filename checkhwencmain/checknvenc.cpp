// -----------------------------------------------------------------------------------------
// checkhwenc by rigaya
// -----------------------------------------------------------------------------------------
//
// The MIT License
//
// Copyright (c) 2022 rigaya
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
// ------------------------------------------------------------------------------------------

#include "NVEncDevice.h"

int show_nvenc_device_list(const RGYParamLogLevel& loglevel) {
    if (!check_if_nvcuda_dll_available()) {
        _ftprintf(stdout, _T("CUDA not available.\n"));
        return 1;
    }

    const int deviceID = -1;
    const int cudaSchedule = 0;
    const bool skipHWDecodeCheck = false;

    NVEncCtrl nvEnc;
    if (NV_ENC_SUCCESS == nvEnc.Initialize(deviceID, loglevel.get(RGY_LOGT_CORE))
        && NV_ENC_SUCCESS == nvEnc.ShowDeviceList(cudaSchedule, skipHWDecodeCheck)) {
        return 0;
    }
    return 1;
}

int show_nvenc_hw(const int deviceid, const RGYParamLogLevel& loglevel) {
    const int cudaSchedule = 0;
    const bool skipHWDecodeCheck = false;

    NVEncCtrl nvEnc;
    if (NV_ENC_SUCCESS == nvEnc.Initialize(deviceid, loglevel.get(RGY_LOGT_CORE))
        && NV_ENC_SUCCESS == nvEnc.ShowCodecSupport(cudaSchedule, skipHWDecodeCheck)) {
        return 0;
    }
    return 1;
}
