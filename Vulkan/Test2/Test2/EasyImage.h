#ifndef _EASYIMAGE_
#define _EASYIMAGE_

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <cstdint>
#include <stdexcept>

#include "NonDependingFunktions.h"

class EasyImage {
private:
	int32_t width = -1;
	int32_t height = -1;
	int32_t chanels = -1;
	stbi_uc* pixels = nullptr;

	bool isUploaded = false;

	VkImage image = nullptr;
	VkDeviceMemory imageMem = nullptr;
	VkImageView imageView = nullptr;
	VkImageLayout imageLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	VkSampler imageSampler = nullptr;

	VkDevice device;

	void changeLayount(VkDevice &device, VkCommandPool &commandPool, VkQueue &queue, VkImageLayout layout) {
		
		changeImageLayout(device, commandPool, queue, image, VK_FORMAT_R8G8B8A8_UNORM,imageLayout, layout);

		imageLayout = layout;
	}

	void writeBufferToImmage(VkDevice& device, VkCommandPool& commandPool, VkQueue& queue, VkBuffer &buffer) {
		auto cBuffer = startRecordingSingleTimeBuffer(device, commandPool);

		VkBufferImageCopy BIC;
		BIC.bufferOffset = 0;
		BIC.bufferRowLength = 0;
		BIC.bufferImageHeight = 0;
		BIC.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		BIC.imageSubresource.mipLevel = 0;
		BIC.imageSubresource.baseArrayLayer = 0;
		BIC.imageSubresource.layerCount = 1;
		BIC.imageOffset = { 0,0,0 };
		BIC.imageExtent = { (uint32_t)width, (uint32_t)height, 1 };

		vkCmdCopyBufferToImage(cBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &BIC);

		endRecordingSingleTimeBuffer(device, queue, commandPool, cBuffer);
	}

public:
	EasyImage(const char* path) { load(path); }
	EasyImage() {}
	~EasyImage() {unload();}
	EasyImage(const EasyImage&) = delete;
	EasyImage(EasyImage&&) = delete;
	EasyImage& operator=(const EasyImage&) = delete;
	EasyImage& operator=(EasyImage&&) = delete;

	void load(const char* path) {
		pixels = stbi_load(path, &width, &height, &chanels, STBI_rgb_alpha);	//load and force alpha
		if (pixels == nullptr)
			throw std::invalid_argument("Could not loadImage");
	}
	void upload(VkDevice &device, VkPhysicalDevice &physikalDevice, VkCommandPool &commandPool, VkQueue &queue) {
		if (isUploaded)
			throw std::logic_error("is already uploaded");
		if(pixels == nullptr)
			throw std::logic_error("image empty");

		this->device = device;

		VkDeviceSize iSize = getSize();

		VkBuffer sBuffer;
		VkDeviceMemory sBufferMem;

		createBuffer(device, physikalDevice, iSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, sBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sBufferMem);

		void* data;
		vkMapMemory(device, sBufferMem, 0, iSize, 0, &data);
		memcpy(data, getRaw(), iSize);
		vkUnmapMemory(device, sBufferMem);

		createImage(device, physikalDevice, width, height, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_FORMAT_R8G8B8A8_UNORM, image, imageMem);

		changeLayount(device,commandPool,queue,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		writeBufferToImmage(device, commandPool, queue, sBuffer);
		changeLayount(device, commandPool, queue, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(device, sBuffer, nullptr);
		vkFreeMemory(device, sBufferMem, nullptr);

		createImageView(device, image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, imageView);

		VkSamplerCreateInfo samplerInfo;
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.pNext = nullptr;
		samplerInfo.flags = 0;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = 16;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;

		testErrorCode(vkCreateSampler(device, &samplerInfo, nullptr, &imageSampler));

		isUploaded = true;
	}
	void unload() {
		if (pixels != nullptr) {
			if (isUploaded) {
				vkDestroySampler(device, imageSampler, nullptr);
				vkDestroyImageView(device, imageView, nullptr);
				vkDestroyImage(device, image, nullptr);
				vkFreeMemory(device, imageMem, nullptr);
				isUploaded = false;
			}
			stbi_image_free(pixels);
			pixels = nullptr;
		}
	}

	int32_t getWidth() {
		if (pixels == nullptr)
			throw std::logic_error("image empty");
		return width;
	}
	int32_t getHeight() {
		if (pixels == nullptr)
			throw std::logic_error("image empty");
		return height;
	}
	int32_t getChanels() {
		if (pixels == nullptr)
			throw std::logic_error("image empty");
		return STBI_rgb_alpha;
	}
	uint32_t getSize() {
		if (pixels == nullptr)
			throw std::logic_error("image empty");
		return width * height * STBI_rgb_alpha;
	}
	stbi_uc* getRaw() {
		return pixels;
	}

	VkSampler getSampler() {
		if (!isUploaded)
			throw std::logic_error("is not uploaded");
		if (pixels == nullptr)
			throw std::logic_error("image empty");
		return imageSampler;
	}
	VkImageView getImageView() {
		if (!isUploaded)
			throw std::logic_error("is not uploaded");
		if (pixels == nullptr)
			throw std::logic_error("image empty");
		return imageView;
	}
};
#endif