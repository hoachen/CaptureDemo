//
// Created by jade on 2021/8/26.
//

#ifndef CAPTUREDEMO_COMMON_H
#define CAPTUREDEMO_COMMON_H

#include <iostream>

#define AV_PLANAR_NUM 8

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
