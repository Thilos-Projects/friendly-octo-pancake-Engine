#ifndef _PIPELINE_
#define _PIPELINE_

#include "Vertex.h"
#include "DepthImage.h"
#include "Shader.h"
#include "NonDependingFunktions.h"
#include <vector>
#include <stdexcept>

class RenderPass {
private:
	VkAttachmentDescription attechmentDesription;
	VkAttachmentReference attechmentRefference;
	VkAttachmentDescription depthAttachment;
	VkAttachmentReference depthAttechmentRefference;
	VkSubpassDescription subpassDescription;
	VkSubpassDependency subpassDependency;
	std::vector<VkAttachmentDescription> attachments;

	VkRenderPass renderPass;

	VkDevice device;

	bool isInit = false;
	bool isCreated = false;

public:

	void init(VkFormat colorFormat) {
		attechmentDesription.flags = 0;											//speicher überlappung möglich
		attechmentDesription.format = colorFormat;
		attechmentDesription.samples = VK_SAMPLE_COUNT_1_BIT;
		attechmentDesription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;				//was pasiert mit werten nach dem laden
		attechmentDesription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;			//was pasiert mit dem speichen
		attechmentDesription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;		//fog of ware
		attechmentDesription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		attechmentDesription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			//format vor laden
		attechmentDesription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;		//format nach laden

		attechmentRefference.attachment = 0;
		attechmentRefference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		depthAttechmentRefference.attachment = 1;
		depthAttechmentRefference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


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

		subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependency.dstSubpass = 0;
		subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.srcAccessMask = 0;
		subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependency.dependencyFlags = 0;

		attachments.push_back(attechmentDesription);


		isInit = true;
	};

	void create(VkDevice device, VkPhysicalDevice physicalDevice) {
		if (!isInit)
			throw std::logic_error("is not init");
		if (isCreated)
			throw std::logic_error("is already created");

		this->device = device;

		attachments.push_back(DepthImage::getdepthAttachment(physicalDevice));

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

		isCreated = true;
	};

	void destroy() {
		if(isCreated)
			vkDestroyRenderPass(device, renderPass, nullptr);
		device = VK_NULL_HANDLE;
		isCreated = false;
	};

	VkRenderPass getRenderPass() {
		if (!isCreated)
			throw std::logic_error("is not created");

		return renderPass;
	}

};

class Pipeline {
private:
	std::vector<Shader*> shaderModules;
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

	std::vector<VkViewport> viewports;
	std::vector<VkRect2D> scissores;

	VkPipeline pipeline = VK_NULL_HANDLE;
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	VkDescriptorSetLayout layout = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;

	bool isInit = false;
	bool isCreated = false;

	VkAllocationCallbacks* allucationCallback;

	void createShaderStageCreateInfo() {
		shaderStages.clear();
		for (int i = 0; i < shaderModules.size(); i++) {
			shaderStages.push_back(shaderModules[i]->getShaderStageCreateInfo());
		}
	}

	void createDescriptorLayout() {
		std::vector< VkDescriptorSetLayoutBinding> layoutBinding;

		for (int i = 0; i < shaderModules.size(); i++) {
			auto temp = shaderModules[i]->getLayoutBindings();

			for (int j = 0; j < temp.size(); j++) {
				layoutBinding.push_back(temp[j]);
			}
		}

		VkDescriptorSetLayoutCreateInfo desSetLayoutCreateInfo;
		desSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		desSetLayoutCreateInfo.pNext = nullptr;
		desSetLayoutCreateInfo.flags = 0;
		desSetLayoutCreateInfo.bindingCount = layoutBinding.size();
		desSetLayoutCreateInfo.pBindings = layoutBinding.data();

		testErrorCode(vkCreateDescriptorSetLayout(device, &desSetLayoutCreateInfo, nullptr, &layout));
	}

public:
	Pipeline(VkAllocationCallbacks* allucationCallback = nullptr) {this->allucationCallback = allucationCallback;};
	~Pipeline() { destroy(); };
	Pipeline(const Pipeline&) = delete;
	Pipeline(Pipeline&&) = delete;
	Pipeline& operator=(const Pipeline&) = delete;
	Pipeline& operator=(Pipeline&&) = delete;

	void addShader(Shader* sh) {
		shaderModules.push_back(sh);
	}

	void changePoligonMode(VkPolygonMode poligonMode) {
		rasterisationCreateInfo.polygonMode = poligonMode;
	}

	void changeCullMode(VkCullModeFlags cullMode) {
		rasterisationCreateInfo.cullMode = cullMode;
	}

	void changeFrontFaceIndication(VkFrontFace frontFaceIndication) {
		rasterisationCreateInfo.frontFace = frontFaceIndication;
	}

	void addViewports(uint32_t x, uint32_t y, uint32_t width, uint32_t height, float minDepth, float maxDepth) {
		viewports.push_back(VkViewport());
		VkViewport* viewport = &(viewports[viewports.size() - 1]);
		viewport->x = x;
		viewport->y = y;
		viewport->width = width;
		viewport->height = height;
		viewport->minDepth = minDepth;
		viewport->maxDepth = maxDepth;
	};

	void addScissores(uint32_t xOffset, uint32_t yOffset, uint32_t width, uint32_t height) {
		scissores.push_back(VkRect2D());
		VkRect2D* scissore = &(scissores[scissores.size() - 1]);
		scissore->extent = {width,height};
		scissore->offset = { (int32_t)xOffset, (int32_t)yOffset };
	};

	void resetViewports() {
		viewports.clear();
	};

	void resetScissores() {
		scissores.clear();
	};

	void setViewportsAndScissores() {
		viewportStateCreateInfo.viewportCount = viewports.size();
		viewportStateCreateInfo.pViewports = viewports.data();	//array
		viewportStateCreateInfo.scissorCount = scissores.size();
		viewportStateCreateInfo.pScissors = scissores.data();	//array
	}

	void setVertexTypologie(VkPrimitiveTopology topologie) {
		inputAssemblyCreateInfo.topology = topologie;	//liste an dreieck
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
		inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;				//restart des zeichnens

		setVertexTypologie(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

		viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateCreateInfo.pNext = nullptr;
		viewportStateCreateInfo.flags = 0;

		resetViewports();
		resetScissores();

		addViewports(0, 0, width, height, 0, 1);
		addScissores(0, 0, width, height);

		setViewportsAndScissores();

		rasterisationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterisationCreateInfo.pNext = nullptr;
		rasterisationCreateInfo.flags = 0;
		rasterisationCreateInfo.depthClampEnable = VK_FALSE;
		rasterisationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
		changePoligonMode(VK_POLYGON_MODE_FILL);
		changeCullMode(VK_CULL_MODE_BACK_BIT);
		changeFrontFaceIndication(VK_FRONT_FACE_CLOCKWISE);
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
	void create(VkDevice device, VkRenderPass renderPass) 
	{
		if (!isInit)
			throw std::logic_error("call init First");
		if (isCreated)
			throw std::logic_error("was already Created");

		this->device = device;

		createShaderStageCreateInfo();
		createDescriptorLayout();

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.pNext = nullptr;
		pipelineLayoutCreateInfo.flags = 0;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts = &layout;
		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstRange;

		testErrorCode(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

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

		testErrorCode(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline));
		isCreated = true;
	};
	void destroy() {
		if (isCreated) {
			vkDestroyPipeline(device, pipeline, nullptr);
			vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
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
#endif