#pragma once

#include <render.h>
#include <helpers.h>
#include <objects.h>

#include <d3d12.h>
#include <wrl.h>

class SphereRender : public Render {

public:

	SphereRender();
	~SphereRender();
	void LoadContent() override;
	void OnRender() override;
	void OnUpdate(double elapsedTime) override;
	void OnMouseWheel(float mouseWheelDelta) override;

private:

	//Wrapper for vertex, index and instance data
	struct VerticesWrapper {

		UINT64 c_vertices;
		VertexInfo* p_vertices;

	};

	struct IndiciesWrapper {

		UINT64 c_incidies;
		WORD* p_indicies;

	};

	struct InstancesWrapper {

		UINT64 c_instances;
		DirectX::XMFLOAT3* p_instances;

	};

	//D3D12 device
	Microsoft::WRL::ComPtr<ID3D12Device2> m_device;

	//Assembled vertex, index and instance data
	VerticesWrapper m_tetrahedronVertices;
	IndiciesWrapper m_tetrahedronIndicies;
	InstancesWrapper m_tetrahedronInstances;

	//Vertex, index and instance buffers
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_instanceBuffer;

	//Vertex, index and instance buffer views
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	D3D12_VERTEX_BUFFER_VIEW m_instanceBufferView;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

	//Root signature and pipeline state objects
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

	//Load functions
	void LoadIAStage();
	void CreateRootSignature();

};