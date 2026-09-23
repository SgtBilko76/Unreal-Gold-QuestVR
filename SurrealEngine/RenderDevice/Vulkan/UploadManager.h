#pragma once

#include "TextureUploader.h"
#include <unordered_map>

class VulkanRenderDevice;
class CachedTexture;
struct TextureInfo;

class UploadManager
{
public:
	UploadManager(VulkanRenderDevice* renderer);
	~UploadManager();

	bool SupportsTextureFormat(TextureFormat Format) const;

	void UploadTexture(CachedTexture* tex, const TextureInfo& Info, bool masked);
	void UploadTextureRect(CachedTexture* tex, const TextureInfo& Info, int x, int y, int w, int h);

	void SubmitUploads();

	// Rewinds the staging buffer. Only CommandBufferManager calls this, once it knows no
	// pending submission can still be copying out of the buffer (see its class comment) -
	// SubmitUploads() itself no longer rewinds, since with in-flight submissions the copies it
	// records haven't executed yet when it returns.
	void ResetUploadBufferPos() { UploadBufferPos = 0; }

	void ClearCache();

private:
	void UploadData(CachedTexture* tex, const TextureInfo& Info, bool masked, TextureUploader* uploader);
	void UploadWhite(CachedTexture* tex);
	void WaitIfUploadBufferIsFull(int bytes);
	void AddPendingUpload(CachedTexture* tex, const VkBufferImageCopy& region, bool isPartial);

	VulkanRenderDevice* renderer = nullptr;

	int UploadBufferPos = 0;
	std::vector<CachedTexture*> PendingUploads;
};
