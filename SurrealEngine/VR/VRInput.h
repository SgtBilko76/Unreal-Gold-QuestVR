#pragma once

#include "Math/vec.h"
#include "Math/rotator.h"
#include <surrealwidgets/window/window.h>

// Head/body/aim decoupling for the OpenXR VR port - see the plan this was built against
// (head tracking drives the camera only; a body-facing yaw independent of head yaw drives
// movement; the right motion controller drives weapon aim independently of both), modeled on
// Team Beef Studios' QuakeQuest's hmdorientation/playerYaw/gunangles split
// (QuakeQuest-master/Projects/Android/jni/QuakeQuestSrc/QuakeQuest_OpenXR.c).
//
// Owned by Engine (see Engine.h's `window` member for the analogous existing pattern) and
// updated once per frame from Engine::Run()'s VR branch, which is the only caller.
class VRInput
{
public:
	// Updates HmdView/RightController/LeftController from the current OpenXR frame state.
	// eyeViews[0]/[1] are this frame's WaitFrame()-returned per-eye views (left/right); only
	// the head-tracking-relevant parts (position/orientation) are used from them here -
	// FovAngle* is consumed separately by VRCamera for the render projection.
	void Update(Widget* window, float deltaSeconds);

	// The body's world-facing yaw (radians), independent of HmdOrientation - this is what
	// VRCamera anchors the play space to, and what movement axes are rotated by. Turning
	// your head does NOT change this; only explicit body-turn input does (snap-turn button,
	// or optionally smooth-turn on the right thumbstick - see Update()'s implementation).
	float BodyYawRadians = 0.0f;

	// Head pose for this frame, in OpenXR tracking space (same convention as
	// StereoEyeView - see SurrealWidgets/include/surrealwidgets/window/window.h). Averaged
	// from both eyes' positions (cheap approximation of head-space center) with the left
	// eye's orientation (both eyes share the same head orientation; only position differs by
	// IPD/2).
	StereoEyeView HeadView = {};

	// The head's STAGE-space XR position captured on the first Update() call (or after a
	// recenter), i.e. wherever the player happened to be physically standing in their room
	// when tracking started. OpenXR's STAGE origin is the room's Guardian-boundary center, not
	// the game's spawn point, so using raw HeadView/eye positions directly as a world offset
	// would place the camera off by however far the player is standing from that room-center
	// (confirmed on real hardware: camera ended up outside the map). VRCamera instead offsets
	// from THIS anchor - see GetHeadOffsetFromAnchor() - so at the moment of tracking start the
	// camera sits exactly at worldPlayerLocation, and only subsequent physical movement within
	// the room translates into in-game camera movement around that point.
	vec3 HeadAnchorPosition = vec3(0.0f);
	bool HasHeadAnchor = false;

	// currentPositionMeters (StereoEyeView::PositionX/Y/Z, i.e. an eye or the head position for
	// this frame) minus HeadAnchorPosition, still in OpenXR meters/axes - the physical-room
	// displacement to turn into a world-space offset (see VRCamera::GetEyeWorldLocation).
	vec3 GetOffsetFromAnchor(vec3 currentPositionMeters) const { return currentPositionMeters - HeadAnchorPosition; }

	// Right controller pose, OpenXR tracking space - drives weapon aim independently of
	// HeadView/BodyYawRadians. See VRCamera::GetEyeWorldRotation() for the same
	// OpenXR-to-UE1 axis remapping applied to this when computing a world-space aim Rotator.
	vec3 RightControllerPosition = vec3(0.0f);
	float RightControllerOrientation[4] = { 0.0f, 0.0f, 0.0f, 1.0f}; // xyzw quaternion
	bool RightControllerActive = false;

