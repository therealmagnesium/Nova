#include "Core/Project.h"
#include "Core/AssetManager.h"
#include "Core/Log.h"
#include "Core/Serialization.h"
#include "UI/UI.h"

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
            const YAML::Node node_root = YAML::LoadFile(path);
            active_project.name = node_root["Project"].as<std::string>();
            active_project.path_assets = node_root["Assets Directory"].as<std::filesystem::path>();
            active_project.path_scene_start = node_root["Startup Scene"].as<std::filesystem::path>();

            const YAML::Node node_registry = node_root["Asset Registry"];
            for (const YAML::Node& node : node_registry)
            {
                AssetHandle current_handle = AssetHandle_Invalid;
                std::filesystem::path current_path = "";
                string current_name = "";
                AssetType current_type = AssetType::Invalid;

                for (auto it = node.begin(); it != node.end(); it++)
                {
                    const std::string_view key = it->first.as<std::string_view>();

                    if (key == "Handle")
                        current_handle = it->second.as<AssetHandle>();
                    else if (key == "Path")
                        current_path = it->second.as<std::filesystem::path>();
                    else if (key == "Name")
                        current_name = it->second.as<string>();
                    else if (key == "Type")
                        current_type = StringViewToAssetType(it->second.as<std::string_view>());
                }

                if (current_handle != AssetHandle_Invalid && current_type != AssetType::Invalid)
                {
                    if (!current_path.empty())
                        AssetManager::ImportByPath(current_path, current_type, current_handle);
                    else
                        AssetManager::ImportByName(current_name, current_type, current_handle);
                }
            }
        }
        catch (const YAML::Exception& e)
        {
            ERROR("Projects::Import - Failed to import project \"%s\"! (yaml-cpp): %s", path.string().c_str(), e.what());
            active_project = Stub_Project;
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

        out << YAML::Key << "Asset Registry" << YAML::BeginSeq;
        for (const auto& [handle, metadata] : AssetManager::GetRegistry())
        {
            out << YAML::BeginMap;
            out << YAML::Key << "Handle" << YAML::Value << handle;
            out << YAML::Key << "Path" << YAML::Value << metadata.path;
            out << YAML::Key << "Name" << YAML::Value << metadata.name;
            out << YAML::Key << "Type" << YAML::Value << AssetTypeToStringView(metadata.type);
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;

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

        UI::ExportLayout(project);
        INFO("Project was successfully exported to \"%s\"", path.string().c_str());
    }

    std::filesystem::path GetStartScenePath(const Project& project) { return project.path_directory / project.path_assets / project.path_scene_start; }
    std::filesystem::path GetAssetPath(const Project& project) { return project.path_directory / project.path_assets; }
    std::filesystem::path GetAssetPathAbsolute(const std::filesystem::path& path, const Project& project) { return project.path_directory / path; }
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
