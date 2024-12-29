#define WIN32_LEAN_AND_MEAN

#include <objects.h>

void Objects::PrepareBundleDraw(VertexInfo* mp_vertices, UINT64 mp_verticesLength, WORD* mp_indicies, UINT64 mp_indiciesLength, 
	VertexInfo** p_vertices, UINT64& c_vertices, WORD** p_indicies, UINT64& c_indicies, UINT64 c_cubes, float* xShift, float* yShift, float* zShift) {

	//Create vertex and index data for Cube objects
	c_vertices = c_cubes * mp_verticesLength;
	*p_vertices = (VertexInfo*) ::malloc(sizeof(VertexInfo) * c_vertices);
	if (*p_vertices == nullptr) ThrowBadMemoryAllocation();

	c_indicies = c_cubes * mp_indiciesLength;
	*p_indicies = (WORD*) ::malloc(sizeof(WORD) * c_indicies);
	if (*p_indicies == nullptr) ThrowBadMemoryAllocation();

	for (uint8_t i{ 0 }; i < c_cubes; i++) {

		::memcpy(*p_vertices + i * mp_verticesLength, mp_vertices, sizeof(VertexInfo) * mp_verticesLength);
		::memcpy(*p_indicies + i * mp_indiciesLength, mp_indicies, sizeof(WORD) * mp_indiciesLength);

		Objects::shift(*p_vertices + i * mp_verticesLength, mp_verticesLength, xShift[i], yShift[i], zShift[i]);
		Objects::indexShift(*p_indicies + i * mp_indiciesLength, mp_indiciesLength, i * mp_verticesLength);

	}

}

void Objects::PrepareInstancedDraw(DirectX::XMFLOAT3** p_instances, UINT64& c_instances, uint8_t c_cubes, float* xShift, float* yShift, float* zShift) {

	c_instances = c_cubes;
	*p_instances = (DirectX::XMFLOAT3*) ::malloc(sizeof(DirectX::XMFLOAT3) * c_instances);
	if (*p_instances == nullptr) ThrowBadMemoryAllocation();

	for (uint8_t i{ 0 }; i < c_cubes; i++) {

		(*p_instances)[i].x = xShift[i];
		(*p_instances)[i].y = yShift[i];
		(*p_instances)[i].z = zShift[i];

	}

}

void Objects::shift(VertexInfo* p_vertices, UINT64 verticesLength, float xShift, float yShift, float zShift) {

	VertexInfo* p_start = p_vertices;
	VertexInfo* p_end = p_start + verticesLength;

	for (; p_start < p_end; ++p_start) {
		p_start->position.x += xShift;
		p_start->position.y += yShift;
		p_start->position.z += zShift;
	}

}

void Objects::indexShift(WORD* p_indicies, UINT64 indiciesLength, WORD iShift) {

	WORD* p_start = p_indicies;
	WORD* p_end = p_start + indiciesLength;

	for (; p_start < p_end; ++p_start) {
		*p_start += iShift;
	}

}