#ifndef _RENDERER_
#define _RENDERER_

#include <iostream>
#include <fstream>
#include <vector>

#include "DepthImage.h"
#include "EasyImage.h"
#include "Vertex.h"
#include "Mesh.h"
#include "pipeline.h"
#include "NonDependingFunktions.h"
#include "MeshHelper.h"

struct Window
{
	GLFWwindow* window = nullptr;
	uint32_t width = -1;
	uint32_t height = -1;
	uint32_t minSwapchainImageCount = 2;
	uint32_t swapchainImageCount = -1;
	VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
	VkColorSpaceKHR imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_MAILBOX_KHR;

	VkSurfaceKHR surface = VK_NULL_HANDLE;
	std::vector<VkImageView> view;
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;

	VkCommandBuffer* commandBuffers;
	VkSemaphore semaphoreImgAvailable;
	VkSemaphore semaphoreImgRendered;

	VkDevice device;
	VkAllocationCallbacks* allucator;

	void addPipeline() {

	}

	void createSwapchain(VkDevice device, VkAllocationCallbacks* allucator) {
		this->device = device;
		this->allucator = allucator;

		VkSwapchainCreateInfoKHR swapChainCreateInfo;
		swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapChainCreateInfo.pNext = nullptr;
		swapChainCreateInfo.flags = 0;
		swapChainCreateInfo.surface = surface;
		swapChainCreateInfo.minImageCount = minSwapchainImageCount;
		swapChainCreateInfo.imageFormat = colorFormat;
		swapChainCreateInfo.imageColorSpace = imageColorSpace;
		swapChainCreateInfo.imageExtent = { width, height };
		swapChainCreateInfo.imageArrayLayers = 1;
		swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapChainCreateInfo.queueFamilyIndexCount = 0;
		swapChainCreateInfo.pQueueFamilyIndices = nullptr;
		swapChainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		swapChainCreateInfo.compositeAlpha = compositeAlpha;
		swapChainCreateInfo.presentMode = presentMode;
		swapChainCreateInfo.clipped = VK_TRUE;
		swapChainCreateInfo.oldSwapchain = swapchain;

		testErrorCode(vkCreateSwapchainKHR(device, &swapChainCreateInfo, allucator, &swapchain));

		testErrorCode(vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr));

		VkImage* swapchainImages = new VkImage[swapchainImageCount];
		testErrorCode(vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages));

		for (int i = 0; i < swapchainImageCount; i++) {
			createImageView(device, swapchainImages[i], colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, view[i]);
		}

		delete[] swapchainImages;
	}
	
	void recreateSwapChain() {
		vkDeviceWaitIdle(device);

		//depthImage.destroy();

		//vkFreeCommandBuffers(device, commandPool, imageViewsCount, commandBuffers);
		//delete[] commandBuffers;
		//for (int i = 0; i < imageViewsCount; i++)
			//vkDestroyFramebuffer(device, framebuffers[i], nullptr);
		//delete[] framebuffers;
		for (int i = 0; i < view.size(); i++)
			vkDestroyImageView(device, view[i], allucator);
		view.clear();

		auto tempSwapchain = swapchain;

		createSwapchain(device,allucator);
		//createDepthImage();
		//createFrameBuffers();
		//createCommandBuffers();
		//recordCommandBuffers();

		vkDestroySwapchainKHR(device, tempSwapchain, allucator);
	}
};

class Renderer 
{
private:
	std::vector<Window> windows;
	std::vector<const char*> layers;
	std::vector<VkPhysicalDevice> allPyhsicalDevices;
	VkPhysicalDevice selectedPhysicalDevice;
	uint32_t queueFamilyIndex = -1;
	uint32_t queueCount = 1;
	std::vector<RenderPass> renderPasses;

	VkInstance instance = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	VkQueue queue = VK_NULL_HANDLE;
	VkCommandPool commandPool = VK_NULL_HANDLE;

	const char* appName = "defaultName";

	VkAllocationCallbacks* allucator = nullptr;

	typedef std::function<std::vector<VkPhysicalDevice>(std::vector<VkPhysicalDevice> physikalDevices)> physicalDeviceSelectionFunktion;

	physicalDeviceSelectionFunktion externSelectionFunction = nullptr;

	void createInstance()
	{
		VkApplicationInfo appInfo;
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pNext = nullptr;
		appInfo.pApplicationName = appName;
		appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
		appInfo.pEngineName = "friendly-octo-pancake-Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
		appInfo.apiVersion = VK_API_VERSION_1_2;

		uint32_t ammountOfGLFWExtensions;

		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&ammountOfGLFWExtensions);

