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

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/Controls.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>

#include "CharacterDemo.h"

#include <Urho3D/DebugNew.h>

//=============================================================================
//=============================================================================
URHO3D_DEFINE_APPLICATION_MAIN(CharacterDemo)

//=============================================================================
//=============================================================================
CharacterDemo::CharacterDemo(Context* context) :
    Sample(context)
{
}

CharacterDemo::~CharacterDemo()
{
}

void CharacterDemo::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create static scene content
    CreateScene();

    // Create the controllable character
    CreateCharacter();

    // Create the UI content
    CreateInstructions();

    // Subscribe to necessary events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_RELATIVE);
}

void CharacterDemo::CreateScene()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    scene_ = new Scene(context_);

    // Create scene subsystem components
    scene_->CreateComponent<Octree>();
    scene_->CreateComponent<PhysicsWorld>();

    // Create camera and define viewport. We will be doing load / save, so it's convenient to create the camera outside the scene,
    // so that it won't be destroyed and recreated, and we don't have to redefine the viewport on load
    cameraNode_ = new Node(context_);
    cameraNode_->SetPosition(Vector3(0, 1.0f, -3.0f));
    Camera* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetFarClip(300.0f);
    GetSubsystem<Renderer>()->SetViewport(0, new Viewport(context_, scene_, camera));

    // Create static scene content. First create a zone for ambient lighting and fog control
    Node* zoneNode = scene_->CreateChild("Zone");
    Zone* zone = zoneNode->CreateComponent<Zone>();
    zone->SetAmbientColor(Color(0.15f, 0.15f, 0.15f));
    zone->SetFogColor(Color(0.1f, 0.1f, 0.7f));
    zone->SetFogStart(100.0f);
    zone->SetFogEnd(300.0f);
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));

    // Create a directional light with cascaded shadow mapping
    Node* lightNode = scene_->CreateChild("DirectionalLight");
    lightNode->SetDirection(Vector3(0.0f, -0.1f, 0.8f));
    Light* light = lightNode->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetCastShadows(true);
    light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
    light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));
    light->SetSpecularIntensity(0.5f);
}

void CharacterDemo::CreateCharacter()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    Node* objectNode = scene_->CreateChild("Claire");
    objectNode->SetPosition(Vector3(0.0f, 0.0f, 0.0f));

    // spin node
    Node* adjustNode = objectNode->CreateChild("AdjNode");
    //adjustNode->SetRotation( Quaternion(180, Vector3(0,1,0) ) );
    
    // Create the rendering component + animation controller
    AnimatedModel* object = adjustNode->CreateComponent<AnimatedModel>();
    object->SetModel(cache->GetResource<Model>("Claire/Claire.mdl"));
    Material *matOrig = cache->GetResource<Material>("Claire/Materials/JoinedMaterial.xml");
    matEyes = matOrig->Clone();
    matEyeBrows = matOrig->Clone();
    matMouth = matOrig->Clone();
    object->SetMaterial(0, matEyeBrows);
    object->SetMaterial(1, matEyes);
    object->SetMaterial(2, matMouth);
    object->SetMaterial(3, cache->GetResource<Material>("Claire/Materials/Girl01_Body_MAT.xml"));
    object->SetCastShadows(true);
    AnimationController *animCtrl = adjustNode->CreateComponent<AnimationController>();
    animCtrl->Play("Claire/Claire_Idle.ani", 0, true);

    // int uv indeces
    eyesIdx_ = 0;
    eyesIdxMax_ = 30;
    eyesMaxRows_ = 10;

    eyeBrowsIdx_ = 0;
    eyeBrowsIdxMax_ = 10;

    mouthIdx_ = 0;
    mouthIdxMax_ = 30;
    mouthMaxRows_ = 10;

    // cell dims
    cellWidth_ = 0.1f;
    cellHeight_ = 0.1f;
    eyesCellWidth_ = 0.2f;
}

void CharacterDemo::CreateInstructions()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    UI* ui = GetSubsystem<UI>();

    // Construct new Text object, set string to display and font to use
    Text* instructionText = ui->GetRoot()->CreateChild<Text>();
    instructionText->SetText("W=eyes, E=eyebrows, R=mouth");
    instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
    instructionText->SetColor(Color::YELLOW);
    // The text has multiple rows. Center them in relation to each other
    instructionText->SetTextAlignment(HA_CENTER);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    //instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, 20);
}

void CharacterDemo::SubscribeToEvents()
{
    // Subscribe to Update event for setting the character controls before physics simulation
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(CharacterDemo, HandleUpdate));

    UnsubscribeFromEvent(E_SCENEUPDATE);
}

void CharacterDemo::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    Input* input = GetSubsystem<Input>();

    // eyes offset
    if (input->GetKeyPress(KEY_W))
    {
        if (debounceTimer_.GetMSec(false) > 250)
        {
            eyesIdx_ = ++eyesIdx_ % eyesIdxMax_;
            int row = eyesIdx_ % eyesMaxRows_;
            int col = eyesIdx_ / eyesMaxRows_;
            float u = (float)col * eyesCellWidth_;
            float v = (float)row * cellHeight_;
            matEyes->SetShaderParameter("UOffset", Vector4(1.0f, 0.0f, 0.0f, u));
            matEyes->SetShaderParameter("VOffset", Vector4(0.0f, 1.0f, 0.0f, v));

            debounceTimer_.Reset();
        }
    }

    // eye brows offset
    if (input->GetKeyPress(KEY_E))
    {
        if (debounceTimer_.GetMSec(false) > 250)
        {
            eyeBrowsIdx_ = ++eyeBrowsIdx_ % eyeBrowsIdxMax_;
            float v = (float)eyeBrowsIdx_ * cellHeight_;

            matEyeBrows->SetShaderParameter("VOffset", Vector4(0.0f, 1.0f, 0.0f, v));

            debounceTimer_.Reset();
        }
    }

    // mouth offset
    if (input->GetKeyPress(KEY_R))
    {
        if (debounceTimer_.GetMSec(false) > 250)
        {
            mouthIdx_ = ++mouthIdx_ % mouthIdxMax_;
            int row = mouthIdx_ % mouthMaxRows_;
            int col = mouthIdx_ / mouthMaxRows_;
            float u = (float)col * cellWidth_;
            float v = (float)row * cellHeight_;
            matMouth->SetShaderParameter("UOffset", Vector4(1.0f, 0.0f, 0.0f, u));
            matMouth->SetShaderParameter("VOffset", Vector4(0.0f, 1.0f, 0.0f, v));

            debounceTimer_.Reset();
        }
    }
}


