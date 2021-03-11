#ifndef _RENDERER_
#define _RENDERER_

#include <iostream>
#include <fstream>
#include <vector>

#include "Vertex.h"
#include "Mesh.h"
#include "NonDependingFunktions.h"
#include "MeshHelper.h"

#include "glslReader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <chrono>

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

class Image {
public:
	struct Configuration {
		const char* path;
		VkFilter magFilter = VK_FILTER_LINEAR;
		VkFilter minFilter = VK_FILTER_LINEAR;
		VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		VkSamplerAddressMode addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		float mipLodBias = 0.0f;
		VkBool32 anisotropyEnable = VK_TRUE;
		int maxAnisotropy = 16;
		VkBool32 compareEnable = VK_FALSE;
		VkCompareOp compareOp = VK_COMPARE_OP_ALWAYS;
		float minLod = 0.0f;
		float maxLod = 0.0f;
		VkBorderColor borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		VkBool32 unnormalizedCoordinates = VK_FALSE;
	};
private:
	Configuration configuration;

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

	VkDevice device = VK_NULL_HANDLE;
	VkAllocationCallbacks* allucator = nullptr;

	void changeLayount(VkDevice& device, VkCommandPool& commandPool, VkQueue& queue, VkImageLayout layout) {

		changeImageLayout(device, commandPool, queue, image, VK_FORMAT_R8G8B8A8_UNORM, imageLayout, layout);

		imageLayout = layout;
	}

