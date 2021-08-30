#include <jni.h>
#include <string>
#include <memory>
#include <thread>
#include "RecorderBase.h"
#include "OpenSLESRecorder.h"
#include "RenderBase.h"
#include "OpenSLESRender.h"
#include "log.h"

static FILE *file;

extern "C" JNIEXPORT jstring JNICALL
Java_com_video_editor_capturedemo_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

static void audioRecordCallBack(std::shared_ptr<Task> &task)
{
    std::thread::id tid = std::this_thread::get_id();
    LOGI("CHH revc audio ts:%lu size =%d thread-id =%lu", task->timestamp, task->linesize[0], tid);

    if (file)
        fwrite(task->data[0], 1, task->linesize[0], file);
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_video_editor_capturedemo_MainActivity_createAudioRecorder(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    RecorderBase  *recorderBase = new OpenSLESRecorder();
    recorderBase->output = audioRecordCallBack;
    SourceConfig conf;
    conf.channels = 1;
    conf.sampleRate = 16000;
    conf.bitDepth = 8;
    recorderBase->init(conf);
    return reinterpret_cast<long>(recorderBase);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_video_editor_capturedemo_MainActivity_startRecordAudio (
        JNIEnv* env,
        jobject /* this */,
        jlong fd,
        jstring path) {
    const char *filePath = env->GetStringUTFChars(path, nullptr);
    file = fopen(filePath, "wb");
    RecorderBase  *recorderBase = reinterpret_cast<RecorderBase *>(fd);
    env->ReleaseStringUTFChars(path, filePath);
    return recorderBase->start();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_video_editor_capturedemo_MainActivity_stopRecordAudio(
        JNIEnv* env,
        jobject /* this */,
        jlong fd) {
    RecorderBase  *recorderBase = reinterpret_cast<RecorderBase *>(fd);
    recorderBase->stop();
    fclose(file);
    delete recorderBase;
    return 0;
}


static void audioRenderCallBack(std::shared_ptr<Task> &task)
{
    std::thread::id tid = std::this_thread::get_id();
    LOGI("CHH revc audio ts:%lu size =%d thread-id =%lu", task->timestamp, task->linesize[0], tid);

    if (file)
        fwrite(task->data[0], 1, task->linesize[0], file);
}


extern "C" JNIEXPORT jlong JNICALL
Java_com_video_editor_capturedemo_MainActivity_createAudioRender(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    RenderBase  *renderBase = new OpenSLESRender();
    SinkConfig conf;
    renderBase->init(conf);
    return reinterpret_cast<long>(renderBase);
}


extern "C" JNIEXPORT jint JNICALL
Java_com_video_editor_capturedemo_MainActivity_startRenderAudio (
        JNIEnv* env,
        jobject /* this */,
        jlong fd,
        jstring path) {
    int ret = 0;
    const char *filePath = env->GetStringUTFChars(path, nullptr);
    file = fopen(filePath, "rb");
    RenderBase  *renderBase = reinterpret_cast<RenderBase *>(fd);
    env->ReleaseStringUTFChars(path, filePath);
    renderBase->start();

    int bufferSize = 20 * 16000 / 1000 * 2 * 1;
    uint8_t buff[bufferSize];

    while (1) {
        size_t readSize = fread(buff, 1, bufferSize, file);
        if (readSize <= 0) {
            LOGI("CHH read pcm eof");
            fseek(file, 0, SEEK_SET);
            continue;
        }
        LOGI("CHH read pcm size %d", readSize);
        uint8_t *data = new uint8_t[readSize];
        memcpy(data, buff,  readSize);
        std::shared_ptr<Task> task(new Task());
        task->sampleRate = 16000;
        task->bitDepth = 8;
        task->channels = 1;
        task->data[0] = data;
        task->linesize[0] = readSize;
        renderBase->input(task);
        delete[] data;
    }
    renderBase->stop();
    delete renderBase;
    return 0;
}
