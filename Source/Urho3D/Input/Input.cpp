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
#include "../Core/Mutex.h"
#include "../Core/ProcessUtils.h"
#include "../Core/Profiler.h"
#include "../Core/StringUtils.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/GraphicsEvents.h"
#include "../Input/Input.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
#include "../IO/RWOpsWrapper.h"
#include "../Resource/ResourceCache.h"
#include "../UI/Text.h"
#include "../UI/UI.h"

#ifdef _WIN32
#include "../Engine/Engine.h"
#endif


#include <SDL/SDL.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#include <emscripten/emscripten.h>
#include <emscripten/bind.h>
#endif

#include "../DebugNew.h"

extern "C" int SDL_AddTouch(SDL_TouchID touchID, SDL_TouchDeviceType type, const char* name);

// Use a "click inside window to focus" mechanism on desktop platforms when the mouse cursor is hidden
#if defined(_WIN32) || (defined(__APPLE__) && !defined(IOS) && !defined(TVOS)) || (defined(__linux__) && !defined(__ANDROID__))
#define REQUIRE_CLICK_TO_FOCUS
#endif

namespace Urho3D
{

const int SCREEN_JOYSTICK_START_ID = 0x40000000;
const StringHash VAR_BUTTON_KEY_BINDING("VAR_BUTTON_KEY_BINDING");
const StringHash VAR_BUTTON_MOUSE_BUTTON_BINDING("VAR_BUTTON_MOUSE_BUTTON_BINDING");
const StringHash VAR_LAST_KEYSYM("VAR_LAST_KEYSYM");
const StringHash VAR_SCREEN_JOYSTICK_ID("VAR_SCREEN_JOYSTICK_ID");

const unsigned TOUCHID_MAX = 32;

/// Convert SDL keycode if necessary.
Key ConvertSDLKeyCode(int keySym, int scanCode)
{
    if (scanCode == SCANCODE_AC_BACK)
        return KEY_ESCAPE;
    else
        return (Key)SDL_tolower(keySym);
}

UIElement* TouchState::GetTouchedElement()
{
    return touchedElement_.Get();
}

#ifdef __EMSCRIPTEN__
#define EM_TRUE 1
#define EM_FALSE 0

/// Glue between Urho Input and Emscripten HTML5
/** HTML5 (Emscripten) is limited in the way it handles input. The EmscriptenInput class attempts to provide the glue between Urho3D Input behavior and HTML5, where SDL currently fails to do so.
 *
 * Mouse Input:
 * - The OS mouse cursor position can't be set.
 * - The mouse can be trapped within play area via 'PointerLock API', which requires a request and interaction between the user and browser.
 * - To request mouse lock, call SetMouseMode(MM_RELATIVE). The E_MOUSEMODECHANGED event will be sent if/when the user confirms the request.
 * NOTE: The request must be initiated by the user (eg: on mouse button down/up, key down/up).
 * - The user can press 'escape' key and browser will force user out of pointer lock. Urho will send the E_MOUSEMODECHANGED event.
 * - SetMouseMode(MM_ABSOLUTE) will leave pointer lock.
 * - MM_WRAP is unsupported.
 */
/// % Emscripten Input glue. Intended to be used by the Input subsystem only.
class EmscriptenInput
{
    friend class Input;
public:
    /// Constructor, expecting pointer to constructing Input instance.
    EmscriptenInput(Input* inputInst);

    /// Static callback method for Pointer Lock API. Handles change in Pointer Lock state and sends events for mouse mode change.
    static EM_BOOL HandlePointerLockChange(int eventType, const EmscriptenPointerlockChangeEvent* keyEvent, void* userData);
    /// Static callback method for Pointer Lock API for the custom shell implementation
    static void HandleCustomPointerLockChange(bool pointerLocked);
    /// Static callback method for tracking focus change events.
    static EM_BOOL HandleFocusChange(int eventType, const EmscriptenFocusEvent* keyEvent, void* userData);
    /// Static callback method for suppressing mouse jump.
    static EM_BOOL HandleMouseJump(int eventType, const EmscriptenMouseEvent * mouseEvent, void* userData);

    /// Static callback method to handle SDL events.
    static int HandleSDLEvents(void* userData, SDL_Event* event);

    /// Send request to user to gain pointer lock. This requires a user-browser interaction on the first call.
    void RequestPointerLock(MouseMode mode, bool suppressEvent = false);
    /// Send request to exit pointer lock. This has the benefit of not requiring the user-browser interaction on the next pointer lock request.
    void ExitPointerLock(bool suppressEvent = false);
    /// Returns whether the page is visible.
    bool IsVisible();

private:
    /// Instance of Input subsystem that constructed this instance.
    static Input* inputInst_;
    /// The mouse mode being requested for pointer-lock.
    static MouseMode requestedMouseMode_;
    /// Flag indicating whether to suppress the next mouse mode change event.
    static bool suppressMouseModeEvent_;

