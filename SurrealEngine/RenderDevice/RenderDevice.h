#pragma once

#include "Math/vec.h"
#include "Math/mat.h"
#include "Math/coords.h"

#include "Packages/Engine/Resources/Textures/UTexture.h"

#include <surrealwidgets/core/canvas.h>
#include <surrealwidgets/core/rect.h>
#include <surrealwidgets/window/window.h> // for the VkImage forward-declare used by *VR() below

class UTexture;
class UActor;
class Widget;
enum class RenderAPI;

struct SceneNode
{
	int XB, YB; // viewport top left
	int X, Y; // viewport size
	float FX, FY;
	float FX2, FY2;
	Widget* Viewport = nullptr;
	float FovAngle;

	mat4 ObjectToWorld;
	mat4 WorldToView;
	mat4 Projection;

	vec4 NearClip = vec4(0.0f, 0.0f, 1.0f, -1.0f);
	float Zoom = 1.0f;
};

struct GouraudVertex
{
	vec3 Point;
	vec3 Light;
	vec2 UV;
	vec4 Fog;
};

struct SurfaceFacet
{
	Coords MapCoords;
	vec3* Vertices;
	uint32_t VertexCount;
};

struct TextureColor
{
	TextureColor() = default;
	TextureColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) : R(r), G(g), B(b), A(a) { }

	uint8_t R, G, B, A;
};

enum class TextureFormat : uint32_t;

struct TextureInfo
{
	uint64_t CacheID = 0;
	bool bRealtimeChanged = false;

	UTexture* Texture = nullptr;
	float UScale = 1.0f;
	float VScale = 1.0f;
	vec2 Pan = { 0.0f };

	// to do: give these correct values
	TextureFormat Format = {};
	int USize = 1;
	int VSize = 1;
	int NumMips = 0;
	UnrealMipmap* Mips = nullptr;
	TextureColor* Palette = nullptr;
};

inline float GetUMult(const TextureInfo& Info) { return 1.0f / (Info.UScale * Info.USize); }
inline float GetVMult(const TextureInfo& Info) { return 1.0f / (Info.VScale * Info.VSize); }

struct SurfaceInfo
{
	uint32_t PolyFlags = 0;
	TextureInfo* Texture = nullptr;
	TextureInfo* LightMap = nullptr;
	TextureInfo* MacroTexture = nullptr;
	TextureInfo* DetailTexture = nullptr;
	TextureInfo* FogMap = nullptr;
};

class OutputDevice
{
public:
	void Log(const std::string& text) { }
};

class RenderDevice
{
public:
	static std::unique_ptr<RenderDevice> Create(Widget* viewport, RenderAPI renderAPI);

	RenderDevice();
	virtual ~RenderDevice() = default;

	virtual void Flush(bool AllowPrecache) = 0;
	virtual bool Exec(std::string Cmd, OutputDevice& Ar) { return false; }
	virtual void Lock(vec4 FlashScale, vec4 FlashFog, vec4 ScreenClear, uint8_t* HitData, int* HitSize) = 0;
	virtual void Unlock(bool Blit) = 0;
	virtual void DrawComplexSurface(SceneNode* Frame, SurfaceInfo& Surface, SurfaceFacet& Facet) = 0;
	virtual void DrawGouraudPolygon(SceneNode* Frame, TextureInfo& Info, const GouraudVertex* Pts, int NumPts, uint32_t PolyFlags) = 0;
	virtual void DrawTile(SceneNode* Frame, TextureInfo& Info, float X, float Y, float XL, float YL, float U, float V, float UL, float VL, float Z, vec4 Color, vec4 Fog, uint32_t PolyFlags) = 0;
	virtual void Draw3DLine(SceneNode* Frame, vec4 Color, uint32_t LineFlags, vec3 P1, vec3 P2) = 0;
	virtual void Draw2DLine(SceneNode* Frame, vec4 Color, uint32_t LineFlags, vec3 P1, vec3 P2) = 0;
	virtual void Draw2DPoint(SceneNode* Frame, vec4 Color, uint32_t LineFlags, float X1, float Y1, float X2, float Y2, float Z) = 0;
	virtual void ClearZ() = 0;
	virtual void PushHit(const uint8_t* Data, int Count) = 0;
	virtual void PopHit(int Count, bool bForce) = 0;
	virtual void ReadPixels(TextureColor* Pixels) = 0;
	virtual void EndFlash() = 0;
	virtual void SetSceneNode(SceneNode* Frame) = 0;
	virtual void PrecacheTexture(TextureInfo& Info, uint32_t PolyFlags) = 0;
	virtual bool SupportsTextureFormat(TextureFormat Format) = 0;
	virtual void UpdateTextureRect(TextureInfo& Info, int U, int V, int UL, int VL) = 0;

	// VR path (see SurrealEngine/VR/VRRenderLoop.cpp): like Unlock(true), but presents to one
	// eye of an OpenXR stereo display (Viewport->IsStereoDisplay()) instead of a desktop
	// window's swapchain. Default implementation throws, since only VulkanRenderDevice
	// implements it - the OpenXR DisplayWindow backend itself already rejects any RenderAPI
	// other than Vulkan (see SurrealWidgets/src/window/openxr/openxr_display_window.cpp), so
	// GL/D3D11 render devices are never actually asked to do this.
	virtual void UnlockVR(int eye, int imageIndex, VkImage eyeImage, int width, int height) { throw std::runtime_error("This RenderDevice does not support VR stereo presentation"); }

