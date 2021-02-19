#ifndef _VERTEX_
#define _VERTEX_

#include "NonDependingFunktions.h"
#include <vector>
#include <glm/gtx/hash.hpp>

class Vertex {
public:
	Vertex(glm::vec3 pos, glm::vec3 color, glm::vec2 uvCoord, glm::vec3 normal)
		:pos(pos), color(color), uvCoord(uvCoord), normal(normal) {}
	
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 uvCoord;
	glm::vec3 normal;

	bool operator==(const Vertex other) const {
		return pos == other.pos && color == other.color && uvCoord == other.uvCoord && normal == other.normal;
	}

	static VkVertexInputBindingDescription getBindingDescription(const uint32_t index = 0) {
		VkVertexInputBindingDescription toRet;

		toRet.binding = index;
		toRet.stride = sizeof(Vertex);
		toRet.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return toRet;
	}
	static std::vector<VkVertexInputAttributeDescription> getAttributDescriptions(const uint32_t index = 0) {
		std::vector<VkVertexInputAttributeDescription> toRet(4);

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

		toRet[3].location = 3;
		toRet[3].binding = index;
		toRet[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		toRet[3].offset = offsetof(Vertex, normal);

		return toRet;
	}
};

namespace std {
	template<> struct hash<Vertex>
	{
		size_t operator()(Vertex const &vert) const {
			size_t h0 = hash<glm::vec3>()(vert.pos);
			size_t h1 = hash<glm::vec3>()(vert.color);
			size_t h2 = hash<glm::vec2>()(vert.uvCoord);
			size_t h3 = hash<glm::vec2>()(vert.uvCoord);

			return (((h0 ^ (h1 << 1) >> 1) ^ h2) << 1) ^ h3;
		}
	};
}

#endif