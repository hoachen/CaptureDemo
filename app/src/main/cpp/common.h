//
// Created by jade on 2021/8/26.
//

#ifndef CAPTUREDEMO_COMMON_H
#define CAPTUREDEMO_COMMON_H

#include <iostream>

#define AV_PLANAR_NUM 8

enum QCODE {
    QCODE_OK = 0,     //正常
    QCODE_ERROR = 1,  //错误
    QCODE_FORK = 2,   //强制
    QCODE_UNKNOWN = 3 //未知
};


enum StreamType {
    stream_unknown = 0,
    stream_audio = 1,
    stream_video = 2,
};

enum ModuleID {
    module_unknown = 0,

    module_video_record = 1,
    module_video_render = 2,

    module_audio_record = 3,
    module_audio_render = 4,

    module_h264_encoder = 5,
    module_h264_decoder = 6,

    module_aac_encoder = 7,
    module_aac_decoder = 8,

    module_opus_encoder = 9,
    module_opus_decoder = 10,

    module_audio_recv = 11,
    module_video_recv = 12,
};

struct SinkConfig {
    int x = 0;  //x坐标
    int y = 0;  //y坐标
    int w = 0;  //宽度
    int h = 0;  //高度

    uint32_t flags = 0;    //私有flag设置，移动端暂不使用
    std::string windowsTitle;   //显示窗口的title
    StreamType streamType; //流类型

    uint8_t *view = nullptr;//渲染视图

    ModuleID moduleID; //模块ID
};

struct SourceConfig {
    int width = 0;
    int height = 0;
    int frameRate = 25;
    int sampleRate = 44100;
    int channels = 1;
    int bitrate;
    int bitDepth = 8;
};

struct Task {
    int width = 0;
    int height = 0;
    int bitDepth = 8;
    int sampleRate = 44100;
    int channels = 1;
    uint32_t timestamp = 0;
    uint8_t *data[AV_PLANAR_NUM] = {nullptr};
    int linesize[AV_PLANAR_NUM] = {0};
};

#endif //CAPTUREDEMO_COMMON_H
