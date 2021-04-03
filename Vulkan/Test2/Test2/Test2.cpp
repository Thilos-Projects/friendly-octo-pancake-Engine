#include "Renderer.h"

#define InstanceCount 500

struct UBO {
	//glm::mat4 modle;
	glm::mat4 view;
	glm::mat4 proj;
};

struct Light {
	alignas(16) glm::vec3 pos;
	alignas(16) glm::vec3 color;
};

struct InstanceType
{
	//std::vector<glm::mat4> instancen;
	glm::mat4 instancen[InstanceCount];
};

uint32_t uboBuffer;
UBO ubo0;
uint32_t lightBuffer;
glm::uint lArraySize = 6;
Light lArray[20];
uint32_t instanceBuffer;
InstanceType instanzen;

//lehmerRND
uint32_t stateLehmer = 0;
uint32_t Lehmer32() {
	stateLehmer += 0xe120fc15;
	uint64_t temp;
	temp = (uint64_t)stateLehmer * 0x4a39b70d;
	uint32_t m1 = (temp << 32) ^ temp;
	temp = (uint64_t)m1 * 0x12fad5c9;
	//uint32_t m2 = (temp >> 32) ^ temp;
	return (temp >> 32) ^ temp; // m2;
}

glm::vec3 randomPosses[InstanceCount];

float lastFrameTime;

