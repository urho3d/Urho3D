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

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Core/Profiler.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/GraphicsEvents.h"
#include "../Graphics/GraphicsImpl.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/Texture3D.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/XMLFile.h"

#include "../DebugNew.h"

namespace Urho3D
{

Texture3D::Texture3D(Context* context) :
    Texture(context)
{
#ifdef URHO3D_OPENGL
#ifndef URHO3D_GLES2
    target_ = GL_TEXTURE_3D;
#endif
#endif
}

Texture3D::~Texture3D()
{
    Release();
}

void Texture3D::RegisterObject(Context* context)
{
    context->RegisterFactory<Texture3D>();
}

bool Texture3D::BeginLoad(Deserializer& source)
{
    auto* cache = GetSubsystem<ResourceCache>();

    // In headless mode, do not actually load the texture, just return success
    if (!graphics_)
        return true;

    // If device is lost, retry later
    if (graphics_->IsDeviceLost())
    {
        URHO3D_LOGWARNING("Texture load while device is lost");
        dataPending_ = true;
        return true;
    }

    String texPath, texName, texExt;
    SplitPath(GetName(), texPath, texName, texExt);

    cache->ResetDependencies(this);

    loadParameters_ = new XMLFile(context_);
    if (!loadParameters_->Load(source))
    {
        loadParameters_.Reset();
        return false;
    }

    XMLElement textureElem = loadParameters_->GetRoot();
    XMLElement volumeElem = textureElem.GetChild("volume");
    XMLElement colorlutElem = textureElem.GetChild("colorlut");

    if (volumeElem)
    {
        String name = volumeElem.GetAttribute("name");

        String volumeTexPath, volumeTexName, volumeTexExt;
        SplitPath(name, volumeTexPath, volumeTexName, volumeTexExt);
        // If path is empty, add the XML file path
        if (volumeTexPath.Empty())
            name = texPath + name;

        loadImage_ = cache->GetTempResource<Image>(name);
        // Precalculate mip levels if async loading
        if (loadImage_ && GetAsyncLoadState() == ASYNC_LOADING)
            loadImage_->PrecalculateLevels();
        cache->StoreResourceDependency(this, name);
        return true;
    }
    else if (colorlutElem)
    {
        String name = colorlutElem.GetAttribute("name");

        String colorlutTexPath, colorlutTexName, colorlutTexExt;
        SplitPath(name, colorlutTexPath, colorlutTexName, colorlutTexExt);
        // If path is empty, add the XML file path
        if (colorlutTexPath.Empty())
            name = texPath + name;

        SharedPtr<File> file = GetSubsystem<ResourceCache>()->GetFile(name);
        loadImage_ = new Image(context_);
        if (!loadImage_->LoadColorLUT(*(file.Get())))
        {
            loadParameters_.Reset();
            loadImage_.Reset();
            return false;
        }
        // Precalculate mip levels if async loading
        if (loadImage_ && GetAsyncLoadState() == ASYNC_LOADING)
            loadImage_->PrecalculateLevels();
        cache->StoreResourceDependency(this, name);
        return true;
    }

    URHO3D_LOGERROR("Texture3D XML data for " + GetName() + " did not contain either volume or colorlut element");
    return false;
}


bool Texture3D::EndLoad()
{
    // In headless mode, do not actually load the texture, just return success
    if (!graphics_ || graphics_->IsDeviceLost())
        return true;

    // If over the texture budget, see if materials can be freed to allow textures to be freed
    CheckTextureBudget(GetTypeStatic());

    SetParameters(loadParameters_);
    bool success = SetData(loadImage_);

    loadImage_.Reset();
    loadParameters_.Reset();

    return success;
}

bool Texture3D::SetSize(int width, int height, int depth, unsigned format, TextureUsage usage)
{
    if (width <= 0 || height <= 0 || depth <= 0)
    {
        URHO3D_LOGERROR("Zero or negative 3D texture dimensions");
        return false;
    }
    if (usage >= TEXTURE_RENDERTARGET)
    {
        URHO3D_LOGERROR("Rendertarget or depth-stencil usage not supported for 3D textures");
        return false;
    }

    usage_ = usage;

    width_ = width;
    height_ = height;
    depth_ = depth;
    format_ = format;

    return Create();
}

}
