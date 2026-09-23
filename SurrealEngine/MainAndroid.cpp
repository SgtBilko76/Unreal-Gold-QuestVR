
#include "Precomp.h"
#include "GameApp.h"
#include "GameFolder.h"
#include "Engine.h"
#include "LauncherSettings.h"
#include "Utils/CommandLine.h"
#include "Utils/Exception.h"
#include "Utils/Logger.h"
#include "Utils/File.h"
#include "UI/WidgetResourceData.h"
#include <surrealwidgets/core/theme.h>
#include <surrealwidgets/window/window.h>
#include <jni.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <thread>
#include <memory>
#include <fstream>
#include <sstream>

#define LOG_TAG "SurrealEngine"
#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Native counterpart to Projects/Android/java/com/dpjudas/surrealengine/SurrealEngineActivity.java
// and SurrealEngineJNILib.java - mirrors QuakeQuest's JNI entry point shape
// (Projects/Android/jni/QuakeQuestSrc/QuakeQuest_OpenXR.c's
// Java_com_drbeef_quakequest_GLES3JNILib_* functions), adapted for Vulkan/OpenXR instead of
// GLES/EGL and for SurrealEngine's Engine/GameWindow architecture instead of DarkPlaces'.
//
// Unlike the desktop entry point (MainGame.cpp -> GameApp::main -> LauncherWindow::ExecModal
// -> Engine::Run()), there is no launcher dialog here: game files are sideloaded to a fixed
// directory by the user (see SurrealEngineActivity.java's `dir` field), auto-detected via
// GameFolderSelection the same way the desktop build does when pointed at a folder from the
// command line, and Engine::Run() is driven from a dedicated native thread for the lifetime
// of the Activity (matching QuakeQuest's AppThreadFunction) since it blocks in its own
// while(!quit) loop.
//
// SurrealEngine's built-in menu/HUD/launcher fonts are loaded from SurrealEngine.pk3
// (SurrealEngine/UI/WidgetResourceData.cpp's ResourceLoaderPK3) - confirmed on real Quest 3
// hardware that OS::executable_path()'s normal lookup fails here (it resolves via
// /proc/self/exe, meaningless for a shared library loaded into an app process), so
// ResourceLoaderPK3 also checks $SURREALENGINE_DIR/SurrealEngine.pk3 on Android. The user
// must sideload SurrealEngine.pk3 (from the repo root) into the same directory as their game
// files (see SurrealEngineActivity.java's `dir` field) for this to be found.

namespace
{
	struct NativeApp
	{
		JavaVM* vm = nullptr;
		jobject activityObject = nullptr;
		ANativeWindow* nativeWindow = nullptr;
		std::string gameDataDir;

		std::unique_ptr<Engine> engine;
		std::thread engineThread;
	};
}

static void EngineThreadMain(NativeApp* app)
{
	JNIEnv* env = nullptr;
	app->vm->AttachCurrentThread(&env, nullptr);

	try
	{
		SetOpenXRAndroidApp(app->vm, app->activityObject, app->nativeWindow);

		auto backend = DisplayBackend::TryCreateBackend();
		if (!backend || !backend->IsOpenXR())
		{
			ALOGE("Failed to create the OpenXR display backend");
			return;
		}
		DisplayBackend::Set(std::move(backend));

		InitWidgetResources(); // see the KNOWN GAP note above - may throw on this platform today
		WidgetTheme::SetTheme(std::make_unique<DarkWidgetTheme>());

		// Optional extra desktop-style command-line args (e.g. --url=DM-Deck16][.unr to pick a
		// start map other than the game's default entry/intro map, --engineversion=..., etc.)
		// sideloaded as a single line of text next to the game files - same idea as
		// QuakeQuest's commandline.txt (Projects/Android/java/.../GLES3JNIActivity.java), used
		// here since there is no launcher UI on this platform to pick a map interactively.
		Array<std::string> args = { app->gameDataDir };
		{
			std::ifstream cmdlineFile((fs::path(app->gameDataDir) / "commandline.txt").string());
			std::string line;
			if (cmdlineFile && std::getline(cmdlineFile, line) && !line.empty())
			{
				ALOGV("Using commandline.txt: %s", line.c_str());
				std::istringstream stream(line);
				std::string token;
				while (stream >> token)
					args.push_back(token);
			}
		}

		CommandLine cmd(args);
		commandline = &cmd;

		GameFolderSelection::UpdateList();
		if (GameFolderSelection::Games.empty())
		{
			ALOGE("No supported game found in %s - sideload Unreal/UnrealTournament/DeusEx game files there first", app->gameDataDir.c_str());
			return;
		}

		GameLaunchInfo info = GameFolderSelection::GetLaunchInfo(0);

		// Vulkan is the only RenderAPI the OpenXR backend supports (enforced in
		// OpenXRDisplayWindow's constructor) - make sure LauncherSettings agrees, in case a
		// stale Settings.json (see LauncherSettings.cpp's GetSettingsFilename, under $HOME
		// which SurrealEngineActivity.java points at app-private storage) says otherwise.
		LauncherSettings::Get().RenderDevice.Type = RenderDeviceType::Vulkan;

		// Engine log -> logcat (tag SurrealEngine-Log). Without this the engine's LogMessage()
		// output (game detection, script exceptions, unimplemented natives, map changes) only
		// ever lived in memory on Android; the desktop build has --logfile for the same purpose.
		Logger::Get()->SetCallback([](const LogMessageLine& line)
		{
			std::string text = line.Source.empty() ? line.Text : "[" + line.Source + "] " + line.Text;
			__android_log_print(ANDROID_LOG_INFO, "SurrealEngine-Log", "%s", text.c_str());
		});

		app->engine = std::make_unique<Engine>(info);
		app->engine->Run();
	}
	catch (const std::exception& e)
	{
		ALOGE("Engine::Run() threw: %s", e.what());
	}

	DeinitWidgetResources();
	app->engine.reset();

	app->vm->DetachCurrentThread();
}

