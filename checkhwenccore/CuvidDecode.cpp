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

#include "CuvidDecode.h"
#if ENABLE_AVSW_READER
#include "NVEncUtil.h"
#include "rgy_bitstream.h"

#if defined(_WIN32) || defined(_WIN64)
static const TCHAR *NVCUVID_DLL_NAME = _T("nvcuvid.dll");
static const TCHAR *NVCUVID_DLL_NAME2 = nullptr;
#else
static const TCHAR *NVCUVID_DLL_NAME = _T("libnvcuvid.so");
static const TCHAR *NVCUVID_DLL_NAME2 = _T("libnvcuvid.so.1");
#endif

bool check_if_nvcuvid_dll_available() {
    //check for nvcuvid.dll
    HMODULE hModule = RGY_LOAD_LIBRARY(NVCUVID_DLL_NAME);
    if (hModule == nullptr && NVCUVID_DLL_NAME2 != nullptr) {
        hModule = RGY_LOAD_LIBRARY(NVCUVID_DLL_NAME2);
    }
    if (hModule == nullptr) {
        return false;
    }
    RGY_FREE_LIBRARY(hModule);
    return true;
}

CodecCsp getHWDecCodecCsp(bool skipHWDecodeCheck) {
    static const auto test_target_yv12 = make_array<RGY_CSP>(
        RGY_CSP_NV12,
        RGY_CSP_YV12_10,
        RGY_CSP_YV12_12,
        RGY_CSP_YV12_14,
        RGY_CSP_YV12_16,
        RGY_CSP_YV12_09);
    static const auto test_target_yuv444 = make_array<RGY_CSP>(
        RGY_CSP_YUV444,
        RGY_CSP_YUV444_10,
        RGY_CSP_YUV444_12,
        RGY_CSP_YUV444_14,
        RGY_CSP_YUV444_16,
        RGY_CSP_YUV444_09
        );

    CodecCsp HWDecCodecCsp;

    for (int i = 0; i < _countof(HW_DECODE_LIST); i++) {
        std::vector<RGY_CSP> supported_csp;
        const auto enc_codec = codec_rgy_to_dec(HW_DECODE_LIST[i].rgy_codec);
        for (auto csp : test_target_yv12) {
            if (skipHWDecodeCheck) {
                supported_csp.push_back(csp);
                continue;
            }
            CUVIDDECODECAPS caps_test;
            memset(&caps_test, 0, sizeof(caps_test));
            caps_test.eCodecType = enc_codec;
            caps_test.nBitDepthMinus8 = RGY_CSP_BIT_DEPTH[csp] - 8;
            caps_test.eChromaFormat = chromafmt_rgy_to_enc(RGY_CSP_CHROMA_FORMAT[csp]);
            auto ret = cuvidGetDecoderCaps(&caps_test);
            if (ret != CUDA_SUCCESS || !caps_test.bIsSupported) {
                break;
            }
            supported_csp.push_back(csp);
            if (csp == RGY_CSP_NV12) {
                supported_csp.push_back(RGY_CSP_YV12);
            }
        }
        if (supported_csp.size() > 0) {
            for (auto csp : test_target_yuv444) {
                if (skipHWDecodeCheck) {
                    supported_csp.push_back(csp);
                    continue;
                }
                CUVIDDECODECAPS caps_test;
                memset(&caps_test, 0, sizeof(caps_test));
                caps_test.eCodecType = enc_codec;
                caps_test.nBitDepthMinus8 = RGY_CSP_BIT_DEPTH[csp] - 8;
                caps_test.eChromaFormat = chromafmt_rgy_to_enc(RGY_CSP_CHROMA_FORMAT[csp]);
                auto ret = cuvidGetDecoderCaps(&caps_test);
                if (ret != CUDA_SUCCESS || !caps_test.bIsSupported) {
                    break;
                }
                supported_csp.push_back(csp);
            }
        }
        if (supported_csp.size() > 0) {
            HWDecCodecCsp[HW_DECODE_LIST[i].rgy_codec] = supported_csp;
        }
    }

    //もし、なんらかの原因で正常に取得されていなければ、
    //基本的なコーデックはデコード可能だと返す
    std::vector<RGY_CODEC> basic_codec_list = { RGY_CODEC_H264, RGY_CODEC_MPEG2, RGY_CODEC_MPEG1 };
    std::vector<RGY_CSP> basic_csp_list = { RGY_CSP_NV12, RGY_CSP_YV12 };
    for (auto codec : basic_codec_list) {
        if (HWDecCodecCsp.count(codec) == 0) {
            HWDecCodecCsp[codec] = basic_csp_list;
        }
    }
    return HWDecCodecCsp;
}

