#pragma once
#include "global.h"

class Basic_Render_Class
{
public:
    virtual ~Basic_Render_Class() = default;
    virtual void draw () {}
    virtual void update () {}
};