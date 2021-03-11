#pragma once

#define testErrorCode(val)\
	if(val != VK_SUCCESS) __debugbreak();

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "GLFW/glfw3.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


std::vector<char> readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::binary | std::ios::ate);

	if (file) {
		size_t fSize = file.tellg();
		std::vector<char> fileBuffer(fSize);
		file.seekg(0);
		file.read(fileBuffer.data(), fileBuffer.size());
		file.close();
		return fileBuffer;
	}
	else {
		throw std::runtime_error("Failed To Open File");
	}
}

uint32_t findMemoryTypeIndex(VkPhysicalDevice& physikalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties pDMemProp;
	vkGetPhysicalDeviceMemoryProperties(physikalDevice, &pDMemProp);

	for (uint32_t i = 0; i < pDMemProp.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (pDMemProp.memoryTypes[i].propertyFlags & properties) == properties)
			return i;
	}

	throw std::runtime_error("mem type not found");
}

VkBufferCreateInfo createBufferCreateInfo() {
	VkBufferCreateInfo bufferInfo;
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.pNext = nullptr;
	bufferInfo.flags = 0;
	bufferInfo.queueFamilyIndexCount = 0;
	bufferInfo.pQueueFamilyIndices = nullptr;

	return bufferInfo;
}

void createBuffer(VkDevice &device, VkPhysicalDevice &physikalDevice, VkDeviceSize deviceSize, VkBufferUsageFlags usageFlags, VkBuffer& buffer, VkMemoryPropertyFlags memPropFlags, VkDeviceMemory& deviceMem) {
	VkBufferCreateInfo bufferInfo = createBufferCreateInfo();
	bufferInfo.size = deviceSize;
	bufferInfo.usage = usageFlags;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	testErrorCode(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer));

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	VkMemoryAllocateInfo memAlkinfo;
	memAlkinfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAlkinfo.pNext = nullptr;
	memAlkinfo.allocationSize = memRequirements.size;
	memAlkinfo.memoryTypeIndex = findMemoryTypeIndex(physikalDevice, memRequirements.memoryTypeBits, memPropFlags);

	testErrorCode(vkAllocateMemory(device, &memAlkinfo, nullptr, &deviceMem));

	testErrorCode(vkBindBufferMemory(device, buffer, deviceMem, 0));
}

void removeBuffer(VkDevice &device, VkBuffer& buffer, VkDeviceMemory& deviceMem) {
	vkFreeMemory(device, deviceMem, nullptr);
	vkDestroyBuffer(device, buffer, nullptr);
}

VkCommandBuffer startRecordingSingleTimeBuffer(VkDevice& device, VkCommandPool& commandPool) {
	VkCommandBufferAllocateInfo cBufferAlInfo;
	cBufferAlInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cBufferAlInfo.pNext = nullptr;
	cBufferAlInfo.commandPool = commandPool;
	cBufferAlInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cBufferAlInfo.commandBufferCount = 1;

	VkCommandBuffer cBuffer;
	testErrorCode(vkAllocateCommandBuffers(device, &cBufferAlInfo, &cBuffer));

	VkCommandBufferBeginInfo cBufferBeginInfo;
	cBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cBufferBeginInfo.pNext = nullptr;
	cBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	cBufferBeginInfo.pInheritanceInfo = nullptr;

	testErrorCode(vkBeginCommandBuffer(cBuffer, &cBufferBeginInfo));
	return cBuffer;
}

void endRecordingSingleTimeBuffer(VkDevice& device, VkQueue& queue, VkCommandPool& commandPool, VkCommandBuffer buffer) {
	testErrorCode(vkEndCommandBuffer(buffer));

	VkSubmitInfo submitInfo;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &buffer;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr;

	VkResult temp = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);

	testErrorCode(temp);

	testErrorCode(vkQueueWaitIdle(queue));

	vkFreeCommandBuffers(device, commandPool, 1, &buffer);
}

void copyBuffer(VkDevice &device, VkQueue &queue, VkCommandPool &commandPool, VkBuffer src, VkBuffer dest, VkDeviceSize size) {
	
	auto cBuffer = startRecordingSingleTimeBuffer(device, commandPool);

	VkBufferCopy cp;
	cp.srcOffset = 0;
	cp.dstOffset = 0;
	cp.size = size;

	vkCmdCopyBuffer(cBuffer, src, dest, 1, &cp);

	endRecordingSingleTimeBuffer(device, queue, commandPool, cBuffer);
}

