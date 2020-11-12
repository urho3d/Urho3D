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

#include "ASResult.h"
#include "Tuning.h"
#include "Utils.h"

#include <fstream>

namespace ASBindingGenerator
{

ASGeneratedFile_WithRegistrationFunction::ASGeneratedFile_WithRegistrationFunction(const string& outputFilePath, const string& functionName)
{
    outputFilePath_ = outputFilePath;
    functionName_ = functionName;
}

// ============================================================================

void ASGeneratedFile_Enums::Save()
{
    ofstream out(outputFilePath_);

    out <<
        "// DO NOT EDIT. This file is generated\n"
        "\n"
        "// We need register all enums before registration of any members because members can use any enums\n"
        "\n"
        "#include \"../Precompiled.h\"\n"
        "#include \"../AngelScript/APITemplates.h\"\n"
        "#include \"../AngelScript/GeneratedIncludes.h\"\n"
        "\n";

    out <<
        "namespace Urho3D\n"
        "{\n"
        "\n"
        << glue_.str() <<
        "void " << functionName_ << "(asIScriptEngine* engine)\n"
        "{\n"
        << reg_.str() <<
        "}\n"
        "\n"
        "}\n";
}

// ============================================================================

void ASGeneratedFile_Classes::Save()
{
    ofstream out(outputFilePath_);

    out <<
        "// DO NOT EDIT. This file is generated\n"
        "\n"
        "// We need register all types before registration of any members because members can use any types\n"
        "\n"
        "#include \"../Precompiled.h\"\n"
        "#include \"../AngelScript/APITemplates.h\"\n"
        "#include \"../AngelScript/GeneratedIncludes.h\"\n"
        "\n";

    out <<
        "namespace Urho3D\n"
        "{\n"
        "\n"
        "void " << functionName_ << "(asIScriptEngine* engine)\n"
        "{\n"
        << reg_.str() <<
        "}\n"
        "\n"
        "}\n";
}

// ============================================================================

void ASGeneratedFile_Members_HighPriority::Save()
{
    ofstream out(outputFilePath_);

    out <<
        "// DO NOT EDIT. This file is generated\n"
        "\n"
        "// We need register default constructors before any members to allow using in Array<type>\n"
        "\n"
        "#include \"../Precompiled.h\"\n"
        "#include \"../AngelScript/APITemplates.h\"\n"
        "#include \"../AngelScript/GeneratedIncludes.h\"\n"
        "\n";

    out <<
        "namespace Urho3D\n"
        "{\n"
        "\n"
        "void FakeAddRef(void* ptr);\n"
        "void FakeReleaseRef(void* ptr);\n"
        "\n"
        << glue_.str() <<
        "void " << functionName_ << "(asIScriptEngine* engine)\n"
        "{\n"
        << reg_.str() <<
        "}\n"
        "\n"
        "}\n";
}

// ============================================================================

void ASGeneratedFile_Members::Save()
{
    ofstream out(outputFilePath_);

    out <<
        "// DO NOT EDIT. This file is generated\n"
        "\n"
        "#include \"../Precompiled.h\"\n"
        "#include \"../AngelScript/APITemplates.h\"\n"
        "#include \"../AngelScript/GeneratedIncludes.h\"\n"
        "#include \"../AngelScript/GeneratedGlue.h\"\n"
        "\n";

    out <<
        "#include \"../AngelScript/Manual.h\"\n"
        "\n"
        "namespace Urho3D\n"
        "{\n"
        "\n"
        "void FakeAddRef(void* ptr);\n"
        "void FakeReleaseRef(void* ptr);\n"
        "\n"
        << glue_.str() <<
        "void " << functionName_ << "(asIScriptEngine* engine)\n"
        "{\n"
        << reg_.str() <<
        "}\n"
        "\n"
        "}\n";
}

// ============================================================================

void ASGeneratedFile_GlobalVariables::Save()
{
    ofstream out(outputFilePath_);

    out <<
        "// DO NOT EDIT. This file is generated\n"
        "\n"
        "#include \"../Precompiled.h\"\n"
        "#include \"../AngelScript/APITemplates.h\"\n"
        "#include \"../AngelScript/GeneratedIncludes.h\"\n"
        "\n";

    out <<
        "// Some headers could re-define M_PI, ensure that it's undefined\n"
        "#undef M_PI\n"
        "\n";

    out <<
        "namespace Urho3D\n"
        "{\n"
        "\n"
        << glue_.str() <<
        "void " << functionName_ << "(asIScriptEngine* engine)\n"
        "{\n"
        << reg_.str() <<
        "}\n"
        "\n"
        "}\n";
}

// ============================================================================

void ASGeneratedFile_GlobalFunctions::Save()
{
    ofstream out(outputFilePath_);

    out <<
        "// DO NOT EDIT. This file is generated\n"
        "\n"
        "#include \"../Precompiled.h\"\n"
        "#include \"../AngelScript/APITemplates.h\"\n"
        "#include \"../AngelScript/GeneratedIncludes.h\"\n"
        "#include \"../AngelScript/GeneratedGlue.h\"\n"
        "\n";

    out <<
        "namespace Urho3D\n"
        "{\n"
        "\n"
        << glue_.str() <<
        "void " << functionName_ << "(asIScriptEngine* engine)\n"
        "{\n"
        << reg_.str() <<
        "}\n"
        "\n"
        "}\n";
}

// ============================================================================

ASGeneratedFile_Templates::ASGeneratedFile_Templates(const string& outputFilePath)
{
    outputFilePath_ = outputFilePath;
}

void ASGeneratedFile_Templates::Save()
{
    ofstream out(outputFilePath_);

    out <<
        "// DO NOT EDIT. This file is generated\n"
        "\n"
        "#pragma once\n"
        "\n"
        "#include \"../Precompiled.h\"\n"
        "#include \"../AngelScript/APITemplates.h\"\n"
        "#include \"../AngelScript/GeneratedIncludes.h\"\n"
        "\n";

    out <<
        "#include \"../AngelScript/Manual.h\"\n"
        "\n"
        "namespace Urho3D\n"
        "{\n"
        "\n"
        "void FakeAddRef(void* ptr);\n"
        "void FakeReleaseRef(void* ptr);\n"
        "\n"
        << glue_.str()
        << reg_.str() <<
        "}\n";
}

namespace ResultGlobalFunctions
{
    // Wrappers
    stringstream glue_;

