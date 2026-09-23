#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include "../core/rect.h"

#ifndef VULKAN_H_

#define VK_DEFINE_HANDLE(object) typedef struct object##_T* object;

#if defined(__LP64__) || defined(_WIN64) || defined(__x86_64__) || defined(_M_X64) || defined(__ia64) || defined (_M_IA64) || defined(__aarch64__) || defined(__powerpc64__)
#define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef struct object##_T *object;
#else
#define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef uint64_t object;
#endif

VK_DEFINE_HANDLE(VkInstance)
VK_DEFINE_HANDLE(VkPhysicalDevice)
VK_DEFINE_HANDLE(VkDevice)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkSurfaceKHR)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkImage)

#endif

class Widget;
class OpenFileDialog;
class SaveFileDialog;
class OpenFolderDialog;
class Image;
class CustomCursor;

enum class StandardCursor
{
	arrow,
	appstarting,
	cross,
	hand,
	ibeam,
	no,
	size_all,
	size_nesw,
	size_ns,
	size_nwse,
	size_we,
	uparrow,
	wait
};

enum class InputKey : uint32_t
{
	None, LeftMouse, RightMouse, Cancel,
	MiddleMouse, Unknown05, Unknown06, Unknown07,
	Backspace, Tab, Unknown0A, Unknown0B,
	Unknown0C, Enter, Unknown0E, Unknown0F,
	Shift, Ctrl, Alt, Pause,
	CapsLock, LCommand, RCommand, Unknown17,
	Unknown18, Unknown19, Unknown1A, Escape,
	Unknown1C, Unknown1D, Unknown1E, Unknown1F,
	Space, PageUp, PageDown, End,
	Home, Left, Up, Right,
	Down, Select, Print, Execute,
	PrintScrn, Insert, Delete, Help,
	_0, _1, _2, _3,
	_4, _5, _6, _7,
	_8, _9, Unknown3A, Unknown3B,
	Unknown3C, Unknown3D, Unknown3E, Unknown3F,
	Unknown40, A, B, C,
	D, E, F, G,
	H, I, J, K,
	L, M, N, O,
	P, Q, R, S,
	T, U, V, W,
	X, Y, Z, Unknown5B,
	Unknown5C, Unknown5D, Unknown5E, Unknown5F,
	NumPad0, NumPad1, NumPad2, NumPad3,
	NumPad4, NumPad5, NumPad6, NumPad7,
	NumPad8, NumPad9, GreyStar, GreyPlus,
	Separator, GreyMinus, NumPadPeriod, GreySlash,
	F1, F2, F3, F4,
	F5, F6, F7, F8,
	F9, F10, F11, F12,
	F13, F14, F15, F16,
	F17, F18, F19, F20,
	F21, F22, F23, F24,
	Unknown88, Unknown89, Unknown8A, Unknown8B,
	Unknown8C, Unknown8D, Unknown8E, Unknown8F,
	NumLock, ScrollLock, Unknown92, Unknown93,
	Unknown94, Unknown95, Unknown96, Unknown97,
	Unknown98, Unknown99, Unknown9A, Unknown9B,
	Unknown9C, Unknown9D, Unknown9E, Unknown9F,
	LShift, RShift, LControl, RControl,
	UnknownA4, UnknownA5, UnknownA6, UnknownA7,
	UnknownA8, UnknownA9, UnknownAA, UnknownAB,
	UnknownAC, UnknownAD, UnknownAE, UnknownAF,
	UnknownB0, UnknownB1, UnknownB2, UnknownB3,
	UnknownB4, UnknownB5, UnknownB6, UnknownB7,
	UnknownB8, UnknownB9, Semicolon, Equals,
	Comma, Minus, Period, Slash,
	Tilde, UnknownC1, UnknownC2, UnknownC3,
	UnknownC4, UnknownC5, UnknownC6, UnknownC7,
	Joy1, Joy2, Joy3, Joy4,
	Joy5, Joy6, Joy7, Joy8,
	Joy9, Joy10, Joy11, Joy12,
	Joy13, Joy14, Joy15, Joy16,
	UnknownD8, UnknownD9, UnknownDA, LeftBracket,
	Backslash, RightBracket, SingleQuote, UnknownDF,
	JoyX, JoyY, JoyZ, JoyR,
	MouseX, MouseY, MouseZ, MouseW,
	JoyU, JoyV, UnknownEA, UnknownEB,
	MouseWheelUp, MouseWheelDown, Unknown10E, Unknown10F,
	JoyPovUp, JoyPovDown, JoyPovLeft, JoyPovRight,
	UnknownF4, UnknownF5, Attn, CrSel,
	ExSel, ErEof, Play, Zoom,
	NoName, PA1, OEMClear
};

