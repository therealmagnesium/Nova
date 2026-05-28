#pragma once
#include "Core/Base.h"
#include <glm/vec4.hpp>

namespace Nova
{
    struct Texture;
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
        const Texture* texture = NULL;
        GPULoadOp load_op = GPULoadOp::Load;
        GPUStoreOp store_op = GPUStoreOp::Store;
    };

    struct DepthStencilTargetInfo
    {
        const Texture* texture = NULL;
        float clear_depth = 0.f;
        GPULoadOp load_op = GPULoadOp::Load;
        GPUStoreOp store_op = GPUStoreOp::Store;
    };

    namespace RenderPasses
    {
        RenderPassHandle Begin(const ColorTargetInfo* color_target_infos, u8 color_target_count, const DepthStencilTargetInfo& info);
        void End(RenderPassHandle render_pass);
    }
}