static int CUDAAPI HandleVideoData(void *pUserData, CUVIDSOURCEDATAPACKET *pPacket) {
    assert(pUserData);
    return ((CuvidDecode*)pUserData)->DecVideoData(pPacket);
}

static int CUDAAPI HandleVideoSequence(void *pUserData, CUVIDEOFORMAT *pFormat) {
    assert(pUserData);
    return ((CuvidDecode*)pUserData)->DecVideoSequence(pFormat);
}

static int CUDAAPI HandlePictureDecode(void *pUserData, CUVIDPICPARAMS *pPicParams) {
    assert(pUserData);
    return ((CuvidDecode*)pUserData)->DecPictureDecode(pPicParams);
}

static int CUDAAPI HandlePictureDisplay(void *pUserData, CUVIDPARSERDISPINFO *pPicParams) {
    assert(pUserData);
    return ((CuvidDecode*)pUserData)->DecPictureDisplay(pPicParams);
}

CuvidDecode::CuvidDecode() :
    m_pFrameQueue(nullptr), m_decodedFrames(0), m_parsedPackets(0), m_videoParser(nullptr), m_videoDecoder(nullptr),
    m_ctxLock(nullptr), m_pPrintMes(), m_bIgnoreDynamicFormatChange(false), m_bError(false), m_videoInfo(), m_nDecType(0) {
    memset(&m_videoDecodeCreateInfo, 0, sizeof(m_videoDecodeCreateInfo));
    memset(&m_videoFormatEx, 0, sizeof(m_videoFormatEx));
}

CuvidDecode::~CuvidDecode() {
    CloseDecoder();
}

int CuvidDecode::DecVideoData(CUVIDSOURCEDATAPACKET *pPacket) {
    AddMessage(RGY_LOG_TRACE, _T("DecVideoData packet: timestamp %lld, size %u\n"), pPacket->timestamp, pPacket->payload_size);
    CUresult curesult = CUDA_SUCCESS;
    cuvidCtxLock(m_ctxLock, 0);
    try {
        curesult = cuvidParseVideoData(m_videoParser, pPacket);
    } catch(...) {
        AddMessage(RGY_LOG_ERROR, _T("cuvidParseVideoData exception\n"));
        curesult = CUDA_ERROR_UNKNOWN;
    }
    cuvidCtxUnlock(m_ctxLock, 0);
    if (curesult != CUDA_SUCCESS) {
        AddMessage(RGY_LOG_DEBUG, _T("cuvidParseVideoData error\n"));
        m_bError = true;
    }
    return (curesult == CUDA_SUCCESS);
}

int CuvidDecode::DecPictureDecode(CUVIDPICPARAMS *pPicParams) {
    AddMessage(RGY_LOG_TRACE, _T("DecPictureDecode idx: %d\n"), pPicParams->CurrPicIdx);
    m_pFrameQueue->waitUntilFrameAvailable(pPicParams->CurrPicIdx);
    CUresult curesult = CUDA_SUCCESS;
    cuvidCtxLock(m_ctxLock, 0);
    try {
        curesult = cuvidDecodePicture(m_videoDecoder, pPicParams);
    } catch(...) {
        AddMessage(RGY_LOG_ERROR, _T("cuvidDecodePicture exception\n"));
        curesult = CUDA_ERROR_UNKNOWN;
    }
    cuvidCtxUnlock(m_ctxLock, 0);
    if (curesult != CUDA_SUCCESS) {
        AddMessage(RGY_LOG_DEBUG, _T("cuvidDecodePicture error\n"));
        m_bError = true;
    }
    return (curesult == CUDA_SUCCESS);
}