	void writeBufferToImmage(VkDevice& device, VkCommandPool& commandPool, VkQueue& queue, VkBuffer& buffer) {
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

	void load(Configuration &configuration) {
		this->configuration = configuration;
		pixels = stbi_load(configuration.path, &width, &height, &chanels, STBI_rgb_alpha);	//load and force alpha
		if (pixels == nullptr)
			throw std::invalid_argument("Could not loadImage");
	}
	void upload(VkDevice& device, VkAllocationCallbacks* allucator, VkPhysicalDevice& physikalDevice, VkCommandPool& commandPool, VkQueue& queue) {
		if (isUploaded)
			throw std::logic_error("is already uploaded");
		if (pixels == nullptr)
			throw std::logic_error("image empty");

		this->device = device;
		this->allucator = allucator;

		VkDeviceSize iSize = getSize();

		VkBuffer sBuffer;
		VkDeviceMemory sBufferMem;

		createBuffer(device, physikalDevice, iSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, sBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sBufferMem);

		void* data;
		vkMapMemory(device, sBufferMem, 0, iSize, 0, &data);
		memcpy(data, getRaw(), iSize);
		vkUnmapMemory(device, sBufferMem);

		createImage(device, physikalDevice, width, height, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_FORMAT_R8G8B8A8_UNORM, image, imageMem);

		changeLayount(device, commandPool, queue, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		writeBufferToImmage(device, commandPool, queue, sBuffer);
		changeLayount(device, commandPool, queue, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(device, sBuffer, allucator);
		vkFreeMemory(device, sBufferMem, allucator);

		createImageView(device, image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, imageView);

		VkSamplerCreateInfo samplerInfo;
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.pNext = nullptr;
		samplerInfo.flags = 0;
		samplerInfo.magFilter = configuration.magFilter;
		samplerInfo.minFilter = configuration.minFilter;
		samplerInfo.mipmapMode = configuration.mipmapMode;
		samplerInfo.addressModeU = configuration.addressModeU;
		samplerInfo.addressModeV = configuration.addressModeV;
		samplerInfo.addressModeW = configuration.addressModeW;
		samplerInfo.mipLodBias = configuration.mipLodBias;
		samplerInfo.anisotropyEnable = configuration.anisotropyEnable;
		samplerInfo.maxAnisotropy = configuration.maxAnisotropy;
		samplerInfo.compareEnable = configuration.compareEnable;
		samplerInfo.compareOp = configuration.compareOp;
		samplerInfo.minLod = configuration.minLod;
		samplerInfo.maxLod = configuration.maxLod;
		samplerInfo.borderColor = configuration.borderColor;
		samplerInfo.unnormalizedCoordinates = configuration.unnormalizedCoordinates;

		testErrorCode(vkCreateSampler(device, &samplerInfo, allucator, &imageSampler));

		isUploaded = true;
	}
	void unload() {
		if (pixels != nullptr) {
			if (isUploaded) {
				vkDestroySampler(device, imageSampler, allucator);
				vkDestroyImageView(device, imageView, allucator);
				vkDestroyImage(device, image, allucator);
				vkFreeMemory(device, imageMem, allucator);

				device = VK_NULL_HANDLE;
				allucator = nullptr;

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

class Shader {
public: 
	struct Configuration
	{
		struct DescriptorSetLayoutBindingConfiguration {
			uint32_t bindingId;
			VkDescriptorType type;
			VkSampler* samplerPointer = nullptr;
			uint32_t multiuse = 0;					//wird für uniforms als size verwendet
													//wird für samplers als imgId verwendet
			uint32_t buferIndex;
		};
		std::vector<DescriptorSetLayoutBindingConfiguration> bindingConfiugurations;
		const char* shaderPath;
		const char* compilerPath;
		const char* shaderMainFunction;
		VkShaderStageFlagBits shaderStageFlagBits;
	};

private:
	Configuration configuration;
	VkShaderModule shaderModule;
	std::vector<VkDescriptorSetLayoutBinding> descriptorLayoutBindings;
	std::vector<std::pair<uint32_t, std::pair<uint32_t, uint32_t>>> descriptorUinformWriteInfos;
	std::vector<std::pair<uint32_t, std::pair<uint32_t, uint32_t>>> descriptorUinformDynamicWriteInfos;
	std::vector<std::pair<uint32_t, uint32_t>> descriptorsamplerWriteInfos;

	std::vector<char> shaderCodeSpv;

	VkDevice device;
	VkAllocationCallbacks* allucator;

	bool isInit = false;
	bool isCreated = false;

public:

	~Shader() { destroy(); };

	void init(Configuration &configuration) {
		this->configuration = configuration;
		std::string path = configuration.shaderPath;
		std::string newPath = path.substr(0, path.find_last_of(".")) + ".spv";
		std::string command = configuration.compilerPath;
		command += " -V " + path + " -o " + newPath;
		auto returnCode = system(command.c_str());
		std::vector<char> shaderCode = readFile(path.c_str());
		shaderCodeSpv = readFile(newPath.c_str());

		for (int i = 0; i < configuration.bindingConfiugurations.size(); i++)
		{
			descriptorLayoutBindings.push_back(VkDescriptorSetLayoutBinding());
			VkDescriptorSetLayoutBinding* binding = &descriptorLayoutBindings[descriptorLayoutBindings.size() - 1];
			binding->binding = configuration.bindingConfiugurations[i].bindingId;
			binding->descriptorType = configuration.bindingConfiugurations[i].type;
			binding->descriptorCount = 1;
			binding->stageFlags = configuration.shaderStageFlagBits;
			binding->pImmutableSamplers = configuration.bindingConfiugurations[i].samplerPointer;
		}

		for (int i = 0; i < configuration.bindingConfiugurations.size(); i++)
		{
			if (configuration.bindingConfiugurations[i].type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
				descriptorUinformWriteInfos.push_back(std::make_pair(configuration.bindingConfiugurations[i].bindingId, std::make_pair(configuration.bindingConfiugurations[i].multiuse, configuration.bindingConfiugurations[i].buferIndex)));
			}
			else if (configuration.bindingConfiugurations[i].type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
				descriptorsamplerWriteInfos.push_back(std::make_pair(configuration.bindingConfiugurations[i].bindingId, configuration.bindingConfiugurations[i].multiuse));
			}
			else if (configuration.bindingConfiugurations[i].type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) {
				descriptorUinformDynamicWriteInfos.push_back(std::make_pair(configuration.bindingConfiugurations[i].bindingId, std::make_pair(configuration.bindingConfiugurations[i].multiuse, configuration.bindingConfiugurations[i].buferIndex)));
			}
			else if (configuration.bindingConfiugurations[i].type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
				descriptorUinformDynamicWriteInfos.push_back(std::make_pair(configuration.bindingConfiugurations[i].bindingId, std::make_pair(configuration.bindingConfiugurations[i].multiuse, configuration.bindingConfiugurations[i].buferIndex)));
			}


			
		}

		isInit = true;
	};
	void create(VkDevice device, VkAllocationCallbacks *allucator) {
		if (!isInit)
			std::logic_error("is not Init");
		if (isCreated)
			std::logic_error("is already Created");

		this->device = device;
		this->allucator = allucator;

		VkShaderModuleCreateInfo shaderCreateInfo;
		shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderCreateInfo.pNext = nullptr;
		shaderCreateInfo.flags = 0;
		shaderCreateInfo.codeSize = shaderCodeSpv.size();
		shaderCreateInfo.pCode = (uint32_t*)shaderCodeSpv.data();

		testErrorCode(vkCreateShaderModule(device, &shaderCreateInfo, allucator, &shaderModule));

		isCreated = true;
	};
	void destroy() {
		if (isCreated) {

			shaderModule = VK_NULL_HANDLE;
			isCreated = false;

			vkDestroyShaderModule(device, shaderModule, allucator);

			allucator = nullptr;
			device = nullptr;
		}
	};

	VkShaderStageFlagBits getStageBits() {
		if (!isInit)
			throw std::logic_error("is not init");
		return configuration.shaderStageFlagBits;
	}
	VkShaderModule getModul() {
		if (!isCreated)
			throw std::logic_error("is not created");
		return shaderModule;
	};
	const char* getShaderMainFunction() {
		if (!isInit)
			throw std::logic_error("is not init");
		return configuration.shaderMainFunction;
	}
	std::vector<VkDescriptorSetLayoutBinding> getDescriptorLayoutBindings() {
		if (!isInit)
			throw std::logic_error("is not created");
		return descriptorLayoutBindings;
	}
	std::vector<std::pair<uint32_t, std::pair<uint32_t, uint32_t>>> getUniformDynamicBindings() {
		if (!isInit)
			throw std::logic_error("is not created");
		return descriptorUinformDynamicWriteInfos;
	}
	std::vector<std::pair<uint32_t, std::pair<uint32_t, uint32_t>>> getUniformBindings() {
		if (!isInit)
			throw std::logic_error("is not created");
		return descriptorUinformWriteInfos;
	}
	std::vector<std::pair<uint32_t, uint32_t>> getSamplerBindings() {
		if (!isInit)
			throw std::logic_error("is not created");
		return descriptorsamplerWriteInfos;
	}
};

class Pipeline {
public:
	struct Configuration
	{
		std::vector<uint32_t> shaderIds;
		VkPolygonMode poligonMode = VK_POLYGON_MODE_FILL;
		uint32_t renderPassId = 0;
	};
private:
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	VkVertexInputBindingDescription vertexBindingDescriptions;
	std::vector<VkVertexInputAttributeDescription> vertexAtributDescriptions;
	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo;
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo;
	VkPipelineViewportStateCreateInfo viewportStateCreateInfo;
	VkPipelineRasterizationStateCreateInfo rasterisationCreateInfo;
	VkPipelineMultisampleStateCreateInfo multisamplerCreateInfo;
	VkPipelineDepthStencilStateCreateInfo depthStencileStateCreateInfo;
	VkPipelineColorBlendAttachmentState colorBlendAttachmend;
	VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo;
	std::vector<VkDynamicState> dynamicStates;
	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo;
	VkPushConstantRange pushConstRange;

	VkViewport viewport;
	VkRect2D scissore;

	VkPipeline pipeline = VK_NULL_HANDLE;
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

	VkDevice device = VK_NULL_HANDLE;

	bool isInit = false;
	bool isCreated = false;

	VkAllocationCallbacks* allucator;

public:
	Pipeline() {};
	~Pipeline() { destroy(); };

	void addShaderStage(VkShaderStageFlagBits stageFlag, const char* mainName, VkShaderModule shaderModul, VkSpecializationInfo* specialisationInfo = nullptr) {

		shaderStages.push_back(VkPipelineShaderStageCreateInfo());
		VkPipelineShaderStageCreateInfo* vertShaderCreateInfo = &shaderStages[shaderStages.size() - 1];
		vertShaderCreateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderCreateInfo->pNext = nullptr;
		vertShaderCreateInfo->flags = 0;
		vertShaderCreateInfo->stage = stageFlag;
		vertShaderCreateInfo->module = shaderModul;
		vertShaderCreateInfo->pName = mainName;					//startpunkt in shader
		vertShaderCreateInfo->pSpecializationInfo = nullptr;	//constanten in shader setzen
	}

	void changePoligonMode(VkPolygonMode poligonMode) {
		rasterisationCreateInfo.polygonMode = poligonMode;
	}

	void init(uint32_t width, uint32_t height)
	{
		vertexBindingDescriptions = Vertex::getBindingDescription();
		vertexAtributDescriptions = Vertex::getAttributDescriptions();

		vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputCreateInfo.pNext = nullptr;
		vertexInputCreateInfo.flags = 0;
		vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
		vertexInputCreateInfo.pVertexBindingDescriptions = &vertexBindingDescriptions;				//platz pro vertex //per vertex oder per instanz		//mesh instanzing
		vertexInputCreateInfo.vertexAttributeDescriptionCount = vertexAtributDescriptions.size();
		vertexInputCreateInfo.pVertexAttributeDescriptions = vertexAtributDescriptions.data();

		inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyCreateInfo.pNext = nullptr;
		inputAssemblyCreateInfo.flags = 0;
		inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;	//liste an dreieck
		inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;				//restart des zeichnens

		//mehrere render ziehle
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = width;
		viewport.height = height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		//kann elemente des screens wegschneiden alles was nicht drin ist wird weg geschnitten
		scissore.offset.x = 0;
		scissore.offset.y = 0;
		scissore.extent.width = width;
		scissore.extent.height = height;

		viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateCreateInfo.pNext = nullptr;
		viewportStateCreateInfo.flags = 0;
		viewportStateCreateInfo.viewportCount = 1;
		viewportStateCreateInfo.pViewports = &viewport;	//array
		viewportStateCreateInfo.scissorCount = 1;
		viewportStateCreateInfo.pScissors = &scissore;	//array

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

		multisamplerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisamplerCreateInfo.pNext = nullptr;
		multisamplerCreateInfo.flags = 0;
		multisamplerCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;		//AAx1
		multisamplerCreateInfo.sampleShadingEnable = VK_FALSE;						//AA aus
		multisamplerCreateInfo.minSampleShading = 1.0f;
		multisamplerCreateInfo.pSampleMask = nullptr;
		multisamplerCreateInfo.alphaToCoverageEnable = VK_TRUE;
		multisamplerCreateInfo.alphaToOneEnable = VK_FALSE;

		colorBlendAttachmend.blendEnable = VK_TRUE;
		colorBlendAttachmend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachmend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachmend.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachmend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachmend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachmend.alphaBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachmend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		depthStencileStateCreateInfo = DepthImage::getDepthStencilStateCreateInfoOpaque();

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

		dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateCreateInfo.pNext = nullptr;
		dynamicStateCreateInfo.flags = 0;
		dynamicStateCreateInfo.dynamicStateCount = dynamicStates.size();
		dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

		pushConstRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstRange.offset = 0;
		pushConstRange.size = sizeof(VkBool32);
		isInit = true;
	};
	void create(VkDevice device, VkAllocationCallbacks* allucator, VkRenderPass renderPass, VkDescriptorSetLayout descriptorSetLayout)
	{
		if (!isInit)
			throw std::logic_error("call init First");
		if (isCreated)
			throw std::logic_error("was already Created");

		this->device = device;
		this->allucator = allucator;

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.pNext = nullptr;
		pipelineLayoutCreateInfo.flags = 0;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstRange;

		testErrorCode(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, allucator, &pipelineLayout));

		VkGraphicsPipelineCreateInfo pipelineCreateInfo;
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.pNext = nullptr;
		pipelineCreateInfo.flags = 0;
		pipelineCreateInfo.stageCount = shaderStages.size();
		pipelineCreateInfo.pStages = shaderStages.data();
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

		testErrorCode(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, allucator, &pipeline));
		isCreated = true;
	};
	void destroy() {
		if (isCreated) {
			vkDestroyPipeline(device, pipeline, allucator);
			vkDestroyPipelineLayout(device, pipelineLayout, allucator);

			device = VK_NULL_HANDLE;
			allucator = nullptr;
			isCreated = false;
		}
	};

	VkPipeline getPipeline() {
		if (!isCreated)
			throw std::logic_error("is not Created");
		return pipeline;
	}
	VkPipelineLayout getLayout() {
		if (!isCreated)
			throw std::logic_error("is not Created");
		return pipelineLayout;
	}

};

class Buffer {
public:
	struct Configuration
	{
		VkBufferUsageFlags bufferusage;
		uint64_t maxBufferSize;
		uint32_t singleBufferSize;
		bool isInGrakaMemory;
	};
private:
	Configuration configuration;

	VkDevice device = VK_NULL_HANDLE;
	VkAllocationCallbacks* allucator = nullptr;
	VkQueue queue = VK_NULL_HANDLE;
	VkPhysicalDevice physikalDevice = VK_NULL_HANDLE;
	VkCommandPool commandPool = VK_NULL_HANDLE;

	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory bufferMem = VK_NULL_HANDLE;

	uint64_t pointer = 0;

	std::vector<uint32_t> savedData;

	bool dataChanged = true;
	bool resizeNeeded = true;

	void createBuffer(VkDeviceSize deviceSize, VkBufferUsageFlags usageFlags, VkBuffer& buffer, VkMemoryPropertyFlags memPropFlags, VkDeviceMemory& deviceMem) {
		VkBufferCreateInfo bufferInfo = createBufferCreateInfo();
		bufferInfo.size = deviceSize;
		bufferInfo.usage = usageFlags;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		testErrorCode(vkCreateBuffer(device, &bufferInfo, allucator, &buffer));

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		VkMemoryAllocateInfo memAlkinfo;
		memAlkinfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memAlkinfo.pNext = nullptr;
		memAlkinfo.allocationSize = memRequirements.size;
		memAlkinfo.memoryTypeIndex = findMemoryTypeIndex(physikalDevice, memRequirements.memoryTypeBits, memPropFlags);

		testErrorCode(vkAllocateMemory(device, &memAlkinfo, allucator, &deviceMem));

		testErrorCode(vkBindBufferMemory(device, buffer, deviceMem, 0));
	}
	void removeBuffer(VkBuffer& buffer, VkDeviceMemory& deviceMem) {
		vkFreeMemory(device, deviceMem, allucator);
		vkDestroyBuffer(device, buffer, allucator);
	}
	void copyBuffer(VkBuffer src, VkBuffer dest, VkDeviceSize size) {
		auto cBuffer = startRecordingSingleTimeBuffer(device, commandPool);

		VkBufferCopy cp;
		cp.srcOffset = 0;
		cp.dstOffset = 0;
		cp.size = size;

		vkCmdCopyBuffer(cBuffer, src, dest, 1, &cp);

		endRecordingSingleTimeBuffer(device, queue, commandPool, cBuffer);
	}

	void removeBufferAndMem() {
		if(buffer != VK_NULL_HANDLE)
			removeBuffer(buffer, bufferMem);
		bufferMem = VK_NULL_HANDLE;
		buffer = VK_NULL_HANDLE;
	}
	void createBufferAndMem() {
		uint32_t dataSize = savedData.size() * sizeof(uint32_t);
		if (configuration.isInGrakaMemory)
			createBuffer(dataSize, configuration.bufferusage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, bufferMem);
		else
			createBuffer(dataSize, configuration.bufferusage, buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, bufferMem);
	}

	void updateBufferToMem() {
		if (resizeNeeded) {
			removeBufferAndMem();
			createBufferAndMem();
			resizeNeeded = false;
		}
		uint32_t dataSize = savedData.size() * sizeof(uint32_t);
		if (configuration.isInGrakaMemory) {
			VkBuffer sBuffer;
			VkDeviceMemory sBufferMem;
			createBuffer(dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, sBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sBufferMem);

			void* rawData;
			testErrorCode(vkMapMemory(device, sBufferMem, 0, dataSize, 0, &rawData));
			memcpy(rawData, savedData.data(), dataSize);
			vkUnmapMemory(device, sBufferMem);

			copyBuffer(sBuffer, buffer, dataSize);

			removeBuffer(sBuffer, sBufferMem);
		}
		else {
			void* data;
			testErrorCode(vkMapMemory(device, bufferMem, 0, dataSize, 0, &data));
			memcpy(data, savedData.data(), dataSize);
			vkUnmapMemory(device, bufferMem);
		}
	}

public:
	void setCopyPointer(uint64_t pointer) {
		this->pointer = pointer;
	}
	void setData(void* data, uint64_t dataSize) {
		uint64_t requiredSize = pointer + dataSize;
		if (requiredSize > configuration.maxBufferSize * configuration.singleBufferSize)	throw std::logic_error("max size reached");
		dataChanged = true;
		if (requiredSize > savedData.size()) {
			savedData.resize(requiredSize);
			resizeNeeded = true;
		}
		memcpy(savedData.data() + pointer, data, dataSize);
		pointer += dataSize;
	}
	void clearDataAfterPointer() {
		savedData.resize(pointer);
	}
	void clearDataAfterPointer(uint64_t pointer) {
		savedData.resize(pointer);
		if (this->pointer = pointer) this->pointer = pointer;
	}
	void clearAllData() {
		pointer = 0;
		savedData.resize(0);
	}

	bool updateBuffer() {
		if (dataChanged) {
			updateBufferToMem();
			dataChanged = false;
			return true;
		}
		return false;
	}

	void Init(Configuration& configuration) {
		this->configuration = configuration;
	}
	void Create(VkDevice device, VkAllocationCallbacks* allucator, VkQueue queue, VkPhysicalDevice physikalDevice, VkCommandPool commandPool) {
		this->device = device;
		this->allucator = allucator;
		this->queue = queue;
		this->physikalDevice = physikalDevice;
		this->commandPool = commandPool;
	}
	void Destroy() {
		removeBufferAndMem();
		clearAllData();
		device = VK_NULL_HANDLE;
		allucator = nullptr;
		queue = VK_NULL_HANDLE;
		physikalDevice = VK_NULL_HANDLE;
		commandPool = VK_NULL_HANDLE;
	}

	VkBuffer* getBuffer() {
		return &buffer;
	}
	uint64_t getInSize() {
		return savedData.size() / (float)configuration.singleBufferSize;
	}
	uint64_t getOutSize() {
		return savedData.size() * sizeof(uint32_t);
	}
};

struct RenderPassConfiguration
{
	struct attachmentConfiguration {
		VkAttachmentDescriptionFlags flags = 0;
		VkFormat imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
		VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
		VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		VkAttachmentLoadOp stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		VkAttachmentStoreOp stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	};
	struct RenderDraw {
		bool usePushConstant;
		uint32_t pushConstantSize;
		const void* pushConstantData;
		VkShaderStageFlagBits shaderStage;
		uint32_t pipelineIndex = 0;
		VkPipelineBindPoint pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		VkViewport viewport;				//sind von 0 bis 2048 zur image size gemappt // 0 = 0% 1024 = 50% 2048 = 100% // minDepth und maxDepth ausgenommen
		std::vector< VkRect2D> scissores;	//sind von 0 bis 2048 zur image size gemappt // 0 = 0% 1024 = 50% 2048 = 100%

		uint32_t vertexBufferIndex;
		std::vector<uint64_t> vertexOffset;
		uint32_t indexBufferIndex = 0;
		uint64_t indexOffset = 0;

		uint32_t instanceCount = 1;
		uint32_t instanceOffset = 0;
	};
	std::vector<RenderPassConfiguration::attachmentConfiguration> attachments;
	bool usesDepth = true;
	//uint32_t depthImageId = 0;		//kp wo ich das angeben muss

	VkRect2D area;		//sind von 0 bis 2048 zur image size gemappt 0 = 0% 1024 = 50% 2048 = 100%
	std::vector<VkClearValue> clearValues;

	std::vector<RenderDraw> renderSessions;
};

class Renderer
{
public:
#pragma region einstellungsStructs
	struct Configuration {

		GLFWwindow* window;
		VkAllocationCallbacks* allucator = nullptr;
		const char* appName = "VulcanTest";
		uint32_t	appVersion = VK_MAKE_VERSION(0, 0, 0);
		const char* engName = "friendly-octo-pancake-Engine";
		uint32_t	engVersion = VK_MAKE_VERSION(0, 1, 0);
		uint32_t	apiVersion = VK_API_VERSION_1_2;
		std::vector<const char*> layers;
		uint32_t queueCount = 4;
		VkPhysicalDeviceFeatures usedFeatures = {};
		std::vector<const char*> deviceExtensions;
		uint32_t screenHeight;
		uint32_t screenWidth;
		uint32_t minImageCount = 3;
		VkFormat imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
		VkColorSpaceKHR imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
		VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		VkSharingMode imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VkSurfaceTransformFlagBitsKHR preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		VkPresentModeKHR presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
		VkBool32 clipped = VK_TRUE;
		//VkAttachmentDescriptionFlags flags = 0;
		//VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		//VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		//VkAttachmentLoadOp stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		//VkAttachmentStoreOp stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;

		std::vector<Shader::Configuration> shaderConfigurations;
		std::vector<Image::Configuration> imageConfigurations;
		std::vector<Pipeline::Configuration> pipelineConfigurations;
		std::vector<RenderPassConfiguration> renderPassConfigurations;
	};
#pragma endregion

public:
#pragma region PrivateVarriablen
	Configuration consturctionInfo;

private:

#pragma region VulcanVarriablen
	VkInstance instance = VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	uint32_t queueFamilyIndex;
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	std::vector<VkImageView> imageViews;
	VkDescriptorSetLayout descriptorLayout = VK_NULL_HANDLE;
	VkCommandPool commandPool = VK_NULL_HANDLE;
	VkQueue queue = VK_NULL_HANDLE;
	VkFramebuffer* framebuffers = nullptr;
	VkCommandBuffer* commandBuffers = VK_NULL_HANDLE;
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

	VkSemaphore semaphoreImgAvailable;
	VkSemaphore semaphoreImgRendered;

	uint32_t renderPassId = 0;
	std::vector<VkRenderPass> renderPasses;

	std::vector<Shader> shaders;

	std::vector<Pipeline> pipelines;

	DepthImage depthImage;

	std::vector<Image> images;

	std::vector<Buffer> buffers;

	bool isCreated = false;

#pragma endregion
#pragma endregion

	void initiationCodePart0() {
		VkApplicationInfo appInfo;
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pNext = nullptr;
		appInfo.pApplicationName = consturctionInfo.appName;
		appInfo.applicationVersion = consturctionInfo.appVersion;
		appInfo.pEngineName = consturctionInfo.engName;
		appInfo.engineVersion = consturctionInfo.engVersion;
		appInfo.apiVersion = consturctionInfo.apiVersion;

		uint32_t ammountOfGLFWExtensions;

		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&ammountOfGLFWExtensions);

		VkInstanceCreateInfo instanceInfo;
		instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceInfo.pNext = nullptr;
		instanceInfo.flags = 0;
		instanceInfo.pApplicationInfo = &appInfo;
		instanceInfo.enabledLayerCount = consturctionInfo.layers.size();
		instanceInfo.ppEnabledLayerNames = consturctionInfo.layers.data();
		instanceInfo.enabledExtensionCount = ammountOfGLFWExtensions;
		instanceInfo.ppEnabledExtensionNames = glfwExtensions;

		testErrorCode(vkCreateInstance(&instanceInfo, consturctionInfo.allucator, &instance));

		testErrorCode(glfwCreateWindowSurface(instance, consturctionInfo.window, consturctionInfo.allucator, &surface));

		auto physikalDevices = getAllPhysikalDevices(instance);

		physicalDevice = physikalDevices[0];						//auswahl verfahren verbessern
		queueFamilyIndex = 0;											//auswahlVerbessern
																	//queueCount testen


		float quePrios[] = { 1.0f,1.0f,1.0f,1.0f };					//anhand der QueueCount Füllen

		VkBool32 supportsSwapchain;
		testErrorCode(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, 0, surface, &supportsSwapchain));

		if (!supportsSwapchain)
			__debugbreak();

		VkDeviceQueueCreateInfo deviceQueCreateInfo;
		deviceQueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		deviceQueCreateInfo.pNext = nullptr;
		deviceQueCreateInfo.flags = 0;
		deviceQueCreateInfo.queueFamilyIndex = queueFamilyIndex;
		deviceQueCreateInfo.queueCount = consturctionInfo.queueCount;
		deviceQueCreateInfo.pQueuePriorities = quePrios;

		consturctionInfo.usedFeatures.samplerAnisotropy = VK_TRUE;
		consturctionInfo.usedFeatures.fillModeNonSolid = VK_TRUE;

		consturctionInfo.deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		VkDeviceCreateInfo deviceCreateInfo;
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pNext = nullptr;
		deviceCreateInfo.flags = 0;
		deviceCreateInfo.queueCreateInfoCount = 1;
		deviceCreateInfo.pQueueCreateInfos = &deviceQueCreateInfo;
		deviceCreateInfo.enabledLayerCount = 0;
		deviceCreateInfo.ppEnabledLayerNames = nullptr;
		deviceCreateInfo.enabledExtensionCount = consturctionInfo.deviceExtensions.size();
		deviceCreateInfo.ppEnabledExtensionNames = consturctionInfo.deviceExtensions.data();
		deviceCreateInfo.pEnabledFeatures = &consturctionInfo.usedFeatures;

		testErrorCode(vkCreateDevice(physicalDevice, &deviceCreateInfo, consturctionInfo.allucator, &device));

		vkGetDeviceQueue(device, 0, 0, &queue);

		shaders.clear();
		for (int i = 0; i < consturctionInfo.shaderConfigurations.size(); i++) {
			shaders.push_back(Shader());
			shaders[i].init(consturctionInfo.shaderConfigurations[i]);
			shaders[i].create(device, consturctionInfo.allucator);
		}

		images.clear();
		for (int i = 0; i < consturctionInfo.imageConfigurations.size(); i++)
		{
			images.push_back(Image());
			images[i].load(consturctionInfo.imageConfigurations[i]);
		}
	}

	void resizeCodePart0() {

		//check if minImageCount Ok
		//check if imageFormat OK
		//check if imageColorSpace Ok
		//check if presentMode Ok

		VkSwapchainCreateInfoKHR swapChainCreateInfo;
		swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapChainCreateInfo.pNext = nullptr;
		swapChainCreateInfo.flags = 0;
		swapChainCreateInfo.surface = surface;
		swapChainCreateInfo.minImageCount = consturctionInfo.minImageCount;
		swapChainCreateInfo.imageFormat = consturctionInfo.imageFormat;
		swapChainCreateInfo.imageColorSpace = consturctionInfo.imageColorSpace;
		swapChainCreateInfo.imageExtent = { consturctionInfo.screenWidth, consturctionInfo.screenHeight };
		swapChainCreateInfo.imageArrayLayers = 1;
		swapChainCreateInfo.imageUsage = consturctionInfo.imageUsage;				//zeichen mode nicht einfach nur zum speichern
		swapChainCreateInfo.imageSharingMode = consturctionInfo.imageSharingMode;
		swapChainCreateInfo.queueFamilyIndexCount = 0;
		swapChainCreateInfo.pQueueFamilyIndices = nullptr;
		swapChainCreateInfo.preTransform = consturctionInfo.preTransform;			//hier mirroring oä
		swapChainCreateInfo.compositeAlpha = consturctionInfo.compositeAlpha;		//durchsichtiges fenster hier möglich
		swapChainCreateInfo.presentMode = consturctionInfo.presentMode;
		swapChainCreateInfo.clipped = consturctionInfo.clipped;					//geklippte pixel wegschmeißen
		swapChainCreateInfo.oldSwapchain = swapchain;						//alte swapchain hier angeben z.b. bei resize

		testErrorCode(vkCreateSwapchainKHR(device, &swapChainCreateInfo, consturctionInfo.allucator, &swapchain));

		uint32_t imageViewCount;
		testErrorCode(vkGetSwapchainImagesKHR(device, swapchain, &imageViewCount, nullptr));

		VkImage* swapchainImages = new VkImage[imageViewCount];
		testErrorCode(vkGetSwapchainImagesKHR(device, swapchain, &imageViewCount, swapchainImages));

		imageViews.resize(imageViewCount);
		for (int i = 0; i < imageViewCount; i++) {
			createImageView(device, swapchainImages[i], consturctionInfo.imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, imageViews[i]);
		}

		delete[] swapchainImages;

	}

	void initiationCodePart1() {

		for (int i = 0; i < consturctionInfo.renderPassConfigurations.size(); i++) renderPasses.push_back(VkRenderPass());

		for (int i = 0; i < consturctionInfo.renderPassConfigurations.size(); i++) {

			std::vector<VkAttachmentDescription> attachments;
			std::vector< VkAttachmentReference> refferenzes;
			VkAttachmentReference depthRefferenzes;
			VkAttachmentDescription* attechmentDesription;
			VkAttachmentReference* attechmentRefference;

			for (int j = 0; j < consturctionInfo.renderPassConfigurations[i].attachments.size(); j++) {
				attachments.push_back(VkAttachmentDescription());
				refferenzes.push_back(VkAttachmentReference());
			}
			if (consturctionInfo.renderPassConfigurations[i].usesDepth) {
				attachments.push_back(DepthImage::getdepthAttachment(physicalDevice));
				depthRefferenzes.attachment = attachments.size() - 1;
				depthRefferenzes.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			}
			for (int j = 0; j < consturctionInfo.renderPassConfigurations[i].attachments.size(); j++) {
				attechmentDesription = &attachments[j];
				attechmentRefference = &refferenzes[j];
				attechmentDesription->flags = consturctionInfo.renderPassConfigurations[i].attachments[j].flags;							//speicher überlappung möglich
				attechmentDesription->format = consturctionInfo.renderPassConfigurations[i].attachments[j].imageFormat;
				attechmentDesription->samples = consturctionInfo.renderPassConfigurations[i].attachments[j].samples;																		//kp wie das funktioniert
				attechmentDesription->loadOp = consturctionInfo.renderPassConfigurations[i].attachments[j].loadOp;						//was pasiert mit werten nach dem laden
				attechmentDesription->storeOp = consturctionInfo.renderPassConfigurations[i].attachments[j].storeOp;						//was pasiert mit dem speichen
				attechmentDesription->stencilLoadOp = consturctionInfo.renderPassConfigurations[i].attachments[j].stencilLoadOp;		//fog of ware
				attechmentDesription->stencilStoreOp = consturctionInfo.renderPassConfigurations[i].attachments[j].stencilStoreOp;
				attechmentDesription->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			//format vor laden
				attechmentDesription->finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;		//format nach laden

				attechmentRefference->attachment = j;
				attechmentRefference->layout = consturctionInfo.renderPassConfigurations[i].attachments[j].layout;
			}

			VkSubpassDescription subpassDescription;
			subpassDescription.flags = 0;
			subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpassDescription.inputAttachmentCount = 0;									//inputs
			subpassDescription.pInputAttachments = nullptr;
			subpassDescription.colorAttachmentCount = refferenzes.size();
			subpassDescription.pColorAttachments = refferenzes.data();
			subpassDescription.pResolveAttachments = nullptr;
			subpassDescription.pDepthStencilAttachment = &depthRefferenzes;
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
			renderPassCreateInfo.attachmentCount = attachments.size();
			renderPassCreateInfo.pAttachments = attachments.data();
			renderPassCreateInfo.subpassCount = 1;
			renderPassCreateInfo.pSubpasses = &subpassDescription;
			renderPassCreateInfo.dependencyCount = 1;
			renderPassCreateInfo.pDependencies = &subpassDependency;

			testErrorCode(vkCreateRenderPass(device, &renderPassCreateInfo, consturctionInfo.allucator, &renderPasses[i]));
		}

		std::vector< VkDescriptorSetLayoutBinding> layoutBinding;

		for (int i = 0; i < shaders.size(); i++) {
			auto bindings = shaders[i].getDescriptorLayoutBindings();
			for (int j = 0; j < bindings.size(); j++)
				layoutBinding.push_back(bindings[j]);
		}

		VkDescriptorSetLayoutCreateInfo desSetLayoutCreateInfo;
		desSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		desSetLayoutCreateInfo.pNext = nullptr;
		desSetLayoutCreateInfo.flags = 0;
		desSetLayoutCreateInfo.bindingCount = layoutBinding.size();
		desSetLayoutCreateInfo.pBindings = layoutBinding.data();

		testErrorCode(vkCreateDescriptorSetLayout(device, &desSetLayoutCreateInfo, consturctionInfo.allucator, &descriptorLayout));

		Pipeline* pipeline;

		for (int i = 0; i < consturctionInfo.pipelineConfigurations.size(); i++) {
			pipelines.push_back(Pipeline());
			pipeline = &pipelines[i];
			pipeline->init(consturctionInfo.screenWidth, consturctionInfo.screenHeight);
			for (int j = 0; j < consturctionInfo.pipelineConfigurations[i].shaderIds.size(); j++)
				pipeline->addShaderStage(shaders[j].getStageBits(), shaders[j].getShaderMainFunction(), shaders[j].getModul());

			pipeline->changePoligonMode(consturctionInfo.pipelineConfigurations[i].poligonMode);

			pipeline->create(device, consturctionInfo.allucator, renderPasses[consturctionInfo.pipelineConfigurations[i].renderPassId], descriptorLayout);	
		}

		VkCommandPoolCreateInfo commandPollCreateInfo;
		commandPollCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPollCreateInfo.pNext = nullptr;
		commandPollCreateInfo.flags = 0;
		commandPollCreateInfo.queueFamilyIndex = queueFamilyIndex;

		testErrorCode(vkCreateCommandPool(device, &commandPollCreateInfo, consturctionInfo.allucator, &commandPool));
	}

	void resizeCodePart1() {
		depthImage.create(device, physicalDevice, commandPool, queue, consturctionInfo.screenWidth, consturctionInfo.screenHeight);

		framebuffers = new VkFramebuffer[imageViews.size()];

		VkFramebufferCreateInfo framebufferCreateInfo;
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.pNext = nullptr;
		framebufferCreateInfo.flags = 0;
		framebufferCreateInfo.renderPass = renderPasses[renderPassId];
		framebufferCreateInfo.width = consturctionInfo.screenWidth;
		framebufferCreateInfo.height = consturctionInfo.screenHeight;
		framebufferCreateInfo.layers = 1;

		std::vector<VkImageView> attachmentViews;

		for (int i = 0; i < imageViews.size(); i++) {
			attachmentViews.clear();
			attachmentViews.push_back(imageViews[i]);
			attachmentViews.push_back(depthImage.getImageView());																//ToDo au derzeitigen renderpass Anpassen
			framebufferCreateInfo.attachmentCount = attachmentViews.size();
			framebufferCreateInfo.pAttachments = attachmentViews.data();
			testErrorCode(vkCreateFramebuffer(device, &framebufferCreateInfo, consturctionInfo.allucator, &(framebuffers[i])));
		}

		VkCommandBufferAllocateInfo commandBufferAllocateInfo;
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.pNext = nullptr;
		commandBufferAllocateInfo.commandPool = commandPool;
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocateInfo.commandBufferCount = imageViews.size();

		commandBuffers = new VkCommandBuffer[imageViews.size()];

		testErrorCode(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, commandBuffers));
	}

	void initiationCodePart2() {


		for (int i = 0; i < images.size(); i++)
			images[i].upload(device, consturctionInfo.allucator, physicalDevice, commandPool, queue);

		for (int i = 0; i < buffers.size(); i++) {
			buffers[i].Create(device, consturctionInfo.allucator, queue, physicalDevice, commandPool);
			buffers[i].updateBuffer();
		}

		VkDescriptorPoolSize dUSize;							//ToDo auf die anzahl der uniforms anpassen
		dUSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		dUSize.descriptorCount = 1;

		VkDescriptorPoolSize dUDSize;							//ToDo auf die anzahl der storrages anpassen
		dUDSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		dUDSize.descriptorCount = 1;

		VkDescriptorPoolSize dSSize;							//ToDo auf die anzahl der storrages anpassen
		dSSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		dSSize.descriptorCount = 1;

		VkDescriptorPoolSize dSDSize;							//ToDo auf die anzahl der storrages anpassen
		dSDSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
		dSDSize.descriptorCount = 1;

		VkDescriptorPoolSize dSamplerSize;						//ToDo auf die anzahl der images anpassen
		dSamplerSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		dSamplerSize.descriptorCount = 2;									//anzahl der texturen

		std::vector< VkDescriptorPoolSize> poolSizeA;
		poolSizeA.push_back(dUSize);
		poolSizeA.push_back(dUDSize);
		poolSizeA.push_back(dSSize);
		poolSizeA.push_back(dSDSize);
		poolSizeA.push_back(dSamplerSize);

		VkDescriptorPoolCreateInfo dPollCreateInfo;
		dPollCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		dPollCreateInfo.pNext = nullptr;
		dPollCreateInfo.flags = 0;
		dPollCreateInfo.maxSets = 1;
		dPollCreateInfo.poolSizeCount = poolSizeA.size();
		dPollCreateInfo.pPoolSizes = poolSizeA.data();

		testErrorCode(vkCreateDescriptorPool(device, &dPollCreateInfo, consturctionInfo.allucator, &descriptorPool));

		VkDescriptorSetAllocateInfo desSAllocInfo;
		desSAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		desSAllocInfo.pNext = nullptr;
		desSAllocInfo.descriptorPool = descriptorPool;
		desSAllocInfo.descriptorSetCount = 1;
		desSAllocInfo.pSetLayouts = &descriptorLayout;

		testErrorCode(vkAllocateDescriptorSets(device, &desSAllocInfo, &descriptorSet));
	}

	void bufferChangeCode0() {
		std::vector<VkWriteDescriptorSet> writeSet;
		std::vector<VkDescriptorBufferInfo> tempBufferInfo;
		std::vector<VkDescriptorImageInfo> tempImageInfo;

		VkWriteDescriptorSet* wDS;
		VkDescriptorBufferInfo* dBI;
		VkDescriptorImageInfo* dII;

		for (int i = 0; i < shaders.size(); i++) {
			auto allUniformInfos = shaders[i].getUniformBindings();
			auto allUniformDynamicInfos = shaders[i].getUniformDynamicBindings();
			auto allSamplerInfos = shaders[i].getSamplerBindings();

			for (int i = 0; i < allUniformInfos.size(); i++) {
				tempBufferInfo.push_back(VkDescriptorBufferInfo());
				writeSet.push_back(VkWriteDescriptorSet());
			}
			for (int i = 0; i < allUniformDynamicInfos.size(); i++) {
				tempBufferInfo.push_back(VkDescriptorBufferInfo());
				writeSet.push_back(VkWriteDescriptorSet());
			}
			for (int i = 0; i < allSamplerInfos.size(); i++) {
				tempImageInfo.push_back(VkDescriptorImageInfo());
				writeSet.push_back(VkWriteDescriptorSet());
			}
		}

		uint32_t writerIndex = 0;
		uint32_t uniformIndex = 0;
		uint32_t samplerIndex = 0;

		for (int i = 0; i < shaders.size(); i++) {
			auto allUniformInfos = shaders[i].getUniformBindings();
			auto allUniformDynamicInfos = shaders[i].getUniformDynamicBindings();
			auto allSamplerInfos = shaders[i].getSamplerBindings();

			for (int i = 0; i < allUniformInfos.size(); i++) {
				dBI = &tempBufferInfo[uniformIndex];
				wDS = &writeSet[writerIndex];
				writerIndex++;
				uniformIndex++;

				dBI->buffer = *buffers[allUniformInfos[i].second.second].getBuffer();
				dBI->offset = 0;
				dBI->range = allUniformInfos[i].second.first;

				wDS->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				wDS->pNext = nullptr;
				wDS->dstSet = descriptorSet;
				wDS->dstBinding = allUniformInfos[i].first;
				wDS->dstArrayElement = 0;
				wDS->descriptorCount = 1;
				wDS->descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				wDS->pImageInfo = nullptr;
				wDS->pBufferInfo = dBI;
				wDS->pTexelBufferView = nullptr;
			}
			for (int i = 0; i < allUniformDynamicInfos.size(); i++) {
				dBI = &tempBufferInfo[uniformIndex];
				wDS = &writeSet[writerIndex];
				writerIndex++;
				uniformIndex++;

				dBI->buffer = *buffers[allUniformDynamicInfos[i].second.second].getBuffer();
				dBI->offset = 0;
				dBI->range = allUniformDynamicInfos[i].second.first;

				wDS->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				wDS->pNext = nullptr;
				wDS->dstSet = descriptorSet;
				wDS->dstBinding = allUniformDynamicInfos[i].first;
				wDS->dstArrayElement = 0;
				wDS->descriptorCount = 1;
				wDS->descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				wDS->pImageInfo = nullptr;
				wDS->pBufferInfo = dBI;
				wDS->pTexelBufferView = nullptr;
			}
			for (int i = 0; i < allSamplerInfos.size(); i++) {
				dII = &tempImageInfo[samplerIndex];
				wDS = &writeSet[writerIndex];
				writerIndex++;
				samplerIndex++;

				dII->sampler = images[allSamplerInfos[i].second].getSampler();
				dII->imageView = images[allSamplerInfos[i].second].getImageView();
				dII->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				wDS->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				wDS->pNext = nullptr;
				wDS->dstSet = descriptorSet;
				wDS->dstBinding = allSamplerInfos[i].first;
				wDS->dstArrayElement = 0;
				wDS->descriptorCount = 1;
				wDS->descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				wDS->pImageInfo = dII;
				wDS->pBufferInfo = nullptr;
				wDS->pTexelBufferView = nullptr;
			}
		}

		vkUpdateDescriptorSets(device, writeSet.size(), writeSet.data(), 0, nullptr);
	}

	void resizeCodePart2() {
		VkCommandBufferBeginInfo commandBufferBeginInfo;
		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandBufferBeginInfo.pNext = nullptr;
		commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		commandBufferBeginInfo.pInheritanceInfo = nullptr;

		VkRenderPassBeginInfo renderPassBeginInfo;
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext = nullptr;

		for (int i = 0; i < imageViews.size(); i++) {
			testErrorCode(vkBeginCommandBuffer(commandBuffers[i], &commandBufferBeginInfo));
			renderPassBeginInfo.framebuffer = framebuffers[i];

			renderPassBeginInfo.renderPass = renderPasses[renderPassId];
			renderPassBeginInfo.renderArea.offset.x = consturctionInfo.renderPassConfigurations[renderPassId].area.offset.x * ((float)consturctionInfo.screenWidth / (float)2048);
			renderPassBeginInfo.renderArea.offset.y = consturctionInfo.renderPassConfigurations[renderPassId].area.offset.y * ((float)consturctionInfo.screenHeight / (float)2048);
			renderPassBeginInfo.renderArea.extent.width = consturctionInfo.renderPassConfigurations[renderPassId].area.extent.width * ((float)consturctionInfo.screenWidth / (float)2048);
			renderPassBeginInfo.renderArea.extent.height = consturctionInfo.renderPassConfigurations[renderPassId].area.extent.height * ((float)consturctionInfo.screenHeight / (float)2048);
			renderPassBeginInfo.clearValueCount = consturctionInfo.renderPassConfigurations[renderPassId].clearValues.size();
			renderPassBeginInfo.pClearValues = consturctionInfo.renderPassConfigurations[renderPassId].clearValues.data();

			vkCmdBeginRenderPass(commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			for (int j = 0; j < consturctionInfo.renderPassConfigurations[renderPassId].renderSessions.size(); j++) {

				if(consturctionInfo.renderPassConfigurations[renderPassId].renderSessions[j].usePushConstant)
					vkCmdPushConstants(
						commandBuffers[i], 
						pipelines[consturctionInfo.renderPassConfigurations[renderPassId].renderSessions[j].pipelineIndex].getLayout(), 
						VK_SHADER_STAGE_FRAGMENT_BIT, 
						0, 
						consturctionInfo.renderPassConfigurations[renderPassId].renderSessions[j].pushConstantSize, 
						consturctionInfo.renderPassConfigurations[renderPassId].renderSessions[j].pushConstantData
					);

				vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[consturctionInfo.renderPassConfigurations[renderPassId].renderSessions[j].pipelineIndex].getPipeline());
			
				VkViewport viewport0;
				viewport0.x = consturctionInfo.renderPassConfigurations[renderPassId].renderSessions[j].viewport.x * ((float)consturctionInfo.screenWidth / (float)2048);
				viewport0.y = consturctionInfo.renderPassConfigurations[renderPassId].renderSessions[j].viewport.y * ((float)consturctionInfo.screenHeight / (float)2048);
				viewport0.width = consturctionInfo.renderPassConfigurations[renderPassId].renderSessions[j].viewport.width * ((float)consturctionInfo.screenWidth / (float)2048);
				viewport0.height = consturctionInfo.renderPassConfigurations[renderPassId].renderSessions[j].viewport.height * ((float)consturctionInfo.screenHeight / (float)2048);
				viewport0.minDepth = consturctionInfo.renderPassConfigurations[renderPassId].renderSessions[j].viewport.minDepth;
				viewport0.maxDepth = consturctionInfo.renderPassConfigurations[renderPassId].renderSessions[j].viewport.maxDepth;

				vkCmdSetViewport(commandBuffers[i], 0, 1, &viewport0);
				
				std::vector<VkRect2D> scissores;

				for (int a = 0; a < consturctionInfo.renderPassConfigurations[renderPassId].renderSessions[j].scissores.size(); a++)
					scissores.push_back({
						(int32_t)(consturctionInfo.renderPassConfigurations[renderPassId].renderSessions[j].scissores[a].offset.x * ((float)consturctionInfo.screenWidth / (float)2048)),
						(int32_t)(consturctionInfo.renderPassConfigurations[renderPassId].renderSessions[j].scissores[a].offset.y * ((float)consturctionInfo.screenHeight / (float)2048)),
						(uint32_t)(consturctionInfo.renderPassConfigurations[renderPassId].renderSessions[j].scissores[a].extent.width * ((float)consturctionInfo.screenWidth / (float)2048)),
						(uint32_t)(consturctionInfo.renderPassConfigurations[renderPassId].renderSessions[j].scissores[a].extent.height * ((float)consturctionInfo.screenHeight / (float)2048))
						});

				vkCmdSetScissor(commandBuffers[i], 0, scissores.size(), scissores.data());

				vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, buffers[consturctionInfo.renderPassConfigurations[renderPassId].renderSessions[j].vertexBufferIndex].getBuffer(), consturctionInfo.renderPassConfigurations[renderPassId].renderSessions[j].vertexOffset.data());
				vkCmdBindIndexBuffer(commandBuffers[i], *buffers[consturctionInfo.renderPassConfigurations[renderPassId].renderSessions[j].indexBufferIndex].getBuffer(), consturctionInfo.renderPassConfigurations[renderPassId].renderSessions[j].indexOffset, VK_INDEX_TYPE_UINT32);
				uint32_t offsets[] = {0};
				vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[consturctionInfo.renderPassConfigurations[renderPassId].renderSessions[j].pipelineIndex].getLayout(), 0, 1, &descriptorSet, 0, offsets);
				vkCmdDrawIndexed(commandBuffers[i], buffers[consturctionInfo.renderPassConfigurations[renderPassId].renderSessions[j].indexBufferIndex].getInSize(), consturctionInfo.renderPassConfigurations[renderPassId].renderSessions[j].instanceCount, 0, 0, consturctionInfo.renderPassConfigurations[renderPassId].renderSessions[j].instanceOffset);
			}
			vkCmdEndRenderPass(commandBuffers[i]);

			testErrorCode(vkEndCommandBuffer(commandBuffers[i]));
		}
	}

