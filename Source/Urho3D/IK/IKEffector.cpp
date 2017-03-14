//
// Copyright (c) 2008-2016 the Urho3D project.
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

#include "../IK/IKEffector.h"
#include "../IK/IKSolver.h"
#include "../IK/IKConverters.h"
#include "../IK/IKEvents.h"
#include "../Core/Context.h"
#include "../Graphics/DebugRenderer.h"
#include "../Scene/Node.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneEvents.h"
#include <ik/effector.h>
#include <ik/log.h>

namespace Urho3D
{

extern const char* IK_CATEGORY;

// ----------------------------------------------------------------------------
IKEffector::IKEffector(Context* context) :
    Component(context),
    ikEffector_(NULL),
    chainLength_(0),
    weight_(1.0f),
    inheritParentRotation_(false),
    weightedChildrotations_(false)
{
}

// ----------------------------------------------------------------------------
IKEffector::~IKEffector()
{
}

// ----------------------------------------------------------------------------
void IKEffector::RegisterObject(Context* context)
{
    context->RegisterFactory<IKEffector>(IK_CATEGORY);

    URHO3D_ACCESSOR_ATTRIBUTE("Target Node", GetTargetName, SetTargetName, String, String::EMPTY, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Chain Length", GetChainLength, SetChainLength, unsigned, true, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Target Position", GetTargetPosition, SetTargetPosition, Vector3, Vector3::ZERO, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Target Rotation", GetTargetRotationEuler, SetTargetRotationEuler, Vector3, Vector3::ZERO, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Weight", GetWeight, SetWeight, float, 1.0, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Inherit Parent Rotation", DoInheritParentRotation, SetInheritParentRotation, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Weighted Child Rotations", DoWeightedChildRotations, SetWeightedChildRotations, bool, false, AM_DEFAULT);
}

// ----------------------------------------------------------------------------
Node* IKEffector::GetTargetNode() const
{
    return targetNode_;
}

// ----------------------------------------------------------------------------
void IKEffector::SetTargetNode(Node* targetNode)
{
    using namespace IKEffectorTargetChanged;

    VariantMap& eventData = GetEventDataMap();
    eventData[P_EFFECTORNODE] = node_;       // The effector node that has changed targets
    eventData[P_TARGETNODE]   = targetNode_; // The new target node. NOTE: Can be NULL (means no target)
    SendEvent(E_IKEFFECTORTARGETCHANGED, eventData);

    // Finally change the target node
    targetNode_ = targetNode;
    targetName_ = targetNode != NULL ? targetNode->GetName() : "";
}

// ----------------------------------------------------------------------------
const String& IKEffector::GetTargetName() const
{
    return targetName_;
}

// ----------------------------------------------------------------------------
void IKEffector::SetTargetName(const String& nodeName)
{
    targetName_ = nodeName;
    targetNode_ = NULL;
}

// ----------------------------------------------------------------------------
const Vector3& IKEffector::GetTargetPosition() const
{
    return targetPosition_;
}

// ----------------------------------------------------------------------------
void IKEffector::SetTargetPosition(const Vector3& targetPosition)
{
    targetPosition_ = targetPosition;
    if (ikEffector_ != NULL)
        ikEffector_->target_position = Vec3Urho2IK(targetPosition);
}

// ----------------------------------------------------------------------------
const Quaternion& IKEffector::GetTargetRotation() const
{
    return targetRotation_;
}

// ----------------------------------------------------------------------------
void IKEffector::SetTargetRotation(const Quaternion& targetRotation)
{
    targetRotation_ = targetRotation;
    if(ikEffector_)
        ikEffector_->target_rotation = QuatUrho2IK(targetRotation);
}

// ----------------------------------------------------------------------------
Vector3 IKEffector::GetTargetRotationEuler() const
{
    return targetRotation_.EulerAngles();
}

// ----------------------------------------------------------------------------
void IKEffector::SetTargetRotationEuler(const Vector3& targetRotation)
{
    SetTargetRotation(Quaternion(targetRotation.x_, targetRotation.y_, targetRotation.z_));
}

// ----------------------------------------------------------------------------
unsigned int IKEffector::GetChainLength() const
{
    return chainLength_;
}

// ----------------------------------------------------------------------------
void IKEffector::SetChainLength(unsigned chainLength)
{
    chainLength_ = chainLength;
    if (ikEffector_ != NULL)
    {
        ikEffector_->chain_length = chainLength;
        solver_->MarkSolverTreeDirty();
    }
}

// ----------------------------------------------------------------------------
float IKEffector::GetWeight() const
{
    return weight_;
}

// ----------------------------------------------------------------------------
void IKEffector::SetWeight(float weight)
{
    weight_ = weight;
    if (ikEffector_ != NULL)
        /*ikEffector_->weight = weight*/;
}

// ----------------------------------------------------------------------------
bool IKEffector::DoInheritParentRotation() const
{
    return inheritParentRotation_;
}

// ----------------------------------------------------------------------------
void IKEffector::SetInheritParentRotation(bool enable)
{
    inheritParentRotation_ = enable;
    if(ikEffector_ != NULL)
        /*TODO*/;
}

// ----------------------------------------------------------------------------
bool IKEffector::DoWeightedChildRotations() const
{
    return weightedChildrotations_;
}

// ----------------------------------------------------------------------------
void IKEffector::SetWeightedChildRotations(bool enable)
{
    weightedChildrotations_ = enable;
    if(ikEffector_ != NULL)
        /*TODO*/;
}

// ----------------------------------------------------------------------------
void IKEffector::UpdateTargetNodePosition()
{
    if (targetNode_ == NULL)
    {
        SetTargetNode(node_->GetScene()->GetChild(targetName_, true));
        if (targetNode_ == NULL)
            return;
    }

    SetTargetPosition(targetNode_->GetWorldPosition());
}

// ----------------------------------------------------------------------------
void IKEffector::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    Node* terminationNode;
    if (solver_ == NULL)
        terminationNode = GetScene();
    else
        terminationNode = solver_->GetNode();

