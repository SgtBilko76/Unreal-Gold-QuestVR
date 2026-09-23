
#include "Precomp.h"
#include "FramebufferManager.h"
#include "VulkanRenderDevice.h"
#include <surrealgpu/vulkanbuilders.h>
#include <surrealgpu/vulkanswapchain.h>

FramebufferManager::FramebufferManager(VulkanRenderDevice* renderer) : renderer(renderer)
{
}

void FramebufferManager::CreateSceneFramebuffer()
{
	SceneFramebuffer = FramebufferBuilder()
		.RenderPass(renderer->RenderPasses->Scene.RenderPass.get())
		.Size(renderer->Textures->Scene->Width, renderer->Textures->Scene->Height)
		.AddAttachment(renderer->Textures->Scene->ColorBufferView.get())
		.AddAttachment(renderer->Textures->Scene->HitBufferView.get())
		.AddAttachment(renderer->Textures->Scene->DepthBufferView.get())
		.DebugName("SceneFramebuffer")
		.Create(renderer->Device.get());

	for (int level = 0; level < NumBloomLevels; level++)
	{
		BloomBlurLevels[level].VTextureFB = FramebufferBuilder()
			.RenderPass(renderer->RenderPasses->Postprocess.RenderPass.get())
			.Size(renderer->Textures->Scene->BloomBlurLevels[level].Width, renderer->Textures->Scene->BloomBlurLevels[level].Height)
			.AddAttachment(renderer->Textures->Scene->BloomBlurLevels[level].VTextureView.get())
			.DebugName("VTextureFB")
			.Create(renderer->Device.get());

		BloomBlurLevels[level].HTextureFB = FramebufferBuilder()
			.RenderPass(renderer->RenderPasses->Postprocess.RenderPass.get())
			.Size(renderer->Textures->Scene->BloomBlurLevels[level].Width, renderer->Textures->Scene->BloomBlurLevels[level].Height)
			.AddAttachment(renderer->Textures->Scene->BloomBlurLevels[level].HTextureView.get())
			.DebugName("HTextureFB")
			.Create(renderer->Device.get());
	}

	for (int i = 0; i < 2; i++)
	{
		PPImageFB[i] = FramebufferBuilder()
			.RenderPass(renderer->RenderPasses->Postprocess.RenderPass.get())
			.Size(renderer->Textures->Scene->Width, renderer->Textures->Scene->Height)
			.AddAttachment(renderer->Textures->Scene->PPImageView[i].get())
			.DebugName("BloomPPImageFB")
			.Create(renderer->Device.get());
	}
}

void FramebufferManager::DestroySceneFramebuffer()
{
	SceneFramebuffer.reset();
	for (int level = 0; level < NumBloomLevels; level++)
	{
		BloomBlurLevels[level].VTextureFB.reset();
		BloomBlurLevels[level].HTextureFB.reset();
	}

	for (int i = 0; i < 2; i++)
		PPImageFB[i].reset();
}

void FramebufferManager::CreateSwapChainFramebuffers()
{
	renderer->RenderPasses->CreatePresentRenderPass();
	renderer->RenderPasses->CreatePresentPipeline();

	auto swapchain = renderer->Commands->SwapChain.get();
	for (int i = 0; i < swapchain->ImageCount(); i++)
	{
		SwapChainFramebuffers.push_back(
			FramebufferBuilder()
				.RenderPass(renderer->RenderPasses->Present.RenderPass.get())
				.Size(renderer->Commands->SwapChain->Width(), renderer->Commands->SwapChain->Height())
				.AddAttachment(swapchain->GetImageView(i))
				.DebugName("SwapChainFramebuffer")
				.Create(renderer->Device.get()));
	}
}

void FramebufferManager::DestroySwapChainFramebuffers()
{
	SwapChainFramebuffers.clear();
}

VulkanFramebuffer* FramebufferManager::GetSwapChainFramebuffer()
{
	return SwapChainFramebuffers[renderer->Commands->PresentImageIndex].get();
}

VulkanFramebuffer* FramebufferManager::GetOrCreateVREyeFramebuffer(int eye, int imageIndex, VkImage image, int width, int height)
{
	auto& eyeFramebuffers = VREyeFramebuffers[eye];
	if ((int)eyeFramebuffers.size() <= imageIndex)
		eyeFramebuffers.resize(imageIndex + 1);

	VREyeFramebuffer& entry = eyeFramebuffers[imageIndex];
	if (entry.Framebuffer && entry.Image == image)
		return entry.Framebuffer.get();

	// The OpenXR runtime is free to hand back a different VkImage for the same swapchain
	// index across session recreations (uncommon, but not disallowed) - rebuild rather than
	// assume the index alone is a stable key.
	entry.Framebuffer.reset();
	entry.View.reset();
	entry.WrappedImage.reset();

	entry.Image = image;
	entry.WrappedImage = std::make_unique<VulkanImage>(renderer->Device.get(), image, nullptr, width, height, 1, 1);
	entry.View = ImageViewBuilder()
		.Image(entry.WrappedImage.get(), renderer->RenderPasses->PresentVR.Format, VK_IMAGE_ASPECT_COLOR_BIT)
		.DebugName("VREyeFramebufferView")
		.Create(renderer->Device.get());
	entry.Framebuffer = FramebufferBuilder()
		.RenderPass(renderer->RenderPasses->PresentVR.RenderPass.get())
		.Size(width, height)
		.AddAttachment(entry.View.get())
		.DebugName("VREyeFramebuffer")
		.Create(renderer->Device.get());

	return entry.Framebuffer.get();
}

void FramebufferManager::DestroyVREyeFramebuffers()
{
	for (auto& eyeFramebuffers : VREyeFramebuffers)
		eyeFramebuffers.clear();
}

VulkanFramebuffer* FramebufferManager::GetOrCreateScreenFramebuffer(int imageIndex, VkImage image, int width, int height)
{
	if ((int)ScreenFramebuffers.size() <= imageIndex)
		ScreenFramebuffers.resize(imageIndex + 1);

	VREyeFramebuffer& entry = ScreenFramebuffers[imageIndex];
	if (entry.Framebuffer && entry.Image == image)
		return entry.Framebuffer.get();

	entry.Framebuffer.reset();
	entry.View.reset();
	entry.WrappedImage.reset();

	entry.Image = image;
	entry.WrappedImage = std::make_unique<VulkanImage>(renderer->Device.get(), image, nullptr, width, height, 1, 1);
	entry.View = ImageViewBuilder()
		.Image(entry.WrappedImage.get(), renderer->RenderPasses->PresentVR.Format, VK_IMAGE_ASPECT_COLOR_BIT)
		.DebugName("ScreenFramebufferView")
		.Create(renderer->Device.get());
	entry.Framebuffer = FramebufferBuilder()
		.RenderPass(renderer->RenderPasses->PresentVR.RenderPass.get())
		.Size(width, height)
		.AddAttachment(entry.View.get())
		.DebugName("ScreenFramebuffer")
		.Create(renderer->Device.get());

	return entry.Framebuffer.get();
}

void FramebufferManager::DestroyScreenFramebuffers()
{
	ScreenFramebuffers.clear();
}
