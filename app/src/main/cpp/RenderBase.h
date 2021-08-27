//
// Created by SHAREit on 2021/8/27.
//

#pragma once
#include <memory>
#include "common.h"

/**
 * RenderBase is a class represents
 */
class RenderBase {
public:
    RenderBase();

    ~RenderBase();

    virtual int init(SinkConfig &cfg) = 0; //初始化，支持任意时间点的初始化

    virtual int start() = 0; //启动，支持重入

    virtual int stop() = 0;  //关闭

    virtual int input(const std::shared_ptr<Task> &task) = 0; //数据源输入，不可阻塞

public:
    SinkConfig cfg;
};