	void initiationCodePart3() {

		VkSemaphoreCreateInfo semaphoreCreateInfo;
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreCreateInfo.pNext = nullptr;
		semaphoreCreateInfo.flags = 0;

		testErrorCode(vkCreateSemaphore(device, &semaphoreCreateInfo, consturctionInfo.allucator, &semaphoreImgAvailable));
		testErrorCode(vkCreateSemaphore(device, &semaphoreCreateInfo, consturctionInfo.allucator, &semaphoreImgRendered));
	}

public:
	Renderer(Configuration consturctionInfo) { create(consturctionInfo); };
	Renderer() {};
	~Renderer() { if(isCreated) destroy(); };
	Renderer(const Renderer&) = delete;
	Renderer(Renderer&&) = delete;
	Renderer& operator=(const Renderer&) = delete;
	Renderer& operator=(Renderer&&) = delete;

	//darf nur nach initation aufgerufen werden
	void setRenderPass(uint32_t renderPassId) {

		vkFreeCommandBuffers(device, commandPool, imageViews.size(), commandBuffers);
		delete[] commandBuffers;

		for (int i = 0; i < imageViews.size(); i++)
			vkDestroyFramebuffer(device, framebuffers[i], consturctionInfo.allucator);
		delete[] framebuffers;


		this->renderPassId = renderPassId;


		resizeCodePart1();
		resizeCodePart2();
	}

