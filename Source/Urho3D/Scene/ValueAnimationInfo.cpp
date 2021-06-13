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

#include "../IO/Log.h"
#include "../Scene/ValueAnimation.h"
#include "../Scene/ValueAnimationInfo.h"

#include "../DebugNew.h"

namespace Urho3D
{

ValueAnimationInfo::ValueAnimationInfo(ValueAnimation* animation, WrapMode wrapMode, float speed) :
    animation_(animation),
    wrapMode_(wrapMode),
    speed_(speed),
    currentTime_(0.0f),
    lastScaledTime_(0.0f)
{
    speed_ = Max(0.0f, speed_);
}

ValueAnimationInfo::ValueAnimationInfo(Object* target, ValueAnimation* animation, WrapMode wrapMode, float speed) :
    target_(target),
    animation_(animation),
    wrapMode_(wrapMode),
    speed_(speed),
    currentTime_(0.0f),
    lastScaledTime_(0.0f)
{
    speed_ = Max(0.0f, speed_);
}

ValueAnimationInfo::ValueAnimationInfo(const ValueAnimationInfo& other) :
    target_(other.target_),
    animation_(other.animation_),
    wrapMode_(other.wrapMode_),
    speed_(other.speed_),
    currentTime_(0.0f),
    lastScaledTime_(0.0f)
{
}

ValueAnimationInfo::~ValueAnimationInfo() = default;

bool ValueAnimationInfo::Update(float timeStep)
{
    if (!animation_ || !target_)
        return true;

    return SetTime(currentTime_ + timeStep * speed_);
}

bool ValueAnimationInfo::SetTime(float time)
{
    if (!animation_)
        return true;

    currentTime_ = time;

    if (!target_ || !animation_->IsValid())
        return true;

    bool finished = false;

    // Calculate scale time by wrap mode
    float scaledTime = CalculateScaledTime(currentTime_, finished);

    // Apply to the target object
    ApplyValue(animation_->GetAnimationValue(scaledTime));

    // Send keyframe event if necessary
    if (animation_->HasEventFrames())
    {
        PODVector<const VAnimEventFrame*> eventFrames;
        GetEventFrames(lastScaledTime_, scaledTime, eventFrames);

        if (eventFrames.Size())
        {
            // Make a copy of the target weakptr, since if it expires, the AnimationInfo is deleted as well, in which case the
            // member variable cannot be accessed
            WeakPtr<Object> targetWeak(target_);

            for (unsigned i = 0; i < eventFrames.Size(); ++i)
                target_->SendEvent(eventFrames[i]->eventType_, const_cast<VariantMap&>(eventFrames[i]->eventData_));

            // Break immediately if target expired due to event
            if (targetWeak.Expired())
                return true;
        }
    }

    lastScaledTime_ = scaledTime;

    return finished;
}

Object* ValueAnimationInfo::GetTarget() const
{
    return target_;
}

void ValueAnimationInfo::ApplyValue(const Variant& newValue)
{
}

float ValueAnimationInfo::CalculateScaledTime(float currentTime, bool& finished) const
{
    float beginTime = animation_->GetBeginTime();
    float endTime = animation_->GetEndTime();

    switch (wrapMode_)
    {
    case WM_LOOP:
        {
            float span = endTime - beginTime;
            float time = fmodf(currentTime - beginTime, span);
            if (time < 0.0f)
                time += span;
            return beginTime + time;
        }

    case WM_ONCE:
        finished = (currentTime >= endTime);
        // Fallthrough

    case WM_CLAMP:
        return Clamp(currentTime, beginTime, endTime);

    default:
        URHO3D_LOGERROR("Unsupported attribute animation wrap mode");
        return beginTime;
    }
}

void ValueAnimationInfo::GetEventFrames(float beginTime, float endTime, PODVector<const VAnimEventFrame*>& eventFrames)
{
    switch (wrapMode_)
    {
    case WM_LOOP:
        /// \todo This can miss an event if the deltatime is exactly the animation's length
        if (beginTime <= endTime)
            animation_->GetEventFrames(beginTime, endTime, eventFrames);
        else
        {
            animation_->GetEventFrames(beginTime, animation_->GetEndTime(), eventFrames);
            animation_->GetEventFrames(animation_->GetBeginTime(), endTime, eventFrames);
        }
        break;

    case WM_ONCE:
    case WM_CLAMP:
        animation_->GetEventFrames(beginTime, endTime, eventFrames);
        break;
    }
}

}
