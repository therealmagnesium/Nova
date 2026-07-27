#include "Graphics/Model.h"
#include "Graphics/Texture.h"
#include "Core/Log.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <SDL3/SDL_filesystem.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

namespace Nova::Models
{
    u32 CountNodeMeshReferences(aiNode* node);
    void ProcessNode(Model& model, aiNode* ai_node, const aiScene* ai_scene);
    Mesh ProcessMesh(Model& model, aiMesh* ai_mesh, const aiScene* ai_scene);
    void LoadAndStoreMapsAlbedo(Material& material, const aiMaterial* ai_material, const aiScene* ai_scene);
    void LoadAndStoreMapsNormal(Material& material, const aiMaterial* ai_material, const aiScene* ai_scene);
    void LoadAndStoreMapsMetallic(Material& material, const aiMaterial* ai_material, const aiScene* ai_scene);
    void LoadAndStoreMapsRoughness(Material& material, const aiMaterial* ai_material, const aiScene* ai_scene);

    glm::mat4 AssimpToGLMMat4(const aiMatrix4x4& mat);
    void ProcessNodeSkinned(AnimatedModel& model, aiNode* ai_node, const aiScene* ai_scene);
    Mesh ProcessMeshSkinned(AnimatedModel& model, aiMesh* ai_mesh, const aiScene* ai_scene);
    void ProcessBones(Skeleton& skeleton, std::vector<VertexSkinned>& vertices, const aiMesh* ai_mesh);
    void BuildSkeletonHierarchy(Skeleton& skeleton, const aiNode* node, s32 parent_bone_index, const aiMatrix4x4& accumulated_parent_offset = aiMatrix4x4());

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
            Textures::Unload(material.texture_metallic);
            Textures::Unload(material.texture_roughness);
        }

        for (Mesh& mesh : model.meshes)
            Meshes::Destroy(mesh);

        model.meshes.clear();
        model.materials.clear();
    }

    AnimatedModel LoadAnimated(const std::filesystem::path& path)
    {
        AnimatedModel model;

        const std::filesystem::path path_base = SDL_GetBasePath();
        const std::filesystem::path path_full = path_base / path;

        Assimp::Importer importer;
        // aiProcess_LimitBoneWeights caps + renormalizes each vertex to Nova's MAX_BONE_INFLUENCE (4) for us.
        const u32 flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace | aiProcess_LimitBoneWeights;
        const aiScene* scene = importer.ReadFile(path_full, flags);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            ERROR("Models::LoadAnimated - Failed to load animated model \"%s\" and gave an Assimp error: \"%s\"!", path_full.c_str(), importer.GetErrorString());
            return Stub_AnimatedModel;
        }

        model.skeleton.global_inverse_transform = glm::inverse(AssimpToGLMMat4(scene->mRootNode->mTransformation));

        const u32 mesh_count = CountNodeMeshReferences(scene->mRootNode);
        model.meshes.reserve(mesh_count);
        model.materials.resize(scene->mNumMaterials);
        ProcessNodeSkinned(model, scene->mRootNode, scene);

        // Bone offset matrices are collected per-mesh above; the hierarchy (parent/child +
        // rest-pose local transforms) is a property of the node tree as a whole, so it's
        // resolved in a single pass over the full scene afterward.
        BuildSkeletonHierarchy(model.skeleton, scene->mRootNode, -1);

        if (!model.skeleton.IsValid())
            WARN("Models::LoadAnimated - \"%s\" loaded with no bones, did you mean to use Models::Load instead?", path_full.c_str());

        INFO("Animated model \"%s\" loaded successfully with %ld meshes, %ld materials and %ld bones", path_full.c_str(), model.meshes.size(), model.materials.size(), model.skeleton.bones.size());
        return model;
    }

    void UnloadAnimated(AnimatedModel& model)
    {
        for (Material& material : model.materials)
        {
            Textures::Unload(material.texture_albedo);
            Textures::Unload(material.texture_normal);
            Textures::Unload(material.texture_metallic);
            Textures::Unload(material.texture_roughness);
        }

        for (Mesh& mesh : model.meshes)
            Meshes::Destroy(mesh);

        model.meshes.clear();
        model.materials.clear();
        model.skeleton.bones.clear();
        model.skeleton.bone_indices_by_name.clear();
    }

    glm::mat4 AssimpToGLMMat4(const aiMatrix4x4& mat)
    {
        return glm::mat4(
            mat.a1, mat.b1, mat.c1, mat.d1,
            mat.a2, mat.b2, mat.c2, mat.d2,
            mat.a3, mat.b3, mat.c3, mat.d3,
            mat.a4, mat.b4, mat.c4, mat.d4
        );
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

    void ProcessNodeSkinned(AnimatedModel& model, aiNode* ai_node, const aiScene* ai_scene)
    {
        for (u32 i = 0; i < ai_node->mNumMeshes; i++)
        {
            aiMesh* ai_mesh = ai_scene->mMeshes[ai_node->mMeshes[i]];
            model.meshes.emplace_back(std::move(ProcessMeshSkinned(model, ai_mesh, ai_scene)));
        }

        for (u32 i = 0; i < ai_node->mNumChildren; i++)
            ProcessNodeSkinned(model, ai_node->mChildren[i], ai_scene);
    }

    Mesh ProcessMeshSkinned(AnimatedModel& model, aiMesh* ai_mesh, const aiScene* ai_scene)
    {
        std::vector<VertexSkinned> vertices;
        std::vector<u32> indices;

        vertices.reserve(ai_mesh->mNumVertices);

        u32 index_count = 0;
        for (u32 i = 0; i < ai_mesh->mNumFaces; i++)
            index_count += ai_mesh->mFaces[i].mNumIndices;
        indices.reserve(index_count);

        for (u32 i = 0; i < ai_mesh->mNumVertices; i++)
        {
            VertexSkinned vertex;
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
            const aiFace& face = ai_mesh->mFaces[i];
            for (u32 j = 0; j < face.mNumIndices; j++)
                indices.emplace_back(face.mIndices[j]);
        }

        ProcessBones(model.skeleton, vertices, ai_mesh);

        // FIX: Normalize weights and safeguard unweighted vertices from collapsing
        for (VertexSkinned& vertex : vertices)
        {
            const float total_weight = vertex.bone_weights.x + vertex.bone_weights.y +
                                       vertex.bone_weights.z + vertex.bone_weights.w;
            if (total_weight > 0.0f)
            {
                vertex.bone_weights /= total_weight; // Normalize to exactly 1.0f
            }
            else
            {
                // Unweighted vertex: bind it to the root bone to prevent origin collapse
                vertex.bone_ids[0] = 0;
                vertex.bone_weights = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
            }
        }

        Mesh mesh = Meshes::CreateSkinned(vertices.data(), static_cast<u32>(vertices.size()), indices.data(), static_cast<u32>(indices.size()));
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

        LoadAndStoreMapsAlbedo(material, ai_material, ai_scene);
        LoadAndStoreMapsNormal(material, ai_material, ai_scene);
        LoadAndStoreMapsMetallic(material, ai_material, ai_scene);
        LoadAndStoreMapsRoughness(material, ai_material, ai_scene);

        return mesh;
    }

    void ProcessBones(Skeleton& skeleton, std::vector<VertexSkinned>& vertices, const aiMesh* ai_mesh)
    {
        for (u32 bone_i = 0; bone_i < ai_mesh->mNumBones; bone_i++)
        {
            const aiBone* ai_bone = ai_mesh->mBones[bone_i];
            const std::string bone_name = ai_bone->mName.C_Str();

            // The same bone is often shared across multiple meshes in a model (e.g. a body and
            // a separate cloth mesh both skinned to "spine_02") - only register it once.
            u32 bone_index;
            const auto it = skeleton.bone_indices_by_name.find(bone_name);
            if (it == skeleton.bone_indices_by_name.end())
            {
                if (skeleton.bones.size() >= MAX_BONES)
                {
                    WARN("Models::ProcessBones - Skeleton exceeds MAX_BONES (%d), bone \"%s\" will be dropped!", MAX_BONES, bone_name.c_str());
                    continue;
                }

                BoneInfo info;
                info.name = bone_name;
                info.offset = AssimpToGLMMat4(ai_bone->mOffsetMatrix);

                bone_index = static_cast<u32>(skeleton.bones.size());
                skeleton.bones.emplace_back(std::move(info));
                skeleton.bone_indices_by_name[bone_name] = bone_index;
            }
            else
                bone_index = it->second;

            for (u32 w = 0; w < ai_bone->mNumWeights; w++)
            {
                const aiVertexWeight& weight = ai_bone->mWeights[w];
                VertexSkinned& vertex = vertices[weight.mVertexId];

                // aiProcess_LimitBoneWeights already caps + sorts each vertex to MAX_BONE_INFLUENCE
                // entries, so the first zero-weight slot found here is always a genuinely free one.
                for (u32 slot = 0; slot < MAX_BONE_INFLUENCE; slot++)
                {
                    if (vertex.bone_weights[slot] == 0.f)
                    {
                        vertex.bone_ids[slot] = bone_index;
                        vertex.bone_weights[slot] = weight.mWeight;
                        break;
                    }
                }
            }
        }
    }

    /*
    void BuildSkeletonHierarchy(Skeleton& skeleton, const aiNode* node, s32 parent_bone_index)
    {
        s32 current_bone_index = parent_bone_index;

        const auto it = skeleton.bone_indices_by_name.find(node->mName.C_Str());
        if (it != skeleton.bone_indices_by_name.end())
        {
            current_bone_index = static_cast<s32>(it->second);

            BoneInfo& bone = skeleton.bones[current_bone_index];
            bone.parent_index = parent_bone_index;

            // FIX: Use Assimp's native decomposition for perfect quaternion extraction
            aiVector3D scaling;
            aiQuaternion rotation;
            aiVector3D position;
            node->mTransformation.Decompose(scaling, rotation, position);
            bone.local_bind_scale = glm::vec3(scaling.x, scaling.y, scaling.z);
            bone.local_bind_rotation = glm::quat(rotation.w, rotation.x, rotation.y, rotation.z);
            bone.local_bind_translation = glm::vec3(position.x, position.y, position.z);
        }

        for (u32 i = 0; i < node->mNumChildren; i++)
            BuildSkeletonHierarchy(skeleton, node->mChildren[i], current_bone_index);
    }*/

    void BuildSkeletonHierarchy(Skeleton& skeleton, const aiNode* node, s32 parent_bone_index, const aiMatrix4x4& accumulated_parent_offset)
    {
        // Everything accumulated from non-bone ancestor nodes since the last real bone - most
        // notably Blender's "Armature" object wrapper, which every Blender FBX export inserts
        // above its bones and which commonly carries the corrective Z-up -> Y-up rotation. This
        // is folded forward through non-bone nodes instead of being silently dropped, and reset
        // to identity once "cashed out" onto a real bone below.
        s32 current_bone_index = parent_bone_index;
        aiMatrix4x4 next_accumulated_parent_offset = accumulated_parent_offset * node->mTransformation;

        const auto it = skeleton.bone_indices_by_name.find(node->mName.C_Str());
        if (it != skeleton.bone_indices_by_name.end())
        {
            current_bone_index = static_cast<s32>(it->second);

            BoneInfo& bone = skeleton.bones[current_bone_index];
            bone.parent_index = parent_bone_index;
            bone.parent_offset = AssimpToGLMMat4(accumulated_parent_offset); // Excludes this bone's own transform below - see BoneInfo::parent_offset

            // First root bone found - the node directly above it (its Assimp parent, NOT the
            // registered-bone parent_index, which is -1 here) is almost always the Blender
            // Armature object itself. Capture its name once so Animations::Bind can recognize
            // and apply a clip that animates that object directly instead of an internal bone -
            // see Skeleton::root_motion_node_name.
            if (parent_bone_index == -1 && skeleton.root_motion_node_name.empty() && node->mParent != NULL)
                skeleton.root_motion_node_name = node->mParent->mName.C_Str();

            /*
            glm::vec3 skew;
            glm::vec4 perspective;
            glm::decompose(AssimpToGLMMat4(node->mTransformation), bone.local_bind_scale, bone.local_bind_rotation, bone.local_bind_translation, skew, perspective);*/

            // FIX: Use Assimp's native decomposition for perfect quaternion extraction
            aiVector3D scaling;
            aiQuaternion rotation;
            aiVector3D position;
            node->mTransformation.Decompose(scaling, rotation, position);
            bone.local_bind_scale = glm::vec3(scaling.x, scaling.y, scaling.z);
            bone.local_bind_rotation = glm::quat(rotation.w, rotation.x, rotation.y, rotation.z);
            bone.local_bind_translation = glm::vec3(position.x, position.y, position.z);

            // This bone's own node transform is now accounted for via local_bind_* (or an active
            // clip's channel) - descendants fold forward from identity again, not from this chain a second time.
            next_accumulated_parent_offset = aiMatrix4x4();
        }

        for (u32 i = 0; i < node->mNumChildren; i++)
            BuildSkeletonHierarchy(skeleton, node->mChildren[i], current_bone_index, next_accumulated_parent_offset);
    }
}
