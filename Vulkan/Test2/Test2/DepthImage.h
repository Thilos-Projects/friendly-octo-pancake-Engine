#ifndef _DEPTHIMAGE_
#define _DEPTHIMAGE_

#include "NonDependingFunktions.h"

class DepthImage {
private:
	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory imageMem = VK_NULL_HANDLE;
	VkImageView imageView = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;

	static VkFormat findDepthFormat(VkPhysicalDevice physicalDevice) {
		std::vector<VkFormat> possibleFormats = {
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D24_UNORM_S8_UINT,
			VK_FORMAT_D32_SFLOAT
		};

		return findSupportedFormat(physicalDevice, possibleFormats, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}

public:

	DepthImage() {}
	~DepthImage() { destroy(); }
	DepthImage(const DepthImage&) = delete;
	DepthImage(DepthImage&&) = delete;
	DepthImage& operator=(const DepthImage&) = delete;
	DepthImage& operator=(DepthImage&&) = delete;

	void create(VkDevice device, VkPhysicalDevice physikalDevice, VkCommandPool commandPool, VkQueue queue, uint32_t width, uint32_t height) 
	{
		if (image != VK_NULL_HANDLE)
			throw std::logic_error("DepthImage bereits erstellt");

		this->device = device;

		VkFormat depthFormat = findDepthFormat(physikalDevice);

		createImage(device, physikalDevice, width, height, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthFormat, image, imageMem);
		createImageView(device, image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, imageView);

		changeImageLayout(device, commandPool, queue, image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	}
	void destroy() {
		if (image != VK_NULL_HANDLE) {
			vkDestroyImageView(device, imageView, nullptr);
			vkDestroyImage(device, image, nullptr);
			vkFreeMemory(device, imageMem, nullptr);
			image = VK_NULL_HANDLE;
			imageView = VK_NULL_HANDLE;
			imageMem = VK_NULL_HANDLE;
			device = VK_NULL_HANDLE;
		}
	}

	VkImageView getImageView()
	{
		return imageView;
	}

	static VkAttachmentDescription getdepthAttachment(VkPhysicalDevice physicalDevice) {
		VkAttachmentDescription attechmentDesription;
		attechmentDesription.flags = 0;															//speicher überlappung möglich
		attechmentDesription.format = findDepthFormat(physicalDevice);
		attechmentDesription.samples = VK_SAMPLE_COUNT_1_BIT;
		attechmentDesription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;								//was pasiert mit werten nach dem laden
		attechmentDesription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;						//was pasiert mit dem speichen
		attechmentDesription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;					//fog of ware
		attechmentDesription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attechmentDesription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;							//format vor laden
		attechmentDesription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;	//format nach laden

		return attechmentDesription;
	}

	static VkPipelineDepthStencilStateCreateInfo getDepthStencilStateCreateInfoOpaque() {
		VkPipelineDepthStencilStateCreateInfo dssci;
		dssci.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		dssci.pNext = nullptr;
		dssci.flags = 0;
		dssci.depthTestEnable = VK_TRUE;			//tiefe hier ein aus schalten
		dssci.depthWriteEnable = VK_TRUE;			//transparentes nicht in den depth bufer
		dssci.depthCompareOp = VK_COMPARE_OP_LESS;	//was ist forne
		dssci.depthBoundsTestEnable = VK_FALSE;		//nur zeichnen was innerhalb eines bereichs ist
		dssci.stencilTestEnable = VK_FALSE;
		dssci.front = {};
		dssci.back = {};
		dssci.minDepthBounds = 0.0f;				//min Tiefe zum Zeichen
		dssci.maxDepthBounds = 1.0f;				//max Tiefe Zum Zeichene

		return dssci;
	}
};

#endif