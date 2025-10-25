#pragma once
#include <string>

// 所有接口类的共同基类
struct IObject {
    virtual ~IObject() = default;
};

// 为接口定义唯一ID字符串
#define DEFINE_IID(name) \
    static const char* IID() { return #name; }