	uint32_t addBuffer(Buffer::Configuration& configuration) {
		buffers.push_back(Buffer());
		Buffer* buffer = &buffers[buffers.size() - 1];
		buffer->Init(configuration);
		return buffers.size() - 1;
	}

	//bleib gültig bis ein buffer hinzugefügt oder entfernt wird
	Buffer* getBufferByID(uint32_t id) {
		return &buffers[id];
	}

	void create(Configuration consturctionInfo) {
		if (isCreated)
			throw std::logic_error("Is already created");

		this->consturctionInfo = consturctionInfo;
		initiationCodePart0();
		resizeCodePart0();
		initiationCodePart1();
		resizeCodePart1();
		initiationCodePart2();
		bufferChangeCode0();
		resizeCodePart2();
		initiationCodePart3();

		isCreated = true;
	}

	void destroy() {
		vkDeviceWaitIdle(device);

		//dragonMesh.unload();

		depthImage.destroy();

		vkDestroyDescriptorSetLayout(device, descriptorLayout, consturctionInfo.allucator);
		vkDestroyDescriptorPool(device, descriptorPool, consturctionInfo.allucator);

		for (int i = 0; i < buffers.size(); i++)
			buffers[i].Destroy();
		buffers.clear();

		//vkFreeMemory(device, vertexBufferMem, consturctionInfo.allucator);
		//vkDestroyBuffer(device, vertexBuffer, consturctionInfo.allucator);
		//vkFreeMemory(device, indexBufferMem, consturctionInfo.allucator);
		//vkDestroyBuffer(device, indexBuffer, consturctionInfo.allucator);
		//vkFreeMemory(device, uniformBufferMem, consturctionInfo.allucator);
		//vkDestroyBuffer(device, uniformBuffer, consturctionInfo.allucator);


		for (int i = 0; i < images.size(); i++)
			images[i].unload();

		vkDestroySemaphore(device, semaphoreImgAvailable, consturctionInfo.allucator);
		vkDestroySemaphore(device, semaphoreImgRendered, consturctionInfo.allucator);
		vkFreeCommandBuffers(device, commandPool, imageViews.size(), commandBuffers);
		delete[] commandBuffers;
		vkDestroyCommandPool(device, commandPool, consturctionInfo.allucator);
		for (int i = 0; i < imageViews.size(); i++)
			vkDestroyFramebuffer(device, framebuffers[i], consturctionInfo.allucator);
		delete[] framebuffers;

		for(int i = 0; i < renderPasses.size(); i++)
			vkDestroyRenderPass(device, renderPasses[i], consturctionInfo.allucator);
		renderPasses.clear();

		for (int i = 0; i < pipelines.size(); i++)
			pipelines[i].destroy();
		pipelines.clear();

		for (int i = 0; i < shaders.size(); i++)
			shaders[i].destroy();
		shaders.clear();

		for (int i = 0; i < imageViews.size(); i++)
			vkDestroyImageView(device, imageViews[i], consturctionInfo.allucator);
		imageViews.clear();
		vkDestroySwapchainKHR(device, swapchain, consturctionInfo.allucator);
		vkDestroyDevice(device, consturctionInfo.allucator);
		vkDestroySurfaceKHR(instance, surface, consturctionInfo.allucator);
		vkDestroyInstance(instance, consturctionInfo.allucator);

		isCreated = false;
	}

