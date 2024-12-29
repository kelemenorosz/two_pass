#pragma once

#include <helpers.h>

#include <DirectXMath.h>

struct VertexInfo {

	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT2 texturePosition;
	DirectX::XMFLOAT3 color;

};

struct TetrahedronInfo {
    
    VertexInfo tetrahedronVertex[VERTEX_COUNT_TETRAHEDRON] = {
        //first face 0, 1, 2
        {
        DirectX::XMFLOAT3(1.0f, -1.0f, -1.0f),
        DirectX::XMFLOAT2(0.0f, 0.0f),
        DirectX::XMFLOAT3(1.0f, 1.0f, 0.0f)
        },
        {
        DirectX::XMFLOAT3(-1.0f,  1.0f, -1.0f),
        DirectX::XMFLOAT2(0.0f, 0.0f),
        DirectX::XMFLOAT3(1.0f, 0.0f, 1.0f)
        },
        {
        DirectX::XMFLOAT3(1.0f,  1.0f, -1.0f),
        DirectX::XMFLOAT2(0.0f, 0.0f),
        DirectX::XMFLOAT3(0.0f, 1.0f, 1.0f)
        },
        //second face 3, 4, 5
        {
        DirectX::XMFLOAT3(1.0f, -1.0f, -1.0f),
        DirectX::XMFLOAT2(0.0f, 0.0f),
        DirectX::XMFLOAT3(1.0f, 1.0f, 0.0f)
        },
        {
        DirectX::XMFLOAT3(-1.0f, -1.0f, 1.0f),
        DirectX::XMFLOAT2(0.0f, 0.0f),
        DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f)
        },
        {
        DirectX::XMFLOAT3(1.0f,  1.0f, -1.0f),
        DirectX::XMFLOAT2(0.0f, 0.0f),
        DirectX::XMFLOAT3(0.0f, 1.0f, 1.0f)
        },
        //third face 6, 7, 8
        {
        DirectX::XMFLOAT3(1.0f, -1.0f, -1.0f),
        DirectX::XMFLOAT2(0.0f, 0.0f),
        DirectX::XMFLOAT3(1.0f, 1.0f, 0.0f)
        },
        {
        DirectX::XMFLOAT3(-1.0f, -1.0f, 1.0f),
        DirectX::XMFLOAT2(0.0f, 0.0f),
        DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f)
        },
        {
        DirectX::XMFLOAT3(-1.0f,  1.0f, -1.0f),
        DirectX::XMFLOAT2(0.0f, 0.0f),
        DirectX::XMFLOAT3(1.0f, 0.0f, 1.0f)
        },
        //fourth face 9, 10, 11
        {
        DirectX::XMFLOAT3(-1.0f,  1.0f, -1.0f),
        DirectX::XMFLOAT2(0.0f, 0.0f),
        DirectX::XMFLOAT3(1.0f, 0.0f, 1.0f)
        },
        {
        DirectX::XMFLOAT3(1.0f,  1.0f, -1.0f),
        DirectX::XMFLOAT2(0.0f, 0.0f),
        DirectX::XMFLOAT3(0.0f, 1.0f, 1.0f)
        },
        {
        DirectX::XMFLOAT3(-1.0f, -1.0f, 1.0f),
        DirectX::XMFLOAT2(0.0f, 0.0f),
        DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f)
        }
    };
    WORD tetrahedronIndex[INDEX_COUNT_TETRAHEDRON] = {
        //first face
        0, 1, 2,
        //second face
        3, 5, 4,
        //third face
        7, 8, 6,
        //fourth face
        11, 10, 9
    };

};

struct CubeInfo {

