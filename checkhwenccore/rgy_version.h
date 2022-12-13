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
// ------------------------------------------------------------------------------------------

#pragma once
#ifndef __RGY_VERSION_H__
#define __RGY_VERSION_H__

#include "rgy_rev.h"

#define VER_FILEVERSION             0,0,0,0
#define VER_STR_FILEVERSION          "0.00"
#define VER_STR_FILEVERSION_TCHAR _T("0.00")

#ifdef _M_IX86
#define BUILD_ARCH_STR _T("x86")
#else
#define BUILD_ARCH_STR _T("x64")
#endif

static const int HW_TIMEBASE = 90000;

#if _UNICODE
const wchar_t *get_encoder_version();
#else
const char *get_encoder_version();
#endif

#define ENCODER_QSV    1
#define ENCODER_NVENC  1
#define ENCODER_VCEENC 1
#define CLFILTERS_AUF  0
#define CHECKHWENC     1

#define GPU_VENDOR ""
#define NV_DRIVER_VER_MIN 418081

#define ENABLE_NVRTC 0

#define AV1_TIMESTAMP_OVERRIDE 1
#define OVERRIDE_HYPER_MODE_HEVC_FROM_H264 1
#define LIMIT_HYPER_MODE_TO_KNOWN_CODECS 1

#if defined(_WIN32) || defined(_WIN64)

#define USE_ONEVPL 1

#define D3D_SURFACES_SUPPORT 1
#define ENABLE_D3D9  1
#define ENABLE_D3D11 1

#define MFX_D3D11_SUPPORT 1

#define LIBVA_SUPPORT 0

#define ENABLE_ADVANCED_DEINTERLACE 0

#define ENABLE_MVC_ENCODING 0

#define ENABLE_FPS_CONVERSION 0

#define ENABLE_SESSION_THREAD_CONFIG 0

#define ENABLE_PERF_COUNTER 1

#define ENABLE_AVCODEC_OUT_THREAD 1
#define ENABLE_AVCODEC_AUDPROCESS_THREAD 1
#define ENABLE_CPP_REGEX 1
#define ENABLE_DTL 0

#define ENABLE_AVCODEC_ITERATE 1
#define ENABLE_DOVI_METADATA_OPTIONS 1
#define ENABLE_KEYFRAME_INSERT 0
#define ENABLE_AUTO_PICSTRUCT 0

#define ENABLE_HYPER_MODE 1

#define ENCODER_NAME              "checkhwenc"
#define DECODER_NAME              "checkhwenc"
#define FOR_AUO                   0
#define ENABLE_OPENCL             1
#define ENABLE_AUO_LINK           0
#define ENABLE_RAW_READER         0
#define ENABLE_AVI_READER         0
#define ENABLE_AVISYNTH_READER    0
#define ENABLE_VAPOURSYNTH_READER 0
#define ENABLE_AVSW_READER        0
#define ENABLE_SM_READER          0
#define ENABLE_LIBASS_SUBBURN     0
#define ENABLE_CUSTOM_VPP         0
#ifndef ENABLE_METRIC_FRAMEWORK
#if defined(_M_IX86)
#define ENABLE_METRIC_FRAMEWORK   0
#else
#define ENABLE_METRIC_FRAMEWORK   0
#endif
#define ENABLE_CAPTION2ASS        0
#endif //#ifndef ENABLE_METRIC_FRAMEWORK

#else //#if defined(WIN32) || defined(WIN64)
#define ENABLE_D3D9  0
#define ENABLE_D3D11 0
#define ENABLE_VULKAN 1
#define VULKAN_DEFAULT_DEVICE_ONLY 1
#define USE_ONEVPL 1
#define D3D_SURFACES_SUPPORT 0
#define MFX_D3D11_SUPPORT 0
#define FOR_AUO 0
#define ENABLE_METRIC_FRAMEWORK 0
#define ENABLE_PERF_COUNTER 0
#define ENABLE_CAPTION2ASS 0
#define ENABLE_HYPER_MODE 0
#include "rgy_config.h"
#define ENCODER_NAME              "checkhwenc"
#define DECODER_NAME              "checkhwenc"
#endif // #if defined(WIN32) || defined(WIN64)

#endif //__RGY_VERSION_H__
