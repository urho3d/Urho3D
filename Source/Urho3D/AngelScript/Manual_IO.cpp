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
#include "../AngelScript/APITemplates.h"

#include "../AngelScript/Manual_IO.h"

namespace Urho3D
{

// This function is called before ASRegisterGenerated()
void ASRegisterManualFirst_IO(asIScriptEngine* engine)
{
}

// ========================================================================================

// unsigned char* VectorBuffer::GetModifiableData() | File: ../IO/VectorBuffer.h
static unsigned char* VectorBufferAt(unsigned index, VectorBuffer* ptr)
{
    if (index >= ptr->GetSize())
    {
        asGetActiveContext()->SetException("Index out of bounds");
        return nullptr;
    }

    return ptr->GetModifiableData() + index;
}

static void RegisterVectorBuffer(asIScriptEngine* engine)
{
    // unsigned char* VectorBuffer::GetModifiableData() | File: ../IO/VectorBuffer.h
    engine->RegisterObjectMethod("VectorBuffer", "uint8 &opIndex(uint)", AS_FUNCTION_OBJLAST(VectorBufferAt), AS_CALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("VectorBuffer", "const uint8 &opIndex(uint) const", AS_FUNCTION_OBJLAST(VectorBufferAt), AS_CALL_CDECL_OBJLAST);
}

// ========================================================================================

#ifdef URHO3D_LOGGING

static void Print(const String& value, bool error)
{
    Log::WriteRaw(value + "\n", error);
}

static void Print(int value, bool error)
{
    Log::WriteRaw(String(value) + "\n", error);
}

static void Print(long long value, bool error)
{
    Log::WriteRaw(String(value) + "\n", error);
}

static void Print(unsigned value, bool error)
{
    Log::WriteRaw(String(value) + "\n", error);
}

static void Print(unsigned long long value, bool error)
{
    Log::WriteRaw(String(value) + "\n", error);
}

static void Print(float value, bool error)
{
    Log::WriteRaw(String(value) + "\n", error);
}

static void Print(bool value, bool error)
{
    Log::WriteRaw(String(value) + "\n", error);
}

static void Print(const Variant& value, bool error)
{
    Log::WriteRaw(value.ToString() + "\n", error);
}

static void PrintCallStack(bool error)
{
    asIScriptContext* context = asGetActiveContext();
    if (context)
        Log::WriteRaw(Script::GetCallStack(context), error);
}

static void LogWrite(const String& str, bool error, Log* ptr)
{
    Log::WriteRaw(str + "\n", error);
}

static void LogTrace(const String& str, Log* ptr)
{
    Log::Write(LOG_TRACE, str);
}

static void LogDebug(const String& str, Log* ptr)
{
    Log::Write(LOG_DEBUG, str);
}

static void LogInfo(const String& str, Log* ptr)
{
    Log::Write(LOG_INFO, str);
}

static void LogWarning(const String& str, Log* ptr)
{
    Log::Write(LOG_WARNING, str);
}

static void LogError(const String& str, Log* ptr)
{
    Log::Write(LOG_ERROR, str);
}

#else

static void Print(const String& value, bool error) { }
static void Print(int value, bool error) { }
static void Print(long long value, bool error) { }
static void Print(unsigned value, bool error) { }
static void Print(unsigned long long value, bool error) { }
static void Print(float value, bool error) { }
static void Print(bool value, bool error) { }
static void Print(const Variant& value, bool error) { }
static void PrintCallStack(bool error) { }
static void LogWrite(const String& str, bool error, Log* ptr) { }
static void LogTrace(const String& str, Log* ptr) { }
static void LogDebug(const String& str, Log* ptr) { }
static void LogInfo(const String& str, Log* ptr) { }
static void LogWarning(const String& str, Log* ptr) { }
static void LogError(const String& str, Log* ptr) { }

#endif

static void RegisterLog(asIScriptEngine* engine)
{
    engine->RegisterObjectMethod("Log", "void Write(const String&in, bool error = false)", AS_FUNCTION_OBJLAST(LogWrite), AS_CALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("Log", "void Trace(const String&in)", AS_FUNCTION_OBJLAST(LogTrace), AS_CALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("Log", "void Debug(const String&in)", AS_FUNCTION_OBJLAST(LogDebug), AS_CALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("Log", "void Info(const String&in)", AS_FUNCTION_OBJLAST(LogInfo), AS_CALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("Log", "void Warning(const String&in)", AS_FUNCTION_OBJLAST(LogWarning), AS_CALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("Log", "void Error(const String&in)", AS_FUNCTION_OBJLAST(LogError), AS_CALL_CDECL_OBJLAST);
	
	// Register also Print() functions for convenience
    engine->RegisterGlobalFunction("void Print(const String&in, bool error = false)", AS_FUNCTIONPR(Print, (const String&, bool), void), AS_CALL_CDECL);
    engine->RegisterGlobalFunction("void Print(int, bool error = false)", AS_FUNCTIONPR(Print, (int, bool), void), AS_CALL_CDECL);
    engine->RegisterGlobalFunction("void Print(int64, bool error = false)", AS_FUNCTIONPR(Print, (long long, bool), void), AS_CALL_CDECL);
    engine->RegisterGlobalFunction("void Print(uint, bool error = false)", AS_FUNCTIONPR(Print, (unsigned, bool), void), AS_CALL_CDECL);
    engine->RegisterGlobalFunction("void Print(uint64, bool error = false)", AS_FUNCTIONPR(Print, (unsigned long long, bool), void), AS_CALL_CDECL);
    engine->RegisterGlobalFunction("void Print(float, bool error = false)", AS_FUNCTIONPR(Print, (float, bool), void), AS_CALL_CDECL);
    engine->RegisterGlobalFunction("void Print(bool, bool error = false)", AS_FUNCTIONPR(Print, (bool, bool), void), AS_CALL_CDECL);
    engine->RegisterGlobalFunction("void Print(const Variant&in, bool error = false)", AS_FUNCTIONPR(Print, (const Variant&, bool), void), AS_CALL_CDECL);
    engine->RegisterGlobalFunction("void PrintCallStack(bool error = false)", AS_FUNCTION(PrintCallStack), AS_CALL_CDECL);
}

// ========================================================================================

// template<class T> T * Object::GetSubsystem() const | File: ../Core/Object.h
static FileSystem* GetFileSystem()
{
    return GetScriptContext()->GetSubsystem<FileSystem>();
}

// template<class T> T * Object::GetSubsystem() const | File: ../Core/Object.h
static Log* GetLog()
{
    return GetScriptContext()->GetSubsystem<Log>();
}

// This function is called after ASRegisterGenerated()
void ASRegisterManualLast_IO(asIScriptEngine* engine)
{
    RegisterVectorBuffer(engine);
    RegisterLog(engine);

    // template<class T> T * Object::GetSubsystem() const | File: ../Core/Object.h
    engine->RegisterGlobalFunction("FileSystem@+ get_fileSystem()", AS_FUNCTION(GetFileSystem), AS_CALL_CDECL);
    // template<class T> T * Object::GetSubsystem() const | File: ../Core/Object.h
    engine->RegisterGlobalFunction("Log@+ get_log()", AS_FUNCTION(GetLog), AS_CALL_CDECL);
}

// ========================================================================================

// virtual unsigned Deserializer::Read(void *dest, unsigned size)=0 | File: ../IO/Deserializer.h
CScriptArray* DeserializerRead(unsigned size, Deserializer* ptr)
{
    PODVector<unsigned char> vector(size);
    unsigned bytesRead = size ? ptr->Read(&vector[0], size) : 0;
    vector.Resize(bytesRead);
    return VectorToArray(vector, "Array<uint8>");
}

// VectorBuffer(Deserializer& source, unsigned size) | File: .. / IO / VectorBuffer.h
VectorBuffer DeserializerReadVectorBuffer(unsigned size, Deserializer* ptr)
{
    return VectorBuffer(*ptr, size);
}

// ========================================================================================

// virtual unsigned Serializer::Write(const void *data, unsigned size)=0 | File: ../IO/Serializer.h
unsigned SerializerWrite(CScriptArray* arr, Serializer* ptr)
{
    unsigned bytesToWrite = arr->GetSize();
    return bytesToWrite ? ptr->Write(arr->At(0), bytesToWrite) : 0;
}

// virtual unsigned Serializer::Write(const void *data, unsigned size)=0 | File: ../IO/Serializer.h
bool SerializerWriteVectorBuffer(VectorBuffer* src, Serializer* ptr)
{
    return ptr->Write(src->GetData(), src->GetSize()) == src->GetSize();
}

// ========================================================================================

// void FileSystem::ScanDir(Vector< String > &result, const String &pathName, const String &filter, unsigned flags, bool recursive) const | File: ../IO/FileSystem.h
CScriptArray* FileSystemScanDir(const String& pathName, const String& filter, unsigned flags, bool recursive, FileSystem* ptr)
{
    Vector<String> result;
    ptr->ScanDir(result, pathName, filter, flags, recursive);
    return VectorToArray<String>(result, "Array<String>");
}

}