template<typename T>
void createBufferFromArrayToGraca(VkDevice& device, VkQueue& queue, VkPhysicalDevice& physikalDevice, VkCommandPool& commandPool, std::vector<T>& data, VkBufferUsageFlags usage, VkBuffer& buffer, VkDeviceMemory& bufferMem) {
	VkDeviceSize bSize = sizeof(T) * data.size();

	VkBuffer sBuffer;
	VkDeviceMemory sBufferMem;
	createBuffer(device, physikalDevice, bSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, sBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sBufferMem);


	void* rawData;
	vkMapMemory(device, sBufferMem, 0, bSize, 0, &rawData);
	memcpy(rawData, data.data(), bSize);
	vkUnmapMemory(device, sBufferMem);

	createBuffer(device, physikalDevice, bSize, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, bufferMem);

	copyBuffer(device, queue, commandPool, sBuffer, buffer, bSize);

	removeBuffer(device, sBuffer, sBufferMem);
}

void createBufferFromArrayToGraca(VkDevice& device, VkQueue& queue, VkPhysicalDevice& physikalDevice, VkCommandPool& commandPool, uint32_t *data, uint64_t dataSize, VkBufferUsageFlags usage, VkBuffer& buffer, VkDeviceMemory& bufferMem) {
	VkBuffer sBuffer;
	VkDeviceMemory sBufferMem;
	createBuffer(device, physikalDevice, dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, sBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sBufferMem);

	void* rawData;
	vkMapMemory(device, sBufferMem, 0, dataSize, 0, &rawData);
	memcpy(rawData, data, dataSize);
	vkUnmapMemory(device, sBufferMem);

	createBuffer(device, physikalDevice, dataSize, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, bufferMem);

	copyBuffer(device, queue, commandPool, sBuffer, buffer, dataSize);

	removeBuffer(device, sBuffer, sBufferMem);
}

std::vector<VkPhysicalDevice> getAllPhysikalDevices(VkInstance &instance) {
	uint32_t physikalDeviceCount;
	testErrorCode(vkEnumeratePhysicalDevices(instance, &physikalDeviceCount, nullptr));	//anzahl der grakas + fehler break

	std::vector<VkPhysicalDevice> physikalDevices;
	physikalDevices.resize(physikalDeviceCount);
	testErrorCode(vkEnumeratePhysicalDevices(instance, &physikalDeviceCount, physikalDevices.data()));	//alle grakas holen + fehler break

	return physikalDevices;
}