    VertexInfo cubeVertex[VERTEX_COUNT_CUBE] = {
        //front face 0, 1, 2, 3
        {
            DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f),
            DirectX::XMFLOAT2(0.0f, 1.0f),
            DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f)
        },
        {
            DirectX::XMFLOAT3(-1.0f,  1.0f, -1.0f),
            DirectX::XMFLOAT2(0.0f, 0.0f),
            DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f)
        },
        {
            DirectX::XMFLOAT3(1.0f,  1.0f, -1.0f),
            DirectX::XMFLOAT2(1.0f, 0.0f),
            DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f)
        },
        {
            DirectX::XMFLOAT3(1.0f, -1.0f, -1.0f),
            DirectX::XMFLOAT2(1.0f, 1.0f),
            DirectX::XMFLOAT3(1.0f, 1.0f, 0.0f)
        },
        //right face 4, 5, 6, 7
        {
            DirectX::XMFLOAT3(1.0f, -1.0f, -1.0f),
            DirectX::XMFLOAT2(0.0f, 1.0f),
            DirectX::XMFLOAT3(1.0f, 1.0f, 0.0f)
        },
        {
            DirectX::XMFLOAT3(1.0f,  1.0f, -1.0f),
            DirectX::XMFLOAT2(0.0f, 0.0f),
            DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f)
        },
        {
            DirectX::XMFLOAT3(1.0f,  1.0f,  1.0f),
            DirectX::XMFLOAT2(1.0f, 0.0f),
            DirectX::XMFLOAT3(1.0f, 0.0f, 1.0f)
        },
        {
            DirectX::XMFLOAT3(1.0f, -1.0f,  1.0f),
            DirectX::XMFLOAT2(1.0f, 1.0f),
            DirectX::XMFLOAT3(0.0f, 1.0f, 1.0f)
        },
        //left face 8, 9, 10, 11
        {
            DirectX::XMFLOAT3(-1.0f, -1.0f, 1.0f),
            DirectX::XMFLOAT2(0.0f, 1.0f),
            DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f)
        },
        {
            DirectX::XMFLOAT3(-1.0f,  1.0f, 1.0f),
            DirectX::XMFLOAT2(0.0f, 0.0f),
            DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f)
        },
        {
            DirectX::XMFLOAT3(-1.0f,  1.0f,  -1.0f),
            DirectX::XMFLOAT2(1.0f, 0.0f),
            DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f)
        },
        {
            DirectX::XMFLOAT3(-1.0f, -1.0f,  -1.0f),
            DirectX::XMFLOAT2(1.0f, 1.0f),
            DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f)
        },
        //back face 12, 13, 14, 15
        {
            DirectX::XMFLOAT3(1.0f, -1.0f,  1.0f),
            DirectX::XMFLOAT2(0.0f, 1.0f),
            DirectX::XMFLOAT3(0.0f, 1.0f, 1.0f)
        },
        {
            DirectX::XMFLOAT3(1.0f,  1.0f,  1.0f),
            DirectX::XMFLOAT2(0.0f, 0.0f),
            DirectX::XMFLOAT3(1.0f, 0.0f, 1.0f)
        },
        {
            DirectX::XMFLOAT3(-1.0f,  1.0f, 1.0f),
            DirectX::XMFLOAT2(1.0f, 0.0f),
            DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f)
        },
        {
            DirectX::XMFLOAT3(-1.0f, -1.0f, 1.0f),
            DirectX::XMFLOAT2(1.0f, 1.0f),
            DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f)
        },
        //top face 16, 17, 18, 19
        {
            DirectX::XMFLOAT3(-1.0f,  1.0f,  -1.0f),
            DirectX::XMFLOAT2(0.0f, 1.0f),
            DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f)
        },
        {
            DirectX::XMFLOAT3(-1.0f,  1.0f, 1.0f),
            DirectX::XMFLOAT2(0.0f, 0.0f),
            DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f)
        },
        {
            DirectX::XMFLOAT3(1.0f,  1.0f,  1.0f),
            DirectX::XMFLOAT2(1.0f, 0.0f),
            DirectX::XMFLOAT3(1.0f, 0.0f, 1.0f)
        },
        {
            DirectX::XMFLOAT3(1.0f,  1.0f, -1.0f),
            DirectX::XMFLOAT2(1.0f, 1.0f),
            DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f)
        },
        //bottom face 20, 21, 22, 23
        {
            DirectX::XMFLOAT3(-1.0f, -1.0f, 1.0f),
            DirectX::XMFLOAT2(0.0f, 1.0f),
            DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f)
        },
        {
            DirectX::XMFLOAT3(-1.0f, -1.0f,  -1.0f),
            DirectX::XMFLOAT2(0.0f, 0.0f),
            DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f)
        },
        {
            DirectX::XMFLOAT3(1.0f, -1.0f, -1.0f),
            DirectX::XMFLOAT2(1.0f, 0.0f),
            DirectX::XMFLOAT3(1.0f, 1.0f, 0.0f)
        },
        {
            DirectX::XMFLOAT3(1.0f, -1.0f,  1.0f),
            DirectX::XMFLOAT2(1.0f, 1.0f),
            DirectX::XMFLOAT3(0.0f, 1.0f, 1.0f)
        }
    };
    WORD cubeIndex[INDEX_COUNT_CUBE] = {

        //front face
        0, 1, 2,
        0, 2, 3,
        //back face
        15,13,14,
        15,12,13,
        //right face
        4,5,6,
        4,6,7,
        //left face
        8,9,10,
        8,10,11,
        //top face
        16,17,18,
        16,18,19,
        //bottom face
        20,21,22,
        20,22,23

    };


};

class Objects {

public:

	static void PrepareBundleDraw(VertexInfo* mp_vertices, UINT64 mp_verticesLength, WORD* mp_indicies, UINT64 mp_indiciesLength,
		VertexInfo** p_vertices, UINT64& c_vertices, WORD** p_indicies, UINT64& c_indicies, UINT64 c_cubes, float* xShift, float* yShift, float* zShift);
    static void PrepareInstancedDraw(DirectX::XMFLOAT3** p_instances, UINT64& c_instances, uint8_t c_cubes, float* xShift, float* yShift, float* zShift);

private:

    static void shift(VertexInfo* p_vertices, UINT64 verticesLength, float xShift, float yShift, float zShift);
    static void indexShift(WORD* p_indicies, UINT64 indiciesLength, WORD iShift);

};