	void draw() {

		uint32_t imageindex;
		testErrorCode(vkAcquireNextImageKHR(device, swapchain, -1, semaphoreImgAvailable, VK_NULL_HANDLE, &imageindex));
		VkPipelineStageFlags waitStageMask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSubmitInfo submitInfo;
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &semaphoreImgAvailable;
		submitInfo.pWaitDstStageMask = waitStageMask;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &(commandBuffers[imageindex]);
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &semaphoreImgRendered;

		VkResult temp = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		testErrorCode(temp);

		VkPresentInfoKHR presentInfo;
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext = nullptr;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &semaphoreImgRendered;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapchain;
		presentInfo.pImageIndices = &imageindex;
		presentInfo.pResults = nullptr;

		testErrorCode(vkQueuePresentKHR(queue, &presentInfo));
	}

	void recreateSwapchain(uint32_t screenWidth, uint32_t screenHeight) {
		vkDeviceWaitIdle(device);

		this->consturctionInfo.screenWidth = screenWidth;
		this->consturctionInfo.screenHeight = screenHeight;

		depthImage.destroy();

		vkFreeCommandBuffers(device, commandPool, imageViews.size(), commandBuffers);
		delete[] commandBuffers;
		for (int i = 0; i < imageViews.size(); i++)
			vkDestroyFramebuffer(device, framebuffers[i], nullptr);
		delete[] framebuffers;
		for (int i = 0; i < imageViews.size(); i++)
			vkDestroyImageView(device, imageViews[i], nullptr);
		imageViews.clear();

		auto tempSwapchain = swapchain;

		resizeCodePart0();
		resizeCodePart1();
		resizeCodePart2();

		vkDestroySwapchainKHR(device, tempSwapchain, nullptr);
	}
};

Renderer renderer;

void onWindowResized(GLFWwindow* window, int width, int height) {
	renderer.recreateSwapchain(width,height);
}

GLFWwindow* startGLFW(uint32_t screenWidth, uint32_t screenHeight) {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);		//später ändern auf rezisable

	GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Test", nullptr, nullptr);	//größe ändern
	glfwSetWindowSizeCallback(window, onWindowResized);

	return window;
}

void shutDownGLFW(GLFWwindow* window) {
	glfwDestroyWindow(window);
	glfwTerminate();
}

#endif // !_RENDERER_