		VkInstanceCreateInfo instanceInfo;
		instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceInfo.pNext = nullptr;
		instanceInfo.flags = 0;
		instanceInfo.pApplicationInfo = &appInfo;
		instanceInfo.enabledLayerCount = layers.size();
		instanceInfo.ppEnabledLayerNames = layers.data();
		instanceInfo.enabledExtensionCount = ammountOfGLFWExtensions;
		instanceInfo.ppEnabledExtensionNames = glfwExtensions;

		testErrorCode(vkCreateInstance(&instanceInfo, allucator, &instance));	//erstelen + fehler break //nullptr ist DefaultAucator
	};

	void createSurfaces() {
		for (int i = 0; i < windows.size(); i++) {
			windows[i].surface = VkSurfaceKHR();
			testErrorCode(glfwCreateWindowSurface(instance, windows[i].window, allucator, &(windows[i].surface)));
		}
	}

	void createDevice() {
		std::vector<float> queuePrios;
		for (int i = 0; i < queueCount; i++)
			queuePrios.push_back(1);

		VkDeviceQueueCreateInfo deviceQueCreateInfo;
		deviceQueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		deviceQueCreateInfo.pNext = nullptr;
		deviceQueCreateInfo.flags = 0;
		queueFamilyIndex = selectQueueFamily();
		deviceQueCreateInfo.queueFamilyIndex = queueFamilyIndex;
		deviceQueCreateInfo.queueCount = queueCount;
		deviceQueCreateInfo.pQueuePriorities = queuePrios.data();

		VkPhysicalDeviceFeatures usedFeatures = {};
		usedFeatures.samplerAnisotropy = VK_TRUE;
		usedFeatures.fillModeNonSolid = VK_TRUE;

		const std::vector<const char*> deviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		VkDeviceCreateInfo deviceCreateInfo;
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pNext = nullptr;
		deviceCreateInfo.flags = 0;
		deviceCreateInfo.queueCreateInfoCount = 1;						//anzahl für mehrere unterschiedliche ques
		deviceCreateInfo.pQueueCreateInfos = &deviceQueCreateInfo;		//die ques als array
		deviceCreateInfo.enabledLayerCount = 0;
		deviceCreateInfo.ppEnabledLayerNames = nullptr;
		deviceCreateInfo.enabledExtensionCount = deviceExtensions.size();
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
		deviceCreateInfo.pEnabledFeatures = &usedFeatures;

		testErrorCode(vkCreateDevice(selectedPhysicalDevice, &deviceCreateInfo, allucator, &device));
	}

	void initQueue() {
		vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);
	}

	void createCommandPool() {
		VkCommandPoolCreateInfo commandPollCreateInfo;
		commandPollCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPollCreateInfo.pNext = nullptr;
		commandPollCreateInfo.flags = 0;
		commandPollCreateInfo.queueFamilyIndex = queueFamilyIndex;

		testErrorCode(vkCreateCommandPool(device, &commandPollCreateInfo, allucator, &commandPool));
	}

	void createSwapchains() {
		for (int i = 0; i < windows.size(); i++)
			windows[i].createSwapchain(device, allucator);
	}

	uint32_t selectQueueFamily() {
		uint32_t amountOfQueFamelies;
		vkGetPhysicalDeviceQueueFamilyProperties(selectedPhysicalDevice, &amountOfQueFamelies, nullptr);

		VkQueueFamilyProperties* familyProperties = new VkQueueFamilyProperties[amountOfQueFamelies];
		vkGetPhysicalDeviceQueueFamilyProperties(selectedPhysicalDevice, &amountOfQueFamelies, familyProperties);

		std::cout << "quefamilieCount: " << amountOfQueFamelies << std::endl;

		for (int i = 0; i < amountOfQueFamelies; i++)
			if ((familyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 && (familyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) != 0 && queueCount <= familyProperties[i].queueCount)
				return i;

		throw std::logic_error("es wurde keine queue family ausgewählt");
	}

	bool doesPhysicalDeviceSupportPresentMode(const VkPhysicalDevice physicalDevice) const {
		uint32_t presentationModeCount;
		for (int i = 0; i < windows.size(); i++)
		{
			testErrorCode(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, windows[i].surface, &presentationModeCount, nullptr));

			VkPresentModeKHR* presentationModes = new VkPresentModeKHR[presentationModeCount];
			testErrorCode(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, windows[i].surface, &presentationModeCount, presentationModes));

			bool ok = false;

			for (int j = 0; j < presentationModeCount; j++) {
				if (presentationModes[j] == windows[i].presentMode) {
					ok = true;
					break;
				}
			}
			delete[] presentationModes;

			if (!ok)
				return false;
		}
		return true;
	}

	bool doesPhysicalDeviceSupportColorFormat(const VkPhysicalDevice physicalDevice) const {

		uint32_t colorFormatCount;
		for (int i = 0; i < windows.size(); i++)
		{
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, windows[i].surface, &colorFormatCount, nullptr);

			VkSurfaceFormatKHR* surfaceFormats = new VkSurfaceFormatKHR[colorFormatCount];
			testErrorCode(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, windows[i].surface, &colorFormatCount, surfaceFormats));

			bool ok = false;

			for (int j = 0; j < colorFormatCount; j++) {
				if (surfaceFormats[j].format == windows[i].colorFormat && surfaceFormats[i].colorSpace == windows[i].imageColorSpace) {
					ok = true;
					break;
				}
			}
			delete[] surfaceFormats;

			if (!ok)
				return false;
		}
		return true;
	}

	bool doesPhysicalDeviceSupportSwapchain(const VkPhysicalDevice physicalDevice) const {
		VkBool32 supportsSwapchain;
		for (int i = 0; i < windows.size(); i++)
		{
			testErrorCode(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, 0, windows[i].surface, &supportsSwapchain));
			if (!supportsSwapchain)
				return false;
		}
		return true;
	}

	std::vector<VkPhysicalDevice> internSelectionFunction(const std::vector<VkPhysicalDevice> physicalDevices) const {
		std::vector<VkPhysicalDevice> toRet;
		for (int i = 0; i < physicalDevices.size(); i++)
			if (doesPhysicalDeviceSupportSwapchain(physicalDevices[i]) && doesPhysicalDeviceSupportColorFormat(physicalDevices[i]))
				toRet.push_back(physicalDevices[i]);
		return toRet;
	}

	static std::vector<VkPhysicalDevice> getAllPhysicalDevices(const VkInstance& instance) {
		uint32_t physikalDeviceCount;
		testErrorCode(vkEnumeratePhysicalDevices(instance, &physikalDeviceCount, nullptr));

		std::vector<VkPhysicalDevice> physikalDevices;
		physikalDevices.resize(physikalDeviceCount);
		testErrorCode(vkEnumeratePhysicalDevices(instance, &physikalDeviceCount, physikalDevices.data()));

		return physikalDevices;
	}

