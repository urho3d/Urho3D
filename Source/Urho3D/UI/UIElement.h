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

/// \file

#pragma once

#include "../Math/Vector2.h"
#include "../Input/InputConstants.h"
#include "../Resource/XMLFile.h"
#include "../Scene/Animatable.h"
#include "../UI/UIBatch.h"

namespace Urho3D
{

/// %UI element horizontal alignment.
enum HorizontalAlignment
{
    HA_LEFT = 0,
    HA_CENTER,
    HA_RIGHT,
    HA_CUSTOM
};

/// %UI element vertical alignment.
enum VerticalAlignment
{
    VA_TOP = 0,
    VA_CENTER,
    VA_BOTTOM,
    VA_CUSTOM
};

/// %UI element corners.
enum Corner
{
    C_TOPLEFT = 0,
    C_TOPRIGHT,
    C_BOTTOMLEFT,
    C_BOTTOMRIGHT,
    MAX_UIELEMENT_CORNERS
};

/// %UI element orientation.
enum Orientation
{
    O_HORIZONTAL = 0,
    O_VERTICAL
};

/// %UI element focus mode.
enum FocusMode
{
    /// Is not focusable and does not affect existing focus.
    FM_NOTFOCUSABLE = 0,
    /// Resets focus when clicked.
    FM_RESETFOCUS,
    /// Is focusable.
    FM_FOCUSABLE,
    /// Is focusable and also defocusable by pressing ESC.
    FM_FOCUSABLE_DEFOCUSABLE
};

/// Layout operation mode.
enum LayoutMode
{
    /// No layout operations will be performed.
    LM_FREE = 0,
    /// Layout child elements horizontally and resize them to fit. Resize element if necessary.
    LM_HORIZONTAL,
    /// Layout child elements vertically and resize them to fit. Resize element if necessary.
    LM_VERTICAL
};

/// Traversal mode for rendering.
enum TraversalMode
{
    /// Traverse through children having same priority first and recurse into their children before traversing children having higher priority.
    TM_BREADTH_FIRST = 0,
    /// Traverse through each child and its children immediately after in sequence.
    TM_DEPTH_FIRST
};

enum DragAndDropMode : unsigned
{
    /// Drag and drop disabled.
    DD_DISABLED = 0x0,
    /// Drag and drop source flag.
    DD_SOURCE = 0x1,
    /// Drag and drop target flag.
    DD_TARGET = 0x2,
    /// Drag and drop source and target.
    DD_SOURCE_AND_TARGET = 0x3,
};
URHO3D_FLAGSET(DragAndDropMode, DragAndDropModeFlags);

class Cursor;
class ResourceCache;
class Texture2D;

/// Base class for %UI elements.
class URHO3D_API UIElement : public Animatable
{
    URHO3D_OBJECT(UIElement, Animatable);

public:
    /// Construct.
    explicit UIElement(Context* context);
    /// Destruct.
    ~UIElement() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Apply attribute changes that can not be applied immediately.
    void ApplyAttributes() override;
    /// Load from binary data. Return true if successful.
    bool Load(Deserializer& source) override;
    /// Save as binary data. Return true if successful.
    bool Save(Serializer& dest) const override;
    /// Load from XML data. Return true if successful.
    bool LoadXML(const XMLElement& source) override;
    /// Load from XML data with style. Return true if successful.
    virtual bool LoadXML(const XMLElement& source, XMLFile* styleFile);
    /// Create a child by loading from XML data with style. Returns the child element if successful, null if otherwise.
    virtual UIElement* LoadChildXML(const XMLElement& childElem, XMLFile* styleFile);
    /// Save as XML data. Return true if successful.
    bool SaveXML(XMLElement& dest) const override;
    /// Load from JSON data. Return true if successful.
    bool LoadJSON(const JSONValue& source) override;
    /// Load from JSON data with style. Return true if successful.
    virtual bool LoadJSON(const JSONValue& source, XMLFile* styleFile);
    /// Create a child by loading from JSON data with style. Returns the child element if successful, null if otherwise.
    virtual UIElement* LoadChildJSON(const JSONValue& childElem, XMLFile* styleFile);
    /// Save as JSON data. Return true if successful.
    bool SaveJSON(JSONValue& dest) const override;

