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

#pragma once

#ifdef AS_MAX_PORTABILITY
#include "../AngelScript/aswrappedcall.h"
#endif

namespace Urho3D
{

// f - function
// c - class
// m - method
// p - parameters
// r - return type
// PR - parameters, return type

// RegisterGlobalFunction(decl, AS_FUNCTION(f), AS_CALL_CDECL)
// RegisterGlobalFunction(decl, AS_FUNCTIONPR(f, p, r), AS_CALL_CDECL)
// RegisterObjectBehaviour(obj, asBEHAVE_FACTORY, decl, AS_FUNCTION(f), AS_CALL_CDECL);
#ifdef AS_MAX_PORTABILITY
	#define AS_FUNCTION(f) WRAP_FN(f) // RegisterGlobalFunction(decl, WRAP_FN(f), asCALL_GENERIC)
	#define AS_FUNCTIONPR(f, p, r) WRAP_FN_PR(f, p, r) // RegisterGlobalFunction(decl, WRAP_FN_PR(f, p, r), asCALL_GENERIC)
	#define AS_CALL_CDECL asCALL_GENERIC
#else
	#define AS_FUNCTION(f) asFUNCTION(f) // RegisterGlobalFunction(decl, asFUNCTION(f), asCALL_CDECL)
	#define AS_FUNCTIONPR(f, p, r) asFUNCTIONPR(f, p, r) // RegisterGlobalFunction(decl, asFUNCTIONPR(f, p, r), asCALL_CDECL)
	#define AS_CALL_CDECL asCALL_CDECL
#endif

// RegisterObjectMethod(obj, decl, AS_METHOD(c, m), AS_CALL_THISCALL)
// RegisterObjectMethod(obj, decl, AS_METHODPR(c, m, p, r), AS_CALL_THISCALL)
// RegisterObjectBehaviour(obj, asBEHAVE_ADDREF, decl, AS_METHODPR(c, m, p, r), AS_CALL_THISCALL)
// RegisterObjectBehaviour(obj, asBEHAVE_RELEASE, decl, AS_METHODPR(c, m, p, r), AS_CALL_THISCALL);
#ifdef AS_MAX_PORTABILITY
	#define AS_METHOD(c, m) WRAP_MFN(c, m) // RegisterObjectMethod(obj, decl, WRAP_MFN(c, m), asCALL_GENERIC)
	#define AS_METHODPR(c, m, p, r) WRAP_MFN_PR(c, m, p, r) // RegisterObjectMethod(obj, decl, WRAP_MFN_PR(c, m, p, r), asCALL_GENERIC)
	#define AS_CALL_THISCALL asCALL_GENERIC
#else
	#define AS_METHOD(c, m) asMETHOD(c, m) // RegisterObjectMethod(obj, decl, asMETHOD(c, m), asCALL_THISCALL)
	#define AS_METHODPR(c, m, p, r) asMETHODPR(c, m, p, r) // RegisterObjectMethod(obj, decl, asMETHODPR(c, m, p, r), asCALL_THISCALL)
	#define AS_CALL_THISCALL asCALL_THISCALL
#endif

// RegisterObjectMethod(obj, decl, AS_FUNCTION_OBJFIRST(f), AS_CALL_CDECL_OBJFIRST)
// RegisterObjectMethod(obj, decl, AS_FUNCTION_OBJLAST(f), AS_CALL_CDECL_OBJLAST)
// RegisterObjectMethod(obj, decl, AS_FUNCTIONPT_OBJFIRST(f, p, r), AS_CALL_CDECL_OBJFIRST)
// RegisterObjectMethod(obj, decl, AS_FUNCTIONPT_OBJLAST(f, p, r), AS_CALL_CDECL_OBJLAST)
#ifdef AS_MAX_PORTABILITY
	#define AS_FUNCTION_OBJFIRST(f) WRAP_OBJ_FIRST(f) // RegisterObjectMethod(obj, decl, WRAP_OBJ_FIRST(f), asCALL_GENERIC)
	#define AS_FUNCTION_OBJLAST(f) WRAP_OBJ_LAST(f) // RegisterObjectMethod(obj, decl, WRAP_OBJ_LAST(f), asCALL_GENERIC)
	#define AS_FUNCTIONPR_OBJFIRST(f, p, r) WRAP_OBJ_FIRST_PR(f, p, r) // RegisterObjectMethod(obj, decl, WRAP_OBJ_FIRST_PR(f, p, r), asCALL_GENERIC)
	#define AS_FUNCTIONPR_OBJLAST(f, p, r) WRAP_OBJ_LAST_PR(f, p, r) // RegisterObjectMethod(obj, decl, WRAP_OBJ_LAST_PR(f, p, r), asCALL_GENERIC)
	#define AS_CALL_CDECL_OBJFIRST asCALL_GENERIC
	#define AS_CALL_CDECL_OBJLAST asCALL_GENERIC
#else
	#define AS_FUNCTION_OBJFIRST(f) asFUNCTION(f) // RegisterObjectMethod(obj, decl, asFUNCTION(f), asCALL_CDECL_OBJFIRST)
	#define AS_FUNCTION_OBJLAST(f) asFUNCTION(f) // RegisterObjectMethod(obj, decl, asFUNCTION(f), asCALL_CDECL_OBJLAST)
	#define AS_FUNCTIONPR_OBJFIRST(f, p, r) asFUNCTIONPR(f, p, r) // RegisterObjectMethod(obj, decl, asFUNCTIONPR(f, p, r), asCALL_CDECL_OBJFIRST)
	#define AS_FUNCTIONPR_OBJLAST(f, p, r) asFUNCTIONPR(f, p, r) // RegisterObjectMethod(obj, decl, asFUNCTIONPR(f, p, r), asCALL_CDECL_OBJLAST)
	#define AS_CALL_CDECL_OBJFIRST asCALL_CDECL_OBJFIRST
	#define AS_CALL_CDECL_OBJLAST asCALL_CDECL_OBJLAST
#endif

// RegisterObjectBehaviour(obj, asBEHAVE_DESTRUCT, decl, AS_DESTRUCTOR(c), AS_CALL_CDECL_OBJFIRST)
#ifdef AS_MAX_PORTABILITY
	#define AS_DESTRUCTOR(c) WRAP_DES(c) // RegisterObjectBehaviour(obj, asBEHAVE_DESTRUCT, decl, WRAP_DES(c), asCALL_GENERIC)
#else
	template <typename T>
	void ASNativeDestructor(T* ptr)
	{
		ptr->~T();
	}

	// RegisterObjectBehaviour(obj, asBEHAVE_DESTRUCT, decl, asFUNCTION(ASNativeDestructor<c>), asCALL_CDECL_OBJFIRST)
	#define AS_DESTRUCTOR(c) asFUNCTION(ASNativeDestructor<c>)
#endif

} // namespace Urho3D
