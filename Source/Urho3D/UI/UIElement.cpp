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
#include "../Core/CoreEvents.h"
#include "../Container/HashSet.h"
#include "../Container/Sort.h"
#include "../IO/Log.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/JSONFile.h"
#include "../Scene/ObjectAnimation.h"
#include "../UI/Cursor.h"
#include "../UI/UI.h"
#include "../UI/UIElement.h"
#include "../UI/UIEvents.h"

#include "../DebugNew.h"

namespace Urho3D
{

const char* horizontalAlignments[] =
{
    "Left",
    "Center",
    "Right",
    "Custom",
    nullptr
};

const char* verticalAlignments[] =
{
    "Top",
    "Center",
    "Bottom",
    "Custom",
    nullptr
};

static const char* focusModes[] =
{
    "NotFocusable",
    "ResetFocus",
    "Focusable",
    "FocusableDefocusable",
    nullptr
};

static const char* dragDropModes[] =
{
    "Disabled",
    "Source",
    "Target",
    "SourceAndTarget",
    nullptr
};

static const char* layoutModes[] =
{
    "Free",
    "Horizontal",
    "Vertical",
    nullptr
};

extern const char* UI_CATEGORY;

static bool CompareUIElements(const UIElement* lhs, const UIElement* rhs)
{
    return lhs->GetPriority() < rhs->GetPriority();
}

XPathQuery UIElement::styleXPathQuery_("/elements/element[@type=$typeName]", "typeName:String");

UIElement::UIElement(Context* context) :
    Animatable(context),
    pivot_(std::numeric_limits<float>::max(), std::numeric_limits<float>::max())
{
    SetEnabled(false);
}

UIElement::~UIElement()
{
    // If child elements have outside references, detach them
    for (Vector<SharedPtr<UIElement> >::Iterator i = children_.Begin(); i < children_.End(); ++i)
    {
        if (i->Refs() > 1)
            (*i)->Detach();
    }
}

void UIElement::RegisterObject(Context* context)
{
    context->RegisterFactory<UIElement>(UI_CATEGORY);

    URHO3D_ACCESSOR_ATTRIBUTE("Name", GetName, SetName, String, String::EMPTY, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Position", GetPosition, SetPosition, IntVector2, IntVector2::ZERO, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Size", GetSize, SetSize, IntVector2, IntVector2::ZERO, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Min Size", GetMinSize, SetMinSize, IntVector2, IntVector2::ZERO, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Max Size", GetMaxSize, SetMaxSize, IntVector2, IntVector2(M_MAX_INT, M_MAX_INT), AM_FILE);
    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Horiz Alignment", GetHorizontalAlignment, SetHorizontalAlignment, HorizontalAlignment,
        horizontalAlignments, HA_LEFT, AM_FILEREADONLY);
    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Vert Alignment", GetVerticalAlignment, SetVerticalAlignment, VerticalAlignment, verticalAlignments,
        VA_TOP, AM_FILEREADONLY);
    URHO3D_ACCESSOR_ATTRIBUTE("Min Anchor", GetMinAnchor, SetMinAnchor, Vector2, Vector2::ZERO, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Max Anchor", GetMaxAnchor, SetMaxAnchor, Vector2, Vector2::ZERO, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Min Offset", GetMinOffset, SetMinOffset, IntVector2, IntVector2::ZERO, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Max Offset", GetMaxOffset, SetMaxOffset, IntVector2, IntVector2::ZERO, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Pivot", GetPivot, SetPivot, Vector2, Vector2(std::numeric_limits<float>::max(), std::numeric_limits<float>::max()), AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Enable Anchor", GetEnableAnchor, SetEnableAnchor, bool, false, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Clip Border", GetClipBorder, SetClipBorder, IntRect, IntRect::ZERO, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Priority", GetPriority, SetPriority, int, 0, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Opacity", GetOpacity, SetOpacity, float, 1.0f, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Color", GetColorAttr, SetColor, Color, Color::WHITE, AM_FILE);
    URHO3D_ATTRIBUTE("Top Left Color", Color, colors_[0], Color::WHITE, AM_FILE);
    URHO3D_ATTRIBUTE("Top Right Color", Color, colors_[1], Color::WHITE, AM_FILE);
    URHO3D_ATTRIBUTE("Bottom Left Color", Color, colors_[2], Color::WHITE, AM_FILE);
    URHO3D_ATTRIBUTE("Bottom Right Color", Color, colors_[3], Color::WHITE, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, false, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Is Editable", IsEditable, SetEditable, bool, true, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Is Selected", IsSelected, SetSelected, bool, false, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Is Visible", IsVisible, SetVisible, bool, true, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Bring To Front", GetBringToFront, SetBringToFront, bool, false, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Bring To Back", GetBringToBack, SetBringToBack, bool, true, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Clip Children", GetClipChildren, SetClipChildren, bool, false, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Use Derived Opacity", GetUseDerivedOpacity, SetUseDerivedOpacity, bool, true, AM_FILE);
    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Focus Mode", GetFocusMode, SetFocusMode, FocusMode, focusModes, FM_NOTFOCUSABLE, AM_FILE);
    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Drag And Drop Mode", GetDragDropMode, SetDragDropMode, DragAndDropModeFlags, dragDropModes, DD_DISABLED, AM_FILE);
    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Layout Mode", GetLayoutMode, SetLayoutMode, LayoutMode, layoutModes, LM_FREE, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Layout Spacing", GetLayoutSpacing, SetLayoutSpacing, int, 0, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Layout Border", GetLayoutBorder, SetLayoutBorder, IntRect, IntRect::ZERO, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Layout Flex Scale", GetLayoutFlexScale, SetLayoutFlexScale, Vector2, Vector2::ONE, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Indent", GetIndent, SetIndent, int, 0, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Indent Spacing", GetIndentSpacing, SetIndentSpacing, int, 16, AM_FILE);
    URHO3D_ATTRIBUTE("Variables", VariantMap, vars_, Variant::emptyVariantMap, AM_FILE);
    URHO3D_ATTRIBUTE("Tags", StringVector, tags_, Variant::emptyStringVector, AM_FILE);
}

void UIElement::ApplyAttributes()
{
    colorGradient_ = false;
    derivedColorDirty_ = true;

    for (unsigned i = 1; i < MAX_UIELEMENT_CORNERS; ++i)
    {
        if (colors_[i] != colors_[0])
            colorGradient_ = true;
    }
}

bool UIElement::Load(Deserializer& source)
{
    // Just used for loading children
    source.ReadString(); // Clear type string
    source.ReadBool(); // Clear internal bool

    appliedStyle_ =  source.ReadString(); // Can't filter attributes on save, so it effectively becomes this

    if (!Animatable::Load(source))
        return false;

    unsigned numChildren = source.ReadVLE();
    unsigned nextInternalChild = 0;
    for (unsigned i = 0; i < numChildren; ++i)
    {
        unsigned startPos = source.Tell();
        String typeName = source.ReadString();
        bool internalElem = source.ReadBool();
        if (typeName.Empty())
            typeName = "UIElement";
        source.SeekRelative(startPos - source.Tell());

        UIElement* child = nullptr;
        if (!internalElem)
            child = CreateChild(typeName, String::EMPTY, M_MAX_UNSIGNED);
        else
        {
            for (unsigned i = nextInternalChild; i < children_.Size(); ++i)
            {
                if (children_[i]->IsInternal() && children_[i]->GetTypeName() == typeName)
                {
                    child = children_[i];
                    nextInternalChild = i + 1;
                    break;
                }
            }

            if (!child)
                URHO3D_LOGWARNING("Could not find matching internal child element of type " + typeName + " in " + GetTypeName());
        }

        if (child)
        {
            if (!child->Load(source))
                return false;
        }
    }

    ApplyAttributes();

    EnableLayoutUpdate();
    UpdateLayout();

    return true;
}

bool UIElement::Save(Serializer& dest) const
{
    // Write type
    if (!dest.WriteString(GetTypeName()))
        return false;

    // Write internal flag
    if (!dest.WriteBool(internal_))
        return false;

    // Write style
    if (!dest.WriteString(appliedStyle_))
        return false;

    // Write attributes
    if (!Animatable::Save(dest))
        return false;

    // Write child elements
    dest.WriteVLE(children_.Size());
    for (unsigned i = 0; i < children_.Size(); ++i)
    {
        UIElement* element = children_[i];
        if (element->IsTemporary())
            continue;

        if (!element->Save(dest))
            return false;
    }

    return true;
}

bool UIElement::LoadXML(const XMLElement& source)
{
    return LoadXML(source, nullptr);
}