    /// Perform UI element update.
    virtual void Update(float timeStep);
    /// Return whether is visible and inside a scissor rectangle and should be rendered.
    virtual bool IsWithinScissor(const IntRect& currentScissor);
    /// Update and return screen position.
    /// @property
    virtual const IntVector2& GetScreenPosition() const;
    /// Return UI rendering batches.
    virtual void GetBatches(PODVector<UIBatch>& batches, PODVector<float>& vertexData, const IntRect& currentScissor);
    /// Return UI rendering batches for debug draw.
    virtual void GetDebugDrawBatches(PODVector<UIBatch>& batches, PODVector<float>& vertexData, const IntRect& currentScissor);
    /// React to mouse hover.
    virtual void OnHover(const IntVector2& position, const IntVector2& screenPosition, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor);
    /// React to mouse click begin.
    virtual void OnClickBegin
        (const IntVector2& position, const IntVector2& screenPosition, MouseButton button, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor) { }
    /// React to mouse click end.
    virtual void OnClickEnd
        (const IntVector2& position, const IntVector2& screenPosition, MouseButton button, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor,
            UIElement* beginElement) { }
    /// React to double mouse click.
    virtual void OnDoubleClick
        (const IntVector2& position, const IntVector2& screenPosition, MouseButton button, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor) { }
    /// React to mouse drag begin.
    virtual void
        OnDragBegin(const IntVector2& position, const IntVector2& screenPosition, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor);
    /// React to mouse drag motion.
    virtual void OnDragMove
        (const IntVector2& position, const IntVector2& screenPosition, const IntVector2& deltaPos, MouseButtonFlags buttons, QualifierFlags qualifiers,
            Cursor* cursor);
    /// React to mouse drag end.
    virtual void
        OnDragEnd(const IntVector2& position, const IntVector2& screenPosition, MouseButtonFlags dragButtons, MouseButtonFlags releaseButtons, Cursor* cursor);
    /// React to a mouse drag cancel event (ie, when an extra button is pressed).
    virtual void OnDragCancel
        (const IntVector2& position, const IntVector2& screenPosition, MouseButtonFlags dragButtons, MouseButtonFlags cancelButtons, Cursor* cursor);
    /// React to drag and drop test. Return true to signal that the drop is acceptable.
    virtual bool OnDragDropTest(UIElement* source);
    /// React to drag and drop finish. Return true to signal that the drop was accepted.
    virtual bool OnDragDropFinish(UIElement* source);
    /// React to mouse wheel.
    virtual void OnWheel(int delta, MouseButtonFlags buttons, QualifierFlags qualifiers) { }
    /// React to a key press.
    virtual void OnKey(Key key, MouseButtonFlags buttons, QualifierFlags qualifiers) { }
    /// React to text input event.
    virtual void OnTextInput(const String& text) { }

    /// React to resize.
    virtual void OnResize(const IntVector2& newSize, const IntVector2& delta) { }

    /// React to position change.
    virtual void OnPositionSet(const IntVector2& newPosition) { }

    /// React to editable status change.
    virtual void OnSetEditable() { }

    /// React to indent change.
    virtual void OnIndentSet() { }

    /// Convert screen coordinates to element coordinates.
    virtual IntVector2 ScreenToElement(const IntVector2& screenPosition);
    /// Convert element coordinates to screen coordinates.
    virtual IntVector2 ElementToScreen(const IntVector2& position);

    /// Return whether the element could handle wheel input.
    virtual bool IsWheelHandler() const { return false; }

    /// Load from an XML file. Return true if successful.
    bool LoadXML(Deserializer& source);
    /// Save to an XML file. Return true if successful.
    bool SaveXML(Serializer& dest, const String& indentation = "\t") const;
    /// Load from a JSON file. Return true if successful.
    bool LoadJSON(Deserializer& source);
    /// Save to a JSON file. Return true if successful.
    bool SaveJSON(Serializer& dest, const String& indentation = "\t") const;
    /// Filter attributes in serialization process.
    bool FilterAttributes(XMLElement& dest) const;
    /// Filter attributes in serialization process.
    bool FilterAttributes(JSONValue& dest) const;

