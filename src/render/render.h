#pragma once
#include "global.h"

class BasicRenderClass
{
public:
    virtual ~BasicRenderClass() = default;
    virtual void draw () {}
    virtual void update () {}
};