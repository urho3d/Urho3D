//=============================================================================
// Copyright (c) 2015 LumakSoftware
//
//=============================================================================
#pragma once

#include "../Urho3D.h"
#include "../Core/Object.h"
#include "../UI/UIElement.h"
#include <TurboBadger/tb_widgets_listener.h>

using namespace tb;

namespace Urho3D
{
class Object;

//=============================================================================
//=============================================================================
class URHO3D_API UTBMain : public UIElement, TBWidgetListener
{
    URHO3D_OBJECT(UTBMain, UIElement);

public:
    UTBMain(Context *_context, int _width, int _height); 
    virtual ~UTBMain();

    void Init(const String &_strDataPath);
    void SetWindowSize();

    // demo - if configured as demo
    void RunDemo();

    // tbwidget access
    TBWidget& Root();

    // language methods
    virtual bool SetLanguage(const String &_strLang);
    virtual const String& GetLanguageSetting();
    virtual bool AddLanguageType(const String &_strAbbr, const String &_strFilename);

    // font
    virtual bool AddFontInfo(const String &_strFilename, const String &_strName);

    // skin
    virtual bool SetSkinBackground(const String &_strSkinName);

protected:
    virtual void CreateLanguageMap();
    virtual void LoadDefaultResources();

protected:
    String                  strDataPath_;
    HashMap<String, String> LanguageHashMap_;
    String                  strLanguageSetting_;
};


}//namespace Urho3D
