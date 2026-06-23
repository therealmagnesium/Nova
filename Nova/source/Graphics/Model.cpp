#include "Graphics/Model.h"
#include "Graphics/Mesh.h"
#include "Graphics/Texture.h"
#include "Core/Log.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <SDL3/SDL_filesystem.h>

namespace Nova::Models
{
    u32 CountNodeMeshReferences(aiNode* node);
    void ProcessNode(Model& model, aiNode* ai_node, const aiScene* ai_scene);
    Mesh ProcessMesh(Model& model, aiMesh* ai_mesh, const aiScene* ai_scene);
    void LoadAndStoreMapsAlbedo(Material& material, const aiMaterial* ai_material, const aiScene* ai_scene);
    void LoadAndStoreMapsNormal(Material& material, const aiMaterial* ai_material, const aiScene* ai_scene);
    void LoadAndStoreMapsMetallic(Material& material, const aiMaterial* ai_material, const aiScene* ai_scene);
    void LoadAndStoreMapsRoughness(Material& material, const aiMaterial* ai_material, const aiScene* ai_scene);

    Model Load(const std::filesystem::path& path)
    {
        Model model;

        const std::filesystem::path path_base = SDL_GetBasePath();
        const std::filesystem::path path_full = path_base / path;

        Assimp::Importer importer;
        const u32 flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace;
        const aiScene* scene = importer.ReadFile(path_full, flags);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            ERROR("Models::Load - Failed to load model \"%s\" and gave an Assimp error: \"%s\"!", path_full.c_str(), importer.GetErrorString());
            return Stub_Model;
        }

        const u32 mesh_count = CountNodeMeshReferences(scene->mRootNode);
        model.meshes.reserve(mesh_count);
        model.materials.resize(scene->mNumMaterials);
        ProcessNode(model, scene->mRootNode, scene);

