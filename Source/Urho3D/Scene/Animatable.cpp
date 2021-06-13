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
#include "../IO/Log.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/JSONValue.h"
#include "../Resource/XMLElement.h"
#include "../Scene/Animatable.h"
#include "../Scene/ObjectAnimation.h"
#include "../Scene/SceneEvents.h"
#include "../Scene/ValueAnimation.h"

#include "../DebugNew.h"

namespace Urho3D
{

extern const char* wrapModeNames[];

AttributeAnimationInfo::AttributeAnimationInfo(Animatable* animatable, const AttributeInfo& attributeInfo,
    ValueAnimation* attributeAnimation, WrapMode wrapMode, float speed) :
    ValueAnimationInfo(animatable, attributeAnimation, wrapMode, speed),
    attributeInfo_(attributeInfo)
{
}

AttributeAnimationInfo::AttributeAnimationInfo(const AttributeAnimationInfo& other) = default;

AttributeAnimationInfo::~AttributeAnimationInfo() = default;

void AttributeAnimationInfo::ApplyValue(const Variant& newValue)
{
    auto* animatable = static_cast<Animatable*>(target_.Get());
    if (animatable)
    {
        animatable->OnSetAttribute(attributeInfo_, newValue);
        animatable->ApplyAttributes();
    }
}

Animatable::Animatable(Context* context) :
    Serializable(context),
    animationEnabled_(true)
{
}

Animatable::~Animatable() = default;

void Animatable::RegisterObject(Context* context)
{
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Object Animation", GetObjectAnimationAttr, SetObjectAnimationAttr, ResourceRef,
        ResourceRef(ObjectAnimation::GetTypeStatic()), AM_DEFAULT);
}

bool Animatable::LoadXML(const XMLElement& source)
{
    if (!Serializable::LoadXML(source))
        return false;

    SetObjectAnimation(nullptr);
    attributeAnimationInfos_.Clear();

    XMLElement elem = source.GetChild("objectanimation");
    if (elem)
    {
        SharedPtr<ObjectAnimation> objectAnimation;
        ResourceRef rref = elem.GetResourceRef();
        if (rref.type_ == ObjectAnimation::GetTypeStatic())
        {
            ResourceCache* cache = GetSubsystem<ResourceCache>();
            objectAnimation = cache->GetResource<ObjectAnimation>(rref.name_);
        }
        else
        {
            objectAnimation = new ObjectAnimation(context_);
            if (!objectAnimation->LoadXML(elem))
                return false;
        }
        if (objectAnimation)
        {
            XMLElement animTimes = elem.GetChild("animationtime");
            while (animTimes)
            {
                String name = animTimes.GetAttribute("name");
                ValueAnimationInfo* animInfo = objectAnimation->GetAttributeAnimationInfo(name);
                if (animInfo)
                    animInfo->SetTime(animTimes.GetFloat("time"));
                animTimes = animTimes.GetNext("animationtime");
            }
        }
        SetObjectAnimationOnLoad(objectAnimation);
    }

    elem = source.GetChild("attributeanimation");
    while (elem)
    {
        String name = elem.GetAttribute("name");
        SharedPtr<ValueAnimation> attributeAnimation;
        ResourceRef rr = elem.GetResourceRef();
        if (rr.type_ == ValueAnimation::GetTypeStatic())
        {
            ResourceCache* cache = GetSubsystem<ResourceCache>();
            attributeAnimation = cache->GetResource<ValueAnimation>(rr.name_);
        }
        else
        {
            attributeAnimation = new ValueAnimation(context_);
            if (!attributeAnimation->LoadXML(elem))
                return false;
        }

        String wrapModeString = source.GetAttribute("wrapmode");
        WrapMode wrapMode = WM_LOOP;
        for (int i = 0; i <= WM_CLAMP; ++i)
        {
            if (wrapModeString == wrapModeNames[i])
            {
                wrapMode = (WrapMode)i;
                break;
            }
        }

        float speed = elem.GetFloat("speed");
        SetAttributeAnimation(name, attributeAnimation, wrapMode, speed);
        SetAttributeAnimationTime(name, elem.GetFloat("time"));
        elem = elem.GetNext("attributeanimation");
    }

    return true;
}

