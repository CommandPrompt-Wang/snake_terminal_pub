#pragma once
#include "global.h"

// 可加入 DrawList 的渲染对象基类；子类实现 update / draw 参与每帧循环
class BasicRenderClass
{
public:
    virtual ~BasicRenderClass() = default;
    virtual void draw () {}
    virtual void update () {}
};