    /// Set name.
    /// @property
    void SetName(const String& name);
    /// Set position.
    /// @property
    void SetPosition(const IntVector2& position);
    /// Set position.
    void SetPosition(int x, int y);
    /// Set size.
    /// @property
    void SetSize(const IntVector2& size);
    /// Set size.
    void SetSize(int width, int height);
    /// Set width only.
    /// @property
    void SetWidth(int width);
    /// Set height only.
    /// @property
    void SetHeight(int height);
    /// Set minimum size.
    /// @property
    void SetMinSize(const IntVector2& minSize);
    /// Set minimum size.
    void SetMinSize(int width, int height);
    /// Set minimum width.
    /// @property
    void SetMinWidth(int width);
    /// Set minimum height.
    /// @property
    void SetMinHeight(int height);
    /// Set maximum size.
    /// @property
    void SetMaxSize(const IntVector2& maxSize);
    /// Set maximum size.
    void SetMaxSize(int width, int height);
    /// Set maximum width.
    /// @property
    void SetMaxWidth(int width);
    /// Set maximum height.
    /// @property
    void SetMaxHeight(int height);
    /// Set fixed size.
    void SetFixedSize(const IntVector2& size);
    /// Set fixed size.
    void SetFixedSize(int width, int height);
    /// Set fixed width.
    void SetFixedWidth(int width);
    /// Set fixed height.
    void SetFixedHeight(int height);
    /// Set horizontal and vertical alignment.
    void SetAlignment(HorizontalAlignment hAlign, VerticalAlignment vAlign);
    /// Set horizontal alignment.
    /// @property
    void SetHorizontalAlignment(HorizontalAlignment align);
    /// Set vertical alignment.
    /// @property
    void SetVerticalAlignment(VerticalAlignment align);
    /// Enable automatic positioning & sizing of the element relative to its parent using min/max anchor and min/max offset. Default false.
    /// @property
    void SetEnableAnchor(bool enable);
    /// Set minimum (top left) anchor in relation to the parent element (from 0 to 1). No effect when anchor is not enabled.
    /// @property
    void SetMinAnchor(const Vector2& anchor);
    /// Set minimum anchor.
    void SetMinAnchor(float x, float y);
    /// Set maximum (bottom right) anchor in relation to the parent element (from 0 to 1). No effect when anchor is not enabled.
    /// @property
    void SetMaxAnchor(const Vector2& anchor);
    /// Set maximum anchor.
    void SetMaxAnchor(float x, float y);
    /// Set offset of element's top left from the minimum anchor in pixels. No effect when anchor is not enabled.
    /// @property
    void SetMinOffset(const IntVector2& offset);
    /// Set offset of element's bottom right from the maximum anchor in pixels. No effect when anchor is not enabled.
    /// @property
    void SetMaxOffset(const IntVector2& offset);
    /// Set pivot relative to element's size (from 0 to 1, where 0.5 is center). Overrides horizontal & vertical alignment.
    /// @property
    void SetPivot(const Vector2& pivot);
    /// Set pivot relative to element's size (from 0 to 1, where 0.5 is center). Overrides horizontal & vertical alignment.
    void SetPivot(float x, float y);
    /// Set child element clipping border.
    /// @property
    void SetClipBorder(const IntRect& rect);
    /// Set color on all corners.
    /// @property
    void SetColor(const Color& color);
    /// Set color on one corner.
    /// @property{set_colors}
    void SetColor(Corner corner, const Color& color);
    /// Set priority.
    /// @property
    void SetPriority(int priority);
    /// Set opacity.
    /// @property
    void SetOpacity(float opacity);
    /// Set whether should be brought to front when focused.
    /// @property
    void SetBringToFront(bool enable);
    /// Set whether should be put to background when another element is focused.
    /// @property
    void SetBringToBack(bool enable);
    /// Set whether should clip child elements. Default false.
    /// @property
    void SetClipChildren(bool enable);
    /// Set whether should sort child elements according to priority. Default true.
    /// @property
    void SetSortChildren(bool enable);
    /// Set whether parent elements' opacity affects opacity. Default true.
    /// @property
    void SetUseDerivedOpacity(bool enable);
    /// Set whether reacts to input. Default false, but is enabled by subclasses if applicable.
    /// @property
    void SetEnabled(bool enable);
    /// Set enabled state on self and child elements. Elements' own enabled state is remembered (IsEnabledSelf) and can be restored.
    void SetDeepEnabled(bool enable);
    /// Reset enabled state to the element's remembered state prior to calling SetDeepEnabled.
    void ResetDeepEnabled();
    /// Set enabled state on self and child elements. Unlike SetDeepEnabled this does not remember the elements' own enabled state, but overwrites it.
    void SetEnabledRecursive(bool enable);
    /// Set whether value is editable through input. Not applicable to all elements. Default true.
    /// @property
    void SetEditable(bool enable);
    /// Set whether is focused. Only one element can be focused at a time.
    /// @property
    void SetFocus(bool enable);
    /// Set selected mode. Actual meaning is element dependent, for example constant hover or pressed effect.
    /// @property
    void SetSelected(bool enable);
    /// Set whether is visible. Visibility propagates to child elements.
    /// @property
    void SetVisible(bool enable);
    /// Set focus mode.
    /// @property
    void SetFocusMode(FocusMode mode);
    /// Set drag and drop flags.
    /// @property
    void SetDragDropMode(DragAndDropModeFlags mode);
    /// Set style from an XML file. Find the style element by name. If the style file is not explicitly provided, use the default style from parental chain. Return true if the style is applied successfully. See also \ref UI_Programmatic.
    bool SetStyle(const String& styleName, XMLFile* file = nullptr);
    /// Set style from an XML element. Return true if the style is applied successfully.
    bool SetStyle(const XMLElement& element);
    /// Set style from an XML file. Find the style element automatically by using the element's typename. If the style file is not explicitly provided, use the default style from parental chain. Return true if the style is applied successfully. See also \ref UI_Programmatic.
    bool SetStyleAuto(XMLFile* file = nullptr);
    /// Set default style file for later use by children elements.
    /// @property
    void SetDefaultStyle(XMLFile* style);
    /// Set layout parameters.
    void SetLayout(LayoutMode mode, int spacing = 0, const IntRect& border = IntRect::ZERO);
    /// Set layout mode only.
    /// @property
    void SetLayoutMode(LayoutMode mode);
    /// Set layout spacing.
    /// @property
    void SetLayoutSpacing(int spacing);
    /// Set layout border.
    /// @property
    void SetLayoutBorder(const IntRect& border);
    /// Set layout flex scale.
    /// @property
    void SetLayoutFlexScale(const Vector2& scale);
    /// Set horizontal indentation.
    /// @property
    void SetIndent(int indent);
    /// Set indent spacing (number of pixels per indentation level).
    /// @property
    void SetIndentSpacing(int indentSpacing);
    /// Manually update layout. Should not be necessary in most cases, but is provided for completeness.
    void UpdateLayout();
    /// Disable automatic layout update. Should only be used if there are performance problems.
    void DisableLayoutUpdate();
    /// Enable automatic layout update.
    void EnableLayoutUpdate();
    /// Bring UI element to front.
    void BringToFront();
    /// Create and add a child element and return it.
    UIElement* CreateChild(StringHash type, const String& name = String::EMPTY, unsigned index = M_MAX_UNSIGNED);
    /// Add a child element.
    void AddChild(UIElement* element);
    /// Insert a child element into a specific position in the child list.
    void InsertChild(unsigned index, UIElement* element);
    /// Remove a child element. Starting search at specified index if provided.
    void RemoveChild(UIElement* element, unsigned index = 0);
    /// Remove a child element at index.
    void RemoveChildAtIndex(unsigned index);
    /// Remove all child elements.
    void RemoveAllChildren();
    /// Remove from the parent element. If no other shared pointer references exist, causes immediate deletion.
    void Remove();
    /// Find child index. Return M_MAX_UNSIGNED if not found.
    unsigned FindChild(UIElement* element) const;
    /// Set parent element. Same as parent->InsertChild(index, this).
    void SetParent(UIElement* parent, unsigned index = M_MAX_UNSIGNED);
    /// Set a user variable.
    void SetVar(StringHash key, const Variant& value);
    /// Mark as internally (programmatically) created. Used when an element composes itself out of child elements.
    /// @property
    void SetInternal(bool enable);
    /// Set traversal mode for rendering. The default traversal mode is TM_BREADTH_FIRST for non-root element. Root element should be set to TM_DEPTH_FIRST to avoid artifacts during rendering.
    /// @property
    void SetTraversalMode(TraversalMode traversalMode);
    /// Set element event sender flag. When child element is added or deleted, the event would be sent using UIElement found in the parental chain having this flag set. If not set, the event is sent using UI's root as per normal.
    /// @property
    void SetElementEventSender(bool flag);