int CuvidDecode::DecVideoSequence(CUVIDEOFORMAT *pFormat) {
    AddMessage(RGY_LOG_TRACE, _T("DecVideoSequence\n"));
    if (   (pFormat->codec         != m_videoDecodeCreateInfo.CodecType)
        || (pFormat->chroma_format != m_videoDecodeCreateInfo.ChromaFormat)) {
        if (m_videoDecodeCreateInfo.CodecType != cudaVideoCodec_NumCodecs) {
            AddMessage(RGY_LOG_DEBUG, _T("dynamic video format changing detected\n"));
        }
        CreateDecoder(pFormat);
        return 1;
    }
    if (   (pFormat->coded_width   != m_videoDecodeCreateInfo.ulWidth)
        || (pFormat->coded_height  != m_videoDecodeCreateInfo.ulHeight)) {
        AddMessage(RGY_LOG_DEBUG, _T("dynamic video format changing detected\n"));
        m_videoDecodeCreateInfo.CodecType    = pFormat->codec;
        m_videoDecodeCreateInfo.ulWidth      = pFormat->coded_width;
        m_videoDecodeCreateInfo.ulHeight     = pFormat->coded_height;
        m_videoDecodeCreateInfo.ChromaFormat = pFormat->chroma_format;
        if (pFormat->coded_width != m_videoDecodeCreateInfo.ulWidth && pFormat->coded_height != m_videoDecodeCreateInfo.ulHeight) {
            memcpy(&m_videoDecodeCreateInfo.display_area, &pFormat->display_area, sizeof(pFormat->display_area));
        }
        return 0;
    }
    return 1;
}

int CuvidDecode::DecPictureDisplay(CUVIDPARSERDISPINFO *pPicParams) {
    AddMessage(RGY_LOG_TRACE, _T("DecPictureDisplay idx: %d, %I64d\n"), pPicParams->picture_index, pPicParams->timestamp);
    m_pFrameQueue->enqueue(pPicParams);
    m_decodedFrames++;

    return 1;
}

RGY_ERR CuvidDecode::CloseDecoder() {
    RGY_ERR err = RGY_ERR_NONE;
    AddMessage(RGY_LOG_DEBUG, _T("Closing decoder...\n"));
    if (m_videoDecoder) {
        try {
            NVEncCtxAutoLock(ctxlock(m_ctxLock));
            cuvidDestroyDecoder(m_videoDecoder);
            AddMessage(RGY_LOG_DEBUG, _T("cuvidDestroyDecoder: Fin.\n"));
        } catch (std::exception& e) {
            AddMessage(RGY_LOG_ERROR, _T("Error in cuvidDestroyDecoder: %s\n"), char_to_tstring(e.what()).c_str());
            err = RGY_ERR_UNKNOWN;
        }
        m_videoDecoder = nullptr;
    }
    if (m_videoParser) {
        try {
            NVEncCtxAutoLock(ctxlock(m_ctxLock));
            cuvidDestroyVideoParser(m_videoParser);
            AddMessage(RGY_LOG_DEBUG, _T("cuvidDestroyVideoParser: Fin.\n"));
        } catch (std::exception& e) {
            AddMessage(RGY_LOG_ERROR, _T("Error in cuvidDestroyVideoParser: %s\n"), char_to_tstring(e.what()).c_str());
            err = RGY_ERR_UNKNOWN;
        }
        m_videoParser = nullptr;
    }
    m_ctxLock = nullptr;
    if (m_pFrameQueue) {
        delete m_pFrameQueue;
        m_pFrameQueue = nullptr;
    }
    m_decodedFrames = 0;
    m_bError = false;
    AddMessage(RGY_LOG_DEBUG, _T("Closed decoder.\n"));
    m_pPrintMes.reset();
    return err;
}

