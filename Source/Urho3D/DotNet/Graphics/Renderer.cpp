#include "../../Graphics/Renderer.h"
#include "../Defines.h"

extern "C"
{

URHO3D_API void Renderer_SetViewport(Renderer* nativeInstance, unsigned index, Viewport* viewport)
{
    nativeInstance->SetViewport(index, viewport);
}

}