    /// Set tags. Old tags are overwritten.
    void SetTags(const StringVector& tags);
    /// Add a tag.
    void AddTag(const String& tag);
    /// Add tags with the specified separator (; by default).
    void AddTags(const String& tags, char separator = ';');
    /// Add tags.
    void AddTags(const StringVector& tags);
    /// Remove specific tag. Return true if existed.
    bool RemoveTag(const String& tag);
    /// Remove all tags.
    void RemoveAllTags();

    /// Template version of creating a child element.
    template <class T> T* CreateChild(const String& name = String::EMPTY, unsigned index = M_MAX_UNSIGNED);
    /// Template version of returning child element by index using static cast.
    template <class T> T* GetChildStaticCast(unsigned index) const;
    /// Template version of returning child element by name using static cast.
    template <class T> T* GetChildStaticCast(const String& name, bool recursive = false) const;
    /// Template version of returning child element by variable using static cast. If only key is provided, return the first child having the matching variable key. If value is also provided then the actual variable value would also be checked against.
    template <class T> T* GetChildStaticCast(const StringHash& key, const Variant& value = Variant::EMPTY, bool recursive = false) const;
    /// Template version of returning child element by index using dynamic cast. May return 0 when casting failed.
    template <class T> T* GetChildDynamicCast(unsigned index) const;
    /// Template version of returning child element by name using dynamic cast. May return 0 when casting failed.
    template <class T> T* GetChildDynamicCast(const String& name, bool recursive = false) const;
    /// Template version of returning child element by variable. If only key is provided, return the first child having the matching variable key. If value is also provided then the actual variable value would also be checked against using dynamic cast. May return 0 when casting failed.
    template <class T> T* GetChildDynamicCast(const StringHash& key, const Variant& value = Variant::EMPTY, bool recursive = false) const;

