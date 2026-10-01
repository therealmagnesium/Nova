#pragma once
#include "Core/Base.h"
#include <filesystem>

namespace Nova
{
    struct FileDialogFilter
    {
        const char* name = "";
        const char* specification = "";
    };

    namespace FileDialogs
    {
        void Init();
        void Shutdown();

        bool AskOpen(const FileDialogFilter* filters = NULL, u8 filter_count = 0);
        bool AskSave(const FileDialogFilter* filters, u8 filter_count);
        void OpenExplorer(const std::filesystem::path& path);

        const std::filesystem::path& GetPathSelected();
        const std::filesystem::path& GetPathDefault();
        void SetDefaultPath(const std::filesystem::path& path);
    }
}