CUresult CuvidDecode::CreateDecoder() {
    CUresult curesult = CUDA_SUCCESS;
    try {
        curesult = cuvidCreateDecoder(&m_videoDecoder, &m_videoDecodeCreateInfo);
    } catch(...) {
        AddMessage(RGY_LOG_ERROR, _T("cuvidCreateDecoder error\n"));
        curesult = CUDA_ERROR_UNKNOWN;
        m_bError = true;
    }
    return curesult;
}


CUresult CuvidDecode::CreateDecoder(CUVIDEOFORMAT *pFormat) {
    if (m_videoDecoder) {
        cuvidDestroyDecoder(m_videoDecoder);
        m_videoDecoder = nullptr;
    }
    m_videoDecodeCreateInfo.CodecType = pFormat->codec;
    m_videoDecodeCreateInfo.ChromaFormat = pFormat->chroma_format;
    m_videoDecodeCreateInfo.ulWidth   = pFormat->coded_width;
    m_videoDecodeCreateInfo.ulHeight  = pFormat->coded_height;
    m_videoDecodeCreateInfo.bitDepthMinus8 = pFormat->bit_depth_luma_minus8;

    if (m_videoInfo.dstWidth > 0 && m_videoInfo.dstHeight > 0) {
        m_videoDecodeCreateInfo.ulTargetWidth  = m_videoInfo.dstWidth;
        m_videoDecodeCreateInfo.ulTargetHeight = m_videoInfo.dstHeight;
    } else {
#if CUVID_DISABLE_CROP
        m_videoDecodeCreateInfo.ulTargetWidth  = m_videoInfo.srcWidth;
        m_videoDecodeCreateInfo.ulTargetHeight = m_videoInfo.srcHeight;
#else
        m_videoDecodeCreateInfo.ulTargetWidth  = m_videoInfo.srcWidth - m_videoInfo.crop.e.right - m_videoInfo.crop.e.left;
        m_videoDecodeCreateInfo.ulTargetHeight = m_videoInfo.srcHeight - m_videoInfo.crop.e.up - m_videoInfo.crop.e.bottom;
#endif
    }
    m_videoDecodeCreateInfo.target_rect.left = 0;
    m_videoDecodeCreateInfo.target_rect.top = 0;
    m_videoDecodeCreateInfo.target_rect.right = (short)m_videoDecodeCreateInfo.ulTargetWidth;
    m_videoDecodeCreateInfo.target_rect.bottom = (short)m_videoDecodeCreateInfo.ulTargetHeight;

#if CUVID_DISABLE_CROP
    //cuvidでcropすると2で割り切れない高さのcropがうまく処理されなかったりよくわからないので、
    //いろいろ調査するのも面倒なのでcropの使用そのものをやめる
    m_videoDecodeCreateInfo.display_area.left   = (short)(pFormat->display_area.left);
    m_videoDecodeCreateInfo.display_area.top    = (short)(pFormat->display_area.top);
    m_videoDecodeCreateInfo.display_area.right  = (short)(pFormat->display_area.right);
    m_videoDecodeCreateInfo.display_area.bottom = (short)(pFormat->display_area.bottom);
#else
    m_videoDecodeCreateInfo.display_area.left   = (short)(pFormat->display_area.left + m_videoInfo.crop.e.left);
    m_videoDecodeCreateInfo.display_area.top    = (short)(pFormat->display_area.top + m_videoInfo.crop.e.up);
    m_videoDecodeCreateInfo.display_area.right  = (short)(pFormat->display_area.right - m_videoInfo.crop.e.right);
    m_videoDecodeCreateInfo.display_area.bottom = (short)(pFormat->display_area.bottom - m_videoInfo.crop.e.bottom);
#endif

    NVEncCtxAutoLock(ctxlock(m_ctxLock));
    m_videoDecodeCreateInfo.CodecType = pFormat->codec;
    CUresult curesult = CreateDecoder();
    if (CUDA_SUCCESS != curesult) {
        AddMessage(RGY_LOG_ERROR, _T("Failed cuvidCreateDecoder %d (%s)\n"), curesult, char_to_tstring(_cudaGetErrorEnum(curesult)).c_str());
        m_bError = true;
        return curesult;
    }
    AddMessage(RGY_LOG_DEBUG, _T("created decoder (mode: %s)\n"), get_chr_from_value(list_cuvid_mode, m_nDecType));
    return curesult;
}

