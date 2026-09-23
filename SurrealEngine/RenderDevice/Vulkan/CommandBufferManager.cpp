
#include "Precomp.h"
#include "CommandBufferManager.h"
#include "VulkanRenderDevice.h"
#include <surrealgpu/vulkanswapchain.h>

CommandBufferManager::CommandBufferManager(VulkanRenderDevice* renderer) : renderer(renderer)
{
	SwapChain = VulkanSwapChainBuilder()
		.Create(renderer->Device.get());

	ImageAvailableSemaphore = SemaphoreBuilder()
		.DebugName("ImageAvailableSemaphore")
		.Create(renderer->Device.get());

	for (InFlight& slot : Slots)
	{
		slot.Fence = FenceBuilder()
			.DebugName("RenderFinishedFence")
			.Create(renderer->Device.get());
		slot.Deletes = std::make_unique<DeleteList>();
	}

	TransferFence = FenceBuilder()
		.DebugName("TransferFence")
		.Create(renderer->Device.get());

	CommandPool = CommandPoolBuilder()
		.QueueFamily(renderer->Device.get()->GraphicsFamily)
		.DebugName("CommandPool")
		.Create(renderer->Device.get());

	FrameDeleteList = std::make_unique<DeleteList>();
}

CommandBufferManager::~CommandBufferManager()
{
	WaitForAll();
	DeleteFrameObjects();
}

int CommandBufferManager::PendingCount() const
{
	int count = 0;
	for (const InFlight& slot : Slots)
		if (slot.Pending)
			count++;
	return count;
}

void CommandBufferManager::WaitForSlot(InFlight& slot)
{
	if (!slot.Pending)
		return;

	VkResult result = vkWaitForFences(renderer->Device.get()->device, 1, &slot.Fence->fence, VK_TRUE, std::numeric_limits<uint64_t>::max());
	if (result != VK_SUCCESS)
		throw std::runtime_error("vkWaitForFences failed");
	result = vkResetFences(renderer->Device.get()->device, 1, &slot.Fence->fence);
	if (result != VK_SUCCESS)
		throw std::runtime_error("vkResetFences failed");

	slot.Pending = false;
	slot.DrawCommands.reset();
	slot.TransferCommands.reset();
	slot.Deletes = std::make_unique<DeleteList>(); // frees everything retired by that submission
}

void CommandBufferManager::WaitForAll()
{
	for (InFlight& slot : Slots)
		WaitForSlot(slot);
	if (renderer->Uploads) // null while VulkanRenderDevice is still constructing its managers (TextureManager records uploads before UploadManager exists)
		renderer->Uploads->ResetUploadBufferPos();
}

void CommandBufferManager::EnsureSlotReady()
{
	InFlight& slot = Slots[CurrentSlot];
	if (slot.Pending)
		WaitForSlot(slot);
	if (PendingCount() == 0 && renderer->Uploads)
		renderer->Uploads->ResetUploadBufferPos(); // nobody is reading the staging buffer any more
}

void CommandBufferManager::WaitForTransfer()
{
	renderer->Uploads->SubmitUploads();

	// Called from the texture upload path when the staging buffer is full, typically in the
	// middle of recording draw commands. Submit just the transfer commands recorded so far on
	// their own fence, drain everything, and rewind the staging buffer - the slot's draw
	// command buffer (and its vertex/index buffers) stay exactly as they are.
	InFlight& slot = Slots[CurrentSlot];
	if (slot.TransferCommands)
	{
		slot.TransferCommands->end();

		QueueSubmit()
			.AddCommandBuffer(slot.TransferCommands.get())
			.Execute(renderer->Device.get(), renderer->Device.get()->GraphicsQueue, TransferFence.get());

		VkResult result = vkWaitForFences(renderer->Device.get()->device, 1, &TransferFence->fence, VK_TRUE, std::numeric_limits<uint64_t>::max());
		if (result != VK_SUCCESS)
			throw std::runtime_error("vkWaitForFences failed");
		result = vkResetFences(renderer->Device.get()->device, 1, &TransferFence->fence);
		if (result != VK_SUCCESS)
			throw std::runtime_error("vkResetFences failed");

		slot.TransferCommands.reset();
	}

	WaitForAll();
}

