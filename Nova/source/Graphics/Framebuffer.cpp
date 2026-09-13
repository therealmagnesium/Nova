#include "Graphics/Framebuffer.h"
#include "Core/Log.h"

namespace Nova::Framebuffers
{
    struct CachedFramebuffer
    {
        FramebufferSpecification spec;
        Framebuffer metadata;
    };

    static u32 next_id = 0;
    static std::unordered_map<u32, CachedFramebuffer> cache;

    Framebuffer Create(const FramebufferSpecification& spec)
    {
        ASSERT_RETURN(spec.width > 0 && spec.height > 0, Stub_Framebuffer,
                      "Framebuffers::Create - Dimensions must be greater than zero! Given: %dx%d", spec.width, spec.height);
        ASSERT_RETURN(!spec.attachments.empty(), Stub_Framebuffer,
                      "Framebuffers::Create - %s", "Cannot build a framebuffer with zero attachment specifications!");

        Framebuffer framebuffer;
        framebuffer.id = ++next_id;
        framebuffer.width = spec.width;
        framebuffer.height = spec.height;

        for (const auto& attach_spec : spec.attachments)
        {
            if (attach_spec.type == FramebufferAttachmentType::Color)
            {
                // Always generate the base rendering target destination texture
                Texture color_texture = Textures::CreateFramebufferAttachmentHDR(framebuffer.width, framebuffer.height, attach_spec.msaa);
                framebuffer.attachments_color.push_back(color_texture);

                // If the target configuration requested multi-sampling, automatically bind its resolve texture asset here
                if (attach_spec.requires_resolve)
                {
                    Texture resolve_texture = Textures::CreateFramebufferAttachmentHDR(framebuffer.width, framebuffer.height, MSAASamples::One);
                    framebuffer.attachments_resolve.push_back(resolve_texture);
                }
            }
            else if (attach_spec.type == FramebufferAttachmentType::DepthStencil)
                framebuffer.attachment_depth_stencil = Textures::CreateFramebufferAttachmentDepth(framebuffer.width, framebuffer.height, attach_spec.msaa);
        }

        const CachedFramebuffer cache_entry = {
            .spec = spec,
            .metadata = framebuffer
        };
        cache[framebuffer.id] = std::move(cache_entry);

        INFO("Framebuffer ID %u successfully generated [%dx%d] with %zu color targets.",
             framebuffer.id, framebuffer.width,
             framebuffer.height, framebuffer.attachments_color.size());
        return framebuffer;
    }

    void Destroy(Framebuffer& framebuffer)
    {
        if (framebuffer.id == 0)
            return;

        auto it = cache.find(framebuffer.id);
        if (it != cache.end())
        {
            for (auto& attachment : it->second.metadata.attachments_color)
                Textures::Unload(attachment);
            for (auto& attachment : it->second.metadata.attachments_resolve)
                Textures::Unload(attachment);
            if (it->second.metadata.attachment_depth_stencil.IsValid())
                Textures::Unload(it->second.metadata.attachment_depth_stencil);
            cache.erase(it);
        }

        framebuffer.id = 0;
        framebuffer.width = 0;
        framebuffer.height = 0;
        framebuffer.attachments_color.clear();
        framebuffer.attachments_resolve.clear();
        framebuffer.attachment_depth_stencil = Stub_Texture;
    }

    void Resize(Framebuffer& framebuffer, u16 width, u16 height)
    {
        if (framebuffer.width == width && framebuffer.height == height)
            return;

        auto it = cache.find(framebuffer.id);
        if (it == cache.end())
        {
            ERROR("Framebuffers::Resize - Attempted to resize an untracked or stale Framebuffer ID: %u", framebuffer.id);
            return;
        }

        FramebufferSpecification saved_spec = it->second.spec;
        saved_spec.width = width;
        saved_spec.height = height;

        // Strip structural handles while keeping allocations clean
        Destroy(framebuffer);

        // Recompile targets matching our saved structural footprint
        framebuffer = Create(saved_spec);
    }

    const Texture& GetColorAttachment(const Framebuffer& framebuffer, u32 index)
    {
        auto it = cache.find(framebuffer.id);
        ASSERT_RETURN(it != cache.end(), Stub_Texture,
                      "Framebuffers::GetColorAttachment - %s", "Invalid framebuffer ID!");

        if (index >= it->second.metadata.attachments_color.size())
            return Stub_Texture;

        return it->second.metadata.attachments_color[index];
    }

    const Texture& GetResolveAttachment(const Framebuffer& framebuffer, u32 index)
    {
        auto it = cache.find(framebuffer.id);
        ASSERT_RETURN(it != cache.end(), Stub_Texture,
                      "Framebuffers::GetResolveAttachment - %s", "Invalid framebuffer ID!");

        if (index >= it->second.metadata.attachments_resolve.size())
            return Stub_Texture;

        return it->second.metadata.attachments_resolve[index];
    }

    bool HasDepthAttachment(const Framebuffer& framebuffer)
    {
        auto it = cache.find(framebuffer.id);

        if (it == cache.end())
            return false;

        return it->second.metadata.attachment_depth_stencil.IsValid();
    }

}
