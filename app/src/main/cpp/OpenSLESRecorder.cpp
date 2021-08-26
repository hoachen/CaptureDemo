//
// Created by chenhao on 2021/8/26.
//

#include "OpenSLESRecorder.h"
#include "log.h"
#include <memory>


#define LOG_TAG "OpenSLRecorder"
// 20ms
#define BUFFER_TIME_MS 20

OpenSLESRecorder::OpenSLESRecorder() {

}

OpenSLESRecorder::~OpenSLESRecorder() {

}

static void bpRecordCallBack(SLAndroidSimpleBufferQueueItf bp, void *context);

int OpenSLESRecorder::Init(SourceConfig &config) {
    audioConfig = config;
    slChannelNum = static_cast<SLuint32>(config.channels);
    slChannelMask = (slChannelNum == 2)
                       ? SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT : SL_SPEAKER_BACK_CENTER;
    slSampleRate = SL_SAMPLINGRATE_44_1;
    switch (audioConfig.sampleRate) {
        case 8000:
            slSampleRate = SL_SAMPLINGRATE_8;
            break;
        case 11025:
            slSampleRate = SL_SAMPLINGRATE_11_025;
            break;
        case 12000:
            slSampleRate = SL_SAMPLINGRATE_12;
            break;
        case 16000:
            slSampleRate = SL_SAMPLINGRATE_16;
            break;
        case 44100:
            slSampleRate = SL_SAMPLINGRATE_44_1;
            break;
        case 48000:
            slSampleRate = SL_SAMPLINGRATE_48;
            break;
        case 64000:
            slSampleRate = SL_SAMPLINGRATE_64;
            break;
        default:
            slSampleRate = SL_SAMPLINGRATE_44_1;
            break;
    }
    bufferSize = config.sampleRate * BUFFER_TIME_MS / 1000 * config.channels;
    timestamp = 0;
   return 0;
}

int OpenSLESRecorder::InitEngine() {
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
    return 0;
}

static void bpRecordCallBack(SLAndroidSimpleBufferQueueItf bp, void *context)
{
    OpenSLESRecorder *recorder = reinterpret_cast<OpenSLESRecorder *>(context);
    uint8_t *pcmData = new uint8_t [recorder->bufferSize];

    if (recorder->isRecording) {
        if (recorder->output) {
            std::shared_ptr<Task> task(new Task());
            task->channels = recorder->audioConfig.channels;
            task->sampleRate = recorder->audioConfig.sampleRate;
            task->bitDepth = recorder->audioConfig.bitDepth;
            memcpy(pcmData, recorder->inputBuffer[recorder->inputBufferIndex], recorder->bufferSize);
            task->data[0] = pcmData;
            task->linesize[0] = recorder->bufferSize;
            task->timestamp = recorder->timestamp;
            recorder->output(task);
        }
        recorder->timestamp += BUFFER_TIME_MS;
        recorder->inputBufferIndex = recorder->inputBufferIndex ? 0 : 1;
        (*bp)->Enqueue(bp, recorder->inputBuffer[recorder->inputBufferIndex], recorder->bufferSize);
    }
    delete []pcmData;
}

void OpenSLESRecorder::threadFun() {
    ALOGI(LOG_TAG, "start record thread");
    SLresult result;
    if ((result = InitEngine()) != 0) {
        ALOGE(LOG_TAG, "init audio engine failed");
        return;
    }
    SLDataLocator_IODevice ioDevice = {
            SL_DATALOCATOR_IODEVICE,
            SL_IODEVICE_AUDIOINPUT,
            SL_DEFAULTDEVICEID_AUDIOINPUT,
            nullptr
    };
    SLDataSource dataSource = {
            &ioDevice,
            nullptr
    };

    SLDataLocator_AndroidSimpleBufferQueue buffer_queue = {
            SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,    //类型 这里只能是SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE
            2                                           //buffer的数量
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
    SLDataSink audioSink = {
            &buffer_queue,                   //SLDataFormat_PCM配置输出
            &format_pcm                      //输出数据格式
    };
    const SLInterfaceID id[1] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    // 创建recordObject
    result = (*engineEngine)->CreateAudioRecorder(engineEngine,        //引擎接口
                                          &recorderObject,   //录制对象地址，用于传出对象
                                          &dataSource,          //输入配置
                                          &audioSink,         //输出配置
                                          1,                  //支持的接口数量
                                          id,                 //具体的要支持的接口
                                          req                 //具体的要支持的接口是开放的还是关闭的
    );
    if (result != SL_RESULT_SUCCESS) {
        ALOGE(TAG, "create recorder object failed %u", result);
        DestroyEngine();
        return;
    }
    result = (*recorderObject)->Realize(recorderObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        ALOGE(LOG_TAG, "create recorder object failed %u", result);
        DestroyEngine();
        return;
    }
    result = (*recorderObject)->GetInterface(recorderObject, SL_IID_RECORD, &recorderRecorder);
    if (result != SL_RESULT_SUCCESS) {
        ALOGE(LOG_TAG, "create recorder object failed %u", result);
        DestroyEngine();
        return;
    }
    result = (*recorderObject)->GetInterface(recorderObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &recordBufferQueue);

    result = (*recordBufferQueue)->RegisterCallback(recordBufferQueue,
                                                    bpRecordCallBack, this);

    // 开始录制
    (*recorderRecorder)->SetRecordState(recorderRecorder, SL_RECORDSTATE_RECORDING);

    inputBufferIndex= 0;
    if ((inputBuffer[0] = (uint8_t *)calloc(bufferSize, sizeof(uint8_t))) == nullptr ||
            (inputBuffer[1] = (uint8_t *)calloc(bufferSize, sizeof(uint8_t))) == nullptr) {
        ALOGE(TAG, "oom calloc buffer failed");
        return;
    }
    (*recordBufferQueue)->Enqueue(recordBufferQueue, inputBuffer[inputBufferIndex], bufferSize);
    // 设置开始录音
    while (isRecording) {

    }
    ALOGI(TAG, "Stop Audio Record...");
    if (recorderRecorder != nullptr) {
        (*recorderRecorder)->SetRecordState(recorderRecorder, SL_RECORDSTATE_STOPPED);
    }
    // 结束录音
    DestroyEngine();
}

int OpenSLESRecorder::Start() {
    if (isRecording)
        return 0;
    isRecording = true;
    audioRecordThread = std::thread(&OpenSLESRecorder::threadFun, this);
    return 0;
}

int OpenSLESRecorder::DestroyEngine() {
    if (recorderObject != nullptr) {
        (*recorderObject)->Destroy(recorderObject);
        recorderObject = nullptr;
        recorderRecorder = nullptr;
        recordBufferQueue = nullptr;
    }
    if (engineObject != nullptr) {
        (*engineObject)->Destroy(engineObject);
        engineEngine = nullptr;
        engineObject = nullptr;
    }
    if (inputBuffer[0] != nullptr) {
        free(inputBuffer[0]);
    }
    if (inputBuffer[1] != nullptr)
        free(inputBuffer[1]);
    return 0;
}

int OpenSLESRecorder::Stop() {
    isRecording = false;
    if (audioRecordThread.joinable())
        audioRecordThread.join();
    return 0;
}