void updateMVP() {

	Buffer* buffer;

	static auto gameStartTime = std::chrono::high_resolution_clock::now();
	auto frameTime = std::chrono::high_resolution_clock::now();

	float timeSinceStart = std::chrono::duration_cast<std::chrono::milliseconds>(frameTime - gameStartTime).count() / 100.0f;
	float deltaTime = timeSinceStart - lastFrameTime;
	lastFrameTime = timeSinceStart;
	for (int i = 0; i < InstanceCount; i++)
		instanzen.instancen[i] = glm::rotate(instanzen.instancen[i], deltaTime * glm::radians(0.3f + 0.2f * i), glm::vec3(0.0f, 0.0f, 1.0f));

	ubo0.view = glm::lookAt(glm::vec3(sin(timeSinceStart / 5) + 0.5f, cos(timeSinceStart / 5) + 0.5f, 2.0f), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f));

	for (int i = 0; i < lArraySize; i++)
		lArray[i].pos = glm::rotate(glm::mat4(1.0f), timeSinceStart * glm::radians(4.5f) + i * glm::radians(45.0f) , glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(0.0f, 3.0f, 1.0f, 0.0f);

	ubo0.proj = glm::perspective(glm::radians(60.0f), (float)renderer.consturctionInfo.screenWidth / (float)renderer.consturctionInfo.screenHeight, 0.01f, 100.0f);
	ubo0.proj[1][1] *= -1;

	buffer = renderer.getBufferByID(uboBuffer);
	buffer->setCopyPointer(0);
	buffer->setData((uint32_t*)&ubo0, sizeof(ubo0));
	buffer->updateBuffer();

	buffer = renderer.getBufferByID(lightBuffer);
	buffer->setCopyPointer(0);
	buffer->setData((uint32_t*)&lArray, lArraySize * sizeof(Light));
	buffer->updateBuffer();

	buffer = renderer.getBufferByID(instanceBuffer);
	buffer->setCopyPointer(0);
	buffer->setData((uint32_t*)&instanzen, sizeof(InstanceType));
	buffer->setCopyPointer(1 * sizeof(InstanceType));
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
	for (int i = 0; i < InstanceCount; i++) {
		randomPosses[i] = glm::vec3((Lehmer32() % 1000) / 1000.0f, (Lehmer32() % 1000) / 1000.0f, (Lehmer32() % 1000) / 1000.0f);
		instanzen.instancen[i] = glm::mat4(1.0f);
		instanzen.instancen[i] = glm::translate(instanzen.instancen[i], randomPosses[i]);
		instanzen.instancen[i] = glm::scale(instanzen.instancen[i], glm::vec3(0.01f, 0.01f, 0.01f));
	}

	for (int i = 0; i < lArraySize; i++)
		lArray[i].color = glm::vec3((Lehmer32() % 100) / 100.0f, (Lehmer32() % 100) / 100.0f, (Lehmer32() % 100) / 100.0f);

	lArray[0].color = glm::vec3(1.0f, 0.0f, 0.0f);
	lArray[1].color = glm::vec3(1.0f, 1.0f, 0.0f);
	lArray[2].color = glm::vec3(1.0f, 0.0f, 1.0f);
	lArray[3].color = glm::vec3(1.0f, 1.0f, 1.0f);
	lArray[4].color = glm::vec3(0.0f, 1.0f, 0.0f);
	lArray[5].color = glm::vec3(0.0f, 0.0f, 1.0f);

	VkBool32 pushConstant1 = VK_TRUE;

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

	/*configuration.imageConfigurations.push_back(Image::Configuration());
	imgConfig = &configuration.imageConfigurations[configuration.imageConfigurations.size() - 1];
	imgConfig->path = "151.jpg";

	configuration.imageConfigurations.push_back(Image::Configuration());
	imgConfig = &configuration.imageConfigurations[configuration.imageConfigurations.size() - 1];
	imgConfig->path = "151_norm.jpg";*/

	//buffer creation phase start
	Mesh dragonMesh;
	std::vector<Vertex> verticies;
	std::vector<uint32_t> indices;
	dragonMesh.load("dragon.obj");
	verticies = dragonMesh.getVertecies();
	indices = dragonMesh.getIndices();

	//vertex Buffer Start
	Buffer::Configuration bufferConfigs = Buffer::Configuration();
	bufferConfigs.bufferusage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufferConfigs.isInGrakaMemory = true;
	bufferConfigs.maxBufferSize = verticies.size();
	bufferConfigs.singleBufferSize = sizeof(Vertex);

	uint32_t vertexBufferIndex = renderer.addBuffer(bufferConfigs);;

	Buffer* buffer = renderer.getBufferByID(vertexBufferIndex);
	buffer->setCopyPointer(0);
	buffer->setData(verticies.data(), verticies.size() * sizeof(Vertex));
	//vertex Buffer Ende

	//index Buffer Start
	bufferConfigs = Buffer::Configuration();
	bufferConfigs.bufferusage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	bufferConfigs.isInGrakaMemory = true;
	bufferConfigs.maxBufferSize = indices.size();
	bufferConfigs.singleBufferSize = sizeof(uint32_t);

	uint32_t indexBufferIndex = renderer.addBuffer(bufferConfigs);
	buffer = renderer.getBufferByID(indexBufferIndex);
	buffer->setCopyPointer(0);
	buffer->setData(indices.data(), indices.size() * sizeof(uint32_t));
	//index Buffer Ende

	//UBO Buffer Start
	bufferConfigs = Buffer::Configuration();
	bufferConfigs.bufferusage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	bufferConfigs.isInGrakaMemory = false;
	bufferConfigs.maxBufferSize = sizeof(UBO);
	bufferConfigs.singleBufferSize = sizeof(UBO);

	uboBuffer = renderer.addBuffer(bufferConfigs);
	buffer = renderer.getBufferByID(uboBuffer);
	buffer->setCopyPointer(0);
	buffer->setData(&ubo0, sizeof(UBO));
	//UBO Buffer Ende

	//light Buffer Start
	bufferConfigs = Buffer::Configuration();
	bufferConfigs.bufferusage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	bufferConfigs.isInGrakaMemory = false;
	bufferConfigs.maxBufferSize = 20;		// noch verbessern
	bufferConfigs.offset = sizeof(lArraySize);
	bufferConfigs.singleBufferSize = sizeof(Light);

	lightBuffer = renderer.addBuffer(bufferConfigs);
	buffer = renderer.getBufferByID(lightBuffer);
	buffer->setCopyPointer(0);
	buffer->setData(&lArray, lArraySize * sizeof(Light));
	buffer->setOffsetData(&lArraySize, sizeof(lArraySize), 0);
	//light Buffer Ende 

	//instance buffer Start
	bufferConfigs = Buffer::Configuration();
	bufferConfigs.bufferusage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	bufferConfigs.isInGrakaMemory = true;
	bufferConfigs.maxBufferSize = sizeof(InstanceType);
	bufferConfigs.singleBufferSize = sizeof(InstanceType);

	instanceBuffer = renderer.addBuffer(bufferConfigs);
	buffer = renderer.getBufferByID(instanceBuffer);
	buffer->setCopyPointer(0);
	buffer->setData(&instanzen, sizeof(InstanceType));
	//instance Buffer Ende
	//buffer creation phase ende

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
	//vertex Binding Start
	renderSession->vertexBufferIndex = vertexBufferIndex;
	renderSession->vertexOffset.push_back(0);
	//vertex Binding Ende

	//index Binding Start
	renderSession->indexBufferIndex = indexBufferIndex;
	renderSession->indexOffset = 0;
	//index Binding Ende

	//scissore Start
	renderSession->scissores.push_back({ 0,0,2048,2048 });
	//scissore Ende

	//viewport Start
	renderSession->viewport.x = 0;
	renderSession->viewport.y = 0;
	renderSession->viewport.width = 2048;
	renderSession->viewport.height = 2048;
	renderSession->viewport.minDepth = 0.0f;
	renderSession->viewport.maxDepth = 1.0f;
	//viewport Ende

	//pushConstante Start
	renderSession->usePushConstant = true;
	renderSession->pushConstantSize = sizeof(pushConstant1);
	renderSession->pushConstantData = &pushConstant1;
	//pushConstante Ende

	renderSession->instanceCount = InstanceCount;

	//shader Configuratin Start
	//vertex shader Start
	configuration.shaderConfigurations.push_back(Shader::Configuration());
	conf = &configuration.shaderConfigurations[configuration.shaderConfigurations.size() - 1];

	conf->compilerPath = "F:/Vulkan/1.2.162.1/Bin32/glslangValidator.exe";
	conf->shaderMainFunction = "main";
	conf->shaderPath = "VertexShader.vert";
	conf->shaderStageFlagBits = VK_SHADER_STAGE_VERTEX_BIT;

	//UBO Binding Start
	conf->bindingConfiugurations.push_back(Shader::Configuration::DescriptorSetLayoutBindingConfiguration());
	bindingConf = &conf->bindingConfiugurations[conf->bindingConfiugurations.size() - 1];
	bindingConf->bindingId = 0;
	bindingConf->type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindingConf->samplerPointer = nullptr;
	bindingConf->multiuse = sizeof(UBO);
	bindingConf->buferIndex = uboBuffer;
	//UBO Binding Ende

	//instance Binding Start
	conf->bindingConfiugurations.push_back(Shader::Configuration::DescriptorSetLayoutBindingConfiguration());
	bindingConf = &conf->bindingConfiugurations[conf->bindingConfiugurations.size() - 1];
	bindingConf->bindingId = 1;
	bindingConf->type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindingConf->samplerPointer = nullptr;
	bindingConf->multiuse = sizeof(InstanceType);
	bindingConf->buferIndex = instanceBuffer;
	//indexBinding Ende
	//vertex shader Ende

	//fragment shader Start
	configuration.shaderConfigurations.push_back(Shader::Configuration());
	conf = &configuration.shaderConfigurations[configuration.shaderConfigurations.size() - 1];

	conf->compilerPath = "F:/Vulkan/1.2.162.1/Bin32/glslangValidator.exe";
	conf->shaderMainFunction = "main";
	conf->shaderPath = "FragmentShader.frag";
	conf->shaderStageFlagBits = VK_SHADER_STAGE_FRAGMENT_BIT;
	
	//light Binding Start
	conf->bindingConfiugurations.push_back(Shader::Configuration::DescriptorSetLayoutBindingConfiguration());
	bindingConf = &conf->bindingConfiugurations[conf->bindingConfiugurations.size() - 1];
	bindingConf->bindingId = 2;
	bindingConf->type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindingConf->samplerPointer = nullptr;
	bindingConf->multiuse = sizeof(Light) * lArraySize + sizeof(lArraySize);
	bindingConf->buferIndex = lightBuffer;
	//light Binding Ende

	/*bindingConf = &conf->bindingConfiugurations[conf->bindingConfiugurations.size() - 1];
	bindingConf->bindingId = 2;
	bindingConf->type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindingConf->samplerPointer = nullptr;
	bindingConf->multiuse = 0;

	conf->bindingConfiugurations.push_back(Shader::Configuration::DescriptorSetLayoutBindingConfiguration());

	bindingConf = &conf->bindingConfiugurations[conf->bindingConfiugurations.size() - 1];
	bindingConf->bindingId = 3;
	bindingConf->type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindingConf->samplerPointer = nullptr;
	bindingConf->multiuse = 1;*/
	//fragment shader Ende
	//shader Configuratin Ende


	renderer.create(configuration);

	loop(window);
	renderer.destroy();
	shutDownGLFW(window);

	return 0;
}