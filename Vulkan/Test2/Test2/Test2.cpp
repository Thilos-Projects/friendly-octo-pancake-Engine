#include <iostream>
#include <fstream>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include <glm/glm.hpp>

#define testErrorCode(val)\
	if(val != VK_SUCCESS) __debugbreak();

//#define ShowDebug

VkInstance instance;
VkDevice device;
VkSurfaceKHR surface;
VkSwapchainKHR swapchain = VK_NULL_HANDLE; 
uint32_t imageViewsCount;
VkImageView* imageViews;
GLFWwindow *window;
VkShaderModule vertShaderModul;
VkShaderModule fragShaderModul;
VkPipelineLayout pipelineLayout;
VkPipeline pipeline;
VkFramebuffer* framebuffers;
VkRenderPass renderPass;
VkCommandPool commandPool;
VkCommandBuffer* commandBuffers;
VkSemaphore semaphoreImgAvailable;
VkSemaphore semaphoreImgRendered;
VkQueue queue;
VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMem;

uint32_t screenWidth = 300;
uint32_t screenHeight = 300;
const VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM;

std::vector<VkPhysicalDevice> allPyhsikalDevices;

class Vertex {
public:
	Vertex(glm::vec2 pos, glm::vec3 color)
		:pos(pos),color(color){}
	glm::vec2 pos;
	glm::vec3 color;

