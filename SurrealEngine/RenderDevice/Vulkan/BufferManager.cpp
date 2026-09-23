
#include "Precomp.h"
#include "BufferManager.h"
#include "VulkanRenderDevice.h"

BufferManager::BufferManager(VulkanRenderDevice* renderer) : renderer(renderer)
{
	CreateSceneVertexBuffers();
	CreateSceneIndexBuffers();
	CreateUploadBuffer();
	SetCurrentSlot(0);
}

BufferManager::~BufferManager()
{
	for (int i = 0; i < NumSlots; i++)
	{
		if (SceneVerticesSlots[i]) { SceneVertexBuffers[i]->Unmap(); SceneVerticesSlots[i] = nullptr; }
		if (SceneIndexesSlots[i]) { SceneIndexBuffers[i]->Unmap(); SceneIndexesSlots[i] = nullptr; }
	}
	SceneVertices = nullptr;
	SceneIndexes = nullptr;
	if (UploadData) { UploadBuffer->Unmap(); UploadData = nullptr; }
}

void BufferManager::SetCurrentSlot(int slot)
{
	SceneVertexBuffer = SceneVertexBuffers[slot].get();
	SceneIndexBuffer = SceneIndexBuffers[slot].get();
	SceneVertices = SceneVerticesSlots[slot];
	SceneIndexes = SceneIndexesSlots[slot];
}

void BufferManager::CreateSceneVertexBuffers()
{
	size_t size = sizeof(SceneVertex) * SceneVertexBufferSize;

	for (int i = 0; i < NumSlots; i++)
	{
		SceneVertexBuffers[i] = BufferBuilder()
			.Usage(
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VMA_MEMORY_USAGE_UNKNOWN, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT)
			.MemoryType(
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			.Size(size)
			.DebugName("SceneVertexBuffer")
			.Create(renderer->Device.get());

		SceneVerticesSlots[i] = (SceneVertex*)SceneVertexBuffers[i]->Map(0, size);
	}
}

void BufferManager::CreateSceneIndexBuffers()
{
	size_t size = sizeof(uint32_t) * SceneIndexBufferSize;

	for (int i = 0; i < NumSlots; i++)
	{
		SceneIndexBuffers[i] = BufferBuilder()
			.Usage(
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				VMA_MEMORY_USAGE_UNKNOWN, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT)
			.MemoryType(
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			.Size(size)
			.DebugName("SceneIndexBuffer")
			.Create(renderer->Device.get());

		SceneIndexesSlots[i] = (uint32_t*)SceneIndexBuffers[i]->Map(0, size);
	}
}

void BufferManager::CreateUploadBuffer()
{
	UploadBuffer = BufferBuilder()
		.Usage(
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_UNKNOWN, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT)
		.MemoryType(
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		.Size(UploadBufferSize)
		.DebugName("UploadBuffer")
		.Create(renderer->Device.get());

	UploadData = (uint8_t*)UploadBuffer->Map(0, UploadBufferSize);
}
