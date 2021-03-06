//
// Created by chenhao on 2021/8/26.
//

#include "AudioRecorder.h"
#include "log.h"
#include <memory>


#define LOG_TAG "OpenSLRecorder"
// 20ms
#define BUFFER_TIME_MS 20

AudioRecorder::AudioRecorder() {

}

AudioRecorder::~AudioRecorder() {

}

static void bpRecordCallBack(SLAndroidSimpleBufferQueueItf bp, void *context);

int AudioRecorder::init(SourceConfig &config) {
    audioConfig = config;
    slChannelNum = static_cast<SLuint32>(config.channels);
    slChannelMask = (slChannelNum == 2)
                       ? SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT : SL_SPEAKER_BACK_CENTER;
    slSampleRate = SL_SAMPLINGRATE_16;
//    switch (audioConfig.sampleRate) {
//        case 8000:
//            slSampleRate = SL_SAMPLINGRATE_8;
//            break;
//        case 11025:
//            slSampleRate = SL_SAMPLINGRATE_11_025;
//            break;
//        case 12000:
//            slSampleRate = SL_SAMPLINGRATE_12;
//            break;
//        case 16000:
//            slSampleRate = SL_SAMPLINGRATE_16;
//            break;
//        case 44100:
//            slSampleRate = SL_SAMPLINGRATE_44_1;
//            break;
//        case 48000:
//            slSampleRate = SL_SAMPLINGRATE_48;
//            break;
//        case 64000:
//            slSampleRate = SL_SAMPLINGRATE_64;
//            break;
//        default:
//            slSampleRate = SL_SAMPLINGRATE_44_1;
//            break;
//    }
    bufferSize = 16000 * BUFFER_TIME_MS / 1000 * config.channels * 2;
    timestamp = 0;
   return QCODE_OK;
}

int AudioRecorder::InitEngine() {
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
    return QCODE_OK;
}

static void bpRecordCallBack(SLAndroidSimpleBufferQueueItf bp, void *context)
{
    ALOGI(TAG, "bpRecordCallBack");
    AudioRecorder *recorder = reinterpret_cast<AudioRecorder *>(context);
    if (recorder == nullptr)
        return;

    if (recorder->isRecording) {
        if (recorder->output) {
            uint8_t pcmData[recorder->bufferSize];
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
}

void AudioRecorder::threadFun() {
    std::thread::id tid = std::this_thread::get_id();
    ALOGI(LOG_TAG, "start record thread thread-id= %lu",tid);

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
            SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,    //?????? ???????????????SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE
            2                                           //buffer?????????
    };

    SLDataFormat_PCM format_pcm = {
            SL_DATAFORMAT_PCM,                             //??????PCM???????????????
            slChannelNum,                                  //?????????????????????
            slSampleRate,                          //?????????????????????????????????44100Hz
            SL_PCMSAMPLEFORMAT_FIXED_16,                   //?????????????????????????????????16bit
            SL_PCMSAMPLEFORMAT_FIXED_16,                   //????????????????????????????????????
            slChannelMask,  //?????????????????????????????????????????? SL_SPEAKER_FRONT_CENTER
            SL_BYTEORDER_LITTLEENDIAN                      //PCM????????????????????????
    };
    SLDataSink audioSink = {
            &buffer_queue,                   //SLDataFormat_PCM????????????
            &format_pcm                      //??????????????????
    };
    const SLInterfaceID id[1] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    // ??????recordObject
    result = (*engineEngine)->CreateAudioRecorder(engineEngine,        //????????????
                                          &recorderObject,   //???????????????????????????????????????
                                          &dataSource,          //????????????
                                          &audioSink,         //????????????
                                          1,                  //?????????????????????
                                          id,                 //???????????????????????????
                                          req                 //??????????????????????????????????????????????????????
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

    // ????????????
    (*recorderRecorder)->SetRecordState(recorderRecorder, SL_RECORDSTATE_RECORDING);
    inputBufferIndex= 0;
    if ((inputBuffer[0] = (uint8_t *)calloc(bufferSize, sizeof(uint8_t))) == nullptr ||
            (inputBuffer[1] = (uint8_t *)calloc(bufferSize, sizeof(uint8_t))) == nullptr) {
        ALOGE(TAG, "oom calloc buffer failed");
        return;
    }
    (*recordBufferQueue)->Enqueue(recordBufferQueue, inputBuffer[inputBufferIndex], bufferSize);
    // ??????????????????
    while (isRecording) {

    }
    ALOGI(TAG, "stop Audio Record...");
    if (recorderRecorder != nullptr) {
        (*recorderRecorder)->SetRecordState(recorderRecorder, SL_RECORDSTATE_STOPPED);
    }
    // ????????????
    DestroyEngine();
}

int AudioRecorder::start() {
    std::thread::id tid = std::this_thread::get_id();
    ALOGI(LOG_TAG, "call start() thread-id= %lu",tid);
    if (isRecording)
        return 0;
    isRecording = true;
    audioRecordThread = std::thread(&AudioRecorder::threadFun, this);
    return QCODE_OK;
}

int AudioRecorder::DestroyEngine() {
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
    ALOGI(TAG, "Destroy Engine fine");
    return QCODE_OK;
}

int AudioRecorder::stop() {
    std::thread::id tid = std::this_thread::get_id();
    ALOGI(LOG_TAG, "call stop() thread-id= %lu",tid);
    isRecording = false;
    if (audioRecordThread.joinable())
        audioRecordThread.join();
    return QCODE_OK;
}


