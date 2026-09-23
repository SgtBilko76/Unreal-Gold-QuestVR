#pragma once

#include "SceneTextures.h"

class VulkanRenderDevice;

class FramebufferManager
{
public:
	FramebufferManager(VulkanRenderDevice* renderer);

	void CreateSceneFramebuffer();
	void DestroySceneFramebuffer();

	void CreateSwapChainFramebuffers();
	void DestroySwapChainFramebuffers();

	VulkanFramebuffer* GetSwapChainFramebuffer();

	// VR path: wraps the externally-owned VkImage OpenXR hands back for one eye's swapchain
	// image (VulkanRenderDevice::DrawPresentTextureVR, VulkanRenderDevice.cpp) in a
	// non-owning VulkanImage/VulkanImageView/VulkanFramebuffer, cached per eye per swapchain
	// image index so repeated frames don't rebuild them. Not used on the desktop WSI path.
	VulkanFramebuffer* GetOrCreateVREyeFramebuffer(int eye, int imageIndex, VkImage image, int width, int height);
	void DestroyVREyeFramebuffers();

	// Same idea as GetOrCreateVREyeFramebuffer, for the mono "big screen" quad layer's
	// swapchain (VulkanRenderDevice::DrawPresentTextureScreen) - a separate cache since it's
	// keyed by its own imageIndex range, unrelated to either eye's.
	VulkanFramebuffer* GetOrCreateScreenFramebuffer(int imageIndex, VkImage image, int width, int height);
	void DestroyScreenFramebuffers();

	std::unique_ptr<VulkanFramebuffer> SceneFramebuffer;
	std::unique_ptr<VulkanFramebuffer> PPImageFB[2];

	struct
	{
		std::unique_ptr<VulkanFramebuffer> VTextureFB;
		std::unique_ptr<VulkanFramebuffer> HTextureFB;
	} BloomBlurLevels[NumBloomLevels];

private:
	VulkanRenderDevice* renderer = nullptr;
	std::vector<std::unique_ptr<VulkanFramebuffer>> SwapChainFramebuffers;

	struct VREyeFramebuffer
	{
		VkImage Image = VK_NULL_HANDLE; // key: rebuild if the runtime hands back a different image for this slot
		std::unique_ptr<VulkanImage> WrappedImage;
		std::unique_ptr<VulkanImageView> View;
		std::unique_ptr<VulkanFramebuffer> Framebuffer;
	};
	// [eye][imageIndex] - OpenXR swapchains are typically 2-4 images deep, so a flat map
	// keyed by (eye, imageIndex) would be overkill; a small per-eye vector matches how the
	// desktop SwapChainFramebuffers vector already works.
	std::vector<VREyeFramebuffer> VREyeFramebuffers[2];
	std::vector<VREyeFramebuffer> ScreenFramebuffers;
};
