#pragma once

#include <helpers.h>
#include <render.h>
#include <objects.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <DirectXMath.h>

class Game : public Render {

public:

	Game();
	void LoadContent() override;
	void OnRender() override;
	void OnUpdate(double time) override;
	void OnMouseWheel(float delta) override;
	~Game();

private:

	struct VerticesWrapper {
	
		VertexInfo* p_vertices;
		UINT64 c_vertices;
	
	};

	struct IndiciesWrapper {

		WORD* p_indicies;
		UINT64 c_indicies;

	};

	struct InstancesWrapper {

		DirectX::XMFLOAT3* p_instances;
		UINT64 c_instances;

	};

	Microsoft::WRL::ComPtr<ID3D12Device2> m_device;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_depthBuffer;
	bool m_contentLoaded { false };
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;
	DirectX::XMMATRIX m_modelMatrix;
	DirectX::XMMATRIX m_viewMatrix;
	DirectX::XMMATRIX m_projectionMatrix;
	uint64_t m_fenceValues[FRAME_COUNT] = {};
	float m_fov;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_textureBuffer;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_samplerDescriptorHeap;
	VerticesWrapper m_vertices;
	IndiciesWrapper m_indicies;
	InstancesWrapper m_instances;
	float m_angle;

	
	//Heap for vertex, index and instance buffers
	Microsoft::WRL::ComPtr<ID3D12Heap> m_bufferHeap;
	
	//Placed vertex, index and instance buffers
	Microsoft::WRL::ComPtr<ID3D12Resource> m_placedVertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_placedIndexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_placedInstanceBuffer;

	//Vertex, index and instance buffer view
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	D3D12_VERTEX_BUFFER_VIEW m_instanceBufferView;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

	//Descriptor heaps and views for mip-gen
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_mipGenSRVDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_mipGenUAVDescriptorHeap;

	void UpdateBufferResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> d3d12CommandList, ID3D12Resource** p_destinationResource, ID3D12Resource** p_intermediateResource,
		const void* bufferData, size_t numberOfElements, size_t sizeOfElements);
	void CreateDepthBuffer(uint64_t width, uint64_t height);
	void TransitionResource(Microsoft::WRL::ComPtr<ID3D12Resource> resource, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, 
		D3D12_RESOURCE_STATES firstState, D3D12_RESOURCE_STATES secondState);
	void ClearRTV(D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, FLOAT* color);
	void ClearDSV(D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);
	Microsoft::WRL::ComPtr<ID3D12Resource> LoadTexture(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, ID3D12Resource** p_intermediateResource);
	void CreateSRVDescriptorHeap();
	void CreateSampler();
	void PrepareBundleCubeDraw(uint8_t c_cubes, uint32_t* xShift, uint32_t* yShift, uint32_t* zShift);
	void PrepareInstancedCubeDraw(uint8_t c_cubes, float* xShift, float* yShift, float* zShift);
	void GenerateMips();

};