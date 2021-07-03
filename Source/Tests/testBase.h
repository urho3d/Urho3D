#ifndef __testBase_h
#define __testBase_h

#include <gtest/gtest.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Core/WorkQueue.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Scene/Scene.h>



using namespace Urho3D;



class UrhoBaseTest : public ::testing::Test
{
protected:
    
    SharedPtr<Context> context_;
    
	void SetUp() override
	{
        context_ = SharedPtr<Context>(new Context());
        context_->RegisterSubsystem(new FileSystem(context_));
        context_->RegisterSubsystem(new ResourceCache(context_));
        context_->RegisterSubsystem(new WorkQueue(context_));
        RegisterSceneLibrary(context_);
        RegisterGraphicsLibrary(context_);
        RegisterPhysicsLibrary(context_);
	}

	void TearDown() override
	{
		
	}

};

#endif