    /// The mouse mode of the previous request for pointer-lock.
    static MouseMode invalidatedRequestedMouseMode_;
    /// Flag indicating the previous request to suppress the next mouse mode change event.
    static bool invalidatedSuppressMouseModeEvent_;
};

Input* EmscriptenInput::inputInst_ = nullptr;
bool EmscriptenInput::suppressMouseModeEvent_ = false;
MouseMode EmscriptenInput::requestedMouseMode_ = MM_INVALID;
bool EmscriptenInput::invalidatedSuppressMouseModeEvent_ = false;
MouseMode EmscriptenInput::invalidatedRequestedMouseMode_ = MM_INVALID;

EmscriptenInput::EmscriptenInput(Input* inputInst)
{
    inputInst_ = inputInst;
    auto* vInputInst = (void*)inputInst;

#ifndef URHO3D_CUSTOM_SHELL
    // Handle pointer lock
    emscripten_set_pointerlockchange_callback(NULL, vInputInst, false, EmscriptenInput::HandlePointerLockChange);
#endif

    // Handle mouse events to prevent mouse jumps
    emscripten_set_mousedown_callback(NULL, vInputInst, true, EmscriptenInput::HandleMouseJump);
    emscripten_set_mousemove_callback(NULL, vInputInst, true, EmscriptenInput::HandleMouseJump);

    // Handle focus changes
    emscripten_set_focusout_callback(NULL, vInputInst, false, EmscriptenInput::HandleFocusChange);
    emscripten_set_focus_callback(NULL, vInputInst, false, EmscriptenInput::HandleFocusChange);

    // Handle SDL events
    SDL_AddEventWatch(EmscriptenInput::HandleSDLEvents, vInputInst);
}

void EmscriptenInput::RequestPointerLock(MouseMode mode, bool suppressEvent)
{
    requestedMouseMode_ = mode;
    suppressMouseModeEvent_ = suppressEvent;
#ifdef URHO3D_CUSTOM_SHELL
    EM_ASM({
        Module.RequestPointerLock();
    });
#else
    emscripten_request_pointerlock(NULL, true);
#endif
}

void EmscriptenInput::ExitPointerLock(bool suppressEvent)
{
    if (requestedMouseMode_ != MM_INVALID)
    {
        invalidatedRequestedMouseMode_ = requestedMouseMode_;
        invalidatedSuppressMouseModeEvent_ = suppressMouseModeEvent_;
    }
    requestedMouseMode_ = MM_INVALID;
    suppressMouseModeEvent_ = suppressEvent;

    if (inputInst_->IsMouseLocked())
    {
        inputInst_->emscriptenExitingPointerLock_ = true;
#ifndef URHO3D_CUSTOM_SHELL
        emscripten_exit_pointerlock();
#endif
    }

#ifdef URHO3D_CUSTOM_SHELL
    EM_ASM({
        Module.ExitPointerLock();
    });
#endif
}

bool EmscriptenInput::IsVisible()
{
    EmscriptenVisibilityChangeEvent visibilityStatus;
    if (emscripten_get_visibility_status(&visibilityStatus) >= EMSCRIPTEN_RESULT_SUCCESS)
        return visibilityStatus.hidden >= EM_TRUE ? false : true;

    // Assume visible
    URHO3D_LOGWARNING("Could not determine visibility status.");
    return true;
}

void EmscriptenInput::HandleCustomPointerLockChange(bool pointerLocked)
{
    bool invalid = false;
    const bool suppress = suppressMouseModeEvent_;
    if (requestedMouseMode_ == MM_INVALID && invalidatedRequestedMouseMode_ != MM_INVALID)
    {
        invalid = true;
        requestedMouseMode_ = invalidatedRequestedMouseMode_;
        suppressMouseModeEvent_ = invalidatedSuppressMouseModeEvent_;
        invalidatedRequestedMouseMode_ = MM_INVALID;
        invalidatedSuppressMouseModeEvent_ = false;
    }

    if (pointerLocked)
    {
        // Pointer Lock is now active
        inputInst_->emscriptenPointerLock_ = true;
        inputInst_->emscriptenEnteredPointerLock_ = true;
        inputInst_->SetMouseModeEmscriptenFinal(requestedMouseMode_, suppressMouseModeEvent_);
    }
    else
    {
        // Pointer Lock is now inactive
        inputInst_->emscriptenPointerLock_ = false;

        if (inputInst_->mouseMode_ == MM_RELATIVE)
            inputInst_->SetMouseModeEmscriptenFinal(MM_FREE, suppressMouseModeEvent_);
        else if (inputInst_->mouseMode_ == MM_ABSOLUTE)
            inputInst_->SetMouseModeEmscriptenFinal(MM_ABSOLUTE, suppressMouseModeEvent_);

        inputInst_->emscriptenExitingPointerLock_ = false;
    }

    requestedMouseMode_ = MM_INVALID;
    suppressMouseModeEvent_ = false;

    invalidatedRequestedMouseMode_ = MM_INVALID;
    invalidatedSuppressMouseModeEvent_ = false;
}

using namespace emscripten;
EMSCRIPTEN_BINDINGS(Module) {
    function("JSPointerLockChange", &EmscriptenInput::HandleCustomPointerLockChange);
}

EM_BOOL EmscriptenInput::HandlePointerLockChange(int eventType, const EmscriptenPointerlockChangeEvent* keyEvent, void* userData)
{
    auto* const inputInst = (Input*)userData;

    bool invalid = false;
    const bool suppress = suppressMouseModeEvent_;
    if (requestedMouseMode_ == MM_INVALID && invalidatedRequestedMouseMode_ != MM_INVALID)
    {
        invalid = true;
        requestedMouseMode_ = invalidatedRequestedMouseMode_;
        suppressMouseModeEvent_ = invalidatedSuppressMouseModeEvent_;
        invalidatedRequestedMouseMode_ = MM_INVALID;
        invalidatedSuppressMouseModeEvent_ = false;
    }

    if (keyEvent->isActive >= EM_TRUE)
    {
        // Pointer Lock is now active
        inputInst->emscriptenPointerLock_ = true;
        inputInst->emscriptenEnteredPointerLock_ = true;
        inputInst->SetMouseModeEmscriptenFinal(requestedMouseMode_, suppressMouseModeEvent_);
    }
    else
    {
        // Pointer Lock is now inactive
        inputInst->emscriptenPointerLock_ = false;

        if (inputInst->mouseMode_ == MM_RELATIVE)
            inputInst->SetMouseModeEmscriptenFinal(MM_FREE, suppressMouseModeEvent_);
        else if (inputInst->mouseMode_ == MM_ABSOLUTE)
            inputInst->SetMouseModeEmscriptenFinal(MM_ABSOLUTE, suppressMouseModeEvent_);

        inputInst->emscriptenExitingPointerLock_ = false;
    }

    if (invalid)
    {
        if (keyEvent->isActive >= EM_TRUE)
        {
            // ExitPointerLock was called before the pointer-lock request was accepted.
            // Exit from pointer-lock to avoid unexpected behavior.
            invalidatedRequestedMouseMode_ = MM_INVALID;
            inputInst->emscriptenInput_->ExitPointerLock(suppress);
            return EM_TRUE;
        }
    }

    requestedMouseMode_ = MM_INVALID;
    suppressMouseModeEvent_ = false;

    invalidatedRequestedMouseMode_ = MM_INVALID;
    invalidatedSuppressMouseModeEvent_ = false;

    return EM_TRUE;
}

EM_BOOL EmscriptenInput::HandleFocusChange(int eventType, const EmscriptenFocusEvent* keyEvent, void* userData)
{
    auto* const inputInst = (Input*)userData;

    inputInst->SuppressNextMouseMove();

    if (eventType == EMSCRIPTEN_EVENT_FOCUSOUT)
        inputInst->LoseFocus();
    else if (eventType == EMSCRIPTEN_EVENT_FOCUS)
        inputInst->GainFocus();

    return EM_TRUE;
}

EM_BOOL EmscriptenInput::HandleMouseJump(int eventType, const EmscriptenMouseEvent * mouseEvent, void* userData)
{
    // Suppress mouse jump on pointer-lock change
    auto* const inputInst = (Input*)userData;
    bool suppress = false;
    if (eventType == EMSCRIPTEN_EVENT_MOUSEDOWN && inputInst->emscriptenEnteredPointerLock_)
    {
        suppress = true;
        inputInst->emscriptenEnteredPointerLock_ = false;
    }
    else if (eventType == EMSCRIPTEN_EVENT_MOUSEMOVE && inputInst->emscriptenExitingPointerLock_)
    {
        suppress = true;
    }

    if (suppress)
        inputInst->SuppressNextMouseMove();

    return EM_FALSE;
}

int EmscriptenInput::HandleSDLEvents(void* userData, SDL_Event* event)
{
    auto* const inputInst = (Input*)userData;

    inputInst->HandleSDLEvent(event);

    return 0;
}

#endif

#ifdef _WIN32
// On Windows repaint while the window is actively being resized.
int Win32_ResizingEventWatcher(void* data, SDL_Event* event)
{
    if (event->type == SDL_WINDOWEVENT && event->window.event == SDL_WINDOWEVENT_RESIZED)
    {
        SDL_Window* win = SDL_GetWindowFromID(event->window.windowID);
        if (win == (SDL_Window*)data)
        {
            if (auto* ctx = (Context*)SDL_GetWindowData(win, "URHO3D_CONTEXT"))
            {
                if (auto* graphics = ctx->GetSubsystem<Graphics>())
                {
                    if (graphics->IsInitialized())
                    {
                        graphics->OnWindowResized();
                        ctx->GetSubsystem<Engine>()->RunFrame();
                    }
                }
            }
        }
    }
    return 0;
}
#endif

void JoystickState::Initialize(unsigned numButtons, unsigned numAxes, unsigned numHats)
{
    buttons_.Resize(numButtons);
    buttonPress_.Resize(numButtons);
    axes_.Resize(numAxes);
    hats_.Resize(numHats);

    Reset();
}

void JoystickState::Reset()
{
    for (unsigned i = 0; i < buttons_.Size(); ++i)
    {
        buttons_[i] = false;
        buttonPress_[i] = false;
    }
    for (unsigned i = 0; i < axes_.Size(); ++i)
        axes_[i] = 0.0f;
    for (unsigned i = 0; i < hats_.Size(); ++i)
        hats_[i] = HAT_CENTER;
}

Input::Input(Context* context) :
    Object(context),
    mouseButtonDown_(0),
    mouseButtonPress_(0),
    lastVisibleMousePosition_(MOUSE_POSITION_OFFSCREEN),
    mouseMoveWheel_(0),
    inputScale_(Vector2::ONE),
    windowID_(0),
    toggleFullscreen_(true),
    mouseVisible_(false),
    lastMouseVisible_(false),
    mouseGrabbed_(false),
    lastMouseGrabbed_(false),
    mouseMode_(MM_ABSOLUTE),
    lastMouseMode_(MM_ABSOLUTE),
#ifndef __EMSCRIPTEN__
    sdlMouseRelative_(false),
#else
    emscriptenPointerLock_(false),
    emscriptenEnteredPointerLock_(false),
    emscriptenExitingPointerLock_(false),
#endif
    touchEmulation_(false),
    inputFocus_(false),
    minimized_(false),
    focusedThisFrame_(false),
    suppressNextMouseMove_(false),
    mouseMoveScaled_(false),
    initialized_(false)
{
    context_->RequireSDL(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);

    for (int i = 0; i < TOUCHID_MAX; i++)
        availableTouchIDs_.Push(i);

    SubscribeToEvent(E_SCREENMODE, URHO3D_HANDLER(Input, HandleScreenMode));

#if defined(__ANDROID__)
    // Prevent mouse events from being registered as synthetic touch events and vice versa
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
#elif defined(__EMSCRIPTEN__)
    emscriptenInput_ = new EmscriptenInput(this);
#endif

    // Try to initialize right now, but skip if screen mode is not yet set
    Initialize();
}

Input::~Input()
{
    context_->ReleaseSDL();
}

void Input::Update()
{
    assert(initialized_);

    URHO3D_PROFILE(UpdateInput);

#ifndef __EMSCRIPTEN__
    bool mouseMoved = false;
    if (mouseMove_ != IntVector2::ZERO)
        mouseMoved = true;

    ResetInputAccumulation();

    SDL_Event evt;
    while (SDL_PollEvent(&evt))
        HandleSDLEvent(&evt);

    if (suppressNextMouseMove_ && (mouseMove_ != IntVector2::ZERO || mouseMoved))
        UnsuppressMouseMove();
#endif

    // Check for focus change this frame
    SDL_Window* window = graphics_->GetWindow();
    unsigned flags = window ? SDL_GetWindowFlags(window) & (SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS) : 0;
#ifndef __EMSCRIPTEN__
    if (window)
    {
#ifdef REQUIRE_CLICK_TO_FOCUS
        // When using the "click to focus" mechanism, only focus automatically in fullscreen or non-hidden mouse mode
        if (!inputFocus_ && ((mouseVisible_ || mouseMode_ == MM_FREE) || graphics_->GetFullscreen()) && (flags & SDL_WINDOW_INPUT_FOCUS))
#else
        if (!inputFocus_ && (flags & SDL_WINDOW_INPUT_FOCUS))
#endif
            focusedThisFrame_ = true;

        if (focusedThisFrame_)
            GainFocus();

        // Check for losing focus. The window flags are not reliable when using an external window, so prevent losing focus in that case
        if (inputFocus_ && !graphics_->GetExternalWindow() && (flags & SDL_WINDOW_INPUT_FOCUS) == 0)
            LoseFocus();
    }
    else
        return;

    // Handle mouse mode MM_WRAP
    if (mouseVisible_ && mouseMode_ == MM_WRAP)
    {
        IntVector2 windowPos = graphics_->GetWindowPosition();
        IntVector2 mpos;
        SDL_GetGlobalMouseState(&mpos.x_, &mpos.y_);
        mpos -= windowPos;

        const int buffer = 5;
        const int width = graphics_->GetWidth() - buffer * 2;
        const int height = graphics_->GetHeight() - buffer * 2;

        // SetMousePosition utilizes backbuffer coordinate system, scale now from window coordinates
        mpos.x_ = (int)(mpos.x_ * inputScale_.x_);
        mpos.y_ = (int)(mpos.y_ * inputScale_.y_);

        bool warp = false;
        if (mpos.x_ < buffer)
        {
            warp = true;
            mpos.x_ += width;
        }

        if (mpos.x_ > buffer + width)
        {
            warp = true;
            mpos.x_ -= width;
        }

        if (mpos.y_ < buffer)
        {
            warp = true;
            mpos.y_ += height;
        }

        if (mpos.y_ > buffer + height)
        {
            warp = true;
            mpos.y_ -= height;
        }

        if (warp)
        {
            SetMousePosition(mpos);
            SuppressNextMouseMove();
        }
    }
#else
    if (!window)
        return;
#endif

#ifndef __EMSCRIPTEN__
    if (!touchEmulation_ && (graphics_->GetExternalWindow() || ((!sdlMouseRelative_ && !mouseVisible_ && mouseMode_ != MM_FREE) && inputFocus_ && (flags & SDL_WINDOW_MOUSE_FOCUS))))
#else
    if (!touchEmulation_ && !emscriptenPointerLock_ && (graphics_->GetExternalWindow() || (!mouseVisible_ && inputFocus_ && (flags & SDL_WINDOW_MOUSE_FOCUS))))
#endif
    {
        const IntVector2 mousePosition = GetMousePosition();
        mouseMove_ = mousePosition - lastMousePosition_;
        mouseMoveScaled_ = true; // Already in backbuffer scale, since GetMousePosition() operates in that

#ifndef __EMSCRIPTEN__
        if (graphics_->GetExternalWindow())
            lastMousePosition_ = mousePosition;
        else
        {
            // Recenter the mouse cursor manually after move
            CenterMousePosition();
        }
#else
        if (mouseMode_ == MM_ABSOLUTE || mouseMode_ == MM_FREE)
            lastMousePosition_ = mousePosition;

        if (emscriptenExitingPointerLock_)
            SuppressNextMouseMove();
#endif
        // Send mouse move event if necessary
        if (mouseMove_ != IntVector2::ZERO)
        {
            if (!suppressNextMouseMove_)
            {
                using namespace MouseMove;

                VariantMap& eventData = GetEventDataMap();

                eventData[P_X] = mousePosition.x_;
                eventData[P_Y] = mousePosition.y_;
                eventData[P_DX] = mouseMove_.x_;
                eventData[P_DY] = mouseMove_.y_;
                eventData[P_BUTTONS] = (unsigned)mouseButtonDown_;
                eventData[P_QUALIFIERS] = (unsigned)GetQualifiers();
                SendEvent(E_MOUSEMOVE, eventData);
            }
        }
    }
#ifndef __EMSCRIPTEN__
    else if (!touchEmulation_ && !mouseVisible_ && sdlMouseRelative_ && inputFocus_ && (flags & SDL_WINDOW_MOUSE_FOCUS))
    {
        // Keep the cursor trapped in window.
        CenterMousePosition();
    }
#endif
}

void Input::SetMouseVisible(bool enable, bool suppressEvent)
{
    const bool startMouseVisible = mouseVisible_;

    // In touch emulation mode only enabled mouse is allowed
    if (touchEmulation_)
        enable = true;

    // In mouse mode relative, the mouse should be invisible
    if (mouseMode_ == MM_RELATIVE)
    {
        if (!suppressEvent)
            lastMouseVisible_ = enable;

        enable = false;
    }

    // SDL Raspberry Pi "video driver" does not have proper OS mouse support yet, so no-op for now
#ifndef RPI
    if (enable != mouseVisible_)
    {
        if (initialized_)
        {
            // External windows can only support visible mouse cursor
            if (graphics_->GetExternalWindow())
            {
                mouseVisible_ = true;
                if (!suppressEvent)
                    lastMouseVisible_ = true;
                return;
            }

            if (!enable && inputFocus_)
            {
#ifndef __EMSCRIPTEN__
                if (mouseVisible_)
                    lastVisibleMousePosition_ = GetMousePosition();

                if (mouseMode_ == MM_ABSOLUTE)
                    SetMouseModeAbsolute(SDL_TRUE);
#else
                if (mouseMode_ == MM_ABSOLUTE && !emscriptenPointerLock_ || !enable)
                    emscriptenInput_->RequestPointerLock(MM_ABSOLUTE, suppressEvent);
#endif
                SDL_ShowCursor(SDL_FALSE);
                mouseVisible_ = false;
            }
            else if (mouseMode_ != MM_RELATIVE)
            {
                SetMouseGrabbed(false, suppressEvent);

                SDL_ShowCursor(SDL_TRUE);
                mouseVisible_ = true;

#ifndef __EMSCRIPTEN__
                if (mouseMode_ == MM_ABSOLUTE)
                    SetMouseModeAbsolute(SDL_FALSE);

                // Update cursor position
                auto* ui = GetSubsystem<UI>();
                Cursor* cursor = ui->GetCursor();
                // If the UI Cursor was visible, use that position instead of last visible OS cursor position
                if (cursor && cursor->IsVisible())
                {
                    IntVector2 pos = cursor->GetScreenPosition();
                    if (pos != MOUSE_POSITION_OFFSCREEN)
                    {
                        SetMousePosition(pos);
                        lastMousePosition_ = pos;
                    }
                }
                else
                {
                    if (lastVisibleMousePosition_ != MOUSE_POSITION_OFFSCREEN)
                    {
                        SetMousePosition(lastVisibleMousePosition_);
                        lastMousePosition_ = lastVisibleMousePosition_;
                    }
                }
#else
                if (mouseMode_ == MM_ABSOLUTE && emscriptenPointerLock_ || enable)
                    emscriptenInput_->ExitPointerLock(suppressEvent);
#endif
            }
        }
        else
        {
            // Allow to set desired mouse visibility before initialization
            mouseVisible_ = enable;
        }

        if (mouseVisible_ != startMouseVisible)
        {
            SuppressNextMouseMove();
            if (!suppressEvent)
            {
                lastMouseVisible_ = mouseVisible_;
                using namespace MouseVisibleChanged;

                VariantMap& eventData = GetEventDataMap();
                eventData[P_VISIBLE] = mouseVisible_;
                SendEvent(E_MOUSEVISIBLECHANGED, eventData);
            }
        }
    }
#endif
}

void Input::ResetMouseVisible()
{
#ifndef __EMSCRIPTEN__
    SetMouseVisible(lastMouseVisible_, false);
#else
    SetMouseVisibleEmscripten(lastMouseVisible_, false);
#endif
}

#ifdef __EMSCRIPTEN__
void Input::SetMouseVisibleEmscripten(bool enable, bool suppressEvent)
{
    if (enable != mouseVisible_)
    {
        if (mouseMode_ == MM_ABSOLUTE)
        {
            if (enable)
            {
                mouseVisible_ = true;
                SDL_ShowCursor(SDL_TRUE);
                emscriptenInput_->ExitPointerLock(suppressEvent);
            }
            else
            {
                if (emscriptenPointerLock_)
                {
                    mouseVisible_ = false;
                    SDL_ShowCursor(SDL_FALSE);
                }
                else
                    emscriptenInput_->RequestPointerLock(MM_ABSOLUTE, suppressEvent);
            }
        }
        else
        {
            mouseVisible_ = enable;
            SDL_ShowCursor(enable ? SDL_TRUE : SDL_FALSE);
        }
    }

    if (!suppressEvent)
        lastMouseVisible_ = mouseVisible_;
}

void Input::SetMouseModeEmscriptenFinal(MouseMode mode, bool suppressEvent)
{
    if (!suppressEvent)
        lastMouseMode_ = mode;

    mouseMode_ = mode;

    if (mode == MM_ABSOLUTE)
    {
        if (emscriptenPointerLock_)
        {
            SetMouseVisibleEmscripten(false, suppressEvent);
        }
        else
        {
            SetMouseVisibleEmscripten(true, suppressEvent);
        }

        UI* const ui = GetSubsystem<UI>();
        Cursor* const cursor = ui->GetCursor();
        SetMouseGrabbed(!(mouseVisible_ || (cursor && cursor->IsVisible())), suppressEvent);
    }
    else if (mode == MM_RELATIVE && emscriptenPointerLock_)
    {
        SetMouseGrabbed(true, suppressEvent);
        SetMouseVisibleEmscripten(false, suppressEvent);
    }
    else
    {
        SetMouseGrabbed(false, suppressEvent);
    }

    SuppressNextMouseMove();

    if (!suppressEvent)
    {
        VariantMap& eventData = GetEventDataMap();
        eventData[MouseModeChanged::P_MODE] = mode;
        eventData[MouseModeChanged::P_MOUSELOCKED] = IsMouseLocked();
        SendEvent(E_MOUSEMODECHANGED, eventData);
    }
}

void Input::SetMouseModeEmscripten(MouseMode mode, bool suppressEvent)
{
    if (mode != mouseMode_)
        SuppressNextMouseMove();

    const MouseMode previousMode = mouseMode_;
    mouseMode_ = mode;

    UI* const ui = GetSubsystem<UI>();
    Cursor* const cursor = ui->GetCursor();

    // Handle changing from previous mode
    if (previousMode == MM_RELATIVE)
        ResetMouseVisible();

    // Handle changing to new mode
    if (mode == MM_FREE)
    {
        // Attempt to cancel pending pointer-lock requests
        emscriptenInput_->ExitPointerLock(suppressEvent);
        SetMouseGrabbed(!(mouseVisible_ || (cursor && cursor->IsVisible())), suppressEvent);
    }
    else if (mode == MM_ABSOLUTE)
    {
        if (!mouseVisible_)
        {
            if (emscriptenPointerLock_)
            {
                SetMouseVisibleEmscripten(false, suppressEvent);

            }
            else
            {
                if (!cursor)
                    SetMouseVisible(true, suppressEvent);
                // Deferred mouse mode change to pointer-lock callback
                mouseMode_ = previousMode;
                emscriptenInput_->RequestPointerLock(MM_ABSOLUTE, suppressEvent);
            }

            SetMouseGrabbed(!(mouseVisible_ || (cursor && cursor->IsVisible())), suppressEvent);
        }
    }
    else if (mode == MM_RELATIVE)
    {
        if (emscriptenPointerLock_)
        {
            SetMouseVisibleEmscripten(false, true);
            SetMouseGrabbed(!(cursor && cursor->IsVisible()), suppressEvent);
        }
        else
        {
            // Defer mouse mode change to pointer-lock callback
            SetMouseGrabbed(false, true);
            mouseMode_ = previousMode;
            emscriptenInput_->RequestPointerLock(MM_RELATIVE, suppressEvent);
        }
    }
}
#endif

void Input::SetMouseGrabbed(bool grab, bool suppressEvent)
{
// To not interfere with touch UI operation, never report the mouse as grabbed on Android / iOS
#if !defined(__ANDROID__) && !defined(IOS)
    mouseGrabbed_ = grab;

    if (!suppressEvent)
        lastMouseGrabbed_ = grab;
#endif
}

void Input::ResetMouseGrabbed()
{
    SetMouseGrabbed(lastMouseGrabbed_, true);
}


#ifndef __EMSCRIPTEN__
void Input::SetMouseModeAbsolute(SDL_bool enable)
{
    SDL_Window* const window = graphics_->GetWindow();

    SDL_SetWindowGrab(window, enable);
}

void Input::SetMouseModeRelative(SDL_bool enable)
{
    SDL_Window* const window = graphics_->GetWindow();

    int result = SDL_SetRelativeMouseMode(enable);
    sdlMouseRelative_ = enable && (result == 0);

    if (result == -1)
        SDL_SetWindowGrab(window, enable);
}
#endif

void Input::SetMouseMode(MouseMode mode, bool suppressEvent)
{
    const MouseMode previousMode = mouseMode_;

#ifdef __EMSCRIPTEN__
    SetMouseModeEmscripten(mode, suppressEvent);
#else
    if (mode != mouseMode_)
    {
        if (initialized_)
        {
            SuppressNextMouseMove();

            mouseMode_ = mode;
            SDL_Window* const window = graphics_->GetWindow();

            auto* const ui = GetSubsystem<UI>();
            Cursor* const cursor = ui->GetCursor();

            // Handle changing from previous mode
            if (previousMode == MM_ABSOLUTE)
            {
                if (!mouseVisible_)
                    SetMouseModeAbsolute(SDL_FALSE);
            }
            if (previousMode == MM_RELATIVE)
            {
                SetMouseModeRelative(SDL_FALSE);
                ResetMouseVisible();
            }
            else if (previousMode == MM_WRAP)
                SDL_SetWindowGrab(window, SDL_FALSE);

            // Handle changing to new mode
            if (mode == MM_ABSOLUTE)
            {
                if (!mouseVisible_)
                    SetMouseModeAbsolute(SDL_TRUE);
            }
            else if (mode == MM_RELATIVE)
            {
                SetMouseVisible(false, true);
                SetMouseModeRelative(SDL_TRUE);
            }
            else if (mode == MM_WRAP)
            {
                SetMouseGrabbed(true, suppressEvent);
                SDL_SetWindowGrab(window, SDL_TRUE);
            }

            if (mode != MM_WRAP)
                SetMouseGrabbed(!(mouseVisible_ || (cursor && cursor->IsVisible())), suppressEvent);
        }
        else
        {
            // Allow to set desired mouse mode before initialization
            mouseMode_ = mode;
        }
    }
#endif

    if (!suppressEvent)
    {
        lastMouseMode_ = mode;

        if (mouseMode_ != previousMode)
        {
            VariantMap& eventData = GetEventDataMap();
            eventData[MouseModeChanged::P_MODE] = mode;
            eventData[MouseModeChanged::P_MOUSELOCKED] = IsMouseLocked();
            SendEvent(E_MOUSEMODECHANGED, eventData);
        }
    }
}

void Input::ResetMouseMode()
{
    SetMouseMode(lastMouseMode_, false);
}

void Input::SetToggleFullscreen(bool enable)
{
    toggleFullscreen_ = enable;
}

static void PopulateKeyBindingMap(HashMap<String, int>& keyBindingMap)
{
    if (keyBindingMap.Empty())
    {
        keyBindingMap.Insert(MakePair<String, int>("SPACE", KEY_SPACE));
        keyBindingMap.Insert(MakePair<String, int>("LCTRL", KEY_LCTRL));
        keyBindingMap.Insert(MakePair<String, int>("RCTRL", KEY_RCTRL));
        keyBindingMap.Insert(MakePair<String, int>("LSHIFT", KEY_LSHIFT));
        keyBindingMap.Insert(MakePair<String, int>("RSHIFT", KEY_RSHIFT));
        keyBindingMap.Insert(MakePair<String, int>("LALT", KEY_LALT));
        keyBindingMap.Insert(MakePair<String, int>("RALT", KEY_RALT));
        keyBindingMap.Insert(MakePair<String, int>("LGUI", KEY_LGUI));
        keyBindingMap.Insert(MakePair<String, int>("RGUI", KEY_RGUI));
        keyBindingMap.Insert(MakePair<String, int>("TAB", KEY_TAB));
        keyBindingMap.Insert(MakePair<String, int>("RETURN", KEY_RETURN));
        keyBindingMap.Insert(MakePair<String, int>("RETURN2", KEY_RETURN2));
        keyBindingMap.Insert(MakePair<String, int>("ENTER", KEY_KP_ENTER));
        keyBindingMap.Insert(MakePair<String, int>("SELECT", KEY_SELECT));
        keyBindingMap.Insert(MakePair<String, int>("LEFT", KEY_LEFT));
        keyBindingMap.Insert(MakePair<String, int>("RIGHT", KEY_RIGHT));
        keyBindingMap.Insert(MakePair<String, int>("UP", KEY_UP));
        keyBindingMap.Insert(MakePair<String, int>("DOWN", KEY_DOWN));
        keyBindingMap.Insert(MakePair<String, int>("PAGEUP", KEY_PAGEUP));
        keyBindingMap.Insert(MakePair<String, int>("PAGEDOWN", KEY_PAGEDOWN));
        keyBindingMap.Insert(MakePair<String, int>("F1", KEY_F1));
        keyBindingMap.Insert(MakePair<String, int>("F2", KEY_F2));
        keyBindingMap.Insert(MakePair<String, int>("F3", KEY_F3));
        keyBindingMap.Insert(MakePair<String, int>("F4", KEY_F4));
        keyBindingMap.Insert(MakePair<String, int>("F5", KEY_F5));
        keyBindingMap.Insert(MakePair<String, int>("F6", KEY_F6));
        keyBindingMap.Insert(MakePair<String, int>("F7", KEY_F7));
        keyBindingMap.Insert(MakePair<String, int>("F8", KEY_F8));
        keyBindingMap.Insert(MakePair<String, int>("F9", KEY_F9));
        keyBindingMap.Insert(MakePair<String, int>("F10", KEY_F10));
        keyBindingMap.Insert(MakePair<String, int>("F11", KEY_F11));
        keyBindingMap.Insert(MakePair<String, int>("F12", KEY_F12));
    }
}

static void PopulateMouseButtonBindingMap(HashMap<String, int>& mouseButtonBindingMap)
{
    if (mouseButtonBindingMap.Empty())
    {
        mouseButtonBindingMap.Insert(MakePair<String, int>("LEFT", SDL_BUTTON_LEFT));
        mouseButtonBindingMap.Insert(MakePair<String, int>("MIDDLE", SDL_BUTTON_MIDDLE));
        mouseButtonBindingMap.Insert(MakePair<String, int>("RIGHT", SDL_BUTTON_RIGHT));
        mouseButtonBindingMap.Insert(MakePair<String, int>("X1", SDL_BUTTON_X1));
        mouseButtonBindingMap.Insert(MakePair<String, int>("X2", SDL_BUTTON_X2));
    }
}

SDL_JoystickID Input::AddScreenJoystick(XMLFile* layoutFile, XMLFile* styleFile)
{
    static HashMap<String, int> keyBindingMap;
    static HashMap<String, int> mouseButtonBindingMap;

    if (!graphics_)
    {
        URHO3D_LOGWARNING("Cannot add screen joystick in headless mode");
        return -1;
    }

    // If layout file is not given, use the default screen joystick layout
    if (!layoutFile)
    {
        auto* cache = GetSubsystem<ResourceCache>();
        layoutFile = cache->GetResource<XMLFile>("UI/ScreenJoystick.xml");
        if (!layoutFile)    // Error is already logged
            return -1;
    }

    auto* ui = GetSubsystem<UI>();
    SharedPtr<UIElement> screenJoystick = ui->LoadLayout(layoutFile, styleFile);
    if (!screenJoystick)     // Error is already logged
        return -1;

    screenJoystick->SetSize(ui->GetRoot()->GetSize());
    ui->GetRoot()->AddChild(screenJoystick);

    // Get an unused ID for the screen joystick
    /// \todo After a real joystick has been plugged in 1073741824 times, the ranges will overlap
    SDL_JoystickID joystickID = SCREEN_JOYSTICK_START_ID;
    while (joysticks_.Contains(joystickID))
        ++joystickID;

    JoystickState& state = joysticks_[joystickID];
    state.joystickID_ = joystickID;
    state.name_ = screenJoystick->GetName();
    state.screenJoystick_ = screenJoystick;

    unsigned numButtons = 0;
    unsigned numAxes = 0;
    unsigned numHats = 0;
    const Vector<SharedPtr<UIElement> >& children = state.screenJoystick_->GetChildren();
    for (Vector<SharedPtr<UIElement> >::ConstIterator iter = children.Begin(); iter != children.End(); ++iter)
    {
        UIElement* element = iter->Get();
        String name = element->GetName();
        if (name.StartsWith("Button"))
        {
            ++numButtons;

            // Check whether the button has key binding
            auto* text = element->GetChildDynamicCast<Text>("KeyBinding", false);
            if (text)
            {
                text->SetVisible(false);
                const String& key = text->GetText();
                int keyBinding;
                if (key.Length() == 1)
                    keyBinding = key[0];
                else
                {
                    PopulateKeyBindingMap(keyBindingMap);

                    HashMap<String, int>::Iterator i = keyBindingMap.Find(key);
                    if (i != keyBindingMap.End())
                        keyBinding = i->second_;
                    else
                    {
                        URHO3D_LOGERRORF("Unsupported key binding: %s", key.CString());
                        keyBinding = M_MAX_INT;
                    }
                }

                if (keyBinding != M_MAX_INT)
                    element->SetVar(VAR_BUTTON_KEY_BINDING, keyBinding);
            }

            // Check whether the button has mouse button binding
            text = element->GetChildDynamicCast<Text>("MouseButtonBinding", false);
            if (text)
            {
                text->SetVisible(false);
                const String& mouseButton = text->GetText();
                PopulateMouseButtonBindingMap(mouseButtonBindingMap);

                HashMap<String, int>::Iterator i = mouseButtonBindingMap.Find(mouseButton);
                if (i != mouseButtonBindingMap.End())
                    element->SetVar(VAR_BUTTON_MOUSE_BUTTON_BINDING, i->second_);
                else
                    URHO3D_LOGERRORF("Unsupported mouse button binding: %s", mouseButton.CString());
            }
        }
        else if (name.StartsWith("Axis"))
        {
            ++numAxes;

            ///\todo Axis emulation for screen joystick is not fully supported yet.
            URHO3D_LOGWARNING("Axis emulation for screen joystick is not fully supported yet");
        }
        else if (name.StartsWith("Hat"))
        {
            ++numHats;

            auto* text = element->GetChildDynamicCast<Text>("KeyBinding", false);
            if (text)
            {
                text->SetVisible(false);
                String keyBinding = text->GetText();
                int mappedKeyBinding[4] = {KEY_W, KEY_S, KEY_A, KEY_D};
                Vector<String> keyBindings;
                if (keyBinding.Contains(' '))   // e.g.: "UP DOWN LEFT RIGHT"
                    keyBindings = keyBinding.Split(' ');    // Attempt to split the text using ' ' as separator
                else if (keyBinding.Length() == 4)
                {
                    keyBindings.Resize(4);      // e.g.: "WSAD"
                    for (unsigned i = 0; i < 4; ++i)
                        keyBindings[i] = keyBinding.Substring(i, 1);
                }
                if (keyBindings.Size() == 4)
                {
                    PopulateKeyBindingMap(keyBindingMap);

                    for (unsigned j = 0; j < 4; ++j)
                    {
                        if (keyBindings[j].Length() == 1)
                            mappedKeyBinding[j] = keyBindings[j][0];
                        else
                        {
                            HashMap<String, int>::Iterator i = keyBindingMap.Find(keyBindings[j]);
                            if (i != keyBindingMap.End())
                                mappedKeyBinding[j] = i->second_;
                            else
                                URHO3D_LOGERRORF("%s - %s cannot be mapped, fallback to '%c'", name.CString(), keyBindings[j].CString(),
                                    mappedKeyBinding[j]);
                        }
                    }
                }
                else
                    URHO3D_LOGERRORF("%s has invalid key binding %s, fallback to WSAD", name.CString(), keyBinding.CString());
                element->SetVar(VAR_BUTTON_KEY_BINDING, IntRect(mappedKeyBinding));
            }
        }

        element->SetVar(VAR_SCREEN_JOYSTICK_ID, joystickID);
    }

    // Make sure all the children are non-focusable so they do not mistakenly to be considered as active UI input controls by application
    PODVector<UIElement*> allChildren;
    state.screenJoystick_->GetChildren(allChildren, true);
    for (PODVector<UIElement*>::Iterator iter = allChildren.Begin(); iter != allChildren.End(); ++iter)
        (*iter)->SetFocusMode(FM_NOTFOCUSABLE);

    state.Initialize(numButtons, numAxes, numHats);

    // There could be potentially more than one screen joystick, however they all will be handled by a same handler method
    // So there is no harm to replace the old handler with the new handler in each call to SubscribeToEvent()
    SubscribeToEvent(E_TOUCHBEGIN, URHO3D_HANDLER(Input, HandleScreenJoystickTouch));
    SubscribeToEvent(E_TOUCHMOVE, URHO3D_HANDLER(Input, HandleScreenJoystickTouch));
    SubscribeToEvent(E_TOUCHEND, URHO3D_HANDLER(Input, HandleScreenJoystickTouch));

    return joystickID;
}

bool Input::RemoveScreenJoystick(SDL_JoystickID id)
{
    if (!joysticks_.Contains(id))
    {
        URHO3D_LOGERRORF("Failed to remove non-existing screen joystick ID #%d", id);
        return false;
    }

    JoystickState& state = joysticks_[id];
    if (!state.screenJoystick_)
    {
        URHO3D_LOGERRORF("Failed to remove joystick with ID #%d which is not a screen joystick", id);
        return false;
    }

    state.screenJoystick_->Remove();
    joysticks_.Erase(id);

    return true;
}

void Input::SetScreenJoystickVisible(SDL_JoystickID id, bool enable)
{
    if (joysticks_.Contains(id))
    {
        JoystickState& state = joysticks_[id];

        if (state.screenJoystick_)
            state.screenJoystick_->SetVisible(enable);
    }
}

void Input::SetScreenKeyboardVisible(bool enable)
{
    if (enable != SDL_IsTextInputActive())
    {
        if (enable)
            SDL_StartTextInput();
        else
            SDL_StopTextInput();
    }
}

void Input::SetTouchEmulation(bool enable)
{
#if !defined(__ANDROID__) && !defined(IOS)
    if (enable != touchEmulation_)
    {
        if (enable)
        {
            // Touch emulation needs the mouse visible
            if (!mouseVisible_)
                SetMouseVisible(true);

            // Add a virtual touch device the first time we are enabling emulated touch
            if (!SDL_GetNumTouchDevices())
                SDL_AddTouch(0, SDL_TOUCH_DEVICE_INDIRECT_RELATIVE, "Emulated Touch");
        }
        else
            ResetTouches();

        touchEmulation_ = enable;
    }
#endif
}

bool Input::RecordGesture()
{
    // If have no touch devices, fail
    if (!SDL_GetNumTouchDevices())
    {
        URHO3D_LOGERROR("Can not record gesture: no touch devices");
        return false;
    }

    return SDL_RecordGesture(-1) != 0;
}

bool Input::SaveGestures(Serializer& dest)
{
    RWOpsWrapper<Serializer> wrapper(dest);
    return SDL_SaveAllDollarTemplates(wrapper.GetRWOps()) != 0;
}

bool Input::SaveGesture(Serializer& dest, unsigned gestureID)
{
    RWOpsWrapper<Serializer> wrapper(dest);
    return SDL_SaveDollarTemplate(gestureID, wrapper.GetRWOps()) != 0;
}

unsigned Input::LoadGestures(Deserializer& source)
{
    // If have no touch devices, fail
    if (!SDL_GetNumTouchDevices())
    {
        URHO3D_LOGERROR("Can not load gestures: no touch devices");
        return 0;
    }

    RWOpsWrapper<Deserializer> wrapper(source);
    return (unsigned)SDL_LoadDollarTemplates(-1, wrapper.GetRWOps());
}


bool Input::RemoveGesture(unsigned gestureID)
{
#ifdef __EMSCRIPTEN__
    return false;
#else
    return SDL_RemoveDollarTemplate(gestureID) != 0;
#endif
}

void Input::RemoveAllGestures()
{
#ifndef __EMSCRIPTEN__
    SDL_RemoveAllDollarTemplates();
#endif
}

SDL_JoystickID Input::OpenJoystick(unsigned index)
{
    SDL_Joystick* joystick = SDL_JoystickOpen(index);
    if (!joystick)
    {
        URHO3D_LOGERRORF("Cannot open joystick #%d", index);
        return -1;
    }

    // Create joystick state for the new joystick
    int joystickID = SDL_JoystickInstanceID(joystick);
    JoystickState& state = joysticks_[joystickID];
    state.joystick_ = joystick;
    state.joystickID_ = joystickID;
    state.name_ = SDL_JoystickName(joystick);
    if (SDL_IsGameController(index))
        state.controller_ = SDL_GameControllerOpen(index);

    auto numButtons = (unsigned)SDL_JoystickNumButtons(joystick);
    auto numAxes = (unsigned)SDL_JoystickNumAxes(joystick);
    auto numHats = (unsigned)SDL_JoystickNumHats(joystick);

    // When the joystick is a controller, make sure there's enough axes & buttons for the standard controller mappings
    if (state.controller_)
    {
        if (numButtons < SDL_CONTROLLER_BUTTON_MAX)
            numButtons = SDL_CONTROLLER_BUTTON_MAX;
        if (numAxes < SDL_CONTROLLER_AXIS_MAX)
            numAxes = SDL_CONTROLLER_AXIS_MAX;
    }

    state.Initialize(numButtons, numAxes, numHats);

    return joystickID;
}

Key Input::GetKeyFromName(const String& name) const
{
    return (Key)SDL_GetKeyFromName(name.CString());
}

Key Input::GetKeyFromScancode(Scancode scancode) const
{
    return (Key)SDL_GetKeyFromScancode((SDL_Scancode)scancode);
}

String Input::GetKeyName(Key key) const
{
    return String(SDL_GetKeyName(key));
}

Scancode Input::GetScancodeFromKey(Key key) const
{
    return (Scancode)SDL_GetScancodeFromKey(key);
}

Scancode Input::GetScancodeFromName(const String& name) const
{
    return (Scancode)SDL_GetScancodeFromName(name.CString());
}

String Input::GetScancodeName(Scancode scancode) const
{
    return SDL_GetScancodeName((SDL_Scancode)scancode);
}

bool Input::GetKeyDown(Key key) const
{
    return keyDown_.Contains(SDL_tolower(key));
}

bool Input::GetKeyPress(Key key) const
{
    return keyPress_.Contains(SDL_tolower(key));
}

bool Input::GetScancodeDown(Scancode scancode) const
{
    return scancodeDown_.Contains(scancode);
}

bool Input::GetScancodePress(Scancode scancode) const
{
    return scancodePress_.Contains(scancode);
}

bool Input::GetMouseButtonDown(MouseButtonFlags button) const
{
    return mouseButtonDown_ & button;
}

bool Input::GetMouseButtonPress(MouseButtonFlags button) const
{
    return mouseButtonPress_ & button;
}

bool Input::GetQualifierDown(Qualifier qualifier) const
{
    if (qualifier == QUAL_SHIFT)
        return GetKeyDown(KEY_LSHIFT) || GetKeyDown(KEY_RSHIFT);
    if (qualifier == QUAL_CTRL)
        return GetKeyDown(KEY_LCTRL) || GetKeyDown(KEY_RCTRL);
    if (qualifier == QUAL_ALT)
        return GetKeyDown(KEY_LALT) || GetKeyDown(KEY_RALT);

    return false;
}

bool Input::GetQualifierPress(Qualifier qualifier) const
{
    if (qualifier == QUAL_SHIFT)
        return GetKeyPress(KEY_LSHIFT) || GetKeyPress(KEY_RSHIFT);
    if (qualifier == QUAL_CTRL)
        return GetKeyPress(KEY_LCTRL) || GetKeyPress(KEY_RCTRL);
    if (qualifier == QUAL_ALT)
        return GetKeyPress(KEY_LALT) || GetKeyPress(KEY_RALT);

    return false;
}

QualifierFlags Input::GetQualifiers() const
{
    QualifierFlags ret;
    if (GetQualifierDown(QUAL_SHIFT))
        ret |= QUAL_SHIFT;
    if (GetQualifierDown(QUAL_CTRL))
        ret |= QUAL_CTRL;
    if (GetQualifierDown(QUAL_ALT))
        ret |= QUAL_ALT;

    return ret;
}

IntVector2 Input::GetMousePosition() const
{
    IntVector2 ret = IntVector2::ZERO;

    if (!initialized_)
        return ret;

    SDL_GetMouseState(&ret.x_, &ret.y_);
    ret.x_ = (int)(ret.x_ * inputScale_.x_);
    ret.y_ = (int)(ret.y_ * inputScale_.y_);

    return ret;
}

IntVector2 Input::GetMouseMove() const
{
    if (!suppressNextMouseMove_)
        return mouseMoveScaled_ ? mouseMove_ : IntVector2((int)(mouseMove_.x_ * inputScale_.x_), (int)(mouseMove_.y_ * inputScale_.y_));
    else
        return IntVector2::ZERO;
}

int Input::GetMouseMoveX() const
{
    if (!suppressNextMouseMove_)
        return mouseMoveScaled_ ? mouseMove_.x_ : (int)(mouseMove_.x_ * inputScale_.x_);
    else
        return 0;
}

int Input::GetMouseMoveY() const
{
    if (!suppressNextMouseMove_)
        return mouseMoveScaled_ ? mouseMove_.y_ : mouseMove_.y_ * inputScale_.y_;
    else
        return 0;
}

TouchState* Input::GetTouch(unsigned index) const
{
    if (index >= touches_.Size())
        return nullptr;

    HashMap<int, TouchState>::ConstIterator i = touches_.Begin();
    while (index--)
        ++i;

    return const_cast<TouchState*>(&i->second_);
}

JoystickState* Input::GetJoystickByIndex(unsigned index)
{
    unsigned compare = 0;
    for (HashMap<SDL_JoystickID, JoystickState>::Iterator i = joysticks_.Begin(); i != joysticks_.End(); ++i)
    {
        if (compare++ == index)
            return &(i->second_);
    }

    return nullptr;
}

JoystickState* Input::GetJoystickByName(const String& name)
{
    for (HashMap<SDL_JoystickID, JoystickState>::Iterator i = joysticks_.Begin(); i != joysticks_.End(); ++i)
    {
        if (i->second_.name_ == name)
            return &(i->second_);
    }

    return nullptr;
}

JoystickState* Input::GetJoystick(SDL_JoystickID id)
{
    HashMap<SDL_JoystickID, JoystickState>::Iterator i = joysticks_.Find(id);
    return i != joysticks_.End() ? &(i->second_) : nullptr;
}

bool Input::IsScreenJoystickVisible(SDL_JoystickID id) const
{
    HashMap<SDL_JoystickID, JoystickState>::ConstIterator i = joysticks_.Find(id);
    return i != joysticks_.End() && i->second_.screenJoystick_ && i->second_.screenJoystick_->IsVisible();
}

bool Input::GetScreenKeyboardSupport() const
{
    return SDL_HasScreenKeyboardSupport();
}

bool Input::IsScreenKeyboardVisible() const
{
    return SDL_IsTextInputActive();
}

bool Input::IsMouseLocked() const
{
#ifdef __EMSCRIPTEN__
    return emscriptenPointerLock_;
#else
    return !((mouseMode_ == MM_ABSOLUTE && mouseVisible_) || mouseMode_ == MM_FREE);
#endif
}

bool Input::IsMinimized() const
{
    // Return minimized state also when unfocused in fullscreen
    if (!inputFocus_ && graphics_ && graphics_->GetFullscreen())
        return true;
    else
        return minimized_;
}

void Input::Initialize()
{
    auto* graphics = GetSubsystem<Graphics>();
    if (!graphics || !graphics->IsInitialized())
        return;

    graphics_ = graphics;

    // In external window mode only visible mouse is supported
    if (graphics_->GetExternalWindow())
        mouseVisible_ = true;

    // Set the initial activation
    initialized_ = true;
#ifndef __EMSCRIPTEN__
    GainFocus();
#else
    // Note: Page visibility and focus are slightly different, however we can't query last focus with Emscripten (1.29.0)
    if (emscriptenInput_->IsVisible())
        GainFocus();
    else
        LoseFocus();
#endif

    ResetJoysticks();
    ResetState();

    SubscribeToEvent(E_BEGINFRAME, URHO3D_HANDLER(Input, HandleBeginFrame));
#ifdef __EMSCRIPTEN__
    SubscribeToEvent(E_ENDFRAME, URHO3D_HANDLER(Input, HandleEndFrame));
#endif

#ifdef _WIN32
    // Register callback for resizing in order to repaint.
    if (SDL_Window* window = graphics_->GetWindow())
    {
        SDL_SetWindowData(window, "URHO3D_CONTEXT", GetContext());
        SDL_AddEventWatch(Win32_ResizingEventWatcher, window);
    }
#endif

    URHO3D_LOGINFO("Initialized input");
}

void Input::ResetJoysticks()
{
    joysticks_.Clear();

    // Open each detected joystick automatically on startup
    auto size = static_cast<unsigned>(SDL_NumJoysticks());
    for (unsigned i = 0; i < size; ++i)
        OpenJoystick(i);
}

void Input::ResetInputAccumulation()
{
    // Reset input accumulation for this frame
    keyPress_.Clear();
    scancodePress_.Clear();
    mouseButtonPress_ = MOUSEB_NONE;
    mouseMove_ = IntVector2::ZERO;
    mouseMoveWheel_ = 0;
    for (HashMap<SDL_JoystickID, JoystickState>::Iterator i = joysticks_.Begin(); i != joysticks_.End(); ++i)
    {
        for (unsigned j = 0; j < i->second_.buttonPress_.Size(); ++j)
            i->second_.buttonPress_[j] = false;
    }

    // Reset touch delta movement
    for (HashMap<int, TouchState>::Iterator i = touches_.Begin(); i != touches_.End(); ++i)
    {
        TouchState& state = i->second_;
        state.lastPosition_ = state.position_;
        state.delta_ = IntVector2::ZERO;
    }
}

void Input::GainFocus()
{
    ResetState();

    inputFocus_ = true;
    focusedThisFrame_ = false;

    // Restore mouse mode
#ifndef __EMSCRIPTEN__
    const MouseMode mm = mouseMode_;
    mouseMode_ = MM_FREE;
    SetMouseMode(mm, true);
#endif

    SuppressNextMouseMove();

    // Re-establish mouse cursor hiding as necessary
    if (!mouseVisible_)
        SDL_ShowCursor(SDL_FALSE);

    SendInputFocusEvent();
}

void Input::LoseFocus()
{
    ResetState();

    inputFocus_ = false;
    focusedThisFrame_ = false;

    // Show the mouse cursor when inactive
    SDL_ShowCursor(SDL_TRUE);

    // Change mouse mode -- removing any cursor grabs, etc.
#ifndef __EMSCRIPTEN__
    const MouseMode mm = mouseMode_;
    SetMouseMode(MM_FREE, true);
    // Restore flags to reflect correct mouse state.
    mouseMode_ = mm;
#endif

    SendInputFocusEvent();
}

void Input::ResetState()
{
    keyDown_.Clear();
    keyPress_.Clear();
    scancodeDown_.Clear();
    scancodePress_.Clear();

    /// \todo Check if resetting joystick state on input focus loss is even necessary
    for (HashMap<SDL_JoystickID, JoystickState>::Iterator i = joysticks_.Begin(); i != joysticks_.End(); ++i)
        i->second_.Reset();

    ResetTouches();

    // Use SetMouseButton() to reset the state so that mouse events will be sent properly
    SetMouseButton(MOUSEB_LEFT, false, 0);
    SetMouseButton(MOUSEB_RIGHT, false, 0);
    SetMouseButton(MOUSEB_MIDDLE, false, 0);

    mouseMove_ = IntVector2::ZERO;
    mouseMoveWheel_ = 0;
    mouseButtonPress_ = MOUSEB_NONE;
}

void Input::ResetTouches()
{
    for (HashMap<int, TouchState>::Iterator i = touches_.Begin(); i != touches_.End(); ++i)
    {
        TouchState& state = i->second_;

        using namespace TouchEnd;

        VariantMap& eventData = GetEventDataMap();
        eventData[P_TOUCHID] = state.touchID_;
        eventData[P_X] = state.position_.x_;
        eventData[P_Y] = state.position_.y_;
        SendEvent(E_TOUCHEND, eventData);
    }

    touches_.Clear();
    touchIDMap_.Clear();
    availableTouchIDs_.Clear();
    for (int i = 0; i < TOUCHID_MAX; i++)
        availableTouchIDs_.Push(i);

}

unsigned Input::GetTouchIndexFromID(int touchID)
{
    HashMap<int, int>::ConstIterator i = touchIDMap_.Find(touchID);
    if (i != touchIDMap_.End())
    {
        return (unsigned)i->second_;
    }

    unsigned index = PopTouchIndex();
    touchIDMap_[touchID] = index;
    return index;
}

unsigned Input::PopTouchIndex()
{
    if (availableTouchIDs_.Empty())
        return 0;

    auto index = (unsigned)availableTouchIDs_.Front();
    availableTouchIDs_.PopFront();
    return index;
}

void Input::PushTouchIndex(int touchID)
{
    HashMap<int, int>::ConstIterator ci = touchIDMap_.Find(touchID);
    if (ci == touchIDMap_.End())
        return;

    int index = touchIDMap_[touchID];
    touchIDMap_.Erase(touchID);

    // Sorted insertion
    bool inserted = false;
    for (List<int>::Iterator i = availableTouchIDs_.Begin(); i != availableTouchIDs_.End(); ++i)
    {
        if (*i == index)
        {
            // This condition can occur when TOUCHID_MAX is reached.
            inserted = true;
            break;
        }

        if (*i > index)
        {
            availableTouchIDs_.Insert(i, index);
            inserted = true;
            break;
        }
    }

    // If empty, or the lowest value then insert at end.
    if (!inserted)
        availableTouchIDs_.Push(index);
}

void Input::SendInputFocusEvent()
{
    using namespace InputFocus;

    VariantMap& eventData = GetEventDataMap();
    eventData[P_FOCUS] = HasFocus();
    eventData[P_MINIMIZED] = IsMinimized();
    SendEvent(E_INPUTFOCUS, eventData);
}

void Input::SetMouseButton(MouseButton button, bool newState, int clicks)
{
    if (newState)
    {
        if (!(mouseButtonDown_ & button))
            mouseButtonPress_ |= button;

        mouseButtonDown_ |= button;
    }
    else
    {
        if (!(mouseButtonDown_ & button))
            return;

        mouseButtonDown_ &= ~button;
    }

    using namespace MouseButtonDown;

    VariantMap& eventData = GetEventDataMap();
    eventData[P_BUTTON] = button;
    eventData[P_BUTTONS] = (unsigned)mouseButtonDown_;
    eventData[P_QUALIFIERS] = (unsigned)GetQualifiers();
    eventData[P_CLICKS] = clicks;
    SendEvent(newState ? E_MOUSEBUTTONDOWN : E_MOUSEBUTTONUP, eventData);
}

void Input::SetKey(Key key, Scancode scancode, bool newState)
{
    bool repeat = false;

    if (newState)
    {
        scancodeDown_.Insert(scancode);
        scancodePress_.Insert(scancode);

        if (!keyDown_.Contains(key))
        {
            keyDown_.Insert(key);
            keyPress_.Insert(key);
        }
        else
            repeat = true;
    }
    else
    {
        scancodeDown_.Erase(scancode);

        if (!keyDown_.Erase(key))
            return;
    }

    using namespace KeyDown;

    VariantMap& eventData = GetEventDataMap();
    eventData[P_KEY] = key;
    eventData[P_SCANCODE] = scancode;
    eventData[P_BUTTONS] = (unsigned)mouseButtonDown_;
    eventData[P_QUALIFIERS] = (unsigned)GetQualifiers();
    if (newState)
        eventData[P_REPEAT] = repeat;
    SendEvent(newState ? E_KEYDOWN : E_KEYUP, eventData);

    if ((key == KEY_RETURN || key == KEY_RETURN2 || key == KEY_KP_ENTER) && newState && !repeat && toggleFullscreen_ &&
        (GetKeyDown(KEY_LALT) || GetKeyDown(KEY_RALT)))
        graphics_->ToggleFullscreen();
}

void Input::SetMouseWheel(int delta)
{
    if (delta)
    {
        mouseMoveWheel_ += delta;

        using namespace MouseWheel;

        VariantMap& eventData = GetEventDataMap();
        eventData[P_WHEEL] = delta;
        eventData[P_BUTTONS] = (unsigned)mouseButtonDown_;
        eventData[P_QUALIFIERS] = (unsigned)GetQualifiers();
        SendEvent(E_MOUSEWHEEL, eventData);
    }
}

void Input::SetMousePosition(const IntVector2& position)
{
    if (!graphics_)
        return;

    SDL_WarpMouseInWindow(graphics_->GetWindow(), (int)(position.x_ / inputScale_.x_), (int)(position.y_ / inputScale_.y_));
}

void Input::CenterMousePosition()
{
    const IntVector2 center(graphics_->GetWidth() / 2, graphics_->GetHeight() / 2);
    if (GetMousePosition() != center)
    {
        SetMousePosition(center);
        lastMousePosition_ = center;
    }
}

void Input::SuppressNextMouseMove()
{
    suppressNextMouseMove_ = true;
    mouseMove_ = IntVector2::ZERO;
}

void Input::UnsuppressMouseMove()
{
    suppressNextMouseMove_ = false;
    mouseMove_ = IntVector2::ZERO;
    lastMousePosition_ = GetMousePosition();
}

void Input::HandleSDLEvent(void* sdlEvent)
{
    SDL_Event& evt = *static_cast<SDL_Event*>(sdlEvent);

    // While not having input focus, skip key/mouse/touch/joystick events, except for the "click to focus" mechanism
    if (!inputFocus_ && evt.type >= SDL_KEYDOWN && evt.type <= SDL_MULTIGESTURE)
    {
#ifdef REQUIRE_CLICK_TO_FOCUS
        // Require the click to be at least 1 pixel inside the window to disregard clicks in the title bar
        if (evt.type == SDL_MOUSEBUTTONDOWN && evt.button.x > 0 && evt.button.y > 0 && evt.button.x < graphics_->GetWidth() - 1 &&
            evt.button.y < graphics_->GetHeight() - 1)
        {
            focusedThisFrame_ = true;
            // Do not cause the click to actually go throughfin
            return;
        }
        else if (evt.type == SDL_FINGERDOWN)
        {
            // When focusing by touch, call GainFocus() immediately as it resets the state; a touch has sustained state
            // which should be kept
            GainFocus();
        }
        else
#endif
            return;
    }

    // Possibility for custom handling or suppression of default handling for the SDL event
    {
        using namespace SDLRawInput;

        VariantMap eventData = GetEventDataMap();
        eventData[P_SDLEVENT] = &evt;
        eventData[P_CONSUMED] = false;
        SendEvent(E_SDLRAWINPUT, eventData);

        if (eventData[P_CONSUMED].GetBool())
            return;
    }

    switch (evt.type)
    {
    case SDL_KEYDOWN:
        SetKey(ConvertSDLKeyCode(evt.key.keysym.sym, evt.key.keysym.scancode), (Scancode)evt.key.keysym.scancode, true);
        break;

    case SDL_KEYUP:
        SetKey(ConvertSDLKeyCode(evt.key.keysym.sym, evt.key.keysym.scancode), (Scancode)evt.key.keysym.scancode, false);
        break;

    case SDL_TEXTINPUT:
        {
            using namespace TextInput;

            VariantMap textInputEventData;
            textInputEventData[P_TEXT] = textInput_ = &evt.text.text[0];
            SendEvent(E_TEXTINPUT, textInputEventData);
        }
        break;

    case SDL_TEXTEDITING:
        {
            using namespace TextEditing;

            VariantMap textEditingEventData;
            textEditingEventData[P_COMPOSITION] = &evt.edit.text[0];
            textEditingEventData[P_CURSOR] = evt.edit.start;
            textEditingEventData[P_SELECTION_LENGTH] = evt.edit.length;
            SendEvent(E_TEXTEDITING, textEditingEventData);
        }
        break;

    case SDL_MOUSEBUTTONDOWN:
        if (!touchEmulation_)
        {
            const auto mouseButton = static_cast<MouseButton>(1u << (evt.button.button - 1u));  // NOLINT(misc-misplaced-widening-cast)
            SetMouseButton(mouseButton, true, evt.button.clicks);
        }
        else
        {
            int x, y;
            SDL_GetMouseState(&x, &y);
            x = (int)(x * inputScale_.x_);
            y = (int)(y * inputScale_.y_);

            SDL_Event event;
            event.type = SDL_FINGERDOWN;
            event.tfinger.touchId = 0;
            event.tfinger.fingerId = evt.button.button - 1;
            event.tfinger.pressure = 1.0f;
            event.tfinger.x = (float)x / (float)graphics_->GetWidth();
            event.tfinger.y = (float)y / (float)graphics_->GetHeight();
            event.tfinger.dx = 0;
            event.tfinger.dy = 0;
            SDL_PushEvent(&event);
        }
        break;

    case SDL_MOUSEBUTTONUP:
        if (!touchEmulation_)
        {
            const auto mouseButton = static_cast<MouseButton>(1u << (evt.button.button - 1u));  // NOLINT(misc-misplaced-widening-cast)
            SetMouseButton(mouseButton, false, evt.button.clicks);
        }
        else
        {
            int x, y;
            SDL_GetMouseState(&x, &y);
            x = (int)(x * inputScale_.x_);
            y = (int)(y * inputScale_.y_);

            SDL_Event event;
            event.type = SDL_FINGERUP;
            event.tfinger.touchId = 0;
            event.tfinger.fingerId = evt.button.button - 1;
            event.tfinger.pressure = 0.0f;
            event.tfinger.x = (float)x / (float)graphics_->GetWidth();
            event.tfinger.y = (float)y / (float)graphics_->GetHeight();
            event.tfinger.dx = 0;
            event.tfinger.dy = 0;
            SDL_PushEvent(&event);
        }
        break;

    case SDL_MOUSEMOTION:
#ifndef __EMSCRIPTEN__
        if ((sdlMouseRelative_ || mouseVisible_ || mouseMode_ == MM_FREE) && !touchEmulation_)
#else
        if ((mouseVisible_ || emscriptenPointerLock_ || mouseMode_ == MM_FREE) && !touchEmulation_)
#endif
        {
#ifdef __EMSCRIPTEN__
            if (emscriptenExitingPointerLock_)
            {
                SuppressNextMouseMove();
                break;
            }
#endif

            // Accumulate without scaling for accuracy, needs to be scaled to backbuffer coordinates when asked
            mouseMove_.x_ += evt.motion.xrel;
            mouseMove_.y_ += evt.motion.yrel;
            mouseMoveScaled_ = false;

            if (!suppressNextMouseMove_)
            {
                using namespace MouseMove;

                VariantMap& eventData = GetEventDataMap();
                eventData[P_X] = (int)(evt.motion.x * inputScale_.x_);
                eventData[P_Y] = (int)(evt.motion.y * inputScale_.y_);
                // The "on-the-fly" motion data needs to be scaled now, though this may reduce accuracy
                eventData[P_DX] = (int)(evt.motion.xrel * inputScale_.x_);
                eventData[P_DY] = (int)(evt.motion.yrel * inputScale_.y_);
                eventData[P_BUTTONS] = (unsigned)mouseButtonDown_;
                eventData[P_QUALIFIERS] = (unsigned)GetQualifiers();
                SendEvent(E_MOUSEMOVE, eventData);
            }
        }
        // Only the left mouse button "finger" moves along with the mouse movement
        else if (touchEmulation_ && touches_.Contains(0))
        {
            int x, y;
            SDL_GetMouseState(&x, &y);
            x = (int)(x * inputScale_.x_);
            y = (int)(y * inputScale_.y_);

            SDL_Event event;
            event.type = SDL_FINGERMOTION;
            event.tfinger.touchId = 0;
            event.tfinger.fingerId = 0;
            event.tfinger.pressure = 1.0f;
            event.tfinger.x = (float)x / (float)graphics_->GetWidth();
            event.tfinger.y = (float)y / (float)graphics_->GetHeight();
            event.tfinger.dx = (float)evt.motion.xrel * inputScale_.x_ / (float)graphics_->GetWidth();
            event.tfinger.dy = (float)evt.motion.yrel * inputScale_.y_ / (float)graphics_->GetHeight();
            SDL_PushEvent(&event);
        }
        break;

    case SDL_MOUSEWHEEL:
        if (!touchEmulation_)
            SetMouseWheel(evt.wheel.y);
        break;

    case SDL_FINGERDOWN:
        if (evt.tfinger.touchId != SDL_TOUCH_MOUSEID)
        {
            int touchID = GetTouchIndexFromID(evt.tfinger.fingerId & 0x7ffffffu);
            TouchState& state = touches_[touchID];
            state.touchID_ = touchID;
            state.lastPosition_ = state.position_ = IntVector2((int)(evt.tfinger.x * graphics_->GetWidth()),
                (int)(evt.tfinger.y * graphics_->GetHeight()));
            state.delta_ = IntVector2::ZERO;
            state.pressure_ = evt.tfinger.pressure;

            using namespace TouchBegin;

            VariantMap& eventData = GetEventDataMap();
            eventData[P_TOUCHID] = touchID;
            eventData[P_X] = state.position_.x_;
            eventData[P_Y] = state.position_.y_;
            eventData[P_PRESSURE] = state.pressure_;
            SendEvent(E_TOUCHBEGIN, eventData);

            // Finger touch may move the mouse cursor. Suppress next mouse move when cursor hidden to prevent jumps
            if (!mouseVisible_)
                SuppressNextMouseMove();
        }
        break;

    case SDL_FINGERUP:
        if (evt.tfinger.touchId != SDL_TOUCH_MOUSEID)
        {
            int touchID = GetTouchIndexFromID(evt.tfinger.fingerId & 0x7ffffffu);
            TouchState& state = touches_[touchID];

            using namespace TouchEnd;

            VariantMap& eventData = GetEventDataMap();
            // Do not trust the position in the finger up event. Instead use the last position stored in the
            // touch structure
            eventData[P_TOUCHID] = touchID;
            eventData[P_X] = state.position_.x_;
            eventData[P_Y] = state.position_.y_;
            SendEvent(E_TOUCHEND, eventData);

            // Add touch index back to list of available touch Ids
            PushTouchIndex(evt.tfinger.fingerId & 0x7ffffffu);

            touches_.Erase(touchID);
        }
        break;

    case SDL_FINGERMOTION:
        if (evt.tfinger.touchId != SDL_TOUCH_MOUSEID)
        {
            int touchID = GetTouchIndexFromID(evt.tfinger.fingerId & 0x7ffffffu);
            // We don't want this event to create a new touches_ event if it doesn't exist (touchEmulation)
            if (touchEmulation_ && !touches_.Contains(touchID))
                break;
            TouchState& state = touches_[touchID];
            state.touchID_ = touchID;
            state.position_ = IntVector2((int)(evt.tfinger.x * graphics_->GetWidth()),
                (int)(evt.tfinger.y * graphics_->GetHeight()));
            state.delta_ = state.position_ - state.lastPosition_;
            state.pressure_ = evt.tfinger.pressure;

            using namespace TouchMove;

            VariantMap& eventData = GetEventDataMap();
            eventData[P_TOUCHID] = touchID;
            eventData[P_X] = state.position_.x_;
            eventData[P_Y] = state.position_.y_;
            eventData[P_DX] = (int)(evt.tfinger.dx * graphics_->GetWidth());
            eventData[P_DY] = (int)(evt.tfinger.dy * graphics_->GetHeight());
            eventData[P_PRESSURE] = state.pressure_;
            SendEvent(E_TOUCHMOVE, eventData);

            // Finger touch may move the mouse cursor. Suppress next mouse move when cursor hidden to prevent jumps
            if (!mouseVisible_)
                SuppressNextMouseMove();
        }
        break;

    case SDL_DOLLARRECORD:
        {
            using namespace GestureRecorded;

            VariantMap& eventData = GetEventDataMap();
            eventData[P_GESTUREID] = (int)evt.dgesture.gestureId;
            SendEvent(E_GESTURERECORDED, eventData);
        }
        break;

    case SDL_DOLLARGESTURE:
        {
            using namespace GestureInput;

            VariantMap& eventData = GetEventDataMap();
            eventData[P_GESTUREID] = (int)evt.dgesture.gestureId;
            eventData[P_CENTERX] = (int)(evt.dgesture.x * graphics_->GetWidth());
            eventData[P_CENTERY] = (int)(evt.dgesture.y * graphics_->GetHeight());
            eventData[P_NUMFINGERS] = (int)evt.dgesture.numFingers;
            eventData[P_ERROR] = evt.dgesture.error;
            SendEvent(E_GESTUREINPUT, eventData);
        }
        break;

    case SDL_MULTIGESTURE:
        {
            using namespace MultiGesture;

            VariantMap& eventData = GetEventDataMap();
            eventData[P_CENTERX] = (int)(evt.mgesture.x * graphics_->GetWidth());
            eventData[P_CENTERY] = (int)(evt.mgesture.y * graphics_->GetHeight());
            eventData[P_NUMFINGERS] = (int)evt.mgesture.numFingers;
            eventData[P_DTHETA] = M_RADTODEG * evt.mgesture.dTheta;
            eventData[P_DDIST] = evt.mgesture.dDist;
            SendEvent(E_MULTIGESTURE, eventData);
        }
        break;

    case SDL_JOYDEVICEADDED:
        {
            using namespace JoystickConnected;

            SDL_JoystickID joystickID = OpenJoystick((unsigned)evt.jdevice.which);

            VariantMap& eventData = GetEventDataMap();
            eventData[P_JOYSTICKID] = joystickID;
            SendEvent(E_JOYSTICKCONNECTED, eventData);
        }
        break;

    case SDL_JOYDEVICEREMOVED:
        {
            using namespace JoystickDisconnected;

            joysticks_.Erase(evt.jdevice.which);

            VariantMap& eventData = GetEventDataMap();
            eventData[P_JOYSTICKID] = evt.jdevice.which;
            SendEvent(E_JOYSTICKDISCONNECTED, eventData);
        }
        break;

    case SDL_JOYBUTTONDOWN:
        {
            using namespace JoystickButtonDown;

            unsigned button = evt.jbutton.button;
            SDL_JoystickID joystickID = evt.jbutton.which;
            JoystickState& state = joysticks_[joystickID];

            // Skip ordinary joystick event for a controller
            if (!state.controller_)
            {
                VariantMap& eventData = GetEventDataMap();
                eventData[P_JOYSTICKID] = joystickID;
                eventData[P_BUTTON] = button;

                if (button < state.buttons_.Size())
                {
                    state.buttons_[button] = true;
                    state.buttonPress_[button] = true;
                    SendEvent(E_JOYSTICKBUTTONDOWN, eventData);
                }
            }
        }
        break;

    case SDL_JOYBUTTONUP:
        {
            using namespace JoystickButtonUp;

            unsigned button = evt.jbutton.button;
            SDL_JoystickID joystickID = evt.jbutton.which;
            JoystickState& state = joysticks_[joystickID];

            if (!state.controller_)
            {
                VariantMap& eventData = GetEventDataMap();
                eventData[P_JOYSTICKID] = joystickID;
                eventData[P_BUTTON] = button;

                if (button < state.buttons_.Size())
                {
                    if (!state.controller_)
                        state.buttons_[button] = false;
                    SendEvent(E_JOYSTICKBUTTONUP, eventData);
                }
            }
        }
        break;

    case SDL_JOYAXISMOTION:
        {
            using namespace JoystickAxisMove;

            SDL_JoystickID joystickID = evt.jaxis.which;
            JoystickState& state = joysticks_[joystickID];

            if (!state.controller_)
            {
                VariantMap& eventData = GetEventDataMap();
                eventData[P_JOYSTICKID] = joystickID;
                eventData[P_AXIS] = evt.jaxis.axis;
                eventData[P_POSITION] = Clamp((float)evt.jaxis.value / 32767.0f, -1.0f, 1.0f);

                if (evt.jaxis.axis < state.axes_.Size())
                {
                    // If the joystick is a controller, only use the controller axis mappings
                    // (we'll also get the controller event)
                    if (!state.controller_)
                        state.axes_[evt.jaxis.axis] = eventData[P_POSITION].GetFloat();
                    SendEvent(E_JOYSTICKAXISMOVE, eventData);
                }
            }
        }
        break;

    case SDL_JOYHATMOTION:
        {
            using namespace JoystickHatMove;

            SDL_JoystickID joystickID = evt.jaxis.which;
            JoystickState& state = joysticks_[joystickID];

            VariantMap& eventData = GetEventDataMap();
            eventData[P_JOYSTICKID] = joystickID;
            eventData[P_HAT] = evt.jhat.hat;
            eventData[P_POSITION] = evt.jhat.value;

            if (evt.jhat.hat < state.hats_.Size())
            {
                state.hats_[evt.jhat.hat] = evt.jhat.value;
                SendEvent(E_JOYSTICKHATMOVE, eventData);
            }
        }
        break;

    case SDL_CONTROLLERBUTTONDOWN:
        {
            using namespace JoystickButtonDown;

            unsigned button = evt.cbutton.button;
            SDL_JoystickID joystickID = evt.cbutton.which;
            JoystickState& state = joysticks_[joystickID];

            VariantMap& eventData = GetEventDataMap();
            eventData[P_JOYSTICKID] = joystickID;
            eventData[P_BUTTON] = button;

            if (button < state.buttons_.Size())
            {
                state.buttons_[button] = true;
                state.buttonPress_[button] = true;
                SendEvent(E_JOYSTICKBUTTONDOWN, eventData);
            }
        }
        break;

    case SDL_CONTROLLERBUTTONUP:
        {
            using namespace JoystickButtonUp;

            unsigned button = evt.cbutton.button;
            SDL_JoystickID joystickID = evt.cbutton.which;
            JoystickState& state = joysticks_[joystickID];

            VariantMap& eventData = GetEventDataMap();
            eventData[P_JOYSTICKID] = joystickID;
            eventData[P_BUTTON] = button;

            if (button < state.buttons_.Size())
            {
                state.buttons_[button] = false;
                SendEvent(E_JOYSTICKBUTTONUP, eventData);
            }
        }
        break;

    case SDL_CONTROLLERAXISMOTION:
        {
            using namespace JoystickAxisMove;

            SDL_JoystickID joystickID = evt.caxis.which;
            JoystickState& state = joysticks_[joystickID];

            VariantMap& eventData = GetEventDataMap();
            eventData[P_JOYSTICKID] = joystickID;
            eventData[P_AXIS] = evt.caxis.axis;
            eventData[P_POSITION] = Clamp((float)evt.caxis.value / 32767.0f, -1.0f, 1.0f);

            if (evt.caxis.axis < state.axes_.Size())
            {
                state.axes_[evt.caxis.axis] = eventData[P_POSITION].GetFloat();
                SendEvent(E_JOYSTICKAXISMOVE, eventData);
            }
        }
        break;

    case SDL_WINDOWEVENT:
        {
            switch (evt.window.event)
            {
            case SDL_WINDOWEVENT_MINIMIZED:
                minimized_ = true;
                SendInputFocusEvent();
                break;

            case SDL_WINDOWEVENT_MAXIMIZED:
            case SDL_WINDOWEVENT_RESTORED:
#if defined(IOS) || defined(TVOS) || defined (__ANDROID__)
                // On iOS/tvOS we never lose the GL context, but may have done GPU object changes that could not be applied yet. Apply them now
                // On Android the old GL context may be lost already, restore GPU objects to the new GL context
                graphics_->Restore();
#endif
                minimized_ = false;
                SendInputFocusEvent();
                break;

            case SDL_WINDOWEVENT_RESIZED:
                graphics_->OnWindowResized();
                break;
            case SDL_WINDOWEVENT_MOVED:
                graphics_->OnWindowMoved();
                break;

            default: break;
            }
        }
        break;

    case SDL_DROPFILE:
        {
            using namespace DropFile;

            VariantMap& eventData = GetEventDataMap();
            eventData[P_FILENAME] = GetInternalPath(String(evt.drop.file));
            SDL_free(evt.drop.file);

            SendEvent(E_DROPFILE, eventData);
        }
        break;

    case SDL_QUIT:
        SendEvent(E_EXITREQUESTED);
        break;

    default: break;
    }
}

void Input::HandleScreenMode(StringHash eventType, VariantMap& eventData)
{
    if (!initialized_)
        Initialize();

    // Re-enable cursor clipping, and re-center the cursor (if needed) to the new screen size, so that there is no erroneous
    // mouse move event. Also get new window ID if it changed
    SDL_Window* window = graphics_->GetWindow();
    windowID_ = SDL_GetWindowID(window);

    // Resize screen joysticks to new screen size
    for (HashMap<SDL_JoystickID, JoystickState>::Iterator i = joysticks_.Begin(); i != joysticks_.End(); ++i)
    {
        UIElement* screenjoystick = i->second_.screenJoystick_;
        if (screenjoystick)
            screenjoystick->SetSize(graphics_->GetWidth(), graphics_->GetHeight());
    }

    if (graphics_->GetFullscreen() || !mouseVisible_)
        focusedThisFrame_ = true;

    // After setting a new screen mode we should not be minimized
    minimized_ = (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) != 0;

    // Calculate input coordinate scaling from SDL window to backbuffer ratio
    int winWidth, winHeight;
    int gfxWidth = graphics_->GetWidth();
    int gfxHeight = graphics_->GetHeight();
    SDL_GetWindowSize(window, &winWidth, &winHeight);
    if (winWidth > 0 && winHeight > 0 && gfxWidth > 0 && gfxHeight > 0)
    {
        inputScale_.x_ = (float)gfxWidth / (float)winWidth;
        inputScale_.y_ = (float)gfxHeight / (float)winHeight;
    }
    else
        inputScale_ = Vector2::ONE;
}

void Input::HandleBeginFrame(StringHash eventType, VariantMap& eventData)
{
    // Update input right at the beginning of the frame
    SendEvent(E_INPUTBEGIN);
    Update();
    SendEvent(E_INPUTEND);
}

#ifdef __EMSCRIPTEN__
void Input::HandleEndFrame(StringHash eventType, VariantMap& eventData)
{
    if (suppressNextMouseMove_ && mouseMove_ != IntVector2::ZERO)
        UnsuppressMouseMove();

    ResetInputAccumulation();
}
#endif

void Input::HandleScreenJoystickTouch(StringHash eventType, VariantMap& eventData)
{
    using namespace TouchBegin;

    // Only interested in events from screen joystick(s)
    TouchState& state = touches_[eventData[P_TOUCHID].GetInt()];
    IntVector2 position(int(state.position_.x_ / GetSubsystem<UI>()->GetScale()), int(state.position_.y_ / GetSubsystem<UI>()->GetScale()));
    UIElement* element = eventType == E_TOUCHBEGIN ? GetSubsystem<UI>()->GetElementAt(position) : state.touchedElement_;
    if (!element)
        return;
    Variant variant = element->GetVar(VAR_SCREEN_JOYSTICK_ID);
    if (variant.IsEmpty())
        return;
    SDL_JoystickID joystickID = variant.GetInt();

    if (eventType == E_TOUCHEND)
        state.touchedElement_.Reset();
    else
        state.touchedElement_ = element;

    // Prepare a fake SDL event
    SDL_Event evt;

    const String& name = element->GetName();
    if (name.StartsWith("Button"))
    {
        if (eventType == E_TOUCHMOVE)
            return;

        // Determine whether to inject a joystick event or keyboard/mouse event
        const Variant& keyBindingVar = element->GetVar(VAR_BUTTON_KEY_BINDING);
        const Variant& mouseButtonBindingVar = element->GetVar(VAR_BUTTON_MOUSE_BUTTON_BINDING);
        if (keyBindingVar.IsEmpty() && mouseButtonBindingVar.IsEmpty())
        {
            evt.type = eventType == E_TOUCHBEGIN ? SDL_JOYBUTTONDOWN : SDL_JOYBUTTONUP;
            evt.jbutton.which = joystickID;
            evt.jbutton.button = (Uint8)ToUInt(name.Substring(6));
        }
        else
        {
            if (!keyBindingVar.IsEmpty())
            {
                evt.type = eventType == E_TOUCHBEGIN ? SDL_KEYDOWN : SDL_KEYUP;
                evt.key.keysym.sym = ToLower(keyBindingVar.GetInt());
                evt.key.keysym.scancode = SDL_SCANCODE_UNKNOWN;
            }
            if (!mouseButtonBindingVar.IsEmpty())
            {
                // Mouse button are sent as extra events besides key events
                // Disable touch emulation handling during this to prevent endless loop
                bool oldTouchEmulation = touchEmulation_;
                touchEmulation_ = false;

                SDL_Event mouseEvent;
                mouseEvent.type = eventType == E_TOUCHBEGIN ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
                mouseEvent.button.button = (Uint8)mouseButtonBindingVar.GetInt();
                HandleSDLEvent(&mouseEvent);

                touchEmulation_ = oldTouchEmulation;
            }
        }
    }
    else if (name.StartsWith("Hat"))
    {
        Variant keyBindingVar = element->GetVar(VAR_BUTTON_KEY_BINDING);
        if (keyBindingVar.IsEmpty())
        {
            evt.type = SDL_JOYHATMOTION;
            evt.jaxis.which = joystickID;
            evt.jhat.hat = (Uint8)ToUInt(name.Substring(3));
            evt.jhat.value = HAT_CENTER;
            if (eventType != E_TOUCHEND)
            {
                IntVector2 relPosition = position - element->GetScreenPosition() - element->GetSize() / 2;
                if (relPosition.y_ < 0 && Abs(relPosition.x_ * 3 / 2) < Abs(relPosition.y_))
                    evt.jhat.value |= HAT_UP;
                if (relPosition.y_ > 0 && Abs(relPosition.x_ * 3 / 2) < Abs(relPosition.y_))
                    evt.jhat.value |= HAT_DOWN;
                if (relPosition.x_ < 0 && Abs(relPosition.y_ * 3 / 2) < Abs(relPosition.x_))
                    evt.jhat.value |= HAT_LEFT;
                if (relPosition.x_ > 0 && Abs(relPosition.y_ * 3 / 2) < Abs(relPosition.x_))
                    evt.jhat.value |= HAT_RIGHT;
            }
        }
        else
        {
            // Hat is binded by 4 integers representing keysyms for 'w', 's', 'a', 'd' or something similar
            IntRect keyBinding = keyBindingVar.GetIntRect();

            if (eventType == E_TOUCHEND)
            {
                evt.type = SDL_KEYUP;
                evt.key.keysym.sym = element->GetVar(VAR_LAST_KEYSYM).GetInt();
                if (!evt.key.keysym.sym)
                    return;

                element->SetVar(VAR_LAST_KEYSYM, 0);
            }
            else
            {
                evt.type = SDL_KEYDOWN;
                IntVector2 relPosition = position - element->GetScreenPosition() - element->GetSize() / 2;
                if (relPosition.y_ < 0 && Abs(relPosition.x_ * 3 / 2) < Abs(relPosition.y_))
                    evt.key.keysym.sym = keyBinding.left_;      // The integers are encoded in WSAD order to l-t-r-b
                else if (relPosition.y_ > 0 && Abs(relPosition.x_ * 3 / 2) < Abs(relPosition.y_))
                    evt.key.keysym.sym = keyBinding.top_;
                else if (relPosition.x_ < 0 && Abs(relPosition.y_ * 3 / 2) < Abs(relPosition.x_))
                    evt.key.keysym.sym = keyBinding.right_;
                else if (relPosition.x_ > 0 && Abs(relPosition.y_ * 3 / 2) < Abs(relPosition.x_))
                    evt.key.keysym.sym = keyBinding.bottom_;
                else
                    return;

                if (eventType == E_TOUCHMOVE && evt.key.keysym.sym != element->GetVar(VAR_LAST_KEYSYM).GetInt())
                {
                    // Dragging past the directional boundary will cause an additional key up event for previous key symbol
                    SDL_Event keyEvent;
                    keyEvent.type = SDL_KEYUP;
                    keyEvent.key.keysym.sym = element->GetVar(VAR_LAST_KEYSYM).GetInt();
                    if (keyEvent.key.keysym.sym)
                    {
                        keyEvent.key.keysym.scancode = SDL_SCANCODE_UNKNOWN;
                        HandleSDLEvent(&keyEvent);
                    }

                    element->SetVar(VAR_LAST_KEYSYM, 0);
                }

                evt.key.keysym.scancode = SDL_SCANCODE_UNKNOWN;

                element->SetVar(VAR_LAST_KEYSYM, evt.key.keysym.sym);
            }
        }
    }
    else
        return;

    // Handle the fake SDL event to turn it into Urho3D genuine event
    HandleSDLEvent(&evt);
}

}