bool Animatable::LoadJSON(const JSONValue& source)
{
    if (!Serializable::LoadJSON(source))
        return false;

    SetObjectAnimation(nullptr);
    attributeAnimationInfos_.Clear();

    JSONValue value = source.Get("objectanimation");
    if (!value.IsNull())
    {
        SharedPtr<ObjectAnimation> objectAnimation;
        if (value.Contains("resource"))
        {
            Variant var = value["resource"].GetVariantValue(VAR_RESOURCEREF);
            ResourceRef rref = var.GetResourceRef();
            if (rref.type_ == ObjectAnimation::GetTypeStatic())
            {
                ResourceCache* cache = GetSubsystem<ResourceCache>();
                objectAnimation = cache->GetResource<ObjectAnimation>(rref.name_);
            }
        }
        else
        {
            objectAnimation = new ObjectAnimation(context_);
            if (!objectAnimation->LoadJSON(value))
                return false;
        }
        if (objectAnimation)
        {
            JSONValue animTimes = value["animationtime"];
            if (animTimes.GetValueType() == JSON_ARRAY)
            {
                const JSONArray& arr = animTimes.GetArray();
                for (unsigned i = 0, c = arr.Size(); i < c; i++)
                {
                    JSONValue atime = arr[i];
                    String name = atime["name"].GetString();
                    if (!name.Empty())
                    {
                        ValueAnimationInfo* animInfo = objectAnimation->GetAttributeAnimationInfo(name);
                        if (animInfo)
                            animInfo->SetTime(atime["time"].GetFloat());
                    }
                }
            }
        }
        SetObjectAnimationOnLoad(objectAnimation);
    }

    JSONValue attributeAnimationValue = source.Get("attributeanimation");

    if (attributeAnimationValue.IsNull())
        return true;

    if (!attributeAnimationValue.IsObject())
    {
        URHO3D_LOGWARNING("'attributeanimation' value is present in JSON data, but is not a JSON object; skipping it");
        return true;
    }

    const JSONObject& attributeAnimationObject = attributeAnimationValue.GetObject();
    for (JSONObject::ConstIterator it = attributeAnimationObject.Begin(); it != attributeAnimationObject.End(); it++)
    {
        String name = it->first_;
        JSONValue value = it->second_;
        SharedPtr<ValueAnimation> attributeAnimation(new ValueAnimation(context_));
        if (!attributeAnimation->LoadJSON(it->second_))
            return false;

        String wrapModeString = value.Get("wrapmode").GetString();
        WrapMode wrapMode = WM_LOOP;
        for (int i = 0; i <= WM_CLAMP; ++i)
        {
            if (wrapModeString == wrapModeNames[i])
            {
                wrapMode = (WrapMode)i;
                break;
            }
        }

        float speed = value.Get("speed").GetFloat();
        SetAttributeAnimation(name, attributeAnimation, wrapMode, speed);
        SetAttributeAnimationTime(name, value["time"].GetFloat());
    }

    return true;
}