    // Calculate average length of all segments so we can determine the radius
    // of the debug spheres to draw
    int chainLength = chainLength_ == 0 ? -1 : chainLength_;
    Node* a = node_;
    float averageLength = 0.0f;
    unsigned numberOfSegments = 0;
    while (a && a != terminationNode->GetParent() && chainLength-- != 0)
    {
        averageLength += a->GetPosition().Length();
        ++numberOfSegments;
        a = a->GetParent();
    }
    averageLength /= numberOfSegments;

    // connect all chained nodes together with lines
    chainLength = chainLength_ == 0 ? -1 : chainLength_;
    a = node_;
    Node* b = a->GetParent();
    debug->AddSphere(
        Sphere(a->GetWorldPosition(), averageLength * 0.1),
        Color::YELLOW,
        depthTest
    );
    while (b && b != terminationNode->GetParent() && chainLength-- != 0)
    {
        debug->AddLine(
            a->GetWorldPosition(),
            b->GetWorldPosition(),
            Color::WHITE,
            depthTest
        );
        debug->AddSphere(
            Sphere(b->GetWorldPosition(), averageLength * 0.1),
            Color::YELLOW,
            depthTest
        );
        a = b;
        b = b->GetParent();
    }
}

// ----------------------------------------------------------------------------
void IKEffector::SetEffector(ik_effector_t* effector)
{
    ikEffector_ = effector;
    if (effector)
    {
        effector->chain_length = chainLength_;
        effector->target_position = Vec3Urho2IK(targetPosition_);
        effector->target_rotation = QuatUrho2IK(targetRotation_);
    }
}

// ----------------------------------------------------------------------------
void IKEffector::SetSolver(IKSolver* solver)
{
    solver_ = solver;
}

} // namespace Urho3D
