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
#ifndef __RGY_AVUTIL_H__
#define __RGY_AVUTIL_H__

#include "rgy_version.h"
#include "rgy_tchar.h"
#include "rgy_def.h"

static const TCHAR *RGY_AVCODEC_AUTO = _T("auto");
static const TCHAR *RGY_AVCODEC_COPY = _T("copy");

#if ENABLE_AVSW_READER
#include <algorithm>

#pragma warning (push)
#pragma warning (disable: 4244)
#pragma warning (disable: 4819)
extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/error.h>
#include <libavutil/frame.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavutil/display.h>
#include <libavutil/mastering_display_metadata.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavcodec/avcodec.h>
#if __has_include(<libavcodec/bsf.h>)
#include <libavcodec/bsf.h>
#endif
#include <libswresample/swresample.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
}
#pragma comment (lib, "avcodec.lib")
#pragma comment (lib, "avformat.lib")
#pragma comment (lib, "avutil.lib")
#pragma comment (lib, "swresample.lib")
#pragma comment (lib, "avfilter.lib")
#pragma warning (pop)

#include "rgy_log.h"
#include "rgy_util.h"
#include "rgy_queue.h"

#define AVCODECID(x) AV_CODEC_ID_ ## x
#else
#define AVCodecID int
#define AVCODECID(x) 0
#endif

typedef struct CodecMap {
    AVCodecID avcodec_id;   //avcodecのコーデックID
    RGY_CODEC rgy_codec; //QSVのfourcc
} CodecMap;

//QSVでデコード可能なコーデックのリスト
static const CodecMap HW_DECODE_LIST[] = {
    { AVCODECID(H264),       RGY_CODEC_H264 },
    { AVCODECID(HEVC),       RGY_CODEC_HEVC },
    { AVCODECID(MPEG2VIDEO), RGY_CODEC_MPEG2 },
#if !ENCODER_VCEENC
    { AVCODECID(VP8),        RGY_CODEC_VP8 },
#endif
    { AVCODECID(VP9),        RGY_CODEC_VP9 },
#if !ENCODER_QSV
    { AVCODECID(VC1),        RGY_CODEC_VC1   },
#endif
#if ENCODER_NVENC
    { AVCODECID(MPEG1VIDEO), RGY_CODEC_MPEG1 },
    { AVCODECID(MPEG4),      RGY_CODEC_MPEG4 },
#endif
#if !ENCODER_VCEENC
    { AVCODECID(AV1),        RGY_CODEC_AV1 },
#endif
    //{ AVCODEC(WMV3),       RGY_CODEC_VC1   },
};
#if !ENABLE_AVSW_READER
#undef AVCodecID
#endif

#if ENABLE_AVSW_READER
#if _DEBUG
#define RGY_AV_LOG_LEVEL AV_LOG_WARNING
#else
#define RGY_AV_LOG_LEVEL AV_LOG_ERROR
#endif

template<typename T>
struct RGYAVDeleter {
    RGYAVDeleter() : deleter(nullptr) {};
    RGYAVDeleter(std::function<void(T**)> deleter) : deleter(deleter) {};
    void operator()(T *p) { deleter(&p); }
    std::function<void(T**)> deleter;
};

#define RGYPOOLAV_DEBUG 0
#define RGYPOOLAV_COUNT 0

