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
#include "ASUtils.h"
#include "Tuning.h"
#include "Utils.h"
#include "XmlAnalyzer.h"
#include "XmlSourceData.h"

#include <memory>

namespace ASBindingGenerator
{

static unique_ptr<ASGeneratedFile_GlobalVariables> _result;

static void ProcessGlobalVariable(GlobalVariableAnalyzer varAnalyzer)
{
    string header = varAnalyzer.GetHeaderFile();
   
    if (IsIgnoredHeader(header))
    {
        _result->AddIgnoredHeader(header);
        return;
    }

    string insideDefine = InsideDefine(header);

    if (varAnalyzer.IsArray())
    {
        if (!insideDefine.empty())
            _result->reg_ << "#ifdef " << insideDefine << "\n";

        _result->reg_ <<
            "    // " << varAnalyzer.GetLocation() << "\n"
            "    // Not registered because array\n";

        if (!insideDefine.empty())
            _result->reg_ << "#endif\n";

        return;
    }

    _result->AddHeader(header);

    if (!insideDefine.empty())
        _result->reg_ << "#ifdef " << insideDefine << "\n";

    TypeAnalyzer typeAnalyzer = varAnalyzer.GetType();

    string asType = CppFundamentalTypeToAS(typeAnalyzer.GetName());

    if (typeAnalyzer.IsConst())
        asType = "const " + asType;

    string varName = varAnalyzer.GetName();

    _result->reg_ <<
        "    // " << varAnalyzer.GetLocation() << "\n"
        "    engine->RegisterGlobalProperty(\"" << asType << " " << varName << "\", (void*)&" << varName << ");\n";

    if (!insideDefine.empty())
        _result->reg_ << "#endif\n";
}

void ProcessAllGlobalVariables(const string& outputBasePath)
{
    string outputPath = outputBasePath + "/Source/Urho3D/AngelScript/Generated_GlobalVariables.cpp";
    _result.reset(new ASGeneratedFile_GlobalVariables(outputPath, "ASRegisterGenerated_GlobalVariables"));

    NamespaceAnalyzer namespaceAnalyzer(SourceData::namespaceUrho3D_);
    vector<GlobalVariableAnalyzer> globalVariableAnalyzers = namespaceAnalyzer.GetVariables();

    for (const GlobalVariableAnalyzer& globalVariableAnalyzer : globalVariableAnalyzers)
        ProcessGlobalVariable(globalVariableAnalyzer);

    _result->Save();
}

} // namespace ASBindingGenerator
