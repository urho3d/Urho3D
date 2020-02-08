//
// Copyright (c) 2008-2020 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rightsR
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

#include "../../Core/Context.h"
#include "../../Core/Profiler.h"
#include "../../Graphics/Graphics.h"
#include "../../Graphics/GraphicsEvents.h"
#include "../../Graphics/GraphicsImpl.h"
#include "../../Graphics/Renderer.h"
#include "../../Graphics/Texture2D.h"
#include "../../IO/FileSystem.h"
#include "../../IO/Log.h"
#include "../../Resource/ResourceCache.h"
#include "../../Resource/XMLFile.h"

#include "../../DebugNew.h"

namespace Urho3D
{

void Texture2D::OnDeviceLost()
{
    GPUObject::OnDeviceLost();

    if (renderSurface_)
        renderSurface_->OnDeviceLost();
}

void Texture2D::OnDeviceReset()
{
    if (!object_.name_ || dataPending_)
    {
        // If has a resource file, reload through the resource cache. Otherwise just recreate.
        auto* cache = GetSubsystem<ResourceCache>();
        if (cache->Exists(GetName()))
            dataLost_ = !cache->ReloadResource(this);

        if (!object_.name_)
        {
            Create();
            dataLost_ = true;
        }
    }

    dataPending_ = false;
}

void Texture2D::Release()
{
    if (object_.name_)
    {
        if (!graphics_)
            return;

        if (!graphics_->IsDeviceLost())
        {
            for (unsigned i = 0; i < MAX_TEXTURE_UNITS; ++i)
            {
                if (graphics_->GetTexture(i) == this)
                    graphics_->SetTexture(i, nullptr);
            }

            glDeleteTextures(1, &object_.name_);
        }

        if (renderSurface_)
            renderSurface_->Release();

        object_.name_ = 0;
    }
    else
    {
        if (renderSurface_)
            renderSurface_->Release();
    }

    resolveDirty_ = false;
    levelsDirty_ = false;
}

bool Texture2D::SetData(unsigned level, int x, int y, int width, int height, const void* data)
{
    URHO3D_PROFILE(SetTextureData);

    if (!object_.name_ || !graphics_)
    {
        URHO3D_LOGERROR("No texture created, can not set data");
        return false;
    }

    if (!data)
    {
        URHO3D_LOGERROR("Null source for setting data");
        return false;
    }

    if (level >= levels_)
    {
        URHO3D_LOGERROR("Illegal mip level for setting data");
        return false;
    }

    if (graphics_->IsDeviceLost())
    {
        URHO3D_LOGWARNING("Texture data assignment while device is lost");
        dataPending_ = true;
        return true;
    }

    if (IsCompressed())
    {
        x &= ~3u;
        y &= ~3u;
    }

    int levelWidth = GetLevelWidth(level);
    int levelHeight = GetLevelHeight(level);
    if (x < 0 || x + width > levelWidth || y < 0 || y + height > levelHeight || width <= 0 || height <= 0)
    {
        URHO3D_LOGERROR("Illegal dimensions for setting data");
        return false;
    }

    graphics_->SetTextureForUpdate(this);

    bool wholeLevel = x == 0 && y == 0 && width == levelWidth && height == levelHeight;
    unsigned format = GetSRGB() ? GetSRGBFormat(format_) : format_;

    if (!IsCompressed())
    {
        if (wholeLevel)
            glTexImage2D(target_, level, format, width, height, 0, GetExternalFormat(format_), GetDataType(format_), data);
        else
            glTexSubImage2D(target_, level, x, y, width, height, GetExternalFormat(format_), GetDataType(format_), data);
    }
    else
    {
        if (wholeLevel)
            glCompressedTexImage2D(target_, level, format, width, height, 0, GetDataSize(width, height), data);
        else
            glCompressedTexSubImage2D(target_, level, x, y, width, height, format, GetDataSize(width, height), data);
    }

    graphics_->SetTexture(0, nullptr);
    return true;
}

bool Texture2D::SetData(Image* image, bool useAlpha)
{
    if (!image)
    {
        URHO3D_LOGERROR("Null image, can not set data");
        return false;
    }

    // Use a shared ptr for managing the temporary mip images created during this function
    SharedPtr<Image> mipImage;
    unsigned memoryUse = sizeof(Texture2D);
    MaterialQuality quality = QUALITY_HIGH;
    auto* renderer = GetSubsystem<Renderer>();
    if (renderer)
        quality = renderer->GetTextureQuality();

    if (!image->IsCompressed())
    {
        // Convert unsuitable formats to RGBA
        unsigned components = image->GetComponents();
        if (Graphics::GetGL3Support() && ((components == 1 && !useAlpha) || components == 2))
        {
            mipImage = image->ConvertToRGBA(); image = mipImage;
            if (!image)
                return false;
            components = image->GetComponents();
        }

        unsigned char* levelData = image->GetData();
        int levelWidth = image->GetWidth();
        int levelHeight = image->GetHeight();
        unsigned format = 0;

        // Discard unnecessary mip levels
        for (unsigned i = 0; i < mipsToSkip_[quality]; ++i)
        {
            mipImage = image->GetNextLevel(); image = mipImage;
            levelData = image->GetData();
            levelWidth = image->GetWidth();
            levelHeight = image->GetHeight();
        }

        switch (components)
        {
        case 1:
            format = useAlpha ? Graphics::GetAlphaFormat() : Graphics::GetLuminanceFormat();
            break;

        case 2:
            format = Graphics::GetLuminanceAlphaFormat();
            break;

        case 3:
            format = Graphics::GetRGBFormat();
            break;

        case 4:
            format = Graphics::GetRGBAFormat();
            break;

        default:
            assert(false);  // Should not reach here
            break;
        }

        // If image was previously compressed, reset number of requested levels to avoid error if level count is too high for new size
        if (IsCompressed() && requestedLevels_ > 1)
            requestedLevels_ = 0;
        SetSize(levelWidth, levelHeight, format);
        if (!object_.name_)
            return false;

        for (unsigned i = 0; i < levels_; ++i)
        {
            SetData(i, 0, 0, levelWidth, levelHeight, levelData);
            memoryUse += levelWidth * levelHeight * components;

            if (i < levels_ - 1)
            {
                mipImage = image->GetNextLevel(); image = mipImage;
                levelData = image->GetData();
                levelWidth = image->GetWidth();
                levelHeight = image->GetHeight();
            }
        }
    }
    else
    {
        int width = image->GetWidth();
        int height = image->GetHeight();
        unsigned levels = image->GetNumCompressedLevels();
        unsigned format = graphics_->GetFormat(image->GetCompressedFormat());
        bool needDecompress = false;

        if (!format)
        {
            format = Graphics::GetRGBAFormat();
            needDecompress = true;
        }

        unsigned mipsToSkip = mipsToSkip_[quality];
        if (mipsToSkip >= levels)
            mipsToSkip = levels - 1;
        while (mipsToSkip && (width / (1u << mipsToSkip) < 4 || height / (1u << mipsToSkip) < 4))
            --mipsToSkip;
        width /= (1u << mipsToSkip);
        height /= (1u << mipsToSkip);

        SetNumLevels(Max((levels - mipsToSkip), 1U));
        SetSize(width, height, format);

        for (unsigned i = 0; i < levels_ && i < levels - mipsToSkip; ++i)
        {
            CompressedLevel level = image->GetCompressedLevel(i + mipsToSkip);
            if (!needDecompress)
            {
                SetData(i, 0, 0, level.width_, level.height_, level.data_);
                memoryUse += level.rows_ * level.rowSize_;
            }
            else
            {
                auto* rgbaData = new unsigned char[level.width_ * level.height_ * 4];
                level.Decompress(rgbaData);
                SetData(i, 0, 0, level.width_, level.height_, rgbaData);
                memoryUse += level.width_ * level.height_ * 4;
                delete[] rgbaData;
            }
        }
    }

    SetMemoryUse(memoryUse);
    return true;
}

bool Texture2D::GetData(unsigned level, void* dest) const
{
    if (!object_.name_ || !graphics_)
    {
        URHO3D_LOGERROR("No texture created, can not get data");
        return false;
    }

#ifndef GL_ES_VERSION_2_0
    if (!dest)
    {
        URHO3D_LOGERROR("Null destination for getting data");
        return false;
    }

    if (level >= levels_)
    {
        URHO3D_LOGERROR("Illegal mip level for getting data");
        return false;
    }

    if (graphics_->IsDeviceLost())
    {
        URHO3D_LOGWARNING("Getting texture data while device is lost");
        return false;
    }

    if (multiSample_ > 1 && !autoResolve_)
    {
        URHO3D_LOGERROR("Can not get data from multisampled texture without autoresolve");
        return false;
    }

    if (resolveDirty_)
        graphics_->ResolveToTexture(const_cast<Texture2D*>(this));

    graphics_->SetTextureForUpdate(const_cast<Texture2D*>(this));

    if (!IsCompressed())
        glGetTexImage(target_, level, GetExternalFormat(format_), GetDataType(format_), dest);
    else
        glGetCompressedTexImage(target_, level, dest);

    graphics_->SetTexture(0, nullptr);
    return true;
#else
    // Special case on GLES: if the texture is a rendertarget, can make it current and use glReadPixels()
    if (usage_ == TEXTURE_RENDERTARGET)
    {
        graphics_->SetRenderTarget(0, const_cast<Texture2D*>(this));
        // Ensure the FBO is current; this viewport is actually never rendered to
        graphics_->SetViewport(IntRect(0, 0, width_, height_));
        glReadPixels(0, 0, width_, height_, GetExternalFormat(format_), GetDataType(format_), dest);
        return true;
    }

    URHO3D_LOGERROR("Getting texture data not supported");
    return false;
#endif
}

bool Texture2D::Create()
{
    Release();

    if (!graphics_ || !width_ || !height_)
        return false;

    if (graphics_->IsDeviceLost())
    {
        URHO3D_LOGWARNING("Texture creation while device is lost");
        return true;
    }

#ifdef GL_ES_VERSION_2_0
    if (multiSample_ > 1)
    {
        URHO3D_LOGWARNING("Multisampled texture is not supported on OpenGL ES");
        multiSample_ = 1;
        autoResolve_ = false;
    }
#endif

    unsigned format = GetSRGB() ? GetSRGBFormat(format_) : format_;
    unsigned externalFormat = GetExternalFormat(format_);
    unsigned dataType = GetDataType(format_);

    // Create a renderbuffer instead of a texture if depth texture is not properly supported, or if this will be a packed
    // depth stencil texture
#ifdef DESKTOP_GRAPHICS_OR_GLES3
    if (format == Graphics::GetDepthStencilFormat())
#else
    if (format == GL_DEPTH_COMPONENT16 || format == GL_DEPTH_COMPONENT24_OES || format == GL_DEPTH24_STENCIL8_OES ||
        (format == GL_DEPTH_COMPONENT && !graphics_->GetShadowMapFormat()))
#endif
    {
        if (renderSurface_)
        {
            renderSurface_->CreateRenderBuffer(width_, height_, format, multiSample_);
            return true;
        }
        else
            return false;
    }
    else
    {
        if (multiSample_ > 1)
        {
            if (autoResolve_)
            {
                // Multisample with autoresolve: create a renderbuffer for rendering, but also a texture
                renderSurface_->CreateRenderBuffer(width_, height_, format, multiSample_);
            }
            else
            {
                // Multisample without autoresolve: create a texture only
#ifndef GL_ES_VERSION_2_0
                if (!Graphics::GetGL3Support() && !GLEW_ARB_texture_multisample)
                {
                    URHO3D_LOGERROR("Multisampled texture extension not available");
                    return false;
                }

                target_ = GL_TEXTURE_2D_MULTISAMPLE;
                if (renderSurface_)
                    renderSurface_->target_ = GL_TEXTURE_2D_MULTISAMPLE;
#endif
            }
        }
    }

    glGenTextures(1, &object_.name_);

    // Ensure that our texture is bound to OpenGL texture unit 0
    graphics_->SetTextureForUpdate(this);

    // If not compressed, create the initial level 0 texture with null data
    bool success = true;

    if (!IsCompressed())
    {
        glGetError();
#ifndef GL_ES_VERSION_2_0
        if (multiSample_ > 1 && !autoResolve_)
        {
            glTexImage2DMultisample(target_, multiSample_, format, width_, height_, GL_TRUE);
        }
        else
#endif
        {
            glTexImage2D(target_, 0, format, width_, height_, 0, externalFormat, dataType, nullptr);
        }
        GLenum err = glGetError();
        if (err)
        {
            URHO3D_LOGERRORF("Failed to create 2D texture err=%d, target=%d, format=%d, externalFormat=%d, dataType=%d", err, target_, format, externalFormat, dataType);
            success = false;
        }
    }

    // Set mipmapping
    if (usage_ == TEXTURE_DEPTHSTENCIL || usage_ == TEXTURE_DYNAMIC)
        requestedLevels_ = 1;
    else if (usage_ == TEXTURE_RENDERTARGET)
    {
#if defined(__EMSCRIPTEN__) || defined(IOS) || defined(TVOS)
        // glGenerateMipmap appears to not be working on WebGL or iOS/tvOS, disable rendertarget mipmaps for now
        requestedLevels_ = 1;
#else
        if (requestedLevels_ != 1)
        {
            // Generate levels for the first time now
            RegenerateLevels();
            // Determine max. levels automatically
            requestedLevels_ = 0;
        }
#endif
    }

    levels_ = CheckMaxLevels(width_, height_, requestedLevels_);
#ifndef GL_ES_VERSION_2_0
    glTexParameteri(target_, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(target_, GL_TEXTURE_MAX_LEVEL, levels_ - 1);
#endif

    // Set initial parameters, then unbind the texture
    UpdateParameters();
    graphics_->SetTexture(0, nullptr);

    return success;
}

}