void printStats(VkPhysicalDevice& device, VkSurfaceKHR surface) {
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(device, &properties);

	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceFeatures(device, &features);

	VkPhysicalDeviceMemoryProperties memProp;
	vkGetPhysicalDeviceMemoryProperties(device, &memProp);

	uint32_t amountOfQueFamelies;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &amountOfQueFamelies, nullptr);

	VkQueueFamilyProperties* familyProperties = new VkQueueFamilyProperties[amountOfQueFamelies];
	vkGetPhysicalDeviceQueueFamilyProperties(device, &amountOfQueFamelies, familyProperties);		//muss aufgerufen werden um die que family zu verwenden

	VkSurfaceCapabilitiesKHR surfaceCapabilitiers;
	testErrorCode(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &surfaceCapabilitiers));

	uint32_t colorFormatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &colorFormatCount, nullptr);

	VkSurfaceFormatKHR* surfaceFormats = new VkSurfaceFormatKHR[colorFormatCount];
	testErrorCode(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &colorFormatCount, surfaceFormats));

	uint32_t presentationModeCount;
	testErrorCode(vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationModeCount, nullptr));

	VkPresentModeKHR* presentationModes = new VkPresentModeKHR[presentationModeCount];
	testErrorCode(vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationModeCount, presentationModes));

	std::cout << "Name:            " << properties.deviceName << std::endl;
	std::cout << "ApiVersion:      " << VK_VERSION_MAJOR(properties.apiVersion) << "." << VK_VERSION_MINOR(properties.apiVersion) << "." << VK_VERSION_PATCH(properties.apiVersion) << std::endl;
	std::cout << "Treiber Version: " << properties.driverVersion << std::endl;
	std::cout << "Vendor ID:       " << properties.vendorID << std::endl;
	std::cout << "Device ID:       " << properties.deviceID << std::endl;
	std::cout << "Device Type:     " << properties.deviceType << std::endl;
	std::cout << "descrete Priorities: " << properties.limits.discreteQueuePriorities << std::endl;
	std::cout << std::endl;


	std::cout << "geometry Shader: " << features.geometryShader << std::endl;
	std::cout << std::endl;

	for (int i = 0; i < memProp.memoryHeapCount; i++)
		std::cout << "Heap             " << i << " size:      " << memProp.memoryHeaps[i].size << std::endl;
	for (int i = 0; i < memProp.memoryTypeCount; i++)
		std::cout << "Type             " << i << " heapIndex: " << memProp.memoryTypes[i].heapIndex << " Flag: " << memProp.memoryTypes[i].propertyFlags << std::endl;
	std::cout << std::endl;

	std::cout << "quefamilieCount: " << amountOfQueFamelies << std::endl;

	for (int i = 0; i < amountOfQueFamelies; i++) {
		std::cout << "queFamily " << i << std::endl;
		std::cout << "Que Graphics Bit " << ((familyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 ? "true" : "false") << std::endl;
		std::cout << "Que Compute Bit " << ((familyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0 ? "true" : "false") << std::endl;
		std::cout << "Que Transfer Bit " << ((familyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) != 0 ? "true" : "false") << std::endl;
		std::cout << "Que SparceBinding Bit " << ((familyProperties[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) != 0 ? "true" : "false") << std::endl;
		std::cout << "Que Count: " << familyProperties[i].queueCount << std::endl;
		std::cout << "Que TimeStamp vallide: " << familyProperties[i].timestampValidBits << std::endl;
		std::cout << "Min Image Timastamp Granularyty: width: " << familyProperties[i].minImageTransferGranularity.width << " height: " << familyProperties[i].minImageTransferGranularity.height << " depth: " << familyProperties[i].minImageTransferGranularity.depth << std::endl << std::endl;
	}

	std::cout << "SurfaceCapabilities: " << std::endl;
	std::cout << "\tminImgCount: " << surfaceCapabilitiers.minImageCount << std::endl;
	std::cout << "\tmaxImgCount: " << surfaceCapabilitiers.maxImageCount << std::endl;
	std::cout << "\tcurrent image Extend: " << surfaceCapabilitiers.currentExtent.width << "/" << surfaceCapabilitiers.currentExtent.height << std::endl;
	std::cout << "\tmin image Extend: " << surfaceCapabilitiers.minImageExtent.width << "/" << surfaceCapabilitiers.minImageExtent.height << std::endl;
	std::cout << "\tmax image Extend: " << surfaceCapabilitiers.maxImageExtent.width << "/" << surfaceCapabilitiers.maxImageExtent.height << std::endl;
	std::cout << "\tmaxArrayLayers: " << surfaceCapabilitiers.maxImageArrayLayers << std::endl;
	std::cout << "\tsuportedTransform: " << surfaceCapabilitiers.supportedTransforms << std::endl;
	std::cout << "\tcurrentTransform: " << surfaceCapabilitiers.currentTransform << std::endl;
	std::cout << "\tcomposit alpha: " << surfaceCapabilitiers.supportedCompositeAlpha << std::endl;
	std::cout << "\tsoportedFlags: " << surfaceCapabilitiers.supportedUsageFlags << std::endl;


	std::cout << "Anzahl der farbFormate: " << colorFormatCount << std::endl;
	for (int i = 0; i < colorFormatCount; i++) {
		std::cout << surfaceFormats[i].format << std::endl;
	}

	std::cout << "amount of presentation Modes: " << presentationModeCount << std::endl;
	for (int i = 0; i < presentationModeCount; i++) {
		std::cout << "mode: " << presentationModes[i] << std::endl;
	}


	delete[] presentationModes;
	delete[] surfaceFormats;
	delete[] familyProperties;
}

void printExtensions() {
	uint32_t ammountOfLayers;
	testErrorCode(vkEnumerateInstanceLayerProperties(&ammountOfLayers, nullptr));

	VkLayerProperties* layerProperties = new VkLayerProperties[ammountOfLayers];
	testErrorCode(vkEnumerateInstanceLayerProperties(&ammountOfLayers, layerProperties));

	std::cout << "ammount of instance Layers: " << ammountOfLayers << std::endl;

	for (int i = 0; i < ammountOfLayers; i++) {
		std::cout << "layer        " << i << std::endl;
		std::cout << "Name:        " << layerProperties[i].layerName << std::endl;
		std::cout << "SpecVersion: " << VK_VERSION_MAJOR(layerProperties[i].specVersion) << "." << VK_VERSION_MINOR(layerProperties[i].specVersion) << "." << VK_VERSION_PATCH(layerProperties[i].specVersion) << std::endl;
		std::cout << "ImplVersion: " << VK_VERSION_MAJOR(layerProperties[i].implementationVersion) << "." << VK_VERSION_MINOR(layerProperties[i].implementationVersion) << "." << VK_VERSION_PATCH(layerProperties[i].implementationVersion) << std::endl;
		std::cout << "Description: " << layerProperties[i].description << std::endl;

		uint32_t ammountOfExtensions;
		testErrorCode(vkEnumerateInstanceExtensionProperties(layerProperties[i].layerName, &ammountOfExtensions, nullptr));

		VkExtensionProperties* extensionsProperties;
		extensionsProperties = new VkExtensionProperties[ammountOfExtensions];
		testErrorCode(vkEnumerateInstanceExtensionProperties(layerProperties[i].layerName, &ammountOfExtensions, extensionsProperties));

		std::cout << "ammount of instance Extensions: " << ammountOfExtensions << std::endl;
		for (int i = 0; i < ammountOfExtensions; i++) {
			std::cout << "Name:        " << extensionsProperties[i].extensionName << std::endl;
			std::cout << "SpecVersion: " << VK_VERSION_MAJOR(extensionsProperties[i].specVersion) << "." << VK_VERSION_MINOR(extensionsProperties[i].specVersion) << "." << VK_VERSION_PATCH(extensionsProperties[i].specVersion) << std::endl;
			std::cout << std::endl;
		}

		std::cout << std::endl;
		delete[] extensionsProperties;
	}

	uint32_t ammountOfExtensions;
	testErrorCode(vkEnumerateInstanceExtensionProperties(nullptr, &ammountOfExtensions, nullptr));

	VkExtensionProperties* extensionsProperties;
	extensionsProperties = new VkExtensionProperties[ammountOfExtensions];
	testErrorCode(vkEnumerateInstanceExtensionProperties(nullptr, &ammountOfExtensions, extensionsProperties));

	std::cout << "ammount of instance Extensions: " << ammountOfExtensions << std::endl;
	for (int i = 0; i < ammountOfExtensions; i++) {
		std::cout << "Name:        " << extensionsProperties[i].extensionName << std::endl;
		std::cout << "SpecVersion: " << VK_VERSION_MAJOR(extensionsProperties[i].specVersion) << "." << VK_VERSION_MINOR(extensionsProperties[i].specVersion) << "." << VK_VERSION_PATCH(extensionsProperties[i].specVersion) << std::endl;
		std::cout << std::endl;
	}

	std::cout << std::endl;


	delete[] extensionsProperties;
	delete[] layerProperties;
}

void printAllPhysikalDevizes(VkInstance instance, VkSurfaceKHR surface) {
	auto physikalDevices = getAllPhysikalDevices(instance);

	for (uint32_t i = 0; i < physikalDevices.size(); i++)
		printStats(physikalDevices[i], surface);
}

bool isStencilFormate(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void createImage(VkDevice& device, VkPhysicalDevice& physikalDevice, uint32_t width, uint32_t height, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlagBits properties, VkFormat format, VkImage &image, VkDeviceMemory &imageMemory) {

	VkImageCreateInfo iCreateInfo;
	iCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	iCreateInfo.pNext = nullptr;
	iCreateInfo.flags = 0;
	iCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	iCreateInfo.format = format;
	iCreateInfo.extent.width = width;
	iCreateInfo.extent.height = height;
	iCreateInfo.extent.depth = 1;
	iCreateInfo.mipLevels = 1;
	iCreateInfo.arrayLayers = 1;
	iCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	iCreateInfo.tiling = tiling;
	iCreateInfo.usage = usage;
	iCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	iCreateInfo.queueFamilyIndexCount = 0;
	iCreateInfo.pQueueFamilyIndices = nullptr;
	iCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

	testErrorCode(vkCreateImage(device, &iCreateInfo, nullptr, &image));

	VkMemoryRequirements memReq;
	vkGetImageMemoryRequirements(device, image, &memReq);

	VkMemoryAllocateInfo memAlocInfo;
	memAlocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAlocInfo.pNext = nullptr;
	memAlocInfo.allocationSize = memReq.size;
	memAlocInfo.memoryTypeIndex = findMemoryTypeIndex(physikalDevice, memReq.memoryTypeBits, properties);

	testErrorCode(vkAllocateMemory(device, &memAlocInfo, nullptr, &imageMemory));

	vkBindImageMemory(device, image, imageMemory, 0);
}

void createImageView(VkDevice& device, VkImage& image, VkFormat formate, VkImageAspectFlags aspectFlags, VkImageView &imageView) {
	VkImageViewCreateInfo iVCinfo;
	iVCinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	iVCinfo.pNext = nullptr;
	iVCinfo.flags = 0;
	iVCinfo.image = image;
	iVCinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	iVCinfo.format = formate;
	iVCinfo.components.r = VK_COMPONENT_SWIZZLE_R;
	iVCinfo.components.g = VK_COMPONENT_SWIZZLE_G;
	iVCinfo.components.b = VK_COMPONENT_SWIZZLE_B;
	iVCinfo.components.a = VK_COMPONENT_SWIZZLE_A;
	iVCinfo.subresourceRange.aspectMask = aspectFlags;
	iVCinfo.subresourceRange.baseMipLevel = 0;
	iVCinfo.subresourceRange.levelCount = 1;
	iVCinfo.subresourceRange.baseArrayLayer = 0;
	iVCinfo.subresourceRange.layerCount = 1;

	testErrorCode(vkCreateImageView(device, &iVCinfo, nullptr, &imageView));
}

void changeImageLayout(VkDevice& device, VkCommandPool& commandPool, VkQueue& queue, VkImage& image, VkFormat format, VkImageLayout srcLayout, VkImageLayout destImageLayout) {
	auto cBuffer = startRecordingSingleTimeBuffer(device, commandPool);

	VkImageMemoryBarrier barrier;
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.pNext = nullptr;
	if (srcLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && destImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
	{
		barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	else if (srcLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && destImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}
	else if (srcLayout == VK_IMAGE_LAYOUT_UNDEFINED && destImageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}
	else
		throw std::invalid_argument("laoutTransition not jet supported");

	barrier.oldLayout = srcLayout;
	barrier.newLayout = destImageLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	if (destImageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) 
		if (isStencilFormate(format))
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_DEPTH_BIT;
		else 
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	else 
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	vkCmdPipelineBarrier(cBuffer, VK_PIPELINE_STAGE_HOST_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	endRecordingSingleTimeBuffer(device, queue, commandPool, cBuffer);
}

bool isFormatSupported(VkPhysicalDevice physikalDevice, VkFormat format, VkImageTiling tiling, VkFormatFeatureFlags featureFlags) {
	VkFormatProperties fProp;
	vkGetPhysicalDeviceFormatProperties(physikalDevice, format, &fProp);

	if (tiling == VK_IMAGE_TILING_LINEAR && (fProp.linearTilingFeatures & featureFlags) == featureFlags)
		return true;
	else if (tiling == VK_IMAGE_TILING_OPTIMAL && (fProp.optimalTilingFeatures & featureFlags) == featureFlags)
		return true;
	return false;
}

VkFormat findSupportedFormat(VkPhysicalDevice physikalDevice, std::vector<VkFormat> &formats , VkImageTiling tiling, VkFormatFeatureFlags featureFlags) {
	for (VkFormat format : formats) {
		if (isFormatSupported(physikalDevice, format, tiling, featureFlags))
			return format;
	}
	throw std::runtime_error("non of the supportet formats found");
}