template<typename T, T *Talloc(), void Tunref(T* ptr), void Tfree(T** ptr)>
class RGYPoolAV {
private:
    RGYQueueMPMP<T*> queue;
#if RGYPOOLAV_COUNT
    std::atomic<uint64_t> allocated, reused;
#endif
public:
    RGYPoolAV() : queue()
#if RGYPOOLAV_COUNT
        , allocated(0), reused(0)
#endif
    { queue.init(); }
    ~RGYPoolAV() {
#if RGYPOOLAV_COUNT
        fprintf(stderr, "RGYPoolAV: allocated %lld, reused %lld\n", allocated, reused);
#endif
        queue.close([](T **ptr) { Tfree(ptr); });
    }
    std::unique_ptr<T, RGYAVDeleter<T>> getUnique(T *ptr) {
        return std::unique_ptr<T, RGYAVDeleter<T>>(ptr, RGYAVDeleter<T>([this](T **ptr) { returnFree(ptr); }));
    }
    std::unique_ptr<T, RGYAVDeleter<T>> getFree() {
#if RGYPOOLAV_DEBUG
        T *ptr = Talloc();
#else
        T *ptr = nullptr;
        if (!queue.front_copy_and_pop_no_lock(&ptr) || ptr == nullptr) {
            ptr = Talloc();
#if RGYPOOLAV_COUNT
            allocated++;
#endif
        }
#if RGYPOOLAV_COUNT
        else {
            reused++;
        }
#endif
#endif
        return getUnique(ptr);
    }
    void returnFree(T **ptr) {
        if (ptr == nullptr || *ptr == nullptr) return;
#if RGYPOOLAV_DEBUG
        Tfree(ptr);
#else
        Tunref(*ptr);
        queue.push(*ptr);
        *ptr = nullptr;
#endif
    }
};

using RGYPoolAVPacket = RGYPoolAV<AVPacket, av_packet_alloc, av_packet_unref, av_packet_free>;
using RGYPoolAVFrame = RGYPoolAV<AVFrame, av_frame_alloc, av_frame_unref, av_frame_free>;

static inline AVCodecID getAVCodecId(RGY_CODEC codec) {
    for (int i = 0; i < _countof(HW_DECODE_LIST); i++)
        if (HW_DECODE_LIST[i].rgy_codec == codec)
            return HW_DECODE_LIST[i].avcodec_id;
    return AV_CODEC_ID_NONE;
}

static const AVPixelFormat HW_DECODE_PIXFMT_LIST[] = {
    AV_PIX_FMT_YUV420P,
    AV_PIX_FMT_YUVJ420P,
    AV_PIX_FMT_NV12,
    AV_PIX_FMT_YUV420P10LE,
#if ENCODER_NVENC
    AV_PIX_FMT_YUV420P12LE,
#endif
};

static const int AVQSV_DEFAULT_AUDIO_BITRATE = 192;

static inline bool av_isvalid_q(AVRational q) {
    return q.den * q.num != 0;
}
template<typename T>
static inline AVRational av_make_q(rgy_rational<T> r) {
    return av_make_q(r.n(), r.d());
}
static rgy_rational<int> to_rgy(AVRational r) {
    return rgy_rational<int>(r.num, r.den);
}

static inline bool avcodecIsCopy(const TCHAR *codec) {
    return codec == nullptr || 0 == _tcsicmp(codec, RGY_AVCODEC_COPY);
}
static inline bool avcodecIsAuto(const TCHAR *codec) {
    return codec != nullptr && 0 == _tcsicmp(codec, RGY_AVCODEC_AUTO);
}
static inline bool avcodecIsCopy(const tstring& codec) {
    return codec.length() == 0 || avcodecIsCopy(codec.c_str());
}
static inline bool avcodecIsAuto(const tstring &codec) {
    return codec.length() > 0 && avcodecIsAuto(codec.c_str());
}

//AV_LOG_TRACE    56 - RGY_LOG_TRACE -3
//AV_LOG_DEBUG    48 - RGY_LOG_DEBUG -2
//AV_LOG_VERBOSE  40 - RGY_LOG_MORE  -1
//AV_LOG_INFO     32 - RGY_LOG_INFO   0
//AV_LOG_WARNING  24 - RGY_LOG_WARN   1
//AV_LOG_ERROR    16 - RGY_LOG_ERROR  2
static inline RGYLogLevel log_level_av2rgy(int level) {
    return (RGYLogLevel)clamp((AV_LOG_INFO / 8) - (level / 8), RGY_LOG_TRACE, RGY_LOG_ERROR);
}

static inline int log_level_rgy2av(RGYLogLevel level) {
    return clamp(AV_LOG_INFO - level * 8, AV_LOG_QUIET, AV_LOG_TRACE);
}