// Raw keyboard code. Same as the DirectInput keycodes
enum class RawKeycode : uint32_t
{
	None = 0x00,
	Escape = 0x01,
	_1 = 0x02,
	_2 = 0x03,
	_3 = 0x04,
	_4 = 0x05,
	_5 = 0x06,
	_6 = 0x07,
	_7 = 0x08,
	_8 = 0x09,
	_9 = 0x0A,
	_0 = 0x0B,
	Minus = 0x0C,
	Equals = 0x0D,
	Backspace = 0x0E,
	Tab = 0x0F,
	Q = 0x10,
	W = 0x11,
	E = 0x12,
	R = 0x13,
	T = 0x14,
	Y = 0x15,
	U = 0x16,
	I = 0x17,
	O = 0x18,
	P = 0x19,
	LBracket = 0x1A,
	RBracket = 0x1B,
	Return = 0x1C,
	LControl = 0x1D,
	A = 0x1E,
	S = 0x1F,
	D = 0x20,
	F = 0x21,
	G = 0x22,
	H = 0x23,
	J = 0x24,
	K = 0x25,
	L = 0x26,
	Semicolon = 0x27,
	Apostrophe = 0x28,
	Grave = 0x29,
	LShift = 0x2A,
	Backslash = 0x2B,
	Z = 0x2C,
	X = 0x2D,
	C = 0x2E,
	V = 0x2F,
	B = 0x30,
	N = 0x31,
	M = 0x32,
	Comma = 0x33,
	Period = 0x34,
	Slash = 0x35,
	RShift = 0x36,
	NumpadMultiply = 0x37,
	LAlt = 0x38,
	Space = 0x39,
	CapsLock = 0x3A,
	F1 = 0x3B,
	F2 = 0x3C,
	F3 = 0x3D,
	F4 = 0x3E,
	F5 = 0x3F,
	F6 = 0x40,
	F7 = 0x41,
	F8 = 0x42,
	F9 = 0x43,
	F10 = 0x44,
	Numlock = 0x45,
	Scroll = 0x46,
	Numpad7 = 0x47,
	Numpad8 = 0x48,
	Numpad9 = 0x49,
	NumpadSubstract = 0x4A,
	Numpad4 = 0x4B,
	Numpad5 = 0x4C,
	Numpad6 = 0x4D,
	NumpadAdd = 0x4E,
	Numpad1 = 0x4F,
	Numpad2 = 0x50,
	Numpad3 = 0x51,
	Numpad0 = 0x52,
	NumpadDecimal = 0x53,
	OEM_102 = 0x56, // <> or \| on RT 102-key keyboard (Non-U.S.)
	F11 = 0x57,
	F12 = 0x58,
	F13 = 0x64,
	F14 = 0x65,
	F15 = 0x66,
	Kana = 0x70,
	AbntC1 = 0x73,
	Convert = 0x79,
	NoConvert = 0x7B,
	Yen = 0x7D,
	AbntC2 = 0x7E,
	NumpadEquals = 0x8D,
	PrevTrack = 0x90,
	At = 0x91,
	Colon = 0x92,
	Underline = 0x93,
	Kanji = 0x94,
	Stop = 0x95,
	Ax = 0x96,
	Unlabeled = 0x97,
	NextTrack = 0x99,
	NumpadEnter = 0x9C,
	RControl = 0x9D,
	Mute = 0xA0,
	Calculator = 0xA1,
	PlayPause = 0xA2,
	MediaStop = 0xA4,
	VolumeDown = 0xAE,
	VolumeUp = 0xB0,
	WebHome = 0xB2,
	NumpadComma = 0xB3,
	NumpadDivide = 0xB5,
	SysRq = 0xB7,
	RAlt = 0xB8,
	Pause = 0xC5,
	Home = 0xC7,
	Up = 0xC8,
	PageUp = 0xC9,
	Left = 0xCB,
	Right = 0xCD,
	End = 0xCF,
	Down = 0xD0,
	PageDown = 0xD1,
	Insert = 0xD2,
	Delete = 0xD3,
	LCmd = 0xDB,
	RCmd = 0xDC,
	Apps = 0xDD,
	Power = 0xDE,
	Sleep = 0xDF,
	Wake = 0xE3,
	WebSearch = 0xE5,
	WebFavorites = 0xE6,
	WebRefresh = 0xE7,
	WebStop = 0xE8,
	WebForward = 0xE9,
	WebBack = 0xEA,
	MyComputer = 0xEB,
	Mail = 0xEC,
	MediaSelect = 0xED
};

