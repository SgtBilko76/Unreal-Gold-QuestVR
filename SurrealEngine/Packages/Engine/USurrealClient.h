#pragma once

#include "UClient.h"

class USurrealClient : public UClient
{
public:
	using UClient::UClient;

	std::string Class = "Engine.SurrealClient";
	bool StartupFullscreen = false;
	int WindowedViewportX = 1920;
	int WindowedViewportY = 1080;
	int WindowedColorBits = 32;
	int FullscreenViewportX = 0;
	int FullscreenViewportY = 0;
	int FullscreenColorBits = 32;
	float Brightness = 0.5f;

	// Brightness as the render device expects it (0.5 = neutral). OldUnreal's UT 469 rescaled
	// the ini/slider value so that 1.0 is neutral (its Default.ini ships Brightness=1.000000),
	// so a 469 install's value is halved here instead of blowing the picture out; the stored
	// property keeps the game's own scale so the menu slider and SaveConfig round-trip.
	float RenderBrightness() const;
	bool UseJoystick = false;
	bool UseDirectInput = true;
	int MinDesiredFrameRate = 200;
	bool Decals = true;
	bool NoDynamicLights = false;
	std::string TextureDetail = "High";
	std::string SkinDetail = "High";

	void LoadProperties(const NameString& from = "");
	void SaveConfig() override;

	std::string GetPropertyAsString(const NameString& propertyName) const override;
	void SetPropertyFromString(const NameString& propertyName, const std::string& value) override;
};
