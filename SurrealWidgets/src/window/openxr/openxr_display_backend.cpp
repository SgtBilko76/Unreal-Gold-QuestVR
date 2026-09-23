#include "openxr_display_backend.h"
#include "openxr_display_window.h"
#include <stdexcept>

// Set once by the Android JNI entry point via OpenXRDisplayBackend::SetAndroidApp() before
// DisplayBackend::TryCreateBackend() is called - the OpenXR loader on Android needs these to
// initialize (xrInitializeLoaderKHR) and to create the instance
// (XrInstanceCreateInfoAndroidKHR), and nothing else in this backend has another way to get
// them (there is no windowing system to source them from).
JavaVM* OpenXR_JavaVM = nullptr;
jobject OpenXR_ActivityObject = nullptr;
static ANativeWindow* OpenXR_NativeWindow = nullptr;

bool OpenXRDisplayBackend::ExitRunLoop = false;
OpenXRDisplayWindow* OpenXRDisplayBackend::ActiveWindow = nullptr;

void OpenXRDisplayBackend::SetAndroidApp(JavaVM* vm, jobject activity, ANativeWindow* window)
{
	OpenXR_JavaVM = vm;
	OpenXR_ActivityObject = activity;
	OpenXR_NativeWindow = window;
}

void SetOpenXRAndroidApp(void* javaVM, void* activity, void* nativeWindow)
{
	OpenXRDisplayBackend::SetAndroidApp((JavaVM*)javaVM, (jobject)activity, (ANativeWindow*)nativeWindow);
}

OpenXRDisplayBackend::OpenXRDisplayBackend()
{
	if (!OpenXR_JavaVM || !OpenXR_ActivityObject)
		throw std::runtime_error("OpenXRDisplayBackend::SetAndroidApp() must be called before creating the OpenXR display backend");
}

OpenXRDisplayBackend::~OpenXRDisplayBackend()
{
}

std::unique_ptr<DisplayWindow> OpenXRDisplayBackend::Create(DisplayWindowHost* windowHost, WidgetType type, DisplayWindow* owner, RenderAPI renderAPI)
{
	// There is exactly one XR session for the app's lifetime (see class comment in the
	// header) - SurrealEngine only ever creates one top-level GameWindow, so this is not a
	// practical limitation, just worth asserting rather than silently doing the wrong thing.
	if (ActiveWindow)
		throw std::runtime_error("The OpenXR backend supports only one DisplayWindow at a time");

	auto window = std::make_unique<OpenXRDisplayWindow>(windowHost, type, renderAPI);
	ActiveWindow = window.get();
	return window;
}

void OpenXRDisplayBackend::ProcessEvents()
{
	// Unlike the desktop backends there is no separate OS message queue distinct from the XR
	// session's own event queue - Engine::TickWindow() (SurrealEngine/Engine.cpp) calls
	// GameWindow::ProcessEvents() once per tick, which reaches this via
	// DisplayBackend::Get()->ProcessEvents(), so this is where xrPollEvent actually gets
	// pumped from the engine's normal per-frame tick.
	if (ActiveWindow)
		ActiveWindow->PollEvents();
}

void OpenXRDisplayBackend::RunLoop()
{
	// SurrealEngine's Engine::Run() drives its own while(!quit) loop and never calls
	// DisplayBackend::RunLoop() - only GameWindow::RunLoop()/ExitLoop() exist for backends
	// that need Show()-and-block semantics (e.g. a modal desktop dialog). Not used on
	// Android; the JNI Activity lifecycle owns the outer loop instead (see MainAndroid.cpp).
}

void OpenXRDisplayBackend::ExitLoop()
{
	ExitRunLoop = true;
}

void* OpenXRDisplayBackend::StartTimer(int timeoutMilliseconds, std::function<void()> onTimer)
{
	// SurrealEngine doesn't rely on DisplayBackend timers for anything on the VR frame-loop
	// path (LauncherWindow UI timers are desktop-only and never constructed on Android).
	throw std::runtime_error("Timers are not supported by the OpenXR backend");
}

void OpenXRDisplayBackend::StopTimer(void* timerID)
{
}

Size OpenXRDisplayBackend::GetScreenSize()
{
	// No 2D desktop to report a size for; callers needing a size (e.g. the launcher UI) are
	// desktop-only code paths not exercised on Android.
	return Size(0.0, 0.0);
}

std::unique_ptr<DisplayBackend> DisplayBackend::TryCreateOpenXR()
{
	try
	{
		return std::make_unique<OpenXRDisplayBackend>();
	}
	catch (...)
	{
		return nullptr;
	}
}