enum class RenderAPI
{
	Unspecified,
	Bitmap,
	Vulkan,
	OpenGL,
	D3D11,
	D3D12,
	Metal
};

enum class WidgetType
{
	Child,
	Window,
	Popup,
	Dialog
};

// Plain-data head/eye pose and field of view, in the XR runtime's tracking space, using only
// primitive floats so this header depends on neither <openxr/openxr.h> (an implementation
// detail of one backend) nor SurrealEngine's vec3/quaternion math types (SurrealWidgets has
// no dependency on SurrealEngine - it is the other way around). Right-handed, +Y up, +X
// right, -Z forward (OpenXR's convention); the four FOV angles are half-angles in radians
// from forward, matching XrFovf. A caller building a render camera converts this into
// whatever mat4/quaternion types it uses.
struct StereoEyeView
{
	float PositionX, PositionY, PositionZ;
	float OrientationX, OrientationY, OrientationZ, OrientationW; // quaternion, xyzw
	float FovAngleLeft, FovAngleRight, FovAngleUp, FovAngleDown; // radians, tangent-space half-angles
};

// Same tracking-space convention as StereoEyeView, for a hand-held motion controller pose.
struct MotionControllerPose
{
	bool Active = false; // false if the runtime is not currently tracking this controller (e.g. out of view, powered off)
	float PositionX, PositionY, PositionZ;
	float OrientationX, OrientationY, OrientationZ, OrientationW; // quaternion, xyzw
};

enum class VRControllerHand
{
	Left,
	Right
};

// Digital/analog input this frame from one motion controller. Field names follow the OpenXR
// standard "simple"/Touch-style interaction profile naming rather than any one headset's
// button silkscreen labels, since that's what XR_KHR_composition_layer_depth-style APIs
// converge on and what QuakeQuest's own action set (see
// Projects/Android/jni/QuakeQuestSrc/OpenXrInput.c) also targets.
struct VRControllerState
{
	float TriggerValue = 0.0f;   // index trigger, 0..1
	float GripValue = 0.0f;      // grip/squeeze trigger, 0..1
	float ThumbstickX = 0.0f;    // -1..1
	float ThumbstickY = 0.0f;    // -1..1
	bool ButtonA = false;        // A/X (primary face button)
	bool ButtonB = false;        // B/Y (secondary face button)
	bool ThumbstickClick = false;
	bool MenuButton = false;
};

class DisplayWindow;

class DisplayWindowHost
{
public:
	virtual void OnWindowPaint() = 0;
	virtual void OnWindowMouseMove(const Point& pos) = 0;
	virtual void OnWindowMouseLeave() = 0;
	virtual void OnWindowMouseDown(const Point& pos, InputKey key) = 0;
	virtual void OnWindowMouseDoubleclick(const Point& pos, InputKey key) = 0;
	virtual void OnWindowMouseUp(const Point& pos, InputKey key) = 0;
	virtual void OnWindowMouseWheel(const Point& pos, InputKey key) = 0;
	virtual void OnWindowRawMouseMove(int dx, int dy) = 0;
	virtual void OnWindowRawKey(RawKeycode keycode, bool down) = 0;
	virtual void OnWindowKeyChar(std::string chars) = 0;
	virtual void OnWindowKeyDown(InputKey key) = 0;
	virtual void OnWindowKeyUp(InputKey key) = 0;
	virtual void OnWindowGeometryChanged() = 0;
	virtual void OnWindowClose() = 0;
	virtual void OnWindowActivated() = 0;
	virtual void OnWindowDeactivated() = 0;
	virtual void OnWindowDpiScaleChanged() = 0;
};

class DisplayWindow
{
public:
	static std::unique_ptr<DisplayWindow> Create(DisplayWindowHost* windowHost, WidgetType type, DisplayWindow* owner, RenderAPI renderAPI);

	static void ProcessEvents();
	static void RunLoop();
	static void ExitLoop();

	static void* StartTimer(int timeoutMilliseconds, std::function<void()> onTimer);
	static void StopTimer(void* timerID);

	static Size GetScreenSize();

	virtual ~DisplayWindow() = default;

