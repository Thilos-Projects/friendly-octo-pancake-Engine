#ifndef _MESHHELPER_
#define _MESHHELPER_

#include "Vertex.h"
#include <vector>

std::vector<Vertex> getQuadVertecies() {
	return {
		Vertex({ -0.5f, -0.5f,  0.0f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}),
		Vertex({  0.5f,  0.5f,  0.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}),
		Vertex({ -0.5f,  0.5f,  0.0f}, {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}),
		Vertex({  0.5f, -0.5f,  0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 0.0f})
	};
}
std::vector<uint32_t> getQuadIndicis() {
	return {
		1,0,2,
		3,0,1
	};
}


#endif // !_MESHHELPER_
