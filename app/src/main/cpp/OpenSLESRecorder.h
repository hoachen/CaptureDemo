//
// Created by chenhao on 2021/8/26.
//

#ifndef CAPTUREDEMO_OPENSLESRECORDER_H
#define CAPTUREDEMO_OPENSLESRECORDER_H

#include "common.h"
#include "RecorderBase.h"
#include <thread>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

class OpenSLESRecorder : public RecorderBase {

public:
    OpenSLESRecorder();
    ~OpenSLESRecorder();
    int init(SourceConfig &config) override;
    int start() override;
    int stop() override;
private:
    int InitEngine();
    int DestroyEngine();
    void threadFun();
//    void bpRecordCallBack(SLAndroidSimpleBufferQueueItf bq, void *context);
public:
    bool isRecording = false;
    uint8_t * inputBuffer[2] = {nullptr};
    int inputBufferIndex = 0;
    int bufferSize;
    SourceConfig audioConfig;
    uint32_t timestamp;
private:
    std::thread audioRecordThread;
    SLuint32 slChannelNum;
    SLuint32 slChannelMask;
    SLuint32 slSampleRate;
    SLObjectItf engineObject = nullptr;
    SLEngineItf engineEngine = nullptr;
    SLObjectItf recorderObject = nullptr;
    SLRecordItf recorderRecorder = nullptr;
    SLAndroidSimpleBufferQueueItf recordBufferQueue = nullptr;
};


#endif //CAPTUREDEMO_OPENSLESRECORDER_H
