﻿// -----------------------------------------------------------------------------------------
// NVEnc by rigaya
// -----------------------------------------------------------------------------------------
//
// The MIT License
//
// Copyright (c) 2014-2016 rigaya
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
#ifndef __CUVID_DECODE_H__
#define __CUVID_DECODE_H__

#include "cuda_drvapi_dynlink.h"
#pragma warning(push)
#pragma warning(disable: 4201)
#include "dynlink_nvcuvid.h"
#pragma warning(pop)

#if ENABLE_AVSW_READER
#define NVEncCtxAutoLock(x) CCtxAutoLock x
#else
#define NVEncCtxAutoLock(x)
#endif

#if ENABLE_AVSW_READER
#include "FrameQueue.h"
#include "NVEncParam.h"
#include "NVEncFrameInfo.h"
#include "rgy_log.h"
#include "rgy_util.h"
#include "rgy_avutil.h"

bool check_if_nvcuvid_dll_available();
CodecCsp getHWDecCodecCsp(bool skipHWDecodeCheck);

struct VideoInfo;

class CuvidDecode {
public:
    CuvidDecode();
    ~CuvidDecode();

    CUresult InitDecode(CUvideoctxlock ctxLock, const VideoInfo *input, const VppParam *vpp, AVRational streamtimebase, shared_ptr<RGYLog> pLog, int nDecType, bool bCuvidResize, bool lowLatency = false, bool ignoreDynamicFormatChange = false);
    RGY_ERR CloseDecoder();
    CUresult DecodePacket(uint8_t *data, size_t nSize, int64_t timestamp, AVRational streamtimebase);
    CUresult FlushParser();

    void* GetDecoder() { return m_videoDecoder; };

    CUVIDDECODECREATEINFO GetDecodeInfo() { return m_videoDecodeCreateInfo; };
    RGYFrameInfo GetDecFrameInfo();

    bool GetError() { return m_bError; };

    int DecVideoData(CUVIDSOURCEDATAPACKET* pPacket);
    int DecPictureDecode(CUVIDPICPARAMS* pPicParams);
    int DecVideoSequence(CUVIDEOFORMAT* pFormat);
    int DecPictureDisplay(CUVIDPARSERDISPINFO* pPicParams);
    cudaVideoDeinterlaceMode getDeinterlaceMode() {
        return m_deinterlaceMode;
    }
    FrameQueue *frameQueue() {
        return m_pFrameQueue;
    }
protected:
    void AddMessage(RGYLogLevel log_level, const tstring& str) {
        if (m_pPrintMes == nullptr || log_level < m_pPrintMes->getLogLevel(RGY_LOGT_DEC)) {
            return;
        }
        auto lines = split(str, _T("\n"));
        for (const auto& line : lines) {
            if (line[0] != _T('\0')) {
                m_pPrintMes->write(log_level, RGY_LOGT_DEC, (_T("cuvid: ") + line + _T("\n")).c_str());
            }
        }
    }
    void AddMessage(RGYLogLevel log_level, const TCHAR *format, ... ) {
        if (m_pPrintMes == nullptr || log_level < m_pPrintMes->getLogLevel(RGY_LOGT_DEC)) {
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

    CUresult CreateDecoder();
    CUresult CreateDecoder(CUVIDEOFORMAT *pFormat);

    FrameQueue                  *m_pFrameQueue;
    int64_t                      m_decodedFrames;
    int64_t                      m_parsedPackets;
    CUvideoparser                m_videoParser;
    CUvideodecoder               m_videoDecoder;
    CUvideoctxlock               m_ctxLock;
    CUVIDDECODECREATEINFO        m_videoDecodeCreateInfo;
    CUVIDEOFORMATEX              m_videoFormatEx;
    shared_ptr<RGYLog>           m_pPrintMes;  //ログ出力
    bool                         m_bIgnoreDynamicFormatChange;
    bool                         m_bError;
    cudaVideoDeinterlaceMode     m_deinterlaceMode;
    VideoInfo                    m_videoInfo;
    int                          m_nDecType;
};

#endif //#if ENABLE_AVSW_READER

#endif //__CUVID_DECODE_H__