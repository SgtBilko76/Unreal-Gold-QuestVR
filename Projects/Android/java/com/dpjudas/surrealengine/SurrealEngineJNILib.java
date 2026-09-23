package com.dpjudas.surrealengine;

import android.app.Activity;
import android.view.Surface;

// Wrapper for the native SurrealEngine library. Mirrors the JNI shape used by
// Team Beef Studios' QuakeQuest (GLES3JNILib), adapted for the Vulkan/OpenXR backend
// added in SurrealWidgets' openxr DisplayBackend.
public class SurrealEngineJNILib
{
	// Activity lifecycle
	public static native long onCreate(Activity obj, String gameDataDir);
	public static native void onStart(long handle, Object obj);
	public static native void onResume(long handle);
	public static native void onPause(long handle);
	public static native void onStop(long handle);
	public static native void onDestroy(long handle);

	// Surface lifecycle
	public static native void onSurfaceCreated(long handle, Surface s);
	public static native void onSurfaceChanged(long handle, Surface s);
	public static native void onSurfaceDestroyed(long handle);
}