public:
	Renderer() {};
	~Renderer() {};
	Renderer(const Pipeline&) = delete;
	Renderer(Pipeline&&) = delete;
	Renderer& operator=(const Pipeline&) = delete;
	Renderer& operator=(Pipeline&&) = delete;

	int addWindow(GLFWwindow* window, uint32_t width, uint32_t height) {
		uint32_t index = windows.size();
		windows.push_back(Window());
		windows[index].width = width;
		windows[index].height = height;
		windows[index].window = window;

		return index;
	}

	void addLayer(const char* layer) {
		layers.push_back(layer);
	}

	void setAppName(const char* name = "defaultName") {
		appName = name;
	}

	void setDefaultAllucator(VkAllocationCallbacks* allucator = nullptr) {
		this->allucator = allucator;
	}

	void setExternPhysicalDeviceRatingFunction(physicalDeviceSelectionFunktion externSelectionFunction = nullptr) {
		this->externSelectionFunction = externSelectionFunction;
	}

	void setQueueCount(uint32_t queueCount = 4) {
		this->queueCount = queueCount;
	}

	void setMinSwapchainImageCount(uint32_t windowIndex, uint32_t minSwapchainImageCount = 2) {
		windows[windowIndex].minSwapchainImageCount = minSwapchainImageCount;
	}

	void setColorFormat(uint32_t windowIndex, VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM, VkColorSpaceKHR imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
		windows[windowIndex].colorFormat = colorFormat;
		windows[windowIndex].imageColorSpace = imageColorSpace;
	}

	void setCompositAlpha(uint32_t windowIndex, VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) {
		windows[windowIndex].compositeAlpha = compositeAlpha;
	}

	void setPresentMode(uint32_t windowIndex, VkPresentModeKHR presentMode = VK_PRESENT_MODE_MAILBOX_KHR) {
		windows[windowIndex].presentMode = presentMode;
	}

	uint32_t addRenderPass(VkFormat colorFormat) {
		uint32_t index = renderPasses.size();
		renderPasses.push_back(RenderPass());
		renderPasses[index].init(colorFormat);
		return index;
	}

	void Start() {
		createInstance();
		createSurfaces();
		allPyhsicalDevices = internSelectionFunction(getAllPhysicalDevices(instance));
		if (externSelectionFunction != nullptr)
			allPyhsicalDevices = externSelectionFunction(allPyhsicalDevices);
		selectedPhysicalDevice = allPyhsicalDevices[0];
		createDevice();
		initQueue(); 
		createCommandPool();
		createSwapchains();
		for (int i = 0; i < renderPasses.size(); i++)
			renderPasses[i].create(device, selectedPhysicalDevice);
	}

	void Update() {

	}

};

#endif