bool UIElement::LoadXML(const XMLElement& source, XMLFile* styleFile)
{
    // Get style override if defined
    String styleName = source.GetAttribute("style");

    // Apply the style first, if the style file is available
    if (styleFile)
    {
        // If not defined, use type name
        if (styleName.Empty())
            styleName = GetTypeName();

        SetStyle(styleName, styleFile);
    }
    // The 'style' attribute value in the style file cannot be equals to original's applied style to prevent infinite loop
    else if (!styleName.Empty() && styleName != appliedStyle_)
    {
        // Attempt to use the default style file
        styleFile = GetDefaultStyle();

        if (styleFile)
        {
            // Remember the original applied style
            String appliedStyle(appliedStyle_);
            SetStyle(styleName, styleFile);
            appliedStyle_ = appliedStyle;
        }
    }

    // Prevent updates while loading attributes
    DisableLayoutUpdate();

    // Then load rest of the attributes from the source
    if (!Animatable::LoadXML(source))
        return false;

    unsigned nextInternalChild = 0;

    // Load child elements. Internal elements are not to be created as they already exist
    XMLElement childElem = source.GetChild("element");
    while (childElem)
    {
        bool internalElem = childElem.GetBool("internal");
        String typeName = childElem.GetAttribute("type");
        if (typeName.Empty())
            typeName = "UIElement";
        unsigned index = childElem.HasAttribute("index") ? childElem.GetUInt("index") : M_MAX_UNSIGNED;
        UIElement* child = nullptr;

        if (!internalElem)
            child = CreateChild(typeName, String::EMPTY, index);
        else
        {
            for (unsigned i = nextInternalChild; i < children_.Size(); ++i)
            {
                if (children_[i]->IsInternal() && children_[i]->GetTypeName() == typeName)
                {
                    child = children_[i];
                    nextInternalChild = i + 1;
                    break;
                }
            }

            if (!child)
                URHO3D_LOGWARNING("Could not find matching internal child element of type " + typeName + " in " + GetTypeName());
        }

        if (child)
        {
            if (!styleFile)
                styleFile = GetDefaultStyle();
            if (!child->LoadXML(childElem, styleFile))
                return false;
        }

        childElem = childElem.GetNext("element");
    }

    ApplyAttributes();

    EnableLayoutUpdate();
    UpdateLayout();

    return true;
}

UIElement* UIElement::LoadChildXML(const XMLElement& childElem, XMLFile* styleFile)
{
    bool internalElem = childElem.GetBool("internal");
    if (internalElem)
    {
        URHO3D_LOGERROR("Loading internal child element is not supported");
        return nullptr;
    }

    String typeName = childElem.GetAttribute("type");
    if (typeName.Empty())
        typeName = "UIElement";
    unsigned index = childElem.HasAttribute("index") ? childElem.GetUInt("index") : M_MAX_UNSIGNED;
    UIElement* child = CreateChild(typeName, String::EMPTY, index);

    if (child)
    {
        if (!styleFile)
            styleFile = GetDefaultStyle();
        if (!child->LoadXML(childElem, styleFile))
        {
            RemoveChild(child, index);
            return nullptr;
        }
    }

    return child;
}

bool UIElement::SaveXML(XMLElement& dest) const
{
    // Write type
    if (GetTypeName() != "UIElement")
    {
        if (!dest.SetString("type", GetTypeName()))
            return false;
    }

    // Write internal flag
    if (internal_)
    {
        if (!dest.SetBool("internal", internal_))
            return false;
    }

    // Write style
    if (!appliedStyle_.Empty() && appliedStyle_ != "UIElement")
    {
        if (!dest.SetAttribute("style", appliedStyle_))
            return false;
    }
    else if (internal_)
    {
        if (!dest.SetAttribute("style", "none"))
            return false;
    }

    // Write attributes
    if (!Animatable::SaveXML(dest))
        return false;

    // Write child elements
    for (unsigned i = 0; i < children_.Size(); ++i)
    {
        UIElement* element = children_[i];
        if (element->IsTemporary())
            continue;

        XMLElement childElem = dest.CreateChild("element");
        if (!element->SaveXML(childElem))
            return false;
    }

    // Filter UI-style and implicit attributes
    return FilterAttributes(dest);
}

bool UIElement::LoadJSON(const JSONValue& source)
{
    return LoadJSON(source, nullptr);
}

bool UIElement::LoadJSON(const JSONValue& source, XMLFile* styleFile)
{
    // Get style override if defined
    String styleName = source["style"].GetString();

    // Apply the style first, if the style file is available
    if (styleFile)
    {
        // If not defined, use type name
        if (styleName.Empty())
            styleName = GetTypeName();

        SetStyle(styleName, styleFile);
    }
    // The 'style' attribute value in the style file cannot be equals to original's applied style to prevent infinite loop
    else if (!styleName.Empty() && styleName != appliedStyle_)
    {
        // Attempt to use the default style file
        styleFile = GetDefaultStyle();

        if (styleFile)
        {
            // Remember the original applied style
            String appliedStyle(appliedStyle_);
            SetStyle(styleName, styleFile);
            appliedStyle_ = appliedStyle;
        }
    }

    // Prevent updates while loading attributes
    DisableLayoutUpdate();

    // Then load rest of the attributes from the source
    if (!Animatable::LoadJSON(source))
        return false;

    unsigned nextInternalChild = 0;

    // Load child elements. Internal elements are not to be created as they already exist
    const JSONArray& children = source["children"].GetArray();
    for (auto childElem : children)
    {
        bool internalElem = childElem["internal"].GetBool();
        String typeName = childElem["type"].GetString();
        if (typeName.Empty())
            typeName = "UIElement";
        unsigned index = childElem["index"].GetUInt(M_MAX_UNSIGNED);
        UIElement* child = nullptr;

        if (!internalElem)
            child = CreateChild(typeName, String::EMPTY, index);
        else
        {
            for (unsigned i = nextInternalChild; i < children_.Size(); ++i)
            {
                if (children_[i]->IsInternal() && children_[i]->GetTypeName() == typeName)
                {
                    child = children_[i];
                    nextInternalChild = i + 1;
                    break;
                }
            }

            if (!child)
                URHO3D_LOGWARNING("Could not find matching internal child element of type " + typeName + " in " + GetTypeName());
        }

        if (child)
        {
            if (!styleFile)
                styleFile = GetDefaultStyle();
            if (!child->LoadJSON(childElem, styleFile))
                return false;
        }
    }

    ApplyAttributes();

    EnableLayoutUpdate();
    UpdateLayout();

    return true;
}

UIElement* UIElement::LoadChildJSON(const JSONValue& childElem, XMLFile* styleFile)
{
    bool internalElem = childElem["internal"].GetBool();
    if (internalElem)
    {
        URHO3D_LOGERROR("Loading internal child element is not supported");
        return nullptr;
    }

    String typeName = childElem["type"].GetString();
    if (typeName.Empty())
        typeName = "UIElement";
    unsigned index = childElem["index"].GetUInt(M_MAX_UNSIGNED);
    UIElement* child = CreateChild(typeName, String::EMPTY, index);

    if (child)
    {
        if (!styleFile)
            styleFile = GetDefaultStyle();
        if (!child->LoadJSON(childElem, styleFile))
        {
            RemoveChild(child, index);
            return nullptr;
        }
    }

    return child;
}

bool UIElement::SaveJSON(JSONValue& dest) const
{
    // Write type
    if (GetTypeName() != "UIElement")
        dest.Set("type", GetTypeName());

    // Write internal flag
    if (internal_)
        dest.Set("internal", internal_);

    // Write style
    if (!appliedStyle_.Empty() && appliedStyle_ != "UIElement")
        dest.Set("style", appliedStyle_);
    else if (internal_)
        dest.Set("style", "none");

    // Write attributes
    if (!Animatable::SaveJSON(dest))
        return false;

    // Write child elements
    JSONArray children;
    for (unsigned i = 0; i < children_.Size(); ++i)
    {
        UIElement* element = children_[i];
        if (element->IsTemporary())
            continue;

        JSONValue childElem;
        if (!element->SaveJSON(childElem))
            return false;

        children.Push(childElem);
    }
    if (!children.Empty())
        dest.Set("children", children);

    // Filter UI-style and implicit attributes
    return FilterAttributes(dest);
}

void UIElement::Update(float timeStep)
{
}

void UIElement::GetBatches(PODVector<UIBatch>& batches, PODVector<float>& vertexData, const IntRect& currentScissor)
{
    // Reset hovering for next frame
    hovering_ = false;
}

void UIElement::GetDebugDrawBatches(PODVector<UIBatch>& batches, PODVector<float>& vertexData, const IntRect& currentScissor)
{
    UIBatch batch(this, BLEND_ALPHA, currentScissor, nullptr, &vertexData);

    int horizontalThickness = 1;
    int verticalThickness = 1;
    if (parent_)
    {
        switch (parent_->layoutMode_)
        {
        case LM_HORIZONTAL:
            verticalThickness += 2;
            break;

        case LM_VERTICAL:
            horizontalThickness += 2;
            break;

        default:
            break;
        }
    }

    batch.SetColor(Color::BLUE, true);
    // Left
    batch.AddQuad(0, 0, horizontalThickness, size_.y_, 0, 0);
    // Top
    batch.AddQuad(0, 0, size_.x_, verticalThickness, 0, 0);
    // Right
    batch.AddQuad(size_.x_ - horizontalThickness, 0, horizontalThickness, size_.y_, 0, 0);
    // Bottom
    batch.AddQuad(0, size_.y_ - verticalThickness, size_.x_, verticalThickness, 0, 0);

    UIBatch::AddOrMerge(batch, batches);
}

bool UIElement::IsWithinScissor(const IntRect& currentScissor)
{
    if (!visible_)
        return false;

    const IntVector2& screenPos = GetScreenPosition();
    return screenPos.x_ < currentScissor.right_ && screenPos.x_ + GetWidth() > currentScissor.left_ &&
           screenPos.y_ < currentScissor.bottom_ && screenPos.y_ + GetHeight() > currentScissor.top_;
}

