
#include "Precomp.h"
#include "VRCamera.h"
#include <cmath>

// See VRCamera.h for the axis-convention caveat this all rests on.
static vec3 OpenXRToUnrealDirection(float x, float y, float z)
{
	// forward_ue = -z_xr, right_ue = x_xr, up_ue = y_xr
	return vec3(-z, x, y);
}

static vec3 RotateAroundZ(vec3 v, float yawRadians)
{
	float c = std::cos(yawRadians);
	float s = std::sin(yawRadians);
	return vec3(v.x * c - v.y * s, v.x * s + v.y * c, v.z);
}

vec3 VRCamera::GetEyeWorldLocation(const StereoEyeView& eye, vec3 worldPlayerLocation, float worldPlayerYawRadians, float uuPerMeter, vec3 headAnchorPosition, vec3 headCenterPositionMeters, float eyeSeparationScale)
{
	vec3 rawPositionMeters = vec3(eye.PositionX, eye.PositionY, eye.PositionZ);

	// Split into head-center movement (since the anchor, scaled by uuPerMeter) and this eye's
	// IPD offset from head-center (scaled by uuPerMeter * eyeSeparationScale) - see VRCamera.h's
	// BuildWorldToView comment for why these need separate scales.
	vec3 headOffsetMeters = headCenterPositionMeters - headAnchorPosition;
	vec3 eyeOffsetFromHeadMeters = rawPositionMeters - headCenterPositionMeters;

	vec3 headOffsetUnreal = OpenXRToUnrealDirection(headOffsetMeters.x, headOffsetMeters.y, headOffsetMeters.z) * uuPerMeter;
	vec3 eyeOffsetUnreal = OpenXRToUnrealDirection(eyeOffsetFromHeadMeters.x, eyeOffsetFromHeadMeters.y, eyeOffsetFromHeadMeters.z) * uuPerMeter * eyeSeparationScale;

	vec3 offsetUnreal = headOffsetUnreal + eyeOffsetUnreal;
	vec3 offsetWorld = RotateAroundZ(offsetUnreal, worldPlayerYawRadians);
	return worldPlayerLocation + offsetWorld;
}

vec3 VRCamera::StageToWorldPosition(vec3 stagePositionMeters, vec3 worldPlayerLocation, float worldPlayerYawRadians, float uuPerMeter, vec3 headAnchorPosition)
{
	vec3 offsetMeters = stagePositionMeters - headAnchorPosition;
	vec3 offsetUnreal = OpenXRToUnrealDirection(offsetMeters.x, offsetMeters.y, offsetMeters.z) * uuPerMeter;
	return worldPlayerLocation + RotateAroundZ(offsetUnreal, worldPlayerYawRadians);
}

vec3 VRCamera::StageToWorldDirection(vec3 stageDirection, float worldPlayerYawRadians)
{
	return RotateAroundZ(OpenXRToUnrealDirection(stageDirection.x, stageDirection.y, stageDirection.z), worldPlayerYawRadians);
}

Coords VRCamera::GetEyeWorldRotation(float orientationX, float orientationY, float orientationZ, float orientationW, float worldPlayerYawRadians)
{
	// The head/eye orientation quaternion (OpenXR space) becomes a rotation matrix, then its
	// basis vectors are remapped into UE1 world space the same way GetEyeWorldLocation()
	// remaps the position, and finally rotated by the body's world-facing yaw so a player
	// turning their body (not just their head) turns the whole play space with them.
	mat4 quatMatrix = mat4::quaternion(orientationX, orientationY, orientationZ, orientationW);

	vec3 rightXR(quatMatrix[0 * 4 + 0], quatMatrix[0 * 4 + 1], quatMatrix[0 * 4 + 2]);
	vec3 upXR(quatMatrix[1 * 4 + 0], quatMatrix[1 * 4 + 1], quatMatrix[1 * 4 + 2]);
	vec3 forwardXR(-quatMatrix[2 * 4 + 0], -quatMatrix[2 * 4 + 1], -quatMatrix[2 * 4 + 2]);

	Coords eyeRotation;
	eyeRotation.Origin = vec3(0.0f);
	eyeRotation.XAxis = RotateAroundZ(OpenXRToUnrealDirection(forwardXR.x, forwardXR.y, forwardXR.z), worldPlayerYawRadians);
	eyeRotation.YAxis = RotateAroundZ(OpenXRToUnrealDirection(rightXR.x, rightXR.y, rightXR.z), worldPlayerYawRadians);
	eyeRotation.ZAxis = RotateAroundZ(OpenXRToUnrealDirection(upXR.x, upXR.y, upXR.z), worldPlayerYawRadians);
	return eyeRotation;
}