        INFO("Model \"%s\" loaded successfully with %ld meshes and %ld materials", path_full.c_str(), model.meshes.size(), model.materials.size());
        return model;
    }

    void Unload(Model& model)
    {
        for (Material& material : model.materials)
        {
            Textures::Unload(material.texture_albedo);
            Textures::Unload(material.texture_normal);
        }

        for (Mesh& mesh : model.meshes)
            Meshes::Destroy(mesh);

        model.meshes.clear();
        model.materials.clear();
    }

    u32 CountNodeMeshReferences(aiNode* node)
    {
        u32 count = node->mNumMeshes;
        for (u32 i = 0; i < node->mNumChildren; i++)
            count += CountNodeMeshReferences(node->mChildren[i]);
        return count;
    }

    void ProcessNode(Model& model, aiNode* ai_node, const aiScene* ai_scene)
    {
        for (u32 i = 0; i < ai_node->mNumMeshes; i++)
        {
            aiMesh* ai_mesh = ai_scene->mMeshes[ai_node->mMeshes[i]];
            model.meshes.emplace_back(std::move(ProcessMesh(model, ai_mesh, ai_scene)));
        }

        for (u32 i = 0; i < ai_node->mNumChildren; i++)
            ProcessNode(model, ai_node->mChildren[i], ai_scene);
    }

    Mesh ProcessMesh(Model& model, aiMesh* ai_mesh, const aiScene* ai_scene)
    {
        // TODO: Switch to using stack allocated arrays if std::vector heap allocations are too expensive
        std::vector<Vertex> vertices;
        std::vector<u32> indices;

        vertices.reserve(ai_mesh->mNumVertices);

        u32 index_count = 0;
        for (u32 i = 0; i < ai_mesh->mNumFaces; i++)
        {
            aiFace& face = ai_mesh->mFaces[i];
            index_count += face.mNumIndices;
        }
        indices.reserve(index_count);

        for (u32 i = 0; i < ai_mesh->mNumVertices; i++)
        {
            Vertex vertex;
            vertex.position.x = ai_mesh->mVertices[i].x;
            vertex.position.y = ai_mesh->mVertices[i].y;
            vertex.position.z = ai_mesh->mVertices[i].z;

            if (ai_mesh->HasNormals())
            {
                vertex.normal.x = ai_mesh->mNormals[i].x;
                vertex.normal.y = ai_mesh->mNormals[i].y;
                vertex.normal.z = ai_mesh->mNormals[i].z;
            }

            if (ai_mesh->HasTextureCoords(0))
            {
                vertex.uv.x = ai_mesh->mTextureCoords[0][i].x;
                vertex.uv.y = ai_mesh->mTextureCoords[0][i].y;
            }

            if (ai_mesh->HasTangentsAndBitangents())
            {
                vertex.tangent.x = ai_mesh->mTangents[i].x;
                vertex.tangent.y = ai_mesh->mTangents[i].y;
                vertex.tangent.z = ai_mesh->mTangents[i].z;
            }

            vertices.emplace_back(vertex);
        }

        for (u32 i = 0; i < ai_mesh->mNumFaces; i++)
        {
            aiFace& face = ai_mesh->mFaces[i];
            for (u32 j = 0; j < face.mNumIndices; j++)
                indices.emplace_back(face.mIndices[j]);
        }

        Mesh mesh = Meshes::Create(vertices.data(), vertices.size(), indices.data(), indices.size());
        mesh.material_index = ai_mesh->mMaterialIndex;

        const aiMaterial* ai_material = ai_scene->mMaterials[mesh.material_index];
        Material& material = model.materials[mesh.material_index];

        aiColor4D albedo;
        if (ai_material->Get(AI_MATKEY_COLOR_DIFFUSE, albedo) == aiReturn_SUCCESS)
        {
            material.albedo.r = albedo.r;
            material.albedo.g = albedo.g;
            material.albedo.b = albedo.b;
            material.albedo.a = albedo.a;
        }

        float metallic = 0.f;
        if (ai_material->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == aiReturn_SUCCESS)
            material.metallic = metallic;

        float roughness = 0.f;
        if (ai_material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == aiReturn_SUCCESS)
            material.roughness = roughness;

        const char* mesh_name = ai_mesh->mName.C_Str();
        if (strstr(mesh_name, "(indoor)") != NULL)
            mesh.pipeline = GPUPipeline::IndoorMeshes;

        LoadAndStoreMapsAlbedo(material, ai_material, ai_scene);
        LoadAndStoreMapsNormal(material, ai_material, ai_scene);
        LoadAndStoreMapsMetallic(material, ai_material, ai_scene);
        LoadAndStoreMapsRoughness(material, ai_material, ai_scene);

        return mesh;
    }

    void LoadAndStoreMapsAlbedo(Material& material, const aiMaterial* ai_material, const aiScene* ai_scene)
    {
        for (u32 i = 0; i < ai_material->GetTextureCount(aiTextureType_DIFFUSE); i++)
        {
            aiString ai_path;
            ai_material->GetTexture(aiTextureType_DIFFUSE, 0, &ai_path);

            const aiTexture* embedded_texture = ai_scene->GetEmbeddedTexture(ai_path.C_Str());
            if (embedded_texture != NULL && embedded_texture->mHeight == 0)
            {
                const u8* data = reinterpret_cast<const u8*>(embedded_texture->pcData);
                Texture texture = Textures::LoadFromMemory(data, embedded_texture->mWidth, TextureFormat::RGBA8_SRGB);
                if (texture.IsValid())
                    material.texture_albedo = texture;
            }
            else
            {
                Texture texture = Textures::Load(ai_path.C_Str(), TextureFormat::RGBA8_SRGB, TextureSampler::LinearClamp);
                if (texture.IsValid())
                    material.texture_albedo = texture;
            }
        }
    }

    void LoadAndStoreMapsNormal(Material& material, const aiMaterial* ai_material, const aiScene* ai_scene)
    {
        for (u32 i = 0; i < ai_material->GetTextureCount(aiTextureType_NORMALS); i++)
        {
            aiString ai_path;
            ai_material->GetTexture(aiTextureType_NORMALS, 0, &ai_path);

            const aiTexture* embedded_texture = ai_scene->GetEmbeddedTexture(ai_path.C_Str());
            if (embedded_texture != NULL && embedded_texture->mHeight == 0)
            {
                const u8* data = reinterpret_cast<const u8*>(embedded_texture->pcData);
                Texture texture = Textures::LoadFromMemory(data, embedded_texture->mWidth, TextureFormat::RGBA8, TextureSampler::LinearClamp);
                if (texture.IsValid())
                    material.texture_normal = texture;
            }
            else
            {
                Texture texture = Textures::Load(ai_path.C_Str(), TextureFormat::RGBA8, TextureSampler::LinearClamp);
                if (texture.IsValid())
                    material.texture_normal = texture;
            }
        }
    }

    void LoadAndStoreMapsMetallic(Material& material, const aiMaterial* ai_material, const aiScene* ai_scene)
    {
        for (u32 i = 0; i < ai_material->GetTextureCount(aiTextureType_METALNESS); i++)
        {
            aiString ai_path;
            ai_material->GetTexture(aiTextureType_METALNESS, 0, &ai_path);

            const aiTexture* embedded_texture = ai_scene->GetEmbeddedTexture(ai_path.C_Str());
            if (embedded_texture != NULL && embedded_texture->mHeight == 0)
            {
                const u8* data = reinterpret_cast<const u8*>(embedded_texture->pcData);
                Texture texture = Textures::LoadFromMemory(data, embedded_texture->mWidth, TextureFormat::RGBA8, TextureSampler::LinearClamp);
                if (texture.IsValid())
                    material.texture_metallic = texture;
            }
            else
            {
                Texture texture = Textures::Load(ai_path.C_Str(), TextureFormat::RGBA8, TextureSampler::LinearClamp);
                if (texture.IsValid())
                    material.texture_metallic = texture;
            }
        }
    }

    void LoadAndStoreMapsRoughness(Material& material, const aiMaterial* ai_material, const aiScene* ai_scene)
    {
        for (u32 i = 0; i < ai_material->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS); i++)
        {
            aiString ai_path;
            ai_material->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &ai_path);

            const aiTexture* embedded_texture = ai_scene->GetEmbeddedTexture(ai_path.C_Str());
            if (embedded_texture != NULL && embedded_texture->mHeight == 0)
            {
                const u8* data = reinterpret_cast<const u8*>(embedded_texture->pcData);
                Texture texture = Textures::LoadFromMemory(data, embedded_texture->mWidth, TextureFormat::RGBA8, TextureSampler::LinearClamp);
                if (texture.IsValid())
                    material.texture_roughness = texture;
            }
            else
            {
                Texture texture = Textures::Load(ai_path.C_Str(), TextureFormat::RGBA8, TextureSampler::LinearClamp);
                if (texture.IsValid())
                    material.texture_roughness = texture;
            }
        }
    }
}
