#pragma once
#include "src/event/event.h"

class Basic_Render_Class
{
public:
    virtual void draw () = 0;
    virtual void update () = 0;
};