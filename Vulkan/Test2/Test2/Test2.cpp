#include "Renderer.h"

#define InstanceCount 1000

struct UBO {
	//glm::mat4 modle;
	glm::mat4 view;
	glm::mat4 proj;
	glm::vec3 light;		//mehrere ermöglichen
};

struct InstanceType
{
	//std::vector<glm::mat4> instancen;
	glm::mat4 instancen[InstanceCount];
};

uint32_t uboBuffer;
UBO ubo0;
uint32_t instanceBuffer;
InstanceType instanzen;

glm::vec3 randomPosses[InstanceCount];

void updateMVP() {

	Buffer* buffer;

	static auto gameStartTime = std::chrono::high_resolution_clock::now();
	auto frameTime = std::chrono::high_resolution_clock::now();

	float timeSinceStart = std::chrono::duration_cast<std::chrono::milliseconds>(frameTime - gameStartTime).count() / 1000.0f;

	glm::vec3 offset = glm::vec3(0.0f, 0.0f, 0.0f);

	for (int i = 0; i < InstanceCount; i++) {

		instanzen.instancen[i] = glm::mat4(1.0f);
		instanzen.instancen[i] = glm::translate(instanzen.instancen[i], randomPosses[i]);
		instanzen.instancen[i] = glm::translate(instanzen.instancen[i], offset);
		instanzen.instancen[i] = glm::scale(instanzen.instancen[i], glm::vec3(0.01f, 0.01f, 0.01f));
		instanzen.instancen[i] = glm::rotate(instanzen.instancen[i], -timeSinceStart * glm::radians(30.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	}

	ubo0.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f) + offset, glm::vec3(0.0f, 0.0f, 0.0f) + offset, glm::vec3(0.0f, 0.0f, 1.0f));

	ubo0.light = glm::vec4(offset, 0.0f) + glm::rotate(glm::mat4(1.0f), timeSinceStart * 0.1f * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(0.0f, 3.0f, 1.0f, 0.0f);

	ubo0.proj = glm::perspective(glm::radians(60.0f), (float)renderer.consturctionInfo.screenWidth / (float)renderer.consturctionInfo.screenHeight, 0.01f, 100.0f);
	ubo0.proj[1][1] *= -1;

	buffer = renderer.getBufferByID(uboBuffer);
	buffer->setCopyPointer(0);
	buffer->setData((uint32_t*)&ubo0, sizeof(ubo0));
	buffer->updateBuffer();

	buffer = renderer.getBufferByID(instanceBuffer);
	buffer->setCopyPointer(0);
	buffer->setData((uint32_t*)&instanzen, sizeof(InstanceType));
	buffer->updateBuffer();

}

void loop(GLFWwindow* window) {
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		updateMVP();
		renderer.draw();
	}
}