CUresult CuvidDecode::InitDecode(CUvideoctxlock ctxLock, const VideoInfo *input, const VppParam *vpp, AVRational streamtimebase, shared_ptr<RGYLog> pLog, int nDecType, bool bCuvidResize, bool lowLatency, bool ignoreDynamicFormatChange) {
    //初期化
    CloseDecoder();

    m_videoInfo = *input;
    if (!bCuvidResize) {
        m_videoInfo.dstWidth = 0;
        m_videoInfo.dstHeight = 0;
    }
    m_nDecType = nDecType;
    m_pPrintMes = pLog;
    m_bIgnoreDynamicFormatChange = ignoreDynamicFormatChange;
    m_deinterlaceMode = (vpp) ? vpp->deinterlace : cudaVideoDeinterlaceMode_Weave;

    if (!check_if_nvcuvid_dll_available()) {
        AddMessage(RGY_LOG_ERROR, _T("nvcuvid.dll does not exist.\n"));
        return CUDA_ERROR_NOT_FOUND;
    }
    AddMessage(RGY_LOG_DEBUG, _T("nvcuvid.dll available\n"));

    if (!ctxLock) {
        AddMessage(RGY_LOG_ERROR, _T("invalid ctxLock.\n"));
        return CUDA_ERROR_INVALID_VALUE;
    }

    m_ctxLock = ctxLock;

    if (nullptr == (m_pFrameQueue = new CUVIDFrameQueue(m_ctxLock))) {
        AddMessage(RGY_LOG_ERROR, _T("Failed to alloc frame queue for decoder.\n"));
        m_bError = true;
        return CUDA_ERROR_OUT_OF_MEMORY;
    }
    m_pFrameQueue->init(input->srcWidth, input->srcHeight);
    AddMessage(RGY_LOG_DEBUG, _T("created frame queue\n"));

    //init video parser
    memset(&m_videoFormatEx, 0, sizeof(CUVIDEOFORMATEX));
    if (input->codecExtra && input->codecExtraSize) {
        //if (input->codecExtraSize > sizeof(m_videoFormatEx.raw_seqhdr_data)) {
        //    AddMessage(RGY_LOG_ERROR, _T("Parsed header too large!\n"));
        //    return CUDA_ERROR_INVALID_VALUE;
        //}

        std::vector<uint8_t> tmpBuf;
        //抽出されたextradataが大きすぎる場合、適当に縮める
        //NVEncのデコーダが受け取れるヘッダは1024byteまで
        if (input->codecExtraSize > 1024) {
            if (input->codec == RGY_CODEC_H264) {
                std::vector<nal_info> nal_list = get_parse_nal_unit_h264_func()((const uint8_t *)input->codecExtra, input->codecExtraSize);
                const auto h264_sps_nal = std::find_if(nal_list.begin(), nal_list.end(), [](nal_info info) { return info.type == NALU_H264_SPS; });
                const auto h264_pps_nal = std::find_if(nal_list.begin(), nal_list.end(), [](nal_info info) { return info.type == NALU_H264_PPS; });
                const bool header_check = (nal_list.end() != h264_sps_nal) && (nal_list.end() != h264_pps_nal);
                if (header_check) {
                    tmpBuf.resize(h264_sps_nal->size + h264_pps_nal->size);
                    memcpy(tmpBuf.data(), h264_sps_nal->ptr, h264_sps_nal->size);
                    memcpy(tmpBuf.data() + h264_sps_nal->size, h264_pps_nal->ptr, h264_pps_nal->size);
                }
            } else if (input->codec == RGY_CODEC_HEVC) {
                std::vector<nal_info> nal_list = get_parse_nal_unit_hevc_func()((const uint8_t *)input->codecExtra, input->codecExtraSize);
                const auto hevc_vps_nal = std::find_if(nal_list.begin(), nal_list.end(), [](nal_info info) { return info.type == NALU_HEVC_VPS; });
                const auto hevc_sps_nal = std::find_if(nal_list.begin(), nal_list.end(), [](nal_info info) { return info.type == NALU_HEVC_SPS; });
                const auto hevc_pps_nal = std::find_if(nal_list.begin(), nal_list.end(), [](nal_info info) { return info.type == NALU_HEVC_PPS; });
                const bool header_check = (nal_list.end() != hevc_vps_nal) && (nal_list.end() != hevc_sps_nal) && (nal_list.end() != hevc_pps_nal);
                if (header_check) {
                    tmpBuf.resize(hevc_vps_nal->size + hevc_sps_nal->size + hevc_pps_nal->size);
                    memcpy(tmpBuf.data(), hevc_vps_nal->ptr, hevc_vps_nal->size);
                    memcpy(tmpBuf.data() + hevc_vps_nal->size, hevc_sps_nal->ptr, hevc_sps_nal->size);
                    memcpy(tmpBuf.data() + hevc_vps_nal->size + hevc_sps_nal->size, hevc_pps_nal->ptr, hevc_pps_nal->size);
                }
            } else {
                AddMessage(RGY_LOG_WARN, _T("GetHeader: Unknown codec.\n"));
                return CUDA_ERROR_INVALID_VALUE;
            }
        } else {
            tmpBuf.resize(input->codecExtraSize);
            memcpy(tmpBuf.data(), input->codecExtra, tmpBuf.size());
        }
        m_videoFormatEx.format.seqhdr_data_length = (decltype(m_videoFormatEx.format.seqhdr_data_length))std::min(tmpBuf.size(), sizeof(m_videoFormatEx.raw_seqhdr_data));
        memcpy(m_videoFormatEx.raw_seqhdr_data, tmpBuf.data(), m_videoFormatEx.format.seqhdr_data_length);
    }
    if (!av_isvalid_q(streamtimebase)) {
        AddMessage(RGY_LOG_ERROR, _T("Invalid stream timebase %d/%d\n"), streamtimebase.num, streamtimebase.den);
        m_bError = true;
        return CUDA_ERROR_INVALID_VALUE;
    }

    CUVIDPARSERPARAMS oVideoParserParameters;
    memset(&oVideoParserParameters, 0, sizeof(CUVIDPARSERPARAMS));
    oVideoParserParameters.CodecType              = codec_rgy_to_dec(input->codec);
    oVideoParserParameters.ulClockRate            = streamtimebase.den;
    oVideoParserParameters.ulMaxNumDecodeSurfaces = FrameQueue::cnMaximumSize;
    oVideoParserParameters.ulMaxDisplayDelay      = (lowLatency) ? 0 : 1;
    oVideoParserParameters.pUserData              = this;
    oVideoParserParameters.pfnSequenceCallback    = HandleVideoSequence;
    oVideoParserParameters.pfnDecodePicture       = HandlePictureDecode;
    oVideoParserParameters.pfnDisplayPicture      = HandlePictureDisplay;
    oVideoParserParameters.pExtVideoInfo          = &m_videoFormatEx;

    CUresult curesult = CUDA_SUCCESS;
    if (CUDA_SUCCESS != (curesult = cuvidCreateVideoParser(&m_videoParser, &oVideoParserParameters))) {
        AddMessage(RGY_LOG_ERROR, _T("Failed cuvidCreateVideoParser %d (%s)\n"), curesult, char_to_tstring(_cudaGetErrorEnum(curesult)).c_str());
        m_bError = true;
        return curesult;
    }
    AddMessage(RGY_LOG_DEBUG, _T("created video parser\n"));

    cuvidCtxLock(m_ctxLock, 0);
    memset(&m_videoDecodeCreateInfo, 0, sizeof(CUVIDDECODECREATEINFO));
    m_videoDecodeCreateInfo.CodecType = cudaVideoCodec_NumCodecs; // こうしておいて後からDecVideoSequence()->CreateDecoder()で設定する
    m_videoDecodeCreateInfo.ulWidth   = input->srcWidth;
    m_videoDecodeCreateInfo.ulHeight  = input->srcHeight;
    m_videoDecodeCreateInfo.ulNumDecodeSurfaces = FrameQueue::cnMaximumSize;

    m_videoDecodeCreateInfo.ChromaFormat = chromafmt_rgy_to_enc(RGY_CSP_CHROMA_FORMAT[input->csp]);
    m_videoDecodeCreateInfo.OutputFormat = csp_rgy_to_surfacefmt(input->csp);
    m_videoDecodeCreateInfo.DeinterlaceMode = (vpp) ? vpp->deinterlace : cudaVideoDeinterlaceMode_Weave;

    if (m_videoInfo.dstWidth > 0 && m_videoInfo.dstHeight > 0) {
        m_videoDecodeCreateInfo.ulTargetWidth  = m_videoInfo.dstWidth;
        m_videoDecodeCreateInfo.ulTargetHeight = m_videoInfo.dstHeight;
    } else {
        m_videoDecodeCreateInfo.ulTargetWidth  = m_videoInfo.srcWidth - input->crop.e.right - input->crop.e.left;
        m_videoDecodeCreateInfo.ulTargetHeight = m_videoInfo.srcHeight - input->crop.e.up - input->crop.e.bottom;
    }

    m_videoDecodeCreateInfo.display_area.left   = (short)input->crop.e.left;
    m_videoDecodeCreateInfo.display_area.top    = (short)input->crop.e.up;
    m_videoDecodeCreateInfo.display_area.right  = (short)(input->srcWidth - input->crop.e.right);
    m_videoDecodeCreateInfo.display_area.bottom = (short)(input->srcHeight - input->crop.e.bottom);

    m_videoDecodeCreateInfo.ulNumOutputSurfaces = 1;
    m_videoDecodeCreateInfo.ulCreationFlags = (nDecType == NV_ENC_AVCUVID_CUDA) ? cudaVideoCreate_PreferCUDA : cudaVideoCreate_PreferCUVID;
    m_videoDecodeCreateInfo.vidLock = m_ctxLock;
#if 0
    curesult = CreateDecoder();
    if (CUDA_SUCCESS != curesult) {
        AddMessage(RGY_LOG_ERROR, _T("Failed cuvidCreateDecoder %d (%s)\n"), curesult, char_to_tstring(_cudaGetErrorEnum(curesult)).c_str());
        return curesult;
    }
    AddMessage(RGY_LOG_DEBUG, _T("created decoder (mode: %s)\n"), get_chr_from_value(list_cuvid_mode, nDecType));

    if (m_videoFormatEx.raw_seqhdr_data && m_videoFormatEx.format.seqhdr_data_length) {
        if (CUDA_SUCCESS != (curesult = DecodePacket(m_videoFormatEx.raw_seqhdr_data, m_videoFormatEx.format.seqhdr_data_length, AV_NOPTS_VALUE, HW_NATIVE_TIMEBASE))) {
            AddMessage(RGY_LOG_ERROR, _T("Failed to decode header %d (%s).\n"), curesult, char_to_tstring(_cudaGetErrorEnum(curesult)).c_str());
            m_bError = true;
            return curesult;
        }
    }
#else
    if (m_videoFormatEx.format.seqhdr_data_length > 0) {
        CUVIDSOURCEDATAPACKET pCuvidPacket;
        memset(&pCuvidPacket, 0, sizeof(pCuvidPacket));
        pCuvidPacket.payload = m_videoFormatEx.raw_seqhdr_data;
        pCuvidPacket.payload_size = m_videoFormatEx.format.seqhdr_data_length;
        curesult = cuvidParseVideoData(m_videoParser, &pCuvidPacket);
        if (curesult != CUDA_SUCCESS) {
            AddMessage(RGY_LOG_ERROR, _T("cuvidParseVideoData error\n"));
            m_bError = true;
        }
    }
#endif
    cuvidCtxUnlock(m_ctxLock, 0);
    AddMessage(RGY_LOG_DEBUG, _T("DecodePacket: success\n"));
    return curesult;
}