const IntVector2& UIElement::GetScreenPosition() const
{
    if (positionDirty_)
    {
        IntVector2 pos = position_;
        const UIElement* parent = parent_;

        if (parent)
        {
            const IntVector2& parentScreenPos = parent->GetScreenPosition();

            pos.x_ += parentScreenPos.x_ + (int)Lerp(0.0f, (float)parent->size_.x_, anchorMin_.x_);
            pos.y_ += parentScreenPos.y_ + (int)Lerp(0.0f, (float)parent->size_.y_, anchorMin_.y_);
            pos.x_ -= (int)(size_.x_ * pivot_.x_);
            pos.y_ -= (int)(size_.y_ * pivot_.y_);

            pos += parent_->childOffset_;
        }

        screenPosition_ = pos;
        positionDirty_ = false;
    }

    return screenPosition_;
}

void UIElement::OnHover(const IntVector2& position, const IntVector2& screenPosition, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor)
{
    hovering_ = true;
}

void UIElement::OnDragBegin(const IntVector2& position, const IntVector2& screenPosition, MouseButtonFlags buttons, QualifierFlags qualifiers,
    Cursor* cursor)
{
    dragButtonCombo_ = buttons;
    dragButtonCount_ = CountSetBits((unsigned)dragButtonCombo_);
}

void UIElement::OnDragMove(const IntVector2& position, const IntVector2& screenPosition, const IntVector2& deltaPos, MouseButtonFlags buttons,
    QualifierFlags qualifiers, Cursor* cursor)
{
}

void UIElement::OnDragEnd(const IntVector2& position, const IntVector2& screenPosition, MouseButtonFlags /*dragButtons*/, MouseButtonFlags /*releaseButtons*/,
    Cursor* /*cursor*/)
{
    dragButtonCombo_ = MOUSEB_NONE;
    dragButtonCount_ = 0;
}

void UIElement::OnDragCancel(const IntVector2& position, const IntVector2& screenPosition, MouseButtonFlags /*dragButtons*/, MouseButtonFlags /*cancelButtons*/,
    Cursor* /*cursor*/)
{
    dragButtonCombo_ = MOUSEB_NONE;
    dragButtonCount_ = 0;
}

bool UIElement::OnDragDropTest(UIElement* source)
{
    return true;
}

bool UIElement::OnDragDropFinish(UIElement* source)
{
    return true;
}

IntVector2 UIElement::ScreenToElement(const IntVector2& screenPosition)
{
    return screenPosition - GetScreenPosition();
}

IntVector2 UIElement::ElementToScreen(const IntVector2& position)
{
    return position + GetScreenPosition();
}

bool UIElement::LoadXML(Deserializer& source)
{
    SharedPtr<XMLFile> xml(new XMLFile(context_));
    return xml->Load(source) && LoadXML(xml->GetRoot());
}

bool UIElement::SaveXML(Serializer& dest, const String& indentation) const
{
    SharedPtr<XMLFile> xml(new XMLFile(context_));
    XMLElement rootElem = xml->CreateRoot("element");
    return SaveXML(rootElem) && xml->Save(dest, indentation);
}

bool UIElement::LoadJSON(Deserializer& source)
{
    SharedPtr<JSONFile> json(new JSONFile(context_));
    return json->Load(source) && LoadJSON(json->GetRoot());
}

bool UIElement::SaveJSON(Serializer& dest, const String& indentation) const
{
    SharedPtr<JSONFile> json(new JSONFile(context_));
    JSONValue& rootElem = json->GetRoot();
    return SaveJSON(rootElem) && json->Save(dest, indentation);
}

bool UIElement::FilterAttributes(XMLElement& dest) const
{
    // Filter UI styling attributes
    XMLFile* styleFile = GetDefaultStyle();
    if (styleFile)
    {
        String style = dest.GetAttribute("style");
        if (!style.Empty() && style != "none")
        {
            if (styleXPathQuery_.SetVariable("typeName", style))
            {
                XMLElement styleElem = GetDefaultStyle()->GetRoot().SelectSinglePrepared(styleXPathQuery_);
                if (styleElem && !FilterUIStyleAttributes(dest, styleElem))
                    return false;
            }
        }
    }

    // Filter implicit attributes
    if (!FilterImplicitAttributes(dest))
    {
        URHO3D_LOGERROR("Could not remove implicit attributes");
        return false;
    }

    return true;
}

bool UIElement::FilterAttributes(JSONValue& dest) const
{
    // Filter UI styling attributes
    XMLFile* styleFile = GetDefaultStyle();
    if (styleFile)
    {
        String style = dest["style"].GetString();
        if (!style.Empty() && style != "none")
        {
            if (styleXPathQuery_.SetVariable("typeName", style))
            {
                XMLElement styleElem = GetDefaultStyle()->GetRoot().SelectSinglePrepared(styleXPathQuery_);
                if (styleElem && !FilterUIStyleAttributes(dest, styleElem))
                    return false;
            }
        }
    }

    // Filter implicit attributes
    if (!FilterImplicitAttributes(dest))
    {
        URHO3D_LOGERROR("Could not remove implicit attributes");
        return false;
    }

    return true;
}

void UIElement::SetName(const String& name)
{
    name_ = name;

    using namespace NameChanged;

    VariantMap& eventData = GetEventDataMap();
    eventData[P_ELEMENT] = this;

    SendEvent(E_NAMECHANGED, eventData);
}

void UIElement::SetPosition(const IntVector2& position)
{
    if (position != position_)
    {
        position_ = position;
        OnPositionSet(position);
        MarkDirty();

        using namespace Positioned;

        VariantMap& eventData = GetEventDataMap();
        eventData[P_ELEMENT] = this;
        eventData[P_X] = position.x_;
        eventData[P_Y] = position.y_;
        SendEvent(E_POSITIONED, eventData);
    }
}

void UIElement::SetPosition(int x, int y)
{
    SetPosition(IntVector2(x, y));
}

void UIElement::SetSize(const IntVector2& size)
{
    ++resizeNestingLevel_;

    IntVector2 oldSize = size_;
    IntVector2 validatedSize;
    IntVector2 effectiveMinSize = GetEffectiveMinSize();
    validatedSize.x_ = Clamp(size.x_, effectiveMinSize.x_, maxSize_.x_);
    validatedSize.y_ = Clamp(size.y_, effectiveMinSize.y_, maxSize_.y_);

    if (validatedSize != size_)
    {
        size_ = validatedSize;

        if (resizeNestingLevel_ == 1)
        {
            // Check if parent element's layout needs to be updated first
            if (parent_)
                parent_->UpdateLayout();

            IntVector2 delta = size_ - oldSize;
            MarkDirty();
            OnResize(size_, delta);
            UpdateLayout();

            using namespace Resized;

            VariantMap& eventData = GetEventDataMap();
            eventData[P_ELEMENT] = this;
            eventData[P_WIDTH] = size_.x_;
            eventData[P_HEIGHT] = size_.y_;
            eventData[P_DX] = delta.x_;
            eventData[P_DY] = delta.y_;
            SendEvent(E_RESIZED, eventData);
        }
    }

    --resizeNestingLevel_;
}

void UIElement::SetSize(int width, int height)
{
    SetSize(IntVector2(width, height));
}

void UIElement::SetWidth(int width)
{
    SetSize(IntVector2(width, size_.y_));
}

void UIElement::SetHeight(int height)
{
    SetSize(IntVector2(size_.x_, height));
}

void UIElement::SetMinSize(const IntVector2& minSize)
{
    minSize_.x_ = Max(minSize.x_, 0);
    minSize_.y_ = Max(minSize.y_, 0);
    SetSize(size_);
}

void UIElement::SetMinSize(int width, int height)
{
    SetMinSize(IntVector2(width, height));
}

void UIElement::SetMinWidth(int width)
{
    SetMinSize(IntVector2(width, minSize_.y_));
}

void UIElement::SetMinHeight(int height)
{
    SetMinSize(IntVector2(minSize_.x_, height));
}

void UIElement::SetMaxSize(const IntVector2& maxSize)
{
    maxSize_.x_ = Max(maxSize.x_, 0);
    maxSize_.y_ = Max(maxSize.y_, 0);
    SetSize(size_);
}

void UIElement::SetMaxSize(int width, int height)
{
    SetMaxSize(IntVector2(width, height));
}

void UIElement::SetMaxWidth(int width)
{
    SetMaxSize(IntVector2(width, maxSize_.y_));
}

void UIElement::SetMaxHeight(int height)
{
    SetMaxSize(IntVector2(maxSize_.x_, height));
}

void UIElement::SetFixedSize(const IntVector2& size)
{
    minSize_ = maxSize_ = IntVector2(Max(size.x_, 0), Max(size.y_, 0));
    SetSize(size);
}

void UIElement::SetFixedSize(int width, int height)
{
    SetFixedSize(IntVector2(width, height));
}

void UIElement::SetFixedWidth(int width)
{
    minSize_.x_ = maxSize_.x_ = Max(width, 0);
    SetWidth(width);
}

void UIElement::SetFixedHeight(int height)
{
    minSize_.y_ = maxSize_.y_ = Max(height, 0);
    SetHeight(height);
}

void UIElement::SetAlignment(HorizontalAlignment hAlign, VerticalAlignment vAlign)
{
    SetHorizontalAlignment(hAlign);
    SetVerticalAlignment(vAlign);
}