//"<mes> for codec"型のエラーメッセージを作成する
static tstring errorMesForCodec(const TCHAR *mes, AVCodecID targetCodec) {
    return mes + tstring(_T(" for ")) + char_to_tstring(avcodec_get_name(targetCodec)) + tstring(_T(".\n"));
};

static const AVRational HW_NATIVE_TIMEBASE = { 1, (int)HW_TIMEBASE };
static const TCHAR *AVCODEC_DLL_NAME[] = {
    _T("avcodec-59.dll"), _T("avformat-59.dll"), _T("avutil-57.dll"), _T("avfilter-8.dll"), _T("swresample-4.dll")
};

enum RGYAVCodecType : uint32_t {
    RGY_AVCODEC_DEC = 0x01,
    RGY_AVCODEC_ENC = 0x02,
};

enum RGYAVFormatType : uint32_t {
    RGY_AVFORMAT_DEMUX = 0x01,
    RGY_AVFORMAT_MUX   = 0x02,
};

int64_t rational_rescale(int64_t v, rgy_rational<int> from, rgy_rational<int> to);

//NV_ENC_PIC_STRUCTから、AVFieldOrderを返す
AVFieldOrder picstrcut_rgy_to_avfieldorder(RGY_PICSTRUCT picstruct);

//AVFrameの情報からRGY_PICSTRUCTを返す
RGY_PICSTRUCT picstruct_avframe_to_rgy(const AVFrame *frame);

//avcodecのエラーを表示
tstring qsv_av_err2str(int ret);

//コーデックが一致するか確認
bool avcodec_equal(const std::string& codec, const AVCodecID id);

//コーデックが存在するか確認
bool avcodec_exists(const std::string& codec, const AVMediaType type = AVMEDIA_TYPE_NB);

//コーデックの種類を表示
tstring get_media_type_string(AVCodecID codecId);

// trackの言語を取得
std::string getTrackLang(const AVStream *stream);

//必要なavcodecのdllがそろっているかを確認
bool check_avcodec_dll();

//avcodecのdllが存在しない場合のエラーメッセージ
tstring error_mes_avcodec_dll_not_found();

//avcodecのライセンスがLGPLであるかどうかを確認
bool checkAvcodecLicense();

//avqsvでサポートされている動画コーデックを表示
tstring getHWDecSupportedCodecList();

//利用可能な音声エンコーダ/デコーダを表示
tstring getAVCodecs(RGYAVCodecType flag);

//音声エンコーダで利用可能なプロファイルのリストを作成
std::vector<tstring> getAudioPofileList(const tstring& codec_name);

//利用可能なフォーマットを表示
tstring getAVFormats(RGYAVFormatType flag);

//利用可能なフィルターを表示
tstring getAVFilters();

//チャンネルレイアウトを表示
std::string getChannelLayoutChar(int channels, uint64_t channel_layout);
tstring getChannelLayoutString(int channels, uint64_t channel_layout);

//時刻を表示
std::string getTimestampChar(int64_t ts, const AVRational& timebase);
tstring getTimestampString(int64_t ts, const AVRational& timebase);

//tag関連
uint32_t tagFromStr(std::string tagstr);
std::string tagToStr(uint32_t tag);

//利用可能なプロトコル情報のリストを取得
vector<std::string> getAVProtocolList(int bOutput);

//利用可能なプロトコル情報を取得
tstring getAVProtocols();

//protocolを使用
bool usingAVProtocols(const std::string& filename, int bOutput);

//バージョン情報の取得
tstring getAVVersions();

MAP_PAIR_0_1_PROTO(csp, avpixfmt, AVPixelFormat, rgy, RGY_CSP);

#define AV_DISPOSITION_UNSET (0x00000000)
#define AV_DISPOSITION_COPY  (0xffffffff)
MAP_PAIR_0_1_PROTO(disposition, str, tstring, av, uint32_t);

uint32_t parseDisposition(const tstring &disposition_str);
tstring getDispositionStr(uint32_t disposition);

#else
#define AV_NOPTS_VALUE (-1)
class RGYPoolAVPacket;
class RGYPoolAVFrame;
#endif //ENABLE_AVSW_READER

#endif //__RGY_AVUTIL_H__
