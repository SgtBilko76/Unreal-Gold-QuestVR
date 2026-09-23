#pragma once

#include <surrealwidgets/window/window.h>
#include "Math/mat.h"
#include "Math/vec.h"
#include "Math/coords.h"
#include "Math/rotator.h"

// Converts one eye's OpenXR pose/FOV (StereoEyeView, in OpenXR's own tracking-space
// convention: right-handed, +Y up, +X right, -Z forward) into the WorldToView matrix and
// asymmetric projection matrix RenderSubsystem::DrawScene() needs for that eye, anchored to
// the player Pawn's UE1 location/yaw (so the play-space origin follows the player's body
// through the level, matching QuakeQuest's playerYaw-anchoring approach).
//
// AXIS CONVENTION WARNING: SurrealEngine's world space (via Coords/Rotator, see
// Math/coords.h, Math/rotator.h) is UE1's convention - X=forward, Z=up - built for a
// keyboard/mouse game with no independent notion of "which way is right" from head tracking.
// This code assumes UE1's Y axis is "right" when facing along X (forward_ue = -Z_xr,
// right_ue = +X_xr, up_ue = +Y_xr) by analogy with a standard right-handed forward/right/up
// basis. This has NOT been verified against real Quest hardware. The symptom of getting it
// wrong (right_ue's sign flipped) is that turning your head physically left makes the
// in-game view roll/mirror in an unnatural way, or strafe direction feels inverted relative
// to head yaw - if that happens, flip the sign on the right_ue mapping below, matching how
// Team Beef Studios' QuakeQuest instead maps right_ue = -X_xr for their engine's convention
// (Projects/Android/jni/QuakeQuestSrc/TBXR_Common.c, QuatToYawPitchRoll).
class VRCamera
{
public:
	// Unreal units per real-world meter - matches SurrealEngine/Audio/AudioDevice.cpp's
	// UU_PER_METER. Shared by Engine::RunVR()'s camera setup, RunVRMenuScreen()'s panel
	// placement and RenderSubsystem::DrawActor()'s controller-held weapon so they all agree.
	static constexpr float UnitsPerMeter = 43.0f;

	// Extra height added to the game camera in VR (Engine::RunVR), in meters. The script camera
	// sits at the Pawn's EyeHeight, which was tested with a raised offset on Quest 3; the
	// offset was tuned to 30 cm. Applied once per frame before eyes, menu panel, scope and aim
	// are derived from CameraLocation, so they all agree.
	static constexpr float HeadHeightOffsetMeters = 0.0f; // was 0.30; removed - it skewed the fire-convergence origin (see VRAimOverrideScope). Recenter sets the height now.

