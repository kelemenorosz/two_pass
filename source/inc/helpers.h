#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <exception>
#include <DirectXMath.h>

#define FRAME_COUNT (3)
#define WINDOW_WIDTH (400)
#define WINDOW_HEIGHT (400) 
#define VERTEX_COUNT_CUBE (24)
#define INDEX_COUNT_CUBE (36)
#define VERTEX_COUNT_TETRAHEDRON (12)
#define INDEX_COUNT_TETRAHEDRON (12)

inline void ThrowIfFailed(HRESULT result) {

	if (FAILED(result)) throw std::exception();

}

inline void ThrowBadMemoryAllocation() {

	throw std::exception("Bad Memory Allocation.");

}

inline UINT RoundedConstantBufferSize(UINT byteSize) {

	return (byteSize + 255) & ~255;

}