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

#include "vce_amf.h"

int show_vce_hw(const int deviceid, const RGYParamLogLevel& loglevel) {
    auto core = std::make_unique<VCEAMF>();
    auto err = RGY_ERR_NONE;
    if ((err = core->initLogLevel(loglevel)) == RGY_ERR_NONE
        && (err = core->initAMFFactory()) == RGY_ERR_NONE
        && (err = core->initTracer(loglevel.get(RGY_LOGT_AMF))) == RGY_ERR_NONE) {
#if ENABLE_D3D11
        const auto devList = core->createDeviceList(false, true, false, true, false);
#else
        const auto devList = core->createDeviceList(false, true, ENABLE_VULKAN != 0, true, false);
#endif
        if (devList.size() > 0) {
            _ftprintf(stdout, _T("VCE available\n"));
            for (auto &dev : devList) {
                if (deviceid < 0 || dev->id() == deviceid) {
                    _ftprintf(stdout, _T("device #%d: %s\n"), dev->id(), dev->name().c_str());
                }
            }
            return 0;
        }
    }
    _ftprintf(stdout, _T("VCE unavailable.\n"));
    return 1;
}

