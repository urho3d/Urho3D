#include "../../Container/RefCounted.h"
#include "../Defines.h"

extern "C"
{

URHO3D_API void RefCounted_AddRef(RefCounted* nativeInstance)
{
    nativeInstance->AddRef();
}

URHO3D_API void RefCounted_ReleaseRef(RefCounted* nativeInstance)
{
    nativeInstance->ReleaseRef();
}

}
