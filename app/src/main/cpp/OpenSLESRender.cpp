//
// Created by SHAREit on 2021/8/27.
//

#include "OpenSLESRender.h"
#include "log.h"
#include <chrono>

#define LOG_TAG "OpenSLESRender"

/** ms **/
#define TIME_PER_BUFFER 20
#define BUFFER_COUNT 255

OpenSLESRender::OpenSLESRender() {

}

OpenSLESRender::~OpenSLESRender() {
}

static void opensles_callback(SLAndroidSimpleBufferQueueItf bp, void *pContext) {
    ALOGI(LOG_TAG, "opensles_callback");
    OpenSLESRender *render = (OpenSLESRender *)(pContext);
    std::lock_guard<std::mutex> guard(render->weekUpMutex);
    render->weekUpCond.notify_one();
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
    result = (*playerBufferQueue)->RegisterCallback(playerBufferQueue, opensles_callback, this);
    if (SL_RESULT_SUCCESS != result) {
        ALOGE(LOG_TAG, "RegisterCallback failed %u", result);
        return result;
    }
    int bytesPerFrame = slChannelNum * 2; // 一个采样占16位，2 byte
    int milliPerBuffer = TIME_PER_BUFFER; // 每个buffer 20ms
    int sr = 16000;
    int framesPerBuffer = milliPerBuffer * sr / 1000; // 每s 16000个采样， 20m个采样
    bytesPerBuffer = bytesPerFrame * framesPerBuffer; // 每个buffer占多大
    bufferCapacity = BUFFER_COUNT * bytesPerBuffer; // 所有buffer占的大小
    outputBuffer = new uint8_t [bufferCapacity];
    if (outputBuffer == nullptr) {
        return QCODE_ERROR;
    }
    return QCODE_OK;
}

int OpenSLESRender::DestroyEngine() {
    if (playerBufferQueue)
        (*playerBufferQueue)->Clear(playerBufferQueue);

    if (playerBufferQueue)
        playerBufferQueue = nullptr;
    if (volumeItf)
        volumeItf = nullptr;
    if (playerPlay)
        playerPlay = nullptr;

    if (playerObject) {
        (*playerObject)->Destroy(playerObject);
        playerObject = nullptr;
    }
    if (engineEngine)
        engineEngine = nullptr;
    if (engineObject) {
        (*engineObject)->Destroy(engineObject);
        engineObject = nullptr;
    }
    if (outputBuffer)
        delete[] outputBuffer;

}

int OpenSLESRender::fillBufferData(uint8_t *buffer, int size) {
    std::lock_guard<std::mutex> guard(pcmListMutex);
    int needSize = size;
    while (needSize > 0 && !pcmList.empty())
    if (!pcmList.empty()) {
        std::pair<uint8_t *, size_t> node = pcmList.front();
        pcmList.pop_front();
        uint8_t *data = node.first;
        int dataLen = node.second;
        memcpy(buffer, data, dataLen);
        buffer+= dataLen;
        needSize -= dataLen;
    }
    return 0;
}

void OpenSLESRender::threadFun() {
    SLresult ret;
    outputBufferIndex = 0;
    memset(outputBuffer, 0, bufferCapacity);
    uint8_t *nextBuffer;
    (*playerPlay)->SetPlayState(playerPlay, SL_PLAYSTATE_PLAYING);
    while (isRunning) {
        SLAndroidSimpleBufferQueueState slState = {0};
        ret = (*playerBufferQueue)->GetState(playerBufferQueue, &slState);
        if (ret != SL_RESULT_SUCCESS) {
            ALOGE("%s: slBufferQueueItf->GetState() failed\n", __func__);
        }
        std::unique_lock<std::mutex> lck(weekUpMutex);
        if (isRunning && slState.count >= BUFFER_COUNT) {
            while (isRunning && slState.count >= BUFFER_COUNT) {
                ALOGI(LOG_TAG, "Wait buffer queue 1s start");
                weekUpCond.wait_for(lck, std::chrono::microseconds (1000));
                ALOGI(LOG_TAG, "Wait buffer queue 1s end");
                SLresult slRet = (*playerBufferQueue)->GetState(playerBufferQueue, &slState);
                if (slRet != SL_RESULT_SUCCESS) {
                    ALOGE("%s: slBufferQueueItf->GetState() failed\n", __func__);
                }
            }
        }
        ALOGI(LOG_TAG, "try send next buffer");
        nextBuffer = outputBuffer + outputBufferIndex * bytesPerBuffer;
        outputBufferIndex = (outputBufferIndex + 1) % BUFFER_COUNT;
        fillBufferData(nextBuffer, bytesPerBuffer);
        ret = (*playerBufferQueue)->Enqueue(playerBufferQueue, nextBuffer, bytesPerBuffer);
        if (ret == SL_RESULT_SUCCESS) {
            // do nothing
        } else if (ret == SL_RESULT_BUFFER_INSUFFICIENT) {
                // don't retry, just pass through
            ALOGE(LOG_TAG, "SL_RESULT_BUFFER_INSUFFICIENT\n");
        } else {
            ALOGE(LOG_TAG, "slBufferQueueItf->Enqueue() = %d\n", (int)ret);
            break;
        }
    }
    (*playerPlay)->SetPlayState(playerPlay, SL_PLAYSTATE_STOPPED);
}

int OpenSLESRender::input(const std::shared_ptr<Task> &task) {
    ALOGI(LOG_TAG, "Rec input data %d", task->linesize[0]);
    std::lock_guard<std::mutex> lck (pcmListMutex);
    int dataSize = task->linesize[0];
    uint8_t *buf = new uint8_t[task->linesize[0]];
    memcpy(buf, task->data[0], dataSize);
    pcmList.push_back(std::pair<uint8_t *, size_t>(buf, dataSize));
    return QCODE_OK;
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
    std::lock_guard<std::mutex> lck (weekUpMutex);
    isRunning = false;
    weekUpCond.notify_one();
    if (renderThread.joinable())
        renderThread.join();
    DestroyEngine();
    return QCODE_OK;
}