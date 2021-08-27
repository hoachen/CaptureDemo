//
// Created by SHAREit on 2021/8/27.
//

#ifndef CAPTUREDEMO_OPENSLESRENDER_H
#define CAPTUREDEMO_OPENSLESRENDER_H

#include "RenderBase.h"
#include "common.h"
#include <thread>
#include <list>
#include <mutex>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>



class OpenSLESRender : public  RenderBase {
public:
    OpenSLESRender();
    ~OpenSLESRender();
    int init(SinkConfig &cfg) override;
    int start() override;
    int stop() override;
    int input(const std::shared_ptr<Task> &task) override;
private:
    int InitEngine();
    int DestroyEngine();
    void threadFun();
public:
    bool isRunning = false;
private:
    std::list<uint8_t> pcmList;
    std::mutex pcmListMutex;
    std::mutex weekUpMutex;
    std::condition_variable weekUpCond;
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


#endif //CAPTUREDEMO_OPENSLESRENDER_H
