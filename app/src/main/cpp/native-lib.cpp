#include <jni.h>
#include <string>
#include "RecorderBase.h"
#include "OpenSLESRecorder.h"
#include <memory>
#include "log.h"
#include <thread>

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
    conf.sampleRate = 44100;
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