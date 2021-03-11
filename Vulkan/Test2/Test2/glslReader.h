#ifndef _GLSLREADER_
#define _GLSLREADER_

#include <string>
#include <sstream>
#include <vector>

namespace GLSLReader {

	struct directedDatatype {
		int8_t direction;
		int32_t binding;
		uint32_t size;
	};

	class glslReader {
	private:
		std::vector<const char*> supportedTypes = {
			"vec2",
			"vec3",
			"vec4",
			"uvec2",
			"uvec3",
			"uvec4",
			"mat2x2",
			"mat2x3",
			"mat2x4",
			"mat2",
			"mat3x2",
			"mat3x3",
			"mat3x4",
			"mat3",
			"mat4x2",
			"mat4x3",
			"mat4x4",
			"mat4",
			"sampler1D",
			"sampler2D",
			"sampler3D"
		};
		std::vector<VkFormat> supportedTypesSize = {
		};

		bool seekWord(std::istringstream& sStream, std::string word, bool moveBack = false, char stoppChar = '\n') {
			uint32_t currentPos = 0;
			uint32_t pointer = 0;
			while (sStream.peek() != -1 && sStream.peek() != stoppChar && pointer < word.size()) {
				char temp = sStream.peek();
				if (sStream.get() == word[pointer]) {
					pointer++;
				}
				else {
					pointer = 0;
				}
				currentPos++;
			}
			if (moveBack)
				if (pointer != word.size())
					sStream.seekg(-(int32_t)currentPos, sStream.cur);
				else
					sStream.seekg(-(int32_t)word.size(), sStream.cur);
			return pointer == word.size();

		}

		uint32_t readInt(std::istringstream& sStream) {
			uint32_t number = 0;
			while (sStream.peek() != -1 && !std::isdigit(sStream.peek())) sStream.get();
			while (sStream.peek() != -1 && std::isdigit(sStream.peek()))
			{
				number *= 10;
				number += sStream.get() - 48;
			}
			return number;
		}

	public:
		std::vector<directedDatatype> fill(std::vector<char> &code) {

			std::string codeString;
			for (int i = 0; i < code.size(); i++) {
				codeString += code[i];
			}
			std::istringstream sStream(codeString);

			std::vector<directedDatatype> allData;

			while (sStream.peek() != -1) {
				if (!seekWord(sStream, "layout", false, -1)) break;
				allData.push_back(directedDatatype());
				directedDatatype* data = &allData[allData.size() - 1];

				if (seekWord(sStream, "location", true, '\n') || seekWord(sStream, "binding", true, '\n')) {
					seekWord(sStream, "=", false, '\n');
					data->binding = readInt(sStream);
				}
				else if (seekWord(sStream, "push_constant", true, '\n')) {
					data->binding = -1;
				}

				if (seekWord(sStream, "out", true, '\n')) {
					data->direction = 1;
				}
				else if (seekWord(sStream, "in", true, '\n')) {
					data->direction = 2;
				}
				else if (seekWord(sStream, "uniform", true, '\n')) {
					data->direction = 0;
				}

				for (int i = 0; i < supportedTypes.size(); i++) {
					if (seekWord(sStream, supportedTypes[i], true, ';')) {

					}
				}
			}

			/*bool found = false;
			for (int i = 0; i < supportedTypes.size(); i++)
				if ((varriablePos = line.find(supportedTypes[i])) != std::string::npos) {
					int start = varriablePos + strlen(supportedTypes[i]) + 1;
					simpleDataTypes.push_back(simpleDataType(supportedTypes[i], line.substr(start, line.find_last_of(";") - start).c_str()));
					found = true;
					break;
				}

			if (!found) continue;*/

			return allData;
		}
	};

}

#endif