mat4 VRCamera::BuildWorldToView(const StereoEyeView& eye, vec3 worldPlayerLocation, float worldPlayerYawRadians, float uuPerMeter, vec3 headAnchorPosition, vec3 headCenterPositionMeters, float eyeSeparationScale, float headOrientationX, float headOrientationY, float headOrientationZ, float headOrientationW)
{
	// Rotation comes from the SHARED head orientation (queried once via HeadSpace, NOT each
	// eye's own individually-reported OpenXR quaternion) - matches Team Beef Studios'
	// QuakeQuest's approach (TBXR_Common.c: a single xfStageFromHead orientation drives both
	// eyes). Confirmed via real Quest 3 hardware testing: using each eye's own orientation made
	// the two eyes' views feel unacceptably "different" in a way that reducing eye separation
	// (which only affects POSITION, not rotation) didn't fix - see VRCamera.h's comment.
	Coords eyeRotation = GetEyeWorldRotation(headOrientationX, headOrientationY, headOrientationZ, headOrientationW, worldPlayerYawRadians); // Origin stays (0,0,0)
	vec3 eyeLocation = GetEyeWorldLocation(eye, worldPlayerLocation, worldPlayerYawRadians, uuPerMeter, headAnchorPosition, headCenterPositionMeters, eyeSeparationScale);

	// Must match RenderScene.cpp's DrawScene() desktop composition EXACTLY:
	//   ViewToRenderDev().ToMatrix() * Rotation(camRot).Inverse().ToMatrix() * Location(camLoc).ToMatrix()
	// i.e. rotation and translation as two SEPARATE Coords multiplied as separate matrices.
	// Coords::Inverse()'s Origin formula (-dot(Origin,XAxis), using the pre-inverse axes) only
	// yields the correct world-to-view translation when Origin is (0,0,0) going in - a previous
	// version of this function set eyeCoords.Origin to the eye's world position BEFORE calling
	// Inverse() on the combined rotation+origin Coords, which silently produced the wrong
	// camera translation for any non-identity eye rotation. Confirmed on real Quest 3 hardware:
	// this desynced the two eyes' apparent camera positions and put the camera outside the
	// level (camera outside the level, eyes desynced).
	return Coords::ViewToRenderDev().ToMatrix() * eyeRotation.Inverse().ToMatrix() * Coords::Location(eyeLocation).ToMatrix();
}

mat4 VRCamera::BuildProjection(const StereoEyeView& eye, float zNear, float zFar)
{
	// FovAngleLeft/Down are negative per the OpenXR spec (angles left of / below forward),
	// so these tangents come out with the correct sign for mat4::frustum() directly.
	float left = std::tan(eye.FovAngleLeft) * zNear;
	float right = std::tan(eye.FovAngleRight) * zNear;
	float down = std::tan(eye.FovAngleDown) * zNear;
	float up = std::tan(eye.FovAngleUp) * zNear;

	// The render device's view space has +y pointing DOWN the screen: Coords::ViewToRenderDev()
	// (Math/coords.h) maps UE1 up (+Z) to view -y, and DrawTile's pixel-to-view unprojection
	// (VulkanRenderDevice.cpp) likewise maps increasing screen Y to increasing view y - the
	// Vulkan NDC y-down convention carried through without a viewport flip. mat4::frustum's
	// "bottom" parameter is whichever view-y edge lands on NDC -1, i.e. the TOP of the screen
	// here. So the screen's top edge sits at view y = -tan(angleUp) and its bottom edge at
	// view y = -tan(angleDown) (positive, since angleDown is negative). Passing (down, up)
	// straight through mirrored the vertical asymmetry: OpenXR's real FOV is ~44 degrees up
	// / ~55 down, and rendering it as 55 up / 44 down while telling the compositor the
	// opposite made its reprojection visibly "squeeze" the view on every head movement
	// (real Quest 3 hardware). Same for both eyes, so it never showed as an eye difference.
	return mat4::frustum(left, right, -up, -down, zNear, zFar, handedness::left, clipzrange::zero_positive_w);
}