bool Animatable::SaveXML(XMLElement& dest) const
{
    if (!Serializable::SaveXML(dest))
        return false;

    // Object animation without name
    if (objectAnimation_)
    {
        XMLElement elem = dest.CreateChild("objectanimation");
        String name = objectAnimation_->GetName();
        if (name.Empty())
        {
            if (!objectAnimation_->SaveXML(elem))
                return false;
        }
        else
            elem.SetResourceRef(ResourceRef(ObjectAnimation::GetTypeStatic(), name));
        const HashMap<String, SharedPtr<ValueAnimationInfo> >& infos = objectAnimation_->GetAttributeAnimationInfos();
        for (HashMap<String, SharedPtr<ValueAnimationInfo> >::ConstIterator i = infos.Begin(); i != infos.End(); ++i)
        {
            String outName;
            Animatable* a = const_cast<Animatable*>(this)->FindAttributeAnimationTarget(i->first_, outName);
            if (a != nullptr && !outName.Empty())
            {
                XMLElement timesElem = elem.CreateChild("animationtime");
                timesElem.SetAttribute("name", i->first_);
                timesElem.SetFloat("time", a->GetAttributeAnimationTime(outName));
            }
        }
    }

    for (HashMap<String, SharedPtr<AttributeAnimationInfo> >::ConstIterator i = attributeAnimationInfos_.Begin();
         i != attributeAnimationInfos_.End(); ++i)
    {
        ValueAnimation* attributeAnimation = i->second_->GetAnimation();
        if (attributeAnimation->GetOwner())
            continue;

        const AttributeInfo& attr = i->second_->GetAttributeInfo();
        XMLElement elem = dest.CreateChild("attributeanimation");
        elem.SetAttribute("name", attr.name_);
        String name = attributeAnimation->GetName();
        if (name.Empty())
        {
            if (!attributeAnimation->SaveXML(elem))
                return false;
        }
        else
            elem.SetResourceRef(ResourceRef(ValueAnimation::GetTypeStatic(), name));

        elem.SetAttribute("wrapmode", wrapModeNames[i->second_->GetWrapMode()]);
        elem.SetFloat("speed", i->second_->GetSpeed());
        elem.SetFloat("time", i->second_->GetTime());
    }

    return true;
}

bool Animatable::SaveJSON(JSONValue& dest) const
{
    if (!Serializable::SaveJSON(dest))
        return false;

    // Object animation without name
    if (objectAnimation_)
    {
        JSONValue objectAnimationValue;
        String name = objectAnimation_->GetName();
        if (name.Empty())
        {
            if (!objectAnimation_->SaveJSON(objectAnimationValue))
                return false;
        }
        else
        {
            JSONValue rr;
            rr.SetVariantValue(ResourceRef(ObjectAnimation::GetTypeStatic(), name));
            objectAnimationValue.Set("resource", rr);
        }
        JSONValue animTimes;
        const HashMap<String, SharedPtr<ValueAnimationInfo> >& infos = objectAnimation_->GetAttributeAnimationInfos();
        for (HashMap<String, SharedPtr<ValueAnimationInfo> >::ConstIterator i = infos.Begin(); i != infos.End(); ++i)
        {
            String outName;
            Animatable* a = const_cast<Animatable*>(this)->FindAttributeAnimationTarget(i->first_, outName);
            if (a != nullptr && !outName.Empty())
            {
                JSONValue aInfo;
                aInfo["name"] = i->first_;
                aInfo["time"] = a->GetAttributeAnimationTime(outName);
                animTimes.Push(aInfo);
            }
        }
        objectAnimationValue["animationtime"] = animTimes;
        dest.Set("objectanimation", objectAnimationValue);
    }

    JSONValue attributeAnimationValue;

    for (HashMap<String, SharedPtr<AttributeAnimationInfo> >::ConstIterator i = attributeAnimationInfos_.Begin();
         i != attributeAnimationInfos_.End(); ++i)
    {
        ValueAnimation* attributeAnimation = i->second_->GetAnimation();
        if (attributeAnimation->GetOwner())
            continue;

        const AttributeInfo& attr = i->second_->GetAttributeInfo();
        JSONValue attributeValue;
        attributeValue.Set("name", attr.name_);
        if (!attributeAnimation->SaveJSON(attributeValue))
            return false;

        attributeValue.Set("wrapmode", wrapModeNames[i->second_->GetWrapMode()]);
        attributeValue.Set("speed", (float) i->second_->GetSpeed());
        attributeValue.Set("time", (float)i->second_->GetTime());

        attributeAnimationValue.Set(attr.name_, attributeValue);
    }

    if (!attributeAnimationValue.IsNull())
        dest.Set("attributeanimation", attributeAnimationValue);

    return true;
}