	static VkVertexInputBindingDescription getBindingDescription(const uint32_t index = 0) {
		VkVertexInputBindingDescription toRet;

		toRet.binding = index;
		toRet.stride = sizeof(Vertex);
		toRet.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return toRet;
	}
	static std::vector<VkVertexInputAttributeDescription> getAttributDescriptions(const uint32_t index = 0) {
		std::vector<VkVertexInputAttributeDescription> toRet(2);

		toRet[0].location = 0;
		toRet[0].binding = index;
		toRet[0].format = VK_FORMAT_R32G32_SFLOAT;
		toRet[0].offset = offsetof(Vertex, pos);

		toRet[1].location = 1;
		toRet[1].binding = index;
		toRet[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		toRet[1].offset = offsetof(Vertex, color);

		return toRet;
	}
};

std::vector<Vertex> verticies = {
	Vertex({ -0.5f, -0.5f},{ 1.0f, 0.0f, 0.0f}),
	Vertex({  0.5f,  0.5f},{ 0.0f, 1.0f, 0.0f}),
	Vertex({ -0.5f,  0.5f},{ 0.0f, 0.0f, 1.0f}),

	Vertex({ -0.5f, -0.5f},{ 1.0f, 0.0f, 0.0f}),
	Vertex({  0.5f, -0.5f},{ 1.0f, 1.0f, 1.0f}),
	Vertex({  0.5f,  0.5f},{ 0.0f, 1.0f, 0.0f})
};

void CreateShaderModul(const std::vector<char> &code, VkShaderModule* shaderModule) {
	VkShaderModuleCreateInfo shaderCreateInfo;
	shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderCreateInfo.pNext = nullptr;
	shaderCreateInfo.flags = 0;
	shaderCreateInfo.codeSize = code.size();
	shaderCreateInfo.pCode = (uint32_t*)code.data();

	testErrorCode(vkCreateShaderModule(device, &shaderCreateInfo, nullptr, shaderModule));
}

std::vector<char> readFile(const std::string & filename) {
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

void printStats(VkPhysicalDevice &device) {
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

void recreateSwapChain();

void onWindowResized(GLFWwindow *window, int width, int height) {

	screenWidth = width;
	screenHeight = height;

	recreateSwapChain();
}

void startGLFW() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);		//später ändern auf rezisable

	window = glfwCreateWindow(screenWidth, screenHeight, "Test", nullptr, nullptr);	//größe ändern
	glfwSetWindowSizeCallback(window, onWindowResized);
}

void createInstance() {
	VkApplicationInfo appInfo;
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = nullptr;
	appInfo.pApplicationName = "VulcanTest";
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
	appInfo.pEngineName = "friendly-octo-pancake-Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.apiVersion = VK_API_VERSION_1_2;

	const std::vector<const char*> layers = {
#ifdef ShowDebug
		"VK_LAYER_KHRONOS_validation"
#endif
	};

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

	testErrorCode(vkCreateInstance(&instanceInfo, nullptr, &instance));	//erstelen + fehler break //nullptr ist DefaultAucator
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

void createGLFWWindowSurface() {
	testErrorCode(glfwCreateWindowSurface(instance, window, nullptr, &surface));
}

std::vector<VkPhysicalDevice> getAllPhysikalDevices() {
	uint32_t physikalDeviceCount;
	testErrorCode(vkEnumeratePhysicalDevices(instance, &physikalDeviceCount, nullptr));	//anzahl der grakas + fehler break

	std::vector<VkPhysicalDevice> physikalDevices;
	physikalDevices.resize(physikalDeviceCount);
	testErrorCode(vkEnumeratePhysicalDevices(instance, &physikalDeviceCount, physikalDevices.data()));	//alle grakas holen + fehler break

	return physikalDevices;
}

void printAllPhysikalDevizes() {
	auto physikalDevices = getAllPhysikalDevices();

	for (uint32_t i = 0; i < physikalDevices.size(); i++)
		printStats(physikalDevices[i]);
}

void createDevice() {
	allPyhsikalDevices = getAllPhysikalDevices();

	float quePrios[] = { 1.0f,1.0f,1.0f,1.0f };

	VkDeviceQueueCreateInfo deviceQueCreateInfo;
	deviceQueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceQueCreateInfo.pNext = nullptr;
	deviceQueCreateInfo.flags = 0;
	deviceQueCreateInfo.queueFamilyIndex = 0;	//hier nach einer automatischen familie suchen 
	deviceQueCreateInfo.queueCount = 4;			//hier count prüfen ob die anzahl unterstützt wird
	deviceQueCreateInfo.pQueuePriorities = quePrios; //muss der anzahl der ques entsprechen

	VkPhysicalDeviceFeatures usedFeatures = {}; //hier features enablen

	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	VkDeviceCreateInfo deviceCreateInfo;
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pNext = nullptr;
	deviceCreateInfo.flags = 0;
	deviceCreateInfo.queueCreateInfoCount = 1;	//anzahl für mehrere unterschiedliche ques
	deviceCreateInfo.pQueueCreateInfos = &deviceQueCreateInfo;		//die ques als array
	deviceCreateInfo.enabledLayerCount = 0;
	deviceCreateInfo.ppEnabledLayerNames = nullptr;
	deviceCreateInfo.enabledExtensionCount = deviceExtensions.size();
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
	deviceCreateInfo.pEnabledFeatures = &usedFeatures;

	testErrorCode(vkCreateDevice(allPyhsikalDevices[0], &deviceCreateInfo, nullptr, &device));	//physikal device auswahl besser gestallten //nullptr ist DefaultAucator
}

void checkSurfaceSuport() {
	auto physikalDevices = getAllPhysikalDevices();

	VkBool32 supportsSwapchain;
	testErrorCode(vkGetPhysicalDeviceSurfaceSupportKHR(physikalDevices[0], 0, surface, &supportsSwapchain));

	if (!supportsSwapchain)
		__debugbreak();
}

void initQueue() {
	vkGetDeviceQueue(device, 0, 0, &queue);
}

void createSwapchain() {
	VkSwapchainCreateInfoKHR swapChainCreateInfo;
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.pNext = nullptr;
	swapChainCreateInfo.flags = 0;
	swapChainCreateInfo.surface = surface;
	swapChainCreateInfo.minImageCount = 3;	// check if possible
	swapChainCreateInfo.imageFormat = colorFormat; // check if possible
	swapChainCreateInfo.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR; // check if possible
	swapChainCreateInfo.imageExtent = { screenWidth,screenHeight }; // check if possible // bildschirmgröße
	swapChainCreateInfo.imageArrayLayers = 1;
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; //zeichen mode nicht einfach nur zum speichern
	swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapChainCreateInfo.queueFamilyIndexCount = 0;
	swapChainCreateInfo.pQueueFamilyIndices = nullptr;
	swapChainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR; //hier mirroring oä
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; //durchsichtiges fenster hier möglich
	swapChainCreateInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR; // check if possible
	swapChainCreateInfo.clipped = VK_TRUE;	//geklippte pixel wegschmeißen
	swapChainCreateInfo.oldSwapchain = swapchain; //alte swapchain hier angeben z.b. bei resize

	testErrorCode(vkCreateSwapchainKHR(device, &swapChainCreateInfo, nullptr, &swapchain));
}

void createImageviews() {
	testErrorCode(vkGetSwapchainImagesKHR(device, swapchain, &imageViewsCount, nullptr));

	VkImage* swapchainImages = new VkImage[imageViewsCount];
	testErrorCode(vkGetSwapchainImagesKHR(device, swapchain, &imageViewsCount, swapchainImages));

	VkImageViewCreateInfo imageViewcreateInfo;
	imageViewcreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewcreateInfo.pNext = nullptr;
	imageViewcreateInfo.flags = 0;
	imageViewcreateInfo.image = swapchainImages[0];
	imageViewcreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewcreateInfo.format = colorFormat; // check if possible
	imageViewcreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;	//farb kanäle tauschen
	imageViewcreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
	imageViewcreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
	imageViewcreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
	imageViewcreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; //farb type
	imageViewcreateInfo.subresourceRange.baseMipLevel = 0; //mip map kleinere texture
	imageViewcreateInfo.subresourceRange.levelCount = 1;
	imageViewcreateInfo.subresourceRange.baseArrayLayer = 0; //unterschiedliche texturen für mehrere augen
	imageViewcreateInfo.subresourceRange.layerCount = 1;

	imageViews = new VkImageView[imageViewsCount];
	for (int i = 0; i < imageViewsCount; i++) {
		imageViewcreateInfo.image = swapchainImages[i];
		testErrorCode(vkCreateImageView(device, &imageViewcreateInfo, nullptr, &imageViews[i]));
	}

	delete[] swapchainImages;
}

void createRenderPass() {

	VkAttachmentDescription attechmentDesription;
	attechmentDesription.flags = 0;											//speicher überlappung möglich
	attechmentDesription.format = colorFormat;
	attechmentDesription.samples = VK_SAMPLE_COUNT_1_BIT;
	attechmentDesription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;				//was pasiert mit werten nach dem laden
	attechmentDesription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;			//was pasiert mit dem speichen
	attechmentDesription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;		//fog of ware
	attechmentDesription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
	attechmentDesription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			//format vor laden
	attechmentDesription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;		//format nach laden

	VkAttachmentReference attechmentRefference;
	attechmentRefference.attachment = 0;
	attechmentRefference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDescription;
	subpassDescription.flags = 0;
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.inputAttachmentCount = 0;									//inputs
	subpassDescription.pInputAttachments = nullptr;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &attechmentRefference;
	subpassDescription.pResolveAttachments = nullptr;
	subpassDescription.pDepthStencilAttachment = nullptr;
	subpassDescription.preserveAttachmentCount = 0;
	subpassDescription.pPreserveAttachments = nullptr;

	VkSubpassDependency subpassDependency;
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependency.dstSubpass = 0;
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.srcAccessMask = 0;
	subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependency.dependencyFlags = 0;

	VkRenderPassCreateInfo renderPassCreateInfo;
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.pNext = nullptr;
	renderPassCreateInfo.flags = 0;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &attechmentDesription;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDescription;
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &subpassDependency;

	testErrorCode(vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass));
}

void createRenderPipeline() {
	std::vector<char> vertShaderCode = readFile("vert.spv");
	std::vector<char> fragShader = readFile("frag.spv");

	CreateShaderModul(vertShaderCode, &vertShaderModul);
	CreateShaderModul(fragShader, &fragShaderModul);

	VkPipelineShaderStageCreateInfo vertShaderCreateInfo;
	vertShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderCreateInfo.pNext = nullptr;
	vertShaderCreateInfo.flags = 0;
	vertShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderCreateInfo.module = vertShaderModul;
	vertShaderCreateInfo.pName = "main";				//startpunkt in shader
	vertShaderCreateInfo.pSpecializationInfo = nullptr;	//constanten in shader setzen

	VkPipelineShaderStageCreateInfo fragShaderCreateInfo;
	fragShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderCreateInfo.pNext = nullptr;
	fragShaderCreateInfo.flags = 0;
	fragShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderCreateInfo.module = fragShaderModul;
	fragShaderCreateInfo.pName = "main";				//startpunkt in shader
	fragShaderCreateInfo.pSpecializationInfo = nullptr;	//constanten in shader setzen

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderCreateInfo ,fragShaderCreateInfo };

