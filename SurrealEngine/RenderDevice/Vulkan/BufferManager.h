#pragma once

#include "ShaderManager.h"

class VulkanRenderDevice;
struct SceneVertex;

class BufferManager
{
public:
	BufferManager(VulkanRenderDevice* renderer);
	~BufferManager();

	// One scene vertex/index buffer pair per in-flight submission slot (see
	// CommandBufferManager): the CPU fills these while the GPU may still be reading the
	// previous slot's pair, so they can't be shared. SceneVertexBuffer/SceneIndexBuffer and the
	// mapped pointers always refer to the CURRENT slot's pair - SetCurrentSlot() switches them
	// in step with CommandBufferManager's slot.
	static const int NumSlots = 3; // must match CommandBufferManager::NumInFlight

	void SetCurrentSlot(int slot);

	VulkanBuffer* SceneVertexBuffer = nullptr;
	VulkanBuffer* SceneIndexBuffer = nullptr;
	std::unique_ptr<VulkanBuffer> UploadBuffer;

	SceneVertex* SceneVertices = nullptr;
	uint32_t* SceneIndexes = nullptr;
	uint8_t* UploadData = nullptr;

	static const int SceneVertexBufferSize = 1 * 1024 * 1024;
	static const int SceneIndexBufferSize = 1 * 1024 * 1024;

	static const int UploadBufferSize = 64 * 1024 * 1024;

private:
	void CreateSceneVertexBuffers();
	void CreateSceneIndexBuffers();
	void CreateUploadBuffer();

	VulkanRenderDevice* renderer = nullptr;

	std::unique_ptr<VulkanBuffer> SceneVertexBuffers[NumSlots];
	std::unique_ptr<VulkanBuffer> SceneIndexBuffers[NumSlots];
	SceneVertex* SceneVerticesSlots[NumSlots] = {};
	uint32_t* SceneIndexesSlots[NumSlots] = {};
};