void Animatable::SetAnimationEnabled(bool enable)
{
    if (objectAnimation_)
    {
        // In object animation there may be targets in hierarchy. Set same enable/disable state in all
        HashSet<Animatable*> targets;
        const HashMap<String, SharedPtr<ValueAnimationInfo> >& infos = objectAnimation_->GetAttributeAnimationInfos();
        for (HashMap<String, SharedPtr<ValueAnimationInfo> >::ConstIterator i = infos.Begin(); i != infos.End(); ++i)
        {
            String outName;
            Animatable* target = FindAttributeAnimationTarget(i->first_, outName);
            if (target && target != this)
                targets.Insert(target);
        }

        for (HashSet<Animatable*>::Iterator i = targets.Begin(); i != targets.End(); ++i)
            (*i)->animationEnabled_ = enable;
    }

    animationEnabled_ = enable;
}

void Animatable::SetAnimationTime(float time)
{
    if (objectAnimation_)
    {
        // In object animation there may be targets in hierarchy. Set same time in all
        const HashMap<String, SharedPtr<ValueAnimationInfo> >& infos = objectAnimation_->GetAttributeAnimationInfos();
        for (HashMap<String, SharedPtr<ValueAnimationInfo> >::ConstIterator i = infos.Begin(); i != infos.End(); ++i)
        {
            String outName;
            Animatable* target = FindAttributeAnimationTarget(i->first_, outName);
            if (target)
                target->SetAttributeAnimationTime(outName, time);
        }
    }
    else
    {
        for (HashMap<String, SharedPtr<AttributeAnimationInfo> >::ConstIterator i = attributeAnimationInfos_.Begin();
            i != attributeAnimationInfos_.End(); ++i)
            i->second_->SetTime(time);
    }
}

void Animatable::SetObjectAnimation(ObjectAnimation* objectAnimation)
{
    if (objectAnimation == objectAnimation_)
        return;

    if (objectAnimation_)
    {
        OnObjectAnimationRemoved(objectAnimation_);
        UnsubscribeFromEvent(objectAnimation_, E_ATTRIBUTEANIMATIONADDED);
        UnsubscribeFromEvent(objectAnimation_, E_ATTRIBUTEANIMATIONREMOVED);
    }

    objectAnimation_ = objectAnimation;

    if (objectAnimation_)
    {
        OnObjectAnimationAdded(objectAnimation_);
        SubscribeToEvent(objectAnimation_, E_ATTRIBUTEANIMATIONADDED, URHO3D_HANDLER(Animatable, HandleAttributeAnimationAdded));
        SubscribeToEvent(objectAnimation_, E_ATTRIBUTEANIMATIONREMOVED, URHO3D_HANDLER(Animatable, HandleAttributeAnimationRemoved));
    }
}

void Animatable::SetObjectAnimationOnLoad(ObjectAnimation* objectAnimation)
{
    if (objectAnimation == objectAnimation_)
        return;

    if (objectAnimation_)
    {
        OnObjectAnimationRemoved(objectAnimation_);
        UnsubscribeFromEvent(objectAnimation_, E_ATTRIBUTEANIMATIONADDED);
        UnsubscribeFromEvent(objectAnimation_, E_ATTRIBUTEANIMATIONREMOVED);
    }

    objectAnimation_ = objectAnimation;
}

void Animatable::SetAttributeAnimation(const String& name, ValueAnimation* attributeAnimation, WrapMode wrapMode, float speed)
{
    AttributeAnimationInfo* info = GetAttributeAnimationInfo(name);

    if (attributeAnimation)
    {
        if (info && attributeAnimation == info->GetAnimation())
        {
            info->SetWrapMode(wrapMode);
            info->SetSpeed(speed);
            return;
        }

        // Get attribute info
        const AttributeInfo* attributeInfo = nullptr;
        if (info)
            attributeInfo = &info->GetAttributeInfo();
        else
        {
            const Vector<AttributeInfo>* attributes = GetAttributes();
            if (!attributes)
            {
                URHO3D_LOGERROR(GetTypeName() + " has no attributes");
                return;
            }

            for (Vector<AttributeInfo>::ConstIterator i = attributes->Begin(); i != attributes->End(); ++i)
            {
                if (name == (*i).name_)
                {
                    attributeInfo = &(*i);
                    break;
                }
            }
        }

        if (!attributeInfo)
        {
            URHO3D_LOGERROR("Invalid name: " + name);
            return;
        }

        // Check value type is same with attribute type
        if (attributeAnimation->GetValueType() != attributeInfo->type_)
        {
            URHO3D_LOGERROR("Invalid value type");
            return;
        }

        // Add network attribute to set
        if (attributeInfo->mode_ & AM_NET)
            animatedNetworkAttributes_.Insert(attributeInfo);

        attributeAnimationInfos_[name] = new AttributeAnimationInfo(this, *attributeInfo, attributeAnimation, wrapMode, speed);

        if (!info)
            OnAttributeAnimationAdded();
    }
    else
    {
        if (!info)
            return;

        // Remove network attribute from set
        if (info->GetAttributeInfo().mode_ & AM_NET)
            animatedNetworkAttributes_.Erase(&info->GetAttributeInfo());

        attributeAnimationInfos_.Erase(name);
        OnAttributeAnimationRemoved();
    }
}

