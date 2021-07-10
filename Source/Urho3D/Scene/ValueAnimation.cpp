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
#include "../Core/StringUtils.h"
#include "../IO/Deserializer.h"
#include "../IO/Log.h"
#include "../IO/Serializer.h"
#include "../Resource/XMLFile.h"
#include "../Resource/JSONFile.h"
#include "../Scene/Animatable.h"
#include "../Scene/ObjectAnimation.h"
#include "../Scene/ValueAnimation.h"

#include "../DebugNew.h"

namespace Urho3D
{

const char* interpMethodNames[] =
{
    "None",
    "Linear",
    "Spline",
    nullptr
};

ValueAnimation::ValueAnimation(Context* context) :
    Resource(context),
    owner_(nullptr),
    interpolationMethod_(IM_LINEAR),
    splineTension_(0.5f),
    valueType_(VAR_NONE),
    interpolatable_(false),
    beginTime_(M_INFINITY),
    endTime_(-M_INFINITY),
    splineTangentsDirty_(false)
{
}

ValueAnimation::~ValueAnimation() = default;

void ValueAnimation::RegisterObject(Context* context)
{
    context->RegisterFactory<ValueAnimation>();
}

bool ValueAnimation::BeginLoad(Deserializer& source)
{
    XMLFile xmlFile(context_);
    if (!xmlFile.Load(source))
        return false;

    return LoadXML(xmlFile.GetRoot());
}

bool ValueAnimation::Save(Serializer& dest) const
{
    XMLFile xmlFile(context_);

    XMLElement rootElem = xmlFile.CreateRoot("valueanimation");
    if (!SaveXML(rootElem))
        return false;

    return xmlFile.Save(dest);
}

bool ValueAnimation::LoadXML(const XMLElement& source)
{
    valueType_ = VAR_NONE;
    eventFrames_.Clear();

    String interpMethodString = source.GetAttribute("interpolationmethod");
    auto method = (InterpMethod)GetStringListIndex(interpMethodString.CString(), interpMethodNames, IM_LINEAR);

    SetInterpolationMethod(method);
    if (interpolationMethod_ == IM_SPLINE)
        splineTension_ = source.GetFloat("splinetension");

    XMLElement keyFrameElem = source.GetChild("keyframe");
    while (keyFrameElem)
    {
        float time = keyFrameElem.GetFloat("time");
        Variant value = keyFrameElem.GetVariant();
        SetKeyFrame(time, value);

        keyFrameElem = keyFrameElem.GetNext("keyframe");
    }

    XMLElement eventFrameElem = source.GetChild("eventframe");
    while (eventFrameElem)
    {
        float time = eventFrameElem.GetFloat("time");
        unsigned eventType = 0;
        if (eventFrameElem.HasAttribute("eventname"))
            eventType = StringHash(eventFrameElem.GetAttributeCString("eventname")).Value();
        else
            eventType = eventFrameElem.GetUInt("eventtype");
        VariantMap eventData = eventFrameElem.GetChild("eventdata").GetVariantMap();

        SetEventFrame(time, StringHash(eventType), eventData);
        eventFrameElem = eventFrameElem.GetNext("eventframe");
    }

    return true;
}

bool ValueAnimation::SaveXML(XMLElement& dest) const
{
    dest.SetAttribute("interpolationmethod", interpMethodNames[interpolationMethod_]);
    if (interpolationMethod_ == IM_SPLINE)
        dest.SetFloat("splinetension", splineTension_);

    for (unsigned i = 0; i < keyFrames_.Size(); ++i)
    {
        const VAnimKeyFrame& keyFrame = keyFrames_[i];
        XMLElement keyFrameEleme = dest.CreateChild("keyframe");
        keyFrameEleme.SetFloat("time", keyFrame.time_);
        keyFrameEleme.SetVariant(keyFrame.value_);
    }

    for (unsigned i = 0; i < eventFrames_.Size(); ++i)
    {
        const VAnimEventFrame& eventFrame = eventFrames_[i];
        XMLElement eventFrameElem = dest.CreateChild("eventframe");
        eventFrameElem.SetFloat("time", eventFrame.time_);
        if (StringHash::GetGlobalStringHashRegister() && !eventFrame.eventType_.Reverse().Empty())
            eventFrameElem.SetAttribute("eventname",eventFrame.eventType_.Reverse());
        else
            eventFrameElem.SetUInt("eventtype", eventFrame.eventType_.Value());
        eventFrameElem.CreateChild("eventdata").SetVariantMap(eventFrame.eventData_);
    }

    return true;
}

bool ValueAnimation::LoadJSON(const JSONValue& source)
{
    valueType_ = VAR_NONE;
    eventFrames_.Clear();

    String interpMethodString = source.Get("interpolationmethod").GetString();
    auto method = (InterpMethod)GetStringListIndex(interpMethodString.CString(), interpMethodNames, IM_LINEAR);

    SetInterpolationMethod(method);
    if (interpolationMethod_ == IM_SPLINE)
        splineTension_ = source.Get("splinetension").GetFloat();

    // Load keyframes
    JSONArray keyFramesArray = source.Get("keyframes").GetArray();
    for (unsigned i = 0; i < keyFramesArray.Size(); i++)
    {
        const JSONValue& val = keyFramesArray[i];
        float time = val.Get("time").GetFloat();
        Variant value = val.Get("value").GetVariant();
        SetKeyFrame(time, value);
    }

    // Load event frames
    JSONArray eventFramesArray = source.Get("eventframes").GetArray();
    for (unsigned i = 0; i < eventFramesArray.Size(); i++)
    {
        const JSONValue& eventFrameVal = eventFramesArray[i];
        float time = eventFrameVal.Get("time").GetFloat();
        unsigned eventType = 0;
        if (eventFrameVal.Contains("eventname"))
            eventType = StringHash(eventFrameVal.GetCString()).Value();
        else
            eventType = eventFrameVal.Get("eventtype").GetUInt();
        VariantMap eventData = eventFrameVal.Get("eventdata").GetVariantMap();
        SetEventFrame(time, StringHash(eventType), eventData);
    }

    return true;
}

bool ValueAnimation::SaveJSON(JSONValue& dest) const
{
    dest.Set("interpolationmethod", interpMethodNames[interpolationMethod_]);
    if (interpolationMethod_ == IM_SPLINE)
        dest.Set("splinetension", (float) splineTension_);

    JSONArray keyFramesArray;
    keyFramesArray.Reserve(keyFrames_.Size());
    for (unsigned i = 0; i < keyFrames_.Size(); ++i)
    {
        const VAnimKeyFrame& keyFrame = keyFrames_[i];
        JSONValue keyFrameVal;
        keyFrameVal.Set("time", keyFrame.time_);
        JSONValue valueVal;
        valueVal.SetVariant(keyFrame.value_,GetContext());
        keyFrameVal.Set("value", valueVal);
        keyFramesArray.Push(keyFrameVal);
    }
    dest.Set("keyframes", keyFramesArray);

    JSONArray eventFramesArray;
    eventFramesArray.Reserve(eventFrames_.Size());
    for (unsigned i = 0; i < eventFrames_.Size(); ++i)
    {
        const VAnimEventFrame& eventFrame = eventFrames_[i];
        JSONValue eventFrameVal;
        eventFrameVal.Set("time", eventFrame.time_);
        if (StringHash::GetGlobalStringHashRegister() && !eventFrame.eventType_.Reverse().Empty())
            eventFrameVal.Set("eventname",eventFrame.eventType_.Reverse());
        else
            eventFrameVal.Set("eventtype", eventFrame.eventType_.Value());
        JSONValue eventDataVal;
        eventDataVal.SetVariantMap(eventFrame.eventData_,GetContext());
        eventFrameVal.Set("eventdata", eventDataVal);

        eventFramesArray.Push(eventFrameVal);
    }
    dest.Set("eventframes", eventFramesArray);

    return true;
}

void ValueAnimation::SetValueType(VariantType valueType)
{
    if (valueType == valueType_)
        return;

    valueType_ = valueType;
    interpolatable_ =
        (valueType_ == VAR_FLOAT) || (valueType_ == VAR_VECTOR2) || (valueType_ == VAR_VECTOR3) || (valueType_ == VAR_VECTOR4) ||
        (valueType_ == VAR_QUATERNION) || (valueType_ == VAR_COLOR);

    if ((valueType_ == VAR_INTRECT) || (valueType_ == VAR_INTVECTOR2) || (valueType_ == VAR_INTVECTOR3))
    {
        interpolatable_ = true;
        // Force linear interpolation for IntRect, IntVector2 and IntVector3
        if (interpolationMethod_ == IM_SPLINE)
            interpolationMethod_ = IM_LINEAR;
    }

    keyFrames_.Clear();
    eventFrames_.Clear();
    beginTime_ = M_INFINITY;
    endTime_ = -M_INFINITY;
}

void ValueAnimation::SetOwner(void* owner)
{
    owner_ = owner;
}

void ValueAnimation::SetInterpolationMethod(InterpMethod method)
{
    if (method == interpolationMethod_)
        return;

    // Force linear interpolation for IntRect, IntVector2 and IntVector3
    if (method == IM_SPLINE && (valueType_ == VAR_INTRECT || valueType_ == VAR_INTVECTOR2 || valueType_ == VAR_INTVECTOR3))
        method = IM_LINEAR;

    interpolationMethod_ = method;
    splineTangentsDirty_ = true;
}

void ValueAnimation::SetSplineTension(float tension)
{
    splineTension_ = tension;
    splineTangentsDirty_ = true;
}

bool ValueAnimation::SetKeyFrame(float time, const Variant& value)
{
    if (valueType_ == VAR_NONE)
        SetValueType(value.GetType());
    else if (value.GetType() != valueType_)
        return false;

    VAnimKeyFrame keyFrame;
    keyFrame.time_ = time;
    keyFrame.value_ = value;

    if (keyFrames_.Empty() || time > keyFrames_.Back().time_)
        keyFrames_.Push(keyFrame);
    else
    {
        for (unsigned i = 0; i < keyFrames_.Size(); ++i)
        {
            // Guard against interpolation error caused by division by error due to 0 delta time between two key frames
            if (time == keyFrames_[i].time_)
                return false;
            if (time < keyFrames_[i].time_)
            {
                keyFrames_.Insert(i, keyFrame);
                break;
            }
        }
    }

    beginTime_ = Min(time, beginTime_);
    endTime_ = Max(time, endTime_);
    splineTangentsDirty_ = true;

    return true;
}

void ValueAnimation::SetEventFrame(float time, const StringHash& eventType, const VariantMap& eventData)
{
    VAnimEventFrame eventFrame;
    eventFrame.time_ = time;
    eventFrame.eventType_ = eventType;
    eventFrame.eventData_ = eventData;

    if (eventFrames_.Empty() || time >= eventFrames_.Back().time_)
        eventFrames_.Push(eventFrame);
    else
    {
        for (unsigned i = 0; i < eventFrames_.Size(); ++i)
        {
            if (time < eventFrames_[i].time_)
            {
                eventFrames_.Insert(i, eventFrame);
                break;
            }
        }
    }

    beginTime_ = Min(time, beginTime_);
    endTime_ = Max(time, endTime_);
}

bool ValueAnimation::IsValid() const
{
    return (interpolationMethod_ == IM_NONE) ||
           (interpolationMethod_ == IM_LINEAR && keyFrames_.Size() > 1) ||
           (interpolationMethod_ == IM_SPLINE && keyFrames_.Size() > 2);
}

Variant ValueAnimation::GetAnimationValue(float scaledTime) const
{
    unsigned index = 1;
    for (; index < keyFrames_.Size(); ++index)
    {
        if (scaledTime < keyFrames_[index].time_)
            break;
    }

    if (index >= keyFrames_.Size() || !interpolatable_ || interpolationMethod_ == IM_NONE)
        return keyFrames_[index - 1].value_;
    else
    {
        if (interpolationMethod_ == IM_LINEAR)
            return LinearInterpolation(index - 1, index, scaledTime);
        else
            return SplineInterpolation(index - 1, index, scaledTime);
    }
}

void ValueAnimation::GetEventFrames(float beginTime, float endTime, PODVector<const VAnimEventFrame*>& eventFrames) const
{
    for (unsigned i = 0; i < eventFrames_.Size(); ++i)
    {
        const VAnimEventFrame& eventFrame = eventFrames_[i];
        if (eventFrame.time_ > endTime)
            break;

        if (eventFrame.time_ >= beginTime)
            eventFrames.Push(&eventFrame);
    }
}

Variant ValueAnimation::LinearInterpolation(unsigned index1, unsigned index2, float scaledTime) const
{
    const VAnimKeyFrame& keyFrame1 = keyFrames_[index1];
    const VAnimKeyFrame& keyFrame2 = keyFrames_[index2];

    float t = (scaledTime - keyFrame1.time_) / (keyFrame2.time_ - keyFrame1.time_);
    const Variant& value1 = keyFrame1.value_;
    const Variant& value2 = keyFrame2.value_;

    switch (valueType_)
    {
    case VAR_FLOAT:
        return Lerp(value1.GetFloat(), value2.GetFloat(), t);

    case VAR_VECTOR2:
        return value1.GetVector2().Lerp(value2.GetVector2(), t);

    case VAR_VECTOR3:
        return value1.GetVector3().Lerp(value2.GetVector3(), t);

    case VAR_VECTOR4:
        return value1.GetVector4().Lerp(value2.GetVector4(), t);

    case VAR_QUATERNION:
        return value1.GetQuaternion().Slerp(value2.GetQuaternion(), t);

    case VAR_COLOR:
        return value1.GetColor().Lerp(value2.GetColor(), t);

    case VAR_INTRECT:
        {
            float s = 1.0f - t;
            const IntRect& r1 = value1.GetIntRect();
            const IntRect& r2 = value2.GetIntRect();
            return IntRect((int)(r1.left_ * s + r2.left_ * t), (int)(r1.top_ * s + r2.top_ * t), (int)(r1.right_ * s + r2.right_ * t),
                (int)(r1.bottom_ * s + r2.bottom_ * t));
        }

    case VAR_INTVECTOR2:
        {
            float s = 1.0f - t;
            const IntVector2& v1 = value1.GetIntVector2();
            const IntVector2& v2 = value2.GetIntVector2();
            return IntVector2((int)(v1.x_ * s + v2.x_ * t), (int)(v1.y_ * s + v2.y_ * t));
        }

    case VAR_INTVECTOR3:
        {
            float s = 1.0f - t;
            const IntVector3& v1 = value1.GetIntVector3();
            const IntVector3& v2 = value2.GetIntVector3();
            return IntVector3((int)(v1.x_ * s + v2.x_ * t), (int)(v1.y_ * s + v2.y_ * t), (int)(v1.z_ * s + v2.z_ * t));
        }

    case VAR_DOUBLE:
        return value1.GetDouble() * (1.0f - t) + value2.GetDouble() * t;

    default:
        URHO3D_LOGERROR("Invalid value type for linear interpolation");
        return Variant::EMPTY;
    }
}

Variant ValueAnimation::SplineInterpolation(unsigned index1, unsigned index2, float scaledTime) const
{
    if (splineTangentsDirty_)
        UpdateSplineTangents();

    const VAnimKeyFrame& keyFrame1 = keyFrames_[index1];
    const VAnimKeyFrame& keyFrame2 = keyFrames_[index2];

    float t = (scaledTime - keyFrame1.time_) / (keyFrame2.time_ - keyFrame1.time_);

    float tt = t * t;
    float ttt = t * tt;

    float h1 = 2.0f * ttt - 3.0f * tt + 1.0f;
    float h2 = -2.0f * ttt + 3.0f * tt;
    float h3 = ttt - 2.0f * tt + t;
    float h4 = ttt - tt;

    const Variant& v1 = keyFrame1.value_;
    const Variant& v2 = keyFrame2.value_;
    const Variant& t1 = splineTangents_[index1];
    const Variant& t2 = splineTangents_[index2];

    switch (valueType_)
    {
    case VAR_FLOAT:
        return v1.GetFloat() * h1 + v2.GetFloat() * h2 + t1.GetFloat() * h3 + t2.GetFloat() * h4;

    case VAR_VECTOR2:
        return v1.GetVector2() * h1 + v2.GetVector2() * h2 + t1.GetVector2() * h3 + t2.GetVector2() * h4;

    case VAR_VECTOR3:
        return v1.GetVector3() * h1 + v2.GetVector3() * h2 + t1.GetVector3() * h3 + t2.GetVector3() * h4;

    case VAR_VECTOR4:
        return v1.GetVector4() * h1 + v2.GetVector4() * h2 + t1.GetVector4() * h3 + t2.GetVector4() * h4;

    case VAR_QUATERNION:
        return v1.GetQuaternion() * h1 + v2.GetQuaternion() * h2 + t1.GetQuaternion() * h3 + t2.GetQuaternion() * h4;

    case VAR_COLOR:
        return v1.GetColor() * h1 + v2.GetColor() * h2 + t1.GetColor() * h3 + t2.GetColor() * h4;

    case VAR_DOUBLE:
        return v1.GetDouble() * h1 + v2.GetDouble() * h2 + t1.GetDouble() * h3 + t2.GetDouble() * h4;

    default:
        URHO3D_LOGERROR("Invalid value type for spline interpolation");
        return Variant::EMPTY;
    }
}

void ValueAnimation::UpdateSplineTangents() const
{
    splineTangents_.Clear();

    if (!IsValid())
        return;

    unsigned size = keyFrames_.Size();
    splineTangents_.Resize(size);

    for (unsigned i = 1; i < size - 1; ++i)
        splineTangents_[i] = SubstractAndMultiply(keyFrames_[i + 1].value_, keyFrames_[i - 1].value_, splineTension_);

    // If spline is not closed, make end point's tangent zero
    if (keyFrames_[0].value_ != keyFrames_[size - 1].value_)
        splineTangents_[0] = splineTangents_[size - 1] =
            SubstractAndMultiply(keyFrames_[0].value_, keyFrames_[0].value_, splineTension_);
    else
        splineTangents_[0] = splineTangents_[size - 1] =
            SubstractAndMultiply(keyFrames_[1].value_, keyFrames_[size - 2].value_, splineTension_);

    splineTangentsDirty_ = false;
}

Variant ValueAnimation::SubstractAndMultiply(const Variant& value1, const Variant& value2, float t) const
{
    switch (valueType_)
    {
    case VAR_FLOAT:
        return (value1.GetFloat() - value2.GetFloat()) * t;

    case VAR_VECTOR2:
        return (value1.GetVector2() - value2.GetVector2()) * t;

    case VAR_VECTOR3:
        return (value1.GetVector3() - value2.GetVector3()) * t;

    case VAR_VECTOR4:
        return (value1.GetVector4() - value2.GetVector4()) * t;

    case VAR_QUATERNION:
        return (value1.GetQuaternion() - value2.GetQuaternion()) * t;

    case VAR_COLOR:
        return (value1.GetColor() - value2.GetColor()) * t;

    case VAR_DOUBLE:
        return (value1.GetDouble() - value2.GetDouble()) * t;

    default:
        URHO3D_LOGERROR("Invalid value type for spline interpolation's subtract and multiply operation");
        return Variant::EMPTY;
    }
}

}