extern "C"
{

JNIEXPORT jlong JNICALL Java_com_dpjudas_surrealengine_SurrealEngineJNILib_onCreate(JNIEnv* env, jclass, jobject activity, jstring gameDataDir)
{
	auto app = new NativeApp();

	JavaVM* vm = nullptr;
	env->GetJavaVM(&vm);
	app->vm = vm;
	app->activityObject = env->NewGlobalRef(activity);

	const char* dirChars = env->GetStringUTFChars(gameDataDir, nullptr);
	app->gameDataDir = dirChars;
	env->ReleaseStringUTFChars(gameDataDir, dirChars);

	return (jlong)(intptr_t)app;
}

JNIEXPORT void JNICALL Java_com_dpjudas_surrealengine_SurrealEngineJNILib_onStart(JNIEnv*, jobject, jlong handle, jobject)
{
	// Nothing to do until the surface exists - see onSurfaceCreated below, which is what
	// actually starts the engine thread, mirroring QuakeQuest's message-queue-driven
	// lifecycle but simplified since SurrealEngine doesn't need a native window handle at
	// all (it renders via OpenXR swapchains, not an ANativeWindow-backed EGL/Vulkan surface -
	// see openxr_display_window.cpp's CreateVulkanSession, which never touches a
	// VkSurfaceKHR). onSurfaceCreated is kept only because SurrealEngineJNILib's shape
	// mirrors the desktop GameWindow lifecycle 1:1; the ANativeWindow it provides is unused.
}

JNIEXPORT void JNICALL Java_com_dpjudas_surrealengine_SurrealEngineJNILib_onResume(JNIEnv*, jobject, jlong handle)
{
}

JNIEXPORT void JNICALL Java_com_dpjudas_surrealengine_SurrealEngineJNILib_onPause(JNIEnv*, jobject, jlong handle)
{
}

JNIEXPORT void JNICALL Java_com_dpjudas_surrealengine_SurrealEngineJNILib_onStop(JNIEnv*, jobject, jlong handle)
{
}

JNIEXPORT void JNICALL Java_com_dpjudas_surrealengine_SurrealEngineJNILib_onDestroy(JNIEnv*, jobject, jlong handle)
{
	auto app = reinterpret_cast<NativeApp*>((intptr_t)handle);
	if (!app)
		return;

	// The engine thread normally exits on its own: OpenXRDisplayWindow::PollEvents() calls
	// WindowHost->OnWindowClose() (-> Engine::OnWindowClose(), Engine.cpp) when the OpenXR
	// session reaches XR_SESSION_STATE_EXITING/LOSS_PENDING, which sets Engine::quit and lets
	// Run()'s while(!quit) loop return normally. This join just waits for that to finish
	// (or for a crash/early exception in EngineThreadMain) before releasing the JNI handle.
	if (app->engineThread.joinable())
		app->engineThread.join();

	if (app->activityObject)
	{
		JNIEnv* env = nullptr;
		app->vm->GetEnv((void**)&env, JNI_VERSION_1_6);
		if (env)
			env->DeleteGlobalRef(app->activityObject);
	}

	delete app;
}

JNIEXPORT void JNICALL Java_com_dpjudas_surrealengine_SurrealEngineJNILib_onSurfaceCreated(JNIEnv* env, jobject, jlong handle, jobject surface)
{
	auto app = reinterpret_cast<NativeApp*>((intptr_t)handle);
	if (!app || app->engineThread.joinable())
		return; // already started - onSurfaceChanged can re-fire this on some devices

	app->nativeWindow = ANativeWindow_fromSurface(env, surface);
	app->engineThread = std::thread(EngineThreadMain, app);
}

JNIEXPORT void JNICALL Java_com_dpjudas_surrealengine_SurrealEngineJNILib_onSurfaceChanged(JNIEnv* env, jobject, jlong handle, jobject surface)
{
}

JNIEXPORT void JNICALL Java_com_dpjudas_surrealengine_SurrealEngineJNILib_onSurfaceDestroyed(JNIEnv*, jobject, jlong handle)
{
}

} // extern "C"
