#pragma once
#include "Core/Base.h"
#include "Graphics/Texture.h"

#include <glm/vec4.hpp>

namespace Nova
{
    using RenderPassHandle = void*;

    enum class GPULoadOp : u8
    {
        Load = 0,
        Clear,
        Discard
    };

    enum class GPUStoreOp : u8
    {
        Store = 0,
        Discard,
        Resolve,
        ResolveAndStore
    };

    struct ColorTargetInfo
    {
        glm::vec4 clear_color = glm::vec4(0.f, 0.f, 0.f, 1.f);
        TextureHandle texture = NULL;
        TextureHandle texture_msaa_resolve = NULL;
        GPULoadOp load_op = GPULoadOp::Load;
        GPUStoreOp store_op = GPUStoreOp::Store;
        u8 layer = 0;
        u8 mip_level = 0;
    };

    struct DepthStencilTargetInfo
    {
        TextureHandle texture = NULL;
        float clear_depth = 0.f;
        GPULoadOp load_op = GPULoadOp::Load;
        GPUStoreOp store_op = GPUStoreOp::Store;

        inline bool operator==(const DepthStencilTargetInfo& other) const
        {
            return texture == other.texture && clear_depth == other.clear_depth &&
                   load_op == other.load_op && store_op == other.store_op;
        }
        inline bool operator!=(const DepthStencilTargetInfo& other) const { return !(*this == other); }
    };

    inline const ColorTargetInfo Stub_ColorTargetInfo;
    inline const DepthStencilTargetInfo Stub_DepthStencilTargetInfo;

    namespace RenderPasses
    {
        RenderPassHandle Begin(const ColorTargetInfo* color_target_infos, u8 color_target_count, const DepthStencilTargetInfo& info);
        void End(RenderPassHandle render_pass);
    }
}
