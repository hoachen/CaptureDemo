//
// Created by jade on 2021/8/26.
//

#ifndef CAPTUREDEMO_RECORDERBASE_H
#define CAPTUREDEMO_RECORDERBASE_H

#include <iostream>
#include <functional>
#include <memory>
#include "common.h"


class RecorderBase {
public:
    RecorderBase();
    ~RecorderBase();
    virtual int init(SourceConfig &config) = 0;
    virtual int start() = 0;
    virtual int stop() = 0;
    std::function<void(std::shared_ptr<Task> &task)> output = nullptr;
};


#endif //CAPTUREDEMO_RECORDERBASE_H
