#pragma once
#include "src/event/event.h"

class BasicRenderClass
{
public:
    virtual void draw () = 0;
    virtual void update () = 0;
};