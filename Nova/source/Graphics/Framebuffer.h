#pragma once
#include "Graphics/Texture.h"
#include "Core/Base.h"

#include <vector>

namespace Nova
{
    enum class FramebufferAttachmentType : u8
    {
        Color = 0,
        DepthStencil
    };

    struct FramebufferAttachmentSpecification
    {
        TextureFormat format = TextureFormat::RGBA8;
        TextureSampler sampler = TextureSampler::LinearClamp;
        MSAASamples msaa = MSAASamples::One;
        FramebufferAttachmentType type = FramebufferAttachmentType::Color;
        bool requires_resolve = false;
    };

    struct FramebufferSpecification
    {
        u16 width = 0;
        u16 height = 0;
        std::vector<FramebufferAttachmentSpecification> attachments;
    };

    struct Framebuffer
    {
        u32 id = 0;
        u16 width = 0;
        u16 height = 0;
        Texture attachment_depth_stencil = Stub_Texture;
        std::vector<Texture> attachments_color;
        std::vector<Texture> attachments_resolve;

        inline bool IsValid() const { return id != 0; }
    };

    inline const Framebuffer Stub_Framebuffer;

    namespace Framebuffers
    {
        Framebuffer Create(const FramebufferSpecification& spec);
        void Destroy(Framebuffer& framebuffer);
        void Resize(Framebuffer& framebuffer, u16 width, u16 height);

        const Texture& GetColorAttachment(const Framebuffer& framebuffer, u32 index = 0);
        const Texture& GetResolveAttachment(const Framebuffer& framebuffer, u32 index = 0);
        bool HasDepthAttachment(const Framebuffer& framebuffer);
    }
}
