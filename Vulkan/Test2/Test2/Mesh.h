#ifndef _MESH_
#define _MESH_

#include "Vertex.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "objLoader.h"
#include <vector>
#include <iostream>
#include <unordered_map>

class Mesh
{
private :
	std::vector<Vertex> verticies;
	std::vector<uint32_t> indices;

	VkBuffer vertexBuffer;
	VkDeviceMemory VertexMemory;

	VkDevice device;

public:

	Mesh() {}
	~Mesh() {}
	Mesh(const Mesh&) = delete;
	Mesh(Mesh&&) = delete;
	Mesh& operator=(const Mesh&) = delete;
	Mesh& operator=(Mesh&&) = delete;

	void load(const char *path) {
		tinyobj::attrib_t vertexAtributes;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warnString;
		std::string errorString;

		bool sucess = tinyobj::LoadObj(&vertexAtributes, &shapes, &materials, &warnString, &errorString, path);

		if (!sucess) {
			throw std::runtime_error(errorString);
		}

		std::unordered_map<Vertex, uint32_t> tempVertecies;

		for (tinyobj::shape_t shape : shapes) {
			for (tinyobj::index_t index : shape.mesh.indices) {
				glm::vec3 pos = 
				{
					vertexAtributes.vertices[3 * index.vertex_index + 0],
					vertexAtributes.vertices[3 * index.vertex_index + 2],
					vertexAtributes.vertices[3 * index.vertex_index + 1]
				};
				/*glm::vec3 color = 
				{
					0.0f,	//vertexAtributes.colors[3 * index.vertex_index + 0],
					1.0f,	//vertexAtributes.colors[3 * index.vertex_index + 1],
					0.0f	//vertexAtributes.colors[3 * index.vertex_index + 2]
				};
				glm::vec2 uvCoord = 
				{
					0,	//vertexAtributes.texcoords[2 * index.vertex_index + 0],
					0	//vertexAtributes.texcoords[2 * index.vertex_index + 1]
				};*/
				glm::vec3 normal =
				{
					vertexAtributes.normals[3 * index.normal_index + 0],
					vertexAtributes.normals[3 * index.normal_index + 2],
					vertexAtributes.normals[3 * index.normal_index + 1]
				};

				Vertex vert(pos/*, color, uvCoord*/, normal);

				if (tempVertecies.count(vert) == 0) {
					tempVertecies[vert] = tempVertecies.size();
					verticies.push_back(vert);
				}
				indices.push_back(tempVertecies[vert]);
			}
		}

	}
	void upload() {

	}
	void unload() {

	}

	//überarbeiten
	std::vector<Vertex> getVertecies() {
		return verticies;
	}
	std::vector<uint32_t> getIndices() {
		return indices;
	}

};

#endif _MESH_