void Animatable::SetAttributeAnimationWrapMode(const String& name, WrapMode wrapMode)
{
    AttributeAnimationInfo* info = GetAttributeAnimationInfo(name);
    if (info)
        info->SetWrapMode(wrapMode);
}

void Animatable::SetAttributeAnimationSpeed(const String& name, float speed)
{
    AttributeAnimationInfo* info = GetAttributeAnimationInfo(name);
    if (info)
        info->SetSpeed(speed);
}

void Animatable::SetAttributeAnimationTime(const String& name, float time)
{
    AttributeAnimationInfo* info = GetAttributeAnimationInfo(name);
    if (info)
        info->SetTime(time);
}

void Animatable::RemoveObjectAnimation()
{
    SetObjectAnimation(nullptr);
}

void Animatable::RemoveAttributeAnimation(const String& name)
{
    SetAttributeAnimation(name, nullptr);
}

ObjectAnimation* Animatable::GetObjectAnimation() const
{
    return objectAnimation_;
}

ValueAnimation* Animatable::GetAttributeAnimation(const String& name) const
{
    const AttributeAnimationInfo* info = GetAttributeAnimationInfo(name);
    return info ? info->GetAnimation() : nullptr;
}

WrapMode Animatable::GetAttributeAnimationWrapMode(const String& name) const
{
    const AttributeAnimationInfo* info = GetAttributeAnimationInfo(name);
    return info ? info->GetWrapMode() : WM_LOOP;
}

float Animatable::GetAttributeAnimationSpeed(const String& name) const
{
    const AttributeAnimationInfo* info = GetAttributeAnimationInfo(name);
    return info ? info->GetSpeed() : 1.0f;
}

float Animatable::GetAttributeAnimationTime(const String& name) const
{
    const AttributeAnimationInfo* info = GetAttributeAnimationInfo(name);
    return info ? info->GetTime() : -1.0f;
}

void Animatable::SetObjectAnimationAttr(const ResourceRef& value)
{
    if (!value.name_.Empty())
    {
        auto* cache = GetSubsystem<ResourceCache>();
        SetObjectAnimation(cache->GetResource<ObjectAnimation>(value.name_));
    }
}

ResourceRef Animatable::GetObjectAnimationAttr() const
{
    return GetResourceRef(objectAnimation_, ObjectAnimation::GetTypeStatic());
}

Animatable* Animatable::FindAttributeAnimationTarget(const String& name, String& outName)
{
    // Base implementation only handles self
    outName = name;
    return this;
}

void Animatable::SetObjectAttributeAnimation(const String& name, ValueAnimation* attributeAnimation, WrapMode wrapMode, float speed, float time)
{
    if (time < 0.0f) // finished animation
        return;
    String outName;
    Animatable* target = FindAttributeAnimationTarget(name, outName);
    if (target)
    {
        target->SetAttributeAnimation(outName, attributeAnimation, wrapMode, speed);
        target->SetAttributeAnimationTime(outName, time);
    }
}