	auto vertexBindingDescriptions = Vertex::getBindingDescription();
	auto vertexAtributDescriptions = Vertex::getAttributDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo;
	vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.pNext = nullptr;
	vertexInputCreateInfo.flags = 0;
	vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputCreateInfo.pVertexBindingDescriptions = &vertexBindingDescriptions;		//platz pro vertex //per vertex oder per instanz		//mesh instanzing
	vertexInputCreateInfo.vertexAttributeDescriptionCount = vertexAtributDescriptions.size();
	vertexInputCreateInfo.pVertexAttributeDescriptions = vertexAtributDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo;
	inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCreateInfo.pNext = nullptr;
	inputAssemblyCreateInfo.flags = 0;
	inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;	//liste an dreieck
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;				//restart des zeichnens

	//mehrere render ziehle
	VkViewport viewport;
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = screenWidth;
	viewport.height = screenHeight;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	//kann elemente des screens wegschneiden alles was nicht drin ist wird weg geschnitten
	VkRect2D scissore;
	scissore.offset.x = 0;
	scissore.offset.y = 0;
	scissore.extent.width = screenWidth;
	scissore.extent.height = screenHeight;

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo;
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.pNext = nullptr;
	viewportStateCreateInfo.flags = 0;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &viewport;	//array
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &scissore;	//array

