#pragma once
#include "render/render.h"
#include <functional>
#include <string>
#include <map>
class AnimateState : public BasicRenderClass
{
protected:
    std::string state_name;
public:
    std::string next_state;
    virtual void update() = 0;
    virtual void draw() = 0;
    virtual void on_exit() = 0;
    virtual void on_enter() = 0;
    AnimateState(const std::string& state_name):state_name (state_name){}
    AnimateState(){}
    virtual ~AnimateState() = default;
    AnimateState(AnimateState&&) = default;
    AnimateState& operator=(AnimateState&&) = default;
};
class AnimateManager : public BasicRenderClass
{
private:
    std::map<std::string, AnimateState*> mp;
public:
    std::string current_state = "";
    AnimateManager(){}
    ~AnimateManager(){mp.clear();}
    void update()
    {
        if(current_state == "")return;
        mp[current_state]->update();
        if(mp[current_state]->next_state != "")
        {
            switch_state(mp[current_state]->next_state);
        }
    }
    void draw()
    {
        if(current_state == "")return;
        mp[current_state]->draw();
    }
    void add_state(const std::string&  state_name, AnimateState& state)
    {
        mp[state_name] = &state;
    }
    void switch_state(std::string state_name)
    {
        if(current_state == state_name || mp.find(state_name) == mp.end())return;
        if(current_state != "")
        {
            mp[current_state]->on_exit();
        }
        mp[current_state = state_name]->on_enter();
    }
    const std::string& get_current_state() const { return current_state; }
};