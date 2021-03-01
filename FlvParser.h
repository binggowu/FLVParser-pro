#ifndef FLVPARSER_H
#define FLVPARSER_H

#include <stdint.h>
#include <iostream>
#include <vector>
#include "Videojj.h"
using namespace std;

// typedef unsigned long long uint64_t;

class CFlvParser
{
public:
    CFlvParser();
    virtual ~CFlvParser();

    int Parse(uint8_t *pBuf, int nBufSize, int &nUsedLen);

    int PrintInfo();

    int DumpH264(const std::string &path);
    int DumpAAC(const std::string &path);
    int DumpFlv(const std::string &path);

private:
    // FLV头
    typedef struct FlvHeader_s
    {
        int nVersion;   // 版本
        int bHaveVideo; // 是否包含视频
        int bHaveAudio; // 是否包含音频
        int nHeadSize;  // FLV头部长度

        uint8_t *pFlvHeader; // 真实的FLV头部的二进制比特串
    } FlvHeader;

    // Tag头部
    struct TagHeader
    {
        int nType;      // 类型
        int nDataSize;  // Tag Body的大小
        int nTimeStamp; // 时间戳
        int nTSEx;      // 时间戳的扩展字节
        int nStreamID;  // 流的ID, 总是0

        uint32_t nTotalTS; // 完整的时间戳, nTimeStamp和nTSEx拼装

        TagHeader() : nType(0), nDataSize(0), nTimeStamp(0), nTSEx(0), nStreamID(0), nTotalTS(0) {}
        ~TagHeader() {}
    };

    class Tag
    {
    public:
        Tag() : _pTagHeader(NULL), _pTagData(NULL), _pMedia(NULL), _nMediaLen(0) {}
        void Init(TagHeader *pHeader, uint8_t *pBuf, int nLeftLen);

        // 在Init()中初始化下面3个成员变量
        TagHeader _header;
        uint8_t *_pTagHeader; // Tag Headler 的二进制数据
        uint8_t *_pTagData;   // Tag Body 的二进制数据

        uint8_t *_pMedia; // 指向标签的元数据, 解析后的数据
        int _nMediaLen;   // 元数据的长度
    };

    // 视频Tag
    class CVideoTag : public Tag
    {
    public:
        /**
         * @brief CVideoTag
         * @param pHeader
         * @param pBuf 整个tag的起始地址
         * @param nLeftLen
         * @param pParser
         */
        CVideoTag(TagHeader *pHeader, uint8_t *pBuf, int nLeftLen, CFlvParser *pParser);

        int _nFrameType; // 帧类型
        int _nCodecID;   // 视频编解码ID(7:AVC)

        int ParseH264Tag(CFlvParser *pParser);
        int ParseH264Configuration(CFlvParser *pParser, uint8_t *pTagData);
        int ParseNalu(CFlvParser *pParser, uint8_t *pTagData);
    };

    // 音频Tag
    class CAudioTag : public Tag
    {
    public:
        CAudioTag(TagHeader *pHeader, uint8_t *pBuf, int nLeftLen, CFlvParser *pParser);

        int _nSoundFormat; // 音频编码类型
        int _nSoundRate;   // 采样率
        int _nSoundSize;   // 精度
        int _nSoundType;   // 类型

        // aac
        static int _aacProfile;      // 对应AAC profile
        static int _sampleRateIndex; // 采样率索引
        static int _channelConfig;   // 通道设置

        int ParseAACTag(CFlvParser *pParser);
        int ParseAudioSpecificConfig(CFlvParser *pParser, uint8_t *pTagData);
        int ParseRawAAC(CFlvParser *pParser, uint8_t *pTagData);
    };

    // Script Tag
    class CMetaDataTag : public Tag
    {
    public:
        CMetaDataTag(TagHeader *pHeader, uint8_t *pBuf, int nLeftLen, CFlvParser *pParser);

        double hexStr2double(const unsigned char *hex, const unsigned int length);
        int parseMeta(CFlvParser *pParser);
        void printMeta();

        uint8_t m_amf1_type;  // AMF1包类型, 总是0x02, 表示字符串.
        uint32_t m_amf1_size; // 字符串的⻓度, 一般总是0x00 0A ("onMetaData"⻓度)
        uint8_t m_amf2_type;  // AMF2包类型, 一般总是0x08, 表示数组.
        unsigned char *m_meta;
        unsigned int m_length;

        double m_duration;      // 时⻓(秒)
        double m_width;         // 视频宽度
        double m_height;        // 视频高度
        double m_videodatarate; // 视频码率
        double m_framerate;     // 视频帧率
        double m_videocodecid;  // 视频编码ID

        double m_audiodatarate;   // 音频码率
        double m_audiosamplerate; // 音频采样率
        double m_audiosamplesize; //
        bool m_stereo;            // 是否立体声
        double m_audiocodecid;    // 音频编码ID

        string m_major_brand;       // 格式规范相关
        string m_minor_version;     // 格式规范相关
        string m_compatible_brands; // 格式规范相关
        string m_encoder;           // 封装工具名称, 如Lavf54.63.104
        double m_filesize;          // 文件大小(字节)
    };

    struct FlvStat
    {
        int nMetaNum, nVideoNum, nAudioNum;
        int nMaxTimeStamp;
        int nLengthSize;

        FlvStat() : nMetaNum(0), nVideoNum(0), nAudioNum(0), nMaxTimeStamp(0), nLengthSize(0) {}
        ~FlvStat() {}
    };

    static uint32_t ShowU32(uint8_t *pBuf)
    {
        // 大端模式
        return (pBuf[0] << 24) | (pBuf[1] << 16) | (pBuf[2] << 8) | pBuf[3];
    }

    static uint32_t ShowU24(uint8_t *pBuf) { return (pBuf[0] << 16) | (pBuf[1] << 8) | (pBuf[2]); }
    static uint32_t ShowU16(uint8_t *pBuf) { return (pBuf[0] << 8) | (pBuf[1]); }
    static uint32_t ShowU8(uint8_t *pBuf) { return (pBuf[0]); }
    static void WriteU64(uint64_t &x, int length, int value)

    {
        uint64_t mask = 0xFFFFFFFFFFFFFFFF >> (64 - length);
        x = (x << length) | ((uint64_t)value & mask);
    }

    static uint32_t WriteU32(uint32_t n)
    {
        uint32_t nn = 0;
        uint8_t *p = (uint8_t *)&n;
        uint8_t *pp = (uint8_t *)&nn;
        pp[0] = p[3];
        pp[1] = p[2];
        pp[2] = p[1];
        pp[3] = p[0];
        return nn;
    }

    friend class Tag;

private:
    FlvHeader *CreateFlvHeader(uint8_t *pBuf);
    int DestroyFlvHeader(FlvHeader *pHeader);
    Tag *CreateTag(uint8_t *pBuf, int nLeftLen);
    int DestroyTag(Tag *pTag);
    int Stat();
    int StatVideo(Tag *pTag);
    int IsUserDataTag(Tag *pTag);

private:
    FlvHeader *_pFlvHeader;
    vector<Tag *> _vpTag;
    FlvStat _sStat;
    CVideojj *_vjj;

    int _nNalUnitLength; // NalUnit长度表示占用的字节
};

#endif // FLVPARSER_H