	VkPipelineRasterizationStateCreateInfo rasterisationCreateInfo;
	rasterisationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterisationCreateInfo.pNext = nullptr;
	rasterisationCreateInfo.flags = 0;
	rasterisationCreateInfo.depthClampEnable = VK_FALSE;
	rasterisationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterisationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;		//wiremesh mit line  VK_POLYGON_MODE_LINE	//fehler noch lösen
	rasterisationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterisationCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterisationCreateInfo.depthBiasEnable = VK_FALSE;
	rasterisationCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterisationCreateInfo.depthBiasClamp = 0.0f;
	rasterisationCreateInfo.depthBiasSlopeFactor = 0.0f;
	rasterisationCreateInfo.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisamplerCreateInfo;
	multisamplerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplerCreateInfo.pNext = nullptr;
	multisamplerCreateInfo.flags = 0;
	multisamplerCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;		//AAx1
	multisamplerCreateInfo.sampleShadingEnable = VK_FALSE;						//AA aus
	multisamplerCreateInfo.minSampleShading = 1.0f;
	multisamplerCreateInfo.pSampleMask = nullptr;
	multisamplerCreateInfo.alphaToCoverageEnable = VK_TRUE;
	multisamplerCreateInfo.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachmend;
	colorBlendAttachmend.blendEnable = VK_TRUE;
	colorBlendAttachmend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachmend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachmend.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachmend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmend.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo;
	colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendCreateInfo.pNext = nullptr;
	colorBlendCreateInfo.flags = 0;
	colorBlendCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendCreateInfo.logicOp = VK_LOGIC_OP_NO_OP;
	colorBlendCreateInfo.attachmentCount = 1;
	colorBlendCreateInfo.pAttachments = &colorBlendAttachmend;
	colorBlendCreateInfo.blendConstants[0] = 0.0f;
	colorBlendCreateInfo.blendConstants[1] = 0.0f;
	colorBlendCreateInfo.blendConstants[2] = 0.0f;
	colorBlendCreateInfo.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo layoutCreateInfo;
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutCreateInfo.pNext = nullptr;
	layoutCreateInfo.flags = 0;
	layoutCreateInfo.setLayoutCount;
	layoutCreateInfo.pSetLayouts;
	layoutCreateInfo.pushConstantRangeCount;
	layoutCreateInfo.pPushConstantRanges;

	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo;
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.pNext = nullptr;
	dynamicStateCreateInfo.flags = 0;
	dynamicStateCreateInfo.dynamicStateCount = 2;
	dynamicStateCreateInfo.pDynamicStates = dynamicStates;

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pNext = nullptr;
	pipelineLayoutCreateInfo.flags = 0;
	pipelineLayoutCreateInfo.setLayoutCount = 0;
	pipelineLayoutCreateInfo.pSetLayouts = nullptr;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

