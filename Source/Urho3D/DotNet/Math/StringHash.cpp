#include "../../Math/StringHash.h"
#include "../Defines.h"

extern "C"
{

URHO3D_API unsigned StringHash_Calculate(const char* str)
{
    return StringHash::Calculate(str);
}

}