	virtual void SetWindowTitle(const std::string& text) = 0;
	virtual void SetWindowIcon(const std::vector<std::shared_ptr<Image>>& images) = 0;
	virtual void Show() = 0;
	virtual void ShowFullscreen() = 0;
	virtual void ShowMaximized() = 0;
	virtual void ShowMinimized() = 0;
	virtual void ShowNormal() = 0;
	virtual bool IsWindowFullscreen() = 0;
	virtual void Hide() = 0;
	virtual void Activate() = 0;
	virtual void ShowCursor(bool enable) = 0;
	virtual void LockKeyboard() = 0;
	virtual void UnlockKeyboard() = 0;
	virtual void LockCursor() = 0;
	virtual void UnlockCursor() = 0;
	virtual void CaptureMouse() = 0;
	virtual void ReleaseMouseCapture() = 0;
	virtual void Update() = 0;
	virtual bool GetKeyState(InputKey key) = 0;

	// The geometry of a top level widget is always its client area due to Linux limitations
	virtual void SetClientFrame(const Rect& box) = 0;
	virtual Rect GetClientFrame() const = 0;

	virtual void SetCursor(StandardCursor cursor, std::shared_ptr<CustomCursor> custom) = 0;

	virtual Size GetClientSize() const = 0;
	virtual int GetPixelWidth() const = 0;
	virtual int GetPixelHeight() const = 0;
	virtual double GetDpiScale() const = 0;

	virtual Point MapFromGlobal(const Point& pos) const = 0;
	virtual Point MapToGlobal(const Point& pos) const = 0;

	virtual void SetBorderColor(uint32_t bgra8) = 0;
	virtual void SetCaptionColor(uint32_t bgra8) = 0;
	virtual void SetCaptionTextColor(uint32_t bgra8) = 0;

	virtual void PresentBitmap(int width, int height, const uint32_t* pixels) = 0;

	virtual std::string GetClipboardText() = 0;
	virtual void SetClipboardText(const std::string& text) = 0;

	virtual void* GetNativeHandle() = 0;

	virtual std::vector<std::string> GetVulkanInstanceExtensions() = 0;
	virtual VkSurfaceKHR CreateVulkanSurface(VkInstance instance) = 0;

	virtual void CreateGLContext() { throw std::runtime_error("CreateOpenGLContext not supported for this backend"); }
	virtual void MakeGLContextCurrent() { throw std::runtime_error("MakeGLContextCurrent not supported for this backend"); }
	virtual bool SetGLSwapInterval(int interval) { throw std::runtime_error("SetOpenGLSwapInterval not supported for this backend"); }
	virtual void SwapGLBuffers() { throw std::runtime_error("SwapOpenGLBuffers not supported for this backend"); }
	typedef void(*GLFuncPtr)();
	virtual GLFuncPtr GetGLProcAddress(const char* name) { throw std::runtime_error("GetGLProcAddress not supported for this backend"); }

	// Stereo VR support (OpenXR backend only): there is no VkSurfaceKHR/WSI swapchain to
	// present to - RenderDevice renders directly into swapchain images the XR runtime
	// itself owns, once per eye, and xrEndFrame composites them instead of vkQueuePresentKHR.
	// Default implementations throw so existing (non-VR) backends need no changes; see
	// SurrealWidgets/src/window/openxr/openxr_display_window.h for the real implementation
	// and RenderDevice/Vulkan/VulkanRenderDevice.cpp for how it's consumed.
	virtual bool IsStereoDisplay() { return false; }
	virtual int GetEyeCount() { throw std::runtime_error("GetEyeCount not supported for this backend"); }
	virtual void GetEyeImageSize(int eye, int* width, int* height) { throw std::runtime_error("GetEyeImageSize not supported for this backend"); }

	// Extensions the XR runtime's Vulkan graphics binding requires, to be passed into
	// VulkanInstanceBuilder::RequireExtensions()/VulkanDeviceBuilder::RequireExtension()
	// before the VkInstance/VkDevice are created.
	virtual std::vector<std::string> GetVulkanInstanceRequirements() { throw std::runtime_error("GetVulkanInstanceRequirements not supported for this backend"); }
	virtual std::vector<std::string> GetVulkanDeviceRequirements() { throw std::runtime_error("GetVulkanDeviceRequirements not supported for this backend"); }