	testErrorCode(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

	VkGraphicsPipelineCreateInfo pipelineCreateInfo;
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.pNext = nullptr;
	pipelineCreateInfo.flags = 0;
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = shaderStages;
	pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	pipelineCreateInfo.pTessellationState = nullptr;
	pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	pipelineCreateInfo.pRasterizationState = &rasterisationCreateInfo;
	pipelineCreateInfo.pMultisampleState = &multisamplerCreateInfo;
	pipelineCreateInfo.pDepthStencilState = nullptr;
	pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
	pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	pipelineCreateInfo.layout = pipelineLayout;
	pipelineCreateInfo.renderPass = renderPass;
	pipelineCreateInfo.subpass = 0;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;	//vererbungs pipeline //kürzer //switchen bei vererbung schneller
	pipelineCreateInfo.basePipelineIndex = -1;

	testErrorCode(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline));
}

void createFrameBuffers() {
	framebuffers = new VkFramebuffer[imageViewsCount];

	VkFramebufferCreateInfo framebufferCreateInfo;
	framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferCreateInfo.pNext = nullptr;
	framebufferCreateInfo.flags = 0;
	framebufferCreateInfo.renderPass = renderPass;
	framebufferCreateInfo.attachmentCount = 1;
	framebufferCreateInfo.pAttachments;
	framebufferCreateInfo.width = screenWidth;
	framebufferCreateInfo.height = screenHeight;
	framebufferCreateInfo.layers = 1;

	for (int i = 0; i < imageViewsCount; i++) {
		framebufferCreateInfo.pAttachments = &(imageViews[i]);
		testErrorCode(vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &(framebuffers[i])));
	}
}

void createCommandPool() {
	VkCommandPoolCreateInfo commandPollCreateInfo;
	commandPollCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPollCreateInfo.pNext = nullptr;
	commandPollCreateInfo.flags = 0;
	commandPollCreateInfo.queueFamilyIndex = 0;		//check if ok

	testErrorCode(vkCreateCommandPool(device, &commandPollCreateInfo, nullptr, &commandPool));
}

void createCommandBuffers() {
	VkCommandBufferAllocateInfo commandBufferAllocateInfo;
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.pNext = nullptr;
	commandBufferAllocateInfo.commandPool = commandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = imageViewsCount;

	commandBuffers = new VkCommandBuffer[imageViewsCount];

	testErrorCode(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, commandBuffers));
}

uint32_t memTypeHelper(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties pDMemProp;
	vkGetPhysicalDeviceMemoryProperties(allPyhsikalDevices[0], &pDMemProp);

	for (uint32_t i = 0; i < pDMemProp.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (pDMemProp.memoryTypes[i].propertyFlags & properties) == properties )
			return i;
	}

	throw std::runtime_error("mem type not found");
}

void CreateBuffer(VkDeviceSize deviceSize, VkBufferUsageFlags usageFlags, VkBuffer &buffer, VkMemoryPropertyFlags memPropFlags, VkDeviceMemory &deviceMem) {
	VkBufferCreateInfo bufferInfo;
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.pNext = nullptr;
	bufferInfo.flags = 0;
	bufferInfo.size = deviceSize;
	bufferInfo.usage = usageFlags;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.queueFamilyIndexCount = 0;
	bufferInfo.pQueueFamilyIndices = nullptr;

	testErrorCode(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer));

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	VkMemoryAllocateInfo memAlkinfo;
	memAlkinfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAlkinfo.pNext = nullptr;
	memAlkinfo.allocationSize = memRequirements.size;
	memAlkinfo.memoryTypeIndex = memTypeHelper(memRequirements.memoryTypeBits, memPropFlags);

	testErrorCode(vkAllocateMemory(device, &memAlkinfo, nullptr, &deviceMem));

	testErrorCode(vkBindBufferMemory(device, buffer, deviceMem, 0));
}

