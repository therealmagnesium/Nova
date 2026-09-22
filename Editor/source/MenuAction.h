#pragma once
#include <Nova.h>

using ActionCallback = void (*)(Nova::Scene&);

struct MenuAction
{
    std::string_view label;
    std::string_view separator_text = "";
    ActionCallback execute;
};
