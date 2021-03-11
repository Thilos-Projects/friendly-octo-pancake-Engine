#ifndef _PIPELINE_
#define _PIPELINE_

#include "Vertex.h"
#include "DepthImage.h"
#include "NonDependingFunktions.h"
#include <vector>
#include <stdexcept>

class Pipeline {
public:
	struct Configuration
	{
		uint32_t width = 300;
		uint32_t height = 300;
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

	VkAllocationCallbacks* allucationCallback;

public:
	Pipeline(VkAllocationCallbacks* allucationCallback = nullptr) {this->allucationCallback = allucationCallback;};
	~Pipeline() { destroy(); };
	Pipeline(const Pipeline&) = delete;
	Pipeline(Pipeline&&) = delete;
	Pipeline& operator=(const Pipeline&) = delete;
	Pipeline& operator=(Pipeline&&) = delete;

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
	
	void create(VkDevice device, VkRenderPass renderPass, VkDescriptorSetLayout descriptorSetLayout) 
	{
		if (!isInit)
			throw std::logic_error("call init First");
		if (isCreated)
			throw std::logic_error("was already Created");

		this->device = device;

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.pNext = nullptr;
		pipelineLayoutCreateInfo.flags = 0;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
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