	vec3 LeftControllerPosition = vec3(0.0f);
	float LeftControllerOrientation[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	bool LeftControllerActive = false;

	// Left thumbstick, [-1,1] per axis, already deadzoned - x is strafe, y is forward.
	float MoveAxisX = 0.0f;
	float MoveAxisY = 0.0f;

	// Right thumbstick X, deadzoned - currently only drives the discrete snap-turn step in
	// Update() (BodyYawRadians jumps by a fixed increment past a deflection threshold, then
	// waits for the stick to recenter); exposed raw here too in case a caller wants it.
	float TurnAxisX = 0.0f;

	// Buttons this frame, already OR'd across both controllers where a single logical action
	// makes sense on either hand (mirrors how the desktop keybinding-driven activeInputButtons
	// map works - see Engine::UpdateVRInput(), Engine.cpp). Right trigger is the primary
	// fire/interact control since aiming is right-hand (VR/VRInput.h's GetAimRotation).
	bool FirePressed = false;   // right trigger
	bool AltFirePressed = false; // right grip

	// Edge-triggered (true for exactly one Update() call) versions of FirePressed and the left
	// controller's Menu button - used to synthesize mouse-click and Escape key-press events for
	// the desktop menu/UI system (Engine::UpdateVRMenuCursor(), Engine.cpp) without also
	// re-triggering on every frame the button stays held.
	bool TriggerJustPressed = false;
	bool TriggerJustReleased = false;
	bool GripJustPressed = false;
	bool GripJustReleased = false;
	bool MenuButtonJustPressed = false;
	// Right thumbstick pushed up/down past the trigger threshold this frame (one-shot).
	bool WeaponNextJustPressed = false;
	bool WeaponPrevJustPressed = false;
	bool JumpJustPressed = false;
	bool ScoresJustPressed = false; // left controller Y (one-shot) - scoreboard toggle
	bool TranslatorJustPressed = false; // right controller B (one-shot) - synthesized F2 key tap (Unreal: ActivateTranslator)
	bool CrouchPressed = false;      // left controller grip held - crouch (Unreal: C = Duck, a held Button/Axis command)
	bool CrouchJustPressed = false;
	bool CrouchJustReleased = false;
	bool ActivateItemJustPressed = false; // right controller A (one-shot) - synthesized Enter key tap (Unreal: InventoryActivate)
	bool NextItemJustPressed = false; // left controller X (one-shot) - synthesized RightBracket key tap (Unreal: InventoryNext)

	// Raw, undeadzoned analog values behind FirePressed/AltFirePressed - exposed for
	// diagnostics only (see Engine::RunVR()'s VRDiag log, Engine.cpp) while chasing a real-
	// hardware report of trigger presses not registering: this tells us whether the OpenXR action is
	// reporting inactive (stuck at 0.0 always) vs. reporting a real value that just never
	// crosses FirePressed's 0.5 threshold.
	float RightTriggerValueRaw = 0.0f;
	float RightGripValueRaw = 0.0f;
	bool JumpPressed = false;   // either A/X face button

	// Returns the world-space Rotator the right controller is currently aiming along, using
	// the same body-yaw anchoring as the render camera - the value to temporarily assign to
	// the firing Pawn's Rotation() around a weapon fire CallEvent (see Engine.cpp's
	// InputEvent/fire-key handling and VRInput.cpp's ApplyAimOverride/RestoreAimOverride).
	Rotator GetAimRotation() const;

	// The right controller's forward/aim direction in UE1 world space (unit vector) - the same
	// direction GetAimRotation() derives a Rotator from, exposed raw for
	// Engine::UpdateVRMenuCursor()'s controller-ray-to-screen-position math, which needs the
	// vector form rather than a Yaw/Pitch Rotator.
	vec3 GetAimForwardWorld() const;

	// Temporarily overrides pawnRotation (a reference to the firing Pawn's UActor::Rotation()
	// property) with GetAimRotation(), saving the previous value into savedRotation for
	// RestoreAimOverride() to put back immediately after the native Fire/AltFire CallEvent
	// returns - see VRCamera.h's file comment on why this, not a native aim hook, is the
	// integration point (SurrealEngine has no native weapon-fire-direction function to hook;
	// it's UnrealScript bytecode reading Owner.Rotation).
	void ApplyAimOverride(Rotator& pawnRotation, Rotator& savedRotation) const;
	void RestoreAimOverride(Rotator& pawnRotation, const Rotator& savedRotation) const;

private:
	bool snapTurnButtonWasDown = false;
	bool triggerWasDown = false;
	bool gripWasDown = false;
	bool weaponStickWasDown = false;
	bool jumpWasDown = false;
	bool scoresWasDown = false;
	bool translatorWasDown = false;
	bool crouchWasDown = false;
	bool activateItemWasDown = false;
	bool nextItemWasDown = false;

	// Recenter handling: the runtime announces the reference-space jump a frame or two before
	// the poses actually reflect it, and Update() reads the previous frame's poses anyway, so
	// the head anchor is re-captured a few frames after the event rather than immediately.
	int recenterFramesRemaining = 0;
	bool menuButtonWasDown = false;
	// Debounce guard for MenuButtonJustPressed, matching the snap-turn button's own cooldown
	// (SnapTurnCooldownSeconds) - defensive against the SAME class of real-hardware chatter
	// that caused BodyYawRadians to spin uncontrollably before that fix, applied here too since
	// a real-hardware report ("the big screen keeps following my head") is consistent with the
	// menu-screen's position getting re-anchored on spurious repeat presses rather than a
	// single clean press-and-release.
	float menuButtonCooldownRemaining = 0.0f;

	// Guards against snap-turn runaway from thumbstick noise/chatter right at the trigger
	// threshold - confirmed on real Quest 3 hardware: BodyYawRadians jumped by several
	// multiples of the 45 degree step within a couple of seconds with no deliberate input,
	// consistent with the analog X value oscillating across +-0.7 rather than one clean
	// press-and-release. Counts down by deltaSeconds each Update(); a new snap-turn can only
	// fire once it reaches zero (see VRInput.cpp's SnapTurnCooldownSeconds).
	float snapTurnCooldownRemaining = 0.0f;
};
