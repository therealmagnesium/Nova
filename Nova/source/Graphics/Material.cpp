#include "Graphics/Material.h"
#include "Core/AssetManager.h"
#include "Core/Serialization.h"
#include "Core/Log.h"

#include <yaml-cpp/yaml.h>
#include <fstream>

namespace Nova::Materials
{
    Material Import(const std::filesystem::path& path)
    {
        Material material;

        try
        {
            const YAML::Node root = YAML::LoadFile(path);
            material.handle = root["Material"].as<AssetHandle>();
            material.albedo = root["Albedo"].as<glm::vec4>();
            material.metallic = root["Metallic"].as<float>();
            material.roughness = root["Roughness"].as<float>();

            const AssetHandle handle_albedo = root["Texture Albedo"].as<AssetHandle>();
            const AssetHandle handle_normal = root["Texture Normal"].as<AssetHandle>();
            const AssetHandle handle_metallic = root["Texture Metallic"].as<AssetHandle>();
            const AssetHandle handle_roughness = root["Texture Roughness"].as<AssetHandle>();

            if (AssetManager::IsHandleValid(handle_albedo))
                material.texture_albedo = *AssetManager::GetAsset<Texture>(handle_albedo);

            if (AssetManager::IsHandleValid(handle_normal))
                material.texture_normal = *AssetManager::GetAsset<Texture>(handle_normal);

            if (AssetManager::IsHandleValid(handle_metallic))
                material.texture_metallic = *AssetManager::GetAsset<Texture>(handle_metallic);

            if (AssetManager::IsHandleValid(handle_roughness))
                material.texture_roughness = *AssetManager::GetAsset<Texture>(handle_roughness);
        }
        catch (const YAML::Exception& e)
        {
            ERROR("Materials::Import - Failed to import \"%s\"! (yaml-cpp): %s", path.string().c_str(), e.what());
            return Stub_Material;
        }

        INFO("Material \"%s\" imported successfully", path.string().c_str());
        return material;
    }

    void Export(const std::filesystem::path& path, const Material& material)
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Material" << YAML::Value << material.handle;
        out << YAML::Key << "Albedo" << YAML::Value << material.albedo;
        out << YAML::Key << "Metallic" << YAML::Value << material.metallic;
        out << YAML::Key << "Roughness" << YAML::Value << material.roughness;
        out << YAML::Key << "Texture Albedo" << YAML::Value << material.texture_albedo.handle;
        out << YAML::Key << "Texture Normal" << YAML::Value << material.texture_normal.handle;
        out << YAML::Key << "Texture Metallic" << YAML::Value << material.texture_metallic.handle;
        out << YAML::Key << "Texture Roughness" << YAML::Value << material.texture_roughness.handle;
        out << YAML::EndMap;

        try
        {
            std::ofstream fout(path);
            fout << out.c_str();
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            ERROR("Materials::Export - Failed to export material \"%s\"! (std::filesystem): %s", path.string().c_str(), e.what());
        }
    }
}