    /// Return name.
    /// @property
    const String& GetName() const { return name_; }

    /// Return position.
    /// @property
    const IntVector2& GetPosition() const { return position_; }

    /// Return size.
    /// @property
    const IntVector2& GetSize() const { return size_; }

    /// Return width.
    /// @property
    int GetWidth() const { return size_.x_; }

    /// Return height.
    /// @property
    int GetHeight() const { return size_.y_; }

    /// Return minimum size.
    /// @property
    const IntVector2& GetMinSize() const { return minSize_; }

    /// Return minimum width.
    /// @property
    int GetMinWidth() const { return minSize_.x_; }

    /// Return minimum height.
    /// @property
    int GetMinHeight() const { return minSize_.y_; }

    /// Return maximum size.
    /// @property
    const IntVector2& GetMaxSize() const { return maxSize_; }

    /// Return minimum width.
    /// @property
    int GetMaxWidth() const { return maxSize_.x_; }

    /// Return minimum height.
    /// @property
    int GetMaxHeight() const { return maxSize_.y_; }

    /// Return true if size is fixed.
    /// @property
    bool IsFixedSize() const { return minSize_ == maxSize_; }

    /// Return true if width is fixed.
    /// @property
    bool IsFixedWidth() const { return minSize_.x_ == maxSize_.x_; }

    /// Return true if height is fixed.
    /// @property
    bool IsFixedHeight() const { return minSize_.y_ == maxSize_.y_; }

    /// Return child element offset.
    /// @property
    const IntVector2& GetChildOffset() const { return childOffset_; }

    /// Return horizontal alignment. If pivot has been adjusted to a custom horizontal setting, returns HA_CUSTOM.
    /// @property
    HorizontalAlignment GetHorizontalAlignment() const;

    /// Return vertical alignment. If pivot has been adjusted to a custom vertical setting, returns VA_CUSTOM.
    /// @property
    VerticalAlignment GetVerticalAlignment() const;

    /// Return whether anchor positioning & sizing is enabled.
    /// @property
    bool GetEnableAnchor() const { return enableAnchor_; }

    /// Return minimum anchor.
    /// @property
    const Vector2& GetMinAnchor() const { return anchorMin_; }

    /// Return maximum anchor.
    /// @property
    const Vector2& GetMaxAnchor() const { return anchorMax_; }

    // Return minimum offset.
    /// @property
    const IntVector2& GetMinOffset() const { return minOffset_; }

    // Return maximum offset.
    /// @property
    const IntVector2& GetMaxOffset() const { return maxOffset_; }

    /// Return pivot.
    /// @property
    const Vector2& GetPivot() const { return pivot_; }

    /// Return child element clipping border.
    /// @property
    const IntRect& GetClipBorder() const { return clipBorder_; }

    /// Return corner color.
    /// @property{get_colors}
    const Color& GetColor(Corner corner) const { return colors_[corner]; }

    /// Return priority.
    /// @property
    int GetPriority() const { return priority_; }

    /// Return opacity.
    /// @property
    float GetOpacity() const { return opacity_; }

    /// Return derived opacity (affected by parent elements). If UseDerivedOpacity is false, returns same as element's own opacity.
    /// @property
    float GetDerivedOpacity() const;

    /// Return whether should be brought to front when focused.
    /// @property
    bool GetBringToFront() const { return bringToFront_; }

    /// Return whether should be put to background when another element is focused.
    /// @property
    bool GetBringToBack() const { return bringToBack_; }

    /// Return whether should clip child elements.
    /// @property
    bool GetClipChildren() const { return clipChildren_; }

    /// Return whether should sort child elements according to priority.
    /// @property
    bool GetSortChildren() const { return sortChildren_; }

    /// Return whether parent elements' opacity affects opacity.
    /// @property
    bool GetUseDerivedOpacity() const { return useDerivedOpacity_; }

    /// Return whether has focus.
    /// @property{get_focus}
    bool HasFocus() const;

    /// Return whether is a direct or indirect child of specified element.
    bool IsChildOf(UIElement* element) const;

    /// Return whether reacts to input.
    /// @property
    bool IsEnabled() const { return enabled_; }

    /// Returns the element's last own enabled state. May be different than the value returned by IsEnabled when SetDeepEnabled has been used.
    /// @property
    bool IsEnabledSelf() const { return enabledPrev_; }

    /// Return whether value is editable through input.
    /// @property
    bool IsEditable() const { return editable_; }

