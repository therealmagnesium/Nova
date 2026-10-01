#pragma once
#include <Nova.h>

namespace ContentBrowserPanel
{
    void Init();
    void Shutdown();
    void Display();

    std::filesystem::path GetSelectionContext();
    void SetSelectionContext(const std::filesystem::path& path);
}
