#include "Core/Project.h"
#include "Core/Log.h"
#include "Core/Serialization.h"

#include <yaml-cpp/yaml.h>
#include <fstream>
#include <imgui.h>

namespace Nova::Projects
{
    local Project active_project = Stub_Project;
    local bool pending_layout_load = false;

    Project& Import(const std::filesystem::path& path)
    {
        active_project.path_config = path;
        active_project.path_directory = std::filesystem::absolute(path.parent_path());

        try
        {
            const YAML::Node root = YAML::LoadFile(path);
            active_project.name = root["Project"].as<std::string>();
            active_project.path_assets = root["Assets Directory"].as<std::filesystem::path>();
            active_project.path_scene_start = root["Startup Scene"].as<std::filesystem::path>();
        }
        catch (const YAML::Exception& e)
        {
            ERROR("Projects::Import - Failed to import project \"%s\"! (yaml-cpp): %s", path.string().c_str(), e.what());
            return active_project;
        }

        pending_layout_load = true;
        INFO("Project \"%s\" imported successfully", path.string().c_str());
        return active_project;
    }

    void Export(const std::filesystem::path& path, const Project& project)
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Project" << YAML::Value << project.name;
        out << YAML::Key << "Assets Directory" << YAML::Value << project.path_assets;
        out << YAML::Key << "Startup Scene" << YAML::Value << project.path_scene_start;
        out << YAML::EndMap;

        try
        {
            std::ofstream fout(path);
            fout << out.c_str();
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            ERROR("Materials::Export - Failed to export material \"%s\"! (std::filesystem): %s", path.string().c_str(), e.what());
            return;
        }

        const std::filesystem::path path_layout = project.path_directory / "UI-Layout.ini";
        ImGui::SaveIniSettingsToDisk(path_layout.string().c_str());

        INFO("Project was successfully exported to \"%s\"", path.string().c_str());
    }

    std::filesystem::path GetStartScenePath(const Project& project) { return project.path_directory / project.path_assets / project.path_scene_start; }
    std::filesystem::path GetAssetPath(const Project& project) { return project.path_directory / project.path_assets; }
    std::filesystem::path GetAssetPathAbsolute(const std::filesystem::path& path, const Project& project) { return std::filesystem::absolute(project.path_directory / project.path_assets / path); }
    std::filesystem::path GetAssetPathRelative(const std::filesystem::path& path, const Project& project) { return std::filesystem::proximate(path, project.path_directory); }

    Project& GetContext() { return active_project; }
    bool ClearPendingLayoutLoad()
    {
        if (pending_layout_load)
        {
            pending_layout_load = false;
            return true;
        }

        return false;
    }
}