    // Registration function body
    stringstream reg_;

    // Write result to file
    void Save(const string& outputBasePath)
    {
        ofstream ofs(outputBasePath + "/Source/Urho3D/AngelScript/GeneratedGlobalFunctions.cpp");

        ofs <<
            "// DO NOT EDIT. This file is generated\n"
            "\n"
            "#include \"../Precompiled.h\"\n"
            "#include \"../AngelScript/APITemplates.h\"\n"
            "#include \"../AngelScript/GeneratedIncludes.h\"\n"
            "\n";

        ofs <<
            "namespace Urho3D\n"
            "{\n"
            "\n"
            << glue_.str() <<
            "void ASRegisterGenerated_GlobalFunctions(asIScriptEngine* engine)\n"
            "{\n"
            << reg_.str() <<
            "}\n"
            "\n"
            "}\n";
    }
}

namespace ResultIncludes
{
    // List of all required header files
    static vector<string> headers_;

    // Discarded header files for statistic
    static vector<string> ignoredHeaders_;

    // Add header to lists if not added yet
    void AddHeader(const string& headerFile)
    {
        if (IsIgnoredHeader(headerFile))
        {
            if (!CONTAINS(ignoredHeaders_, headerFile))
                ignoredHeaders_.push_back(headerFile);
        }
        else
        {
            if (!CONTAINS(headers_, headerFile))
                headers_.push_back(headerFile);
        }
    }