void removeBuffer(VkBuffer& buffer, VkDeviceMemory& deviceMem) {
	vkFreeMemory(device, deviceMem, nullptr);
	vkDestroyBuffer(device, buffer, nullptr);
}

void copyBuffer(VkBuffer src, VkBuffer dest, VkDeviceSize size) {
	VkCommandBufferAllocateInfo cBufferAlInfo;
	cBufferAlInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cBufferAlInfo.pNext = nullptr;
	cBufferAlInfo.commandPool = commandPool;
	cBufferAlInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cBufferAlInfo.commandBufferCount = 1;

	VkCommandBuffer cBuffer;
	testErrorCode(vkAllocateCommandBuffers(device, &cBufferAlInfo, &cBuffer));

	VkCommandBufferBeginInfo cBufferBeginInfo;
	cBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cBufferBeginInfo.pNext = nullptr;
	cBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	cBufferBeginInfo.pInheritanceInfo = nullptr;

	testErrorCode(vkBeginCommandBuffer(cBuffer, &cBufferBeginInfo));

	VkBufferCopy cp;
	cp.srcOffset = 0;
	cp.dstOffset = 0;
	cp.size = size;

	vkCmdCopyBuffer(cBuffer, src, dest, 1, &cp);

	testErrorCode(vkEndCommandBuffer(cBuffer));

	VkSubmitInfo submitInfo;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cBuffer;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr;

	testErrorCode(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

	vkQueueWaitIdle(queue);

	vkFreeCommandBuffers(device, commandPool, 1, &cBuffer);
}

void createVertexBuffer() {
	
	VkDeviceSize bSize = sizeof(Vertex) * verticies.size();

	VkBuffer sBuffer;
	VkDeviceMemory sBufferMem;
	CreateBuffer(bSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, sBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sBufferMem);


	void* rawData;
	vkMapMemory(device, sBufferMem, 0, bSize, 0, &rawData);
	memcpy(rawData, verticies.data(), bSize);
	vkUnmapMemory(device, sBufferMem);

	CreateBuffer(bSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vertexBuffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBufferMem);

	copyBuffer(sBuffer, vertexBuffer, bSize);

	removeBuffer(sBuffer, sBufferMem);
}

void recordCommandBuffers() {
	VkCommandBufferBeginInfo commandBufferBeginInfo;
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pNext = nullptr;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	commandBufferBeginInfo.pInheritanceInfo = nullptr;

	VkRenderPassBeginInfo renderPassBeginInfo;
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.pNext = nullptr;
	renderPassBeginInfo.renderPass = renderPass;
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent.height = screenHeight;
	renderPassBeginInfo.renderArea.extent.width = screenWidth;
	renderPassBeginInfo.clearValueCount = 1;
	VkClearValue clearValue;
	clearValue.color = { 0.0,0.0,0.0,1.0 };
	renderPassBeginInfo.pClearValues = &clearValue;

	for (int i = 0; i < imageViewsCount; i++) {
		testErrorCode(vkBeginCommandBuffer(commandBuffers[i], &commandBufferBeginInfo));
		renderPassBeginInfo.framebuffer = framebuffers[i];
		vkCmdBeginRenderPass(commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		VkViewport viewport;
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = screenWidth;
		viewport.height = screenHeight;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffers[i],0,1,&viewport);

		VkRect2D scissor;
		scissor.offset = { 0, 0};
		scissor.extent= { screenWidth,screenHeight };
		vkCmdSetScissor(commandBuffers[i], 0, 1, &scissor);

		VkDeviceSize offsets[] = { 0 };

		vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &vertexBuffer, offsets);

		vkCmdDraw(commandBuffers[i], verticies.size(), 1, 0, 0);
		vkCmdEndRenderPass(commandBuffers[i]);
		testErrorCode(vkEndCommandBuffer(commandBuffers[i]));
	}
}