void UIElement::SetHorizontalAlignment(HorizontalAlignment align)
{
    if (align != HA_LEFT && parent_ && parent_->GetLayoutMode() == LM_HORIZONTAL)
    {
        URHO3D_LOGWARNING("Forcing left alignment because parent element has horizontal layout");
        align = HA_LEFT;
    }

    Vector2 min = anchorMin_;
    Vector2 max = anchorMax_;
    float pivot = pivot_.x_;
    float anchorSize = max.x_ - min.x_;

    if (align == HA_CENTER)
        min.x_ = pivot = 0.5f;
    else if (align == HA_LEFT)
        min.x_ = pivot = 0.0f;
    else if (align == HA_RIGHT)
        min.x_ = pivot = 1.0f;

    max.x_ = enableAnchor_ ? (min.x_ + anchorSize) : min.x_;

    if (min.x_ != anchorMin_.x_ || max.x_ != anchorMax_.x_ || pivot != pivot_.x_)
    {
        anchorMin_.x_ = min.x_;
        anchorMax_.x_ = max.x_;
        pivot_.x_ = pivot;
        if (enableAnchor_)
            UpdateAnchoring();
        MarkDirty();
    }
}

void UIElement::SetVerticalAlignment(VerticalAlignment align)
{
    if (align != VA_TOP && parent_ && parent_->GetLayoutMode() == LM_VERTICAL)
    {
        URHO3D_LOGWARNING("Forcing top alignment because parent element has vertical layout");
        align = VA_TOP;
    }

    Vector2 min = anchorMin_;
    Vector2 max = anchorMax_;
    float pivot = pivot_.y_;
    float anchorSize = max.y_ - min.y_;

    if (align == VA_CENTER)
        min.y_ = pivot = 0.5f;
    else if (align == VA_TOP)
        min.y_ = pivot = 0.0f;
    else if (align == VA_BOTTOM)
        min.y_ = pivot = 1.0f;

    max.y_ = enableAnchor_ ? (min.y_ + anchorSize) : min.y_;

    if (min.y_ != anchorMin_.y_ || max.y_ != anchorMax_.y_ || pivot != pivot_.y_)
    {
        anchorMin_.y_ = min.y_;
        anchorMax_.y_ = max.y_;
        pivot_.y_ = pivot;
        if (enableAnchor_)
            UpdateAnchoring();
        MarkDirty();
    }
}

void UIElement::SetEnableAnchor(bool enable)
{
    enableAnchor_ = enable;
    if (enableAnchor_)
        UpdateAnchoring();
}

void UIElement::SetMinOffset(const IntVector2& offset)
{
    if (offset != minOffset_)
    {
        minOffset_ = offset;
        if (enableAnchor_)
            UpdateAnchoring();
    }
}

void UIElement::SetMaxOffset(const IntVector2& offset)
{
    if (offset != maxOffset_)
    {
        maxOffset_ = offset;
        if (enableAnchor_)
            UpdateAnchoring();
    }
}

void UIElement::SetMinAnchor(const Vector2& anchor)
{
    if (anchor != anchorMin_)
    {
        anchorMin_ = anchor;
        if (enableAnchor_)
            UpdateAnchoring();
    }
}

void UIElement::SetMinAnchor(float x, float y)
{
    SetMinAnchor(Vector2(x, y));
}

void UIElement::SetMaxAnchor(const Vector2& anchor)
{
    if (anchor != anchorMax_)
    {
        anchorMax_ = anchor;
        if (enableAnchor_)
            UpdateAnchoring();
    }
}

void UIElement::SetMaxAnchor(float x, float y)
{
    SetMaxAnchor(Vector2(x, y));
}

void UIElement::SetPivot(const Vector2& pivot)
{
    if (pivot != pivot_)
    {
        pivotSet_ = true;
        pivot_ = pivot;
        MarkDirty();
    }
}

void UIElement::SetPivot(float x, float y)
{
    SetPivot(Vector2(x, y));
}

void UIElement::SetClipBorder(const IntRect& rect)
{
    clipBorder_.left_ = Max(rect.left_, 0);
    clipBorder_.top_ = Max(rect.top_, 0);
    clipBorder_.right_ = Max(rect.right_, 0);
    clipBorder_.bottom_ = Max(rect.bottom_, 0);
}

void UIElement::SetColor(const Color& color)
{
    for (auto& cornerColor : colors_)
        cornerColor = color;
    colorGradient_ = false;
    derivedColorDirty_ = true;
}

void UIElement::SetColor(Corner corner, const Color& color)
{
    colors_[corner] = color;
    colorGradient_ = false;
    derivedColorDirty_ = true;

    for (unsigned i = 0; i < MAX_UIELEMENT_CORNERS; ++i)
    {
        if (i != corner && colors_[i] != colors_[corner])
            colorGradient_ = true;
    }
}

void UIElement::SetPriority(int priority)
{
    if (priority_ == priority)
        return;

    priority_ = priority;
    if (parent_)
        parent_->sortOrderDirty_ = true;
}

void UIElement::SetOpacity(float opacity)
{
    opacity_ = Clamp(opacity, 0.0f, 1.0f);
    MarkDirty();
}

void UIElement::SetBringToFront(bool enable)
{
    bringToFront_ = enable;
}

void UIElement::SetBringToBack(bool enable)
{
    bringToBack_ = enable;
}

void UIElement::SetClipChildren(bool enable)
{
    clipChildren_ = enable;
}

void UIElement::SetSortChildren(bool enable)
{
    if (!sortChildren_ && enable)
        sortOrderDirty_ = true;

    sortChildren_ = enable;
}

void UIElement::SetUseDerivedOpacity(bool enable)
{
    useDerivedOpacity_ = enable;
}

void UIElement::SetEnabled(bool enable)
{
    enabled_ = enable;
    enabledPrev_ = enable;
}