    /// Return whether is selected. Actual meaning is element dependent.
    /// @property
    bool IsSelected() const { return selected_; }

    /// Return whether element itself should be visible. Elements can be also hidden due to the parent being not visible, use IsVisibleEffective() to check.
    /// @property
    bool IsVisible() const { return visible_; }

    /// Return whether element is effectively visible (parent element chain is visible).
    /// @property
    bool IsVisibleEffective() const;

    /// Return whether the cursor is hovering on this element.
    /// @property
    bool IsHovering() const { return hovering_; }

    /// Return whether is internally created.
    /// @property
    bool IsInternal() const { return internal_; }

    /// Return whether has different color in at least one corner.
    /// @property{get_colorGradient}
    bool HasColorGradient() const { return colorGradient_; }

    /// Return focus mode.
    /// @property
    FocusMode GetFocusMode() const { return focusMode_; }

    /// Return drag and drop flags.
    /// @property
    DragAndDropModeFlags GetDragDropMode() const { return dragDropMode_; }

    /// Return applied style name. Return an empty string when the applied style is an 'auto' style (i.e. style derived from instance's type).
    /// @property{get_style}
    const String& GetAppliedStyle() const;
    /// Return default style.
    XMLFile* GetDefaultStyle(bool recursiveUp = true) const;

    /// Return layout mode.
    /// @property
    LayoutMode GetLayoutMode() const { return layoutMode_; }

    /// Return layout spacing.
    /// @property
    int GetLayoutSpacing() const { return layoutSpacing_; }

    /// Return layout border.
    /// @property
    const IntRect& GetLayoutBorder() const { return layoutBorder_; }

    /// Return layout flex scale.
    /// @property
    const Vector2& GetLayoutFlexScale() const { return layoutFlexScale_; }

    /// Return number of child elements.
    unsigned GetNumChildren(bool recursive = false) const;
    /// Return child element by index.
    /// @property{get_children}
    UIElement* GetChild(unsigned index) const;
    /// Return child element by name.
    UIElement* GetChild(const String& name, bool recursive = false) const;
    /// Return child element by variable. If only key is provided, return the first child having the matching variable key. If value is also provided then the actual variable value would also be checked against.
    UIElement* GetChild(const StringHash& key, const Variant& value = Variant::EMPTY, bool recursive = false) const;

    /// Return immediate child elements.
    const Vector<SharedPtr<UIElement> >& GetChildren() const { return children_; }

    /// Return child elements either recursively or non-recursively.
    void GetChildren(PODVector<UIElement*>& dest, bool recursive = false) const;

    /// Return child elements, optionally recursive.
    PODVector<UIElement*> GetChildren(bool recursive) const;

    /// Return parent element.
    /// @property
    UIElement* GetParent() const { return parent_; }

    /// Return root element.
    /// @property
    UIElement* GetRoot() const;
    /// Return derived color. Only valid when no gradient.
    const Color& GetDerivedColor() const;
    /// Return a user variable.
    const Variant& GetVar(const StringHash& key) const;

    /// Return all user variables.
    const VariantMap& GetVars() const { return vars_; }

    /// Return whether element is tagged by a specific tag.
    bool HasTag(const String& tag) const;

    /// Return all tags.
    /// @property
    const StringVector& GetTags() const { return tags_; }

    /// Return child elements with a specific tag either recursively or non-recursively.
    void GetChildrenWithTag(PODVector<UIElement*>& dest, const String& tag, bool recursive = false) const;

    /// Return child elements with a specific tag either recursively or non-recursively.
    PODVector<UIElement*> GetChildrenWithTag(const String& tag, bool recursive = false) const;

    /// Return the drag button combo if this element is being dragged.
    /// @property
    MouseButtonFlags GetDragButtonCombo() const { return dragButtonCombo_; }

    /// Return the number of buttons dragging this element.
    /// @property
    unsigned GetDragButtonCount() const { return dragButtonCount_; }

    /// Return whether a point (either in element or screen coordinates) is inside the element.
    bool IsInside(IntVector2 position, bool isScreen);
    /// Return whether a point (either in element or screen coordinates) is inside the combined rect of the element and its children.
    bool IsInsideCombined(IntVector2 position, bool isScreen);
    /// Return combined screen coordinate rect of element and its children.
    /// @property
    IntRect GetCombinedScreenRect();
    /// Sort child elements if sorting enabled and order dirty. Called by UI.
    void SortChildren();

    /// Return maximum layout element size in the layout direction. Only valid after layout has been calculated. Used internally by UI for optimizations.
    int GetLayoutElementMaxSize() const { return layoutElementMaxSize_; }

    /// Return horizontal indentation.
    /// @property
    int GetIndent() const { return indent_; }

