
#include "Precomp.h"
#include "VRInput.h"
#include "VRCamera.h"
#include <surrealwidgets/core/widget.h>
#include <cmath>

// Snap-turn increment. Comfort setting - see VR/VRCamera.h's file comment on the general
// unverified-against-real-hardware caveat this whole module inherits.
static constexpr float SnapTurnRadians = 45.0f * (3.14159265359f / 180.0f);
static constexpr float ThumbstickDeadzone = 0.2f;

// Minimum time between snap-turn triggers, regardless of stick state. Confirmed on real
// Quest 3 hardware: with only a raw +-0.7 edge trigger and no cooldown, BodyYawRadians
// jumped by several 45 degree steps within ~1-2 seconds without deliberate input - the
// thumbstick's analog X value chatters across the threshold rather than crossing it once
// cleanly. This cooldown bounds the turn rate to something a human press-and-release
// sequence could actually produce.
static constexpr float SnapTurnCooldownSeconds = 0.35f;
// Trigger threshold vs. the lower re-arm threshold below (hysteresis band) - stops a value
// noisily hovering right at 0.7 from re-triggering the instant the cooldown above expires.
static constexpr float SnapTurnTriggerThreshold = 0.7f;
static constexpr float SnapTurnReleaseThreshold = 0.3f;

static float ApplyDeadzone(float value)
{
	if (std::fabs(value) < ThumbstickDeadzone)
		return 0.0f;
	float sign = value < 0.0f ? -1.0f : 1.0f;
	return sign * (std::fabs(value) - ThumbstickDeadzone) / (1.0f - ThumbstickDeadzone);
}

