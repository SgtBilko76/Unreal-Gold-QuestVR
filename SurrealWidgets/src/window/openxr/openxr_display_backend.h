#pragma once

#include "window/window.h"
#include "openxr_vulkan_loader.h"
#include <memory>

// DisplayBackend for standalone Android/OpenXR VR headsets (Meta Quest 2/3).
//
// Unlike the desktop backends (Win32/SDL2/X11/...), there is exactly one DisplayWindow for
// the lifetime of the app - the OpenXR session itself - and it has no real OS window: no
// native handle, no mouse/keyboard events, no resizing. ProcessEvents() drives the OpenXR
// event/frame loop (xrPollEvent, then xrWaitFrame/xrBeginFrame once a session is running)
// instead of pumping OS messages. See openxr_display_window.h for how rendering attaches to
// this: there is no VkSurfaceKHR/swapchain-of-the-OS-window - RenderDevice code renders
// directly into swapchain images OpenXR itself owns, once per eye.
class OpenXRDisplayWindow;

class OpenXRDisplayBackend : public DisplayBackend
{
public:
	OpenXRDisplayBackend();
	~OpenXRDisplayBackend();

	std::unique_ptr<DisplayWindow> Create(DisplayWindowHost* windowHost, WidgetType type, DisplayWindow* owner, RenderAPI renderAPI) override;
	void ProcessEvents() override;
	void RunLoop() override;
	void ExitLoop() override;

	void* StartTimer(int timeoutMilliseconds, std::function<void()> onTimer) override;
	void StopTimer(void* timerID) override;

	Size GetScreenSize() override;

	bool IsOpenXR() override { return true; }

	// Set by the Android JNI entry point (SurrealEngine/MainAndroid.cpp) before the engine's
	// main loop starts, since OpenXR on Android needs the JavaVM/Activity/ANativeWindow that
	// only Java-side lifecycle callbacks can provide - there is no windowing system to ask.
	static void SetAndroidApp(JavaVM* vm, jobject activity, ANativeWindow* window);

	// Called by OpenXRDisplayWindow's destructor.
	static void ClearActiveWindow(OpenXRDisplayWindow* window) { if (ActiveWindow == window) ActiveWindow = nullptr; }

private:
	static bool ExitRunLoop;
	static OpenXRDisplayWindow* ActiveWindow;
};