CUresult CuvidDecode::FlushParser() {
    CUVIDSOURCEDATAPACKET pCuvidPacket;
    memset(&pCuvidPacket, 0, sizeof(pCuvidPacket));

    pCuvidPacket.flags |= CUVID_PKT_ENDOFSTREAM;
    CUresult result = CUDA_SUCCESS;

    //cuvidCtxLock(m_ctxLock, 0);
    try {
        result = cuvidParseVideoData(m_videoParser, &pCuvidPacket);
    } catch(...) {
        result = CUDA_ERROR_UNKNOWN;
    }
    if (result != CUDA_SUCCESS) {
        AddMessage(RGY_LOG_DEBUG, _T("cuvidParseVideoData error\n"));
        m_bError = true;
    }
    //cuvidCtxUnlock(m_ctxLock, 0);
    m_pFrameQueue->endDecode();
    return result;
}

CUresult CuvidDecode::DecodePacket(uint8_t *data, size_t nSize, int64_t timestamp, AVRational streamtimebase) {
    if (data == nullptr || nSize == 0) {
        return FlushParser();
    }

    CUVIDSOURCEDATAPACKET pCuvidPacket;
    memset(&pCuvidPacket, 0, sizeof(pCuvidPacket));
    pCuvidPacket.payload      = data;
    pCuvidPacket.payload_size = (uint32_t)nSize;
    CUresult result = CUDA_SUCCESS;

    if (timestamp != AV_NOPTS_VALUE && av_isvalid_q(streamtimebase)) {
        pCuvidPacket.flags     |= CUVID_PKT_TIMESTAMP;
        pCuvidPacket.timestamp  = timestamp * streamtimebase.num;
    }

    //cuvidCtxLock(m_ctxLock, 0);
    try {
        result = cuvidParseVideoData(m_videoParser, &pCuvidPacket);
    } catch(...) {
        result = CUDA_ERROR_UNKNOWN;
    }
    if (result != CUDA_SUCCESS) {
        AddMessage(RGY_LOG_DEBUG, _T("cuvidParseVideoData error\n"));
        m_bError = true;
    }
    //cuvidCtxUnlock(m_ctxLock, 0);
    m_parsedPackets++;
    if (m_parsedPackets >= 1000 && m_decodedFrames == 0) {
        //パケットを投入しているけど、デコードされないと検出できた場合はエラーを返す
        AddMessage(RGY_LOG_ERROR, _T("cuvid failing to parse/decode video stream.\n"));
        result = CUDA_ERROR_UNKNOWN;
        m_bError = true;
    }
    return result;
}

RGYFrameInfo CuvidDecode::GetDecFrameInfo() {
    RGYFrameInfo frame;
    frame.ptr = nullptr;
    frame.csp = m_videoInfo.csp;
    frame.width = m_videoDecodeCreateInfo.ulTargetWidth;
    frame.height = m_videoDecodeCreateInfo.ulTargetHeight;
    frame.pitch = 0; //この段階では取得できない、cuvidMapVideoFrameで取得
    frame.timestamp = (uint64_t)AV_NOPTS_VALUE;
    frame.deivce_mem = true;
    return frame;
}

#endif // #if ENABLE_AVSW_READER