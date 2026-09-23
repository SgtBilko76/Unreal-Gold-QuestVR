
#include "Precomp.h"
#include "RenderSubsystem.h"
#include "RenderDevice/RenderDevice.h"
#include "GameWindow.h"
#include "VM/ScriptCall.h"
#include "Engine.h"
#include "VisibleFrame.h"
#include "Packages/Engine/Actors/Pawn/UPawn.h"
#include "Packages/Extension/Windows/UViewportWindow.h"
#include "Packages/Extension/Windows/TabGroup/URootWindow.h"

void RenderSubsystem::DrawScene()
{
	if (!engine->Level)
		return;

	// Light.BeginFrame()/TextureFrameCounter++ intentionally live in BeginVRFrame()/DrawGame()
	// instead of here - see their call sites' comments. DrawScene() runs once per EYE in VR
	// (twice per app-frame), but both of those represent per-app-frame state (light tree
	// rebuild + flicker/pulse timing, texture animation gating) - calling them here would
	// silently double their effective rate under VR and let flicker-light timing diverge
	// between the two eyes' otherwise-identical scene (confirmed on real Quest 3 hardware via
	// a diagnostic build that forced both eyes to render from identical camera data: the
	// doubling/ghosting persisted even then, which ruled out a stereo/camera bug and pointed
	// at exactly this kind of per-call-instead-of-per-frame state).

	// Make sure all actors are at the right location in the BSP
	for (UActor* actor : engine->Level->Actors)
	{
		if (actor)
			actor->UpdateBspInfo();
	}

	if (VREyeOverride.Active)
	{
		MainFrame.Process(VREyeOverride.EyeLocation, VREyeOverride.WorldToView, VREyeOverride.EyeRotation, false, 0, {}, vec4(0.0f, 0.0f, 0.0f, 1.0f), &VREyeOverride.Projection);
	}
	else
	{
		mat4 worldToView = Coords::ViewToRenderDev().ToMatrix() * Coords::Rotation(engine->CameraRotation).Inverse().ToMatrix() * Coords::Location(engine->CameraLocation).ToMatrix();
		MainFrame.Process(engine->CameraLocation, worldToView, Coords::Rotation(engine->CameraRotation));
	}
	MainFrame.Draw();
	MainFrame.DrawCoronas();
}

void RenderSubsystem::DrawViewport(UViewportWindow* viewport)
{
	if (!engine->Level)
		return;

	float x = 0.0f, y = 0.0f;
	engine->dxRootWindow->ConvertCoordinates(viewport, 0.0f, 0.0f, engine->dxRootWindow, x, y);
	engine->dxRootWindow->SetRenderViewport(x, y, viewport->Width(), viewport->Height());

	if (viewport->bClearZ())
		Device->ClearZ();

	bool originActorWasHidden = false;
	vec3 location;
	Rotator rotation(0,0,0);
	if (UActor* originActor = viewport->originActor())
	{
		originActorWasHidden = originActor->bHidden();
		originActor->bHidden() = true;
		location = originActor->Location() + viewport->relLocation();
		if (viewport->bUseEyeHeight())
		{
			if (auto pawn = UObject::TryCast<UPawn>(originActor))
				location.z += pawn->BaseEyeHeight();
		}
		rotation = originActor->Rotation();
	}
	else
	{
		location = viewport->Location() + viewport->relLocation();
		rotation = viewport->Rotation();
	}

	if (viewport->bUseViewRotation())
	{
		rotation = engine->CameraRotation;
	}
	else if (UActor* watchActor = viewport->watchActor())
	{
		vec3 lookAt = watchActor->Location();
		if (viewport->bWatchEyeHeight())
		{
			if (auto pawn = UObject::TryCast<UPawn>(watchActor))
				lookAt.z += pawn->BaseEyeHeight();
		}
		rotation = Rotator::FromVector(lookAt - location);
	}

	rotation += viewport->relRotation();

	mat4 worldToView = Coords::ViewToRenderDev().ToMatrix() * Coords::Rotation(rotation).Inverse().ToMatrix() * Coords::Location(location).ToMatrix();
	MainFrame.Process(location, worldToView, Coords::Rotation(rotation));
	MainFrame.Draw();
	MainFrame.DrawCoronas();

	Device->SetSceneNode(&Canvas.Frame);

	engine->dxRootWindow->ResetRenderViewport();

	if (UActor* originActor = viewport->originActor())
		originActor->bHidden() = originActorWasHidden;
}
