#ifndef _SHADER_
#define _SHADER_

#include "NonDependingFunktions.h"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#define GLSLCOMPILERPATH "F:/Vulkan/1.2.162.1/Bin32/glslangValidator.exe"

class Shader
{
private:
	std::vector<char> shaderCodeSpv;
	std::vector<char> shaderCode;
	VkShaderModule shaderModule;

	VkShaderStageFlagBits shaderStage;
	const char* mainMethodName;

	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;

	VkDevice device = VK_NULL_HANDLE;
	VkAllocationCallbacks* allucationCallback = nullptr;

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

	void compileShader(const char* path) {
		std::string command = GLSLCOMPILERPATH;
		std::string pa = path;
		int32_t from = 0; // pa.find_last_of("/") + 1;
		int32_t to = pa.find_last_of(".");
		std::string newPa = pa.substr(from, to - from) + ".spv";
		command += " -V ";
		command += pa;
		command += " -o ";
		command += newPa;
		int32_t result = system(command.c_str());
		shaderCode = readFile(pa);
		shaderCodeSpv = readFile(newPa);
	}

	void CreateShaderModul() {
		VkShaderModuleCreateInfo shaderCreateInfo;
		shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderCreateInfo.pNext = nullptr;
		shaderCreateInfo.flags = 0;
		shaderCreateInfo.codeSize = shaderCodeSpv.size();
		shaderCreateInfo.pCode = (uint32_t*)shaderCodeSpv.data();

		testErrorCode(vkCreateShaderModule(device, &shaderCreateInfo, allucationCallback, &shaderModule));
	}

	void createSetLayoutBindings() {

		std::string code;
		for (int i = 0; i < shaderCode.size(); i++) {
			code += shaderCode[i];
		}

		std::string line;
		std::stringstream sStream = std::stringstream(code);

		while (std::getline(sStream, line)) {
			if (line.starts_with("layout")) {
				if (line.find("uniform") != std::string::npos) {
					auto posStart = line.find("binding");
					if (posStart == std::string::npos)
						continue;

					auto pos = std::stoi(line.substr(posStart + 10, 3));

					setLayoutBindings.push_back(VkDescriptorSetLayoutBinding());
					VkDescriptorSetLayoutBinding* binding = &setLayoutBindings[setLayoutBindings.size() - 1];

					binding->binding = pos;							//in shader
					binding->descriptorCount = 1;
					binding->stageFlags = shaderStage;
					binding->pImmutableSamplers = nullptr;

					if (line.find("sampler2D") != std::string::npos) {
						binding->descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					}
					else {
						binding->descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					}
				}
			}
		}
	}

	bool isInit = false;
	bool isCreated = false;

public:
	Shader() {};
	~Shader() {};
	Shader(const Shader&) = delete;
	Shader(Shader&&) = delete;
	Shader& operator=(const Shader&) = delete;
	Shader& operator=(Shader&&) = delete;

	void init(const char* filename, VkShaderStageFlagBits shaderStage, const char* mainMethodName) {
		this->shaderStage = shaderStage;
		this->mainMethodName = mainMethodName;
		compileShader(filename);
		createSetLayoutBindings();
		isInit = true;
	};
	void create(VkDevice device, VkAllocationCallbacks* allucationCallback) {
		if (!isInit)
			throw std::logic_error("is not initated");
		if (isCreated)
			throw std::logic_error("is already created");
		this->device = device;
		this->allucationCallback = allucationCallback;
		CreateShaderModul();
		isCreated = true;
	};
	void destroy() {
		if (isCreated) {
			vkDestroyShaderModule(device, shaderModule, allucationCallback);
			isCreated = false;
			device = VK_NULL_HANDLE;
		}
	};

	VkPipelineShaderStageCreateInfo getShaderStageCreateInfo() {
		if (!isCreated)
			throw std::logic_error("is not created");
		VkPipelineShaderStageCreateInfo shaderStageCreateInfo;
		shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageCreateInfo.pNext = nullptr;
		shaderStageCreateInfo.flags = 0;
		shaderStageCreateInfo.stage = shaderStage;		//die gefragteStage
		shaderStageCreateInfo.module = shaderModule;	//shaderModul
		shaderStageCreateInfo.pName = mainMethodName;	//funktionsName
		shaderStageCreateInfo.pSpecializationInfo = nullptr;
		return shaderStageCreateInfo;
	}

	std::vector<VkDescriptorSetLayoutBinding> getLayoutBindings() {
		return setLayoutBindings;
	}
};
#endif