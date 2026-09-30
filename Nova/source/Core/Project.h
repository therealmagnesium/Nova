#pragma once
#include <filesystem>

namespace Nova
{
    struct Project
    {
        std::string name = "Untitled";
        std::filesystem::path path_config;
        std::filesystem::path path_directory;
        std::filesystem::path path_assets = "Assets";
        std::filesystem::path path_scene_start;

        inline bool IsValid() const { return !name.empty() &&
                                             !path_config.empty() &&
                                             !path_directory.empty() &&
                                             !path_assets.empty(); }
    };

    inline const Project Stub_Project;

    namespace Projects
    {
        Project& Import(const std::filesystem::path& path);
        void Export(const std::filesystem::path& path, const Project& project = Stub_Project);

        // std::filesystem::path GetScriptModulePath(const Project& project);
        std::filesystem::path GetStartScenePath(const Project& project);
        std::filesystem::path GetAssetPath(const Project& project);
        std::filesystem::path GetAssetPathAbsolute(const std::filesystem::path& path, const Project& project);
        std::filesystem::path GetAssetPathRelative(const std::filesystem::path& path, const Project& project);

        Project& GetContext();
        bool ClearPendingLayoutLoad();
    }
}
