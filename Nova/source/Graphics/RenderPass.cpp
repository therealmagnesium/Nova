#include "Graphics/RenderPass.h"
#include "Graphics/Renderer.h"
#include "Graphics/Pipeline.h"
#include "Core/Log.h"

#include <SDL3/SDL_gpu.h>

namespace Nova::RenderPasses
{
    SDL_GPULoadOp GPULoadOpToSDL(GPULoadOp load_op);
    SDL_GPUStoreOp GPUStoreOpToSDL(GPUStoreOp store_op);

    RenderPassHandle Begin(const Framebuffer* framebuffer, const RenderPassSpecification& spec)
    {
        SDL_GPUCommandBuffer* command_buffer = static_cast<SDL_GPUCommandBuffer*>(Renderer::GetCommandBuffer());
        if (command_buffer == NULL)
            return NULL;

        // Determine if we are rendering to a user custom Framebuffer or falling back to the engine Swapchain
        const bool use_custom_framebuffer = (framebuffer != NULL && framebuffer->IsValid());

        u32 color_target_count = 0;
        SDL_GPUColorTargetInfo color_infos[8] = {}; // SDL_gpu supports up to 8 MRT configurations simultaneously

        if (use_custom_framebuffer)
        {
            color_target_count = static_cast<u32>(framebuffer->attachments_color.size());
            ASSERT(color_target_count <= 8, "RenderPasses::Begin - %s", "Framebuffer color targets exceed modern MRT hardware limits!");

            for (u32 i = 0; i < color_target_count; i++)
            {
                SDL_GPUColorTargetInfo& info = color_infos[i];
                info = {};
                info.texture = static_cast<SDL_GPUTexture*>(Textures::GetHandle(framebuffer->attachments_color[i]));

                // Inspect if a parallel multi-sampled resolve target asset exists for this specific MRT slice
                if (i < framebuffer->attachments_resolve.size())
                {
                    SDL_GPUTexture* resolve_texture_handle = static_cast<SDL_GPUTexture*>(Textures::GetHandle(framebuffer->attachments_resolve[i]));
                    ASSERT(resolve_texture_handle != NULL,
                           "RenderPasses::Begin - Mismatch detected! Store op requests RESOLVE at target slice %u, but resolve hardware attachment handle is NULL.", i);

                    info.resolve_texture = resolve_texture_handle;
                    info.store_op = (spec.color_store_op == GPUStoreOp::Store) ? SDL_GPU_STOREOP_RESOLVE : GPUStoreOpToSDL(spec.color_store_op);
                }
                else
                {
                    info.resolve_texture = NULL;
                    info.store_op = GPUStoreOpToSDL(spec.color_store_op);
                }

                info.clear_color = (SDL_FColor){ spec.clear_color.r, spec.clear_color.g, spec.clear_color.b, spec.clear_color.a };
                info.load_op = GPULoadOpToSDL(spec.color_load_op);
                info.resolve_mip_level = 0;
                info.resolve_layer = 0;
                info.mip_level = 0;
                info.layer_or_depth_plane = 0;
                info.cycle = false;
            }
        }
        else
        {
            // SWAPCHAIN FALLBACK: Render directly to screen backbuffers
            color_target_count = 1;
            SDL_GPUColorTargetInfo& info = color_infos[0];
            info = {};
            info.texture = static_cast<SDL_GPUTexture*>(Renderer::GetSwapchainHandle());
            info.resolve_texture = NULL;
            info.clear_color = (SDL_FColor){ spec.clear_color.r, spec.clear_color.g, spec.clear_color.b, spec.clear_color.a };
            info.load_op = GPULoadOpToSDL(spec.color_load_op);
            info.store_op = GPUStoreOpToSDL(spec.color_store_op);
            info.cycle = false;
        }

        // Configure depth-stencil descriptor attachments mapping setup
        SDL_GPUDepthStencilTargetInfo ds_info = {};
        bool has_depth = false;

        if (use_custom_framebuffer)
        {
            if (framebuffer->attachment_depth_stencil.IsValid())
            {
                ds_info.texture = static_cast<SDL_GPUTexture*>(Textures::GetHandle(framebuffer->attachment_depth_stencil));
                has_depth = true;
            }
        }
        else
        {
            // Default depth system fallback pulled straight out of central renderer configuration caches
            ds_info.texture = static_cast<SDL_GPUTexture*>(Textures::GetHandle(Renderer::GetTextureDepthStencil()));
            has_depth = true;
        }

        if (has_depth)
        {
            ds_info.clear_depth = spec.clear_depth;
            ds_info.load_op = GPULoadOpToSDL(spec.depth_load_op);
            ds_info.store_op = GPUStoreOpToSDL(spec.depth_store_op);
            ds_info.cycle = false;
            ds_info.mip_level = 0;
        }

        SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, color_infos, color_target_count, has_depth ? &ds_info : NULL);
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
