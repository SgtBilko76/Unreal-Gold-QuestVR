#pragma once

#include <surrealgpu/vulkanobjects.h>

class VulkanRenderDevice;

// Owns the per-submission GPU objects (command buffers, fences, deferred-delete lists) as a
// ring of NumInFlight slots, so a submission can be left running on the GPU while the CPU
// records the next one. The VR path uses this to overlap eye 0's GPU work with eye 1's CPU
// work (and eye 1's GPU work with the next frame's game tick) instead of blocking on a fence
// after every eye - confirmed on real Quest 3 hardware as the actual frame-rate limiter (GPU
// ~55% busy, CPU mostly waiting) once the per-eye resolution went up. The desktop path keeps
// its old submit-then-wait behavior (SubmitCommands with waitForCompletion=true).
//
// Everything the CPU writes and the GPU reads during a submission must be per-slot or
// otherwise protected: the scene vertex/index buffers are per-slot (BufferManager, switched in
// step with the slot), the texture upload staging buffer is only rewound when no submission is
// pending (UploadManager::ResetUploadBufferPos from EnsureSlotReady/WaitForAll), objects
// retired during recording go to the slot's delete list and are freed after its fence, and
// anything that destroys shared GPU resources outright (scene buffer recreation, texture cache
// clears, sampler rebuilds, screenshots) calls WaitForAll() first.
class CommandBufferManager
{
public:
	CommandBufferManager(VulkanRenderDevice* renderer);
	~CommandBufferManager();

	static const int NumInFlight = 3; // menu texture + two eyes per VR frame without stalling

	void WaitForTransfer();

	// present=false (used by the VR path, VulkanRenderDevice::UnlockVR) skips the desktop WSI
	// swapchain acquire/present entirely and just submits whatever was already recorded onto
	// the current draw command buffer. waitForCompletion=false returns as soon as the work is
	// queued and advances to the next slot; the slot is only waited on when it is about to be
	// reused (EnsureSlotReady) or by WaitForAll().
	void SubmitCommands(bool present, int presentWidth, int presentHeight, bool presentFullscreen, bool waitForCompletion = true);

	// Blocks until every pending submission has finished on the GPU, frees their retired
	// objects and rewinds the upload staging buffer.
	void WaitForAll();

	VulkanCommandBuffer* GetTransferCommands();
	VulkanCommandBuffer* GetDrawCommands();
	void DeleteFrameObjects();

	struct DeleteList
	{
		std::vector<std::unique_ptr<VulkanImage>> images;
		std::vector<std::unique_ptr<VulkanImageView>> imageViews;
		std::vector<std::unique_ptr<VulkanBuffer>> buffers;
		std::vector<std::unique_ptr<VulkanDescriptorSet>> descriptors;
	};
	// Objects retired while recording the CURRENT slot; moved into the slot on submit.
	std::unique_ptr<DeleteList> FrameDeleteList;

	std::shared_ptr<VulkanSwapChain> SwapChain;
	int PresentImageIndex = -1;
	bool UsingVsync = false;
	bool UsingHdr = false;

private:
	struct InFlight
	{
		std::unique_ptr<VulkanFence> Fence;
		std::unique_ptr<VulkanCommandBuffer> DrawCommands;
		std::unique_ptr<VulkanCommandBuffer> TransferCommands;
		std::unique_ptr<DeleteList> Deletes;
		bool Pending = false;
	};

	// Waits for the current slot's previous submission (if still pending) before anything is
	// recorded into it, and rewinds the upload staging buffer if nothing is pending at all.
	void EnsureSlotReady();
	void WaitForSlot(InFlight& slot);
	int PendingCount() const;

	VulkanRenderDevice* renderer = nullptr;

	std::unique_ptr<VulkanSemaphore> ImageAvailableSemaphore;
	std::vector<std::unique_ptr<VulkanSemaphore>> RenderFinishedSemaphores;
	std::unique_ptr<VulkanFence> TransferFence; // WaitForTransfer's standalone upload submission
	std::unique_ptr<VulkanCommandPool> CommandPool;

	InFlight Slots[NumInFlight];
	int CurrentSlot = 0;
};
