#pragma once

#include "Engine.h"
#include "VisibleNode.h"
#include "VisibleTranslucent.h"
#include "VisibleCorona.h"
#include "VisibleActor.h"
#include "VisiblePortal.h"
#include "BspClipper.h"
#include "RenderDevice/RenderDevice.h"

class VisibleFrame
{
public:
	// projectionOverride: used for VR stereo rendering, where the projection must be an
	// asymmetric per-eye frustum (see SurrealEngine/VR/VRCamera.h) rather than the symmetric
	// FovAngle-derived one SetupSceneFrame() builds by default. Null for the normal path.
	void Process(const vec3& location, const mat4& worldToView, const Coords& viewRotation, bool mirrorFlag = false, int portalDepth = 0, const Array<PortalSpan>& portalSpans = {}, const vec4& portalPlane = vec4(0.0f, 0.0f, 0.0f, 1.0f), const mat4* projectionOverride = nullptr);
	void Draw();
	void DrawCoronas();

	RenderDevice* Device = nullptr;

	SceneNode Frame;
	BspClipper Clipper;
	vec4 ViewLocation = vec4(0.0f);
	Coords ViewRotation = {};
	int ViewZone = 0;
	//uint64_t ViewZoneMask = 0;
	int FrameCounter = 0;
	bool MirrorFlag = false;
	int PortalDepth = 0;

	Array<VisibleNode> OpaqueNodes;
	Array<VisibleActor> Actors;
	Array<VisibleTranslucent> Translucents;
	Array<VisibleCorona> Coronas;
	Array<VisiblePortal> Portals;

	// Fills Frame (viewport rect, matrices) without traversing the BSP - the VR eye pass
	// uses this on frames where the world isn't drawn at all (console bNoDrawWorld) but the
	// world-space menu panel still needs this eye's camera (RenderSubsystem::DrawEyeVR).
	void SetupSceneFrame(const mat4& worldToView, const mat4* projectionOverride);

private:
	void ProcessNode(BspNode* node);
	void ProcessNodeSurface(BspNode* node, bool front);
	void SortTranslucent();

	void DrawOpaqueNodes();
	void DrawOpaqueActors();
	void DrawTranslucent();
	void DrawPortals();

	int FindZoneAt(const vec3& location);
	int FindZoneAt(const vec4& location, BspNode* node, BspNode* nodes);

	vec3 WarpLocationToOtherSide(UWarpZoneInfo* warpZone, vec3 p);
	vec3 WarpNormalToOtherSide(UWarpZoneInfo* warpZone, vec3 n);
	Coords WarpRotationToOtherSide(UWarpZoneInfo* warpZone, Coords rotation);
};
