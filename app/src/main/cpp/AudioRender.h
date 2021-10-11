//
// Created by SHAREit on 2021/8/27.
//

#ifndef CAPTUREDEMO_AUDIORENDER_H
#define CAPTUREDEMO_AUDIORENDER_H

#include "RenderBase.h"
#include "common.h"
#include <thread>
#include <list>
#include <mutex>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>


class AudioRender : public  RenderBase {
public:
    AudioRender();
    ~AudioRender();
    int init(SinkConfig &cfg) override;
    int start() override;
    int stop() override;
    int input(const std::shared_ptr<Task> &task) override;
private:
    int InitEngine();
    int DestroyEngine();
    void threadFun();
    int fillBufferData(uint8_t *buffer, int size);
public:
    bool isRunning = false;
    std::mutex weekUpMutex;
    std::condition_variable weekUpCond;
private:
    std::list<std::pair<uint8_t *, size_t>> pcmList;
    std::mutex pcmListMutex;
    uint8_t* outputBuffer;
    int outputBufferIndex;
    int bytesPerBuffer;
    int bufferCapacity;
    std::thread renderThread;
    SLuint32 slChannelNum;
    SLuint32 slChannelMask;
    SLuint32 slSampleRate;
    SLObjectItf engineObject = nullptr;
    SLEngineItf engineEngine = nullptr;
    SLObjectItf outputMixObject = nullptr;
    SLVolumeItf volumeItf = nullptr;
    SLObjectItf playerObject = nullptr;
    SLPlayItf playerPlay = nullptr;
    SLAndroidSimpleBufferQueueItf playerBufferQueue = nullptr;
};


#endif //CAPTUREDEMO_AUDIORENDER_H