void createSemaphores() {
	VkSemaphoreCreateInfo semaphoreCreateInfo;
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = nullptr;
	semaphoreCreateInfo.flags = 0;

	testErrorCode(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphoreImgAvailable));
	testErrorCode(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphoreImgRendered));
}

void startVulkan() {

	createInstance();
#ifdef ShowDebug 
	printExtensions(); 
#endif
	createGLFWWindowSurface();
#ifdef ShowDebug 
	printAllPhysikalDevizes();
#endif
	checkSurfaceSuport();
	createDevice();
	initQueue();
	createSwapchain();
	createImageviews();
	createRenderPass();
	createRenderPipeline();
	createFrameBuffers();
	createCommandPool();
	createCommandBuffers();
	createVertexBuffer();
	recordCommandBuffers();
	createSemaphores();
}

uint32_t getNextImageIndes() {
	uint32_t imageindex;
	testErrorCode(vkAcquireNextImageKHR(device, swapchain, -1, semaphoreImgAvailable, VK_NULL_HANDLE, &imageindex));
	return imageindex;
}

void setNextRendering(uint32_t imageIndex) {
	VkPipelineStageFlags waitStageMask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submitInfo;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &semaphoreImgAvailable;
	submitInfo.pWaitDstStageMask = waitStageMask;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &(commandBuffers[imageIndex]);
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &semaphoreImgRendered;

	testErrorCode(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
}

void presendNextimage(uint32_t imageIndex) {
	VkPresentInfoKHR presentInfo;
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &semaphoreImgRendered;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	testErrorCode(vkQueuePresentKHR(queue, &presentInfo));
}

void recreateSwapChain() {
	vkDeviceWaitIdle(device);

	vkFreeCommandBuffers(device, commandPool, imageViewsCount, commandBuffers);
	delete[] commandBuffers;
	vkDestroyCommandPool(device, commandPool, nullptr);
	for (int i = 0; i < imageViewsCount; i++)
		vkDestroyFramebuffer(device, framebuffers[i], nullptr);
	delete[] framebuffers;
	for (int i = 0; i < imageViewsCount; i++)
		vkDestroyImageView(device, imageViews[i], nullptr);
	delete[] imageViews;

	auto tempSwapchain = swapchain;

	createSwapchain();
	createImageviews();
	createFrameBuffers();
	createCommandPool();
	createCommandBuffers();
	recordCommandBuffers();

	vkDestroySwapchainKHR(device, tempSwapchain, nullptr);
}

void drawFrame() {
	uint32_t imageIndex = getNextImageIndes();
	setNextRendering(imageIndex);
	presendNextimage(imageIndex);
}

void loop() {
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		drawFrame();
	}
}

void shutDownVulkan() {
	vkDeviceWaitIdle(device);							//für jedes logische device

	//in umgekehrter reienfolge zum erstellen

	removeBuffer(vertexBuffer, vertexBufferMem);

	vkDestroySemaphore(device, semaphoreImgAvailable, nullptr);
	vkDestroySemaphore(device, semaphoreImgRendered, nullptr);
	vkFreeCommandBuffers(device, commandPool, imageViewsCount, commandBuffers);
	delete[] commandBuffers;
	vkDestroyCommandPool(device, commandPool, nullptr);
	for (int i = 0; i < imageViewsCount; i++)
		vkDestroyFramebuffer(device, framebuffers[i], nullptr);
	delete[] framebuffers;
	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroyRenderPass(device, renderPass, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyShaderModule(device, vertShaderModul, nullptr);
	vkDestroyShaderModule(device, fragShaderModul, nullptr);
	for (int i = 0; i < imageViewsCount; i++)
		vkDestroyImageView(device, imageViews[i], nullptr);
	delete[] imageViews;
	vkDestroySwapchainKHR(device, swapchain, nullptr);
	vkDestroyDevice(device, nullptr);					
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);				//zerstöhrt auch die physikal devices

}

void shutDownGLFW() {
	glfwDestroyWindow(window);
	glfwTerminate();
}

int main()
{
	startGLFW();
	startVulkan();
	loop();
	shutDownVulkan();
	shutDownGLFW();

	return 0;
}