	// Same idea as UnlockVR, for the mono "big screen" quad layer swapchain (menu/UI) instead
	// of a per-eye one - see VulkanRenderDevice.h's UnlockScreen comment. Currently unused:
	// real-hardware testing showed the XrCompositionLayerQuad this fed kept following the
	// player's head despite byte-identical submitted poses frame to frame, so
	// Engine::RunVRMenuScreen() now uses UnlockMenuTexture/DrawMenuWorldQuad below instead.
	// Left in place rather than ripped out - it's self-contained and may be worth revisiting
	// if that runtime quirk is ever root-caused.
	virtual void UnlockScreen(int imageIndex, VkImage screenImage, int width, int height) { throw std::runtime_error("This RenderDevice does not support VR screen-layer presentation"); }

	// World-space "big screen" menu quad (Engine::RunVRMenuScreen(), Engine.cpp). Renders the
	// current 2D menu/UI canvas content into an engine-owned texture (see
	// VulkanRenderDevice.h's MenuTexture) rather than an OpenXR swapchain image, called once
	// per app-frame before either eye renders.
	virtual void UnlockMenuTexture(int width, int height) { throw std::runtime_error("This RenderDevice does not support VR menu-texture rendering"); }

	// Submits that texture as ordinary textured world-space geometry (Corners, world units,
	// wound consistently for a front-facing quad) during the per-eye Scene pass - called from
	// RenderSubsystem::DrawEyeVR after DrawScene(), while Frame still holds that eye's
	// camera transform, so the quad automatically inherits the same already-correct
	// VREyeOverride/MainFrame.Frame projection ordinary level geometry uses instead of a
	// separate OpenXR composition layer.
	// UVMin..UVMax: the texture sub-rectangle this quad shows - the menu canvas only covers a
	// 4:3 region of the (eye-sized) texture, and the curved panel is built from vertical strips
	// that each show a slice of it, see Engine::RunVRMenuScreen().
	virtual void DrawMenuWorldQuad(SceneNode* Frame, const vec3 Corners[4], vec2 UVMin, vec2 UVMax) { throw std::runtime_error("This RenderDevice does not support VR menu-texture rendering"); }

	// Untextured, unlit, solid-color world-space quad (same corner winding as
	// DrawMenuWorldQuad) - the VR aim reticle (RenderSubsystem::DrawVRAimReticle).
	virtual void DrawSolidWorldQuad(SceneNode* Frame, const vec3 Corners[4], vec4 Color) { throw std::runtime_error("This RenderDevice does not support solid world quads"); }

	bool ParseCommand(std::string* cmd, const std::string& keyword) { return false; }

	Widget* Viewport = nullptr;
	bool PrecacheOnFlip = false;
	float Brightness = 0.5f;

	// 2D rendering
	bool IsOrtho = false;
	bool IsOrthoLowDetail = false;

	// For editor hit testing
	int HitX = 0;
	int HitY = 0;
	int HitWidth = 0;
	int HitHeight = 0;

	// Settings
	bool UseVSync = true;
	float GammaOffset = 0.0f;
	float GammaOffsetRed = 0.0f;
	float GammaOffsetGreen = 0.0f;
	float GammaOffsetBlue = 0.0f;
	uint8_t LinearBrightness = 128; // 0.0f;
	uint8_t Contrast = 128; // 1.0f;
	uint8_t Saturation = 255; // 1.0f;
	int GrayFormula = 1;
	bool Hdr = false;
	uint8_t HdrScale = 128;
	bool Bloom = false;
	uint8_t BloomAmount = 128;
	float LODBias = -0.5f;
	uint8_t AntialiasMode = 2; // 4x multisample
	uint8_t GammaMode = 0;
	uint8_t LightMode = 0;
	bool GammaCorrectScreenshots = true;
	bool UseDebugLayer = false;
};

class RenderDeviceTexture : public CanvasTexture
{
public:
	TextureInfo Info;
	UnrealMipmap Mip;
};

class RenderDeviceCanvas : public Canvas
{
public:
	RenderDeviceCanvas(RenderDevice* device);

	void begin(const Colorf& color) override;
	void end() override;

	void begin3d() override;
	void end3d() override;

	RenderDevice* GetRenderDevice() { return device; }

protected:
	std::unique_ptr<CanvasTexture> createTexture(int width, int height, const void* pixels, ImageFormat format) override;
	void drawLineAntialiased(float x0, float y0, float x1, float y1, Colorf color) override;
	void fillTile(float x, float y, float width, float height, Colorf color) override;
	void drawTile(CanvasTexture* texture, float x, float y, float width, float height, float u, float v, float uvwidth, float uvheight, Colorf color) override;
	void drawGlyph(CanvasTexture* texture, float x, float y, float width, float height, float u, float v, float uvwidth, float uvheight, Colorf color) override;

private:
	RenderDevice* device = nullptr;
	SceneNode frame;
};