	// worldPlayerLocation/worldPlayerYawRadians: the anchor point in the level the play-space
	// origin is centered on - typically the Pawn's Location() and body-facing yaw (NOT head
	// yaw - see VRInput.h for the head/body split), so that walking in the level moves the
	// whole play space, matching the non-VR camera's behavior. headAnchorPosition
	// (VRInput::HeadAnchorPosition) is subtracted from the eye's raw OpenXR STAGE-space
	// position before converting to a world offset, since STAGE space is anchored to the
	// physical room's Guardian-boundary center, not the game's spawn point - using the raw
	// position directly would place the camera off by however far from room-center the player
	// happened to be standing when tracking started (confirmed on real hardware).
	// headCenterPositionMeters (VRInput::HeadView's averaged position) and eyeSeparationScale
	// split the eye's raw offset into two independently-scaled parts: the head-center's
	// movement since headAnchorPosition (how far you physically walked, scaled by uuPerMeter -
	// this calibrates against the level's geometry/movement speed) and this eye's IPD offset
	// from head-center (scaled by uuPerMeter * eyeSeparationScale - this calibrates stereo
	// depth perception). These need separate scales: uuPerMeter is inherited from
	// AudioDevice.cpp's UU_PER_METER (calibrated for realistic sound falloff distance, not
	// visual depth), and using it unscaled for eye separation too produced a "miniaturized
	// world / exaggerated depth" (hyperstereo) sensation on real Quest 3 hardware - report
	// turned out NOT to be fixed by reducing eyeSeparationScale (tried down to 0.3, still felt
	// mismatched), which pointed at rotation, not position - see headOrientation* below.
	// eyeSeparationScale is kept as a tunable knob (1.0 = QuakeQuest's own unreduced approach:
	// GetStereoSeparation() = vr_worldscale * VR_GetIPD(), gl_rmain.c) but is not expected to
	// need adjusting now that rotation is fixed.
	//
	// headOrientationX/Y/Z/W: the SHARED head orientation (Widget::GetHeadPose(), queried once
	// per app-frame via the OpenXR HeadSpace/VIEW reference space) used for BOTH eyes' rotation
	// - NOT each eye's own individually-reported orientation from StereoEyeView. Matches Team
	// Beef Studios' QuakeQuest (TBXR_Common.c: a single xfStageFromHead orientation drives both
	// eyes, only position varies via IPD). Confirmed via real Quest 3 hardware testing this was
	// the actual cause of a "the two eyes feel too different" report that persisted even with
	// eye separation reduced to near-zero.
	static mat4 BuildWorldToView(const StereoEyeView& eye, vec3 worldPlayerLocation, float worldPlayerYawRadians, float uuPerMeter, vec3 headAnchorPosition, vec3 headCenterPositionMeters, float eyeSeparationScale, float headOrientationX, float headOrientationY, float headOrientationZ, float headOrientationW);

	// The world-space forward/right/up basis (UE1 world coordinates) for the given orientation
	// quaternion (OpenXR convention), for VisibleFrame::Process()'s viewRotation parameter -
	// used for sprite billboard orientation and decal depth offsetting
	// (Render/VisibleSprite.cpp, Render/VisibleDecal.cpp). Callers pass EITHER the shared head
	// orientation (BuildWorldToView's use, matching QuakeQuest - see its comment) or a
	// controller's own orientation (VRInput::GetAimRotation(), where per-controller orientation
	// genuinely is what's wanted).
	static Coords GetEyeWorldRotation(float orientationX, float orientationY, float orientationZ, float orientationW, float worldPlayerYawRadians);

	// Builds the asymmetric off-axis projection matrix for one eye from its FOV half-angles
	// (StereoEyeView::FovAngleLeft/Right/Up/Down, tangent-space radians from forward).
	static mat4 BuildProjection(const StereoEyeView& eye, float zNear, float zFar);

	// The eye's location in UE1 world coordinates (uu), for VisibleFrame::Process()'s
	// location parameter (used for zone-finding, not just rendering). See BuildWorldToView()
	// for why headAnchorPosition is needed.
	static vec3 GetEyeWorldLocation(const StereoEyeView& eye, vec3 worldPlayerLocation, float worldPlayerYawRadians, float uuPerMeter, vec3 headAnchorPosition, vec3 headCenterPositionMeters, float eyeSeparationScale);

	// Maps an arbitrary OpenXR STAGE-space point (meters) into UE1 world coordinates (uu) using
	// the exact same anchor/scale/body-yaw chain GetEyeWorldLocation() applies to the eyes -
	// so anything placed with this (Engine::RunVRMenuScreen()'s world-space menu quad) sits
	// in the same frame of reference as the rendered camera and stays put in the room as the
	// player moves their head.
	static vec3 StageToWorldPosition(vec3 stagePositionMeters, vec3 worldPlayerLocation, float worldPlayerYawRadians, float uuPerMeter, vec3 headAnchorPosition);

	// Same for a direction (no anchor/scale, just the axis remap + body yaw).
	static vec3 StageToWorldDirection(vec3 stageDirection, float worldPlayerYawRadians);
};
