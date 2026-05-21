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
    void ProcessNode(Model& model, aiNode* ai_node, const aiScene* ai_scene);
    Mesh ProcessMesh(Model& model, aiMesh* ai_mesh, const aiScene* ai_scene);

    Model Load(const std::filesystem::path& path)
    {
        Model model;

        const std::filesystem::path path_base = SDL_GetBasePath();
        const std::filesystem::path path_full = path_base / path;

        Assimp::Importer importer;
        const u32 flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices;
        const aiScene* scene = importer.ReadFile(path_full, flags);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            ERROR("Models::Load - Failed to load model \"%s\" and gave an Assimp error: \"%s\"!", path_full.c_str(), importer.GetErrorString());
            return Stub_Model;
        }

        model.materials.reserve(scene->mNumMaterials);
        model.meshes.reserve(scene->mNumMeshes);
        ProcessNode(model, scene->mRootNode, scene);

        INFO("Models::Load - Model \"%s\" loaded successfully with %ld meshes and %ld materials", path_full.c_str(), model.meshes.size(), model.materials.size());
        return model;
    }

    void Unload(Model& model)
    {
        for (Material& material : model.materials)
            if (material.albedo_texture != NULL)
                Textures::Unload(*material.albedo_texture);

        for (Mesh& mesh : model.meshes)
            Meshes::Destroy(mesh);

        model.meshes.clear();
        model.materials.clear();
    }

    void ProcessNode(Model& model, aiNode* ai_node, const aiScene* ai_scene)
    {
        for (u32 i = 0; i < ai_node->mNumMeshes; i++)
        {
            aiMesh* ai_mesh = ai_scene->mMeshes[ai_node->mMeshes[i]];
            model.meshes.emplace_back(ProcessMesh(model, ai_mesh, ai_scene));
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

            if (ai_mesh->HasVertexColors(0))
            {
                vertex.color.r = ai_mesh->mColors[0][i].r;
                vertex.color.g = ai_mesh->mColors[0][i].g;
                vertex.color.b = ai_mesh->mColors[0][i].b;
                vertex.color.a = ai_mesh->mColors[0][i].a;
            }

            if (ai_mesh->HasNormals())
            {
                // TODO: Once normal vectors are supported, copy the data from ai_mesh here
            }

            if (ai_mesh->HasTextureCoords(0))
            {
                vertex.uv.x = ai_mesh->mTextureCoords[0][i].x;
                vertex.uv.y = ai_mesh->mTextureCoords[0][i].y;
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

        if (ai_mesh->mMaterialIndex >= 0)
        {
            mesh.material_index = ai_mesh->mMaterialIndex;
            aiMaterial* assimpMaterial = ai_scene->mMaterials[mesh.material_index];
            Material material;

            aiColor4D albedo;
            if (assimpMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, albedo) == aiReturn_SUCCESS)
            {
                material.albedo.r = albedo.r;
                material.albedo.g = albedo.g;
                material.albedo.b = albedo.b;
                material.albedo.a = albedo.a;
            }

            model.materials.emplace_back(material);
        }

        return mesh;
    }
}