void CommandBufferManager::SubmitCommands(bool present, int presentWidth, int presentHeight, bool presentFullscreen, bool waitForCompletion)
{
	renderer->Uploads->SubmitUploads();

	if (present)
	{
		if (SwapChain->Lost() || SwapChain->Width() != presentWidth || SwapChain->Height() != presentHeight || UsingVsync != renderer->UseVSync || UsingHdr != renderer->Hdr)
		{
			WaitForAll(); // the old swapchain framebuffers may still be in use
			UsingVsync = renderer->UseVSync;
			UsingHdr = renderer->Hdr;
			renderer->Framebuffers->DestroySwapChainFramebuffers();
			SwapChain->Create(presentWidth, presentHeight, renderer->UseVSync ? 2 : 3, renderer->UseVSync, renderer->Hdr);
			renderer->Framebuffers->CreateSwapChainFramebuffers();

			RenderFinishedSemaphores.clear();
			for (int i = 0; i < SwapChain->ImageCount(); i++)
			{
				RenderFinishedSemaphores.push_back(SemaphoreBuilder()
					.DebugName("RenderFinishedSemaphore")
					.Create(renderer->Device.get()));
			}
		}

		PresentImageIndex = SwapChain->AcquireImage(ImageAvailableSemaphore.get());
		if (PresentImageIndex != -1)
		{
			renderer->DrawPresentTexture(presentWidth, presentHeight);
		}
	}

	InFlight& slot = Slots[CurrentSlot];

	if (slot.TransferCommands)
		slot.TransferCommands->end();
	if (slot.DrawCommands)
		slot.DrawCommands->end();

	// Transfer and draw command buffers go into ONE submission, transfer first - command
	// buffers within a submission execute in order (the upload barriers inside them provide
	// the memory dependencies), so no semaphore is needed between them. That also keeps the
	// ring simple: one fence per slot covers both.
	QueueSubmit submit;
	if (slot.TransferCommands)
		submit.AddCommandBuffer(slot.TransferCommands.get());
	if (slot.DrawCommands)
		submit.AddCommandBuffer(slot.DrawCommands.get());
	if (present && PresentImageIndex != -1)
	{
		submit.AddWait(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, ImageAvailableSemaphore.get());
		submit.AddSignal(RenderFinishedSemaphores[PresentImageIndex].get());
	}
	submit.Execute(renderer->Device.get(), renderer->Device.get()->GraphicsQueue, slot.Fence.get());
	slot.Pending = true;
	slot.Deletes = std::move(FrameDeleteList);
	FrameDeleteList = std::make_unique<DeleteList>();

	if (present && PresentImageIndex != -1)
	{
		SwapChain->QueuePresent(PresentImageIndex, RenderFinishedSemaphores[PresentImageIndex].get());
	}

	CurrentSlot = (CurrentSlot + 1) % NumInFlight;
	renderer->Buffers->SetCurrentSlot(CurrentSlot);

	if (waitForCompletion)
		WaitForAll();
	else
		EnsureSlotReady();
}

VulkanCommandBuffer* CommandBufferManager::GetTransferCommands()
{
	InFlight& slot = Slots[CurrentSlot];
	if (!slot.TransferCommands)
	{
		EnsureSlotReady();
		slot.TransferCommands = CommandPool->createBuffer();
		slot.TransferCommands->begin();
	}
	return slot.TransferCommands.get();
}

VulkanCommandBuffer* CommandBufferManager::GetDrawCommands()
{
	InFlight& slot = Slots[CurrentSlot];
	if (!slot.DrawCommands)
	{
		EnsureSlotReady();
		slot.DrawCommands = CommandPool->createBuffer();
		slot.DrawCommands->begin();
	}
	return slot.DrawCommands.get();
}

void CommandBufferManager::DeleteFrameObjects()
{
	FrameDeleteList = std::make_unique<DeleteList>();
}
