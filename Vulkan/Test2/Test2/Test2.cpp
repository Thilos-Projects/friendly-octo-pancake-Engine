#include <iostream>
#include <fstream>
#include <vector>

#include "DepthImage.h"
#include "EasyImage.h"
#include "NonDependingFunktions.h"

#include <chrono>

#define ShowDebug

DepthImage depthImage;
EasyImage testImage;

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
VkBuffer indexBuffer;
VkDeviceMemory indexBufferMem;
VkBuffer uniformBuffer;
VkDeviceMemory uniformBufferMem;

VkDescriptorSetLayout desLayout;
VkDescriptorPool desPool;
VkDescriptorSet desSet;

uint32_t screenWidth = 300;
uint32_t screenHeight = 300;
const VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM;

std::vector<VkPhysicalDevice> allPyhsikalDevices;

class Vertex {
public:
	Vertex(glm::vec3 pos, glm::vec3 color, glm::vec2 uvCoord)
		:pos(pos),color(color), uvCoord(uvCoord){}
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 uvCoord;

	static VkVertexInputBindingDescription getBindingDescription(const uint32_t index = 0) {
		VkVertexInputBindingDescription toRet;

		toRet.binding = index;
		toRet.stride = sizeof(Vertex);
		toRet.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return toRet;
	}
	static std::vector<VkVertexInputAttributeDescription> getAttributDescriptions(const uint32_t index = 0) {
		std::vector<VkVertexInputAttributeDescription> toRet(3);

		toRet[0].location = 0;
		toRet[0].binding = index;
		toRet[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		toRet[0].offset = offsetof(Vertex, pos);

		toRet[1].location = 1;
		toRet[1].binding = index;
		toRet[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		toRet[1].offset = offsetof(Vertex, color);

		toRet[2].location = 2;
		toRet[2].binding = index;
		toRet[2].format = VK_FORMAT_R32G32_SFLOAT;
		toRet[2].offset = offsetof(Vertex, uvCoord);

		return toRet;
	}
};

std::vector<Vertex> verticies = {
	Vertex({  0.5f, -0.5f, 0.0f}, { 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}),
	Vertex({ -0.5f,  0.5f, 0.0f}, { 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}),
	Vertex({ -0.5f, -0.5f, 0.0f}, { 1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}),
	Vertex({  0.5f,  0.5f, 0.0f}, { 0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}),

	Vertex({  0.5f, -0.5f, -0.5f}, { 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}),
	Vertex({ -0.5f,  0.5f, -0.5f}, { 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}),
	Vertex({ -0.5f, -0.5f, -0.5f}, { 1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}),
	Vertex({  0.5f,  0.5f, -0.5f}, { 0.0f, 1.0f, 0.0f}, {1.0f, 0.0f})
};

std::vector<uint32_t> indices = {
	2,0,3,2,3,1,
	6,4,7,6,7,5
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

void recreateSwapChain();

void onWindowResized(GLFWwindow* window, int width, int height) {

	screenWidth = width;
	screenHeight = height;

	recreateSwapChain();
}

GLFWwindow* startGLFW(uint32_t screenWidth, uint32_t screenHeight) {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);		//später ändern auf rezisable

	GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Test", nullptr, nullptr);	//größe ändern
	glfwSetWindowSizeCallback(window, onWindowResized);

	return window;
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

void printAllPhysikalDevizes() {
	auto physikalDevices = getAllPhysikalDevices(instance);

	for (uint32_t i = 0; i < physikalDevices.size(); i++)
		printStats(physikalDevices[i], surface);
}

void createDevice() {
	allPyhsikalDevices = getAllPhysikalDevices(instance);

	float quePrios[] = { 1.0f,1.0f,1.0f,1.0f };

	VkDeviceQueueCreateInfo deviceQueCreateInfo;
	deviceQueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceQueCreateInfo.pNext = nullptr;
	deviceQueCreateInfo.flags = 0;
	deviceQueCreateInfo.queueFamilyIndex = 0;	//hier nach einer automatischen familie suchen 
	deviceQueCreateInfo.queueCount = 4;			//hier count prüfen ob die anzahl unterstützt wird
	deviceQueCreateInfo.pQueuePriorities = quePrios; //muss der anzahl der ques entsprechen

	VkPhysicalDeviceFeatures usedFeatures = {}; //hier features enablen
	usedFeatures.samplerAnisotropy = VK_TRUE;

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
	auto physikalDevices = getAllPhysikalDevices(instance);

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

	imageViews = new VkImageView[imageViewsCount];
	for (int i = 0; i < imageViewsCount; i++) {
		createImageView(device, swapchainImages[i], colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, imageViews[i]);
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

	VkAttachmentDescription depthAttachment = DepthImage::getdepthAttachment(allPyhsikalDevices[0]);

	VkAttachmentReference depthAttechmentRefference;
	depthAttechmentRefference.attachment = 1;
	depthAttechmentRefference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


	VkSubpassDescription subpassDescription;
	subpassDescription.flags = 0;
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.inputAttachmentCount = 0;									//inputs
	subpassDescription.pInputAttachments = nullptr;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &attechmentRefference;
	subpassDescription.pResolveAttachments = nullptr;
	subpassDescription.pDepthStencilAttachment = &depthAttechmentRefference;
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

	std::vector<VkAttachmentDescription> attachments;
	attachments.push_back(attechmentDesription);
	attachments.push_back(depthAttachment);

	VkRenderPassCreateInfo renderPassCreateInfo;
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.pNext = nullptr;
	renderPassCreateInfo.flags = 0;
	renderPassCreateInfo.attachmentCount = attachments.size();
	renderPassCreateInfo.pAttachments = attachments.data();
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDescription;
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &subpassDependency;

	testErrorCode(vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass));
}

void createDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding desLayoutBinding;
	desLayoutBinding.binding = 0;							//in shader
	desLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	desLayoutBinding.descriptorCount = 1;
	desLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	desLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding samplerDInfo;
	samplerDInfo.binding = 1;								//in shader
	samplerDInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerDInfo.descriptorCount = 1;
	samplerDInfo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerDInfo.pImmutableSamplers = nullptr;

	std::vector< VkDescriptorSetLayoutBinding> layoutBinding;
	layoutBinding.push_back(desLayoutBinding);
	layoutBinding.push_back(samplerDInfo);

	VkDescriptorSetLayoutCreateInfo desSetLayoutCreateInfo;
	desSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	desSetLayoutCreateInfo.pNext = nullptr;
	desSetLayoutCreateInfo.flags = 0;
	desSetLayoutCreateInfo.bindingCount = layoutBinding.size();
	desSetLayoutCreateInfo.pBindings = layoutBinding.data();

	testErrorCode(vkCreateDescriptorSetLayout(device, &desSetLayoutCreateInfo, nullptr, &desLayout));
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
	rasterisationCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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

	VkPipelineDepthStencilStateCreateInfo depthStencileStateCreateInfo = DepthImage::getDepthStencilStateCreateInfoOpaque();

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
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &desLayout;
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
	pipelineCreateInfo.pDepthStencilState = &depthStencileStateCreateInfo;
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
	framebufferCreateInfo.width = screenWidth;
	framebufferCreateInfo.height = screenHeight;
	framebufferCreateInfo.layers = 1;

	std::vector<VkImageView> attachmentViews;

	for (int i = 0; i < imageViewsCount; i++) {
		attachmentViews.clear();
		attachmentViews.push_back(imageViews[i]);
		attachmentViews.push_back(depthImage.getImageView());
		framebufferCreateInfo.attachmentCount = attachmentViews.size();
		framebufferCreateInfo.pAttachments = attachmentViews.data();
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

void createDepthImage() {
	depthImage.create(device, allPyhsikalDevices[0], commandPool, queue, screenWidth, screenHeight);
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

void loadTexture() {
	testImage.load("7.1.3.jpg");
	testImage.upload(device, allPyhsikalDevices[0], commandPool, queue);
}

void createUniformBuffer() {
	VkDeviceSize bSize = sizeof(glm::mat4);
	createBuffer(device, allPyhsikalDevices[0], bSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, uniformBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBufferMem);
}

void createDescriptorPool() {
	VkDescriptorPoolSize dSize;
	dSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	dSize.descriptorCount = 1;

	VkDescriptorPoolSize dSamplerSize;
	dSamplerSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	dSamplerSize.descriptorCount = 1;

	std::vector< VkDescriptorPoolSize> poolSizeA;
	poolSizeA.push_back(dSize);
	poolSizeA.push_back(dSamplerSize);

	VkDescriptorPoolCreateInfo dPollCreateInfo;
	dPollCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	dPollCreateInfo.pNext = nullptr;
	dPollCreateInfo.flags = 0;
	dPollCreateInfo.maxSets = 1;
	dPollCreateInfo.poolSizeCount = poolSizeA.size();
	dPollCreateInfo.pPoolSizes = poolSizeA.data();

	testErrorCode(vkCreateDescriptorPool(device, &dPollCreateInfo, nullptr, &desPool));
}

void createDescriptorSet() {
	VkDescriptorSetAllocateInfo desSAllocInfo;
	desSAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	desSAllocInfo.pNext = nullptr;
	desSAllocInfo.descriptorPool = desPool;
	desSAllocInfo.descriptorSetCount = 1;
	desSAllocInfo.pSetLayouts = &desLayout;

	testErrorCode(vkAllocateDescriptorSets(device, &desSAllocInfo, &desSet));

	VkDescriptorBufferInfo desBuffInfo;
	desBuffInfo.buffer = uniformBuffer;
	desBuffInfo.offset = 0;
	desBuffInfo.range = sizeof(glm::mat4);

	VkWriteDescriptorSet desWrite;
	desWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desWrite.pNext = nullptr;
	desWrite.dstSet = desSet;
	desWrite.dstBinding = 0;		//gegeben aus shader
	desWrite.dstArrayElement = 0;
	desWrite.descriptorCount = 1;
	desWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	desWrite.pImageInfo = nullptr;
	desWrite.pBufferInfo = &desBuffInfo;
	desWrite.pTexelBufferView = nullptr;

	VkDescriptorImageInfo samplerInfo;
	samplerInfo.sampler = testImage.getSampler();
	samplerInfo.imageView = testImage.getImageView();
	samplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet desSamplerWrite;
	desSamplerWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desSamplerWrite.pNext = nullptr;
	desSamplerWrite.dstSet = desSet;
	desSamplerWrite.dstBinding = 1;		//gegeben aus shader
	desSamplerWrite.dstArrayElement = 0;
	desSamplerWrite.descriptorCount = 1;
	desSamplerWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	desSamplerWrite.pImageInfo = &samplerInfo;
	desSamplerWrite.pBufferInfo = nullptr;
	desSamplerWrite.pTexelBufferView = nullptr;

	std::vector< VkWriteDescriptorSet> writeSet;
	writeSet.push_back(desWrite);
	writeSet.push_back(desSamplerWrite);

	vkUpdateDescriptorSets(device, writeSet.size(), writeSet.data(), 0, nullptr);
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

	VkClearValue colorClearValue =  { 0.0f, 0.0f, 0.0f, 1.0f };

	VkClearValue depthClearValue = { 1.0f, 0.0f };

	std::vector<VkClearValue> clearValues;
	clearValues.push_back(colorClearValue);
	clearValues.push_back(depthClearValue);

	renderPassBeginInfo.clearValueCount = clearValues.size();
	renderPassBeginInfo.pClearValues = clearValues.data();

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
		vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &desSet, 0, nullptr);
		vkCmdDrawIndexed(commandBuffers[i], indices.size(), 1, 0, 0, 0);
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
	createDescriptorSetLayout();
	createRenderPipeline();
	createCommandPool();
	createDepthImage();
	createFrameBuffers();
	createCommandBuffers();
	loadTexture();
	createBufferFromArrayToGraca(device, queue, allPyhsikalDevices[0], commandPool, verticies, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBuffer, vertexBufferMem);
	createBufferFromArrayToGraca(device, queue, allPyhsikalDevices[0], commandPool, indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexBuffer, indexBufferMem);
	createUniformBuffer();
	createDescriptorPool();
	createDescriptorSet();
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

	depthImage.destroy();

	vkFreeCommandBuffers(device, commandPool, imageViewsCount, commandBuffers);
	delete[] commandBuffers;
	for (int i = 0; i < imageViewsCount; i++)
		vkDestroyFramebuffer(device, framebuffers[i], nullptr);
	delete[] framebuffers;
	for (int i = 0; i < imageViewsCount; i++)
		vkDestroyImageView(device, imageViews[i], nullptr);
	delete[] imageViews;

	auto tempSwapchain = swapchain;

	createSwapchain();
	createImageviews();
	createDepthImage();
	createFrameBuffers();
	createCommandBuffers();
	recordCommandBuffers();

	vkDestroySwapchainKHR(device, tempSwapchain, nullptr);
}

void drawFrame() {
	uint32_t imageIndex = getNextImageIndes();
	setNextRendering(imageIndex);
	presendNextimage(imageIndex);
}

auto gameStartTime = std::chrono::high_resolution_clock::now();
void updateMVP() {

	auto frameTime = std::chrono::high_resolution_clock::now();

	float timeSinceStart = std::chrono::duration_cast<std::chrono::milliseconds>(frameTime - gameStartTime).count() / 1000.0f;

	glm::mat4 modle = glm::rotate(glm::mat4(1.0f), timeSinceStart * glm::radians(30.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	glm::mat4 view = glm::lookAt(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	glm::mat4 projection = glm::perspective(glm::radians(60.0f), (float)screenWidth / (float)screenHeight, 0.01f, 10.0f);
	projection[1][1] *= -1;

	glm::mat4 MVP = projection * view * modle;

	void* data;
	testErrorCode(vkMapMemory(device, uniformBufferMem, 0, sizeof(glm::mat4), 0, &data));
	memcpy(data, &MVP, sizeof(glm::mat4));
	vkUnmapMemory(device, uniformBufferMem);
}

void loop() {
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		updateMVP();
		drawFrame();
	}
}

void shutDownVulkan() {
	vkDeviceWaitIdle(device);							//für jedes logische device

	//in umgekehrter reienfolge zum erstellen

	depthImage.destroy();

	vkDestroyDescriptorSetLayout(device, desLayout,nullptr);
	vkDestroyDescriptorPool(device, desPool, nullptr);

	removeBuffer(device, uniformBuffer, uniformBufferMem);
	removeBuffer(device, vertexBuffer, vertexBufferMem);
	removeBuffer(device, indexBuffer, indexBufferMem);

	testImage.unload();

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
	window = startGLFW(screenWidth, screenHeight);
	startVulkan();
	loop();
	shutDownVulkan();
	shutDownGLFW();

	return 0;
}