void VRInput::Update(Widget* window, float deltaSeconds)
{
	StereoEyeView leftEye = window->GetEyeView(0);
	StereoEyeView rightEye = window->GetEyeView(1);

	HeadView = leftEye;
	HeadView.PositionX = (leftEye.PositionX + rightEye.PositionX) * 0.5f;
	HeadView.PositionY = (leftEye.PositionY + rightEye.PositionY) * 0.5f;
	HeadView.PositionZ = (leftEye.PositionZ + rightEye.PositionZ) * 0.5f;
	// Orientation is shared between both eyes (only position differs by half the IPD), so the
	// left eye's is used as-is.

	// Update() is called before WaitFrame()/xrLocateViews has run for this app-frame (see
	// Engine::Run(), Engine.cpp - vrInput state needs to exist before the tick/physics step,
	// but WaitFrame() only happens later during rendering), so it's reading last frame's eye
	// poses. On the very first call there IS no previous frame yet: OpenXRDisplayWindow's
	// Views[] array starts zero-initialized, which includes a zero orientation quaternion
	// (0,0,0,0) - not a valid rotation (magnitude 0, not 1). Guard the anchor capture on a
	// non-zero quaternion instead of "first call", so it latches onto the first frame that
	// actually has a real head pose rather than permanently anchoring at the origin (confirmed
	// on real hardware: HeadAnchorPosition stayed (0,0,0) forever, corrupting every frame's
	// camera position - see VRCamera::GetEyeWorldLocation).
	bool haveValidPose = (HeadView.OrientationX != 0.0f || HeadView.OrientationY != 0.0f ||
		HeadView.OrientationZ != 0.0f || HeadView.OrientationW != 0.0f);

	// Headset recenter (see OpenXRDisplayWindow::PollEvents): the LOCAL space origin moves to
	// the current head pose, so the old anchor is meaningless - re-capture it once the new
	// poses have arrived. The head's yaw reads as zero in the recentered space, which makes
	// the direction the player is now facing the body/movement direction (BodyYawRadians is
	// untouched), exactly what a recenter is expected to do.
	if (window->ConsumeRecenterEvent())
		recenterFramesRemaining = 3;
	if (recenterFramesRemaining > 0 && --recenterFramesRemaining == 0)
		HasHeadAnchor = false;

	if (!HasHeadAnchor && haveValidPose)
	{
		HeadAnchorPosition = vec3(HeadView.PositionX, HeadView.PositionY, HeadView.PositionZ);
		HasHeadAnchor = true;
	}

	MotionControllerPose right = window->GetControllerPose(VRControllerHand::Right);
	RightControllerActive = right.Active;
	RightControllerPosition = vec3(right.PositionX, right.PositionY, right.PositionZ);
	RightControllerOrientation[0] = right.OrientationX;
	RightControllerOrientation[1] = right.OrientationY;
	RightControllerOrientation[2] = right.OrientationZ;
	RightControllerOrientation[3] = right.OrientationW;

	MotionControllerPose left = window->GetControllerPose(VRControllerHand::Left);
	LeftControllerActive = left.Active;
	LeftControllerPosition = vec3(left.PositionX, left.PositionY, left.PositionZ);
	LeftControllerOrientation[0] = left.OrientationX;
	LeftControllerOrientation[1] = left.OrientationY;
	LeftControllerOrientation[2] = left.OrientationZ;
	LeftControllerOrientation[3] = left.OrientationW;

	VRControllerState leftState = window->GetControllerState(VRControllerHand::Left);
	VRControllerState rightState = window->GetControllerState(VRControllerHand::Right);

	MoveAxisX = ApplyDeadzone(leftState.ThumbstickX);
	MoveAxisY = ApplyDeadzone(leftState.ThumbstickY);
	// Digital, not analog: any deflection past the deadzone means full speed in that
	// direction. UT is a run-by-default game and a linear analog ramp made every movement
	// ramping up from a slow start - full speed immediately feels better in VR.
	{
		float len = std::sqrt(MoveAxisX * MoveAxisX + MoveAxisY * MoveAxisY);
		if (len > 0.0f)
		{
			MoveAxisX /= len;
			MoveAxisY /= len;
		}
	}
	TurnAxisX = ApplyDeadzone(rightState.ThumbstickX);
	float weaponAxisY = ApplyDeadzone(rightState.ThumbstickY);

	// Right stick up/down = next/previous weapon, one step per push (same hysteresis idea as
	// the snap-turn below: fire past 0.7, re-arm once back inside 0.3).
	bool weaponStickDown = weaponStickWasDown
		? (weaponAxisY > SnapTurnReleaseThreshold || weaponAxisY < -SnapTurnReleaseThreshold)
		: (weaponAxisY > SnapTurnTriggerThreshold || weaponAxisY < -SnapTurnTriggerThreshold);
	WeaponNextJustPressed = weaponStickDown && !weaponStickWasDown && weaponAxisY > 0.0f;
	WeaponPrevJustPressed = weaponStickDown && !weaponStickWasDown && weaponAxisY < 0.0f;
	weaponStickWasDown = weaponStickDown;

	RightTriggerValueRaw = rightState.TriggerValue;
	RightGripValueRaw = rightState.GripValue;
	FirePressed = rightState.TriggerValue > 0.5f;
	AltFirePressed = rightState.GripValue > 0.5f;
	JumpPressed = leftState.TriggerValue > 0.5f; // left trigger = jump; left X is "next item", right A "activate item" (see below)
	JumpJustPressed = JumpPressed && !jumpWasDown;
	jumpWasDown = JumpPressed;

	ScoresJustPressed = leftState.ButtonB && !scoresWasDown; // left Y = scoreboard
	scoresWasDown = leftState.ButtonB;

	TranslatorJustPressed = rightState.ButtonB && !translatorWasDown; // right B = F2 / translator
	translatorWasDown = rightState.ButtonB;

	CrouchPressed = leftState.GripValue > 0.5f; // left grip = crouch
	CrouchJustPressed = CrouchPressed && !crouchWasDown;
	CrouchJustReleased = !CrouchPressed && crouchWasDown;
	crouchWasDown = CrouchPressed;

	ActivateItemJustPressed = rightState.ButtonA && !activateItemWasDown; // right A = Enter / activate item
	activateItemWasDown = rightState.ButtonA;

	NextItemJustPressed = leftState.ButtonA && !nextItemWasDown; // left X = RightBracket / next inventory item
	nextItemWasDown = leftState.ButtonA;

	TriggerJustPressed = FirePressed && !triggerWasDown;
	TriggerJustReleased = !FirePressed && triggerWasDown;
	triggerWasDown = FirePressed;

	GripJustPressed = AltFirePressed && !gripWasDown;
	GripJustReleased = !AltFirePressed && gripWasDown;
	gripWasDown = AltFirePressed;

	if (menuButtonCooldownRemaining > 0.0f)
		menuButtonCooldownRemaining -= deltaSeconds;
	MenuButtonJustPressed = leftState.MenuButton && !menuButtonWasDown && menuButtonCooldownRemaining <= 0.0f;
	if (MenuButtonJustPressed)
		menuButtonCooldownRemaining = 0.5f; // generous - this is a deliberate toggle press, not something needing fast repeats
	menuButtonWasDown = leftState.MenuButton;

	// Snap-turn: fire once per press-and-release-past-threshold, not continuously while held.
	// Uses a hysteresis band (trigger at 0.7, only re-arm once back below 0.3) plus a cooldown
	// timer (see SnapTurnCooldownSeconds) - either one alone can still be fooled by analog
	// noise sitting right at a single threshold; together they bound the turn rate to what a
	// deliberate press-and-release could produce (see VRInput.h's snapTurnCooldownRemaining
	// comment for the real-hardware symptom this fixes).
	if (snapTurnCooldownRemaining > 0.0f)
		snapTurnCooldownRemaining -= deltaSeconds;

	bool snapTurnButtonDown = snapTurnButtonWasDown
		? (TurnAxisX > SnapTurnReleaseThreshold || TurnAxisX < -SnapTurnReleaseThreshold)
		: (TurnAxisX > SnapTurnTriggerThreshold || TurnAxisX < -SnapTurnTriggerThreshold);
	if (snapTurnButtonDown && !snapTurnButtonWasDown && snapTurnCooldownRemaining <= 0.0f)
	{
		BodyYawRadians += (TurnAxisX > 0.0f ? SnapTurnRadians : -SnapTurnRadians);
		snapTurnCooldownRemaining = SnapTurnCooldownSeconds;
	}
	snapTurnButtonWasDown = snapTurnButtonDown;
}

vec3 VRInput::GetAimForwardWorld() const
{
	Coords aimCoords = VRCamera::GetEyeWorldRotation(RightControllerOrientation[0], RightControllerOrientation[1], RightControllerOrientation[2], RightControllerOrientation[3], BodyYawRadians);
	return aimCoords.XAxis; // forward, per VRCamera's established UE1 axis convention
}

Rotator VRInput::GetAimRotation() const
{
	// Coords doesn't carry Roll independently of the basis vectors, so recover a UE1 Rotator
	// the same way the rest of the engine derives one from a direction - Rotator::FromVector
	// gives Yaw/Pitch from the forward axis; Roll is intentionally left at 0 (weapon fire
	// traces don't need visual roll, and UE1 weapon meshes are not typically roll-sensitive
	// for aim purposes).
	Rotator rotation = Rotator::FromVector(GetAimForwardWorld());
	rotation.Roll = 0;
	return rotation;
}

void VRInput::ApplyAimOverride(Rotator& pawnRotation, Rotator& savedRotation) const
{
	savedRotation = pawnRotation;
	pawnRotation = GetAimRotation();
}

void VRInput::RestoreAimOverride(Rotator& pawnRotation, const Rotator& savedRotation) const
{
	pawnRotation = savedRotation;
}
