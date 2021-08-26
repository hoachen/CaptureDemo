//
// Created by wlanjie on 2019-07-23.
//

#ifndef ALITAFORANDROID_LOG_H
#define ALITAFORANDROID_LOG_H

#include <android/log.h>

#define TAG "CHH"

#define LOGV(...)  __android_log_print(ANDROID_LOG_VERBOSE, TAG ,__VA_ARGS__)
#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, TAG ,__VA_ARGS__)
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO, TAG ,__VA_ARGS__)
#define LOGW(...)  __android_log_print(ANDROID_LOG_WARN, TAG ,__VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, TAG ,__VA_ARGS__)

#define ALOGV(USER_TAG, ...)  __android_log_print(ANDROID_LOG_VERBOSE, USER_TAG, __VA_ARGS__)
#define ALOGD(USER_TAG, ...)  __android_log_print(ANDROID_LOG_DEBUG, USER_TAG, __VA_ARGS__)
#define ALOGI(USER_TAG, ...)  __android_log_print(ANDROID_LOG_INFO, USER_TAG, __VA_ARGS__)
#define ALOGW(USER_TAG, ...)  __android_log_print(ANDROID_LOG_WARN, USER_TAG, __VA_ARGS__)
#define ALOGE(USER_TAG, ...)  __android_log_print(ANDROID_LOG_ERROR, USER_TAG, __VA_ARGS__)
#endif //ALITAFORANDROID_LOG_H
