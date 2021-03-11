#ifndef _MESHHELPER_
#define _MESHHELPER_

#include "Vertex.h"
#include <vector>

std::vector<Vertex> getQuadVertecies() {
	return {
		Vertex({ -0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}),
		Vertex({  0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}),
		Vertex({ -0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}),
		Vertex({  0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}),

		Vertex({ -0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}),
		Vertex({  0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}),
		Vertex({ -0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}),
		Vertex({  0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f})
	};
}
std::vector<uint32_t> getQuadIndicis() {
	return {
		1,0,2,
		3,0,1,

		2,0,4,
		2,4,6,
		
		3,1,7,
		7,1,5,

		7,0,3,
		4,0,7,

		1,2,5,
		2,6,5
	};
}


#endif // !_MESHHELPER_