    // Write result to file
    static void Save(const string& outputBasePath)
    {
        ofstream ofs(outputBasePath + "/Source/Urho3D/AngelScript/GeneratedIncludes.h");
        sort(headers_.begin(), headers_.end());
        sort(ignoredHeaders_.begin(), ignoredHeaders_.end());

        ofs <<
            "// DO NOT EDIT. This file is generated\n"
            "\n"
            "#pragma once\n"
            "\n";

        string openedDefine;
        bool isFirst = true;

        for (const string& header : headers_)
        {
            string insideDefine = InsideDefine(header);

            if (insideDefine != openedDefine)
            {
                if (!openedDefine.empty())
                {
                    ofs << "#endif\n";
                    openedDefine.clear();
                }

                if (!isFirst) // First include can be guarded. Avoid print \n before it
                    ofs << "\n";

                if (!insideDefine.empty())
                {
                    ofs << "#ifdef " << insideDefine << "\n";
                    openedDefine = insideDefine;
                }
            }

            ofs << "#include \"" << header << "\"\n";
            isFirst = false;
        }

        if (!openedDefine.empty())
        {
            ofs << "#endif\n";
            openedDefine.clear();
        }

        if (headers_.size() > 0)
            ofs << "\n";

        if (ignoredHeaders_.size() > 0)
            ofs << "// Ignored headers\n\n";

        isFirst = true;

        for (string header : ignoredHeaders_)
        {
            string insideDefine = InsideDefine(header);

            if (insideDefine != openedDefine)
            {
                if (!openedDefine.empty())
                {
                    ofs << "//#endif\n";
                    openedDefine.clear();
                }

                if (!isFirst) // First include can be guarded. Avoid print \n before it
                    ofs << "\n";

                if (!insideDefine.empty())
                {
                    ofs << "//#ifdef " << insideDefine << "\n";
                    openedDefine = insideDefine;
                }
            }

            ofs << "//#include \"" << header << "\"\n";
            isFirst = false;
        }

        if (!openedDefine.empty())
            ofs << "//#endif\n";
    }
}

// GeneratedMembers.cpp
namespace ResultMembers
{
    stringstream glue_;
    stringstream reg_;

    static void Save(const string& outputBasePath)
    {

    }
}

namespace ResultClasses
{
    // GeneratedGlue.h
    stringstream glueH_;

    // GeneratedGlue.cpp
    stringstream glueCpp_;

    // GeneratedTemplates.h
    stringstream templates_;

    stringstream members_;

    // Write result to files
    static void Save(const string& outputBasePath)
    {
        ofstream ofsH(outputBasePath + "/Source/Urho3D/AngelScript/GeneratedGlue.h");

        ofsH <<
            "// DO NOT EDIT. This file is generated\n"
            "\n"
            "#pragma once\n"
            "\n"
            "namespace Urho3D\n"
            "{\n"
            "\n"
            << glueH_.str() <<
            "\n"
            "}\n";

        ofstream ofsCpp(outputBasePath + "/Source/Urho3D/AngelScript/GeneratedGlue.cpp");

        ofsCpp <<
            "// DO NOT EDIT. This file is generated\n"
            "\n"
            "#include \"../Precompiled.h\"\n"
            "\n"
            "#include \"../AngelScript/APITemplates.h\"\n"
            "#include \"../AngelScript/GeneratedIncludes.h\"\n"
            "\n"
            "namespace Urho3D\n"
            "{\n"
            "\n"
            "void FakeAddRef(void* ptr);\n"
            "void FakeReleaseRef(void* ptr);\n"
            "\n"
            << glueCpp_.str() <<
            "}\n";

        ofstream ofsTemplates(outputBasePath + "/Source/Urho3D/AngelScript/GeneratedTemplates.h");

        ofsTemplates <<
            "// DO NOT EDIT. This file is generated\n"
            "\n"
            "#pragma once\n"
            "\n"
            "namespace Urho3D\n"
            "{\n"
            "\n"
            << templates_.str() <<
            "}\n";

        ofstream ofsMembers(outputBasePath + "/Source/Urho3D/AngelScript/GeneratedMembers.cpp");

        ofsMembers <<
            "// DO NOT EDIT. This file is generated\n"
            "\n"
            "#include \"../Precompiled.h\"\n"
            "\n"
            "#include \"../AngelScript/APITemplates.h\"\n"
            "#include \"../AngelScript/GeneratedIncludes.h\"\n"
            "#include \"../AngelScript/GeneratedTemplates.h\"\n"
            "\n"
            "namespace Urho3D\n"
            "{\n"
            "\n"
            "void FakeAddRef(void* ptr);\n"
            "void FakeReleaseRef(void* ptr);\n"
            "\n"
            << members_.str() <<
            "}\n";
    }
}

namespace Result
{
    
}

void SaveResult(const string& outputBasePath)
{
    ResultIncludes::Save(outputBasePath);
    ResultClasses::Save(outputBasePath);
    ResultGlobalFunctions::Save(outputBasePath);
    //Result::SaveClasses(outputBasePath);
    /*ResultEnums::Save(outputBasePath);
    Result::Save(outputBasePath);
    ResultClasses::Save(outputBasePath);*/
}

}