void UIElement::SetDeepEnabled(bool enable)
{
    enabled_ = enable;

    for (Vector<SharedPtr<UIElement> >::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
        (*i)->SetDeepEnabled(enable);
}

void UIElement::ResetDeepEnabled()
{
    enabled_ = enabledPrev_;

    for (Vector<SharedPtr<UIElement> >::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
        (*i)->ResetDeepEnabled();
}

void UIElement::SetEnabledRecursive(bool enable)
{
    enabled_ = enable;
    enabledPrev_ = enable;

    for (Vector<SharedPtr<UIElement> >::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
        (*i)->SetEnabledRecursive(enable);
}

void UIElement::SetEditable(bool enable)
{
    editable_ = enable;
    OnSetEditable();
}

void UIElement::SetFocusMode(FocusMode mode)
{
    focusMode_ = mode;
}

void UIElement::SetFocus(bool enable)
{
    // Invisible elements should not receive focus
    if (focusMode_ < FM_FOCUSABLE || !IsVisibleEffective())
        enable = false;

    auto* ui = GetSubsystem<UI>();
    // Can be null at exit time; no-op in that case
    if (!ui)
        return;

    if (enable)
    {
        if (ui->GetFocusElement() != this)
            ui->SetFocusElement(this);
    }
    else
    {
        if (ui->GetFocusElement() == this)
            ui->SetFocusElement(nullptr);
    }
}

void UIElement::SetSelected(bool enable)
{
    selected_ = enable;
}

void UIElement::SetVisible(bool enable)
{
    auto* ui = GetSubsystem<UI>();
    // Can be null at exit time; no-op in that case
    if (!ui)
        return;

    if (enable != visible_)
    {
        visible_ = enable;

        // Parent's layout may change as a result of visibility change
        if (parent_)
            parent_->UpdateLayout();

        using namespace VisibleChanged;

        VariantMap& eventData = GetEventDataMap();
        eventData[P_ELEMENT] = this;
        eventData[P_VISIBLE] = visible_;
        SendEvent(E_VISIBLECHANGED, eventData);

        // If the focus element becomes effectively hidden, clear focus
        if (!enable)
        {
            UIElement* focusElement = ui->GetFocusElement();
            if (focusElement && !focusElement->IsVisibleEffective())
                focusElement->SetFocus(false);
        }
    }
}

void UIElement::SetDragDropMode(DragAndDropModeFlags mode)
{
    dragDropMode_ = mode;
}

bool UIElement::SetStyle(const String& styleName, XMLFile* file)
{
    // If empty style was requested, replace with type name
    String actualStyleName = !styleName.Empty() ? styleName : GetTypeName();

    appliedStyle_ = actualStyleName;
    if (styleName == "none")
        return true;

    if (!file)
    {
        file = GetDefaultStyle();
        if (!file)
            return false;
    }
    else
    {
        // If a custom style file specified, remember it
        defaultStyle_ = file;
    }

    // Remember the effectively applied style file, either custom or default
    appliedStyleFile_ = file;

    styleXPathQuery_.SetVariable("typeName", actualStyleName);
    XMLElement styleElem = file->GetRoot().SelectSinglePrepared(styleXPathQuery_);
    return styleElem && SetStyle(styleElem);
}

bool UIElement::SetStyle(const XMLElement& element)
{
    appliedStyle_ = element.GetAttribute("type");

    // Consider style attribute values as instance-level attribute default values
    SetInstanceDefault(true);
    bool success = LoadXML(element);
    SetInstanceDefault(false);
    return success;
}

bool UIElement::SetStyleAuto(XMLFile* file)
{
    return SetStyle(String::EMPTY, file);
}

void UIElement::SetDefaultStyle(XMLFile* style)
{
    defaultStyle_ = style;
}

void UIElement::SetLayout(LayoutMode mode, int spacing, const IntRect& border)
{
    layoutMode_ = mode;
    layoutSpacing_ = Max(spacing, 0);
    layoutBorder_ = IntRect(Max(border.left_, 0), Max(border.top_, 0), Max(border.right_, 0), Max(border.bottom_, 0));
    VerifyChildAlignment();
    UpdateLayout();
}

void UIElement::SetLayoutMode(LayoutMode mode)
{
    layoutMode_ = mode;
    VerifyChildAlignment();
    UpdateLayout();
}

void UIElement::SetLayoutSpacing(int spacing)
{
    layoutSpacing_ = Max(spacing, 0);
    UpdateLayout();
}

void UIElement::SetLayoutBorder(const IntRect& border)
{
    layoutBorder_ = IntRect(Max(border.left_, 0), Max(border.top_, 0), Max(border.right_, 0), Max(border.bottom_, 0));
    UpdateLayout();
}

void UIElement::SetLayoutFlexScale(const Vector2& scale)
{
    layoutFlexScale_ = Vector2(Max(scale.x_, 0.0f), Max(scale.y_, 0.0f));
}

void UIElement::SetIndent(int indent)
{
    indent_ = indent;
    if (parent_)
        parent_->UpdateLayout();
    UpdateLayout();
    OnIndentSet();
}

void UIElement::SetIndentSpacing(int indentSpacing)
{
    indentSpacing_ = Max(indentSpacing, 0);
    if (parent_)
        parent_->UpdateLayout();
    UpdateLayout();
    OnIndentSet();
}

void UIElement::UpdateLayout()
{
    if (layoutNestingLevel_)
        return;

    // Prevent further updates while this update happens
    DisableLayoutUpdate();

    PODVector<int> positions;
    PODVector<int> sizes;
    PODVector<int> minSizes;
    PODVector<int> maxSizes;
    PODVector<float> flexScales;

    int baseIndentWidth = GetIndentWidth();

    if (layoutMode_ == LM_HORIZONTAL)
    {
        int minChildHeight = 0;

        for (unsigned i = 0; i < children_.Size(); ++i)
        {
            if (!children_[i]->IsVisible())
                continue;
            positions.Push(baseIndentWidth);
            auto indent = (unsigned)children_[i]->GetIndentWidth();
            sizes.Push(children_[i]->GetWidth() + indent);
            minSizes.Push(children_[i]->GetEffectiveMinSize().x_ + indent);
            maxSizes.Push(children_[i]->GetMaxWidth() + indent);
            flexScales.Push(children_[i]->GetLayoutFlexScale().x_);
            minChildHeight = Max(minChildHeight, children_[i]->GetEffectiveMinSize().y_);
        }

        CalculateLayout(positions, sizes, minSizes, maxSizes, flexScales, GetWidth(), layoutBorder_.left_, layoutBorder_.right_,
            layoutSpacing_);

        int width = CalculateLayoutParentSize(sizes, layoutBorder_.left_, layoutBorder_.right_, layoutSpacing_);
        int height = Max(GetHeight(), minChildHeight + layoutBorder_.top_ + layoutBorder_.bottom_);
        int minWidth = CalculateLayoutParentSize(minSizes, layoutBorder_.left_, layoutBorder_.right_, layoutSpacing_);
        int minHeight = minChildHeight + layoutBorder_.top_ + layoutBorder_.bottom_;
        layoutMinSize_ = IntVector2(minWidth, minHeight);
        SetSize(width, height);
        // Validate the size before resizing child elements, in case of min/max limits
        width = size_.x_;
        height = size_.y_;

        unsigned j = 0;
        for (unsigned i = 0; i < children_.Size(); ++i)
        {
            if (!children_[i]->IsVisible())
                continue;
            children_[i]->SetPosition(positions[j], GetLayoutChildPosition(children_[i]).y_);
            children_[i]->SetSize(sizes[j], height - layoutBorder_.top_ - layoutBorder_.bottom_);
            ++j;
        }
    }
    else if (layoutMode_ == LM_VERTICAL)
    {
        int minChildWidth = 0;

        for (unsigned i = 0; i < children_.Size(); ++i)
        {
            if (!children_[i]->IsVisible())
                continue;
            positions.Push(0);
            sizes.Push(children_[i]->GetHeight());
            minSizes.Push(children_[i]->GetEffectiveMinSize().y_);
            maxSizes.Push(children_[i]->GetMaxHeight());
            flexScales.Push(children_[i]->GetLayoutFlexScale().y_);
            minChildWidth = Max(minChildWidth, children_[i]->GetEffectiveMinSize().x_ + children_[i]->GetIndentWidth());
        }

        CalculateLayout(positions, sizes, minSizes, maxSizes, flexScales, GetHeight(), layoutBorder_.top_, layoutBorder_.bottom_,
            layoutSpacing_);

        int height = CalculateLayoutParentSize(sizes, layoutBorder_.top_, layoutBorder_.bottom_, layoutSpacing_);
        int width = Max(GetWidth(), minChildWidth + layoutBorder_.left_ + layoutBorder_.right_);
        int minHeight = CalculateLayoutParentSize(minSizes, layoutBorder_.top_, layoutBorder_.bottom_, layoutSpacing_);
        int minWidth = minChildWidth + layoutBorder_.left_ + layoutBorder_.right_;
        layoutMinSize_ = IntVector2(minWidth, minHeight);
        SetSize(width, height);
        width = size_.x_;
        height = size_.y_;

        unsigned j = 0;
        for (unsigned i = 0; i < children_.Size(); ++i)
        {
            if (!children_[i]->IsVisible())
                continue;
            children_[i]->SetPosition(GetLayoutChildPosition(children_[i]).x_ + baseIndentWidth, positions[j]);
            children_[i]->SetSize(width - layoutBorder_.left_ - layoutBorder_.right_, sizes[j]);
            ++j;
        }
    }
    else
    {
        for (unsigned i = 0; i < children_.Size(); ++i)
        {
            if (children_[i]->GetEnableAnchor())
                children_[i]->UpdateAnchoring();
        }
    }

    using namespace LayoutUpdated;

    VariantMap& eventData = GetEventDataMap();
    eventData[P_ELEMENT] = this;
    SendEvent(E_LAYOUTUPDATED, eventData);

    EnableLayoutUpdate();
}

void UIElement::DisableLayoutUpdate()
{
    ++layoutNestingLevel_;
}

void UIElement::EnableLayoutUpdate()
{
    --layoutNestingLevel_;
}

void UIElement::BringToFront()
{
    // Follow the parent chain to the top level window. If it has BringToFront mode, bring it to front now
    UIElement* root = GetRoot();
    // If element is detached from hierarchy, this must be a no-op
    if (!root)
        return;

    UIElement* ptr = this;
    while (ptr && ptr->GetParent() != root)
        ptr = ptr->GetParent();
    if (!ptr || !ptr->GetBringToFront())
        return;

    // Get the highest priority used by all other top level elements, assign that to the new front element
    // and decrease others' priority where necessary. However, take into account only input-enabled
    // elements and those which have the BringToBack flag set
    HashSet<int> usedPriorities;

    int maxPriority = M_MIN_INT;
    const Vector<SharedPtr<UIElement> >& rootChildren = root->GetChildren();
    for (Vector<SharedPtr<UIElement> >::ConstIterator i = rootChildren.Begin(); i != rootChildren.End(); ++i)
    {
        UIElement* other = *i;
        if (other->IsEnabled() && other->bringToBack_ && other != ptr)
        {
            int priority = other->GetPriority();
            // M_MAX_INT is used by popups and tooltips. Disregard these to avoid an "arms race" with the priorities
            if (priority == M_MAX_INT)
                continue;
            usedPriorities.Insert(priority);
            maxPriority = Max(priority, maxPriority);
        }
    }

    if (maxPriority != M_MIN_INT && maxPriority >= ptr->GetPriority())
    {
        ptr->SetPriority(maxPriority);

        int minPriority = maxPriority;
        while (usedPriorities.Contains(minPriority))
            --minPriority;

        for (Vector<SharedPtr<UIElement> >::ConstIterator i = rootChildren.Begin(); i != rootChildren.End(); ++i)
        {
            UIElement* other = *i;
            int priority = other->GetPriority();

            if (other->IsEnabled() && other->bringToBack_ && other != ptr && priority >= minPriority && priority <= maxPriority)
                other->SetPriority(priority - 1);
        }
    }
}

UIElement* UIElement::CreateChild(StringHash type, const String& name, unsigned index)
{
    // Check that creation succeeds and that the object in fact is a UI element
    SharedPtr<UIElement> newElement = DynamicCast<UIElement>(context_->CreateObject(type));
    if (!newElement)
    {
        URHO3D_LOGERROR("Could not create unknown UI element type " + type.ToString());
        return nullptr;
    }

    if (!name.Empty())
        newElement->SetName(name);

    InsertChild(index, newElement);
    return newElement;
}

void UIElement::AddChild(UIElement* element)
{
    InsertChild(M_MAX_UNSIGNED, element);
}

void UIElement::InsertChild(unsigned index, UIElement* element)
{
    // Check for illegal or redundant parent assignment
    if (!element || element == this || element->parent_ == this)
        return;
    // Check for possible cyclic parent assignment
    UIElement* parent = parent_;
    while (parent)
    {
        if (parent == element)
            return;
        parent = parent->parent_;
    }

    // Add first, then remove from old parent, to ensure the element does not get deleted
    if (index >= children_.Size())
        children_.Push(SharedPtr<UIElement>(element));
    else
        children_.Insert(children_.Begin() + index, SharedPtr<UIElement>(element));

    element->Remove();

    if (sortChildren_)
        sortOrderDirty_ = true;

    element->parent_ = this;
    element->MarkDirty();

    // Apply style now if child element (and its children) has it defined
    ApplyStyleRecursive(element);

    VerifyChildAlignment();
    UpdateLayout();

    // Send change event
    UIElement* root = GetRoot();
    UIElement* sender = GetElementEventSender();
    if (sender)
    {
        using namespace ElementAdded;

        VariantMap& eventData = GetEventDataMap();
        eventData[P_ROOT] = root;
        eventData[P_PARENT] = this;
        eventData[P_ELEMENT] = element;

        sender->SendEvent(E_ELEMENTADDED, eventData);
    }
}

void UIElement::RemoveChild(UIElement* element, unsigned index)
{
    for (unsigned i = index; i < children_.Size(); ++i)
    {
        if (children_[i] == element)
        {
            // Send change event if not already being destroyed
            UIElement* sender = Refs() > 0 ? GetElementEventSender() : nullptr;
            if (sender)
            {
                using namespace ElementRemoved;

                VariantMap& eventData = GetEventDataMap();
                eventData[P_ROOT] = GetRoot();
                eventData[P_PARENT] = this;
                eventData[P_ELEMENT] = element;

                sender->SendEvent(E_ELEMENTREMOVED, eventData);
            }

            element->Detach();
            children_.Erase(i);
            UpdateLayout();
            return;
        }
    }
}

void UIElement::RemoveChildAtIndex(unsigned index)
{
    if (index >= children_.Size())
        return;

    // Send change event if not already being destroyed
    UIElement* sender = Refs() > 0 ? GetElementEventSender() : nullptr;
    if (sender)
    {
        using namespace ElementRemoved;

        VariantMap& eventData = GetEventDataMap();
        eventData[P_ROOT] = GetRoot();
        eventData[P_PARENT] = this;
        eventData[P_ELEMENT] = children_[index];

        sender->SendEvent(E_ELEMENTREMOVED, eventData);
    }

    children_[index]->Detach();
    children_.Erase(index);
    UpdateLayout();
}

void UIElement::RemoveAllChildren()
{
    UIElement* root = GetRoot();
    UIElement* sender = Refs() > 0 ? GetElementEventSender() : nullptr;

    for (Vector<SharedPtr<UIElement> >::Iterator i = children_.Begin(); i < children_.End();)
    {
        // Send change event if not already being destroyed
        if (sender)
        {
            using namespace ElementRemoved;

            VariantMap& eventData = GetEventDataMap();
            eventData[P_ROOT] = root;
            eventData[P_PARENT] = this;
            eventData[P_ELEMENT] = (*i).Get();

            sender->SendEvent(E_ELEMENTREMOVED, eventData);
        }

        (*i++)->Detach();
    }
    children_.Clear();
    UpdateLayout();
}

void UIElement::Remove()
{
    if (parent_)
        parent_->RemoveChild(this);
}

unsigned UIElement::FindChild(UIElement* element) const
{
    Vector<SharedPtr<UIElement> >::ConstIterator i = children_.Find(SharedPtr<UIElement>(element));
    return i != children_.End() ? (unsigned)(i - children_.Begin()) : M_MAX_UNSIGNED;
}

void UIElement::SetParent(UIElement* parent, unsigned index)
{
    if (parent)
        parent->InsertChild(index, this);
}

void UIElement::SetVar(StringHash key, const Variant& value)
{
    vars_[key] = value;
}

void UIElement::SetInternal(bool enable)
{
    internal_ = enable;
}

void UIElement::SetTraversalMode(TraversalMode traversalMode)
{
    traversalMode_ = traversalMode;
}

void UIElement::SetElementEventSender(bool flag)
{
    elementEventSender_ = flag;
}

void UIElement::SetTags(const StringVector& tags)
{
    RemoveAllTags();
    AddTags(tags);
}

void UIElement::AddTag(const String& tag)
{
    if (tag.Empty() || HasTag(tag))
        return;

    tags_.Push(tag);
}

void UIElement::AddTags(const String& tags, char separator)
{
    StringVector tagVector = tags.Split(separator);
    AddTags(tagVector);
}

void UIElement::AddTags(const StringVector& tags)
{
    for (unsigned i = 0; i < tags.Size(); ++i)
        AddTag(tags[i]);
}

bool UIElement::RemoveTag(const String& tag)
{
    return tags_.Remove(tag);
}

void UIElement::RemoveAllTags()
{
    tags_.Clear();
}

HorizontalAlignment UIElement::GetHorizontalAlignment() const
{
    if (anchorMin_.x_ == 0.0f && anchorMax_.x_ == 0.0f && (!pivotSet_ || pivot_.x_ == 0.0f))
        return HA_LEFT;
    else if (anchorMin_.x_ == 0.5f && anchorMax_.x_ == 0.5f && (!pivotSet_ || pivot_.x_ == 0.5f))
        return HA_CENTER;
    else if (anchorMin_.x_ == 1.0f && anchorMax_.x_ == 1.0f && (!pivotSet_ || pivot_.x_ == 1.0f))
        return HA_RIGHT;

    return HA_CUSTOM;
}

VerticalAlignment UIElement::GetVerticalAlignment() const
{
    if (anchorMin_.y_ == 0.0f && anchorMax_.y_ == 0.0f && (!pivotSet_ || pivot_.y_ == 0.0f))
        return VA_TOP;
    else if (anchorMin_.y_ == 0.5f && anchorMax_.y_ == 0.5f && (!pivotSet_ || pivot_.y_ == 0.5f))
        return VA_CENTER;
    else if (anchorMin_.y_ == 1.0f && anchorMax_.y_ == 1.0f && (!pivotSet_ || pivot_.y_ == 1.0f))
        return VA_BOTTOM;

    return VA_CUSTOM;
}

float UIElement::GetDerivedOpacity() const
{
    if (!useDerivedOpacity_)
        return opacity_;

    if (opacityDirty_)
    {
        derivedOpacity_ = opacity_;
        const UIElement* parent = parent_;

        while (parent)
        {
            derivedOpacity_ *= parent->opacity_;
            parent = parent->parent_;
        }

        opacityDirty_ = false;
    }

    return derivedOpacity_;
}

bool UIElement::HasFocus() const
{
    auto* ui = GetSubsystem<UI>();
    return ui ? ui->GetFocusElement() == this : false;
}

bool UIElement::IsChildOf(UIElement* element) const
{
    UIElement* parent = parent_;
    while (parent)
    {
        if (parent == element)
            return true;
        parent = parent->parent_;
    }
    return false;
}

bool UIElement::IsVisibleEffective() const
{
    bool visible = visible_;
    const UIElement* element = parent_;

    // Traverse the parent chain
    while (visible && element)
    {
        visible &= element->visible_;
        element = element->parent_;
    }

    return visible;
}

const String& UIElement::GetAppliedStyle() const
{
    return appliedStyle_ == GetTypeName() ? String::EMPTY : appliedStyle_;
}

XMLFile* UIElement::GetDefaultStyle(bool recursiveUp) const
{
    if (recursiveUp)
    {
        const UIElement* element = this;
        while (element)
        {
            if (element->defaultStyle_)
                return element->defaultStyle_;
            element = element->parent_;
        }
        return nullptr;
    }
    else
        return defaultStyle_;
}

void UIElement::GetChildren(PODVector<UIElement*>& dest, bool recursive) const
{
    dest.Clear();

    if (!recursive)
    {
        dest.Reserve(children_.Size());
        for (Vector<SharedPtr<UIElement> >::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
            dest.Push(*i);
    }
    else
        GetChildrenRecursive(dest);
}

PODVector<UIElement*> UIElement::GetChildren(bool recursive) const
{
    PODVector<UIElement*> dest;
    GetChildren(dest, recursive);
    return dest;
}

unsigned UIElement::GetNumChildren(bool recursive) const
{
    if (!recursive)
        return children_.Size();
    else
    {
        unsigned allChildren = children_.Size();
        for (Vector<SharedPtr<UIElement> >::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
            allChildren += (*i)->GetNumChildren(true);

        return allChildren;
    }
}

UIElement* UIElement::GetChild(unsigned index) const
{
    return index < children_.Size() ? children_[index] : nullptr;
}

UIElement* UIElement::GetChild(const String& name, bool recursive) const
{
    for (Vector<SharedPtr<UIElement> >::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
    {
        if ((*i)->name_ == name)
            return *i;

        if (recursive)
        {
            UIElement* element = (*i)->GetChild(name, true);
            if (element)
                return element;
        }
    }

    return nullptr;
}

UIElement* UIElement::GetChild(const StringHash& key, const Variant& value, bool recursive) const
{
    for (Vector<SharedPtr<UIElement> >::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
    {
        const Variant& varValue = (*i)->GetVar(key);
        if (value != Variant::EMPTY ? varValue == value : varValue != Variant::EMPTY)
            return *i;

        if (recursive)
        {
            UIElement* element = (*i)->GetChild(key, value, true);
            if (element)
                return element;
        }
    }

    return nullptr;
}

UIElement* UIElement::GetRoot() const
{
    UIElement* root = parent_;
    if (!root)
        return nullptr;
    while (root->GetParent())
        root = root->GetParent();
    return root;
}

const Color& UIElement::GetDerivedColor() const
{
    if (derivedColorDirty_)
    {
        derivedColor_ = colors_[C_TOPLEFT];
        derivedColor_.a_ *= GetDerivedOpacity();
        derivedColorDirty_ = false;
    }

    return derivedColor_;
}

const Variant& UIElement::GetVar(const StringHash& key) const
{
    VariantMap::ConstIterator i = vars_.Find(key);
    return i != vars_.End() ? i->second_ : Variant::EMPTY;
}

bool UIElement::HasTag(const String& tag) const
{
    return tags_.Contains(tag);
}

void UIElement::GetChildrenWithTag(PODVector<UIElement*>& dest, const String& tag, bool recursive) const
{
    dest.Clear();

    if (!recursive)
    {
        for (Vector<SharedPtr<UIElement> >::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
        {
            UIElement* element = *i;
            if (element->HasTag(tag))
                dest.Push(element);
        }
    }
    else
        GetChildrenWithTagRecursive(dest, tag);
}

PODVector<UIElement*> UIElement::GetChildrenWithTag(const String& tag, bool recursive) const
{
    PODVector<UIElement*> dest;
    GetChildrenWithTag(dest, tag, recursive);
    return dest;
}

void UIElement::GetChildrenWithTagRecursive(PODVector<UIElement*>& dest, const String& tag) const
{
    for (Vector<SharedPtr<UIElement> >::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
    {
        UIElement* element = *i;
        if (element->HasTag(tag))
            dest.Push(element);
        if (!element->children_.Empty())
            element->GetChildrenWithTagRecursive(dest, tag);
    }
}

bool UIElement::IsInside(IntVector2 position, bool isScreen)
{
    if (isScreen)
        position = ScreenToElement(position);
    return position.x_ >= 0 && position.y_ >= 0 && position.x_ < size_.x_ && position.y_ < size_.y_;
}

bool UIElement::IsInsideCombined(IntVector2 position, bool isScreen)
{
    // If child elements are clipped, no need to expand the rect
    if (clipChildren_)
        return IsInside(position, isScreen);

    if (!isScreen)
        position = ElementToScreen(position);

    IntRect combined = GetCombinedScreenRect();
    return position.x_ >= combined.left_ && position.y_ >= combined.top_ && position.x_ < combined.right_ &&
        position.y_ < combined.bottom_;
}

IntRect UIElement::GetCombinedScreenRect()
{
    IntVector2 screenPosition(GetScreenPosition());
    IntRect combined(screenPosition.x_, screenPosition.y_, screenPosition.x_ + size_.x_, screenPosition.y_ + size_.y_);

    if (!clipChildren_)
    {
        for (Vector<SharedPtr<UIElement> >::Iterator i = children_.Begin(); i != children_.End(); ++i)
        {
            IntRect childCombined((*i)->GetCombinedScreenRect());

            if (childCombined.left_ < combined.left_)
                combined.left_ = childCombined.left_;
            if (childCombined.right_ > combined.right_)
                combined.right_ = childCombined.right_;
            if (childCombined.top_ < combined.top_)
                combined.top_ = childCombined.top_;
            if (childCombined.bottom_ > combined.bottom_)
                combined.bottom_ = childCombined.bottom_;
        }
    }

    return combined;
}

void UIElement::SortChildren()
{
    if (sortChildren_ && sortOrderDirty_)
    {
        // Only sort when there is no layout
        /// \todo Order is not stable when children have same priorities
        if (layoutMode_ == LM_FREE)
            Sort(children_.Begin(), children_.End(), CompareUIElements);
        sortOrderDirty_ = false;
    }
}

void UIElement::SetChildOffset(const IntVector2& offset)
{
    if (offset != childOffset_)
    {
        childOffset_ = offset;
        for (Vector<SharedPtr<UIElement> >::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
            (*i)->MarkDirty();
    }
}

void UIElement::SetHovering(bool enable)
{
    hovering_ = enable;
}

void UIElement::AdjustScissor(IntRect& currentScissor)
{
    if (clipChildren_)
    {
        IntVector2 screenPos = GetScreenPosition();
        currentScissor.left_ = Max(currentScissor.left_, screenPos.x_ + clipBorder_.left_);
        currentScissor.top_ = Max(currentScissor.top_, screenPos.y_ + clipBorder_.top_);
        currentScissor.right_ = Min(currentScissor.right_, screenPos.x_ + size_.x_ - clipBorder_.right_);
        currentScissor.bottom_ = Min(currentScissor.bottom_, screenPos.y_ + size_.y_ - clipBorder_.bottom_);

        if (currentScissor.right_ < currentScissor.left_)
            currentScissor.right_ = currentScissor.left_;
        if (currentScissor.bottom_ < currentScissor.top_)
            currentScissor.bottom_ = currentScissor.top_;
    }
}

void UIElement::GetBatchesWithOffset(IntVector2& offset, PODVector<UIBatch>& batches, PODVector<float>& vertexData,
    IntRect currentScissor)
{
    Vector2 floatOffset((float)offset.x_, (float)offset.y_);
    unsigned initialSize = vertexData.Size();

    GetBatches(batches, vertexData, currentScissor);
    for (unsigned i = initialSize; i < vertexData.Size(); i += 6)
    {
        vertexData[i] += floatOffset.x_;
        vertexData[i + 1] += floatOffset.y_;
    }

    AdjustScissor(currentScissor);
    for (Vector<SharedPtr<UIElement> >::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
    {
        if ((*i)->IsVisible())
            (*i)->GetBatchesWithOffset(offset, batches, vertexData, currentScissor);
    }
}

UIElement* UIElement::GetElementEventSender() const
{
    auto* element = const_cast<UIElement*>(this);
    if (elementEventSender_)
        return element;

    while (element->parent_)
    {
        element = element->parent_;
        if (element->elementEventSender_)
            return element;
    }

    // If no predefined element event sender in the parental chain, return ultimate root element
    return element;
}

IntVector2 UIElement::GetEffectiveMinSize() const
{
    if (IsFixedSize() || layoutMode_ == LM_FREE || layoutMinSize_ == IntVector2::ZERO)
        return minSize_;
    else
        return IntVector2(Max(minSize_.x_, layoutMinSize_.x_), Max(minSize_.y_, layoutMinSize_.y_));
}

void UIElement::OnAttributeAnimationAdded()
{
    if (attributeAnimationInfos_.Size() == 1)
        SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(UIElement, HandlePostUpdate));
}

void UIElement::OnAttributeAnimationRemoved()
{
    if (attributeAnimationInfos_.Empty())
        UnsubscribeFromEvent(E_POSTUPDATE);
}

Animatable* UIElement::FindAttributeAnimationTarget(const String& name, String& outName)
{
    Vector<String> names = name.Split('/');
    // Only attribute name
    if (names.Size() == 1)
    {
        outName = name;
        return this;
    }
    else
    {
        // Name must in following format: "#0/#1/attribute"
        UIElement* element = this;
        for (unsigned i = 0; i < names.Size() - 1; ++i)
        {
            if (names[i].Front() != '#')
            {
                URHO3D_LOGERROR("Invalid name " + name);
                return nullptr;
            }

            String name = names[i].Substring(1, names[i].Length() - 1);
            char s = name.Front();
            if (s >= '0' && s <= '9')
            {
                unsigned index = ToUInt(name);
                element = element->GetChild(index);
            }
            else
            {
                element = element->GetChild(name, true);
            }

            if (!element)
            {
                URHO3D_LOGERROR("Could not find element by name " + name);
                return nullptr;
            }
        }

        outName = names.Back();
        return element;
    }
}

void UIElement::MarkDirty()
{
    positionDirty_ = true;
    opacityDirty_ = true;
    derivedColorDirty_ = true;

    for (Vector<SharedPtr<UIElement> >::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
        (*i)->MarkDirty();
}

bool UIElement::RemoveChildXML(XMLElement& parent, const String& name) const
{
    static XPathQuery matchXPathQuery("./attribute[@name=$attributeName]", "attributeName:String");

    if (!matchXPathQuery.SetVariable("attributeName", name))
        return false;

    XMLElement removeElem = parent.SelectSinglePrepared(matchXPathQuery);
    return !removeElem || parent.RemoveChild(removeElem);
}

bool UIElement::RemoveChildXML(XMLElement& parent, const String& name, const String& value) const
{
    static XPathQuery matchXPathQuery
        ("./attribute[@name=$attributeName and @value=$attributeValue]", "attributeName:String, attributeValue:String");

    if (!matchXPathQuery.SetVariable("attributeName", name))
        return false;
    if (!matchXPathQuery.SetVariable("attributeValue", value))
        return false;

    XMLElement removeElem = parent.SelectSinglePrepared(matchXPathQuery);
    return !removeElem || parent.RemoveChild(removeElem);
}

bool UIElement::FilterUIStyleAttributes(XMLElement& dest, const XMLElement& styleElem) const
{
    // Remove style attribute only when its value is identical to the value stored in style file
    String style = styleElem.GetAttribute("style");
    if (!style.Empty())
    {
        if (style == dest.GetAttribute("style"))
        {
            if (!dest.RemoveAttribute("style"))
            {
                URHO3D_LOGWARNING("Could not remove style attribute");
                return false;
            }
        }
    }

    // Perform the same action recursively for internal child elements stored in style file
    XMLElement childDest = dest.GetChild("element");
    XMLElement childElem = styleElem.GetChild("element");
    while (childDest && childElem)
    {
        if (!childElem.GetBool("internal"))
        {
            URHO3D_LOGERROR("Invalid style file, style element can only contain internal child elements");
            return false;
        }
        if (!FilterUIStyleAttributes(childDest, childElem))
            return false;

        childDest = childDest.GetNext("element");
        childElem = childElem.GetNext("element");
    }

    // Remove style attribute when it is the same as its type, however, if it is an internal element then replace it to "none" instead
    if (!dest.GetAttribute("style").Empty() && dest.GetAttribute("style") == dest.GetAttribute("type"))
    {
        if (internal_)
        {
            if (!dest.SetAttribute("style", "none"))
                return false;
        }
        else
        {
            if (!dest.RemoveAttribute("style"))
                return false;
        }
    }

    return true;
}

bool UIElement::FilterImplicitAttributes(XMLElement& dest) const
{
    // Remove positioning and sizing attributes when they are under the influence of layout mode
    if (layoutMode_ != LM_FREE && !IsFixedWidth() && !IsFixedHeight())
    {
        if (!RemoveChildXML(dest, "Min Size"))
            return false;
    }
    if (parent_ && parent_->layoutMode_ != LM_FREE)
    {
        if (!RemoveChildXML(dest, "Position"))
            return false;
        if (!RemoveChildXML(dest, "Size"))
            return false;
    }

    return true;
}

bool UIElement::FilterUIStyleAttributes(JSONValue& dest, const XMLElement& styleElem) const
{
    // Remove style attribute only when its value is identical to the value stored in style file
    String style = styleElem.GetAttribute("style");
    if (!style.Empty())
    {
        if (style == dest["style"].GetString())
        {
            if (!dest.Erase("style"))
            {
                URHO3D_LOGWARNING("Could not remove style attribute");
                return false;
            }
        }
    }

    // Perform the same action recursively for internal child elements stored in style file
    JSONArray childDest = dest["children"].GetArray();
    JSONArray::Iterator currentDest = childDest.Begin();
    XMLElement childElem = styleElem.GetChild("element");
    while (currentDest != childDest.End() && childElem)
    {
        if (!childElem.GetBool("internal"))
        {
            URHO3D_LOGERROR("Invalid style file, style element can only contain internal child elements");
            return false;
        }
        if (!FilterUIStyleAttributes(*currentDest, childElem))
            return false;

        ++currentDest;
        childElem = childElem.GetNext("element");
    }
    if (!childDest.Empty())
        dest.Set("children", childDest);

    // Remove style attribute when it is the same as its type, however, if it is an internal element then replace it to "none" instead
    if (!dest["style"].GetString().Empty() && dest["style"].GetString() == dest["type"].GetString())
    {
        if (internal_)
            dest.Set("style", "none");
        else
        {
            if (!dest.Erase("style"))
                return false;
        }
    }

    return true;
}

bool UIElement::FilterImplicitAttributes(JSONValue& dest) const
{
    // Remove positioning and sizing attributes when they are under the influence of layout mode
    if (layoutMode_ != LM_FREE && !IsFixedWidth() && !IsFixedHeight())
        dest.Erase("Min Size");
    if (parent_ && parent_->layoutMode_ != LM_FREE)
    {
        dest.Erase("Position");
        dest.Erase("Size");
    }

    return true;
}

void UIElement::UpdateAnchoring()
{
    if (parent_ && enableAnchor_)
    {
        IntVector2 newSize;
        newSize.x_ = (int)(parent_->size_.x_ * Clamp(anchorMax_.x_ - anchorMin_.x_, 0.0f, 1.0f)) + maxOffset_.x_ - minOffset_.x_;
        newSize.y_ = (int)(parent_->size_.y_ * Clamp(anchorMax_.y_ - anchorMin_.y_, 0.0f, 1.0f)) + maxOffset_.y_ - minOffset_.y_;

        if (position_ != minOffset_)
            SetPosition(minOffset_);
        if (size_ != newSize)
            SetSize(newSize);
    }
}

void UIElement::GetChildrenRecursive(PODVector<UIElement*>& dest) const
{
    for (Vector<SharedPtr<UIElement> >::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
    {
        UIElement* element = *i;
        dest.Push(element);
        if (!element->children_.Empty())
            element->GetChildrenRecursive(dest);
    }
}

void UIElement::ApplyStyleRecursive(UIElement* element)
{
    // If child element style file changes as result of being (re)parented and it has a defined style, apply it now
    if (!element->appliedStyle_.Empty() && element->appliedStyleFile_.Get() != element->GetDefaultStyle())
    {
        element->SetStyle(element->appliedStyle_);
        for (Vector<SharedPtr<UIElement> >::ConstIterator i = element->children_.Begin(); i != element->children_.End(); ++i)
            element->ApplyStyleRecursive(*i);
    }
}

int UIElement::CalculateLayoutParentSize(const PODVector<int>& sizes, int begin, int end, int spacing)
{
    int width = begin + end;
    if (sizes.Empty())
        return width;

    for (unsigned i = 0; i < sizes.Size(); ++i)
    {
        // If calculating maximum size, and the default is specified, do not overflow it
        if (sizes[i] == M_MAX_INT)
            return M_MAX_INT;
        width += sizes[i] + spacing;
    }
    // The last spacing is not needed
    return width - spacing;
}

void UIElement::CalculateLayout(PODVector<int>& positions, PODVector<int>& sizes, const PODVector<int>& minSizes,
    const PODVector<int>& maxSizes, const PODVector<float>& flexScales, int targetSize, int begin, int end, int spacing)
{
    unsigned numChildren = sizes.Size();
    if (!numChildren)
        return;
    int targetTotalSize = targetSize - begin - end - (numChildren - 1) * spacing;
    if (targetTotalSize < 0)
        targetTotalSize = 0;
    int targetChildSize = targetTotalSize / numChildren;
    int remainder = targetTotalSize % numChildren;
    float add = (float)remainder / numChildren;
    float acc = 0.0f;

    // Initial pass
    for (unsigned i = 0; i < numChildren; ++i)
    {
        auto targetSize = (int)(targetChildSize * flexScales[i]);
        if (remainder)
        {
            acc += add;
            if (acc >= 0.5f)
            {
                acc -= 1.0f;
                ++targetSize;
                --remainder;
            }
        }
        sizes[i] = Clamp(targetSize, minSizes[i], maxSizes[i]);
    }

    // Error correction passes
    for (;;)
    {
        int actualTotalSize = 0;
        for (unsigned i = 0; i < numChildren; ++i)
            actualTotalSize += sizes[i];
        int error = targetTotalSize - actualTotalSize;
        // Break if no error
        if (!error)
            break;

        // Check which of the children can be resized to correct the error. If none, must break
        PODVector<unsigned> resizable;
        for (unsigned i = 0; i < numChildren; ++i)
        {
            if (error < 0 && sizes[i] > minSizes[i])
                resizable.Push(i);
            else if (error > 0 && sizes[i] < maxSizes[i])
                resizable.Push(i);
        }
        if (resizable.Empty())
            break;

        int numResizable = resizable.Size();
        int errorPerChild = error / numResizable;
        remainder = (abs(error)) % numResizable;
        add = (float)remainder / numResizable;
        acc = 0.0f;

        for (int i = 0; i < numResizable; ++i)
        {
            unsigned index = resizable[i];
            int targetSize = sizes[index] + errorPerChild;
            if (remainder)
            {
                acc += add;
                if (acc >= 0.5f)
                {
                    acc -= 1.0f;
                    targetSize = error < 0 ? targetSize - 1 : targetSize + 1;
                    --remainder;
                }
            }

            sizes[index] = Clamp(targetSize, minSizes[index], maxSizes[index]);
        }
    }

    // Calculate final positions and store the maximum child element size for optimizations
    layoutElementMaxSize_ = 0;
    int position = begin;
    for (unsigned i = 0; i < numChildren; ++i)
    {
        positions[i] = position;
        position += sizes[i] + spacing;
        if (sizes[i] > layoutElementMaxSize_)
            layoutElementMaxSize_ = sizes[i];
    }
}

IntVector2 UIElement::GetLayoutChildPosition(UIElement* child)
{
    IntVector2 ret(IntVector2::ZERO);

    HorizontalAlignment ha = child->GetHorizontalAlignment();
    switch (ha)
    {
    case HA_LEFT:
        ret.x_ = layoutBorder_.left_;
        break;

    case HA_RIGHT:
        ret.x_ = -layoutBorder_.right_;
        break;

    default:
        break;
    }

    VerticalAlignment va = child->GetVerticalAlignment();
    switch (va)
    {
    case VA_TOP:
        ret.y_ = layoutBorder_.top_;
        break;

    case VA_BOTTOM:
        ret.y_ = -layoutBorder_.bottom_;
        break;

    default:
        break;
    }

    return ret;
}

void UIElement::Detach()
{
    parent_ = nullptr;
    MarkDirty();
}

void UIElement::VerifyChildAlignment()
{
    for (Vector<SharedPtr<UIElement> >::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
    {
        // Reapply child alignments. If they are illegal compared to layout, they will be set left/top as neded
        (*i)->SetHorizontalAlignment((*i)->GetHorizontalAlignment());
        (*i)->SetVerticalAlignment((*i)->GetVerticalAlignment());
    }
}

void UIElement::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace PostUpdate;

    UpdateAttributeAnimations(eventData[P_TIMESTEP].GetFloat());
}

void UIElement::SetRenderTexture(Texture2D* texture)
{
    if (auto* ui = GetSubsystem<UI>())
        ui->SetElementRenderTexture(this, texture);
}

}
