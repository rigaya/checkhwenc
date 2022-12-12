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

#include "rgy_def.h"
#include "rgy_log.h"
#include "rgy_util.h"

int check_qsv_devices(const int deviceNum);
int show_nvenc_device_list(const RGYParamLogLevel& loglevel);
int show_nvenc_hw(const int deviceid, const RGYParamLogLevel& loglevel);
int show_vce_hw(const int deviceid, const RGYParamLogLevel& loglevel);

void show_version();

static void show_help() {
    tstring str;
    str += strsprintf(_T("\n")
        _T("Information Options: \n")
        _T("   --help                       print help\n")
        _T("   --version                    print version info\n")
        _T("   --log-level <string>         set log level\n")
        _T("                                  debug, info(default), warn, error\n")
        _T("   --device <string>            set target device to check\n")
        _T("                                  all(default), qsv, nvenc, vce, vcn\n"));
    _ftprintf(stdout, _T("%s\n"), str.c_str());
}

int parse_log_level_param(const TCHAR *option_name, const TCHAR *arg_value, RGYParamLogLevel& loglevel) {
    std::vector<std::string> paramList;
    for (const auto& param : RGY_LOG_TYPE_STR) {
        paramList.push_back(tchar_to_string(param.second));
    }

    for (const auto &param : split(arg_value, _T(","))) {
        auto pos = param.find_first_of(_T("="));
        if (pos != std::string::npos) {
            auto param_arg = param.substr(0, pos);
            auto param_val = param.substr(pos + 1);
            param_arg = tolowercase(param_arg);
            int value = 0;
            if (get_list_value(list_log_level, param_val.c_str(), &value)) {
                auto type_ret = std::find_if(RGY_LOG_TYPE_STR.begin(), RGY_LOG_TYPE_STR.end(), [param_arg](decltype(RGY_LOG_TYPE_STR[0])& type) {
                    return param_arg == type.second;
                    });
                if (type_ret != RGY_LOG_TYPE_STR.end()) {
                    loglevel.set((RGYLogLevel)value, type_ret->first);
                    continue;
                } else {
                    return 1;
                }
            } else {
                return 1;
            }
        } else {
            int value = 0;
            if (get_list_value(list_log_level, param.c_str(), &value)) {
                loglevel.set((RGYLogLevel)value, RGY_LOGT_ALL);
                continue;
            } else {
                return 1;
            }
        }
    }
    return 0;
}

int check_devices_all(const int deviceID, const RGYParamLogLevel& loglevel) {
    const TCHAR *separator = _T("\n----------------------------------------------------\n");
    _ftprintf(stdout, _T("Checking QSV...\n"));
    const int ret_qsv = check_qsv_devices(deviceID);
    _ftprintf(stdout, separator);
    _ftprintf(stdout, _T("Checking NVENC...\n"));
    const int ret_nvenc = show_nvenc_device_list(loglevel);
    _ftprintf(stdout, separator);
    _ftprintf(stdout, _T("Checking VCE/VCN...\n"));
    const int ret_vce = show_vce_hw(deviceID, loglevel);
    return (ret_qsv != 0 && ret_nvenc != 0 && ret_vce != 0);
}

enum class CheckHWEncTarget {
    ALL,
    QSV,
    NVENC,
    VCE
};

int _tmain(int argc, TCHAR **argv) {
    for (int iarg = 1; iarg < argc; iarg++) {
        if (tstring(argv[iarg]) == _T("--help")) {
            show_help();
            return 0;
        }
    }

    RGYParamLogLevel loglevelPrint(RGY_LOG_ERROR);
    for (int iarg = 1; iarg < argc - 1; iarg++) {
        if (tstring(argv[iarg]) == _T("--log-level")) {
            parse_log_level_param(argv[iarg], argv[iarg + 1], loglevelPrint);
            break;
        }
    }

    CheckHWEncTarget target = CheckHWEncTarget::ALL;
    int deviceID = -1;
    for (int iarg = 1; iarg < argc - 1; iarg++) {
        if (tstring(argv[iarg]) == _T("--device")) {
            if (tstring(argv[iarg + 1]) == _T("all")) {
                target = CheckHWEncTarget::ALL;
            } else if (tstring(argv[iarg + 1]) == _T("qsv")) {
                target = CheckHWEncTarget::QSV;
            } else if (tstring(argv[iarg + 1]) == _T("nvenc")) {
                target = CheckHWEncTarget::NVENC;
            } else if (tstring(argv[iarg + 1]) == _T("vce") || tstring(argv[iarg + 1]) == _T("vcn")) {
                target = CheckHWEncTarget::VCE;
            }
            iarg++;
        }
    }
    int ret = 0;
    switch (target) {
    case CheckHWEncTarget::QSV:
        ret = check_qsv_devices(deviceID);
        break;
    case CheckHWEncTarget::NVENC:
        ret = show_nvenc_device_list(loglevelPrint);
        break;
    case CheckHWEncTarget::VCE:
        ret = show_vce_hw(deviceID, loglevelPrint);
        break;
    case CheckHWEncTarget::ALL:
        ret = check_devices_all(deviceID, loglevelPrint);
        break;
    default:
        break;
    }
    return ret;
}


#define SSTRING(str) STRING(str)
#define STRING(str) #str

const TCHAR *get_encoder_version() {
    return
        _T(ENCODER_NAME) _T(" (")
        BUILD_ARCH_STR _T(") ") VER_STR_FILEVERSION_TCHAR _T(", ")  _T(__DATE__) _T(" ") _T(__TIME__)
#if defined(_MSC_VER)
        _T(" (VC ") _T(SSTRING(_MSC_VER))
#elif defined(__clang__)
        _T(" (clang ") _T(SSTRING(__clang_major__)) _T(".") _T(SSTRING(__clang_minor__)) _T(".") _T(SSTRING(__clang_patchlevel__))
#elif defined(__GNUC__)
        _T(" (gcc ") _T(SSTRING(__GNUC__)) _T(".") _T(SSTRING(__GNUC_MINOR__)) _T(".") _T(SSTRING(__GNUC_PATCHLEVEL__))
#else
        _T(" (unknown")
#endif
        _T("/")
#ifdef _WIN32
        _T("Win")
#elif  __linux
        _T("Linux")
#else
        _T("unknown")
#endif
        _T(")");
}

void show_version() {
    _ftprintf(stdout, _T("%s"), get_encoder_version());
}
