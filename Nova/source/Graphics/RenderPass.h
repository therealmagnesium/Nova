#pragma once
#include "Core/Base.h"
#include "Graphics/Framebuffer.h"

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

    struct RenderPassSpecification
    {
        glm::vec4 clear_color = glm::vec4(0.f, 0.f, 0.f, 1.f);
        float clear_depth = 1.f;
        GPULoadOp color_load_op = GPULoadOp::Clear;
        GPUStoreOp color_store_op = GPUStoreOp::Store;
        GPULoadOp depth_load_op = GPULoadOp::Clear;
        GPUStoreOp depth_store_op = GPUStoreOp::Discard;
    };

    inline const RenderPassSpecification Stub_RenderPassSpecification;

    namespace RenderPasses
    {
        /**
         * @brief Begins a hardware render pass using a structured engine Framebuffer.
         * @param framebuffer [in] The framebuffer containing our rendering targets. Pass NULL or Stub_Framebuffer to target the swapchain.
         * @param spec [in] The explicit behavioral load/store configurations applied across this pass execution.
         * @return An abstract render pass handle used for binding graphics pipelines.
         */
        RenderPassHandle Begin(const Framebuffer* framebuffer, const RenderPassSpecification& spec);
        void End(RenderPassHandle render_pass);
    }
}