    /// Return indent spacing (number of pixels per indentation level).
    /// @property
    int GetIndentSpacing() const { return indentSpacing_; }

    /// Return indent width in pixels.
    /// @property
    int GetIndentWidth() const { return indent_ * indentSpacing_; }

    /// Set child offset.
    void SetChildOffset(const IntVector2& offset);
    /// Set hovering state.
    void SetHovering(bool enable);
    /// Adjust scissor for rendering.
    void AdjustScissor(IntRect& currentScissor);
    /// Get UI rendering batches with a specified offset. Also recurse to child elements.
    void GetBatchesWithOffset(IntVector2& offset, PODVector<UIBatch>& batches, PODVector<float>& vertexData, IntRect currentScissor);

    /// Return color attribute. Uses just the top-left color.
    const Color& GetColorAttr() const { return colors_[0]; }

    /// Return traversal mode for rendering.
    /// @property
    TraversalMode GetTraversalMode() const { return traversalMode_; }

    /// Return whether element should send child added / removed events by itself. If false, defers to parent element.
    /// @property
    bool IsElementEventSender() const { return elementEventSender_; }

    /// Get element which should send child added / removed events.
    UIElement* GetElementEventSender() const;

    /// Return effective minimum size, also considering layout. Used internally.
    IntVector2 GetEffectiveMinSize() const;

    /// Set texture to which element will be rendered.
    void SetRenderTexture(Texture2D* texture);

protected:
    /// Handle attribute animation added.
    void OnAttributeAnimationAdded() override;
    /// Handle attribute animation removed.
    void OnAttributeAnimationRemoved() override;
    /// Find target of an attribute animation from object hierarchy by name.
    Animatable* FindAttributeAnimationTarget(const String& name, String& outName) override;
    /// Mark screen position as needing an update.
    void MarkDirty();
    /// Remove child XML element by matching attribute name.
    bool RemoveChildXML(XMLElement& parent, const String& name) const;
    /// Remove child XML element by matching attribute name and value.
    bool RemoveChildXML(XMLElement& parent, const String& name, const String& value) const;
    /// Filter UI-style attributes in serialization process.
    bool FilterUIStyleAttributes(XMLElement& dest, const XMLElement& styleElem) const;
    /// Filter implicit attributes in serialization process.
    virtual bool FilterImplicitAttributes(XMLElement& dest) const;
    /// Filter UI-style attributes in serialization process.
    bool FilterUIStyleAttributes(JSONValue& dest, const XMLElement& styleElem) const;
    /// Filter implicit attributes in serialization process.
    virtual bool FilterImplicitAttributes(JSONValue& dest) const;
    /// Update anchored size & position. Only called when anchoring is enabled.
    void UpdateAnchoring();

    /// Name.
    String name_;
    /// Child elements.
    Vector<SharedPtr<UIElement> > children_;
    /// Parent element.
    UIElement* parent_{};
    /// Child element clipping border.
    IntRect clipBorder_;
    /// Colors.
    Color colors_[MAX_UIELEMENT_CORNERS];
    /// User variables.
    VariantMap vars_;
    /// Priority.
    int priority_{};
    /// Bring to front when focused flag.
    bool bringToFront_{};
    /// Bring to back when defocused flag.
    bool bringToBack_{true};
    /// Clip children flag.
    bool clipChildren_{};
    /// Sort children according to priority flag.
    bool sortChildren_{true};
    /// Use derived opacity flag.
    bool useDerivedOpacity_{true};
    /// Input enabled flag.
    bool enabled_{};
    /// Last SetEnabled flag before any SetDeepEnabled.
    bool enabledPrev_{};
    /// Value editable flag.
    bool editable_{true};
    /// Selected flag.
    bool selected_{};
    /// Visible flag.
    bool visible_{true};
    /// Hovering flag.
    bool hovering_{};
    /// Internally created flag.
    bool internal_{};
    /// Focus mode.
    FocusMode focusMode_{FM_NOTFOCUSABLE};
    /// Drag and drop flags.
    DragAndDropModeFlags dragDropMode_{DD_DISABLED};
    /// Layout mode.
    LayoutMode layoutMode_{LM_FREE};
    /// Layout spacing.
    int layoutSpacing_{};
    /// Layout borders.
    IntRect layoutBorder_{};
    /// Layout flex scale.
    Vector2 layoutFlexScale_{Vector2::ONE};
    /// Resize nesting level to prevent multiple events and endless loop.
    unsigned resizeNestingLevel_{};
    /// Layout update nesting level to prevent endless loop.
    unsigned layoutNestingLevel_{};
    /// Layout element maximum size in layout direction.
    int layoutElementMaxSize_{};
    /// Horizontal indentation.
    int indent_{};
    /// Indent spacing (number of pixels per indentation level).
    int indentSpacing_{16};
    /// Position.
    IntVector2 position_{};
    /// Screen position.
    mutable IntVector2 screenPosition_;
    /// Screen position dirty flag.
    mutable bool positionDirty_{true};
    /// Applied style.
    String appliedStyle_;
    /// Drag button combo.
    MouseButtonFlags dragButtonCombo_{};
    /// Drag button count.
    unsigned dragButtonCount_{};

private:
    /// Return child elements recursively.
    void GetChildrenRecursive(PODVector<UIElement*>& dest) const;
    /// Return child elements with a specific tag recursively.
    void GetChildrenWithTagRecursive(PODVector<UIElement*>& dest, const String& tag) const;
    /// Recursively apply style to a child element hierarchy when adding to an element.
    void ApplyStyleRecursive(UIElement* element);
    /// Calculate layout width for resizing the parent element.
    int CalculateLayoutParentSize(const PODVector<int>& sizes, int begin, int end, int spacing);
    /// Calculate child widths/positions in the layout.
    void CalculateLayout
        (PODVector<int>& positions, PODVector<int>& sizes, const PODVector<int>& minSizes, const PODVector<int>& maxSizes,
            const PODVector<float>& flexScales, int targetSize, int begin, int end, int spacing);
    /// Get child element constant position in a layout.
    IntVector2 GetLayoutChildPosition(UIElement* child);
    /// Detach from parent.
    void Detach();
    /// Verify that child elements have proper alignment for layout mode.
    void VerifyChildAlignment();
    /// Handle logic post-update event.
    void HandlePostUpdate(StringHash eventType, VariantMap& eventData);

