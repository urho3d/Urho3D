//
// Copyright (c) 2008-2020 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../../Precompiled.h"

#include "../../Graphics/Graphics.h"
#include "../../Graphics/GraphicsImpl.h"
#include "../../Graphics/Material.h"
#include "../../Graphics/RenderSurface.h"
#include "../../IO/Log.h"
#include "../../Resource/ResourceCache.h"
#include "../../Resource/XMLFile.h"

#include "../../DebugNew.h"

#if URHO3D_GLES3
#define GL_COMPARE_R_TO_TEXTURE GL_COMPARE_REF_TO_TEXTURE
#endif

namespace Urho3D
{

static GLenum glWrapModes[] =
{
    GL_REPEAT,
    GL_MIRRORED_REPEAT,
    GL_CLAMP_TO_EDGE,
#ifndef GL_ES_VERSION_2_0
    GL_CLAMP
#else
    GL_CLAMP_TO_EDGE
#endif
};

#ifndef GL_ES_VERSION_2_0
static GLenum gl3WrapModes[] =
{
    GL_REPEAT,
    GL_MIRRORED_REPEAT,
    GL_CLAMP_TO_EDGE,
    GL_CLAMP_TO_BORDER
};
#endif

static GLenum GetWrapMode(TextureAddressMode mode)
{
#ifndef GL_ES_VERSION_2_0
    return Graphics::GetGL3Support() ? gl3WrapModes[mode] : glWrapModes[mode];
#else
    return glWrapModes[mode];
#endif
}

void Texture::SetSRGB(bool enable)
{
    if (graphics_)
        enable &= graphics_->GetSRGBSupport();

    if (enable != sRGB_)
    {
        sRGB_ = enable;
        // If texture had already been created, must recreate it to set the sRGB texture format
        if (object_.name_)
            Create();

        // If texture in use in the framebuffer, mark it dirty
        if (graphics_ && graphics_->GetRenderTarget(0) && graphics_->GetRenderTarget(0)->GetParentTexture() == this)
            graphics_->MarkFBODirty();
    }
}

void Texture::UpdateParameters()
{
    if (!object_.name_ || !graphics_)
        return;

    // If texture is multisampled, do not attempt to set parameters as it's illegal, just return
#ifndef GL_ES_VERSION_2_0
    if (target_ == GL_TEXTURE_2D_MULTISAMPLE)
    {
        parametersDirty_ = false;
        return;
    }
#endif

    // Wrapping
    glTexParameteri(target_, GL_TEXTURE_WRAP_S, GetWrapMode(addressModes_[COORD_U]));
    glTexParameteri(target_, GL_TEXTURE_WRAP_T, GetWrapMode(addressModes_[COORD_V]));
#ifndef URHO3D_GLES2
    glTexParameteri(target_, GL_TEXTURE_WRAP_R, GetWrapMode(addressModes_[COORD_W]));
#endif

    TextureFilterMode filterMode = filterMode_;
    if (filterMode == FILTER_DEFAULT)
        filterMode = graphics_->GetDefaultTextureFilterMode();

    // Filtering
    switch (filterMode)
    {
    case FILTER_NEAREST:
        if (levels_ < 2)
            glTexParameteri(target_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        else
            glTexParameteri(target_, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTexParameteri(target_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        break;

    case FILTER_BILINEAR:
        if (levels_ < 2)
            glTexParameteri(target_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        else
            glTexParameteri(target_, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTexParameteri(target_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        break;

    case FILTER_ANISOTROPIC:
    case FILTER_TRILINEAR:
        if (levels_ < 2)
            glTexParameteri(target_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        else
            glTexParameteri(target_, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(target_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        break;

    case FILTER_NEAREST_ANISOTROPIC:
        if (levels_ < 2)
            glTexParameteri(target_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        else
            glTexParameteri(target_, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
        glTexParameteri(target_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        break;

    default:
        break;
    }

#ifndef GL_ES_VERSION_2_0
    // Anisotropy
    if (graphics_->GetAnisotropySupport())
    {
        unsigned maxAnisotropy = anisotropy_ ? anisotropy_ : graphics_->GetDefaultTextureAnisotropy();
        glTexParameterf(target_, GL_TEXTURE_MAX_ANISOTROPY_EXT,
            (filterMode == FILTER_ANISOTROPIC || filterMode == FILTER_NEAREST_ANISOTROPIC) ? (float)maxAnisotropy : 1.0f);
    }
    glTexParameterfv(target_, GL_TEXTURE_BORDER_COLOR, borderColor_.Data());
#endif

#ifndef URHO3D_GLES2
    // Shadow compare
    if (shadowCompare_)
    {
        glTexParameteri(target_, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
        glTexParameteri(target_, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    }
    else
        glTexParameteri(target_, GL_TEXTURE_COMPARE_MODE, GL_NONE);
#endif

    parametersDirty_ = false;
}

bool Texture::GetParametersDirty() const
{
    return parametersDirty_;
}

bool Texture::IsCompressed() const
{
    return format_ == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT || format_ == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT ||
           format_ == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT || format_ == GL_ETC1_RGB8_OES ||
           format_ == GL_ETC2_RGB8_OES || format_ == GL_ETC2_RGBA8_OES ||
           format_ == COMPRESSED_RGB_PVRTC_4BPPV1_IMG || format_ == COMPRESSED_RGBA_PVRTC_4BPPV1_IMG ||
           format_ == COMPRESSED_RGB_PVRTC_2BPPV1_IMG || format_ == COMPRESSED_RGBA_PVRTC_2BPPV1_IMG ||
           format_ == GL_ETC2_RGB8_OES || format_ == GL_ETC2_RGBA8_OES;
}

unsigned Texture::GetRowDataSize(int width) const
{
    switch (format_)
    {
    case GL_ALPHA:
    case GL_LUMINANCE:
        return (unsigned)width;

    case GL_LUMINANCE_ALPHA:
        return (unsigned)(width * 2);

    case GL_RGB:
        return (unsigned)(width * 3);

    case GL_RGBA:
#ifndef GL_ES_VERSION_2_0
    case GL_DEPTH24_STENCIL8_EXT:
    case GL_RG16:
    case GL_RG16F:
    case GL_R32F:
#endif
#ifdef GL_ES_VERSION_3_0
    case GL_DEPTH24_STENCIL8:
#endif
        return (unsigned)(width * 4);

#ifndef GL_ES_VERSION_2_0
    case GL_R8:
        return (unsigned)width;

    case GL_RG8:
    case GL_R16F:
        return (unsigned)(width * 2);

    case GL_RGBA16:
    case GL_RGBA16F_ARB:
        return (unsigned)(width * 8);

    case GL_RGBA32F_ARB:
        return (unsigned)(width * 16);
#endif
#ifdef GL_ES_VERSION_3_0
    case GL_R8:
        return (unsigned) width;

    case GL_RG8:
    case GL_R16F:
        return (unsigned) (width * 2);
    case GL_RGBA16F:
        return (unsigned) (width * 8);
    case GL_RGBA32F:
        return (unsigned) (width * 16);
#endif

    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        return ((unsigned)(width + 3) >> 2u) * 8;

    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        return ((unsigned)(width + 3) >> 2u) * 16;

    case GL_ETC1_RGB8_OES:
    case GL_ETC2_RGB8_OES:
        return ((unsigned)(width + 3) >> 2u) * 8;

    case GL_ETC2_RGBA8_OES:
        return ((unsigned)(width + 3) >> 2u) * 16;

    case COMPRESSED_RGB_PVRTC_4BPPV1_IMG:
    case COMPRESSED_RGBA_PVRTC_4BPPV1_IMG:
        return ((unsigned)(width + 3) >> 2u) * 8;

    case COMPRESSED_RGB_PVRTC_2BPPV1_IMG:
    case COMPRESSED_RGBA_PVRTC_2BPPV1_IMG:
        return ((unsigned)(width + 7) >> 3u) * 8;

    default:
        return 0;
    }
}

#ifdef GL_ES_VERSION_3_0
#define GL_SRGB_EXT GL_SRGB
#define GL_SRGB_ALPHA_EXT GL_SRGB8_ALPHA8
#endif

unsigned Texture::GetExternalFormat(unsigned format)
{
#ifndef GL_ES_VERSION_2_0
    if (format == GL_DEPTH_COMPONENT16 || format == GL_DEPTH_COMPONENT24 || format == GL_DEPTH_COMPONENT32)
        return GL_DEPTH_COMPONENT;
    else if (format == GL_DEPTH24_STENCIL8_EXT)
        return GL_DEPTH_STENCIL_EXT;
    else if (format == GL_SLUMINANCE_EXT)
        return GL_LUMINANCE;
    else if (format == GL_SLUMINANCE_ALPHA_EXT)
        return GL_LUMINANCE_ALPHA;
    else if (format == GL_R8 || format == GL_R16F || format == GL_R32F)
        return GL_RED;
    else if (format == GL_RG8 || format == GL_RG16 || format == GL_RG16F || format == GL_RG32F)
        return GL_RG;
    else if (format == GL_RGBA16 || format == GL_RGBA16F_ARB || format == GL_RGBA32F_ARB || format == GL_SRGB_ALPHA_EXT)
        return GL_RGBA;
    else if (format == GL_SRGB_EXT)
        return GL_RGB;
    else
        return format;
#else
#if defined(GL_ES_VERSION_3_0)
    if (format == GL_DEPTH_COMPONENT16 || format == GL_DEPTH_COMPONENT24 || format == GL_DEPTH_COMPONENT32F)
        return GL_DEPTH_COMPONENT;
    else if (format == GL_DEPTH24_STENCIL8)
        return GL_DEPTH_STENCIL;
    else if (format == GL_R8 || format == GL_R16F || format == GL_R32F)
        return GL_RED;
    else if (format == GL_RG8 || format == GL_RG16F || format == GL_RG32F)
        return GL_RG;
    else if (format == GL_RGBA16F || format == GL_RGBA32F || format == GL_SRGB_ALPHA_EXT)
        return GL_RGBA;
    else if (format == GL_RG16UI)
        return GL_RG_INTEGER;
    else if (format == GL_RGBA16UI)
        return GL_RGBA_INTEGER;
#endif
    return format;
#endif
}

unsigned Texture::GetDataType(unsigned format)
{
#ifndef GL_ES_VERSION_2_0
    if (format == GL_DEPTH24_STENCIL8_EXT)
        return GL_UNSIGNED_INT_24_8_EXT;
    else if (format == GL_RG16 || format == GL_RGBA16)
        return GL_UNSIGNED_SHORT;
    else if (format == GL_RGBA32F_ARB || format == GL_RG32F || format == GL_R32F)
        return GL_FLOAT;
    else if (format == GL_RGBA16F_ARB || format == GL_RG16F || format == GL_R16F)
        return GL_HALF_FLOAT_ARB;
    else
        return GL_UNSIGNED_BYTE;
#else
#if defined(GL_ES_VERSION_3_0)
    if (format == GL_DEPTH_COMPONENT24)
        return GL_UNSIGNED_INT;
    else if (format == GL_DEPTH24_STENCIL8)
        return GL_UNSIGNED_INT_24_8;
    else if (format == GL_RGBA16UI || format == GL_RG16UI)
        return GL_UNSIGNED_SHORT;
    else if (format == GL_RGBA16F || format == GL_RG16F)
        return GL_HALF_FLOAT;
    else if (format == GL_DEPTH_COMPONENT32F || format == GL_RGBA32F || format == GL_RG32F || format == GL_R32F)
        return GL_FLOAT;
    else if (format == GL_RGBA16F || format == GL_RG16F || format == GL_R16F)
        return GL_HALF_FLOAT;
#endif
    if (format == GL_DEPTH_COMPONENT24_OES)
        return GL_UNSIGNED_INT;
    else if (format == GL_DEPTH_COMPONENT || format == GL_DEPTH_COMPONENT16)
        return GL_UNSIGNED_SHORT;
    else
        return GL_UNSIGNED_BYTE;
#endif
}

unsigned Texture::GetSRGBFormat(unsigned format)
{
#ifndef URHO3D_GLES2
    if (!graphics_ || !graphics_->GetSRGBSupport())
        return format;

    switch (format)
    {
    case GL_RGB:
        return GL_SRGB_EXT;
    case GL_RGBA:
        return GL_SRGB_ALPHA_EXT;
#ifndef GL_ES_VERSION_3_0
    case GL_LUMINANCE:
        return GL_SLUMINANCE_EXT;
    case GL_LUMINANCE_ALPHA:
        return GL_SLUMINANCE_ALPHA_EXT;
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
#endif
    default:
        return format;
    }
#else
    return format;
#endif
}

void Texture::RegenerateLevels()
{
    if (!object_.name_)
        return;

#ifndef GL_ES_VERSION_2_0
    if (Graphics::GetGL3Support())
        glGenerateMipmap(target_);
    else
        glGenerateMipmapEXT(target_);
#else
    glGenerateMipmap(target_);
#endif

    levelsDirty_ = false;
}

}
