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

#include <cassert>
#include <fstream>
#include <iostream>
#include <regex>
#include <vector>

namespace ASBindingGenerator
{

static void RegisterObjectType(const ClassAnalyzer& classAnalyzer, ProcessedClass& inoutProcessedClass)
{
    string className = classAnalyzer.GetClassName();

    if (classAnalyzer.IsRefCounted() || Contains(classAnalyzer.GetComment(), "FAKE_REF"))
    {
        inoutProcessedClass.objectTypeRegistration_ = "engine->RegisterObjectType(\"" + className + "\", 0, asOBJ_REF);";
    }
    else // Value type
    {
        string flags = "asOBJ_VALUE | asGetTypeTraits<" + className + ">()";

        if (classAnalyzer.IsPod())
        {
            flags += " | asOBJ_POD";

            if (classAnalyzer.AllFloats())
                flags += " | asOBJ_APP_CLASS_ALLFLOATS";
            else if (classAnalyzer.AllInts())
                flags += " | asOBJ_APP_CLASS_ALLINTS";
        }

        inoutProcessedClass.objectTypeRegistration_ = "engine->RegisterObjectType(\"" + className + "\", sizeof(" + className + "), " + flags + ");";
    }
}

static void ProcessClass(const ClassAnalyzer& classAnalyzer)
{
    if (classAnalyzer.IsInternal())
        return;

    // TODO: Remove
    if (classAnalyzer.IsTemplate())
        return;

    // TODO: Remove
    if (classAnalyzer.IsAbstract() && !(classAnalyzer.IsRefCounted() || Contains(classAnalyzer.GetComment(), "FAKE_REF")))
        return;

    string header = classAnalyzer.GetHeaderFile();
    Result::AddHeader(header);

    if (IsIgnoredHeader(header))
        return;

    ProcessedClass processedClass;
    processedClass.name_ = classAnalyzer.GetClassName();
    processedClass.comment_ = classAnalyzer.GetLocation();
    processedClass.insideDefine_ = InsideDefine(header);

    string classComment = classAnalyzer.GetComment();

    if (Contains(classComment, "NO_BIND"))
    {
        processedClass.objectTypeRegistration_ = "// Not registered because have @nobind mark";
        Result::classes_.push_back(processedClass);
        return;
    }

    if (Contains(classComment, "MANUAL_BIND"))
    {
        processedClass.objectTypeRegistration_ = "// Not registered because have @manualbind mark";
        Result::classes_.push_back(processedClass);
        return;
    }

    RegisterObjectType(classAnalyzer, processedClass);
    Result::classes_.push_back(processedClass);
}

void ProcessAllClassesNew()
{
    for (auto element : SourceData::classesByID_)
    {
        xml_node compounddef = element.second;
        ClassAnalyzer analyzer(compounddef);
        ProcessClass(analyzer);
    }
}

} // namespace ASBindingGenerator