	// The specific VkPhysicalDevice the XR runtime's compositor requires. Must be called
	// after the VkInstance exists (with the extensions above enabled) and before the
	// VkDevice is created. Match the returned handle against VulkanInstance::PhysicalDevices
	// to find the index to pass to VulkanDeviceBuilder::SelectDevice().
	virtual VkPhysicalDevice SelectVulkanPhysicalDevice(VkInstance instance) { throw std::runtime_error("SelectVulkanPhysicalDevice not supported for this backend"); }

	// Creates the XR session bound to this Vulkan device/queue and its per-eye swapchains.
	// Must be called once, after the VkDevice and graphics queue exist.
	virtual void CreateVulkanSession(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex) { throw std::runtime_error("CreateVulkanSession not supported for this backend"); }

	// Advances the XR frame loop. WaitFrame() blocks (like vsync) until the runtime wants
	// the next frame and returns false if nothing should be rendered this iteration (e.g.
	// session not visible); on true, GetEyeView() returns each eye's pose/FOV for that frame
	// (valid until the next WaitFrame() call) for building the render camera.
	// AcquireEyeImage()/ReleaseEyeImage() bracket rendering into one eye's current swapchain
	// image (returned via GetEyeImage()). EndFrame() submits both eye layers to the
	// compositor.
	virtual bool WaitFrame() { throw std::runtime_error("WaitFrame not supported for this backend"); }
	virtual StereoEyeView GetEyeView(int eye) { throw std::runtime_error("GetEyeView not supported for this backend"); }
	virtual int AcquireEyeImage(int eye) { throw std::runtime_error("AcquireEyeImage not supported for this backend"); }
	virtual VkImage GetEyeImage(int eye, int imageIndex) { throw std::runtime_error("GetEyeImage not supported for this backend"); }
	virtual void ReleaseEyeImage(int eye) { throw std::runtime_error("ReleaseEyeImage not supported for this backend"); }
	virtual void EndFrame() { throw std::runtime_error("EndFrame not supported for this backend"); }

	// A separate, MONO (non-stereo), world-anchored quad composition layer for 2D UI (the
	// pause/main menu), submitted alongside the stereo projection layer - matches Team Beef
	// Studios' QuakeQuest's "big screen" mode (bigScreen/VR_UseScreenLayer(),
	// TBXR_Common.c/QuakeQuest_OpenXR.c: XrCompositionLayerQuad positioned in front of the
	// player, offset from the head's STAGE-space position by playerYaw). Unlike the per-eye
	// projection layer, this content is rendered ONCE (not once per eye) into its own swapchain.
	// CreateScreenLayerSwapchain() must be called once after CreateVulkanSession(). Each frame
	// this is wanted: SetScreenLayerActive(true), SetScreenLayerPose(...), Acquire/Get/Release
	// around rendering exactly like the per-eye Acquire/Get/ReleaseEyeImage calls. When not
	// wanted, SetScreenLayerActive(false) - EndFrame() then omits the quad layer entirely.
	virtual void CreateScreenLayerSwapchain(int width, int height) { throw std::runtime_error("CreateScreenLayerSwapchain not supported for this backend"); }
	virtual void GetScreenLayerImageSize(int* width, int* height) { throw std::runtime_error("GetScreenLayerImageSize not supported for this backend"); }
	virtual int AcquireScreenLayerImage() { throw std::runtime_error("AcquireScreenLayerImage not supported for this backend"); }
	virtual VkImage GetScreenLayerImage(int imageIndex) { throw std::runtime_error("GetScreenLayerImage not supported for this backend"); }
	virtual void ReleaseScreenLayerImage() { throw std::runtime_error("ReleaseScreenLayerImage not supported for this backend"); }
	// positionX/Y/Z and orientationX/Y/Z/W are in OpenXR STAGE space (same convention as
	// StereoEyeView/MotionControllerPose) - the caller (Engine::RunVRMenuScreen(), Engine.cpp)
	// computes these directly in that space (mirroring QuakeQuest's own approach) rather than
	// via SurrealEngine's UE1 world-space math, since the quad's pose has no gameplay meaning
	// beyond "a fixed distance in front of wherever the player is standing/facing".
	// widthMeters/heightMeters size the quad in the real world (XrCompositionLayerQuad::size).
	virtual void SetScreenLayerPose(float positionX, float positionY, float positionZ, float orientationX, float orientationY, float orientationZ, float orientationW, float widthMeters, float heightMeters) { throw std::runtime_error("SetScreenLayerPose not supported for this backend"); }
	virtual void SetScreenLayerActive(bool active) { throw std::runtime_error("SetScreenLayerActive not supported for this backend"); }