    /// Size.
    IntVector2 size_;
    /// Minimum size.
    IntVector2 minSize_;
    /// Maximum size.
    IntVector2 maxSize_{M_MAX_INT, M_MAX_INT};
    /// Child elements' offset. Used internally.
    IntVector2 childOffset_;
    /// Parent's minimum size calculated by layout. Used internally.
    IntVector2 layoutMinSize_;
    /// Minimum offset.
    IntVector2 minOffset_;
    /// Maximum offset.
    IntVector2 maxOffset_;
    /// Use min/max anchor & min/max offset for position & size instead of setting explicitly.
    bool enableAnchor_{};
    /// Has pivot changed manually.
    bool pivotSet_{};
    /// Anchor minimum position.
    Vector2 anchorMin_;
    /// Anchor maximum position.
    Vector2 anchorMax_;
    /// Pivot Position.
    Vector2 pivot_;
    /// Opacity.
    float opacity_{1.0f};
    /// Derived opacity.
    mutable float derivedOpacity_{};
    /// Derived color. Only valid when no gradient.
    mutable Color derivedColor_;
    /// Derived opacity dirty flag.
    mutable bool opacityDirty_{true};
    /// Derived color dirty flag (only used when no gradient).
    mutable bool derivedColorDirty_{true};
    /// Child priority sorting dirty flag.
    bool sortOrderDirty_{};
    /// Has color gradient flag.
    bool colorGradient_{};
    /// Default style file.
    SharedPtr<XMLFile> defaultStyle_;
    /// Last applied style file.
    WeakPtr<XMLFile> appliedStyleFile_;
    /// Traversal mode for rendering.
    TraversalMode traversalMode_{TM_BREADTH_FIRST};
    /// Flag whether node should send child added / removed events by itself.
    bool elementEventSender_{};
    /// XPath query for selecting UI-style.
    static XPathQuery styleXPathQuery_;
    /// Tag list.
    StringVector tags_;
};

template <class T> T* UIElement::CreateChild(const String& name, unsigned index)
{
    return static_cast<T*>(CreateChild(T::GetTypeStatic(), name, index));
}

template <class T> T* UIElement::GetChildStaticCast(unsigned index) const
{
    return static_cast<T*>(GetChild(index));
}

template <class T> T* UIElement::GetChildStaticCast(const String& name, bool recursive) const
{
    return static_cast<T*>(GetChild(name, recursive));
}

template <class T> T* UIElement::GetChildStaticCast(const StringHash& key, const Variant& value, bool recursive) const
{
    return static_cast<T*>(GetChild(key, value, recursive));
}

template <class T> T* UIElement::GetChildDynamicCast(unsigned index) const
{
    return dynamic_cast<T*>(GetChild(index));
}

template <class T> T* UIElement::GetChildDynamicCast(const String& name, bool recursive) const
{
    return dynamic_cast<T*>(GetChild(name, recursive));
}

template <class T> T* UIElement::GetChildDynamicCast(const StringHash& key, const Variant& value, bool recursive) const
{
    return dynamic_cast<T*>(GetChild(key, value, recursive));
}

}