void Animatable::OnObjectAnimationAdded(ObjectAnimation* objectAnimation)
{
    if (!objectAnimation)
        return;

    // Set all attribute animations from the object animation
    const HashMap<String, SharedPtr<ValueAnimationInfo> >& attributeAnimationInfos = objectAnimation->GetAttributeAnimationInfos();
    for (HashMap<String, SharedPtr<ValueAnimationInfo> >::ConstIterator i = attributeAnimationInfos.Begin();
         i != attributeAnimationInfos.End(); ++i)
    {
        const String& name = i->first_;
        ValueAnimationInfo* info = i->second_;
        SetObjectAttributeAnimation(name, info->GetAnimation(), info->GetWrapMode(), info->GetSpeed(), info->GetTime());
    }
}

void Animatable::OnObjectAnimationRemoved(ObjectAnimation* objectAnimation)
{
    if (!objectAnimation)
        return;

    // Just remove all attribute animations listed by the object animation
    const HashMap<String, SharedPtr<ValueAnimationInfo> >& infos = objectAnimation->GetAttributeAnimationInfos();
    for (HashMap<String, SharedPtr<ValueAnimationInfo> >::ConstIterator i = infos.Begin(); i != infos.End(); ++i)
        SetObjectAttributeAnimation(i->first_, nullptr, WM_LOOP, 1.0f, 0);
}

void Animatable::UpdateAttributeAnimations(float timeStep)
{
    if (!animationEnabled_)
        return;

    // Keep weak pointer to self to check for destruction caused by event handling
    WeakPtr<Animatable> self(this);

    Vector<String> finishedNames;
    for (HashMap<String, SharedPtr<AttributeAnimationInfo> >::ConstIterator i = attributeAnimationInfos_.Begin();
         i != attributeAnimationInfos_.End(); ++i)
    {
        bool finished = i->second_->Update(timeStep);
        // If self deleted as a result of an event sent during animation playback, nothing more to do
        if (self.Expired())
            return;

        if (finished)
            finishedNames.Push(i->second_->GetAttributeInfo().name_);
    }

    for (unsigned i = 0; i < finishedNames.Size(); ++i)
        SetAttributeAnimation(finishedNames[i], nullptr);
}

bool Animatable::IsAnimatedNetworkAttribute(const AttributeInfo& attrInfo) const
{
    return animatedNetworkAttributes_.Find(&attrInfo) != animatedNetworkAttributes_.End();
}

AttributeAnimationInfo* Animatable::GetAttributeAnimationInfo(const String& name) const
{
    HashMap<String, SharedPtr<AttributeAnimationInfo> >::ConstIterator i = attributeAnimationInfos_.Find(name);
    if (i != attributeAnimationInfos_.End())
        return i->second_;

    return nullptr;
}

void Animatable::HandleAttributeAnimationAdded(StringHash eventType, VariantMap& eventData)
{
    if (!objectAnimation_)
        return;

    using namespace AttributeAnimationAdded;
    const String& name = eventData[P_ATTRIBUTEANIMATIONNAME].GetString();

    ValueAnimationInfo* info = objectAnimation_->GetAttributeAnimationInfo(name);
    if (!info)
        return;

    SetObjectAttributeAnimation(name, info->GetAnimation(), info->GetWrapMode(), info->GetSpeed(), info->GetTime());
}

void Animatable::HandleAttributeAnimationRemoved(StringHash eventType, VariantMap& eventData)
{
    if (!objectAnimation_)
        return;

    using namespace AttributeAnimationRemoved;
    const String& name = eventData[P_ATTRIBUTEANIMATIONNAME].GetString();

    SetObjectAttributeAnimation(name, nullptr, WM_LOOP, 1.0f, .0f);
}

void Animatable::ApplyAttributes()
{
    Serializable::ApplyAttributes();
    if (objectAnimation_)
    {
        OnObjectAnimationAdded(objectAnimation_);
        SubscribeToEvent(objectAnimation_, E_ATTRIBUTEANIMATIONADDED, URHO3D_HANDLER(Animatable, HandleAttributeAnimationAdded));
        SubscribeToEvent(objectAnimation_, E_ATTRIBUTEANIMATIONREMOVED, URHO3D_HANDLER(Animatable, HandleAttributeAnimationRemoved));
    }
}

}