int main()
{
	GLFWwindow* window = startGLFW(1024, 1024);

	Renderer::Configuration configuration;
	Image::Configuration* imgConfig;
	Pipeline::Configuration* piplineConfig;
	RenderPassConfiguration* renderPassConfig;
	RenderPassConfiguration::attachmentConfiguration* renderPassAttachmendConfig;
	RenderPassConfiguration::RenderDraw* renderSession;
	Shader::Configuration* conf;
	Shader::Configuration::DescriptorSetLayoutBindingConfiguration* bindingConf;

	configuration.window = window;
	configuration.screenWidth = 1024;
	configuration.screenHeight = 1024;
	//configuration.layers.push_back("VK_LAYER_KHRONOS_validation");

	configuration.imageConfigurations.push_back(Image::Configuration());
	imgConfig = &configuration.imageConfigurations[configuration.imageConfigurations.size() - 1];
	imgConfig->path = "151.jpg";

	configuration.imageConfigurations.push_back(Image::Configuration());
	imgConfig = &configuration.imageConfigurations[configuration.imageConfigurations.size() - 1];
	imgConfig->path = "151_norm.jpg";

	configuration.pipelineConfigurations.push_back(Pipeline::Configuration());
	piplineConfig = &configuration.pipelineConfigurations[configuration.pipelineConfigurations.size() - 1];
	piplineConfig->poligonMode = VK_POLYGON_MODE_FILL;
	piplineConfig->renderPassId = 0;
	piplineConfig->shaderIds.push_back(0);
	piplineConfig->shaderIds.push_back(1);

	configuration.renderPassConfigurations.push_back(RenderPassConfiguration());
	renderPassConfig = &configuration.renderPassConfigurations[configuration.renderPassConfigurations.size() - 1];
	renderPassConfig->usesDepth = true;
	renderPassConfig->area = { 0,0,2048,2048 };
	renderPassConfig->clearValues.push_back({ 0.0f,0.0f,0.2f,0.5f });
	renderPassConfig->clearValues.push_back({ 1.0f,0.0f });
	renderPassConfig->attachments.push_back(RenderPassConfiguration::attachmentConfiguration());
	renderPassAttachmendConfig = &renderPassConfig->attachments[renderPassConfig->attachments.size() - 1];

	renderPassConfig->renderSessions.push_back(RenderPassConfiguration::RenderDraw());
	renderSession = &renderPassConfig->renderSessions[renderPassConfig->renderSessions.size() - 1];
	renderSession->pipelineIndex = 0;
	renderSession->vertexBufferIndex = 0;
	renderSession->vertexOffset.push_back(0);
	renderSession->indexBufferIndex = 1;
	renderSession->indexOffset = 0;
	renderSession->scissores.push_back({ 0,0,2048,2048 });
	renderSession->viewport.x = 0;
	renderSession->viewport.y = 0;
	renderSession->viewport.width = 2048;
	renderSession->viewport.height = 2048;
	renderSession->viewport.minDepth = 0.0f;
	renderSession->viewport.maxDepth = 1.0f;
	renderSession->usePushConstant = true;
	VkBool32 pushConstant1 = VK_TRUE;
	renderSession->pushConstantSize = sizeof(pushConstant1);
	renderSession->pushConstantData = &pushConstant1;
	renderSession->instanceCount = InstanceCount;

	configuration.shaderConfigurations.push_back(Shader::Configuration());
	conf = &configuration.shaderConfigurations[configuration.shaderConfigurations.size() - 1];

	conf->compilerPath = "F:/Vulkan/1.2.162.1/Bin32/glslangValidator.exe";
	conf->shaderMainFunction = "main";
	conf->shaderPath = "VertexShader.vert";
	conf->shaderStageFlagBits = VK_SHADER_STAGE_VERTEX_BIT;

	conf->bindingConfiugurations.push_back(Shader::Configuration::DescriptorSetLayoutBindingConfiguration());
	bindingConf = &conf->bindingConfiugurations[conf->bindingConfiugurations.size() - 1];
	bindingConf->bindingId = 0;
	bindingConf->type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindingConf->samplerPointer = nullptr;
	bindingConf->multiuse = sizeof(UBO);
	bindingConf->buferIndex = 2;

	conf->bindingConfiugurations.push_back(Shader::Configuration::DescriptorSetLayoutBindingConfiguration());
	bindingConf = &conf->bindingConfiugurations[conf->bindingConfiugurations.size() - 1];
	bindingConf->bindingId = 1;
	bindingConf->type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindingConf->samplerPointer = nullptr;
	bindingConf->multiuse = sizeof(InstanceType);
	bindingConf->buferIndex = 3;

	configuration.shaderConfigurations.push_back(Shader::Configuration());
	conf = &configuration.shaderConfigurations[configuration.shaderConfigurations.size() - 1];

	conf->compilerPath = "F:/Vulkan/1.2.162.1/Bin32/glslangValidator.exe";
	conf->shaderMainFunction = "main";
	conf->shaderPath = "FragmentShader.frag";
	conf->shaderStageFlagBits = VK_SHADER_STAGE_FRAGMENT_BIT;

	conf->bindingConfiugurations.push_back(Shader::Configuration::DescriptorSetLayoutBindingConfiguration());

	bindingConf = &conf->bindingConfiugurations[conf->bindingConfiugurations.size() - 1];
	bindingConf->bindingId = 2;
	bindingConf->type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindingConf->samplerPointer = nullptr;
	bindingConf->multiuse = 0;

	conf->bindingConfiugurations.push_back(Shader::Configuration::DescriptorSetLayoutBindingConfiguration());

	bindingConf = &conf->bindingConfiugurations[conf->bindingConfiugurations.size() - 1];
	bindingConf->bindingId = 3;
	bindingConf->type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindingConf->samplerPointer = nullptr;
	bindingConf->multiuse = 1;

	Mesh dragonMesh;
	std::vector<Vertex> verticies;
	std::vector<uint32_t> indices;
	dragonMesh.load("dragon.obj");
	verticies = dragonMesh.getVertecies();
	indices = dragonMesh.getIndices();


	Buffer::Configuration bufferConfigs = Buffer::Configuration();
	bufferConfigs.bufferusage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufferConfigs.isInGrakaMemory = true;
	bufferConfigs.maxBufferSize = verticies.size();
	bufferConfigs.singleBufferSize = sizeof(Vertex);

	uint32_t index = renderer.addBuffer(bufferConfigs);;

	Buffer* buffer = renderer.getBufferByID(index);
	buffer->setCopyPointer(0);
	buffer->setData(verticies.data(), verticies.size() * sizeof(Vertex));

	bufferConfigs = Buffer::Configuration();
	bufferConfigs.bufferusage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	bufferConfigs.isInGrakaMemory = true;
	bufferConfigs.maxBufferSize = indices.size();
	bufferConfigs.singleBufferSize = sizeof(uint32_t);

	index = renderer.addBuffer(bufferConfigs);
	buffer = renderer.getBufferByID(index);
	buffer->setCopyPointer(0);
	buffer->setData(indices.data(), indices.size() * sizeof(uint32_t));

	bufferConfigs = Buffer::Configuration();
	bufferConfigs.bufferusage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	bufferConfigs.isInGrakaMemory = false;
	bufferConfigs.maxBufferSize = sizeof(UBO);
	bufferConfigs.singleBufferSize = sizeof(UBO);

	uboBuffer = renderer.addBuffer(bufferConfigs);
	buffer = renderer.getBufferByID(uboBuffer);
	buffer->setCopyPointer(0);
	buffer->setData(&ubo0, sizeof(UBO));

	bufferConfigs = Buffer::Configuration();
	bufferConfigs.bufferusage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	bufferConfigs.isInGrakaMemory = false;
	bufferConfigs.maxBufferSize = sizeof(InstanceType);
	bufferConfigs.singleBufferSize = sizeof(InstanceType);

	for (int i = 0; i < InstanceCount; i++)
		randomPosses[i] = glm::vec3((rand() % 1000) / 1000.0f, (rand() % 1000) / 1000.0f, (rand() % 1000) / 1000.0f);

	instanceBuffer = renderer.addBuffer(bufferConfigs);
	buffer = renderer.getBufferByID(instanceBuffer);
	buffer->setCopyPointer(0);
	buffer->setData(&instanzen, sizeof(InstanceType));

	renderer.create(configuration);

	loop(window);
	renderer.destroy();
	shutDownGLFW(window);

	return 0;
}