	// True once after the runtime reported a reference-space change (the headset's recenter
	// gesture) - the caller re-anchors its tracking-space offsets. Non-VR backends: never.
	virtual bool ConsumeRecenterEvent() { return false; }

	// VR motion controller support (OpenXR backend only). Valid after WaitFrame() returns
	// true for the current frame, until the next WaitFrame() call - same lifetime as
	// GetEyeView().
	// The head's own central pose (OpenXR VIEW reference space - see HeadSpace in
	// openxr_display_window.h), distinct from either eye's individual pose (StereoEyeView, from
	// GetEyeView()). Team Beef Studios' QuakeQuest (TBXR_Common.c: xfStageFromHead) uses this
	// single, shared head orientation for BOTH eyes' rendering, varying only position (via IPD)
	// between them - confirmed via real Quest 3 hardware testing to be the right approach after
	// using each eye's own individually-reported OpenXR orientation (which usually differs only
	// slightly, but visibly enough) made the two eyes' views feel unacceptably "different" from
	// each other in a way that reducing eye separation didn't fix.
	virtual MotionControllerPose GetHeadPose() { throw std::runtime_error("GetHeadPose not supported for this backend"); }
	virtual MotionControllerPose GetControllerPose(VRControllerHand hand) { throw std::runtime_error("GetControllerPose not supported for this backend"); }
	virtual VRControllerState GetControllerState(VRControllerHand hand) { throw std::runtime_error("GetControllerState not supported for this backend"); }
	virtual void TriggerHapticPulse(VRControllerHand hand, float amplitude, float durationSeconds) { throw std::runtime_error("TriggerHapticPulse not supported for this backend"); }
};

class DisplayBackend
{
public:
	static DisplayBackend* Get();
	static void Set(std::unique_ptr<DisplayBackend> instance);

	static std::unique_ptr<DisplayBackend> TryCreateWin32();
	static std::unique_ptr<DisplayBackend> TryCreateSDL2();
	static std::unique_ptr<DisplayBackend> TryCreateSDL3();
	static std::unique_ptr<DisplayBackend> TryCreateX11();
	static std::unique_ptr<DisplayBackend> TryCreateWayland();
	static std::unique_ptr<DisplayBackend> TryCreateCocoa();
	static std::unique_ptr<DisplayBackend> TryCreateOpenXR();

	static std::unique_ptr<DisplayBackend> TryCreateBackend();

	virtual ~DisplayBackend() = default;

	virtual bool IsWin32() { return false; }
	virtual bool IsSDL2() { return false; }
	virtual bool IsSDL3() { return false; }
	virtual bool IsX11() { return false; }
	virtual bool IsWayland() { return false; }
	virtual bool IsCocoa() { return false; }
	virtual bool IsOpenXR() { return false; }

	virtual std::unique_ptr<DisplayWindow> Create(DisplayWindowHost* windowHost, WidgetType type, DisplayWindow* owner, RenderAPI renderAPI) = 0;
	virtual void ProcessEvents() = 0;
	virtual void RunLoop() = 0;
	virtual void ExitLoop() = 0;

	virtual void* StartTimer(int timeoutMilliseconds, std::function<void()> onTimer) = 0;
	virtual void StopTimer(void* timerID) = 0;

	virtual Size GetScreenSize() = 0;

	virtual std::unique_ptr<OpenFileDialog> CreateOpenFileDialog(DisplayWindow* owner);
	virtual std::unique_ptr<SaveFileDialog> CreateSaveFileDialog(DisplayWindow* owner);
	virtual std::unique_ptr<OpenFolderDialog> CreateOpenFolderDialog(DisplayWindow* owner);
};

// Must be called once, before DisplayBackend::TryCreateBackend(), on Android - the OpenXR
// backend needs the JavaVM/Activity to initialize the loader and create the XR instance (see
// SurrealWidgets/src/window/openxr/openxr_display_window.cpp's CreateInstanceAndSystem), and
// there is no windowing system on this platform to source them from otherwise. javaVM,
// activity, and nativeWindow are JavaVM*/jobject/ANativeWindow* respectively, passed as void*
// so this header (shared with every desktop backend) doesn't need <jni.h>. No-op on backends
// other than OpenXR (SetAndroidApp on a not-yet-constructed OpenXR backend just records the
// values for its constructor to pick up - see OpenXRDisplayBackend::SetAndroidApp).
void SetOpenXRAndroidApp(void* javaVM, void* activity, void* nativeWindow);
