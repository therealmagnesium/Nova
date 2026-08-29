#include "Graphics/RenderPass.h"
#include "Graphics/Renderer.h"
#include "Graphics/Pipeline.h"

#include <SDL3/SDL_gpu.h>

namespace Nova::RenderPasses
{
    SDL_GPULoadOp GPULoadOpToSDL(GPULoadOp load_op);
    SDL_GPUStoreOp GPUStoreOpToSDL(GPUStoreOp store_op);

    RenderPassHandle Begin(const ColorTargetInfo* color_target_infos, u8 color_target_count, const DepthStencilTargetInfo& depth_stencil_target_info)
    {
        SDL_GPUCommandBuffer* command_buffer = static_cast<SDL_GPUCommandBuffer*>(Renderer::GetCommandBuffer());
        if (command_buffer == NULL)
            return NULL;

        SDL_GPUColorTargetInfo color_infos[color_target_count];

        for (u8 i = 0; i < color_target_count; i++)
        {
            const ColorTargetInfo& p_info = color_target_infos[i];
            SDL_GPUColorTargetInfo& info = color_infos[i];
            info = {};
            info.texture = p_info.texture != NULL ? static_cast<SDL_GPUTexture*>(p_info.texture) : static_cast<SDL_GPUTexture*>(Renderer::GetSwapchainHandle());
            info.resolve_texture = p_info.texture_msaa_resolve != NULL ? static_cast<SDL_GPUTexture*>(p_info.texture_msaa_resolve) : NULL;
            info.resolve_mip_level = 0;
            info.resolve_layer = 0;
            info.clear_color = (SDL_FColor){ p_info.clear_color.r, p_info.clear_color.g, p_info.clear_color.b, p_info.clear_color.a };
            info.load_op = GPULoadOpToSDL(p_info.load_op);
            info.store_op = GPUStoreOpToSDL(p_info.store_op);
            info.mip_level = p_info.mip_level;
            info.layer_or_depth_plane = p_info.layer;
            info.cycle = false;
        }

        const auto ds_info = (SDL_GPUDepthStencilTargetInfo){
            .texture = depth_stencil_target_info.texture != NULL ? static_cast<SDL_GPUTexture*>(depth_stencil_target_info.texture) : static_cast<SDL_GPUTexture*>(Textures::GetHandle(Renderer::GetTextureDepthStencil())),
            .clear_depth = depth_stencil_target_info.clear_depth,
            .load_op = GPULoadOpToSDL(depth_stencil_target_info.load_op),
            .store_op = GPUStoreOpToSDL(depth_stencil_target_info.store_op),
            .cycle = false,
            .mip_level = 0
        };

        SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, color_infos, color_target_count, depth_stencil_target_info != Stub_DepthStencilTargetInfo ? &ds_info : NULL);
        Renderer::SetActiveRenderPass(render_pass);

        return render_pass;
    }

    void End(RenderPassHandle render_pass)
    {
        Renderer::Flush();
        SDL_EndGPURenderPass(static_cast<SDL_GPURenderPass*>(render_pass));
        Pipelines::ResetBindingCache();
        Renderer::SetActiveRenderPass(NULL);
    }

    SDL_GPULoadOp GPULoadOpToSDL(GPULoadOp load_op)
    {
        SDL_GPULoadOp op = SDL_GPU_LOADOP_LOAD;
        switch (load_op)
        {
            case GPULoadOp::Load:
                op = SDL_GPU_LOADOP_LOAD;
                break;
            case GPULoadOp::Clear:
                op = SDL_GPU_LOADOP_CLEAR;
                break;
            case GPULoadOp::Discard:
                op = SDL_GPU_LOADOP_DONT_CARE;
                break;
        }
        return op;
    }

    SDL_GPUStoreOp GPUStoreOpToSDL(GPUStoreOp store_op)
    {
        SDL_GPUStoreOp op = SDL_GPU_STOREOP_STORE;
        switch (store_op)
        {
            case GPUStoreOp::Store:
                op = SDL_GPU_STOREOP_STORE;
                break;
            case GPUStoreOp::Discard:
                op = SDL_GPU_STOREOP_DONT_CARE;
                break;
            case GPUStoreOp::Resolve:
                op = SDL_GPU_STOREOP_RESOLVE;
                break;
            case GPUStoreOp::ResolveAndStore:
                op = SDL_GPU_STOREOP_RESOLVE_AND_STORE;
                break;
        }
        return op;
    }
}
