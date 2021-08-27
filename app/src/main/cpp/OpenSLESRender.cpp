//
// Created by SHAREit on 2021/8/27.
//

#include "OpenSLESRender.h"
#include "log.h"

#define LOG_TAG "OpenSLESRender"

/** ms **/
#define TIME_PER_BUFFER 20
#define BUFFER_COUNT 255

OpenSLESRender::OpenSLESRender() {

}

OpenSLESRender::~OpenSLESRender() {

}

static void aout_opensles_callback(SLAndroidSimpleBufferQueueItf bp, void *pContext) {

}

int OpenSLESRender::init(SinkConfig &cfg) {
    this->cfg = cfg;
    slChannelNum = 1;
    slChannelMask = slChannelNum == 1 ?
            SL_SPEAKER_FRONT_CENTER : SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
    slSampleRate = SL_SAMPLINGRATE_16;
    if (InitEngine() != QCODE_OK) {
        return QCODE_ERROR;
    }
    return QCODE_OK;
}

int OpenSLESRender::InitEngine() {
    SLresult result;
    result = slCreateEngine(&engineObject, 0, nullptr, 0, nullptr, nullptr);
    if (SL_RESULT_SUCCESS != result) {
        ALOGE(LOG_TAG, "create engine object failed %u", result);
        return result;
    }
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        ALOGE(LOG_TAG, "Realize engine object failed %u", result);
        return result;
    }
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    if (SL_RESULT_SUCCESS != result) {
        ALOGE(LOG_TAG, "Get engine interface failed %u", result);
        return result;
    }


    const SLInterfaceID ids1[] = {SL_IID_VOLUME};
    const SLboolean req1[] = {SL_BOOLEAN_FALSE};
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, ids1, req1);
    if (SL_RESULT_SUCCESS != result) {
        ALOGE(LOG_TAG, "CreateOutputMix failed %u", result);
        return result;
    }
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        ALOGE(LOG_TAG, "Realize OutputMix failed %u", result);
        return result;
    }
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {
            SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
            BUFFER_COUNT
    };
    SLDataFormat_PCM format_pcm = {
            SL_DATAFORMAT_PCM,                             //输出PCM格式的数据
            slChannelNum,                                  //输出的声道数量
            slSampleRate,                          //输出的采样频率，这里是44100Hz
            SL_PCMSAMPLEFORMAT_FIXED_16,                   //输出的采样格式，这里是16bit
            SL_PCMSAMPLEFORMAT_FIXED_16,                   //一般来说，跟随上一个参数
            slChannelMask,  //双声道配置，如果单声道可以用 SL_SPEAKER_FRONT_CENTER
            SL_BYTEORDER_LITTLEENDIAN                      //PCM数据的大小端排列
    };

    SLDataSource audioSource = {&loc_bufq, &format_pcm};
    SLDataLocator_OutputMix locOutputMix = {
            SL_DATALOCATOR_OUTPUTMIX,
            outputMixObject
    };
    SLDataSink audioSink = {&locOutputMix, NULL};
    const SLInterfaceID ids2[] = { SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_VOLUME, SL_IID_PLAY };
    static const SLboolean req2[] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };
    result = (*engineEngine)->CreateAudioPlayer(engineEngine, &playerObject, &audioSource,
                                         &audioSink, sizeof(ids2) / sizeof(*ids2),
                                         ids2, req2);
    if (SL_RESULT_SUCCESS != result) {
        ALOGE(LOG_TAG, "Create PlayerObject failed %u", result);
        return result;
    }
    result = (*playerObject)->Realize(playerObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        ALOGE(LOG_TAG, "Realize PlayerObject failed %u", result);
        return result;
    }
    result = (*playerObject)->GetInterface(playerObject, SL_IID_PLAY, &playerPlay);
    if (SL_RESULT_SUCCESS != result) {
        ALOGE(LOG_TAG, "GetInterface PlayerItf failed %u", result);
        return result;
    }
    result = (*playerObject)->GetInterface(playerObject, SL_IID_VOLUME, &volumeItf);
    if (SL_RESULT_SUCCESS != result) {
        ALOGE(LOG_TAG, "GetInterface volumeItf failed %u", result);
        return result;
    }
    result = (*playerObject)->GetInterface(playerObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &playerBufferQueue);
    if (SL_RESULT_SUCCESS != result) {
        ALOGE(LOG_TAG, "GetInterface volumeItf failed %u", result);
        return result;
    }
    result = (*playerBufferQueue)->RegisterCallback(playerBufferQueue, aout_opensles_callback, this);
    if (SL_RESULT_SUCCESS != result) {
        ALOGE(LOG_TAG, "RegisterCallback failed %u", result);
        return result;
    }

    return QCODE_OK;
}

int OpenSLESRender::DestroyEngine() {

}

void OpenSLESRender::threadFun() {
}

int OpenSLESRender::input(const std::shared_ptr<Task> &task) {

}

int OpenSLESRender::start() {
    if (isRunning)
        return QCODE_OK;
    isRunning = true;
    renderThread = std::thread(&OpenSLESRender::threadFun, this);
    return QCODE_OK;
}

int OpenSLESRender::stop() {
    std::thread::id tid = std::this_thread::get_id();
    ALOGI(LOG_TAG, "call stop() thread-id= %lu",tid);
    isRunning = false;
    if (rendrThread.joinable())
        renderThread.join();
    return QCODE_OK;
}