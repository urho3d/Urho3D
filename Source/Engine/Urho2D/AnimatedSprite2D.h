//
// Copyright (c) 2008-2014 the Urho3D project.
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

#include "Animation2D.h"
#include "StaticSprite2D.h"

/// Loop mode.
enum LoopMode2D
{
    /// Default, use animation's value.
    LM_DEFAULT = 0,
    /// Force looped.
    LM_FORCE_LOOPED,
    /// Force clamped.
    LM_FORCE_CLAMPED
};

namespace Urho3D
{

class AnimationSet2D;

/// Spriter animation component.
class URHO3D_API AnimatedSprite2D : public StaticSprite2D
{
    OBJECT(AnimatedSprite2D);

public:
    /// Construct.
    AnimatedSprite2D(Context* context);
    /// Destruct.
    virtual ~AnimatedSprite2D();
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Handle enabled/disabled state change.
    virtual void OnSetEnabled();

    /// Set speed.
    void SetSpeed(float speed);
    /// Set animation by animation set, name and loop mode.
    void SetAnimation(AnimationSet2D* animationSet, const String& name, LoopMode2D loopMode = LM_DEFAULT);
    /// Set animation by name and loop mode.
    void SetAnimation(const String& name, LoopMode2D loopMode = LM_DEFAULT);
    /// Set animation set.
    void SetAnimationSet(AnimationSet2D* animationSet);
    /// Set loop mode.
    void SetLoopMode(LoopMode2D loopMode);

    /// Return speed.
    float GetSpeed() const { return speed_; }
    /// Return animation name.
    const String& GetAnimation() const { return animationName_; }
    /// Return animation.
    AnimationSet2D* GetAnimationSet() const;
    /// Return loop mode.
    LoopMode2D GetLoopMode() const { return loopMode_; }
    /// Return root node.
    Node* GetRootNode() const;

    /// Set animation set attribute.
    void SetAnimationSetAttr(const ResourceRef& value);
    /// Return animation set attribute.
    ResourceRef GetAnimationSetAttr() const;
    /// Set anmiation by name.
    void SetAnimationAttr(const String& name);

protected:
    /// Handle node being assigned.
    virtual void OnNodeSet(Node* node);
    /// Recalculate the world-space bounding box.
    virtual void OnWorldBoundingBoxUpdate();
    /// Handle layer changed.
    virtual void OnLayerChanged();
    /// Handle blend mode changed.
    virtual void OnBlendModeChanged();
    /// Handle update vertices.
    virtual void UpdateVertices();
    /// Handle flip changed.
    virtual void OnFlipChanged();
    /// Set animation.
    void SetAnimation(Animation2D* animation, LoopMode2D loopMode);
    /// Update animation.
    void UpdateAnimation(float timeStep);
    /// Calculate time line world world transform.
    void CalculateTimelineWorldTransform(unsigned index);
    /// Handle scene post update.
    void HandleScenePostUpdate(StringHash eventType, VariantMap& eventData);

    /// Speed.
    float speed_;
    /// Animation set.
    SharedPtr<AnimationSet2D> animationSet_;
    /// Animation name.
    String animationName_;
    /// Animation.
    SharedPtr<Animation2D> animation_;
    /// Loop mode.
    LoopMode2D loopMode_;
    /// Looped.
    bool looped_;
    /// Current time.
    float currentTime_;
    /// Root node.
    SharedPtr<Node> rootNode_;
    /// Timeline nodes.
    Vector<SharedPtr<Node> > timelineNodes_;
    /// Transform info.
    struct TransformInfo
    {
        /// Construct.
        TransformInfo() :
            parent_(-1),
            worldSpace_(false)
        {
        }

        /// Parent.
        int parent_;
        /// World space.
        bool worldSpace_;
        /// Transform.
        Transform2D transform_;
    };
    /// Timeline transform infos.
    Vector<TransformInfo> timelineTransformInfos_;
};

}
